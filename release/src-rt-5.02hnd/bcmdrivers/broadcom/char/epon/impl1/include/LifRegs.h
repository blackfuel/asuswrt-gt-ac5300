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
/// \file LifRegs.h
/// \brief Lif Register Definitions
///
////////////////////////////////////////////////////////////////////////////////
#if !defined(LifRegs_h)
#define LifRegs_h

#if defined(__cplusplus)
extern "C" {
#endif

#include "Teknovus.h"


#define LifLaserOffIdleBefore   0x00000080UL

#define LifDpDataNoEncryption    1
#define LifDpDataEncryptWithKey0 2
#define LifDpDataEncryptWithKey1 3

#define LifEncryptionTeknovusMode 0
#define LifEncryptionReserved1    1
#define LifEncryptionEponMode     2
#define LifEncryptionTripleChurn  3
#define LifEncryptionDisable      4

#define LifGetRxStatReg(stat)  TkOnuReg(0x150 + (stat))
#define LifGetTxStatReg(stat)  TkOnuReg(0x163 + (stat))

#define LifIFserdesCtl                       TkOnuTable(0x0030)
#define LifIFserdesCtlCount                  0x00000003UL

#define LifIFclkCtl                          TkOnuTable(0x0033)
#define LifIFclkCtlCount                     0x00000002UL

#define LifIFtestCtl                         TkOnuTable(0x0035)
#define LifIFtestCtlCount                    0x00000002UL

#define LifIFserdesInt                       TkOnuTable(0x0037)
#define LifIFserdesIntCount                  0x00000002UL

#define LifSERDES_INT_STATUS                 TkOnuReg(0x0039)

#define LifSERDES_INT_MASK                   TkOnuReg(0x003A)

#define LifIFserdesHidden                    TkOnuTable(0x003B)
#define LifIFserdesHiddenCount               0x00000002UL

#define LifSER_DSCR_CTRL                     TkOnuReg(0x003D)

#define LifPonCtl                            TkOnuReg(0x0100)
                       
#define LifPonCtlRxEn                        0x00000001UL // offset 0
                     
#define LifPonCtlRxEnSft                     0
#define LifPonCtlRxEnMsk                     0x00000001UL // [0:0]


#define LifPonCtlTxEn                        0x00000002UL // offset 1
#define LifPonCtlTxEnSft                     1
#define LifPonCtlTxEnMsk                     0x00000002UL // [1:1]


#define LifPonCtlNotRxRst                    0x00000004UL // offset 2
#define LifPonCtlNotRxRstSft                 2
#define LifPonCtlNotRxRstMsk                 0x00000004UL // [2:2]


#define LifPonCtlNotTxRst                    0x00000008UL // offset 3
#define LifPonCtlNotTxRstSft                 3
#define LifPonCtlNotTxRstMsk                 0x00000008UL // [3:3]

//  0: Laser On is active low.            
#define LifPonCtlCfTxLaserOnActHi            0x00000010UL // offset 4
//  0: Laser On is active low.            
#define LifPonCtlCfTxLaserOnActHiSft         4
#define LifPonCtlCfTxLaserOnActHiMsk         0x00000010UL // [4:4]


//  0: Turn laser on during burst.       
#define LifPonCtlCfTxLaserOn                 0x00000020UL // offset 5
//  0: Turn laser on during burst.       
#define LifPonCtlCfTxLaserOnSft              5
#define LifPonCtlCfTxLaserOnMsk              0x00000010UL // [5:5]

//  Takes the laser out of tri state      
#define LifPonCtlCfTxLaserEn                 0x00000040UL // offset 6
//  Takes the laser out of tri state      
#define LifPonCtlCfTxLaserEnSft              6
#define LifPonCtlCfTxLaserEnMsk              0x00000040UL // [6:6]

//  Enable the Lif to send short preamble when in P2P 
#define LifPonCtlCfP2PShortPre               0x00000080UL // offset 7
//  Enable the Lif to send short preamble when in P2P 
#define LifPonCtlCfP2PShortPreSft            7
#define LifPonCtlCfP2PShortPreMsk            0x00000080UL // [7:7]


//  Point to Point Mode            
#define LifPonCtlCfP2PMode                   0x00000100UL // offset 8
//  Point to Point Mode            
#define LifPonCtlCfP2PModeSft                8
#define LifPonCtlCfP2PModeMsk                0x00000100UL // [8:8]

//  0: lifIoTxData continue to tx IDLE    
#define LifPonCtlCfTxZeroDurLOff             0x00000200UL // offset 9
//  0: lifIoTxData continue to tx IDLE    
#define LifPonCtlCfTxZeroDurLOffSft          8
#define LifPonCtlCfTxZeroDurLOffMsk          0x00000200UL // [9:9]

//  Enable LIF do Up to Downstream LpBck  
#define LifPonCtlCfTx2RxLpBack               0x00000400UL // offset 10
//  Enable LIF do Up to Downstream LpBck  
#define LifPonCtlCfTx2RxLpBackSft            10
#define LifPonCtlCfTx2RxLpBackMsk            0x00000400UL // [10:10]

//  Enable LIF do Down to Upstream LpBck  
#define LifPonCtlCfRx2TxLpBack               0x00000800UL // offset 11
//  Enable LIF do Down to Upstream LpBck  
#define LifPonCtlCfRx2TxLpBackSft            11
#define LifPonCtlCfRx2TxLpBackMsk            0x00000800UL // [11:11]

//  1Hz pulse clock out on posedge of     
#define LifPonCtlCfPpsClkRbc                 0x00001000UL // offset 12
//  1Hz pulse clock out on posedge of     
#define LifPonCtlCfPpsClkRbcSft              12
#define LifPonCtlCfPpsClkRbcMsk              0x00001000UL // [12:12]


//  enable 10Mhz and 1Hz signals          
#define LifPonCtlCfPpsEn                     0x00002000UL // offset 13
//  enable 10Mhz and 1Hz signals          
#define LifPonCtlCfPpsEnSft                  13
#define LifPonCtlCfPpsEnMsk                  0x00002000UL // [13:13]


//  Quick Sync non stanard mode         
#define LifPonCtlCfRxEnQuickSync             0x00004000UL // offset 14
//  Quick Sync non stanard mode         
#define LifPonCtlCfRxEnQuickSyncSft          14
#define LifPonCtlCfRxEnQuickSyncMsk          0x00004000UL // [14:14]

//  Stay in snyc state longer           
#define LifPonCtlCfRxEnExtendSync            0x00008000UL // offset 15
//  Stay in snyc state longer           
#define LifPonCtlCfRxEnExtendSyncSft         15
#define LifPonCtlCfRxEnExtendSyncMsk         0x00008000UL // [15:15]


//  Rx Sync statemachine will not 
#define LifPonCtlCfRxEnSoftwareSyncHold      0x00010000UL // offset 16
//  Rx Sync statemachine will not 
#define LifPonCtlCfRxEnSoftwareSyncHoldSft   16
#define LifPonCtlCfRxEnSoftwareSyncHoldMsk   0x00010000UL // [16:16]

//  bit flip
#define LifPonCtlCfgRxDataBitFlip            0x00100000UL // offset 20
//  bit flip
#define LifPonCtlCfgRxDataBitFlipSft         20
#define LifPonCtlCfgRxDataBitFlipMsk         0x00100000UL // [20:20]
//  PON module force fec
#define LifPonCtlCfRxForceFecAbort           0x00200000UL // offset 21
//  PON module force fec
#define LifPonCtlCfRxForceFecAbortSft        21
#define LifPonCtlCfRxForceFecAbortMsk        0x00200000UL // [21:21]
//  PON module force NonFec
#define LifPonCtlCfRxForceNonFecAbort        0x00400000UL // offset 22
//  PON module force NonFec
#define LifPonCtlCfRxForceNonFecAbortSft     22
#define LifPonCtlCfRxForceNonFecAbortMsk     0x00400000UL // [22:22]


#define LifPonCtlCfSynsSMSelect              0x00800000UL // offset 23
#define LifPonCtlCfSynsSMSelectSft           23
#define LifPonCtlCfSynsSMSelectMsk           0x00c00000UL // [23:24]

#define LifPonCtlCfMaxCommaErr               0x02000000UL // offset 25
#define LifPonCtlCfMaxCommaErrSft            25
#define LifPonCtlCfMaxCommaErrMsk            0x1e000000UL // [25:28]

#define LifPonCtlCfOneGigPonDn               0x20000000UL // offset 25
#define LifPonCtlCfOneGigPonDnSft            29
#define LifPonCtlCfOneGigPonDnMsk            0x20000000UL // [29:29]




#define LifPonIopCtl                         TkOnuReg(0x0101)
//  Lif tx pipe line delay                 
#define LifPonIopCtlCfTxPipeDelaySft         0
#define LifPonIopCtlCfTxPipeDelayMsk         0x0000000FUL // [3:0]
//  =0: Normal Operation                  
#define LifPonIopCtlCfTxASyncLenSft          4
#define LifPonIopCtlCfTxASyncLenMsk          0x000000F0UL // [7:4]
//  Tx Ipg count                           
#define LifPonIopCtlCfTxIpgCntSft            8
#define LifPonIopCtlCfTxIpgCntMsk            0x00000F00UL // [11:8]
//  7 byte preamble (SOP,d5)              
#define LifPonIopCtlCfTxShortPre             0x00001000UL // offset 12
//  7 byte preamble (SOP,d5)              
#define LifPonIopCtlCfTxShortPreSft          12
#define LifPonIopCtlCfTxShortPreMsk          0x00001000UL // [12:12]
//  Generate crc8 from Msb to Lsb         
#define LifPonIopCtlCfTxCrc8Msb2Lsb          0x00002000UL // offset 13
//  Generate crc8 from Msb to Lsb         
#define LifPonIopCtlCfTxCrc8Msb2LsbSft       13
#define LifPonIopCtlCfTxCrc8Msb2LsbMsk       0x00002000UL // [13:13]
//  Bit Swap Crc8 Byte                    
#define LifPonIopCtlCfTxCrc8BitSwap          0x00004000UL // offset 14
//  Bit Swap Crc8 Byte                    
#define LifPonIopCtlCfTxCrc8BitSwapSft       14
#define LifPonIopCtlCfTxCrc8BitSwapMsk       0x00004000UL // [14:14]
//  Generate Bad crc-8                    
#define LifPonIopCtlCfTxCrc8Bad              0x00008000UL // offset 15
//  Generate Bad crc-8                    
#define LifPonIopCtlCfTxCrc8BadSft           15
#define LifPonIopCtlCfTxCrc8BadMsk           0x00008000UL // [15:15]
//  Allow Sec to invert CRC8              
#define LifPonIopCtlCfTxCrc8Inv              0x00010000UL // offset 16
//  Allow Sec to invert CRC8              
#define LifPonIopCtlCfTxCrc8InvSft           16
#define LifPonIopCtlCfTxCrc8InvMsk           0x00010000UL // [16:16]
//  1: Set Upstream bit 15 of LLID        
#define LifPonIopCtlCfTxLlidBit15Set         0x00020000UL // offset 17
//  1: Set Upstream bit 15 of LLID        
#define LifPonIopCtlCfTxLlidBit15SetSft      17
#define LifPonIopCtlCfTxLlidBit15SetMsk      0x00020000UL // [17:17]
//  1: Mask highest bit of LLID
#define LifPonIopCtlCfRxLlidBit15Msk         0x00040000UL // offset 18
//  1: Mask highest bit of LLID
#define LifPonIopCtlCfRxLlidBit15MskSft      18
#define LifPonIopCtlCfRxLlidBit15MskMsk      0x00040000UL // [18:18]
//  disables checking of CRC8 in FEC      
#define LifPonIopCtlCfRxCrc8Disable          0x00080000UL // offset 19
//  disables checking of CRC8 in FEC      
#define LifPonIopCtlCfRxCrc8DisableSft       19
#define LifPonIopCtlCfRxCrc8DisableMsk       0x00080000UL // [19:19]
//  Msb to Lsb checking for crc8         
#define LifPonIopCtlCfRxCrc8Msb2Lsb          0x00100000UL // offset 20
//  Msb to Lsb checking for crc8         
#define LifPonIopCtlCfRxCrc8Msb2LsbSft       20
#define LifPonIopCtlCfRxCrc8Msb2LsbMsk       0x00100000UL // [20:20]
//  Checkin for bit swap crc-8 byte       
#define LifPonIopCtlCfRxCrc8BitSwap          0x00200000UL // offset 21
//  Checkin for bit swap crc-8 byte       
#define LifPonIopCtlCfRxCrc8BitSwapSft       21
#define LifPonIopCtlCfRxCrc8BitSwapMsk       0x00200000UL // [21:21]
//  0: Do not enable crc8 invert chk      
#define LifPonIopCtlCfRxCrc8InvChk           0x00400000UL // offset 22
//  0: Do not enable crc8 invert chk      
#define LifPonIopCtlCfRxCrc8InvChkSft        22
#define LifPonIopCtlCfRxCrc8InvChkMsk        0x00400000UL // [22:22]

#define LifPonIopCtlCfUseFecIpg              0x00800000UL // offset 23
#define LifPonIopCtlCfLlidModMsk             0x01000000UL // offset 24
#define LifPonIopCtlCfLlidProMode            0x02000000UL // offset 25


#define LifFecCtl                            TkOnuReg(0x0102)
//  Enable Tx-Fec
#define LifFecCtlCfTxEn                      0x00000001UL // offset 0
//  Enable Tx-Fec
#define LifFecCtlCfTxEnSft                   0
#define LifFecCtlCfTxEnMsk                   0x00000001UL // [0:0]
//  Tx Fec Per LLID
#define LifFecCtlCfTxPerLlid                 0x00000002UL // offset 1
//  Tx Fec Per LLID
#define LifFecCtlCfTxPerLlidSft              1
#define LifFecCtlCfTxPerLlidMsk              0x00000002UL // [1:1]
//  Enable Rx-Fec  
#define LifFecCtlCfRxEn                      0x00000004UL // offset 2
//  Enable Rx-Fec  
#define LifFecCtlCfRxEnSft                   2
#define LifFecCtlCfRxEnMsk                   0x00000004UL // [2:2]
//  all FEC frames will be force aborted 
#define LifFecCtlCfRxForceAbort              0x00000008UL // offset 3
//  all FEC frames will be force aborted 
#define LifFecCtlCfRxForceAbortSft           3
#define LifFecCtlCfRxForceAbortMsk           0x00000008UL // [3:3]
//  all non FEC frames will be aborted
#define LifFecCtlCfRxForceNonAbort           0x00000010UL // offset 4
//  all non FEC frames will be aborted
#define LifFecCtlCfRxForceNonAbortSft        4
#define LifFecCtlCfRxForceNonAbortMsk        0x00000010UL // [4:4]
//  forward frame with an uncorrectable
#define LifFecCtlCfRxErrorProp               0x00000020UL // offset 5
//  forward frame with an uncorrectable
#define LifFecCtlCfRxErrorPropSft            5
#define LifFecCtlCfRxErrorPropMsk            0x00000020UL // [5:5]

#define LifFecCtlTxFecLlidEnBase            0x00010000UL
#define LifFecCtlTxFecLlidEnSft             16
#define LifFecCtlTxFecLlidEnMsk             0x00FF0000UL
#define LifSecCtl                            TkOnuReg(0x0103)
//  enable downstream security 
#define LifSecCtlEnDn                        0x00000001UL // offset 0
//  enable downstream security 
#define LifSecCtlEnDnSft                     0
#define LifSecCtlEnDnMsk                     0x00000001UL // [0:0]
//  enable upstream security
#define LifSecCtlEnUp                        0x00000002UL // offset 1
//  enable upstream security
#define LifSecCtlEnUpSft                     1
#define LifSecCtlEnUpMsk                     0x00000002UL // [1:1]
#define LifSecCtlNotDnRst                    0x00000004UL // offset 2
#define LifSecCtlNotDnRstSft                 2
#define LifSecCtlNotDnRstMsk                 0x00000004UL // [2:2]
#define LifSecCtlNotUpRst                    0x00000008UL // offset 3
#define LifSecCtlNotUpRstSft                 3
#define LifSecCtlNotUpRstMsk                 0x00000008UL // [3:3]
#define LifSecCtlDnSchemeSft                 4
#define LifSecCtlDnSchemeMsk                 0x00000030UL // [5:4]
#define LifSecCtlUpSchemeSft                 6
#define LifSecCtlUpSchemeMsk                 0x000000C0UL // [7:6]
//  Disable down da/sa encryption
#define LifSecCtlDisDnDaSaEncrpt             0x00000100UL // offset 8
//  Disable down da/sa encryption
#define LifSecCtlDisDnDaSaEncrptSft          8
#define LifSecCtlDisDnDaSaEncrptMsk          0x00000100UL // [8:8]
//  Disable up da/sa encryption
#define LifSecCtlDisUpDaSaEncrpt             0x00000200UL // offset 9
//  Disable up da/sa encryption
#define LifSecCtlDisUpDaSaEncrptSft          9
#define LifSecCtlDisUpDaSaEncrptMsk          0x00000200UL // [9:9]
//  mix mode encryption
#define LifSecCtlEnEpnMix                    0x00000400UL // offset 10
//  mix mode encryption
#define LifSecCtlEnEpnMixSft                 10
#define LifSecCtlEnEpnMixMsk                 0x00000400UL // [10:10]
//  triple churning mode
#define LifSecCtlEnTripleChurn               0x00000800UL // offset 11
//  triple churning mode
#define LifSecCtlEnTripleChurnSft            11
#define LifSecCtlEnTripleChurnMsk            0x00000800UL // [11:11]
//  fec mode IPG 
#define LifSecCtlCfgFecIpgLenSft             13
#define LifSecCtlCfgFecIpgLenMsk             0x001FE000UL // [20:13]
//  Enable mpcp swap           
#define LifSecCtlEnDnMpcpSwap                0x00200000UL // offset 21
//  Enable mpcp swap           
#define LifSecCtlEnDnMpcpSwapSft             21
#define LifSecCtlEnDnMpcpSwapMsk             0x00200000UL // [21:21]
//  Enable upstream mpcp swap  
#define LifSecCtlEnUpMpcpSwap                0x00400000UL // offset 22
//  Enable upstream mpcp swap  
#define LifSecCtlEnUpMpcpSwapSft             22
#define LifSecCtlEnUpMpcpSwapMsk             0x00400000UL // [22:22]
//  Enable fake AES            
#define LifSecCtlEnFakeDnAES                 0x00800000UL // offset 23
//  Enable fake AES            
#define LifSecCtlEnFakeDnAESSft              23
#define LifSecCtlEnFakeDnAESMsk              0x00800000UL // [23:23]
//  Enable fake AES            
#define LifSecCtlEnFakeUpAES                 0x01000000UL // offset 24
//  Enable fake AES            
#define LifSecCtlEnFakeUpAESSft              24
#define LifSecCtlEnFakeUpAESMsk              0x01000000UL // [24:24]

#define LifInt                               TkOnuReg(0x0104)
#define LifIntPonSft                         0
#define LifIntPonMsk                         0x0007FFFFUL // [18:0]
#define LifIntRxOutOfSync                    0x00000001UL // offset 0
#define LifIntRxOutOfSyncSft                 0
#define LifIntRxOutOfSyncMsk                 0x00000001UL // [0:0]

#define LifIntRxSyncAcq                      0x00000002UL // offset 1
#define LifIntRxSyncAcqSft                   1
#define LifIntRxSyncAcqMsk                   0x00000002UL // [1:1]

#define LifIntRxErrAftAlign                  0x00000004UL // offset 2
#define LifIntRxErrAftAlignSft               2
#define LifIntRxErrAftAlignMsk               0x00000004UL // [2:2]

#define LifIntRxMaxLenErr                    0x00000008UL // offset 3
#define LifIntRxMaxLenErrSft                 3
#define LifIntRxMaxLenErrMsk                 0x00000008UL // [3:3]

#define LifIntNoRxClk                        0x00000010UL // offset 4
#define LifIntNoRxClkSft                     4
#define LifIntNoRxClkMsk                     0x00000010UL // [4:4]

#define LifIntAbortRxPkt                     0x00000020UL // offset 5
#define LifIntAbortRxPktSft                  5
#define LifIntAbortRxPktMsk                  0x00000020UL // [5:5]

#define LifIntGrntStartTimeLag               0x00000040UL // offset 6
#define LifIntGrntStartTimeLagSft            6
#define LifIntGrntStartTimeLagMsk            0x00000040UL // [6:6]

#define LifIntFrOutOfAlign                   0x00000080UL // offset 7
#define LifIntFrOutOfAlignSft                7
#define LifIntFrOutOfAlignMsk                0x00000080UL // [7:7]

#define LifIntUpTimeFulld                    0x00000100UL // offset 8
#define LifIntUpTimeFulldSft                 8
#define LifIntUpTimeFulldMsk                 0x00000100UL // [8:8]

#define LifMsk                               TkOnuReg(0x0105)
#define LifMskPonSft                         0
#define LifMskPonMsk                         0x0000FFFFUL // [16:0]

#define LifIntFecMask                         TkOnuReg(0x0107)
#define LifIntFecMaskSft                      0
#define LifIntFecMaskMsk                      0x0000003FUL // [5:0]

#define LifDpCmd                             TkOnuReg(0x0108)
#define LifDpCmdRamSelSft                    0
#define LifDpCmdRamSelMsk                    0x00000007UL // [2:0]
#define LifDpCmdRamWr                        0x00000010UL // offset 4
#define LifDpCmdRamWrSft                     4
#define LifDpCmdRamWrMsk                     0x00000010UL // [4:4]
#define LifDpCmdBusy                         0x80000000UL // offset 31
#define LifDpCmdBusySft                      31
#define LifDpCmdBusyMsk                      0x80000000UL // [31:31]

#define LifDpAddr                            TkOnuReg(0x0109)
//  PBI port address to access RAM's
#define LifDpAddrPbiPortSft                  0
#define LifDpAddrPbiPortMsk                  0x0000FFFFUL // [15:0]

#define LifDpData                            TkOnuTable(0x010A)
#define LifDpDataCount                       0x00000005UL

#define LifLlid                              TkOnuTable(0x0110)
#define LifLlidCount                         0x00000020UL
//  LLID 0 lookup value
#define LifLlidLkup0Sft                      0
#define LifLlidLkup0Msk                      0x0000FFFFUL // [15:0]
#define LifLlidEn                            0x00010000UL // Offset 16

#define LifTimeRefCnt                        TkOnuReg(0x0130)
//  If the downstream timer is greater   
#define LifTimeRefCntCfMaxPosValSft          0
#define LifTimeRefCntCfMaxPosValMsk          0x000000FFUL // [7:0]
//  If the downstream timer is less      
#define LifTimeRefCntCfMaxNegValSft          8
#define LifTimeRefCntCfMaxNegValMsk          0x0000FF00UL // [15:8]
//  If the downstream timer absolute     
#define LifTimeRefCntCfFullUpdateValSft      16
#define LifTimeRefCntCfFullUpdateValMsk      0x00FF0000UL // [23:16]

#define LifTimeStampUpdPer                   TkOnuReg(0x0131)
//  Timestamp update period allow
#define LifTimeStampUpdPerCfSft              0
#define LifTimeStampUpdPerCfMsk              0x0000FFFFUL // [15:0]

#define LifTpTime                            TkOnuReg(0x0132)

#define LifMpcpTime                          TkOnuTable(0x0133)
#define LifMpcpTimeCount                     0x00000002UL
#define LifMpcpTimeWidth                     0x00000002UL
//  latched downstream mpcp time
#define LifMpcpTimeLtSft                     0
#define LifMpcpTimeLtMsk                     0x0000FFFFUL // [15:0]


#define LifMaxLenCtr                         TkOnuReg(0x0135)
//  Used to configure the maximum         
#define LifMaxLenCtrCfRxFrameSft             0
#define LifMaxLenCtrCfRxFrameMsk             0x000007FFUL // [10:0]

#define LifLaserOnDelta                      TkOnuReg(0x0136)
//  Delta from time laser is turned on   
#define LifLaserOnDeltaCfTxSft               0
#define LifLaserOnDeltaCfTxMsk               0x0003FFFFUL // [17:0]

#define LifLaserOffIdle                      TkOnuReg(0x0137)
//  Delta from time laser is turned off  
#define LifLaserOffIdleCfTxDeltaSft          0
#define LifLaserOffIdleCfTxDeltaMsk          0x000000FFUL // [7:0]
//  Time required to transmit idle after  
#define LifLaserOffIdleCfTxInitSft           16
#define LifLaserOffIdleCfTxInitMsk           0xFFFF0000UL // [31:16]

#define LifLFecInitIdle                      TkOnuReg(0x0138)
//Idle number required at the beginning of a grant
#define LifLFecInitIdleCfFecInitSft          0
#define LifLFecInitIdleCfFecInitMsk          0x0000FFFFUL // [15:0]

#define LifFecErrAllow                       TkOnuReg(0x0139)
//  Hamming distance allowed to find 
#define LifFecErrAllowCfRxSfecBitSft         0
#define LifFecErrAllowCfRxSfecBitMsk         0x0000000FUL // [3:0]
//  Hamming distance allowed to find  
#define LifFecErrAllowCfRxTfecBitSft         4
#define LifFecErrAllowCfRxTfecBitMsk         0x000000F0UL // [7:4]

#define LifSecKeySel                         TkOnuReg(0x0148)

#define LifDnSec                             TkOnuReg(0x0149)

#define LifSecUp                             TkOnuReg(0x014A)
//  upstream encryption status per LLID   
#define LifSecUpEnSft                        16
#define LifSecUpEnMsk                        0xFFFF0000UL // [31:16]
//  upstream key number per LLID          
#define LifSecUpKeySelSft                    0
#define LifSecUpKeySelMsk                    0x0000FFFFUL // [15:0]

#define LifSecUpMpcpOffset                   TkOnuReg(0x014B)
//  mpcp offset
#define LifSecUpMpcpOffsetSft                0
#define LifSecUpMpcpOffsetMsk                0x000FFFFFUL // [19:0]

#define LifSecUpPreamble                     TkOnuTable(0x014C)
#define LifSecUpPreambleCount                0x00000004UL

#define LifRxLineCodeErrCnt                  TkOnuReg(0x0150)

#define LifRxAggGatePkt                      TkOnuReg(0x0151)

#define LifRxAggGoodPkt                      TkOnuReg(0x0152)

#define LifRxAggGoodByte                     TkOnuReg(0x0153)

#define LifRxAggUnderszPkt                   TkOnuReg(0x0154)

#define LifRxAggOverszPkt                    TkOnuReg(0x0155)

#define LifRxAggCrc8Pkt                      TkOnuReg(0x0156)

#define LifRxAggFecPkt                       TkOnuReg(0x0157)

#define LifRxAggFecByte                      TkOnuReg(0x0158)

#define LifRxAggFecExcErrPkt                 TkOnuReg(0x0159)

#define LifRxAggNonfecGoodPkt                TkOnuReg(0x015A)

#define LifRxAggNonfecGoodByte               TkOnuReg(0x015B)

#define LifRxAggErrBytes                     TkOnuReg(0x015C)

#define LifRxAggErrZeroes                    TkOnuReg(0x015D)

#define LifRxAggNoErrBlks                    TkOnuReg(0x015E)

#define LifRxAggCorBlks                      TkOnuReg(0x015F)

#define LifRxAggUncorBlks                    TkOnuReg(0x0160)

#define LifRxAggErrOnes                      TkOnuReg(0x0161)

#define LifRxAggErrPkt                       TkOnuReg(0x0162)


#define LifTxPktCnt                          TkOnuReg(0x0163)

#define LifTxByteCnt                         TkOnuReg(0x0164)

#define LifTxNonFecPktCnt                    TkOnuReg(0x0165)

#define LifTxNonFecByteCnt                   TkOnuReg(0x0166)

#define LifTxFecPktCnt                       TkOnuReg(0x0167)

#define LifTxFecByteCnt                      TkOnuReg(0x0168)

#define LifTxFecBlkCnt                       TkOnuReg(0x0169)

#define LifTxRptPktCnt                       TkOnuReg(0x016a)


#define LifFecLlidInt                        TkOnuReg(0x0170)

#define LifPonBerIntervThresh                TkOnuReg(0x0181)
//  BER interval 1ms max
#define LifPonBerIntervThreshCfRxIntvlSft    15
#define LifPonBerIntervThreshCfRxIntvlMsk    0xFFFF8000UL // [31:15]
//  BER threshold 10^-2 error rate
#define LifPonBerIntervThreshCfRxldSft       2
#define LifPonBerIntervThreshCfRxldMsk       0x00007FFCUL // [14:2]
//  BER interrupt control
#define LifPonBerIntervThreshCfRxCntrlSft    0
#define LifPonBerIntervThreshCfRxCntrlMsk    0x00000003UL // [1:0]


#define LifLaserMonCtrl                      TkOnuReg(0x01d0)
#define LifLaserMonRstN                      0x00000001UL // offset 0

#define LifLaserMonIntStat                   TkOnuReg(0x01d1)
#define LifLaserMonLaserOffInt               0x00000001UL // offset 0
#define LifLaserMonLaserOnMaxInt             0x00000002UL // offset 1

#define LifLaserMonIntMask                   TkOnuReg(0x01d2)
#define LifLaserMonLaserOffIntMask           0x00000001UL // offset 0
#define LifLaserMonLaserOnMaxIntMask         0x00000002UL // offset 1

#define LifLaserMonMaxThres                  TkOnuReg(0x01d3)


#if defined(__cplusplus)
}
#endif

#endif // LifRegs.h

