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

/*                      Copyright(c) 2011 Broadcom, Corp.                     */
#if !defined(OsAstMsgQ_h)
#define OsAstMsgQ_h
////////////////////////////////////////////////////////////////////////////////
/// \file OsAstMsgQ.h
/// \brief ONU
/// \author Lingyong Chen
/// \date May 24, 2011
///
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"

typedef struct{
    U8   type;// 1:BCM,2:CTC
    U16  almid;
    BOOL raise;
    U8   inst;
    U8   infoLen;
    U64  value;
} PACK EponAlarmData;

typedef struct{
    U8   oui[3];
    U8   count;
    U32  crc;
} PACK EponAlarmHeader;


typedef struct{
    EponAlarmHeader hd;
    EponAlarmData   *data;
} PACK EponAlarmPara;


// message queue fields
#define   MsgQRaiseSet      0x80000000
#define   MsgQRaiseMsk      0x80000000
#define   MsgQRaiseSft      31
#define   MsgQIdMsk         0x7fff0000
#define   MsgQIdSft         16
#define   MsgQInstMsk       0x0000ff00
#define   MsgQInstSft       8
#define   MsgQStatMsk       0x000000ff
#define   MsgQStatSft       0

typedef U32 TkOsTick;
#define TkOsMsPerTick   1

typedef enum
    {
    OsAstAppBase                    = 0x00, //change to 0x00 might break other alarms especially power loss
    ////////////////////////////////////////////////////////////////////////////
    /// Os Alarm Assert Id
    ////////////////////////////////////////////////////////////////////////////
    // per-ONT alarms  
    OsAstAlmOnt1GDnRegistered = OsAstAppBase,
    OsAstAlmOnt1GDnActivity,
    OsAstAlmOnt10GDnRegistered,
    OsAstAlmOnt10GDnActivity,
    OsAstAlmOnt1GUpRegistered,
    OsAstAlmOnt1GUpActivity,
    OsAstAlmOnt10GUpRegistered,
    OsAstAlmOnt10GUpActivity,
    OsAstAlmOntSingleLinkReg,
    OsAstAlmOntMultiLinkReg,

    OsAstAlmOntFecUp,
    OsAstAlmOntFecDn,
    OsAstAlmOntProtSwitch,
    OsAstAlmOntRogueOnu,

    // EPON port alarms             
    OsAstAlmEponLos,
    OsAstAlmEponLostSync,
    OsAstAlmEponStatThreshold,
    OsAstAlmEponActLed,
    OsAstAlmEponStandbyLos,
    OsAstAlmEponLaserShutdownTemp,
    OsAstAlmEponLaserShutdownPerm,
    OsAstAlmEponLaserOn,

    // per-LLID alarms
    OsAstAlmLinkNoGates,
    OsAstAlmLinkUnregistered,
    OsAstAlmLinkOamTimeout,
    OsAstAlmLinkOamDiscComplete,
    OsAstAlmLinkStatThreshold,
    OsAstAlmLinkKeyExchange,
    OsAstAlmLinkLoopback,
    OsAstAlmLinkAllDown,
    OsAstAlmLinkDisabled,
    OsAstAlmLinkRegStart,
    OsAstAlmLinkRegSucc,
    OsAstAlmLinkRegFail,
    OsAstAlmLinkRegTimeOut,


    OsAstNums,
    
    //extend besides previous alarm set
    OsAstAlmLinkFailSafeReset = 0xFE,                 //0xFE
   
    OsAstInvalid                    = 0xFF

    } OnuAssertId_e;

#define OnuAssertIdInVld    0xFF

typedef U8 OnuAssertId;

typedef struct
    {
    BOOL            raise;
    OnuAssertId     id;
    U8              inst;
    U8              stat;
    } PACK TkOsAstRec;


////////////////////////////////////////////////////////////////////////////////
/// \brief Get the current state of an assertion
///
/// This function gets the active state of an assertion.
///
/// \param id Assertion to get
/// \param inst Instance number
///
/// \return
/// TRUE if the assert is active, FALSE if not
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OnuOsAssertGet (OnuAssertId id, U8 inst);



////////////////////////////////////////////////////////////////////////////////
/// OsAstMsgQSet:  Set os assert into message queue
///
 // Parameters:
/// \param id   Onu assert id
/// \param inst Instance of that condition
/// \param stat stat id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OsAstMsgQSet (OnuAssertId id, U8 inst, U8 stat) ;


////////////////////////////////////////////////////////////////////////////////
/// OsAstMsgQClr:  Clear os assert from message queue
///
 // Parameters:
/// \param id   Onu assert id
/// \param inst Instance of that condition
/// \param stat stat id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OsAstMsgQClr (OnuAssertId id, U8 inst, U8 stat);


////////////////////////////////////////////////////////////////////////////////
/// OsAstMsgQInit:  Initialization for os assert message queue
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OsAstMsgQInit (void);
extern
void OsAstMsgQExit (void);

extern 
void OsAstMsgQInfo (void);
extern
TkOsTick TkOsSysTick (void);


#endif // End of file OsAstMsgQ.h

