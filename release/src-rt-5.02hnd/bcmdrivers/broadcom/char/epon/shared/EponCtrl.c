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
/*
File Name  : Epon_ctrl.c
Description: Broadcom EPON  Interface Driver
This file mainly support the EPON stack manager interface for user space. The EPON control library
send the command and parameters. 
*/    
//**************************************************************************

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/ctype.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>

#include "EponUser.h"
#include "EponCtrl.h"
#include "EponFrame.h"
#include "OntmMpcp.h"

#include "EponStats.h"
#include "CtcStats.h"
#include "CtcAlarms.h"
#include "CtcOam.h"

#include "EponCmds.h"
#include "OpticCmds.h"
#include "OsAstMsgQ.h"
#include "OptCtrl.h"
#include "OntDirector.h"
#include "PonManager.h"

#include "Teknovus.h"

#include "EponRogueDrv.h"
#include "rdpa_api.h"
#include "autogen/rdpa_ag_epon.h"

#ifdef CONFIG_EPON_CLOCK_TRANSPORT
#include "ClockTransport.h"
#endif

BOOL resettest = FALSE;
static U8 Gates1G[60] = 
{0x01, 0x80, 0xC2, 0x00, 0x00, 0x01,
0x00, 0x0D, 0xB6, 0x41, 0x51, 0x40,
0x88, 0x08, 0x00, 0x02, 0x03, 0xE2,
0xDD, 0xF1, 0x09, 0x03, 0xE3, 0x1B, 
0x55, 0x20, 0x40, 0x00, 0x20, 0x00 };

static U8 Reg1G[60] = 
{0x00, 0x0D, 0xB6, 0xC0, 0x03, 0xC0, 
0x00, 0x0D, 0xB6, 0x41, 0x51, 0x40,
0x88, 0x08, 0x00, 0x05, 0x0F, 0x16,
0x50, 0x8A, 0x3C, 0x02, 0x03, 0x00, 
0x20, 0x10};

static U8 Gates10G[60] = 
{0x01, 0x80, 0xC2, 0x00, 0x00, 0x01,
0x00, 0x0D, 0xB6, 0x00, 0x00, 0x00,
0x88, 0x08, 0x00, 0x02, 0xd3, 0xc8,
0x55, 0x3f, 0x09, 0xd3, 0xc8, 0x90, 
0x06, 0x25, 0x01, 0x00, 0x20, 0x00,
0x22, 0x00};

static U8 Reg10G[60] = 
{0x00, 0x0D, 0xB6, 0xC0, 0x03, 0xC0, 
0x00, 0x0D, 0xB6, 0x41, 0x51, 0x40,
0x88, 0x08, 0x00, 0x05, 0x18, 0x7E,
0x26, 0x3A, 0x3C, 0x00, 0x03, 0x00, 
0x20, 0x10, 0x20, 0x20, 0x00, 0x00};


static U8 Oam1G[60] = 
{0x00, 0x0D, 0xB6, 0xC0, 0x03, 0xC0, 
0x00, 0x0D, 0xB6, 0x41, 0x51, 0x40,
0x88, 0x09, 0x00, 0x05, 0x0F, 0x16,
0x50, 0x8A, 0x3C, 0x02, 0x03, 0x00, 
0x20, 0x10};

int EponCfg(EponCtlParamt* EArg)
{
    EponCfgParam ponCfg;
    
    DumpData(DebugIoctl, "EponCfg", (U8 *)EArg, sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        copy_from_user(&ponCfg, (void *)EArg->eponparm.poncfg, sizeof(EponCfgParam));
        DumpData(DebugIoctl,"PonCfg", (U8 *)&ponCfg, sizeof(EponCfgParam));
        local_bh_disable();
        PonCfgDbSetPonParam(&ponCfg);
        local_bh_enable();
    }
    else
    {
        memset(&ponCfg,0,sizeof(EponCfgParam));
        copy_from_user(&ponCfg, (void *)EArg->eponparm.poncfg, sizeof(EponCfgParam));
        PonCfgDbGetPonParam(&ponCfg);
        copy_to_user((void *)EArg->eponparm.poncfg,&ponCfg, sizeof(EponCfgParam));
        DumpData(DebugIoctl,"PonCfg",(U8 *)&ponCfg,sizeof(EponCfgParam));
    }
	
    return EponSTATUSSUCCESS;
}


int EponCfgEponUsr(EponCtlParamt* EArg)
{    
    bdmf_object_handle epon_obj = NULL;
    int rc = BDMF_ERR_INTERNAL; 
    int ret = EponSTATUSPARAMETERERROR;

    DumpData(DebugIoctl, "EponCfgEponUsr", (U8 *)&EArg, sizeof(EponCtlParamt));	
   
    rc = rdpa_epon_get(&epon_obj);
    if (rc)
        goto error;

    switch (EArg->eponparm.eponact)                
    {
        case EponLoadCfgCur:
            rc = rdpa_epon_init_set(epon_obj,1);
            break;
        default:
            break;
    }

    if (!rc)
        ret = EponSTATUSSUCCESS;
	
    if (epon_obj)
        bdmf_put(epon_obj);

error:
    return ret;
}

extern int EponUsrLinkSend(U8 link, U8 *pbuf, U32 size, BOOL skb);
int EponCfgDebug(EponCtlParamt* EArg)
{
    DebugModule id;
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl, "EponCfgDebug", (U8 *)&EArg ,sizeof(EponCtlParamt));
	
    id = EArg->eponparm.debug.id;
    if (EArg->ope == EponSetOpe)
    {
        if (id < DebugModuleCount)
        {
            eponUsrDebugEn[id] = EArg->eponparm.debug.flag;
        }
        else
        {
            //Just for internel debug
            if (id == 0x11)
            {
                printk("EponMpcpNotify %s gates\n", (PonCfgDbGetDnRate() == LaserRate10G) ? "10G" : "1G");
                EponMpcpNotify((PonCfgDbGetDnRate() == LaserRate10G) ? Gates10G: Gates1G, 0x20);
            }
            else if (id == 0x12)
            {
                printk("EponMpcpNotify %s reg\n", (PonCfgDbGetDnRate() == LaserRate10G) ? "10G" : "1G");
                EponMpcpNotify((PonCfgDbGetDnRate() == LaserRate10G) ? Reg10G: Reg1G, 0x20);
            }
            else if (id == 0x13)
            {
                OntmMpcpCmdInfo();
            }
            else if (id == 0x14)
            {
                OsAstMsgQInfo();
            }
            else if (id == 0x16)
            {
                EponUsrLinkSend(0, Oam1G,(U8)EArg->eponparm.debug.flag,FALSE);
            }
	    else
	    {
	        ret = EponSTATUSPARAMETERERROR;
	    }
        }
    }
    else if (EArg->ope == EponGetOpe)
    {
        if (id < DebugModuleCount)
        {
	    EArg->eponparm.debug.flag = eponUsrDebugEn[id];
        }
	else
        {
            ret = EponSTATUSPARAMETERERROR;
        }
    }

    return ret;    
}

#ifndef CONFIG_EPON_10G_SUPPORT 
#define OREN_REG_ADDR(x) (volatile U32* )(0xA0000000+x)
#define EPONCTL_REGSTART(x) ((x->eponparm.reg.regStart))
#define EPONCTL_REGVALUE(x) ((x->eponparm.reg.regval))
int EponCfgReg(EponCtlParamt* EArg)
{
    U8 count,i;
    U32 regval[MaxRegCount];
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl, "EponCfgReg", (U8 *)&EArg, sizeof(EponCtlParamt));
    count = EArg->eponparm.reg.regCount;
    
    if (EArg->ope == EponSetOpe)
    {
        //Only write one register
        if (count == 1)
        {
            if (EPONCTL_REGSTART(EArg) >= ORENREG_BASE)
            {
                // the address is for globle
                OnuRegWrite(OREN_REG_ADDR(EPONCTL_REGSTART(EArg)),
                	EPONCTL_REGVALUE(EArg));
            }         
            else if (EPONCTL_REGSTART(EArg) < 0x600) 
            {
                // the address is for local
            OnuRegWrite (&TkOnuReg((EPONCTL_REGSTART(EArg))),
            	EPONCTL_REGVALUE(EArg));
            }
	    else
	    {
	        // wrong address
	        ret = EponSTATUSPARAMETERERROR;
	    }
        }
        else if ((count != 0)&&(count < MaxRegCount))
        {
            memset(regval, 0, count*4);
            copy_from_user((void *)regval, (void *)EPONCTL_REGVALUE(EArg), count*4);
            DumpData(DebugIoctl, "Reg" ,(U8 *)regval, count*4);
            for (i = 0; i < count; i++)
            {
                OnuRegWrite (&TkOnuReg((EPONCTL_REGSTART(EArg)+i)),regval[i]);
            }
        }
	else
        {
            // wrong address
            ret = EponSTATUSPARAMETERERROR;
        }
    }
    else if (EArg->ope == EponGetOpe)
    {
        if ((count != 0)&&(count < MaxRegCount))
        {
            memset(regval,0,count*4);
            for (i = 0; i < count; i++)
            {
                if (EArg->eponparm.reg.regStart >= ORENREG_BASE)
                {
                regval[i] = OnuRegRead(OREN_REG_ADDR(EPONCTL_REGSTART(EArg)+ i*4));
                }
                else if (EPONCTL_REGSTART(EArg) < 0x600) 
                {
                regval[i] = OnuRegRead(&TkOnuReg((EPONCTL_REGSTART(EArg)+i)));
                }
		else
                {
                    // wrong address
                    ret = EponSTATUSPARAMETERERROR;
                }
            }
        }
        else
        {
            // wrong address
            ret = EponSTATUSPARAMETERERROR;
        }

        if (ret == EponSTATUSSUCCESS)
        {
            // copy value to return pointer
            DumpData(DebugIoctl, "Reg" ,(U8 *)regval, count*4);
            copy_to_user((void *)EPONCTL_REGVALUE(EArg), 
            			(void *)regval,count*4);
        }
    }
    	
    return ret;  
}
#else
int EponCfgReg(EponCtlParamt* EArg)
{
    return EponSTATUSSUCCESS;
}
#endif

int EponCfgReportMode(EponCtlParamt* EArg)
{
    PonMgrRptMode modTmp;
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl, "EponCfgReportMode", (U8 *)EArg, sizeof(EponCtlParamt));
    modTmp = EArg->eponparm.reportmod;
	
    if (EArg->ope == EponSetOpe)
    {		
        if ((modTmp == RptModeFrameAligned) ||
	        (modTmp == RptModeMultiPri3) ||
	        (modTmp == RptModeMultiPri4) ||
	        (modTmp == RptModeMultiPri8))
        {
            PonMgrSetReporting(modTmp);
        }
	else
        {
            ret = EponSTATUSPARAMETERERROR;
        }
    }
    else if (EArg->ope == EponGetOpe)
    {		
        EArg->eponparm.reportmod = PonMgrReportModeGet();
    }
	
    return ret;
}


int EponCfgEponMacQueue(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    epon_mac_q_cfg_t queue_cfg; 
        
    memset(&queue_cfg, 0, sizeof(queue_cfg));
    DumpData(DebugIoctl, "EponCfgLinkQueue", (U8 *)EArg, sizeof(EponCtlParamt));

    if (EArg->ope == EponSetOpe)
    {
        copy_from_user((void *)&queue_cfg,
            (void *)EArg->eponparm.epon_mac_q.cfg, sizeof(epon_mac_q_cfg_t));
        DumpData(DebugIoctl,"Epon Mac Queue", 
            (U8 *)&queue_cfg, sizeof(epon_mac_q_cfg_t));
        
        if (!ont_dir_queue_set(&queue_cfg))
        {
            ret = EponSTATUSERROR;
        }
    }
    else if (EArg->ope == EponGetOpe)
    {
        if (ont_dir_queue_get(&queue_cfg))
        {
            DumpData(DebugIoctl, "Epon Mac Queue", 
                (U8 *)&queue_cfg, sizeof(epon_mac_q_cfg_t));
            copy_to_user((void *)EArg->eponparm.epon_mac_q.cfg, 
                &queue_cfg, sizeof(epon_mac_q_cfg_t));
        }
        else
        {
            ret = EponSTATUSERROR;
        }
    }
    
    return ret;    
}


int EponStats(EponCtlParamt* EArg)
    {
    U8 count,i;
    StatsCntOne statsTmp[StatIdNumStats+1];
    EponStatsIf port;
    int ret = EponSTATUSSUCCESS;
    U8 inst;
    
    DumpData(DebugIoctl,"EponStats",(U8 *)EArg,sizeof(EponCtlParamt));
    
    count  = EArg->eponparm.stats.statsCount;
    port   = EArg->eponparm.stats.port;
    inst   = EArg->eponparm.stats.instance;
    
    if (EArg->ope == EponGetOpe)
    {		
        if ((count != 0)&&(count <= StatIdNumStats))
        {
            memset(statsTmp,0,count*sizeof(StatsCntOne));
            //get statsid.
            copy_from_user((void *)statsTmp,
            	(void *)(EArg->eponparm.stats.StatsCnt.statsVal2), count*sizeof(StatsCntOne));
			
            switch (port)
            {
                case EponStatsPon:					
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {                  
                            statsTmp[i].errorid = StatsGetEpon(statsTmp[i].statsId,
									&statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break;
					
                case EponStatsLink :
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {
                            statsTmp[i].errorid = StatsGetLink(inst,
									statsTmp[i].statsId,&statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break;
					
                case EponStatsLif:					
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {
                            statsTmp[i].errorid = StatsGetLif(statsTmp[i].statsId,
									&statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break; 
#ifdef CONFIG_EPON_10G_SUPPORT                    
                case EponStatsXif:                  
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {
                            statsTmp[i].errorid = StatsGetXif(statsTmp[i].statsId,
                                    &statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break;

                case EponStatsXpcs32:                  
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {
                            statsTmp[i].errorid = StatsGetXpcs32(statsTmp[i].statsId,
                                    &statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break;

                case EponStatsXpcs40:                  
                    for (i = 0;i < count;i++)
                    {
                        if (statsTmp[i].statsId < StatIdNumStats)
                        {
                            statsTmp[i].errorid = StatsGetXpcs40(statsTmp[i].statsId,
                                    &statsTmp[i].statsVal);
                        }
                        else
                        {
                            statsTmp[i].errorid = FALSE;
                        }
                    }
                    break;
#endif
                default:
                    break;
            }
            copy_to_user((void *)(EArg->eponparm.stats.StatsCnt.statsVal2), 
            		(void *)statsTmp,count*sizeof(StatsCntOne));
        }
    }
    else
    {
        switch (port)
        {
            case EponStatsPon:					
                StatsClearEpon();
                break;
                
		    case EponStatsLink:
                StatsClearEponLinkStats(inst, inst+1);
                break;
                
            default:
                ret = EponSTATUSPARAMETERERROR;                
                break;
        }
    }

    return ret;    
    }

int EponCfgGetLLID(EponCtlParamt* EArg)
{
    U16 llid;
    int ret = EponSTATUSSUCCESS;
	
    DumpData(DebugIoctl,"EponCfgGetLLID",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponGetOpe)
    {		
        OntDirLLIDGet(EArg->eponparm.llid.link,&llid);
	EArg->eponparm.llid.LLID = llid;
    }
	
    return ret;    
}


int EponCfgLinkNum(EponCtlParamt* EArg)
{
    uint64_t num = 0;
    uint64_t onlinelinks = 0;
    int ret = EponSTATUSSUCCESS;
    int rc = BDMF_ERR_NOENT;
    bdmf_object_handle epon_obj = NULL;

    DumpData(DebugIoctl,"EponCfgLinkNum",(U8 *)EArg,sizeof(EponCtlParamt));

    rc = rdpa_epon_get(&epon_obj); 
    if(rc)
        goto error;

    switch (EArg->ope)
    { 
    case EponSetOpe:
            num = EArg->eponparm.linksInfo.maxLinks;
            rc = rdpa_epon_max_link_count_set(epon_obj,num);
            if(rc)
                goto error;
            break;
    case EponGetOpe:
            rc = rdpa_epon_max_link_count_get(epon_obj, &num);
            rc ? rc : rdpa_epon_registered_links_get(epon_obj, &onlinelinks);
            if(rc)
                goto error;
            EArg->eponparm.linksInfo.maxLinks = (U8)num;
            EArg->eponparm.linksInfo.onlineLinks= (U8)onlinelinks;
            break;
    default:
        goto error;
    }

    bdmf_put(epon_obj);	    
    return ret;

error:
    if(epon_obj)    
        bdmf_put(epon_obj);    
    return EponSTATUSPARAMETERERROR;    
}

int EponCfgL2PonState(EponCtlParamt* EArg)
{
    int ret = EponSTATUSPARAMETERERROR;
    int rc = BDMF_ERR_PARM;
    bdmf_object_handle epon_obj = NULL ; 

    DumpData(DebugIoctl,"EponCfgL2PonState",(U8 *)EArg,sizeof(EponCtlParamt));

    rc = rdpa_epon_get(&epon_obj);
    if (rc)
        goto error;

    if (EArg->ope == EponSetOpe)
        rc = rdpa_epon_link_enable_set(epon_obj, EArg->eponparm.wanstate.index, 
            EArg->eponparm.wanstate.enable);
    else
        rc = rdpa_epon_link_enable_get(epon_obj, EArg->eponparm.wanstate.index,
            &EArg->eponparm.wanstate.enable);

    if (!rc) 
        ret = EponSTATUSSUCCESS;    

error:
    if (epon_obj)
        bdmf_put(epon_obj);

    return ret;    
}

int EponCfgFec(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSTATEERROR;
    bdmf_object_handle epon_obj;
    rdpa_epon_fec_enable_t fec_enable;
    int rc = -1;

    DumpData(DebugIoctl,"EponCfgFec",(U8 *)EArg,sizeof(EponCtlParamt));

    if ( rdpa_epon_get(&epon_obj) )
	return ret;

    if (EArg->ope == EponSetOpe)
    { 
        //oam only get/set 1G fec setting, so keep setting rate is 10G.
        rc = rdpa_epon_fec_enable_get(epon_obj, EArg->eponparm.fec.link, &fec_enable);
        
        if (PonCfgDbGetDnRate() == LaserRate1G)
            fec_enable.ds = EArg->eponparm.fec.rx;
        if (PonCfgDbGetUpRate() == LaserRate1G)
            fec_enable.us = EArg->eponparm.fec.tx;
			
        rc = rdpa_epon_fec_enable_set(epon_obj, EArg->eponparm.fec.link, &fec_enable);
    }
    else if (EArg->ope == EponGetOpe)
    {
        rc = rdpa_epon_fec_enable_get(epon_obj, EArg->eponparm.fec.link, &fec_enable);
        //oam only get/set 1G fec setting, so return false directly if rate is 10G.
        if (PonCfgDbGetDnRate() == LaserRate10G)
            EArg->eponparm.fec.rx = false;
        else
            EArg->eponparm.fec.rx = fec_enable.ds;

        if (PonCfgDbGetUpRate() == LaserRate10G)
            EArg->eponparm.fec.tx = false;
        else
            EArg->eponparm.fec.tx = fec_enable.us;
    }
    
    if(!rc)
      ret = EponSTATUSSUCCESS;

    bdmf_put(epon_obj);
    return ret;
}


int EponGetMpcpStatus(EponCtlParamt* EArg)
{
    int rc = BDMF_ERR_PARM;
    bdmf_object_handle epon_obj = NULL;

    DumpData(DebugIoctl,"EponGetMpcpStatus",(U8 *)EArg,sizeof(EponCtlParamt));
    
    rc = rdpa_epon_get(&epon_obj);
    rc ? rc : rdpa_epon_link_mpcp_state_get(epon_obj, 
        EArg->eponparm.linkstatus.link, &EArg->eponparm.linkstatus.state);

    if (epon_obj)
        bdmf_put(epon_obj);

    return 0;
}



int EponDumpStats(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;	
	
    DumpData(DebugIoctl,"EponDumpStats",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponGetOpe)
    {
        switch (EArg->eponparm.statsdumpid)
        {
            case 0:
                CmdLifStats();
                break;
            case 1:
                CmdEponStats();
                break;
#ifdef CONFIG_EPON_10G_SUPPORT
            case 34:           
                CmdXifStats();
                break;
            case 35:
                CmdXpcs32Stats();
                break;
            case 36:
                CmdXpcs40Stats();  
                break;
#endif
            default:
                CmdLinkStat(EArg->eponparm.statsdumpid-2);
                break;				
        }	
    }
    
    return ret;    
}


int EponGatherEn(EponCtlParamt* EArg)
    {
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponGatherEn",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        if (EArg->eponparm.gather == 0)
        {
            StatsGatherSet(FALSE);
        }
        else
        {
            StatsGatherSet(TRUE);
        }
    }
    
    return ret;    
    }


int EponSetSilence(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponSetSilence",(U8 *)EArg,sizeof(EponCtlParamt)); 
	
    if (EArg->ope == EponSetOpe)
    {
        SilenceDeallocFlagSet((BOOL)EArg->eponparm.silence.flag);
        SilenceTimeSet(EArg->eponparm.silence.time);
    }
    
    return ret;    
}


int EponCfgProtectSwitch(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgProtectSwitch",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        OntDirProtectSwitchSet(EArg->eponparm.psstate);	
    }
    else if (EArg->ope == EponGetOpe)
    {		
        OntDirProtectSwitchGet(&EArg->eponparm.psstate);
    }
    
    return ret;    
}


int EponCfgTxPower(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
	
    DumpData(DebugIoctl,"EponCfgTxPower",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        OptCtrlTxDiagSet(EArg->eponparm.txpower.actOpt,
        EArg->eponparm.txpower.enabletime,TRUE);
    }
    
    return ret;    
}


int EponCfgLaserEn(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    bdmf_object_handle epon_obj = NULL;
    int rc = BDMF_ERR_NOENT;

    DumpData(DebugIoctl,"EponCfgLaserEn",(U8 *)EArg,sizeof(EponCtlParamt));

    rc = rdpa_epon_get(&epon_obj);
    if (rc) 
       goto error;

    if (EArg->ope == EponSetOpe)
    {
        rdpa_epon_laser_tx_mode mode;
        switch (EArg->eponparm.laserpara.enable)
        {
        case LaserTxModeOff:
            mode = rdpa_epon_laser_tx_off;
            break;
        case LaserTxModeBurst:
            mode = rdpa_epon_laser_tx_burst;
            break;
        case LaserTxModeContinuous:
            mode = rdpa_epon_laser_tx_continuous;
            break;
        default:
            mode = rdpa_epon_laser_tx_off;
        }

        rc = rdpa_epon_laser_tx_mode_set(epon_obj, mode);
        if (rc )
            goto error;
    }  

    bdmf_put(epon_obj); 
    return ret;

error:
    if (epon_obj)
        bdmf_put(epon_obj);
    return EponSTATUSPARAMETERERROR;
}

int EponCfgAssignMcast(EponCtlParamt* EArg)
{
    AssignMcast *assignMcast = NULL;
    bdmf_object_handle epon_obj;
    rdpa_epon_mcast_link_t mcast_link;
    int rc;
 
    rc = rdpa_epon_get(&epon_obj);
    if (rc) 
        goto epn_ioctl_cfg_mcast_err_1;

    assignMcast = &EArg->eponparm.assignMcast;
    DumpData(DebugIoctl,"EponCfgAssignMcast",(U8 *)EArg,sizeof(EponCtlParamt));

    if (EArg->ope == EponSetOpe)
    {			    
        mcast_link.llid = assignMcast->assignedPort;
        mcast_link.enable = (assignMcast->flags ==  MpcpRegFlagSuccess) ? 1 : 0; 

        rc = rdpa_epon_mcast_link_set(epon_obj, assignMcast->link, &mcast_link);
        if (rc) 
            goto epn_ioctl_cfg_mcast_err_2;
    }
    else
    {
        rc = rdpa_epon_mcast_link_get(epon_obj, assignMcast->link, &mcast_link);
        if (rc) 
            goto epn_ioctl_cfg_mcast_err_2;

        assignMcast->assignedPort = mcast_link.llid;
        assignMcast->idxMcast = mcast_link.flow_idx;
    }

    bdmf_put(epon_obj); 
    return EponSTATUSSUCCESS;

epn_ioctl_cfg_mcast_err_2:
    bdmf_put(epon_obj);
epn_ioctl_cfg_mcast_err_1:
    return EponSTATUSPARAMETERERROR;
}

int EponCfgCtcAlm(EponCtlParamt* EArg)
    {
    U8 count;
    int ret = EponSTATUSSUCCESS;
    CtcStatsAlarm almbuf[OamCtc30PonIfAlmNum];
	
    memset(almbuf,0,sizeof(almbuf));
    DumpData(DebugIoctl,"EponCfgCtcAlm",(U8 *)EArg,sizeof(EponCtlParamt));
    count = EArg->eponparm.ctcstatsalm.statsCount;
	
    if (EArg->ope == EponSetOpe)
    {			
        if (count <= OamCtc30PonIfAlmNum)
        {
            U8 i;
            copy_from_user((void *)almbuf, (void *)EArg->eponparm.ctcstatsalm.statsAlmVal,
				sizeof(CtcStatsAlarm)*count);
            DumpData(DebugIoctl,"Alm",(U8 *)almbuf,sizeof(CtcStatsAlarm)*count);
			
            //check alarm id if valid.
            for (i = 0;i < count;i++)
            {
                if (!CtcAlmIdValid(almbuf[i].alarmId))
                {
                    break;
                }				
            }
            
            if (i == count)
            {
                for (i = 0;i < count;i++)
                {
                    CtcAlmAdminStateSet(almbuf[i].alarmId,
                    		((almbuf[i].enable) ? OamCtcActionEnable : OamCtcActionDisable));				
                }
            }
        }
    }
    else if (EArg->ope == EponGetOpe)
    {
        if (count <= OamCtc30PonIfAlmNum)
        {
            U8 i;
            copy_from_user((void *)almbuf,(void *)EArg->eponparm.ctcstatsalm.statsAlmVal,
				sizeof(CtcStatsAlarm)*count);
            for (i = 0;i < count;i++)
            {
                if (!CtcAlmIdValid(almbuf[i].alarmId))
                {
                    break;
                }				
            }
            if (i == count)
            {
                for (i = 0;i < count;i++)
                {
                    almbuf[i].enable = CtcAlmAdminStateGet(almbuf[i].alarmId);
                }
                copy_to_user((void *)EArg->eponparm.ctcstatsalm.statsAlmVal, almbuf, 
					sizeof(CtcStatsAlarm)*count);
            }
        }
    }
    
    return ret;    
    }

int EponCfgCtcThe(EponCtlParamt* EArg)
{
    U8 count;
    int ret = EponSTATUSSUCCESS;
    CtcStatsAlmThe almbuf[OamCtc30PonIfAlmNum];
    
    memset(almbuf,0,sizeof(almbuf));
    DumpData(DebugIoctl,"EponCfgCtcThe",(U8 *)EArg,sizeof(EponCtlParamt));
    count = EArg->eponparm.ctcstatsthe.statsCount;
	
    if (EArg->ope == EponSetOpe)
    {
        if (count <= OamCtc30PonIfAlmNum)
        {
            U8 i;
            copy_from_user((void *)almbuf,
            (void *)EArg->eponparm.ctcstatsthe.statsTheVal,sizeof(CtcStatsAlmThe)*count);
            DumpData(DebugIoctl,"The",(U8 *)almbuf,sizeof(CtcStatsAlmThe)*count);
            //check alarm id if valid.
            for (i = 0;i < count;i++)
            {
                if (!CtcAlmIdValid(almbuf[i].alarmId))
                {
                    break;
                }				
            }
            
            if (i == count)
            {
                CtcAlmMonThd thd;				
                for (i = 0;i < count;i++)
                {
                    thd.CtcAlmRaise = almbuf[i].setThe;
                    thd.CtcAlmClear = almbuf[i].clearThe;
                    thd.CtcAlmFlag = FALSE;
                    CtcEvtThSet((almbuf[i].alarmId-OamCtcPonIfAlarmEnd),&thd);			
                }
            }
        }
    }
    else if (EArg->ope == EponGetOpe)
    {
        if (count <= OamCtc30PonIfAlmNum)
        {
            U8 i;
            copy_from_user((void *)almbuf,(void *)EArg->eponparm.ctcstatsthe.statsTheVal,
				sizeof(CtcStatsAlmThe)*count);
            for (i = 0;i < count;i++)
            {
                if (!CtcAlmIdValid(almbuf[i].alarmId))
                {
                    break;
                }				
            }
			
            if (i == count)
            {
                CtcAlmMonThd thd;
                for (i = 0;i < count;i++)
                {
                    if (CtcEvtThGet((almbuf[i].alarmId-OamCtcPonIfAlarmEnd),&thd))
                    {
                        almbuf[i].setThe = thd.CtcAlmRaise;
                        almbuf[i].clearThe = thd.CtcAlmClear;
                    }
                }
                copy_to_user((void *)EArg->eponparm.ctcstatsthe.statsTheVal,almbuf, 
					sizeof(CtcStatsAlmThe)*count);
            }
        }
    }
    
    return ret;    
}

int EponCfgCtcPeriod(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgCtcPeriod",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        if (EArg->eponparm.ctcstatsperiod == 0)
        {
            CtcStatsSetEnable (FALSE);
        }
        else
        {
            CtcStatsClear();
            CtcStatsSetEnable (TRUE);
            CtcStatsSetPeriod(EArg->eponparm.ctcstatsperiod);
        }		
    }
    else if (EArg->ope == EponGetOpe)
    {		
        if (CtcStatsGetEnable())
        {
            EArg->eponparm.ctcstatsperiod = CtcStatsGetPeriod();
        }
        else
        {
            EArg->eponparm.ctcstatsperiod = 0;
        }
    }
    
    return ret;    
}

int EponCfgCtcStats(EponCtlParamt* EArg)
{
    U8 count;
    int ret = EponSTATUSSUCCESS;
    StatsCntOne almbuf[CtcStatIdNums+1];
    
    memset(almbuf,0,sizeof(almbuf));
    DumpData(DebugIoctl,"EponCfgCtcStats",(U8 *)EArg,sizeof(EponCtlParamt));
    count = EArg->eponparm.ctcstats.statsCount;
	
    if (EArg->ope == EponGetOpe)
    {
        if ((count != 0) && (count <= CtcStatIdNums))
        {
            U8 i;
            copy_from_user((void *)almbuf,(void *)EArg->eponparm.ctcstats.statsVal,
				sizeof(StatsCntOne)*count);
            for (i = 0;i < count;i++)
            {
                if (!CtcStatsStatsVaild(almbuf[i].statsId))
                {
                    break;
                }				
            }
            
            if (i == count)
            {
                for (i = 0;i < count;i++)
                {
                    if (EArg->eponparm.ctcstats.history)
                    {
                        if (CtcStatsGetEnable())
                        {
                            CtcStatsGetHisStats(almbuf[i].statsId,&almbuf[i].statsVal);
                            almbuf[i].errorid = TRUE;
                        }
                        else
                        {
                            almbuf[i].errorid = FALSE;
                        }
                    }
                    else
                    {
                        CtcStatsGetStats(almbuf[i].statsId,&almbuf[i].statsVal);
                        almbuf[i].errorid = TRUE;
                    }
                }
                copy_to_user((void *)EArg->eponparm.ctcstats.statsVal,almbuf, 
					sizeof(StatsCntOne)*count);
            }
        }
    }
    else
    {
        CtcStatsClear();
    }
    
    return ret;    
}


static int burst_cap_struct_to_buf(const rdpa_epon_burst_cap_per_priority_t *bcap_struct,
    uint16_t *bcap_buf, uint8_t *count)
{
    if ( !bcap_struct || !bcap_buf)
        return -1;

    bcap_buf[0] = bcap_struct->priority_0;
    *count = 1;

    if (bcap_struct->priority_1 != 0)
    {
        *count = 8;
        bcap_buf[1] = bcap_struct->priority_1;
        bcap_buf[2] = bcap_struct->priority_2;
        bcap_buf[3] = bcap_struct->priority_3;
        bcap_buf[4] = bcap_struct->priority_4;
        bcap_buf[5] = bcap_struct->priority_5;
        bcap_buf[6] = bcap_struct->priority_6;
        bcap_buf[7] = bcap_struct->priority_7;
    }

    return 0;
}

static int buf_to_burst_cap_struct(rdpa_epon_burst_cap_per_priority_t *bcap_struct,
    const uint16_t *bcap_buf, uint8_t *count)
{
    if ( !bcap_struct || !bcap_buf)
        return -1;

    bcap_struct->priority_0 = bcap_buf[0]; 
    *count = 1;

    if (bcap_buf[1] != 0)
    {
        *count = 8;
        bcap_struct->priority_1= bcap_buf[1] ;
        bcap_struct->priority_2= bcap_buf[2] ;
        bcap_struct->priority_3= bcap_buf[3] ;
        bcap_struct->priority_4= bcap_buf[4] ;
        bcap_struct->priority_5= bcap_buf[5] ;
        bcap_struct->priority_6= bcap_buf[6] ;
        bcap_struct->priority_7= bcap_buf[7] ;
    }

    return 0;
}

int EponCfgBurstCap(EponCtlParamt* EArg)
{
    U8 link;
    int ret = EponSTATUSSUCCESS;
    int rc = BDMF_ERR_PARM;
    bdmf_object_handle epon_obj = NULL;
    uint16_t burst_cap_in[8];
    
    rdpa_epon_get(&epon_obj);
    if(!epon_obj)
        goto error; 

    DumpData(DebugIoctl,"EponCfgBurstCap",(U8 *)EArg,sizeof(EponCtlParamt));

    link = EArg->eponparm.bcapval.link;

    if (EArg->ope == EponSetOpe)
    {
        rdpa_epon_burst_cap_per_priority_t burst_cap_val = {};
        uint8_t count = 0;
        
        copy_from_user(burst_cap_in, (void *)EArg->eponparm.bcapval.bcapsize, sizeof(burst_cap_in));
        rc = buf_to_burst_cap_struct(&burst_cap_val, burst_cap_in, &count); 
        rc ? rc : rdpa_epon_burst_cap_map_set(epon_obj, link, &burst_cap_val);
    }
    else if (EArg->ope == EponGetOpe)
    {
        rdpa_epon_burst_cap_per_priority_t burst_cap_val = {};

        rc = rdpa_epon_burst_cap_map_get(epon_obj, link, &burst_cap_val);
        rc ? rc : burst_cap_struct_to_buf(&burst_cap_val, burst_cap_in, &EArg->eponparm.bcapval.count);
        if(!rc)
            copy_to_user((void *)EArg->eponparm.bcapval.bcapsize, burst_cap_in, sizeof(burst_cap_in));
    }

    if(!rc)
        ret = EponSTATUSSUCCESS;

error:
    if(epon_obj)
        bdmf_put(epon_obj);
    return ret;    
}

int EponCfgShaper(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgShaper",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        if (EArg->eponparm.shpval.add == FALSE)
        {
            OntDirShaperDel(EArg->eponparm.shpval.shpid);
        }
        else
        {
            OntDirShaperSet(EArg->eponparm.shpval.l1map,
		            EArg->eponparm.shpval.rate,
		            EArg->eponparm.shpval.size,
		            &EArg->eponparm.shpval.shpid);
        }
    }
    else if (EArg->ope == EponGetOpe)
    {	
        OntDirShaperGet(EArg->eponparm.shpval.shpid,
		        &EArg->eponparm.shpval.l1map,
		        &EArg->eponparm.shpval.rate,
		        &EArg->eponparm.shpval.size);
    }
    
    return ret;    
}

int EponGetKeyIndexUsed(EponCtlParamt* EArg)
{
    U8 link;
    U8 index;
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponGetKeyIndexUsed",(U8 *)EArg,sizeof(EponCtlParamt));

    if (EArg->ope == EponGetOpe)
    {
        link = EArg->eponparm.keyinuse.link;
        index = EncryptKeyInUse(link);
        EArg->eponparm.keyinuse.keyinUse = index;
    }
    
    return ret;    
}

int EponCfgKeyMode(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgKeyMode",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        if (EncryptLinkSet(EArg->eponparm.keymode.link,
		        EArg->eponparm.keymode.mode,
		        EArg->eponparm.keymode.opts) == FALSE)
        {
            ret = EponSTATUSERROR;
        }
    }
    else if (EArg->ope == EponGetOpe)
    {	
        EArg->eponparm.keymode.mode = EncryptModeGet(EArg->eponparm.keymode.link);
        EArg->eponparm.keymode.opts = EncryptOptsGet(EArg->eponparm.keymode.link);
    }
    
    return ret;    
}

int EponCfgKeyData(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    U32 keyData[4], sciData[2];
    
    memset(keyData, 0, sizeof(keyData));
    DumpData(DebugIoctl,"EponCfgKeyData",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        copy_from_user((void*)keyData, (void*)EArg->eponparm.keydata.key, 
			EArg->eponparm.keydata.length);
        copy_from_user((void*)sciData, (void*)EArg->eponparm.keydata.sci, sizeof(sciData));
        if (EncryptKeySet(EArg->eponparm.keydata.link,
    		    EArg->eponparm.keydata.dir,
    		    EArg->eponparm.keydata.keyindex,
    		    EArg->eponparm.keydata.length,
    		    keyData,
    		    sciData) == FALSE)
        {
            ret = EponSTATUSERROR;
        }
    }
    
    return ret;
}

int EponCfgHoldover(EponCtlParamt* EArg)
{
    int ret = EponSTATUSPARAMETERERROR;
    bdmf_object_handle epon_obj = NULL;
    int rc = BDMF_ERR_NOENT;

    rc = rdpa_epon_get(&epon_obj);
    if (rc)
        goto error;

    DumpData(DebugIoctl,"EponCfgHoldover",(U8 *)EArg,sizeof(EponCtlParamt));

    if (EArg->ope == EponSetOpe)
        rdpa_epon_holdover_set(epon_obj, &EArg->eponparm.holdover);
    else if (EArg->ope == EponGetOpe)
        rdpa_epon_holdover_get(epon_obj, &EArg->eponparm.holdover);
    else
        goto error;

    ret = EponSTATUSSUCCESS;

error:
    if (epon_obj)
        bdmf_put(epon_obj);
    return ret;
}

int EponCfgL1ByteLimit(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
	
    DumpData(DebugIoctl,"EponCfgL1ByteLimit",(U8 *)&EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        OntDirL1ByteLimitSet(EArg->eponparm.bytelimit.queue,
           EArg->eponparm.bytelimit.limit);
    }
    else if (EArg->ope == EponGetOpe)
    {		
        OntDirL1ByteLimitGet(EArg->eponparm.bytelimit.queue,
           &EArg->eponparm.bytelimit.limit);
    }
    return ret;
}

int EponCfgLosCheckTime(EponCtlParamt* EArg)
{
    int ret = EponSTATUSPARAMETERERROR;
    int rc = BDMF_ERR_PARM;
    bdmf_object_handle epon_obj = NULL;
    rdpa_epon_los_t los = {};

    DumpData(DebugIoctl,"EponCfgLosCheckTime",(U8 *)EArg,sizeof(EponCtlParamt));

    rc = rdpa_epon_get(&epon_obj);
    if (rc)
        goto error;

 
    if (EArg->ope == EponSetOpe)
    {
        los.pon = EArg->eponparm.lostime.losopttime;
        los.gate = EArg->eponparm.lostime.losmpcptime;
        rc = rdpa_epon_los_threshold_set(epon_obj,&los);
    }
    else if (EArg->ope == EponGetOpe)
    {
        rc = rdpa_epon_los_threshold_get(epon_obj,&los);
        EArg->eponparm.lostime.losopttime  = los.pon;
        EArg->eponparm.lostime.losmpcptime = los.gate;
    }

    if(!rc)
        ret = EponSTATUSSUCCESS;
    
error:
    if(epon_obj)
        bdmf_put(epon_obj);
    
    return ret;    
}

int EponSetNetLinkPid(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponSetNetLinkPid",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        EponNLSetPid(EArg->eponparm.nlpid);
    }
    
    return ret;    
}

int EponSetStaticLink(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
	
    DumpData(DebugIoctl,"EponSetStaticLink",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        PonMgrCreateLink(EArg->eponparm.staticlink.link.link,
				EArg->eponparm.staticlink.link.LLID);
    }
    
    return ret;    
}


int EponCfgUserTraffic(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgUserTraffic",(U8 *)EArg,sizeof(EponCtlParamt));
	
    if (EArg->ope == EponSetOpe)
    {
        PonMgrUserTrafficSet(EArg->eponparm.userTraffic.link, 
			EArg->eponparm.userTraffic.state);
    }
    else if (EArg->ope == EponGetOpe)
    {
        PonMgrUserTrafficGet(EArg->eponparm.userTraffic.link,
        		&EArg->eponparm.userTraffic.state);
    }
    
    return ret;    
}


int EponCfgLinkLoopback(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    LinkLoopbackPara *linkLoopback = &EArg->eponparm.loopback;
    U8 loopbackState;
    
    DumpData(DebugIoctl,"EponCfgLinkLoopback",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        if (!OntDirEponLinkLoopbackSet(linkLoopback->link, linkLoopback->isOn))
            ret = EponSTATUSERROR;
    }
    else
    {       
        if (!OntDirEponLinkLoopbackGet(linkLoopback->link, &loopbackState))
        {
            ret = EponSTATUSERROR;
        }
        else 
        {
            EArg->eponparm.loopback.isOn = loopbackState;
        }
    }
    
    return ret;    
}

#ifdef CONFIG_EPON_CLOCK_TRANSPORT
int EponCfgClkTransport(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    ClkTransPara *clkTransport = &EArg->eponparm.clkTrans;
    
    DumpData(DebugIoctl,"EponCfgClkTransport",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        if (!ClockTransportHandleCtlMsg(clkTransport))
            ret = EponSTATUSERROR;
    }
   
    return ret;    
}

int EponCfgClk1ppsTickTrans(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgClk1ppsTickTrans",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        ClkTrans1ppsTickCallback (EArg->eponparm.extmpcpClk.nextPulseTime);
    }
   
    return ret;    
}

int EponCfgClk1ppsCompTrans(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgClk1ppsCompTrans",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        ClkTrans1ppsCompCallback(&EArg->eponparm.clkTransCfg);
    }

    return ret;    
}

int EponCfgClkTodTrans(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    
    DumpData(DebugIoctl,"EponCfgClkTodTrans",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        ClkTransTodCallback(&EArg->eponparm.extTod);
    }
   
    return ret;    
}
#endif


int EponCfgRogueOnuDet(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    EponRogueOnuPara *rogueOnuCfg = &EArg->eponparm.rogueonudetect;
    
    DumpData(DebugIoctl,
        "EponCfgRogueOnuDetection",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        if (!EponRogueDetSet(rogueOnuCfg->enable, rogueOnuCfg->threshold,
            rogueOnuCfg->times))
        {
            ret = EponSTATUSERROR;
        }
    }
    else
    {       
        if (!EponRogueDetGet(&rogueOnuCfg->enable, &rogueOnuCfg->threshold,
            &rogueOnuCfg->times))
        {
            ret = EponSTATUSERROR;
        }
    }
    
    return ret;       
}


int EponCfgFailSafe(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    BOOL failSafe = (BOOL)(EArg->eponparm.failsafe);
    
    DumpData(DebugIoctl,
        "EponCfgFailSafe",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope == EponSetOpe)
    {
        OntmMpcpSetFailsafeEnabled(failSafe);
    }
    else
    {
        EArg->eponparm.failsafe = (U8)OntmMpcpIsFailsafeEnabled();
    }
    
    return ret;       
}

int EponCfgCfgMaxFrameSize(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    EponCfgParam ponCfg;
    
    DumpData(DebugIoctl,
        "EponCfgCfgMaxFrameSize",(U8 *)EArg,sizeof(EponCtlParamt));
    
    if (EArg->ope != EponSetOpe) 
        return ret;
               
    memset(&ponCfg,0,sizeof(EponCfgParam));    
    ponCfg.index = EponCfgItemPortMaxFrameSize;
    ponCfg.eponparm.maxFrameSize = EArg->eponparm.poncfg->eponparm.maxFrameSize;

    local_bh_disable();
    PonMgrMaxFrameSizeSet(ponCfg.eponparm.maxFrameSize);
    PonCfgDbSetPonParam(&ponCfg);
    local_bh_enable();

    return ret;       
}


int EponCfgDumpEpnInfo(EponCtlParamt* EArg)
{
    int ret = EponSTATUSSUCCESS;
    U32 epnSetVal = EArg->eponparm.epninfo.epnSetVal;
    
    DumpData(DebugIoctl,
        "EponCfgFailSafe",(U8 *)EArg,sizeof(EponCtlParamt));

    if (EArg->ope == EponSetOpe && EArg->eponparm.epninfo.epnInfoId == EpnInfoL1Cnt)
    {
        CmdL1AccSel(epnSetVal);
    }
    else if(EArg->ope == EponGetOpe && EArg->eponparm.epninfo.epnInfoId == EpnInfoL1Cnt)
    {
        CmdEponEpnInfoShow(EpnInfoL1Cnt);
    }
    else if(EArg->ope == EponGetOpe && EArg->eponparm.epninfo.epnInfoId == EpnInfoInterrupt)
    {
        CmdEponEpnInfoShow(EpnInfoInterrupt);
    }
    else
        ret = EponSTATUSPARAMETERERROR;
		
    return ret;       
}


EponIoctlHandle_t  ioctlFuncs[] =  //IOTCL function handle
{
    {BCMEPONCfgPers ,        EponCfg}, //Pers configuration
    {BCMEPONCfgDebug,      EponCfgDebug},//Debug enable or disable
    {BCMEPONCfgReg,          EponCfgReg},//Register configuration 
    {BCMEPONCfgReport,      EponCfgReportMode},//report mode configuration
    {BCMEPONCfgStats,        EponStats},//Stats Get/Clear
    {BCMEPONGetLLID,         EponCfgGetLLID},//Physic LLID get
    {BCMEPONCfgLink,          EponCfgLinkNum},//Link number configuration
    {BCMEPONCfgQueue,       EponCfgEponMacQueue},//Link queue report configuration
    {BCMEPONGetMpcpStatus,EponGetMpcpStatus},//MPCP stats configuration
    {BCMEPONCfgFec,            EponCfgFec},//FEC configuration
    {BCMEPONCfgProtParm,    EponCfgProtectSwitch},//Protect switch configuration
    {BCMEPONCfgTxPower ,    EponCfgTxPower},//Tx Power configuration
    {BCMEPONDumpStats,      EponDumpStats},//Dump stats for Lif/Xif/Xpcs32/Xpcs40/LINK
    {BCMEPONGather,            EponGatherEn},//Gather enable or disable
    {BCMEPONSilence,            EponSetSilence},//Set Silence parameter
    {BCMEPONBCap,               EponCfgBurstCap},//Burst Cap configuration
    {BCMEPONKeyInuse,         EponGetKeyIndexUsed},
    {BCMEPONKeyMode,          EponCfgKeyMode},
    {BCMEPONKeyData,           EponCfgKeyData},
    {BCMEPONShaper,             EponCfgShaper},
    {BCMEPONCtcAlm,             EponCfgCtcAlm},
    {BCMEPONCtcThe,             EponCfgCtcThe},
    {BCMEPONCtcPer,              EponCfgCtcPeriod},
    {BCMEPONCtcStats,            EponCfgCtcStats},
    {BCMEPONCtcDeallocSilence,NULL},
    {BCMEPONHoldover,          EponCfgHoldover},
    {BCMEPONLosTime,          EponCfgLosCheckTime},
    {BCMEPONSetPid,             EponSetNetLinkPid},
    {BCMEPONLoadPers,     EponCfgEponUsr},
    {BCMEPONCfgL2PonState,     EponCfgL2PonState},
    {BCMEPONCfgLaserEn ,    EponCfgLaserEn}, //Laser tx/rx enable/disable
    {BCMEPONCfgAssignMcast ,    EponCfgAssignMcast},
    {BCMEPONCfgByteLimit ,    EponCfgL1ByteLimit}, 
    {BCMEPONCfgLoopback ,    EponCfgLinkLoopback},
    {BCMEPONCfgRogueOnuDet ,    EponCfgRogueOnuDet},
    {BCMEPONCfgFailSafe ,    EponCfgFailSafe},
    {BCMEPONCfgMaxFrameSize, EponCfgCfgMaxFrameSize},
#ifdef CONFIG_EPON_CLOCK_TRANSPORT    
    {BCMEPONCfgClktrans ,    EponCfgClkTransport},
    {BCMEPONCfgClk1ppsTickTrans, EponCfgClk1ppsTickTrans},
    {BCMEPONCfgClk1ppsCompTrans, EponCfgClk1ppsCompTrans},
    {BCMEPONCfgClkTodTrans,     EponCfgClkTodTrans},
#endif
    {BCMEPONDumpEpnInfo, EponCfgDumpEpnInfo}
};

long EponIoctlAcess(unsigned int cmd, unsigned long arg)
{
    EponCtlParamt EArg;
    int ret = EponSTATUSSUCCESS;
    unsigned int cmdnr = _IOC_NR(cmd);
    
    if ((cmdnr >= BCMEPONCfgPers && cmdnr < BCM_EPON_IOCTL_MAX) &&
	    (ioctlFuncs[cmdnr].cmd == cmdnr) &&
	    (ioctlFuncs[cmdnr].fun != NULL))
    {
        //Get data from userspace.
        memset(&EArg, 0, sizeof(EponCtlParamt));
        copy_from_user(&EArg,(void *)arg,sizeof(EponCtlParamt));
        
        ret = (*ioctlFuncs[cmdnr].fun) (&EArg);
        copy_to_user((void *)arg, (void *)&EArg, sizeof(EponCtlParamt));
    }
    else
    {
        ret = EponSTATUSERROR;
    }
    
    return ret;
}
// end of Epon_ctrl.c

