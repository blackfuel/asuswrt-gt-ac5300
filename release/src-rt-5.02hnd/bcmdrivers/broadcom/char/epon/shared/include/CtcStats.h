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

/*                   Copyright(c) 2011 Broadcom, Inc.                         */

#if !defined(CtcStats_h)
#define CtcStats_h
////////////////////////////////////////////////////////////////////////////////
/// \file CtcStats.h
/// \brief General Ctc Statistics support
///
/// This file contains the CTC stats processing.
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define CtcStatsTimer  (1000) //1S


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetEnable:  Get CTC stat enable state
///
 // Parameters:
/// \param port    port number
///
/// \return
/// TRUE if stat is enabled, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern 
BOOL CtcStatsGetEnable(void);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsSetEnable:  Set CTC stat enable state
///
 // Parameters:
/// \param port    port number
/// \param enable  enable 
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsSetEnable(BOOL enable);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetPeriod:  Get CTC stat period
///
 // Parameters:
/// \param port    port number
///
/// \return
/// Stats period
////////////////////////////////////////////////////////////////////////////////
extern
U32 CtcStatsGetPeriod(void);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsSetPeriod:  Set CTC stat period
///
 // Parameters:
/// \param port    port number
/// \param period  period time
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsSetPeriod(U32 period);

extern
BOOL CtcStatsStatsVaild(CtcStatId id);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetStats:  Get CTC stat
///
 // Parameters:
/// \param port    port number
/// \param id      id
/// \param dst     pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsGetStats(CtcStatId id, U64 BULK * dst);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsClear:  Clear CTC stat
///
 // Parameters:
/// \param port    port number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsClear(void);


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetHisStats:  Get CTC history stat
///
 // Parameters:
/// \param port    port number
/// \param id      id
/// \param dst     pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsGetHisStats(CtcStatId id, U64 BULK * dst);



////////////////////////////////////////////////////////////////////////////////
/// CtcStatsInit:  CTC Statistics Init
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
extern
void CtcStatsInit (void);


#if defined(__cplusplus)
}
#endif

#endif // CtcStats.h

