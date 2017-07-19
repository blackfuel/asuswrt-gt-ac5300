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
File Name  : OntDirector.c
Description: Broadcom EPON  Interface Driver
This file mainly support the EPON stack manager interface for kernel space. 
All the functions in this files are set to "EXPORT_SYMBOL". So other module only call the API in 
this file to communicate with EPON stack.
*/    
//**************************************************************************

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/irq.h>

#include "OntDirector.h"
#include "EponUser.h"
#include "bcm_OS_Deps.h"
#include <linux/bcm_log.h>

#include <rdpa_api.h>
#include <rdpa_types.h>
#include <autogen/rdpa_ag_epon.h>

static  DEFINE_SPINLOCK(ontmgr_spinlock);
static PonShaper LinkShaperIdx[TkOnuMaxBiDirLlids];
static OntShaper shaperprofile[EponUpstreamShaperCount];
static PonShaper globalShaperIndex = PonShaperBad;
extern U16 linkBcap[TkOnuMaxBiDirLlids][EponMaxPri];
static epon_mac_q_cfg_t queue_cfg_rec;
rdpa_epon_mode oam_mode = rdpa_epon_ctc;

////////////////////////////////////////////////////////////////////////////////
/// \brief get the oam queue index of the link
///
/// \param link     Link index to set
///
/// \return
/// the oam queue index
////////////////////////////////////////////////////////////////////////////////
//extern
U8 OntDirLinkOamQGet(LinkIndex link)
    {
    U8 oamQ = PonMgrLinkOamQGet(link);
    return oamQ;
    }


////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirMacSet(LinkIndex link, U8 *Mac)
    {
    BOOL retVal = TRUE;
    MacAddr   zeromac;    
    if (!Mac)
        {
        return FALSE;
        }
    //Invalid MAC.
    memset(&zeromac,0,sizeof(MacAddr));
    if (memcmp(Mac,&zeromac,sizeof(MacAddr)) == 0)
        {
        return FALSE;
        }
    
    spin_lock_bh(&ontmgr_spinlock);    
    OntmMpcpSetNewMac(link,(MacAddr *)Mac);
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirMacSet);

BOOL OntDirMacGet(LinkIndex link, U8 *Mac)
    {
    BOOL retVal = TRUE;
    if (!Mac)
        {
        return FALSE;
        }
    spin_lock_bh(&ontmgr_spinlock);
    EponGetMac(link, (MacAddr *)Mac);
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirMacGet);


BOOL OntDirFecModeSet(LinkIndex link, BOOL rx, BOOL tx)
    {
    BOOL retVal = TRUE;
    spin_lock_bh(&ontmgr_spinlock);
    if ((rx != FecRxLinkState (link)) || (tx != FecTxLinkState (link)))
        {
        PonMgrStopLinks(1UL << link);
        retVal = FecModeSet(link, rx, tx, 3);
        PonMgrStartLinks(1UL << link);
        }
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirFecModeSet);


////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirFecModeGet(LinkIndex link, BOOL *rx, BOOL *tx)
    {
    BOOL retVal = TRUE;

    if ((!rx) || (!tx))
        {
        return FALSE;
        }
    spin_lock_bh(&ontmgr_spinlock);

    *rx = FecRxLinkState (link);
    *tx = FecTxLinkState (link);
    
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirFecModeGet);

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
LaserRate OntDirUpRateGet (void)
    {
    return PonCfgDbGetUpRate();
    }
    
EXPORT_SYMBOL(OntDirUpRateGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON downstream rate
///
/// \param
/// None
///
/// \return
/// Upstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
//extern
LaserRate OntDirDnRateGet (void)
    {
    return PonCfgDbGetDnRate();
    } 
    
EXPORT_SYMBOL(OntDirDnRateGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies a new burst cap to a link on the fly
///
/// \param link     Link index for burst cap
/// \param bcap     New burst cap values in a array of 16 bytes values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntDirBurstCapSet(LinkIndex link, const U16 * bcap)
    {
    spin_lock_bh(&ontmgr_spinlock);
    PonMgrStopLinks(1UL << link);
    PonMgrSetBurstCap(link, bcap);
    PonMgrStartLinks(1UL << link);
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirBurstCapSet);



////////////////////////////////////////////////////////////////////////////////
/// \brief Get burst cap to a link on the fly
///
/// \param link     Link index for burst cap
/// \param bcap     New burst cap values in a array of 16 bytes values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntDirBurstCapGet(LinkIndex link,  U16 * bcap)
    {   
    spin_lock_bh(&ontmgr_spinlock);
    memcpy(bcap,linkBcap[link],PonMgrRptModeToPri(PonMgrReportModeGet())*sizeof(U16));
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirBurstCapGet);


void OntDirLaserRxPowerSet(bdmf_boolean on)
    {
    spin_lock_bh(&ontmgr_spinlock);
    
    if (on)
        PonMgrRxEnable();
    else
        PonMgrRxDisable();
    
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirLaserRxPowerSet);

void OntDirLaserTxModeSet(rdpa_epon_laser_tx_mode mode)
    {
    spin_lock_bh(&ontmgr_spinlock);
    PonMgrSetLaserStatus(mode);
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirLaserTxModeSet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Applies get Layser status configuration
///
/// \param dir          upstream/dnstream
/// \param mode     pon configure mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntDirLaserTxModeGet(rdpa_epon_laser_tx_mode *mode)
    {
    spin_lock_bh(&ontmgr_spinlock);
    *mode = PonMgrGetLaserStatus();
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirLaserTxModeGet);


void OntDirLaserRxPowerGet(bdmf_boolean *enable)
    {
    spin_lock_bh(&ontmgr_spinlock);
    PonMgrLaserRxPowerGet(enable);
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirLaserRxPowerGet);


void OntDirHoldoverSet(rdpa_epon_holdover_t *holdover)
    {
    spin_lock_bh(&ontmgr_spinlock);
    HoldoverSetParams(holdover->time, holdover->flags);
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirHoldoverSet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies a new Layser status configuration
///
/// \param dir          upstream/dnstream
/// \param mode     pon configure mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLaserStatusSet(Direction dir, LaserTxMode mode)
    {

    if ((dir > Dnstream) || (mode >= LaserTxModeContinuous))
        {
        return FALSE;
        }
    spin_lock_bh(&ontmgr_spinlock);
    if (dir == Upstream)
        {
        PonMgrSetLaserStatus(mode);
        }
    else
        {
        if (mode == LaserTxModeOff)
            {
            PonMgrRxDisable();			
            }
        else
            {
            PonMgrRxEnable();
            }
        }
    spin_unlock_bh(&ontmgr_spinlock);
    return TRUE;
    }

EXPORT_SYMBOL(OntDirLaserStatusSet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies get Layser status configuration
///
/// \param dir          upstream/dnstream
/// \param mode     pon configure mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLaserStatusGet(Direction dir, LaserTxMode *mode)
	{
	
	if ((dir != Upstream) || (!mode))
		{
		return FALSE;
		}
	spin_lock_bh(&ontmgr_spinlock);
	*mode = PonMgrGetLaserStatus();
	spin_unlock_bh(&ontmgr_spinlock);
	return TRUE;
	}

EXPORT_SYMBOL(OntDirLaserStatusGet);


void OntDirHoldoverGet(rdpa_epon_holdover_t *holdover)
    {
    spin_lock_bh(&ontmgr_spinlock);
    holdover->time  = HoldoverGetTime();
    holdover->flags  = HoldoverGetFlags();
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirHoldoverGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies protect switch configuration
///
/// \param state         protect state
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirProtectSwitchSet(PsOnuState state)
    {
    spin_lock_bh(&ontmgr_spinlock);
    PsSetState(state);
    spin_unlock_bh(&ontmgr_spinlock);
    return TRUE;
    }

EXPORT_SYMBOL(OntDirProtectSwitchSet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get protect switch configuration
///
/// \param state         protect state
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirProtectSwitchGet(PsOnuState *state)
    {
    spin_lock_bh(&ontmgr_spinlock);
    *state = PsGetState();
    spin_unlock_bh(&ontmgr_spinlock);
    return TRUE;
    }

EXPORT_SYMBOL(OntDirProtectSwitchGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief get link stats
///
/// \param link          link value
/// \param id            stats id
/// \param dptr        pointer to the stat value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern

void OntDirMpcpStateGet(LinkIndex link, rdpa_epon_link_mpcp_state *state)
    {
    spin_lock_bh(&ontmgr_spinlock);
    *state = OntmMpcpLinkStateGet(link);
    spin_unlock_bh(&ontmgr_spinlock);
    }

EXPORT_SYMBOL(OntDirMpcpStateGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set New link numbers need to register.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
int OntDirNewLinkNumSet(U8 newlinks)
    {
    if (newlinks > TkOnuNumBiDirLlids)
        return -1;

    spin_lock_bh(&ontmgr_spinlock);
    OntmMpcpAdjustNumLinks(newlinks);
    spin_unlock_bh(&ontmgr_spinlock);
    
    return 0;
    }

EXPORT_SYMBOL(OntDirNewLinkNumSet);



////////////////////////////////////////////////////////////////////////////////
/// \brief Get register link numbers.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
int OntDirRegLinkNumGet(U8 *reglinks, U8 *onlinelinks)
    {
    
    if (!reglinks || !onlinelinks)
        return -1;
    
    spin_lock_bh(&ontmgr_spinlock);
    *reglinks = OntmMpcpGetNumLinks();
    *onlinelinks = OntmMpcpLinksRegistered();
    spin_unlock_bh(&ontmgr_spinlock);
    
    return 0;
    }

EXPORT_SYMBOL(OntDirRegLinkNumGet);	


////////////////////////////////////////////////////////////////////////////////
/// \brief get link stats
///
/// \param link          link value
/// \param id            stats id
/// \param dptr        pointer to the stat value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLinkStatsGet(LinkIndex link, StatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetLink(link,id,dst);
    return retVal;
    }


EXPORT_SYMBOL(OntDirLinkStatsGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
/// \param dptr    pointer to the stat value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirPonStatsGet(StatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetEpon(id,dst);
    return retVal;
    }

EXPORT_SYMBOL(OntDirPonStatsGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLifStatsGet(LifStatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetLif(id,dst);
    return retVal;
    }

EXPORT_SYMBOL(OntDirLifStatsGet);

#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirXifStatsGet(XifStatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetXif(id,dst);
    return retVal;
    }

EXPORT_SYMBOL(OntDirXifStatsGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirXpcs32StatsGet(Xpcs32RxStatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetXpcs32(id,dst);
    return retVal;
    }

EXPORT_SYMBOL(OntDirXpcs32StatsGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirXpcs40StatsGet(Xpcs40RxStatId id, U64  *dst)
    {
    BOOL retVal = TRUE;
    if (!dst)
        {
        return FALSE;
        }
    retVal = StatsGetXpcs40(id,dst);
    return retVal;
    }

EXPORT_SYMBOL(OntDirXpcs40StatsGet);
#endif


BOOL ont_dir_queue_set(epon_mac_q_cfg_t *cfg)
    {

    LinkMap  oldStartedLinks = 0;
    BOOL retVal = TRUE;

    spin_lock_bh(&ontmgr_spinlock);
    
    oldStartedLinks = PonMgrStartedLinksGet();
    PonMgrStopLinks(AllLinks);
    
    memcpy(&queue_cfg_rec, cfg, sizeof(epon_mac_q_cfg_t));
    
    PonMgrSetReporting(cfg->rpt_mode);
    
    pon_mgr_q_cfg_start();

    PonMgrLinkQueueConfig(cfg);

    PonMgrL2Cfg(cfg->link_num);
    
    oldStartedLinks &= ~(AllLinks << cfg->link_num);
    PonMgrStartLinks(AllLinks & oldStartedLinks);
    
    spin_unlock_bh(&ontmgr_spinlock);

    return retVal;
    }

EXPORT_SYMBOL(ont_dir_queue_set);


BOOL ont_dir_queue_get(epon_mac_q_cfg_t *cfg)
    {   
    spin_lock_bh(&ontmgr_spinlock);
    memcpy(cfg, &queue_cfg_rec, sizeof(epon_mac_q_cfg_t));
    spin_unlock_bh(&ontmgr_spinlock);
    return TRUE;
    }

EXPORT_SYMBOL(ont_dir_queue_get);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set upstream global shaper for whole ONU
///
/// \param rate            rate to set
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirGlobalShaperSet(U32 rate)
    {
    U32 l1Map = 0;

    l1Map = PonMgrDataQueueL1MapGet(LinkIndexAll);
	
    OntDirShaperDel (globalShaperIndex);
    return OntDirShaperSet (l1Map,rate,OnuShaperDefBurstSize,&globalShaperIndex);
    }

EXPORT_SYMBOL(OntDirGlobalShaperSet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set upstream shaper for a link
///
/// \param rate            rate to set, kbps,
/// \param burst           kBytes
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
//extern
int OntDirLinkShaperSet(LinkIndex link, U32 rate, PonMaxBurstSize burst)
    {
    U32 l1Map = 0;
    int retVal = 0;
    PonShaper *shp = &LinkShaperIdx[link];
    U32 rateEpon, burstEpon;
    
    l1Map = PonMgrDataQueueL1MapGet(link);
    
    OntDirShaperDel(*shp);

    if((rate != 0) || ( burst != 0))
        {
        /* In EPON, rate unit is bps, burst unit is 256bytes */
        rateEpon = rate;
        burstEpon = burst/8/256; /* bps -> 256bytes */
        retVal = OntDirShaperSet (l1Map,rateEpon,burstEpon,shp)? 0 : (-1);
        }
    else
        {
        *shp = PonShaperBad;
        }
    
    return retVal;
    }

EXPORT_SYMBOL(OntDirLinkShaperSet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get upstream shaper for a link
///
/// \param rate            rate
/// \param burst          burst
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLinkShaperGet(LinkIndex link, U32 *rate, PonMaxBurstSize *burst)
    {
    U32 l1Map = 0;
    BOOL retVal = TRUE;
    PonShaper shp = LinkShaperIdx[link];
    U32 rateEpon;
    U16 burstEpon;
    if(shp != PonShaperBad)
        { 
        retVal = OntDirShaperGet(shp, &l1Map, &rateEpon, &burstEpon);
        /* In EPON, rate unit is ~2kbps, burst unit is 256bytes */
        *rate = rateEpon*2;
        *burst = burstEpon*256/1000;
        }
    else
        {
        *rate = 0;
        *burst = 0;
        }
    
    return retVal;
    }

EXPORT_SYMBOL(OntDirLinkShaperGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set Link Queue configuration
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirShaperSet(U32 shaperL1Map,	PonShaperRate rate,
						  PonMaxBurstSize size,PonShaper *shp)
    {
    U8 i;
    BOOL retVal = FALSE;
    if (!shp)
        {
        return FALSE;
        }
	
    spin_lock_bh(&ontmgr_spinlock);
    for (i = 0; i < EponUpstreamShaperCount; ++i)
        {
        if ((shaperprofile[i].queueMap == shaperL1Map)&&
        	(shaperprofile[i].rate == rate)&&
        	(shaperprofile[i].burstSize == size))
            {
            *shp = i;
            spin_unlock_bh(&ontmgr_spinlock);
            return TRUE;
            }
        }
	
    *shp = PonMgrAddShaper(shaperL1Map,rate,size);
    if (*shp < EponUpstreamShaperCount)
        {
        shaperprofile[*shp].queueMap = shaperL1Map;
        shaperprofile[*shp].rate = rate;
        shaperprofile[*shp].burstSize = size;
        retVal = TRUE;
        }
	
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirShaperSet);


BOOL OntDirShaperDel(PonShaper shp)
    {
    BOOL retVal = TRUE;
    
    spin_lock_bh(&ontmgr_spinlock);	
    PonMgrDelChannelShaper(shp);
    if (shp < EponUpstreamShaperCount)
        {
        memset(&shaperprofile[shp],0,sizeof(OntShaper));
        }
    spin_unlock_bh(&ontmgr_spinlock);
	
    return retVal;
    }

EXPORT_SYMBOL(OntDirShaperDel);


BOOL OntDirShaperGet(PonShaper shp, U32 *shaperL1Map,PonShaperRate *rate,
						  PonMaxBurstSize *size)
    {
    BOOL retVal = TRUE;
    
    spin_lock_bh(&ontmgr_spinlock);	
    if (shp < EponUpstreamShaperCount)
        {
        *shaperL1Map = shaperprofile[shp].queueMap;
        *rate = shaperprofile[shp].rate;
        *size = shaperprofile[shp].burstSize;
        }
    spin_unlock_bh(&ontmgr_spinlock);
	
    return retVal;
    }

EXPORT_SYMBOL(OntDirShaperGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get LLID according to the link index.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLLIDGet(LinkIndex link,PhyLlid *llid)
    {
    BOOL retVal = TRUE;
    
    if (!llid) 
        {
        return FALSE;
        }
    spin_lock_bh(&ontmgr_spinlock);
    *llid = PonMgrGetPhyLlid(link);
    spin_unlock_bh(&ontmgr_spinlock);
	
    return retVal;
    }

EXPORT_SYMBOL(OntDirLLIDGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirL1ByteLimitSet(U8 queue, U8 limit)
    {
    BOOL retVal;

    spin_lock_bh(&ontmgr_spinlock);
    retVal = PonMgrL1ByteLimitSet(queue, limit);
    spin_unlock_bh(&ontmgr_spinlock);
        
    return retVal;
    }

EXPORT_SYMBOL(OntDirL1ByteLimitSet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirL1ByteLimitGet(U8 queue, U8* limit)
    {
    BOOL retVal;
    spin_lock_bh(&ontmgr_spinlock);
    retVal = PonMgrL1ByteLimitGet(queue, limit);
    spin_unlock_bh(&ontmgr_spinlock);
    return retVal;
    }

EXPORT_SYMBOL(OntDirL1ByteLimitGet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Reset EPON MAC .
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntDirEponReset(void)
    {    
    EponTopInit();
    DelayMs(10);
    EponMacInit();
    }

EXPORT_SYMBOL(OntDirEponReset);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get link flow id.
///
///
/// \return  flow id
/// 
////////////////////////////////////////////////////////////////////////////////
//extern
U16 OntUserGetLinkFlow(U32 link, U32 queue)
    {
    U16 flow = PonMgrLinkFlowGet(link, queue);

    return flow;
    }

EXPORT_SYMBOL(OntUserGetLinkFlow);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get link loopback state.
///
///
/// \return
/// T/F
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEponLinkLoopbackGet(LinkIndex link, U8 *loopback)
    {    
    bdmf_object_handle port_obj = NULL;
    rdpa_port_loopback_t loopback_cfg;
    BOOL ret = TRUE;
    U16 flow_id = OntUserGetLinkFlow(link, 0);
    
    if (rdpa_port_get(rdpa_if_wan0, &port_obj) != 0)
        return FALSE;

    memset(&loopback_cfg, 0, sizeof(loopback_cfg));	
    
    if (rdpa_port_loopback_get(port_obj, &loopback_cfg) != 0)
        {
        ret = FALSE;
        }    
    else if (loopback_cfg.type != rdpa_loopback_type_fw)
        {
        *loopback = 0;
        }
    else if (loopback_cfg.wan_flow != flow_id)
        {
        *loopback = 0;
        }
    else
        {
        *loopback = 1;
        }

    if (port_obj)
        bdmf_put(port_obj);    

    return ret;
    }

EXPORT_SYMBOL(OntDirEponLinkLoopbackGet);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set link loopback state.
///
///
/// \return
/// T/F
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEponLinkLoopbackSet(LinkIndex link, U8 loopback)
    {    
    bdmf_object_handle port_obj = NULL;
    rdpa_port_loopback_t loopback_cfg;
    BOOL ret = TRUE;
    
    if (rdpa_port_get(rdpa_if_wan0, &port_obj) != 0)
        return FALSE;

    memset(&loopback_cfg, 0, sizeof(loopback_cfg));	

    if (loopback != 0)
        loopback_cfg.type = rdpa_loopback_type_fw; 
    
    loopback_cfg.op = rdpa_loopback_op_remote;
    loopback_cfg.wan_flow = OntUserGetLinkFlow(link, 0);
    loopback_cfg.queue = 0;
    loopback_cfg.ic_idx = 0;   

    ret = (rdpa_port_loopback_set(port_obj, &loopback_cfg) == 0);

    if (port_obj)
        bdmf_put(port_obj);    
    
    return ret;
    }

EXPORT_SYMBOL(OntDirEponLinkLoopbackSet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption mode
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEncryptModeSet(LinkIndex link, EncryptMode mode, EncryptOptions opts)
    {
    return EncryptLinkSet(link, mode, opts);
    }

EXPORT_SYMBOL(OntDirEncryptModeSet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption key
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEncryptKeySet(LinkIndex link, Direction dir, U8 keyIndex, U8 keylen, U32 *key)
    {
    return EncryptKeySet(link, dir, keyIndex, keylen, key, NULL);
    }

EXPORT_SYMBOL(OntDirEncryptKeySet);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption key
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirLinkEncrypted(LinkIndex link, Direction dir)
    {
    return EncryptGetLinkEncrypted(link, dir);
    }

EXPORT_SYMBOL(OntDirLinkEncrypted);


void OntDirInit(void)
    {
    int rc = 0;
    epon_mac_q_cfg_t cfg;
    bdmf_object_handle epon_obj = NULL;
	u8 i = 0;
    
    memset(&cfg, 0, sizeof(cfg));
    memset(&queue_cfg_rec, 0, sizeof(queue_cfg_rec));
    memset(LinkShaperIdx,PonShaperBad,sizeof(LinkShaperIdx));

    if ((rc = rdpa_epon_get(&epon_obj)))
        {
        BCM_LOG_ERROR(BCM_LOG_ID_EPON, "rdpa_epon_get failed, rc(%d) \n", rc);
        }
    else
        {
        rdpa_epon_mode epon_mode;
        if ((rc = rdpa_epon_mode_get(epon_obj, &epon_mode)))
            BCM_LOG_ERROR(BCM_LOG_ID_EPON, "rdpa_epon_mode_get failed, rc(%d) \n", rc);
        else
            oam_mode = epon_mode;
		
        bdmf_put(epon_obj);
        }

    PonMgrEponMacQueueConfigInit(&cfg);
    
    ont_dir_queue_set(&cfg);  

    for (i = 0; i < SLlidTotalQNum; i++)
        {
        OntDirL1ByteLimitSet(i, DefaultL1ByteLimit);
        }
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief Get default link state
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirDefLinkState(U32 link)
    {   
    BOOL ret = TRUE;
    switch (oam_mode)
        {
        case rdpa_epon_dpoe:
#ifndef CONFIG_BCM96858
            ret = FALSE;
#endif
            break;
        case rdpa_epon_ctc:
            /* fall through */
        case rdpa_epon_bcm:
            /* fall through */
        default:
            break;
        }
    
    return ret;
    }

EXPORT_SYMBOL(OntDirDefLinkState);

// OnuDirector.c
