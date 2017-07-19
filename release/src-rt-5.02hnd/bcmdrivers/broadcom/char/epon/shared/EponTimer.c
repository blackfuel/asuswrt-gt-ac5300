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

//**************************************************************************
// File Name  : epon_timer.c
// This file mainly used to manager the resource for epon stack.
///1)Create Global timer for ALL EPON stack polling hanlde.
// Description: Broadcom EPON  Interface Driver
//               
//**************************************************************************
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include <linux/skbuff.h>
#include <asm/uaccess.h>
#include <net/sock.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>


#include "Teknovus.h"
#include "EponTimer.h"
#include "EponFrame.h"


static EponUsrModuleTimer EponModuleTimerHandler[EPON_TIMER_End][EPON_TIMER_Num];
static EponTimerModuleId   curTimerModuleid = 0;
static EponTimer EponTimerIdTable[EPON_TIMER_End]; 

////////////////////////////////////////////////////////////////////////////////
/// \brief Total EPON module timer handle
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static void EponUsrTimerHandler(unsigned long data)
    {
    EponTimerModuleId id; 
    EponTimer *eponTimer = (EponTimer*)data;
    EponUsrModuleTimer *usrTimer;

    if (eponTimer->timerId >= EPON_TIMER_End)
        {
        return ;
        }
	
    usrTimer = (EponUsrModuleTimer *)(&EponModuleTimerHandler[eponTimer->timerId][0]);
	
    if (eponTimer->state != TIMER_IDLE) 
        {
        // Signal that timer has expired.
        eponTimer->state = TIMER_EXPIRED;
        //////////////////////////////////////
        ////////Detail  Function call///////////////

        for (id = 0; id < curTimerModuleid;id++)
            {
            if ((usrTimer->modId == id ) && 
            (usrTimer->fun != NULL)  &&
            (usrTimer->expiretime > 0))
                {
                if(time_after_eq64(get_jiffies_64(),
                    (((usrTimer->expiretime*HZ)/1000) + usrTimer->curtime)))
                    {
                    usrTimer->curtime = get_jiffies_64();
                    (*usrTimer->fun)(id);
                    }				
                }
            usrTimer++;
            }

        //update scheduling
        mod_timer(&eponTimer->timer, (jiffies + ((eponTimer->timeoutMs * HZ)/1000)));
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief EPON TIMER initialization.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponUsrTimerInit(EponTimer* usrTimer, EponTimerId timerId, U32 timeoutMs)
    {
    usrTimer->timerId = timerId;
    usrTimer->state = TIMER_IDLE;
    usrTimer->timeoutMs = timeoutMs;
    memset(&EponModuleTimerHandler[timerId][0],0,sizeof(EponUsrModuleTimer));
    }


////////////////////////////////////////////////////////////////////////////////
/// \start EPON timer.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponUsrTimerStart(EponTimer* usrTimer)
    {
    if (usrTimer->state == TIMER_IDLE) 
        {
        usrTimer->state = TIMER_RUNNING;
        init_timer(&usrTimer->timer);
        usrTimer->timer.function = EponUsrTimerHandler;
        usrTimer->timer.data = (unsigned long)usrTimer;
        usrTimer->timer.expires = (jiffies + ((usrTimer->timeoutMs * HZ)/1000)); /*HZ=1000*/
        add_timer(&usrTimer->timer);
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \delete EPON timer.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
 void EponUsrTimerStop(EponTimer* usrTimer)
    {
    del_timer(&usrTimer->timer);
    usrTimer->state = TIMER_IDLE;
    }


////////////////////////////////////////////////////////////////////////////////
/// \Detail module timer register.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
EponTimerModuleId EponUsrModuleTimerRegister(EponTimerId timerid,
														  unsigned int tick,
														  EponTimerHandler fun)
	{	
	if ((timerid >= EPON_TIMER_End) || (curTimerModuleid >= EPON_TIMER_Num))
		{
		return EPON_TIMER_Num;
		}
	
	if (EponModuleTimerHandler[timerid][curTimerModuleid].fun == NULL)
		{
		EponModuleTimerHandler[timerid][curTimerModuleid].modId = 
			curTimerModuleid;
		
		EponModuleTimerHandler[timerid][curTimerModuleid].expiretime = tick;
		
		EponModuleTimerHandler[timerid][curTimerModuleid].curtime = 
			get_jiffies_64();
		
		EponModuleTimerHandler[timerid][curTimerModuleid].fun = fun;
		}
	curTimerModuleid++;
	
	return (curTimerModuleid-1);
	}


////////////////////////////////////////////////////////////////////////////////
/// \Configure the usrer module timer. the module timer will stop if the tick is zero.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponUsrModuleTimerCfg(EponTimerId timerid,
									 EponTimerModuleId id,
									 unsigned int tick)
    {
    if ((timerid >= EPON_TIMER_End) || (id >= EPON_TIMER_Num))
        {
        return ;
        }
    
    EponModuleTimerHandler[timerid][id].curtime = 
    	get_jiffies_64();
    
    EponModuleTimerHandler[timerid][id].expiretime = tick;
    }



////////////////////////////////////////////////////////////////////////////////
/// \Get the current tick of this timer..
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
U64 EponUsrModuleTimerCurTickGet(EponTimerId timerid,
									 EponTimerModuleId id )
    {
    if ((timerid >= EPON_TIMER_End) || (id >= EPON_TIMER_Num))
        {
        return 0;
        }
    
    return (EponModuleTimerHandler[timerid][id].curtime);
    }


void EponUserTimerReset(void)
    {
    curTimerModuleid = 0;
    memset(EponModuleTimerHandler,0,sizeof(EponModuleTimerHandler));
    }

void EponStackTimerInit(void)
    {
    memset(EponTimerIdTable, 0, sizeof(EponTimer)*EPON_TIMER_End);

    //just start TO1 timer
    EponUsrTimerInit(&(EponTimerIdTable[EPON_TIMER_TO1]),
    		EPON_TIMER_TO1,EPON_TIMER_INTERVEL_MS);
    EponUsrTimerStart(&(EponTimerIdTable[EPON_TIMER_TO1]));
    }

void EponStackTimerExit(void)
    {
    EponUsrTimerStop(&(EponTimerIdTable[EPON_TIMER_TO1]));
    }

// end of Epon_timer.c

