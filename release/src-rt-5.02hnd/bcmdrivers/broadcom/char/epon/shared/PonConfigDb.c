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
#include "bcm_epon_cfg.h"
#include "PonConfigDb.h"
#include "Holdover.h"
#include "ProtSwitch.h"
#include "EponFrame.h"
#include "EponUser.h"
#include "board.h"

static PonCfg ponCfg;
static PonSchMode ponScheduling;
static MacAddr eponBaseMac;
static U8 maxLinkNum;
static rdpa_epon_holdover_t eponHoldCfg;
static U16 eponActiveTime;
static U16 eponStandbyTime;
static PsConfig10G eponPs10g;
static BOOL eponShaperIpg;
static U8 eponOamSel;
static U8 eponIdleTimeOffset;

////////////////////////////////////////////////////////////////////////////////
/// \brief  Return if we are configured for 3715 or SharedL2 mode
///
/// \param
/// None
///
/// \return
/// PonSchMode that is active
////////////////////////////////////////////////////////////////////////////////
//extern
PonSchMode PonCfgDbSchMode (void)
    {
    return ponScheduling;
    } // PonCfgDbSchMode


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return the idle time offset
///
/// \param
/// None
///
/// \return
/// the idle time offset
////////////////////////////////////////////////////////////////////////////////
//extern
U8 PonCfgDbIdleTimeOffset (void)
    {
    return eponIdleTimeOffset;
    } // PonCfgDbIdleTimeOffset


////////////////////////////////////////////////////////////////////////////////
//extern
PonPortCfg * PonCfgDbGet(PortInstance pon)
    {
    return &(ponCfg.pon[pon]);
    }



////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonCfgDbGetAvailable (void)
    {
    return ponCfg.pon[ponCfg.activePon].available;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return the PON downstream rate
///
/// \param
/// None
///
/// \return
/// Downstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
//extern
LaserRate PonCfgDbGetDnRate (void)
    {
    return ponCfg.pon[ponCfg.activePon].dnRate;
    } // PonCfgDbGetDnRate


////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON upstream rate
///
/// \param
/// None
///
/// \return
/// Upstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
//extern
LaserRate PonCfgDbGetUpRate (void)
    {
    return ponCfg.pon[ponCfg.activePon].upRate;
    } // PonCfgDbGetUpRate


////////////////////////////////////////////////////////////////////////////////
/// \brief Gets laser-On time
///
/// \param
/// None
///
/// \return
/// laser on Time (in TQ)
////////////////////////////////////////////////////////////////////////////////
//extern
U8 PonCfgDbGetLaserOnTime (void)
    {
    return (U8)ponCfg.pon[ponCfg.activePon].laserOn;
    } // PonCfgDbGetLaserOnTime


////////////////////////////////////////////////////////////////////////////////
/// \brief Gets laser-Off time
///
/// \param
/// None
///
/// \return
/// laser off Time (in TQ)
////////////////////////////////////////////////////////////////////////////////
//extern
U8 PonCfgDbGetLaserOffTime (void)
    {
    return (U8)ponCfg.pon[ponCfg.activePon].laserOff;
    } // PonCfgDbGetLaserOffTime


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void PonCfgDbSetRegCb(RegisteredCb cb)
    {
    ponCfg.cb = cb;
    } // PonCfgDbSetRegCb


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
RegisteredCb PonCfgDbGetRegCb(void)
    {
    return ponCfg.cb;
    } // PonCfgDbGetRegCb


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void PonCfgDbSetMpcpCb(MpcpCallback cb)
    {
    ponCfg.mpcpCb = cb;
    } // PonCfgDbSetRegCbCheckGates


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
MpcpCallback PonCfgDbGetMpcpCb(void)
    {
    return ponCfg.mpcpCb;
    } // PonCfgDbGetRegCbCheckGates


////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON upstream polarity
///
/// \param
/// None
///
/// \return
/// Upstream Pon Polarity
////////////////////////////////////////////////////////////////////////////////
//extern
Polarity PonCfgDbGetUpPolarity (void)
    {
    return ponCfg.pon[ponCfg.activePon].txPolarity;
    } // PonCfgDbGetUpPolarity


////////////////////////////////////////////////////////////////////////////////
/// \brief Initialize EPON port
///
/// Resets EPON MAC to default values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
PortInstance PonCfgDbGetActivePon (void)
    {
    return ponCfg.activePon;
    } // PonCfgDbGetActivePon;


////////////////////////////////////////////////////////////////////////////////
//extern
void PonCfgDbActivePonSet(PortInstance pon)
    {
    ponCfg.activePon = pon;
    } // PonCfgDbGetActivePon;


////////////////////////////////////////////////////////////////////////////////
/// \brief Get standby PON interface
///
/// \return
/// Standby PON
////////////////////////////////////////////////////////////////////////////////
//extern
PortInstance PonCfgDbGetStandbyPon (void)
    {
    return (ponCfg.activePon == 0) ? 1 : 0;
    } // PonCfgDbGetStandbyPon


////////////////////////////////////////////////////////////////////////////////
/// \brief Get off time offset of active PON
///
/// \return
/// Off time offset
////////////////////////////////////////////////////////////////////////////////
//extern
U8 PonCfgDbGetOffTimeOffset (void)
    {
    return ponCfg.pon[ponCfg.activePon].offTimeOffset;
    } // PonCfgDbGetOffTimeOffset


////////////////////////////////////////////////////////////////////////////////
/// \brief Get max frmae size of active PON
///
/// \return
/// Max frame size
////////////////////////////////////////////////////////////////////////////////
//extern
U16 PonCfgDbGetMaxFrameSize (void)
    {
    return ponCfg.pon[ponCfg.activePon].maxFrameSize;
    } // PonCfgDbGetMaxFrameSize


////////////////////////////////////////////////////////////////////////////////
/// \brief Get pre-draft 2.1 of active PON
///
/// \return
/// Pre draft 2.1
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonCfgDbGetPreDraft2Dot1 (void)
    {
    return ponCfg.pon[ponCfg.activePon].preDraft2dot1;
    } // PonCfgDbGetPreDraft2Dot1


////////////////////////////////////////////////////////////////////////////////
/// \brief Get NTT reporting of active PON
///
/// \return
/// NTT reporting
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonCfgDbGetNttReporting (void)
    {
    return ponCfg.pon[ponCfg.activePon].nttReporting;
    } // PonCfgDbGetNttReporting


////////////////////////////////////////////////////////////////////////////////
/// \brief Get laser powerup time
///
/// \param pon  Laser instance
///
/// \return
/// Laser powerup time
////////////////////////////////////////////////////////////////////////////////
//extern
U16 PonCfgDbGetPowerupTime (U8 pon)
    {
    return ponCfg.pon[pon].powerupTime;
    } // PonCfgDbGetPowerupTime

////////////////////////////////////////////////////////////////////////////////
/// \brief GettxoffIdle
///
/// \param none
///
/// \return
/// txOffIdle
////////////////////////////////////////////////////////////////////////////////

BOOL PonCfgDbGettxOffIdle (void)
    {
    return ponCfg.pon[ponCfg.activePon].txOffIdle;
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief Get laser control interface
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
Port PonCfgDbGetCtrlInf (U8 pon)
    {
    return ponCfg.pon[pon].ctrlInf;
    } // PonCfgDbGetPowerupTime


////////////////////////////////////////////////////////////////////////////////
/// \brief Get Holdover default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonGetHoldover(rdpa_epon_holdover_t *val)
    {
    val->time = eponHoldCfg.time;
    val->flags = eponHoldCfg.flags;
	return TRUE;
    } // PonCfgDbGetPowerupTime



////////////////////////////////////////////////////////////////////////////////
/// \brief Get active optctrl default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonGetActOptCtrl(U16 *time)
    {
    *time = eponActiveTime;
    return (*time != 0);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief Get protect switch configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonGetActPs(PsConfig10G *cfg)
    {
    cfg->type = eponPs10g.type;
    cfg->mode = eponPs10g.mode;
    return TRUE;
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief Get standby optctrl default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonGetStdOptCtrl(U16 *time)
    {
    *time = eponStandbyTime;
    return (*time != 0);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief Get IPG configuration.
///
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL PonCfgDbGetIpg (void)
    {
    return eponShaperIpg;
    } // PonCfgDbGetPowerupTime


MacAddr const* PonCfgDbGetBaseMacAddr(void)
    {
    return &eponBaseMac;
    } // PonCfgDbGetMacAddrPers


U8 PonCfgDbGetLinkNum(void)
    {
    return maxLinkNum;
    } // PonCfgDbGetLinkNumPers


void PonCfgDbSetPonParam(EponCfgParam *poncfgparm)
    {    
    U8 inst = poncfgparm->inst;
    switch (poncfgparm->index)
        {
        case EponCfgItemPortUpRate:
             if (poncfgparm->eponparm.upRate != 0xFF)
             	{
                ponCfg.pon[inst].upRate = poncfgparm->eponparm.upRate;
                EponUserConfigObjectRate(ponCfg.pon[inst].dnRate, ponCfg.pon[inst].upRate);
             	}
            break;

        case EponCfgItemPortDnRate:
             if (poncfgparm->eponparm.dnRate != 0xFF)
             	{
                ponCfg.pon[inst].dnRate = poncfgparm->eponparm.dnRate;
                EponUserConfigObjectRate(ponCfg.pon[inst].dnRate, ponCfg.pon[inst].upRate);
             	}
            break;

        case EponCfgItemPortLaserOn:
            ponCfg.pon[inst].laserOn = poncfgparm->eponparm.laserOn;
            break;

        case EponCfgItemPortLaserOff:
            ponCfg.pon[inst].laserOff = poncfgparm->eponparm.laserOff;
            break;

        case EponCfgItemPortTxPolarity:
            ponCfg.pon[inst].txPolarity = poncfgparm->eponparm.txPolarity;
            break;

        case EponCfgItemPortOffTimeOffset:
            ponCfg.pon[inst].offTimeOffset = poncfgparm->eponparm.offTimeOffset;
            break;

        case EponCfgItemPortMaxFrameSize:
            ponCfg.pon[inst].maxFrameSize = poncfgparm->eponparm.maxFrameSize;
            break;
            
        case EponCfgItemPortPreDraft2dot1:
            ponCfg.pon[inst].preDraft2dot1 = poncfgparm->eponparm.preDraft2dot1;
            break;
        
        case EponCfgItemPortNttReporting:
            ponCfg.pon[inst].nttReporting = poncfgparm->eponparm.txOffIdle;
            break;
                
        case EponCfgItemPortPowerupTime:
            ponCfg.pon[inst].powerupTime = poncfgparm->eponparm.powerupTime;
            break;
            
        case EponCfgItemPortTxOffIdle:
            ponCfg.pon[inst].txOffIdle = poncfgparm->eponparm.txOffIdle;
            break;
            
        case EponCfgItemShaperipg:
            eponShaperIpg = poncfgparm->eponparm.shaperipg;
            break;

        case EponCfgItemPonMac:
            memcpy(&eponBaseMac, &poncfgparm->eponparm.ponMac, sizeof(MacAddr));
            break;

        case EponCfgItemHoldover:
            eponHoldCfg.time = poncfgparm->eponparm.holdoverval.time;
            eponHoldCfg.flags = poncfgparm->eponparm.holdoverval.flags;
            break;

        case EponCfgItemActiveTime:
            eponActiveTime = poncfgparm->eponparm.activetime;
            break;

        case EponCfgItemStandbyTime:
            eponStandbyTime = poncfgparm->eponparm.standbytime;
            break;
            
        case EponCfgItemDefaultLinkNum:
            maxLinkNum = poncfgparm->eponparm.defaultlinknum;
            break;

        case EponCfgItemPsCfg:
            eponPs10g.type = poncfgparm->eponparm.pscfg.type;
            eponPs10g.mode = poncfgparm->eponparm.pscfg.mode;
            break;

        case EponCfgItemOamSel:
            eponOamSel = poncfgparm->eponparm.oamsel;
            EponDyingGaspOamOui(eponOamSel);
            break;
            
        case EponCfgItemSchMode:
            ponScheduling = poncfgparm->eponparm.schMode;
            break;

        case EponCfgItemIdileTimOffset:
            eponIdleTimeOffset = poncfgparm->eponparm.idleTimeOffset;
            break;

        default:
            break;
        }
    
    }


void PonCfgDbGetPonParam(EponCfgParam *poncfgparm)
    {
    U8 inst = poncfgparm->inst;
    switch (poncfgparm->index)
        {
        case EponCfgItemPortUpRate:
            poncfgparm->eponparm.upRate = ponCfg.pon[inst].upRate;
            break;

        case EponCfgItemPortDnRate:
            poncfgparm->eponparm.dnRate = ponCfg.pon[inst].dnRate;
            break;

        case EponCfgItemPortLaserOn:
            poncfgparm->eponparm.laserOn = ponCfg.pon[inst].laserOn;
            break;

        case EponCfgItemPortLaserOff:
            poncfgparm->eponparm.laserOff = ponCfg.pon[inst].laserOff;
            break;

        case EponCfgItemPortTxPolarity:
            poncfgparm->eponparm.txPolarity = ponCfg.pon[inst].txPolarity;
            break;

        case EponCfgItemPortOffTimeOffset:
            poncfgparm->eponparm.offTimeOffset = ponCfg.pon[inst].offTimeOffset;
            break;

        case EponCfgItemPortMaxFrameSize:
            poncfgparm->eponparm.maxFrameSize = ponCfg.pon[inst].maxFrameSize;
            break;
            
        case EponCfgItemPortPreDraft2dot1:
            poncfgparm->eponparm.preDraft2dot1 = ponCfg.pon[inst].preDraft2dot1;
            break;
        
        case EponCfgItemPortNttReporting:
            poncfgparm->eponparm.txOffIdle = ponCfg.pon[inst].nttReporting;
            break;
                
        case EponCfgItemPortPowerupTime:
            poncfgparm->eponparm.powerupTime = PonCfgDbGetPowerupTime(inst);
            break;
            
        case EponCfgItemPortTxOffIdle:
            poncfgparm->eponparm.txOffIdle = ponCfg.pon[inst].txOffIdle;
            break;
            
        case EponCfgItemShaperipg:
            poncfgparm->eponparm.shaperipg = PonCfgDbGetIpg();
            break;

        case EponCfgItemPonMac:
            memcpy(&poncfgparm->eponparm.ponMac, &eponBaseMac, sizeof(MacAddr));
            break;

        case EponCfgItemHoldover:
            (void)PonGetHoldover(&poncfgparm->eponparm.holdoverval);                 
            break;

        case EponCfgItemActiveTime:
            (void)PonGetActOptCtrl(&poncfgparm->eponparm.activetime);
            break;

        case EponCfgItemStandbyTime:
            (void)PonGetStdOptCtrl(&poncfgparm->eponparm.standbytime);
            break;
            
        case EponCfgItemDefaultLinkNum:
            poncfgparm->eponparm.defaultlinknum = PonCfgDbGetLinkNum();
            break;

        case EponCfgItemPsCfg:
            (void)PonGetActPs(&poncfgparm->eponparm.pscfg);
            break;

        case EponCfgItemOamSel:
            poncfgparm->eponparm.oamsel = eponOamSel;
            break;
            
        case EponCfgItemSchMode:
            poncfgparm->eponparm.schMode = PonCfgDbSchMode();
            break;

        case EponCfgItemIdileTimOffset:
            poncfgparm->eponparm.idleTimeOffset = PonCfgDbIdleTimeOffset();
            break;         
            
        default:
            break;
        }   
    }

void GetWanRate(LaserRate *dn_rate, LaserRate *up_rate)
    {
    char buf[PSP_BUFLEN_16];
    int count;
#define RATE_STR_LEN 2
#define PSP_RATE_STR_LEN 4

    *dn_rate = LaserRate1G;
    *up_rate = LaserRate1G;
	
    count = kerSysScratchPadGet(RDPA_WAN_RATE_PSP_KEY, buf, PSP_BUFLEN_16);
    if (count > 0)
        {
        if (strlen(buf) >= PSP_RATE_STR_LEN)
            {
            if (!strncmp(buf, RDPA_WAN_RATE_10G, RATE_STR_LEN))
                *dn_rate = LaserRate10G;
            else if(!strncmp(buf, RDPA_WAN_RATE_2_5G, RATE_STR_LEN))
                *dn_rate = LaserRate2G;
            else if(!strncmp(buf, RDPA_WAN_RATE_1G, RATE_STR_LEN))
                *dn_rate = LaserRate1G;
       
            if (!strncmp(&buf[RATE_STR_LEN], RDPA_WAN_RATE_10G, RATE_STR_LEN))
                *up_rate = LaserRate10G;
            else if (!strncmp(&buf[RATE_STR_LEN], RDPA_WAN_RATE_1G, RATE_STR_LEN))
                *up_rate = LaserRate1G;

            return;
            }
        }

    count = kerSysScratchPadGet(RDPA_EPON_SPEED_PSP_KEY, buf, PSP_BUFLEN_16);
    if (count > 0)
        {
        if (!strcmp(buf, EPON_SPEED_TURBO))
            {
            *dn_rate = LaserRate2G;
            }
        }
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief Read and parse personality record.
///
/// \param
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void ReadPonDefCfg (void)
    {
    U8  ponIf = NumPonIf;

    memset (&ponCfg, 0x00, sizeof (ponCfg));
    ponCfg.activePon = NoActivePon;
    
    eponBaseMac.byte[0] = 0x00;
    eponBaseMac.byte[1] = 0x0D;
    eponBaseMac.byte[2] = 0xB6;
    eponBaseMac.byte[3] = 0xC0;
    eponBaseMac.byte[4] = 0x03;
    eponBaseMac.byte[5] = 0xC0;

    maxLinkNum = GetEponDefLinkNum();

    eponHoldCfg.time = GetEponDefHoldOverTime();
    eponHoldCfg.flags = GetEponDefHoldOverFlags();

    eponActiveTime = GetEponDefActivetime();
    eponStandbyTime = GetEponDefStandbytime();

    eponPs10g.type = GetEponDefPsConfig();
    eponPs10g.mode = GetEponDefPsMode();

    eponShaperIpg = GetEponDefShaperIpg();
    ponScheduling = GetEponDefSchMode();
    eponIdleTimeOffset = GetEponDefIdleTimeOffset();

    for (ponIf = 0; ponIf < NumPonIf; ponIf++)
        {
        ponCfg.pon[ponIf].available = TRUE;

        if (ponCfg.pon[ponIf].available)
            {
            GetWanRate(&ponCfg.pon[ponIf].dnRate, &ponCfg.pon[ponIf].upRate);
            ponCfg.pon[ponIf].laserOn = GetEponDefLaserOn(ponIf);
            ponCfg.pon[ponIf].laserOff = GetEponDefLaserOff(ponIf);
            ponCfg.pon[ponIf].txPolarity = (Polarity)GetEponDefTxPolarity(ponIf);
            ponCfg.pon[ponIf].offTimeOffset = GetEponDefOffTimeOffset(ponIf);
            ponCfg.pon[ponIf].maxFrameSize = GetEponDefMaxFrameSize(ponIf);
            ponCfg.pon[ponIf].preDraft2dot1 = (GetEponDefPreDraft2dot1(ponIf) == Available);
            ponCfg.pon[ponIf].nttReporting = (GetEponDefNttReporting(ponIf) == Available);
            ponCfg.pon[ponIf].powerupTime = GetEponDefPowerupTime(ponIf);
            ponCfg.pon[ponIf].txOffIdle = GetEponDefTxOffIdle(ponIf);
            ponCfg.pon[ponIf].ctrlInf.pift = PortIfPon;
            ponCfg.pon[ponIf].ctrlInf.inst = ponIf;

            if (ponCfg.activePon == NoActivePon)
                {
                ponCfg.activePon = ponIf;
                }
            }
        }

    if (ponCfg.activePon == NoActivePon)
        {
        ponCfg.activePon = 0;
        }
    } // ReadPersonality

////////////////////////////////////////////////////////////////////////////////
/// \brief sets up uni config DB
///
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
//extern
void PonCfgDbInitPon(void)
    {    
    ReadPonDefCfg();
    }  // PonCfgDbInitPonPers

// PonConfigDb.c

