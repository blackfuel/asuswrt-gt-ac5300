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
 * File Name  : bcmxtmrtbond.c
 *
 * Description: This file implements BCM6368 ATM/PTM bonding network device driver
 *              runtime processing - sending and receiving data.
 *              Current implementation pertains to PTM bonding. Broadcom ITU G.998.2 
 *              solution.
 ***************************************************************************/


/* Includes. */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/version.h>
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
#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <board.h>
#include "bcmnet.h"
#include "bcmxtmcfg.h"
#include "bcmxtmrt.h"
#include <asm/io.h>
#include <asm/r4kcache.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>
#include "bcmxtmrtimpl.h"

#if defined(PERF_MON_BONDING_US) || defined(PERF_MON_BONDING_DS)
#include <asm/pmonapi.h>
#endif

#ifdef PERF_MON_BONDING_US
#define PMON_US_LOG(x)        pmon_log(x)
#define PMON_US_CLR(x)        pmon_clr(x)
#define PMON_US_BGN           pmon_bgn()
#define PMON_US_END(x)        pmon_end(x)
#define PMON_US_REG(x,y)      pmon_reg(x,y)
#else
#define PMON_US_LOG(x)        
#define PMON_US_CLR(x)        
#define PMON_US_BGN
#define PMON_US_END(x)        
#define PMON_US_REG(x,y)
#endif

#ifdef PERF_MON_BONDING_DS
#define PMON_DS_LOG(x)        pmon_log(x)
#define PMON_DS_CLR(x)        pmon_clr(x)
#define PMON_DS_BGN           pmon_bgn()
#define PMON_DS_END(x)        pmon_end(x)
#define PMON_DS_REG(x,y)      pmon_reg(x,y)
#else
#define PMON_DS_LOG(x)        
#define PMON_DS_CLR(x)        
#define PMON_DS_BGN
#define PMON_DS_END(x)        
#define PMON_DS_REG(x,y)
#endif


#ifdef PTM_BONDING_DEBUG
#define XTMRT_BOND_LOG(level, format, args...)               \
   do {                                            \
      if (level < g_GlobalInfo.ptmBondInfo.logLevel) {            \
         printk(format, ## args);            \
      }                                 \
   } while (0)

#define XTMRT_BOND_LOG3_FKB(fkb, dir)                  \
   if (3 < g_GlobalInfo.ptmBondInfo.logLevel)                  \
      log_fkb(fkb, dir)

#define XTMRT_BOND_LOG4_FKB(fkb, dir)                  \
   if (4 < g_GlobalInfo.ptmBondInfo.logLevel)                  \
      log_fkb(fkb, dir)

static void log_fkb(FkBuff_t *fkb, char *dir)
{
   int i, j;
   for (i=0; i<fkb->len-1; ) {
      printk("\n%s %04X: ", dir, i);
      for (j=0; j<16 && i<fkb->len-1; j+=2, i+=2)
         printk("%02X%02X ", fkb->data[i], fkb->data[i+1]);
   }
   if (i==fkb->len-1)
      printk("%02X ", fkb->data[i]);
   printk("\n\n");
}
#else
#define XTMRT_BOND_LOG(level, format, args...) 
#define XTMRT_BOND_LOG3_FKB(fkb, dir)
#define XTMRT_BOND_LOG4_FKB(fkb, dir)
#endif

#define XTMRT_BOND_LOG0(format, args...) XTMRT_BOND_LOG(0, format, ## args)
#define XTMRT_BOND_LOG1(format, args...) XTMRT_BOND_LOG(1, format, ## args)
#define XTMRT_BOND_LOG2(format, args...) XTMRT_BOND_LOG(2, format, ## args)
#define XTMRT_BOND_LOG3(format, args...) XTMRT_BOND_LOG(3, format, ## args)

#define CONTEXT_TO_CHANNEL(context)  (int)((context) & 0x3u)

static inline int getPtmFragmentLen (int len) ;
static int constructPtmBondHdr (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT32 ulPtmPrioIdx, int len) ;
static inline int bcmxtmrt_ptmbond_get_port (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT16 bufStatus) ;
static int bcmxtmrt_ptmbond_process_rx_fragment (XtmRtPtmBondInfo *pBondInfo,
                                                 XtmRtPtmBondRxQInfo *inBuf) ;
static int bcmxtmrt_ptmbond_rx_send_expected_fragment (XtmRtPtmBondInfo *pBondInfo,
                                                       XtmRtPtmBondRxQInfo *inBuf, int seq_nr,
                                                       int rxFlags) ;
static void bcmxtmrt_ptmbond_rx_send (XtmRtPtmBondInfo *pBondInfo, XtmRtPtmBondRxQInfo *inBuf,
                                     int flags) ;
static int bcmxtmrt_ptmbond_rx_resync (XtmRtPtmBondInfo *pBondInfo, int type, int port) ;
static int bcmxtmrt_ptmbond_rx_send_waiting_fragments (XtmRtPtmBondInfo *pBondInfo, int start) ;
static inline void bcmxtmrt_ptmbond_del_link (XtmRtPtmBondInfo *pBondInfo, int port) ;
static void bond_fkb_free(FkBuff_t *fkb, int flushLen, UINT16 rxdmaIndex) ;

/***************************************************************************
 * Function Name: getPtmFragmentLen
 * Description  : Compute next fragment length
 * Returns      : fragment length.
 ***************************************************************************/
static inline int getPtmFragmentLen (int len)
{
   int fragmentLen ;
   int leftOver ;

   if (len <= XTMRT_PTM_BOND_TX_MAX_FRAGMENT_SIZE) {
      fragmentLen = len ;
   }
   else {
      /* send as much as possible unless we don't have
         enough data left anymore for a full fragment */
      fragmentLen = XTMRT_PTM_BOND_TX_MAX_FRAGMENT_SIZE ;
      leftOver = len-fragmentLen ;
      if (leftOver < XTMRT_PTM_BOND_TX_MIN_FRAGMENT_SIZE) {
         fragmentLen -= (XTMRT_PTM_BOND_TX_MIN_FRAGMENT_SIZE + leftOver) ;
         fragmentLen &= ~0x3 ; /* make it a multiple of 4 bytes */
      }
   }
   return fragmentLen ;
}

/***************************************************************************
 * Function Name: constructPtmBondHdr
 * Description  : Calculates the PTM bonding hdr information and fills it up
 *                in the global buffer to be used for the packet.
 * Returns      : NoofFragments.
 ***************************************************************************/
static int constructPtmBondHdr (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT32 ulPtmPrioIdx, int len)
{
   int i, serviced = 0 ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo ;
   XtmRtPtmTxBondHeader  *pPtmHeader  = NULL ;
   int                   fragNo, fragLen ;
   XtmRtPtmBondInfo      *pBondInfo = &pGi->ptmBondInfo ;
   volatile UINT32       *pPortDataMask = &pDevCtx->ulPortDataMask ;
   volatile UINT32       *pulLinkWts    = &pBondInfo->ulLinkUsWt[0] ;
   volatile UINT8        *pu8ConfWtPortDist = &pBondInfo->u8ConfWtPortDist[0] ;
   volatile UINT32       *pulCurrWtPortDistStartIndex = &pBondInfo->ulCurrWtPortDistStartIndex ;
   volatile UINT32       *pulCurrWtPortDistRunIndex   = &pBondInfo->ulCurrWtPortDistRunIndex ;
   volatile UINT32       *pulCurrWtTotalIterationsBeforeReset   = &pBondInfo->ulCurrWtTotalIterationsBeforeReset ;
   UINT8                 portCount ;
	int					    reloadCount ;

   fragNo = 0 ;
   len += ETH_FCS_LEN ;    /* Original Packet Len + 4 bytes of Eth FCS Len */
   
   while (len != 0) {
      fragLen = getPtmFragmentLen (len) ;
      len -= fragLen ;
      pPtmHeader = &pGi->ptmBondHdr [fragNo] ;

      fragLen += XTMRT_PTM_BOND_FRAG_HDR_SIZE + XTMRT_PTM_CRC_SIZE ;  /* With actual hdrs/trailers */
      pPtmHeader->sVal.FragSize = fragLen ;

#ifndef PTMBOND_US_PRIO_TRAFFIC_SPLIT
		reloadCount = 0 ;
      do {
         for (i = 0; i< MAX_BOND_PORTS; i++) {

            portCount = pu8ConfWtPortDist [*pulCurrWtPortDistRunIndex] ;

            if ((((*pPortDataMask >> portCount) & 0x1) != 0)
                  &&
                  (pulLinkWts[portCount] >= fragLen)) {  /* port is enabled with valid weights */

               pPtmHeader->sVal.portSel = portCount ;
               pulLinkWts[portCount] -= fragLen ;
               serviced = 1 ;
            }

            *pulCurrWtTotalIterationsBeforeReset += 1 ;
            if (*pulCurrWtTotalIterationsBeforeReset == pBondInfo->totalWtPortDist) {

               *pulCurrWtPortDistStartIndex = ((*pulCurrWtPortDistStartIndex+1) >= pBondInfo->totalWtPortDist) ? 0 :
                  *pulCurrWtPortDistStartIndex+1 ;
               *pulCurrWtPortDistRunIndex = *pulCurrWtPortDistStartIndex ;
               *pulCurrWtTotalIterationsBeforeReset = 0 ;
            }
            else {
               *pulCurrWtPortDistRunIndex = ((*pulCurrWtPortDistRunIndex+1) >= pBondInfo->totalWtPortDist) ? 0 :
                  *pulCurrWtPortDistRunIndex+1 ;
            }

            if (serviced == 1) {
               break ;
            }
         } /* for (i) */

         if (serviced == 1) {
            serviced  = 0 ;
            break ;
         }
         else {/* No port selection happened  for this fragment, reload the weights with unused credits */
            reloadCount ++;
            /* reload the weights more than one time, CPE does not have enought bandwidth. */
            if (reloadCount > 1) { 
               //printk (CARDNAME " constructPtmBondHdr: does not have enough uplink bandwidth. \n") ;             
               return 0 ;
            }

            pulLinkWts[0] += pBondInfo->ulConfLinkUsWt[0] ;
            pulLinkWts[1] += pBondInfo->ulConfLinkUsWt[1] ;
         }
      } while (1) ;
#else
      pPtmHeader->sVal.portSel = (ulPtmPrioIdx == PTM_FLOW_PRI_LOW) ? PHY_PORTID_0 : PHY_PORTID_1 ;
#endif

      pPtmHeader->sVal.PktEop = XTMRT_PTM_BOND_HDR_NON_EOP ;
      fragNo++ ;
   } /* while (len != 0) */

   pPtmHeader->sVal.PktEop = XTMRT_PTM_BOND_FRAG_HDR_EOP ;
   return (fragNo) ;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_add_hdr
 * Description  : Adds the PTM Bonding Tx header to a WAN packet before transmitting
 *                it.
 * Returns      : None.
 ***************************************************************************/
int bcmxtmrt_ptmbond_add_hdr (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT32 ulPtmPrioIdx,
                               pNBuff_t *ppNBuff, struct sk_buff **ppNBuffSkb, 
                               UINT8 **ppData, int *pLen)
{
   int                   frags = 0 ;
   XtmRtPtmTxBondHeader  *pPtmBondHdr ;
   int                   headroom ;
   int                   len ;
   int                   minheadroom ;
   PBCMXTMRT_GLOBAL_INFO pGi ;
   XtmRtPtmBondInfo      *pBondInfo ;
   XtmRtPtmTxBondHeader  *pPtmSrcBondHdr ;

   PMON_US_BGN ;

   minheadroom  = sizeof (XtmRtPtmTxBondHeader) * XTMRT_PTM_BOND_MAX_FRAG_PER_PKT ;
   pGi = &g_GlobalInfo ;
   pBondInfo = &pGi->ptmBondInfo ;
   pPtmSrcBondHdr = &pGi->ptmBondHdr[0] ;

   //printk ("bcmxtmrt: ptm tx priority %d \n", ulPtmPrioIdx) ;

   PMON_US_LOG(1) ;

   len = *pLen ;

   if ( *ppNBuffSkb == NULL) {

      struct fkbuff *fkb = PNBUFF_2_FKBUFF(*ppNBuff);
      headroom = fkb_headroom (fkb) ;

      if (headroom >= minheadroom) {
         frags = constructPtmBondHdr (pDevCtx, ulPtmPrioIdx, len) ;

			if (frags != 0) {
				PMON_US_LOG(2) ;
				*ppData = fkb_push (fkb, minheadroom) ;
				*pLen   = len + minheadroom ;
				pPtmBondHdr = (XtmRtPtmTxBondHeader *) *ppData ;
				PMON_US_LOG(3) ;
				bond_memcpy (pPtmBondHdr, pPtmSrcBondHdr, sizeof (XtmRtPtmTxBondHeader) * frags) ;
				PMON_US_LOG(4) ;
			}
      }
      else
         printk(CARDNAME "bcmxtmrt_xmit: FKB not enough headroom.\n") ;
   }
   else {
      struct sk_buff *skb = *ppNBuffSkb ;
      headroom = skb_headroom (skb) ;

      if (headroom < minheadroom) {
         struct sk_buff *skb2 = skb_realloc_headroom(skb, minheadroom);
         XTMRT_BOND_LOG2 ("bcmxtmrt: Warning!!, headroom (%d) is less than min headroom (%d) \n",
               headroom, minheadroom) ;
         dev_kfree_skb_any(skb);
         if (skb2 == NULL) {
            printk (CARDNAME ": Fatal!!, NULL Skb \n") ;
            skb = NULL ;
         }
         else
            skb = skb2 ;
      }

      if( skb ) {
         frags = constructPtmBondHdr (pDevCtx, ulPtmPrioIdx, len) ;

			if (frags != 0) {
				PMON_US_LOG(2) ;
				*ppData = skb_push (skb, minheadroom) ;
				*pLen   = len + minheadroom ;
				pPtmBondHdr = (XtmRtPtmTxBondHeader *) *ppData ;
				PMON_US_LOG(3) ;
				u16cpy (pPtmBondHdr, pPtmSrcBondHdr, sizeof (XtmRtPtmTxBondHeader) * frags) ;
				PMON_US_LOG(4) ;
			}
      }

      *ppNBuffSkb = skb ;
      *ppNBuff = SKBUFF_2_PNBUFF(skb) ;
   }

   PMON_US_LOG(5) ;
   PMON_US_END(5) ;

   return frags ;
} /* bcmxtmrt_ptmbond_add_hdr */


/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_calculate_link_weights
 * Description  : Calculates the ptm bonding link weights, based on the
 *                link availability.
 * Returns      : None.
 ***************************************************************************/
int bcmxtmrt_ptmbond_calculate_link_weights (PBCMXTMRT_DEV_CONTEXT pDevCtx) 
{
   int i, max, min, nRet = 0 ;
   int quot, mod, quotFromMod, ratioMin, ratioMax ;
   XtmRtPtmBondInfo      *pBondInfo ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo ;

   pBondInfo = &pGi->ptmBondInfo ;

   for (i=0; i< MAX_BOND_PORTS; i++) {
      if (pDevCtx->ulLinkUsRate[i] != 0)
         pBondInfo->ulConfLinkUsWt [i] = pDevCtx->ulLinkUsRate[i]/8 ;  /* bytes/sec */
      else
         pBondInfo->ulConfLinkUsWt [i] = 0 ;
   }

   memcpy (&pBondInfo->ulLinkUsWt[0], &pBondInfo->ulConfLinkUsWt[0], (MAX_BOND_PORTS*sizeof (UINT32))) ;

   /* Calculate the wt port distribution */

   if (pDevCtx->ulLinkUsRate[0] >= pDevCtx->ulLinkUsRate[1]) {
      max = 0; min = 1 ;
   }
   else {
      max = 1; min = 0 ;
   }

   if (pDevCtx->ulLinkUsRate[max] != 0) {

      if (pDevCtx->ulLinkUsRate[min] != 0) {
         quot = pDevCtx->ulLinkUsRate[max]/pDevCtx->ulLinkUsRate[min] ;
         mod  = pDevCtx->ulLinkUsRate[max]%pDevCtx->ulLinkUsRate[min] ;
         quotFromMod = (mod*10)/pDevCtx->ulLinkUsRate[min];
         if ((mod==0) || (quotFromMod==0)) {
            ratioMax = quot ;
            ratioMin = 0x1 ;
         }
         else {
            ratioMax = (quot*10)+quotFromMod ;
            ratioMin = 0x1*10 ;
         }

         pBondInfo->totalWtPortDist = ratioMax+ratioMin ;
         if (pBondInfo->totalWtPortDist > MAX_WT_PORT_DIST) {
            nRet = -ENOBUFS ;
            goto _End ;
         }

         quot = (pBondInfo->totalWtPortDist/ratioMin) ;

         for (i=0;i<pBondInfo->totalWtPortDist;i++) {
            if (((i%quot)==0) && ratioMin != 0) {
               pBondInfo->u8ConfWtPortDist[i] = min ;
               ratioMin--;
            }
            else
               pBondInfo->u8ConfWtPortDist[i] = max ;
         }
      }
      else {
         pBondInfo->totalWtPortDist = 0x1 ;
         pBondInfo->u8ConfWtPortDist[0]    = max ;
      }
   }
   else {
      if (pDevCtx->ulLinkUsRate[min] != 0) {
         pBondInfo->totalWtPortDist = 0x1 ;
         pBondInfo->u8ConfWtPortDist[0]    = min ;
      }
      else {
         pBondInfo->totalWtPortDist = 0x0 ;
         pBondInfo->u8ConfWtPortDist[0]    = 0xFF ;
      }
   }

   pBondInfo->ulCurrWtPortDistStartIndex = 0 ;
   pBondInfo->ulCurrWtPortDistRunIndex   = pBondInfo->ulCurrWtPortDistStartIndex ;
   pBondInfo->ulCurrWtTotalIterationsBeforeReset   = 0 ;

_End :
   return (nRet) ;
}

/***************************************************************** */
/* ================= Re-Order And Forward engine ================= */
/***************************************************************** */

void *bond_memcpy (void * dest, void const * src, size_t cnt)
{
   if ((!((unsigned int) src & 0x03)) && (!((unsigned int) dest & 0x03))) {
      /* Word Alignment Copy */

      long * pldest = (long *) dest;
      long const * plSrc = (long const *) src;
      long lval ;

      while (cnt >= 4) {
         lval = *plSrc ;
         plSrc++ ;
         cnt -= 4;
         *pldest = lval ;
         pldest++ ;
      }

      {
         char * pcdest = (char *) pldest;
         char const * pcSrc = (char const *) plSrc;
         char cval ;

         while (cnt) {
            cval = *pcSrc ;
            pcSrc++ ;
            cnt-- ;
            *pcdest = cval ;
            pcdest++ ;
         }
      }
   }
   else if ((!((unsigned int) src & 0x01)) && (!((unsigned int) dest & 0x01))) {
      /* Short Word Alignment Copy */

      short * psdest = (short *) dest;
      short const * psSrc = (short const *) src;
      long sval ;

      while (cnt >= 2) {
         sval = *psSrc ;
         psSrc++ ;
         cnt -=2 ;
         *psdest = sval ;
         psdest++ ;
      }

      {
         char * pcdest = (char *) psdest;
         char const * pcSrc = (char const *) psSrc;
         char cval ;

         while (cnt) {
            cval = *pcSrc ;
            pcSrc++ ;
            cnt-- ;
            *pcdest = cval ;
            pcdest++ ;
         }
      }
   }
   else {
      /* Byte Alignment Copy */
      memcpy (dest, src, cnt) ;
   }

   return (dest) ;
}

static void bond_fkb_free (FkBuff_t *fkb, int flushLen, UINT16 rxdmaIndex)
{
   UINT8 *pucData ;

   pucData = PFKBUFF_TO_PDATA(fkb, BCM_PKT_HEADROOM) ;
   FlushAssignRxBuffer (rxdmaIndex, pucData, pucData+flushLen) ;
}

/***************************************************************************
 * Function Name: bond_ptm_rx_send
 * Description  : Assemble and send the fragment
 * Returns      : 0 on success.
 ***************************************************************************/
static void bcmxtmrt_ptmbond_rx_send (XtmRtPtmBondInfo *pBondInfo, XtmRtPtmBondRxQInfo *inBuf,
                                     int flags)
{
   int ret ;
   FkBuff_t       *fkb_frag = inBuf->rxFkb ;
   FkBuff_t       *processFkb ;
   XtmRtPtmBondRxQInfo  *fwd = &pBondInfo->fwd [PTMBOND_FWD_BUF_INDEX] ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo ;

   if (fkb_frag) {
      /* remove PTM fragment header */
      fkb_pull(fkb_frag, XTMRT_PTM_BOND_FRAG_HDR_SIZE);

      //printk ("F len = %d flags=%x \n", fkb_frag->len, flags) ;

      /* copy data to current forward buffer */
      if (fwd->valid == TRUE) {
         unsigned char *to;
         FkBuff_t *rxFkb ;

         rxFkb = fwd->rxFkb ;

         //if ((rxFkb->len+fkb_frag->len) > (MAX_MTU_SIZE+ENABLED --todo
#ifdef NOT_NEEDED
         if (unlikely(fkb_tailroom(rxFkb) < fkb_frag->len)) {
            processFkb = fkb_copy_expand(rxFkb, 0, fkb_frag->len, GFP_ATOMIC);
            //processFkb = fkb_copy_expand(rxFkb, 0, RXBUF_SIZE-rxFkb->len, GFP_ATOMIC);
            XTMRT_BOND_LOG2 ("btmxtmrt: fkb_copy_expand len %d \n", fkb_frag->len) ;
            if (processFkb == NULL)
               return 1;
            //spin_unlock_bh(&pGi->xtmlock_rx);
            bond_fkb_free(rxFkb, 8);
            //spin_lock_bh(&pGi->xtmlock_rx);
            fwd->rxFkb = processFkb;
            rxFkb = fwd->rxFkb ;
            pBondInfo->bStats.cpe++ ;
         }
#endif
#if 0
         to = _fkb_put(rxFkb, fkb_frag->len);
#else
         to = rxFkb->data + rxFkb->len ;
         rxFkb->len  += fkb_frag->len ;
#endif
         bond_memcpy(to, fkb_frag->data, fkb_frag->len);
         cache_flush_len (to, fkb_frag->len) ;
         pBondInfo->bStats.cpe++ ;
         bond_fkb_free(fkb_frag, fkb_frag->len+8, inBuf->rxdmaIndex) ;
      }
      else {               /* first frame fragment */
         fwd->rxFkb       = fkb_frag ;
         fwd->pDevCtx     = inBuf->pDevCtx ;
         fwd->rxFkbStatus = inBuf->rxFkbStatus ;
         fwd->phyPort     = inBuf->phyPort ;
         fwd->rxdmaIndex  = inBuf->rxdmaIndex ;
         fwd->valid       = TRUE ;
      }
   } /* fkb_frag */
  
   /* when packet is complete, send it */
   if (fwd->valid == TRUE) {

      if (flags & XTMRT_PTM_BOND_FRAG_HDR_EOP) {
      processFkb = fwd->rxFkb ;
      //__fkb_trim(processFkb, processFkb->len-ETH_FCS_LEN) ; /* remove FCS */
      processFkb->len -= ETH_FCS_LEN ;

      XTMRT_BOND_LOG2("rx forward frame len=%d\n", processFkb->len);
      XTMRT_BOND_LOG4_FKB(processFkb, "Rx");

      //pBondInfo->lastRxPktSz = processFkb->len + 8 ;
      cache_flush_len (processFkb->data-XTMRT_PTM_BOND_FRAG_HDR_SIZE, 8) ;
      fwd->valid = FALSE ;
      spin_lock_bh (&pGi->xtmlock_rx) ;
         if ((ret = bcmxtmrt_process_rx_pkt (fwd->pDevCtx, pGi->rxdma[fwd->rxdmaIndex], processFkb,
                                   fwd->rxFkbStatus, XTMRT_PTM_BOND_FRAG_HDR_SIZE, 4, TRUE)) == PACKET_BLOG)
         pBondInfo->bStats.rxp++;
         else {
            if (ret == PACKET_NORMAL)
                pBondInfo->bStats.rxpl++;
      else
                pBondInfo->bStats.rxd++;
         }

      //printk("rx forward frame count fc=%d l=%d\n", pBondInfo->bStats.rxp, pBondInfo->bStats.rxpl) ;
      pBondInfo->bStats.rxo += processFkb->len ;
   }
      else if (flags & XTMRT_PTM_BOND_FRAG_HDR_EOP_DROP) {
         processFkb = fwd->rxFkb ;
         bond_fkb_free (processFkb, processFkb->len, fwd->rxdmaIndex) ;
         fwd->valid = FALSE ;
         flags = XTMRT_PTM_BOND_FRAG_HDR_EOP ;
      }
   } /* if (fwd->valid == TRUE) */

   pBondInfo->lastRxFlags = flags;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_rx_send_expected_fragment
 * Description  : Check for assembly error on expected fragment and send it
 * Returns      : 0 on success.
 ***************************************************************************/
static int bcmxtmrt_ptmbond_rx_send_expected_fragment (XtmRtPtmBondInfo *pBondInfo,
                                                       XtmRtPtmBondRxQInfo *inBuf,
                                                       int seq_nr, int rxFlags)
{
   FkBuff_t    *fkb  = inBuf->rxFkb ;

   /* discard offending non-SOP fragment */
   if (!(rxFlags & XTMRT_PTM_BOND_FRAG_HDR_SOP) &&
        (pBondInfo->lastRxFlags & XTMRT_PTM_BOND_FRAG_HDR_EOP)) {
      pBondInfo->bStats.eop++;
      //XTMRT_BOND_LOG0("unexpected m/e of packet, total=%d\n", pBondInfo->bStats.eop);
      XTMRT_BOND_LOG0("E-eop\n") ;
      bond_fkb_free(fkb,8, inBuf->rxdmaIndex) ;
      return 1;
   }

   if ((rxFlags & XTMRT_PTM_BOND_FRAG_HDR_SOP) &&
      !(pBondInfo->lastRxFlags & XTMRT_PTM_BOND_FRAG_HDR_EOP)) {
      pBondInfo->bStats.sop++;
      //XTMRT_BOND_LOG0("unexpected sop, total=%d\n", pBondInfo->bStats.sop);
      XTMRT_BOND_LOG0("E-sop\n") ;
      /* abort previous frame and continue with this new one */
      inBuf->rxFkb = NULL ;
		bcmxtmrt_ptmbond_rx_send (pBondInfo, inBuf, XTMRT_PTM_BOND_FRAG_HDR_EOP_DROP);
      inBuf->rxFkb = fkb ;
   }
      
   pBondInfo->expectedSeqNum = NEXT_SEQ_NR(seq_nr);
   pBondInfo->bStats.rfw++;
      
   cache_flush_len (fkb->data, 8) ;
   bcmxtmrt_ptmbond_rx_send (pBondInfo, inBuf, rxFlags);
   return (0) ;
}


/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_rx_send_waiting_fragments
 * Description  : Send and de-queue waiting fragments
 * Returns      : 0 on success.
 ***************************************************************************/
static int bcmxtmrt_ptmbond_rx_send_waiting_fragments (XtmRtPtmBondInfo *pBondInfo, int start)
{
   int   result = 0 ;
   XtmRtPtmBondRxQInfo  *processBuf ;

   XTMRT_BOND_LOG2 ("Flushing from %d\n", start) ;

   processBuf = &pBondInfo->rxq[start] ;
   
   while (processBuf->valid == TRUE) {
      int seq_nr = SEQ_NR(processBuf->fragHdr) ;
      int rxFlags = RX_FLAGS(processBuf->fragHdr) ;

      if (seq_nr == pBondInfo->expectedSeqNum) {
         result |= bcmxtmrt_ptmbond_rx_send_expected_fragment (pBondInfo, processBuf, seq_nr, rxFlags);
      }
      else {
         //XTMRT_BOND_LOG0("unexp seq %d in ordered queue\n", seq_nr);
         XTMRT_BOND_LOG0("E-usq %d Exp %d \n", seq_nr, pBondInfo->expectedSeqNum);
         pBondInfo->bStats.usq ++;
         pBondInfo->expectedSeqNum = NEXT_SEQ_NR(pBondInfo->expectedSeqNum)  ;
         bond_fkb_free (processBuf->rxFkb,8, processBuf->rxdmaIndex) ;
         result |= 1;
      }

      processBuf->valid = FALSE ;
      pBondInfo->rxFragQueued-- ;

      start = Q_INDEX(start+1);
      processBuf = &pBondInfo->rxq[start] ;
      if (!pBondInfo->rxFragQueued)
         break ;
   }

   return (result);
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_rx_resync
 * Description  : Flush queue and resync on error condition
 * Returns      : 0 on success.
 ***************************************************************************/
static int bcmxtmrt_ptmbond_rx_resync (XtmRtPtmBondInfo *pBondInfo, int type, int port)
{
   XtmRtPtmBondRxQInfo  *processBuf ;
   XtmRtPtmBondRxQInfo  *inBuf2 = &pBondInfo->fwd [PTMBOND_SCRATCHPAD_BUF_INDEX2] ;
   int seqStart, qIndex, sendStart = -1;
   int discardedPackets = 0;
   int resyncSequenceNr = -1;
   int result = 0 ;
   
   XTMRT_BOND_LOG2("resync type=%d at expected seq=%d\n", type, pBondInfo->expectedSeqNum) ;

   qIndex = Q_INDEX(pBondInfo->expectedSeqNum);
   /* if we were waiting for an end-of-packet, close the ongoing packet */
   if (!(pBondInfo->lastRxFlags & XTMRT_PTM_BOND_FRAG_HDR_EOP)) {
      //XTMRT_BOND_LOG0("Terminating packet that will never arrive ... (seq=%d)\n",
           //pBondInfo->expectedSeqNum);
      XTMRT_BOND_LOG0("E-end %d\n", pBondInfo->expectedSeqNum);
      inBuf2->rxFkb = NULL ;
		bcmxtmrt_ptmbond_rx_send (pBondInfo, inBuf2, XTMRT_PTM_BOND_FRAG_HDR_EOP_DROP);
      pBondInfo->bStats.end ++;
   }

   if (type & RESYNC_PORT) {
      resyncSequenceNr = Q_INDEX(pBondInfo->lastSeqNum[port]);
   }

   if (type & RESYNC_FLUSH) {
      resyncSequenceNr = XTMRT_PTM_BOND_RX_QUEUE_LEN;
   }

   seqStart = 1 ;
   for (; seqStart<XTMRT_PTM_BOND_RX_QUEUE_LEN; seqStart++) {

      sendStart = Q_INDEX(qIndex+seqStart);
      pBondInfo->expectedSeqNum = NEXT_SEQ_NR(pBondInfo->expectedSeqNum) ;
      if (!pBondInfo->rxFragQueued) {
         break;
      }

      processBuf = &pBondInfo->rxq[sendStart];
      if (processBuf->valid == TRUE) {
         if (!(RX_FLAGS(processBuf->fragHdr) & XTMRT_PTM_BOND_FRAG_HDR_SOP)
               || 
               resyncSequenceNr != -1) {
            processBuf->valid = FALSE ;
            pBondInfo->rxFragQueued-- ;
            discardedPackets ++;
            bond_fkb_free(processBuf->rxFkb,8, processBuf->rxdmaIndex) ;
         }
         else
            break;
      }

#if 0
      /* as of now we can stop throwing away packets and look for  new start */
      if (sendStart == resyncSequenceNr)
         resyncSequenceNr = -1;
#endif
   } /* for (seqStart) */

   if (sendStart != -1) {
      result |= bcmxtmrt_ptmbond_rx_send_waiting_fragments (pBondInfo, sendStart) ;
      /* Too much error, Just flush */
   }

   if (type & RESYNC_FLUSH) 
      pBondInfo->bStats.flu += discardedPackets;
   else if (type & RESYNC_TIMEOUT)
      pBondInfo->bStats.tim += discardedPackets;
   else if (type & RESYNC_PORT)
      pBondInfo->bStats.dro += discardedPackets;
   else if (type & RESYNC_OVERFLOW)
      pBondInfo->bStats.bqo += discardedPackets;
	else if (type & RESYNC_MISSING)
		pBondInfo->bStats.mfd += discardedPackets;
   else 
      pBondInfo->bStats.stp += discardedPackets;

   XTMRT_BOND_LOG1("discarded=%d\n", discardedPackets);
   return (result) ;
}


/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_process_rx_fragment
 * Description  : Receive fragments.  If they come in sequence, assemble and
 *                forward them, else temporarily keep them in their buffer
 *                until the missing packet arrives/timeout condition
 *                happens.
 * Returns      : 0 on success.
 ***************************************************************************/
static int bcmxtmrt_ptmbond_process_rx_fragment (XtmRtPtmBondInfo *pBondInfo, XtmRtPtmBondRxQInfo
                                                 *inBuf)
{
#ifdef PTM_BONDING_DEBUG
   FkBuff_t *fkb = inBuf->rxFkb ;
#endif
   int            port = inBuf->phyPort ;
   int            seq_nr ;
   int            rxFlags ;
   int            qIndex ;
   int            delta = 0;
   int            result = 0;
#ifndef PTMBOND_DS_UNI_CHANNEL
   int            otherPort ;
#endif
   XtmRtPtmBondRxQInfo  *processBuf ;

   seq_nr = SEQ_NR(inBuf->fragHdr);
   rxFlags = RX_FLAGS(inBuf->fragHdr);
   qIndex = Q_INDEX(seq_nr);
    
   XTMRT_BOND_LOG2("{P-%d, L-%d, H-0x%x, H-0x%x},// expected=%d, seq=%d, rxFlags=%x\n", 
       port, fkb->len, fkb->data[0], fkb->data[1], 
       pBondInfo->expectedSeqNum, seq_nr, rxFlags);
   XTMRT_BOND_LOG3_FKB(fkb, "Rx");

#ifdef PTM_BONDING_DEBUG
   if (pBondInfo->seqNum >= MAX_SEQ_DUMP)
      pBondInfo->seqNum = 0 ;
   pBondInfo->seqLen[pBondInfo->seqNum] = fkb->len;
   pBondInfo->seqFlags[pBondInfo->seqNum] = *((XtmRtPtmBondFragHdr*) fkb->data) ;
   pBondInfo->port[pBondInfo->seqNum] = port ;
   pBondInfo->seqNum++ ;
#endif

   PMON_DS_LOG(2) ;

#ifndef PTMBOND_DS_UNI_CHANNEL
   /* verify if the received sequence number is in range */
   if (seq_nr != pBondInfo->expectedSeqNum) {

      if (pBondInfo->lastSeqNum[port] != -1) {
         int deltaOnThisLine = DELTA_SEQ_NR(pBondInfo->lastSeqNum[port], seq_nr);
         if (deltaOnThisLine>0)
            goto _skip1 ;
         if (deltaOnThisLine<0) {
            pBondInfo->bStats.old ++;
            //XTMRT_BOND_LOG0("old sequence=%d, last one on this port=%d, total=%d\n", 
                //seq_nr, pBondInfo->lastSeqNum[port], pBondInfo->bStats.old);
            XTMRT_BOND_LOG0("E-old %d, last one=%d\n", seq_nr, pBondInfo->lastSeqNum[port]) ;
            //dumpaddr (fkb->data, 8) ;
            result |= 1;
         }
         else if (deltaOnThisLine == 0) {
            pBondInfo->bStats.dup ++;
            //XTMRT_BOND_LOG0("duplicate sequence=%d on port %d, total=%d\n",
                //seq_nr, port, pBondInfo->bStats.dup);
            XTMRT_BOND_LOG0("E-dup %d P-%d\n", seq_nr, port) ;
            result |= 1;
         }
      }

_skip1 :
      if (!result) {
         otherPort = port^1 ;
         /* if this is not the first time reception */
         if (pBondInfo->expectedSeqNum != -1) {
            delta = DELTA_SEQ_NR(pBondInfo->expectedSeqNum, seq_nr);
            /* clearly out-of-range sequence number */
            if (ABSOLUTE(delta) > MAX_SEQ_NR_DEVIATION) {
               pBondInfo->bStats.oos ++;
               //XTMRT_BOND_LOG0("out of range seq=%d, delta=%d, lastSeq=%d, total=%d\n",
                     //seq_nr, delta, pBondInfo->lastSeqNum[port], pBondInfo->bStats.oos);
               XTMRT_BOND_LOG0("E-oos %d, delta=%d, lastSeq=%d\n", seq_nr, delta, 
                               pBondInfo->lastSeqNum[port]) ;
               /* if we see this a number of times, force a resync  */
               result |= 1;
            }
            else if (delta<0) {
               /* if delta<0 (old packet, older than what we expect), throw the data away */
               pBondInfo->bStats.old ++;
               //XTMRT_BOND_LOG0("old sequence=%d, expected=%d, total=%d\n", 
                     //seq_nr, pBondInfo->expectedSeqNum, pBondInfo->bStats.old);
               XTMRT_BOND_LOG0("E-old %d, exp=%d \n", seq_nr, pBondInfo->expectedSeqNum) ;
               //dumpaddr (fkb->data, 8) ;
               result |= 1;
            }
            /* FIXME: case for more than two ports not covered here */
            else if (pBondInfo->lastSeqNum[otherPort] != -1) {
               int diff = DELTA_SEQ_NR(pBondInfo->lastSeqNum[otherPort], pBondInfo->expectedSeqNum);
               if (diff<0) {
                  /* This could actually happen if a port had errors for a while */
                  pBondInfo->bStats.ess ++;
                  //XTMRT_BOND_LOG0("unexpected sequence=%d, expected=%d, last=%d, other=%d\n", 
                        //seq_nr, pBondInfo->expectedSeqNum, 
                        //pBondInfo->lastSeqNum[port], pBondInfo->lastSeqNum[otherPort]);
                  XTMRT_BOND_LOG0("E-ess %d, exp=%d, last=%d, other=%d\n",
                        seq_nr, pBondInfo->expectedSeqNum, 
                        pBondInfo->lastSeqNum[port], pBondInfo->lastSeqNum[otherPort]) ;
                  result |= 1;
               }
            }
         }
         else {
               /* Course follows normally */
            if (rxFlags & XTMRT_PTM_BOND_FRAG_HDR_SOP) { /* blindly synchronize */
                  pBondInfo->expectedSeqNum = seq_nr;
               }
               else {         /* throw any packet away until we see a SOP */
                  result |= 1;
                  XTMRT_BOND_LOG0 ("E-flags non SOP %x !!!!!", rxFlags) ;
               }
         } /* else if (pBondInfo->expectedSeqNum != -1) */
      } /* (!result) */
   } /* seq_nr */
#else
   /* verify if the received sequence number is in range */
   if (seq_nr != pBondInfo->expectedSeqNum) {

      if (pBondInfo->expectedSeqNum != -1) {
         delta = DELTA_SEQ_NR(pBondInfo->expectedSeqNum, seq_nr);
      }
      else {
         /* Course follows normally */
         if (rxFlags & XTMRT_PTM_BOND_FRAG_HDR_SOP) { /* blindly synchronize */
            pBondInfo->expectedSeqNum = seq_nr;
         }
         else {         /* throw any packet away until we see a SOP */
            result |= 1;
            XTMRT_BOND_LOG0 ("E-flags non SOP %x !!!!!", rxFlags) ;
         }
      }
   }
#endif /* end of #ifndef PTMBOND_DS_UNI_CHANNEL */

   if (!result) {
      pBondInfo->lastSeqNum[port] = seq_nr;
      if (pBondInfo->expectedSeqNum == seq_nr) {
         result |= bcmxtmrt_ptmbond_rx_send_expected_fragment (pBondInfo, inBuf, seq_nr, rxFlags);
         if (!result) {
            qIndex = Q_INDEX(qIndex+1);
            result |= bcmxtmrt_ptmbond_rx_send_waiting_fragments (pBondInfo, qIndex);
         }
      }
      else {               /* store the fragment on the queue */

         /* is a fragment already pending at that qIndex ? */
         processBuf = &pBondInfo->rxq [qIndex] ;

         if (processBuf->valid == FALSE) {
            processBuf->rxFkb       = inBuf->rxFkb ;
            processBuf->fragHdr     = inBuf->fragHdr ;
            processBuf->pDevCtx     = inBuf->pDevCtx ;
            processBuf->rxFkbStatus = inBuf->rxFkbStatus ;
            processBuf->phyPort     = inBuf->phyPort ;
            processBuf->rxdmaIndex  = inBuf->rxdmaIndex ;
            processBuf->valid       = TRUE ;
            pBondInfo->rxFragQueued++;
         }
         else {
            /* the same sequence number/same slot seq that appears again.
             * more than 1 seq can map onto the same slot, especially, if
             * frag_q_window is full and we are still waiting for the expected
             * seq */
            if (SEQ_NR(processBuf->fragHdr) == seq_nr) {
               pBondInfo->bStats.dup ++;
               //XTMRT_BOND_LOG0("duplicate sequence=%d already on queue %d, total=%d\n",
               //seq_nr, qIndex, pBondInfo->bStats.dup);
               XTMRT_BOND_LOG0("E-dup %d\n", seq_nr) ;
            }
            else { /* queue overflow not detected yet, discard old packet */
               //spin_unlock_bh(&pGi->xtmlock_rx);
               //spin_lock_bh(&pGi->xtmlock_rx);
               pBondInfo->bStats.bqo ++;
               //XTMRT_BOND_LOG0("collision sequence=%d, dropping queue=%d, total=%d\n",
               //seq_nr, qIndex, pBondInfo->bStats.bqo);
               XTMRT_BOND_LOG0("E-bqo %d\n", seq_nr) ;
            }

            bond_fkb_free (inBuf->rxFkb, 8, inBuf->rxdmaIndex) ;
            result |= 1;
         }

         inBuf->valid = FALSE ;

         if (delta >= (XTMRT_PTM_BOND_RX_QUEUE_LEN)) {
            XTMRT_BOND_LOG0("buffer queue overrun\n");
            result =  bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_OVERFLOW, 0) ;
         }
         else if (delta >= inBuf->pDevCtx->ulDsSeqDeviation) {
            PBCMXTMRT_DEV_CONTEXT pDevCtx ;

            pDevCtx = inBuf->pDevCtx ;
				XTMRT_BOND_LOG0("missing fragment\n") ;
				result =  bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_MISSING, 0) ;
            pDevCtx->ulDsSeqDeviation += 2 ;
            if (pDevCtx->ulDsSeqDeviation > pDevCtx->ulDsOrigSeqDeviation)
               pDevCtx->ulDsSeqDeviation = pDevCtx->ulDsOrigSeqDeviation ;
         }
      } /* else store the fragment */
   } /* !result */
   else {
      bond_fkb_free (inBuf->rxFkb, 8, inBuf->rxdmaIndex) ;
      inBuf->valid = FALSE ;
   }

   PMON_DS_LOG(3) ;
 
   if (!result) {
      if (pBondInfo->dropMonitor) 
         pBondInfo->dropMonitor -= SEND_WEIGHT;
   } 
   else {
      pBondInfo->dropMonitor += DROP_WEIGHT;
      /* a number of consecutive drops flush the queue entirely throwing all the
         packets away */
      XTMRT_BOND_LOG0 ("E-Mon %d \n", pBondInfo->dropMonitor) ;
      if (pBondInfo->dropMonitor >= RESYNC_LIMIT) {
         bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_FLUSH, 0); /* ret value is ignored */
         bcmxtmrt_ptmbond_initialize (PTM_BOND_INITIALIZE_LOCAL) ;
         pBondInfo->dropMonitor = 0;
      }
   }

   PMON_DS_LOG(4) ;

   pBondInfo->bStats.rfg[port]++;
   pBondInfo->rxLastFrag = jiffies;
   return result;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_get_port
 * Description  : Find port id of the rxed fragment from the buf status.
 * Returns      : port index.
 ***************************************************************************/
static inline int bcmxtmrt_ptmbond_get_port (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT16 bufStatus)
{
   int retport ;
   volatile UINT32  *portDataMask = &pDevCtx->ulPortDataMask ;

#ifndef PTMBOND_DS_UNI_CHANNEL
   UINT32           vcId ;
   int                   port ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;

   /* ports 0/1 split in even/odd ranges */
   vcId    = bufStatus & FSTAT_MATCH_ID_MASK ;
   port = pGi->pulRxCamBase [vcId << 0x1] ;     /* Access due to buffer not carrying
                                                 * the rx port  information.
                                                 */
   port &= 0x3 ;                          /* Hardcoded for 6368 */

   if (((0x1<<port) & *portDataMask))
      retport = port ;
   else 
      retport = MAX_BOND_PORTS ;

#else
   if (*portDataMask != 0x0)
      retport = DS_PTMBOND_CHANNEL ;
   else 
      retport = MAX_BOND_PORTS ;
#endif

   return (retport) ;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_del_link
 * Description  : Remove a link from the bonding group
 * Returns      : None
 ***************************************************************************/
static inline void bcmxtmrt_ptmbond_del_link(XtmRtPtmBondInfo *pBondInfo, int port)
{
   if (pBondInfo->rxFragQueued) {
#ifndef PTMBOND_DS_UNI_CHANNEL
      XTMRT_BOND_LOG2 ("bcmxtmrt: Resync port %d \n", port) ;
      bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_PORT, port) ;
#else
      bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_FLUSH, 0);
      bcmxtmrt_ptmbond_initialize (PTM_BOND_INITIALIZE_LOCAL) ;
      pBondInfo->dropMonitor = 0;
#endif
   }
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_tick
 * Description  : Performs some periodic checks/update
 * Returns      : None
 ***************************************************************************/
void bcmxtmrt_ptmbond_tick (PBCMXTMRT_DEV_CONTEXT pDevCtx, XtmRtPtmBondInfo *pBondInfo, int timeCheck)
{
   unsigned long oldest_frag = jiffies-RX_SEQ_TIMEOUT;
	unsigned long missing_frag  = jiffies-RX_MISSING_TIMEOUT ;
   XtmRtPtmBondRxQInfo  *inBuf2 = &pBondInfo->fwd [PTMBOND_SCRATCHPAD_BUF_INDEX2] ;
   
   /* detect seq time out.
    * Either there are pending fragments (or) we are waiting for an EOP */

   if ((pBondInfo->rxMissingFrag != 0) && time_after(missing_frag, pBondInfo->rxMissingFrag)) {
      pDevCtx->ulDsSeqDeviation = pDevCtx->ulDsOrigSeqDeviation ;
      pBondInfo->rxMissingFrag = 0 ;
   }

   if (!timeCheck || time_after(oldest_frag, pBondInfo->rxLastFrag)) {

      if (pBondInfo->rxFragQueued) {
         if (bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_TIMEOUT, 0) != 0) {
            bcmxtmrt_ptmbond_rx_resync (pBondInfo, RESYNC_FLUSH, 0) ;
            bcmxtmrt_ptmbond_initialize (PTM_BOND_INITIALIZE_LOCAL) ;
            pBondInfo->dropMonitor = 0;
         }
      }
      else if (!(pBondInfo->lastRxFlags & XTMRT_PTM_BOND_FRAG_HDR_EOP)) {
         //XTMRT_BOND_LOG0("Terminating packet that will never arrive ... (seq=%d)\n",
         //pBondInfo->expectedSeqNum);
         XTMRT_BOND_LOG0("E-end %d\n", pBondInfo->expectedSeqNum) ;
         inBuf2->rxFkb = NULL ;
         bcmxtmrt_ptmbond_rx_send (pBondInfo, inBuf2, XTMRT_PTM_BOND_FRAG_HDR_EOP_DROP) ;
         pBondInfo->bStats.end ++ ;
      }
   } /* (time_after (last frag) */
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_receive_rx_fragment
 * Description  : PTM bonding fragment receive function
 * Returns      : 0 on success
 ***************************************************************************/
void bcmxtmrt_ptmbond_receive_rx_fragment (PBCMXTMRT_DEV_CONTEXT pDevCtx, FkBuff_t *fkb,
                                           UINT16 bufStatus, UINT16 rxdmaIndex)
{
   int result = 1 ;
   int port ;
   XtmRtPtmBondInfo *pBondInfo ;
   XtmRtPtmBondRxQInfo *inBuf ;
   //PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo ;

   PMON_DS_BGN ;

   pBondInfo = &g_GlobalInfo.ptmBondInfo ;
   inBuf     = &pBondInfo->fwd [PTMBOND_SCRATCHPAD_BUF_INDEX] ;

   {
#if 1
   port = bcmxtmrt_ptmbond_get_port (pDevCtx, bufStatus) ;
#else
   port = bufStatus & FSTAT_MATCH_ID_MASK ;

   //pBondInfo->bStats.rfg[port]++;
   //pBondInfo->bStats.rfg[0]++;
#endif
   }
   PMON_DS_LOG(1) ;

   if (port != MAX_BOND_PORTS) {
      inBuf->pDevCtx     = pDevCtx ;
      inBuf->rxFkbStatus = bufStatus ;
      inBuf->rxdmaIndex  = rxdmaIndex ;
      inBuf->rxFkb       = fkb ;
      inBuf->phyPort     = port ;
      inBuf->fragHdr     = *((XtmRtPtmBondFragHdr *)fkb->data) ;
      inBuf->valid       = TRUE ;
#if 1
      result = bcmxtmrt_ptmbond_process_rx_fragment (pBondInfo, inBuf) ;
      inBuf->valid       = FALSE ;
      cache_flush_len (fkb->data, 8) ;
#else
      pBondInfo->bStats.rfg[port]++;
      //pBondInfo->lastRxPktSz = fkb->len + 8 ;
      bond_fkb_free(fkb, 8, rxdmaIndex) ;
#endif
   }
   else {
      pBondInfo->bStats.inl++ ;
      XTMRT_BOND_LOG1("inactive-port error, total=%d\n", pBondInfo->bStats.inl);
      bond_fkb_free(fkb, 8, rxdmaIndex) ;
   }

   PMON_DS_END(4) ;
}

void bcmxtmrt_ptm_receive_and_drop (PBCMXTMRT_DEV_CONTEXT pDevCtx, FkBuff_t *fkb,
                                    UINT16 bufStatus, UINT16 rxdmaIndex)
{
   int port ;
   XtmRtPtmBondInfo *pBondInfo ;

   pBondInfo = &g_GlobalInfo.ptmBondInfo ;
   port = bufStatus & FSTAT_MATCH_ID_MASK ;
   pBondInfo->bStats.rfg[port]++;
   bond_fkb_free(fkb, 8, rxdmaIndex) ;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_handle_link_change
 * Description  : Handle port status change conditions. Link becoming
 *                down will need to be handled for the ReSync operation.
 * Returns      : 0 on success
 ***************************************************************************/
void bcmxtmrt_ptmbond_handle_port_status_change (PBCMXTMRT_DEV_CONTEXT pDevCtx, XtmRtPtmBondInfo *pBondInfo,
                                                 UINT32 ulLinkState, UINT32 dmaSize)
{
#ifndef PTMBOND_DS_UNI_CHANNEL
   int port;
   volatile UINT32 *pulPortMask = &pDevCtx->ulPortDataMask ;
#endif
   int delport ;

   /* on link down we just remove all links associated with slave from the list */
	if (ulLinkState == LINK_DOWN) {
#ifndef PTMBOND_DS_UNI_CHANNEL
      for (port=0; port<MAX_BOND_PORTS; port++) {
         if (!(ulPortMask & (0x1 << port))) {
            delport = port ;
            if (delport != DS_PTMBOND_CHANNEL)
               delport = DS_PTMBOND_CHANNEL ;
            bcmxtmrt_ptmbond_del_link (pBondInfo, delport) ;
      }
   }
#else
      delport = DS_PTMBOND_CHANNEL ;
      bcmxtmrt_ptmbond_del_link (pBondInfo, delport) ;
#endif
   } /* if (linkstate == down) */

   if (ulLinkState != LINK_UP) {
      pDevCtx->ulDsSeqDeviation = pBondInfo->rxDsReducedDeviation ;
      pBondInfo->rxMissingFrag = jiffies ;
   }
}

static int __get_bond_rx_ctrs (char *page)
{
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    XtmRtPtmBondInfo *pBondInfo ;
    int sz = 0;

    if (pGi->bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) {

       pBondInfo = &pGi->ptmBondInfo ;

       sz += sprintf(page + sz, "\nPTM Rx Bonding Information \n") ;
       sz += sprintf(page + sz, "\n========================== \n") ;
       sz += sprintf(page + sz, "Rx Expected Seq                  : %lu \n", (long unsigned int) pBondInfo->expectedSeqNum) ;
       sz += sprintf(page + sz, "Rx Number Of Outstanding Frags   : %lu \n", (long unsigned int) pBondInfo->rxFragQueued) ;
       sz += sprintf(page + sz, "Rx Last Seq [0]                  : %lu \n", (long unsigned int) pBondInfo->lastSeqNum[0]) ;
       sz += sprintf(page + sz, "Rx Last Seq [1]                  : %lu \n", (long unsigned int) pBondInfo->lastSeqNum[1]) ;
       sz += sprintf(page + sz, "Rx Forwarded Fragments           : %lu \n", (long unsigned int) pBondInfo->bStats.rfw) ;
       sz += sprintf(page + sz, "Rx Forwarded Octets              : %lu \n", (long unsigned int) pBondInfo->bStats.rxo) ;
       sz += sprintf(page + sz, "Rx Forwarded Packets             : %lu \n", (long unsigned int) pBondInfo->bStats.rxp) ;
       sz += sprintf(page + sz, "Rx Forwarded Packets (NonAccel)  : %lu \n", (long unsigned int) pBondInfo->bStats.rxpl) ;
       sz += sprintf(page + sz, "Rx Dropped Packets (NonAccel)    : %lu \n", (long unsigned int) pBondInfo->bStats.rxd) ;
       sz += sprintf(page + sz, "Rx Old Seq Nums Rcvd (Past)      : %lu \n", (long unsigned int) pBondInfo->bStats.old) ;
       sz += sprintf(page + sz, "Rx duplicated Seq Rcvd           : %lu \n", (long unsigned int) pBondInfo->bStats.dup) ;
       sz += sprintf(page + sz, "Rx Out Of Sync Fragments Rcvd    : %lu \n", (long unsigned int) pBondInfo->bStats.oos) ;
       sz += sprintf(page + sz, "Rx UnExp Seq Num From Q Store    : %lu \n", (long unsigned int) pBondInfo->bStats.usq) ;
       sz += sprintf(page + sz, "Rx UnExp InSeq Mid/End Of Pkt    : %lu \n", (long unsigned int) pBondInfo->bStats.eop) ;
       sz += sprintf(page + sz, "Rx UnExp InSeq Start Of Pkt      : %lu \n", (long unsigned int) pBondInfo->bStats.sop) ;
       sz += sprintf(page + sz, "Rx Exp Seq Number Skip           : %lu \n", (long unsigned int) pBondInfo->bStats.ess) ;
       sz += sprintf(page + sz, "Rx Artificially Ended Pkts       : %lu \n", (long unsigned int) pBondInfo->bStats.end) ;
       sz += sprintf(page + sz, "Rx Drop due to Flush/Buf Q ORun  : %lu \n", (long unsigned int) pBondInfo->bStats.bqo) ;
       sz += sprintf(page + sz, "Rx Drop due to Missing Fragment  : %lu \n", (long unsigned int) pBondInfo->bStats.mfd) ;
       sz += sprintf(page + sz, "Rx Drop due to Startup syncing   : %lu \n", (long unsigned int) pBondInfo->bStats.stp) ;
       sz += sprintf(page + sz, "Rx Drop due to Flush             : %lu \n", (long unsigned int) pBondInfo->bStats.flu) ;
       sz += sprintf(page + sz, "Rx Drop due to Exp Seq Num TmOut : %lu \n", (long unsigned int) pBondInfo->bStats.tim) ;
       sz += sprintf(page + sz, "Rx Drop due to Port Out Os Status: %lu \n", (long unsigned int) pBondInfo->bStats.dro) ;
       sz += sprintf(page + sz, "Rx Fragment Rcvd From InAct Port : %lu \n", (long unsigned int) pBondInfo->bStats.inl) ;
       sz += sprintf(page + sz, "Times of Buffer Expand And CopyDS: %lu \n", (long unsigned int) pBondInfo->bStats.cpe) ;

       sz += sprintf(page + sz, "Rx Fragment Port [0]             : 0x%x %lu \n", (unsigned int) pBondInfo->bStats.rfg[0],
                     (long unsigned int) pBondInfo->bStats.rfg[0]) ;
       sz += sprintf(page + sz, "Rx Fragment Port [1]             : 0x%x %lu \n", (unsigned int) pBondInfo->bStats.rfg[1],
                     (long unsigned int) pBondInfo->bStats.rfg[1]) ;
       sz += sprintf(page + sz, "Total Fragments                  : %ld \n", (long unsigned int) (pBondInfo->bStats.rfg[0]+pBondInfo->bStats.rfg[1])) ;
       sz += sprintf(page + sz, "Last Rx Flags                    : %x  \n", (unsigned int) (pBondInfo->lastRxFlags)) ;
       sz += sprintf(page + sz, "Fwd Validity                     : %lu  \n", (long unsigned int) (pBondInfo->fwd[PTMBOND_FWD_BUF_INDEX].valid)) ;

       memset (&pBondInfo->bStats, 0, sizeof (struct _bStats)) ;
    }
    else {
       sz += sprintf(page + sz, "No Bonding Information \n") ;
    }

    return( sz );
} /* ProcRxBondCtrs */

/***************************************************************************
 * Function Name: ProcRxBondCtrs
 * Description  : Displays information about Bonding Rx side counters.
 *                Currently for PTM bonding.
 * Returns      : 0 if successful or error status
 ***************************************************************************/
int ProcRxBondCtrs(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int sz=0;
    if(*offset == 0)
    { 
        *offset = sz =__get_bond_rx_ctrs(buf);
    }
    return sz;
}

static int __get_bond_rx_seq0 (char *page)
{
    int sz = 0;
#ifdef PTM_BONDING_DEBUG
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    XtmRtPtmBondInfo *pBondInfo ;
    UINT32 seqCount ;

    if (pGi->bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) {

       pBondInfo = &pGi->ptmBondInfo ;

       /* Rx Seq Information */
       for (seqCount = 0; seqCount < MAX_SEQ_DUMP/2; seqCount++) {
         if (!(seqCount%6))
            sz += sprintf(page + sz, "\n") ;
         sz += sprintf(page + sz, "L%u, ", pBondInfo->seqLen[seqCount]) ;
       }
    }
    else {
#endif
       sz += sprintf(page + sz, "No Bonding Rx Sequence Information \n") ;
#ifdef PTM_BONDING_DEBUG
    }
#endif

    return( sz );
} /* ProcRxBondSeq0 */

/***************************************************************************
 * Function Name: ProcRxBondSeq0
 * Description  : Displays information about Bonding Rx side Sequences
 *                0-499.
 *                Currently for PTM bonding.
 * Returns      : 0 if successful or error status
 ***************************************************************************/
int ProcRxBondSeq0(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int sz=0;
    if(*offset == 0)
    { 
        *offset = sz =__get_bond_rx_seq0(buf);
    }
    return sz;
}

static int __get_bond_rx_seq1 (char *page)
{
    int sz = 0;

#ifdef PTM_BONDING_DEBUG
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    XtmRtPtmBondInfo *pBondInfo ;
    UINT32 seqCount ;

    if (pGi->bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) {

       pBondInfo = &pGi->ptmBondInfo ;

       /* Rx Seq Information */
       for (seqCount = MAX_SEQ_DUMP/2; seqCount < MAX_SEQ_DUMP; seqCount++) {
         if (!(seqCount%6))
            sz += sprintf(page + sz, "\n") ;
         sz += sprintf(page + sz, "L%u, ", pBondInfo->seqLen[seqCount]) ;
       }
    }
    else {
#endif
       sz += sprintf(page + sz, "No Bonding Rx Sequence Information \n") ;
#ifdef PTM_BONDING_DEBUG
    }
#endif

    return( sz );
} /* ProcRxBondSeq1 */

/***************************************************************************
 * Function Name: ProcRxBondSeq1
 * Description  : Displays information about Bonding Rx side Sequences
 *                500-999.
 *                Currently for PTM bonding.
 * Returns      : 0 if successful or error status
 ***************************************************************************/
int ProcRxBondSeq1(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int sz=0;
    if(*offset == 0)
    { 
        *offset = sz =__get_bond_rx_seq1(buf);
    }
    return sz;
}

static int __get_bond_tx_info (char *page)
{
    PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;
    PBCMXTMRT_DEV_CONTEXT pDevCtx;
    XtmRtPtmBondInfo *pBondInfo = &pGi->ptmBondInfo ;
    volatile XtmRtPtmTxBondHeader *pPtmHeader;
    UINT32 i ;
    int sz = 0, port, eopStatus, fragSize ;

    if (pGi->bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) {

       pPtmHeader = &pGi->ptmBondHdr [0] ;

       sz += sprintf(page + sz, "\nPTM Tx Bonding Information \n") ;
       sz += sprintf(page + sz, "\n========================== \n") ;

       sz += sprintf(page + sz, "\nPTM Header Information") ; 
       
       for (i=0; i<XTMRT_PTM_BOND_MAX_FRAG_PER_PKT; i++) {
          
          port       = pPtmHeader[i].sVal.portSel ;
          eopStatus  = pPtmHeader[i].sVal.PktEop ;
          fragSize   = pPtmHeader[i].sVal.FragSize ;
          fragSize   -= (XTMRT_PTM_BOND_FRAG_HDR_SIZE+
                        XTMRT_PTM_CRC_SIZE) ;
          if (eopStatus == 1) {
             sz += sprintf(page + sz, "\nFragment[%u]: port-%d, eopStatus-%d, size-%d \n",
                           (unsigned int)i, port, eopStatus, fragSize-ETH_FCS_LEN) ;
             break ;
          }
          else {
             sz += sprintf(page + sz, "\nFragment[%u]: port-%u, eopStatus-%u, size-%u \n",
                           (unsigned int)i, port, eopStatus, fragSize) ;
          }
       } /* for (i) */

       for( i = 0; i < MAX_DEV_CTXS; i++ )
       {
          pDevCtx = pGi->pDevCtxs[i];
          if ( pDevCtx != (PBCMXTMRT_DEV_CONTEXT) NULL )
          {
             sz += sprintf(page + sz, "dev: %s, ActivePortMask: %u \n",
                   pDevCtx->pDev->name, (unsigned int) pDevCtx->ulPortDataMask) ;

          }
       }

       for( i = 0; i < MAX_BOND_PORTS; i++ )
       {
          sz += sprintf(page + sz, "Port %u ConfWt : %u CurrWt : %u\n",
                (unsigned int) i, (unsigned int) pBondInfo->ulConfLinkUsWt[i], (unsigned int) pBondInfo->ulLinkUsWt[i]) ;

       }

       sz += sprintf(page + sz, "totalWtPortDist : %u CurrWtPortDistStartIndex : %u "
             "CurrWtPortDistRunIndex : %u CurrBeforeReset : %u \n", (unsigned int) pBondInfo->totalWtPortDist, 
             (unsigned int) pBondInfo->ulCurrWtPortDistStartIndex, (unsigned int) pBondInfo->ulCurrWtPortDistRunIndex,
             (unsigned int) pBondInfo->ulCurrWtTotalIterationsBeforeReset) ;

       sz += sprintf(page + sz, "All Wt Port Distributions ==>") ;

       for( i = 0; i < pBondInfo->totalWtPortDist; i++ ) {
          sz += sprintf(page + sz, " %u,", pBondInfo->u8ConfWtPortDist[i]) ;
       }

       sz += sprintf(page + sz, "\n") ;
    }
    else {
       sz += sprintf(page + sz, "No Bonding Information \n") ;
    }

    return( sz );
} /* ProcTxBondInfo */


/***************************************************************************
 * Function Name: ProcTxBondInfo
 * Description  : Displays information about Bonding Tx side counters.
 *                Currently for PTM bonding.
 * Returns      : 0 if successful or error status
 ***************************************************************************/
int ProcTxBondInfo(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    int sz=0;
    if(*offset == 0)
    { 
        *offset = sz =__get_bond_tx_info(buf);
    }
    return sz;
}

/***************************************************************************
 * Function Name: bcmxtmrt_ptmbond_initialize
 * Description  : Initialize ptm bonding information.
 * Returns      : None
 ***************************************************************************/
void bcmxtmrt_ptmbond_initialize (int globalInit)
{
   int i ;
   PBCMXTMRT_GLOBAL_INFO pGi = &g_GlobalInfo;

   if (globalInit) {
      printk (CARDNAME ": PTM Bonding configured in system \n") ;
#ifdef PTM_BONDING_DEBUG
      pGi->ptmBondInfo.logLevel = 1 ;
#endif

      //printk (CARDNAME ": PTM Bonding data size - %d \n", sizeof (XtmRtPtmBondInfo)) ;
      //pGi->ptmBondInfo.lastTxPktSz   = 0 ;

      PMON_US_REG(1, "XTMBOND: Get a right size FKB") ;
      PMON_US_REG(2, "XTMBOND: Construct PTM Hdr/Frags") ;
      PMON_US_REG(3, "XTMBOND: Prepare FKB space for frag hdr") ;
      PMON_US_REG(4, "XTMBOND: COPY PTM Hdr/Frags to Tx Buf") ;
      PMON_US_REG(5, "XTMBOND: Return FKB to caller") ;

      PMON_DS_REG(1, "XTMBOND: Rx Fragment Processing Get Port") ;
      PMON_DS_REG(2, "XTMBOND: Rx Fragment Processing Get Seq Info") ;
      PMON_DS_REG(3, "XTMBOND: Rx Fragment Processing exp/non-exp") ;
      PMON_DS_REG(4, "XTMBOND: Rx Fragment Processing Drop Logic due to error") ;
      pGi->noOfNonAccelPktsDSInSec = 0 ;
      pGi->ptmBondInfo.rxDsReducedDeviation = XTM_DEF_REDUCED_MISSING_DS_MAX_DEVIATION ;
   }

   pGi->ptmBondInfo.expectedSeqNum = -1 ;
   for (i=0; i<MAX_BOND_PORTS; ++i) {
      pGi->ptmBondInfo.lastSeqNum[i] = -1;
   }


   pGi->ptmBondInfo.rxFragQueued = 0 ;
   pGi->ptmBondInfo.lastRxFlags = XTMRT_PTM_BOND_FRAG_HDR_EOP ; /* allow the first SOP */
   //pGi->ptmBondInfo.lastRxPktSz   = 512 ;
}
