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



////////////////////////////////////////////////////////////////////////////////
/// \file Stats.c
/// \brief General Statistics support
///
/// This file contains the routines needed to handle statistics gathering
/// and retrieval
///
////////////////////////////////////////////////////////////////////////////////
#include <linux/string.h>
#include "bcm_epon_cfg.h"
#include "Teknovus.h"
#include "PonConfigDb.h"
#include "EponStats.h"
#include "EponUser.h"
#include <rdpa_api.h>


////////////////////////////////////////////////////////////////////////////////
/// Macro definition
////////////////////////////////////////////////////////////////////////////////
#define GetU32Stat(stat)                    ((stat) & 0xffffffffUL)

////////////////////////////////////////////////////////////////////////////////
/// Typedef enum definition
////////////////////////////////////////////////////////////////////////////////
// possible states of state collection
typedef enum
    {
    //Link RX are bidirectional link downstream stats
    StatGatherLinkRx1,
    StatGatherLinkRx2,
    //Link TX are bidirectional link upstream stats
    StatGatherLinkTx1,
    StatGatherLinkTx2,
    //UNI TX
    StatGatherRxOnlyLink,
    //Lif
    StatGatherLif,
#ifdef CONFIG_EPON_10G_SUPPORT 
    //Xif,Xpcs32,Xpcs40
    StatGatherXif,
    StatGatherXpcs32,
    StatGatherXpcs40,
#endif
    NumStatGatherStates
    } StatGatherState;

#if STATS_THRESHOLD
// possible states of state check
typedef enum
    {
    StatCheckEponRx,
    StatCheckEponTx,

    StatCheckLinkRx,
    StatCheckLinkTx,
    NumStatCheckStates
   } StatCheckState;
#endif


////////////////////////////////////////////////////////////////////////////////
/// Typedef struct definition
////////////////////////////////////////////////////////////////////////////////

/// statistic ehternet bound
typedef struct
    {
    U8 first;
    U8 last;
    U8 comp;
    } PACK StatsEthBound;

#if STATS_THRESHOLD
/// statistic threshold value
typedef struct
    {
    U32 raise;
    U32 clear;
    } PACK StatThdVal;

/// threshold crossing information
typedef struct
    {
    U32 raise;          ///< set condition when above
    U32 clear;          ///< clear condition when below
    U32 stats;          ///< stat count in current period
    U32 alarm;          ///< save alarm trigger value for alarm message
    U16 period;         ///< interval for threshold, 100 ms increments
    U16 tick;           ///< ticks for threshold, 100 ms increments
    } PACK StatThd;
#endif



////////////////////////////////////////////////////////////////////////////////
/// Constant definition
////////////////////////////////////////////////////////////////////////////////
const U8 CODE statGatherPollCtrl[NumStatGatherStates] =
    {
    TkOnuMaxBiDirLlids,         // StatGatherLinkRx1
    TkOnuMaxBiDirLlids,         // StatGatherLinkRx2
    TkOnuMaxBiDirLlids,         // StatGatherLinkTx1
    TkOnuMaxBiDirLlids,         // StatGatherLinkTx2
    TkOnuMaxRxOnlyLlids,        // StatGatherRxOnlyLink
#ifdef CONFIG_EPON_10G_SUPPORT 
    1,                          //StatGatherLif        
    1,                          //StatGatherXif 
    1,                          //StatGatherXpcs32
#endif
    1                           //StatGatherLif  or  StatGatherXpcs40
    };

#if STATS_THRESHOLD
////////////////////////////////////////////////////////////////////////////////
/// Constant definition
////////////////////////////////////////////////////////////////////////////////
const U8 CODE statCheckPollCtrl[NumStatCheckStates] =
    {
    1,                          // StatCheckEponRx,
    1,                          // StatCheckEponTx,
    TkOnuNumRxLlids,            // StatCheckLinkRx,
    TkOnuNumRxLlids,            // StatCheckLinkTx,
    };
#endif



////////////////////////////////////////////////////////////////////////////////
/// llidStatToEponStatMap - Llid stat to Epon stat mapping
///
////////////////////////////////////////////////////////////////////////////////
const EponLinkStatId CODE llidStatToEponStatMap[EponLinkHwStatCount] =
    {
    // Bidirectional stats
    EponBiDnTotalBytesRx,               // EponBiDnTotalBytesRx
    EponBiDnFcsErrors,                  // EponBiDnFcsErrors
    EponBiDnOamFramesRx,                // EponBiDnOamFramesRx
    EponBiDnGateFramesRx,               // EponBiDnGateFramesRx
    EponBiDn64ByteFramesRx,             // EponBiDn64ByteFramesRx
    EponBiDn65to127ByteFramesRx,        // EponBiDn65to127ByteFramesRx
    EponBiDn128to255ByteFramesRx,       // EponBiDn128to255ByteFramesRx
    EponBiDn256to511ByteFramesRx,       // EponBiDn256to511ByteFramesRx
    EponBiDn512to1023ByteFramesRx,      // EponBiDn512to1023ByteFramesRx
    EponBiDn1024to1518ByteFramesRx,     // EponBiDn1024to1518ByteFramesRx
    EponBiDn1518PlusByteFramesRx,       // EponBiDn1518PlusByteFramesRx
    EponBiDnOversizedFramesRx,          // EponBiDnOversizedFramesRx
    EponBiDnBroadcastFramesRx,          // EponBiDnBroadcastFramesRx
    EponBiDnMulticastFramesRx,          // EponBiDnMulticastFramesRx
    EponBiDnUnicastFramesRx,            // EponBiDnUnicastFramesRx
    EponBiDnUndersizedFramesRx,         // EponBiDnUndersizedFramesRx
    EponBiDnOamBytesRx,                 // EponBiDnOamBytesRx
    EponBiDnRegisterFramesRx,           // EponBiDnRegisterFramesRx

    // Downstream only stats
    EponBiDnTotalBytesRx,               // EponDnTotalBytesRx
    EponBiDnFcsErrors,                  // EponDnFCSErrors
    EponBiDnOamFramesRx,                // EponDnOamFramesRx
    EponBiDnGateFramesRx,               // EponDnGateFramesRx
    EponBiDnBroadcastFramesRx,          // EponDnBroadcastFramesRx
    EponBiDnMulticastFramesRx,          // EponDnMulticastFramesRx
    EponBiDnUnicastFramesRx,            // EponDnUnicastFramesRx
    EponLinkStatNotSupport,             // EponDn64to511ByteFramesRx
    EponBiDn512to1023ByteFramesRx,      // EponDn512to1023ByteFramesRx
    EponBiDn1024to1518ByteFramesRx,     // EponDn1024to1518ByteFramesRx
    EponBiDn1518PlusByteFramesRx,       // EponDn1518PlusByteFramesRx
    EponBiDnOversizedFramesRx,          // EponDnOversizedFramesRx
    EponBiDnUndersizedFramesRx,         // EponDnUndersizedFramesRx
    EponBiDnOamBytesRx,                 // EponDnOamBytesRx

    // Upstream only stats
    EponBiUpTotalBytesTx,               // EponBiUpTotalBytesTx
    EponBiUpReserved,                   // EponBiUpReserved
    EponBiUpOamFramesTx,                // EponBiUpOamFramesTx
    EponBiUpReportFramesTx,             // EponBiUpReportFramesTx
    EponBiUp64ByteFramesTx,             // EponBiUp64ByteFramesTx
    EponBiUp65To127ByteFramesTx,        // EponBiUp65To127ByteFramesTx
    EponBiUp128To255ByteFramesTx,       // EponBiUp128To255ByteFramesTx
    EponBiUp256To511ByteFramesTx,       // EponBiUp256To511ByteFramesTx
    EponBiUp512To1023ByteFamesTx,       // EponBiUp512To1023ByteFamesTx
    EponBiUp1024To1518ByteFramesTx,     // EponBiUp1024To1518ByteFramesTx
    EponBiUp1518PlusByteFramesTx,       // EponBiUp1518PlusByteFramesTx
    EponBiUpOamBytesTx,                 // EponBiUpOamBytesTx
    EponBiUpBroadcastFramesTx,          // EponBiUpBroadcastFramesTx
    EponBiUpMulticastFramesTx,          // EponBiUpMulticastFramesTx
    EponBiUpUnicastFramesTx,             // EponBiUpUnicastFramesTx
    EponLinkMPCPRegReq,                  //EponLinkMPCPRegReq
    EponLinkMPCPRegAck                   //EponLinkMPCPRegAck                   
    };





////////////////////////////////////////////////////////////////////////////////
/// Constant definition
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// statIdMap - StatId to Epon, Llid(bi-direction and rx-only) stat mapping
///
/// Notice:
/// Some of case need special process, for example, Frames Receive and Frames
/// Transmit Which is the sum of Unicast Frames, Broadcast Frames and Multicast
/// Frames.
///
////////////////////////////////////////////////////////////////////////////////
const TkStatIdMap statIdMap[StatIdNumStats] =
    {
    // Rx stats
        // StatIdBytesRx = StatIdRxFirst,
        {
        EponBiDnTotalBytesRx,
        EponBiDnTotalBytesRx,
        EponDnTotalBytesRx

        },

        // StatIdTotalFramesRx,
        {
        EponLinkStatNotSupport,             // Special Process for Epon
        EponLinkStatNotSupport,             // Special Process for Bidir Llid
        EponLinkStatNotSupport             // Special Process for RxOnly Llid       
        },

        // StatIdUnicastFramesRx,
        {
        EponBiDnUnicastFramesRx,
        EponBiDnUnicastFramesRx,
        EponDnUnicastFramesRx      
        },

        // StatIdMcastFramesRx,
        {
        EponBiDnMulticastFramesRx,
        EponBiDnMulticastFramesRx,
        EponDnMulticastFramesRx
        
        },

        // StatIdBcastFramesRx,
        {
        EponBiDnBroadcastFramesRx,
        EponBiDnBroadcastFramesRx,
        EponDnBroadcastFramesRx       
        },

        // StatIdFcsErr,
        {
        EponBiDnFcsErrors,
        EponBiDnFcsErrors,
        EponDnFCSErrors        
        },

        // StatIdCrc8Err,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdLineCodeErr,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdFrameTooShort,
        {
        EponBiDnUndersizedFramesRx,
        EponDnUndersizedFramesRx,
        EponBiDnUndersizedFramesRx        
        },

        // StatIdFrameTooLong,
        {
        EponBiDnOversizedFramesRx,
        EponBiDnOversizedFramesRx,
        EponDnOversizedFramesRx       
        },

        // StatIdInRangeLenErr,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdOutRangeLenErr,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdAlignErr,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // bin sizes available on Ethernet ports only

        // StatIdRx64Bytes,
        {
        EponBiDn64ByteFramesRx,
        EponBiDn64ByteFramesRx,
        EponLinkStatNotSupport
        },

        // StatIdRx65_127Bytes,
        {
        EponBiDn65to127ByteFramesRx,
        EponBiDn65to127ByteFramesRx,
        EponLinkStatNotSupport
        },

        // StatIdRx128_255Bytes,
        {
        EponBiDn128to255ByteFramesRx,
        EponBiDn128to255ByteFramesRx,
        EponLinkStatNotSupport
        },

        // StatIdRx256_511Bytes,
        {
        EponBiDn256to511ByteFramesRx,
        EponBiDn256to511ByteFramesRx,
        EponLinkStatNotSupport
        },

        // StatIdRx512_1023Bytes,
        {
        EponBiDn512to1023ByteFramesRx,
        EponBiDn512to1023ByteFramesRx,
        EponDn512to1023ByteFramesRx       
        },

        // StatIdRx1024_1518Bytes,
        {
        EponBiDn1024to1518ByteFramesRx,
        EponBiDn1024to1518ByteFramesRx,
        EponDn1024to1518ByteFramesRx      
        },

        // StatIdRx1519PlusBytes,
        {
        EponBiDn1518PlusByteFramesRx,
        EponBiDn1518PlusByteFramesRx,
        EponDn1518PlusByteFramesRx       
        },

        // StatIdRxFramesDropped, - dropped in queue, that is
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdRxBytesDropped, - dropped in queue, that is
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdRxBytesDelayed,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdRxMaxDelay,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport      
        },

        // StatIdRxDelayThresh,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdRxPauseFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdRxControlFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport        
        },

        // StatIdErrFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdErrFramePeriods,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },

        // StatIdErrFrameSummary, - last of RxStats
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport       
        },


        // Tx stats
        // StatIdTxFirst = StatIdNumRxStats,
        // StatIdBytesTx = StatIdTxFirst,
        {
        EponBiUpTotalBytesTx,
        EponBiUpTotalBytesTx,
        EponLinkStatNotSupport      
        },

        // StatIdTotalFramesTx,
        {
        EponLinkStatNotSupport,     // Special Process for Epon
        EponLinkStatNotSupport,     // Special Process for Bidir Llid
        EponLinkStatNotSupport     // Special Process for RxOnly Llid        
        },

        // StatIdUnicastFramesTx,
        {
        EponBiUpUnicastFramesTx,
        EponBiUpUnicastFramesTx,
        EponLinkStatNotSupport       
        },

        // StatIdMcastFramesTx,
        {
        EponBiUpMulticastFramesTx,
        EponBiUpMulticastFramesTx,
        EponLinkStatNotSupport       
        },

        // StatIdBcastFramesTx,
        {
        EponBiUpBroadcastFramesTx,
        EponBiUpBroadcastFramesTx,
        EponLinkStatNotSupport        
        },

        // StatIdSingleColl,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdMultiColl,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // StatIdLateColl,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // StatIdFrAbortXsColl,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // bin sizes available on Ethernet ports only

        // StatIdTx64Bytes,
        {
        EponBiUp64ByteFramesTx,
        EponBiUp64ByteFramesTx,
        EponLinkStatNotSupport
        
        },

        // StatIdTx65_127Bytes,
        {
        EponBiUp65To127ByteFramesTx,
        EponBiUp65To127ByteFramesTx,
        EponLinkStatNotSupport
       
        },

        // StatIdTx128_255Bytes,
        {
        EponBiUp128To255ByteFramesTx,
        EponBiUp128To255ByteFramesTx,
        EponLinkStatNotSupport
       
        },

        // StatIdTx256_511Bytes,
        {
        EponBiUp256To511ByteFramesTx,
        EponBiUp256To511ByteFramesTx,
        EponLinkStatNotSupport
       
        },

        // StatIdTx512_1023Bytes,
        {
        EponBiUp512To1023ByteFamesTx,
        EponBiUp512To1023ByteFamesTx,
        EponLinkStatNotSupport
        
        },

        // StatIdTx1024_1518Bytes,
        {
        EponBiUp1024To1518ByteFramesTx,
        EponBiUp1024To1518ByteFramesTx,
        EponLinkStatNotSupport
        
        },

        // StatIdTx1519PlusBytes,
        {
        EponBiUp1518PlusByteFramesTx,
        EponBiUp1518PlusByteFramesTx,
        EponLinkStatNotSupport
        
        },

        // StatIdTxFramesDropped,                 // dropped in queue, that is
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdTxBytesDropped,                 // dropped in queue, that is
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdTxBytesDelayed,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdTxMaxDelay,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // StatIdTxDelayThresh,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // StatIdTxUpUnusedBytes,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
       
        },

        // StatIdTxPauseFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },
        
        // StatIdTxControlFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdTxDeferredFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdTxExcessiveDeferralFrames,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdMpcpMACCtrlFramesTx,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdMpcpMACCtrlFramesRx,
        {
        EponLinkStatNotSupport,
        EponLinkStatNotSupport,
        EponLinkStatNotSupport
        
        },

        // StatIdMpcpTxRegAck,
        {
        EponLinkMPCPRegAck,
        EponLinkMPCPRegAck,
        EponLinkStatNotSupport
        
        },

        // StatIdMpcpTxRegRequest,
        {
        EponLinkMPCPRegReq,
        EponLinkMPCPRegReq,
        EponLinkStatNotSupport
        
        },

        // StatIdMpcpTxReport,
        {
        EponBiUpReportFramesTx,
        EponBiUpReportFramesTx,
        EponLinkStatNotSupport
       
        },

        // StatIdMpcpRxGate,
        {
        EponBiDnGateFramesRx,
        EponBiDnGateFramesRx,
        EponDnGateFramesRx
        
        },

        // StatIdMpcpRxRegister,
        {
        EponBiDnRegisterFramesRx,
        EponBiDnRegisterFramesRx,
        EponLinkStatNotSupport
        
        }
    };

const StatId CODE swStatIdMap[SwStatIdNum] = 
    {
    // SwStatIdTxBytesDropped
    StatIdTxBytesDropped,
        
    // SwStatIdTxFramesDropped
    StatIdTxFramesDropped,

    // SwStatIdTxTotalFrames
    StatIdTotalFramesTx,

    // SwStatIdRxBytesDropped,
    StatIdRxBytesDropped,
        
    // SwStatIdRxFramesDropped
    StatIdRxFramesDropped,

    // SwStatIdRxTotalFrames
    StatIdTotalFramesRx
    };


////////////////////////////////////////////////////////////////////////////////
/// Stats Buffer Definition
////////////////////////////////////////////////////////////////////////////////

// The Epon stats uses the sum of all Link stats.
// ----------------------------------------------------------
// |        (1)         |       (2)     |       (3)         |
// ----------------------------------------------------------
// The Epon stats table is most same as Link stats table. For Block 1 is records
// the sum of Bidirectional Link stats Plus the sum of Receive only stats.
// Block 2 is leave empty. Block 3 is the sum of Tx Link stats
U64 BULK statsPon[EponLinkHwStatCount];
U64 BULK statsLif[LifStatCount];
#ifdef CONFIG_EPON_10G_SUPPORT
U64 BULK statsXif[XifStatCount];
U64 BULK statsXpcs32[Xpcs32RxStatCount];
U64 BULK statsXpcs40[Xpcs40RxStatCount];
#endif

// Although we have 32 Links,  we use this table(16 * EponLinkHwStatCount) to
// save all the Link stats.
// This design is smart enough to save the code space and cut down the code
// difficulty.
// ----------------------------------------------------------
// |        (1)         |       (2)     |       (3)         |
// ----------------------------------------------------------
// Block 1 records the 16 bidirectional Rx stats, Block 2 records the 16 Receive
// only Stats. Block 3 records the 16 bidirectional Tx stats. So the dimension
// of 16 * EponLinkHwStatCount is enough to save all the 32 Links
U64 BULK statsLink[TkOnuMaxBiDirLlids][EponLinkHwStatCount];

// For Link stats, it don't support Total Frames receive and transmit stats, we
// caculate this stats by software and use this array to store the stats.
U64 BULK statsLinkSwStats[TkOnuNumRxLlids][SwStatIdNum];




// For Epon/Port stats, it don't support Total Frames receive and transmit
// stats, we caculate this stats by software and use this array to store the
// stats. For port Frame stats EPON will be port = 0, UNI ports will be
// port =  index + 1
U64 BULK statsPortSwStats[1][SwStatIdNum];


#if STATS_THRESHOLD
U16 BULK statThdTick = 0;
StatThd BULK statsPonThd[EponLinkHwStatCount+1];
StatThd BULK statsLinkThd[TkOnuNumBiDirLlids][EponLinkHwStatCount];
StatThd BULK statsLinkSwStatsThd[TkOnuNumRxLlids][SwStatIdNum];
StatThd BULK statsPortSwStatsThd[1][SwStatIdNum];
#endif



////////////////////////////////////////////////////////////////////////////////
/// Stats Poll Varible Definition
////////////////////////////////////////////////////////////////////////////////

/// current state of stats collection
StatGatherState FAST collectState = StatGatherLinkRx1;
U8 FAST collectInstance = 0;
BOOL FAST statsGatherEnabled = TRUE;

#if STATS_THRESHOLD
/// current state of stats check
StatCheckState FAST checkState = StatCheckEponRx;
U8 FAST checkInstance = 0;
#endif


////////////////////////////////////////////////////////////////////////////////
///
///Collect Stats Section
///
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// CollectLinkRx:  Store RX statistics for link
///
 // Parameters:
/// \param link link to collect
/// \param statStatrt first stat to collect
/// \param statEnd last stat to collect
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectLinkRx ( LinkIndex BULK lnk, U8 statStart, U8 statEnd )
    {
    U8 FAST stat;
    U64 BULK dbuf;
    // stats collected directly
    //downstream
    for (stat = statStart; stat <= statEnd; ++stat)
        {
        EponReadLinkStat(lnk, (EponLinkStatId)stat, &dbuf);
        if ((stat >= EponBiDnBroadcastFramesRx) &&
           (stat <= EponBiDnUnicastFramesRx))
            {
            U64AddEqU64(&statsLinkSwStats[lnk][SwStatIdRxTotalFrames],&dbuf);
            U64AddEqU64(&statsPortSwStats[0][SwStatIdRxTotalFrames],&dbuf);
#if STATS_THRESHOLD
            statsLinkSwStatsThd[lnk][SwStatIdRxTotalFrames].stats += 
                                                             GetU32Stat(dbuf);
            statsPortSwStatsThd[0][SwStatIdRxTotalFrames].stats += 
                                                             GetU32Stat(dbuf);
#endif
            }
        U64AddEqU64(&statsLink[lnk][stat],&dbuf);
        U64AddEqU64(&statsPon[stat],&dbuf);
		
        if ((stat >= EponBiDn1518PlusByteFramesRx) &&
            (stat < EponBiDnOversizedFramesRx))
        {
            U64AddEqU64(&statsLink[lnk][EponBiDnOversizedFramesRx],&dbuf);
            U64AddEqU64(&statsPon[EponBiDnOversizedFramesRx],&dbuf);
        }       
     
#if STATS_THRESHOLD
        statsLinkThd[lnk][stat].stats += GetU32Stat(dbuf);
        statsPonThd[stat].stats += GetU32Stat(dbuf);
#endif
        }
    } // CollectLinkRx



////////////////////////////////////////////////////////////////////////////////
/// CollectLinkTx:  Store TX statistics for link
///
 // Parameters:
/// \param link link to collect
/// \param statStatrt first stat to collect
/// \param statEnd last stat to collect
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectLinkTx ( LinkIndex BULK lnk, U8 statStart, U8 statEnd)
    {
    U8 FAST stat;
    U64 BULK dbuf;
    // stats collected directly
    //upstream
    for (stat = statStart; stat <= statEnd; ++stat)
        {
        EponReadLinkStat(lnk, (EponLinkStatId)stat, &dbuf);
        if ((stat >= EponBiUpBroadcastFramesTx) &&
            (stat <= EponBiUpUnicastFramesTx))
            {
            U64AddEqU64(&statsLinkSwStats[lnk][SwStatIdTxTotalFrames],&dbuf);
            U64AddEqU64(&statsPortSwStats[0][SwStatIdTxTotalFrames],&dbuf);
#if STATS_THRESHOLD
            statsLinkSwStatsThd[lnk][SwStatIdTxTotalFrames].stats += 
                                                             GetU32Stat(dbuf);

            statsPortSwStatsThd[0][SwStatIdTxTotalFrames].stats += 
                                                             GetU32Stat(dbuf);
#endif
            }
        U64AddEqU64(&statsLink[lnk][stat],&dbuf);
        U64AddEqU64(&statsPon[stat],&dbuf);
#if STATS_THRESHOLD
        statsLinkThd[lnk][stat].stats += GetU32Stat(dbuf);
        statsPonThd[stat].stats += GetU32Stat(dbuf);
#endif
        }
    } // CollectLinkTx


////////////////////////////////////////////////////////////////////////////////
/// CollectLinkRxOnly:  Store statistics for RX only links
///
 // Parameters:
/// \param index of the recieve only link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectLinkRxOnly (U8 lnk)
    {
    EponLinkStatId FAST stat;
    U64 BULK dbuf;
    // stats collected directly
    //downstream
    for (stat = EponDnTotalBytesRx; stat <= EponDnOamBytesRx; ++stat)
        {
        EponReadLinkStat (TkOnuFirstRxOnlyLlid + lnk,stat,&dbuf);
        if ((stat >= EponDnBroadcastFramesRx) &&
            (stat <= EponDnUnicastFramesRx))
            {
            U64AddEqU64 (&statsPortSwStats[0][SwStatIdRxTotalFrames],&dbuf);
           
#if STATS_THRESHOLD
            statsPortSwStatsThd[0][SwStatIdRxTotalFrames].stats += GetU32Stat(dbuf);
#endif
            }
        U64AddEqU64 (&statsLink[lnk][stat],&dbuf);
#if STATS_THRESHOLD
        statsLinkThd[lnk][stat].stats += GetU32Stat(dbuf);
#endif
        if (llidStatToEponStatMap[stat] != EponLinkStatNotSupport)
            {
            U64AddEqU64 (&statsPon[llidStatToEponStatMap[stat]],&dbuf);
#if STATS_THRESHOLD
            statsPonThd[llidStatToEponStatMap[stat]].stats += GetU32Stat(dbuf);
#endif
            }
        }
    }  // CollectLinkRxOnly


////////////////////////////////////////////////////////////////////////////////
/// CollectLif:  Store statistics for Wan port
///
 // Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectWanStats (void)
    {
    bdmf_object_handle port_obj = NULL;
    rdpa_port_stat_t  port_stat;
    int rc;

    bdmf_lock();
    rc = rdpa_port_get(rdpa_if_wan0, &port_obj);
    if (rc)
        goto unlock_exit;

    rc = rdpa_port_stat_get(port_obj, &port_stat);
    if (rc)
        goto unlock_exit;

    U64AddEqU32(&statsPortSwStats[0][SwStatIdRxFramesDropped], 
        calcStatDelta(statsPortSwStats[0][SwStatIdRxFramesDropped], port_stat.rx_discard_1));
    U64AddEqU32(&statsPortSwStats[0][SwStatIdTxFramesDropped], 
        calcStatDelta(statsPortSwStats[0][SwStatIdTxFramesDropped], port_stat.tx_discard));

unlock_exit:
    if (port_obj)
      bdmf_put(port_obj);

    bdmf_unlock();
    }  // CollectLinkRxOnly


////////////////////////////////////////////////////////////////////////////////
/// CollectLif:  Store statistics for Lif
///
 // Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectLif (void)
    {
    LifStatId lifStat;
    for (lifStat = LifStatFirst; lifStat < LifStatCount; ++lifStat)
        {
        U64AddEqU32(&statsLif[lifStat],LifReadStat(lifStat));
        }
    }  // CollectLinkRxOnly

#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// CollectXif:  Store statistics for Xif
///
// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectXif (void)
    {
    XifStatId xifStat;
    for (xifStat = XifStatFirst; xifStat < XifStatCount; ++xifStat)
        {
        U64AddEqU32(&statsXif[xifStat],XifReadStat(xifStat));
        }
    }
	
////////////////////////////////////////////////////////////////////////////////
/// CollectXpcs32:  Store statistics for Xpcs32
///
// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectXpcs32 (void)
    {
    Xpcs32RxStatId xpcs32Stat;
    for (xpcs32Stat = Xpcs32RxStatFirst; xpcs32Stat < Xpcs32RxStatFirst; ++xpcs32Stat)
        {
        U64AddEqU32(&statsXpcs32[xpcs32Stat],XifReadStat(xpcs32Stat));
        }
    }

////////////////////////////////////////////////////////////////////////////////
/// CollectXpcs40:  Store statistics for Xpcs40
///
// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CollectXpcs40 (void)
    {
    Xpcs40RxStatId xpcs40Stat;
    for (xpcs40Stat = Xpcs40RxStatFirst; xpcs40Stat < Xpcs40RxStatFirst; ++xpcs40Stat)
        {
        U64AddEqU32(&statsXpcs40[xpcs40Stat],XifReadStat(xpcs40Stat));
        }
    }
#endif


////////////////////////////////////////////////////////////////////////////////
/// StatsGather:  gather stats
///
/// This function gather the ONU stats including Epon, Uni, Llid and Fifo.
/// The Epon stats use the sum of all the Llid stats. The Uni stats use Xmc
/// stats for 10G Uni port, use Gmc stats for 1G Uni port and use Emc stats for
/// 100M Uni port. The Llid stats including 16 bidirectional Llids and 16
/// receive only Llids. The Fifo stats including Processor, Epon and Uni Fifo
/// stats.
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void StatsGather (void)
    {
    switch (collectState)
        {
        case StatGatherLinkRx1 :
            CollectLinkRx(collectInstance,
                          EponBiDnTotalBytesRx,
                          EponBiDn512to1023ByteFramesRx);
            break;
        case StatGatherLinkRx2 :
            CollectLinkRx(collectInstance,
                          EponBiDn1024to1518ByteFramesRx,
                          EponBiDnRegisterFramesRx);
            break;
        case StatGatherLinkTx1 :
            CollectLinkTx(collectInstance,
                          EponBiUpTotalBytesTx,
                          EponBiUp128To255ByteFramesTx);
            break;
        case StatGatherLinkTx2 :
            CollectLinkTx(collectInstance,
                          EponBiUp256To511ByteFramesTx,
                          EponBiUpUnicastFramesTx);
            break; 

        case StatGatherRxOnlyLink:
            CollectLinkRxOnly(collectInstance);
            break;

        case StatGatherLif:
            CollectWanStats();
            CollectLif();
            break;
            
#ifdef CONFIG_EPON_10G_SUPPORT
        case StatGatherXif:
            CollectXif();
            break;

        case StatGatherXpcs32:
            CollectXpcs32();
            break;

        case StatGatherXpcs40:
            CollectXpcs40();
            break;
#endif
        default :
            break;
        }

    // Cycle through instances and states
    if (++collectInstance >= statGatherPollCtrl[collectState])
        {
        collectInstance = 0;
        collectState++;
        if (collectState >= NumStatGatherStates)
            {
            collectState = StatGatherLinkRx1;
            }
        }
    } // StatsGather


#if STATS_THRESHOLD
////////////////////////////////////////////////////////////////////////////////
/// CheckEpon:  Check Epon stat for threshold crossings
///
 // Parameters:
/// \param statStatrt first stat to check
/// \param statEnd last stat to check
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CheckEpon (StatId statStart, StatId statEnd)
    {
    StatId FAST oamStat;
    EponLinkStatId FAST stat;
    StatThd BULK* FAST thd = NULL;

    for (oamStat = statStart; oamStat < statEnd; oamStat++)
        {
        stat = statIdMap[oamStat].eponStat;
        if (stat != EponLinkStatNotSupport)
            {
            thd = &statsPonThd[stat];
            }
        else
            {
            SwStatId FAST swStatId = StatsGetSwStatId(oamStat);
            if(swStatId != SwStatIdNum)
                {
                thd = &statsPortSwStatsThd[0][swStatId];
                }
            }

        if ((thd != NULL) && (thd->raise > 0) && (thd->period > 0))
            {
            if (U16LessThan((thd->tick + thd->period), statThdTick))
                {
                thd->tick = statThdTick;
                thd->alarm = thd->stats;

                if(thd->stats >= thd->raise)
                    {
                    
                    //OsAstMsgQSet (OnuAssertStatId(oamStat),
                    //   OsAstStatEponInst(0), oamStat);
                    }
                if(thd->stats <= thd->clear)
                    {
                    //OsAstMsgQClr (OnuAssertStatId(oamStat),
                    //    OsAstStatEponInst(0), oamStat);
                    }
                thd->stats = 0x0; // reset the stats
                }
            }
        }
    }//CheckEpon


////////////////////////////////////////////////////////////////////////////////
/// CheckLink:  Check Link stat for threshold crossings
///
 // Parameters:
/// \param link link to check
/// \param statStatrt first stat to check
/// \param statEnd last stat to check
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CheckLink(LinkIndex link, StatId statStart, StatId statEnd)
    {
    StatId FAST oamStat;
    EponLinkStatId FAST stat;
    StatThd BULK* FAST thd = NULL;

    for (oamStat = statStart; oamStat <= statEnd; oamStat++)
        {
        if(link < TkOnuNumBiDirLlids)
            {
            stat = statIdMap[oamStat].biDirLlidStat;
            if (stat != EponLinkStatNotSupport)
                {
                thd = &statsLinkThd[link][stat];
                }
            }
        else
            {
            stat = statIdMap[oamStat].rxOnlyLlidStat;
            if (stat != EponLinkStatNotSupport)
                {                    
                thd = &statsLinkThd[link - TkOnuNumBiDirLlids][stat];                    
                }
            }

        if(stat == EponLinkStatNotSupport)
            {
            SwStatId FAST swStatId = StatsGetSwStatId(oamStat);
            if(swStatId != SwStatIdNum)
                {
                thd = &statsLinkSwStatsThd[link][swStatId];
                }
            }

        if ((thd != NULL) && (thd->raise > 0) && (thd->period > 0))
            {
            if (U16LessThan((thd->tick + thd->period), statThdTick))
                {
                thd->tick = statThdTick;
                thd->alarm = thd->stats;
                if(thd->stats >= thd->raise)
                    {
                    //OsAstMsgQSet (OnuAssertStatId(oamStat),
                    //    OsAstStatLinkInst(link), oamStat);
                    }
                if(thd->stats <= thd->clear)
                    {
                    //OsAstMsgQClr (OnuAssertStatId(oamStat),
                    //    OsAstStatLinkInst(link), oamStat);
                    }
                thd->stats = 0x0; // reset the stats
                }
            }
        }
    }//CheckLink






////////////////////////////////////////////////////////////////////////////////
/// StatsCheck:  check stats
///
/// This function check the ONU stats including Epon, Uni, Llid and Fifo.
/// The Epon stats use the sum of all the Llid stats. The Uni stats use Xmc
/// stats for 10G Uni port, use Gmc stats for 1G Uni port and use Emc stats for
/// 100M Uni port. The Llid stats including 16 bidirectional Llids and 16
/// receive only Llids.
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void StatsCheck(void)
    {
    switch (checkState)
        {
        case StatCheckEponRx:
            CheckEpon(StatIdRxFirst, StatIdNumRxStats);
            break;

        case StatCheckEponTx:
            CheckEpon(StatIdTxFirst, StatIdNumStats);
            break;

        case StatCheckLinkRx:
            CheckLink(checkInstance, StatIdRxFirst, StatIdNumRxStats);
            break;

        case StatCheckLinkTx:
            CheckLink(checkInstance, StatIdTxFirst, StatIdNumStats);
            break;

        default:
            break;
        }

   // Cycle through instances and states
    if (++checkInstance >= statCheckPollCtrl[checkState])
        {
        checkInstance = 0;
        checkState++;
        if (checkState >= NumStatCheckStates)
            {
            checkState = StatCheckEponRx;
            }
        }
  } // StatsCheck
#endif


////////////////////////////////////////////////////////////////////////////////
/// StatsGetSwStatId:  get the software calculated statistic id for the statId
///
 // Parameters:
/// \param statId The statId to get
///
/// \return
/// SwStatId when found, SwStatIdNum otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
SwStatId StatsGetSwStatId(StatId statId)
    {
    SwStatId swStatId;
    for(swStatId = 0; swStatId < SwStatIdNum; swStatId++)
        {
        if(swStatIdMap[swStatId] == statId)
            {
            break;
            }
        }
    return swStatId;
    } // StatsGetSwStatId


////////////////////////////////////////////////////////////////////////////////
/// StatsPoll:  Statistics Polling Routine
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
// extern
static
void StatsPoll (EponTimerModuleId moduleId)
    {
    if (statsGatherEnabled)
        {
        StatsGather ();
#if STATS_THRESHOLD
        StatsCheck ();
#endif
        }
    } // StatsPoll




////////////////////////////////////////////////////////////////////////////////
/// StatsGetEpon:  Get Epon port stat
///
 // Parameters:
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsGetEpon (StatId id, U64 BULK * dst)

    {
    EponLinkStatId FAST stat = statIdMap[id].eponStat;
    
    if(stat != EponLinkStatNotSupport)
        {
        memcpy(dst, &statsPon[stat], sizeof(U64));
        }
    else
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            memcpy(dst, &statsPortSwStats[0][swStatId], sizeof(U64));
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }  // StatsGetEpon



////////////////////////////////////////////////////////////////////////////////
/// StatsGetLink:  Get epon link stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsGetLink (LinkIndex link, StatId id, U64 BULK * dst)
    {
    EponLinkStatId FAST stat;
    if(link < TkOnuNumBiDirLlids)
        {       
        stat = statIdMap[id].biDirLlidStat;

        if (stat != EponLinkStatNotSupport)
            {  
            memcpy (dst,&statsLink[link][stat],sizeof(U64));
            }
        }
    else
        {
        LinkIndex FAST linkIdx = (link - TkOnuNumBiDirLlids);
        stat = statIdMap[id].rxOnlyLlidStat; 

        if (stat != EponLinkStatNotSupport)
            {                
            memcpy (dst,&statsLink[linkIdx][stat],sizeof(U64));                
            }
        }

    if(stat == EponLinkStatNotSupport)
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            memcpy(dst, &statsLinkSwStats[link][swStatId], sizeof(U64));
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }  // StatsGetLink




////////////////////////////////////////////////////////////////////////////////
/// StatsGetLif:  Get lif stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsGetLif (LifStatId id, U64 BULK * dst)
    {
    memcpy(dst, &statsLif[id], sizeof(U64));
    return TRUE;
    }  // StatsGetLink
    
#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// StatsGetXif:  Get xif stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
BOOL StatsGetXif (XifStatId id, U64 BULK * dst)
    {
    memcpy(dst, &statsXif[id], sizeof(U64));
    return TRUE;
    }

////////////////////////////////////////////////////////////////////////////////
/// StatsGetXpcs32:  Get Xpcs32 stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
BOOL StatsGetXpcs32 (Xpcs32RxStatId id, U64 BULK * dst)
    {
    memcpy(dst, &statsXpcs32[id], sizeof(U64));
    return TRUE;
    }

////////////////////////////////////////////////////////////////////////////////
/// StatsGetXpcs40:  Get Xpcs40 stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
BOOL StatsGetXpcs40 (Xpcs40RxStatId id, U64 BULK * dst)
    {
    memcpy(dst, &statsXpcs40[id], sizeof(U64));
    return TRUE;
    }  
#endif  

#if STATS_THRESHOLD
////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponSwStatThdVal:  Get epon port Frame stat threshold value
///
 // Parameters:
/// \param stat   stat
/// \param thd    pointer to the stat threshold
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsGetEponSwStatThdVal (SwStatId stat, void BULK * thd)
    {
    ((StatThdVal BULK *)thd)->raise = statsPortSwStatsThd[0][stat].raise;
    ((StatThdVal BULK *)thd)->clear = statsPortSwStatsThd[0][stat].clear;
    }  // StatsGetEponSwStatThdVal


////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponThdVal:  Get epon port stat threshold value
///
 // Parameters:
/// \param stat   stat
/// \param thd    pointer to the stat threshold
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsGetEponThdVal (EponLinkStatId stat, void BULK * thd)
    {
    ((StatThdVal BULK *)thd)->raise = statsPonThd[stat].raise;
    ((StatThdVal BULK *)thd)->clear = statsPonThd[stat].clear;
    }  // StatsGetEponThdVal


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get EPON stat threshold value
///
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsEponThdValGet(StatId id, void BULK * val)
    {
    EponLinkStatId FAST stat = statIdMap[id].eponStat;
    if (stat != EponLinkStatNotSupport)
        {
        StatsGetEponThdVal(stat, val);
        }
    else
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            StatsGetEponSwStatThdVal(swStatId, val);
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkSwStatThdVal:  Get epon link stat Frame threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param thd    pointer to the stat value
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsGetLinkSwStatThdVal (LinkIndex link,SwStatId stat, void BULK * thd)
    {
    ((StatThdVal BULK *)thd)->raise = statsLinkSwStatsThd[link][stat].raise;
    ((StatThdVal BULK *)thd)->clear = statsLinkSwStatsThd[link][stat].clear;
    }  // StatsGetLinkSwStatThdVal


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkThdVal:  Get epon link stat threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param thd    pointer to the stat value
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsGetLinkThdVal (LinkIndex lnk, EponLinkStatId stat, void BULK * thd)
    {
    ((StatThdVal BULK *)thd)->raise = statsLinkThd[lnk][stat].raise;
    ((StatThdVal BULK *)thd)->clear = statsLinkThd[lnk][stat].clear;
    }  // StatsGetLinkThdVal




////////////////////////////////////////////////////////////////////////////////
/// \brief  Get link stat threshold value
///
/// \param link Index of link
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsLinkThdValGet(LinkIndex link, StatId id, void BULK * val)
    {
    EponLinkStatId FAST stat;
    if(link < TkOnuNumBiDirLlids)
        {
        stat = statIdMap[id].biDirLlidStat;
        if (stat != EponLinkStatNotSupport)
            {
            StatsGetLinkThdVal(link, stat, val);
            }
        }
    else
        {
        stat = statIdMap[id].rxOnlyLlidStat;
        if (stat != EponLinkStatNotSupport)
            {                
            StatsGetLinkThdVal(link - TkOnuNumBiDirLlids, stat, val);               
            }
        }
    
    if (stat == EponLinkStatNotSupport)
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            StatsGetLinkSwStatThdVal(link, swStatId, val);
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }







////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponSwStatThdVal:  Get epon port Frame stat threshold value
///
 // Parameters:
/// \param stat   stat
/// \param thd    pointer to the stat threshold
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsSetEponSwStatThdVal (SwStatId stat, const void BULK * thd)
    {
    statsPortSwStatsThd[0][stat].raise = ((StatThdVal BULK *)thd)->raise;
    statsPortSwStatsThd[0][stat].clear = ((StatThdVal BULK *)thd)->clear;
    }  // StatsSetEponSwStatThdVal


////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponThdVal:  Set epon port stat threshold value
///
 // Parameters:
/// \param stat   stat
/// \param thd    pointer to the stat threshold
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsSetEponThdVal (EponLinkStatId stat, const void * thd)
    {
    statsPonThd[stat].raise = ((StatThdVal BULK *)thd)->raise;
    statsPonThd[stat].clear = ((StatThdVal BULK *)thd)->clear;
    }  // StatsSetEponThdVal


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set EPON stat threshold value
///
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsEponThdValSet(StatId id, const void BULK * val)
    {
    EponLinkStatId FAST stat = statIdMap[id].eponStat;
    if (stat != EponLinkStatNotSupport)
        {
        StatsSetEponThdVal(stat, val);
        }
    else
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            StatsSetEponSwStatThdVal(swStatId, val);
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkSwStatThdVal:  Set epon link stat Frame threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param thd    pointer to the stat value
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsSetLinkSwStatThdVal (LinkIndex link,SwStatId stat,
                              const void BULK * thd)
    {
    statsLinkSwStatsThd[link][stat].raise = ((StatThdVal BULK *)thd)->raise;
    statsLinkSwStatsThd[link][stat].clear = ((StatThdVal BULK *)thd)->clear;
    }  // StatsSetLinkSwStatThdVal


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkThdVal:  Set epon link stat threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param thd    pointer to the stat value
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void StatsSetLinkThdVal (LinkIndex lnk, EponLinkStatId stat, const void * thd)
    {
    statsLinkThd[lnk][stat].raise = ((StatThdVal BULK *)thd)->raise;
    statsLinkThd[lnk][stat].clear = ((StatThdVal BULK *)thd)->clear;
    }  // StatsSetLinkThdVal




////////////////////////////////////////////////////////////////////////////////
/// \brief  Set link stat threshold value
///
/// \param link Index of link
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL StatsLinkThdValSet(LinkIndex link, StatId id, const void BULK * val)
    {
    EponLinkStatId FAST stat;
    if(link < TkOnuNumBiDirLlids)
        {
        stat = statIdMap[id].biDirLlidStat;
        if (stat != EponLinkStatNotSupport)
            {
            StatsSetLinkThdVal(link, stat, val);
            }
        }
    else
        {
        stat = statIdMap[id].rxOnlyLlidStat;
        if (stat != EponLinkStatNotSupport)
            {                
            StatsSetLinkThdVal(link - TkOnuNumBiDirLlids, stat, val);                
            }
        }
    
    if(stat == EponLinkStatNotSupport)
        {
        SwStatId FAST swStatId = StatsGetSwStatId(id);
        if(swStatId != SwStatIdNum)
            {
            StatsSetLinkSwStatThdVal(link, swStatId, val);
            }
        else
            {
            return FALSE;
            }
        }
    return TRUE;
    }






////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponThdInv:  Get epon port stat threshold interval
///
 // Parameters:
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
//extern
U16 StatsGetEponThdInv (EponLinkStatId stat)
    {
    return statsPonThd[stat].period;
    }  // StatsGetEponThdInv


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkThdInv:  Get epon link stat threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
//extern
U16 StatsGetLinkThdInv (LinkIndex lnk, EponLinkStatId stat)
    {
    return statsLinkThd[lnk][stat].period;
    }  // StatsGetLinkThdInv




////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponSwStatThdInv:  Get epon port Frame stat threshold interval
///
 // Parameters:
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
//extern
U16 StatsGetEponSwStatThdInv (SwStatId stat)
    {
    return statsPortSwStatsThd[0][stat].period;
    }  // StatsGetEponSwStatThdInv


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkSwStatThdInv:  Get epon link stat Frame threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
//extern
U16 StatsGetLinkSwStatThdInv (LinkIndex link,SwStatId stat)
    {
    return statsLinkSwStatsThd[link][stat].period;
    }  // StatsGetLinkSwStatThdInv



////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponThdInv:  Set epon port stat threshold interval
///
 // Parameters:
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsSetEponThdInv (EponLinkStatId stat, U16 inv)
    {
    statsPonThd[stat].period = inv;
    }  // StatsSetEponThdInv


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkThdInv:  Set epon link stat threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsSetLinkThdInv (LinkIndex lnk, EponLinkStatId stat, U16 inv)
    {
    statsLinkThd[lnk][stat].period = inv;
    }  // StatsSetLinkThdInv


////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponSwStatThdInv:  Set epon port Frame stat threshold interval
///
 // Parameters:
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsSetEponSwStatThdInv (SwStatId stat, U16 inv)
    {
    statsPortSwStatsThd[0][stat].period = inv;
    }  // StatsSetEponSwStatThdInv


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkSwStatThdInv:  Set epon link stat Frame threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsSetLinkSwStatThdInv (LinkIndex link,SwStatId stat, U16 inv)
    {
    statsLinkSwStatsThd[link][stat].period = inv;
    }  // StatsSetLinkSwStatThdInv

#endif

////////////////////////////////////////////////////////////////////////////////
/// StatsClearLif:  Reset all LIF stats to zero
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsClearLif (void)
   {
   LifStatId lifStat;

   for(lifStat = LifStatFirst; lifStat < LifStatCount; ++lifStat)
      {
      (void)LifReadStat(lifStat);
      }
   memset (statsLif, 0, sizeof(statsLif));
   }//StatsClearLif

#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// StatsClearXif:  Reset all XIF stats to zero
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StatsClearXif (void)
   {
   XifStatId xifStat;

   for(xifStat = XifStatFirst; xifStat < XifStatCount; ++xifStat)
      {
      (void)XifReadStat(xifStat);
      }
   memset (statsXif, 0, sizeof(statsXif));
   }//StatsClearXLif
   
////////////////////////////////////////////////////////////////////////////////
/// StatsClearXpcs32:  Reset all Xpcs32 stats to zero
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StatsClearXpcs32 (void)
   {
   Xpcs32RxStatId xpcs32Stat;

   for(xpcs32Stat = Xpcs32RxStatFirst; xpcs32Stat < Xpcs32RxStatCount; ++xpcs32Stat)
      {
      (void)XifXpcsRead32Stat(xpcs32Stat);
      }
   memset (statsXpcs32, 0, sizeof(statsXpcs32));
   }//StatsClearXpcs32
   
////////////////////////////////////////////////////////////////////////////////
/// StatsClearXpcs40:  Reset all Xpcs40 stats to zero
///
// Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StatsClearXpcs40 (void)
  {
  Xpcs40RxStatId xpcs40Stat;

  for(xpcs40Stat = Xpcs40RxStatFirst; xpcs40Stat < Xpcs40RxStatCount; ++xpcs40Stat)
     {
     (void)XifXpcsRead40Stat(xpcs40Stat);
     }
  memset (statsXpcs40, 0, sizeof(statsXpcs40));
  }//StatsClearXpcs40

#endif
////////////////////////////////////////////////////////////////////////////////
/// StatsClearNonLinkStats: Reset all non-link stats to zero
///
/// \param None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void StatsClearNonLinkStats (void)
    {
    U8 FAST stat;

    //general EPON stats
    for(stat = 0; stat <= EponUnmappedLlidSmallFrameCount; ++stat)
        {
        (void)EponReadStat(stat);
        }
    } // StatsClearNonLinkStats


////////////////////////////////////////////////////////////////////////////////
/// StatsClearEponLinkStats: Clear EPON link stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsClearEponLinkStats (U8 linkStart, U8 linkEnd)
    {
    EponLinkStatId FAST stat;
    U8 link;
    for(link = linkStart; link < linkEnd; ++link)
        {
        for (stat = EponBiDnTotalBytesRx;
             stat <= EponBiDnRegisterFramesRx; ++stat)
            {
            U64 buf;
            EponReadLinkStat(link,stat, &buf);
            if (((EponBiUpTotalBytesTx + stat)) <= EponBiUpUnicastFramesTx)
                {
                EponReadLinkStat(link, (EponBiUpTotalBytesTx + stat), &buf);
                }
            }
        }
    memset (statsLinkSwStats, 0, sizeof(statsLinkSwStats));
    } // StatsClearEponLinkStats


////////////////////////////////////////////////////////////////////////////////
/// StatsClearEpon: Reset all epon stats to zero
///
/// \param None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsClearEpon(void)
   {
   StatsClearNonLinkStats();
   StatsClearEponLinkStats(0,4);
   memset (statsLink, 0, sizeof(statsLink));
   memset (statsPon, 0, sizeof(statsPon));
   memset (statsLinkSwStats, 0, sizeof(statsLinkSwStats));
   memset (statsPortSwStats[0], 0, sizeof(statsPortSwStats[0]));
   } //StatsClearEpon


////////////////////////////////////////////////////////////////////////////////
/// StatsClearAll:  Zeroes all stats counters
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void StatsClearAll (void)
   {
   memset (statsPon, 0, sizeof(statsPon));
   memset (statsLink, 0, sizeof(statsLink));
   memset (statsLinkSwStats, 0, sizeof(statsLinkSwStats));
   
   memset (statsPortSwStats, 0, sizeof(statsPortSwStats));  
#if STATS_THRESHOLD
   memset (statsPonThd, 0, sizeof(statsPonThd));
   memset (statsLinkThd, 0, sizeof(statsLinkThd));
   memset (statsLinkSwStatsThd, 0, sizeof(statsLinkSwStatsThd));
   memset (statsPortSwStatsThd, 0, sizeof(statsPortSwStatsThd));
#endif
   }//StatsClearAll

void StatsGatherSet(BOOL flag)
    {
    statsGatherEnabled = flag;
    }


////////////////////////////////////////////////////////////////////////////////
/// StatsInit:  Initialize Statistics Manager
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
// extern
void StatsInit (void)
    {
    StatsClearAll ();
    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
    	StatsGatherTimer,StatsPoll);
    } // StatsInit

// end Stats.c
