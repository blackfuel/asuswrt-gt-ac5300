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


////////////////////////////////////////////////////////////////////////////////
/// \file OptCtrl.h
/// \brief Optics Control module
///
/// The Optics Control Module contains functions for controlling the ONU laser
/// as well as determining what control capabilities are available.
///
////////////////////////////////////////////////////////////////////////////////

#if !defined(OptCtrl_h)
#define OptCtrl_h

#if defined(__cplusplus)
extern "C" {
#endif
#include "Teknovus.h"



#define OptCtrlDisableForeverTime       0xFFFF
#define OptCtrlLegacyForever            0xFF

#define OptCtrlTimer   (1000)//1S

typedef enum
    {
    LaserCapsNone       = 0,
    LaserCapsTxPower    = (1 << 0), // TX power control
    LaserCapsRxPower    = (1 << 1), // RX power control
    LaserCapsTxPower2   = (1 << 2), // TX power control on secondary xcvr
    LaserCapsRxPower2   = (1 << 3)  // RX power control on secondary xcvr
    } OptCtrlLaserCaps_e;

typedef U8 OptCtrlLaserCaps;

typedef enum
    {
    LaserStateEnable            = 0,
    LaserStateUnchanged         = LaserStateEnable,
    LaserStateDisOptFail        = (1 << 0),
    LaserStateDisOptSleep       = (1 << 1),
    LaserStateDisStandby        = (1 << 2)
    } OptCtrlLaserState_e;

typedef U8 OptCtrlLaserState;
////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the state of the Tx/Rx laser
///
/// \param tx   New Tx state for the laser
/// \param rx   New Rx state for the laser
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OptCtrlSetLaserState(U8 pon, OptCtrlLaserState tx, OptCtrlLaserState rx);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Clear state from the Tx/Rx laser
///
/// \param tx   New Tx state for the laser
/// \param rx   New Rx state for the laser
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OptCtrlClearLaserState(U8 pon, OptCtrlLaserState tx, OptCtrlLaserState rx);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable laser transmitter
///
/// Disables the laser transmitter for the specified length of time
///
/// \param actOpt       Active optical or standby optical
/// \param enableTime   Time in seconds to reenable
/// \param preserve     Whether to store disable time in NVS
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OptCtrlTxDiagSet(BOOL actOpt, U16 enableTime, BOOL preserve);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get laser control capabilities
///
/// Returns a bitmap of which laser control GPIO functions are currently
/// mapped to a GPIO pin
///
/// \return Current laser control capabilities
////////////////////////////////////////////////////////////////////////////////
extern
OptCtrlLaserCaps OptCtrlGetLaserCaps(void);


////////////////////////////////////////////////////////////////////////////////
/// OptCtrlInit - Initial laser power settings
///
/// This function initializes the laser power settings(enable the transmitter
/// side of laser).
///
/// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void OptCtrlInit (void);



#if defined(__cplusplus)
}
#endif

#endif


// End of File OptCtrl.h
