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



#if !defined(ProtSwitch_h)
#define ProtSwitch_h
////////////////////////////////////////////////////////////////////////////////
/// \file   ProtSwitch.h
/// \brief  ONU Protection Switching configuration
///
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"

typedef enum
    {
    PsReasonHost,
    PsReasonLos,
    PsReasonMpcp,
    PsReasonHighBer,

    PsReasonMax,

    PsReasonNone = 255
    } PsReason_e;

typedef U8 PsReason;

#define PsStateActiveMask   0x0001
#define PsStateOutputMask   0x0006

#define PsTimer   (1/EPON_TIMER_INTERVEL_S)//1S

// time to wait between switches when scanning for a signal
#define PsScanTimeInit          2550 / TkOsMsPerTick
#define PsScanTimeHost          2550 / TkOsMsPerTick
#define PsScanTimeLos           200 / TkOsMsPerTick
#define PsScanTimeMpcp          400 / TkOsMsPerTick


////////////////////////////////////////////////////////////////////////////////
/// PsIsSecXcvrInUse:  Are we using the secondary xcvr?
///
/// Parameters:
///     None
///
/// \return
///     TRUE if secondary xcvr is active, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PsIsSecXcvrInUse(void);


////////////////////////////////////////////////////////////////////////////////
/// PsSetState:  Set the protection switch state
///
/// Parameters:
/// \param state    Protection switch state to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PsSetState(PsOnuState state);


////////////////////////////////////////////////////////////////////////////////
/// PsGetState:  Get the protection switch state
///
/// Parameters:
///     None
///
/// \return
///     The current protection switch state
////////////////////////////////////////////////////////////////////////////////
extern
PsOnuState PsGetState(void);



////////////////////////////////////////////////////////////////////////////////
/// PsGetType:  Get the protection switch type
///
/// Parameters:
///     None
///
/// \return
/// The current protection switch tyoe
////////////////////////////////////////////////////////////////////////////////
extern
PsConfig PsGetType(void);

////////////////////////////////////////////////////////////////////////////////
/// PsStartScan:  Begin scanning for a good signal
///
/// Parameters:
/// \param reason   Reason we are scanning
/// \param time     Delay before next switch (system ticks)
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void PsStartScan(PsReason reason, U16 time);


////////////////////////////////////////////////////////////////////////////////
/// PsScan:  Scan for a good signal
///
/// Parameters:
/// \param time   Time between switches (in system ticks)
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void PsScan(U16 time);


////////////////////////////////////////////////////////////////////////////////
/// PsStopScan:  Stop scanning for a good signal
///
/// Parameters:
/// None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void PsStopScan(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Notify host of reason for switch (if any)
///
/// \param reason   Reason to send to host
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PsNotifyReason(void);


////////////////////////////////////////////////////////////////////////////////
/// PsInit:  Initialize the protection switching module
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsInit(void);


#endif // End of File ProtSwitch.h
