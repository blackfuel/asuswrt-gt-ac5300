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

#include <linux/string.h>
#include <linux/jiffies.h>

#include "bcm_epon_cfg.h"

#include "OsAstMsgQ.h"
#include "EponUser.h"
#include "EponCtrl.h"
#include "CtcAlarms.h"

static U32  osAssertDbase[OsAstNums];
char const *  condNames[OsAstNums] = 
    {
    // per-ONT alarms  
    "Ont1GDnRegistered",
    "Ont1GDnActivity",
    "Ont10GDnRegistered",
    "Ont10GDnActivity",
    "Ont1GUpRegistered",
    "Ont1GUpActivity",
    "Ont10GUpRegistered",
    "Ont10GUpActivity",
    "OntSingleLinkReg",
    "OntMultiLinkReg",

    "OntFecUp",
    "OntFecDn",
    "OntProtSwitch",
    "OntRogueOnu",
    // EPON port alarms             
    "EponLos",
    "EponLostSync",
    "EponStatThreshold",
    "EponActLed",
    "EponStandbyLos",
    "EponLaserShutdownTemp",
    "EponLaserShutdownPerm",
    "EponLaserOn",
    // per-LLID alarms
    "LinkNoGates",
    "LinkUnregistered",
    "LinkOamTimeout",
    "LinkOamDiscComplete",
    "LinkStatThreshold",
    "LinkKeyExchange",
    "LinkLoopback",
    "LinkAllDown",
    "LinkDisabled",
    "LinkRegStart",
    "LinkRegSucc",
    "LinkRegFail",
    "LinkRegTimeOut"
    };



static
void OnuOsAssertSetCallBack(OnuAssertId id)
    {
    switch (id)
        {
        case OsAstAlmOntRogueOnu:
            /*To fill the action for Rogue ONU alarm here */

            break;
        default:
            break;
        }
    
    return ;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief Set an assertion
///
/// This function sets an assertion state to TRUE.
///
/// \param id Assertion to raise
/// \param inst Instance number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void OnuOsAssertSet (OnuAssertId id, U8 inst)
    {
    U16  idx = id;
    if (idx != OnuAssertIdInVld)
        {
        osAssertDbase[idx] |= (1UL << inst);        
        OnuOsAssertSetCallBack(idx);
        }
    } // OnuOsAssertSet



////////////////////////////////////////////////////////////////////////////////
/// \brief Clean an assertion
///
/// This function sets an assertion state to FALSE.
///
/// \param id Assertion to clear
/// \param inst Instance number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void OnuOsAssertClr (OnuAssertId id, U8 inst)
    {
    U16  idx = id;

    if (idx != OnuAssertIdInVld)
        {
        osAssertDbase[idx] &= ~(1UL << inst);
        }
    } // OnuOsAssertClr


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
//extern
BOOL OnuOsAssertGet (OnuAssertId id, U8 inst)
    {
    U16 FAST idx = id;

    if (idx != OnuAssertIdInVld)
        {
        return (osAssertDbase[idx] & (1UL << inst)) != 0;
        }

    return FALSE;
    } // OnuOsAssertGet



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
//extern
void OsAstMsgQSet (OnuAssertId id, U8 inst, U8 stat) 
    {
    U8 buffer[30];
    EponAlarmPara *tkevent = (EponAlarmPara *)buffer;
    EponAlarmData   *data  = (EponAlarmData *)(buffer+sizeof(EponAlarmHeader));	
    if (!OnuOsAssertGet(id, inst))
        {
        OnuOsAssertSet(id, inst);
        memset(buffer,0,sizeof(buffer));
        tkevent->hd.count = 1;
        memcpy(tkevent->hd.oui,&TeknovusOui.byte[0],sizeof(tkevent->hd.oui));
        data->type = 1;//BCM
        data->almid   = id;
        data->raise   = TRUE;
        data->inst    = inst;
        data->infoLen = 1;
        data->value   = stat;
        EponAlarmMsgSend(buffer,(sizeof(EponAlarmData)+ sizeof(EponAlarmHeader)));        
        }
    } // OsAstMsgQSet


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
//extern
void OsAstMsgQClr (OnuAssertId id, U8 inst, U8 stat) 
    {
    U8 buffer[30];
    EponAlarmPara *tkevent = (EponAlarmPara *)buffer;
    EponAlarmData *data = NULL;
    
    if (OnuOsAssertGet(id, inst))
        {
        OnuOsAssertClr(id, inst);
        memset(buffer,0,sizeof(buffer));
        
        memcpy(tkevent->hd.oui,&TeknovusOui.byte[0],sizeof(tkevent->hd.oui));
        tkevent->hd.count = 1;
        
        data = (EponAlarmData *)(buffer+sizeof(EponAlarmHeader));
        
        data->type = 1;//BCM
        data->almid   = id;
        data->raise   = FALSE;
        data->inst    = inst;
        data->infoLen = 1;
        data->value   = stat;
        EponAlarmMsgSend(buffer,(sizeof(EponAlarmData)+ 1));
        }
    } // OsAstMsgQClr


////////////////////////////////////////////////////////////////////////////////
/// OsAstMsgQInit:  Initialization for os assert message queue
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OsAstMsgQInit (void)
    {
    memset (osAssertDbase, 0, sizeof(osAssertDbase));
    }//OsAstMsgQInit

void OsAstMsgQExit (void)
    {
    ;
    }

void OsAstMsgQInfo (void)
    {
    OnuAssertId cond;
    U8  i;
    
    // show status
    printk ("\nAlarms Info\n");
    for (cond = OsAstAppBase; cond < OsAstNums; ++cond)
        {
        printk ("%25s (%d):", condNames[cond], cond);
        for (i = 0; i < TkOnuNumRxLlids; ++i)
            {
            printk(" ");
            if (OnuOsAssertGet(cond, i))
                {
                printk("!");
                }
            else
                {
                printk("-");
                }
            }  
        printk("\n");
        }   
    } // CmdInfo

TkOsTick TkOsSysTick (void)
    {
    return jiffies;
    } // TkOsSysTick

// end OsAstMsgQ.c

