/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
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

/****************************************************************************
 *
 * BcmAdslDiagLinux.c -- Linux speciifc DslDiag support functions
 *
 * Description:
 *	Linux speciifc DslDiag support functions
 *
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.3 $
 *
 * $Id: BcmAdslDiagLinux.c,v 1.3 2004/07/20 23:45:48 ilyas Exp $
 *
 * $Log: BcmAdslDiagLinux.c,v $
 * Revision 1.3  2004/07/20 23:45:48  ilyas
 * Added driver version info, SoftDslPrintf support. Fixed G.997 related issues
 *
 * Revision 1.2  2004/06/10 00:20:33  ilyas
 * Added L2/L3 and SRA
 *
 * Revision 1.1  2004/04/14 21:11:59  ilyas
 * Inial CVS checkin
 *
 ****************************************************************************/

/* Includes. */
#include "BcmOs.h"

#include <linux/types.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
#include <linux/module.h> 
#endif
#define	_SYS_TYPES_H

#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>

#include <net/ip.h>
#include <net/route.h>
#include <net/arp.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
#include <linux/proc_fs.h>
#endif

//#define TEST_CMD_BUF_RING

#ifdef TEST_CMD_BUF_RING
#include <linux/timer.h>
#include <linux/jiffies.h>
#endif
#include <linux/vmalloc.h>

#if defined(SUPPORT_2CHIP_BONDING)
#include "bcm_ext_bonding_comm.h"
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
#include "bcm_common.h"
#include "bcm_map.h"
#include "bcm_intr.h"
#else
#include "6345_common.h"
#include "6345_map.h"
#include "6345_intr.h"
#endif
#include "board.h"

#include "bcmadsl.h"
#include "BcmAdslCore.h"
#include "AdslCore.h"
#include "AdslCoreMap.h"

#define EXCLUDE_CYGWIN32_TYPES
#include "softdsl/SoftDsl.h"

#include "BcmAdslDiag.h"
#include "BcmAdslDiagCommon.h"

#include <bcmxtmcfg.h>
#include <asm/io.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#include <net/net_namespace.h>
#endif

extern stretchBufferStruct * volatile pPhySbCmd;

extern DiagConnectInfo diagConnectInfo;

#define DMA_INTR_MASK		(ADSL_INT_RCV | ADSL_INT_RCV_FIFO_OF | ADSL_INT_RCV_DESC_UF | \
							ADSL_INT_DESC_PROTO_ERR |ADSL_INT_DATA_ERR |ADSL_INT_DESC_ERR)
#define DMA_ERROR_MASK	(ADSL_INT_RCV_FIFO_OF | ADSL_INT_RCV_DESC_UF | \
							ADSL_INT_DESC_PROTO_ERR |ADSL_INT_DATA_ERR |ADSL_INT_DESC_ERR)
#define PHY_INTR_ENABLE(x)	(x[ADSL_INTMASK_F] |= DMA_INTR_MASK); \
							(x[ADSL_INTMASK_I] |= ADSL_INT_HOST_MSG)
#define PHY_INTR_DISABLE(x)	(x[ADSL_INTMASK_F]  &= ~DMA_INTR_MASK); \
							(x[ADSL_INTMASK_I] &= ~ADSL_INT_HOST_MSG)

					
/*
**
**	Socket diagnostic support
**
*/


#define	DIAG_SKB_USERS		0x3FFFFFFF

#define SKB_PRE_ALLOC_SIZE	1536
#define	NUM_OF_SKBS_IN_POOL	16

#define SHORT_SKB_PRE_ALLOC_SIZE	128
#define NUM_OF_SHORT_SKBS_IN_POOL	64

#define DIAG_MAX_NUM_FRAG		100

/* Local vars */

void    *diagDescMemStart = NULL;
ulong   diagDescMemSize = 0;

dslDrvSkbPool * DevSkbAllocate(struct sk_buff *model,
		int skbMaxBufSize, int numOfSkbsInPool,
		int shortSkbMaxBufSize, int numOfShortSkbsInPool,
		int dataAlignMask, int frameHeaderLength, int dmaZone)
{
	int			 n;
	struct sk_buff	*skb = NULL;
	dslDrvSkbPool	* skbDev = NULL;
	struct sk_buff ** ppskb;
	gfp_t	gfp_mask;

	if(dmaZone)
		gfp_mask = (GFP_ATOMIC | GFP_DMA);
	else
		gfp_mask = GFP_ATOMIC;
		
	skbDev = (dslDrvSkbPool *) kmalloc(sizeof(*skbDev) + sizeof(struct sk_buff **) * (numOfSkbsInPool+numOfShortSkbsInPool), GFP_ATOMIC);
	if (skbDev != NULL) {
		skbDev->skbPool = (struct sk_buff **)(skbDev + 1);
		ppskb = skbDev->skbPool;
		for (n = 0; n < numOfSkbsInPool; n++) {
			skb = alloc_skb (skbMaxBufSize+dataAlignMask, gfp_mask);
			if(skb != NULL) {
				skb->dev = model->dev;
				skb->protocol = htons(eth_type_trans (skb, model->dev));
				skb->data = DIAG_DATA_ALIGN(skb->head, dataAlignMask);
				memcpy(skb->data, model->data, frameHeaderLength);
				atomic_set(&skb->users, DIAG_SKB_USERS-1);
				ppskb[n] = skb;
			} else {
				n--;
				break;
			}
		}
		skbDev->numOfSkbs = n;
		for (; n < (skbDev->numOfSkbs+numOfShortSkbsInPool); n++) {
			skb = alloc_skb (shortSkbMaxBufSize+dataAlignMask, gfp_mask);
			if(skb != NULL) {
				skb->dev = model->dev;
				skb->protocol = htons(eth_type_trans (skb, model->dev));
				skb->data = DIAG_DATA_ALIGN(skb->head, dataAlignMask);
				memcpy(skb->data, model->data, frameHeaderLength);
				atomic_set(&skb->users, DIAG_SKB_USERS-1);
				ppskb[n] = skb;
			} else {
				n--;
				break;
			}
		}
		
		skbDev->numOfShortSkbs = n - skbDev->numOfSkbs;
		skbDev->skbLengh = skbMaxBufSize;
		skbDev->shortSkbLengh = shortSkbMaxBufSize;
		skbDev->skbBufIndex = 0;
		skbDev->shortSkbBufIndex = skbDev->numOfSkbs;
		skbDev->frameHeaderLen = frameHeaderLength;
		skbDev->dataAlignMask = dataAlignMask;
		skbDev->extraSkb = 0;
	}
	return skbDev;
}

/* len is total length not including frame and pad */
struct sk_buff * GetSkb(dslDrvSkbPool *skbDev, int len)
{
	struct sk_buff	*skbDiag = NULL;
	struct sk_buff	*skbRef = NULL;
	struct sk_buff	** ppskb = skbDev->skbPool;
	int	totalLen = (len + skbDev->frameHeaderLen);
	
	BcmCoreDpcSyncEnter();
	if(totalLen < skbDev->shortSkbLengh) {
		if (DIAG_SKB_USERS != atomic_read(&(ppskb[skbDev->shortSkbBufIndex])->users)) {
			skbDiag = ppskb[skbDev->shortSkbBufIndex];
			skbDev->shortSkbBufIndex++;
			if (skbDev->shortSkbBufIndex == (skbDev->numOfSkbs + skbDev->numOfShortSkbs))
				skbDev->shortSkbBufIndex -= skbDev->numOfShortSkbs;
		}
	} else if (DIAG_SKB_USERS != atomic_read(&(ppskb[skbDev->skbBufIndex])->users)) {
		skbDiag = ppskb[skbDev->skbBufIndex];
		skbDev->skbBufIndex++;
		if (skbDev->skbBufIndex == skbDev->numOfSkbs)
			skbDev->skbBufIndex -= skbDev->numOfSkbs;
	}
	
	if (skbDiag != NULL) {
		atomic_set(&skbDiag->users, DIAG_SKB_USERS);
		skbDiag->data = DIAG_DATA_ALIGN(skbDiag->head, skbDev->dataAlignMask);
		skbDiag->len = skbDev->frameHeaderLen;
	}
	else if( NULL != (skbDiag = alloc_skb (totalLen+skbDev->dataAlignMask, GFP_ATOMIC)) ) {
		skbDev->extraSkb++;
		skbRef = ppskb[0];
		skbDiag->data = DIAG_DATA_ALIGN(skbDiag->head, skbDev->dataAlignMask);
		skbDiag->dev = skbRef->dev;
		skbDiag->protocol = htons(eth_type_trans (skbDiag, skbRef->dev));
		memcpy(skbDiag->data, DIAG_DATA_ALIGN(skbRef->head, skbDev->dataAlignMask), skbDev->frameHeaderLen);
		skbDiag->len = skbDev->frameHeaderLen;
	}
	
	BcmCoreDpcSyncExit();
	
	return skbDiag;
}

int DevSkbFree(dslDrvSkbPool *skbDev, int enableWA){
	OS_TICKS	osTime0;
	int			n;
	struct sk_buff	*skb;
	struct sk_buff	*skbRef;
	struct sk_buff ** ppskb = skbDev->skbPool;
	Boolean freeSkbDev = 0;
	int	totalSkbs = skbDev->numOfSkbs + skbDev->numOfShortSkbs;
	
	do {
		for (n = 0; n < totalSkbs; n++) {
			skb = ppskb[n];
			if ( DIAG_SKB_USERS == atomic_read(&skb->users ) )
				break;
		}
		if ( n >= totalSkbs ){
			freeSkbDev = 1;
			break;
		}
		if(enableWA) {
			/* Work-around: send a dummy (DIAG PING) packet so the system will release the skbs */
			skb = alloc_skb (skbDev->dataAlignMask + DIAG_FRAME_HEADER_LEN + 4, GFP_ATOMIC);
			if(skb != NULL) {
				char buf[4];
				skbRef = ppskb[0];
				skb->dev = skbRef->dev;
				skb->protocol = htons(eth_type_trans(skb, skbRef->dev));
				skb->data = DIAG_DATA_ALIGN(skb->head, skbDev->dataAlignMask);
				memcpy(skb->data, DIAG_DATA_ALIGN(skbRef->head, skbDev->dataAlignMask), skbDev->frameHeaderLen);
				skb->len = skbDev->frameHeaderLen;
				__DiagWriteData(skb, LOG_CMD_PING_REQ, buf, 2, NULL, 0);
			}
			/* Loop 10 Ms */
			bcmOsGetTime(&osTime0);
			while (BcmAdslCoreOsTimeElapsedMs(osTime0) < 10);
		}
	} while (enableWA);

	if(freeSkbDev)
	{
		for (n = 0; n < totalSkbs; n++) {
			skb = ppskb[n];
			atomic_set(&skb->users, 1);
			kfree_skb(skb);
		}
		kfree(skbDev);
	}
	
	return(freeSkbDev);
}


static int __GdbWriteData(struct sk_buff *skb, char *buf0, int len0)
{
	diagSockFrame		*dd;
	int					n;

	dd = (diagSockFrame *)((ulong) skb->data-DIAG_FRAME_PAD_SIZE);
	DiagUpdateDataLen(dd, len0 - sizeof(LogProtoHeader));

	memcpy (dd->diagData-sizeof(LogProtoHeader), buf0, len0);

	skb->len  = DIAG_FRAME_HEADER_LEN + len0 - sizeof(LogProtoHeader);
	skb->tail = skb->data + skb->len;

	n = DEV_TRANSMIT(skb);

	return n;
}

int GdbWriteData(char *buf0, int len0)
{
  struct sk_buff *skb;
#ifdef DEBUG_GDB_STUB
  int i;
#endif
  int dataAlignMask;
  dataAlignMask = 3;

  if (NULL == diagCtrl.gdbDev)
    return 0;

#ifdef DEBUG_GDB_STUB
  printk("GdbWriteData::'");
  for (i = 0; i < len0; i++) 
    printk("%c", buf0[i]);
  printk("' len=%d\n", len0);
#endif

  skb = alloc_skb (dataAlignMask + DIAG_FRAME_HEADER_LEN + len0 + 16, GFP_ATOMIC);
  if (NULL == skb)
    return -ENOMEM;

  if(diagCtrl.skbGdb != NULL){
    skb->dev = diagCtrl.gdbDev;
    skb->protocol = htons(eth_type_trans (skb, diagCtrl.gdbDev));
    skb->data = DIAG_DATA_ALIGN(skb->head, dataAlignMask);
    memcpy(skb->data, diagCtrl.skbGdb->data, DIAG_FRAME_HEADER_LEN);
    return __GdbWriteData(skb, buf0, len0);
  }
  return -ENOMEM;
}

#ifdef SUPPORT_EXT_DSL_BONDING_SLAVE
extern void BcmXdslCoreSendStatToExtBondDev(struct sk_buff *skbDiag, ulong cmd, char *buf0, int len0, char *buf1, int len1);

int BcmXdslCoreDiagGetFrHdrLen(void)
{
	return DIAG_FRAME_HEADER_LEN;
}
#endif

#if defined(SUPPORT_EXT_DSL_BONDING_MASTER)
int BcmXdslCoreExtBondDiagWriteData(EXT_BONDING_COMM_BufferDesc *pBufDesc)
{
	struct sk_buff *skb;
	int n, alignMask;
	diagSockFrame *dd;
	alignMask = 3;

	if ((NULL == diagCtrl.dbgDev) || (NULL == diagCtrl.skbModel))
		return;
	
	n = EXT_BONDING_COMM_getBufferSize(pBufDesc);
	skb = alloc_skb (alignMask + DIAG_FRAME_HEADER_LEN + (n-sizeof(LogProtoHeader)) + 16, GFP_ATOMIC);
	if (NULL == skb)
		return -ENOMEM;
	
	skb->dev = diagCtrl.dbgDev;
	skb->protocol = htons(eth_type_trans (skb, diagCtrl.dbgDev));
	skb->data = DIAG_DATA_ALIGN(skb->head, alignMask);
	skb->len  = DIAG_FRAME_HEADER_LEN + (n-sizeof(LogProtoHeader));
	skb->tail = skb->data + skb->len;
	memcpy(skb->data, diagCtrl.skbModel->data, DIAG_FRAME_HEADER_LEN);
	dd = (diagSockFrame *)((ulong)skb->data-DIAG_FRAME_PAD_SIZE);
	memcpy((void*)(&dd->diagHdr), EXT_BONDING_COMM_getData(pBufDesc), n);
	DiagUpdateDataLen(dd, (n-sizeof(LogProtoHeader)));
#if defined(VECTORING)
	/* Assumming VECTORING is built based on new Linux where this function is available in skbuff.h */
	/* This is to prevent "protocol x is buggy" prints from the kernel */
	skb_set_network_header(skb, sizeof(struct ethhdr));
#endif
	return DEV_TRANSMIT(skb);
}
#endif

int __DiagWriteData(struct sk_buff *skbDiag, ulong cmd, char *buf0, int len0, char *buf1, int len1)
{
	diagSockFrame		*dd;
	int					n;
	ulong				lineId, clientType;

	if (skbDiag == NULL)
		return 0;
	dd = (diagSockFrame *) ((ulong)skbDiag->data-DIAG_FRAME_PAD_SIZE);
	DiagUpdateDataLen(dd, len0+len1);

	if (cmd & DIAG_MSG_NUM) {
		*(short *)dd->diagHdr.logProtoId = (ushort)diagCtrl.diagDmaLogBlockNum;
		diagCtrl.diagDmaLogBlockNum++;
	}
	else {
		*(short *)dd->diagHdr.logProtoId = *(short *) LOG_PROTO_ID;
		if (cmd & DIAG_SPLIT_MSG)
			dd->diagHdr.logProtoId[1] += 1;
	}
	lineId = (cmd & DIAG_LINE_MASK) >> DIAG_LINE_SHIFT;
	clientType = (cmd & DIAG_TYPE_MASK) >> DIAG_TYPE_SHIFT;
	dd->diagHdr.logPartyId = LOG_PARTY_CLIENT | (lineId << DIAG_PARTY_LINEID_SHIFT)  | (clientType << DIAG_PARTY_TYPE_SHIFT);
	dd->diagHdr.logCommmand = cmd & 0xFF;
	memcpy (dd->diagData, buf0, len0);
	if (NULL != buf1)
		memcpy (dd->diagData+len0, buf1, len1);

	skbDiag->len += len0 + len1;
	skbDiag->tail = skbDiag->data + skbDiag->len;

#if defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
	if (clientType == DIAG_DSL_CLIENT)
		BcmXdslCoreSendStatToExtBondDev(skbDiag, 0, NULL, 0, NULL, 0);
#endif

#if defined(VECTORING)
	/* Assumming VECTORING is built based on new Linux where this function is available in skbuff.h */
	/* This is to prevent "protocol x is buggy" prints from the kernel */
	skb_set_network_header(skbDiag, sizeof(struct ethhdr));
#endif
	/*	DiagPrintData(skbDiag); */
	n = DEV_TRANSMIT(skbDiag);
	return n;
}

void DiagUpdateSkbForDmaBlock(diagDmaBlock *db, int len)
{
	DiagUpdateDataLen((diagSockFrame *)&db->dataFrame.pad[0], len);
	*(ulong*)&db->dataFrame.diagHdr = *(ulong*)&db->diagHdrDma;
	db->skb.data = (void *)&db->dataFrame.dstMacAddr;
	db->skb.len  = DIAG_FRAME_HEADER_LEN + len;
	db->skb.tail = db->skb.data + DIAG_FRAME_HEADER_LEN + len;
	/* DiagPrintData(skbDiag); */
}

static struct in_ifaddr * BcmAdslCoreGetIntf(struct net_device	*dev, char *pDevName, ulong ipAddr)
{
	struct in_ifaddr		*ifa, **ifap;
	struct in_device	*in_dev;
	char			buf[16], tmpBuf[16];

	/* get device IP address */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
	in_dev = __in_dev_get_rtnl(dev);
#else
	in_dev = __in_dev_get(dev);
#endif
	if (NULL == in_dev)
		return NULL;

	for (ifap=&in_dev->ifa_list; (ifa=*ifap) != NULL; ifap=&ifa->ifa_next) {
		printk ("\tifa_label = %s, ipAddr = %s, mask = %s\n",
			ifa->ifa_label, ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(ifa->ifa_local), buf), ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(ifa->ifa_mask),tmpBuf));
		sprintf(tmpBuf, "%s:0", pDevName);	/* Construct the secondary interface name */
		if ((0 == strcmp(pDevName, ifa->ifa_label)) ||
			(0 == strcmp(tmpBuf, ifa->ifa_label))) {
			if ((ifa->ifa_local & ifa->ifa_mask) == (ipAddr & ifa->ifa_mask)) {
				printk("\tMatched network address found!\n");
				return ifa;
			}
		}
	}
	return NULL;
}

struct net_device * 
BcmAdslCoreInitNetDev(PADSL_DIAG pAdslDiag, int port, struct sk_buff **ppskb, char *devname)
{
	char				*diagDevNames[] = {"eth0", "br1", "br0"};
	struct net_device	*dev = NULL;
	struct in_ifaddr	*ifa;
	struct rtable		rtbl;
	diagSockFrame		*dd;
	int			n, dstPort;
	struct sk_buff	*pskb = *ppskb;
	char			buf[16], tmpBuf[16];
	int				nDevNames = sizeof(diagDevNames)/sizeof(diagDevNames[0]);
	
	dstPort = ((ulong) pAdslDiag->diagMap) >> 16;
	if (0 == dstPort)
		dstPort = port;
	
	printk ("Drv%sCmd: CONNECT map=0x%X, logTime=%d, gwIpAddr=%s, dstPort=%d\n",
		devname, pAdslDiag->diagMap&0xffff, pAdslDiag->logTime,
		ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(pAdslDiag->gwIpAddr),buf), dstPort);
	
	/* find DIAG interface device */
	ifa = NULL;
	printk ("Searching for interface that has matching network address w/ srvIpaddr %s\n", ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(pAdslDiag->srvIpAddr), buf));
	
	if(0 != diagConnectInfo.devName[0]) {
		diagDevNames[0] = diagConnectInfo.devName;
		nDevNames = 1;
	}
	
	for (n = 0; n < nDevNames; n++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
		dev = __dev_get_by_name(&init_net, diagDevNames[n]);
#else
		dev = __dev_get_by_name(diagDevNames[n]);
#endif
		if (NULL == dev)
			continue;
		printk ("\tdev = %s(0x%X) hwAddr=%X:%X:%X:%X:%X:%X\n",
			diagDevNames[n], (int) dev,
			dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
			dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
		if(NULL != (ifa = BcmAdslCoreGetIntf(dev, diagDevNames[n], (ulong)pAdslDiag->srvIpAddr)))
			break;
	}

	if (NULL == ifa) {
		if(0 == pAdslDiag->gwIpAddr) {
			printk ("No interface with matching network address with the srvIpAddr. No gateway specified, Diag support is impossible\n");
			return NULL;
		}
		printk ("Searching for interface that has matching network address w/ gwIpAddr %s\n",
			ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(pAdslDiag->gwIpAddr), buf));
		for (n = 0; n < nDevNames; n++) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
			dev = __dev_get_by_name(&init_net, diagDevNames[n]);
#else
			dev = __dev_get_by_name(diagDevNames[n]);
#endif
			if (NULL == dev)
				continue;
			if(NULL != (ifa = BcmAdslCoreGetIntf(dev, diagDevNames[n], (ulong)pAdslDiag->gwIpAddr)))
				break;
		}
	}
	
	if (NULL == ifa) {
		printk ("No interface with matching network address with the gwIpAddr. Diag support is impossible\n");
		return NULL;
	}

	/* get remote MAC address for Diag srvIpAddr */
	if (NULL == pskb) {
		pskb = alloc_skb (DIAG_FRAME_HEADER_LEN + 32, GFP_ATOMIC);
		if (pskb == NULL)
			return NULL;
	}
	pskb->dev = dev;
	pskb->protocol = htons(eth_type_trans (pskb, dev));
	dd = (diagSockFrame *) pskb->head;
	dd->etype = htons(ETH_P_IP);

	memcpy(dd->srcMacAddr, dev->dev_addr, ETH_ALEN);
	memset(dd->dstMacAddr, 0, ETH_ALEN);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,31))
	skb_dst_set(pskb, (void *) &rtbl);
#else
	pskb->dst = (void *) &rtbl;
#endif



	if ((ifa->ifa_local & ifa->ifa_mask) == (pAdslDiag->srvIpAddr & ifa->ifa_mask)) {
		printk ("Diag server is on the same subnet\n");
		rtbl.rt_gateway = pAdslDiag->srvIpAddr;
		if((0 != diagConnectInfo.devName[0]) &&
			(0 != (diagConnectInfo.macAddr[0] | diagConnectInfo.macAddr[1] | diagConnectInfo.macAddr[2] | 
			diagConnectInfo.macAddr[3] | diagConnectInfo.macAddr[4] | diagConnectInfo.macAddr[5]))) {
			memcpy(dd->dstMacAddr, diagConnectInfo.macAddr, 6);
			n = 0;
		}
		else
#ifdef CONFIG_BRCM_IKOS
			n = -1;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
			n = arp_find(dd->dstMacAddr, pskb);
#else
			n = -1;
#endif
#endif
	}
	else {
		printk ("Diag server is outside subnet, using local IP address %s and gateway IP address %s\n",
			ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(ifa->ifa_local), buf),
			ConvertToDottedIpAddr(ADSL_ENDIAN_CONV_LONG(pAdslDiag->gwIpAddr), tmpBuf));
		rtbl.rt_gateway = pAdslDiag->gwIpAddr;
		if((0 != diagConnectInfo.devName[0]) &&
			(0 != (diagConnectInfo.macAddr[0] | diagConnectInfo.macAddr[1] | diagConnectInfo.macAddr[2] | 
			diagConnectInfo.macAddr[3] | diagConnectInfo.macAddr[4] | diagConnectInfo.macAddr[5]))) {
			memcpy(dd->dstMacAddr, diagConnectInfo.macAddr, 6);
			n = 0;
		}
		else
#ifdef CONFIG_BRCM_IKOS
			n = -1;
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
			n = arp_find(dd->dstMacAddr, pskb);
#else
			n = -1;
#endif
#endif
	}

	printk ("dstMACAddr = %02X:%02X:%02X:%02X:%02X:%02X\n", 
		dd->dstMacAddr[0], dd->dstMacAddr[1], dd->dstMacAddr[2],
		dd->dstMacAddr[3], dd->dstMacAddr[4], dd->dstMacAddr[5]);

	if ((n != 0) ||
		(0 == (dd->dstMacAddr[0] | dd->dstMacAddr[1] | dd->dstMacAddr[2] | dd->dstMacAddr[3] | dd->dstMacAddr[4] | dd->dstMacAddr[5]))) {
		*ppskb = pskb;
		return NULL;
	}

	dd->ipHdr.ver_hl = 0x45;
	dd->ipHdr.tos = 0;
	dd->ipHdr.len = 0;			/* changes for frames */
	dd->ipHdr.id = htons(0x2000);
	dd->ipHdr.off = 0;
	dd->ipHdr.ttl =128;
	dd->ipHdr.proto = 0x11;		/* always UDP */
	dd->ipHdr.checksum = 0;		/* changes for frames */
	dd->ipHdr.srcAddr = ifa->ifa_local;
	dd->ipHdr.dstAddr = pAdslDiag->srvIpAddr;
	dd->ipHdr.checksum = DiagIpComputeChecksum(&dd->ipHdr);
	dd->udpHdr.srcPort = htons(port);
	dd->udpHdr.dstPort = htons(dstPort);
	dd->udpHdr.len = 0;			/* changes for frames */
	dd->udpHdr.checksum = 0;	/* not used */

	/* to prevent skb from deallocation */
	pskb->data = pskb->head + DIAG_FRAME_PAD_SIZE;
	atomic_set(&pskb->users, DIAG_SKB_USERS);

	*ppskb = pskb;

	return dev;
}


void BcmAdslCoreDiagDataInit(unsigned char lineId)
{
	if (0 == diagCtrl.diagDataMap[lineId])
		return;
	

	if ((0 != (diagCtrl.diagDataMap[lineId] & DIAG_DATA_EYE)) && (NULL != diagCtrl.skbModel)) {
		int	dataAlignMask;
			dataAlignMask = 3;
		if(NULL != diagCtrl.pEyeDataAppCtrl[lineId])
			BcmCoreDiagZeroCopyStatAppUnInit(diagCtrl.pEyeDataAppCtrl[lineId]);
		diagCtrl.pEyeDataAppCtrl[lineId] = BcmCoreDiagZeroCopyStatAppInit(lineId, eyeData, diagCtrl.skbModel, 1000, 24, dataAlignMask, DIAG_FRAME_HEADER_LEN);
	}
	

	return;
}


void BcmAdslCoreDiagIsrTask(void)
{
}

#ifdef TEST_CMD_BUF_RING

LOCAL uint32 sendCmdTimerSeed = 0;
LOCAL uint32 __random32(uint32 *seed);
LOCAL uint32 __random32(uint32 *seed) /* FIXME: will get a more sophiscated algorithm later */
{
	*seed = 16807*(*seed);
	*seed += (((*seed) >> 16) & 0xffff);
	return *seed;
}

#define PROF_TIMER_SEED_INIT	(sendCmdTimerSeed = jiffies)
#define PROF_RANDOM32_GEN		__random32(&sendCmdTimerSeed)

LOCAL ulong sndCnt = 0;
LOCAL ulong failCnt = 0, prevFailSndCnt = 0, continousFailcnt=0;

LOCAL int sendCmdStarted = 0;
LOCAL struct timer_list sendCmdTimer;

LOCAL void BcmAdslCoreProfileSendCmdFn(ulong arg);

LOCAL VOID	*pData = NULL;
LOCAL ulong	gInterval = 10;

LOCAL void BcmAdslCoreTestCmdStart(ulong interval)
{
	DiagWriteString(0, DIAG_DSL_CLIENT, "*** %s: interval = %d ***\n", __FUNCTION__, (int)interval);
	printk("*** %s: interval = %d ***\n", __FUNCTION__, (int)interval);
	
	if(NULL == (pData = kmalloc(128, GFP_KERNEL))) {
		printk("*** %s: Failed kmalloc() ***\n", __FUNCTION__);
		return;
	}
	
	gInterval = interval;
	init_timer(&sendCmdTimer);
	sendCmdTimer.function = BcmAdslCoreProfileSendCmdFn;
	sendCmdTimer.data = interval;
	sendCmdTimer.expires = jiffies + 2; /* 10ms */
	PROF_TIMER_SEED_INIT;
	sendCmdStarted = 1;
	sndCnt = 0;
	failCnt = 0;
	add_timer(&sendCmdTimer);
}

void BcmAdslCoreTestCmdStop(void)
{
	DiagWriteString(0, DIAG_DSL_CLIENT, "*** %s ***\n", __FUNCTION__);
	printk("*** %s ***\n", __FUNCTION__);

	if(sendCmdStarted) {
		del_timer(&sendCmdTimer);
		sendCmdStarted = 0;
	}
	
	if(pData) {
		kfree(pData);
		pData = NULL;
	}
}

extern AC_BOOL AdslCoreCommandWrite(dslCommandStruct *cmdPtr);

LOCAL void BcmAdslCoreProfileSendCmdFn(ulong arg)
{
	ulong				msgLen;
	dslCommandStruct		cmd;
	int					len, i;	
	volatile AdslXfaceData	*pAdslX = (AdslXfaceData *) ADSL_LMEM_XFACE_DATA;
	AC_BOOL			res=TRUE;
	
	if(!sendCmdStarted)
		return;
	
	if(0 == sndCnt) {
		printk("*** %s: arg = %d pData=%x***\n", __FUNCTION__, (int)arg, (uint) pData);
		printk("***pStart=%x pRead=%x pWrite=%x pEnd=%x pStretchEnd=%x pExtraEnd=%x\n",
			(uint)pPhySbCmd->pStart, (uint)pPhySbCmd->pRead, (uint)pPhySbCmd->pWrite,
			(uint)pPhySbCmd->pEnd, (uint)pPhySbCmd->pStretchEnd, (uint)pPhySbCmd->pExtraEnd);
		DiagWriteString(0, DIAG_DSL_CLIENT, "*** %s: arg = %d pData=%x***\n", __FUNCTION__, (int)arg, (uint) pData);
		BcmAdslCoreDiagWriteStatusString(0, "***pStart=%x pRead=%x pWrite=%x pEnd=%x pStretchEnd=%x pExtraEnd=%x\n",
			(uint)pPhySbCmd->pStart, (uint)pPhySbCmd->pRead, (uint)pPhySbCmd->pWrite,
			(uint)pPhySbCmd->pEnd, (uint)pPhySbCmd->pStretchEnd, (uint)pPhySbCmd->pExtraEnd);
	}
	
	/* Send Cmd  */
	cmd.command = kDslSendEocCommand;
	cmd.param.dslClearEocMsg.msgId = 370;
	cmd.param.dslClearEocMsg.dataPtr = pData;
	
	for(i=0; i < 14; i++) {
		msgLen = PROF_RANDOM32_GEN;
		msgLen = msgLen % 113;
		if( 0 == msgLen )
			msgLen = 4;		
		len = msgLen + sizeof(cmd.param.dslClearEocMsg);

		cmd.param.dslClearEocMsg.msgType = msgLen;
		cmd.param.dslClearEocMsg.msgType |= kDslClearEocMsgDataVolatile;
		
		if(0 == (sndCnt%2000))
		{
//			printk("*** Sending cmdNum %d, len %d\n", (int)sndCnt, len);
			BcmAdslCoreDiagWriteStatusString(0, "*** Sending cmdNum %d, len %d\n", (int)sndCnt, len);
		}
		
		res = AdslCoreCommandWrite(&cmd);
		
		sndCnt++;
		
		if(FALSE == res) {
			if((prevFailSndCnt + 1) == sndCnt) {
				if(++continousFailcnt > 40 ) {
					printk("***FAILED!!!***\n\n");
					printk("***pStart=%x pRead=%x pWrite=%x pEnd=%x pStretchEnd=%x pExtraEnd=%x\n",
						(uint)pPhySbCmd->pStart, (uint)pPhySbCmd->pRead, (uint)pPhySbCmd->pWrite,
						(uint)pPhySbCmd->pEnd, (uint)pPhySbCmd->pStretchEnd, (uint)pPhySbCmd->pExtraEnd);
					BcmAdslCoreDiagWriteStatusString(0, "***FAILED!!!***\n\n");
					BcmAdslCoreDiagWriteStatusString(0, "***pStart=%x pRead=%x pWrite=%x pEnd=%x pStretchEnd=%x pExtraEnd=%x\n",
						(uint)pPhySbCmd->pStart, (uint)pPhySbCmd->pRead, (uint)pPhySbCmd->pWrite,
						(uint)pPhySbCmd->pEnd, (uint)pPhySbCmd->pStretchEnd, (uint)pPhySbCmd->pExtraEnd);
					sendCmdStarted = 0;
					BcmAdslCoreTestCmdStop();
					return;
				}
			}
			prevFailSndCnt = sndCnt;
			if(0 == (failCnt%2000))
			{
				uint	rdPtr, wrPtr, sePtr;
				rdPtr = (uint)pPhySbCmd->pRead;
				wrPtr = (uint)pPhySbCmd->pWrite;
				sePtr =(uint)pPhySbCmd->pStretchEnd;
				BcmAdslCoreDiagWriteStatusString(0, "***Write len=%d failed(%d/%d): pRead=%x pWrite=%x pStretchEnd=%x\n",
					len, (int)failCnt+1, (int)sndCnt,
					rdPtr, wrPtr, sePtr);
			}
			failCnt++;
		}
		else {
			prevFailSndCnt = 0;
			continousFailcnt = 0;
		}
	}
			
	/* Re-schedule the timer */
	sendCmdTimer.expires = jiffies + gInterval;
	add_timer(&sendCmdTimer);

}
#endif /* TEST_CMD_BUF_RING */


/**************************************************************************
** Function Name: BcmAdslDiagTaskInit
** Description  : This function intializes ADSL driver Diag task
** Returns      : None.
**************************************************************************/
int BcmAdslDiagTaskInit(void)
{
	return 0;
}

/**************************************************************************
** Function Name: BcmAdslDiagTaskUninit
** Description  : This function unintializes ADSL driver Diag task
** Returns      : None.
**************************************************************************/
void BcmAdslDiagTaskUninit(void)
{
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
extern int BcmXdslDiagStatSaveLocalRead(void *pBuf, int bufLen);

static ssize_t BcmXdslCoreDiagStat_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int nByteRead = BcmXdslDiagStatSaveLocalRead((void*)buf, count);

	if(0 == nByteRead) {
		if (file->f_flags & O_NONBLOCK) {
#if defined(SAVE_STAT_LOCAL_DBG)
			printk("%s: No data available\n", __FUNCTION__);
#endif
			return -EAGAIN;
		}
	}
#if defined(SAVE_STAT_LOCAL_DBG)
	printk("%s: Reading %d (asked = %d) bytes\n", __FUNCTION__, nByteRead, count);
#endif
	return nByteRead;
}

static int BcmXdslCoreDiagStat_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int BcmXdslCoreDiagStat_release(struct inode * inode, struct file * file)
{
	return 0;
}

#define XDSL_PROC_FILE_NAME	"xdsldiaglogs"
static struct proc_dir_entry * BcmXdslCoreDiagStat_procEntry;

static const struct file_operations BcmXdslCoreDiagStat_fOps = {
	.read    = BcmXdslCoreDiagStat_read,
	.open    = BcmXdslCoreDiagStat_open,
	.release = BcmXdslCoreDiagStat_release,
};
#endif

int BcmXdslCoreDiagProcFileCreate(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
	if (BcmXdslCoreDiagStat_procEntry == NULL)
		BcmXdslCoreDiagStat_procEntry = proc_create(XDSL_PROC_FILE_NAME, S_IRUSR, NULL, &BcmXdslCoreDiagStat_fOps);

	return (BcmXdslCoreDiagStat_procEntry == NULL ? -ENOMEM : 0);
#else
	return 0;
#endif
}

void BcmXdslCoreDiagProcFileRemove(void)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
	remove_proc_entry(XDSL_PROC_FILE_NAME, NULL);
	BcmXdslCoreDiagStat_procEntry = NULL;
#endif
}
