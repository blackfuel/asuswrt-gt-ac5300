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



//##############################################################################
// Local Type Definitions
//##############################################################################
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/irq.h>

#include "EponMac.h"
#include "EpnRegs.h"
#include "bcm_epon_cfg.h"

typedef enum EpnQueueType  // Used to leverage L1 and L2 initialization code
{
    EpnQueueLevel1,
    EpnQueueLevel2
} EpnQueueType;

typedef U8 LxIndex; // Same purpose

typedef  enum
    {
    MultiPriAllZero,
    MultiPri3,
    MultiPri4,
    MultiPri8,
    MultiPriModeCount
    } MultiPriMode;  // Use to convert from number of priority to Priority mode


//##############################################################################
// Constant Definitions
//##############################################################################
#define EndOfBurst10G                  2
#define DnStatsRamBlockSize           32
#define DnRxOnlyStatsRamOffset        18
#define UpStatsRamBlockSize           16
//below is the best value for 'ReportLengths1G' according to exprience
#define ReportLengths1G     0x142A390AUL 
#define ReportLengths10G    0x1405050AUL // [7:0] not used in 10G but we
#define ReportLengths10GFec 0x000D0D0AUL // set it to 0x0A to be safe
#define ReportByteLen       0x00000054UL // 84 Bytes
#define DiscoverySizeFrame       (42+16) // 42TQ + FEC overhead
#define PercentToWeight     (EpnTxCtcBurstLimitPrv0Msk / 100UL)
#define RoundRobin          1UL
#define PreambleLengthTq               4  //in TQ
#define TimeStampDiff             0x40UL  //in TQ - Trigger for Holdover
#define GrantStartTimeDelta      0x3e8UL  //in TQ//Eddie comment.
#define GrantStartTimeMargin     0x3ffUL  //in TQ//Eddie comment.
#define MisalignThreshold              2  //in TQ
#define MisalignPause                300  //in TQ - just over loop time
#define EpnL2RoundRobin         ((U16)-1)
#define EpnL2StrictPri                0U
#define EpnMinBcapValue          0x400UL  // Guarantee a max frame size


//##############################################################################
// Local Variable Definitions
//##############################################################################

static BOOL iopPreDraft2Dot1;
static BOOL iopNttReporting;
static  U32 reportLengthNoFec;
static  U32 reportLengthFec;
static   U8 endOfBurst;

static  DEFINE_SPINLOCK(epon_spinlock);

//##############################################################################
// Function Definitions
//##############################################################################

void VpbReset (U8 id,BOOL en)
    {
    if (en)
        {
        OnuRegAndEqNot(&VpbCfg,(1Ul<<id));    	
        }
    else
        {
        OnuRegOrEq(&VpbCfg,(1Ul<<id));
        }
    } // VpbReset

void EponTopInit(void)
    {	
    VpbReset(VpbLifR,TRUE);
    VpbReset(VpbEpnR,TRUE);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief  Returns MAC address of the given link
///
/// \param link     Link to query
/// \param mac      Pointer to return MAC addr to get
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponGetMac (LinkIndex link, MacAddr* mac)
    {
    // Each Mac addr takes 2 entries in the table. Hence the x2
    mac->lowHi.low = OnuRegTableRead (EpnOnuMacAddr0, link <<= 1);
    mac->lowHi.hi = (U16)OnuRegTableRead (EpnOnuMacAddr0, link + 1);
    } // EponGetMac


////////////////////////////////////////////////////////////////////////////////
/// \brief  Sets MAC address of the given link
///
/// \param link     Link to change
/// \param mac      Pointer to MAC addr to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetMac (LinkIndex link, const MacAddr* mac)
    {
    // Each Mac addr takes 2 entries in the table. Hence the x2
    OnuRegTableWrite (EpnOnuMacAddr0, link <<= 1, mac->lowHi.low);
    OnuRegTableWrite (EpnOnuMacAddr0, link + 1, mac->lowHi.hi);
    } // EponSetMac


////////////////////////////////////////////////////////////////////////////////
/// \brief  Sets MAC address for OLT
///
/// \param mac  Address to provision for OLT
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetOltMac (const MacAddr* mac)
    {
    OnuRegTableWrite (EpnOltMacAddr, 0, mac->lowHi.low);
    OnuRegTableWrite (EpnOltMacAddr, 1, mac->lowHi.hi);
    } // EponSetOltMac


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set burst cap for l2
///
/// The burst capacity registers limit the size of a Report FIFO. This creates
/// an upper limit on the queue size indicated in the report frames.
///
/// \param l2   L2 index to set the burst cap
/// \param cap  Max bytes per burst on this link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetBurstCap (L2Index l2, U32 cap)
    {
    // A0 and A1 don't support below MTU BCap values
    if (cap < EpnMinBcapValue)
        {
        cap = EpnMinBcapValue;
        }

    OnuRegTableWrite (EpnBurstCap, l2, cap);
    } // EponSetBurstCap


////////////////////////////////////////////////////////////////////////////////
/// \brief Set CTC burst limit for L2 queue
///
/// The burst limit registers set the maximum number of bytes that an L2 queue
/// can transmit in any given round. A round ends when all L2s queues have
/// reached their respective burst limit or there is no more data to transmit.
/// Note that setting a burst limit to zero enables the respective L2s queue to
/// transmit in strict priority. Also, any burst limits that are set to 1 will
/// cause those L2Map queues to transmit in "round-robin" fashion.
///
/// \param l2     Link for new burst cap
/// \param pct    Percent weight for this queue (0 for SP, -1 for RR)
/// \param base The smallest weight in use for any priority on the link.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetBurstLimit(L2Index l2, U16 pct, U16 base)
    {
    U32  limit;

    switch (pct)
        {
        case EpnL2RoundRobin:
            limit = RoundRobin;
            break;
        case EpnL2StrictPri:
            limit = 0;
            break;
        default:
            limit = (PercentToWeight * pct) / base;
            break;
        }
    //OnuRegWrite (&EpnRegBankSel, EpnRegBankTxL2Q);
    OnuRegTableWrite (EpnTxCtcBurstLimit, l2, limit);
    } // EponSetBurstLimit


////////////////////////////////////////////////////////////////////////////////
/// \brief  Sets idle time for EPON
///
/// From the beginning a grant window, the ONT must turn on the laser, wait
/// for it to come on (idling), then continue to idle while waiting for the
/// OLT AGC time, then continue to idle while waiting for the OLT CDR time,
/// and then idle 600ns per IEEE802.3ah.  This routine sets the total idle
/// time from the end of the laser on period.
///
/// \param  front    Should correspond to Lon + Preamble
/// \param  back     Should correspond to Loff
/// \param  idleType It can be a discovery or non discovery idle time
///
/// \return
/// Actual grant overhead provisionned in hardware
////////////////////////////////////////////////////////////////////////////////
//extern
U16 EponSetIdleTime (U16 front, U16 back, IdleTimeType idleType)
    {
    // Version 2.1 of the 802.1ah introduced a cleaner separation between MCPCP,
    // MAC and PHY layers. It makes that at MPCP level, the start grant time
    // correspond to the departure of the first byte of the DA. So at the MAC
    // layer (line interface), it correspond to the first byte of the preamble.
    // The data detector introduces a delay equal to Lon+syncTime so that
    // Grant Start Time correspond to begining of the burst at PHY level.
    // So in normal operation (draft 2.1 and later), Epon should stamp the MPCP
    // frames with grant time + Lon + sync Time + preamble. But if we want to be
    // pre 2.1 compliant we need to stamp them with grant time + preamble.
    U32 upTimeStampOff = PreambleLengthTq + (iopPreDraft2Dot1 ? 0: front);
	
    OnuRegWrite (&EpnUpTimeStampOff,
        upTimeStampOff | (upTimeStampOff << EpnUpTimeStampOffsetFecSft));
	
    // Reuse "front" to save space. Front now means total overhead
    // Adding potential endOfBurst (had been set to zero when in 1G up)
    front += back + endOfBurst;

    if (iopNttReporting) // This is to simulate what a Passave ONU does
        {                // It's Passave's special an undocumented magic 16
        front += 16;     // extra TQ. This is to cover a bug in the Passave OLT
        }

    // Is it discovery idle time or non-discovery idle time?
    OnuRegWrite ((idleType == DiscIdleTime) ? &EpnDiscGntOvrhd : &EpnGntOvrhd,
                 (idleType == DiscIdleTime)? 
                 front :(front | (front << EpnGntOvrdFecSft)));

    // Compute discovery frame size.
    if (idleType == DiscIdleTime)
        {
        OnuRegWrite (&EpnDnDiscSize, front + DiscoverySizeFrame);
        }

    return front;
    } // EponSetIdleTime


////////////////////////////////////////////////////////////////////////////////
/// \brief delay from start of window to transmit
///
/// Usually, transmission begins when a grant window starts.  For replies
/// to discovery gates, though, a random delay needs to be added.
///
/// \param offset   Time delay, in 16-bit-time increments
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetGateOffset (U16 offset)
    {
    OnuRegWrite (&EpnDnDiscSeed, (U32)offset);
    } // EponSetGateOffset


////////////////////////////////////////////////////////////////////////////////
/// \brief Provision L1 or L2 Event Queue
///
/// L1s and L2s are provisionned by setting a start and end address. To keep it
/// simple at the driver level, we let the caller handle the allocation: given
/// a start address we allocate the queue with the correct size and return the
/// next available start address.
///
/// \param  level         are we talking L1 or L2 queues.
/// \param  lx            Indicates the queue we are talking about (0 based)
/// \param  sizeInFrames  Max number of frames in the queue
/// \param  startAddress  What is the current end address in the table
///
/// \return
/// next available address that can be used as start address for next l1 or l2
////////////////////////////////////////////////////////////////////////////////
static
U16 SetLxEventQueue (EpnQueueType level,
                     LxIndex lx,
                     U32 sizeInFrames,
                     U16 startAddress)
    {
    U16  endAddress = startAddress;


    // The granularity of the register is 4 frames, hence the shift operation.
    // Note that start and end are included
    if (sizeInFrames != 0)
        {
        endAddress += (U16)(sizeInFrames >> 2);
		if (level == EpnQueueLevel1)
			{
        	OnuRegTableWrite (EpnTxL1sQCfg, lx,
                          TkPutField(EpnTxL1sQCfgQStart0, startAddress) |
                          TkPutField(EpnTxL1sQCfgQEnd0, endAddress));
			}
		else
			{
			OnuRegTableWrite (EpnTxL2sQCfg, lx,
                          TkPutField(EpnTxL1sQCfgQStart0, startAddress) |
                          TkPutField(EpnTxL1sQCfgQEnd0, endAddress));
			}
        endAddress++; // next available address is one after this end
        }
    else
        {
        if (level == EpnQueueLevel1)
        	{
        	OnuRegTableWrite (EpnTxL1sQCfg, lx, 0UL);
        	}
		else
			{
			OnuRegTableWrite (EpnTxL2sQCfg, lx, 0UL);
			}
        }
    return endAddress;
    } // SetLxEventQueue


////////////////////////////////////////////////////////////////////////////////
/// \brief provision L1 Event Queue
///
/// See description to the function this one is calling and apply it to L1
///
/// \param  l1            Indicates the l1 queue we are talking about (0 based)
/// \param  sizeInFrames  Max number of frames in the queue
/// \param  startAddress  What is the current end address in the table
///
/// \return
/// next available address that can be used as start address for next l1
////////////////////////////////////////////////////////////////////////////////
//extern
U16 EponSetL1EventQueue (L1Index l1, U32 sizeInFrames, U16 startAddress)
    {
    return SetLxEventQueue(EpnQueueLevel1, l1, sizeInFrames, startAddress);
    } // EponSetL1EventQueue


////////////////////////////////////////////////////////////////////////////////
/// \brief provision L1 Event Queue
///
/// See description to the function this one is calling and apply it to L1
///
/// \param  l2            Indicates the l2 queue we are talking about (0 based)
/// \param  sizeInFrames  Max number of frames in the queue
/// \param  startAddress  What is the current end address in the table
///
/// \return
/// next available address that can be used as start address for next l1
////////////////////////////////////////////////////////////////////////////////
//extern
U16 EponSetL2EventQueue (L2Index l2, U32 sizeInFrames, U16 startAddress)
    {
    return SetLxEventQueue(EpnQueueLevel2, l2, sizeInFrames, startAddress);
    } // EponSetL2EventQueue


////////////////////////////////////////////////////////////////////////////////
/// \brief Map an L1 queue to an L2 queue
///
/// \param l1   L1 queue
/// \param l2   L2 queue
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponMapL1ToL2 (L1Index l1, L2Index l2)
    {
    OnuRegTableWrite (EpnQLlidMap, l1, TkPutField(EpnQLlidMapQ0, l2));
    } // EponMapL1ToL2


////////////////////////////////////////////////////////////////////////////////
/// \brief Data port wait
///
/// None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void DpWait (void)
    {
    int timeout = 0x30;
    //wait for busy bit to clear
    while (((OnuRegRead(&EpnDpCmd)&EpnDpCmdNotIntRdy) != 0) &&
		   (timeout--))
        {
        ;
        }
    } // DpWait


////////////////////////////////////////////////////////////////////////////////
/// \brief Flush the Grant FIFO for some links
///
///
/// \param linkMap  Map of links to flush grant FIFO
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponFlushGrantFifo (LinkMap linkMap)
    {
    EponGrantReset(linkMap);
    EponGrantNotReset(linkMap);
    } // EponFlushGrantFifo

////////////////////////////////////////////////////////////////////////////////
/// \brief Flush the Grant FIFO for some links
///
///
/// \param linkMap  Map of links to flush grant FIFO
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponFlushL2Fifo (U32 l2map)
    {
    U8 i;
    U16 cnt;
	
    for (i = 0;i < EponNumL2Queues;i++)
        {
        if (TestBitsSet(l2map,(1<<i)))
            {
            cnt = 0;
            OnuRegWrite(&EpnL2FlushCmd, (i&EpnL2FlushQueueMsk));
			OnuRegOrEq(&EpnL2FlushCmd,EpnL2FlushQueueEnable);

            while(!TestBitsSet(OnuRegRead(&EpnL2FlushCmd),EpnL2FlushQueueDone))//FLush Done
                {
                cnt++;
                DelayUs(50);
                
                if(cnt >1000)
                    {
                    printk("flush not done \n");
                    break;
                    }
                }
			
            cnt = 0;
            OnuRegAndEqNot(&EpnL2FlushCmd,EpnL2FlushQueueEnable);//Flush stop
            while(TestBitsSet(OnuRegRead(&EpnL2FlushCmd),EpnL2FlushQueueDone))
                {
                cnt++;
                if(cnt>10)
                    {
                    cnt = 0;
                    printk("flush not recover \n");
                    break;
                    }
                }
            }
        }
    } // EponFlushGrantFifo

////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantReset(LinkMap linkMap)
    {
    OnuRegOrEq(&EpnRstGntQ, linkMap);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantNotReset(LinkMap linkMap)
    {
    OnuRegAndEqNot(&EpnRstGntQ, linkMap);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantEnable(LinkMap linkMap)
    {
    OnuRegOrEq(&EpnEnGnt, linkMap);
    }



////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantDisable(LinkMap linkMap)
    {
    OnuRegAndEqNot(&EpnEnGnt, linkMap);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponUpstreamDisable (LinkMap linkMap)
    {
    U8 cnt = 0;
    OnuRegAndEqNot(&EpnEnUpstream, linkMap);
	while(0 != (OnuRegRead(&EpnEnUpstream) ^ OnuRegRead(&EpnEnUpstreamFb)))
		{
		cnt++;
		if(cnt >=50)
			{
			printk("upstream epn disable time expire \n");
			break;
			}
		}
    } // EponUpstreamDisable


////////////////////////////////////////////////////////////////////////////////
/// \brief Enables "normal" reporting / traffic send on a link.
/// note this operation waits on the feedback register.
///
/// \param linkMap  Bitmap of links to enable
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponUpstreamEnable (LinkMap linkMap)
    {
    U8 cnt = 0;
    OnuRegOrEq (&EpnEnUpstream, linkMap);
	while(0 != (OnuRegRead(&EpnEnUpstream) ^ OnuRegRead(&EpnEnUpstreamFb)))
		{
		cnt++;
		if(cnt >=50)
			{
			printk("upstream epn enable time expire \n");
			break;
			}
		}
    } // EponUpstreamEnable


////////////////////////////////////////////////////////////////////////////////
/// \brief Enable gates to go to processor for some given links
///
/// \param  linkMap Map of link index for which we want to allow gates
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponAllowGates (LinkMap linkMap)
    {
    OnuRegOrEq (&EpnPassGates, linkMap);
    } // EponAllowGates


////////////////////////////////////////////////////////////////////////////////
/// \brief Disable gates to go to processor for some given links
///
/// \param  linkMap Map of link index for which we want to disallow gates
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponDisallowGates (LinkMap links)
    {
    OnuRegAndEqNot (&EpnPassGates, links);
    } // EponDisallowGates



////////////////////////////////////////////////////////////////////////////////
/// \brief pass unmapped gates or not
///
/// \param  flag enable or disable this feature
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponUnmappedGates (BOOL flag)
    {
    if (flag)
    	{
    	OnuRegOrEq (&EpnPassGates, EpnPassGatesGateLlid16);
    	}
    else
    	{
    	OnuRegAndEqNot (&EpnPassGates, EpnPassGatesGateLlid16);
    	}
    } // EponUnmappedGates


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disables FCS checking of un-mapped frames. 
/// This is intended to be used when passing unmapped frames to a UNI port. 
/// 0: All FCS errored un-mapped frames are discarded
/// 1: All un-mapped frames are passed to a UNI port
///
/// \param  flag enable or disable this feature
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponprvNoUnmappedFcs (BOOL flag)
    {
    if (flag)
    	{
    	OnuRegOrEq(&EpnCtl[0], EpnCtlPrvNoUnmapppedFcs);
    	}
    else
    	{
    	OnuRegAndEqNot(&EpnCtl[0], EpnCtlPrvNoUnmapppedFcs);
    	}
    } // EponprvNoUnmappedFcs


////////////////////////////////////////////////////////////////////////////////
/// \brief Get the map of links that had missing grants since last call
///
/// \param
/// None
///
/// \return
/// map of links that had missing grants since last call
////////////////////////////////////////////////////////////////////////////////
//extern
LinkMap EponMissedGrants (void)
    {
    U32  regVal = OnuRegRead (&EpnGntIntvInt);
    OnuRegWrite (&EpnGntIntvInt, regVal);
    return regVal;
    } // EponMissedGrants


////////////////////////////////////////////////////////////////////////////////
/// \brief Get the map of links that received grants since last call
///
/// \param
/// None
///
/// \return
/// map of links that received grants since last call
////////////////////////////////////////////////////////////////////////////////
//extern
LinkMap EponRcvdGrants (void)
    {
    U32  regVal = OnuRegRead (&EpnGntPresInt);
    OnuRegWrite (&EpnGntPresInt, regVal);
    return regVal;
    } // EponRcvdGrants


////////////////////////////////////////////////////////////////////////////////
/// \brief returns the correct address for a given stat
///
/// \param  link link index for which we want the stats
/// \param  eponStat stat we want to retreive for the link
///
/// \return
/// appropriate stats
////////////////////////////////////////////////////////////////////////////////
static
U32 GetStatDpAddress (LinkIndex linkIndex, EponLinkStatId eponStat)
    {
    //upstream link stats
    if(eponStat >= EponBiUpTotalBytesTx)
        {
        return (UpStatsRamBlockSize* linkIndex) + \
            (eponStat-EponBiUpTotalBytesTx);
        }
    //Bidirectional link downstream stats
    if(linkIndex < TkOnuNumBiDirLlids)
        {
        return (DnStatsRamBlockSize*linkIndex) + \
            (eponStat-EponBiDnTotalBytesRx);
        }
    else
        //Downstream RX only link downstream stats
        {
        return (DnRxOnlyStatsRamOffset + (DnStatsRamBlockSize * linkIndex)) + \
            (eponStat - EponDnTotalBytesRx);
        }
    } // GetStatDpAddress


////////////////////////////////////////////////////////////////////////////////
/// \brief  Reads a link stat to provided buffer
///
/// \param linkIndex    Link to retrieve stat for based on LIF/XIF mapping table
/// \param eponStat     Link stat to look at
/// \param dataPtr      buffer to write value to
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponReadLinkStat (LinkIndex linkIndex,
                       EponLinkStatId eponStat,
                       U64* dataPtr)
    {
    U32  * dptr = (U32 *)dataPtr;


    spin_lock_bh(&epon_spinlock);;

    //The flow for a read operation is as follows.
    //1.    Check if the Data Port Interrupt is ready.
    DpWait ();

    //2.    Update the Data Port Address register.
    OnuRegWrite (&EpnDpAddr, GetStatDpAddress (linkIndex,eponStat));

    //3.    Update the Data Port Command register. Write a ??to the Data Port
    //      Control and the RAM's index into the Data Port Select.
    OnuRegWrite (&EpnDpCmd,
             EpnStatReadOp              |
             ((eponStat < EponBiUpTotalBytesTx) ?
              EpnDpRamSelStatsDn : EpnDpRamSelStatsUp));

    //4.    Check to see if the Data Port Interrupt is read.
    DpWait ();

    //5.    Read the Data Port Data register to get the operation's results.
    *dptr = OnuRegRead (&EpnDpData[1]);
    ++dptr;
    *dptr = OnuRegRead (&EpnDpData[0]);
    spin_unlock_bh(&epon_spinlock);;	
    } // EponReadLinkStat


////////////////////////////////////////////////////////////////////////////////
/// \brief  Reads a statistic
///
/// \param eponStat     Link stat to look at
///
/// \return
/// value of the statistic
////////////////////////////////////////////////////////////////////////////////
//extern
U32 EponReadStat (U8 eponStat)
    {
    return OnuRegRead (&EpnUnmapLlidStat(eponStat));
    } // EponReadStat


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the EPON to filter discovery gates
///
///  Note here that we don't filter out on the OLT capability but only on
///  Discovery window type. It would be strange for an OLT to open a window
///  that it is not capable to handle but in this case we would attempt
///  registration.
///
/// Parameter:
/// \param capability   will the ONU attemp 1G registration or 10G?
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponFilterDiscoveryGates (MpcpDiscoveryInfo capability)
    {
    // See "Detailed description" about why we ignore OLT the capability
    capability &= MpcpGateInfoWindowMsk;
    OnuRegWrite (&EpnDiscFilter,
                 TkPutField(EpnDiscFilterPrvInfoMsk, ~capability) |
                 TkPutField(EpnDiscFilterPrvInfoVal, capability));
    } // EponFiterDiscoveryGates


////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponInSync (BOOL force)
    {
    U32  regVal = OnuRegAnd (&EpnMainInt, (EpnMainIntDnTimeNotInSync | EpnMainIntDnTimeInSync));

    // These interrupts are only set when an MPCP frame arrives, which is less
    // frequent than this polling routine is called, so we only clear the
    // interrupts if we see an out of sync event or the caller requires a full
    // refresh (e.g. - due to a lower level failure such as LOS).  This allows
    // us to detect both in sync and out of sync events without maintaining an
    // internal state.
    if (TestBitsSet(regVal, EpnMainIntDnTimeNotInSync) || force)
        {
        OnuRegWrite (&EpnMainInt, (EpnMainIntDnTimeNotInSync |
                                   EpnMainIntDnTimeInSync));
        }

    return (regVal == EpnMainIntDnTimeInSync);
    } // EponInSync

////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponAnyFatalEvent(void)
    {
    if (TestBitsAny(OnuRegRead(&EpnMainInt),
        EpnMainIntBbhUpsHalt | EpnMainIntBiQOverRun | EpnMainIntBadUpFrLen |
        EpnMainIntL1sQOverRun | EpnMainIntL2sQOverRun) )
        {
        return TRUE;
        }
    return FALSE;
    }

////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponBBHUpsHaultGet(void)
    {
    if (TestBitsSet(OnuRegRead(&EpnMainInt), EpnMainIntBbhUpsHalt) )
        {
        //This Bit must be clear after the Tear Down configuration finished
        return TRUE;
        }
    return FALSE;
    } // EponInSync


////////////////////////////////////////////////////////////////////////////////
//extern
void EponBBHUpsHaultClr(void)
    {
    OnuRegWrite (&EpnMainInt, EpnMainIntBbhUpsHalt);
    } // EponBBHUpsHaultClr    


////////////////////////////////////////////////////////////////////////////////
//extern
void EponBBHUpsHaultStatusClr(void)
    {
    OnuRegWrite (&EpnBBHUpsFaultInt, EpnIntBbhUpsFault);
    } // EponBBHUpsHaultStatusClr    


////////////////////////////////////////////////////////////////////////////////
/// \brief  sets the grant interval register
///
/// \param time     Time to set the grant interval to
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetGrantInterval (U16 time)
    {
    OnuRegWrite(&EpnGntIntvl, time);
    } // EponSetGrantInterval


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set correct EPON overhead depending on FEC tx setting
///
/// \param linkMap  Bitmap of links that have FEC tx enabled
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetFecTx (LinkMap linkMap)
    {
    OnuRegWrite(&EpnEnUpstreamFec, ((U16)linkMap) << EpnEnUpstreamFecCfgSft);
    if (linkMap == 0)
        {
        // Depending on the upstream rate, we set the correct report lengths
        OnuRegWrite (&EpnFecIpgLen, reportLengthNoFec);
        OnuRegAndEqNot (&EpnCtl[1], EpnCtlFecRptEn);
        }
    else
        {
        // Depending on the upstream rate, we set the correct report lengths
        OnuRegWrite (&EpnFecIpgLen, reportLengthFec);
        OnuRegOrEq (&EpnCtl[1], EpnCtlFecRptEn);
        }
    } // EponSetFecTx


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetAesTx (LinkMap links, EponAesMode mode)
    {
    U32  regVal = OnuRegRead (&EpnAesConfiguration);
    U8 link;

    for (link = 0; link < TkOnuNumTxLlids; link++)
        {
        if (TestBitsSet(links, 1UL << link))
            {
            regVal &= ~EpnAesMode(link, EponAesClearAll);
            regVal |= EpnAesMode(link, mode);
            }
        }

    OnuRegWrite (&EpnAesConfiguration, regVal);
    } // EponSetAesTx


////////////////////////////////////////////////////////////////////////////////
/// \brief Put L1 queues into reset
///
/// \param l1Map  Bitmap of L1 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetL1Reset (L1Map l1Map)
    {
    OnuRegOrEq (&EpnRstL1Q, l1Map);
    } // EponSetL1Reset


////////////////////////////////////////////////////////////////////////////////
/// \brief Bring L1 queues out of reset
///
/// \param l1Map  Bitmap of L1 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponClearL1Reset (L1Map l1Map)
    {
    OnuRegAndEqNot (&EpnRstL1Q, l1Map);
    } // EponClearL1Reset


////////////////////////////////////////////////////////////////////////////////
/// \brief Put L2 queues into reset
///
/// \param l2Map  Bitmap of L2 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetL2Reset (L2Map l2Map)
    {
    OnuRegOrEq (&EpnRstL2RptQ, l2Map);
    } // EponSetL2Reset


////////////////////////////////////////////////////////////////////////////////
/// \brief Bring L2 queues out of reset
///
/// \param l2Map  Bitmap of L2 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponClearL2Reset (L2Map l2Map)
    {
    OnuRegAndEqNot (&EpnRstL2RptQ, l2Map);
    } // EponClearL2Reset


////////////////////////////////////////////////////////////////////////////////
/// \brief Enable schedulers for L1 queues
///
/// \param l1Map  Bitmap of L1 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponEnableL1Sched (L1Map l1Map)
    {
    L1Index  l1 = EponNumL1Queues;

    while ((l1--) != 0)
        {
        if (TestBitsSet (l1Map, 1UL << l1))
            {
            OnuRegTableOrEq (EpnQLlidMap,
                             l1,
                             EpnQLlidMapCfgSchEn);
            }
        }
    } // EponEnableL1Sched


////////////////////////////////////////////////////////////////////////////////
/// \brief Disable schedulers for L1 queues
///
/// \param l1Map  Bitmap of L1 queues
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponDisableL1Sched (L1Map l1Map)
    {
    L1Index  l1 = EponNumL1Queues;

    while ((l1--) != 0)
        {
        if (TestBitsSet (l1Map, 1UL << l1))
            {
            OnuRegTableAndEqNot (EpnQLlidMap,
                                 l1,
                                 EpnQLlidMapCfgSchEn);
            }
        }
	DelayMs(1);
    } // EponDisableL1Sched


////////////////////////////////////////////////////////////////////////////////
//extern
void EponClearShaperConfig (EponShpElement shpr)
    {  
                    
    OnuRegTableWrite( EpnTxL1sShpCfg,
                          (shpr*EpnTxL1sShpCfgWidth)+EpnTxL1ShpMapIdx,
                          0);
    // Keep the IPG configuration unchanged.
    OnuRegTableAndEq(EpnTxL1sShpCfg,
                    (shpr*EpnTxL1sShpCfgWidth)+EpnTxL1ShpRateIdx,
                    EpnTxL1sShpCfgIncOvr);   
    } // EponClearShaperConfig


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperRate (EponShpElement shpr, EponShaperRate shaperRate)
    {

    OnuRegTableWriteField( EpnTxL1sShpCfg,
                           (shpr*EpnTxL1sShpCfgWidth)+(EpnTxL1ShpRateIdx),
                           EpnTxL1sShpCfgRate0,
                           shaperRate);

    } // EponSetShaperRate


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperMbs (EponShpElement shpr, EponMaxBurstSize mbs)
    {

    OnuRegTableWriteField( EpnTxL1sShpCfg,
                           (shpr*EpnTxL1sShpCfgWidth)+(EpnTxL1ShpRateIdx),
                           EpnTxL1sShpCfgBstSize0,
                           mbs);
    } // EponSetShaperMbs


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperIpg (EponShpElement shpr, BOOL ipg)
    {

    if (ipg)
        {
        OnuRegTableOrEq(EpnTxL1sShpCfg,
                        (shpr*EpnTxL1sShpCfgWidth)+(EpnTxL1ShpRateIdx),
                        EpnTxL1sShpCfgIncOvr);
        }
    else
        {
        OnuRegTableAndEqNot(EpnTxL1sShpCfg,
                        (shpr*EpnTxL1sShpCfgWidth)+(EpnTxL1ShpRateIdx),
                        EpnTxL1sShpCfgIncOvr);
        }
        
    } // EponSetShaperIpg


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperL1Map (EponShpElement shpr, U32 map)
    {

    OnuRegTableWrite(   EpnTxL1sShpCfg,
                        (shpr*EpnTxL1sShpCfgWidth)+(EpnTxL1ShpMapIdx),
                        map);
    } // EponSetShaperL1Map


////////////////////////////////////////////////////////////////////////////////
/// \brief Setup EPON for given number of priorities
///
/// \param numPri number of priority to set (between 1 and 8). Any other value
/// will lead to undetermined behavior.
/// 
/// \param opts report configuration options
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetNumPri (U8 numPri, PonSchMode mode)
    {
    U8 numPri2Mode[] = {MultiPriAllZero, MultiPriAllZero,
	                MultiPri3,       MultiPri4,
	                MultiPriAllZero, MultiPriAllZero,
	                MultiPriAllZero, MultiPri8};
	

    numPri = (numPri - 1) & 0xF; // Now numPri it is between 0 and 7.
    if (numPri == 0) // Teknovus Mode
        {
        OnuRegFieldWrite (&EpnCtl[0], EpnCtlRptSel, 1);
        OnuRegAndEqNot (&EpnMultiPriCfg,
                        EpnMultiPriCfgRpt
                        | EpnMultiPriCfgRpt80
                        | EpnMultiPriCfgRptSwapQS0
                        | EpnMultiPriCfgRptGntsOutst0
                        | EpnMultiPriCfgSharedL2
                        | EpnMultiPriCfgPrvSharedBurstCap
                        );
        }
    else
        {
        OnuRegFieldWrite (&EpnCtl[0], EpnCtlRptSel, 0);
        OnuRegOrEq (&EpnMultiPriCfg,
                    EpnMultiPriCfgRpt
                    | EpnMultiPriCfgRptSwapQS0
                    | EpnMultiPriCfgRptGntsOutst0
                    | ((mode!=PonSchModePriorityBased)?
                        EpnMultiPriCfgSharedL2:0)
                    | ((mode==PonSchMode3715CompatabilityMode)?
                        EpnMultiPriCfgPrvSharedBurstCap:0)
        );
        }
    OnuRegFieldWrite (&EpnCtl[1], EpnCtlCfgCtcRpt, numPri2Mode[numPri]);
    } // EponSetNumPri


void EponDropUnmapLink (BOOL flag)
    {
    if (flag)
        {
        OnuRegOrEq(&EpnCtl[0], EpnCtlCfgDropUnMap);
        }
    else
        {
        OnuRegAndEqNot(&EpnCtl[0], EpnCtlCfgDropUnMap);
        }
    }


BOOL EponL1QueueEmptyCheck(U32 qIndex)
    {
    U32 queueMap = (1UL << qIndex);
    return (OnuRegAnd (&EpnL1QueueEmpty, queueMap) != 0);
    }
    
    
////////////////////////////////////////////////////////////////////////////////
/// \brief Set received max frame size of epon mac
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponSetMaxFrameSize (U16 maxFrameSize)
    {
     if(maxFrameSize > EponMaxFrameSize)
        return;
        
     OnuRegWrite (&EpnMaxFrameSize,
                 TkPutField (EpnMaxFrameSizeCfg, maxFrameSize));
    } // EponSetMaxFrameSize


////////////////////////////////////////////////////////////////////////////////
/// \brief Set L1 queue byte limit.
///
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
//extern
void EponL1ByteLimitSet(U8 queue, U8 limit)
    {
    OnuRegTableWrite (EpnL1QByteLimit, queue, limit);
    }


///////////////////////////////////////////////////////////////////////////////
/// \brief Get L1 queue byte limit.
///
///
/// \return
/// U8
////////////////////////////////////////////////////////////////////////////////
//extern
U8 EponL1ByteLimitGet(U8 queue)
    {
    return (U8)OnuRegTableRead(EpnL1QByteLimit, queue);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief initialize EPON port
///
/// Resets EPON MAC to default values
///
/// \param up10G TRUE if upstream PON speed is 10G, FALSE otherwise
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponInit (U16 maxFrameSize, BOOL preDraft2Dot1, BOOL add16TqToFront,
               LaserRate upRate)
    {
    //release the EPN interface.
    VpbReset(VpbEpnR,FALSE);
    // Update variables
    iopPreDraft2Dot1 = preDraft2Dot1;
    iopNttReporting = add16TqToFront;

    reportLengthNoFec = ReportLengths1G;
    reportLengthFec = ReportLengths1G; 

    endOfBurst = 0;
    // Re-adjust the data if 10G up. This implementation favors code size over
    // execution time. (a "if" is cheaper that "if-then-else")
    if (upRate == LaserRate10G)
        {
        reportLengthNoFec = ReportLengths10G;
        reportLengthFec = ReportLengths10GFec;
        endOfBurst = EndOfBurst10G;
        }

    // Hold Epon Blocks into reset
    OnuRegWrite (&EpnCtl[0], 0UL);

    OnuRegWrite (&EpnMaxFrameSize,
                 TkPutField (EpnMaxFrameSizeCfg, maxFrameSize));

    // Default Time Stamp Differential
    OnuRegWrite (&EpnTimeStampDiff, TimeStampDiff);

    // Grant Start Time Delta
    OnuRegWrite (&EpnGntTimeStartDelta, GrantStartTimeDelta);
    // Grant Time Margin
    OnuRegWrite (&EpnDnRdGntMargin, GrantStartTimeMargin);

    // Misalignement Count and Pause
    OnuRegWrite (&EpnDnGntMisalignThr, MisalignThreshold);
    OnuRegWrite (&EpnDnGntMisalignPause, MisalignPause);

    // Grant Interval
    OnuRegFieldWrite (&EpnGntIntvl, EpnGntIntvl, MpcpNoGrantTime);

    OnuRegWrite (&EpnRptByteLen, ReportByteLen);

    EponSetL1Reset(EponAllL1Qs);
    EponSetL2Reset(EponAllL2Qs);

    // Discovery scaling is unneeded since we don't use the Discovery increment
    // Remove stall grants
    // and clear epon stats on read.
    OnuRegWrite (&EpnCtl[1],
                 EpnCtlDisableDiscScale |
                 EpnCtlCfgStaleGntChk |
                 EpnCtlFecRptEn |  // Does nothing until you do the per LLID FEC
                 EpnCtlClrOnRd );

    // Bring Epon Out of Reset
    OnuRegWrite (&EpnCtl[0],
                 EpnCtlPrvTekModePrefetch | // even on 1/1 till get requirement
                 EpnCtlCfgDropScb |
                 EpnCtlNotDrxRst | EpnCtlDrxEn |
                 EpnCtlNotUtxRst | EpnCtlUtxEn |
                 ((maxFrameSize == 0) ? EpnCtlCfgVlanMax : 0));
 
	//Enable RX and TX to BBH
	
    //Need to drop the unmapped traffic.
    EponDropUnmapLink(TRUE);
    EponprvNoUnmappedFcs(TRUE);
    EponUnmappedGates(TRUE);
    } // EponInit

// End of Epon.
