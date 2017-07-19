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
/// \file EponCmds.c
/// \brief Tk4701 Epon block commands
///
/// Provides basic access functions for the TK4701 Epon block.
///
////////////////////////////////////////////////////////////////////////////////
#include <linux/string.h>
#include "EponCmds.h"
#include "OntDirector.h"
#include "drv_epon_epn_ag.h"

#define EpnMainInrpNum 32
#define EpnMaxAccumNum 32
#define XifInrpNum 7
#define XpcTxInrpNum 7
static U32 EpnL1AccIndex = 0xFFFFFFFF;
char const * const  StatStr[StatStrNumStrings] =
    {
    "64",
    "0-127",
    "65-127",
    "128-255",
    "256-383",
    "384-511",
    "512-639",
    "640-767",
    "768-895",
    "896-1023",
    "1024-1151",
    "1152-1279",
    "1280-1407",
    "1408-1535",
    "1536-1663",
    "1664-1791",
    "1792-1919",
    "1920-2047",
    "64-511",
    "256-511",
    "512-1023",
    "1024-1518",
    "1519-2047",
    "2048-4095",
    "4096-9216",
    "9216+",   
    "64b66bDecErr",
    "AlignErr",
    "BcastFrmRx",
    "BcastFrmTx",
    "ByteDlyd",
    "ByteDrop",
    "ByteRx",
    "ByteTx",
    "CodeErr",
    "Col 1",
    "Col 2",
    "Col 3",
    "Col 4",
    "Col 5",
    "Col 6",
    "Col 7",
    "Col 8",
    "Col 9",
    "Col 10",
    "Col 11",
    "Col 12-15",
    "ColExc",
    "ColLate",
    "ColTot",
    "Crc32Err",
    "Crc8Err",
    "ErrByte",
    "FcsErr",
    "FecBlock",
    "FecByte",
    "FecCorBlk",
    "FecCorByte",
    "FecCorOne",
    "FecCorZero",
    "FecDecErrCor",
    "FecDecFail",
    "FecDecPass",
    "FecExceedErr",
    "FecFrm",
    "FecUnCorBlk",
    "Frm",
    "FrmRx",
    "FrmTx",
    "FrmBadSh",
    "FrmDrop",
    "GateRx",
    "GenCrc32",
    "LineCodeErr",
    "MaxLenErr",
    "MaxPktDly",
    "McastFrmRx",
    "McastFrmTx",
    "MpcpByte",
    "MpcpFrm",
    "NonFecBytes",
    "NonFecFrm",
    "OamBytesRx",
    "OamBytesTx",
    "OamFrm",
    "OamFrmRx",
    "OamFrmTx",
    "Oversized",
    "PauseRx",
    "PauseTx",
    "RegFrmRx",
    "Reports",
    "SecGoodByte",
    "SecGoodFrm",
    "SecRxBadFrm",
    "TestPrbsErr",
    "TestPseudoErr",
    "UcastFrmRx",
    "UcastFrmTx",
    "Undersized",
    "UnusedWordsTx",
    "BufUnd",
    "RxAbrt"
    };

char const * const  EpnMainInrpStr[EpnMainInrpNum] =
    {
    "intBbhUpFrAbort",
    "intCol2sBurstCapOverFlowPres",
    "intCoEmptyRpt",
    "intCoDrxErrAbortPres",
    "intL2sFifoOverRun",
    "intCo1588TsInt",
    "intCoRptPres",   
    "intCoGntPres",
    "intCoDelStaleGnt",
    "intCoGntNonPoll",
    "intCoGntMisalign",
    "intCoGntTooFar",
    "intCoGntInterval",
    "intCoGntDiscovery",
    "intCoGntMissAbort",
    "intCoGntFullAbort",
    "intBadUpFrLen",
    "intUpTardyPacket",
    "intUpRptFrXmt",
    "intBiFifoOverRun",
    "intBurstGntTooBig",
    "intWrGntTooBig",
    "intRcvGntTooBig",
    "intDnStatsOverRun",
    "intUpStatsOverRun",
    "intDnOutOfOrder",
    "intTruantBbhHalt",
    "intUpInvldGntLen",
    "intCoBbhUpsFault",
    "intDnTimeInSync",
    "intDnTimeNotInSync",
    "intDportRdy"  
    };

char const * const  XifInrpStr[XifInrpNum] =
{
    "secRxOutFfOvrflwInt",
    "pmcTsJttrInt",
    "negTimeInt",
    "txHangInt",
    "tsFullUpdInt",
    "secTxPktNumMaxInt",
    "secRxRplyPrtctAbrtInt"
};

char const * const  XpcTxInrpStr[XpcTxInrpNum] =
{
    "gntTooShort",
    "gearBoxUnderrun",
    "fecUnderrun",
    "back2backGnt",
    "grantLagErr",
    "laserOff",
    "laserOnMax"
};

const StatStrId  EponLinkStatStrTab[] =
    {
//bidirectional stats
    StatStrByteRx,
    StatStrFcsErr,
    StatStrOamFrmRx,
    StatStrGateRx,
    StatStr64,
    StatStr65_127,
    StatStr128_255,
    StatStr256_511,
    StatStr512_1023,
    StatStr1024_1518,
    StatStr1519Plus,
    StatStr2048_4095,
    StatStr4096_9216,
    StatStr9216Plus,    
    StatStrOversized,
    StatStrBcastFrmRx,
    StatStrMcastFrmRx,
    StatStrUcastFrmRx,
    StatStrUndersized,
    StatStrOamBytesRx,
    StatStrRegFrmRx,

//downstream only stats
    StatStrByteRx,
    StatStrFcsErr,
    StatStrOamFrmRx,
    StatStrGateRx,
    StatStrBcastFrmRx,
    StatStrMcastFrmRx,
    StatStrUcastFrmRx,
    StatStr64_511,
    StatStr512_1023,
    StatStr1024_1518,
    StatStr1519Plus,
    StatStrOversized,
    StatStrUndersized,
    StatStrOamBytesRx,

//upstream only stats
    StatStrByteTx,
    StatStrUnusedWordsTx,
    StatStrOamFrmTx,
    StatStrReports,
    StatStr64,
    StatStr65_127,
    StatStr128_255,
    StatStr256_511,
    StatStr512_1023,
    StatStr1024_1518,
    StatStr1519Plus,
    StatStr2048_4095,
    StatStr4096_9216,
    StatStr9216Plus,  
    StatStrOamFrmTx,
    StatStrBcastFrmTx,
    StatStrMcastFrmTx,
    StatStrUcastFrmTx
    };

const StatStrId  EponStatStrTab[] =
    {
    StatStrOversized,
    StatStrFrm,
    StatStrFcsErr,
    StatStrGateRx,
    StatStrOamFrm,
    StatStrUndersized
    };


////////////////////////////////////////////////////////////////////////////////
/// DisplayLinkStats: Display EPON link stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void DisplayLinkStats (U8 linkStart, U8 linkEnd)
    {
    EponLinkStatId  stat;
    LinkIndex  link;
    for (link = linkStart; link < linkEnd; ++link)
        {
        printk("\nLink 0x%0x ( 0x%x ) \n", link, PonMgrGetPhyLlid (link) );
        for (   stat = EponBiDnTotalBytesRx;
            stat <= EponBiDnRegisterFramesRx;
            ++stat)
            {
            U64 upbuf;
            U64 dnbuf;
            EponReadLinkStat(link, stat, &dnbuf);

            if (((EponBiUpTotalBytesTx + stat)) <= EponBiUpUnicastFramesTx)
                {
                EponReadLinkStat (link,
                                  EponBiUpTotalBytesTx + stat,
                                  &upbuf);
                printk ( "%25s: %10lld %25s: %10lld\n",
                    StatStr[EponLinkStatStrTab[stat+EponBiUpTotalBytesTx]],
                    upbuf,
                    StatStr[EponLinkStatStrTab[stat]],
                    dnbuf
                    );
                }
            else
                {
                printk ("%26s %10s %25s: %10lld\n",
                "","",
                StatStr[EponLinkStatStrTab[stat]],
                dnbuf);
                }
            }
        printk ("\n");
        }
    } // DisplayLinkStats



////////////////////////////////////////////////////////////////////////////////
/// DisplayNonLinkStats: Display EPON non-link stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void DisplayNonLinkStats(void)
    {
    U8  stat;
    printk("unmapped stats:\n");
    //general EPON stats
    for(stat = 0; stat <= EponUnmappedLlidSmallFrameCount; ++stat)
        {
        printk("%15s: 0x%0x \n",
            StatStr[EponStatStrTab[stat]],
            EponReadStat(stat)
            );
        }
    } // DisplayNonLinkStats

////////////////////////////////////////////////////////////////////////////////
/// DisplayL1AccInfo: Signed number of bytes in the selected L1S Shaped Virtual Accumulator
///
/// input: None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void DisplayL1AccInfo(void)
    {
    U32 l1svasize[EpnMaxAccumNum] = {0};
    U32 l1uvasize[EpnMaxAccumNum] = {0};
    U8 i = 0;

    printk("EPn index: 0x%08x\n",EpnL1AccIndex);
    for (i = 0; i < EpnMaxAccumNum; i++)
        {
        if (TestBitsSet(EpnL1AccIndex, 1 << i))
            {
            EponL1AccSel(i,i);
            EponL1AccBytesGet(&l1svasize[i],&l1uvasize[i]);
            }
        }

    for (i = 0; i < EpnMaxAccumNum; i++)
        {
        if (TestBitsSet(EpnL1AccIndex, 1 << i))
            {            
            printk("\nvirtual accumulator[%d] was selected to report:\n",i);
            printk("\n");
            printk("%25s:0x%x\n","shaped Virtual bytes",l1svasize[i]);
            printk("%25s:%d\n","actual number of bytes",l1svasize[i]&0x0FFFFFFF);
            printk("%25s:%d\n","overflow indication",(l1svasize[i]>>28)&0x00000001);
            printk("%25s:%d\n","Runner/BBH created a rounding error",(l1svasize[i]>>29)&0x00000001);
            printk("\n");
            printk("%25s:0x%x\n","Un-shaped Virtual bytes",l1uvasize[i]);
            printk("%25s:%d\n","actual number of bytes",l1uvasize[i]&0x0FFFFFFF);
            printk("%25s:%d\n","overflow indication",(l1uvasize[i]>>28)&0x00000001);
            printk("%25s:%d\n","Runner/BBH created a rounding error",(l1uvasize[i]>>29)&0x00000001);
            printk("\n\n");
            }
        }
    } // DisplayL1AccInfo


////////////////////////////////////////////////////////////////////////////////
/// DisplayL1AccInfo: Signed number of bytes in the selected L1S Shaped Virtual Accumulator
///
/// input: None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void DisplayEpnInrptInfo(void)
    {
    epn_main_int_status epnMainInrpStatus;
    xif_int_status xifInrpStatus;
    xpcstx_tx_int_stat xpcsTxInrpStatus;
    U8 l1sDquQueEmpty = 0,l1sUnshapedQueEmpty = 0,l2QueEmpty = 0,l2QueFull = 0,l2QueStopped = 0;
    U8 queueIdx = 0;
    U32 l1shp = 0, l1unshp = 0, l2Empty = 0, l2Full = 0, l2Stop = 0;
    U32 l1shpOverflow = 0, l1unshpOverflow = 0;
    char EpnMainInrpStatus[EpnMainInrpNum] = {0};
    char XifInrpStatus[XifInrpNum] = {0};
    char XpcsTxInrpStatus[XpcTxInrpNum] = {0};
    int i = 0;
    
    memset(&epnMainInrpStatus,0,sizeof(epn_main_int_status));
    memset(&xifInrpStatus,0,sizeof(xif_int_status)); 
    memset(&xpcsTxInrpStatus,0,sizeof(xpcstx_tx_int_stat)); 

    EponMainInrpStatusGet(&epnMainInrpStatus);
    EponTxL1QueOverflowStatusGet(&l1shpOverflow, &l1unshpOverflow);
    XifInrpStatusGet(&xifInrpStatus);
    XifXpcsInrpStatusGet(&xpcsTxInrpStatus);

    for (queueIdx = 0; queueIdx < EpnMaxAccumNum; queueIdx++)
        {
        EponTxL1QueStatusGet(queueIdx,&l1sDquQueEmpty,&l1sUnshapedQueEmpty);
        EponTxL2QueStatusGet(queueIdx,&l2QueEmpty,&l2QueFull,&l2QueStopped);
        l1shp |= (((U32)l1sDquQueEmpty)<<queueIdx);
        l1unshp |= (((U32)l1sUnshapedQueEmpty)<<queueIdx);
        l2Empty |= (((U32)l2QueEmpty)<<queueIdx);
        l2Full |= (((U32)l2QueFull)<<queueIdx);            
        l2Stop |= (((U32)l2QueStopped)<<queueIdx);            
        }

    memcpy(EpnMainInrpStatus,&epnMainInrpStatus,EpnMainInrpNum);
    memcpy(XifInrpStatus,&xifInrpStatus,XifInrpNum);
    memcpy(XpcsTxInrpStatus,&xpcsTxInrpStatus,XpcTxInrpNum);

    printk("\n Main Interrupt status for the EPON module:\n");
    for(i=0;i<EpnMainInrpNum;i++)
        {
        printk("%25s: %d       ",EpnMainInrpStr[i],EpnMainInrpStatus[i]);
        if(((i+1)%2)==0)
            {
            printk("\n"); 
            }
        }
    
    printk("\n\n");
    printk("\nL1 shaped/unshaped queues Empty status:\n");
    printk("%25s:0x%08x       %25s:0x%08x\n","l1sDquQueEmpty",l1shp,"l1sUnshapedQueEmpty",l1unshp);    
    printk("\n\n");

    printk("\nL1 shaped/unshaped queues overflow status:\n");
    printk("%25s:0x%08x       %25s:0x%08x\n","l1shpOverflow",l1shpOverflow,"l1unshpOverflow",l1unshpOverflow);    
    printk("\n\n");
    
    printk("\nL2 queues Empty/Full/Stopped status:\n");
    printk("%25s:0x%08x       %25s:0x%08x\n%25s:0x%08x\n","l2sQueEmpty",l2Empty,"l2sQueFull",l2Full,"l2sStoppedQueues",l2Stop); 
    printk("\n\n");


    printk("\nInterrupt status for the Xif module:\n");
    for(i=0;i<XifInrpNum;i++)
        {
        printk("%25s: %d       ",XifInrpStr[i],XifInrpStatus[i]);
        if(((i+1)%2)==0)
            {
            printk("\n"); 
            }
        }
    printk("\n\n");
    
    printk("\nInterrupt status for the Xpcs Tx module:\n");
    for(i=0;i<XpcTxInrpNum;i++)
        {
        printk("%25s: %d       ",XpcTxInrpStr[i],XpcsTxInrpStatus[i]);
        if(((i+1)%2)==0)
            {
            printk("\n"); 
            }    
        }
    printk("\n\n");
        
    } // DisplayEpnInrptInfo

////////////////////////////////////////////////////////////////////////////////
/// CmdLinkStat: Display EPON link stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void CmdLinkStat (U8 linkid)
    {
    U8 link;
    U8 linkStart = TkOnuFirstTxLlid;
    U8 linkEnd = TkOnuFirstRxOnlyLlid+TkOnuNumRxOnlyLlids;

    if (linkid < linkEnd)
        {
        linkStart = linkid;
        linkEnd  = (linkid+1);
        }

    for(link = linkStart; link < linkEnd; ++link)
        {
        if(link < TkOnuNumRxLlids)
            {
            DisplayLinkStats(link,link+1);
            }
        else
            {
            
            }
        }
    } // CmdLinkStat

////////////////////////////////////////////////////////////////////////////////
/// CmdL1AccSel: selects which virtual accumulator sizes are reported
///
/// input: The L1 index
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void CmdL1AccSel (U32 index)
    {
    EpnL1AccIndex = index;
    } // CmdL1AccSel

////////////////////////////////////////////////////////////////////////////////
/// CmdL1AccSel: show Epon MAc L1 info 
///
/// input: None
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void CmdEponEpnInfoShow (U8 eponInfoId)
    {
        if(eponInfoId == EpnInfoL1Cnt)
        {
            DisplayL1AccInfo();
        }
        else if(eponInfoId == EpnInfoInterrupt)
        {
            DisplayEpnInrptInfo();
        }
    } // CmdL1AccSel

////////////////////////////////////////////////////////////////////////////////
/// CmdEponStats: Display all EPON stats
///
/// \param argc Argument count
/// \param argv Argument values
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void CmdEponStats (void)
    {
    DisplayNonLinkStats ();
    
    DisplayLinkStats (TkOnuFirstTxLlid, TkOnuNumRxLlids);
    } // CmdEponStats


// end EponCmds.c
