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

#include <linux/string.h>

#include "EponStats.h"
#include "CtcStats.h"
#include "CtcAlarms.h"
#include "CtcOam.h"
#include "EponUser.h"
#include "EponCtrl.h"

////////////////////////////////////////////////////////////////////////////////
/// Macro definition
////////////////////////////////////////////////////////////////////////////////
#define CtcStatsDefPeriod   900 // 15 minutes


////////////////////////////////////////////////////////////////////////////////
/// Typedef struct definition
////////////////////////////////////////////////////////////////////////////////
typedef struct
    {
    BOOL    enable;    
    U32     period;
    U32     time;
    U64     stats[CtcStatIdNums];
    } PACK CtcStats;


typedef struct
    {
    StatId      eponStat; 
    StatId      uniStat;
    } PACK CtcStatIdMap;




////////////////////////////////////////////////////////////////////////////////
/// ctcStatIdMap - CTC stat to Broadcom stat mapping
///
////////////////////////////////////////////////////////////////////////////////
const CtcStatIdMap  ctcStatIdMap[CtcStatIdNums] =
    {
    //CtcStatIdDnDropEvents
    {StatIdNumStats, StatIdNumStats}, 
    
    //CtcStatIdUpDropEvents
    {StatIdNumStats, StatIdNumStats},
    
    //CtcStatIdDnBytes
    {StatIdBytesRx, StatIdBytesTx},  
    
    //CtcStatIdUpBytes
    {StatIdBytesTx, StatIdBytesRx},  
    
    //CtcStatIdDnFrames
    {StatIdTotalFramesRx, StatIdTotalFramesTx}, 
    
    //CtcStatIdUpFrames
    {StatIdTotalFramesTx, StatIdTotalFramesRx}, 
    
    //CtcStatIdDnBcastFrames
    {StatIdBcastFramesRx, StatIdBcastFramesTx}, 
    
    //CtcStatIdUpBcastFrames
    {StatIdBcastFramesTx, StatIdBcastFramesRx}, 
    
    //CtcStatIdDnMcastFrames
    {StatIdMcastFramesRx, StatIdMcastFramesTx}, 
    
    //CtcStatIdUpMcastFrames
    {StatIdMcastFramesTx, StatIdMcastFramesRx}, 
    
    //CtcStatIdDnCrcErrFrames
    {StatIdFcsErr, StatIdNumStats},   
    
    //CtcStatIdUpCrcErrFrames
    {StatIdNumStats, StatIdFcsErr}, 
    
    //CtcStatIdDnUndersizeFrames
    {StatIdFrameTooShort, StatIdNumStats}, 
    
    //CtcStatIdUpUndersizeFrames
    {StatIdNumStats, StatIdFrameTooShort},
    
    //CtcStatIdDnOversizeFrames
    {StatIdFrameTooLong, StatIdNumStats}, 
    
    //CtcStatIdUpOversizeFrames
    {StatIdNumStats, StatIdFrameTooLong}, 
    
    //CtcStatIdDnFragments
    {StatIdNumStats, StatIdNumStats}, 
    
    //CtcStatIdUpFragments
    {StatIdNumStats, StatIdNumStats}, 
    
    //CtcStatIdDnJabbers
    {StatIdNumStats, StatIdNumStats}, 
    
    //CtcStatIdUpJabbers
    {StatIdNumStats, StatIdNumStats}, 
    
    //CtcStatIdDn64Bytes    
    {StatIdRx64Bytes, StatIdTx64Bytes},   
    
    //CtcStatIdDn64_127Bytes
    {StatIdRx65_127Bytes, StatIdTx65_127Bytes}, 
    
    //CtcStatIdDn128_255Bytes
    {StatIdRx128_255Bytes, StatIdTx128_255Bytes}, 
    
    //CtcStatIdDn256_511Bytes
    {StatIdRx256_511Bytes, StatIdTx256_511Bytes}, 
    
    //CtcStatIdDn512_1023Bytes
    {StatIdRx512_1023Bytes, StatIdTx512_1023Bytes}, 
    
    //CtcStatIdDn1024_1518Bytes
    {StatIdRx1024_1518Bytes, StatIdTx1024_1518Bytes}, 
    
    //CtcStatIdUp64Bytes
    {StatIdTx64Bytes, StatIdRx64Bytes},  
    
    //CtcStatIdUp64_127Bytes
    {StatIdTx65_127Bytes, StatIdRx65_127Bytes}, 
    
    //CtcStatIdUp128_255Bytes
    {StatIdTx128_255Bytes, StatIdRx128_255Bytes}, 
    
    //CtcStatIdUp256_511Bytes
    {StatIdTx256_511Bytes, StatIdRx256_511Bytes}, 
    
    //CtcStatIdUp512_1023Bytes
    {StatIdTx512_1023Bytes, StatIdRx512_1023Bytes}, 
    
    //CtcStatIdUp1024_1518Bytes
    {StatIdTx1024_1518Bytes, StatIdRx1024_1518Bytes}, 
    
    //CtcStatIdDnDiscards
    {StatIdRxFramesDropped, StatIdTxFramesDropped}, 
    
    //CtcStatIdUpDiscards
    {StatIdTxFramesDropped, StatIdRxFramesDropped}, 
    
    //CtcStatIdDnErrors
    {StatIdErrFrames, StatIdNumStats}, 
    
    //CtcStatIdUpErrors
    {StatIdNumStats, StatIdErrFrames}, 
    
    //CtcStatIdPortChgs
    {StatIdNumStats, StatIdNumStats}
    };


////////////////////////////////////////////////////////////////////////////////
/// Global variable definition
////////////////////////////////////////////////////////////////////////////////

CtcStats  ctcStats;  

U8  CtcPonPerfMonAlmMaptoStat[CtcPonPerfAlmNum] =
    {
    PerfMonDnDropEvents        ,   /* OamCtcAttrDnStreamDropEvent         */  
    PerfMonUpDropEvents        ,   /* OamCtcAttrUpStreamDropEvent         */  
    PerfMonDnCrcErrFrames      ,   /* OamCtcAttrDnStreamCRCErrFrames      */  
    PerfMonUpCrcErrFrames      ,   /* OamCtcAttrUpStreamCRCErrFrames      */  
    PerfMonDnUndersizeFrames   ,   /* OamCtcAttrDnStreamUndersizeFrames   */  
    PerfMonUpUndersizeFrames   ,   /* OamCtcAttrUpStreamUndersizeFrames   */  
    PerfMonDnOversizeFrames    ,   /* OamCtcAttrDnStreamOversizeFrames    */  
    PerfMonUpOversizeFrames    ,   /* OamCtcAttrUpStreamOversizeFrames    */  
    PerfMonDnFragments         ,   /* OamCtcAttrDnStreamFragments         */  
    PerfMonUpFragments         ,   /* OamCtcAttrUpStreamFragments         */  
    PerfMonDnJabbers           ,   /* OamCtcAttrDnStreamJabbers           */  
    PerfMonUpJabbers           ,   /* OamCtcAttrUpStreamJabbers           */  
    PerfMonDnDiscards          ,   /* OamCtcAttrDnStreamDiscards          */  
    PerfMonUpDiscards          ,   /* OamCtcAttrUpStreamDiscards          */  
    PerfMonDnErrors            ,   /* OamCtcAttrDnStreamErrors            */  
    PerfMonUpErrors            ,   /* OamCtcAttrUpStreamErrors            */  
    PerfMonDnDropEvents        ,   /* OamCtcAttrDnStreamDropEventsWarn    */  
    PerfMonUpDropEvents        ,   /* OamCtcAttrUpStreamDropEventsWarn    */  
    PerfMonDnCrcErrFrames      ,   /* OamCtcAttrDnStreamCRCErrFramesWarn  */  
    PerfMonUpCrcErrFrames      ,   /* OamCtcAttrUpStreamCRCErrFramesWarn  */  
    PerfMonDnUndersizeFrames   ,   /* OamCtcAttrDnStreamUndersizeFrmsWarn */  
    PerfMonUpUndersizeFrames   ,   /* OamCtcAttrUpStreamUndersizeFrmsWarn */  
    PerfMonDnOversizeFrames    ,   /* OamCtcAttrDnStreamOversizeFrmsWarn  */  
    PerfMonUpOversizeFrames    ,   /* OamCtcAttrUpStreamOversizeFrmsWarn  */  
    PerfMonDnFragments         ,   /* OamCtcAttrDnStreamFragmentsWarn     */  
    PerfMonUpFragments         ,   /* OamCtcAttrUpStreamFragmentsWarn     */  
    PerfMonDnJabbers           ,   /* OamCtcAttrDnStreamJabbersWarn       */  
    PerfMonUpJabbers           ,   /* OamCtcAttrUpStreamJabbersWarn       */  
    PerfMonDnDiscards          ,   /* OamCtcAttrDnStreamDiscardsWarn      */  
    PerfMonUpDiscards          ,   /* OamCtcAttrUpStreamDiscardsWarn      */  
    PerfMonDnErrors            ,   /* OamCtcAttrDnStreamErrorsWarn        */  
    PerfMonUpErrors                /* OamCtcAttrUpStreamErrorsWarn        */  
    };


U8 CtcStatsEventCmtPort;//which port need to commit event . one bit for one port


////////////////////////////////////////////////////////////////////////////////
/// CtcPerformAlarmChk - check the CTC30 performance monitoring alarm
/// 
/// Parameters:
/// \param obj       alarm object
/// \param port      Ethernet port number from "0" to "TkOnuEthportNum-1"
/// 
/// \return: None
////////////////////////////////////////////////////////////////////////////////
static
void CtcStatsAlarmChk(void)
    {
    U8  i;
    CtcAlmMonThd *pAlmThd;
    OamCtcAlarmId almId;
    U64  tmpData;
    
    /* If Performance Monitoring is enabled on the PON, the alarm will be detected */
    if (TRUE == CtcStatsGetEnable())
        {			
        for (i = 0; i < CtcPonPerfAlmNum; i++)
            {
            almId = i + OamCtcAttrDnStreamDropEvent;
            pAlmThd = &ctc30AlmThd[almId - OamCtcPonIfAlarmEnd];
            CtcStatsGetStats((CtcStatId)CtcPonPerfMonAlmMaptoStat[i],&tmpData);
            CtcPerfMonAlmNotify(pAlmThd, &tmpData, almId);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetEnable:  Get CTC stat enable state
///
 // Parameters:
/// \param port    port number
///
/// \return
/// TRUE if stat is enabled, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
BOOL CtcStatsGetEnable(void)
    {
    return ctcStats.enable;
    }//CtcStatsGetEnable


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
void CtcStatsSetEnable(BOOL enable)
    {
    ctcStats.enable = enable;	
    }//CtcStatsSetEnable


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsGetPeriod:  Get CTC stat period
///
 // Parameters:
/// \param port    port number
///
/// \return
/// Stats period
////////////////////////////////////////////////////////////////////////////////
U32 CtcStatsGetPeriod(void)
    {
    return ctcStats.period;
    }//CtcStatsGetPeriod


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
void CtcStatsSetPeriod(U32 period)
    {
    ctcStats.period = period;
    }//CtcStatsSetPeriod


BOOL CtcStatsStatsVaild(CtcStatId id)
    {
    if (id <=  PerfMonStateButt)
        {
        return TRUE;
        }
    return FALSE;
    }


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
void CtcStatsGetStats(CtcStatId id, U64  * dst)
    {
    StatId stat;

    memset (dst, 0x0, sizeof (U64));    
    stat = ctcStatIdMap[id].eponStat;

    if (stat != StatIdNumStats)
        {
        StatsGetEpon (stat, dst);
        }
    }//CtcStatsGetStats


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsClear:  Clear CTC stat
///
 // Parameters:
/// \param port    port number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void CtcStatsClear(void)
    {
    ctcStats.time = 0;  
    StatsClearEpon ();
    }//CtcStatsClear


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
void CtcStatsGetHisStats(CtcStatId id, U64  * dst)
    {
    if (ctcStats.enable)
        {
        memcpy(dst, &ctcStats.stats[id], sizeof(U64));
        }
    }//CtcStatsGetHisStats





////////////////////////////////////////////////////////////////////////////////
/// CtcStats1sTimer:  CTC Statistics 1s timer
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
// extern
static
void CtcStats1sTimer (EponTimerModuleId moduleId)
    {
    CtcStatId FAST id;
    if (ctcStats.enable)
        {
        if (++(ctcStats.time) >= ctcStats.period)
            {
            // Store the history stats
            for (id = CtcStatIdFirst; id < CtcStatIdNums; id++)
                {                    
                CtcStatsGetStats (id, &ctcStats.stats[id]);
                }
            CtcStatsClear ();
            }		
        CtcStatsAlarmChk();		
        }
	
    CtcAlmPoll();
    } // CtcStats1sTimer


////////////////////////////////////////////////////////////////////////////////
/// CtcStatsInit:  CTC Statistics Init
///
 // Parameters:
/// \param  None
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
// extern
void CtcStatsInit (void)  
    {
    memset(&ctcStats,0,sizeof(CtcStats));
    ctcStats.enable = FALSE;
    ctcStats.period = CtcStatsDefPeriod;	
    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
    	                  CtcStatsTimer,CtcStats1sTimer);
    }// CtcStatsInit


// end CtcStats.c

