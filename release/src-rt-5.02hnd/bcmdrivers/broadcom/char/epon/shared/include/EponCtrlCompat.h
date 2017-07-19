/*
*  Copyright 2015, Broadcom Corporation
*
* <:copyright-BRCM:2015:proprietary:standard
* 
*    Copyright (c) 2015 Broadcom 
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


#if !defined(Eponctrl_compat_h)
#define Eponctrl_compat_h

#include "EponUser.h"
#include "Teknovus.h"

typedef U32 compat_uptr_t;

extern 
long EponIoctlAcessCompat(unsigned int cmd, unsigned long arg);

typedef struct {
    EponStatsIf port;
    U8          instance;
    U8          statsCount;
    union {
        compat_uptr_t  statsVal2; 
    }StatsCnt;
} PACK StatsParaCompat;

typedef struct {
    compat_uptr_t cfg;
} PACK pon_mac_q_Para_compat;

typedef struct {
    U8  link;
    U8  count;
    compat_uptr_t bcapsize;
} PACK BcapParaCompat;

typedef struct {
    U8  statsCount;
    compat_uptr_t statsAlmVal;
} PACK CtcStatsAlarmParaCompat;

typedef struct {
    U8  statsCount;
    compat_uptr_t statsTheVal;
} PACK CtcStatsAlmTheParaCompat;

typedef struct {
    U8  history;
    U8  statsCount;
    compat_uptr_t statsVal;
} PACK CtcStatsParaCompat;

typedef struct {
    Direction dir;
    U8  link;
    U8  keyindex;
    U8  length;
    compat_uptr_t key;
    compat_uptr_t sci;
} PACK KeyDataParaCompat;

typedef union {
        compat_uptr_t  poncfg;          
        EponLoadCfg    eponact;
        DebugPara      debug;
        int            nlpid;
        RegRWPara      reg;
        PonMgrRptMode  reportmod;
        StatsParaCompat stats;           
        pon_mac_q_Para_compat epon_mac_q; 
        StaticLinkPara staticlink;
        PhyLLIDPara    llid;
        EponLinksPara  linksInfo;
        FecPara        fec;
        rdpa_epon_holdover_t  holdover; 
        LinkStatusPara linkstatus;
        TxPowerPara    txpower;
        LaserEnablePara laserpara;
        PsOnuState     psstate;
        LosCheckPara   lostime; 
        SilencePara    silence;
        KeyInUsePara   keyinuse;
        KeyDataParaCompat    keydata;
        KeyModPara     keymode;
        U8  gather;
        U8  statsdumpid;                //0:lif,1:epon,2-9: link
        BcapParaCompat     bcapval;    
        ShaperPara     shpval;
        CtcStatsAlarmParaCompat  ctcstatsalm; 
        CtcStatsAlmTheParaCompat ctcstatsthe;
        CtcStatsPeriodPara ctcstatsperiod;
        CtcStatsParaCompat ctcstats;  
        WanStatePara    wanstate;
        AssignMcast   assignMcast;  
        UserTrafficPara   userTraffic;  
        ByteLimitPara   bytelimit;
        OuiVendor   oamOui;
        LinkLoopbackPara   loopback;
        ClkTransPara clkTrans;
        EponRogueOnuPara rogueonudetect;
        OamExtMpcpClock extmpcpClk;
        TkOamOnuClkTransConfig clkTransCfg;
        OamExtTimeOfDay extTod;
        U8 failsafe;
    } PACK EponParmCompat;

typedef struct {
    EponCtlOpe ope;
    EponParmCompat  eponparm;
} PACK EponCtlParamtCompat;       /*PACK all*/

#endif 
