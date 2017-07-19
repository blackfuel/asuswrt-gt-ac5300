/*
*  Copyright 2011, Broadcom Corporation
*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

//**************************************************************************
// File Name  : epon_usr.c
// This file mainly used to manager the resource for epon stack.
///1)Create Global timer for ALL EPON stack polling hanlde.
///2)Create MPCP queue and kernel thread.
///3)Create OAM queue and tasklet.
///4)Register PON MAC reset call back.
///5)Create netlink interface for event transmit to user space.
///6)Implement the MPCP and OAM packets transmit function for each link.
// Description: Broadcom EPON  Interface Driver
//               
//**************************************************************************
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <asm/uaccess.h>
#include <net/sock.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#include "Teknovus.h"
#include "bcm_OS_Deps.h"
#include <linux/bcm_log.h>
#include <rdpa_api.h>
#include <rdpa_mw_cpu_queue_ids.h>
#include "EponFrame.h"
#include "EponUser.h"
#include "EponTimer.h"
#include "Stream.h"
#include "OntmMpcp.h"
#include "board.h"
#include "OntDirector.h"
#include <rdp_cpu_ring.h>


typedef struct {
  wait_queue_head_t MpcpWaitQueue;
  int               rxMpcpDataAvl;
  struct task_struct *MpcpProcessTask;
} BCM_EponMpcpTaskState;  //current epon stack resource

static BCM_EponMpcpTaskState rxMpcpThread;
#define MpcpProcessTask (rxMpcpThread.MpcpProcessTask)
#define MpcpRxFlag (rxMpcpThread.rxMpcpDataAvl)
#define MpcpWaitQueue (rxMpcpThread.MpcpWaitQueue)

static struct net_device *oam_dev = NULL;


#define MPCP_CPU_RX_QUEUE_SIZE 64
#define MPCP_PACKET_SIZE 128
#define OAM_CPU_RX_QUEUE_SIZE 64

static struct tasklet_struct rxOAMTasklet;

static BOOL mpcpSim = TRUE;
static U16 mpcpoamToken[Token_Num] = 
					{MpcpOamTokenSize,
					MpcpOamTokenSize,
					MpcpOamTokenSize,
					MpcpOamTokenSize};


/* Socket buffer and buffer pointer for msg */
#define DYING_GASP_OAM_LEN  70
static struct sk_buff *dg_eponskbp = NULL;
static rdpa_cpu_tx_info_t dg_info = {};	
OuiVendor gOuiType = OuiTeknovus;
extern rdpa_epon_mode oam_mode;

Port protocolDest;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Mpcp traffic frame handler
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// \notify the Mpcp process task..
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
BOOL EponMpcpNotify(U8 *data,U32 length)
    {
    //No MPCP handle task. Directly return;
    if (MpcpProcessTask == NULL) 
        {
        return FALSE;
        }
	
    mpcpSim = FALSE;
    MpcpRxFlag=1;
    
    //only for MPCP simulation by shell.
    if (data)
        {
        mpcpSim = TRUE;
        memcpy(rxPktBuffer,data,length);
        }
    wake_up_interruptible(&MpcpWaitQueue);
	
    return TRUE;
    }


////////////////////////////////////////////////////////////////////////////////
static
void EponMpcpIsrHandle(long queue_id)
    {	
    rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);   
    EponMpcpNotify(NULL,0);    
    }

static 
BOOL EponMpcpReceive(void)
    {
    rdpa_cpu_rx_info_t info = {};
    int rc = 0;
    //Get MPCP Packets from RDPA module.
    while(1)
        {
    	rc = rdp_cpu_ring_get_packet(RDPA_MPCP_CPU_RX_QUEUE_ID, &info);
        if (rc )
            {	 
            return TRUE;
            }
        //need to check if it is MPCP packet.
        if (OntmMpcpPacketsCheck((U8 *)info.data + info.data_offset))
            {
            memcpy(rxPktBuffer,(U8 *)info.data + info.data_offset,info.size);
            HandleMpcp((U8 *)rxPktBuffer);                          
            }
        rdp_cpu_ring_free_mem(RDPA_MPCP_CPU_RX_QUEUE_ID, (void*)(info.data));
        
       if (mpcpoamToken[Mpcp_Rx_Token])
            {
            mpcpoamToken[Mpcp_Rx_Token]--;
            }
        //No Token;
        if (mpcpoamToken[Mpcp_Rx_Token] == 0)
            {
            return FALSE;
            }  
        }
    }


////////////////////////////////////////////////////////////////////////////////
static int EponMpcpProcressHandle(void * arg)
    {
    while ( 1 )
    {		
    wait_event_interruptible(MpcpWaitQueue, MpcpRxFlag);
    if (kthread_should_stop())
        {
        break;
        }
    
     /*
     * No more packets.  Indicate we are done (rxDataAvl=0) and
     * re-enable interrupts go to top of  loop to wait for more work.
     */
    local_bh_disable();
    if (EponMpcpReceive() == TRUE)
        {
        MpcpRxFlag = 0;
        rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);
        }
     local_bh_enable();
	 
    //MPCP simulatation 
    if (mpcpSim)
        {
        HandleMpcp((U8 *)rxPktBuffer);
        } 
    }

    return 0;
    }


////////////////////////////////////////////////////////////////////////////////
static bdmf_object_handle mpcp_obj = NULL;
static void* mpcp_kmem_context = NULL;

static 
void EponMpcpQueueOpen(void)
    {
    int rc;
    rdpa_cpu_rxq_cfg_t rxq_cfg;
    bdmf_object_handle system_obj = NULL;
    void *ring_base = NULL;
    RING_CB_FUNC ringCb;

    if (rdpa_system_get(&system_obj))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Failed to retrieve system object");
        goto unlock_exit;
        }

    if (rdpa_cpu_get(rdpa_cpu_host, &mpcp_obj))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Mpcp_obj create failed");
        goto unlock_exit;
        }
    
    rc = rdpa_cpu_rxq_cfg_get(mpcp_obj, RDPA_MPCP_CPU_RX_QUEUE_ID, &rxq_cfg);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "rdpa_cpu_rxq_cfg_get  failed");
        goto unlock_exit;
        }
	
    rxq_cfg.size = MPCP_CPU_RX_QUEUE_SIZE;
    rxq_cfg.isr_priv = RDPA_MPCP_CPU_RX_QUEUE_ID;
    rxq_cfg.rx_isr = EponMpcpIsrHandle;
    
    ringCb.data_dump = rdp_packet_dump;
#ifndef XRDP
    ringCb.databuf_alloc = rdp_databuf_alloc_cache;
    ringCb.databuf_free = rdp_databuf_free_cache;
#endif
    ringCb.buff_mem_context = kmem_cache_create("MpcpBufMem", 
		MPCP_PACKET_SIZE + RDPA_PKT_MIN_ALIGN, 0, SLAB_HWCACHE_ALIGN, NULL);
    mpcp_kmem_context = ringCb.buff_mem_context;
    printk("EponMpcpQueueOpen kmem_cache_create\n");

    rc = rdp_cpu_ring_create_ring(RDPA_MPCP_CPU_RX_QUEUE_ID, rdpa_ring_data,
        MPCP_CPU_RX_QUEUE_SIZE, (bdmf_phys_addr_t *)&ring_base, MPCP_PACKET_SIZE, &ringCb);
    if (rc < 0)
       goto unlock_exit;

    rxq_cfg.ring_head = ring_base;
    rxq_cfg.rx_dump_data_cb = rdp_cpu_dump_data_cb;
    rxq_cfg.rxq_stat = rdp_cpu_rxq_stat_cb;
    rc = rdpa_cpu_rxq_cfg_set(mpcp_obj, RDPA_MPCP_CPU_RX_QUEUE_ID, &rxq_cfg);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Cannot configure CPU Rx queue (%d)", rc);
        goto unlock_exit;
        }
    
    // map a trap reason from Runner to CPU. You would like to map only MPCP ethertype to this queue
    // since the MPCP Ethernet type is not preconfigured in the RDPA system, let's use a user defined Ethernet type filter
#ifdef CONFIG_BCM96858
    /* Map reason UDEF_2 to special TC, and TC to RDPA_MPCP_CPU_RX_QUEUE_ID.
       For simplicity, use TC = RDPA_MPCP_CPU_RX_QUEUE_ID */
    rc = rdpa_system_cpu_reason_to_tc_set(system_obj, rdpa_cpu_rx_reason_etype_udef_2, RDPA_MPCP_CPU_RX_QUEUE_ID);
    rc = rc ? rc : rdpa_cpu_tc_to_rxq_set(mpcp_obj, RDPA_MPCP_CPU_RX_QUEUE_ID, RDPA_MPCP_CPU_RX_QUEUE_ID);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuraing TC to CPU reason and queue", rc);
        goto unlock_exit;
        }	
#else
        {
        rdpa_cpu_reason_cfg_t reason_cfg = {};
        rdpa_cpu_reason_index_t cpu_reason = {};

        cpu_reason.reason = rdpa_cpu_rx_reason_etype_udef_2;
        cpu_reason.dir = rdpa_dir_ds;
        reason_cfg.queue = RDPA_MPCP_CPU_RX_QUEUE_ID; 
        reason_cfg.meter = BDMF_INDEX_UNASSIGNED; 
        reason_cfg.meter_ports = 0;
        rc	= rdpa_cpu_reason_cfg_set(mpcp_obj, &cpu_reason, &reason_cfg);
        if (rc < 0)
            {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuraing CPU reason to queue", rc);
            goto unlock_exit;
            }	
        }


#endif 

    //create MPCP kernel therad
    MpcpProcessTask = kthread_create(EponMpcpProcressHandle, NULL, "EponMPCP");
    if (IS_ERR(MpcpProcessTask)) 
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Mpcp Thread Creation failed");
        goto unlock_exit;
        }	
    wake_up_process(MpcpProcessTask);
    rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID); 
	
unlock_exit:
    if (system_obj)
        bdmf_put(system_obj);
    return ;
    }


////////////////////////////////////////////////////////////////////////////////
void EponMpcpPathInit(void)
    { 
    init_waitqueue_head(&MpcpWaitQueue);
    MpcpRxFlag = 0;
    MpcpProcessTask = NULL;
    
    EponMpcpQueueOpen();
    }


////////////////////////////////////////////////////////////////////////////////
static 
void EponMpcpQueueClose(void)
    {
    rdpa_cpu_rxq_cfg_t rxq_cfg = {};
    if (mpcp_obj)
        {
        rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);
        rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_MPCP_CPU_RX_QUEUE_ID);
        rxq_cfg.size = 0;
        rxq_cfg.isr_priv = RDPA_MPCP_CPU_RX_QUEUE_ID;
        rxq_cfg.rx_isr = NULL;
        rxq_cfg.ring_head = NULL;
        rdpa_cpu_rxq_cfg_set(mpcp_obj, RDPA_MPCP_CPU_RX_QUEUE_ID, &rxq_cfg);
		
        kmem_cache_destroy(mpcp_kmem_context);
        bdmf_put(mpcp_obj);
        }
    }


////////////////////////////////////////////////////////////////////////////////
void EponMpcpPathExit(void)
    {
    if (MpcpProcessTask != NULL)
        {
        MpcpRxFlag = 1;
        kthread_stop(MpcpProcessTask);
        MpcpProcessTask = NULL;
        }
    
    EponMpcpQueueClose();
    }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// OAM and Data traffic frame handler
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static bdmf_object_handle oam_obj = NULL;

////////////////////////////////////////////////////////////////////////////////
/// SetReservedOamFlag:  set reserved flags for our external CPU mode
/// Reserved flags(upper 8 bits):
///  --------------------------------------------------------------------------
///  |      LLID          | ifType |direction|    Reserved (don't modify)     |
///  --------------------------------------------------------------------------
///         (7-4)           (3-2)      (1-0)
/// Parameters:
/// \param source   Link and Physical Interface on ONU
/// \param flags    Pointer to where the OAM flags should be stored
///
/// \return
///  new flags value
////////////////////////////////////////////////////////////////////////////////
static
void EponOamSetFlag (Port port, U8 *data)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;
    OamMsg *msg = (OamMsg *)(ethHeader+1);
    U8 reserved_flag = 0;
    U16 newFlags = EPON_NTOHS(msg->flags);

    reserved_flag = ((port.pift == PortIfPon) ? 
                     (OnuHostIfEpon << OamFlagSrcIfShift) : 
                     (OnuHostIfUni << OamFlagSrcIfShift)) |
                     (port.inst << OamFlagLinkShift);

    newFlags = ((newFlags & OamFlagMask) | 
                    (reserved_flag << OamReservedFlagShift));
    msg->flags = EPON_NTOHS(newFlags);
    } // SetReservedOamFlag


////////////////////////////////////////////////////////////////////////////////
static 
void EponOamGetFlag (Port *port, U8 *data)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;
    OamMsg *msg = (OamMsg *)(ethHeader+1);
    U16 flags = EPON_NTOHS(msg->flags);
    U8 reserved_flag = (U8)((flags & OamReservedFlagMask) >> OamReservedFlagShift);

    switch ((reserved_flag & OamFlagSrcIfMask) >> OamFlagSrcIfShift)
        {
        case OnuHostIfEpon:
            port->pift =  PortIfPon;
            break;
        case OnuHostIfUni:
            port->pift =  PortIfUni;
            break;
        default :
            port->pift =  PortIfNone;
            break;
        }

    port->inst = ((reserved_flag & OamFlagLinkMask) >> OamFlagLinkShift);
    } // SetReservedOamFlag


////////////////////////////////////////////////////////////////////////////////
/// \clear the reserved field in flag.
/// \The reserved field is filled with link information. So need to clear it.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void EponOamClearLinkFlag (U8 *data)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;
    OamMsg *msg = (OamMsg *)(ethHeader+1);
    U16 newFlags = EPON_NTOHS(msg->flags);

    newFlags &= (~OamReservedFlagMask); 
    msg->flags = EPON_NTOHS(newFlags); 
    } // SetReservedOamFlag

////////////////////////////////////////////////////////////////////////////////	
int EponUsrLinkSend(U8 link, U8 *pbuf, U32 size, BOOL skb)
    {	
    rdpa_cpu_tx_info_t info = {};
    int rc;
    
    info.method = rdpa_cpu_tx_port;
    info.port = rdpa_if_wan0;
    info.cpu_port = rdpa_cpu_host;

#if defined(CONFIG_BCM96858)
    if (oam_mode == rdpa_epon_ctc)
        {
        info.x.wan.queue_id = 101+link;
        }
    else
        {
        info.x.wan.queue_id = 101;
        }
#else
        info.x.wan.queue_id = 101+link;
#endif
    info.x.wan.flow = OntUserGetLinkFlow(link, info.x.wan.queue_id);
    info.data_size = size;
    
    if (PonMgrLos())
        {
        eponUsrDebug(DebugEponUsr,("Pon Los should Not send packet to link:%d\n",link));
		
        if (skb)
            bdmf_sysb_free((bdmf_sysb )pbuf);
        
        return -1;
        }
    
    if (skb)
        rc = rdpa_cpu_send_sysb((bdmf_sysb *)pbuf, &info); 
    else
        {
        rc = rdpa_cpu_send_raw(pbuf, size, &info);
        }

    //printk("EponUsrLinkSend %s on link %d rc: %d\n", skb?"skb":"raw", link, rc);
    return rc;
    }

////////////////////////////////////////////////////////////////////////////////
static 
BOOL OamSlowProtocolPacketsCheck(U8 *data)
    {
    U8 vlanNum = 0;
     return (OntmOamPacketsCheck(data, &vlanNum) &&
            ((data[14 + (U8)(vlanNum*4)] == OamSlowProtocolSubtype) ||
            (data[14 + (U8)(vlanNum*4)] == Oam8021asProtocolSubtype)));
    }

////////////////////////////////////////////////////////////////////////////////
static 
BOOL EapolPacketsCheck(U8 *data, U8 *vlanNum)
    {
    EthernetFrame *ethHeader;
    *vlanNum = 0;
    while(1)
        {
        ethHeader = (EthernetFrame*)(data + (*vlanNum)*4);
        if((htons(ethHeader->type) == 0x8100) || 
            (htons(ethHeader->type) == 0x88a8))
            {
            (*vlanNum)++;
            }
        else if(htons(ethHeader->type) == 0x888e)
            {
            return TRUE;
            }
        else
            {
            return FALSE;
            }
        }
    }

////////////////////////////////////////////////////////////////////////////////
static 
BOOL EapTlsPacketsCheck(U8 *data, int len)
    {
    U8 vlanNum = 0;
    return (EapolPacketsCheck(data, &vlanNum) &&
            len >= 24 &&
            data[15 + (U8)(vlanNum*4)] == 0 && //EAPOL type
            data[22 + (U8)(vlanNum*4)] == 0x0D); //TLS type
    }

static 
BOOL OamInfoPacketsCheck(U8 *data)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;
    OamMsg *msg = (OamMsg *)(ethHeader+1);
    return (msg->opcode == OamOpcodeInfo);
    }


static 
void EponBcmOamQueueEmptyCheck(LinkIndex link)
    {
    U16 oamQueueIndex = OntDirLinkOamQGet(link);
    
    if (PonMgrReportModeGet() == RptModeFrameAligned)
        {
        if (!EponL1QueueEmptyCheck(oamQueueIndex))
            {
            PonMgrResetLinkUpPath(1UL << link);
            }
        }
    }

////////////////////////////////////////////////////////////////////////////////
void EponUsrPacketsSend(Port port,U16 size,U8 *pbuf)
    {	
    U32 length = size;
    switch (port.pift)
        {
        case PortIfPon:
            EponUsrLinkSend(port.inst,pbuf,length,FALSE);
            break;
        default:
            break;
        }	
    }

////////////////////////////////////////////////////////////////////////////////
/// \notify the Oam process task..
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponOamNotify(void)
    {
    if (rxOAMTasklet.func != NULL)
        tasklet_schedule(&rxOAMTasklet);
    }

static int EponOamHandle(rdpa_cpu_rx_info_t *info)
    {
        struct sk_buff _sysb, *sysb = &_sysb;
        int len;
        struct net_device_stats *vstats; 
        U8 vlanNum;
        U8 *dataBuf;
        
        /*allocate SKB*/
        sysb = skb_header_alloc();
        if (!sysb)
            {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "SKB Allocation failure");
            return -1;
            }
        
        /* initialize the skb */
        skb_headerinit((BCM_PKT_HEADROOM + info->data_offset),
#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
                       SKB_DATA_ALIGN(info->size + BCM_SKB_TAILROOM),
#else
                       BCM_MAX_PKT_LEN,
#endif
                       sysb, (uint8_t *)(info->data) + info->data_offset, bdmf_sysb_recycle, 0, NULL);
        
        skb_trim(sysb, info->size);
        sysb->recycle_flags &= SKB_NO_RECYCLE; /* no skb recycle,just do data recyle */
        
#if defined(CC_NBUFF_FLUSH_OPTIMIZATION)
        /* Set dirty pointer to optimize cache invalidate */
        skb_shinfo((struct sk_buff *)(sysb))->dirty_p = sysb->data + ETH_HLEN;
#endif
        
        dataBuf = (U8 *) bdmf_sysb_data((bdmf_sysb) sysb);
        len = bdmf_sysb_length((bdmf_sysb) sysb);
        
        DumpData(DebugEponUsr, "EponOamTaskletHandle", dataBuf, len);
        
        if (oam_dev != NULL)
            {
            vstats = &(oam_dev->stats);
            
            if (OamSlowProtocolPacketsCheck(dataBuf) || EapTlsPacketsCheck(dataBuf, len))
                {
                if (OntmOamPacketsCheck(dataBuf, &vlanNum))
                    {
                    Port port;
                    port.inst = info->reason_data;
                    port.pift = PortIfPon;
                    
                    EponOamSetFlag(port, dataBuf);
                    }
                vstats->rx_packets++;
                vstats->rx_bytes += len;
                
                sysb->protocol = eth_type_trans(sysb, oam_dev);
                sysb->dev = oam_dev;
                
                netif_receive_skb(sysb);
                }
            /* Fix Me!!!!EAP handle need change */
#ifndef CONFIG_EPON_10G_SUPPORT 
            else if (OntmOamPacketsCheck(dataBuf, &vlanNum) || EapolPacketsCheck(dataBuf, &vlanNum))
                {
                struct net_device *dev = NULL;
                
                dev = dev_get_by_name(&init_net, "epon0");
                if (dev == NULL)
                    {
                    vstats->rx_dropped++; //temporary put into oam_dev drop stats
                    bdmf_sysb_free((bdmf_sysb) sysb); //release SKB.
                    }
                else
                    {
                    vstats = &(dev->stats);
                    vstats->rx_packets++;
                    vstats->rx_bytes += len;
                    
                    sysb->protocol = eth_type_trans(sysb, dev);
                    sysb->dev = dev;
                    netif_receive_skb(sysb);
                    }
                }
#endif
            else //not oam packets directly drop.
                {
                vstats->rx_dropped++;
                bdmf_sysb_free((bdmf_sysb) sysb); //release SKB.
                }
            }
 
        if (mpcpoamToken[Oam_Rx_Token])
            {
            mpcpoamToken[Oam_Rx_Token]--;
            }
        
        //No Token;
        if (mpcpoamToken[Oam_Rx_Token] == 0)
            {
                return -1;
            }

        return 0;
    }

////////////////////////////////////////////////////////////////////////////////
static 
void EponOamTaskletHandle(unsigned long data)
    {
    rdpa_cpu_rx_info_t info = {};
    int rc = 0;

    /* clear the interrupt here */
    rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
    while (1)
        {
        rc = rdpa_cpu_packet_get(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID, &info);
        if (rc)
            break;

        rc = EponOamHandle(&info);
        if (rc)
            break;
        }
    //Check Token.
    if (mpcpoamToken[Oam_Rx_Token] > 0)
        {
        rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
        }
    }


////////////////////////////////////////////////////////////////////////////////
static
void EponOamIsrHandle(long queue_id)
    {
    rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
    tasklet_schedule(&rxOAMTasklet);
    }

////////////////////////////////////////////////////////////////////////////////
static 
void EponCaptureEapolToOamQueue(void)
    {
    int rc;

#ifndef CONFIG_BCM96858
    rdpa_cpu_reason_cfg_t reason_cfg;
    rdpa_cpu_reason_index_t cpu_reason;
    
    if(oam_obj == NULL)
        {
        if (rdpa_cpu_get(rdpa_cpu_host, &oam_obj))
            {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Failed to retrieve oam object");
            return;
            }
        }
    
    cpu_reason.reason = rdpa_cpu_rx_reason_etype_udef_0;
    cpu_reason.dir = rdpa_dir_ds;
    reason_cfg.queue = RDPA_OAM_CPU_RX_QUEUE_ID; 
    reason_cfg.meter = BDMF_INDEX_UNASSIGNED; 
    reason_cfg.meter_ports = 0;
    rc  = rdpa_cpu_reason_cfg_set(oam_obj, &cpu_reason, &reason_cfg);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuring CPU reason to queue", rc);
        return;
        }
#else
    bdmf_object_handle system_obj = NULL;
    
    if(oam_obj == NULL)
        {
        if (rdpa_cpu_get(rdpa_cpu_host, &oam_obj))
            {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Failed to retrieve oam object");
            return;
            }
        }
    
    if (rdpa_system_get(&system_obj))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Failed to retrieve system object");
        return;
        }
    /* Map reason UDEF_0 to special TC, and TC to RDPA_OAM_CPU_RX_QUEUE_ID.
       For simplicity, use TC = RDPA_OAM_CPU_RX_QUEUE_ID */
    rc = rdpa_system_cpu_reason_to_tc_set(system_obj, rdpa_cpu_rx_reason_etype_udef_0, RDPA_OAM_CPU_RX_QUEUE_ID);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuring TC to CPU reason and queue", rc);
        bdmf_put(system_obj);
        return;
        }
    
    rc = rc ? rc : rdpa_cpu_tc_to_rxq_set(oam_obj, RDPA_OAM_CPU_RX_QUEUE_ID, RDPA_OAM_CPU_RX_QUEUE_ID);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuring TC to CPU queue", rc);
        }
    bdmf_put(system_obj);
#endif 
    }

////////////////////////////////////////////////////////////////////////////////
static 
void EponOamQueueOpen(void)
    {	
    int rc;
    rdpa_cpu_rxq_cfg_t rxq_cfg;
    bdmf_object_handle system_obj = NULL;

    if (rdpa_system_get(&system_obj))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Failed to retrieve system object");
        goto unlock_exit;
        }

	
    if (rdpa_cpu_get(rdpa_cpu_host, &oam_obj))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "oam_obj create failed");
        goto unlock_exit;
        }
    
    rc = rdpa_cpu_rxq_cfg_get(oam_obj, RDPA_OAM_CPU_RX_QUEUE_ID, &rxq_cfg);
    if (rc < 0)
    	{
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "rdpa_cpu_rxq_cfg_get failed");
    	goto unlock_exit;
    	}
    rxq_cfg.size = OAM_CPU_RX_QUEUE_SIZE;
    rxq_cfg.isr_priv = RDPA_OAM_CPU_RX_QUEUE_ID;
    rxq_cfg.rx_isr = EponOamIsrHandle;
    
    rc = rdpa_cpu_rxq_cfg_set(oam_obj, RDPA_OAM_CPU_RX_QUEUE_ID, &rxq_cfg);
    if (rc < 0)
    	{
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, 
			"Cannot configure CPU Rx queue (%d)", rc);
    	goto unlock_exit;
    	}
    
    
    // map a trap reason from Runner to CPU. You would like to map only MPCP ethertype to this queue
    // since the MPCP Ethernet type is not preconfigured in the RDPA system, let's use a user defined Ethernet type filter
    
#ifdef CONFIG_BCM96858
    /* Map reason UDEF_1 to special TC, and TC to RDPA_OAM_CPU_RX_QUEUE_ID.
       For simplicity, use TC = RDPA_OAM_CPU_RX_QUEUE_ID */
    rc = rdpa_system_cpu_reason_to_tc_set(system_obj, rdpa_cpu_rx_reason_etype_udef_1, RDPA_OAM_CPU_RX_QUEUE_ID);
    rc = rc ? rc : rdpa_cpu_tc_to_rxq_set(oam_obj, RDPA_OAM_CPU_RX_QUEUE_ID, RDPA_OAM_CPU_RX_QUEUE_ID);
    if (rc < 0)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Error (%d) configuraing TC to CPU reason and queue", rc);
        goto unlock_exit;
        }	
#else
        {
        rdpa_cpu_reason_cfg_t reason_cfg;
        rdpa_cpu_reason_index_t cpu_reason;

        cpu_reason.reason = rdpa_cpu_rx_reason_etype_udef_1;//
        cpu_reason.dir = rdpa_dir_ds;
        reason_cfg.queue = RDPA_OAM_CPU_RX_QUEUE_ID; 
        reason_cfg.meter = BDMF_INDEX_UNASSIGNED; 
        reason_cfg.meter_ports = 0;
        rc	= rdpa_cpu_reason_cfg_set(oam_obj, &cpu_reason, &reason_cfg);
        if (rc < 0)
            {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, 
                "Error (%d) configuraing CPU reason to queue", rc);
            goto unlock_exit;
            }	
        }
#endif 

    rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);

    BCM_LOG_INFO(BCM_LOG_ID_EPON, "EPON OAM data path open");


unlock_exit:
    if (system_obj)
        bdmf_put(system_obj);
    return ;
    }


/* --------------------------------------------------------------------------
-------------------------------------------------------------------------- */
static int OamEnetOpen(struct net_device * dev)
{
    netif_carrier_on(dev);
    netif_start_queue(dev);

    return 0;
}


static int OamEnetClose(struct net_device * dev)
    {
    netif_stop_queue(dev);
    netif_carrier_off(dev);

    return 0;
    }


static int OamEnetXmit(struct sk_buff *skb, struct net_device *dev)
    {
    U8 *data;
    bdmf_sysb sysb;
    Port port;
    U8 vlanNum;
    int rc = -1;
    struct net_device_stats *vstats = &(dev->stats); 
    
    sysb = (bdmf_sysb )skb;
    data = (U8 *)bdmf_sysb_data(sysb);
    
    DumpData(DebugEponUsr,"EponUsrOamSend", data, bdmf_sysb_length(sysb));
    
    if (OntmOamPacketsCheck(data, &vlanNum))
        {
        EponOamGetFlag(&port,data);
        if ((port.pift == PortIfPon) && 
			(OntmMpcpLinkStateGet(port.inst) == rdpa_epon_link_in_service))
            {
            if (OamInfoPacketsCheck(data))
                {
                EponBcmOamQueueEmptyCheck(port.inst);
                }
            
            EponOamClearLinkFlag(data);
            rc = EponUsrLinkSend(port.inst, (U8*)skb, bdmf_sysb_length(sysb), TRUE);
            }
        else
            {
            goto free_exit;
            }
        }
    else if (EapolPacketsCheck(data, &vlanNum))
        {
        rc = EponUsrLinkSend(0, (U8*)skb, bdmf_sysb_length(sysb), TRUE);
        }
    else
    	{
    	goto free_exit;
    	}
	
    if (rc == 0)
    	{//tranmission done
    	vstats->tx_packets ++;
    	vstats->tx_bytes += bdmf_sysb_length(sysb);
    	}
    else
    	{
    	vstats->tx_errors ++;
    	}
	
    return rc;

free_exit:
    bdmf_sysb_free(sysb);
    vstats->tx_dropped ++;
    
    return rc; 
    }

static int OamEnetSetMacAddr(struct net_device *dev, void *p)
   {
    struct sockaddr *addr = p;

    if(netif_running(dev))
        return -EBUSY;

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    return 0;
}


static const struct net_device_ops oam_netdev_ops = {
    .ndo_open   = OamEnetOpen,
    .ndo_stop   = OamEnetClose,
    .ndo_start_xmit   = (HardStartXmitFuncP)OamEnetXmit,
    .ndo_set_mac_address  = OamEnetSetMacAddr,
    };


void EponCreateOamVport(void)
    {
    struct net_device *dev;
    U8 macAddr[ETH_ALEN];
	
    /* Verify that no interface exists with this name */
    dev = dev_get_by_name(&init_net, EPON_OAM_IF_NAME);
    if (dev != NULL) {
        dev_put(dev);
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, 
			"The given interface %s already exists", EPON_OAM_IF_NAME);
        return;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(0)) == NULL) {
        return;
    }

    /* Assign name to the i/f */
    strcpy(dev->name, EPON_OAM_IF_NAME);
    dev_alloc_name(dev, dev->name);

    SET_MODULE_OWNER(dev);

    dev->netdev_ops = &oam_netdev_ops;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor = free_netdev;

    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    register_netdev(dev);
	
    kerSysGetMacAddress(macAddr, 0);
    memmove(dev->dev_addr, macAddr, ETH_ALEN);
	
    oam_dev = dev;
    }

void EponDeleteOamVport(void)
    {
    struct net_device *dev = oam_dev;
	
    if (oam_dev == NULL)
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, 
			"No %s device", EPON_OAM_IF_NAME);
        return;
        }

    /* */
    synchronize_net();

    /* Delete the given epon virtual interfaces. No need to call free_netdev
       after this as dev->destructor is set to free_netdev */
    unregister_netdev(dev);
    oam_dev = NULL;
    }


////////////////////////////////////////////////////////////////////////////////
void EponOamPathInit(void)
    {
    //init tasklet
    tasklet_init(&rxOAMTasklet, (void *)EponOamTaskletHandle, 0);

    EponOamQueueOpen();
    EponCaptureEapolToOamQueue();
    EponCreateOamVport();
    }


////////////////////////////////////////////////////////////////////////////////
static 
void EponOamQueueClose(void)
    {

    rdpa_cpu_rxq_cfg_t rxq_cfg = {};

    if (oam_obj)
        {
        rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
        rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_OAM_CPU_RX_QUEUE_ID);
        rxq_cfg.size = 0;
        rxq_cfg.isr_priv = RDPA_OAM_CPU_RX_QUEUE_ID;
        rxq_cfg.rx_isr = NULL;		
        rdpa_cpu_rxq_cfg_set(oam_obj, RDPA_OAM_CPU_RX_QUEUE_ID, &rxq_cfg);
        bdmf_put(oam_obj);
        oam_obj = NULL;
        }
    }


////////////////////////////////////////////////////////////////////////////////
void EponOamPathExit(void)
    {
    tasklet_kill(&rxOAMTasklet);
    EponOamQueueClose();
    EponDeleteOamVport();
    }


static
void EponFrameTokenTimer (EponTimerModuleId moduleId)
    {
    BOOL tokenZero = FALSE;
    if (mpcpoamToken[Mpcp_Rx_Token] < (MpcpOamTokenSize-MpcpOamToKenRate))
        {			
        tokenZero = (mpcpoamToken[Mpcp_Rx_Token] > 0) ? FALSE:TRUE;
        mpcpoamToken[Mpcp_Rx_Token] += MpcpOamToKenRate;
        if (tokenZero)
            {
            //Wakeup MPCP task again.
            EponMpcpNotify(NULL,0); 
            }
        }
    
    if (mpcpoamToken[Oam_Rx_Token] < (MpcpOamTokenSize-MpcpOamToKenRate))
        {
        tokenZero = (mpcpoamToken[Oam_Rx_Token] > 0) ? FALSE:TRUE;
        mpcpoamToken[Oam_Rx_Token] += MpcpOamToKenRate;
        if (tokenZero)
            {
            EponOamNotify();
            }
        }
    } // OntmMpcpTimeOutTimer


void EponFrameTaskTimerReg(void)
    {
    EponUsrModuleTimerRegister(EPON_TIMER_TO1, 
    			MpcpOamTokenTimer,EponFrameTokenTimer);
    }


void EponDyingGaspHandler(void)
    {
    BOOL rc;
    
    /* send alarm OAM to OLT directly */
    if (dg_eponskbp == NULL) 
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "No DG skb to send");
        return; 
        }
    
    /* Copy src MAC from dev into dying gasp packet */	
    memcpy(dg_eponskbp->data + ETH_ALEN, PonCfgDbGetBaseMacAddr(), ETH_ALEN);
    
    /* Transmit dying gasp packet */
#ifdef CONFIG_BCM96858
    rc = rdpa_cpu_send_sysb((bdmf_sysb *)dg_eponskbp, &dg_info);
#else
    rc = rdpa_cpu_send_epon_dying_gasp((bdmf_sysb)dg_eponskbp, &dg_info);
#endif
    BCM_LOG_INFO(BCM_LOG_ID_EPON, "EponTrafficSend dyinggasp %s", !rc ? "Done" : "Failed");
    
    dg_eponskbp = NULL;
    
    /* notify the userspack OAM stack, then OAM stack do further action */
    OsAstMsgQSet(OsAstAppBase, 0, 0);
    }


//Dpoe has its own Dying gasp frame
//Tek and Ctc use Tek frame

void EponDyingGaspPayLoad(OuiVendor OuiType)
    {
    unsigned char dg_Tek_eponOam_frame[DYING_GASP_OAM_LEN ] = {
		    0x88, 0x09, 0x3, 0x0, 0x52, 0x01, 0x0, 0xc, 
		    0xFE, 0x0E, 0x0, 0x0D, 0xB6, 
		    0x41, 0x1};
    
    unsigned char dg_Dpoe_eponOam_frame[DYING_GASP_OAM_LEN ] = {
		    0x88, 0x09, 0x03, 0x0, 0x52, 0x01, 0x0, 0x01, 
		    0xFE, 0x0B, 0x0, 0x10, 0x0, 
		    0x41, 0x01, 0x0, 0x02};
			
    //fill with payload
    if (OuiType==OuiDpoe)
        memcpy(&(dg_eponskbp->data[12]), dg_Dpoe_eponOam_frame, (DYING_GASP_OAM_LEN-12)); 
    else
        memcpy(&(dg_eponskbp->data[12]), dg_Tek_eponOam_frame, (DYING_GASP_OAM_LEN-12)); 
    }


void EponDyingGaspAlloc(void)
    {
    /* Set up dying gasp buffer from packet transmit when we power down */
    if (dg_eponskbp == NULL)
        {//make sure the skb doesn't exist before allocation
    	dg_eponskbp = alloc_skb(DYING_GASP_OAM_LEN, GFP_KERNEL);
    	}
	
    if (dg_eponskbp)
        {    
        if (oam_dev == NULL) 
	    {
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "Can't find oam net device");
            /* free the skb */
            bdmf_sysb_free((bdmf_sysb )dg_eponskbp);
            dg_eponskbp = NULL;
            return;
            }
    
        dg_eponskbp->dev = oam_dev;
        memset(dg_eponskbp->data, 0, DYING_GASP_OAM_LEN); 
        dg_eponskbp->len = DYING_GASP_OAM_LEN;
    	
        //fill with DA MAC
        dg_eponskbp->data[0] = 0x01;
        dg_eponskbp->data[1] = 0x80;
        dg_eponskbp->data[2] = 0xC2;
        dg_eponskbp->data[3] = 0x0;
        dg_eponskbp->data[4] = 0x0;
        dg_eponskbp->data[5] = 0x02;
    	
        //fill with SA MAC
        memcpy(&(dg_eponskbp->data[6]), (U8*)PonCfgDbGetBaseMacAddr(), sizeof(MacAddr)); 

	EponDyingGaspPayLoad(gOuiType);
        }
	
    dg_info.method = rdpa_cpu_tx_port;
    dg_info.port = rdpa_if_wan0;
    dg_info.cpu_port = rdpa_cpu_host;
    dg_info.x.wan.flow = 0; 
    dg_info.x.wan.queue_id = 101; 
    dg_info.no_lock = 1;
    }

static OuiVendor EponDyingGaspOuiTran(U8 oamSel)
    {
    U8 i;

    // dpoe has different header
    if (oamSel & (0x1<<OuiDpoe))
        return OuiDpoe;
	
    for (i = 0; i < OuiCuc; i ++)
    	{
    	if (oamSel & (0x1<<i))
            return i;
    	}

    return OuiCtc;
    }

void EponDyingGaspOamOui(U8 oamSel)
    {
    OuiVendor outType = EponDyingGaspOuiTran(oamSel);
    if (gOuiType == outType)
    	{//same return
        return;
    	}

    gOuiType = outType;
    EponDyingGaspPayLoad(gOuiType);
    }


void EponDyingGaspInit(void)
    {
    EponDyingGaspAlloc();
    kerSysRegisterDyingGaspHandler("epon", &EponDyingGaspHandler, NULL);
    }


void EponDyingGaspExit(void)
    {
    kerSysDeregisterDyingGaspHandler("epon");
    }


void EponFrameTaskInit(void)
    {
    protocolDest.pift = PortIfPon;
    protocolDest.inst = 0;
	
    EponMpcpPathInit();
    EponOamPathInit();

    //init dying gasp handler
    EponDyingGaspInit();

    EponFrameTaskTimerReg();
    }


void EponFrameTaskExit(void)
    {
    EponMpcpPathExit();
    EponOamPathExit();

    EponDyingGaspExit();
    }

// end of EponFrame.c

