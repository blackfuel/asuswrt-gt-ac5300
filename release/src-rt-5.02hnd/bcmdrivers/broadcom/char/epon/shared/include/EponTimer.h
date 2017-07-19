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

#ifndef epon_user_h
#define epon_user_h

#include <linux/timer.h>
#include "Teknovus.h"


#define TIMER_IDLE    0
#define TIMER_RUNNING 1
#define TIMER_EXPIRED 2

#define EPON_TIMER_INTERVEL_MS 10
#define EPON_TIMER_INTERVEL_S  1


typedef enum {
  EPON_TIMER_TO1    = 0, 
  EPON_TIMER_TO2    = 1, 
  EPON_TIMER_End
} EponTimerId_e;

#define   EPON_TIMER_Num 64

typedef U8 EponTimerId;
typedef U8 EponTimerModuleId;
typedef void (*EponTimerHandler)(EponTimerModuleId moduleId);

typedef struct {
  EponTimerId      timerId;
  struct timer_list timer;
  volatile int      state;
  int               timeoutMs;
} PACK EponTimer; //epon stack timer

typedef struct {
  EponTimerModuleId  modId;    
  U32                         expiretime;//ms
  U64                         curtime;//ms
  EponTimerHandler   fun;
} PACK EponUsrModuleTimer;

typedef enum {
  Mpcp_Rx_Token    = 0, 
  Mpcp_Tx_Token    = 1, 
  Oam_Rx_Token    = 2,
  Oam_Tx_Token    = 3,
  Token_Num,
} EponToken_e;

#define MpcpOamTokenTimer  100//100Ms
#define MpcpOamTokenSize   1000//1000 packets burst.
#define MpcpOamToKenRate  100//1000 packets second


////////////////////////////////////////////////////////////////////////////////
/// \brief EPON TIMER initialization.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern 
void EponUsrTimerInit(EponTimer* usrTimer, EponTimerId timerId, U32 timeoutMs);

////////////////////////////////////////////////////////////////////////////////
/// \start EPON timer.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////

extern
void EponStackTimerInit(void);

extern
void EponStackTimerExit(void);

extern
void EponUserTimerReset(void);


extern
EponTimerModuleId EponUsrModuleTimerRegister(EponTimerId timerid,
									    unsigned int tick,
								            EponTimerHandler fun);


extern 
void EponUsrModuleTimerCfg(EponTimerId timerid,
								       EponTimerModuleId id,
								       unsigned int tick);

#endif /*epon_user_h*/

