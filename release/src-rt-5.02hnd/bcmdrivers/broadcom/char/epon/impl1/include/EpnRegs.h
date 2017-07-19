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
/// \file EpnReg.h
/// \brief Epn Register Definitions
///
////////////////////////////////////////////////////////////////////////////////
#if !defined(EpnRegs_h)
#define EpnRegs_h

#if defined(__cplusplus)
extern "C" {
#endif

#include "Teknovus.h"



#define VpbCfg                              TkOnuReg(0x0000)
#define VpbLifRst                           0x00000001UL
#define VpbEpnRst                           0x00000002UL
#define VpbNcoRst                           0x00000004UL
#define VpbClkDivRst                        0x000000081UL
#define VpbInt                              TkOnuReg(0x0001)

#define EpnBurstCapSft                       EpnBurstCap0Sft
#define EpnBurstCapMsk                       EpnBurstCap0Msk
#define EpnRegBankMacAddr                    0
#define EpnRegBankTxL1Q                      1
#define EpnRegBankReportPtr                  2
#define EpnRegBankTxL2Q                      3

#define EpnStatReadOp                        0
#define EpnStatWriteOp                       1

#define EpnDpRamSelStatsUp                   (1UL << 4)
#define EpnDpRamSelStatsDn                   0UL
    
#define EpnUnmapLlidStat(stat)               TkOnuReg(0x4A4 + (stat))

#define EpnAesPerLinkShift                   2
#define EpnAesMode(link, mode)               ((U32)(mode) <<\
                                             ((link) * EpnAesPerLinkShift))

#define EpnCtl                               TkOnuTable(0x0400)
#define EpnCtlCount                          0x00000002UL
#define EpnCtlWidth                          0x00000002UL
#define EpnCtlNotDrxRst                      0x00000001UL // offset 0
#define EpnCtlNotDrxRstSft                   0
#define EpnCtlNotDrxRstMsk                   0x00000001UL // [0:0]
//  Enable the Drx
#define EpnCtlDrxEn                          0x00000002UL // offset 1
//  Enable the Drx
#define EpnCtlDrxEnSft                       1
#define EpnCtlDrxEnMsk                       0x00000002UL // [1:1]
//  Enable the Drx loopback
#define EpnCtlDrxLoopBack                    0x00000004UL // offset 2
//  Enable the Drx loopback
#define EpnCtlDrxLoopBackSft                 2
#define EpnCtlDrxLoopBackMsk                 0x00000004UL // [2:2]

//  Enable the Drx Dns
#define EpnCtlDrxDns                         0x00000008UL // offset 3
//  Enable the Drx Dns
#define EpnCtlDrxDnsSft                      3
#define EpnCtlDrxDnsMsk                      0x00000008UL // [3:3]


#define EpnCtlNotUtxRst                      0x00000010UL // offset 4
#define EpnCtlNotUtxRstSft                   4
#define EpnCtlNotUtxRstMsk                   0x00000010UL // [4:4]
//  Enable Utx
#define EpnCtlUtxEn                          0x00000020UL // offset 5
//  Enable Utx
#define EpnCtlUtxEnSft                       5
#define EpnCtlUtxEnMsk                       0x00000020UL // [5:5]
//  Enable Utx loopback
#define EpnCtlUtxLoopBack                    0x00000040UL // offset 6
//  Enable Utx loopback
#define EpnCtlUtxLoopBackSft                 6
#define EpnCtlUtxLoopBackMsk                 0x00000040UL // [6:6]
//  Selects report frame style for all LLIDs
#define EpnCtlRptSelSft                      8
#define EpnCtlRptSelMsk                      0x00000300UL // [9:8]
//  Discover gate destination address filter
#define EpnCtlCfgIncDiDaFlt                  0x00000400UL // offset 10

#define EpnCtlCfgIncDiDaFltSft               10
#define EpnCtlCfgIncDiDaFltMsk               0x00000400UL // [10:10]

// Drop Unmapped LLID
#define EpnCtlCfgDropUnMap                   0x00001000UL // offset 10

#define EpnCtlCfgDropUnMapSft                12
#define EpnCtlCfgDropUnMapMsk                0x00001000UL // [10:10]


#define EpnCtlCfUpClkMux                     0x00004000UL // offset 14
#define EpnCtlCfUpClkMuxSft                  14
#define EpnCtlCfUpClkMuxMsk                  0x00004000UL // [14:14]
//  Corrupt only data frames
#define EpnCtlFcsErrOnlyDataFr               0x00020000UL // offset 17
//  Corrupt only data frames
#define EpnCtlFcsErrOnlyDataFrSft            17
#define EpnCtlFcsErrOnlyDataFrMsk            0x00020000UL // [17:17]
//  Max size is 1518. With Vlan 1522
#define EpnCtlCfgVlanMax                     0x00040000UL // offset 18
//  Max size is 1518. With Vlan 1522
#define EpnCtlCfgVlanMaxSft                  18
#define EpnCtlCfgVlanMaxMsk                  0x00040000UL // [18:18]
//  Enables empty queue discovery supression
#define EpnCtlPrvSupressDiscEn               0x00080000UL // offset 19
//  Enables empty queue discovery supression
#define EpnCtlPrvSupressDiscEnSft            19
#define EpnCtlPrvSupressDiscEnMsk            0x00080000UL // [19:19]
//  Disables FCS checking for unmapped frames
#define EpnCtlPrvNoUnmapppedFcs              0x00100000UL // offset 20
//  Disables FCS checking for unmapped frames
#define EpnCtlPrvNoUnmapppedFcsSft           20
#define EpnCtlPrvNoUnmapppedFcsMsk           0x00100000UL // [20:20]
//  Set to increment non-zero accumulators
#define EpnCtlPrvIncNonZeroAccum             0x00200000UL // offset 21
//  Set to increment non-zero accumulators
#define EpnCtlPrvIncNonZeroAccumSft          21
#define EpnCtlPrvIncNonZeroAccumMsk          0x00200000UL // [21:21]
//  Prefetch accum during congestion
#define EpnCtlPrvTekModePrefetch             0x00800000UL // offset 23
//  Prefetch accum during congestion
#define EpnCtlPrvTekModePrefetchSft          23
#define EpnCtlPrvTekModePrefetchMsk          0x00800000UL // [23:23]
//  Drop single cpoy broadcast packet
#define EpnCtlCfgDropScb                     0x10000000UL // offset 28
#define EpnCtlCfgDropScbSft                  28
#define EpnCtlCfgDropScbMsk                  0x10000000UL // [28:28]

//  Clear Statistics after Proc. read
#define EpnCtlClrOnRd                        0x00000002UL // offset 1
//  Clear Statistics after Proc. read
#define EpnCtlClrOnRdSft                     1
#define EpnCtlClrOnRdMsk                     0x00000002UL // [1:1]
//  Disable scaling of discovery window
#define EpnCtlDisableDiscScale               0x00000004UL // offset 2
//  Disable scaling of discovery window
#define EpnCtlDisableDiscScaleSft            2
#define EpnCtlDisableDiscScaleMsk            0x00000004UL // [2:2]
//  ignore force report on discovery Gate
#define EpnCtlCfgNoDiscRpt                   0x00000008UL // offset 3
//  ignore force report on discovery Gate
#define EpnCtlCfgNoDiscRptSft                3
#define EpnCtlCfgNoDiscRptMsk                0x00000008UL // [3:3]
//  Disable incremntal timestamp correction
#define EpnCtlCfgTsCorrDis                   0x00000010UL // offset 4
//  Disable incremntal timestamp correction
#define EpnCtlCfgTsCorrDisSft                4
#define EpnCtlCfgTsCorrDisMsk                0x00000010UL // [4:4]
//  Selects number of priority levels per LLID
#define EpnCtlCfgCtcRptSft                   5
#define EpnCtlCfgCtcRptMsk                   0x00000060UL // [6:5]
//  FEC reporting enabled
#define EpnCtlFecRptEn                       0x00020000UL // offset 17
//  FEC reporting enabled
#define EpnCtlFecRptEnSft                    17
#define EpnCtlFecRptEnMsk                    0x00020000UL // [17:17]
//  Check for stale grants
#define EpnCtlCfgStaleGntChk                 0x00040000UL // offset 18
//  Check for stale grants
#define EpnCtlCfgStaleGntChkSft              18
#define EpnCtlCfgStaleGntChkMsk              0x00040000UL // [18:18]
//  Enables proprietary report fields
#define EpnCtlCfgTekRpt                      0x00080000UL // offset 19
//  Enables proprietary report fields
#define EpnCtlCfgTekRptSft                   19
#define EpnCtlCfgTekRptMsk                   0x00080000UL // [19:19]
//  Reset misalignment counters
#define EpnCtlRstMisalignThr                 0x00100000UL // offset 20
//  Reset misalignment counters
#define EpnCtlRstMisalignThrSft              20
#define EpnCtlRstMisalignThrMsk              0x00100000UL // [20:20]
//  Enable overlapped grant processing
#define EpnCtlPrvOverlappedGntEn             0x00200000UL // offset 21
//  Enable overlapped grant processing
#define EpnCtlPrvOverlappedGntEnSft          21
#define EpnCtlPrvOverlappedGntEnMsk          0x00200000UL // [21:21]

#define EpnEnGnt                             TkOnuReg(0x0402)
//  Enable grant reception on LLID index 0
#define EpnEnGnt0                            0x00000001UL // offset 0
//  Enable grant reception on LLID index 0
#define EpnEnGnt0Sft                         0
#define EpnEnGnt0Msk                         0x00000001UL // [0:0]
//  Enable grant reception on LLID index 1
#define EpnEnGnt1                            0x00000002UL // offset 1
//  Enable grant reception on LLID index 1
#define EpnEnGnt1Sft                         1
#define EpnEnGnt1Msk                         0x00000002UL // [1:1]
//  Enable grant reception on LLID index 2
#define EpnEnGnt2                            0x00000004UL // offset 2
//  Enable grant reception on LLID index 2
#define EpnEnGnt2Sft                         2
#define EpnEnGnt2Msk                         0x00000004UL // [2:2]
//  Enable grant reception on LLID index 3
#define EpnEnGnt3                            0x00000008UL // offset 3
//  Enable grant reception on LLID index 3
#define EpnEnGnt3Sft                         3
#define EpnEnGnt3Msk                         0x00000008UL // [3:3]
//  Enable grant reception on LLID index 4
#define EpnEnGnt4                            0x00000010UL // offset 4
//  Enable grant reception on LLID index 4
#define EpnEnGnt4Sft                         4
#define EpnEnGnt4Msk                         0x00000010UL // [4:4]
//  Enable grant reception on LLID index 5
#define EpnEnGnt5                            0x00000020UL // offset 5
//  Enable grant reception on LLID index 5
#define EpnEnGnt5Sft                         5
#define EpnEnGnt5Msk                         0x00000020UL // [5:5]
//  Enable grant reception on LLID index 6
#define EpnEnGnt6                            0x00000040UL // offset 6
//  Enable grant reception on LLID index 6
#define EpnEnGnt6Sft                         6
#define EpnEnGnt6Msk                         0x00000040UL // [6:6]
//  Enable grant reception on LLID index 7
#define EpnEnGnt7                            0x00000080UL // offset 7
//  Enable grant reception on LLID index 7
#define EpnEnGnt7Sft                         7
#define EpnEnGnt7Msk                         0x00000080UL // [7:7]


#define EpnDropDiscGates                     TkOnuReg(0x0403)
//  Ignore Disco gates on LLID Index 0
#define EpnDropDiscGatesSink0                0x00000001UL // offset 0
//  Ignore Disco gates on LLID Index 0
#define EpnDropDiscGatesSink0Sft             0
#define EpnDropDiscGatesSink0Msk             0x00000001UL // [0:0]
//  Ignore Disco gates on LLID Index 1
#define EpnDropDiscGatesSink1                0x00000002UL // offset 1
//  Ignore Disco gates on LLID Index 1
#define EpnDropDiscGatesSink1Sft             1
#define EpnDropDiscGatesSink1Msk             0x00000002UL // [1:1]
//  Ignore Disco gates on LLID Index 2
#define EpnDropDiscGatesSink2                0x00000004UL // offset 2
//  Ignore Disco gates on LLID Index 2
#define EpnDropDiscGatesSink2Sft             2
#define EpnDropDiscGatesSink2Msk             0x00000004UL // [2:2]
//  Ignore Disco gates on LLID Index 3
#define EpnDropDiscGatesSink3                0x00000008UL // offset 3
//  Ignore Disco gates on LLID Index 3
#define EpnDropDiscGatesSink3Sft             3
#define EpnDropDiscGatesSink3Msk             0x00000008UL // [3:3]
//  Ignore Disco gates on LLID Index 4
#define EpnDropDiscGatesSink4                0x00000010UL // offset 4
//  Ignore Disco gates on LLID Index 4
#define EpnDropDiscGatesSink4Sft             4
#define EpnDropDiscGatesSink4Msk             0x00000010UL // [4:4]
//  Ignore Disco gates on LLID Index 5
#define EpnDropDiscGatesSink5                0x00000020UL // offset 5
//  Ignore Disco gates on LLID Index 5
#define EpnDropDiscGatesSink5Sft             5
#define EpnDropDiscGatesSink5Msk             0x00000020UL // [5:5]
//  Ignore Disco gates on LLID Index 6
#define EpnDropDiscGatesSink6                0x00000040UL // offset 6
//  Ignore Disco gates on LLID Index 6
#define EpnDropDiscGatesSink6Sft             6
#define EpnDropDiscGatesSink6Msk             0x00000040UL // [6:6]
//  Ignore Disco gates on LLID Index 7
#define EpnDropDiscGatesSink7                0x00000080UL // offset 7
//  Ignore Disco gates on LLID Index 7
#define EpnDropDiscGatesSink7Sft             7
#define EpnDropDiscGatesSink7Msk             0x00000080UL // [7:7]


#define EpnDisFcsChk                         TkOnuReg(0x0404)
//  Do not check FCS on llid index 0 frames
#define EpnDisFcsChkableChk0                 0x00000001UL // offset 0
//  Do not check FCS on llid index 0 frames
#define EpnDisFcsChkableChk0Sft              0
#define EpnDisFcsChkableChk0Msk              0x00000001UL // [0:0]
//  Do not check FCS on llid index 1 frames
#define EpnDisFcsChkableChk1                 0x00000002UL // offset 1
//  Do not check FCS on llid index 1 frames
#define EpnDisFcsChkableChk1Sft              1
#define EpnDisFcsChkableChk1Msk              0x00000002UL // [1:1]
//  Do not check FCS on llid index 2 frames
#define EpnDisFcsChkableChk2                 0x00000004UL // offset 2
//  Do not check FCS on llid index 2 frames
#define EpnDisFcsChkableChk2Sft              2
#define EpnDisFcsChkableChk2Msk              0x00000004UL // [2:2]
//  Do not check FCS on llid index 3 frames
#define EpnDisFcsChkableChk3                 0x00000008UL // offset 3
//  Do not check FCS on llid index 3 frames
#define EpnDisFcsChkableChk3Sft              3
#define EpnDisFcsChkableChk3Msk              0x00000008UL // [3:3]
//  Do not check FCS on llid index 4 frames
#define EpnDisFcsChkableChk4                 0x00000010UL // offset 4
//  Do not check FCS on llid index 4 frames
#define EpnDisFcsChkableChk4Sft              4
#define EpnDisFcsChkableChk4Msk              0x00000010UL // [4:4]
//  Do not check FCS on llid index 5 frames
#define EpnDisFcsChkableChk5                 0x00000020UL // offset 5
//  Do not check FCS on llid index 5 frames
#define EpnDisFcsChkableChk5Sft              5
#define EpnDisFcsChkableChk5Msk              0x00000020UL // [5:5]
//  Do not check FCS on llid index 6 frames
#define EpnDisFcsChkableChk6                 0x00000040UL // offset 6
//  Do not check FCS on llid index 6 frames
#define EpnDisFcsChkableChk6Sft              6
#define EpnDisFcsChkableChk6Msk              0x00000040UL // [6:6]
//  Do not check FCS on llid index 7 frames
#define EpnDisFcsChkableChk7                 0x00000080UL // offset 7
//  Do not check FCS on llid index 7 frames
#define EpnDisFcsChkableChk7Sft              7
#define EpnDisFcsChkableChk7Msk              0x00000080UL // [7:7]
//  Do not check FCS on llid index 8 frames
#define EpnDisFcsChkableChk8                 0x00000100UL // offset 8
//  Do not check FCS on llid index 8 frames
#define EpnDisFcsChkableChk8Sft              8
#define EpnDisFcsChkableChk8Msk              0x00000100UL // [8:8]
//  Do not check FCS on llid index 9 frames
#define EpnDisFcsChkableChk9                 0x00000200UL // offset 9
//  Do not check FCS on llid index 9 frames
#define EpnDisFcsChkableChk9Sft              9
#define EpnDisFcsChkableChk9Msk              0x00000200UL // [9:9]
//  Do not check FCS on llid index 10 frames
#define EpnDisFcsChkableChk10                0x00000400UL // offset 10
//  Do not check FCS on llid index 10 frames
#define EpnDisFcsChkableChk10Sft             10
#define EpnDisFcsChkableChk10Msk             0x00000400UL // [10:10]
//  Do not check FCS on llid index 11 frames
#define EpnDisFcsChkableChk11                0x00000800UL // offset 11
//  Do not check FCS on llid index 11 frames
#define EpnDisFcsChkableChk11Sft             11
#define EpnDisFcsChkableChk11Msk             0x00000800UL // [11:11]
//  Do not check FCS on llid index 12 frames
#define EpnDisFcsChkableChk12                0x00001000UL // offset 12
//  Do not check FCS on llid index 12 frames
#define EpnDisFcsChkableChk12Sft             12
#define EpnDisFcsChkableChk12Msk             0x00001000UL // [12:12]
//  Do not check FCS on llid index 13 frames
#define EpnDisFcsChkableChk13                0x00002000UL // offset 13
//  Do not check FCS on llid index 13 frames
#define EpnDisFcsChkableChk13Sft             13
#define EpnDisFcsChkableChk13Msk             0x00002000UL // [13:13]
//  Do not check FCS on llid index 14 frames
#define EpnDisFcsChkableChk14                0x00004000UL // offset 14
//  Do not check FCS on llid index 14 frames
#define EpnDisFcsChkableChk14Sft             14
#define EpnDisFcsChkableChk14Msk             0x00004000UL // [14:14]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk15                0x00008000UL // offset 15
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk15Sft             15
#define EpnDisFcsChkableChk15Msk             0x00008000UL // [15:15]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk16                0x00010000UL // offset 16
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk16Sft             16
#define EpnDisFcsChkableChk16Msk             0x00010000UL // [16:16]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk17                0x00020000UL // offset 17
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk17Sft             17
#define EpnDisFcsChkableChk17Msk             0x00020000UL // [17:17]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk18                0x00040000UL // offset 18
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk18Sft             18
#define EpnDisFcsChkableChk18Msk             0x00040000UL // [18:18]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk19                0x00080000UL // offset 19
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk19Sft             19
#define EpnDisFcsChkableChk19Msk             0x00080000UL // [19:19]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk20                0x00100000UL // offset 20
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk20Sft             20
#define EpnDisFcsChkableChk20Msk             0x00100000UL // [20:20]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk21                0x00200000UL // offset 21
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk21Sft             21
#define EpnDisFcsChkableChk21Msk             0x00200000UL // [21:21]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk22                0x00400000UL // offset 22
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk22Sft             22
#define EpnDisFcsChkableChk22Msk             0x00400000UL // [22:22]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk23                0x00800000UL // offset 23
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk23Sft             23
#define EpnDisFcsChkableChk23Msk             0x00800000UL // [23:23]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk24                0x01000000UL // offset 24
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk24Sft             24
#define EpnDisFcsChkableChk24Msk             0x01000000UL // [24:24]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk25                0x02000000UL // offset 25
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk25Sft             25
#define EpnDisFcsChkableChk25Msk             0x02000000UL // [25:25]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk26                0x04000000UL // offset 26
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk26Sft             26
#define EpnDisFcsChkableChk26Msk             0x04000000UL // [26:26]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk27                0x08000000UL // offset 27
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk27Sft             27
#define EpnDisFcsChkableChk27Msk             0x08000000UL // [27:27]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk28                0x10000000UL // offset 28
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk28Sft             28
#define EpnDisFcsChkableChk28Msk             0x10000000UL // [28:28]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk29                0x20000000UL // offset 29
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk29Sft             29
#define EpnDisFcsChkableChk29Msk             0x20000000UL // [29:29]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk30                0x40000000UL // offset 30
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk30Sft             30
#define EpnDisFcsChkableChk30Msk             0x40000000UL // [30:30]
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk31                0x80000000UL // offset 31
//  Do not check FCS on llid index 15 frames
#define EpnDisFcsChkableChk31Sft             31
#define EpnDisFcsChkableChk31Msk             0x80000000UL // [31:31]

#define EpnPassGates                         TkOnuReg(0x0405)
//  Pass gate frames received on LLID 0
#define EpnPassGatesGateLlid0                0x00000001UL // offset 0
//  Pass gate frames received on LLID 0
#define EpnPassGatesGateLlid0Sft             0
#define EpnPassGatesGateLlid0Msk             0x00000001UL // [0:0]
//  Pass gate frames received on LLID 1
#define EpnPassGatesGateLlid1                0x00000002UL // offset 1
//  Pass gate frames received on LLID 1
#define EpnPassGatesGateLlid1Sft             1
#define EpnPassGatesGateLlid1Msk             0x00000002UL // [1:1]
//  Pass gate frames received on LLID 2
#define EpnPassGatesGateLlid2                0x00000004UL // offset 2
//  Pass gate frames received on LLID 2
#define EpnPassGatesGateLlid2Sft             2
#define EpnPassGatesGateLlid2Msk             0x00000004UL // [2:2]
//  Pass gate frames received on LLID 3
#define EpnPassGatesGateLlid3                0x00000008UL // offset 3
//  Pass gate frames received on LLID 3
#define EpnPassGatesGateLlid3Sft             3
#define EpnPassGatesGateLlid3Msk             0x00000008UL // [3:3]
//  Pass gate frames received on LLID 4
#define EpnPassGatesGateLlid4                0x00000010UL // offset 4
//  Pass gate frames received on LLID 4
#define EpnPassGatesGateLlid4Sft             4
#define EpnPassGatesGateLlid4Msk             0x00000010UL // [4:4]
//  Pass gate frames received on LLID 5
#define EpnPassGatesGateLlid5                0x00000020UL // offset 5
//  Pass gate frames received on LLID 5
#define EpnPassGatesGateLlid5Sft             5
#define EpnPassGatesGateLlid5Msk             0x00000020UL // [5:5]
//  Pass gate frames received on LLID 6
#define EpnPassGatesGateLlid6                0x00000040UL // offset 6
//  Pass gate frames received on LLID 6
#define EpnPassGatesGateLlid6Sft             6
#define EpnPassGatesGateLlid6Msk             0x00000040UL // [6:6]
//  Pass gate frames received on LLID 7
#define EpnPassGatesGateLlid7                0x00000080UL // offset 7
//  Pass gate frames received on LLID 7
#define EpnPassGatesGateLlid7Sft             7
#define EpnPassGatesGateLlid7Msk             0x00000080UL // [7:7]

//  Pass gatesE received on unmapped LLID
#define EpnPassGatesGateLlid16	            0x00010000UL 
#define EpnPassGatesGateLlid16Sft            16
#define EpnPassGatesGateLlid16Msk            0x00010000UL // [16:16]

#define EpnCfgMisalignFb                     TkOnuReg(0x0406)
//  Make LLID 0 pause reports on misalign
#define EpnCfgMisalignFbFeedback0            0x00000001UL // offset 0
//  Make LLID 0 pause reports on misalign
#define EpnCfgMisalignFbFeedback0Sft         0
#define EpnCfgMisalignFbFeedback0Msk         0x00000001UL // [0:0]
//  Make LLID 1 pause reports on misalign
#define EpnCfgMisalignFbFeedback1            0x00000002UL // offset 1
//  Make LLID 1 pause reports on misalign
#define EpnCfgMisalignFbFeedback1Sft         1
#define EpnCfgMisalignFbFeedback1Msk         0x00000002UL // [1:1]
//  Make LLID 2 pause reports on misalign
#define EpnCfgMisalignFbFeedback2            0x00000004UL // offset 2
//  Make LLID 2 pause reports on misalign
#define EpnCfgMisalignFbFeedback2Sft         2
#define EpnCfgMisalignFbFeedback2Msk         0x00000004UL // [2:2]
//  Make LLID 3 pause reports on misalign
#define EpnCfgMisalignFbFeedback3            0x00000008UL // offset 3
//  Make LLID 3 pause reports on misalign
#define EpnCfgMisalignFbFeedback3Sft         3
#define EpnCfgMisalignFbFeedback3Msk         0x00000008UL // [3:3]
//  Make LLID 4 pause reports on misalign
#define EpnCfgMisalignFbFeedback4            0x00000010UL // offset 4
//  Make LLID 4 pause reports on misalign
#define EpnCfgMisalignFbFeedback4Sft         4
#define EpnCfgMisalignFbFeedback4Msk         0x00000010UL // [4:4]
//  Make LLID 5 pause reports on misalign
#define EpnCfgMisalignFbFeedback5            0x00000020UL // offset 5
//  Make LLID 5 pause reports on misalign
#define EpnCfgMisalignFbFeedback5Sft         5
#define EpnCfgMisalignFbFeedback5Msk         0x00000020UL // [5:5]
//  Make LLID 6 pause reports on misalign
#define EpnCfgMisalignFbFeedback6            0x00000040UL // offset 6
//  Make LLID 6 pause reports on misalign
#define EpnCfgMisalignFbFeedback6Sft         6
#define EpnCfgMisalignFbFeedback6Msk         0x00000040UL // [6:6]
//  Make LLID 7 pause reports on misalign
#define EpnCfgMisalignFbFeedback7            0x00000080UL // offset 7
//  Make LLID 7 pause reports on misalign
#define EpnCfgMisalignFbFeedback7Sft         7
#define EpnCfgMisalignFbFeedback7Msk         0x00000080UL // [7:7]


#define EpnDiscFilter                        TkOnuReg(0x0407)
//  Expected discovery information value
#define EpnDiscFilterPrvInfoValSft           0
#define EpnDiscFilterPrvInfoValMsk           0x0000FFFFUL // [15:0]
//  Expected discovery information mask
#define EpnDiscFilterPrvInfoMskSft           16
#define EpnDiscFilterPrvInfoMskMsk           0xFFFF0000UL // [31:16]

#define EpnRstGntQ                           TkOnuReg(0x0408)
//  clear grant fifo 0
#define EpnRstGntQQ0                         0x00000001UL // offset 0
//  clear grant fifo 0
#define EpnRstGntQQ0Sft                      0
#define EpnRstGntQQ0Msk                      0x00000001UL // [0:0]
//  clear grant fifo 1
#define EpnRstGntQQ1                         0x00000002UL // offset 1
//  clear grant fifo 1
#define EpnRstGntQQ1Sft                      1
#define EpnRstGntQQ1Msk                      0x00000002UL // [1:1]
//  clear grant fifo 2
#define EpnRstGntQQ2                         0x00000004UL // offset 2
//  clear grant fifo 2
#define EpnRstGntQQ2Sft                      2
#define EpnRstGntQQ2Msk                      0x00000004UL // [2:2]
//  clear grant fifo 3
#define EpnRstGntQQ3                         0x00000008UL // offset 3
//  clear grant fifo 3
#define EpnRstGntQQ3Sft                      3
#define EpnRstGntQQ3Msk                      0x00000008UL // [3:3]
//  clear grant fifo 4
#define EpnRstGntQQ4                         0x00000010UL // offset 4
//  clear grant fifo 4
#define EpnRstGntQQ4Sft                      4
#define EpnRstGntQQ4Msk                      0x00000010UL // [4:4]
//  clear grant fifo 5
#define EpnRstGntQQ5                         0x00000020UL // offset 5
//  clear grant fifo 5
#define EpnRstGntQQ5Sft                      5
#define EpnRstGntQQ5Msk                      0x00000020UL // [5:5]
//  clear grant fifo 6
#define EpnRstGntQQ6                         0x00000040UL // offset 6
//  clear grant fifo 6
#define EpnRstGntQQ6Sft                      6
#define EpnRstGntQQ6Msk                      0x00000040UL // [6:6]
//  clear grant fifo 7
#define EpnRstGntQQ7                         0x00000080UL // offset 7
//  clear grant fifo 7
#define EpnRstGntQQ7Sft                      7
#define EpnRstGntQQ7Msk                      0x00000080UL // [7:7]


#define EpnRstL1Q                            TkOnuReg(0x0409)

#define EpnRstRptPri                         TkOnuReg(0x040A)
//  Null REPORT values for Priority 0
#define EpnRstRptPriNull0                    0x00000001UL // offset 0
//  Null REPORT values for Priority 0
#define EpnRstRptPriNull0Sft                 0
#define EpnRstRptPriNull0Msk                 0x00000001UL // [0:0]
//  Null REPORT values for Priority 1
#define EpnRstRptPriNull1                    0x00000002UL // offset 1
//  Null REPORT values for Priority 1
#define EpnRstRptPriNull1Sft                 1
#define EpnRstRptPriNull1Msk                 0x00000002UL // [1:1]
//  Null REPORT values for Priority 2
#define EpnRstRptPriNull2                    0x00000004UL // offset 2
//  Null REPORT values for Priority 2
#define EpnRstRptPriNull2Sft                 2
#define EpnRstRptPriNull2Msk                 0x00000004UL // [2:2]
//  Null REPORT values for Priority 3
#define EpnRstRptPriNull3                    0x00000008UL // offset 3
//  Null REPORT values for Priority 3
#define EpnRstRptPriNull3Sft                 3
#define EpnRstRptPriNull3Msk                 0x00000008UL // [3:3]
//  Null REPORT values for Priority 4
#define EpnRstRptPriNull4                    0x00000010UL // offset 4
//  Null REPORT values for Priority 4
#define EpnRstRptPriNull4Sft                 4
#define EpnRstRptPriNull4Msk                 0x00000010UL // [4:4]
//  Null REPORT values for Priority 5
#define EpnRstRptPriNull5                    0x00000020UL // offset 5
//  Null REPORT values for Priority 5
#define EpnRstRptPriNull5Sft                 5
#define EpnRstRptPriNull5Msk                 0x00000020UL // [5:5]
//  Null REPORT values for Priority 6
#define EpnRstRptPriNull6                    0x00000040UL // offset 6
//  Null REPORT values for Priority 6
#define EpnRstRptPriNull6Sft                 6
#define EpnRstRptPriNull6Msk                 0x00000040UL // [6:6]
//  Null REPORT values for Priority 7
#define EpnRstRptPriNull7                    0x00000080UL // offset 7
//  Null REPORT values for Priority 7
#define EpnRstRptPriNull7Sft                 7
#define EpnRstRptPriNull7Msk                 0x00000080UL // [7:7]

#define EpnRstL2RptQ                         TkOnuReg(0x040B)
#define EpnRstL2RptQCfgsClrQSft              0
#define EpnRstL2RptQCfgsClrQMsk              0x00000007UL // [2:0]

#define EpnEnUpstream                        TkOnuReg(0x040C)
//  Per LLID upstream traffic enable
#define EpnEnUpstreamCfgSft                  0
#define EpnEnUpstreamCfgMsk                  0x000000FFUL // [7:0]

#define EpnEnUpstreamFb                      TkOnuReg(0x040D)
#define EpnEnUpstreamFbCfgFeedBackSft        0
#define EpnEnUpstreamFbCfgFeedBackMsk        0x000000FFUL // [7:0]

#define EpnEnUpstreamFec                     TkOnuReg(0x040E)
//  Per LLID upstream FEC enable
#define EpnEnUpstreamFecCfgSft               0
#define EpnEnUpstreamFecCfgMsk               0x000000FFUL // [7:0]

#define EpnRptByteLen                        TkOnuReg(0x040F)
//  Length of report frame in bytes
#define EpnRptByteLenPrvSft                  0
#define EpnRptByteLenPrvMsk                  0x000000FFUL // [7:0]

#define EpnMainInt                           TkOnuReg(0x0410)
#define EpnMainIntDpRdy                      0x00000001UL // offset 0
#define EpnMainIntDpRdySft                   0
#define EpnMainIntDpRdyMsk                   0x00000001UL // [0:0]
#define EpnMainIntDnTimeNotInSync            0x00000002UL // offset 1
#define EpnMainIntDnTimeNotInSyncSft         1
#define EpnMainIntDnTimeNotInSyncMsk         0x00000002UL // [1:1]
#define EpnMainIntDnTimeInSync               0x00000004UL // offset 2
#define EpnMainIntDnTimeInSyncSft            2
#define EpnMainIntDnTimeInSyncMsk            0x00000004UL // [2:2]

#define EpnMainIntBbhUpsFault                0x00000008UL // offset 3
#define EpnMainIntBbhUpsFaultSft             3
#define EpnMainIntBbhUpsFaultMsk             0x00000008UL // [3:3]

#define EpnMainIntUpInvldGntLen              0x00000010UL // offset 4
#define EpnMainIntUpInvldGntLenSft           4
#define EpnMainIntUpInvldGntLenMsk           0x00000010UL // [4:4]

#define EpnMainIntBbhUpsHalt                 0x00000020UL // offset 5
#define EpnMainIntBbhUpsHaltSft              5
#define EpnMainIntBbhUpsHaltMsk              0x00000020UL // [5:5]

#define EpnMainIntDnOutOfOrder               0x00000040UL // offset 6
#define EpnMainIntDnOutOfOrderSft            6
#define EpnMainIntDnOutOfOrderMsk            0x00000040UL // [6:6]
#define EpnMainIntUpStatsOverRun             0x00000080UL // offset 7
#define EpnMainIntUpStatsOverRunSft          7
#define EpnMainIntUpStatsOverRunMsk          0x00000080UL // [7:7]
#define EpnMainIntDnStatsOverRun             0x00000100UL // offset 8
#define EpnMainIntDnStatsOverRunSft          8
#define EpnMainIntDnStatsOverRunMsk          0x00000100UL // [8:8]
#define EpnMainIntRcvGntTooBig               0x00000200UL // offset 9
#define EpnMainIntRcvGntTooBigSft            9
#define EpnMainIntRcvGntTooBigMsk            0x00000200UL // [9:9]
#define EpnMainIntWrGntTooBig                0x00000400UL // offset 10
#define EpnMainIntWrGntTooBigSft             10
#define EpnMainIntWrGntTooBigMsk             0x00000400UL // [10:10]
#define EpnMainIntBurstGntTooBig             0x00000800UL // offset 11
#define EpnMainIntBurstGntTooBigSft          11
#define EpnMainIntBurstGntTooBigMsk          0x00000800UL // [11:11]
#define EpnMainIntBiQOverRun                 0x00001000UL // offset 12
#define EpnMainIntBiQOverRunSft              12
#define EpnMainIntBiQOverRunMsk              0x00001000UL // [12:12]
#define EpnMainIntUpRptFrXmt                 0x00002000UL // offset 13
#define EpnMainIntUpRptFrXmtSft              13
#define EpnMainIntUpRptFrXmtMsk              0x00002000UL // [13:13]
#define EpnMainIntBadUpFrLen                 0x00008000UL // offset 15
#define EpnMainIntBadUpFrLenSft              15
#define EpnMainIntBadUpFrLenMsk              0x00008000UL // [15:15]
#define EpnMainIntCoGntFullAbort             0x00010000UL // offset 16
#define EpnMainIntCoGntFullAbortSft          16
#define EpnMainIntCoGntFullAbortMsk          0x00010000UL // [16:16]
#define EpnMainIntCoGntMissAbort             0x00020000UL // offset 17
#define EpnMainIntCoGntMissAbortSft          17
#define EpnMainIntCoGntMissAbortMsk          0x00020000UL // [17:17]
#define EpnMainIntCoGntDisc                  0x00040000UL // offset 18
#define EpnMainIntCoGntDiscSft               18
#define EpnMainIntCoGntDiscMsk               0x00040000UL // [18:18]
#define EpnMainIntCoGntvl                    0x00080000UL // offset 19
#define EpnMainIntCoGntvlSft                 19
#define EpnMainIntCoGntvlMsk                 0x00080000UL // [19:19]
#define EpnMainIntCoGntTooFar                0x00100000UL // offset 20
#define EpnMainIntCoGntTooFarSft             20
#define EpnMainIntCoGntTooFarMsk             0x00100000UL // [20:20]
#define EpnMainIntCoGntMisalign              0x00200000UL // offset 21
#define EpnMainIntCoGntMisalignSft           21
#define EpnMainIntCoGntMisalignMsk           0x00200000UL // [21:21]
#define EpnMainIntCoGntNonPoll               0x00400000UL // offset 22
#define EpnMainIntCoGntNonPollSft            22
#define EpnMainIntCoGntNonPollMsk            0x00400000UL // [22:22]
#define EpnMainIntCoDelStaleGnt              0x00800000UL // offset 23
#define EpnMainIntCoDelStaleGntSft           23
#define EpnMainIntCoDelStaleGntMsk           0x00800000UL // [23:23]
#define EpnMainIntCoGntPres                  0x01000000UL // offset 24
#define EpnMainIntCoGntPresSft               24
#define EpnMainIntCoGntPresMsk               0x01000000UL // [24:24]
#define EpnMainIntCoRptPres                  0x02000000UL // offset 25
#define EpnMainIntCoRptPresSft               25
#define EpnMainIntCoRptPresMsk               0x02000000UL // [25:25]
#define EpnMainIntL1sQOverRun                0x04000000UL // offset 26
#define EpnMainIntL1sQOverRunSft             26
#define EpnMainIntL1sQOverRunMsk             0x04000000UL // [26:26]
#define EpnMainIntL2sQOverRun                0x08000000UL // offset 27
#define EpnMainIntL2sQOverRunSft             27
#define EpnMainIntL2sQOverRunMsk             0x08000000UL // [27:27]
#define EpnMainIntCoEmptyRpt                 0x20000000UL // offset 29
#define EpnMainIntCoEmptyRptSft              29
#define EpnMainIntCoEmptyRptMsk              0x20000000UL // [29:29]

#define EpnGntFullInt                        TkOnuReg(0x0411)
#define EpnGntFullIntDnAbort0                0x00000001UL // offset 0
#define EpnGntFullIntDnAbort0Sft             0
#define EpnGntFullIntDnAbort0Msk             0x00000001UL // [0:0]
#define EpnGntFullIntDnAbort1                0x00000002UL // offset 1
#define EpnGntFullIntDnAbort1Sft             1
#define EpnGntFullIntDnAbort1Msk             0x00000002UL // [1:1]
#define EpnGntFullIntDnAbort2                0x00000004UL // offset 2
#define EpnGntFullIntDnAbort2Sft             2
#define EpnGntFullIntDnAbort2Msk             0x00000004UL // [2:2]
#define EpnGntFullIntDnAbort3                0x00000008UL // offset 3
#define EpnGntFullIntDnAbort3Sft             3
#define EpnGntFullIntDnAbort3Msk             0x00000008UL // [3:3]
#define EpnGntFullIntDnAbort4                0x00000010UL // offset 4
#define EpnGntFullIntDnAbort4Sft             4
#define EpnGntFullIntDnAbort4Msk             0x00000010UL // [4:4]
#define EpnGntFullIntDnAbort5                0x00000020UL // offset 5
#define EpnGntFullIntDnAbort5Sft             5
#define EpnGntFullIntDnAbort5Msk             0x00000020UL // [5:5]
#define EpnGntFullIntDnAbort6                0x00000040UL // offset 6
#define EpnGntFullIntDnAbort6Sft             6
#define EpnGntFullIntDnAbort6Msk             0x00000040UL // [6:6]
#define EpnGntFullIntDnAbort7                0x00000080UL // offset 7
#define EpnGntFullIntDnAbort7Sft             7
#define EpnGntFullIntDnAbort7Msk             0x00000080UL // [7:7]
#define EpnGntFullIntDnAbort8                0x00000100UL // offset 8


#define EpnGntMissInt                        TkOnuReg(0x0412)
#define EpnGntMissIntDnAbort0                0x00000001UL // offset 0
#define EpnGntMissIntDnAbort0Sft             0
#define EpnGntMissIntDnAbort0Msk             0x00000001UL // [0:0]
#define EpnGntMissIntDnAbort1                0x00000002UL // offset 1
#define EpnGntMissIntDnAbort1Sft             1
#define EpnGntMissIntDnAbort1Msk             0x00000002UL // [1:1]
#define EpnGntMissIntDnAbort2                0x00000004UL // offset 2
#define EpnGntMissIntDnAbort2Sft             2
#define EpnGntMissIntDnAbort2Msk             0x00000004UL // [2:2]
#define EpnGntMissIntDnAbort3                0x00000008UL // offset 3
#define EpnGntMissIntDnAbort3Sft             3
#define EpnGntMissIntDnAbort3Msk             0x00000008UL // [3:3]
#define EpnGntMissIntDnAbort4                0x00000010UL // offset 4
#define EpnGntMissIntDnAbort4Sft             4
#define EpnGntMissIntDnAbort4Msk             0x00000010UL // [4:4]
#define EpnGntMissIntDnAbort5                0x00000020UL // offset 5
#define EpnGntMissIntDnAbort5Sft             5
#define EpnGntMissIntDnAbort5Msk             0x00000020UL // [5:5]
#define EpnGntMissIntDnAbort6                0x00000040UL // offset 6
#define EpnGntMissIntDnAbort6Sft             6
#define EpnGntMissIntDnAbort6Msk             0x00000040UL // [6:6]
#define EpnGntMissIntDnAbort7                0x00000080UL // offset 7
#define EpnGntMissIntDnAbort7Sft             7
#define EpnGntMissIntDnAbort7Msk             0x00000080UL // [7:7]


#define EpnDiscRxInt                         TkOnuReg(0x0413)
#define EpnDiscRxIntDnGnt0                   0x00000001UL // offset 0
#define EpnDiscRxIntDnGnt0Sft                0
#define EpnDiscRxIntDnGnt0Msk                0x00000001UL // [0:0]
#define EpnDiscRxIntDnGnt1                   0x00000002UL // offset 1
#define EpnDiscRxIntDnGnt1Sft                1
#define EpnDiscRxIntDnGnt1Msk                0x00000002UL // [1:1]
#define EpnDiscRxIntDnGnt2                   0x00000004UL // offset 2
#define EpnDiscRxIntDnGnt2Sft                2
#define EpnDiscRxIntDnGnt2Msk                0x00000004UL // [2:2]
#define EpnDiscRxIntDnGnt3                   0x00000008UL // offset 3
#define EpnDiscRxIntDnGnt3Sft                3
#define EpnDiscRxIntDnGnt3Msk                0x00000008UL // [3:3]
#define EpnDiscRxIntDnGnt4                   0x00000010UL // offset 4
#define EpnDiscRxIntDnGnt4Sft                4
#define EpnDiscRxIntDnGnt4Msk                0x00000010UL // [4:4]
#define EpnDiscRxIntDnGnt5                   0x00000020UL // offset 5
#define EpnDiscRxIntDnGnt5Sft                5
#define EpnDiscRxIntDnGnt5Msk                0x00000020UL // [5:5]
#define EpnDiscRxIntDnGnt6                   0x00000040UL // offset 6
#define EpnDiscRxIntDnGnt6Sft                6
#define EpnDiscRxIntDnGnt6Msk                0x00000040UL // [6:6]
#define EpnDiscRxIntDnGnt7                   0x00000080UL // offset 7
#define EpnDiscRxIntDnGnt7Sft                7
#define EpnDiscRxIntDnGnt7Msk                0x00000080UL // [7:7]
#define EpnDiscRxIntDnGnt8                   0x00000100UL // offset 8


#define EpnGntIntvInt                        TkOnuReg(0x0414)
#define EpnGntIntvIntDnl0                    0x00000001UL // offset 0
#define EpnGntIntvIntDnl0Sft                 0
#define EpnGntIntvIntDnl0Msk                 0x00000001UL // [0:0]
#define EpnGntIntvIntDnl1                    0x00000002UL // offset 1
#define EpnGntIntvIntDnl1Sft                 1
#define EpnGntIntvIntDnl1Msk                 0x00000002UL // [1:1]
#define EpnGntIntvIntDnl2                    0x00000004UL // offset 2
#define EpnGntIntvIntDnl2Sft                 2
#define EpnGntIntvIntDnl2Msk                 0x00000004UL // [2:2]
#define EpnGntIntvIntDnl3                    0x00000008UL // offset 3
#define EpnGntIntvIntDnl3Sft                 3
#define EpnGntIntvIntDnl3Msk                 0x00000008UL // [3:3]
#define EpnGntIntvIntDnl4                    0x00000010UL // offset 4
#define EpnGntIntvIntDnl4Sft                 4
#define EpnGntIntvIntDnl4Msk                 0x00000010UL // [4:4]
#define EpnGntIntvIntDnl5                    0x00000020UL // offset 5
#define EpnGntIntvIntDnl5Sft                 5
#define EpnGntIntvIntDnl5Msk                 0x00000020UL // [5:5]
#define EpnGntIntvIntDnl6                    0x00000040UL // offset 6
#define EpnGntIntvIntDnl6Sft                 6
#define EpnGntIntvIntDnl6Msk                 0x00000040UL // [6:6]
#define EpnGntIntvIntDnl7                    0x00000080UL // offset 7
#define EpnGntIntvIntDnl7Sft                 7
#define EpnGntIntvIntDnl7Msk                 0x00000080UL // [7:7]
#define EpnGntIntvIntDnl8                    0x00000100UL // offset 8


#define EpnGntFarInt                         TkOnuReg(0x0415)
#define EpnGntFarIntDnToo0                   0x00000001UL // offset 0
#define EpnGntFarIntDnToo0Sft                0
#define EpnGntFarIntDnToo0Msk                0x00000001UL // [0:0]
#define EpnGntFarIntDnToo1                   0x00000002UL // offset 1
#define EpnGntFarIntDnToo1Sft                1
#define EpnGntFarIntDnToo1Msk                0x00000002UL // [1:1]
#define EpnGntFarIntDnToo2                   0x00000004UL // offset 2
#define EpnGntFarIntDnToo2Sft                2
#define EpnGntFarIntDnToo2Msk                0x00000004UL // [2:2]
#define EpnGntFarIntDnToo3                   0x00000008UL // offset 3
#define EpnGntFarIntDnToo3Sft                3
#define EpnGntFarIntDnToo3Msk                0x00000008UL // [3:3]
#define EpnGntFarIntDnToo4                   0x00000010UL // offset 4
#define EpnGntFarIntDnToo4Sft                4
#define EpnGntFarIntDnToo4Msk                0x00000010UL // [4:4]
#define EpnGntFarIntDnToo5                   0x00000020UL // offset 5
#define EpnGntFarIntDnToo5Sft                5
#define EpnGntFarIntDnToo5Msk                0x00000020UL // [5:5]
#define EpnGntFarIntDnToo6                   0x00000040UL // offset 6
#define EpnGntFarIntDnToo6Sft                6
#define EpnGntFarIntDnToo6Msk                0x00000040UL // [6:6]
#define EpnGntFarIntDnToo7                   0x00000080UL // offset 7
#define EpnGntFarIntDnToo7Sft                7
#define EpnGntFarIntDnToo7Msk                0x00000080UL // [7:7]


#define EpnGntMisalignInt                    TkOnuReg(0x0416)
#define EpnGntMisalignIntDn0                 0x00000001UL // offset 0
#define EpnGntMisalignIntDn0Sft              0
#define EpnGntMisalignIntDn0Msk              0x00000001UL // [0:0]
#define EpnGntMisalignIntDn1                 0x00000002UL // offset 1
#define EpnGntMisalignIntDn1Sft              1
#define EpnGntMisalignIntDn1Msk              0x00000002UL // [1:1]
#define EpnGntMisalignIntDn2                 0x00000004UL // offset 2
#define EpnGntMisalignIntDn2Sft              2
#define EpnGntMisalignIntDn2Msk              0x00000004UL // [2:2]
#define EpnGntMisalignIntDn3                 0x00000008UL // offset 3
#define EpnGntMisalignIntDn3Sft              3
#define EpnGntMisalignIntDn3Msk              0x00000008UL // [3:3]
#define EpnGntMisalignIntDn4                 0x00000010UL // offset 4
#define EpnGntMisalignIntDn4Sft              4
#define EpnGntMisalignIntDn4Msk              0x00000010UL // [4:4]
#define EpnGntMisalignIntDn5                 0x00000020UL // offset 5
#define EpnGntMisalignIntDn5Sft              5
#define EpnGntMisalignIntDn5Msk              0x00000020UL // [5:5]
#define EpnGntMisalignIntDn6                 0x00000040UL // offset 6
#define EpnGntMisalignIntDn6Sft              6
#define EpnGntMisalignIntDn6Msk              0x00000040UL // [6:6]
#define EpnGntMisalignIntDn7                 0x00000080UL // offset 7
#define EpnGntMisalignIntDn7Sft              7
#define EpnGntMisalignIntDn7Msk              0x00000080UL // [7:7]



#define EpnNpGntInt                          TkOnuReg(0x0417)
#define EpnNpGntIntDnNonPoll0                0x00000001UL // offset 0
#define EpnNpGntIntDnNonPoll0Sft             0
#define EpnNpGntIntDnNonPoll0Msk             0x00000001UL // [0:0]
#define EpnNpGntIntDnNonPoll1                0x00000002UL // offset 1
#define EpnNpGntIntDnNonPoll1Sft             1
#define EpnNpGntIntDnNonPoll1Msk             0x00000002UL // [1:1]
#define EpnNpGntIntDnNonPoll2                0x00000004UL // offset 2
#define EpnNpGntIntDnNonPoll2Sft             2
#define EpnNpGntIntDnNonPoll2Msk             0x00000004UL // [2:2]
#define EpnNpGntIntDnNonPoll3                0x00000008UL // offset 3
#define EpnNpGntIntDnNonPoll3Sft             3
#define EpnNpGntIntDnNonPoll3Msk             0x00000008UL // [3:3]
#define EpnNpGntIntDnNonPoll4                0x00000010UL // offset 4
#define EpnNpGntIntDnNonPoll4Sft             4
#define EpnNpGntIntDnNonPoll4Msk             0x00000010UL // [4:4]
#define EpnNpGntIntDnNonPoll5                0x00000020UL // offset 5
#define EpnNpGntIntDnNonPoll5Sft             5
#define EpnNpGntIntDnNonPoll5Msk             0x00000020UL // [5:5]
#define EpnNpGntIntDnNonPoll6                0x00000040UL // offset 6
#define EpnNpGntIntDnNonPoll6Sft             6
#define EpnNpGntIntDnNonPoll6Msk             0x00000040UL // [6:6]
#define EpnNpGntIntDnNonPoll7                0x00000080UL // offset 7
#define EpnNpGntIntDnNonPoll7Sft             7
#define EpnNpGntIntDnNonPoll7Msk             0x00000080UL // [7:7]


#define EpnDelStaleInt                       TkOnuReg(0x0418)
#define EpnDelStaleIntGnt0                   0x00000001UL // offset 0
#define EpnDelStaleIntGnt0Sft                0
#define EpnDelStaleIntGnt0Msk                0x00000001UL // [0:0]
#define EpnDelStaleIntGnt1                   0x00000002UL // offset 1
#define EpnDelStaleIntGnt1Sft                1
#define EpnDelStaleIntGnt1Msk                0x00000002UL // [1:1]
#define EpnDelStaleIntGnt2                   0x00000004UL // offset 2
#define EpnDelStaleIntGnt2Sft                2
#define EpnDelStaleIntGnt2Msk                0x00000004UL // [2:2]
#define EpnDelStaleIntGnt3                   0x00000008UL // offset 3
#define EpnDelStaleIntGnt3Sft                3
#define EpnDelStaleIntGnt3Msk                0x00000008UL // [3:3]
#define EpnDelStaleIntGnt4                   0x00000010UL // offset 4
#define EpnDelStaleIntGnt4Sft                4
#define EpnDelStaleIntGnt4Msk                0x00000010UL // [4:4]
#define EpnDelStaleIntGnt5                   0x00000020UL // offset 5
#define EpnDelStaleIntGnt5Sft                5
#define EpnDelStaleIntGnt5Msk                0x00000020UL // [5:5]
#define EpnDelStaleIntGnt6                   0x00000040UL // offset 6
#define EpnDelStaleIntGnt6Sft                6
#define EpnDelStaleIntGnt6Msk                0x00000040UL // [6:6]
#define EpnDelStaleIntGnt7                   0x00000080UL // offset 7
#define EpnDelStaleIntGnt7Sft                7
#define EpnDelStaleIntGnt7Msk                0x00000080UL // [7:7]


#define EpnGntPresInt                        TkOnuReg(0x0419)
#define EpnGntPresIntDnRdy0                  0x00000001UL // offset 0
#define EpnGntPresIntDnRdy0Sft               0
#define EpnGntPresIntDnRdy0Msk               0x00000001UL // [0:0]
#define EpnGntPresIntDnRdy1                  0x00000002UL // offset 1
#define EpnGntPresIntDnRdy1Sft               1
#define EpnGntPresIntDnRdy1Msk               0x00000002UL // [1:1]
#define EpnGntPresIntDnRdy2                  0x00000004UL // offset 2
#define EpnGntPresIntDnRdy2Sft               2
#define EpnGntPresIntDnRdy2Msk               0x00000004UL // [2:2]
#define EpnGntPresIntDnRdy3                  0x00000008UL // offset 3
#define EpnGntPresIntDnRdy3Sft               3
#define EpnGntPresIntDnRdy3Msk               0x00000008UL // [3:3]
#define EpnGntPresIntDnRdy4                  0x00000010UL // offset 4
#define EpnGntPresIntDnRdy4Sft               4
#define EpnGntPresIntDnRdy4Msk               0x00000010UL // [4:4]
#define EpnGntPresIntDnRdy5                  0x00000020UL // offset 5
#define EpnGntPresIntDnRdy5Sft               5
#define EpnGntPresIntDnRdy5Msk               0x00000020UL // [5:5]
#define EpnGntPresIntDnRdy6                  0x00000040UL // offset 6
#define EpnGntPresIntDnRdy6Sft               6
#define EpnGntPresIntDnRdy6Msk               0x00000040UL // [6:6]
#define EpnGntPresIntDnRdy7                  0x00000080UL // offset 7
#define EpnGntPresIntDnRdy7Sft               7
#define EpnGntPresIntDnRdy7Msk               0x00000080UL // [7:7]


#define EpnRptPresInt                        TkOnuReg(0x041A)
#define EpnRptPresIntUpQ0                    0x00000001UL // offset 0
#define EpnRptPresIntUpQ0Sft                 0
#define EpnRptPresIntUpQ0Msk                 0x00000001UL // [0:0]
#define EpnRptPresIntUpQ1                    0x00000002UL // offset 1
#define EpnRptPresIntUpQ1Sft                 1
#define EpnRptPresIntUpQ1Msk                 0x00000002UL // [1:1]
#define EpnRptPresIntUpQ2                    0x00000004UL // offset 2
#define EpnRptPresIntUpQ2Sft                 2
#define EpnRptPresIntUpQ2Msk                 0x00000004UL // [2:2]
#define EpnRptPresIntUpQ3                    0x00000008UL // offset 3
#define EpnRptPresIntUpQ3Sft                 3
#define EpnRptPresIntUpQ3Msk                 0x00000008UL // [3:3]
#define EpnRptPresIntUpQ4                    0x00000010UL // offset 4
#define EpnRptPresIntUpQ4Sft                 4
#define EpnRptPresIntUpQ4Msk                 0x00000010UL // [4:4]
#define EpnRptPresIntUpQ5                    0x00000020UL // offset 5
#define EpnRptPresIntUpQ5Sft                 5
#define EpnRptPresIntUpQ5Msk                 0x00000020UL // [5:5]
#define EpnRptPresIntUpQ6                    0x00000040UL // offset 6
#define EpnRptPresIntUpQ6Sft                 6
#define EpnRptPresIntUpQ6Msk                 0x00000040UL // [6:6]
#define EpnRptPresIntUpQ7                    0x00000080UL // offset 7
#define EpnRptPresIntUpQ7Sft                 7
#define EpnRptPresIntUpQ7Msk                 0x00000080UL // [7:7]


#define EpnDrxAbortInt                       TkOnuReg(0x041B)

#define EpnEmptyRptInt                       TkOnuReg(0x041C)
#define EpnEmptyRptInt0                      0x00000001UL // offset 0
#define EpnEmptyRptInt0Sft                   0
#define EpnEmptyRptInt0Msk                   0x00000001UL // [0:0]
#define EpnEmptyRptInt1                      0x00000002UL // offset 1
#define EpnEmptyRptInt1Sft                   1
#define EpnEmptyRptInt1Msk                   0x00000002UL // [1:1]
#define EpnEmptyRptInt2                      0x00000004UL // offset 2
#define EpnEmptyRptInt2Sft                   2
#define EpnEmptyRptInt2Msk                   0x00000004UL // [2:2]
#define EpnEmptyRptInt3                      0x00000008UL // offset 3
#define EpnEmptyRptInt3Sft                   3
#define EpnEmptyRptInt3Msk                   0x00000008UL // [3:3]
#define EpnEmptyRptInt4                      0x00000010UL // offset 4
#define EpnEmptyRptInt4Sft                   4
#define EpnEmptyRptInt4Msk                   0x00000010UL // [4:4]
#define EpnEmptyRptInt5                      0x00000020UL // offset 5
#define EpnEmptyRptInt5Sft                   5
#define EpnEmptyRptInt5Msk                   0x00000020UL // [5:5]
#define EpnEmptyRptInt6                      0x00000040UL // offset 6
#define EpnEmptyRptInt6Sft                   6
#define EpnEmptyRptInt6Msk                   0x00000040UL // [6:6]
#define EpnEmptyRptInt7                      0x00000080UL // offset 7
#define EpnEmptyRptInt7Sft                   7
#define EpnEmptyRptInt7Msk                   0x00000080UL // [7:7]

#define EpnOverFlowBurstCapInt               TkOnuReg(0x041d)
#define EpnBBHUpsFaultInt                    TkOnuReg(0x041e)
#define EpnIntBbhUpsFault                    0x000000ffUL // offset 


#define EpnMainMsk                           TkOnuReg(0x0420)
#define EpnMainMskDpRdy                      0x00000001UL // offset 0
#define EpnMainMskDpRdySft                   0
#define EpnMainMskDpRdyMsk                   0x00000001UL // [0:0]
#define EpnMainMskDnTimeNotInSync            0x00000002UL // offset 1
#define EpnMainMskDnTimeNotInSyncSft         1
#define EpnMainMskDnTimeNotInSyncMsk         0x00000002UL // [1:1]
#define EpnMainMskDnTimeInSync               0x00000004UL // offset 2
#define EpnMainMskDnTimeInSyncSft            2
#define EpnMainMskDnTimeInSyncMsk            0x00000004UL // [2:2]
#define EpnMainMskUpInvldGntLen              0x00000010UL // offset 4
#define EpnMainMskUpInvldGntLenSft           4
#define EpnMainMskUpInvldGntLenMsk           0x00000010UL // [4:4]
#define EpnMainMskDnOutOfOrder               0x00000040UL // offset 6
#define EpnMainMskDnOutOfOrderSft            6
#define EpnMainMskDnOutOfOrderMsk            0x00000040UL // [6:6]
#define EpnMainMskDnStatsOverRun             0x00000100UL // offset 8
#define EpnMainMskDnStatsOverRunSft          8
#define EpnMainMskDnStatsOverRunMsk          0x00000100UL // [8:8]
#define EpnMainMskRcvGntTooBig               0x00000200UL // offset 9
#define EpnMainMskRcvGntTooBigSft            9
#define EpnMainMskRcvGntTooBigMsk            0x00000200UL // [9:9]
#define EpnMainMskWrGntTooBig                0x00000400UL // offset 10
#define EpnMainMskWrGntTooBigSft             10
#define EpnMainMskWrGntTooBigMsk             0x00000400UL // [10:10]
#define EpnMainMskBurstGntTooBig             0x00000800UL // offset 11
#define EpnMainMskBurstGntTooBigSft          11
#define EpnMainMskBurstGntTooBigMsk          0x00000800UL // [11:11]
#define EpnMainMskUpRptFrXmt                 0x00002000UL // offset 13
#define EpnMainMskUpRptFrXmtSft              13
#define EpnMainMskUpRptFrXmtMsk              0x00002000UL // [13:13]
#define EpnMainMskIntCoGntFullAbort          0x00010000UL // offset 16
#define EpnMainMskIntCoGntFullAbortSft       16
#define EpnMainMskIntCoGntFullAbortMsk       0x00010000UL // [16:16]
#define EpnMainMskIntCoGntMissAbort          0x00020000UL // offset 17
#define EpnMainMskIntCoGntMissAbortSft       17
#define EpnMainMskIntCoGntMissAbortMsk       0x00020000UL // [17:17]
#define EpnMainMskIntCoGntDisc               0x00040000UL // offset 18
#define EpnMainMskIntCoGntDiscSft            18
#define EpnMainMskIntCoGntDiscMsk            0x00040000UL // [18:18]
#define EpnMainMskIntCoGntvl                 0x00080000UL // offset 19
#define EpnMainMskIntCoGntvlSft              19
#define EpnMainMskIntCoGntvlMsk              0x00080000UL // [19:19]
#define EpnMainMskIntCoGntTooFar             0x00100000UL // offset 20
#define EpnMainMskIntCoGntTooFarSft          20
#define EpnMainMskIntCoGntTooFarMsk          0x00100000UL // [20:20]
#define EpnMainMskIntCoGntMisalign           0x00200000UL // offset 21
#define EpnMainMskIntCoGntMisalignSft        21
#define EpnMainMskIntCoGntMisalignMsk        0x00200000UL // [21:21]
#define EpnMainMskIntCoGntNonPoll            0x00400000UL // offset 22
#define EpnMainMskIntCoGntNonPollSft         22
#define EpnMainMskIntCoGntNonPollMsk         0x00400000UL // [22:22]
#define EpnMainMskIntCoDelStaleGnt           0x00800000UL // offset 23
#define EpnMainMskIntCoDelStaleGntSft        23
#define EpnMainMskIntCoDelStaleGntMsk        0x00800000UL // [23:23]
#define EpnMainMskIntCoGntPres               0x01000000UL // offset 24
#define EpnMainMskIntCoGntPresSft            24
#define EpnMainMskIntCoGntPresMsk            0x01000000UL // [24:24]
#define EpnMainMskIntCoRptPres               0x02000000UL // offset 25
#define EpnMainMskIntCoRptPresSft            25
#define EpnMainMskIntCoRptPresMsk            0x02000000UL // [25:25]
#define EpnMainMskIntL1sQOverRun             0x04000000UL // offset 26
#define EpnMainMskIntL1sQOverRunSft          26
#define EpnMainMskIntL1sQOverRunMsk          0x04000000UL // [26:26]
#define EpnMainMskIntL2sQOverRun             0x08000000UL // offset 27
#define EpnMainMskIntL2sQOverRunSft          27
#define EpnMainMskIntL2sQOverRunMsk          0x08000000UL // [27:27]
#define EpnMainMskIntDrxErrAbort             0x10000000UL // offset 28
#define EpnMainMskIntDrxErrAbortSft          28
#define EpnMainMskIntDrxErrAbortMsk          0x10000000UL // [28:28]
#define EpnMainMskIntCoEmptyRpt              0x20000000UL // offset 29
#define EpnMainMskIntCoEmptyRptSft           29
#define EpnMainMskIntCoEmptyRptMsk           0x20000000UL // [29:29]

#define EpnMainMskIntL2BCOverf               0x40000000UL // offset 30
#define EpnMainMskIntL2BCOverfSft            30
#define EpnMainMskIntL2BCOverfMsk            0x40000000UL // [30:30]

#define EpnMainMskIntBBHUpFr                 0x80000000UL // offset 31
#define EpnMainMskIntBBHUpFrtSft             31
#define EpnMainMskIntBBHUpFrtMsk             0x80000000UL // [31:31]



#define EpnGntFullMsk                        TkOnuReg(0x0421)
#define EpnGntFullMskDnAbort0                0x00000001UL // offset 0
#define EpnGntFullMskDnAbort0Sft             0
#define EpnGntFullMskDnAbort0Msk             0x00000001UL // [0:0]
#define EpnGntFullMskDnAbort1                0x00000002UL // offset 1
#define EpnGntFullMskDnAbort1Sft             1
#define EpnGntFullMskDnAbort1Msk             0x00000002UL // [1:1]
#define EpnGntFullMskDnAbort2                0x00000004UL // offset 2
#define EpnGntFullMskDnAbort2Sft             2
#define EpnGntFullMskDnAbort2Msk             0x00000004UL // [2:2]
#define EpnGntFullMskDnAbort3                0x00000008UL // offset 3
#define EpnGntFullMskDnAbort3Sft             3
#define EpnGntFullMskDnAbort3Msk             0x00000008UL // [3:3]
#define EpnGntFullMskDnAbort4                0x00000010UL // offset 4
#define EpnGntFullMskDnAbort4Sft             4
#define EpnGntFullMskDnAbort4Msk             0x00000010UL // [4:4]
#define EpnGntFullMskDnAbort5                0x00000020UL // offset 5
#define EpnGntFullMskDnAbort5Sft             5
#define EpnGntFullMskDnAbort5Msk             0x00000020UL // [5:5]
#define EpnGntFullMskDnAbort6                0x00000040UL // offset 6
#define EpnGntFullMskDnAbort6Sft             6
#define EpnGntFullMskDnAbort6Msk             0x00000040UL // [6:6]
#define EpnGntFullMskDnAbort7                0x00000080UL // offset 7
#define EpnGntFullMskDnAbort7Sft             7
#define EpnGntFullMskDnAbort7Msk             0x00000080UL // [7:7]


#define EpnGntMissMsk                        TkOnuReg(0x0422)
#define EpnGntMissMskDnAbort0                0x00000001UL // offset 0
#define EpnGntMissMskDnAbort0Sft             0
#define EpnGntMissMskDnAbort0Msk             0x00000001UL // [0:0]
#define EpnGntMissMskDnAbort1                0x00000002UL // offset 1
#define EpnGntMissMskDnAbort1Sft             1
#define EpnGntMissMskDnAbort1Msk             0x00000002UL // [1:1]
#define EpnGntMissMskDnAbort2                0x00000004UL // offset 2
#define EpnGntMissMskDnAbort2Sft             2
#define EpnGntMissMskDnAbort2Msk             0x00000004UL // [2:2]
#define EpnGntMissMskDnAbort3                0x00000008UL // offset 3
#define EpnGntMissMskDnAbort3Sft             3
#define EpnGntMissMskDnAbort3Msk             0x00000008UL // [3:3]
#define EpnGntMissMskDnAbort4                0x00000010UL // offset 4
#define EpnGntMissMskDnAbort4Sft             4
#define EpnGntMissMskDnAbort4Msk             0x00000010UL // [4:4]
#define EpnGntMissMskDnAbort5                0x00000020UL // offset 5
#define EpnGntMissMskDnAbort5Sft             5
#define EpnGntMissMskDnAbort5Msk             0x00000020UL // [5:5]
#define EpnGntMissMskDnAbort6                0x00000040UL // offset 6
#define EpnGntMissMskDnAbort6Sft             6
#define EpnGntMissMskDnAbort6Msk             0x00000040UL // [6:6]
#define EpnGntMissMskDnAbort7                0x00000080UL // offset 7
#define EpnGntMissMskDnAbort7Sft             7
#define EpnGntMissMskDnAbort7Msk             0x00000080UL // [7:7]


#define EpnDiscRxMsk                         TkOnuReg(0x0423)
#define EpnDiscRxMskDnGnt0                   0x00000001UL // offset 0
#define EpnDiscRxMskDnGnt0Sft                0
#define EpnDiscRxMskDnGnt0Msk                0x00000001UL // [0:0]
#define EpnDiscRxMskDnGnt1                   0x00000002UL // offset 1
#define EpnDiscRxMskDnGnt1Sft                1
#define EpnDiscRxMskDnGnt1Msk                0x00000002UL // [1:1]
#define EpnDiscRxMskDnGnt2                   0x00000004UL // offset 2
#define EpnDiscRxMskDnGnt2Sft                2
#define EpnDiscRxMskDnGnt2Msk                0x00000004UL // [2:2]
#define EpnDiscRxMskDnGnt3                   0x00000008UL // offset 3
#define EpnDiscRxMskDnGnt3Sft                3
#define EpnDiscRxMskDnGnt3Msk                0x00000008UL // [3:3]
#define EpnDiscRxMskDnGnt4                   0x00000008UL // offset 3
#define EpnDiscRxMskDnGnt4Sft                3
#define EpnDiscRxMskDnGnt4Msk                0x00000008UL // [3:3]
#define EpnDiscRxMskDnGnt5                   0x00000020UL // offset 5
#define EpnDiscRxMskDnGnt5Sft                5
#define EpnDiscRxMskDnGnt5Msk                0x00000020UL // [5:5]
#define EpnDiscRxMskDnGnt6                   0x00000040UL // offset 6
#define EpnDiscRxMskDnGnt6Sft                6
#define EpnDiscRxMskDnGnt6Msk                0x00000040UL // [6:6]
#define EpnDiscRxMskDnGnt7                   0x00000080UL // offset 7
#define EpnDiscRxMskDnGnt7Sft                7
#define EpnDiscRxMskDnGnt7Msk                0x00000080UL // [7:7]


#define EpnGntIntvMsk                        TkOnuReg(0x0424)
#define EpnGntIntvMskDnl0                    0x00000001UL // offset 0
#define EpnGntIntvMskDnl0Sft                 0
#define EpnGntIntvMskDnl0Msk                 0x00000001UL // [0:0]
#define EpnGntIntvMskDnl1                    0x00000002UL // offset 1
#define EpnGntIntvMskDnl1Sft                 1
#define EpnGntIntvMskDnl1Msk                 0x00000002UL // [1:1]
#define EpnGntIntvMskDnl2                    0x00000004UL // offset 2
#define EpnGntIntvMskDnl2Sft                 2
#define EpnGntIntvMskDnl2Msk                 0x00000004UL // [2:2]
#define EpnGntIntvMskDnl3                    0x00000008UL // offset 3
#define EpnGntIntvMskDnl3Sft                 3
#define EpnGntIntvMskDnl3Msk                 0x00000008UL // [3:3]
#define EpnGntIntvMskDnl4                    0x00000010UL // offset 4
#define EpnGntIntvMskDnl4Sft                 4
#define EpnGntIntvMskDnl4Msk                 0x00000010UL // [4:4]
#define EpnGntIntvMskDnl5                    0x00000020UL // offset 5
#define EpnGntIntvMskDnl5Sft                 5
#define EpnGntIntvMskDnl5Msk                 0x00000020UL // [5:5]
#define EpnGntIntvMskDnl6                    0x00000040UL // offset 6
#define EpnGntIntvMskDnl6Sft                 6
#define EpnGntIntvMskDnl6Msk                 0x00000040UL // [6:6]
#define EpnGntIntvMskDnl7                    0x00000080UL // offset 7
#define EpnGntIntvMskDnl7Sft                 7
#define EpnGntIntvMskDnl7Msk                 0x00000080UL // [7:7]


#define EpnGntFarMsk                         TkOnuReg(0x0425)
#define EpnGntFarMskDnToo0                   0x00000001UL // offset 0
#define EpnGntFarMskDnToo0Sft                0
#define EpnGntFarMskDnToo0Msk                0x00000001UL // [0:0]
#define EpnGntFarMskDnToo1                   0x00000002UL // offset 1
#define EpnGntFarMskDnToo1Sft                1
#define EpnGntFarMskDnToo1Msk                0x00000002UL // [1:1]
#define EpnGntFarMskDnToo2                   0x00000004UL // offset 2
#define EpnGntFarMskDnToo2Sft                2
#define EpnGntFarMskDnToo2Msk                0x00000004UL // [2:2]
#define EpnGntFarMskDnToo3                   0x00000008UL // offset 3
#define EpnGntFarMskDnToo3Sft                3
#define EpnGntFarMskDnToo3Msk                0x00000008UL // [3:3]
#define EpnGntFarMskDnToo4                   0x00000010UL // offset 4
#define EpnGntFarMskDnToo4Sft                4
#define EpnGntFarMskDnToo4Msk                0x00000010UL // [4:4]
#define EpnGntFarMskDnToo5                   0x00000020UL // offset 5
#define EpnGntFarMskDnToo5Sft                5
#define EpnGntFarMskDnToo5Msk                0x00000020UL // [5:5]
#define EpnGntFarMskDnToo6                   0x00000040UL // offset 6
#define EpnGntFarMskDnToo6Sft                6
#define EpnGntFarMskDnToo6Msk                0x00000040UL // [6:6]
#define EpnGntFarMskDnToo7                   0x00000080UL // offset 7
#define EpnGntFarMskDnToo7Sft                7
#define EpnGntFarMskDnToo7Msk                0x00000080UL // [7:7]


#define EpnGntMisalignMsk                    TkOnuReg(0x0426)
#define EpnGntMisalignMskDn0                 0x00000001UL // offset 0
#define EpnGntMisalignMskDn0Sft              0
#define EpnGntMisalignMskDn0Msk              0x00000001UL // [0:0]
#define EpnGntMisalignMskDn1                 0x00000002UL // offset 1
#define EpnGntMisalignMskDn1Sft              1
#define EpnGntMisalignMskDn1Msk              0x00000002UL // [1:1]
#define EpnGntMisalignMskDn2                 0x00000004UL // offset 2
#define EpnGntMisalignMskDn2Sft              2
#define EpnGntMisalignMskDn2Msk              0x00000004UL // [2:2]
#define EpnGntMisalignMskDn3                 0x00000008UL // offset 3
#define EpnGntMisalignMskDn3Sft              3
#define EpnGntMisalignMskDn3Msk              0x00000008UL // [3:3]
#define EpnGntMisalignMskDn4                 0x00000010UL // offset 4
#define EpnGntMisalignMskDn4Sft              4
#define EpnGntMisalignMskDn4Msk              0x00000010UL // [4:4]
#define EpnGntMisalignMskDn5                 0x00000020UL // offset 5
#define EpnGntMisalignMskDn5Sft              5
#define EpnGntMisalignMskDn5Msk              0x00000020UL // [5:5]
#define EpnGntMisalignMskDn6                 0x00000040UL // offset 6
#define EpnGntMisalignMskDn6Sft              6
#define EpnGntMisalignMskDn6Msk              0x00000040UL // [6:6]
#define EpnGntMisalignMskDn7                 0x00000080UL // offset 7
#define EpnGntMisalignMskDn7Sft              7
#define EpnGntMisalignMskDn7Msk              0x00000080UL // [7:7]


#define EpnNpGntMsk                          TkOnuReg(0x0427)
#define EpnNpGntMskDnNonPoll0                0x00000001UL // offset 0
#define EpnNpGntMskDnNonPoll0Sft             0
#define EpnNpGntMskDnNonPoll0Msk             0x00000001UL // [0:0]
#define EpnNpGntMskDnNonPoll1                0x00000002UL // offset 1
#define EpnNpGntMskDnNonPoll1Sft             1
#define EpnNpGntMskDnNonPoll1Msk             0x00000002UL // [1:1]
#define EpnNpGntMskDnNonPoll2                0x00000004UL // offset 2
#define EpnNpGntMskDnNonPoll2Sft             2
#define EpnNpGntMskDnNonPoll2Msk             0x00000004UL // [2:2]
#define EpnNpGntMskDnNonPoll3                0x00000008UL // offset 3
#define EpnNpGntMskDnNonPoll3Sft             3
#define EpnNpGntMskDnNonPoll3Msk             0x00000008UL // [3:3]
#define EpnNpGntMskDnNonPoll4                0x00000010UL // offset 4
#define EpnNpGntMskDnNonPoll4Sft             4
#define EpnNpGntMskDnNonPoll4Msk             0x00000010UL // [4:4]
#define EpnNpGntMskDnNonPoll5                0x00000020UL // offset 5
#define EpnNpGntMskDnNonPoll5Sft             5
#define EpnNpGntMskDnNonPoll5Msk             0x00000020UL // [5:5]
#define EpnNpGntMskDnNonPoll6                0x00000040UL // offset 6
#define EpnNpGntMskDnNonPoll6Sft             6
#define EpnNpGntMskDnNonPoll6Msk             0x00000040UL // [6:6]
#define EpnNpGntMskDnNonPoll7                0x00000080UL // offset 7
#define EpnNpGntMskDnNonPoll7Sft             7
#define EpnNpGntMskDnNonPoll7Msk             0x00000080UL // [7:7]


#define EpnDelStaleMsk                       TkOnuReg(0x0428)
#define EpnDelStaleMskGnt0                   0x00000001UL // offset 0
#define EpnDelStaleMskGnt0Sft                0
#define EpnDelStaleMskGnt0Msk                0x00000001UL // [0:0]
#define EpnDelStaleMskGnt1                   0x00000002UL // offset 1
#define EpnDelStaleMskGnt1Sft                1
#define EpnDelStaleMskGnt1Msk                0x00000002UL // [1:1]
#define EpnDelStaleMskGnt2                   0x00000004UL // offset 2
#define EpnDelStaleMskGnt2Sft                2
#define EpnDelStaleMskGnt2Msk                0x00000004UL // [2:2]
#define EpnDelStaleMskGnt3                   0x00000008UL // offset 3
#define EpnDelStaleMskGnt3Sft                3
#define EpnDelStaleMskGnt3Msk                0x00000008UL // [3:3]
#define EpnDelStaleMskGnt4                   0x00000010UL // offset 4
#define EpnDelStaleMskGnt4Sft                4
#define EpnDelStaleMskGnt4Msk                0x00000010UL // [4:4]
#define EpnDelStaleMskGnt5                   0x00000020UL // offset 5
#define EpnDelStaleMskGnt5Sft                5
#define EpnDelStaleMskGnt5Msk                0x00000020UL // [5:5]
#define EpnDelStaleMskGnt6                   0x00000040UL // offset 6
#define EpnDelStaleMskGnt6Sft                6
#define EpnDelStaleMskGnt6Msk                0x00000040UL // [6:6]
#define EpnDelStaleMskGnt7                   0x00000080UL // offset 7
#define EpnDelStaleMskGnt7Sft                7
#define EpnDelStaleMskGnt7Msk                0x00000080UL // [7:7]


#define EpnGntPresMsk                        TkOnuReg(0x0429)
#define EpnGntPresMskDnRdy0                  0x00000001UL // offset 0
#define EpnGntPresMskDnRdy0Sft               0
#define EpnGntPresMskDnRdy0Msk               0x00000001UL // [0:0]
#define EpnGntPresMskDnRdy1                  0x00000002UL // offset 1
#define EpnGntPresMskDnRdy1Sft               1
#define EpnGntPresMskDnRdy1Msk               0x00000002UL // [1:1]
#define EpnGntPresMskDnRdy2                  0x00000004UL // offset 2
#define EpnGntPresMskDnRdy2Sft               2
#define EpnGntPresMskDnRdy2Msk               0x00000004UL // [2:2]
#define EpnGntPresMskDnRdy3                  0x00000008UL // offset 3
#define EpnGntPresMskDnRdy3Sft               3
#define EpnGntPresMskDnRdy3Msk               0x00000008UL // [3:3]
#define EpnGntPresMskDnRdy4                  0x00000010UL // offset 4
#define EpnGntPresMskDnRdy4Sft               4
#define EpnGntPresMskDnRdy4Msk               0x00000010UL // [4:4]
#define EpnGntPresMskDnRdy5                  0x00000020UL // offset 5
#define EpnGntPresMskDnRdy5Sft               5
#define EpnGntPresMskDnRdy5Msk               0x00000020UL // [5:5]
#define EpnGntPresMskDnRdy6                  0x00000040UL // offset 6
#define EpnGntPresMskDnRdy6Sft               6
#define EpnGntPresMskDnRdy6Msk               0x00000040UL // [6:6]
#define EpnGntPresMskDnRdy7                  0x00000080UL // offset 7
#define EpnGntPresMskDnRdy7Sft               7
#define EpnGntPresMskDnRdy7Msk               0x00000080UL // [7:7]

#define EpnRptPresMsk                        TkOnuReg(0x042A)
#define EpnRptPresMskUpQRdy0                 0x00000001UL // offset 0
#define EpnRptPresMskUpQRdy0Sft              0
#define EpnRptPresMskUpQRdy0Msk              0x00000001UL // [0:0]
#define EpnRptPresMskUpQRdy1                 0x00000002UL // offset 1
#define EpnRptPresMskUpQRdy1Sft              1
#define EpnRptPresMskUpQRdy1Msk              0x00000002UL // [1:1]
#define EpnRptPresMskUpQRdy2                 0x00000004UL // offset 2
#define EpnRptPresMskUpQRdy2Sft              2
#define EpnRptPresMskUpQRdy2Msk              0x00000004UL // [2:2]
#define EpnRptPresMskUpQRdy3                 0x00000008UL // offset 3
#define EpnRptPresMskUpQRdy3Sft              3
#define EpnRptPresMskUpQRdy3Msk              0x00000008UL // [3:3]
#define EpnRptPresMskUpQRdy4                 0x00000010UL // offset 4
#define EpnRptPresMskUpQRdy4Sft              4
#define EpnRptPresMskUpQRdy4Msk              0x00000010UL // [4:4]
#define EpnRptPresMskUpQRdy5                 0x00000020UL // offset 5
#define EpnRptPresMskUpQRdy5Sft              5
#define EpnRptPresMskUpQRdy5Msk              0x00000020UL // [5:5]
#define EpnRptPresMskUpQRdy6                 0x00000040UL // offset 6
#define EpnRptPresMskUpQRdy6Sft              6
#define EpnRptPresMskUpQRdy6Msk              0x00000040UL // [6:6]
#define EpnRptPresMskUpQRdy7                 0x00000080UL // offset 7
#define EpnRptPresMskUpQRdy7Sft              7
#define EpnRptPresMskUpQRdy7Msk              0x00000080UL // [7:7]


#define EpnDrxAbortMsk                       TkOnuReg(0x042B)

#define EpnEmptyRptMsk                       TkOnuReg(0x042C)
#define EpnEmptyRptMsk0                      0x00000001UL // offset 0
#define EpnEmptyRptMsk0Sft                   0
#define EpnEmptyRptMsk0Msk                   0x00000001UL // [0:0]
#define EpnEmptyRptMsk1                      0x00000002UL // offset 1
#define EpnEmptyRptMsk1Sft                   1
#define EpnEmptyRptMsk1Msk                   0x00000002UL // [1:1]
#define EpnEmptyRptMsk2                      0x00000004UL // offset 2
#define EpnEmptyRptMsk2Sft                   2
#define EpnEmptyRptMsk2Msk                   0x00000004UL // [2:2]
#define EpnEmptyRptMsk3                      0x00000008UL // offset 3
#define EpnEmptyRptMsk3Sft                   3
#define EpnEmptyRptMsk3Msk                   0x00000008UL // [3:3]
#define EpnEmptyRptMsk4                      0x00000010UL // offset 4
#define EpnEmptyRptMsk4Sft                   4
#define EpnEmptyRptMsk4Msk                   0x00000010UL // [4:4]
#define EpnEmptyRptMsk5                      0x00000020UL // offset 5
#define EpnEmptyRptMsk5Sft                   5
#define EpnEmptyRptMsk5Msk                   0x00000020UL // [5:5]
#define EpnEmptyRptMsk6                      0x00000040UL // offset 6
#define EpnEmptyRptMsk6Sft                   6
#define EpnEmptyRptMsk6Msk                   0x00000040UL // [6:6]
#define EpnEmptyRptMsk7                      0x00000080UL // offset 7
#define EpnEmptyRptMsk7Sft                   7
#define EpnEmptyRptMsk7Msk                   0x00000080UL // [7:7]


#define EpnOverFlowBurstCapMsk               TkOnuReg(0x042d)
#define EpnBBHUpsFaultMks                    TkOnuReg(0x042e)


#define EpnMaxGntSize                        TkOnuReg(0x0430)
//  Maximum legal grant size
#define EpnMaxGntSizeSft                     0
#define EpnMaxGntSizeMsk                     0x0000FFFFUL // [15:0]

#define EpnMaxFrameSize                      TkOnuReg(0x0431)
//  Maximum sized frame allowed in EPN
#define EpnMaxFrameSizeCfgSft                0
#define EpnMaxFrameSizeCfgMsk                0x000007FFUL // [10:0]

#define EpnGntOvrhd                          TkOnuReg(0x0432)
//  Portion of grant consumed by overhead
#define EpnGntOvrhdSft                       0
#define EpnGntOvrhdMsk                       0x0000FFFFUL // [15:0]

#define EpnGntOvrdFecSft                     16
#define EpnGntOvrdFecMsk                     0xFFFF0000UL // [31:16]


#define EpnPollSize                          TkOnuReg(0x0433)
//  Size of a poll
#define EpnPollSizeSft                       0
#define EpnPollSizeMsk                       0x0000FFFFUL // [15:0]

#define EpnPollSizeFecSft                    16
#define EpnPollSizeFecMsk                    0xFFFF0000UL // [31:16]


#define EpnDnRdGntMargin                     TkOnuReg(0x0434)
//  How soon before3 to consider a grant
#define EpnDnRdGntMarginStartSft             0
#define EpnDnRdGntMarginStartMsk             0x0000FFFFUL // [15:0]
//  How soon before grant time to Xmt
#define EpnDnRdGntMarginStartTimeDeltaSft    0
#define EpnDnRdGntMarginStartTimeDeltaMsk    0x0000FFFFUL // [15:0]

#define EpnGntTimeStartDelta                 TkOnuReg(0x0435)

#define EpnTimeStampDiff                     TkOnuReg(0x0436)
//  Write Data to the Report FIFO
#define EpnTimeStampDiffDeltaSft             0
#define EpnTimeStampDiffDeltaMsk             0x0000FFFFUL // [15:0]

#define EpnUpTimeStampOff                    TkOnuReg(0x0437)
//  Amount to increase Time stamp
#define EpnUpTimeStampOffsetSft              0
#define EpnUpTimeStampOffsetMsk              0x0000FFFFUL // [15:0]
#define EpnUpTimeStampOffsetFecSft           16
#define EpnUpTimeStampOffsetFecMsk           0xFFFF0000UL // [31:16]


#define EpnGntIntvl                          TkOnuReg(0x0438)
//  Programmable setting for Grant Interval int
#define EpnGntIntvlSft                       0
#define EpnGntIntvlMsk                       0x0000FFFFUL // [15:0]

#define EpnDnGntMisalignThr                  TkOnuReg(0x0439)
//  Num. of grants needed to cause misalign
#define EpnDnGntMisalignThreshSft            0
#define EpnDnGntMisalignThreshMsk            0x000003FFUL // [9:0]
//  Allowable number of unused TimeQuanta
#define EpnDnGntMisalignThrPrvUnusedesholdSft 16
#define EpnDnGntMisalignThrPrvUnusedesholdMsk 0xFFFF0000UL // [31:16]

#define EpnDnGntMisalignPause                TkOnuReg(0x043A)
//  How long to puase from misalign
#define EpnDnGntMisalignPauseSft             0
#define EpnDnGntMisalignPauseMsk             0x0000FFFFUL // [15:0]

#define EpnNonPollIntv                       TkOnuReg(0x043B)
//  Interval for non poll grant interrupts
#define EpnNonPollIntvGntSft                 0
#define EpnNonPollIntvGntMsk                 0x0000FFFFUL // [15:0]

#define EpnForceFcsErr                       TkOnuReg(0x043C)
//  Force FCS error on frames on index 0
#define EpnForceFcsErr0                      0x00000001UL // offset 0
//  Force FCS error on frames on index 0
#define EpnForceFcsErr0Sft                   0
#define EpnForceFcsErr0Msk                   0x00000001UL // [0:0]
//  Force FCS error on frames on index 1
#define EpnForceFcsErr1                      0x00000002UL // offset 1
//  Force FCS error on frames on index 1
#define EpnForceFcsErr1Sft                   1
#define EpnForceFcsErr1Msk                   0x00000002UL // [1:1]
//  Force FCS error on frames on index 2
#define EpnForceFcsErr2                      0x00000004UL // offset 2
//  Force FCS error on frames on index 2
#define EpnForceFcsErr2Sft                   2
#define EpnForceFcsErr2Msk                   0x00000004UL // [2:2]
//  Force FCS error on frames on index 3
#define EpnForceFcsErr3                      0x00000008UL // offset 3
//  Force FCS error on frames on index 3
#define EpnForceFcsErr3Sft                   3
#define EpnForceFcsErr3Msk                   0x00000008UL // [3:3]
//  Force FCS error on frames on index 4
#define EpnForceFcsErr4                      0x00000010UL // offset 4
//  Force FCS error on frames on index 4
#define EpnForceFcsErr4Sft                   4
#define EpnForceFcsErr4Msk                   0x00000010UL // [4:4]
//  Force FCS error on frames on index 5
#define EpnForceFcsErr5                      0x00000020UL // offset 5
//  Force FCS error on frames on index 5
#define EpnForceFcsErr5Sft                   5
#define EpnForceFcsErr5Msk                   0x00000020UL // [5:5]
//  Force FCS error on frames on index 6
#define EpnForceFcsErr6                      0x00000040UL // offset 6
//  Force FCS error on frames on index 6
#define EpnForceFcsErr6Sft                   6
#define EpnForceFcsErr6Msk                   0x00000040UL // [6:6]
//  Force FCS error on frames on index 7
#define EpnForceFcsErr7                      0x00000080UL // offset 7
//  Force FCS error on frames on index 7
#define EpnForceFcsErr7Sft                   7
#define EpnForceFcsErr7Msk                   0x00000080UL // [7:7]


#define EpnGntOverlapLimit                   TkOnuReg(0x043D)
//  Maximum number time quanta overlap
#define EpnGntOverlapLimitPrvSft             0
#define EpnGntOverlapLimitPrvMsk             0x0000FFFFUL // [15:0]

#define EpnSpewQ                             TkOnuReg(0x043E)

#define EpnAesConfiguration                  TkOnuReg(0x043F)

#define EpnDiscGntOvrhd                      TkOnuReg(0x0440)
//  Portion of disc consumed by overhead
#define EpnDiscGntOvrhdSft                   0
#define EpnDiscGntOvrhdMsk                   0x0000FFFFUL // [15:0]

#define EpnDnDiscSeed                        TkOnuReg(0x0441)
//  Initializes disc offset
#define EpnDnDiscSeedCfgSft                  0
#define EpnDnDiscSeedCfgMsk                  0x00000FFFUL // [11:0]

#define EpnDnDiscInc                         TkOnuReg(0x0442)
//  Per clk increment of disc offset
#define EpnDnDiscIncCfgSft                   0
#define EpnDnDiscIncCfgMsk                   0x000003FFUL // [9:0]

#define EpnDnDiscSize                        TkOnuReg(0x0443)
//  Discovery grant length
#define EpnDnDiscSizeCfgSft                  0
#define EpnDnDiscSizeCfgMsk                  0x0000FFFFUL // [15:0]

#define EpnProgRptVec                        TkOnuReg(0x0444)

#define EpnFecIpgLen                         TkOnuReg(0x0445)
//  IPG length for FEC
#define EpnFecIpgLenCfgSft                   0
#define EpnFecIpgLenCfgMsk                   0x000000FFUL // [7:0]
//  block size used for FEC
#define EpnFecIpgLenCfgRptBlkSft             8
#define EpnFecIpgLenCfgRptBlkMsk             0x0000FF00UL // [15:8]
//  Report + IPG + (FEC) length in TQ
#define EpnFecIpgLenCfgRptSft                16
#define EpnFecIpgLenCfgRptMsk                0x00FF0000UL // [23:16]

#define EpnFakeRptValEn                      TkOnuReg(0x0446)
//  Enable for forced report value mode
#define EpnFakeRptValEnSft                   0
#define EpnFakeRptValEnMsk                   0x000000FFUL // [7:0]

#define EpnFakeRptVal                        TkOnuReg(0x0447)
//  The number of bytes to report
#define EpnFakeRptValSft                     0
#define EpnFakeRptValMsk                     0x001FFFFFUL // [20:0]

#define EpnBurstCap                          TkOnuTable(0x0448)
#define EpnBurstCapCount                     0x00000018UL
//  Burst Cap Size for FIFO 0
#define EpnBurstCap0Sft                      0
#define EpnBurstCap0Msk                      0x000FFFFFUL // [19:0]

#define EpnQLlidMap                          TkOnuTable(0x0460)
#define EpnQLlidMapCount                     0x00000020UL
//  Which LLID (Fifo) Queue 0 is on
#define EpnQLlidMapQ0Sft                     0
#define EpnQLlidMapQ0Msk                     0x0000001FUL // [4:0]
//  Enable queue to be scheduled
#define EpnQLlidMapCfgSchEn                  0x80000000UL // offset 31
//  Enable queue to be scheduled
#define EpnQLlidMapCfgSchEnSft               31
#define EpnQLlidMapCfgSchEnMsk               0x80000000UL // [31:31]

#define EpnRptMapPri                         TkOnuTable(0x0480)
#define EpnRptMapPriCount                    0x00000008UL

#define EpnMpcpOpCodeEn                      TkOnuTable(0x0489)
#define EpnUsTxMargin                        TkOnuTable(0x048a)

#define EpnMultiPriCfg                       TkOnuReg(0x0490)
#define EpnMultiPriCfgCount                  0x00000006UL
#define EpnMultiPriCfgWidth                  0x00000006UL
//  0 = LLID 0 not in Multi-Pri mode;
#define EpnMultiPriCfgRpt                    0x00000001UL // offset 0
//  0 = LLID 0 not in Multi-Pri mode;
#define EpnMultiPriCfgRpt0Sft                0
#define EpnMultiPriCfgRpt0Msk                0x00000001UL // [0:0]
//  0 = Report 4 priorities; 1 = report 8
#define EpnMultiPriCfgRpt80                  0x00000002UL // offset 1
//  0 = Report 4 priorities; 1 = report 8
#define EpnMultiPriCfgRpt80Sft               1
#define EpnMultiPriCfgRpt80Msk               0x00000002UL // [1:1]
//  Swap the two Queue Sets in Multi-Pri reporting mode
#define EpnMultiPriCfgRptSwapQS0             0x00000004UL // offset 2
//  Swap the two Queue Sets in Multi-Pri reporting mode
#define EpnMultiPriCfgRptSwapQS0Sft          2
#define EpnMultiPriCfgRptSwapQS0Msk          0x00000004UL // [2:2]
//  In Multi-Pri reports, send high-priority reports first
#define EpnMultiPriCfgRptHiFirst0            0x00000008UL // offset 3
//  In Multi-Pri reports, send high-priority reports first
#define EpnMultiPriCfgRptHiFirst0Sft         3
#define EpnMultiPriCfgRptHiFirst0Msk         0x00000008UL // [3:3]
//  Multi-Pri Report Values INCLUDE
#define EpnMultiPriCfgRptGntsOutst0          0x00000010UL // offset 4
//  Multi-Pri Report Values INCLUDE
#define EpnMultiPriCfgRptGntsOutst0Sft       4
#define EpnMultiPriCfgRptGntsOutst0Msk       0x00000010UL // [4:4]
//  Multi-Pri accumulators share same burst cap
#define EpnMultiPriCfgPrvSharedBurstCap      0x00000020UL // offset 5
//  Multi-Pri accumulators share same burst cap
#define EpnMultiPriCfgPrvSharedBurstCapSft   5
#define EpnMultiPriCfgPrvSharedBurstCapMsk   0x00000020UL // [5:5]

#define EpnMultiPriCfgSharedL2               0x00000040UL // offset 6

#define EpnForceReportEn                    TkOnuReg(0x0497)
#define EpnL2FlushCmd                       TkOnuReg(0x0498)
#define EpnL2FlushQueueSft                0
#define EpnL2FlushQueueMsk              0x0000001FUL // [4:0]

#define EpnL2FlushQueueDone      0x40000000UL // offset 30
#define EpnL2FlushQueueEnable    0x80000000UL // offset 30


#define EpnDpCmd                             TkOnuReg(0x04A0)
#define EpnDpCmdCtlSft                       0
#define EpnDpCmdCtlMsk                       0x00000001UL // [0:0]
//  Data Port RAM Select
#define EpnDpCmdSelSft                       4
#define EpnDpCmdSelMsk                       0x000000F0UL // [7:4]
#define EpnDpCmdNotIntRdy                    0x80000000UL // offset 31
#define EpnDpCmdNotIntRdySft                 31
#define EpnDpCmdNotIntRdyMsk                 0x80000000UL // [31:31]

#define EpnDpAddr                            TkOnuReg(0x04A1)
#define EpnDpAddrSft                         0
#define EpnDpAddrMsk                         0x00007FFFUL // [14:0]

#define EpnDpData                            TkOnuTable(0x04A2)
#define EpnDpDataCount                       0x00000002UL

#define EpnUnmapBigCnt                       TkOnuReg(0x04A4)

#define EpnUnmapFrameCnt                     TkOnuReg(0x04A5)

#define EpnUnmapFcsCnt                       TkOnuReg(0x04A6)

#define EpnUnmapGateCnt                      TkOnuReg(0x04A7)

#define EpnUnmapOamCnt                       TkOnuReg(0x04A8)

#define EpnUnmapSmallCnt                     TkOnuReg(0x04A9)

#define EpnFifEnqueueEventCnt                TkOnuReg(0x04AA)

#define EpnFifDequeueEventCnt                TkOnuReg(0x04AB)

#define EpnUnusedTqCntLlidSel                TkOnuReg(0x04AC)
#define EpnUnusedTqCntLlidSelCfgSft          0
#define EpnUnusedTqCntLlidSelCfgMsk          0x0000000FUL // [3:0]

#define EpnUnusedTqCnt                       TkOnuReg(0x04AD)

#define EpnRegBankSel                        TkOnuReg(0x04AF)
#define EpnRegBankSelSft                     0
#define EpnRegBankSelMsk                     0x00000003UL // [1:0]

#define EpnRegBank4bx                        TkOnuReg(0x004B)

#define EpnRegBank4cx                        TkOnuReg(0x004C)

#define EpnRegBank4dx                        TkOnuReg(0x004D)

#define EpnRegBank4ex                        TkOnuReg(0x004E)




#define EpnBBHUpsFaultHaltEn                 TkOnuTable(0x04B0)

#define EpnOnuMacAddr0                       TkOnuTable(0x0500)

#define EpnOnuMacAddr0Count                  0x00000030UL
#define EpnOnuMacAddr0Width                  0x00000002UL
#define EpnOnuMacAddr0AddrReg0Sft            0
#define EpnOnuMacAddr0AddrReg0Msk            0x00FFFFFFUL // [23:0]
#define EpnOnuMacAddr0MfgAddrRegSft          24
#define EpnOnuMacAddr0MfgAddrRegMsk          0xFF000000UL // [31:24]
#define EpnOnuMacAddr0MfgAddrRegASft         0
#define EpnOnuMacAddr0MfgAddrRegAMsk         0x0000FFFFUL // [15:0]

#define EpnOltMacAddr                        TkOnuTable(0x0530)
#define EpnOltMacAddrCount                   0x00000002UL
#define EpnOltMacAddrWidth                   0x00000002UL
//  Address of the OLT
#define EpnOltMacAddrSft                     0
#define EpnOltMacAddrMsk                     0x0000FFFFUL // [15:0]

#define EpnTxL1sQCfg                         TkOnuTable(0x0540)
#define EpnTxL1sQCfgCount                    0x00000020UL
//  Provisioned end address for queue0
#define EpnTxL1sQCfgQEnd0Sft                 16
#define EpnTxL1sQCfgQEnd0Msk                 0x1FFF0000UL // [28:16]
//  Provisioned start address for queue0
#define EpnTxL1sQCfgQStart0Sft               0
#define EpnTxL1sQCfgQStart0Msk               0x00001FFFUL // [12:0]

#define EpnTxL1sShpCfg                       TkOnuTable(0x0560)
#define EpnTxL1sShpCfgCount                  0x00000012UL
#define EpnTxL1sShpCfgWidth                  0x00000002UL
//  Include packet overhead in shaping
#define EpnTxL1sShpCfgIncOvr                 0x80000000UL
#define EpnTxL1sShpCfgIncOvr0Sft             31
#define EpnTxL1sShpCfgIncOvr0Msk             0x80000000UL // [31:31]
//  Shaping Rate
#define EpnTxL1sShpCfgRate0Sft               8
#define EpnTxL1sShpCfgRate0Msk               0x7FFFFF00UL // [30:8]
//  Maximum Burst Size, 256 byte quanta
#define EpnTxL1sShpCfgBstSize0Sft            0
#define EpnTxL1sShpCfgBstSize0Msk            0x000000FFUL // [7:0]

#define EpnTxL1ShpRateIdx                    0
#define EpnTxL1ShpMapIdx                     1

#define EpnTxL1sUnshapedEmpty                TkOnuReg(0x04E2)

#define EpnTxL1sShpQStopped                  TkOnuReg(0x04E3)

#define EpnRptRdPtr                          TkOnuTable(0x04B0)
#define EpnRptRdPtrCount                     0x00000008UL

#define EpnRptWrPtr                          TkOnuTable(0x04B8)
#define EpnRptWrPtrCount                     0x00000008UL

#define EpnGntQPtr                           TkOnuReg(0x0590)

#define EpnTxL2sQCfg                         TkOnuTable(0x05a0)
#define EpnTxL2sQCfgCount                    0x00000018UL
#define EpnTxL2sQCfgQEnd0Sft                 16
#define EpnTxL2sQCfgQEnd0Msk                 0x00010000UL // [16:16]
#define EpnTxL2sQCfgQStart0Sft               0
#define EpnTxL2sQCfgQStart0Msk               0x00000007UL // [2:0]

#define EpnTxL2sQEmpty                       TkOnuReg(0x04C8)
#define EpnTxL2sQEmptyQSft                   0
#define EpnTxL2sQEmptyQMsk                   0x00000007UL // [2:0]

#define EpnTxL2sQFull                        TkOnuReg(0x04C9)
#define EpnTxL2sQFullQSft                    0
#define EpnTxL2sQFullQMsk                    0x00000007UL // [2:0]

#define EpnTxL2sQStopped                     TkOnuReg(0x04CA)
#define EpnTxL2sQStoppedQSft                 0
#define EpnTxL2sQStoppedQMsk                 0x00000007UL // [2:0]

#define EpnTxCtcBurstLimit                   TkOnuTable(0x05C0)
#define EpnTxCtcBurstLimitCount              0x00000018UL
//  CTC scheduling data burst limit
#define EpnTxCtcBurstLimitPrv0Sft            0
#define EpnTxCtcBurstLimitPrv0Msk            0x0003FFFFUL // [17:0]

#define EpnDebugInt                          TkOnuTable(0x04F0)
#define EpnDebugIntCount                     0x00000002UL

#define EpnL1QueueEmpty                      TkOnuReg(0x0571)
#define EpnL1StoppedQueues                   TkOnuReg(0x0572)
#define EpnL1UnshapedQueueEmpty              TkOnuReg(0x0573)



#define EpnL2QueueEmpty                      TkOnuReg(0x05b8)
#define EpnL2QueueFull                      TkOnuReg(0x05b9)
#define EpnL2StoppedQueues                      TkOnuReg(0x05b9)


#define EpnL1QByteLimit                      TkOnuTable(0x05E0)
#define EpnL1QByteLimitCount                 0x00000020UL
#define EpnL1QByteLimit0Sft                  0
#define EpnL1QByteLimit0Msk                  0x000000FFUL // [7:0]


#define EpnVpnRst                            TkOnuReg(0x000)
#define EpnVpnRstLifRstN                     0x00000001UL       
#define EpnVpnRstEpnRstN                     0x00000002UL       
#define EpnVpnRstNcoRst                      0x00000004UL       
#define EpnVpnRstClkDivRst                   0x00000008UL       

#define EpnVpnIntStatus                      TkOnuReg(0x001)
#define EpnVpnIntMask                        TkOnuReg(0x002)


#if defined(__cplusplus)
}
#endif


#endif // EpnRegs.h

