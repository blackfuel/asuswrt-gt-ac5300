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


// geenric includes
#include "Teknovus.h"
#include "PonManager.h"
#include "PonConfigDb.h"
#include "Holdover.h"
#include "OntmMpcp.h"
#include "EponMac.h"
#include "EponUser.h"



#define HoldoverDebugLvl    DebugHoldOver


typedef enum
    {
    HoldoverRecover,
    HoldoverReady,
    HoldoverRunning,
    HoldoverFail
    } HoldoverState;


static HoldoverState state;
static rdpa_epon_holdover_t holdoverCfg;
static EponTimerModuleId holdovertimerId;


////////////////////////////////////////////////////////////////////////////////
/// HoldoverSetTime - Set ONU Holdover time
///
/// This function sets the time that an ONU will wait before resetting links.
/// The units are 1ms increments.  The value is stored in NVS.
///
/// Parameters:
/// \param time Holdover time in 10ms increments
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL HoldoverIsEnabled(void)
    {
    return holdoverCfg.time != 0;
    }

////////////////////////////////////////////////////////////////////////////////
/// HoldoverExpireTimer - Perform timer expiry operations
///
/// This function performs all cleanup or actions that need to be executed when
/// the holdover timer expires.  The behavior will be different depending on
/// whether or not the ONU has recovered from LOS before expiry.
///
/// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
static
void HoldoverExpireTimer (EponTimerModuleId moduleId)
    {
    if (state == HoldoverRunning)
    	{
	eponUsrDebug(HoldoverDebugLvl, 
                           ("HF:%u\n", TkOsSysTick() * TkOsMsPerTick));        
    	state = HoldoverFail;
    	}
    } // HoldoverExpireTimer
    

////////////////////////////////////////////////////////////////////////////////
/// HoldoverSetParams - Set ONU Holdover params
///
/// This function sets the time that an ONU will wait before resetting links.
/// The units are 1ms increments.  The value is stored in NVS.
///
/// Parameters:
/// \param time     Holdover time in 1ms increments
/// \param flags    Behavioral modifiers
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void HoldoverSetParams (U16 time, rdpa_epon_holdover_flags flags)
    {
    holdoverCfg.time = time;
    holdoverCfg.flags = flags;
    } // HoldoverSetParams


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetTime - Get ONU holdover time
///
/// This function returns the amount of time the ONU should wait after loss of
/// signal or loss of gate to reset the links.  The time is in 1ms increments.
///
/// Parameters:
///
/// \return
/// Current ONU holdover time
////////////////////////////////////////////////////////////////////////////////
//extern
U16 HoldoverGetTime (void)
    {
    return holdoverCfg.time;
    } // HoldoverGetTime


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetFlags - Get current holdover configuration
///
/// This function returns the current extended ONU holdover configuration.
///
/// Parameters:
///
/// \return
/// Current holdover configuration
////////////////////////////////////////////////////////////////////////////////
rdpa_epon_holdover_flags HoldoverGetFlags (void)
    {
    return holdoverCfg.flags;
    } // HoldoverGetFlags


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetFlags - Get current holdover configuration
///
/// This function returns the current extended ONU holdover configuration.
///
/// Parameters:
///
/// \return
/// Current holdover configuration
////////////////////////////////////////////////////////////////////////////////
static
void HoldoverStart(void)
    {
    EponSetGrantInterval(MpcpHoldoverNoGrantTime);
    EponUsrModuleTimerCfg(EPON_TIMER_TO1,holdovertimerId,holdoverCfg.time);
    eponUsrDebug(HoldoverDebugLvl, ("HS:%u\n", TkOsSysTick() * TkOsMsPerTick));
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Restore any settings changed by holdover
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void HoldoverCleanup(void)
    {
    EponUsrModuleTimerCfg(EPON_TIMER_TO1,holdovertimerId,0);
    EponSetGrantInterval(MpcpNoGrantTime);
    }


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetFlags - Get current holdover configuration
///
/// This function returns the current extended ONU holdover configuration.
///
/// Parameters:
///
/// \return
/// Current holdover configuration
////////////////////////////////////////////////////////////////////////////////
static
void HoldoverEnd(void)
    {
    eponUsrDebug(HoldoverDebugLvl, ("HE:%u\n", TkOsSysTick() * TkOsMsPerTick));
    HoldoverCleanup();
    }


////////////////////////////////////////////////////////////////////////////////
//extern
BOOL HoldoverCallback(BOOL los, BOOL eponInSync)
    {
    BOOL sync = !los && eponInSync;

    switch (state)
        {
        case HoldoverRecover:
            if (sync)
                {
                state = HoldoverReady;
                }
            break;

        case HoldoverReady:
            if ((!sync) && (holdoverCfg.time > 0))
                {
                HoldoverStart();
                state = HoldoverRunning;
                return TRUE;
                }
            break;

        case HoldoverRunning:
            if (sync)
                {
                HoldoverEnd();
                state = HoldoverReady;
                }
            return TRUE;

        case HoldoverFail:
            HoldoverCleanup();
            state = HoldoverRecover;
            break;

        default:
            break;
        }

    return sync;
    }


////////////////////////////////////////////////////////////////////////////////
/// HoldoverInit - Initialize ONU holdover values
///
/// This function loads holdover config from NVS or personality if no record
/// exists.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void HoldoverInit (void)
    {
    holdoverCfg.time = 0;
    holdoverCfg.flags = rdpa_epon_holdover_noflags;        
    state = HoldoverRecover;
    PonCfgDbSetRegCb(HoldoverCallback);
    holdovertimerId = EponUsrModuleTimerRegister(
    	EPON_TIMER_TO1,0,HoldoverExpireTimer);
    } // HoldoverInit


// End of File Holdover.c

