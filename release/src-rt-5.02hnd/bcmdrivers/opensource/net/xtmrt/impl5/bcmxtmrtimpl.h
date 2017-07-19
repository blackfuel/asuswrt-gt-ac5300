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

#include <bcmtypes.h>
#include "bcmnet.h"
#include "bcmxtmrt.h"
#if defined(CONFIG_BCM96318) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(SUPPORT_631XX_TX_RX_IUDMA)
#include "bcmPktDma_structs.h"
#else
#include <rdpa_api.h>
#endif

#include "bcm_mm.h"


#ifndef CARDNAME
#define CARDNAME        "bcmxtmrt"
#endif
#define XTMRT_VERSION   "0.9"

#define XTM_CACHE_SMARTFLUSH

#define TEQ_DATA_VCID    15

#define MAX_DEV_CTXS                16
#define MAX_MATCH_IDS               128
#define MAX_DEFAULT_MATCH_IDS       16
#define SAR_DMA_MAX_BURST_LENGTH    8
#define ENET_8021Q_SIZE             4
#define PTM_MAX_TX_FRAME_LEN        1980  /* Per chip limitation, 6318/63268/63381/63138/63148 SAR can only
                                           * transmit max PTM frame size of
                                           * 1980 + 4(FCS) = 1984
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

#define MAX_CIRCUIT_TYPES           16

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
/* If this definition is changed, please make sure accelerator (FAP/Runner)
** definitions are also in sync */
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

#if defined(CONFIG_BCM96318) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(SUPPORT_631XX_TX_RX_IUDMA)

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

#endif

#if !defined(SUPPORT_631XX_TX_RX_IUDMA)
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)

typedef struct BcmXtm_TxQueue
{
    volatile int txEnabled;
    UINT32 ulDmaIndex;
    UINT32 ulPort;
    UINT32 ulPtmPriority;
    UINT32 ulSubPriority;
//    UINT32 ulQueueSize;
    UINT8  ucDropAlg;            /* DT or RED or WRED */
    UINT8  ucLoMinThresh;        /* RED/WRED Low Class min threshold in % of queue size */
    UINT8  ucLoMaxThresh;        /* RED/WRED Low Class max threshold in % of queue size */
    UINT8  ucHiMinThresh;        /* WRED High Class min threshold in % of queue size */
    UINT8  ucHiMaxThresh;        /* WRED High Class max threshold in % of queue size */
} BcmXtm_TxQueue;

#define BcmPktDma_XtmTxDma BcmXtm_TxQueue

typedef struct BcmXtm_TxBdmfObj {
   bdmf_object_handle   xtmchannel; //tcont;
   bdmf_object_handle   egress_tm;
   bdmf_object_handle   xtmflow[MAX_CIRCUIT_TYPES];  //gem[MAX_CIRCUIT_TYPES];
} BcmXtm_TxBdmfObj;

#endif
#endif

#define TXQINFO  BcmPktDma_XtmTxDma
#define PTXQINFO BcmPktDma_XtmTxDma *


/* Struct added for xtmrt dmaStatus field generation for xtm flows - Apr 2010 */
typedef struct dev_params
{
    UINT32 ulFlags;
    UINT32 ulHdrType;
    UINT8  ucTxVcid;
} DEV_PARAMS, *PDEV_PARAMS;


#define PACKET_BLOG        0
#define PACKET_NORMAL      1

#include "bcmxtmrtbond.h"

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
    UINT32 ulTrafficType ;
    UINT32 ulTxPafEnabled ;
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

/* Information that is global to all network device instances. */
#define TXDMACTRL(pDevCtx)       g_GlobalInfo.dmaCtrl
#define TXDMATYPE(pDevCtx)       XTM_HW_DMA

/* ATM Cell header definitions - Ref G.998.1 */
#define ATM_CELL_HDR_VPI_MASK                  0x0FF00000
#define ATM_CELL_HDR_VPI_SHIFT                 20

#define ATM_NON_BONDED_CELL_HDR_VCI_MASK       0x000FFFF0
#define ATM_BONDED_CELL_HDR_VCI_MASK           0x00000FF0
#define ATM_CELL_HDR_VCI_SHIFT                 4

typedef struct bcmxtmrt_global_info
{
    /* Linux structures. */
    PBCMXTMRT_DEV_CONTEXT pDevCtxs[MAX_DEV_CTXS];
    PBCMXTMRT_DEV_CONTEXT pDevCtxsByMatchId[MAX_MATCH_IDS];
    UINT32                ulDevCtxMask;
    UINT32                ulIntEnableMask;
    struct atm_dev        *pAtmDev;
    struct net_device     *pTeqNetDev;  
    struct timer_list     Timer;

    /* Callback functions. */
    XTMRT_CALLBACK pfnOamHandler;
    void *pOamContext;
    XTMRT_CALLBACK pfnAsmHandler;
    void *pAsmContext;

    /* Bonding information */
    UINT32                 atmBondSidMode;
    XtmBondConfig          bondConfig;
    XtmRtPtmTxBondHeader   ptmBondHdr[XTMRT_PTM_BOND_MAX_FRAG_PER_PKT];
    XtmRtPtmBondInfo       ptmBondInfo;

    /* MIB counter registers. */
    UINT32 ulMibRxClrOnRead;
    volatile UINT32 *pulMibTxOctetCountBase;
    volatile UINT32 *pulMibRxCtrl;
    volatile UINT32 *pulMibRxMatch;
    volatile UINT32 *pulMibRxOctetCount;
    volatile UINT32 *pulMibRxPacketCount;
    volatile UINT32 *pulRxCamBase;
    
	 /* Temporary storage for stats collection */
	 struct net_device_stats dummy_stats;
   
    /* Everything else. */
    UINT32 ulChipRev;
    UINT32 ulDrvState;
    
    spinlock_t xtmlock_tx;
    spinlock_t xtmlock_rx;
    spinlock_t xtmlock_rx_regs;

    /* RX thread support */
    struct task_struct *rx_thread;
    wait_queue_head_t   rx_thread_wqh;
    int                 rx_work_avail;
#define XTMRT_BUDGET   32

#if defined(CONFIG_BCM96318) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM963381) || defined(SUPPORT_631XX_TX_RX_IUDMA)
    /* DMA, BD and buffer fields. */
    BcmXtm_RxDma       *rxdma[MAX_RECEIVE_QUEUES];
    volatile DmaRegs   *dmaCtrl;

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

    struct kmem_cache *xtmSkbCache;
#endif

#if !defined(SUPPORT_631XX_TX_RX_IUDMA)
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    bdmf_object_handle  bdmfXtm;
    bdmf_object_handle  bdmfWan;
    BcmXtm_TxBdmfObj    txBdmfObjs[MAX_TRANSMIT_QUEUES];
#endif
#endif
    
} BCMXTMRT_GLOBAL_INFO, *PBCMXTMRT_GLOBAL_INFO;

extern BCMXTMRT_GLOBAL_INFO g_GlobalInfo;

/**** Function Prototypes ****/
int bcmxtmrt_xmit(pNBuff_t pNBuf, struct net_device *dev);

UINT32 bcmxtmrt_processRxPkt(PBCMXTMRT_DEV_CONTEXT pDevCtx, void *rxdma,
                             pNBuff_t pNBuf, int is_skbuff,
                             UINT16 bufStatus, UINT32 len);
void bcmxtmrt_processRxCell(UINT8 *pData);
                             
                             

int bcmxtmrt_ptmbond_add_hdr(UINT32 ulTxPafEnabled, pNBuff_t *ppNBuff, UINT32 ulPtmPrioIdx);
int ProcTxBondInfo(struct file *file, char __user *buf,
                        size_t len, loff_t *offset);


#endif /* FAP_4KE */

/* Macro to wake up the RX thread */
#define BCMXTMRT_WAKEUP_RXWORKER(x) do {                    \
           if ((x)->rx_work_avail == 0) {                   \
               (x)->rx_work_avail = 1;                      \
               wake_up_interruptible(&((x)->rx_thread_wqh)); }} while (0)

#endif /* _BCMXTMRTIMPL_H */
