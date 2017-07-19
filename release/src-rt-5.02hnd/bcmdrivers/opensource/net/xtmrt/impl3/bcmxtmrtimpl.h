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
 * File Name  : bcmxtmrtimpl.h
 *
 * Description: This file contains constant definitions and structure
 *              definitions for the BCM6368 ATM/PTM network device driver.
 ***************************************************************************/

#if !defined(_BCMXTMRTIMPL_H)
#define _BCMXTMRTIMPL_H

#include "bcmPktDma_structs.h"

#define XTM_CACHE_SMARTFLUSH

#define TEQ_DATA_VCID    15

#define CHIP_REV_BCM6368B0          0x636800b0
#define MAX_DEV_CTXS                16
#define MAX_MATCH_IDS               128
#define MAX_DEFAULT_MATCH_IDS       16
#define ENET_8021Q_SIZE             4
#define PTM_MAX_TX_FRAME_LEN        1984  /* Per chip limitation, 6328/6362/6368 SAR can only
                                           * transmit max PTM frame size of
                                           * 1984 + 4(FCS) = 1988
                                           */
#if defined(CONFIG_BCM_JUMBO_FRAME)
#define PTM_MAX_MTU_PAYLOAD_SIZE    (PTM_MAX_TX_FRAME_LEN - 14 - ENET_8021Q_SIZE)
#else
#define PTM_MAX_MTU_PAYLOAD_SIZE    XTM_MAX_MTU_PAYLOAD_SIZE
#endif

#if defined(CONFIG_BCM_USER_DEFINED_DEFAULT_MTU)
    #if (CONFIG_BCM_USER_DEFINED_DEFAULT_MTU_SIZE > PTM_MAX_MTU_PAYLOAD_SIZE)
    #error "ERROR - CONFIG_BCM_USER_DEFINED_DEFAULT_MTU_SIZE > PTM_MAX_MTU_PAYLOAD_SIZE"
    #endif

    #define BCM_PTM_DEFAULT_MTU_SIZE  CONFIG_BCM_USER_DEFINED_DEFAULT_MTU_SIZE

#else /* !CONFIG_BCM_USER_DEFINED_DEFAULT_MTU */

    #define BCM_PTM_DEFAULT_MTU_SIZE  PTM_MAX_MTU_PAYLOAD_SIZE

#endif

#define SAR_DMA_MAX_BURST_LENGTH    8

#define MAX_BUFMEM_BLOCKS           64
#define RFC1626_MTU                 9180

#define SAR_RX_INT_ID_BASE          INTERRUPT_ID_ATM_DMA_0
#define SAR_TX_INT_ID_BASE          INTERRUPT_ID_ATM_DMA_4
#define SAR_RX_DMA_BASE_CHAN        0
#define NR_SAR_RX_DMA_CHANS         2
#define SAR_TX_DMA_BASE_CHAN        4
#define NR_SAR_TX_DMA_CHANS         16
#define SAR_TIMEOUT                 (HZ/20)
#define INVALID_VCID                0xff
#ifndef CARDNAME
#define CARDNAME                    "bcmxtmrt"
#endif

#define CACHE_TO_NONCACHE(x)        KSEG1ADDR(x)
#define NONCACHE_TO_CACHE(x)        KSEG0ADDR(x)

#define XTMRT_UNINITIALIZED         0
#define XTMRT_INITIALIZED           1
#define XTMRT_RUNNING               2

#define SKB_PROTO_ATM_CELL          0xf000
#define XTM_POLL_DONE               0x80000000

/* Circuit types. */
#define XCT_TRANSPARENT             0x00000001
#define XCT_AAL0_PKT                0x00000002
#define XCT_AAL0_CELL               0x00000003
#define XCT_OAM_F5_SEG              0x00000004
#define XCT_OAM_F5_E2E              0x00000005
#define XCT_RM                      0x00000006
#define XCT_AAL5                    0x00000007
#define XCT_ASM_P0                  0x00000008
#define XCT_ASM_P1                  0x00000009
#define XCT_ASM_P2                  0x0000000a
#define XCT_ASM_P3                  0x0000000b
#define XCT_OAM_F4_SEG              0x0000000c
#define XCT_OAM_F4_E2E              0x0000000d
#define XCT_TEQ                     0x0000000e
#define XCT_PTM                     0x0000000f

/* Transmit Buffer Descriptor frame status word for ATM/PTM. */
#define FSTAT_MASK                  0x00000fff
#define FSTAT_ATM_VCID_MASK         0x0000000f
#define FSTAT_ATM_VCID_SHIFT        0
#define FSTAT_PTM_CRC               0x00000001
#define FSTAT_PTM_ENET_FCS          0x00000002
#define FSTAT_CT_MASK               0x000000f0
#define FSTAT_CT_SHIFT              4
#define FSTAT_CT_TRANSPARENT        (XCT_TRANSPARENT << FSTAT_CT_SHIFT)
#define FSTAT_CT_AAL0_PKT           (XCT_AAL0_PKT    << FSTAT_CT_SHIFT)
#define FSTAT_CT_AAL0_CELL          (XCT_AAL0_CELL   << FSTAT_CT_SHIFT)
#define FSTAT_CT_OAM_F5_SEG         (XCT_OAM_F5_SEG  << FSTAT_CT_SHIFT)
#define FSTAT_CT_OAM_F5_E2E         (XCT_OAM_F5_E2E  << FSTAT_CT_SHIFT)
#define FSTAT_CT_RM                 (XCT_RM          << FSTAT_CT_SHIFT)
#define FSTAT_CT_AAL5               (XCT_AAL5        << FSTAT_CT_SHIFT)
#define FSTAT_CT_ASM_P0             (XCT_ASM_P0      << FSTAT_CT_SHIFT)
#define FSTAT_CT_ASM_P1             (XCT_ASM_P1      << FSTAT_CT_SHIFT)
#define FSTAT_CT_ASM_P2             (XCT_ASM_P2      << FSTAT_CT_SHIFT)
#define FSTAT_CT_ASM_P3             (XCT_ASM_P3      << FSTAT_CT_SHIFT)
#define FSTAT_CT_OAM_F4_SEG         (XCT_OAM_F4_SEG  << FSTAT_CT_SHIFT)
#define FSTAT_CT_OAM_F4_E2E         (XCT_OAM_F4_E2E  << FSTAT_CT_SHIFT)
#define FSTAT_CT_TEQ                (XCT_TEQ         << FSTAT_CT_SHIFT)
#define FSTAT_CT_PTM                (XCT_PTM         << FSTAT_CT_SHIFT)
#define FSTAT_COMMON_INS_HDR_EN     0x00000100
#define FSTAT_COMMON_HDR_INDEX_MASK 0x00000600
#define FSTAT_COMMON_HDR_INDEX_SHIFT 9
#define FSTAT_INDEX_CI              0x00000100
#define FSTAT_INDEX_CLP             0x00000200
#define FSTAT_INDEX_USE_ALT_GFC     0x00000400
#define FSTAT_MODE_INDEX            0x00000000
#define FSTAT_MODE_COMMON           0x00000800

/* Receive Buffer Descriptor frame status word for ATM/PTM. */
#define FSTAT_MATCH_ID_MASK         0x0000007f             
#define FSTAT_MATCH_ID_SHIFT        0
#define FSTAT_PACKET_CELL_MASK      0x00000400
#define FSTAT_PACKET                0x00000000
#define FSTAT_CELL                  0x00000400
#define FSTAT_ERROR                 0x00000800

/* First byte of a received cell. */
#define CHDR_CT_MASK                0x0f
#define CHDR_CT_SHIFT               0
#define CHDR_CT_OAM_F5_SEG          (XCT_OAM_F5_SEG  << CHDR_CT_SHIFT)
#define CHDR_CT_OAM_F5_E2E          (XCT_OAM_F5_E2E  << CHDR_CT_SHIFT)
#define CHDR_CT_RM                  (XCT_RM          << CHDR_CT_SHIFT)
#define CHDR_CT_ASM_P0              (XCT_ASM_P0      << CHDR_CT_SHIFT)
#define CHDR_CT_ASM_P1              (XCT_ASM_P1      << CHDR_CT_SHIFT)
#define CHDR_CT_ASM_P2              (XCT_ASM_P2      << CHDR_CT_SHIFT)
#define CHDR_CT_ASM_P3              (XCT_ASM_P3      << CHDR_CT_SHIFT)
#define CHDR_CT_OAM_F4_SEG          (XCT_OAM_F4_SEG  << CHDR_CT_SHIFT)
#define CHDR_CT_OAM_F4_E2E          (XCT_OAM_F4_E2E  << CHDR_CT_SHIFT)
#define CHDR_PORT_MASK              0x60
#define CHDR_PORT_SHIFT             5
#define CHDR_ERROR                  0x80
#define CHDR_ERROR_MISC             0x01
#define CHDR_ERROR_CRC              0x02
#define CHDR_ERROR_CAM              0x04
#define CHDR_ERROR_HEC              0x08
#define CHDR_ERROR_PORT             0x10

/****************************************************************************
   Logging Defines
****************************************************************************/

/* #define BCM_XTM_LOG      
   #define BCM_XTM_RX_LOG  
   #define BCM_XTM_TX_LOG   
   #define BCM_XTM_LINK_LOG */

#if defined(BCM_XTM_LOG)
#define BCM_XTM_DEBUG(fmt, arg...)  BCM_LOG_DEBUG(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_INFO(fmt, arg...)   BCM_LOG_INFO(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_NOTICE(fmt, arg...) BCM_LOG_NOTICE(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_ERROR(fmt, arg...)  BCM_LOG_ERROR(BCM_LOG_ID_XTM, fmt, ##arg)
#else
#define BCM_XTM_DEBUG(fmt, arg...)
#define BCM_XTM_INFO(fmt, arg...)
#define BCM_XTM_NOTICE(fmt, arg...)
#define BCM_XTM_ERROR(fmt, arg...)
#endif

#if defined(BCM_XTM_RX_LOG)
#define BCM_XTM_RX_DEBUG(fmt, arg...)  BCM_LOG_DEBUG(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_RX_INFO(fmt, arg...)   BCM_LOG_INFO(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_RX_NOTICE(fmt, arg...) BCM_LOG_NOTICE(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_RX_ERROR(fmt, arg...)  BCM_LOG_ERROR(BCM_LOG_ID_XTM, fmt, ##arg)
#else
#define BCM_XTM_RX_DEBUG(fmt, arg...)
#define BCM_XTM_RX_INFO(fmt, arg...)
#define BCM_XTM_RX_NOTICE(fmt, arg...)
#define BCM_XTM_RX_ERROR(fmt, arg...)
#endif

#if defined(BCM_XTM_TX_LOG)
#define BCM_XTM_TX_DEBUG(fmt, arg...)  BCM_LOG_DEBUG(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_TX_INFO(fmt, arg...)   BCM_LOG_INFO(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_TX_NOTICE(fmt, arg...) BCM_LOG_NOTICE(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_TX_ERROR(fmt, arg...)  BCM_LOG_ERROR(BCM_LOG_ID_XTM, fmt, ##arg)
#else
#define BCM_XTM_TX_DEBUG(fmt, arg...)
#define BCM_XTM_TX_INFO(fmt, arg...)
#define BCM_XTM_TX_NOTICE(fmt, arg...)
#define BCM_XTM_TX_ERROR(fmt, arg...)
#endif

#if defined(BCM_XTM_LINK_LOG)
#define BCM_XTM_LINK_DEBUG(fmt, arg...)  BCM_LOG_DEBUG(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_LINK_INFO(fmt, arg...) BCM_LOG_INFO(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_LINK_NOTICE(fmt, arg...) BCM_LOG_NOTICE(BCM_LOG_ID_XTM, fmt, ##arg)
#define BCM_XTM_LINK_ERROR(fmt, arg...)  BCM_LOG_ERROR(BCM_LOG_ID_XTM, fmt, ##arg)
#else
#define BCM_XTM_LINK_DEBUG(fmt, arg...)
#define BCM_XTM_LINK_INFO(fmt, arg...)
#define BCM_XTM_LINK_NOTICE(fmt, arg...)
#define BCM_XTM_LINK_ERROR(fmt, arg...)
#endif

/* Information about a DMA transmit channel. A device instance may use more
 * than one transmit DMA channel. A DMA channel corresponds to a transmit queue.
 */
 
struct bcmxtmrt_dev_context;   /* forward reference */

#define BcmPktDma_XtmRxDma BcmPktDma_LocalXtmRxDma
#define RXBDINFO BcmPktDma_XtmRxDma
#define PRXBDINFO BcmPktDma_XtmRxDma *

typedef struct BcmXtm_RxDma {

    BcmPktDma_XtmRxDma pktDmaRxInfo;

    struct sk_buff  *freeSkbList;
    unsigned char   *buf_pool[MAX_BUFMEM_BLOCKS];
    /* SKB Pool now dynamically allocated - Apr 2010 */
    unsigned char    *skbs_p;
    unsigned char    *end_skbs_p;
    int              rxIrq; 
    int              channel; 
} BcmXtm_RxDma;
              
#define BcmPktDma_XtmTxDma BcmPktDma_LocalXtmTxDma
#define TXQINFO BcmPktDma_XtmTxDma
#define PTXQINFO BcmPktDma_XtmTxDma *

/* Struct added for xtmrt dmaStatus field generation for xtm flows - Apr 2010 */
typedef struct dev_params
{
    UINT32 ulFlags;
    UINT32 ulHdrType;
    UINT8  ucTxVcid;
} DEV_PARAMS, *PDEV_PARAMS;

#define PTM_FLOW_PRI_LOW     0
#define PTM_FLOW_PRI_HIGH    1

#define XTM_DEF_REDUCED_MISSING_DS_MAX_DEVIATION 16

#ifndef FAP_4KE

/* The definition of the driver control structure */
typedef struct bcmxtmrt_dev_context
{
    /* Linux structures. */
    struct net_device *pDev;        
    struct net_device_stats DevStats;
    IOCTL_MIB_INFO MibInfo;
    struct ppp_channel Chan;

    /* ATM/PTM fields. */
    XTM_ADDR Addr;
    UINT32 ulLinkState;
    UINT32 ulLinkUsRate[MAX_BOND_PORTS] ;
    UINT32 ulLinkDsRate ;
    UINT32 ulDsSeqDeviation ;
    UINT32 ulDsOrigSeqDeviation ;
    UINT32 ulTrafficType ;
    UINT32 ulPortDataMask ;
    UINT32 ulOpenState;
    UINT32 ulAdminStatus;
    UINT32 ulHdrType;
    UINT32 ulEncapType; /* IPoA, PPPoA, or EoA[bridge,MER,PPPoE] */
    UINT32 ulFlags;

    /* Transmit fields. */
    UINT8 ucTxVcid;
    UINT32 ulTxQInfosSize;
    BcmPktDma_XtmTxDma *txdma[MAX_TRANSMIT_QUEUES];
    BcmPktDma_XtmTxDma *pTxQids[MAX_TRANSMIT_QUEUES];
    BcmPktDma_XtmTxDma *pHighestPrio;
    BcmPktDma_XtmTxDma *pTxPriorities[MAX_PTM_PRIORITIES][MAX_PHY_PORTS][MAX_SUB_PRIORITIES];

    /* DmaKeys, DmaSources, DmaAddresses now allocated with txBds - Apr 2010 */

#if defined(CONFIG_BCM_FAP) || defined(CONFIG_BCM_FAP_MODULE)
    /* Device params to be passed to the FAP on tx enable - Apr 2010 */
    DEV_PARAMS devParams;
#endif

    /*Port Mirroring fields*/
    char szMirrorIntfIn[MIRROR_INTF_SIZE];
    char szMirrorIntfOut[MIRROR_INTF_SIZE];
} BCMXTMRT_DEV_CONTEXT, *PBCMXTMRT_DEV_CONTEXT;
#endif /* FAP_4KE */

/* ATM Bonding Definitions */

#define XTMRT_ATM_BOND_ASM_VPI             0x0
#define XTMRT_ATM_BOND_ASM_VCI             0x14

/* PTM Bonding Common Definitions */

#define DS_PTMBOND_CHANNEL                   0  /* Due to SAR PTM issue, we will have one
                                                 * channel in operation in DS path.
                                                 */

#define ETH_FCS_LEN                          4
#define XTMRT_PTM_BOND_FRAG_HDR_SIZE         2
#define XTMRT_PTM_BOND_FRAG_HDR_EOP          1
#define XTMRT_PTM_BOND_FRAG_HDR_SOP          2
#define XTMRT_PTM_BOND_FRAG_HDR_EOP_DROP     4

typedef union _XtmRtPtmBondFragHdr {
   struct _s1Val {
   UINT16 seqNo      : 14 ;
   UINT16 rxFlags    :  2 ;
   } s1Val ;
   UINT16 uVal ;
} XtmRtPtmBondFragHdr ;

#define XTMRT_PTM_CRC_SIZE                   2

/* PTM Tx Bonding Definitions */

#define XTMRT_PTM_BOND_MAX_FRAG_PER_PKT      8
#define XTMRT_PTM_BOND_TX_MAX_FRAGMENT_SIZE  508
#define XTMRT_PTM_BOND_TX_MIN_FRAGMENT_SIZE  64
#define XTMRT_PTM_BOND_HDR_NON_EOP           0

typedef union _XtmRtPtmTxBondHeader {
   struct _sVal {
      UINT16 portSel    : 1 ;
   UINT16 Reserved   : 2 ;
   UINT16 PktEop     : 1 ;
      UINT16 FragSize   : 12 ;  /* Includes size of the fragment + 2bytes of frag hdr + 2bytes of CRC-16 */
   } sVal ;
   UINT16  usVal ;
} XtmRtPtmTxBondHeader ;

/* PTM Rx Bonding Definitions */

/* tune both the following timeout factors together on receive queue */
#define RX_SEQ_TIMEOUT        (msecs_to_jiffies(50))
#define RX_MISSING_TIMEOUT    (msecs_to_jiffies(60000))
#define RX_THROTTLE_TIMEOUT   (msecs_to_jiffies(1000))
#define MAX_TICK_COUNT        0

/* Commands for the resync method */
#define RESYNC_TIMEOUT        0x01
#define RESYNC_OVERFLOW       0x02
#define RESYNC_PORT           0x04
#define RESYNC_FLUSH          0x08
#define RESYNC_MISSING        0x10

#define RESYNC_LIMIT          5
#define DROP_WEIGHT           5
#define SEND_WEIGHT           1

#define Q_INDEX(i)            ((i)&XTMRT_PTM_BOND_RX_QUEUE_LEN_MASK)

                              /* Change all if required */
#define MAX_SEQ_NR            (0x3fff)

/* Split horizon Delta between two sequence numbers */
/* result is > 0 if i > j, < 0 otherwise */
/* if i<j:
 *  i-j > -R/2 => D = i-j
 *  i-j < -R/2 => D = i-j+R+1
 * if i>= j:
 *  i-j < R/2 => D = i-j
 *  i-j > R/2 => D = i-j-R-1
 */
#if 0
#define DELTA_SEQ_NR(j, i)		\
	((i)<(j)) ? (((i)-(j)>-MAX_SEQ_NR/2)? ((i)-(j)) : ((i)-(j)+MAX_SEQ_NR+1)) : (((i)-(j)>(MAX_SEQ_NR+1)/2)? ((i)-(j)-MAX_SEQ_NR-1) : ((i)-(j)))
#else
#define DELTA_SEQ_NR(j, i)		\
	((i)<(j)) ? (((i)-(j)>-(MAX_SEQ_NR>>1))? ((i)-(j)) : ((i)-(j)+MAX_SEQ_NR+1)) : (((i)-(j)>((MAX_SEQ_NR+1)>>1))? ((i)-(j)-MAX_SEQ_NR-1) : ((i)-(j)))
#endif

#define SEQ_NR(fragHdr)    (fragHdr.s1Val.seqNo)
#define RX_FLAGS(fragHdr)  (fragHdr.s1Val.rxFlags)
#define ABSOLUTE(i)         (((i)>=0)?(i):-(i))
#define MAX_SEQ_NR_DEVIATION (XTMRT_PTM_BOND_RX_QUEUE_LEN/2)
#define NEXT_SEQ_NR(i) (((i)+1)&MAX_SEQ_NR)
#define INC_SEQ_NR(i,seq) (((i)+seq)&MAX_SEQ_NR)

                                            /* Change both the following
                                             * together, if..reqd. *
                                             * For 2ms delay differential.
                                             * @ rates 50 ms DS
                                             */
#define XTMRT_PTM_BOND_RX_QUEUE_LEN          512
#define XTMRT_PTM_BOND_RX_QUEUE_LEN_MASK     (XTMRT_PTM_BOND_RX_QUEUE_LEN-1)

#ifndef FAP_4KE
typedef struct _XtmRtPtmBondRxQInfo {
   FkBuff_t               *rxFkb ;
   XtmRtPtmBondFragHdr    fragHdr ;
   UINT16                 rxdmaIndex ;
   PBCMXTMRT_DEV_CONTEXT  pDevCtx ;
   UINT16                 rxFkbStatus ;
   UINT8                  phyPort ;
   UINT8                  valid ;
} XtmRtPtmBondRxQInfo ;

#define PACKET_BLOG           0
#define PACKET_NORMAL         1
#define PACKET_DROP           2

#define MAX_WT_PORT_DIST      100

#define MAX_NON_ACCEL_PKTS_IN_SEC  8000

typedef struct _XtmRtPtmBondInfo {
   UINT32               totalWtPortDist ;
   UINT32               ulCurrWtPortDistStartIndex ;
   UINT32               ulCurrWtPortDistRunIndex ;
   UINT32               ulCurrWtTotalIterationsBeforeReset ;
   UINT8                u8ConfWtPortDist [MAX_WT_PORT_DIST] ;
   UINT32               ulLinkUsWt [MAX_BOND_PORTS] ;
   UINT32               ulConfLinkUsWt [MAX_BOND_PORTS] ;
   UINT32               rxDsReducedDeviation ;
	XtmRtPtmBondRxQInfo  rxq [XTMRT_PTM_BOND_RX_QUEUE_LEN] ;   /* reordering fkb buff queue */
#define PTMBOND_FWD_BUF_INDEX             0
#define PTMBOND_SCRATCHPAD_BUF_INDEX      1
#define PTMBOND_SCRATCHPAD_BUF_INDEX2     2
	XtmRtPtmBondRxQInfo  fwd [3];          /* fwd[1], current forward buffer,
                                             fwd[0], scratchpad, used for processing */
	int                  expectedSeqNum;	/* expected rx seq number */
	int                  lastSeqNum [MAX_BOND_PORTS];   /* last rx seq number per port */
	int                  lastRxFlags;		/* last rx flags */
	int                  dropMonitor ;		/* successive rx errors force queue resync */
	int                  rxFragQueued;     /* rx number of fragments that are in the queue */
	unsigned long        rxLastFrag;		   /* time last fragment was received */
	unsigned long        rxMissingFrag;		   /* time last fragment was received */
   unsigned long        tickCount ;

	/* (Error) counters */
   struct _bStats {
	UINT32               rfw;				   /* rx forwarded fragments */
	UINT32               rxo;				   /* rx forwarded octets counter */
	UINT32               rxp;				   /* rx forwarded packets counter */
        UINT32               rxpl;				   /* rx forwarded packets non-accel counter */
        UINT32               rxd;				   /* rx dropped packets non-accel counter */
	UINT32               old;              /* rx old sequence number received (past) */
	UINT32               dup;              /* rx duplicated sequence number received */
	UINT32               oos;              /* rx out-of-sync fragments dropped */
	UINT32               usq;              /* rx unexpected sequence number from queue store */
	UINT32               eop;              /* rx unexpected in-sequence mid- or end-of-packet */
	UINT32               sop;              /* rx unexpected in-sequence start-of-packet */
	UINT32               ess;              /* rx expected sequence number skip */
	UINT32               end;              /* rx artificially ended packets */
      UINT32               stp;              /* rx dropped due to sync at the startup */
	UINT32               bqo;              /* rx dropped due to flush after buffer queue overrun. */
	UINT32               mfd;              /* rx dropped due to missing fragment(s) */
	UINT32               flu;              /* rx dropped due to queue flush */
	UINT32               tim;              /* rx dropped due to expected sequence number timeout */
	UINT32               dro;              /* rx dropped due to line removal */
	UINT32               inl;              /* rx fragment received from inactive line */
	UINT32               cpe;              /* times of buffer expand and copy */

	/* Per line counters */
	UINT32               rfg [MAX_BOND_PORTS];	/* rx fragments counter */
   } bStats ;
	
#ifdef PTM_BONDING_DEBUG
   int                  logLevel ;
#define MAX_SEQ_DUMP 750
	int                  seqNum;
	UINT32               seqLen[MAX_SEQ_DUMP];
	XtmRtPtmBondFragHdr  seqFlags[MAX_SEQ_DUMP];
	UINT32               port[MAX_SEQ_DUMP];
#endif
} XtmRtPtmBondInfo ;

#define XTM_SW_SCHED_TIMER_INIT            1
#define XTM_SW_SCHED_TIMER_REINIT          2

#define XTM_HW_TIMER_USEC 50

#define MAX_HW_TRANSMIT_QUEUE_SIZE         4
#define MAX_HW_TRANSMIT_QUEUES             2 /* Per latency/priority, as in BCM6368 data sheet */
                                             /* Only one latency supported due HW deficiency */
#define XTM_SW_SCHED_KEY(channel,index)   ((channel<<16)|index)
#define XTM_SW_CHANNEL(key)               (key>>16)
#define XTM_SW_CHANNEL_INDEX(key)         (key&0xFFFF)

/* Below ordering is important */
#define XTM_SW_DMA_NO_COMMAND              0x0
#define XTM_SW_DMA_FORCE_DELETE            0x1
#define XTM_SW_DMA_DELETE                  0x2
#define XTM_SW_DMA_FORCE_FLUSH             0x3
#define XTM_SW_DMA_FLUSH                   0x4
#define XTM_SW_DMA_FORCED                  0x1

#define MIN_WFQ_ALLOWANCE                  1
typedef struct _swSchedWfqInfo {
   UINT32      prevMaxSC ;
   UINT32      currMaxSC ;
   UINT32      SC [MAX_SUB_PRIORITIES] ;
   UINT32      allowanceQuantity [MAX_SUB_PRIORITIES] ;
   UINT32      sentQuantity [MAX_SUB_PRIORITIES] ;
} SwSchedWfqInfo ;

typedef struct _XtmRtTxSchedContext
{
   UINT32     totalScheduled ;
   UINT16     usHwTxQInfosSize ;
   UINT16     resv ;
   BcmPktDma_XtmTxDma txHwdma[MAX_HW_TRANSMIT_QUEUES] ;
   UINT32     ulSwTxQInfosSize ;
   union {
      volatile BcmPktDma_XtmTxDma *pTxPrioSwDmaL[MAX_PTM_PRIORITIES*MAX_SUB_PRIORITIES] ;  /* Linear */
      volatile BcmPktDma_XtmTxDma *pTxPrioSwDmaD[MAX_PTM_PRIORITIES][MAX_SUB_PRIORITIES] ; /* Dimension */
   } uSwDma ;
   volatile UINT8      ulSubPrioBitMap [MAX_PTM_PRIORITIES] ;
                                                     /* Priority Sorted Array */
   UINT32             rrVisitCount [MAX_PTM_PRIORITIES] ;
   /* WFQ specific information */
   SwSchedWfqInfo     wfq [MAX_PTM_PRIORITIES] ;

   UINT32      ulCommand ;
   UINT32      ulCommandParam ;  /* Currently only for deletion of the tx queue. Write sets this and
                                  * Reader resets this.
                                  */
} XtmRtTxSchedContext ;

/* Information that is global to all network device instances. */
#define SW_SCHED
#ifdef SW_SCHED
#define TXDMACTRL(pDevCtx)       ((pDevCtx->ulTrafficType!=TRAFFIC_TYPE_PTM_BONDED)?\
                                  g_GlobalInfo.dmaCtrl:g_GlobalInfo.swDmaCtrl)
#define TXDMATYPE(pDevCtx)       ((pDevCtx->ulTrafficType!=TRAFFIC_TYPE_PTM_BONDED)?\
                                  XTM_HW_DMA:XTM_SW_DMA)
#else
#define TXDMACTRL(pDevCtx)       g_GlobalInfo.dmaCtrl
#define TXDMATYPE(pDevCtx)       XTM_HW_DMA
#endif

typedef struct bcmxtmrt_global_info
{
    /* Linux structures. */
    PBCMXTMRT_DEV_CONTEXT pDevCtxs[MAX_DEV_CTXS];
    PBCMXTMRT_DEV_CONTEXT pDevCtxsByMatchId[MAX_MATCH_IDS];
    struct timer_list Timer;
    struct atm_dev *pAtmDev;
    struct net_device *pTeqNetDev;  
    UINT32 ulDevCtxMask;

    spinlock_t xtmlock_tx;
    spinlock_t xtmlock_rx;
    spinlock_t xtmlock_rx_regs;
    spinlock_t xtmlock_rx_bond;

    /* DMA, BD and buffer fields. */
    BcmXtm_RxDma       *rxdma[MAX_RECEIVE_QUEUES];
    volatile DmaRegs   *dmaCtrl;
    volatile DmaRegs   *swDmaCtrlBase ;
    volatile DmaRegs   *swDmaCtrl ;
    
    UINT32 ulIntEnableMask;

    /* Global transmit queue fields. */
    UINT32 ulNumExtBufs;
    UINT32 ulNumExtBufsRsrvd;
    UINT32 ulNumExtBufs90Pct;
    UINT32 ulNumExtBufs50Pct;
    UINT32 ulNumTxQs;
    UINT32 ulNumTxBufsQdAll;
    UINT32 ulDbgQ1;
    UINT32 ulDbgQ2;
    UINT32 ulDbgQ3;
    UINT32 ulDbgD1;
    UINT32 ulDbgD2;
    UINT32 ulDbgD3;

    /* Callback functions. */
    XTMRT_CALLBACK pfnOamHandler;
    void *pOamContext;
    XTMRT_CALLBACK pfnAsmHandler;
    void *pAsmContext;

    /* MIB counter registers. */
    UINT32 ulMibRxClrOnRead;
    volatile UINT32 *pulMibTxOctetCountBase;
    volatile UINT32 *pulMibRxCtrl;
    volatile UINT32 *pulMibRxMatch;
    volatile UINT32 *pulMibRxOctetCount;
    volatile UINT32 *pulMibRxPacketCount;
    volatile UINT32 *pulRxCamBase;
    UINT32 noOfNonAccelPktsDSInSec ; /* For throttling DS Non-FC packets */
    UINT32 prevTime ;

    /* SW Scheduler (Used in BCM6368) */
    XtmRtTxSchedContext    txSchedCtxt ;

    /* Bonding information */
    XtmBondConfig          bondConfig ;
    XtmRtPtmTxBondHeader   ptmBondHdr [XTMRT_PTM_BOND_MAX_FRAG_PER_PKT] ;
    XtmRtPtmBondInfo       ptmBondInfo ;

    /* Everything else. */
    UINT32 ulChipRev;
    UINT32 ulDrvState;
    /* Temporary storage for stats collection */
    struct net_device_stats dummy_stats;
    struct kmem_cache *xtmSkbCache;

    /* RX thread support */
    struct task_struct *rx_thread;
    wait_queue_head_t   rx_thread_wqh;
    int                 rx_work_avail;
#define XTMRT_BUDGET   32

} BCMXTMRT_GLOBAL_INFO, *PBCMXTMRT_GLOBAL_INFO;

extern BCMXTMRT_GLOBAL_INFO g_GlobalInfo;

#define PTM_BOND_INITIALIZE_LOCAL      0
#define PTM_BOND_INITIALIZE_GLOBAL     1
/* Function Prototypes */
void AssignRxBuffer(int channel, UINT8 *pucData);
void FlushAssignRxBuffer(int channel, UINT8 *pucData, UINT8 *pucEnd);
void bcmxtmrt_ptmbond_initialize (int) ;
int bcmxtmrt_ptmbond_calculate_link_weights (PBCMXTMRT_DEV_CONTEXT pDevCtx) ;
UINT32 bcmxtmrt_process_rx_pkt( PBCMXTMRT_DEV_CONTEXT pDevCtx, BcmXtm_RxDma *rxdma,
                              FkBuff_t *pFkb, UINT16 bufStatus, int delLen, int trailerDelLen, 
                              BOOLEAN throttle);
int bcmxtmrt_ptmbond_add_hdr (PBCMXTMRT_DEV_CONTEXT pDevCtx, UINT32 ulPtmPrioIdx, pNBuff_t *ppNBuff,
                               struct sk_buff **ppNBuffSkb, UINT8 **ppData, 
                               int *pLen) ;
void bcmxtmrt_ptmbond_receive_rx_fragment (PBCMXTMRT_DEV_CONTEXT pDevCtx, FkBuff_t *pFkb,
                                           UINT16 bufStatus, UINT16 rxdmaIndex) ;
void bcmxtmrt_ptm_receive_and_drop (PBCMXTMRT_DEV_CONTEXT pDevCtx, FkBuff_t *fkb,
                                    UINT16 bufStatus, UINT16 rxdmaIndex) ;
void bcmxtmrt_ptmbond_handle_port_status_change (PBCMXTMRT_DEV_CONTEXT pDevCtx, XtmRtPtmBondInfo *pBondInfo,
                                                 UINT32 ulLinkState, UINT32 dmaSize) ;
void bcmxtmrt_ptmbond_tick (PBCMXTMRT_DEV_CONTEXT pDevCtx, XtmRtPtmBondInfo *pBondInfo, int timeCheck) ;
void *bond_memcpy (void * dest, void const * src, size_t cnt) ;

int ProcRxBondCtrs(struct file *file, char __user *buf, size_t len, loff_t *offset);
int ProcTxBondInfo(struct file *file, char __user *buf, size_t len, loff_t *offset);
int ProcRxBondSeq0(struct file *file, char __user *buf, size_t len, loff_t *offset);
int ProcRxBondSeq1(struct file *file, char __user *buf, size_t len, loff_t *offset);

#endif /* FAP_4KE */

/* Macro to wake up the RX thread */
#define BCMXTMRT_WAKEUP_RXWORKER(x) do {                    \
           if ((x)->rx_work_avail == 0) {                   \
               (x)->rx_work_avail = 1;                      \
               wake_up_interruptible(&((x)->rx_thread_wqh)); }} while (0)

#endif /* _BCMXTMRTIMPL_H */

