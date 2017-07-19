/*
*  Copyright 2014, Broadcom Corporation
*
* <:copyright-BRCM:2014:proprietary:standard
* 
*    Copyright (c) 2014 Broadcom 
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
/// \file EponRogueDrv.c
/// \brief Epon Rogue ONU detection support
///
/// This file contains 
///
////////////////////////////////////////////////////////////////////////////////
#include <linux/string.h>
#include "Teknovus.h"
#ifdef CONFIG_EPON_10G_SUPPORT
#include "Xif.h" 
#endif
#include "Lif.h"
#include "bcm_common.h"
#include "boardparms.h"
#include "shared_utils.h"
#include "EponRogueDrv.h"
#include "OsAstMsgQ.h"
#include "EponUser.h"
#include "EponCtrl.h"

static EponRogueOnuPara rogueOnuCfg;
static U8 curTimes;

BOOL EponRogueDetSet(BOOL enable, U32 threshold, U8 times)
    {
    eponUsrDebug(DebugEponRogue,
    	("set rogue onu en:%u, thres:%08x, times:%u \n", 
    	(U8)enable, threshold, times));
    
    if (enable == rogueOnuCfg.enable)
        {
        eponUsrDebug(DebugEponRogue,
        	("No change to Epon Rogue ONU enable status. \n"));
        }
    else
        {
        rogueOnuCfg.enable = enable;
        rogueOnuCfg.threshold = threshold;
        rogueOnuCfg.times = times;
#ifdef CONFIG_EPON_10G_SUPPORT        
        if (PonCfgDbGetUpRate() == LaserRate10G)
            {
            XifLaserMonSet(enable, threshold);
            }
        
        else
#endif
            {
            LifLaserMonSet(enable, threshold);
            }

        if (!rogueOnuCfg.enable)
            {
            curTimes = 0;
            OsAstMsgQClr(OsAstAlmOntRogueOnu, 0, 0);
            }
        }
    return TRUE;
    }


BOOL EponRogueDetGet(BOOL* enable, U32* threshold, U8* times)
    {
    *enable = rogueOnuCfg.enable;
    *threshold = rogueOnuCfg.threshold;
    *times = rogueOnuCfg.times;
    
    eponUsrDebug(DebugEponRogue,
        ("get rogue onu cfg, en:%u, thres:%08x, times:%u \n", 
        (U8)(*enable), *threshold, *times));
    return TRUE;
    }


void EponRogueDetHandleTimer(EponTimerModuleId moduleId)
    {
    U32 laserOnMaxInt = 0;
    if (rogueOnuCfg.enable)
        {
        if (!OnuOsAssertGet(OsAstAlmOntRogueOnu, 0))
            {
#ifdef CONFIG_EPON_10G_SUPPORT             
            if (PonCfgDbGetUpRate() == LaserRate10G)
                {            
                laserOnMaxInt = XifLaserMonLaserOnMaxIntGet();
                }
            else
#endif
                {
                laserOnMaxInt = LifLaserMonLaserOnMaxIntGet();
                }
            
            if(laserOnMaxInt)
                {
                curTimes++;
                if (curTimes >= rogueOnuCfg.times)
                    {
                    eponUsrDebug(DebugEponRogue,("Rogue ONU alarm raise \n"));
                    OsAstMsgQSet(OsAstAlmOntRogueOnu, 0, 0);
                    }
                else
                    {
#ifdef CONFIG_EPON_10G_SUPPORT                    
                    if (PonCfgDbGetUpRate() == LaserRate10G)                      
                        {
                        XifLaserMonClrLaserOnMaxInt();
                        }
                    else
#endif
                        {
                        LifLaserMonClrLaserOnMaxInt();
                        }
                    }
                }
            }
        }
    }


void EponRogueDetInit (void)
    {    
    unsigned short MuxSel = 0;
	
    /* set rogue onu pinmuxing */
    if( BpGetRogueOnuEn((unsigned short*)&MuxSel) == BP_SUCCESS )
        {
        if (MuxSel)
            {
            set_pinmux(PINMUX_ROGUE_ONU_PIN, PINMUX_ROGUE_ONU_FUNC);
            }
        }

    rogueOnuCfg.enable = FALSE;
    rogueOnuCfg.threshold = 0;
    rogueOnuCfg.times = 0;
    curTimes = 0;

    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
        EponRoguePollTimer, EponRogueDetHandleTimer);
    
    printk("EponRogueDetInit \n");
    } // EponRogueDetInit
	
// end EponRogueDrv.c

