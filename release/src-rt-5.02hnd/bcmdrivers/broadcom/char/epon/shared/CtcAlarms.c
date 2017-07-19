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

#include <linux/unistd.h>
#include <linux/string.h>
#include "Teknovus.h"
#include "Stream.h"
#include "CtcAlarms.h"
#include "OntDirector.h"
#include "OntmMpcp.h"
#include "EponUser.h"
#include "OsAstMsgQ.h"

CtcAlmMonThd ctc30AlmThd[OamCtc30PonIfAlmNum];

const IeeeOui  KnownVendorOui[OuiKnownCount] =
    {
    { { 0x00, 0x0d, 0xb6 } },       // Teknovus
    { { 0x11, 0x11, 0x11 } },       // CTC
    { { 0x00, 0x00, 0xd9 } },       // NTT
    { { 0x00, 0xd0, 0xcb } },       // Dasan
    { { 0x00, 0x10, 0x00 } },       // DPoE
    { { 0xAA, 0xAA, 0xAA } },       // Kt
    { { 0x00, 0x0C, 0xD5 } },       // PmcOui
    { { 0x00, 0x80, 0xC2 } }        // IEEE 802.1
    };


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm Queuing functions
////////////////////////////////////////////////////////////////////////////////
#define CtcAlmMsgQSize              32
#define CtcAlmMsgQMsk               0x1F
#define CtcAlmMsgQNext(idx)         (((idx)+1)&CtcAlmMsgQMsk)


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm Ont/PonIf/Eth Index
////////////////////////////////////////////////////////////////////////////////
#define CtcAlmOntIndex(alm)         ((alm) - OamCtcOnuAlarmBase)
#define CtcAlmPonIfIndex(alm)       ((alm) - OamCtcPonIfAlarmBase)
#define CtcAlmEthIndex(alm)         ((alm) - OamCtcPortAlarmBase)


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm number that raised by teknovus
////////////////////////////////////////////////////////////////////////////////
#define CtcAlmAstNums               2


////////////////////////////////////////////////////////////////////////////////
// Ctc Alarm Message Queue
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    CtcAlmCont  queue[CtcAlmMsgQSize];
    U8          front;
    U8          back;
    } PACK CtcAlmMsgQ;

//##############################################################################
//                     Ctc Alarm Management module
//##############################################################################

static
OamCtcAlarmId const  OamAlarmBase[] =
    {
    OamCtcOnuAlarmBase,
    OamCtcPonIfAlarmBase,
    OamCtcCardAlarmBase,
    OamCtcPortAlarmBase,
    OamCtcPotsAlarmBase,
    OamCtcE1AlarmBase
    };
static void CtcAlmClrState (OamCtcAlarmId almId);


////////////////////////////////////////////////////////////////////////////////
/// CtcAlarmAdminState - CTC alarm admin state information
///
/// This varible use the bitmap to show which alarm is allowed to report
////////////////////////////////////////////////////////////////////////////////
static
CtcAlarmAdminState       ctcAlarmAdminState;


////////////////////////////////////////////////////////////////////////////////
/// AlarmAdminTypeGet - Get Alarm Admin Type
///
/// This function gets alarm admin type from alarm id
///
/// \param AlmId        alarm Id
///
/// \return:
/// Onu admin type
////////////////////////////////////////////////////////////////////////////////
static
OamCtcAlarmType AlarmAdminTypeGet(OamCtcAlarmId almId)
    {
    return (OamCtcAlarmType)(almId/CtcAlarmBlockLen);
    } // AlarmAdminTypeGet


////////////////////////////////////////////////////////////////////////////////
/// AlarmAdminStateMaskGet - Get Alarm Admin State Mask
///
/// This function gets alarm admin state bit mask from alarm id
///
/// Parameters:
/// \param AlmId        alarm Id
///
/// \return:
///           Onu admin state bit mask
////////////////////////////////////////////////////////////////////////////////
static
U64 AlarmAdminStateMaskGet(OamCtcAlarmId almId)
    {
    OamCtcAlarmType  almType;

    almType = AlarmAdminTypeGet(almId);
    return ((U64)CtcAlarmBase<<(almId - OamAlarmBase[almType]));
    }//AlarmAdminStateMaskGet


////////////////////////////////////////////////////////////////////////////////
/// AlarmAdminStateTypeGet- Get Alarm Admin State Type
///
/// This function gets alarm admin state type from alarm id
///
/// \param port     port number
/// \param AlmId    alarm Id
///
/// \return:
/// Onu admin state
////////////////////////////////////////////////////////////////////////////////
static
U64  * AlarmAdminStateTypeGet(OamCtcAlarmId almId)
	{
	return (U64 *)&(ctcAlarmAdminState.ponIf[0]);   
	    
	} // AlarmAdminStateTypeGet


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmIdValid - Check if the alarm ID is valid for this ONU
///
/// \param obj        the CTC object type
/// \param alarmId    alarm Id
///
/// \return
/// TRUE if this alamId is valid or FALSE
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL CtcAlmIdValid(U16  alarmId)
    {    
    if((alarmId >= OamCtcPonIfAlarmBase) &&
       (alarmId <= OamCtcPonIfAlarmEnd30))
        {
        return TRUE;
        }       
   
    return FALSE;
    } // CtcAlmIdValid


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
//extern
BOOL CtcAlmAdminStateGet(OamCtcAlarmId almId)
    {
    U64  bitMask;
    U64  *almState;

    bitMask = AlarmAdminStateMaskGet (almId);
    almState = AlarmAdminStateTypeGet (almId);
    if (almState != NULL)
        {
        return TestBitsSet(*almState, bitMask);
        }

    return FALSE;
    } // CtcAlmAdminStateGet


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
//extern
BOOL CtcAlmAdminStateSet(OamCtcAlarmId almId,U32 config)
    {
    U64  bitMask;
    U64  *almState;

    bitMask = AlarmAdminStateMaskGet (almId);
    almState = AlarmAdminStateTypeGet (almId);
    if (almState != NULL)
        {
        if (config == OamCtcActionDisable)
            {
            *almState &= ~bitMask;
            return TRUE;
            }
        else if (config == OamCtcActionEnable)
            {
            if (!CtcAlmAdminStateGet(almId))
                {
                CtcAlmClrState(almId);
                }
            *almState |= bitMask;
            return TRUE;
            }
        else
            {
            //for lint
            }
        }

    return FALSE;
    } // CtcAlmAdminStateSet


//##############################################################################
//                     Ctc Alarm raise and clear machanism
//##############################################################################

////////////////////////////////////////////////////////////////////////////////
// Static Varible
////////////////////////////////////////////////////////////////////////////////
static CtcAlmMsgQ  ctcAlmMsgQ;
static CtcAlmInfo  ctcAlmInfo;


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmMakeCont: Make raise, alarm Id, instance and value to alarm Context.
///
/// This function makes raise, alarm Id, instance and value to alarm Context.
///
/// Parameters:
/// \param cont alarm content to add to queue
/// \param raised raised flag
/// \param almId Ctc alarm Id
/// \param inst Instance of that alarm Id
/// \param infoLen The length of the info
/// \param value current alarm Id value
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void CtcAlmMakeCont (CtcAlmCont * almCont,
                BOOL raised,
                OamCtcAlarmId almId,
                AlmInfoLen infoLen,
                U64 value )
    {
    almCont->raised = raised;
    almCont->almId  = almId;
    almCont->infoLen = infoLen;
    almCont->value  = value;
    } // CtcAlmMakeCont


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmPushMsgQ: Add another alarm content to an message queue
///
/// This function will attempt to add an alarm content to an message queue.
/// If the queue is full the function will return FALSE.  It will return TRUE
/// if the entry was added.
///
/// Parameters:
/// \param cont alarm content to add to queue
///
/// \return
/// TRUE if operation was successful
////////////////////////////////////////////////////////////////////////////////
static
BOOL CtcAlmPushMsgQ (const CtcAlmCont  *cont)
    {
    if (CtcAlmMsgQNext(ctcAlmMsgQ.back) != ctcAlmMsgQ.front)
        {
        memcpy (&ctcAlmMsgQ.queue[ctcAlmMsgQ.back],
                cont,
                sizeof (CtcAlmCont));
        ctcAlmMsgQ.back = CtcAlmMsgQNext(ctcAlmMsgQ.back);

        return TRUE;
        }

    return FALSE;
    } // CtcAlmPushMsgQ


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmPopMsgQ: Remove an entry from an message queue
///
/// This function dequeues an alarm content from the message queue into the
/// supplied container.  If there was an alarm content pending the function
/// will return TRUE. FALSE will be returned if the message queue was empty.
///
/// Parameters:
/// \param cont alarm content to delete from queue
///
/// \return
/// TRUE if operation was successful
////////////////////////////////////////////////////////////////////////////////
static
BOOL CtcAlmPopMsgQ (CtcAlmCont  *cont)
    {
    if (ctcAlmMsgQ.front != ctcAlmMsgQ.back)
        {
        memcpy (cont,
                &ctcAlmMsgQ.queue[ctcAlmMsgQ.front],
                sizeof (CtcAlmCont));
        ctcAlmMsgQ.front = CtcAlmMsgQNext(ctcAlmMsgQ.front);

        return TRUE;
        }

    return FALSE;
    } // CtcAlmPopMsgQ


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmGetItem- Get Alarm Item Address
///
///
/// This function gets alarm item address
///
/// Parameters:
/// \param almId alarm Id
/// \param inst Instance of that alarm Id
///
/// \return:
///     Ctc Alarm Item Address
////////////////////////////////////////////////////////////////////////////////
static
CtcAlmState  * CtcAlmGetItem(OamCtcAlarmId almId)
    {
    OamCtcAlarmType  almType = (OamCtcAlarmType)(almId/0x100);

    switch(almType)
        {
        case OamCtcAlarmOnu:
            return &(ctcAlmInfo.onu[CtcAlmOntIndex(almId)]);
        case OamCtcAlarmPonIf:
            return &(ctcAlmInfo.ponIf[CtcAlmPonIfIndex(almId)]);
        default:
            return NULL;
        }
    }//CtcAlmGetItem


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmClrState:  clear ctc alarm condition state
///
 // Parameters:
/// \param almId Ctc alarm Id
/// \param inst Instance of that alarm Id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CtcAlmClrState (OamCtcAlarmId almId)
    {
    CtcAlmState  *state = CtcAlmGetItem(almId);

    state->raised = FALSE;
    }//CtcAlmClrState


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
//extern
void CtcAlmSetCond (OamCtcAlarmId almId,
    AlmInfoLen infoLen, U64 value) 
    {
    CtcAlmCont   almCont;
    CtcAlmState  *state = CtcAlmGetItem(almId);

    if (!state->raised)
        {
        state->raised = TRUE;
        CtcAlmMakeCont(&almCont, TRUE, almId,infoLen, value);
        (void)CtcAlmPushMsgQ(&almCont);
        }
    }//CtcAlmSetCond
    

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
//extern
void CtcAlmClrCond (OamCtcAlarmId almId, 
    AlmInfoLen infoLen, U64 value) 
    {
    CtcAlmCont  almCont;
    CtcAlmState  *state = CtcAlmGetItem(almId);

    if (state->raised)
        {
        state->raised = FALSE;
        CtcAlmMakeCont(&almCont, FALSE, almId, infoLen, value);
        (void)CtcAlmPushMsgQ(&almCont);
        }
    }//CtcAlmClrCond


static 
BOOL CtcAlmMsg(const CtcAlmCont  * cont,U8 count)
    {
    U8 i;
    U8 buffer[1000];
    EponAlarmPara *ctcalm = (EponAlarmPara *)buffer;
    EponAlarmData *data  = (EponAlarmData *)(buffer+sizeof(EponAlarmHeader));
    for (i = 0;i < count;i++)
        {
        if (!CtcAlmAdminStateGet(cont->almId))
            {
            return FALSE;
            }
        }
    memset(buffer,0,sizeof(buffer));
    ctcalm->hd.count = count;
    memcpy(ctcalm->hd.oui,&CtcOui,sizeof(ctcalm->hd.oui));
    for (i = 0;i < count;i++)
        {
        data->type    = 2;//CTC
        data->almid   = cont[i].almId;
        data->raise   = cont[i].raised;
        data->inst    = cont[i].inst;
        data->infoLen = cont[i].infoLen;
        data->value   = cont[i].value;
        data++;
        }
    EponAlarmMsgSend(buffer,
    	(sizeof(EponAlarmData)*count + sizeof(EponAlarmHeader)));
    return TRUE;
    }//CtcAlmMsg

////////////////////////////////////////////////////////////////////////////////
//extern
void CtcAlmPonIfSwitchNotify(void)
    {
    CtcAlmCont  almCont;
    almCont.raised   = TRUE;
    almCont.almId    = OamCtcAttrPonIfSwitch;
    almCont.inst    = 0;
    almCont.infoLen = AlmInfoLen4;
    almCont.value   = (U64)OamCtcPonIfSwichAlarmSigLos;
    (void)CtcAlmMsg(&almCont,1);
    } // CtcAlmPonIfSwitchNotify



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
void CtcPerfMonAlmNotify
    (CtcAlmMonThd  * pAlmThd,
     const U64  *pcurrent,
     OamCtcAlarmId almId)
    {
    U32  high = (U32)((*pcurrent >> 32)& 0xFFFFFFFFUL);
    U32  low  = (U32)(*pcurrent & 0xFFFFFFFFUL);


    //alarm stats enable . only send clear alarm.
    if (CtcAlmAdminStateGet(almId))
        {
        // Clear the alarm if the current value less than the threshold.
        if ((low <= pAlmThd->CtcAlmClear) && (0 == high))
            {
            if (pAlmThd->CtcAlmFlag == TRUE)
                {
                CtcAlmClrCond(almId,AlmInfoLen8, *pcurrent);
                }
            pAlmThd->CtcAlmFlag = FALSE;
            }

        else
            {
            if ((low >= pAlmThd->CtcAlmRaise) || (high > 0))
                {
                // Raise the alarm if the current value exceeds the threshold.
                if (pAlmThd->CtcAlmFlag == FALSE)
                    {
                    CtcAlmSetCond(almId, AlmInfoLen8, *pcurrent);
                    }
                pAlmThd->CtcAlmFlag = TRUE;
                }
            }
        }

    return;
    }


////////////////////////////////////////////////////////////////////////////////
/// CtcAlmPoll-Poll Ctc Alarm, Message Queue and Send Ctc Alarm
///
/// This function polls the Ctc alarm, message queue and send the Ctc alarm.
/// The polling must run in less than 500ms timer.
///
/// Parameters:
/// \param None
///
/// \return:
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void CtcAlmPoll (void)
    {
    U8 i,j = 0;
    CtcAlmCont  cont[CtcAlmMsgQSize];
    for (i = 0;i < CtcAlmMsgQSize;i++)
        {
        if (CtcAlmPopMsgQ(&cont[j]))
            {
            j++;	        
            }
        }
    if (j != 0)
        {
        (void)CtcAlmMsg(&cont[0],j);
        }
    }//CtcAlmPoll



//////////////////////////
void OamCtcEvtInit(void)
    {
    U16 i;
    for (i = 0;i < OamCtc30PonIfAlmNum;i++)
        {
        ctc30AlmThd[i].CtcAlmRaise = 0xFFFFFFFFUL;
        ctc30AlmThd[i].CtcAlmClear = 0xFFFFFFFFUL;
        ctc30AlmThd[i].CtcAlmFlag  = FALSE;
        } 
	memset(&ctcAlarmAdminState,0,sizeof(CtcAlarmAdminState));
	memset(&ctcAlmMsgQ,0,sizeof(CtcAlmMsgQ));
	memset(&ctcAlmInfo,0,sizeof(CtcAlmInfo));
	
    }


BOOL CtcEvtThSet(U8 index, CtcAlmMonThd *threshold)
    {
    if (index >=  OamCtc30PonIfAlmNum)
        {
        return FALSE;
        }
    memcpy(&ctc30AlmThd[index],threshold,sizeof(CtcAlmMonThd));
    return TRUE;
    }


BOOL CtcEvtThGet(U8 index, CtcAlmMonThd *threshold)
    {
    if (index >=  OamCtc30PonIfAlmNum)
        {
        return FALSE;
        }
    memcpy(threshold,&ctc30AlmThd[index],sizeof(CtcAlmMonThd));
    return TRUE;
    }

//end CtcAlarms.c

