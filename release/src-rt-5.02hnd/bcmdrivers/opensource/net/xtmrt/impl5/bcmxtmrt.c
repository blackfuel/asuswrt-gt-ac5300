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
/**************************************************************************
 * File Name  : bcmxtmrt.c
 *
 * Description: This file implements BCM63x68 ATM/PTM network device driver
 *              runtime processing - sending and receiving data.
 ***************************************************************************/

/* Includes. */

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
#include <linux/if_vlan.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/atmppp.h>
#include <linux/blog.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ip.h>
#include <linux/if_pppox.h>
#include <bcmtypes.h>
#include <bcm_map_part.h>
#include "bcmnet.h"
#include "bcmxtmcfg.h"
#include "bcmxtmrt.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>
#include <linux/kthread.h>
#include <linux/bcm_realtime.h>
#include "bcmxtmrtimpl.h"
#if defined(CONFIG_BCM96318) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(SUPPORT_631XX_TX_RX_IUDMA)
#include "bcmPktDma.h"
#include "xtmrt_dma.h"
#else
#include "xtmrt_runner.h"
#endif
#if (defined(CONFIG_BCM_SPDSVC) || defined(CONFIG_BCM_SPDSVC_MODULE))
#include <linux/bcm_log.h>
#include "spdsvc_defs.h"
static bcmFun_t *xtmrt_spdsvc_transmit = NULL;
static int speed_service_check (pNBuff_t pNBuf, UINT32 phyOverhead) ;
#endif

/* VLAN TPIDs that need to be checked
   ETH_P_8021Q  0x8100
   ETH_P_8021AD 0x88A8
   ETH_P_QINQ1  0x9100
   ETH_P_QINQ2  0x9200
 */
#define BCM_VLAN_TPID_CHECK(x) ( (x) == htons(ETH_P_8021Q) \
                                || (x) == htons(ETH_P_8021AD)  \
                             /* || (x) == htons(ETH_P_QINQ1) */\
                             /* || (x) == htons(ETH_P_QINQ2) */)

#define VER_STR   "v" XTMRT_VERSION


/**** Externs ****/



/**** Globals ****/
BCMXTMRT_GLOBAL_INFO g_GlobalInfo;


/**** Prototypes ****/

static int __init bcmxtmrt_init(void);
static void bcmxtmrt_cleanup(void);
#ifdef CONFIG_PPP
static int bcmxtmrt_atm_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int bcmxtmrt_pppoatm_send(struct ppp_channel *pChan,struct sk_buff *skb);
#endif
static int bcmxtmrt_atmdev_open(struct atm_vcc *pVcc);
static void bcmxtmrt_atmdev_close(struct atm_vcc *pVcc);
static int bcmxtmrt_atmdev_send(struct atm_vcc *pVcc, struct sk_buff *skb);
static PBCMXTMRT_DEV_CONTEXT FindDevCtx(short vpi, int vci);

static pNBuff_t AddRfc2684Hdr(pNBuff_t pNBuf, UINT32 hdrType);
static void MirrorPacket(char *mirrorIntf, pNBuff_t pNBuf);

#ifdef CONFIG_BLOG
static int bcmxtmrt_get_xmit_channel(void *dev, int channel, unsigned uMark);
#endif

static int bcm63xx_xtmrt_rx_thread(void *arg);



/**** Statics ****/

#ifdef CONFIG_PPP
static struct atm_ioctl g_PppoAtmIoctlOps =
   {
      .ioctl      = bcmxtmrt_atm_ioctl,
   };
static struct ppp_channel_ops g_PppoAtmOps =
   {
      .start_xmit = bcmxtmrt_pppoatm_send
   };
#endif
static const struct atmdev_ops g_AtmDevOps =
   {
      .open       = bcmxtmrt_atmdev_open,
      .close      = bcmxtmrt_atmdev_close,
      .send       = bcmxtmrt_atmdev_send,
   };
    
static UINT32 gs_ulLogPorts[] = {0, 1, 2, 3};

#define PORT_PHYS_TO_LOG(PP) gs_ulLogPorts[PP]


/*---------------------------------------------------------------------------
 * int bcmxtmrt_init(void)
 * Description:
 *    Called when the driver is loaded.
 * Returns:
 *    0 or error status
 *---------------------------------------------------------------------------
 */
static int __init bcmxtmrt_init(void)
{
   UINT16 usChipId  = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
   UINT16 usChipRev = (PERF->RevID & REV_ID_MASK);

   printk(CARDNAME ": Broadcom BCM%X%X ATM/PTM Network Device ",
          usChipId, usChipRev);
   printk(VER_STR "\n");

   memset(&g_GlobalInfo, 0x00, sizeof(g_GlobalInfo));

   g_GlobalInfo.ulChipRev = PERF->RevID;

#ifdef CONFIG_PPP   
   register_atm_ioctl(&g_PppoAtmIoctlOps);
#endif
   
   g_GlobalInfo.pAtmDev = atm_dev_register("bcmxtmrt_atmdev", NULL,
                                           &g_AtmDevOps, -1, NULL);
   if (g_GlobalInfo.pAtmDev)
   {
      g_GlobalInfo.pAtmDev->ci_range.vpi_bits = 12;
      g_GlobalInfo.pAtmDev->ci_range.vci_bits = 16;
   }
   
   bcmxapi_add_proc_files();
   bcmxapi_module_init();

#ifdef CONFIG_BLOG
   blog_xtm_get_tx_chan_fn = bcmxtmrt_get_xmit_channel;
#endif

   /* Create a thread to do the rx processing work. */
   g_GlobalInfo.rx_work_avail = 0;
   init_waitqueue_head(&g_GlobalInfo.rx_thread_wqh);
   g_GlobalInfo.rx_thread = kthread_create(bcm63xx_xtmrt_rx_thread, NULL, "bcmxtm_rx");

  /* Binding is required for the following reasons */
   /* 1. 63381 rate issues.
   ** 2. 63381/63268 platform network layer config issues with high
   ** traffic/link status changes. - will be done later.
   ** 3. 63138 buffers not available with high traffic rate.
   ** Free task for SKBs should be given more chance in other cores
   ** It is done through eid_bcm_kthreads.
   **/

   wake_up_process(g_GlobalInfo.rx_thread);

#if (defined(CONFIG_BCM_SPDSVC) || defined(CONFIG_BCM_SPDSVC_MODULE))
   xtmrt_spdsvc_transmit = bcmFun_get(BCM_FUN_ID_SPDSVC_TRANSMIT);
   BCM_ASSERT(xtmrt_spdsvc_transmit != NULL);
#endif

   return 0;
    
}  /* bcmxtmrt_init() */


/*---------------------------------------------------------------------------
 * void bcmxtmrt_cleanup(void)
 * Description:
 *    Called when the driver is unloaded.
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void bcmxtmrt_cleanup(void)
{
   /* Stop RX thread first so it won't touch anything being deallocated */
   kthread_stop(g_GlobalInfo.rx_thread);

   /* Continue cleanup - ATM stuff */
#ifdef CONFIG_PPP  
   deregister_atm_ioctl(&g_PppoAtmIoctlOps);
#endif
   if (g_GlobalInfo.pAtmDev)
   {
      atm_dev_deregister(g_GlobalInfo.pAtmDev);
      g_GlobalInfo.pAtmDev = NULL;
   }
    
   /* Finally, get rid of temp files and do API cleanup */
   bcmxapi_del_proc_files();
   bcmxapi_module_cleanup();
    
#ifdef CONFIG_BLOG
   blog_xtm_get_tx_chan_fn = (blog_xtm_get_tx_chan_t)NULL;
#endif

}  /* bcmxtmrt_cleanup() */


#if defined(CONFIG_PPP)
/*---------------------------------------------------------------------------
 * int bcmxtmrt_atm_ioctl(struct socket *sock, UINT32 cmd, UINT32 arg)
 * Description:
 *    Driver atm ioctl entry point.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
static int bcmxtmrt_atm_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
    int nRet = -ENOIOCTLCMD;
   PBCMXTMRT_DEV_CONTEXT pDevCtx;
   struct atm_vcc *pAtmVcc = ATM_SD(sock);
   void __user *argp = (void __user *)arg;
   atm_backend_t b;

   switch (cmd)
   {
   case ATM_SETBACKEND:
      if (get_user(b, (atm_backend_t __user *) argp) == 0)
      {
         switch (b)
         {
         case ATM_BACKEND_PPP_BCM:
            if ((pDevCtx = FindDevCtx(pAtmVcc->vpi, pAtmVcc->vci)) != NULL &&
                pDevCtx->Chan.private == NULL)
            {
               pDevCtx->Chan.private = pDevCtx->pDev;
               pDevCtx->Chan.ops     = &g_PppoAtmOps;
               pDevCtx->Chan.mtu     = 1500; /* TBD. Calc value. */
               pAtmVcc->user_back    = pDevCtx;
               if (ppp_register_channel(&pDevCtx->Chan) == 0)
                  nRet = 0;
               else
                  nRet = -EFAULT;
            }
            else
               nRet = (pDevCtx) ? 0 : -EFAULT;
            break;

         case ATM_BACKEND_PPP_BCM_DISCONN:
            /* This is a patch for PPP reconnection.
             * ppp daemon wants us to send out an LCP termination request
             * to let the BRAS ppp server terminate the old ppp connection.
             */
            if ((pDevCtx = FindDevCtx(pAtmVcc->vpi, pAtmVcc->vci)) != NULL)
            {
               struct sk_buff *skb;
               int size = 6;
               int eff  = (size+3) & ~3; /* align to word boundary */

               while (!(skb = alloc_skb(eff, GFP_ATOMIC)))
                  schedule();

               skb->dev = NULL; /* for paths shared with net_device interfaces */
               skb_put(skb, size);

               skb->data[0] = 0xc0;  /* PPP_LCP == 0xc021 */
               skb->data[1] = 0x21;
               skb->data[2] = 0x05;  /* TERMREQ == 5 */
               skb->data[3] = 0x02;  /* id == 2 */
               skb->data[4] = 0x00;  /* HEADERLEN == 4 */
               skb->data[5] = 0x04;

               if (eff > size)
                  memset(skb->data+size,0,eff-size);

               nRet = bcmxtmrt_xmit(SKBUFF_2_PNBUFF(skb), pDevCtx->pDev);
            }
            else
               nRet = -EFAULT;
            break;

         case ATM_BACKEND_PPP_BCM_CLOSE_DEV:
            if ((pDevCtx = FindDevCtx(pAtmVcc->vpi, pAtmVcc->vci)) != NULL)
            {
               bcmxtmrt_pppoatm_send(&pDevCtx->Chan, NULL);
               ppp_unregister_channel(&pDevCtx->Chan);
               pDevCtx->Chan.private = NULL;
            }
            nRet = 0;
            break;

         default:
            break;
         }
      }
      else
         nRet = -EFAULT;
      break;

   case PPPIOCGCHAN:
      if ((pDevCtx = FindDevCtx(pAtmVcc->vpi, pAtmVcc->vci)) != NULL)
      {
         nRet = put_user(ppp_channel_index(&pDevCtx->Chan),
                         (int __user *)argp) ? -EFAULT : 0;
      }
      else
         nRet = -EFAULT;
      break;

   case PPPIOCGUNIT:
      if ((pDevCtx = FindDevCtx(pAtmVcc->vpi, pAtmVcc->vci)) != NULL)
      {
         nRet = put_user(ppp_unit_number(&pDevCtx->Chan),
                         (int __user *)argp) ? -EFAULT : 0;
      }
      else
         nRet = -EFAULT;
      break;
   default:
      break;
   }

   return (nRet);
    
}  /* bcmxtmrt_atm_ioctl() */
#endif     


/*---------------------------------------------------------------------------
 * int bcmxtmrt_atmdev_open(struct atm_vcc *pVcc)
 * Description:
 *    ATM device open
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
static int bcmxtmrt_atmdev_open(struct atm_vcc *pVcc)
{
   set_bit(ATM_VF_READY,&pVcc->flags);
   return 0;
   
}  /* bcmxtmrt_atmdev_open() */


/*---------------------------------------------------------------------------
 * void bcmxtmrt_atmdev_close(struct atm_vcc *pVcc)
 * Description:
 *    ATM device open
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
static void bcmxtmrt_atmdev_close(struct atm_vcc *pVcc)
{
   clear_bit(ATM_VF_READY,&pVcc->flags);
   clear_bit(ATM_VF_ADDR,&pVcc->flags);
   
}  /* bcmxtmrt_atmdev_close() */


/*---------------------------------------------------------------------------
 * int bcmxtmrt_atmdev_send(struct atm_vcc *pVcc, struct sk_buff *skb)
 * Description:
 *    send data
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
static int bcmxtmrt_atmdev_send(struct atm_vcc *pVcc, struct sk_buff *skb)
{
   int nRet;
   PBCMXTMRT_DEV_CONTEXT pDevCtx = FindDevCtx(pVcc->vpi, pVcc->vci);

   if (pDevCtx)
      nRet = bcmxtmrt_xmit(SKBUFF_2_PNBUFF(skb), pDevCtx->pDev);
   else
      nRet = -EIO;

    return (nRet);
    
}  /* bcmxtmrt_atmdev_send() */

#ifdef CONFIG_PPP
/*---------------------------------------------------------------------------
 * int bcmxtmrt_pppoatm_send(struct ppp_channel *pChan, struct sk_buff *skb)
 * Description:
 *    Called by the PPP driver to send data.
 * Returns:
 *    1 if successful or error status
 *---------------------------------------------------------------------------
 */
static int bcmxtmrt_pppoatm_send(struct ppp_channel *pChan, struct sk_buff *skb)
{
   if (skb)
      skb->dev = (struct net_device *)pChan->private;
   bcmxtmrt_xmit(SKBUFF_2_PNBUFF(skb), (struct net_device *)pChan->private);
   
   return 1;   /* Note: must return 1 for success */
   
}  /* bcmxtmrt_pppoatm_send() */
#endif

/*---------------------------------------------------------------------------
 * PBCMXTMRT_DEV_CONTEXT FindDevCtx(short vpi, int vci)
 * Description:
 *    Finds a device context structure for a VCC.
 * Returns:
 *    Pointer to a device context structure or NULL.
 *---------------------------------------------------------------------------
 */
static PBCMXTMRT_DEV_CONTEXT FindDevCtx(short vpi, int vci)
{
   PBCMXTMRT_DEV_CONTEXT pDevCtx = NULL;
   PBCMXTMRT_GLOBAL_INFO pGi     = &g_GlobalInfo;
   UINT32 i;

   for (i = 0; i < MAX_DEV_CTXS; i++)
   {
      if ((pDevCtx = pGi->pDevCtxs[i]) != NULL)
      {
         if (pDevCtx->Addr.u.Vcc.usVpi == vpi &&
             pDevCtx->Addr.u.Vcc.usVci == vci)
         {
            break;
         }
         pDevCtx = NULL;
      }
   }
   return (pDevCtx);
    
}  /* FindDevCtx() */


/*---------------------------------------------------------------------------
 * BcmPktDma_XtmTxDma *find_xmit_channel(PBCMXTMRT_DEV_CONTEXT pDevCtx,
 *                                       UINT32 uMark)
 * Description:
 *    Finds the XTM TX iudma channel based on the skb->mark.
 * Returns:
 *    pointer to txdma structure
 *---------------------------------------------------------------------------
 */
static inline BcmPktDma_XtmTxDma *find_xmit_channel(PBCMXTMRT_DEV_CONTEXT pDevCtx,
                                                    UINT32 uMark)
{
   BcmPktDma_XtmTxDma *txdma = NULL;

#ifdef CONFIG_NETFILTER
   /* See if this is a classified flow */
   if (SKBMARK_GET_FLOW_ID(uMark))
   {
      /* Non-zero flow id implies classified packet.
       * Find tx queue based on its qid.
       */
      /* For ATM classified packet,
       *   bit 3-0 of nfmark is the queue id (0 to 15).
       *   bit 4   of nfmark is the DSL latency, 0=PATH0, 1=PATH1
       *
       * For PTM classified packet,
       *   bit 2-0 of nfmark is the queue id (0 to 7).
       *   bit 3   of nfmark is the PTM priority, 0=LOW, 1=HIGH
       *   bit 4   of nfmark is the DSL latency, 0=PATH0, 1=PATH1
       */
      /* Classified packet. Find tx queue based on its queue id. */
      if ((pDevCtx->Addr.ulTrafficType == TRAFFIC_TYPE_PTM) ||
          (pDevCtx->Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED))
      {
         /* For PTM, bit 2-0 of the 32-bit nfmark is the queue id. */
         txdma = pDevCtx->pTxQids[uMark & 0x7];
      }
      else
      {
         /* For ATM, bit 3-0 of the 32-bit nfmark is the queue id. */
         txdma = pDevCtx->pTxQids[uMark & 0xf];
      }
   }
   else
   {
      /* Flow id 0 implies unclassified packet.
       * Find tx queue based on its subpriority.
       */
      /* There are 2 types of unclassified packet flow.
       *   1) Protocol control packets originated locally from CPE.
       *      Such packets are marked the highest subpriority (7),
       *      and will be sent to the highest subpriority queue of
       *      the connection.
       *   2) Packets that do not match any QoS classification rule.
       *      Such packets will be sent to the first queue of matching
       *      subpriority.
       */
      /* For unclassified packet,
       *   bit 2-0 of nfmark is the subpriority (0 to 7).
       *   bit 3   of nfmark is the PTM priority, 0=LOW, 1=HIGH
       *   bit 4   of nfmark is the DSL latency, 0=PATH0, 1=PATH1
       */

      UINT32 subprio = (uMark & 0x7);
      
      /* If the subpriority is the highest (7), use the existing
       * highest priority queue.
       */
      if (subprio == 0x7)
         txdma = pDevCtx->pHighestPrio;
      else
      {
         int   qid;
        
         /* Find the txdma channel configured with the subpriority */
         for (qid = 0; qid < MAX_TRANSMIT_QUEUES; qid++)
         {
            if (pDevCtx->pTxQids[qid] && (subprio == pDevCtx->pTxQids[qid]->ulSubPriority))
            {
               txdma = pDevCtx->pTxQids[qid];
               break;
            }
         }
      }
   }
#endif

   /* If a transmit queue was not found or the queue was disabled,
    * use the default queue (qid 0).
    */
   if (txdma == NULL || txdma->txEnabled == 0)
      txdma = pDevCtx->txdma[0]; /* the default queue */

   return txdma;
    
}  /* find_xmit_channel() */

#if (defined(CONFIG_BCM_SPDSVC) || defined(CONFIG_BCM_SPDSVC_MODULE))
#define XTMRT_PTM_HEADER_LEN          8   /* 4 (FCS) + 2 (CRC) + 1 (SOF) + 1 (Last CW byte count) */
#define XTMRT_PTM_BOND_FRAG_LEN       4   /* 2 (CRC) + 2 (FragHdr) */
#define XTMRT_PTM_CODEWORD_LEN        64
#define XTMRT_PTM_CODEWORD_SHIFT      6
#define XTMRT_PTM_CODEWORD_SYNC_LEN   1
#define XTMRT_ATM_AAL5_TRAILER_LEN    8
#define XTMRT_ATM_CELL_HEADER_LEN     5
#define XTMRT_ATM_CELL_PAYLOAD_LEN    48

/*---------------------------------------------------------------------------
 * UINT32 xtmOverhead(UINT16 bufStatus, UINT32 hdrType, UINT32 len)
 *
 * Description:
 *    Calculates the overhead for ATM and PTM modes
 * Returns:
 *    The overhead length
 *---------------------------------------------------------------------------
 */
static inline UINT32 xtmOverhead(UINT16 bufStatus, UINT32 hdrType, UINT32 len, UINT32 rfc2684Type, UINT32 trafficType)
{
    UINT32 ctType = ((UINT32)bufStatus & FSTAT_CT_MASK) >> FSTAT_CT_SHIFT;
    UINT32 overhead;
    UINT32 totalBytes ;

    if(ctType == XCT_PTM)
    {
       UINT32 ptmFrame = len + XTMRT_PTM_HEADER_LEN;
       UINT32 codewords = (ptmFrame + XTMRT_PTM_CODEWORD_LEN - 1) >> XTMRT_PTM_CODEWORD_SHIFT;

       totalBytes = codewords * (XTMRT_PTM_CODEWORD_LEN + XTMRT_PTM_CODEWORD_SYNC_LEN);

       if (trafficType == TRAFFIC_TYPE_PTM_BONDED) {
          totalBytes += (len <= XTMRT_PTM_BOND_TX_MAX_FRAGMENT_SIZE) ? (1*XTMRT_PTM_BOND_FRAG_LEN) : (3*XTMRT_PTM_BOND_FRAG_LEN) ;
       }

       overhead = totalBytes - len;

       //        printk("PTM: len %ld, ptmFrame %ld, codewords %ld, overhead %ld\n", len, ptmFrame, codewords, overhead);
    }
    else /* ATM */
    {
        UINT32 packetLen = (rfc2684Type == RFC2684_NONE) ? (len) : (len - HT_LEN(hdrType));
        UINT32 atmFrame = packetLen + HT_LEN(hdrType) + XTMRT_ATM_AAL5_TRAILER_LEN;
        UINT32 cells = atmFrame / XTMRT_ATM_CELL_PAYLOAD_LEN;

        if((cells * XTMRT_ATM_CELL_PAYLOAD_LEN) < atmFrame)
        {
            cells++;
        }

        totalBytes = cells * (XTMRT_ATM_CELL_HEADER_LEN + XTMRT_ATM_CELL_PAYLOAD_LEN);

        overhead = totalBytes - packetLen;

//        printk("ATM: len %ld, packetLen %ld, atmFrame %ld, cells %ld, overhead %ld\n", len, packetLen, atmFrame, cells, overhead);
    }

    return overhead;
}

static int speed_service_check (pNBuff_t pNBuf, UINT32 phyOverhead)
{
   spdsvcHook_transmit_t spdsvc_transmit;

   spdsvc_transmit.pNBuff = pNBuf;
   spdsvc_transmit.dev = NULL;
   spdsvc_transmit.header_type = SPDSVC_HEADER_TYPE_ETH;
   spdsvc_transmit.phy_overhead = phyOverhead;

   return (xtmrt_spdsvc_transmit(&spdsvc_transmit)) ;
}

#else

#define xtmOverhead(_bufStatus, _hdrType, _len, _rfc2684Type, _trafficType)  0
#define speed_service_check(_pNBuf, _phyOverhead)             0

#endif

/*---------------------------------------------------------------------------
 * int bcmxtmrt_xmit(pNBuff_t pNBuf, struct net_device *dev)
 * Description:
 *    Check for transmitted packets to free and, if skb is
 *    non-NULL, transmit a packet. Transmit may be invoked for
 *    a packet originating from the network stack or from a
 *    packet received from another interface.
 * Returns:
 *    0 if successful or error status
 *---------------------------------------------------------------------------
 */
int bcmxtmrt_xmit(pNBuff_t pNBuf, struct net_device *dev)
{
   int rc = -EIO;
   
   PBCMXTMRT_DEV_CONTEXT pDevCtx = netdev_priv(dev);
   spinlock_t           *xtmlock = &g_GlobalInfo.xtmlock_tx;
   UINT16 bufStatus;
   UINT8  *pData;
   UINT32 len, skbMark, skbPrio;
   UINT32 hdrType             = pDevCtx->ulHdrType;
   UINT32 rfc2684Type         = RFC2684_NONE;
   UINT32 isAtmCell           = 0;
   UINT32 txAvailable         = 0;
   struct sk_buff     *skb    = NULL; /* If pNBuf is sk_buff: protocol access */
   BcmPktDma_XtmTxDma *txdma  = NULL;
   int is_spdsvc_setup_packet = 0 ;

   spin_lock_bh(xtmlock);

   if (pDevCtx->ulLinkState != LINK_UP)
   {
      goto tx_drop;
   }
   
   /* Free packets that have been transmitted. */
   bcmxapi_free_xmit_packets(pDevCtx);

   if (nbuff_get_params(pNBuf, &pData, (uint32_t *)&len, (uint32_t *)&skbMark,
                        (uint32_t *)&skbPrio) == (void *)NULL)
   {
      /* pNBuf must be NULL. This call is for freeing the xmit packets. */
      goto unlock_done_xmit;
   }

   if (pDevCtx->ulTxQInfosSize == 0)
   {
      goto tx_drop;
   }
   
   if (IS_SKBUFF_PTR(pNBuf))
   {
      skb = PNBUFF_2_SKBUFF(pNBuf);

      if ((skb->protocol & htons(~FSTAT_CT_MASK)) == htons(SKB_PROTO_ATM_CELL) &&
          (pDevCtx->Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM)
      {
         isAtmCell = 1;
         
         /* Give the highest possible priority to oam cells */
         skbMark |= 0x7;
      }
      else 
      {
            unsigned char l3_offset = sizeof(struct ethhdr);
            struct vlan_hdr    *vlanhdr = (struct vlan_hdr *)(skb->data + l3_offset - sizeof(struct vlan_hdr));

            /* Skip over all the VLAN Tags */
            while ( BCM_VLAN_TPID_CHECK(vlanhdr->h_vlan_encapsulated_proto) )
            {
                vlanhdr = (struct vlan_hdr *)(skb->data + l3_offset);
                l3_offset +=  sizeof(struct vlan_hdr);
            }
            if (vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_ARP))
            {
                /* Give the highest possible priority to ARP packets */
                skbMark |= 0x7;
            }
            else if ( vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_PPP_DISC) )
            {
                /* Give the highest possible priority to PPP Discovery packets */
                skbMark |= 0x7;
            }
            else if ( vlanhdr->h_vlan_encapsulated_proto == htons(ETH_P_PPP_SES) )
            {
                struct pppoe_hdr *pppoe = (struct pppoe_hdr *)(skb->data + l3_offset);

                if ( ! (pppoe->tag[0].tag_type  == htons(PPP_IP) || pppoe->tag[0].tag_type  == htons(PPP_IPV6)) )
                {
                    /* If not carrying IP payload -- give highest possible proirity to PPP Control packet */
                    skbMark |= 0x7;
                }
            }
      }
      if (pDevCtx->Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)
         bcmxapi_blog_ptm_us_bonding (pDevCtx->ulTxPafEnabled, skb) ;
   }

   BCM_XTM_TX_DEBUG("XTM TX: pNBuf<0x%08x> skb<0x%08x> pData<0x%08x>\n", (int)pNBuf,(int)skb, (int)pData);

   if (pData != NULL)
      DUMP_PKT(pData, len);

   /* Find the transmit queue to send on. */
   txdma = find_xmit_channel(pDevCtx, skbMark);

   if (txdma && txdma->txEnabled == 1)
   {
       txAvailable = bcmxapi_xmit_available(txdma, skbMark); 
   }

   if (!txAvailable || !bcmxapi_queue_packet(txdma, isAtmCell))
   {
      /* Transmit queue is full.  Free the socket buffer.  Don't call
       * netif_stop_queue because this device may use more than one
       * queue.
       */
      goto tx_drop;
   }
   
   if (!(pDevCtx->ulFlags & CNI_HW_ADD_HEADER) && HT_LEN(hdrType) && !isAtmCell)
   {
      rfc2684Type = HT_TYPE(hdrType);
   }
   
   /* Calculate bufStatus for the packet */
   if ((pDevCtx->Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM)
   {
      bufStatus = pDevCtx->ucTxVcid;
      
      if (isAtmCell)
      {
         bufStatus |= (htons(skb->protocol) & FSTAT_CT_MASK);
         if (pDevCtx->ulFlags & CNI_USE_ALT_FSTAT)
         {
            bufStatus |= FSTAT_MODE_COMMON;
            bufStatus &= ~(FSTAT_COMMON_INS_HDR_EN |
                               FSTAT_COMMON_HDR_INDEX_MASK);
         }
      }
      else
      {
         bufStatus |= FSTAT_CT_AAL5;
         if (pDevCtx->ulFlags & CNI_USE_ALT_FSTAT)
         {
            bufStatus |= FSTAT_MODE_COMMON;
            if (HT_LEN(hdrType) && (pDevCtx->ulFlags & CNI_HW_ADD_HEADER))
            {
               bufStatus |= FSTAT_COMMON_INS_HDR_EN |
                                   ((HT_TYPE(hdrType) - 1) <<
                                   FSTAT_COMMON_HDR_INDEX_SHIFT);
            }
            else
            {
               bufStatus &= ~(FSTAT_COMMON_INS_HDR_EN |
                                    FSTAT_COMMON_HDR_INDEX_MASK);
            }
         }
      }
   }
   else
   {
      bufStatus = FSTAT_CT_PTM | FSTAT_PTM_ENET_FCS | FSTAT_PTM_CRC;
   }

#ifdef CONFIG_BLOG
   spin_unlock_bh(xtmlock);
   bcmxapi_blog_emit (pNBuf, dev, pDevCtx, txdma, rfc2684Type, bufStatus) ;
   spin_lock_bh(xtmlock);
#endif

   if (pDevCtx->szMirrorIntfOut[0] != '\0' &&
       !isAtmCell &&
       (hdrType == HT_PTM ||
        hdrType == HT_LLC_SNAP_ETHERNET ||
        hdrType == HT_VC_MUX_ETHERNET))
   {
      spin_unlock_bh(xtmlock);
      MirrorPacket(pDevCtx->szMirrorIntfOut, pNBuf);
      spin_lock_bh(xtmlock);
   }

   if (rfc2684Type)
   {
      pNBuf = AddRfc2684Hdr(pNBuf, hdrType);
      nbuff_get_context(pNBuf, &pData, (uint32_t *)&len);
   }
   
   /* pNBuf may have been changed by AddRfc2684Hdr. Update pData and len.  */

   if (len < ETH_ZLEN &&
       !isAtmCell &&
       (hdrType == HT_PTM ||
        hdrType == HT_LLC_SNAP_ETHERNET ||
        hdrType == HT_VC_MUX_ETHERNET))
   {
      nbuff_pad(pNBuf, ETH_ZLEN - len); /* Pad the buffer at the end, as SAR packet mode can not handle packets of lesser
                                         * size than the MinSize */
      nbuff_get_context(pNBuf, &pData, (uint32_t *)&len);
      len = ETH_ZLEN;

      nbuff_set_len (pNBuf, len) ;
   }

   /* Check Speed Service */

   is_spdsvc_setup_packet = speed_service_check (pNBuf, xtmOverhead(bufStatus, hdrType, len, rfc2684Type, pDevCtx->ulTrafficType)) ;

   if (is_spdsvc_setup_packet < 0) {
      pDevCtx->DevStats.tx_dropped++;
      goto unlock_done_xmit ;
   }

   if (pDevCtx->ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)
   {
      /* bit 3 of the 32-bit nfmark is the PTM priority, 0=LOW, 1=HIGH */
      UINT32 ptmPrioIdx = (skbMark >> 3) & 0x1;

      if (pDevCtx->ulPortDataMask == 0 ||
          !bcmxtmrt_ptmbond_add_hdr(pDevCtx->ulTxPafEnabled, &pNBuf, ptmPrioIdx))
      {
         goto tx_drop;
      }
      nbuff_get_context(pNBuf, &pData, (uint32_t *)&len);
   }
   
   /* pNBuf may have been changed by ptm bonding add header. Update pData and len.  */

   rc = bcmxapi_xmit_packet(&pNBuf, &pData, &len, txdma, txdma->ulDmaIndex,
                            bufStatus, skbMark, is_spdsvc_setup_packet);
   if (rc)
   {
#if defined (CONFIG_BCM963138) || defined (CONFIG_BCM963148)
      pDevCtx->DevStats.tx_dropped++;
      goto unlock_done_xmit ;
#else
      goto tx_drop;
#endif
   }

   /* Gather statistics.  */
   pDevCtx->DevStats.tx_packets++;
   pDevCtx->DevStats.tx_bytes += len;

   /* Now, determine extended statistics.  Is this an Ethernet packet? */
   if (hdrType == HT_PTM ||
       hdrType == HT_LLC_SNAP_ETHERNET || 
       hdrType == HT_VC_MUX_ETHERNET)
   {
      /* Yes, this is Ethernet.  Test for multicast packet */
      if (pData[0]  == 0x01)
      {
         /* Multicast packet - record statistics */
         pDevCtx->DevStats.tx_multicast_packets++;
         pDevCtx->DevStats.tx_multicast_bytes += len;                    
      }

      /* Test for broadcast packet */
      if (pData[0] == 0xFF)
      {
         /* Constant value to test for Ethernet broadcast address */
         const unsigned char pucEtherBroadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      
         /* Low byte indicates we might be broadcast - check against the rest */
         if (memcmp(pData, pucEtherBroadcastAddr, 5) == 0)
         {
            /* Broadcast packet - record statistics */
            pDevCtx->DevStats.rx_broadcast_packets++;
         }
      }
   }
   pDevCtx->pDev->trans_start = jiffies;
            
   goto unlock_done_xmit;

tx_drop:
   if (pNBuf)
   {
      nbuff_flushfree(pNBuf);
      pDevCtx->DevStats.tx_dropped++;
   }
      
unlock_done_xmit:
   spin_unlock_bh(xtmlock);

   return rc;
    
}  /* bcmxtmrt_xmit() */


/*---------------------------------------------------------------------------
 * pNBuff_t AddRfc2684Hdr(pNBuff_t pNBuf, UINT32 hdrType)
 * Description:
 *    Inserts the RFC2684 header to an ATM packet before transmitting it.
 *    Note that NBuf pointer may be modified by this function.
 * Returns:
 *    Modified NBuf pointer
 *---------------------------------------------------------------------------
 */
static pNBuff_t AddRfc2684Hdr(pNBuff_t pNBuf, UINT32 hdrType)
{
   UINT8 ucHdrs[][16] =
         {{},
         {0xAA, 0xAA, 0x03, 0x00, 0x80, 0xC2, 0x00, 0x07, 0x00, 0x00},
         {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00},
         {0xFE, 0xFE, 0x03, 0xCF},
         {0x00, 0x00}};
   int minheadroom = HT_LEN(hdrType);

   if (IS_SKBUFF_PTR(pNBuf))
   {
      struct sk_buff *skb = PNBUFF_2_SKBUFF(pNBuf);

      if (skb_headroom(skb) < minheadroom)
      {
         struct sk_buff *skb2 = skb_realloc_headroom(skb, minheadroom);

         if (skb2)
         {
            dev_kfree_skb_any(skb);
            skb = skb2;
         }
      }
      
      if (skb_headroom(skb) >= minheadroom)
      {
         UINT8 *pData = skb_push(skb, minheadroom);
         u16cpy(pData, ucHdrs[HT_TYPE(hdrType)], minheadroom);
      }
      else
      {
         printk(CARDNAME ": Failed to allocate SKB with enough headroom.\n");
      }
      pNBuf = SKBUFF_2_PNBUFF(skb);
   }
   else
   {
      struct fkbuff *fkb = PNBUFF_2_FKBUFF(pNBuf);
        
      if (fkb_headroom(fkb) >= minheadroom)
      {
         UINT8 *pData = fkb_push(fkb, minheadroom);
         u16cpy(pData, ucHdrs[HT_TYPE(hdrType)], minheadroom);
      }
      else
      {
         printk(CARDNAME ": FKB not enough headroom.\n");
      }
   }
   
   return pNBuf;
   
}  /* AddRfc2684Hdr() */


/*---------------------------------------------------------------------------
 * UINT32 bcmxtmrt_processRxPkt(PBCMXTMRT_DEV_CONTEXT pDevCtx, void *rxdma,
 *                              pNBuff_t pNBuf, UINT16 bufStatus, UINT32 len)
 * Description:
 *    Processes a received packet.
 *    Responsible for sending the packet up to the blog and network stack.
 * Returns:
 *    Status as the packet thro BLOG/NORMAL path.
 *---------------------------------------------------------------------------
 */
UINT32 bcmxtmrt_processRxPkt(PBCMXTMRT_DEV_CONTEXT pDevCtx, void *rxdma,
                             pNBuff_t pNBuf, int is_skbuff,
                             UINT16 bufStatus, UINT32 len)
{
   struct sk_buff *skb  = NULL;
   FkBuff_t       *pFkb = NULL;
   UINT32 hdrType       = pDevCtx->ulHdrType;
   UINT32 flags         = pDevCtx->ulFlags;
   UINT32 rfc2684_type  = RFC2684_NONE; /* blog.h: Rfc2684_t */
   UINT32 retStatus;
   UINT8  *pBuf;
   UINT8  *pData;
   int    delLen        = 0;
   int    trailerDelLen = 0;
#ifdef CONFIG_BLOG
   BlogAction_t blogAction;
#endif
#ifdef PHY_LOOPBACK
   char mac[6];
   char Ip[4];
   char port[2];
#endif

   if (is_skbuff)
   {
      /* packet is from RDPA cpu interface */
      skb   = PNBUFF_2_SKBUFF(pNBuf);
      pBuf  = skb->head;
      pData = skb->data;
   }
   else
   {
      /* packet is from FAP dqm or rxdma */
      pBuf  = PNBUFF_2_PBUF(pNBuf);
      pData = pBuf;
   }
   
#ifdef PHY_LOOPBACK
   memcpy(mac, pData, 6);
   memcpy(pData, pData+6, 6);
   memcpy(pData+6, mac, 6);
   memcpy(Ip, pData+30, 4);
   memcpy(pData+30, pData+26, 4);
   memcpy(pData+26, Ip, 4);
   memcpy(port, pData+36, 2);
   memcpy(pData+36, pData+34, 2);
   memcpy(pData+34, port, 2);
#endif

   //DUMP_PKT(pData, len);

   if ((flags & LSC_RAW_ENET_MODE) != 0)
      len -= ETH_FCS_LEN;

   if (hdrType == HT_PTM && (flags & CNI_HW_REMOVE_TRAILER) == 0)
   {
      if (len > (ETH_FCS_LEN+XTMRT_PTM_CRC_SIZE))
      {
         trailerDelLen = (ETH_FCS_LEN+XTMRT_PTM_CRC_SIZE);
         len          -= trailerDelLen;
      }
   }

   if ((flags & CNI_HW_REMOVE_HEADER) == 0)
   {
      delLen = HT_LEN(hdrType);

      /* For PTM flow, this will not take effect and hence so, for
       * bonding flow as well. So we do not need checks here to not
       * make it happen.
       */
      if (delLen > 0)
      {
         pData += delLen;
         len   -= delLen;
      }
      
      /* cannot be an AtmCell, also do not use delLen (bonding), recompute */
      if (HT_LEN(hdrType) > 0)
         rfc2684_type = HT_TYPE(hdrType); /* blog.h: Rfc2684_t */
   }

   //printk ("bcmxtmrt : Len = %d flags = %x \n", (int) len, (unsigned int) flags ) ;

   if (len < ETH_ZLEN)
      len = ETH_ZLEN;

   /* Record time of this RX */
   pDevCtx->pDev->last_rx = jiffies;
    
   /* Calculate total RX packets received */
   pDevCtx->DevStats.rx_packets++;
   pDevCtx->DevStats.rx_bytes += len;

   /* Now, determine extended statistics.  Is this an Ethernet packet? */
   if (hdrType == HT_PTM ||
       hdrType == HT_LLC_SNAP_ETHERNET ||
       hdrType == HT_VC_MUX_ETHERNET)
   {
      UINT8  bIsBcast = 0;  /* Flags a broadcast frame or packet */
      UINT8  bIsMcast = 0;  /* Flags a multicast frame or packet */        
      UINT16 *pusSessEthertypeId = (UINT16 *)(pData + ETH_ALEN + ETH_ALEN);
        
      /* Check for a multicast MAC address and increment multicast counters if so */
      if (is_multicast_ether_addr(pData))
      {
         /* Multicast Ethernet frame -- set flag */
         bIsMcast = 1;
      }         
        
      /* Test for broadcast MAC address */
      else if (pData[0] == 0xFF)
      {
         /* Low byte indicates we might be broadcast - check against the rest */
         if (is_broadcast_ether_addr(pData))
         {
            /* Broadcast Ethernet frame - set flag */
            bIsBcast = 1;
         }
      }      

      /* If the packet does not have a multicast or broadcast Ethernet address,  
         check PPPoE packets for the IP address as well.  */
      if (bIsBcast == 0 && bIsMcast == 0)
      {
         /* Now, examine the ethertype protocol ID and skip past any 802.1Q VLAN header
            tags (i.e. ethertype ETH_P_8021Q) and TCI tags (32 bits total) until
            we reach the inner packet */
         while (*pusSessEthertypeId == htons(ETH_P_8021Q))
             pusSessEthertypeId = pusSessEthertypeId + (VLAN_HLEN/sizeof(unsigned short));  

         /* Check to see if this is a packet encapsulated within PPPoE and a multicast
            by examining the ethertype of the packet (i.e. ethertype ETH_P_PPP_SES or 0x8864) */
         if (*pusSessEthertypeId == htons(ETH_P_PPP_SES))
         {
            /* Yes, this is PPPoE.  Get the destination IP address, which is 
               in an IP header that follows the dest MAC address and the src MAC address */
            struct iphdr *pPPPoEIpHdr = (struct iphdr *)(pusSessEthertypeId + HT_LEN(hdrType));
             
            /* Is destination IP address is multicast? */
            if (ipv4_is_multicast(pPPPoEIpHdr->daddr))
            {
               /* Multicast IP packet -- set flag */
               bIsMcast = 1;
            }
             
            /* Is IP address broadcast? */
            if (ipv4_is_lbcast(pPPPoEIpHdr->daddr))
            {
               /* Broadcast IP packet - set flag */
               bIsBcast = 1;
            }            
         }
      }

      /* Record statistics for multicast or broadcast packets */
      if (bIsMcast != 0)
      {
         /* Multicast packet - record statistics */
         pDevCtx->DevStats.multicast++;
         pDevCtx->DevStats.rx_multicast_bytes += len ;
      }   
      else if (bIsBcast != 0)
      {
         /* Broadcast packet - record statistics */
         pDevCtx->DevStats.rx_broadcast_packets++;
      }
   }

   if (is_skbuff)
   {
      skb->len = len;   /* update skb->len */
   }
   else
   {
      /* Initialize the FKB context for the received packet. */
      pFkb = bcmxapi_fkb_qinit(pNBuf, pData, len, rxdma);
      
      /* convert pFkb to pNBuf */
      pNBuf = FKBUFF_2_PNBUFF(pFkb);
   }

   if (pDevCtx->szMirrorIntfIn[0] != '\0' &&
       (hdrType == HT_PTM ||
        hdrType == HT_LLC_SNAP_ETHERNET ||
        hdrType == HT_VC_MUX_ETHERNET))
   {
      MirrorPacket(pDevCtx->szMirrorIntfIn, pNBuf);
   }

#ifdef CONFIG_BLOG
   if (is_skbuff)
   {
      blogAction = blog_sinit(skb, pDevCtx->pDev, pDevCtx->ulEncapType,
                              (bufStatus & FSTAT_MATCH_ID_MASK),
                              BLOG_SET_PHYHDR(rfc2684_type, BLOG_XTMPHY));
   }
   else
   {
      blogAction = blog_finit(pFkb, pDevCtx->pDev, pDevCtx->ulEncapType,
                              (bufStatus & FSTAT_MATCH_ID_MASK),
                              BLOG_SET_PHYHDR(rfc2684_type, BLOG_XTMPHY));
   }
   
   if (blogAction == PKT_DONE)
   {
      retStatus = PACKET_BLOG ;
   }
   else if (blogAction == PKT_DROP)
   {
      if (rxdma)
         bcmxapi_rx_pkt_drop(rxdma, pBuf, len);
      pDevCtx->DevStats.rx_dropped++;
      retStatus = PACKET_NORMAL;
   }
   else
#endif
   {
      if (!is_skbuff)
      {
         /* Get an skb to return to the network stack. */
         skb = bcmxapi_skb_alloc(rxdma, pNBuf, delLen, trailerDelLen);
         if (skb == NULL)
         {
            fkb_release(pFkb);  /* releases allocated blog */
            pDevCtx->DevStats.rx_dropped++;
         }
      }
      
      if (skb)
      {
         skb->dev = pDevCtx->pDev;
         //printk ("skb forward to net stack \n") ;
         //DUMP_PKT(skb->data, skb->len);

         switch (hdrType)
         {
         case HT_LLC_SNAP_ROUTE_IP:
         case HT_VC_MUX_IPOA:
            /* IPoA */
            skb->protocol = htons(ETH_P_IP);
            skb_reset_mac_header(skb);
            /* Give the received packet to the network stack. */
            netif_receive_skb(skb);
            break;
#ifdef CONFIG_PPP
         case HT_LLC_ENCAPS_PPP:
         case HT_VC_MUX_PPPOA:
            /*PPPoA*/
            ppp_input(&pDevCtx->Chan, skb);
            break;
#endif
         default:
            /* bridge, MER, PPPoE */
            skb->protocol = eth_type_trans(skb,pDevCtx->pDev);
            /* Give the received packet to the network stack. */
            netif_receive_skb(skb);
            break;
         }
      }

      retStatus = PACKET_NORMAL;
   }
   return (retStatus);
   
}  /* bcmxtmrt_processRxPkt() */


/*---------------------------------------------------------------------------
 * void bcmxtmrt_processRxCell(UINT8 *pData)
 * Description:
 *    Processes a received cell.
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxtmrt_processRxCell(UINT8 *pData)
{
   PBCMXTMRT_GLOBAL_INFO   pGi     = &g_GlobalInfo;
   PBCMXTMRT_DEV_CONTEXT   pDevCtx = NULL;
   XTMRT_CELL              Cell;
   
   const UINT16 usOamF4VciSeg = 3;
   const UINT16 usOamF4VciEnd = 4;
   
   UINT8 ucCts[] = {0, 0, 0, 0, CTYPE_OAM_F5_SEGMENT, CTYPE_OAM_F5_END_TO_END,
                    0, 0, CTYPE_ASM_P0, CTYPE_ASM_P1, CTYPE_ASM_P2, CTYPE_ASM_P3,
                    CTYPE_OAM_F4_SEGMENT, CTYPE_OAM_F4_END_TO_END};
                    
   UINT8 ucCHdr   = *pData;
   UINT32 atmHdr  = ntohl(*((UINT32 *)(pData + sizeof(char))));

   //DUMP_PKT(pData, CELL_SIZE) ;

   /* Fill in the XTMRT_CELL structure */
   Cell.ConnAddr.u.Vcc.usVpi = (UINT16)((atmHdr & ATM_CELL_HDR_VPI_MASK) >>
                                         ATM_CELL_HDR_VPI_SHIFT);

   if (pGi->atmBondSidMode == ATMBOND_ASM_MESSAGE_TYPE_NOSID)
   {
      Cell.ConnAddr.u.Vcc.usVci = (UINT16)((atmHdr & ATM_NON_BONDED_CELL_HDR_VCI_MASK) >>
                                            ATM_CELL_HDR_VCI_SHIFT);
   }
   else if ((pGi->atmBondSidMode == ATMBOND_ASM_MESSAGE_TYPE_12BITSID) ||
            (pGi->atmBondSidMode == ATMBOND_ASM_MESSAGE_TYPE_8BITSID))
   {
      Cell.ConnAddr.u.Vcc.usVci = (UINT16)((atmHdr & ATM_BONDED_CELL_HDR_VCI_MASK) >>
                                            ATM_CELL_HDR_VCI_SHIFT);
   }

   if ((Cell.ConnAddr.u.Vcc.usVpi == XTMRT_ATM_BOND_ASM_VPI) &&
       (Cell.ConnAddr.u.Vcc.usVci == XTMRT_ATM_BOND_ASM_VCI))
   {
      pDevCtx = pGi->pDevCtxs[0];
      memcpy(Cell.ucData, pData + sizeof(char), sizeof(Cell.ucData));
   }
   else
   {
      UINT8 ucLogPort;
   
      /* Possibly OAM Cell type */
      Cell.ConnAddr.ulTrafficType = TRAFFIC_TYPE_ATM;

/* Some 63268 processors have problems in their bonding hardware.  In those
   cases, hard code the OAM as an F5END.  */
#if defined(CONFIG_BCM963268)
      if (pGi->atmBondSidMode != ATMBOND_ASM_MESSAGE_TYPE_NOSID)
      {
         const UINT8  oamF5EndToEnd[] = {0x18, 0x00, 0xBC, 0xBC, 0xBC, 0xBC}; /* Hard coded OAM */
         const UINT32 ulAtmHdrSize  = 4; /* no HEC */

         ucLogPort = PORT_PHYS_TO_LOG(PHY_PORTID_0);  /* Only Port 0/1 in bonding mode */
         Cell.ConnAddr.u.Vcc.ulPortMask = PORT_TO_PORTID(ucLogPort);

         pDevCtx = pGi->pDevCtxs[0];

         if (pDevCtx)
         {
            ucCHdr = CHDR_CT_OAM_F5_E2E;
            Cell.ConnAddr.u.Vcc.usVpi = (UINT16)pDevCtx->Addr.u.Vcc.usVpi;
            Cell.ConnAddr.u.Vcc.usVci = (UINT16)pDevCtx->Addr.u.Vcc.usVci;
         }

         memcpy(Cell.ucData+ulAtmHdrSize, oamF5EndToEnd, sizeof(oamF5EndToEnd));
         memcpy(Cell.ucData+ulAtmHdrSize+sizeof(oamF5EndToEnd), pData, 
                  sizeof(Cell.ucData)-ulAtmHdrSize-sizeof(oamF5EndToEnd));
      }
      else
#endif
      {
         /* Get the port number depending on whether we're in bonding mode or not. */ 
         if (pGi->bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE)
         {
            /* In bonding mode, only port 0 is configured - use it instead of reading port from hardware */
            ucLogPort = PORT_PHYS_TO_LOG(PHY_PORTID_0);  /* Only Port 0/1 in bonding mode */
         }
         else
         {
            /* Read port from hardware in nobonded mode */
            ucLogPort = PORT_PHYS_TO_LOG((ucCHdr & CHDR_PORT_MASK) >> CHDR_PORT_SHIFT);
         }

         /* Get port mask form port number */
         Cell.ConnAddr.u.Vcc.ulPortMask = PORT_TO_PORTID(ucLogPort);

         /* Act depending on type of OAM */
         if (Cell.ConnAddr.u.Vcc.usVci == usOamF4VciSeg)
         {
            /* F4 segment OAM */
            ucCHdr  = CHDR_CT_OAM_F4_SEG;
            pDevCtx = pGi->pDevCtxs[0];
         }
         else
         {
            if (Cell.ConnAddr.u.Vcc.usVci == usOamF4VciEnd)
            {
               /* F4 end to end OAM */
               ucCHdr  = CHDR_CT_OAM_F4_E2E;
               pDevCtx = pGi->pDevCtxs[0];
            }
            else
            {
               /* Other OAM */
               pDevCtx = FindDevCtx((short)Cell.ConnAddr.u.Vcc.usVpi,
                                    (int)Cell.ConnAddr.u.Vcc.usVci);
            }
         }
         memcpy(Cell.ucData, pData + sizeof(char), sizeof(Cell.ucData));
      }
   }

   Cell.ucCircuitType = ucCts[(ucCHdr & CHDR_CT_MASK) >> CHDR_CT_SHIFT];

   if ((ucCHdr & CHDR_ERROR) == 0)
   {
      /* Call the registered OAM or ASM callback function. */
      switch (ucCHdr & CHDR_CT_MASK)
      {
      case CHDR_CT_OAM_F5_SEG:
      case CHDR_CT_OAM_F5_E2E:
      case CHDR_CT_OAM_F4_SEG:
      case CHDR_CT_OAM_F4_E2E:
         if (pGi->pfnOamHandler && pDevCtx)
         {
            //printk  ("bcmxtmrt : Rx OAM Cell %d \n", (ucCHdr&CHDR_CT_MASK)) ;
            (*pGi->pfnOamHandler)((XTMRT_HANDLE)pDevCtx,
                                  XTMRTCB_CMD_CELL_RECEIVED, &Cell,
                                  pGi->pOamContext);
         }
         break;

      case CHDR_CT_ASM_P0:
      case CHDR_CT_ASM_P1:
      case CHDR_CT_ASM_P2:
      case CHDR_CT_ASM_P3:
         if (pGi->pfnAsmHandler && pDevCtx)
         {
            (*pGi->pfnAsmHandler)((XTMRT_HANDLE)pDevCtx,
                                  XTMRTCB_CMD_CELL_RECEIVED, &Cell,
                                  pGi->pAsmContext);
         }
         break;

      default:
         printk("bcmxtmrt : unknown cell type %x \n", ucCHdr & CHDR_CT_MASK);
         break;
      }
   }
   else
   {
      if (pDevCtx)
         pDevCtx->DevStats.rx_errors++;
      printk("bcmxtmcfg : Cell Received in Error \n");
   }
}  /* bcmxtmrt_processRxCell() */


/*---------------------------------------------------------------------------
 * void MirrorPacket(char *mirrorIntf, pNBuff_t pNBuf)
 * Description:
 *    This function sends a sent or received packet to a LAN port.
 *    The purpose is to allow packets sent and received on the WAN
 *    to be captured by a protocol analyzer on the Lan for debugging
 *    purposes.
 * Note: must keep pNBuf intact.
 * Returns: void
 *---------------------------------------------------------------------------
 */
static void MirrorPacket(char *mirrorIntf, pNBuff_t pNBuf)
{
   /* pNBuf is either SKBUFF or FKBUFF */
   struct net_device *netDev;
   struct sk_buff    *skb2;   /* mirror skb */
   UINT8  *data_p;
   int    len, size;
   unsigned long flags;
    
   if ((netDev = __dev_get_by_name(&init_net, mirrorIntf)) == NULL)
      return;
      
   if (IS_SKBUFF_PTR(pNBuf))
   {
      struct sk_buff *skbO;   /* Original skb */

      skbO = PNBUFF_2_SKBUFF(pNBuf);
      if (skbO == (struct sk_buff *)NULL)
      {
          printk(CARDNAME ": PNBUFF_2_SKBUFF failure\n");
          return;
      }

      data_p = skbO->data;
      len    = skbO->len;
   }
   else
   {
      FkBuff_t *fkbO_p;
      
      fkbO_p = PNBUFF_2_FKBUFF(pNBuf);
      if (fkbO_p == FKB_NULL)
      {
          printk(CARDNAME ": PNBUFF_2_FKBUFF failure\n");
          return;
      }

      data_p = fkbO_p->data;      
      len    = fkbO_p->len;
   }

   size = (len + 3) & ~3;  /* align to word boundary */
   
   skb2 = alloc_skb(size, GFP_ATOMIC);
   if (skb2 == (struct sk_buff *)NULL)
   {
       printk(CARDNAME ": alloc_skb failure\n");
       return;
   }
   skb_put(skb2, len);
   memcpy(skb2->data, data_p, len);

   /* pad with zero */      
   if (size > len)
      memset(skb2->data+len, 0, size-len);
   
   skb2->dev      = netDev;
   skb2->protocol = htons(ETH_P_802_3);
   local_irq_save(flags);
   local_irq_enable();
   dev_queue_xmit(skb2) ;
   local_irq_restore(flags);
   return;

}  /* MirrorPacket() */


#ifdef CONFIG_BLOG
/*---------------------------------------------------------------------------
 * void bcmxtmrt_update_hw_stats(Blog_t *blog_p, UINT32 hits, UINT32 octets)
 * Description:
 *    Update the XTMRT transmit and receive counters from flow 
 *    hardware if appropriate. This flows through to
 *    the 'ifconfig' counts.
 * Returns: void
 *---------------------------------------------------------------------------
 */
void bcmxtmrt_update_hw_stats(Blog_t *blog_p, UINT32 hits, UINT32 octets)
{
   PBCMXTMRT_DEV_CONTEXT pDevCtx;
   struct net_device *dev_p;

   if (blog_p->tx.info.bmap.BCM_XPHY)
   {
      dev_p = (struct net_device *)blog_p->tx.dev_p;

      /* Is dev_p a good pointer? */
      if (dev_p == NULL)
      {
         /* No, it is not.  Quit */
         return;
      }

      pDevCtx = netdev_priv(dev_p);

      /* Is pDevCtx a good pointer? */
      if (pDevCtx == NULL)
      {
         /* No, it is not.  Quit */
         return;
      }

      /* Adjust xmit packet counts for flows that tx to XTM */
      pDevCtx->DevStats.tx_packets += hits;
      pDevCtx->DevStats.tx_bytes   += octets;

      /* Now, adjust multicast counts if this is a multicast flow. */
      if (blog_p->rx.multicast) 
      {
         pDevCtx->DevStats.tx_multicast_packets += hits;
         pDevCtx->DevStats.tx_multicast_bytes   += octets;
      }
   }
   else if(blog_p->rx.info.bmap.BCM_XPHY)
   {
      dev_p = (struct net_device *) blog_p->rx.dev_p;

      /* Is dev_p a good pointer? */
      if (dev_p == NULL)
      {
         /* No, it is not.  Quit */
         return;
      }

      pDevCtx = netdev_priv(dev_p);

      /* Is pDevCtx a good pointer? */
      if (pDevCtx == NULL)
      {
         /* No, it is not.  Quit */
         return;
      }

      /* Adjust receive packet counts for flows that rx from XTM */
      pDevCtx->DevStats.rx_packets += hits;
      pDevCtx->DevStats.rx_bytes   += octets;

      if (blog_p->rx.multicast) 
      {
         pDevCtx->DevStats.multicast          += hits;
         pDevCtx->DevStats.rx_multicast_bytes += octets;
      }
   }
}  /* bcmxtmrt_update_hw_stats() */


/*---------------------------------------------------------------------------
 * int bcmxtmrt_get_xmit_channel(void *dev, int channel, unsigned uMark)
 * Description:
 *    Gets the XTM TX channel for the netdev and mark. If the 
 *    Link is not up or no FIFOs are created for the device
 *    then the in channel is returned.
 * Returns:
 *    new TX channel number if successful or on error returns the
 *    in channel parameter.
 *---------------------------------------------------------------------------
 */
static int bcmxtmrt_get_xmit_channel(void *dev, int channel, unsigned uMark)
{
   spinlock_t            *xtmlock = &g_GlobalInfo.xtmlock_tx;
   PBCMXTMRT_DEV_CONTEXT pDevCtx  = netdev_priv((struct net_device *)dev);

   spin_lock_bh(xtmlock);

   if (pDevCtx->ulLinkState == LINK_UP)
   {
      if (pDevCtx->ulTxQInfosSize)
      {
         BcmPktDma_XtmTxDma *txdma = find_xmit_channel(pDevCtx, uMark);

         if (txdma && txdma->txEnabled == 1)
         {
#if defined (CONFIG_BCM963138) || defined (CONFIG_BCM963148)
            if ((pDevCtx->Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM)
               channel = (MAX_TRANSMIT_QUEUES * txdma->ulDmaIndex) + XCT_AAL5;
            else
               channel = (MAX_TRANSMIT_QUEUES * txdma->ulDmaIndex) + XCT_PTM;
#else
            channel = txdma->ulDmaIndex;
#endif
         }
      }
   }

   spin_unlock_bh(xtmlock);

   return channel;
    
}  /* bcmxtmrt_get_xmit_channel() */
#endif


/*---------------------------------------------------------------------------
 * void bcm63xx_xtmrt_rx_thread(void *arg)
 * Description:
 *    Handle XTM receive processing.
 * Returns: void
 *---------------------------------------------------------------------------
 */

/*
 * bcm63xx_xtmrt_rx_thread: Handle XTM receive processing
 */
static int bcm63xx_xtmrt_rx_thread(void *arg)
{
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
   UINT32 mask;
   unsigned long flags;
   uint32 work_done;
   uint32 ret_done;
   uint32 more_to_do;
   int budget = XTMRT_BUDGET;

#ifdef CONFIG_BCM963381
   {
      struct sched_param schedParam;   /* To control thread's priority */

      /* Set the priority of this thread to real time */
      /* Compensate for 63381 rate issues with higher priroity */
      schedParam.sched_priority = (BCM_RTPRIO_DATA+1); 

      sched_setscheduler(current, SCHED_RR, &schedParam);
      set_user_nice(current, 0);
   }
#endif

   /* Main task loop */
   while (1)
   {
      /* Wait to be woken up by received packets */
      wait_event_interruptible(pGi->rx_thread_wqh,
                    pGi->rx_work_avail);

      /* Abort if we were woken up to terminate */
      if (kthread_should_stop())
      {
         printk(KERN_INFO "kthread_should_stop detected on bcmxtmrt-rx\n");
         break;
      }

      /* There are new packets to process.  Capture all enabled interrupts
         and then clear them in the global structure. */
      spin_lock_irqsave(&pGi->xtmlock_rx_regs, flags);
      mask = pGi->ulIntEnableMask;
      pGi->ulIntEnableMask = 0;       /* Disable interrupts */
      spin_unlock_irqrestore(&pGi->xtmlock_rx_regs, flags);

      /* Now, process some received packets */
      local_bh_disable();
      work_done  = bcmxapi_rxtask(budget, &more_to_do);
      ret_done   = work_done & XTM_POLL_DONE;            /* Did the poll of channels complete? */
      work_done &= ~XTM_POLL_DONE;                       /* How many packets processed? */
      local_bh_enable();

      /* Did we use up our budget?  Do we still have channels with
         unprocessed data left in them (i.e. did we fail to complete
         a poll of all channels)? */
      if (work_done >= budget || ret_done != XTM_POLL_DONE)
      {
         /* We have either exhausted our budget or there are
            more packets on the DMA (or both). */

         /* Make sure all enabled interrupts are restored to global structure */
         spin_lock_irqsave(&pGi->xtmlock_rx_regs, flags);
         pGi->ulIntEnableMask |= mask;       /* Enable interrupts */
         spin_unlock_irqrestore(&pGi->xtmlock_rx_regs, flags);

         /* Yield CPU to allow others to have a chance, then continue to
            top of loop for more work.  */
         if (current->policy == SCHED_FIFO || current->policy == SCHED_RR)
            yield();
      }
      else
      {
         /*
          * No more packets.  Indicate we are done (rx_work_avail=0) and
          * re-enable interrupts (bcmxapi_clear_xtmrxint) and go to top of
          * loop to wait for more work.
          */
         spin_lock_irqsave(&pGi->xtmlock_rx_regs, flags);
         pGi->rx_work_avail = 0;
         bcmxapi_clear_xtmrxint(mask);
         spin_unlock_irqrestore(&pGi->xtmlock_rx_regs, flags);
      }
   }

    return 0;
}

/***************************************************************************
 * MACRO to call driver initialization and cleanup functions.
 ***************************************************************************/
module_init(bcmxtmrt_init);
module_exit(bcmxtmrt_cleanup);
MODULE_LICENSE("GPL");

#ifdef CONFIG_BLOG
EXPORT_SYMBOL(bcmxtmrt_update_hw_stats);
#endif

