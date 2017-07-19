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



#if !defined(CtcOam_h)
#define CtcOam_h
////////////////////////////////////////////////////////////////////////////////
/// \file CtcOam.c
/// \brief China Telecom extended OAM generic module
/// \author Jason Armstrong
/// \author Joshua Melcon
/// \date March 2, 2006
///
/// \todo
/// Finish this header.
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"


#if defined(__cplusplus)
extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////
/// OamCtcAttr - OAM TLV Leaf Attribute Identifiers
///
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    ////////////////////////////////////////////////////////////////////////////
    // ONU Object Class Attributes, Leaf 0x0001-0x000F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrOnuSn             = 0x0001,
    OamCtcAttrFirmwareVer       = 0x0002,
    OamCtcAttrChipsetId         = 0x0003,
    OamCtcAttrOnuCap            = 0x0004,
    OamCtcAttrPowerMonDiagnosis = 0x0005,
    OamCtcAttrServiceSla        = 0x0006,
    OamCtcAttrOnuCap2           = 0x0007,
    OamCtcAttrOnuHoldover       = 0x0008,
    OamCtcAttrMxUMngGlobalParam = 0x0009,
    OamCtcAttrMxUMngSnmpParam   = 0x000A,
    OamCtcAttrActPonIFAdmin     = 0x000B,
    OamCtcAttrOnuCap3           = 0x000C,
    OamCtcAttrPowerSavingCap    = 0x000D,
    OamCtcAttrPowerSavingCfg    = 0x000E,
    OamCtcAttrOnuProtParam      = 0x000F,
    OamCtcAttrOnuTxPower        = 0x00A1,
    OamCtcAttrOnuMacAgingTime   = 0x00A4,

    ////////////////////////////////////////////////////////////////////////////
    // Port Object Class Attributes, Leaf 0x0011-0x001F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrEthLinkState      = 0x0011,
    OamCtcAttrEthPortPause      = 0x0012,
    OamCtcAttrEthPortPolice     = 0x0013,
    OamCtcAttrVoipPort          = 0x0014,
    OamCtcAttrE1Port            = 0x0015,
    OamCtcAttrEthPortDsRateLimit= 0x0016,
    OamCtcAttrPortLoopDetect    = 0x0017,
    OamCtcAttrPortDisableLooped = 0x0018,

    ////////////////////////////////////////////////////////////////////////////
    // VLAN Object Class Attributes, Leaf 0x0021-0x002F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrVlan              = 0x0021,

    ////////////////////////////////////////////////////////////////////////////
    // QoS Object Class Attributes, Leaf 0x0031-0x003F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrClassMarking      = 0x0031,

    ////////////////////////////////////////////////////////////////////////////
    // Mcast Object Class Attribute, Leaf 0x0041-0x004F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrMcastVlan         = 0x0041,
    OamCtcAttrMcastStrip        = 0x0042,
    OamCtcAttrMcastSwitch       = 0x0043,
    OamCtcAttrMcastCtrl         = 0x0044,
    OamCtcAttrGroupMax          = 0x0045,

    ////////////////////////////////////////////////////////////////////////////
    // Fast Leave Class Attribute, Leaf 0x0046-0x0047
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrFastLeaveAbility  = 0x0046,
    OamCtcAttrFastLeaveState    = 0x0047,

    ////////////////////////////////////////////////////////////////////////////
    // LLID object Class Attribute, Leaf 0x0051-0x005F
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrLlidQueueConfig   = 0x0051,

    ////////////////////////////////////////////////////////////////////////////
    // Alarm Class Attribute, Leaf 0x0046-0x0047
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrAlarmAdminState   = 0x0081,
    OamCtcAttrPowerMonThreshold = 0x0082,

    ////////////////////////////////////////////////////////////////////////////
    // VoIP Class Attribute, Leaf 0x0061 - 0x006D
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrIadInfo           = 0x0061,
    OamCtcAttrVoIPFirst             = OamCtcAttrIadInfo,
    OamCtcAttrGlobalParaConfig  = 0x0062,
    OamCtcAttrH248ParaConfig    = 0x0063,
    OamCtcAttrH248UserTidInfo   = 0x0064,
    OamCtcAttrH248RtpTidConfig  = 0x0065,
    OamCtcAttrH248RtpTidInfo    = 0x0066,
    OamCtcAttrSipParaConfig     = 0x0067,
    OamCtcAttrSipUserParaConfig = 0x0068,
    OamCtcAttrFaxModemConfig    = 0x0069,
    OamCtcAttrH248IadOperationStatus    = 0x006A,
    OamCtcAttrPotsStatus        = 0x006B,
    OamCtcAttrIadOperation      = 0x006C,
    OamCtcAttrSipDigitMap       = 0x006D,
    OamCtcAttrVoIPLast                  = 0x007F,

    ////////////////////////////////////////////////////////////////////////////
    // CMC Attribute
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrRfTransceiverDiag = 0x0100,
    OamCtcAttrCnuList           = 0x0101,
    OamCtcAttrDnRfConfig        = 0x0102,
    OamCtcAttrUpRfConfig        = 0x0103,
    OamCtcAttrMaxUpRfAttenu     = 0x0104,
    OamCtcAttrMoveCnuDn         = 0x0105,
    OamCtcAttrMoveCnuUp         = 0x0106,
    OamCtcAttrDownSla           = 0x0107,

    ////////////////////////////////////////////////////////////////////////////
    // Performance Attribute
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrPrfrmMontrStatus  = 0x00B1,
    OamCtcAttrPrfrmCurrtData    = 0x00B2,
    OamCtcAttrPrfrmHistrData    = 0x00B3,

    OamCtcAttrU16               = 0x7FFF
    } OamCtcAttr_e;

typedef U16 OamCtcAttr;
////////////////////////////////////////////////////////////////////////////////
/// OamCtcAlarmId - OAM CTC AlarmId
///
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    ////////////////////////////////////////////////////////////////////////////
    // ONU Object Class Attributes, AlarmId 0x0001-0x00FF
    ////////////////////////////////////////////////////////////////////////////
    OamCtcOnuAlarmBase                  = 0x0001,
    OamCtcAttrEquipmentAlarm            = OamCtcOnuAlarmBase,
    OamCtcAttrPowerAlarm                = 0x0002,
    OamCtcAttrBatteryMissing            = 0x0003,
    OamCtcAttrBatteryFailure            = 0x0004,
    OamCtcAttrBatteryVoltLow            = 0x0005,
    OamCtcAttrPhysicalInstusionAlarm    = 0x0006,
    OamCtcAttrOnuSelfTestFailure        = 0x0007,
    OamCtcAttrOnuTempHighAlarm          = 0x0009,
    OamCtcAttrOnuTempLowAlarm           = 0x000A,
    OamCtcAttrIadConnectionFailure      = 0x000B,
    OamCtcAttrPonIfSwitch               = 0x000C,
    OamCtcSleepStatusUpdate             = 0x000D,
    OamCtcOnuAlarmEnd                   = OamCtcSleepStatusUpdate,
    OamCtcOnuAlarmNums                  = OamCtcOnuAlarmEnd
                                        - OamCtcOnuAlarmBase + 1,

    ////////////////////////////////////////////////////////////////////////////
    // PON IF Object Class Attributes, AlarmId 0x0101-0x01FF
    ////////////////////////////////////////////////////////////////////////////
    //********* Alarm Section***********//
    OamCtcPonIfAlarmBase                = 0x0101,
    //======== RxPower ==========//
    OamCtcAttrPowerMonRxPowerAlmHigh    = OamCtcPonIfAlarmBase,
    OamCtcAttrPowerMonRxPowerAlmLow     = 0x0102,
    //======== TxPower ==========//
    OamCtcAttrPowerMonTxPowerAlmHigh    = 0x0103,
    OamCtcAttrPowerMonTxPowerAlmLow     = 0x0104,
    //========= TxBias ==========//
    OamCtcAttrPowerMonTxBiasAlmHigh     = 0x0105,
    OamCtcAttrPowerMonTxBiasAlmLow      = 0x0106,
    //=========== Vcc ==========//
    OamCtcAttrPowerMonVccAlmHigh        = 0x0107,
    OamCtcAttrPowerMonVccAlmLow         = 0x0108,
    //======= Temperature ========//
    OamCtcAttrPowerMonTempAlmHigh       = 0x0109,
    OamCtcAttrPowerMonTempAlmLow        = 0x010A,
    //********* Warn Section***********//
    //======== RxPower ==========//
    OamCtcAttrPowerMonRxPowerWarnHigh   = 0x010B,
    OamCtcAttrPowerMonRxPowerWarnLow    = 0x010C,
    //======== TxPower ==========//
    OamCtcAttrPowerMonTxPowerWarnHigh   = 0x010D,
    OamCtcAttrPowerMonTxPowerWarnLow    = 0x010E,
    //========= TxBias ==========//
    OamCtcAttrPowerMonTxBiasWarnHigh    = 0x010F,
    OamCtcAttrPowerMonTxBiasWarnLow     = 0x0110,
    //========== Vcc ===========//
    OamCtcAttrPowerMonVccWarnHigh       = 0x0111,
    OamCtcAttrPowerMonVccWarnLow        = 0x0112,
    //======= Temperature ========//
    OamCtcAttrPowerMonTempWarnHigh      = 0x0113,
    OamCtcAttrPowerMonTempWarnLow       = 0x0114,
    OamCtcPonIfAlarmEnd                 = OamCtcAttrPowerMonTempWarnLow,
    //Performance monitor
    OamCtcAttrDnStreamDropEvent         = 0x0115,
    OamCtcAttrUpStreamDropEvent         = 0x0116,
    OamCtcAttrDnStreamCRCErrFrames      = 0x0117,
    OamCtcAttrUpStreamCRCErrFrames      = 0x0118,
    OamCtcAttrDnStreamUndersizeFrames   = 0x0119,
    OamCtcAttrUpStreamUndersizeFrames   = 0x011A,
    OamCtcAttrDnStreamOversizeFrames    = 0x011B,
    OamCtcAttrUpStreamOversizeFrames    = 0x011C,
    OamCtcAttrDnStreamFragments         = 0x011D,
    OamCtcAttrUpStreamFragments         = 0x011E,
    OamCtcAttrDnStreamJabbers           = 0x011F,
    OamCtcAttrUpStreamJabbers           = 0x0120,
    OamCtcAttrDnStreamDiscards          = 0x0121,
    OamCtcAttrUpStreamDiscards          = 0x0122,
    OamCtcAttrDnStreamErrors            = 0x0123,
    OamCtcAttrUpStreamErrors            = 0x0124,
    OamCtcAttrDnStreamDropEventsWarn    = 0x0125,
    OamCtcAttrUpStreamDropEventsWarn    = 0x0126,
    OamCtcAttrDnStreamCRCErrFramesWarn  = 0x0127,
    OamCtcAttrUpStreamCRCErrFramesWarn  = 0x0128,
    OamCtcAttrDnStreamUndersizeFrmsWarn = 0x0129,
    OamCtcAttrUpStreamUndersizeFrmsWarn = 0x012A,
    OamCtcAttrDnStreamOversizeFrmsWarn  = 0x012B,
    OamCtcAttrUpStreamOversizeFrmsWarn  = 0x012C,
    OamCtcAttrDnStreamFragmentsWarn     = 0x012D,
    OamCtcAttrUpStreamFragmentsWarn     = 0x012E,
    OamCtcAttrDnStreamJabbersWarn       = 0x012F,
    OamCtcAttrUpStreamJabbersWarn       = 0x0130,
    OamCtcAttrDnStreamDiscardsWarn      = 0x0131,
    OamCtcAttrUpStreamDiscardsWarn      = 0x0132,
    OamCtcAttrDnStreamErrorsWarn        = 0x0133,
    OamCtcAttrUpStreamErrorsWarn        = 0x0134,
    OamCtcPonIfAlarmEnd30               = OamCtcAttrUpStreamErrorsWarn,
    OamCtcPonIfAlarmNums                = OamCtcPonIfAlarmEnd30
                                        - OamCtcPonIfAlarmBase + 1,

    ////////////////////////////////////////////////////////////////////////////
    // Card Object Class Attributes, AlarmId 0x0201-0x02FF
    ////////////////////////////////////////////////////////////////////////////
    OamCtcCardAlarmBase                 = 0x0201,
    OamCtcAttrCardAlarm                 = OamCtcCardAlarmBase,
    OamCtcAttrSelfTestFailure           = 0x0202,
    OamCtcCardAlarmEnd                  = OamCtcAttrSelfTestFailure,
    OamCtcCardAlarmNums                 = OamCtcCardAlarmEnd
                                        - OamCtcCardAlarmBase + 1,

    ////////////////////////////////////////////////////////////////////////////
    // Port Object Class Attributes, AlarmId 0x0301-0x05FF
    ////////////////////////////////////////////////////////////////////////////
    //********** Eth Section***********//
    OamCtcPortAlarmBase                 = 0x0301,
    OamCtcAttrEthPortAutoNegFailure     = OamCtcPortAlarmBase,
    OamCtcAttrEthPortLos                = 0x0302,
    OamCtcAttrEthPortFailure            = 0x0303,
    OamCtcAttrEthPortLoopback           = 0x0304,
    OamCtcAttrEthPortCongestion         = 0x0305,
    OamCtcPortAlarmEnd                  = OamCtcAttrEthPortCongestion,

    OamCtcAttrEthPortDnDropEvents       = 0x0306,
    OamCtcAttrEthPortUpDropEvents       = 0x0307,
    OamCtcAttrEthPortDnCRCErrFrms       = 0x0308,
    OamCtcAttrEthPortUpCRCErrFrms       = 0x0309,
    OamCtcAttrEthPortDnUndersizeFrms    = 0x030A,
    OamCtcAttrEthPortUpUndersizeFrms    = 0x030B,
    OamCtcAttrEthPortDnOversizeFrms     = 0x030C,
    OamCtcAttrEthPortUpOversizeFrms     = 0x030D,
    OamCtcAttrEthPortDnFragments        = 0x030E,
    OamCtcAttrEthPortUpFragments        = 0x030F,
    OamCtcAttrEthPortDnJabbers          = 0x0310,
    OamCtcAttrEthPortUpJabbers          = 0x0311,
    OamCtcAttrEthPortDnDiscards         = 0x0312,
    OamCtcAttrEthPortUpDiscards         = 0x0313,
    OamCtcAttrEthPortDnErrors           = 0x0314,
    OamCtcAttrEthPortUpErrors           = 0x0315,
    OamCtcAttrEthPortStatsChangeTimes   = 0x0316,
    OamCtcAttrEthPortDnDropEventsWarn   = 0x0317,
    OamCtcAttrEthPortUpDropEventsWarn   = 0x0318,
    OamCtcAttrEthPortDnCRCErrFrmsWarn   = 0x0319,
    OamCtcAttrEthPortUpCRCErrFrmsWarn   = 0x031A,
    OamCtcAttrEthPortDnUndersizeFrmsWarn= 0x031B,
    OamCtcAttrEthPortUpUndersizeFrmsWarn= 0x031C,
    OamCtcAttrEthPortDnOversizeFrmsWarn = 0x031D,
    OamCtcAttrEthPortUpOversizeFrmsWarn = 0x031E,
    OamCtcAttrEthPortDnFragmentsWarn    = 0x031F,
    OamCtcAttrEthPortUpFragmentsWarn    = 0x0320,
    OamCtcAttrEthPortDnJabbersWarn      = 0x0321,
    OamCtcAttrEthPortUpJabbersWarn      = 0x0322,
    OamCtcAttrEthPortDnDiscardsWarn     = 0x0323,
    OamCtcAttrEthPortUpDiscardsWarn     = 0x0324,
    OamCtcAttrEthPortDnErrorsWarn       = 0x0325,
    OamCtcAttrEthPortUpErrorsWarn       = 0x0326,
    OamCtcAttrEthPortStatChangeTimesWarn= 0x0327,
    OamCtcPortAlarmEnd30                = OamCtcAttrEthPortStatChangeTimesWarn,
    OamCtcPortAlarmNums                 = OamCtcPortAlarmEnd30
                                        - OamCtcPortAlarmBase + 1,

    //********* Pots Section***********//
    OamCtcPotsAlarmBase                 = 0x0401,
    OamCtcAttrPotsPortFailure           = OamCtcPotsAlarmBase,
    OamCtcPotsAlarmEnd                  = OamCtcAttrPotsPortFailure,
    OamCtcPotsAlarmNums                 = OamCtcPotsAlarmEnd
                                        - OamCtcPotsAlarmBase + 1,
    //********** E1 Section***********//
    OamCtcE1AlarmBase                   = 0x0501,
    OamCtcAttrE1PortFailure             = OamCtcE1AlarmBase,
    OamCtcAttrE1TimingUnlock            = 0x0502,
    OamCtcAttrE1Los                     = 0x0503,
    OamCtcE1AlarmEnd                    = OamCtcAttrE1Los,
    OamCtcE1AlarmNums                   = OamCtcE1AlarmEnd
                                        - OamCtcE1AlarmBase + 1,

    ////////////////////////////////////////////////////////////////////////////
    // CMC Events
    ////////////////////////////////////////////////////////////////////////////
    OamCtcAttrCnuRegEvent               = 0x0601,

    OamCtcAlarmInvalid                  = 0x7FFF
    }OamCtcAlarmId_e;

typedef U16 OamCtcAlarmId;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcOnuPowerAlarmCode - The Failure code of the CTC ONU Power alarm
///
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcOnuPowerAlarmPowerDown        = 0x00000001,
    OamCtcOnuPowerAlarmVoltOver         = 0x00000002,
    OamCtcOnuPowerAlarmVoltLess         = 0x00000003,

    OamCtcOnuPowerAlarmCodeNum
    } OamCtcOnuPowerAlarmCode;

typedef enum
    {
    OamCtcPonIfSwichAlarmSigLos         = 0x00000001,
    OamCtcPonIfSwichAlarmSigDegrade     = 0x00000002,
    OamCtcPonIfSwichAlarmHwFault        = 0x00000003,
    } OamCtcPonIfSwitchAlarmCode;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcAct - OAM TLV Leaf Action Identifiers
///
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcActReset              = 0x0001,
    OamCtcActSleepControl       = 0x0002,

    // New Attribute in CTC2.0, Leaf 0x0048
    OamCtcAttrFastLeaveAdminCtl = 0x0048,

    OamCtcActMultiLlidAdminCtl  = 0x0202,

    OamCtcActResetCard          = 0x0401,

    OamCtcActU16                = 0x7FFF
    } OamCtcAct_e;

typedef U16 OamCtcAct;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcKtExtAttr : KT extension attribute
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcKtExtAttrOnuDsShaping           = 0x0001,
    OamCtcKtExtAttrOnuMacLimit            = 0x0002,
    OamCtcKtExtAttrBlockOnu               = 0x0003,
    OamCtcKtExtAttrOnuDiagnostics         = 0x0004,
    OamCtcKtExtAttrOnuGetQueueDropCounter = 0x0005,
    OamCtcKtExtAttrEtherPortGetCounter    = 0x0011,
    OamCtcKtExtAttrEtherPortRstp          = 0x0012,
    OamCtcKtExtAttrLoopDetect             = 0x0013,
    OamCtcKtExtAttrGetOnuMpcpCounter      = 0x0014,
    } OamCtcKtExtAttr_e;

typedef U16 OamCtcKtExtAttr;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcKtExtAct : KT extension actions
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcKtExtActOnuCounterClear   = 0x0001,
    OamCtcKtExtActDefaultRestoreOnu = 0x0002,
    OamCtcKtExtActTxPowerOff        = 0x0003,
    } OamCtcKtExtAct_e;
typedef U16 OamCtcKtExtAct;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcFastLeaveMode - the CTC fast leave mode
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcIgmpSnoopNoFast       = 0x00000000,
    OamCtcIgmpSnoopFastLeave    = 0x00000001,
    OamCtcHostCtlNoFast         = 0x00000010,
    OamCtcHostCtlFastLeave      = 0x00000011
    } OamCtcFastLeaveMode;





////////////////////////////////////////////////////////////////////////////////
/// OamCtcActionU16 - the CTC action
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcActionU16Disable      = 0x0001,
    OamCtcActionU16Enable       = 0x0002
    } OamCtcActionU16_e;

typedef U16 OamCtcActionU16;
////////////////////////////////////////////////////////////////////////////////
/// OamCtcAction - the CTC action
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcActionDisable         = 0x00000001UL,
    OamCtcActionEnable          = 0x00000002UL
    } OamCtcAction;


////////////////////////////////////////////////////////////////////////////////
/// OamCtcAlarmState - the CTC alarm state
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcAlarmDisable          = 0x00000001UL,
    OamCtcAlarmEnable           = 0x00000002UL
    } OamCtcAlarmState;



////////////////////////////////////////////////////////////////////////////////
/// OamCtcObjType - the CTC object type
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcObjPort               = 0x0001,
    OamCtcObjCard,
    OamCtcObjLlid,
    OamCtcObjPon,
    OamCtcObjOnu                = 0xFFFF,
    OamCtcObjErr                = 0xABCD,
    OamCtcObjUnused             = 0x7FFF
    }OamCtcObjType_e;

typedef U16 OamCtcObjType;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcAlarmType - the CTC alarm type
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcAlarmOnu,
    OamCtcAlarmPonIf,
    OamCtcAlarmCard,
    OamCtcAlarmEth,
    OamCtcAlarmPots,
    OamCtcAlarmE1
    }OamCtcAlarmType_e;

typedef U8 OamCtcAlarmType;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcPortType - the CTC port type
///
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcPortEth               = 0x01,
    OamCtcPortVoIP,
    OamCtcPortADSL2,
    OamCtcPortVDSL2,
    OamCtcPortE1
    }OamCtcPortType_e;

typedef U8 OamCtcPortType;


//##############################################################################
// CTC OAM TLV Auxillary Type Definitions
//##############################################################################

#define OamCtcEponPortNum           0x00
#define OamCtcEthPortStart          0x01
#define OamCtcEthPortEnd            0x4F
#define OamCtcPotsPortStart         0x50
#define OamCtcPotsPortEnd           0x8F
#define OamCtcPotsPortExtLenStart   0x4050
#define OamCtcPotsPortExtLenEnd     0x408F
#define OamCtcE1PortStart           0x90
#define OamCtcE1PortEnd             0x9F
#define OamCtcE1PortExtLenStart     0x4090
#define OamCtcE1PortExtLenEnd       0x409F
#define OamCtcAllEthPorts           0xFF

#define OamCtcAlmOnuStart           0x0001
#define OamCtcAlmOnuEnd             0x00FF
#define OamCtcAlmPonIfStart         0x0101
#define OamCtcAlmPonIfEnd           0x01FF
#define OamCtcAlmCardStart          0x0201
#define OamCtcAlmCardEnd            0x02FF
#define OamCtcAlmEthStart           0x0301
#define OamCtcAlmEthEnd             0x03FF
#define OamCtcAlmPotsStart          0x0401
#define OamCtcAlmPotsEnd            0x04FF
#define OamCtcAlmE1Start            0x0501
#define OamCtcAlmE1End              0x05FF
#define OamCtcAllUniPorts           0xFFFF
#define OamCtcOnuInstNum            0xFFFFFFFFUL
#define OamCtcAlarmNoAlarmInfo      0x7FFFFFFFUL


////////////////////////////////////////////////////////////////////////////////
/// OamCtcPortNum - Port number used in CTC port object TLVs
///
/// There is an offset port number scheme for port messages in CTC OAM as
/// follows:
///
///     - 0x00 to 0x4F: Ethernet ports, EPON port is port 0
///     - 0x50 to 0x8F: VoIP ports
///     - 0x90 to 0x9F: E1 ports
////////////////////////////////////////////////////////////////////////////////
typedef U8 OamCtcPortNum;



#define CtcCmcOnuVidBit 0x800

#define onuCapV2OnuTypeMsk      0xff000000
#define onuCapV2OnuTypeSft      24

////////////////////////////////////////////////////////////////////////////////
/// OamCtcOnuProtectionType - ONU Protection Type
///
/// This type is used to present Onu protection type
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    OamCtcOnuUnSupport          = 0x00,
    OamCtcOnuTypeC              = 0x01,
    OamCtcOnuTypeD              = 0x02,
    OamCtcOnuTypeNum            = 0x03
    } OamCtcOnuProtectionType_e;

typedef U8 OamCtcOnuProtectionType;


#define CtcAlarmBase                0x00000001UL
#define CurOnuNumPonPorts           1


typedef enum
    {
    OamEventErrSymbolPeriod         = 1,
    OamEventErrFrameCount           = 2,
    OamEventErrFramePeriod          = 3,
    OamEventErrFrameSecSummary      = 4,
    OamEventErrVendor               = 0xfe,
    OamEventErrVendorOld            = 0xff
    } OamEventType_e;
typedef U8 OamEventType;
/// All TLVs start like this
typedef struct
    {
    OamEventType    type;
    U8              length;
    } PACK OamEventTlv;

/// An IEEE Organization Unique Identifier
typedef struct
    {
    U8  byte[3];
    } PACK IeeeOui;


/// Vendor-extended TLV
typedef struct
    {
    OamEventTlv     tlv;
    IeeeOui         oui;
    } PACK OamEventExt;


////////////////////////////////////////////////////////////////////////////////
/// OamCtcInstNumS - the CTC instance  number struct
///
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    U8                  portType;
    U8                  slotNum;
    U16                 portNum;
    } PACK OamCtcInstNumS;

////////////////////////////////////////////////////////////////////////////////
/// OamCtcInstNumU - the CTC instance number union
///
////////////////////////////////////////////////////////////////////////////////
typedef union
    {
    OamCtcInstNumS      sNum;
    U32                 uNum;
    } PACK OamCtcInstNumU;


////////////////////////////////////////////////////////////////////////////////
/// OamEventCtcAlarm - the CTC Alarm
///
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    OamEventExt         ext;
    U16                 objType;
    OamCtcInstNumU      instNum;
    U16                 almID;
    U16                 timeStamp;
    U8                  state;
    } PACK OamCtcEventTlvHead;


typedef struct
    {
    OamCtcEventTlvHead  head;
    U8                  info[6];
    U32                 dnPrimaryChannel;
    U32                 upPrimaryChannel;
    } PACK OamCtcEventAlarm;

#if defined(__cplusplus)
}
#endif
#endif // End of File CtcOam.h
