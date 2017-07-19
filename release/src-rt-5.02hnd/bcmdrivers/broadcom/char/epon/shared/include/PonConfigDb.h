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

/*                   Copyright(c) 2011 Broadcom                               */

#if !defined(PonConfigDb_h)
#define PonConfigDb_h
////////////////////////////////////////////////////////////////////////////////
/// \file PonConfigDb.h
/// \brief Holds PON specific configuration
///
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"
#include "Mpcp.h"
#include "Ethernet.h"
#include "Holdover.h"
#include "ProtSwitch.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum
    {
    ActiveLo = 1,
    ActiveHi = 0
    } PACK Polarity;

typedef enum
    {
    LaserTxModeOff,
    LaserTxModeBurst,
    LaserTxModeContinuous,
    LaserTxModeCount
    } PACK LaserTxMode;

typedef enum
    {
    LaserRateOff,
    LaserRate1G,
    LaserRate2G,
    LaserRate10G
    } PACK LaserRate;

typedef enum
    {
    PonRateOneG = 1,
    PonRateTwoG = 2,
    PonRateTenG = 4
    } PACK PonRate;

//The personality and firmware are being tricky here.
//We want a SharedL2mode implemented but we do NOT want to
//expose it to the end users.  If you want to test SharedL2 mode
//Use a command line or non-pers editor formatted record to do so.
typedef enum
    {
    PonSchModePriorityBased = 0,
    PonSchMode3715CompatabilityMode = 1,
    PonSchModeSharedL2Mode = 2,
    } PACK PonSchMode;

#define Available   1
#define NoActivePon ((PortInstance)-1)

typedef BOOL (* RegisteredCb) (BOOL, BOOL);
typedef void (* MpcpCallback) (void);

typedef struct
    {
    BOOL            available;
    LaserRate       dnRate;
    LaserRate       upRate;
    MpcpInterval16  laserOn;
    MpcpInterval16  laserOff;
    Polarity        txPolarity;
    U8              offTimeOffset;
    U16             maxFrameSize;
    BOOL            preDraft2dot1;
    BOOL            nttReporting;
    U16             powerupTime;
    BOOL            txOffIdle;
    Port            ctrlInf;
    } PACK PonPortCfg;

typedef struct
    {
    BOOL autoSlect;
    BOOL autoSwitch;
    Port ctrlInf;//BSC instance
    } PACK PonAutoSelect;

typedef struct
    {
    RegisteredCb    cb;
    MpcpCallback    mpcpCb;
    PortInstance    activePon;
    PonPortCfg      pon[NumPonIf];
    PonAutoSelect   autoSel;
    } PACK PonCfg;


////////////////////////////////////////////////////////////////////////////////
/// PonCfgBaseMacGet:  Gets MAC address used for the EPON
///
 // Parameters:
/// \param None
///
/// \return
/// Pointer to MAC address for port 0
////////////////////////////////////////////////////////////////////////////////
extern
MacAddr const* PonCfgBaseMacGet (void);


////////////////////////////////////////////////////////////////////////////////
/// PonCfgBaseMacGet:  Gets MAC address used for the EPON
///
 // Parameters:
/// \param None
///
/// \return
/// Pointer to MAC address for port 0
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonCfgShpIpgGet (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return if we are configured for 3715 or SharedL2 mode
///
/// \param
/// None
///
/// \return
/// PonSchMode that is active
////////////////////////////////////////////////////////////////////////////////
extern
PonSchMode PonCfgDbSchMode (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return the idle time offset
///
/// \param
/// None
///
/// \return
/// the idle time offset
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonCfgDbIdleTimeOffset (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return the PON config
///
/// \param  Instance of PON
///
/// \return
/// PON configuration
////////////////////////////////////////////////////////////////////////////////
extern
PonPortCfg * PonCfgDbGet(PortInstance pon);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Indicate whether the active PON is available
///
/// \param
/// None
///
/// \return
/// TRUE if the active PON is available
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonCfgDbGetAvailable (void);



////////////////////////////////////////////////////////////////////////////////
/// \brief  Return the PON downstream rate
///
/// \param
/// None
///
/// \return
/// Downstream Pon Rate
////////////////////////////////////////////////////////////////////////////////
extern
LaserRate PonCfgDbGetDnRate (void);

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
LaserRate PonCfgDbGetUpRate (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Gets laser-On time
///
/// \param
/// None
///
/// \return
/// laser on Time (in TQ)
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonCfgDbGetLaserOnTime (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Gets laser-Off time
///
/// \param
/// None
///
/// \return
/// laser off Time (in TQ)
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonCfgDbGetLaserOffTime (void);

////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonCfgDbSetRegCb(RegisteredCb cb);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
RegisteredCb PonCfgDbGetRegCb(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonCfgDbSetMpcpCb(MpcpCallback cb);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set external callback to determine if ONU is registered
///
/// \param cb   Callback to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
MpcpCallback PonCfgDbGetMpcpCb(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Return the PON upstream polarity
///
/// \param
/// None
///
/// \return
/// Upstream Pon Polarity
////////////////////////////////////////////////////////////////////////////////
extern
Polarity PonCfgDbGetUpPolarity (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Initialize EPON port
///
/// Resets EPON MAC to default values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
PortInstance PonCfgDbGetActivePon (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the currently active PON port
///
/// \param pon  New active PON port
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonCfgDbActivePonSet(PortInstance pon);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get standby PON interface
///
/// \return
/// Standby PON
////////////////////////////////////////////////////////////////////////////////
extern
PortInstance PonCfgDbGetStandbyPon (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get off time offset of active PON
///
/// \return
/// Off time offset
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonCfgDbGetOffTimeOffset (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get max frmae size of active PON
///
/// \return
/// Max frame size
////////////////////////////////////////////////////////////////////////////////
extern
U16 PonCfgDbGetMaxFrameSize (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get pre-draft 2.1 of active PON
///
/// \return
/// Pre draft 2.1
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonCfgDbGetPreDraft2Dot1 (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get NTT reporting of active PON
///
/// \return
/// NTT reporting
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonCfgDbGetNttReporting (void);

////////////////////////////////////////////////////////////////////////////////
/// \brief Get txoffIdle
///
/// \param none
///
/// \return
/// txoffIdle
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonCfgDbGettxOffIdle (void);
////////////////////////////////////////////////////////////////////////////////
/// \brief Get laser powerup time
///
/// \param pon  Laser instance
///
/// \return
/// Laser powerup time
////////////////////////////////////////////////////////////////////////////////

extern
U16 PonCfgDbGetPowerupTime (U8 pon);


/// \brief Get laser control interface
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////


extern
Port PonCfgDbGetCtrlInf (U8 pon);



////////////////////////////////////////////////////////////////////////////////
/// \brief Get Holdover default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonGetHoldover(rdpa_epon_holdover_t *val);



////////////////////////////////////////////////////////////////////////////////
/// \brief Get active optctrl default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonGetActOptCtrl(U16 *time);


////////////////////////////////////////////////////////////////////////////////
/// \brief Get standby optctrl default configuration.
///
/// \param pon  Laser instance
///
/// \return
/// Laser control interface
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonGetStdOptCtrl(U16 *time);
extern
BOOL PonGetActPs(PsConfig10G *cfg);

extern
BOOL PonCfgDbGetIpg(void);

extern
MacAddr const* PonCfgDbGetBaseMacAddr(void);

extern
U8 PonCfgDbGetLinkNum(void);

extern
void PonCfgDbSetPonParam(EponCfgParam * poncfgparm);

extern 
void PonCfgDbGetPonParam(EponCfgParam *poncfgparm);


////////////////////////////////////////////////////////////////////////////////
/// \brief sets up PON config DB
///
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
extern
void PonCfgDbInitPon(void);



////////////////////////////////////////////////////////////////////////////////
/// \brief function monitor PON speed modification
///
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
extern
void PonSpeedMonitor(void);


#if defined(__cplusplus)
}
#endif

#endif

// PonConfigDb.h

