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



#if !defined(Stats_h)
#define Stats_h
////////////////////////////////////////////////////////////////////////////////
/// \file Stats.h
/// \brief General Statistics support
///
/// This file contains the routines needed to handle statistics gathering
/// and retrieval
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"
#include "EponMac.h"
#include "EponTypes.h"
#include "Lif.h"
#ifdef CONFIG_EPON_10G_SUPPORT
#include "Xif.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define STATS_THRESHOLD 0
#define StatsGatherTimer   10//10MS


////////////////////////////////////////////////////////////////////////////////
/// Typedef definition
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    EponLinkStatId      eponStat;       // EPON stats
    EponLinkStatId      biDirLlidStat;  // Link bidirectional(Rx and Tx) stats
    EponLinkStatId      rxOnlyLlidStat; // Link receive only stats   
    } PACK TkStatIdMap;


extern const TkStatIdMap  statIdMap[];

extern BOOL FAST statsGatherEnabled;

////////////////////////////////////////////////////////////////////////////////
/// StatsGetSwStatId:  get the software calculated statistic id for the statId
///
 // Parameters:
/// \param statId The statId to get
///
/// \return
/// SwStatId when found, SwStatIdNum otherwise
////////////////////////////////////////////////////////////////////////////////
extern
SwStatId StatsGetSwStatId(StatId statId);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetEpon:  Get Epon port stat
///
 // Parameters:
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsGetEpon (StatId id, U64 BULK * dst);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLink:  Get epon link stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsGetLink (LinkIndex link, StatId id, U64 BULK * dst);




////////////////////////////////////////////////////////////////////////////////
/// StatsGetLif:  Get lif stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsGetLif (LifStatId id, U64 BULK * dst);


#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// StatsGetXif:  Get xif stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////

extern
BOOL StatsGetXif (XifStatId id, U64 BULK * dst);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetXif:  Get Xpcs32 stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////

extern
BOOL StatsGetXpcs32(Xpcs32RxStatId id,U64 BULK * dst);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetXif:  Get Xpcs40 stat
///
 // Parameters:
/// \param lnk     link index
/// \param stat    id
/// \param dptr    pointer to the stat value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////

extern
BOOL StatsGetXpcs40(Xpcs40RxStatId id,U64 BULK * dst);
#endif

#if STATS_THRESHOLD
////////////////////////////////////////////////////////////////////////////////
/// \brief  Get EPON stat threshold value
///
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsEponThdValGet(StatId id, void BULK * val);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get link stat threshold value
///
/// \param link Index of link
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsLinkThdValGet(LinkIndex link, StatId id, void BULK * val);




////////////////////////////////////////////////////////////////////////////////
/// \brief  Set EPON stat threshold value
///
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsEponThdValSet(StatId id, const void BULK * val);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set link stat threshold value
///
/// \param link Index of link
/// \param id   Stat
/// \param val  Pointer to threshold value
///
/// \return
/// TRUE if stat is supported, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL StatsLinkThdValSet(LinkIndex link, StatId id, const void BULK * val);




////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponThdInv:  Get epon port stat threshold interval
///
 // Parameters:
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
extern
U16 StatsGetEponThdInv (EponLinkStatId stat);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkThdInv:  Get epon link stat threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
extern
U16 StatsGetLinkThdInv (LinkIndex lnk, EponLinkStatId stat);





////////////////////////////////////////////////////////////////////////////////
/// StatsGetEponSwStatThdInv:  Get epon port Frame stat threshold interval
///
 // Parameters:
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
extern
U16 StatsGetEponSwStatThdInv (SwStatId stat);


////////////////////////////////////////////////////////////////////////////////
/// StatsGetLinkSwStatThdInv:  Get epon link stat Frame threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
///
/// \return
///     sample interval
////////////////////////////////////////////////////////////////////////////////
extern
U16 StatsGetLinkSwStatThdInv (LinkIndex link,SwStatId stat);


////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponThdInv:  Set epon port stat threshold interval
///
 // Parameters:
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsSetEponThdInv (EponLinkStatId stat, U16 inv);


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkThdInv:  Set epon link stat threshold value
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsSetLinkThdInv (LinkIndex lnk, EponLinkStatId stat, U16 inv);


////////////////////////////////////////////////////////////////////////////////
/// StatsSetEponSwStatThdInv:  Set epon port Frame stat threshold interval
///
 // Parameters:
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsSetEponSwStatThdInv (SwStatId stat, U16 inv);


////////////////////////////////////////////////////////////////////////////////
/// StatsSetLinkSwStatThdInv:  Set epon link stat Frame threshold interval
///
 // Parameters:
/// \param lnk    link index
/// \param stat   stat
/// \param inv    sample interval
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsSetLinkSwStatThdInv (LinkIndex link, SwStatId stat, U16 inv);
#endif


////////////////////////////////////////////////////////////////////////////////
/// StatsClearLif:  Reset all LIF stats to zero
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearLif (void);

#ifdef CONFIG_EPON_10G_SUPPORT
////////////////////////////////////////////////////////////////////////////////
/// StatsClearLif:  Reset all XIF stats to zero
///
/// Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearXif (void);

////////////////////////////////////////////////////////////////////////////////
/// StatsClearXpcs32:  Reset all Xpcs32 stats to zero
///
/// Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearXpcs32 (void);

////////////////////////////////////////////////////////////////////////////////
/// StatsClearXpcs40:  Reset all Xpcs40 stats to zero
///
/// Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearXpcs40(void);

#endif
////////////////////////////////////////////////////////////////////////////////
/// StatsClearEpon: Reset all epon stats to zero
///
/// \param None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearEpon(void);


////////////////////////////////////////////////////////////////////////////////
/// StatsClearAll:  Zeroes all stats counters
///
 // Parameters:
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearAll (void);


extern
void StatsGatherSet(BOOL flag);


////////////////////////////////////////////////////////////////////////////////
/// StatsClearEponLinkStats: Clear EPON link stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void StatsClearEponLinkStats (U8 linkStart, U8 linkEnd);

////////////////////////////////////////////////////////////////////////////////
/// StatsInit:  Initialize Statistics Manager
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
extern
void StatsInit (void);


#if defined(__cplusplus)
}
#endif

#endif // Stats.h
