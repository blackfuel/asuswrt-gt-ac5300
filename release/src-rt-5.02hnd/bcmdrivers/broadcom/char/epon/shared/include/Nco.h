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


#if !defined(Nco_h)
#define Nco_h
////////////////////////////////////////////////////////////////////////////////
// \file Nco.h
// \brief Numerically Control Oscillator module declaration
//
// This module implements the Numerically Control Oscillator which is used to
// drive the clock transport feature.
//
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"

typedef enum
    {
    NcoSrcNco       = 0,
    NcoSrcLif       = 1,
    NcoSrcXif       = 2
    } NcoSrc_e;

typedef U8 NcoSrc;

extern
void NcoSetCfgReset(Bool flag);


extern
U32 NcoGetCurPeriodCount(void);


extern 
void NcoKeepIntPerInt(void);


extern 
void NcoSetIntPerInt(U32 value);


extern
void NcoSetGainSync(BOOL sync);


////////////////////////////////////////////////////////////////////////////////
/// clearSyncInterrupt
/// \brief clears the Sync Interrupt
///
/// Parameters:
/// \param none
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern 
void ClearSyncInterrupt(void);


////////////////////////////////////////////////////////////////////////////////
/// syncLocked
/// \brief test the NcoInterrupt register to determine whether we are in sync
///
/// Note: the NcoInterrupt register goes high when sync is not locked.
///
/// Parameters:
/// \param none
///
/// \return
/// Bool true if sync has not been lost.
////////////////////////////////////////////////////////////////////////////////
extern 
BOOL SyncLocked(void);

////////////////////////////////////////////////////////////////////////////////
/// NcoSet1PpsHalfPeriod - Set 1PPS half period value
///
/// This function programs the half period (duty cycle) of the of the 1PPS.
/// The duty cycle is the amount of time in 100ns increments the pulse should
/// be high for each 1 second clock.
///
 // Parameters:
/// \param period Half period value in 100ns time quanta
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void NcoSet1PpsHalfPeriod (U32 period);


////////////////////////////////////////////////////////////////////////////////
/// NcoInit - Initialize NCO Module
///
/// This function initialized the Numerically Control Oscillator module,
/// disabling the module by default and holding it in reset.
///
 // Parameters:
/// \param NcoSrc input source for NCO
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void NcoInit (NcoSrc input);


#endif // Nco_h

