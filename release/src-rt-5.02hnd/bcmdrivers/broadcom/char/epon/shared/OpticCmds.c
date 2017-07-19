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
/// \file OpticCmds.c 
/// \brief dumps LIF related stats
///
///
////////////////////////////////////////////////////////////////////////////////
#include <linux/string.h>
#include "Lif.h"
#include "Teknovus.h"
#include "OpticCmds.h"
#ifdef CONFIG_EPON_10G_SUPPORT               
#include "Xif.h"
#endif

char  * LifStatStrTab [] = 
    {
    "Frames",
    "Bytes",
    "NonFecFrames",
    "NonFecBytes",
    "FecFrames",
    "FecBytes",
    "FecBlocks",
    "Reports",

    "LineCodeErr",
    "Gates",
    "GoodFrames",
    "GoodBytes",
    "UnderSzFrames",
    "OverSzFrames",
    "Crc8ErrFrames",
    "FecFrames",
    "FecBytes",
    "FecExceedErr",
    "NonFecFrames",
    "NonFecBytes",
    "FecCorrBytes",
    "FecCorrZeroes",
    "FecNoErrs",
    "FecCorrBlocks",
    "FecUnCorrBlocks",
    "FecCorrOnes",
    "ErrFrms",
    };

#ifdef CONFIG_EPON_10G_SUPPORT
char  * XifStatStrTab [] = 
    {
    "Frame",
    "Bytes",
    "Runts",
    "CodewordError",
    "Crc8Error",

    "DataFrame",
    "DataByte",
    "MpcpFrame",
    "OamFrame",
    "OamByte",
    "OversizeFrame",
    "SecAbortFrame",
    };

char  * Xpcs32StatStrTab [] = 
    {
    "FecDecFailCnt",
    "FecDecTotalCnt",
    "64b66bDecErrCnt",
    "FramerBadShCnt",
    "TestPseudoErrCnt",
    "TestPrbsErrCnt",
    };

char  * Xpcs40StatStrTab [] = 
    {
    "FecDecErrCorCnt",
    };
#endif


////////////////////////////////////////////////////////////////////////////////
/// CmdLifStats: LIF module stats display command
///
///	Primarily used for debugging this will dump the LIF statistics
///
/// \param argc	Argument count
///	\param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void CmdLifStats (void)
    {
    LifStatId  lifStat;
    
    printk("LIF stats: \n");
    printk("TX:\n");
    for(lifStat = LifStatFirst; lifStat < LifStatCount; ++lifStat)
        {
        if(lifStat == LifFirstRxStat)
            {
            printk("RX:\n");
            }
        printk(
        "%30s: %10u \n",
        LifStatStrTab[lifStat],
        LifReadStat(lifStat) );
        }
    } // CmdLifStats

    
#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// CmdLifStats: XIF module stats display command
///
///	Primarily used for debugging this will dump the XIF statistics
///
/// \param argc	Argument count
///	\param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
void CmdXifStats (void)
    {
    XifStatId  xifStat;
    
    printk("XIF stats: \n");
    
    for(xifStat = XifStatFirst; xifStat < XifStatCount; ++xifStat)
        {
        printk(
        "%30s: %10u \n",
        XifStatStrTab[xifStat],
        XifReadStat(xifStat) );
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// CmdLifStats: XifXpcs32 module stats display command
///
///	Primarily used for debugging this will dump the XifXpcs32 statistics
///
/// \param argc	Argument count
///	\param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
void CmdXpcs32Stats (void)
    {
    Xpcs32RxStatId  Xpcs32Stat;
    
    printk("Xpcs32 stats: \n");
    printk("RX:\n");
    for(Xpcs32Stat = Xpcs32RxStatFirst; Xpcs32Stat < Xpcs32RxStatCount; ++Xpcs32Stat)
        {
        printk(
        "%30s: %10u \n",
        Xpcs32StatStrTab[Xpcs32Stat],
        XifXpcsRead32Stat(Xpcs32Stat) );
        }
    } 


////////////////////////////////////////////////////////////////////////////////
/// CmdLifStats: XifXpcs40 module stats display command
///
///	Primarily used for debugging this will dump the XifXpcs40 statistics
///
/// \param argc	Argument count
///	\param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
void CmdXpcs40Stats (void)
    {
    Xpcs40RxStatId  Xpcs40Stat;
    
    printk("Xpcs40 stats: \n");
    printk("RX:\n");
    for(Xpcs40Stat = Xpcs40RxStatFirst; Xpcs40Stat < Xpcs40RxStatCount; ++Xpcs40Stat)
        {
        printk(
        "%30s: %llu \n",
        Xpcs40StatStrTab[Xpcs40Stat],
        XifXpcsRead40Stat(Xpcs40Stat) );
        }
    }
#endif

// end OpticCmds.c
