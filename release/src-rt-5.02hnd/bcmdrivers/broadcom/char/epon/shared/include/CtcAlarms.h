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


#if !defined(CtcAlarms_h)
#define CtcAlarms_h
////////////////////////////////////////////////////////////////////////////////
/// \file CtcAlarms.h
/// \brief Ctc Alarm conditions and reporting
/// \author Chen Lingyong
/// \date Nov 2, 2010
///
/// The Ctc Alarms code polls Ctc alarm to decide whether to raise/clear Ctc
/// alarm, it will first push the Ctc alarm to message queue, then send the
/// Ctc alarm one by one by pop the Ctc Alarm from message queue.
///
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"
#include "EponStats.h"
#include "CtcOam.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern const IeeeOui  KnownVendorOui[];
#define TeknovusOui     (KnownVendorOui[OuiTeknovus])
#define TekOuiVal       0x000DB6UL
#define CtcOui          (KnownVendorOui[OuiCtc])
#define CtcOuiVal       0x111111UL


/// standard OAM opcodes
typedef enum
    {
    OamOpInfo,
    OamOpEventNotification,
    OamOpVarRequest,
    OamOpVarResponse,
    OamOpLoopback,
    OamLegacyOpVendorExt     = 0x80,
    OamLegacyOpPingRequest   = 0x8B,
    OamLegacyOpPingResponse  = 0x8C,
    OamOpVendorOui           = 0xfe
    } OamOpcode;

////////////////////////////////////////////////////////////////////////////////
// This spefic value presents the Ctc alarm don't contain the alam info
////////////////////////////////////////////////////////////////////////////////
#define OamCtcAlmNoInfo     0x7FFFFFFFUL
#define CtcPonPerfAlmNum 32
#define CtcEthPerfAlmNum 34


#define OamCtc30PonIfAlmNum ((OamCtcPonIfAlarmEnd30 - OamCtcPonIfAlarmEnd) + 1)


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm State
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    BOOL                raised;
    }CtcAlmState;


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm Info
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    CtcAlmState         onu[OamCtcOnuAlarmNums];
    CtcAlmState         ponIf[OamCtcPonIfAlarmNums];
    } PACK CtcAlmInfo;


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm Content
////////////////////////////////////////////////////////////////////////////////
typedef enum
    {
    AlmInfoLen0         = 0,
    AlmInfoLen4         = 4,
    AlmInfoLen8         = 8
    }AlmInfoLen_e;

typedef U8 AlmInfoLen;

typedef struct
    {
    BOOL                raised;
    OamCtcAlarmId       almId;
    U8                  inst;
    AlmInfoLen          infoLen;
    U64                 value;
    } PACK CtcAlmCont;

typedef struct
    {
    // alarm raise threshold
    U32        	CtcAlmRaise;
    U32        	CtcAlmClear;
	//alarm report mode
	BOOL        CtcAlmFlag;
    } PACK CtcAlmMonThd;




#define CtcAlarmBlockLen    0x100

////////////////////////////////////////////////////////////////////////////////
/// OamCtcAlarmAdminState - the CTC alarm admin state
///
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    U64                 onu;
    U64                 ponIf[CurOnuNumPonPorts];
    } PACK CtcAlarmAdminState;

extern CtcAlmMonThd ctc30AlmThd[OamCtc30PonIfAlmNum];


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmIdValid - Check if the alarm ID is valid for this ONU
///
/// \param obj        the CTC object type
/// \param alarmId    alarm Id
///
/// \return
/// TRUE if this alamId is valid or FALSE
////////////////////////////////////////////////////////////////////////////////
extern
BOOL CtcAlmIdValid(U16  alarmId);


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmAdminStateGet - Get Alarm Admin State
///
///
/// This function gets alarm admin state from alarm id
///
/// Parameters:
/// \param port     port number
/// \param AlmId        alarm Id
///
/// \return:
///           Onu admin state
////////////////////////////////////////////////////////////////////////////////
extern
BOOL CtcAlmAdminStateGet(OamCtcAlarmId almId);


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmAdminStateSet - Set Alarm Admin State
///
/// This function sets alarm admin state from alarm id
///
/// \param  port     TkOnuEthPort
/// \param  almId    TkOnuEthPort
/// \param  config   TkOnuEthPort
///
/// \return:
/// TRUE if success, otherwise FALSE
///////////////////////////////////////////////////////////////////////////////
extern
BOOL CtcAlmAdminStateSet(OamCtcAlarmId almId,U32 config);


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmSetCond:  indicates ctc alarm condition has occurred
///
 // Parameters:
/// \param almId Ctc alarm Id
/// \param inst Instance of that alarm Id
/// \param infoLen The length of the info
/// \param value current alarm Id value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcAlmSetCond (OamCtcAlarmId almId, 
    AlmInfoLen infoLen, U64 value) ;


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmClrCond:  indicates ctc alarm condition has gone away
///
 // Parameters:
/// \param almId Ctc alarm Id
/// \param inst Instance of that alarm Id
/// \param infoLen The length of the info
/// \param value current alarm Id value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcAlmClrCond (OamCtcAlarmId almId, 
    AlmInfoLen infoLen, U64 value) ;



////////////////////////////////////////////////////////////////////////////////
/// \brief  Send Ctc PON_IF Switch Alarm Message
///
/// \param None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcAlmPonIfSwitchNotify(void);


////////////////////////////////////////////////////////////////////////////////
/// CtcPerfMonAlmNotify - Notify Performance monitoring alarm
/// 
/// Parameters:
/// \param almId     alarm id
/// \param pAlmThd   pointer to alarm thershold
/// \param current   pointer to current value from performance monitoring
/// \param port      Ethernet port number
/// 
/// \return: None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcPerfMonAlmNotify(CtcAlmMonThd * pAlmThd,
								const U64 * pcurrent,
								OamCtcAlarmId almId);


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmPoll-Poll Ctc Alarm, Message Queue and Send Ctc Alarm
///
/// This function polls the Ctc alarm, message queue and send the Ctc alarm.
/// The polling mustrun in less than 500ms timer.
///
/// Parameters:
/// \param None
///
/// \return:
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcAlmPoll (void);

extern
void OamCtcEvtInit(void);

extern
BOOL CtcEvtThSet(U8 index, CtcAlmMonThd *threshold);

extern
BOOL CtcEvtThGet(U8 index, CtcAlmMonThd *threshold);


#if defined(__cplusplus)
}
#endif

#endif // CtcAlarms.h

