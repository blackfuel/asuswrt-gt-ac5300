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
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <boardparms.h>

#include "OntmMpcp.h"
#include "Ethernet.h"
#include "EponFrame.h"
#include "OsAstMsgQ.h"
#include "PonManager.h"
#include "OntDirector.h"
#include "rdpa_api.h"
#include "autogen/rdpa_ag_epon.h"
#include "rdpa_epon.h"
#include "pmd.h"


#define TkOnuPendingGrantsPerLlid 16
#define RegReqTime                ((84 * 8) / 16)    // Time for a REG_RQ frame
#define AllLinksDownTimeOut       240 // 240 sec
#define LinkThrashingTimeOut      240 // 240 sec of credit
#define BackoffTimeOut             60 //  60 sec
#define WaitForGateTimeOut          5 //   5 sec

#define ThrashingDeregStep         10 //  10 sec credit penalty when dereg.

#define MpcpDebug(lvl, args)      eponUsrDebug(lvl, args)
#define MpcpDebugOff              DebugDisable
#define MpcpDebugHi               DebugMpcp
#define MpcpDebugStd              DebugMpcp
#define MpcpDebugLo               DebugMpcp
#define MpcpDebugDefault          DebugMpcp


static U16 grantLosChkTime = 0;
U8 mpcpGateNum = 0;

static DEFINE_SPINLOCK(mpcp_spinlock);

static OntmMpcpCfg mpcpCfg;

static EponTimerModuleId mpcpTimerLink[TkOnuMaxBiDirLlids];
static U16  mpcpTimeOut10ms[TkOnuMaxBiDirLlids];    // unit 10ms
static MacAddr  MpcpMcMacAddr;
static MacAddr  OamMcMacAddr ;

static Port sendPort;

static MacAddr PonMacCheck[TkOnuNumTxLlids];

extern U64 BULK statsLink[TkOnuMaxBiDirLlids][EponLinkHwStatCount];
extern U64 BULK statsPon[EponLinkHwStatCount];

typedef enum
    {
    MpcpEventNull,          // Non event
    MpcpEventDiscGateRx     // A discovery gate is received
    } MpcpEvent;

#define MpcpFrmMaxSize              64

// Control variable determines if forwarding a copy of MPCP frame
// to external CPU, or not
static BOOL mpcpForward = FALSE;

typedef enum
    {
    RecMpcpFailSafe,
    RecordsCount
    } RecordIds;

#define DefaultMcastLinkRulePri 15

#if PMC_NONSTANDARD_REG_REQUEST
// Yet more IOP fun.  PMC tossed in some extra fields at the end of the register
// request message with the laser on/off times (old, old standard draft) and the
// chip ID and version (complete non standard).  This will interfere with the
// 10G standard so this is only a temporary fix.  It will be removed ASAP.
typedef struct
    {
    U16		laserOnTime;
    U16		laserOffTime;
    U32		chipVersion;
    } PACK MpcpPmcRegisterReqMsg;

#define PmcChipVersion	0x60014102

#endif

/* only start the pmd first burst algorithm once when moving to wait for registration */
uint8_t pmd_fb_counter;
extern int pmd_dev_change_tracking_state(uint32_t old_state, uint32_t new_state);

////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpChangePmdTrackingState: if working with PMD change tracking state
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void OntmMpcpChangePmdTrackingState(rdpa_epon_link_mpcp_state state)
    {
    uint16_t OpticsType;

    BpGetGponOpticsType(&OpticsType);

    if (OpticsType == BP_GPON_OPTICS_TYPE_PMD)
        pmd_dev_change_tracking_state(state, state);
    }

////////////////////////////////////////////////////////////////////////////////
/// GetBroadcastLlid:  Returns "default" or discovery gate LLID
///
 // Parameters:
/// None
///
/// \return
/// LLID value
////////////////////////////////////////////////////////////////////////////////
static
PhyLlid GetBroadcastLlid(void)
    {
    if(PonCfgDbGetDnRate() != LaserRate10G)
        {
        return MpcpBroadcastLlid1G;
        }
    return MpcpBroadcastLlid10G;
    } // GetBroadcastLlid


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpTimeOutTimer:  Background checking on links
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void OntmMpcpTimeOutTimer (EponTimerModuleId moduleId)
    {
    U8 link = (moduleId - mpcpTimerLink[0]);
    // Only take effect when the link mpcp timeout is configured
    if (mpcpTimeOut10ms[link] != 0)
        {
        OsAstMsgQSet (OsAstAlmLinkRegTimeOut, link, 0);
        EponUsrModuleTimerCfg(EPON_TIMER_TO1, mpcpTimerLink[link],0);
        }
    } // OntmMpcpTimeOutTimer

#if REGISTER_NACK_BACKOFF_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// SilenceSomeLinks : some of the links needs to be silenced
///
// Parameters:
/// \param links - bitmaps of the links to be silenced.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void SilenceSomeLinks (U32 links)
    {
    // If no links are currently silenced
    if (mpcpCfg.silencedLinks == 0)
        {
        mpcpCfg.silencedLinks = links;
        // Restart the watchdog
        mpcpCfg.silenceWatchDog = mpcpCfg.silencetime;
        }
    else
        {
        // Some links are alredy being silenced. We need to wait for them to be
        // feed (unsilenced if you will) and then we will start a new w.d.
        // Note that they are silenced starting now. That means they will be
        // silenced longuer.
        mpcpCfg.pendingSilencedLinks |= links;

        //Don't over silence some guys
        mpcpCfg.pendingSilencedLinks &= ~mpcpCfg.silencedLinks;
        }
    } // SilenceSomeLinks


void SilenceTimeSet(U8 time)
    {
    mpcpCfg.silencetime = time;
    }

void SilenceDeallocFlagSet(BOOL flag)
    {
    mpcpCfg.deallocatesilence = flag;
    }

#endif // REGISTER_NACK_BACKOFF_SUPPORT


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpIgnoreFailsafeSet - Enable/disable MPCP registration failsafe ignore
///
/// This function enables or disables the ignore failsafe option which will keep
/// the ONU from rebooting if it can not register.  The no register cycle count
/// will also be reset any time the ignore failsafe state is modified.
///
 // Parameters:
/// \param enable Enable/Disable ignore failsafe
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpSetFailsafeEnabled (BOOL enable)
    {
    mpcpCfg.failsafeEnabled = enable;
    mpcpCfg.allLinksWatchDog = AllLinksDownTimeOut;    
    } // OntmMpcpIgnoreFailsafeSet


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpIgnoreFailsafeGet - Get MPCP registration failsafe ignore safe
///
/// This function returns the state of the ignore failsafe option which will
/// prevent the ONU from rebooting if it can not register.
///
 // Parameters:
///
/// \return
/// TRUE if ignore failsafe is enabled
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntmMpcpIsFailsafeEnabled (void)
    {
    return mpcpCfg.failsafeEnabled;
    } // OntmMpcpIgnoreFailsafeGet


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpMpcpForwardSet: Set the mpcp forward control variable.
///
/// \param fwd   the value to set.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpMpcpForwardSet(BOOL fwd)
    {
    mpcpForward = fwd;
    } // OntmMpcpMpcpForwardSet


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpMpcpForwardGet: Get the mpcp forward control variable.
///
/// \param none
///
/// \return
/// TRUE if need to forwarding mpcp to external CPU, otherwise FALSE
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntmMpcpMpcpForwardGet(void)
    {
    return mpcpForward;
    } // OntmMpcpMpcpForwardGet


////////////////////////////////////////////////////////////////////////////////
/// MpcpFatal:  MPCP has suffered a fatal error.  Resets the ONU.
///
 // Parameters:
///
/// \return
/// None.
////////////////////////////////////////////////////////////////////////////////
static
void MpcpFatal (void)
    {
    if (OntmMpcpIsFailsafeEnabled ())
        {
        //LogCriticalRecordSave (MpcpFatalError, 0, 0);
        printk("failsafe reboot system...\n");
        OsAstMsgQSet(OsAstAlmLinkFailSafeReset, 0, 1);
        }
    } // MpcpFatal


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpLinksRegistered:  counts number of links currently registered
///
 // Parameters:
/// None
///
/// \return
/// Number of links registered
////////////////////////////////////////////////////////////////////////////////
//extern
LinkIndex OntmMpcpLinksRegistered (void)
    {
    return mpcpCfg.numRegistered;
    } // LinksRegistered
    
static
BOOL OntPutAssociateMcastInService (LinkIndex link, PhyLlid assignedPort, LinkIndex *mcastLink)
    {
    LinkIndex  uniDirLink;
  
    MpcpDebug (MpcpDebugStd,("MPCP: McastLink %d assigned Port:%0x \n", link,assignedPort));
    // This link was not already assigned to a Port.
    if (mpcpCfg.biDirLinks[link].associatedUniDirLink == TkOnuMaxRxOnlyLlids)
        {
        // try to find if Port is already assigned
        for (uniDirLink = 0; uniDirLink < TkOnuNumRxOnlyLlids; uniDirLink++)
            {
            if (PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid + uniDirLink) == assignedPort)
                {
                break;
                }
            }

        // if not already assigned, find a free spot.
        if (uniDirLink == TkOnuNumRxOnlyLlids)
            {
            for (uniDirLink = 0; uniDirLink < TkOnuNumRxOnlyLlids; uniDirLink++)
                {
                if (mpcpCfg.uniDirLinks[uniDirLink].useCount == 0)
                    {
                    break;
                    }
                }
#ifdef TkOnuFullFeatureLlids
            if((uniDirLink == TkOnuNumRxOnlyLlids) && (TkOnuNumBiDirLlids > mpcpCfg.numToRegister))
                {
                if(mpcpCfg.biDirLinks[TkOnuFirstRxOnlyLlid-1].state == rdpa_epon_link_unregistered)
                    {
                    TkOnuFirstRxOnlyLlid --;
                    }
                }
#endif             
            }
        else
            {
            //broadcast LLID should be shared for all links.
            //non-broadcast LLID can be shared too
            if  ((uniDirLink <  TkOnuNumRxOnlyLlids) && 
                //shared LLID is installed by other link.
                (mpcpCfg.uniDirLinks[uniDirLink].useCount != 0))
                {
                //just only update the local data.
                mpcpCfg.biDirLinks[link].associatedUniDirLink = uniDirLink;
                    mpcpCfg.uniDirLinks[uniDirLink].useCount++;
                *mcastLink = uniDirLink;
                return TRUE;
                }
            }

        // If no free spot, return right away.
        if ((uniDirLink >= TkOnuNumRxOnlyLlids) ||
            (mpcpCfg.uniDirLinks[uniDirLink].useCount != 0))
            {
            return FALSE;
            }

        // If we are still here, that means everything is in order.
        mpcpCfg.biDirLinks[link].associatedUniDirLink = uniDirLink;
        mpcpCfg.uniDirLinks[uniDirLink].useCount++;

        // Update rules TODO
        // Update hardware. Don't check use count to save code.
        PonMgrCreateLink (TkOnuFirstRxOnlyLlid+uniDirLink, assignedPort);
        *mcastLink = uniDirLink;
        return TRUE;
        }
    else
        {
        uniDirLink = mpcpCfg.biDirLinks[link].associatedUniDirLink;	
        //found the same assigned.
        if (PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid + uniDirLink) == assignedPort)
            {
            *mcastLink = uniDirLink;
            return TRUE;
            }
        return FALSE;
        }
    }

static
BOOL OntPutStandaloneMcastInService (PhyLlid assignedPort, LinkIndex *mcastLink)
    {
    LinkIndex  uniDirLink;
    
    // try to find if Port is already assigned
    for (uniDirLink = 0; uniDirLink < TkOnuNumRxOnlyLlids; uniDirLink++)
        {
        if (PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid + uniDirLink) == assignedPort)
            {
            *mcastLink = uniDirLink;
            return FALSE;
            }
        }
    
    for (uniDirLink = 0; uniDirLink < TkOnuNumRxOnlyLlids; uniDirLink++)
        {
        if (mpcpCfg.uniDirLinks[uniDirLink].useCount == 0)
            {
            mpcpCfg.uniDirLinks[uniDirLink].useCount = 1;
            PonMgrCreateLink (TkOnuFirstRxOnlyLlid+uniDirLink, assignedPort);
            *mcastLink = uniDirLink;
            return TRUE;
            }
        }
    if((uniDirLink == TkOnuNumRxOnlyLlids) && (TkOnuNumBiDirLlids > mpcpCfg.numToRegister))
        {
#ifdef TkOnuFullFeatureLlids
        if(mpcpCfg.biDirLinks[TkOnuFirstRxOnlyLlid-1].state == rdpa_epon_link_unregistered)
            {
            TkOnuFirstRxOnlyLlid --;
            mpcpCfg.uniDirLinks[TkOnuFirstRxOnlyLlid].useCount = 1;
            *mcastLink = TkOnuFirstRxOnlyLlid;
            return TRUE;
            }
#endif    
        }
    return FALSE;
    }
    
////////////////////////////////////////////////////////////////////////////////
/// OntPutMcastInService : Assign a unidirectional LLID to a bidirectional link
///
 // Parameters:
/// \param link         : link index of the bidirectional link
/// \param assignedPort : unidirectional LLID
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
LinkIndex OntPutMcastInService (LinkIndex link, PhyLlid assignedPort)
    {
	BOOL rc;
    LinkIndex mcastLink = TkOnuMaxRxOnlyLlids;
    BDMF_MATTR(gem_attrs, rdpa_gem_drv());
    rdpa_gem_flow_ds_cfg_t ds_cfg = {};
    bdmf_object_handle gem;
    
    if(link < TkOnuMaxBiDirLlids)
        {
        rc = OntPutAssociateMcastInService(link, assignedPort, &mcastLink);
        }
    else
        {
        rc = OntPutStandaloneMcastInService(assignedPort, &mcastLink);
        }

    if(rc)
        {
        if(mpcpCfg.uniDirLinks[mcastLink].useCount == 1)
            {
            rc = rdpa_gem_index_set(gem_attrs, TkOnuFirstRxOnlyLlid+mcastLink);
            ds_cfg.discard_prty = rdpa_discard_prty_low;
            ds_cfg.destination = rdpa_flow_dest_eth;
            rc = rc ? rc : rdpa_gem_ds_cfg_set(gem_attrs, &ds_cfg);        
            rc = bdmf_new_and_set(rdpa_gem_drv(), NULL, gem_attrs, &gem);
            if (rc < 0)
                {
                printk( "Failed to create downstream runner port\n");
                }
            }
       }
        return mcastLink;
    } // OntPutMcastInService

////////////////////////////////////////////////////////////////////////////////
/// OntPutMcastOutOfService : Deassign a unidirectional LLID from a link
///
 // Parameters:
/// \param link         : link index of the bidirectional link
/// \param assignedPort : unidirectional LLID
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
BOOL OntPutMcastOutOfService (LinkIndex link, PhyLlid assignedPort)
    {
    LinkIndex  uniDirLink;
    PhyLlid existPort;
    BOOL toRemoveResource = FALSE;
    int rc;
    bdmf_object_handle gem;
    
    MpcpDebug(MpcpDebugStd, ("MPCP: McastLink %d not assigned Port:%0x \n", link,assignedPort));
    
    if(link < TkOnuMaxBiDirLlids)
        {
        uniDirLink = mpcpCfg.biDirLinks[link].associatedUniDirLink;
        if (uniDirLink != TkOnuMaxRxOnlyLlids)
            {
            BOOL toRemove = FALSE;
            
            existPort = PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid + uniDirLink);
            if(existPort == assignedPort)
                {
                toRemove = TRUE;
                }
            //Sometimes OLT will use 0xffff or 0xfffe as the broadcast llid,
            //so we need to do extra check here.
            if (((existPort & MpcpBroadcastLlid) == GetBroadcastLlid()) && 
                (GetBroadcastLlid() == (assignedPort & MpcpBroadcastLlid)))
                {
                toRemove = TRUE;
                }
              
            if (toRemove)
                {
                mpcpCfg.biDirLinks[link].associatedUniDirLink = TkOnuMaxRxOnlyLlids;
                mpcpCfg.uniDirLinks[uniDirLink].useCount--;
                
                // Update hardware if use count reaches 0
                if (mpcpCfg.uniDirLinks[uniDirLink].useCount == 0)
                    {
                    toRemoveResource = TRUE;
                    }
                }
            }
        }
    else
        {
        for (uniDirLink = 0; uniDirLink < TkOnuNumRxOnlyLlids; uniDirLink++)
            {
            existPort = PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid+uniDirLink);
            if(existPort == assignedPort)
                {
                mpcpCfg.uniDirLinks[uniDirLink].useCount = 0;
                toRemoveResource = TRUE;
                }
            }
        }
    
    if(toRemoveResource)
        {
        rc = rdpa_gem_get(TkOnuFirstRxOnlyLlid + uniDirLink, &gem);
        if(!rc)
            {
            bdmf_put(gem);
            bdmf_destroy(gem);
            }
        PonMgrDestroyLink(TkOnuFirstRxOnlyLlid+uniDirLink);
        // Update rules TODO
        //UpdateMcastLinkRule(TkOnuFirstRxOnlyLlid+uniDirLink,FALSE);
#ifdef TkOnuFullFeatureLlids
        if(uniDirLink == 0 && TkOnuNumRxOnlyLlids!= TkOnuRsvNumRxOnlyLlids)
            {
            TkOnuFirstRxOnlyLlid++; 
            }
#endif
        }
    
        return TRUE;
    } // OntPutMcastOutOfService

#if CONFIG_BCM_ENET_IMPL == 7
extern void enet_pon_drv_link_change(int up);
#endif
////////////////////////////////////////////////////////////////////////////////
/// PutLinkInService:  sets up LLID for in-service operation
///
/// Includes link configuration (priority queues, sizes, user port, etc.)
///
 // Parameters:
/// \param link     Link to move to WaitingForGats state
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void PutLinkInService (LinkIndex link)
    {
    bdmf_object_handle epon_obj;
    rdpa_epon_mcast_link_t mcast_link;
    int rc;
    
    // Update number of links registered
    if ((rdpa_epon_link_in_service != mpcpCfg.biDirLinks[link].state) &&  // Not in service
        (mpcpCfg.numRegistered < mpcpCfg.numToRegister))   // Safety mesure
        {
        mpcpCfg.numRegistered++;
        }

    mpcpCfg.biDirLinks[link].state = rdpa_epon_link_in_service;

    //Enable user traffic at the queue level

    MpcpDebug (MpcpDebugStd, ("MPCP: Link %d registered\n", link));
    // Alarms
    OsAstMsgQClr (OsAstAlmLinkAllDown, 0, 0);
    if(mpcpCfg.numRegistered == 1)
        {
        OntmMpcpChangePmdTrackingState(rdpa_epon_link_in_service);
        pmd_fb_counter = 0;

        OsAstMsgQSet (OsAstAlmOntSingleLinkReg, 0, 0);
        OsAstMsgQClr (OsAstAlmOntMultiLinkReg, 0, 0);
#if CONFIG_BCM_ENET_IMPL == 7
        enet_pon_drv_link_change(1);
#endif
        }
    else
        {
        OsAstMsgQSet (OsAstAlmOntMultiLinkReg, 0, 0);
        OsAstMsgQClr (OsAstAlmOntSingleLinkReg, 0, 0);
        }
    OsAstMsgQClr (OsAstAlmLinkUnregistered, link, 0);
   
    // By default assign broadcast LLID to this link
    mcast_link.llid = (PonCfgDbGetDnRate()== LaserRate10G) ?
        MpcpBroadcastLlid10G : MpcpBroadcastLlid1G;
    mcast_link.enable = TRUE;

    rc = rdpa_epon_get(&epon_obj);
    if (rc)
        {
        MpcpDebug(MpcpDebugHi, ("%s: link 0x%x fail to get rdpa_epon_obj\n", __FUNCTION__, link));
        return;
        }
    rc = rdpa_epon_mcast_link_set(epon_obj, link, &mcast_link);
    if (rc) 
        {
        MpcpDebug(MpcpDebugHi, ("%s: link 0x%x fail to set mcast link\n", __FUNCTION__, link));
        }
    bdmf_put(epon_obj);

    // start OAM discovery
    //OamLinkReset(link);
    
    // Disable upstream by default in DPoE mode
    if(!OntDirDefLinkState(link))
        OntDirLinkShaperSet(link, 0, 2048);
    } // PutLinkInService


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpRegFailAlarm - Ontm MPCP register fail alarm process
///
/// Parameters:
/// \param link     link to be put OoS
///
/// \return
/// None
///
////////////////////////////////////////////////////////////////////////////////
static
void OntmMpcpRegFailAlarm (LinkIndex link)
    {
    OsAstMsgQClr (OsAstAlmLinkRegStart, link, 0);
    OsAstMsgQClr (OsAstAlmLinkRegSucc, link, 0);
    OsAstMsgQSet (OsAstAlmLinkRegFail, link, 0);
    }//OntmMpcpRegFailAlarm


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpRegSuccAlarm - Ontm MPCP register success alarm process
///
/// Parameters:
/// \param link     link to be put OoS
///
/// \return
/// None
///
////////////////////////////////////////////////////////////////////////////////
static
void OntmMpcpRegSuccAlarm (LinkIndex link)
    {
    OsAstMsgQClr (OsAstAlmLinkRegStart, link, 0);
    OsAstMsgQClr (OsAstAlmLinkRegFail, link, 0);
    OsAstMsgQSet (OsAstAlmLinkRegSucc, link, 0);
    OsAstMsgQClr (OsAstAlmLinkRegTimeOut, link, 0);
    }//OntmMpcpRegSuccAlarm


////////////////////////////////////////////////////////////////////////////////
/// PutLinkOutOfService - Free resources and out link OoS
///
/// Parameters:
/// \param link     link to be put OoS
///
/// \return
/// None
///
////////////////////////////////////////////////////////////////////////////////
static
void PutLinkOutOfService (LinkIndex link)
    {
    MpcpDebug (MpcpDebugStd, ("MPCP: Link %d deregistered\n", link));
    // Update number of links registered
    if ((rdpa_epon_link_in_service == mpcpCfg.biDirLinks[link].state) && // Need to be IS
        (mpcpCfg.numRegistered != 0))                 // Avoid roll over
        {
        mpcpCfg.numRegistered--;
        }

    // Take care of any associated unidirectional link
    if(mpcpCfg.biDirLinks[link].associatedUniDirLink != TkOnuMaxRxOnlyLlids)
        {
        (void)OntPutMcastOutOfService (link,
            PonMgrGetPhyLlid(mpcpCfg.biDirLinks[link].associatedUniDirLink +
                             TkOnuFirstRxOnlyLlid));
        }

    if (mpcpCfg.biDirLinks[link].state == rdpa_epon_link_in_service)
        {
        // Protect against thrashing registration.
        mpcpCfg.numDeregisteredSinceAudit++;
        }
    
    // Reset the link state to unregistered.
    mpcpCfg.biDirLinks[link].state = rdpa_epon_link_unregistered;

    PonMgrDestroyLink(link);

    // Alarms
    if (mpcpCfg.numRegistered == 0)
        {
        OntmMpcpChangePmdTrackingState(rdpa_epon_link_unregistered);
        pmd_fb_counter = 0;
        // By default listen to broadcast link
        PonMgrCreateLink (TkOnuFirstRxOnlyLlid, GetBroadcastLlid());
        OsAstMsgQSet (OsAstAlmLinkAllDown, 0, 0);
        OsAstMsgQClr (OsAstAlmOntSingleLinkReg, 0, 0);
        OsAstMsgQClr (OsAstAlmOntMultiLinkReg, 0, 0);
#if CONFIG_BCM_ENET_IMPL == 7
        enet_pon_drv_link_change(0);
#endif
        }
    else if(mpcpCfg.numRegistered == 1)
        {
        OsAstMsgQSet (OsAstAlmOntSingleLinkReg, 0, 0);
        OsAstMsgQClr (OsAstAlmOntMultiLinkReg, 0, 0);
        }
    else
        {
        OsAstMsgQSet (OsAstAlmOntMultiLinkReg, 0, 0);
        OsAstMsgQClr (OsAstAlmOntSingleLinkReg, 0, 0);
        }
    OsAstMsgQSet (OsAstAlmLinkUnregistered, link, 0);
    OntmMpcpRegFailAlarm (link);

    // JCM - Temporary call to OAM to re-init OAM link. In final version, OAM
    // should be polling for status change.
    //OamLinkInit (link);
    } // PutLinkOutOfService


///////////////////////////////////////////////////////////////////////////////
/// IsLinkInService:  Checks if a link is in service
///
 // Parameters:
/// \param link     Link being queried
///
/// \return
/// TRUE if link is in service, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
//extern
BOOL IsLinkInService (LinkIndex link)
    {
    return (mpcpCfg.biDirLinks[link].state == rdpa_epon_link_in_service);
    } // IsLinkInService


///////////////////////////////////////////////////////////////////////////////
/// IsLinkInService:  Checks if a link is in service
///
 // Parameters:
/// \param link     Link being queried
///
/// \return
/// TRUE if link is in service, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
rdpa_epon_link_mpcp_state OntmMpcpLinkStateGet (LinkIndex link)
    {
    return mpcpCfg.biDirLinks[link].state;
    }

	
///////////////////////////////////////////////////////////////////////////////
//extern
LinkMap MpcpActiveLinkMap()
    {
    LinkMap active = 0;
    LinkIndex link;

    for (link = 0; link < TkOnuNumBiDirLlids; link++)
        {
        if (mpcpCfg.biDirLinks[link].state != rdpa_epon_link_unregistered)
            {
            active |= 1UL << link;
            }
        }

    return active;
    }


////////////////////////////////////////////////////////////////////////////////
/// SendToLink:  Send size bytes to given link
///
/// Assumes pktBuffer has already been formatted with packet to send.  This
/// routine will assure minimum MPCP/Ethernet size (64 bytes) and pad with
/// zeros.
///
 // Parameters:
/// \param link     Link on which to transmit packet
/// \param size     Number of bytes in packet
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void SendToLink (LinkIndex link, U16 size)
    {
    // add Ethernet header
    memcpy (&TxFrame->da, &MpcpMcMacAddr, sizeof(MacAddr));
    PonMgrGetMacForLink(link, &TxFrame->sa);

    TxFrame->type = EPON_NTOHS(EthertypeMpcp);

    size += sizeof(EthernetFrame);

    if (size < 60) // If packet is too small...
        {// ...finish out packet with zeros
        memset (txPktBuffer + size, 0, 60 - size);
        size = 60;
        }
	
    DumpData(MpcpDebugStd,"SendToLink",txPktBuffer,size);
    sendPort.pift = PortIfPon;
    sendPort.inst = link;
    } // SendToLink


////////////////////////////////////////////////////////////////////////////////
/// \brief  Send MPCP register request on broadcast link
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void MpcpRegReqEnqueue(LinkIndex link)
    {
#if PMC_NONSTANDARD_REG_REQUEST
    MpcpRegisterReq1GMsg *  regReq =
        (MpcpRegisterReq1GMsg *)(TxFrame + 1);
    MpcpPmcRegisterReqMsg BULK* FAST pmcRegReq =
        (MpcpPmcRegisterReqMsg BULK*)(regReq + 1);

#else
    MpcpRegisterReq10GMsg *  regReq =
        (MpcpRegisterReq10GMsg *)(TxFrame + 1);
#endif
    MpcpInterval16 regReqPacketSizeTq = 0;
    MpcpInterval16 regReqBurstSize;
    U16 availableSlots;  //We want to divide up the full discovery window
                          //into various slots such that the odds of picking 
                          //a slot no one else does is increased.  Previously
                          //we would pick a random offset into the window and
                          //this showed a high number of collisions on large
                          //(128 Xenu(s) each with 4 links) delay in 
                          //registration time.
    U16 chosenSlot;
    U16 slotSpacing = 0;


#if REGISTER_NACK_BACKOFF_SUPPORT
    // Is this link being silenced?
    if (TestBitsSet(mpcpCfg.silencedLinks | mpcpCfg.pendingSilencedLinks,
    		1UL << link))
        {
        return;
        }
#endif

    if (PonCfgDbGetUpRate()==LaserRate10G)
        {
        //This assumes 10G uses FEC and should work for non-FEC cases as 
        //well.  This could possible be improved for non-FEC if so use 5TQ.
        regReqPacketSizeTq = 13;
        }
    else // assume 1G upstream, assume no FEC to start on 1G links
        {
        regReqPacketSizeTq = 42;
        }
    regReqBurstSize = regReqPacketSizeTq + 
                                 32                             +   //Default Laser On
                                 32                             +   //Default Laser Off
                                 mpcpCfg.syncTime               +
                                 2;                                 //10G End of burst
    availableSlots = mpcpCfg.windowRoom / (regReqBurstSize + slotSpacing);
    chosenSlot = PbiRand16() % availableSlots;
    EponSetGateOffset (chosenSlot*regReqBurstSize);

    regReq->opcode       = EPON_NTOHS(MpcpRegisterReq);
    regReq->flags        = MpcpRegReqFlagRegister;
    regReq->pendGrants   = TkOnuPendingGrantsPerLlid;

#if PMC_NONSTANDARD_REG_REQUEST
    pmcRegReq->laserOnTime = PonCfgDbGetLaserOnTime ();
    pmcRegReq->laserOffTime = PonCfgDbGetLaserOffTime ();
    pmcRegReq->chipVersion = PmcChipVersion;
    SendToLink (link, sizeof(*regReq) + sizeof(*pmcRegReq));
#else
    regReq->discoveryInfo = EPON_NTOHS((MpcpDiscoveryInfo) mpcpCfg.onuCapability);
    regReq->laserOn       = PonCfgDbGetLaserOnTime();
    regReq->laserOff      = PonCfgDbGetLaserOffTime(); 
    MpcpDebug (MpcpDebugStd, ("MPCPreq: lon 0x%0x loff 0x%0x \n",
                             regReq->laserOn, regReq->laserOff));
    SendToLink (link, sizeof(*regReq));
#endif

    statsLink[link][EponLinkMPCPRegReq]++;
    statsPon[EponLinkMPCPRegReq]++;
    } // PreloadRegReq


////////////////////////////////////////////////////////////////////////////////
/// SendRegAck:  Send MPCP register ack msg to given link
///
 // Parameters:
/// \param link     Link on which to send ack
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void SendRegAck (LinkIndex link, U8 flags)
    {
    MpcpRegisterAckMsg *  regAck =
    (MpcpRegisterAckMsg *)(TxFrame + 1);
    Port port;

    regAck->opcode                  = EPON_NTOHS(MpcpRegisterAck);
    regAck->flags                   = flags;
    regAck->assignedPort            = EPON_NTOHS(PonMgrGetPhyLlid(link));
    regAck->nonDiscoverySyncTime    = EPON_NTOHS(mpcpCfg.syncTime);
    MpcpDebug (MpcpDebugStd, ("MPCPreq: Send Ack link:%d\n",link));
    SendToLink (link, sizeof(*regAck));

    port.pift = PortIfProc;
    port.inst = link;
    statsLink[link][EponLinkMPCPRegAck]++;
    statsPon[EponLinkMPCPRegAck]++;
    } // SendRegAck


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpNotifyLinkNotReg: Notify OAM stack Over NETLINK that the MPCP stack
/// wants to start new registration for this logical link index. Happens on LOS,
/// bootup, deregistration by OLT and "no grant at all".
///
///
 // Parameters:
/// \param link     Link being operated
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void OntmMpcpNotifyLinkNotReg(LinkIndex link)
    {
    OsAstMsgQClr (OsAstAlmLinkRegSucc, link, 0);
    } 


////////////////////////////////////////////////////////////////////////////////
/// StartRegisteringNewLink:  Pick a new link and try register it
///
/// WARNING. This function should not be called when all links are registered.
///
 // Parameters:
/// \param link     Link being operated
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void StartRegisteringNewLink(LinkIndex link)
    {
    // Start from clean slate.
    PutLinkOutOfService (link);
    
    PonMgrCreateLink(link, GetBroadcastLlid());
    
    // Now we can allow the gates. It will first be discovery gates
    EponAllowGates (1UL << link);
    OsAstMsgQClr (OsAstAlmLinkRegFail, link, 0);
    OsAstMsgQClr (OsAstAlmLinkRegSucc, link, 0);
    OsAstMsgQSet (OsAstAlmLinkRegStart, link, 0);
    
    mpcpCfg.biDirLinks[link].state = rdpa_epon_link_registering;
    
    statsLink[link][EponLinkMPCPRegReq] = 0;
    statsLink[link][EponLinkMPCPRegAck] = 0;
    EponUsrModuleTimerCfg(EPON_TIMER_TO1, mpcpTimerLink[link],mpcpTimeOut10ms[link]);
    } // StartRegisteringNewLink


////////////////////////////////////////////////////////////////////////////////
/// HandleMpcpRegister:  Handle register message on given link
///
 // Parameters:
/// \param link     Link which should be registered
/// \param reg      Register PDU of frame received
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void HandleMpcpRegister (const MpcpRegister10GMsg * reg)
    {
    LinkIndex link;
    Port port;

    port.pift = PortIfPon;
    port.inst = 0;

    switch (reg->flags)
        {
        case MpcpRegFlagReregister :
        case MpcpRegFlagDeallocate :	
        
        // This can be sent for any link, we need to look at the llid (or
        // assignedPort) in the message
        link = PonMgrPhyLlidToIndex (EPON_NTOHS(reg->assignedPort));
        
        MpcpDebug (MpcpDebugStd,
                   ("MPCP: Unreg: LLID %04x link %d\n",
                    EPON_NTOHS(reg->assignedPort), link));
        
        if (link < TkOnuNumBiDirLlids)
            {
#if REGISTER_NACK_BACKOFF_SUPPORT 
            if (mpcpCfg.deallocatesilence &&
                (reg->flags == MpcpRegFlagDeallocate))
                { // Why bother checking # of links?
                mpcpCfg.deallocatesilence = FALSE;
                SilenceSomeLinks(AllLinks);
                }
#endif
        OsAstMsgQClr (OsAstAlmLinkRegTimeOut, link, 0);
        OntmMpcpNotifyLinkNotReg(link);
            }
        else
            {
            MpcpDebug (MpcpDebugStd, ("MPCP: ignoring %s\n", (reg->flags==MpcpRegFlagReregister? "Rereg":"Dealloc")));
            }
        
        break;
        
        case MpcpRegFlagSuccess :
        // Here we are talking about the link that tries to register
        MpcpDebug (MpcpDebugStd,
                   ("MPCP: Register LLID %04x "
                    "Stime %u, Lon %d, Loff %d\n",
                    EPON_NTOHS(reg->assignedPort),
                    EPON_NTOHS(reg->nonDiscoverySyncTime),
                    reg->targetLaserOn,
                    reg->targetLaserOff));
        
        link = PonMgrGetLinkForMac(&RxFrame->da);
        if ((link < TkOnuNumBiDirLlids) &&
            ((mpcpCfg.biDirLinks[link].state == rdpa_epon_link_registering) ||
            (mpcpCfg.biDirLinks[link].state == rdpa_epon_link_awaiting_register)))
            {                
            
            // remember sync time
            mpcpCfg.syncTime = EPON_NTOHS(reg->nonDiscoverySyncTime);
        
            // Set Idle time based on sync time
            (void)PonMgrSetTargetIdleTime (mpcpCfg.syncTime,
                                           reg->targetLaserOn,
                                           reg->targetLaserOff);
        
            mpcpCfg.biDirLinks[link].state = rdpa_epon_link_awaiting_gate;
            mpcpCfg.biDirLinks[link].waitForGateWatchDog
                = WaitForGateTimeOut;
        
            // We don't want to receive discovery gate anymore.
            EponDisallowGates (1UL << link);
        
            // Set the LLID in the hardware lookup table and enable the table entry.
            PonMgrCreateLink (link, EPON_NTOHS(reg->assignedPort));
        	
            SendRegAck (link, MpcpRegAckFlagsSuccess);
            OntmMpcpRegSuccAlarm (link);
            }
        else
            {
            MpcpDebug (MpcpDebugStd, ("MPCP: ignoring Reg\n"));
            }
        break;
        
        case MpcpRegFlagNack :
        // Again only apply to link trying to register
        MpcpDebug (MpcpDebugStd,
                   ("MPCP: Reg Nack: LLID %04x\n",
                    EPON_NTOHS(reg->assignedPort)));
        
        link = PonMgrGetLinkForMac(&RxFrame->da);
        if (link < TkOnuNumBiDirLlids)
            {
            // remember sync time
            mpcpCfg.syncTime = EPON_NTOHS(reg->nonDiscoverySyncTime);
        
            // Set Idle time based on sync time
            (void)PonMgrSetTargetIdleTime (mpcpCfg.syncTime,
                                           reg->targetLaserOn,
                                           reg->targetLaserOff);
#if REGISTER_NACK_BACKOFF_SUPPORT
            SilenceSomeLinks (1UL << link);
#endif
            OntmMpcpNotifyLinkNotReg(link);
            }
        else
            {
            MpcpDebug (MpcpDebugStd, ("MPCP: ignoring Reg Nack\n"));
            }
        break;
        
        default :
        MpcpDebug (MpcpDebugStd,
                   ("Unk flags (%02x) in Reg msg\n", reg->flags));
        break;
        }
    } // HandleMpcpRegister


////////////////////////////////////////////////////////////////////////////////
/// MpcpGateSynctime:
///
/// The caller should ensure that the message is actually a discovery gate
/// (msg->flags & Discovery)
/// \param m    Message containing discovery info
///
/// \return
/// Pointer to synctime
////////////////////////////////////////////////////////////////////////////////
static
MpcpInterval16 MpcpGateSynctime (const MpcpGateMsg* msg)
    {
    Stream strm;

    StreamInit(&strm, (U8  *)msg);
    StreamSkip(&strm,
               sizeof(MpcpGateMsg) +    // put us just after flags field
               sizeof(U32) +            // put us after grant start time
               sizeof(U16));            // put us after grant length aka
                                        // at synctime
    return EPON_NTOHS(StreamReadU16(&strm));
    } // MpcpGateSynctime


////////////////////////////////////////////////////////////////////////////////
/// SynchronizeMpcp:  Make sure ONU and OLT are in sync (capability, sync time)
///
 // Parameters:
/// \param gate     Discover gate message to be processed
///
/// \return
/// TRUE if the discovery gate was used to synchronize
////////////////////////////////////////////////////////////////////////////////
static
BOOL SynchronizeMpcp (const MpcpGateMsg * gateMsg)
    {
    U16  burstLen; // Total time needed to send reg req on the PON
    U16  grantOverHd;

    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        mpcpCfg.oltCapability =
            (U8)(EPON_NTOHS(MpcpGateDiscovery10G (gateMsg)->discoveryInfo));
        }

    // We first make sure the discovery gate is usable.
    if (((mpcpCfg.onuCapability &
          mpcpCfg.oltCapability &
          MpcpGateInfoWindowMsk) != 0) ||
        (PonCfgDbGetDnRate() != LaserRate10G))
        {
        // We may not yet be operational, but wil this discovery gate, we will
        // Remember sync time from OLT
        mpcpCfg.syncTime = MpcpGateSynctime(gateMsg);
        MpcpDebug (MpcpDebugStd, ("MPCP: Stime: 0x%x \n", mpcpCfg.syncTime));

        // Set Idle time based on sync time and default ONU laser ON/OFF
        // the return value is the actual grant overhead.
        grantOverHd = PonMgrSetDefaultIdleTime (mpcpCfg.syncTime);

        // bust lenght is time to send RegReq + grant overhead
        burstLen = RegReqTime + grantOverHd;

        mpcpCfg.windowRoom  = EPON_NTOHS(MpcpGateGrant(gateMsg, 0)->length);

        MpcpDebug (MpcpDebugStd,
                   ("MPCP: Disc.Gate W [0x%0x..0x%0x]\n",
                    EPON_NTOHL(MpcpGateGrant(gateMsg, 0)->startTime),
                    EPON_NTOHL(MpcpGateGrant(gateMsg, 0)->startTime)+mpcpCfg.windowRoom
                    ));

#ifdef CONFIG_BCM96858
        MpcpDebug (MpcpDebugStd,
                   ("MPCP: Mpcptime [0x%0x]\n", 
                   (PonCfgDbGetDnRate() == LaserRate10G) ? XifMpcpTimeGet() : LifMpcpTimeGet()
                   ));
#endif
        // Do we have enough room to send the reg req?
        if (mpcpCfg.windowRoom >= burstLen)
            {
            mpcpCfg.windowRoom -= burstLen;
            mpcpCfg.windowRoom++; // PRNG generates from 0..N-1
            }
        else
            {
            mpcpCfg.windowRoom = 0;
            }

        memcpy (&mpcpCfg.oltMacAddr, &(RxFrame->sa), sizeof(MacAddr));
        return TRUE;
        }

    return FALSE;
    } // SynchronizeMpcp


////////////////////////////////////////////////////////////////////////////////
/// CheckTimerWatchDogs - Check timer driven Watch Dogs
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CheckTimerWatchDogs (void)
    {
#if REGISTER_NACK_BACKOFF_SUPPORT
    // Silence mechanism just expired
    if ((mpcpCfg.silenceWatchDog != 0) && ((--mpcpCfg.silenceWatchDog) == 0))
        {
        mpcpCfg.silencedLinks = mpcpCfg.pendingSilencedLinks;
        mpcpCfg.pendingSilencedLinks = 0;
        // We start the watchdog even if we have no silenced links. It saves
        // code not to check and it costs nothing to have the w.d. running.
        mpcpCfg.silenceWatchDog = mpcpCfg.silencetime;
        }
#endif

    // Protect against stalled registration
    if (mpcpCfg.numRegistered == 0)
        {
        // We had been too long with no link registered
        if ((--mpcpCfg.allLinksWatchDog) == 0)
            {
            MpcpFatal ();
            }
        }
    else
        {
        mpcpCfg.allLinksWatchDog = AllLinksDownTimeOut;
        }

    // Protect against thrashing registration
    if (mpcpCfg.numDeregisteredSinceAudit != 0)
        {
        mpcpCfg.numDeregisteredSinceAudit--;

        // If the ONU had been continuously deregistering for too long
        if (mpcpCfg.thrashingWatchDog < ThrashingDeregStep)
            {
            MpcpFatal ();
            }
        else
            {
            mpcpCfg.thrashingWatchDog -= ThrashingDeregStep;
            }
        }
    else
        {
        if (mpcpCfg.thrashingWatchDog < LinkThrashingTimeOut)
            {
            mpcpCfg.thrashingWatchDog++;
            }
        }
    } // CheckTimerWatchDogs


////////////////////////////////////////////////////////////////////////////////
/// HandleMpcpDiscovery:  Handle discovery message on given link
///
 // Parameters:
/// \param gate     Discover gate message to be processed
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void HandleMpcpDiscovery (const MpcpGateMsg * gateMsg)
    {
    LinkIndex currentLink = 0;
    
    MpcpDebug (MpcpDebugStd,
            ("MPCP: Disc.Gate W [0x%0x..0x%0x]\n",
            EPON_NTOHL(MpcpGateGrant(gateMsg, 0)->startTime),
            EPON_NTOHL(MpcpGateGrant(gateMsg, 0)->startTime)+mpcpCfg.windowRoom));

    while (currentLink < mpcpCfg.numToRegister)
        {   
        if ((mpcpCfg.biDirLinks[currentLink].state >= rdpa_epon_link_in_service) || 
            (TestBitsSet(mpcpCfg.silencedLinks | mpcpCfg.pendingSilencedLinks, 
            1UL << currentLink)))
            {
            currentLink++;
            }
        else
            {
            break;
            }
        }
    if (currentLink >= mpcpCfg.numToRegister)
        {
        //all done, no more link need to get registered
        return;
        }
    
    if (rdpa_epon_link_registering == mpcpCfg.biDirLinks[currentLink].state)
        {
        if (SynchronizeMpcp (gateMsg))
            {
            MpcpRegReqEnqueue(currentLink);
            mpcpCfg.biDirLinks[currentLink].state = rdpa_epon_link_awaiting_register;
            if (mpcpCfg.numRegistered == 0)
                {
                if (pmd_fb_counter - (pmd_fb_counter >> 2) == 0)
                    OntmMpcpChangePmdTrackingState(rdpa_epon_link_awaiting_register);

                pmd_fb_counter++;
                }
            }
        }
    else if (rdpa_epon_link_awaiting_register == mpcpCfg.biDirLinks[currentLink].state)
        {
        mpcpCfg.biDirLinks[currentLink].state = rdpa_epon_link_registering;
        }
    } // HandleMpcpDiscovery



////////////////////////////////////////////////////////////////////////////////
/// HandleMpcp:  Handle MPCP messages
///
 // Parameters:
/// \param link     Link on which message arrived
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void HandleMpcp (U8  *msg)
    {
    MpcpGateMsg *  gateMsg =(MpcpGateMsg *)(msg+sizeof(EthernetFrame));
	
    MpcpDebug (MpcpDebugStd,
    	("\nMPCP: TS %0x\n", gateMsg->timestamp));
    DumpData(MpcpDebugStd,"HandleMpcp",msg,60);
	
    spin_lock_bh(&mpcp_spinlock);
    sendPort.pift = PortIfNone;
    if (mpcpCfg.numToRegister)
        {
        switch (EPON_NTOHS(gateMsg->opcode))
            {
            case MpcpGate :
            if ((gateMsg->flags & MpcpGateFlagDiscovery) != 0)
                { // Drop regular gates                    
                HandleMpcpDiscovery (gateMsg);
                
                if (mpcpGateNum < MaxMpcpGateCheck)
                    {
                    mpcpGateNum++;
                    }
                }
            	break;
            
            case MpcpRegister :
            HandleMpcpRegister ((MpcpRegister10GMsg *)gateMsg);
            	break;
            
            // ONT is passive, and should never get these
            case MpcpRegisterReq :
            case MpcpReport :
            case MpcpRegisterAck :
            default :                
            MpcpDebug (MpcpDebugStd,
            	("MPCP: Bad opcode %04x\n", EPON_NTOHS(gateMsg->opcode)));
            	break;
            }
        }
	
	spin_unlock_bh(&mpcp_spinlock);
	EponUsrPacketsSend(sendPort,60,txPktBuffer);
    } // HandleMpcp


////////////////////////////////////////////////////////////////////////////////
/// \brief  Change the number of links to register
///
/// \param  newNumLinks     New value for number of links
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpAdjustNumLinks(U8 newNumLinks)
    {
    LinkIndex link;
    
    spin_lock_bh(&mpcp_spinlock);
    
    for (link = newNumLinks; link < mpcpCfg.numToRegister; ++link)
        {
	    PutLinkOutOfService (link);
	    OsAstMsgQClr (OsAstAlmLinkUnregistered, link, 0);
        }
    for (link = mpcpCfg.numToRegister; link < newNumLinks; ++link)
        {
	    StartRegisteringNewLink(link);
        }
    mpcpCfg.numToRegister = newNumLinks;

    spin_unlock_bh(&mpcp_spinlock);
    } //  OntmMpcpAdjustNumLinks


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the number of links to register
///
/// \param  newNumLinks     New value for number of links
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
U8 OntmMpcpGetNumLinks(void)
    {    
    return mpcpCfg.numToRegister;
    } //  OntmMpcpAdjustNumLinks

void OntmMpcpSetNewMac(U8 link,const MacAddr *mac)  
    {
    memcpy(&PonMacCheck[link],mac,sizeof(MacAddr));
    if ( OntmMpcpLinkStateGet(link) <= rdpa_epon_link_awaiting_register)
        {
        EponSetMac(link,&PonMacCheck[link]);
        }
    }

////////////////////////////////////////////////////////////////////////////////
//extern
void MpcpReset(void)
    {
    MacAddr   zeromac;
    MacAddr  temp;
    LinkIndex  link;
    PonShaper shp;
    memset(&zeromac,0,sizeof(MacAddr));
    /* Remove all shaper */
    for (shp = 0; shp < EponUpstreamShaperCount; shp++)
        {
        (void)OntDirShaperDel(shp);
        }
    
    for (link = 0; link < mpcpCfg.numToRegister; link++)
        {
        memset(&temp,0,sizeof(MacAddr));
        EponGetMac(link, &temp);
        //Need Set New MAC address to PON MAC.
        if  ((memcmp(&PonMacCheck[link],&zeromac,sizeof(MacAddr)) != 0) &&
            (memcmp(&PonMacCheck[link],&temp,sizeof(MacAddr)) != 0))
            {
            EponSetMac(link,&PonMacCheck[link]);
            }
		OntmMpcpNotifyLinkNotReg(link);
        }
    }

void GateLosCheckTimeSet(U16 time)
    {
    grantLosChkTime = time;
    }


U16 GateLosCheckTimeGet(void)
    {
    return grantLosChkTime;
    }


////////////////////////////////////////////////////////////////////////////////
/// CheckGates:  Check gates for all links
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void CheckGates (void)
    {
    LinkIndex  link;
    for (link = 0; link < mpcpCfg.numToRegister; link++)
        {
        switch (mpcpCfg.biDirLinks[link].state)
            {
            case rdpa_epon_link_unregistered:
                // Idle
                break;
            
            case rdpa_epon_link_registering:
                // We jsut enqueue the reg_req under this state and 
                // wait for the register here. We do nothing here.
                break;
                
            case rdpa_epon_link_awaiting_register:
                // We are still waiting for a register to come. We should receive
                // regsiter in or before this state cyscle and we
                // can't wait forever either because the reg request may had
                // dropped (queue full or other) or it may had collided with
                // another reg request from another ONU on the PON.
                // Since we are protecting the registration with a time out. We
                // do nothing here.
                break;

            case rdpa_epon_link_awaiting_gate:
                // We will either get some Grants or eventually time out
                if (PonMgrLinkAlive (link))
                    {
                    OsAstMsgQClr (OsAstAlmLinkNoGates, link, 0);
                    PutLinkInService (link);
                    }
                else
                    {
                    if(mpcpCfg.biDirLinks[link].waitForGateWatchDog > 0)
                        {
                        mpcpCfg.biDirLinks[link].waitForGateWatchDog--;
                        }
                    else
                        {
                        OntmMpcpNotifyLinkNotReg(link);
                        }
                    }
                break;

            case rdpa_epon_link_in_service:
                // Check for grants on regitered links.
                if (!PonMgrLinkAlive (link))
                    {
                    OntmMpcpNotifyLinkNotReg(link);
                    OsAstMsgQSet (OsAstAlmLinkNoGates, link, 0);
                    }
                break;

            default:
                break;
            }
        }
    } // CheckGates

BOOL OntmMpcpPacketsCheck(U8 *data)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;
    if (EPON_NTOHS(ethHeader->type) == EthertypeMpcp)
        {
        return TRUE;
        }
    return FALSE;
    }

BOOL OntmOamPacketsCheck(U8 *data, U8 *vlanNum)
    {
    EthernetFrame  *ethHeader = (EthernetFrame*)data;

    if((memcmp(&ethHeader->da,&OamMcMacAddr,sizeof(OamMcMacAddr)) == 0) 
        || (memcmp(&ethHeader->da,(U8 *)PonCfgDbGetBaseMacAddr(),
                                        sizeof(MacAddr)) == 0))
        {
        *vlanNum = 0;
        while(1)
            {
            ethHeader = (EthernetFrame *)(data + (*vlanNum)*4);
            if((htons(ethHeader->type) == EthertypeCvlan) || 
                (htons(ethHeader->type) == EthertypeSvlan))
                {
                (*vlanNum)++;
                }
            else if(htons(ethHeader->type) == EthertypeOam)
                {
                return TRUE;
                }
            else
                {
                return FALSE;
                }
            }
        }
    return FALSE;
    }


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpInit:  Initialize MPCP module
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpInit (void)
    {
    LinkIndex  biDirLink;

    // Reset all variable configuration to 0
    memset (&mpcpCfg, 0x00, sizeof(mpcpCfg));
    memset(mpcpTimeOut10ms,0x00,sizeof(mpcpTimeOut10ms));
    memset (mpcpTimerLink,0x00,sizeof(mpcpTimerLink));
    memset(PonMacCheck,0x00,sizeof(PonMacCheck));
    MpcpMcMacAddr.byte[0] = 0x01;
    MpcpMcMacAddr.byte[1] = 0x80;
    MpcpMcMacAddr.byte[2] = 0xc2;
    MpcpMcMacAddr.byte[3] = 0x00;
    MpcpMcMacAddr.byte[4] = 0x00;
    MpcpMcMacAddr.byte[5] = 0x01;
    OamMcMacAddr.byte[0] = 0x01;
    OamMcMacAddr.byte[1] = 0x80;
    OamMcMacAddr.byte[2] = 0xc2;
    OamMcMacAddr.byte[3] = 0x00;
    OamMcMacAddr.byte[4] = 0x00;
    OamMcMacAddr.byte[5] = 0x02;

    //So we set to the opposite value for ignoring the failsafe
    OntmMpcpSetFailsafeEnabled (FALSE);

#if REGISTER_NACK_BACKOFF_SUPPORT
    mpcpCfg.silencetime = BackoffTimeOut;
    mpcpCfg.deallocatesilence = FALSE;

#endif

    // Tracking the associated unidirectional link can't be set to 0
    for (biDirLink = 0; biDirLink < TkOnuNumTxLlids; biDirLink++)
        {
        mpcpCfg.biDirLinks[biDirLink].associatedUniDirLink = TkOnuMaxRxOnlyLlids;
        mpcpCfg.biDirLinks[biDirLink].state = rdpa_epon_link_unregistered;
        mpcpTimerLink[biDirLink] = EponUsrModuleTimerRegister(
            EPON_TIMER_TO1, 0,OntmMpcpTimeOutTimer);
        }

    // Adapt our capability based on configured upstream rate
    switch (PonCfgDbGetUpRate())
        {
        case LaserRate1G:
            mpcpCfg.onuCapability = (U8) (MpcpGateInfo1GCapable |
                                          MpcpGateInfo1GAttempt);
            break;
        case LaserRate10G:
            mpcpCfg.onuCapability = (U8) (MpcpGateInfo10GCapable |
                                          MpcpGateInfo10GAttempt);
            break;
        default:
            mpcpCfg.onuCapability = (U8) (MpcpGateInfo1GCapable |
                                          MpcpGateInfo1GAttempt);
            break;
        }

    // Ony reply to what we are able to handle.
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        EponFilterDiscoveryGates ((MpcpDiscoveryInfo)mpcpCfg.onuCapability);
        }
    // seed random number generator
    //PbiSrand (GenSeed ());

    // Epon needs the MPCP mac address for the report frames
    EponSetOltMac (&MpcpMcMacAddr);

    // create MPCP 1 second handle timer
    //OntmTimerCreateRepeating (TkOsMsToTicks(1000) , MpcpHandleTimer);
    PonCfgDbSetMpcpCb(MpcpReset);

    //register timer to check gates
    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
               MpcpGateCheckTimer,OntmMpcpHandleTimer);

    OntmMpcpAdjustNumLinks(PonCfgDbGetLinkNum());

    } // OntmMpcpInit


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpTimeOutSet:  Background checking on links
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpTimeOutSet (LinkIndex link, U16 time)
    {
    mpcpTimeOut10ms[link] = time;
    } // OntmMpcpTimeOutSet


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpTimeOutGet:  Background checking on links
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
U16 OntmMpcpTimeOutGet (LinkIndex link)
    {
    return mpcpTimeOut10ms[link];
    } // OntmMpcpTimeOutGet


////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpHandleTimer:  Background checking on links
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void OntmMpcpHandleTimer (EponTimerModuleId id)
    {
    CheckTimerWatchDogs();
    CheckGates();
    } // OntmMpcpHandleTimer


BOOL OntmMpcpGetMcast(LinkIndex link, U32 *assignedPort, U32 *idxMcast)
    {
    U16 uniIndex;

    if ((link >= TkOnuNumBiDirLlids) ||
        (mpcpCfg.biDirLinks[link].state != 3))
        return FALSE;

    uniIndex = mpcpCfg.biDirLinks[link].associatedUniDirLink;
    
    if ((uniIndex >= TkOnuNumRxOnlyLlids) ||
        (mpcpCfg.uniDirLinks[uniIndex].useCount == 0))
        return FALSE;

    *idxMcast = TkOnuFirstRxOnlyLlid + uniIndex;
    *assignedPort = PonMgrGetPhyLlid(*idxMcast);

    return TRUE;
    }

BOOL OntmMpcpAssignMcast (U8 flag, LinkIndex unicast_link_id, 
	rdpa_epon_mcast_link_t *mcast_link)
    {
    BOOL  result = FALSE;
    LinkIndex llidIndex;
    
    if (flag == 0)
        {
        result = OntmMpcpGetMcast(unicast_link_id, 
        	&mcast_link->llid, &mcast_link->flow_idx);
        }
    else
        {
        if (mcast_link->enable)
            {
            llidIndex = OntPutMcastInService (unicast_link_id, mcast_link->llid);
            if(llidIndex != TkOnuMaxRxOnlyLlids)
                result = TRUE;
            }
        else
            {
            result = OntPutMcastOutOfService (unicast_link_id, mcast_link->llid);
            }
        }
    
    return result;
    } 

////////////////////////////////////////////////////////////////////////////////
/// OntmMpcpCmdInfo: CLI for MPCP info command
///
 // Parameters:
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OntmMpcpCmdInfo (void)
    {
    U8 argc;
    
    printk("BiDir ");
    
    for (argc = 0; argc<mpcpCfg.numToRegister; argc++)
        {
        LinkIndex  associatedLink =
        		mpcpCfg.biDirLinks[argc].associatedUniDirLink;
        
        printk ("link %d:Mpcp State %d, ", argc, mpcpCfg.biDirLinks[argc].state);		
        printk ("MLink ->%d; ", associatedLink);
        
        }
    printk ("\nMcast ");
    
    for (argc = 0; argc<TkOnuNumRxOnlyLlids; argc++)
        {
        U8  useCount = mpcpCfg.uniDirLinks[argc].useCount;
        if (useCount > 0)
            {
            printk ("argc:%d %d-0x%04x ",argc,
            		useCount,
            		PonMgrGetPhyLlid(TkOnuFirstRxOnlyLlid + argc));
            }
        }
    printk ("\n");
    } // OntmMpcpCmdInfo

// End of OntmMpcp.c
