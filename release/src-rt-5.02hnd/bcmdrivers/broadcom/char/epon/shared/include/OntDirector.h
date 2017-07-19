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



#if !defined(OntDirector_h)
#define OntDirector_h
////////////////////////////////////////////////////////////////////////////////
/// \file OntDirector.h
/// \brief Manages the high level ONU wide configuration
///
///
////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif
#include "Teknovus.h"
#include "PonManager.h"
#include "PonMgrEncrypt.h"
#include "PonMgrFec.h"
#include "Holdover.h"
#include "ProtSwitch.h"
#include "EponStats.h"
#include "OntmMpcp.h"
#include "OptCtrl.h"
#include "Lif.h"
#ifdef CONFIG_EPON_10G_SUPPORT
#include "Xif.h"
#endif

typedef enum
    {
    SimpleReportMode = 0,
    TeknovusReportMode = 1<<0,
    CtcReportMode = 1<<1,
    NttReportMode = 1<<2,
    ManualReportMode = 1<<3,
    TdmNoForceReportMode = 1<<4
    }EponReportModes_e;

typedef U8 EponReportModes;

#define OnuShaperDefBurstSize        50   
#define DefLinkNum                   1
//Refer to XENU-83 for full details
#define GmcTxHangDisableWait         3

typedef struct
    {
    U32 queueMap;
    U32 rate;
    U16 burstSize;
    } PACK OntShaper;

/// tracks config information per link
typedef struct
    {
    U8   oamQ;
    } PACK PerLinkInfo;

////////////////////////////////////////////////////////////////////////////////
/// \brief get the oam queue index of the link
///
/// \param link     Link index to set
///
/// \return
/// the oam queue index
////////////////////////////////////////////////////////////////////////////////
//extern
U8 OntDirLinkOamQGet(LinkIndex link);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Change the MAC address to PON side.
///
/// \param link     Index of link to change
/// \param Mac    PON MAC address
///
/// \return if  if set sucessful
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirMacSet(LinkIndex link, U8 *Mac);


extern
BOOL OntDirMacGet(LinkIndex link, U8 *Mac);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Change the FEC setting on a link
///
/// \param link     Index of link to change
/// \param rx       Enable Rx FEC?
/// \param tx       Enable Tx FEC?
///
/// \return if  if set sucessful
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirFecModeSet(LinkIndex link, BOOL rx, BOOL tx);



////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirFecModeGet(LinkIndex link, BOOL *rx, BOOL *tx);

////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON upstream rate
///
/// \param
/// None
///
/// \return
/// Upstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
extern
LaserRate OntDirUpRateGet (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON Downstream rate
///
/// \param
/// None
///
/// \return
/// Downstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
extern
LaserRate OntDirDnRateGet (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Applies a new burst cap to a link on the fly
///
/// \param link     Link index for burst cap
/// \param bcap     New burst cap values in a array of 16 bytes values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OntDirBurstCapSet(LinkIndex link, const U16 * bcap);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get burst cap to a link on the fly
///
/// \param link     Link index for burst cap
/// \param bcap     New burst cap values in a array of 16 bytes values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OntDirBurstCapGet(LinkIndex link,  U16 * bcap);

/* Set the Laser Rx Power On/Off. Never fails.
 *   on = 1
 *  off = 0
 */
void OntDirLaserRxPowerSet(bdmf_boolean on);

/* Get the Laser Rx power state. Never fails */
void OntDirLaserRxPowerGet(bdmf_boolean *enable);

/* Set the Laser Tx mode. Never fails */
void OntDirLaserTxModeSet(rdpa_epon_laser_tx_mode mode);

/* Get the Laser Tx mode. Never fails*/
void OntDirLaserTxModeGet(rdpa_epon_laser_tx_mode *mode);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies a new Layser status configuration
///
/// \param dir          upstream/dnstream
/// \param mode     pon configure mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLaserStatusSet(Direction dir, LaserTxMode mode);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies get Layser status configuration
///
/// \param dir          upstream/dnstream
/// \param mode     pon configure mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLaserStatusGet(Direction dir, LaserTxMode *mode);

/* Get the Link MPCP registration state. Never fails */
extern
void OntDirMpcpStateGet(LinkIndex link, rdpa_epon_link_mpcp_state *state);



////////////////////////////////////////////////////////////////////////////////
/// \brief Set New link numbers need to register.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
int OntDirNewLinkNumSet(U8 newlinks);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get register link numbers.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
int OntDirRegLinkNumGet(U8 * reglinks,U8 * onlinelinks);

/* set holdover parameters. never fails */
void OntDirHoldoverSet(rdpa_epon_holdover_t *holdover);

/* get holdover parameters. never fails */ 
void OntDirHoldoverGet(rdpa_epon_holdover_t *holdover);

////////////////////////////////////////////////////////////////////////////////
/// \brief Applies protect switch configuration
///
/// \param state         protect state
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirProtectSwitchSet(PsOnuState state);	



////////////////////////////////////////////////////////////////////////////////
/// \brief Get protect switch configuration
///
/// \param state         protect state
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirProtectSwitchGet(PsOnuState *state);


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
extern
BOOL OntDirLinkStatsGet(LinkIndex link, StatId id, U64  *dst);



////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
/// \param dptr    pointer to the stat value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirPonStatsGet(StatId id, U64  *dst);



////////////////////////////////////////////////////////////////////////////////
/// \brief get Pon stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLifStatsGet(LifStatId id, U64  *dst);

#ifdef CONFIG_BCM96858
#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// \brief get Xif stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirXifStatsGet(XifStatId id, U64  *dst);

////////////////////////////////////////////////////////////////////////////////
/// \brief get Xpcs32 stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirXpcs32StatsGet(Xpcs32RxStatId id, U64  *dst);

////////////////////////////////////////////////////////////////////////////////
/// \brief get Xpcs40 stats
///
/// \param id            stats id
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirXpcs40StatsGet(Xpcs40RxStatId id, U64  *dst);

#endif
#endif

////////////////////////////////////////////////////////////////////////////////
/// \brief get epon mac queue report configuration
///
/// \param cfg            pointer to the cfg parameters
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL ont_dir_queue_set(epon_mac_q_cfg_t *cfg);


////////////////////////////////////////////////////////////////////////////////
/// \brief get epon mac queue report configuration
///
/// \param cfg            pointer to store cfg parameters
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL ont_dir_queue_get(epon_mac_q_cfg_t *cfg);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get LLID according to the link index.
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLLIDGet(LinkIndex link,PhyLlid *llid);


extern
BOOL OntDirShaperSet(U32 shaperL1Map,	PonShaperRate rate,
						  PonMaxBurstSize size,PonShaper *shp);
extern
BOOL OntDirShaperDel(PonShaper shp);
extern
BOOL OntDirShaperGet(PonShaper shp, U32 *shaperL1Map,
						  PonShaperRate *rate,PonMaxBurstSize *size);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirL1ByteLimitSet(U8 queue, U8 limit);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirL1ByteLimitGet(U8 queue, U8* limit);


extern
void OntDirEponReset(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set upstream global shaper for whole ONU
///
/// \param rate            rate to set
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirGlobalShaperSet(U32 rate);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get upstream shaper for a link
///
/// \param rate            rate
/// \param burst          burst
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLinkShaperGet(LinkIndex link, U32 *rate, PonMaxBurstSize *burst);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set upstream shaper for a link
///
/// \param rate            rate to set
/// \param burst
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
int OntDirLinkShaperSet(LinkIndex link, U32 rate, PonMaxBurstSize burst);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get link loopback state.
///
///
/// \return
/// T/F
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEponLinkLoopbackGet(LinkIndex link, U8 *loopback);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set link loopback state.
///
///
/// \return
/// T/F
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL OntDirEponLinkLoopbackSet(LinkIndex link, U8 loopback);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get link flow id.
///
///
/// \return  flow id
/// 
////////////////////////////////////////////////////////////////////////////////
extern
U16 OntUserGetLinkFlow(U32 link, U32 queue);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get default link state
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirDefLinkState(U32 link);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption mode
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirEncryptModeSet(LinkIndex link, EncryptMode mode, EncryptOptions opts);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption key
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirEncryptKeySet(LinkIndex link, Direction dir, U8 keyIndex, U8 keylen, U32 *key);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set encryption key
///
///
/// \return  TRUE/FALSE
/// 
////////////////////////////////////////////////////////////////////////////////
extern
BOOL OntDirLinkEncrypted(LinkIndex link, Direction dir);

extern 
void OntDirInit(void);

#if defined(__cplusplus)
}
#endif

#endif // OntDirector.h
