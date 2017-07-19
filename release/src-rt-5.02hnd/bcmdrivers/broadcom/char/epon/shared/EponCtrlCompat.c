/*
*  Copyright 2015, Broadcom Corporation
*
* <:copyright-BRCM:2015:proprietary:standard
* 
*    Copyright (c) 2015 Broadcom 
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
/*
File Name  : EponCtrlCompat.c
Description: Broadcom EPON  compatible Interface Driver
This file mainly support the EPON stack manager interface for user space. The EPON control library
send the command and parameters. 
*/    
//**************************************************************************
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/compat.h>
#include "EponCtrlCompat.h"
#include "EponCtrl.h"
#include "EponUser.h"

extern EponIoctlHandle_t  ioctlFuncs[];

static int compat_get_stats_parms(StatsParaCompat   * ioctlParms32, StatsPara  *stats)
{
    int err = 0;
    
    stats->port = ioctlParms32->port;
    stats->instance = ioctlParms32->instance;
    stats->statsCount = ioctlParms32->statsCount;
    stats->StatsCnt.statsVal2 = (StatsCntOne *)ioctlParms32->StatsCnt.statsVal2;
   
    return err;     
}

static int compat_put_stats_parms(StatsParaCompat  * ioctlParms32, StatsPara   *stats)
{
    int err = 0;

    ioctlParms32->port = stats->port;
    ioctlParms32->instance = stats->instance;
    ioctlParms32->statsCount =  stats->statsCount;
    ioctlParms32->StatsCnt.statsVal2 = (compat_uptr_t)stats->StatsCnt.statsVal2;
   
    return err;     
}

static int compat_get_epon_mac_q_parms(pon_mac_q_Para_compat   * ioctlParms32, pon_mac_q_Para   *epon_mac_q)
{
    int err = 0;

    epon_mac_q->cfg = (epon_mac_q_cfg_t *)ioctlParms32->cfg;		

    return err;     
}

static int compat_put_epon_mac_q_parms(pon_mac_q_Para_compat   * ioctlParms32, pon_mac_q_Para  *epon_mac_q)
{
    int err = 0;

    ioctlParms32->cfg = (compat_uptr_t)epon_mac_q->cfg;

    return err;     
}

static int compat_get_bcapval_parms(BcapParaCompat   * ioctlParms32, BcapPara  *bcapval)
{
    int err = 0;
    
    bcapval->link = ioctlParms32->link;
    bcapval->count = ioctlParms32->count;
    bcapval->bcapsize = (U16 *)ioctlParms32->bcapsize;

    return err;     
}

static int compat_put_bcapval_parms(BcapParaCompat   * ioctlParms32, BcapPara  *bcapval)
{
    int err = 0;
    
    ioctlParms32->link = bcapval->link;
    ioctlParms32->count = bcapval->count;
    ioctlParms32->bcapsize = (compat_uptr_t)bcapval->bcapsize;

    return err;     
}

static int compat_get_keydata_parms(KeyDataParaCompat   * ioctlParms32, KeyDataPara  *keydata)
{
    int err = 0;
    
    keydata->dir  = ioctlParms32->dir;
    keydata->link = ioctlParms32->link;
    keydata->keyindex = ioctlParms32->keyindex;
    keydata->length = ioctlParms32->length;
    keydata->key =  (U32 *)ioctlParms32->key;
    keydata->sci =  (U32 *)ioctlParms32->sci;
    return err;     
}

static int compat_put_keydata_parms(KeyDataParaCompat   * ioctlParms32, KeyDataPara   *keydata)
{
    int err = 0;

    ioctlParms32->dir = keydata->dir;
    ioctlParms32->link = keydata->link;
    ioctlParms32->keyindex = keydata->keyindex;
    ioctlParms32->length = keydata->length;
    ioctlParms32->key = (compat_uptr_t)keydata->key;
    ioctlParms32->sci = (compat_uptr_t)keydata->sci;
        
    return err;     
}

static int compat_get_ctcstatsalm_parms(CtcStatsAlarmParaCompat   * ioctlParms32, CtcStatsAlarmPara   *ctcstatsalm)
{
    int err = 0;
    
    ctcstatsalm->statsCount = ioctlParms32->statsCount;
    ctcstatsalm->statsAlmVal = (CtcStatsAlarm *)ioctlParms32->statsAlmVal;
   
    return err;     
}

static int compat_put_ctcstatsalm_parms(CtcStatsAlarmParaCompat   * ioctlParms32, CtcStatsAlarmPara   *ctcstatsalm)
{
    int err = 0;
        
    ioctlParms32->statsCount = ctcstatsalm->statsCount;
    ioctlParms32->statsAlmVal = (compat_uptr_t)ctcstatsalm->statsAlmVal;
  
    return err;     
}

static int compat_get_ctcstatsthe_parms(CtcStatsAlmTheParaCompat   * ioctlParms32, CtcStatsAlmThePara  *ctcstatsthe)
{
    int err = 0;
        
    ctcstatsthe->statsCount = ioctlParms32->statsCount;
    ctcstatsthe->statsTheVal = (CtcStatsAlmThe *)ioctlParms32->statsTheVal;

    return err;     
}

static int compat_put_ctcstatsthe_parms(CtcStatsAlmTheParaCompat   * ioctlParms32, CtcStatsAlmThePara  *ctcstatsthe)
{
    int err = 0;
        
    ioctlParms32->statsCount = ctcstatsthe->statsCount;
    ioctlParms32->statsTheVal = (compat_uptr_t)ctcstatsthe->statsTheVal;
   
    return err;     
}

static int compat_get_ctcstats_parms(CtcStatsParaCompat   * ioctlParms32, CtcStatsPara  *ctcstats)
{
    int err = 0;
    
    ctcstats->history = ioctlParms32->history;
    ctcstats->statsCount = ioctlParms32->statsCount;
    ctcstats->statsVal = (StatsCntOne *)ioctlParms32->statsVal;
   
    return err;     
}

static int compat_put_ctcstats_parms(CtcStatsParaCompat   * ioctlParms32, CtcStatsPara  *ctcstats)
{
    int err = 0;
    
    ioctlParms32->history= ctcstats->history;
    ioctlParms32->statsCount = ctcstats->statsCount;
    ioctlParms32->statsVal = (compat_uptr_t)ctcstats->statsVal;
   
    return err;     
}

long EponIoctlAcessCompat(unsigned int cmd, unsigned long arg)
{
    int ret = EponSTATUSSUCCESS;
    int err = 0;
    unsigned int cmdnr = _IOC_NR(cmd);
    EponCtlParamtCompat EArgCompat;
    EponCtlParamt EArg;

    if ((cmdnr >= BCMEPONCfgPers && cmdnr < BCM_EPON_IOCTL_MAX) &&
        (ioctlFuncs[cmdnr].cmd == cmdnr) &&
        (ioctlFuncs[cmdnr].fun != NULL))
    {
        copy_from_user(&EArgCompat,(void *)arg,sizeof(EponCtlParamtCompat));
        EArg.ope = EArgCompat.ope;

        switch (cmd) {
            case BCMEPONCfgPers:
            {
                EArg.eponparm.poncfg = (EponCfgParam  *)EArgCompat.eponparm.poncfg;
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                EArgCompat.eponparm.poncfg = (compat_uptr_t)EArg.eponparm.poncfg;
            }    
            break;
            case BCMEPONCfgStats:
            {
                err = compat_get_stats_parms(&EArgCompat.eponparm.stats, &EArg.eponparm.stats);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_stats_parms(&EArgCompat.eponparm.stats, &EArg.eponparm.stats);
            }   
            break;
            case BCMEPONCfgQueue:
            {
                err = compat_get_epon_mac_q_parms(&EArgCompat.eponparm.epon_mac_q, &EArg.eponparm.epon_mac_q);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_epon_mac_q_parms(&EArgCompat.eponparm.epon_mac_q, &EArg.eponparm.epon_mac_q);
            } 
            break;
            case BCMEPONBCap:
            {
                err = compat_get_bcapval_parms(&EArgCompat.eponparm.bcapval, &EArg.eponparm.bcapval);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_bcapval_parms(&EArgCompat.eponparm.bcapval, &EArg.eponparm.bcapval);
            } 
            break;
            case BCMEPONKeyData:
            {
                err = compat_get_keydata_parms(&EArgCompat.eponparm.keydata, &EArg.eponparm.keydata);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_keydata_parms(&EArgCompat.eponparm.keydata, &EArg.eponparm.keydata);
            }
            break;
            case BCMEPONCtcAlm:
            {
                err = compat_get_ctcstatsalm_parms(&EArgCompat.eponparm.ctcstatsalm, &EArg.eponparm.ctcstatsalm);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_ctcstatsalm_parms(&EArgCompat.eponparm.ctcstatsalm, &EArg.eponparm.ctcstatsalm);
            }
            break;
            case BCMEPONCtcThe:
            {
                err = compat_get_ctcstatsthe_parms(&EArgCompat.eponparm.ctcstatsthe, &EArg.eponparm.ctcstatsthe);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_ctcstatsthe_parms(&EArgCompat.eponparm.ctcstatsthe, &EArg.eponparm.ctcstatsthe);
            }
            break;
            case BCMEPONCtcStats:
            {
                err = compat_get_ctcstats_parms(&EArgCompat.eponparm.ctcstats, &EArg.eponparm.ctcstats);
                ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
                err = compat_put_ctcstats_parms(&EArgCompat.eponparm.ctcstats, &EArg.eponparm.ctcstats);
            }
            break;
            default:
            {
                ret = (*ioctlFuncs[cmdnr].fun) ((EponCtlParamt *)&EArgCompat);
            }
            break;
        }
        
        copy_to_user((void *)arg,(void *)&EArgCompat,sizeof(EponCtlParamtCompat));
    }
    else
    {
        ret = EponSTATUSERROR;
    }
    
    return ret;
}


