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
/// \file NcoRegs.h
/// \brief Numerically Control Oscillator Register Definitions
/// 
////////////////////////////////////////////////////////////////////////////////
#if !defined(NcoRegs_h)
#define NcoRegs_h

#if defined(__cplusplus)
extern "C" {
#endif

#include "Teknovus.h"

#define NcoCfg                              TkOnuReg(0x00C0)
#define NcoCfgReset                         0x00000001UL
#define NcoCfgSrcInSft                      1
#define NcoCfgSrcInMsk                      0x00000006UL
#define NcoCfgSrcOutSft                     3
#define NcoCfgSrcOutMsk                     0x00000018UL
#define NcoCfgSrcOut10MhzSft                5
#define NcoCfgSrcOut10MhzMsk                0x00000060UL
#define NcoCfgBypassInSft                   7


#define NcoInt                              TkOnuReg(0x00C1)
#define NcoIntNoLifPps                      0x00000001UL
#define NcoIntNoXifPps                      0x00000002UL
#define NcoIntNoNcoSync                     0x00000004UL 
#define Nco1PpsPeriod                       TkOnuReg(0x00C3)
#define Nco8KhzPeriod                       TkOnuReg(0x00C4)
#define NcoInitPerIntVal                    TkOnuReg(0x00C5)
#define NcoIntGainVal                       TkOnuReg(0x00C6)
#define NcoPropGainVal                      TkOnuReg(0x00C7)
#define NcoCurPerIntVal                     TkOnuReg(0x00C8)
#define NcoCur1PpsHalfPeriod                TkOnuReg(0x00C9)
#define NcoCur8KhzHalfPeriod                TkOnuReg(0x00Ca)
#define NcoCurPeriodCount                   TkOnuReg(0x00cb)
#define NcoCurPhaseErrorCount               TkOnuReg(0x00Cc)
#define EpnCfgNcoDivCfg                     TkOnuReg(0x0d0)
#define EpnCfgNcoDivCfg1                    TkOnuReg(0x0d1)

#if defined(__cplusplus)
}
#endif


#endif // NcoRegs_h


