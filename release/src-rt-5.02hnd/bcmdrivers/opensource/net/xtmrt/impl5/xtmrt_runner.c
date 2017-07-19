/*
<:copyright-BRCM:2011:DUAL/GPL:standard

   Copyright (c) 2011 Broadcom 
   All Rights Reserved

Unless you and Broadcom execute a separate written software license
agreement governing use of this software, this software is licensed
to you under the terms of the GNU General Public License version 2
(the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
with the following added to such license:

   As a special exception, the copyright holders of this software give
   you permission to link this software with independent modules, and
   to copy and distribute the resulting executable under terms of your
   choice, provided that you also meet, for each linked independent
   module, the terms and conditions of the license of that module.
   An independent module is a module which is not derived from this
   software.  The special exception does not apply to any modifications
   of the software.

Not withstanding the above, under no circumstances may you combine
this software in any way with any other Broadcom software provided
under a license other than the GPL, without Broadcom's express prior
written consent.

:>
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>
#include <linux/if_arp.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/atmppp.h>
#include <linux/blog.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ip.h>
#include <linux/bcm_log.h>
#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <board.h>
#include "bcmnet.h"
#include "bcmxtmcfg.h"
#include "bcmxtmrt.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>
#include "bcmxtmrtimpl.h"

#include "xtmrt_runner.h"


/**** Externs ****/
extern bdmf_type_handle rdpa_xtmchannel_drv(void);


/**** Globals ****/


/**** Prototypes ****/

static void bcmxtmrt_timer(PBCMXTMRT_GLOBAL_INFO pGi);
static int cfg_xtmflows(UINT32 queueIdx, UINT32 trafficType, UINT32 hdrType, UINT8 vcid, UINT32 ulTxPafEnabled);
static int add_xtmflow(bdmf_object_handle attrs, UINT32 ctType, UINT32 queueIdx,
                       UINT32 fstat, int ptmBonding);
static UINT32 calc_xtmflow_fstat(UINT32 ctType, UINT32 hdrType, UINT32 vcid,
                              UINT32 flag);
static int  runner_xtm_objects_init(void);
static void runner_xtm_objects_uninit(void);
static int  runner_tx_queues_init(bdmf_object_handle xtm);
static void runner_tx_queues_uninit(void);
static int cfg_cpu_rx_queue(int queue_id, UINT32 queue_size,
                            void *rx_isr);
static int cfg_cpu_reason_to_queue(rdpa_cpu_reason reason,
                                   rdpa_traffic_dir direction,
                                   int queue_id);
static void cpu_rxq_isr(int queue_id);


/**** Statics ****/

static bdmf_object_handle rdpa_cpu_obj = NULL;

/*---------------------------------------------------------------------------
 * int bcmxapi_module_init(void)
 * Description:
 *    Called when the driver is loaded.
 * Returns:
 *    0 or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_module_init(void)
{
   int rc = 0;
   rdpa_cpu_reason_index_t reason_cfg = {BDMF_INDEX_UNASSIGNED, BDMF_INDEX_UNASSIGNED};

   /* get rdpa cpu object */
   if (rdpa_cpu_get(rdpa_cpu_host, &rdpa_cpu_obj))
      return -ESRCH;

   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   
   /* configure cpu rx queues */
   rc = cfg_cpu_rx_queue(RDPA_XTM_CPU_LO_RX_QUEUE_ID, RDPA_XTM_CPU_RX_QUEUE_SIZE,
                         &cpu_rxq_isr);
   if (rc)
   {
      printk(KERN_ERR CARDNAME ": Error(%d) cfg CPU low priority rx queue\n", rc);
      return -EFAULT;
   }
    
   rc = cfg_cpu_rx_queue(RDPA_XTM_CPU_HI_RX_QUEUE_ID, RDPA_XTM_CPU_RX_QUEUE_SIZE,
                         &cpu_rxq_isr);
   if (rc)
   {
      printk(KERN_ERR CARDNAME ": Error(%d) cfg CPU high priority rx queue\n", rc);
      return -EFAULT;
   }
    
   bdmf_lock();
   
   while (!rdpa_cpu_reason_cfg_get_next(rdpa_cpu_obj, &reason_cfg))
   {
      if (reason_cfg.dir == rdpa_dir_us)
      {
         continue;
      }

      if ((reason_cfg.reason == rdpa_cpu_rx_reason_oam) ||
          (reason_cfg.reason == rdpa_cpu_rx_reason_omci) ||
          (reason_cfg.reason >= rdpa_cpu_rx_reason_direct_queue_0 &&
           reason_cfg.reason <= rdpa_cpu_rx_reason_direct_queue_7))
      {
         continue;
      }
      
      /* FIXME: need sort out high and low priority reasons */      
      if (reason_cfg.reason == rdpa_cpu_rx_reason_etype_pppoe_d ||
          reason_cfg.reason == rdpa_cpu_rx_reason_etype_pppoe_s ||
          reason_cfg.reason == rdpa_cpu_rx_reason_etype_arp ||
          reason_cfg.reason == rdpa_cpu_rx_reason_etype_801_1ag_cfm ||
          reason_cfg.reason == rdpa_cpu_rx_reason_l4_icmp ||
          reason_cfg.reason == rdpa_cpu_rx_reason_icmpv6 ||
          reason_cfg.reason == rdpa_cpu_rx_reason_igmp ||
          reason_cfg.reason == rdpa_cpu_rx_reason_dhcp ||
          reason_cfg.reason == rdpa_cpu_rx_reason_l4_udef_0)
      {
         rc = cfg_cpu_reason_to_queue(reason_cfg.reason, reason_cfg.dir,
                                      RDPA_XTM_CPU_HI_RX_QUEUE_ID);
         if (rc)
         {
            printk(KERN_ERR CARDNAME ": Error(%d) cfg CPU reason to high priority rx queue\n", rc);
            break;
         }
      }
      else
      {
         /* Continue with the LOW RX Q */
         rc = cfg_cpu_reason_to_queue(reason_cfg.reason, reason_cfg.dir,
                                      RDPA_XTM_CPU_LO_RX_QUEUE_ID);
         if (rc)
         {
            printk(KERN_ERR CARDNAME ": Error(%d) cfg CPU reason to low priority rx queue\n", rc);
            break;
         }
      }
   }

   bdmf_unlock();
   
   return rc;
   
}  /* bcmxapi_module_init() */


/*---------------------------------------------------------------------------
 * void bcmxapi_module_cleanup(void)
 * Description:
 *    Called when the driver is unloaded.
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_module_cleanup(void)
{
   bcmxapi_disable_rx_interrupt();
   
   bdmf_put(rdpa_cpu_obj);

}  /* bcmxapi_module_cleanup() */


/*---------------------------------------------------------------------------
 * int bcmxapi_enable_rx_interrupt(void)
 * Description:
 *    Enable cpu rx queue interrupt
 * Returns:
 *    0 or error code
 *---------------------------------------------------------------------------
 */
int bcmxapi_enable_rx_interrupt(void)
{
//   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
//   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_enable(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
    
   return 0;
   
}  /* bcmxapi_enable_rx_interrupt() */


/*---------------------------------------------------------------------------
 * int bcmxapi_disable_rx_interrupt(void)
 * Description:
 *    Disable cpu rx queue interrupt
 * Returns:
 *    0 or error code
 *---------------------------------------------------------------------------
 */
int bcmxapi_disable_rx_interrupt(void)
{
   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   rdpa_cpu_rxq_flush_set(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID, TRUE);
   rdpa_cpu_rxq_flush_set(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID, TRUE);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);

   return 0;
   
}  /* bcmxapi_disable_rx_interrupt() */


/*---------------------------------------------------------------------------
 * int cfg_cpu_rx_queue(int queue_id, UINT32_t queue_size, void *rx_isr)
 * Description:
 *    Set cpu rx queue configuration including cpu rx queue isr.
 * Returns:
 *    0 or error code
 *---------------------------------------------------------------------------
 */
static int cfg_cpu_rx_queue(int queue_id, UINT32 queue_size, void *rx_isr)
{
   int rc;
   rdpa_cpu_rxq_cfg_t rxq_cfg;

   /* Read current configuration, set new drop threshold and ISR and write
    * back.
    */
   bdmf_lock();
   rc = rdpa_cpu_rxq_cfg_get(rdpa_cpu_obj, queue_id, &rxq_cfg);
   if (rc == 0)
   {
      rxq_cfg.size     = queue_size;
      rxq_cfg.isr_priv = queue_id;
      rxq_cfg.rx_isr   = (rdpa_cpu_rxq_rx_isr_cb_t)rx_isr;
      rc = rdpa_cpu_rxq_cfg_set(rdpa_cpu_obj, queue_id, &rxq_cfg);
   }
   bdmf_unlock();
   
   return rc;
   
}  /* cfg_cpu_rx_queue() */


/*---------------------------------------------------------------------------
 * int cfg_cpu_reason_to_queue(rdpa_cpu_reason reason,
 *                             rdpa_traffic_dir direction,
 *                             int queue_id)
 * Description:
 *    Set cpe/reason_cfg attribute and trap reason configuration.
 * Returns:
 *    0 or error code
 *---------------------------------------------------------------------------
 */
static int cfg_cpu_reason_to_queue(rdpa_cpu_reason reason,
                                   rdpa_traffic_dir direction,
                                   int queue_id)
{
   int rc;
   rdpa_cpu_reason_cfg_t   reason_cfg = {};
   rdpa_cpu_reason_index_t cpu_reason;

   cpu_reason.reason = reason;
   cpu_reason.dir    = direction;
   cpu_reason.table_index = CPU_REASON_WAN0_TABLE_INDEX;
   reason_cfg.queue  = queue_id; 
   reason_cfg.meter  = BDMF_INDEX_UNASSIGNED; 
   rc = rdpa_cpu_reason_cfg_set(rdpa_cpu_obj, &cpu_reason, &reason_cfg);

   return rc;
   
}  /* cfg_cpu_reason_to_queue() */


/*---------------------------------------------------------------------------
 * void cpu_rxq_isr(int queue_id)
 * Description:
 *    xtm receive interrupt service routine
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void cpu_rxq_isr(int queue_id)
{
   int i;
   PBCMXTMRT_DEV_CONTEXT pDevCtx;
   
   /* disable and clear interrupts from both high and low rx queue */
   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   rdpa_cpu_int_disable(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID);
   rdpa_cpu_int_clear(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID);

   /* Process RX data for each device that is open */
   for (i = 0; i < MAX_DEV_CTXS; i++)
   {
      if ((pDevCtx = g_GlobalInfo.pDevCtxs[i]) != NULL &&
          pDevCtx->ulOpenState == XTMRT_DEV_OPENED)
      {
         /* Device is open.  Start processing RX packets. */
         BCMXTMRT_WAKEUP_RXWORKER(&g_GlobalInfo);
      }
   }
}  /* cpu_rxq_isr() */


/*---------------------------------------------------------------------------
 * UINT32 bcmxapi_rxtask(UINT32 ulBudget, UINT32 *pulMoreToDo)
 * Description:
 *    xtm receive task called RX thread.
 * Returns:
 *    0 - success, Error code - failure
 *---------------------------------------------------------------------------
 */
UINT32 bcmxapi_rxtask(UINT32 ulBudget, UINT32 *pulMoreToDo)
{
   int rc;
   
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   PBCMXTMRT_DEV_CONTEXT pDevCtx;
   struct sk_buff *skb;
   unsigned long irqFlags;
   UINT32 ulCell;
   UINT32 ulMoreToReceive;
   UINT32 ulRxPktGood = 0;
   UINT32 ulRxPktProcessed = 0;
   UINT32 ulRxPktMax = ulBudget + (ulBudget / 2);
   rdpa_cpu_rx_info_t info = {};

   
   /* Receive packets from every receive queue in a round robin order until
   * there are no more packets to receive.
   */
   do
   {
      ulMoreToReceive = 0;

      if (ulBudget == 0)
      {
         *pulMoreToDo = 1;
         break;
      }

      spin_lock_irqsave(&pGi->xtmlock_rx, irqFlags);

      /* Receive packets from high priority cpu rx queue */
      rc = rdpa_cpu_packet_get(rdpa_cpu_host, RDPA_XTM_CPU_HI_RX_QUEUE_ID, &info);
      if (rc == BDMF_ERR_NO_MORE)
      {
         /* Receive packet from low priority cpu rx queue */
         rc = rdpa_cpu_packet_get(rdpa_cpu_host, RDPA_XTM_CPU_LO_RX_QUEUE_ID, &info);
         if (rc == BDMF_ERR_NO_MORE)
         {
            ulRxPktGood |= XTM_POLL_DONE;
            spin_unlock_irqrestore(&pGi->xtmlock_rx, irqFlags);
            break;
         }
      }
      spin_unlock_irqrestore(&pGi->xtmlock_rx, irqFlags);
      
      ulRxPktProcessed++;
      
      if (rc)
      {
         /* Some error */
         printk(KERN_NOTICE "Error in rdpa_cpu_packet_get() (%d)\n", rc);
         
         /* Consider error packets (assuming RDD succeeded dequeuing
          * them) as read packets.
          */
         goto drop_pkt;
      }
      //printk("reason: %d src_bridge_port: %d gemflow: %d\n", info.reason, info.src_port, info.reason_data);
      
      pDevCtx = pGi->pDevCtxsByMatchId[info.reason_data & FSTAT_MATCH_ID_MASK];
      ulCell  = (info.reason_data & FSTAT_PACKET_CELL_MASK) == FSTAT_CELL;
      //printk("pDevCtx =  %x \n", (unsigned int) pDevCtx) ;

        /* create a sysb and initilize it with packet data & len */

      skb = bdmf_sysb_header_alloc(bdmf_sysb_skb, (uint8_t *)info.data, info.size, 0);
      if (!skb)
      {
        /* free the data buffer */
        bdmf_sysb_databuf_free((uint8_t *)info.data, 0);

        printk("%s:sysb_header allocation failed\n",__FUNCTION__);
        goto drop_pkt;
      }

      skb = bdmf_sysb_2_fkb_or_skb(skb);

      /* error status, or packet with no pDev */
      if (((info.reason_data & FSTAT_ERROR) != 0) ||
          ((!ulCell) && (pDevCtx == NULL)))   /* packet */
      {
         if ((info.reason_data & FSTAT_MATCH_ID_MASK) == TEQ_DATA_VCID &&
             pGi->pTeqNetDev)
         {
            unsigned long flags;
                           
            /* Sending TEQ data to interface told to us by DSL Diags */
            skb->dev      = pGi->pTeqNetDev;
            skb->protocol = htons(ETH_P_802_3);
            local_irq_save(flags);
            local_irq_enable();
            dev_queue_xmit(skb);
            local_irq_restore(flags);
         }
         else
         {
            //DUMP_PKT(skb->data, skb->len);
            dev_kfree_skb_any(skb);
            if (pDevCtx)
               pDevCtx->DevStats.rx_errors++;
         }
      }
      else if (!ulCell) /* process packet, pDev != NULL */
      {
         pNBuff_t pNBuf = SKBUFF_2_PNBUFF(skb);
         
         bcmxtmrt_processRxPkt(pDevCtx, NULL, pNBuf, 1,
                               (UINT16)info.reason_data, skb->len);

         ulRxPktGood++;
         ulBudget--;
      }
      else  /* process cell */
      {
         bcmxtmrt_processRxCell(skb->data);
         dev_kfree_skb_any(skb);
      }
drop_pkt:
      if (ulRxPktProcessed >= ulRxPktMax)
         break;
      else
         ulMoreToReceive = 1; /* more packets to receive on Rx queue? */
      
   } while (ulMoreToReceive);

   return (ulRxPktGood);

}  /* bcmxtmrt_rxtask() */


/*---------------------------------------------------------------------------
 * int bcmxapi_add_proc_files(void)
 * Description:
 *    Adds proc file system directories and entries.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_add_proc_files(void)
{
   proc_mkdir("driver/xtm", NULL);

//   create_proc_read_entry("driver/xtm/txdmainfo",  0, NULL, ProcDmaTxInfo,  0);
//   create_proc_read_entry("driver/xtm/rxdmainfo",  0, NULL, ProcDmaRxInfo,  0);
//   create_proc_read_entry("driver/xtm/txbondinfo", 0, NULL, ProcTxBondInfo, 0);
   return 0;
    
}  /* bcmxapi_add_proc_files() */


/*---------------------------------------------------------------------------
 * int bcmxapi_del_proc_files(void)
 * Description:
 *    Deletes proc file system directories and entries.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_del_proc_files(void)
{
//   remove_proc_entry("driver/xtm/txschedinfo", NULL);
//   remove_proc_entry("driver/xtm/txbondinfo", NULL);
//   remove_proc_entry("driver/xtm/txdmainfo", NULL);
   remove_proc_entry("driver/xtm", NULL);
   return 0;
    
}  /* bcmxapi_del_proc_files() */


/*---------------------------------------------------------------------------
 * Function Name: bcmxtmrt_timer
 * Description:
 *    Periodic timer that calls the rx task to receive TEQ cells.
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void bcmxtmrt_timer(PBCMXTMRT_GLOBAL_INFO pGi)
{
   if (pGi->pTeqNetDev && ((void *)pGi->ulDevCtxMask == NULL))
   {
      UINT32 ulNotUsed;
      bcmxapi_rxtask(100, &ulNotUsed);

      /* Restart the timer. */
      pGi->Timer.expires = jiffies + SAR_TIMEOUT;
      add_timer(&pGi->Timer);
   }
}  /* bcmxtmrt_timer() */


/*---------------------------------------------------------------------------
 * int bcmxapi_DoGlobInitReq(PXTMRT_GLOBAL_INIT_PARMS pGip)
 * Description:
 *    Processes an XTMRT_CMD_GLOBAL_INITIALIZATION command.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_DoGlobInitReq(PXTMRT_GLOBAL_INIT_PARMS pGip)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   int rc = 0;
   
   if (pGi->ulDrvState != XTMRT_UNINITIALIZED)
      return -EPERM;

   bcmLog_setLogLevel(BCM_LOG_ID_XTM, BCM_LOG_LEVEL_ERROR);

   spin_lock_init(&pGi->xtmlock_tx);
   spin_lock_init(&pGi->xtmlock_rx);
   spin_lock_init(&pGi->xtmlock_rx_regs);

   /* Save MIB counter/Cam registers. */
   pGi->pulMibTxOctetCountBase = pGip->pulMibTxOctetCountBase;
   pGi->ulMibRxClrOnRead       = pGip->ulMibRxClrOnRead;
   pGi->pulMibRxCtrl           = pGip->pulMibRxCtrl;
   pGi->pulMibRxMatch          = pGip->pulMibRxMatch;
   pGi->pulMibRxOctetCount     = pGip->pulMibRxOctetCount;
   pGi->pulMibRxPacketCount    = pGip->pulMibRxPacketCount;
   pGi->pulRxCamBase           = pGip->pulRxCamBase;
   
   pGi->bondConfig.uConfig = pGip->bondConfig.uConfig;
   if ((pGi->bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) ||
       (pGi->bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE))
      printk(CARDNAME ": PTM/ATM Bonding Mode configured in system\n");
   else
      printk(CARDNAME ": PTM/ATM Non-Bonding Mode configured in system\n");

   pGi->atmBondSidMode = ATMBOND_ASM_MESSAGE_TYPE_NOSID;

   /* Initialize a timer function for TEQ */
   init_timer(&pGi->Timer);
   pGi->Timer.data     = (UINT32)pGi;
   pGi->Timer.function = (void *)bcmxtmrt_timer;

   /* create and initialize xtm objects */
   rc = runner_xtm_objects_init();
   if (rc)
      rc = -EFAULT;
   else
      pGi->ulDrvState = XTMRT_INITIALIZED;
      
   return rc;
   
}  /* bcmxapi_DoGlobInitReq() */


/*---------------------------------------------------------------------------
 * int bcmxapi_DoGlobUninitReq(void)
 * Description:
 *    Processes an XTMRT_CMD_GLOBAL_UNINITIALIZATION command.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_DoGlobUninitReq(void)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;

   if (pGi->ulDrvState == XTMRT_UNINITIALIZED)
      return -EPERM;
      
   del_timer_sync(&pGi->Timer);
      
   runner_xtm_objects_uninit();
     
   pGi->ulDrvState = XTMRT_UNINITIALIZED;
      
   return 0;
   
}  /* bcmxapi_DoGlobUninitReq() */


/*---------------------------------------------------------------------------
 * int bcmxapi_DoSetTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                          PXTMRT_TRANSMIT_QUEUE_ID pTxQId)
 * Description:
 *    Allocate memory for and initialize a transmit queue.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxapi_DoSetTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                         PXTMRT_TRANSMIT_QUEUE_ID pTxQId)
{
   int rc = 0, queueIdx;
   UINT32 ulPort;
   BcmPktDma_XtmTxDma  *txdma;
   bdmf_object_handle   egress_tm ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;

   BCM_XTM_DEBUG("DoSetTxQueue\n");

   local_bh_enable();  // needed to avoid kernel error

   txdma = (BcmPktDma_XtmTxDma *)kzalloc(sizeof(BcmPktDma_XtmTxDma), GFP_ATOMIC);

   local_bh_disable();

   if (txdma == NULL)
   {
      printk("Unable to allocate memory for tx dma info\n");
      return -ENOMEM;
   }

   ulPort = PORTID_TO_PORT(pTxQId->ulPortId);

   if ((ulPort < MAX_PHY_PORTS) && (pTxQId->ucSubPriority < MAX_SUB_PRIORITIES))
   {
      /* Configure a TM egress queue */
      queueIdx  = pTxQId->ulQueueIndex;
      egress_tm = pGi->txBdmfObjs[queueIdx].egress_tm;
     
      if (egress_tm != NULL)
      {
         rdpa_tm_queue_cfg_t  queue_cfg = {};

         queue_cfg.queue_id       = queueIdx;
         queue_cfg.weight         = 0;
         queue_cfg.drop_threshold = QUEUE_DROP_THRESHOLD;
         queue_cfg.stat_enable    = 1;

         if (pTxQId->ucDropAlg == WA_RED)
         {
            /* Currently Runner firmware does not support RED.
             * RED can be implemented as WRED with equal low and high
             * class thresholds.
             */
            queue_cfg.drop_alg = rdpa_tm_drop_alg_wred;
            queue_cfg.low_class.min_threshold  = (pTxQId->ucLoMinThresh * queue_cfg.drop_threshold) / 100;
            queue_cfg.low_class.max_threshold  = (pTxQId->ucLoMaxThresh * queue_cfg.drop_threshold) / 100;
            queue_cfg.high_class.min_threshold = queue_cfg.low_class.min_threshold;
            queue_cfg.high_class.max_threshold = queue_cfg.low_class.max_threshold;
         }
         else if (pTxQId->ucDropAlg == WA_WRED)
         {
            queue_cfg.drop_alg = rdpa_tm_drop_alg_wred;
            queue_cfg.low_class.min_threshold  = (pTxQId->ucLoMinThresh * queue_cfg.drop_threshold) / 100;
            queue_cfg.low_class.max_threshold  = (pTxQId->ucLoMaxThresh * queue_cfg.drop_threshold) / 100;
            queue_cfg.high_class.min_threshold = (pTxQId->ucHiMinThresh * queue_cfg.drop_threshold) / 100;
            queue_cfg.high_class.max_threshold = (pTxQId->ucHiMaxThresh * queue_cfg.drop_threshold) / 100;
         }
         else
         {
            /* DT */
            queue_cfg.drop_alg = rdpa_tm_drop_alg_dt;
            queue_cfg.low_class.min_threshold  = 0;
            queue_cfg.low_class.max_threshold  = 0;
            queue_cfg.high_class.min_threshold = 0;
            queue_cfg.high_class.max_threshold = 0;
         }

         printk("bcmxtmrt: Egress TM Q %d Setup. Buffering/Q - %d \n", (unsigned int)queueIdx, 
                (unsigned int) queue_cfg.drop_threshold );
         if ((rc = rdpa_egress_tm_queue_cfg_set(egress_tm, 0, &queue_cfg)))
         {
            printk(CARDNAME "DoSetTxQueue: rdpa_egress_tm_queue_cfg_set error rc=%d\n", rc);
         }
         /* create rdpa xtmflows for this queue */
         else if (cfg_xtmflows(queueIdx, pDevCtx->ulTrafficType, pDevCtx->ulHdrType,
                               pDevCtx->ucTxVcid, pDevCtx->ulTxPafEnabled) != 0)
         {
            printk(CARDNAME "DoSetTxQueue: Failed to create xtmflows for Q %d\n",
                   (unsigned int)queueIdx);
            rc = -EFAULT;
         }
         else
         {
            UINT32 ulPtmPrioIdx = PTM_FLOW_PRI_LOW;

            txdma->ulPort        = ulPort;
            txdma->ulPtmPriority = pTxQId->ulPtmPriority;
            txdma->ulSubPriority = pTxQId->ucSubPriority;
            txdma->ucDropAlg     = pTxQId->ucDropAlg;
            txdma->ucLoMinThresh = pTxQId->ucLoMinThresh;
            txdma->ucLoMaxThresh = pTxQId->ucLoMaxThresh;
            txdma->ucHiMinThresh = pTxQId->ucHiMinThresh;
            txdma->ucHiMaxThresh = pTxQId->ucHiMaxThresh;
           
            /* Set every transmit queue size to the number of external buffers.
             * The QueuePacket function will control how many packets are queued.
             */
//            txdma->ulQueueSize   = pTxQId->usQueueSize; // pGi->ulNumExtBufs;

            if ((pDevCtx->Addr.ulTrafficType == TRAFFIC_TYPE_PTM) ||
                (pDevCtx->Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED))
               ulPtmPrioIdx = (txdma->ulPtmPriority == PTM_PRI_HIGH)?
                              PTM_FLOW_PRI_HIGH : PTM_FLOW_PRI_LOW;

            pDevCtx->pTxPriorities[ulPtmPrioIdx][ulPort][txdma->ulSubPriority] = txdma;
            pDevCtx->pTxQids[pTxQId->ucQosQId] = txdma;

            if (pDevCtx->pHighestPrio == NULL ||
                pDevCtx->pHighestPrio->ulSubPriority < txdma->ulSubPriority)
               pDevCtx->pHighestPrio = txdma;

            /* Increment channels per dev context */
            pDevCtx->txdma[pDevCtx->ulTxQInfosSize++] = txdma;

            txdma->ulDmaIndex = queueIdx;
            txdma->txEnabled  = 1;
            return 0;
         }
      } /* if (egress_tm) */
   }

   kfree(txdma);
   return rc;

}  /* bcmxapi_DoSetTxQueue() */


/*---------------------------------------------------------------------------
 * void bcmxapi_ShutdownTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                              volatile BcmPktDma_XtmTxDma *txdma)
 * Description:
 *    Flush a transmit queue and delete all the wan flows associated
 *    with the queue.
 * Returns: void
 * Notes:
 *    pDevCtx is not used.
 *---------------------------------------------------------------------------
 */
void bcmxapi_ShutdownTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                             volatile BcmPktDma_XtmTxDma *txdma)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   int      i;
   UINT32   queueIdx;
   bdmf_object_handle   egress_tm, xtmflow;

   /* Changing txEnabled to 0 prevents any more packets
    * from being queued on a transmit channel. Allow all currenlty
    * queued transmit packets to be transmitted before disabling the DMA.
    */
   txdma->txEnabled = 0;
   
   queueIdx = txdma->ulDmaIndex ;
   
   /* delete all the xtmflows associated with the tx queue */
   for (i = 0; i < MAX_CIRCUIT_TYPES; i++)
   {
      xtmflow = pGi->txBdmfObjs[queueIdx].xtmflow[i];
      if (xtmflow != NULL)
      {
         bdmf_destroy(xtmflow);
         pGi->txBdmfObjs[queueIdx].xtmflow[i] = NULL;
      }
   }
      
   /* Set the tx queue suze to 0 by setting drop threshold to 0
    * This will be set back when the Q gets (re)created.
    */
   egress_tm = pGi->txBdmfObjs[queueIdx].egress_tm;
   if (egress_tm != NULL)
   {
      int   rc = 0;
      rdpa_tm_queue_cfg_t  queue_cfg = {};
      
      queue_cfg.queue_id       = queueIdx;
      queue_cfg.weight         = 0;
      queue_cfg.drop_alg       = rdpa_tm_drop_alg_dt;
      queue_cfg.drop_threshold = 0;
      queue_cfg.stat_enable    = 1;
      queue_cfg.low_class.min_threshold  = 0;
      queue_cfg.low_class.max_threshold  = 0;
      queue_cfg.high_class.min_threshold = 0;
      queue_cfg.high_class.max_threshold = 0;

      printk("bcmxtmrt: Egress TM Q %d Shutdown \n", (unsigned int) queueIdx);
      if ((rc = rdpa_egress_tm_queue_cfg_set(egress_tm, 0, &queue_cfg)))
      {
         printk(CARDNAME "ShutdownTxQueue: rdpa_egress_tm_queue_cfg_set error rc=%d\n", rc);
      }
   }
}  /* bcmxapi_ShutdownTxQueue() */

                             
/*---------------------------------------------------------------------------
 * void bcmxapi_FlushdownTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                               volatile BcmPktDma_XtmTxDma *txdma)
 * Description:
 *    Flush a transmit queue and delete all the wan flows associated
 *    with the queue.
 * Returns: void
 * Notes:
 *    pDevCtx is not used.
 *---------------------------------------------------------------------------
 */
void bcmxapi_FlushdownTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                              volatile BcmPktDma_XtmTxDma *txdma)
{
   /* for runner case, we do not need flush operation due to the following */
   /* We either run in CPU mode that means nothing to delete from runners as
    * CPU is in control as well as in runner mode, we always run with TxPAF,
    * which needs data on only one port and it will distribute onto available
    * ports automatically. No need to flush the data on the down port, as there 
    * may nothing pending
    */

   return ;
}  /* bcmxapi_FlushdownTxQueue() */


/*---------------------------------------------------------------------------
 * void bcmxapi_StopTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                          volatile BcmPktDma_XtmTxDma *txdma)
 * Description:
 *    Stop a transmit queue.
 * Returns: void
 * Notes:
 *    pDevCtx is not used.
 *---------------------------------------------------------------------------
 */
void bcmxapi_StopTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                         volatile BcmPktDma_XtmTxDma *txdma)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   UINT32   queueIdx;
   bdmf_object_handle   egress_tm;

   /* Changing txEnabled to 0 prevents any more packets
    * from being queued on a transmit channel. Allow all currenlty
    * queued transmit packets to be transmitted before disabling the DMA.
    */
   txdma->txEnabled = 0;
   
   queueIdx  = txdma->ulDmaIndex;
   
   /* Set the tx queue suze to 0 by setting drop threshold to 0
    * This will be set back when the Q gets (re)created.
    */
   egress_tm = pGi->txBdmfObjs[queueIdx].egress_tm;
   if (egress_tm != NULL)
   {
      int   rc = 0;
      rdpa_tm_queue_cfg_t  queue_cfg = {};
      
      queue_cfg.queue_id                 = queueIdx;
      queue_cfg.weight                   = 0;
      queue_cfg.drop_alg                 = rdpa_tm_drop_alg_dt;
      queue_cfg.drop_threshold           = 0;
      queue_cfg.stat_enable              = 1;
      queue_cfg.high_class.min_threshold = 0;
      queue_cfg.high_class.max_threshold = 0;
      queue_cfg.low_class.min_threshold  = 0;
      queue_cfg.low_class.max_threshold  = 0;

      if ((rc = rdpa_egress_tm_queue_cfg_set(egress_tm, 0, &queue_cfg)))
      {
         printk(CARDNAME "StopTxQueue: rdpa_egress_tm_queue_cfg_set error rc=%d\n", rc);
      }
   }
}  /* bcmxapi_StopTxQueue() */


/*---------------------------------------------------------------------------
 * void bcmxapi_StartTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                          volatile BcmPktDma_XtmTxDma *txdma)
 * Description:
 *    Start a transmit queue.
 * Returns: void
 * Notes:
 *    pDevCtx is not used.
 *---------------------------------------------------------------------------
 */
void bcmxapi_StartTxQueue(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                          volatile BcmPktDma_XtmTxDma *txdma)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   UINT32   queueIdx;
   bdmf_object_handle   egress_tm;

   queueIdx  = txdma->ulDmaIndex;
   
   egress_tm = pGi->txBdmfObjs[queueIdx].egress_tm;
   if (egress_tm != NULL)
   {
      int   rc = 0;
      rdpa_tm_queue_cfg_t  queue_cfg = {};
      
      queue_cfg.queue_id       = queueIdx;
      queue_cfg.weight         = 0;
      queue_cfg.drop_threshold = QUEUE_DROP_THRESHOLD;
      queue_cfg.stat_enable    = 1;

      if (txdma->ucDropAlg == WA_RED)
      {
         /* Currently Runner firmware does not support RED.
          * RED can be implemented as WRED with equal low and high
          * class thresholds.
          */
         queue_cfg.drop_alg = rdpa_tm_drop_alg_wred;
         queue_cfg.low_class.min_threshold  = (txdma->ucLoMinThresh * queue_cfg.drop_threshold) / 100;
         queue_cfg.low_class.max_threshold  = (txdma->ucLoMaxThresh * queue_cfg.drop_threshold) / 100;
         queue_cfg.high_class.min_threshold = queue_cfg.low_class.min_threshold;
         queue_cfg.high_class.max_threshold = queue_cfg.low_class.max_threshold;
      }
      else if (txdma->ucDropAlg == WA_WRED)
      {
         queue_cfg.drop_alg = rdpa_tm_drop_alg_wred;
         queue_cfg.low_class.min_threshold  = (txdma->ucLoMinThresh * queue_cfg.drop_threshold) / 100;
         queue_cfg.low_class.max_threshold  = (txdma->ucLoMaxThresh * queue_cfg.drop_threshold) / 100;
         queue_cfg.high_class.min_threshold = (txdma->ucHiMinThresh * queue_cfg.drop_threshold) / 100;
         queue_cfg.high_class.max_threshold = (txdma->ucHiMaxThresh * queue_cfg.drop_threshold) / 100;
      }
      else
      {
         /* DT */
         queue_cfg.drop_alg = rdpa_tm_drop_alg_dt;
         queue_cfg.low_class.min_threshold  = 0;
         queue_cfg.low_class.max_threshold  = 0;
         queue_cfg.high_class.min_threshold = 0;
         queue_cfg.high_class.max_threshold = 0;
      }


      if ((rc = rdpa_egress_tm_queue_cfg_set(egress_tm, 0, &queue_cfg)))
      {
         printk(CARDNAME "StartTxQueue: rdpa_egress_tm_queue_cfg_set error rc=%d\n", rc);
      }

   }
}  /* bcmxapi_StartTxQueue() */


/*---------------------------------------------------------------------------
 * void bcmxapi_SetPtmBondPortMask(UINT32 portMask)
 * Description:
 *    Set the value of portMask in ptmBondInfo data structure.
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_SetPtmBondPortMask(UINT32 portMask)
{
   g_GlobalInfo.ptmBondInfo.portMask = portMask;
   
}  /* bcmxapi_SetPtmBondPortMask() */


/*---------------------------------------------------------------------------
 * void bcmxapi_SetPtmBonding(UINT32 bonding)
 * Description:
 *    Set the value of bonding in ptmBondInfo data structure.
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_SetPtmBonding(UINT32 bonding)
{
   g_GlobalInfo.ptmBondInfo.bonding = bonding;
   
}  /* bcmxapi_SetPtmBonding() */


/*---------------------------------------------------------------------------
 * void bcmxapi_XtmGetStats(UINT8 vport, UINT32 *rxDropped, UINT32 *txDropped)
 * Description:
 *
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_XtmGetStats(UINT8 vport, UINT32 *rxDropped, UINT32 *txDropped)
{
   *rxDropped = 0;
   *txDropped = 0;
   
}  /* bcmxapi_XtmGetStats() */


/*---------------------------------------------------------------------------
 * void bcmxapi_XtmResetStats(UINT8 vport)
 * Description:
 *
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_XtmResetStats(UINT8 vport)
{
   return;
   
}  /* bcmxapi_XtmGetStats() */

/*---------------------------------------------------------------------------
 * void bcmxapi_blog_ptm_us_bonding (UINT32 ulTxPafEnabled, sk_buff *skb)
 * Description:
 *
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxapi_blog_ptm_us_bonding(UINT32 ulTxPafEnabled, struct sk_buff *skb)
{
#if !defined(SUPPORT_631XX_TX_RX_IUDMA)
   if (!ulTxPafEnabled) {
      blog_ptm_us_bonding (skb, BLOG_PTM_US_BONDING_NO_HW_ACCELERATION) ;
   }
#else
   blog_ptm_us_bonding (skb, BLOG_PTM_US_BONDING_NO_HW_ACCELERATION) ;
#endif
}  /* bcmxapi_blog_ptm_us_bonding() */

/*---------------------------------------------------------------------------
 * int runner_xtm_objects_init(void)
 * Description:
 *    Initialize the following runner bdmf objects:
 *    - xtm (gpon) system object
 *    - wan1 port object owned by xtm
 *    - 16 xtm transmit queues
 * Returns:
 *    0 or bdmf error code
 *---------------------------------------------------------------------------
 */
static int runner_xtm_objects_init(void)
{
   int   rc = 0;
   bdmf_object_handle   xtm;
   bdmf_object_handle   wan;
      
   BDMF_MATTR(xtm_attr,  rdpa_xtm_drv());
   BDMF_MATTR(port_attr, rdpa_port_drv());

   /* Create the xtm system object */   
   rc = bdmf_new_and_set(rdpa_xtm_drv(), NULL, xtm_attr, &xtm);
   if (rc)
   {
      BCM_LOG_NOTICE(BCM_LOG_ID_XTM, "Failed to create xtm object, rc %d\n", rc);
      return rc;
   }
      
   /* Create logical ports object on top of the physical wan */
   rdpa_port_index_set(port_attr, rdpa_if_wan1);
   rc = bdmf_new_and_set(rdpa_port_drv(), xtm, port_attr, &wan); 
   if (rc)
   {
      BCM_LOG_NOTICE(BCM_LOG_ID_XTM,"Problem creating xtm wan port object\n");
      bdmf_destroy(xtm);
      return rc;
   }

   /* Initialize xtmchannel and egress tm objects for all tx queues */
   rc = runner_tx_queues_init(xtm);
   if (rc)
   {
      BCM_LOG_NOTICE(BCM_LOG_ID_XTM,"Problem initialize xtm tx queues\n");
      bdmf_destroy(wan);
      bdmf_destroy(xtm);
      return rc;
   }

   g_GlobalInfo.bdmfXtm = xtm;
   g_GlobalInfo.bdmfWan = wan;
      
   return rc;
   
}  /* runner_xtm_objects_init() */


/*---------------------------------------------------------------------------
 * void runner_xtm_objects_uninit(void)
 * Description:
 *    Uninitialize all the runner xtm bdmf objects:
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void runner_xtm_objects_uninit(void)
{
   runner_tx_queues_uninit();
   
   if (g_GlobalInfo.bdmfWan != NULL)
   {
      bdmf_destroy(g_GlobalInfo.bdmfWan);
      g_GlobalInfo.bdmfWan = NULL;
   }
   if (g_GlobalInfo.bdmfXtm != NULL)
   {
      bdmf_destroy(g_GlobalInfo.bdmfXtm);
      g_GlobalInfo.bdmfXtm = NULL;
   }
}  /* runner_xtm_objects_uninit() */


/*---------------------------------------------------------------------------
 * int runner_tx_queues_init(bdmf_object_handle xtm)
 * Description:
 *    Initialize bdmf objects for 16 xtm tx queues.
 *    In the Runner system, SAR BBH has 16 channels, each corresponds to an
 *    iudma tx channel (queue). For each BBH channel, an xtmchannel (tcont)
 *    object associated with an egress_tm and one tx queue is created.
 *    The configuration is as below:
 *       BBH_Channel0----xtmchannel0----egress_tm0----queue0
 *       BBH_Channel1----xtmchannel1----egress_tm1----queue1
 *       BBH_Channel2----xtmchannel2----egress_tm2----queue2
 *       .....................................
 *       .....................................
 *       BBH_Channel15----xtmchannel15----egress_tm15----queue15
 * Returns:
 *    0 or bdmf error code
 *---------------------------------------------------------------------------
 */
static int runner_tx_queues_init(bdmf_object_handle xtm)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   
   int rc = 0;
   
   bdmf_object_handle   xtmchannel;
   bdmf_object_handle   tm;
   int queueIdx;

   if (xtm == NULL)
      return -EFAULT;
   
   for (queueIdx = 0; queueIdx < MAX_TRANSMIT_QUEUES; queueIdx++)
   {
      BDMF_MATTR(xtmchannel_attr, rdpa_xtmchannel_drv());
      BDMF_MATTR(tm_attr, rdpa_egress_tm_drv());

      /* create xtmchannel for this tx queue */
      /* set xtmchannel index as queueIdx */
      rc = rdpa_xtmchannel_index_set(xtmchannel_attr, queueIdx);
      rc = rc? rc : bdmf_new_and_set(rdpa_xtmchannel_drv(), xtm, xtmchannel_attr, &xtmchannel);
      if (rc) break;
      pGi->txBdmfObjs[queueIdx].xtmchannel = xtmchannel;
   
      /* create egress tm for this xtmchannel */
      rc = rdpa_egress_tm_dir_set(tm_attr, rdpa_dir_us);
      rc = rc? rc : rdpa_egress_tm_level_set(tm_attr, rdpa_tm_level_queue);
      rc = rc? rc : rdpa_egress_tm_mode_set(tm_attr, rdpa_tm_sched_sp);
      rc = rc? rc : bdmf_new_and_set(rdpa_egress_tm_drv(), xtmchannel, tm_attr, &tm);
      if (rc) break;
      pGi->txBdmfObjs[queueIdx].egress_tm = tm;

      /* Set xtmchannel/egress_tm attribute */
      rc = rc? rc : rdpa_xtmchannel_egress_tm_set(xtmchannel, tm);
      if (rc) break;
   }

   if (rc)
      runner_tx_queues_uninit();
      
   return rc;
   
}  /* runner_tx_queues_init() */


/*---------------------------------------------------------------------------
 * void runner_tx_queues_uninit(void)
 * Description:
 *    Uninitialize all runner xtm tx queues by destroying the xtmchannel object,
 *    the egress_tm object and the xtmflow objects associated with each queue.
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void runner_tx_queues_uninit(void)
{
   PBCMXTMRT_GLOBAL_INFO   pGi = &g_GlobalInfo;
   bdmf_object_handle      xtmchannel, egress_tm, xtmflow;
      
   int i, j;

   for (i = 0; i < MAX_TRANSMIT_QUEUES; i++)
   {
      for (j = 0; j < MAX_CIRCUIT_TYPES; j++)
      {
         xtmflow = pGi->txBdmfObjs[i].xtmflow[j];
         if (xtmflow != NULL)
         {
            bdmf_destroy(xtmflow);
            pGi->txBdmfObjs[i].xtmflow[j] = NULL;
         }
      }
      
      egress_tm = pGi->txBdmfObjs[i].egress_tm;
      if (egress_tm != NULL)
      {
         bdmf_destroy(egress_tm);
         pGi->txBdmfObjs[i].egress_tm = NULL;
      }
         
      xtmchannel = pGi->txBdmfObjs[i].xtmchannel;
      if (xtmchannel != NULL)
      {
         bdmf_destroy(xtmchannel);
         pGi->txBdmfObjs[i].xtmchannel = NULL;
      }
   }
}  /* runner_tx_queues_uninit() */


/*---------------------------------------------------------------------------
 * int cfg_xtmflows(UINT32 queueIdx, UINT32 trafficType, UINT32 hdrType, UINT8 vcid)
 * Description:
 *    This function configures rdpa xtmflows for a tx queue. One xtmflow is
 *    created for each XTM circuit type. 
 * Returns:
 *    0 or bdmf error code
 *---------------------------------------------------------------------------
 */
static int cfg_xtmflows(UINT32 queueIdx, UINT32 trafficType, UINT32 hdrType, UINT8 vcid, UINT32 ulTxPafEnabled)
{
   int      rc   = 0;
   UINT32   flag = CNI_USE_ALT_FSTAT | CNI_HW_ADD_HEADER;
   
   BDMF_MATTR(packet_attr, rdpa_xtmflow_drv());
   
   if ((trafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM)
   {
      BDMF_MATTR(f4_seg_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(f4_e2e_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(f5_seg_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(f5_e2e_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(asm_p0_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(asm_p1_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(asm_p2_attr, rdpa_xtmflow_drv());
      BDMF_MATTR(asm_p3_attr, rdpa_xtmflow_drv());
      
      UINT32 packet_fstat = calc_xtmflow_fstat(XCT_AAL5, hdrType, vcid, flag);
      UINT32 f4_seg_fstat = calc_xtmflow_fstat(XCT_OAM_F4_SEG, 0, vcid, flag);
      UINT32 f4_e2e_fstat = calc_xtmflow_fstat(XCT_OAM_F4_E2E, 0, vcid, flag);
      UINT32 f5_seg_fstat = calc_xtmflow_fstat(XCT_OAM_F5_SEG, 0, vcid, flag);
      UINT32 f5_e2e_fstat = calc_xtmflow_fstat(XCT_OAM_F5_E2E, 0, vcid, flag);
      UINT32 asm_p0_fstat = calc_xtmflow_fstat(XCT_ASM_P0,     0, vcid, flag);
      UINT32 asm_p1_fstat = calc_xtmflow_fstat(XCT_ASM_P1,     0, vcid, flag);
      UINT32 asm_p2_fstat = calc_xtmflow_fstat(XCT_ASM_P2,     0, vcid, flag);
      UINT32 asm_p3_fstat = calc_xtmflow_fstat(XCT_ASM_P3,     0, vcid, flag);
      
      rc = rc? rc : add_xtmflow(packet_attr, XCT_AAL5,       queueIdx, packet_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(f4_seg_attr, XCT_OAM_F4_SEG, queueIdx, f4_seg_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(f4_e2e_attr, XCT_OAM_F4_E2E, queueIdx, f4_e2e_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(f5_seg_attr, XCT_OAM_F5_SEG, queueIdx, f5_seg_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(f5_e2e_attr, XCT_OAM_F5_E2E, queueIdx, f5_e2e_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(asm_p0_attr, XCT_ASM_P0,     queueIdx, asm_p0_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(asm_p1_attr, XCT_ASM_P1,     queueIdx, asm_p1_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(asm_p2_attr, XCT_ASM_P2,     queueIdx, asm_p2_fstat, BC_PTM_BONDING_DISABLE);
      rc = rc? rc : add_xtmflow(asm_p3_attr, XCT_ASM_P3,     queueIdx, asm_p3_fstat, BC_PTM_BONDING_DISABLE);
   }
   else
   {
      UINT32 packet_fstat = calc_xtmflow_fstat(XCT_PTM, 0, 0, flag);

      if ((trafficType == TRAFFIC_TYPE_PTM_BONDED) && ulTxPafEnabled) {
         printk ("bcmxtmrt: Traffic Type is PTM_Bonded. TxPAF is enabled. Enable RDP-US-Bonding. \n") ;
         rc = rc? rc : add_xtmflow(packet_attr, XCT_PTM, queueIdx, packet_fstat, BC_PTM_BONDING_ENABLE) ;
      }
      else {
         printk ("bcmxtmrt: Traffic Type is %d.  TxPAF control-%d.  Disable RDP-US-Bonding. \n", (unsigned int) trafficType, (unsigned int) ulTxPafEnabled) ;
         rc = rc? rc : add_xtmflow(packet_attr, XCT_PTM, queueIdx, packet_fstat, BC_PTM_BONDING_DISABLE) ;
      }
   }
   
   return rc;
   
}  /* cfg_xtmflows() */


/*---------------------------------------------------------------------------
 * int add_xtmflow(bdmf_object_handle attrs, UINT32 ctType, UINT32 queueIdx,
 *                 UINT32 fstat, int ptmBonding)
 * Description:
 *    This function adds a rdpa xtmflow for the xtmchannel object.
 * Returns:
 *    0 or bdmf error code
 *---------------------------------------------------------------------------
 */
static int add_xtmflow(bdmf_object_handle attrs, UINT32 ctType, UINT32 queueIdx,
                       UINT32 fstat, int ptmBonding)
{
   int   rc = 0;
   bdmf_number             index;
   bdmf_object_handle      xtmflow;
   rdpa_xtmflow_us_cfg_t   us_cfg;

   /* set xtmflow index */
   index = (MAX_CIRCUIT_TYPES * queueIdx) + ctType;
   rc = rdpa_xtmflow_index_set(attrs, index);
   
   /* set xtmflow fstat */
   rc = rc? rc : rdpa_xtmflow_fstat_set(attrs, fstat);
   
   /* don't need to set ds_cfg */
   
   /* set us_cfg */
   us_cfg.xtmchannel = g_GlobalInfo.txBdmfObjs[queueIdx].xtmchannel;

   rc = rc? rc : rdpa_xtmflow_us_cfg_set(attrs, &us_cfg);
   
   /* set xtmflow ptmBonding */
   rc = rc? rc : rdpa_xtmflow_ptmBonding_set(attrs, ptmBonding);
   
   /* create wan flow */
   rc = rc? rc : bdmf_new_and_set(rdpa_xtmflow_drv(), NULL, attrs, &xtmflow);
   
   if (rc == 0)
   {
      g_GlobalInfo.txBdmfObjs[queueIdx].xtmflow[ctType] = xtmflow;
   }
   
   return rc; 
   
}  /* add_xtmflow() */


/*---------------------------------------------------------------------------
 * UINT32 calc_xtmflow_fstat(UINT32 ctType, UINT32 hdrType, UINT32 vcid,
 *                           UINT32 flag)
 * Description:
 *    This function returns the xtmflow fstat that has the same format
 *    as the tx dma buffer descriptor frame status word defined for
 *    BCM63168 SAR.
 * Returns:
 *    xtmflow fstat
 *---------------------------------------------------------------------------
 */
static UINT32 calc_xtmflow_fstat(UINT32 ctType, UINT32 hdrType, UINT32 vcid,
                                 UINT32 flag)
{
   UINT32 fstat;
   
   fstat = ctType << FSTAT_CT_SHIFT;

   switch (ctType)
   {
   case XCT_AAL5:
      fstat |= vcid;
      if (flag & CNI_USE_ALT_FSTAT)
      {
         fstat |= FSTAT_MODE_COMMON;
         if (HT_LEN(hdrType) && (flag & CNI_HW_ADD_HEADER))
         {
            fstat |= FSTAT_COMMON_INS_HDR_EN |
                     ((HT_TYPE(hdrType) - 1) << FSTAT_COMMON_HDR_INDEX_SHIFT);
         }
      }
      break;
      
   case XCT_PTM:
      fstat |= FSTAT_PTM_ENET_FCS | FSTAT_PTM_CRC;
      break;
      
   default:    /* atm cells */
      fstat |= vcid;
      if (flag & CNI_USE_ALT_FSTAT)
      {
         fstat |= FSTAT_MODE_COMMON;
      }
      break;
   }
   
   return fstat;
       
}  /* calc_xtmflow_fstat() */


