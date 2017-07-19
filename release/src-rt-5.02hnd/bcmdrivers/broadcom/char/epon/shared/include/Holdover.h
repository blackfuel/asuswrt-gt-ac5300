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



#if !defined(Holdover_h)
#define Holdover_h
////////////////////////////////////////////////////////////////////////////////
/// \file Holdover.h
/// \brief ONU Holdover Configuration
///
/// ONU holdover is triggered at loss of signal or loss of gates from the OLT.
/// When this occurs the ONU will disable upstream traffic and flush all queues
/// then wait the alloted amount of time (holdover time).  If no signal has been
/// established when the time expires the links of the ONU will be torn down
/// completely, however in the event of a successful protection switch the ONU
/// will re-enable links as if nothing happened.  See FRD_Protection.doc for
/// more information about this feature.
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"

// allow 5 second lack of gates when in holdover (in 262us units)
#define MpcpHoldoverNoGrantTime (5000000 / 262)

////////////////////////////////////////////////////////////////////////////////
/// HoldoverSetTime - Set ONU Holdover time
///
/// This function sets the time that an ONU will wait before resetting links.
/// The units are 10ms increments.  The value is stored in NVS.
///
/// Parameters:
/// \param time Holdover time in 10ms increments
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
BOOL HoldoverIsEnabled(void);


////////////////////////////////////////////////////////////////////////////////
/// HoldoverSetParams - Set ONU Holdover params
///
/// This function sets the time that an ONU will wait before resetting links.
/// The units are 1ms increments.  The value is stored in NVS.
///
/// Parameters:
/// \param time     Holdover time in 10ms increments
/// \param flags    Behavioral modifiers
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void HoldoverSetParams (U16 time, rdpa_epon_holdover_flags flags);


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetTime - Get ONU holdover time
///
/// This function returns the amount of time the ONU should wait after loss of
/// signal or loss of gate to reset the links.  The time is in 1ms increments.
///
/// Parameters:
///
/// \return
/// Current ONU holdover time
////////////////////////////////////////////////////////////////////////////////
extern
U16 HoldoverGetTime (void);


////////////////////////////////////////////////////////////////////////////////
/// HoldoverGetFlags - Get current holdover configuration
///
/// This function returns the current extended ONU holdover configuration.
///
/// Parameters:
///
/// \return
/// Current holdover configuration
////////////////////////////////////////////////////////////////////////////////
rdpa_epon_holdover_flags HoldoverGetFlags (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Callback to run the holdover state machine
///
/// \param  los         Is ONU in LOS
/// \param eponInSync   Is EPON in sync
///
/// \return
/// Sync state corrected for holdover
////////////////////////////////////////////////////////////////////////////////
extern
BOOL HoldoverCallback(BOOL los, BOOL eponInSync);


////////////////////////////////////////////////////////////////////////////////
/// HoldoverInit - Initialize ONU holdover values
///
/// This function loads holdover times from NVS or personality of no record
/// exists.
///
/// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void HoldoverInit (void);

#endif // End of File Holdover.h

