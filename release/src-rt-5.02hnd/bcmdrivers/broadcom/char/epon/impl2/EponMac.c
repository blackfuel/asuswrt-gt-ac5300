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

#include <linux/spinlock.h>

#include "EponMac.h"
#include "bcm_epon_cfg.h"
#include "drv_epon_epn_ag.h"
#include "drv_epon_epn_onu_mac_addr_ag.h"
#include "drv_epon_epn_tx_l1s_shp_ag.h"
#include "drv_epon_epon_top_ag.h"
#include "bcm_epon_common.h"


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

#define EpnTxCtcBurstLimitPrv0Msk            0x0003FFFFUL

//##############################################################################
// Constant Definitions
//##############################################################################
#define EndOfBurst10G                  2
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
#define EpnMaxGrantSize		0x4e20UL

#define EpnLlidCount                         32
#define StatByteRangeOffset 4
#define EpnDpRamSelStatsDownstream 0
#define EpnDpRamSelStatsUpstream 1
#define EpnQueueNum                          32

#define UpstreamStatsRamBlockSize            17
#define DownstreamStatsRamBlockSize          21

#define EpnStatReadOp                        0
#define EpnStatWriteOp                       1

#define EpnMaxFlushTime                      200

epn_main_int_status main_int_status = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};


typedef bdmf_error_t (*epn_ummap_llid_stat_cb)(uint32_t *stats);

static epn_ummap_llid_stat_cb epn_ummap_llid_stat_get[EponUnmappedLlidSmallFrameCount+1] = 
    {
    ag_drv_epn_unmap_big_cnt_get,               /* Big Counter */
    ag_drv_epn_unmap_frame_cnt_get,             /* Frame Counter */
    ag_drv_epn_unmap_fcs_cnt_get,               /* FCS Counter */
    ag_drv_epn_unmap_gate_cnt_get,              /* Gate Counter */
    ag_drv_epn_unmap_oam_cnt_get,               /* Oam Counter */
    ag_drv_epn_unmap_small_cnt_get,             /* Small Counter */
    };

//##############################################################################
// Local Variable Definitions
//##############################################################################

static BOOL iopPreDraft2Dot1;
static BOOL iopNttReporting;
static   U8 endOfBurst;

static  DEFINE_SPINLOCK(epon_spinlock);


//##############################################################################
// Function Definitions
//##############################################################################
void EponTopSetRate (LaserRate upRate, LaserRate dnRate)
    {
    bdmf_boolean dn2g = 0;
    bdmf_boolean up10g = 0;
    bdmf_boolean dn10g = 0;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    if (dnRate == LaserRate10G)
        {
        dn10g = 1;
        }
    else if(dnRate == LaserRate2G)
        {
        dn2g = 1;
        }
    else
        {
        }
        
    if (upRate == LaserRate10G)
        {
        up10g = 1;
        }

    drv_error += ag_drv_epon_top_control_set(dn2g, up10g, dn10g);
    }


void EponTopClearReset (U8 id,BOOL en)
    {
    epon_top_reset reset;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    drv_error += ag_drv_epon_top_reset_get(&reset);

    switch (id)
        {
        case EponTopXpcsRxRst:
            reset.xpcsrxrst_n = en;
            break;
        case EponTopXpcsTxRst:
            reset.xpcstxrst_n = en;
            break;
        case EponTopXifR:
            reset.xifrst_n = en;
            break;
        case EponTopClkDivR:
            reset.clkprgrst_n = en;
            break;
        case EponTopNcoR:
            reset.ncorst_n = en;
            break;
        case EponTopEpnR:
            reset.epnrst_n = en;
            break;
        case EponTopLifR:
            reset.lifrst_n = en;
            break;
        default:
            break;
        }

    drv_error += ag_drv_epon_top_reset_set(&reset);
    } // EponTopReset

void EponTopInit(void)
    {
    epon_top_interrupt interrupt = {1,1,1,1,1,1,1};
    //epon_top_reset reset;
    bdmf_error_t drv_error = BDMF_ERR_OK;
	
    EponTopSetRate(PonCfgDbGetUpRate(), PonCfgDbGetDnRate());
/*
    reset.xpcsrxrst_n = 1;
    reset.xifrst_n = 1;
    reset.clkprgrst_n = 1;
    reset.ncorst_n = 1;
    reset.lifrst_n = 1;
    reset.epnrst_n = 1;
    drv_error += ag_drv_epon_top_reset_set(&reset);
*/
    /* reset the interrupt status */
    drv_error += ag_drv_epon_top_interrupt_set(&interrupt);
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
    uint8_t mfgAddrRegLo;
    uint32_t onuaddrreg;
    U16 macHi16;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    // Each Mac addr takes 2 entries in the table. Hence the x2
    drv_error += ag_drv_epn_onu_mac_addr_lo_get(link, &mfgAddrRegLo, &onuaddrreg);
    drv_error += ag_drv_epn_onu_mac_addr_hi_get(link, &macHi16);
    mac->lowHi.hi = EPON_HTONS(macHi16);
    mac->lowHi.low = EPON_HTONL((mfgAddrRegLo << 24)|onuaddrreg);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    uint8_t mfgAddrRegLo = EPON_NTOHL(mac->lowHi.low) >> 24;
    uint32_t onuaddrreg = EPON_NTOHL(mac->lowHi.low) & 0xFFFFFF;
    U16 macHi16 = EPON_NTOHS(mac->lowHi.hi);

    // Each Mac addr takes 2 entries in the table. Hence the x2
    drv_error += ag_drv_epn_onu_mac_addr_lo_set(link, mfgAddrRegLo, onuaddrreg);
    drv_error += ag_drv_epn_onu_mac_addr_hi_set(link, macHi16);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;

    drv_error += ag_drv_epn_olt_mac_addr_lo_set(EPON_NTOHL(mac->lowHi.low));
    drv_error += ag_drv_epn_olt_mac_addr_hi_set(EPON_NTOHS(mac->lowHi.hi));
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    // A0 and A1 don't support below MTU BCap values
    if (cap < EpnMinBcapValue)
        {
        cap = EpnMinBcapValue;
        }

    drv_error = ag_drv_epn_burst_cap_set(l2, cap);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;

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

    drv_error = ag_drv_epn_tx_ctc_burst_limit_set(l2, limit);
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
U16 EponSetIdleTime (U16 front, U16 back, IdleTimeType idleType, U16 register_req_packet_size_in_tq)
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
    bdmf_error_t drv_error = 0;
    U32 upTimeStampOff = PreambleLengthTq + (iopPreDraft2Dot1 ? 0: front);

    drv_error += ag_drv_epn_up_time_stamp_off_set(upTimeStampOff, upTimeStampOff);

    // Reuse "front" to save space. Front now means total overhead
    // Adding potential endOfBurst (had been set to zero when in 1G up)
    front += back + endOfBurst;

    if (iopNttReporting) // This is to simulate what a Passave ONU does
        {                // It's Passave's special an undocumented magic 16
        front += 16;     // extra TQ. This is to cover a bug in the Passave OLT
        }

    if (idleType == DiscIdleTime)
        {
        drv_error += ag_drv_epn_disc_grant_ovr_hd_set(front);
        drv_error += ag_drv_epn_dn_discovery_size_set(front + register_req_packet_size_in_tq);
        }
    else
        {
        drv_error += ag_drv_epn_grant_ovr_hd_set(front, front);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    
    drv_error += ag_drv_epn_dn_discovery_seed_set(offset&0xFFFUL);
    } // EponSetGateOffset


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
    return 0;
    } // EponSetL1EventQueue


////////////////////////////////////////////////////////////////////////////////
/// \brief provision L2 Event Queue
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U16  endAddress = startAddress;

    // The granularity of the register is 4 frames, hence the shift operation.
    // Note that start and end are included
    if (sizeInFrames != 0)
        {
        endAddress += (U16)(sizeInFrames >> 2);
        drv_error += ag_drv_epn_tx_l2s_queue_config_set(l2, endAddress, startAddress);
        endAddress++; // next available address is one after this end
        }
    else
        {
        drv_error += ag_drv_epn_tx_l2s_queue_config_set(l2, 0, 0);
        }

    return endAddress;
    } // EponSetL2EventQueue


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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean dportbusy = TRUE;
    uint8_t dportselect;
    bdmf_boolean dportcontrol;

    while ((dportbusy) && (timeout--))
        {
        drv_error += ag_drv_epn_data_port_command_get(&dportbusy, &dportselect, &dportcontrol);
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
    EponGrantClearReset(linkMap);
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
    U16 cnt = 0;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgflushl2sen;
    bdmf_boolean flushl2sdone;
    uint8_t cfgflushl2ssel;
    
    for (i = 0;i < EpnQueueNum;i++)
        {
        if (TestBitsSet(l2map,(1<<i)))
            {
            cnt = 0;

            drv_error += ag_drv_epn_l2s_flush_config_set(TRUE, 0, i);

            do
                {
                drv_error += ag_drv_epn_l2s_flush_config_get(&cfgflushl2sen, &flushl2sdone, &cfgflushl2ssel);
                if(cnt >1000)
                    {
                    printk("flush not done \n");
                    break;
                    }

                cnt++;
                DelayUs(50);
                }while(flushl2sdone == FALSE);

            cnt = 0;
            
            drv_error += ag_drv_epn_l2s_flush_config_set(FALSE, 0, i);
            
            do
                {
                drv_error += ag_drv_epn_l2s_flush_config_get(&cfgflushl2sen, &flushl2sdone, &cfgflushl2ssel);
                if(cnt >10)
                    {
                    printk("flush not recover \n");
                    break;
                    }

                cnt++;
                }while(flushl2sdone == TRUE);
            }
        }
    } // EponFlushGrantFifo


static
BOOL EponCheckL2Empty(U32 l2map)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean l2sQueEmpty = 1;
    U8 queueIdx = 0;

    for (queueIdx = 0; queueIdx < EponNumL2Queues; queueIdx++)
        {        
        if (TestBitsSet(l2map, (1<<queueIdx)))
            {
            drv_error += ag_drv_epn_tx_l2s_que_empty_get(queueIdx, &l2sQueEmpty);
            if (!l2sQueEmpty)
                break;
            }
        }

    return l2sQueEmpty? TRUE : FALSE;            
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief Flush the L2 FIFO repeatly
///
///
/// \param l2map  Map of l2 to flush
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void EponRepeatlyFlushL2Fifo(U32 l2map)
    {
    U32  i=0;

    for (i = 0; i < EpnMaxFlushTime; i++)
        {
        EponFlushL2Fifo(l2map);
	udelay(50);
        if (EponCheckL2Empty(l2map))
           {
           break;            
           }
        }

    if (i == EpnMaxFlushTime)
        printk("FlushL2Fifo failed \n");
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantReset(LinkMap linkMap)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_reset_gnt_fifo_set(link, TRUE);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantClearReset(LinkMap linkMap)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_reset_gnt_fifo_set(link, FALSE);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantEnable(LinkMap linkMap)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_enable_grants_set(link, TRUE);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponGrantDisable(LinkMap linkMap)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_enable_grants_set(link, FALSE);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void EponUpstreamDisable (LinkMap linkMap)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgEnableUpstreamFeedBack;
    Bool feedBackValid = FALSE;
    U8 cnt = 0;

    //Set config
    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_enable_upstream_set(link, FALSE);
            }
        }

    while ((cnt < 50) && (feedBackValid == FALSE))
        {
        feedBackValid = TRUE;
        //Checker
        for (link = 0; link < EpnLlidCount; link++)
            {
            if ((linkMap >> link) &0x1)
                {
                drv_error += ag_drv_epn_enable_upstream_fb_get(link, &cfgEnableUpstreamFeedBack);
                if (cfgEnableUpstreamFeedBack)
                    {
                    feedBackValid = FALSE;
                    break;
                    }
                }
            }
        cnt++;
        if (feedBackValid)
            {
            break;
            }
        }

    if (feedBackValid == FALSE)
        {
        printk("upstream epn disable time expire, link:0x%08x \n", linkMap);
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
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgEnableUpstreamFeedBack;
    Bool feedBackValid = FALSE;
    U8 cnt = 0;

    //Set config
    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) &0x1)
            {
            drv_error += ag_drv_epn_enable_upstream_set(link, TRUE);
            }
        }

    while ((cnt < 50) && (feedBackValid == FALSE))
        {
        //Checker
        feedBackValid = TRUE;
        //Checker
        for (link = 0; link < EpnLlidCount; link++)
            {
            if ((linkMap >> link) &0x1)
                {
                drv_error += ag_drv_epn_enable_upstream_fb_get(link, &cfgEnableUpstreamFeedBack);
                if (!cfgEnableUpstreamFeedBack)
                    {
                    feedBackValid = FALSE;
                    break;
                    }
                }
            }
        cnt++;
        if (feedBackValid)
            {
            break;
            }
        }

    if (feedBackValid == FALSE)
        {
        //printk("upstream epn enable time expire, link:0x%08x \n", linkMap);
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
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((linkMap >> link) & 0x1)
            {
            drv_error += ag_drv_epn_pass_gates_set(link, TRUE);
            }
        }
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
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if ((links >> link) & 0x1)
            {
            drv_error += ag_drv_epn_pass_gates_set(link, FALSE);
            }
        }
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
    bdmf_error_t drv_error;

    drv_error = ag_drv_epn_pass_gates_set(31, flag);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_control_0 epn_ctl_0;

    drv_error += ag_drv_epn_control_0_get(&epn_ctl_0);
    epn_ctl_0.prvnounmapppedfcs = flag;
    drv_error += ag_drv_epn_control_0_set(&epn_ctl_0);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;
    U32 bitmap = 0;
    bdmf_boolean intDnGntInterval;

    for (link = 0; link < EpnLlidCount; link++)
        {
        drv_error += ag_drv_epn_gnt_intv_int_status_get(link, &intDnGntInterval);
        if (intDnGntInterval)
            {
            bitmap |= (0x1 << link);
            }
        }
    
    for (link = 0; link < EpnLlidCount; link++)
        {
        drv_error += ag_drv_epn_gnt_intv_int_status_set(link, TRUE);
        }
    
    return bitmap;
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;
    U32 bitmap = 0;
    bdmf_boolean intDnGntRdy;

    for (link = 0; link < EpnLlidCount; link++)
        {
        drv_error += ag_drv_epn_gnt_pres_int_status_get(link, &intDnGntRdy);
        if (intDnGntRdy)
            {
            bitmap |= (0x1 << link);
            }
        }

    for (link = 0; link < EpnLlidCount; link++)
        {
        drv_error += ag_drv_epn_gnt_pres_int_status_set(link, TRUE);
        }
    
    return bitmap;
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
    U32 pos = 0;
    //upstream link stats
    if ((eponStat >= EponBiUpTotalBytesTx) && (eponStat <= EponBiUpUnicastFramesTx))
        {
        pos =  (UpstreamStatsRamBlockSize* linkIndex) + \
            (eponStat-EponBiUpTotalBytesTx);

        if (eponStat > EponBiUpReserved)
            {
            pos -= 1;
            }
        }

    if (eponStat <= EponBiDnRegisterFramesRx)
        {
        pos = (DownstreamStatsRamBlockSize*linkIndex) + \
            (eponStat-EponBiDnTotalBytesRx);        
        }

    return pos;
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
    uint16_t dportaddr = 0;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean dportbusy = TRUE;
    uint8_t dportselect;
    bdmf_boolean dportcontrol;
    U32  data = 0;


    if (!(((eponStat >= EponBiDnTotalBytesRx) && (eponStat <= EponBiDnRegisterFramesRx))
        || ((eponStat >= EponBiUpTotalBytesTx) && (eponStat <= EponBiUpUnicastFramesTx))) || 
        (eponStat == EponBiUpReserved))
        {
        *dataPtr = 0;
        return;
        }
    spin_lock_bh(&epon_spinlock);

    //The flow for a read operation is as follows.
    //1.    Check if the Data Port Interrupt is ready.
    DpWait ();
    

    //2.    Update the Data Port Address register.
    //OnuRegWrite (&EpnDpAddr, GetStatDpAddress (linkIndex,eponStat));
    dportaddr = GetStatDpAddress (linkIndex,eponStat);
    drv_error += ag_drv_epn_data_port_address_set(dportaddr);

    //3.    Update the Data Port Command register. Write a ??to the Data Port
    //      Control and the RAM's index into the Data Port Select.
    dportselect = ((eponStat < EponBiUpTotalBytesTx) ? EpnDpRamSelStatsDownstream : EpnDpRamSelStatsUpstream);
    dportcontrol = EpnStatReadOp;
    drv_error += ag_drv_epn_data_port_command_set(dportbusy, dportselect, dportcontrol);
    //OnuRegWrite (&EpnDpCmd,
    //         EpnStatReadOp              |
    //         ((eponStat < EponBiUpTotalBytesTx) ?
    //          EpnDpRamSelStatsDn : EpnDpRamSelStatsUp));

    //4.    Check to see if the Data Port Interrupt is read.
    DpWait ();

    //5.    Read the Data Port Data register to get the operation's results.
    drv_error += ag_drv_epn_data_port_data_0_get(&data);
    //*dptr = OnuRegRead (&EpnDpData[1]);
    *dataPtr = (U64)data;
    spin_unlock_bh(&epon_spinlock);   
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    uint32_t unmapStat;
    
    drv_error += epn_ummap_llid_stat_get[eponStat](&unmapStat);
    return unmapStat;
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    uint16_t prvdiscinfomask;
    uint16_t prvdiscinfovalue;
    
    // See "Detailed description" about why we ignore OLT the capability
    capability &= MpcpGateInfoWindowMsk;

    prvdiscinfomask = ~capability;
    prvdiscinfovalue = capability;
    drv_error += ag_drv_epn_discovery_filter_set(prvdiscinfomask, prvdiscinfovalue);
    } // EponFiterDiscoveryGates


////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponInSync (BOOL force)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_main_int_status main_int_status;

    drv_error += ag_drv_epn_main_int_status_get(&main_int_status);

    // These interrupts are only set when an MPCP frame arrives, which is less
    // frequent than this polling routine is called, so we only clear the
    // interrupts if we see an out of sync event or the caller requires a full
    // refresh (e.g. - due to a lower level failure such as LOS).  This allows
    // us to detect both in sync and out of sync events without maintaining an
    // internal state.

    if (main_int_status.intdntimenotinsync || force)
        {
        main_int_status.intdntimeinsync = TRUE;
        main_int_status.intdntimenotinsync = TRUE;

        drv_error += ag_drv_epn_main_int_status_set(&main_int_status);
        }

    return main_int_status.intdntimeinsync;
    } // EponInSync

////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponAnyFatalEvent(void)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_main_int_status main_int_status;

    drv_error += ag_drv_epn_main_int_status_get(&main_int_status);

    if ((main_int_status.inttruantbbhhalt == TRUE) ||
        (main_int_status.intbififooverrun == TRUE) ||
        (main_int_status.intbadupfrlen == TRUE) ||
        (main_int_status.intl2sfifooverrun == TRUE))
        {
        printk("EponAnyFatalEvent:\n");
        printk("\tinttruantbbhhalt:%u \n",main_int_status.inttruantbbhhalt);
        printk("\tintbififooverrun:%u \n",main_int_status.intbififooverrun);
        printk("\tintbadupfrlen:%u \n",main_int_status.intbadupfrlen);
        printk("\tintl2sfifooverrun:%u \n",main_int_status.intl2sfifooverrun);
        
        // Clear Interrupt
        main_int_status.inttruantbbhhalt = TRUE;
        main_int_status.intbififooverrun = TRUE;
        main_int_status.intbadupfrlen = TRUE;
        main_int_status.intl2sfifooverrun = TRUE;
        drv_error += ag_drv_epn_main_int_status_set(&main_int_status);
        return TRUE;
        }

    return FALSE;
    }

////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EponBBHUpsHaultGet(void)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_main_int_status main_int_status;

    drv_error += ag_drv_epn_main_int_status_get(&main_int_status);

    return main_int_status.inttruantbbhhalt;
    } // EponInSync


////////////////////////////////////////////////////////////////////////////////
//extern
void EponBBHUpsHaultClr(void)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_main_int_status main_int_status;
    
    memset(&main_int_status, 0, sizeof(epn_main_int_status));
    main_int_status.inttruantbbhhalt = TRUE;
    drv_error += ag_drv_epn_main_int_status_set(&main_int_status);
    } // EponBBHUpsHaultClr    


////////////////////////////////////////////////////////////////////////////////
//extern
void EponBBHUpsHaultStatusClr(void)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;

    for (link = 0; link < EpnLlidCount; link++)
        {
        drv_error += ag_drv_epn_bbh_ups_fault_int_status_set(link, TRUE);
        }
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    
    drv_error += ag_drv_epn_gnt_interval_set((uint16_t)time);
    } // EponSetGrantInterval


////////////////////////////////////////////////////////////////////////////////
//extern
void EponDataPortOp (U8 dir)
    {
    uint16_t dportaddr, maxaddr;
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean dportbusy = TRUE;
    uint8_t dportselect;
    bdmf_boolean dportcontrol;
    U32 dptr;
#define DN_MAX_ADDR_OFFSET 21
#define UP_MAX_ADDR_OFFSET 17
 
   dportselect = (dir < 1) ? EpnDpRamSelStatsDownstream : EpnDpRamSelStatsUpstream;
   maxaddr = (dir < 1) ? DN_MAX_ADDR_OFFSET : UP_MAX_ADDR_OFFSET;
   dportcontrol = EpnStatReadOp;
 
    for (dportaddr = 0; dportaddr < maxaddr; dportaddr ++)
                {
        spin_lock_bh(&epon_spinlock);
   
        //The flow for a read operation is as follows.
        //1.    Check if the Data Port Interrupt is ready.
        DpWait ();
       
    
        //2.    Update the Data Port Address register.
        //OnuRegWrite (&EpnDpAddr, GetStatDpAddress (linkIndex,eponStat));
        drv_error += ag_drv_epn_data_port_address_set(dportaddr);
   
        //3.    Update the Data Port Command register. Write a ??to the Data Port
        //      Control and the RAM's index into the Data Port Select.
        drv_error += ag_drv_epn_data_port_command_set(dportbusy, dportselect, dportcontrol);
        //OnuRegWrite (&EpnDpCmd,
        //         EpnStatReadOp              |
        //         ((eponStat < EponBiUpTotalBytesTx) ?
        //          EpnDpRamSelStatsDn : EpnDpRamSelStatsUp));
   
        //4.    Check to see if the Data Port Interrupt is read.
        DpWait ();
   
        //5.    Read the Data Port Data register to get the operation's results.
        drv_error += ag_drv_epn_data_port_data_0_get(&dptr);
        printk("dportsel: %d, dportaddr: %d, counter: 0x%x\n", dportselect, dportaddr, dptr);
        spin_unlock_bh(&epon_spinlock);  
                }
    } // EponReadLinkStat
    

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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;
    uint8_t modipgpreamblebytes;
    uint8_t cfgrptlen;
    uint8_t cfgfecrptlength;
    uint8_t cfgfecipglength;
    epn_control_1 control_1;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if (TestBitsSet(linkMap, 1UL << link))
            {
            drv_error += ag_drv_epn_enable_upstream_fec_set(link, TRUE);
            }
        else
            {
            drv_error += ag_drv_epn_enable_upstream_fec_set(link, FALSE);
            }
        }

    drv_error += ag_drv_epn_control_1_get(&control_1);

    modipgpreamblebytes = ModIpgPreambleBytes;
    cfgfecipglength = CfgFecIpgLength;
    
    if (linkMap == 0)
        {
        control_1.fecrpten = FALSE;
        if (LaserRate10G == PonCfgDbGetUpRate())
            {
            cfgrptlen = CfgRptLen10G;
            cfgfecrptlength = CfgFecRptLen10G;
            }
        else
            {
            cfgrptlen = CfgRptLen1G;
            cfgfecrptlength = CfgFecRptLen1G;
            }
        }
    else
        {
        control_1.fecrpten = TRUE;
        if (LaserRate10G == PonCfgDbGetUpRate())
            {
            cfgrptlen = CfgRptLen10GFec;
            cfgfecrptlength = CfgFecRptLen10GFec;
            }
        else
            {
            cfgrptlen = CfgRptLen1G;
            cfgfecrptlength = CfgFecRptLen1G;
            }
        }

    drv_error += ag_drv_epn_fec_ipg_length_set(modipgpreamblebytes, cfgrptlen, cfgfecrptlength, cfgfecipglength);
    drv_error += ag_drv_epn_control_1_set(&control_1);
    } // EponSetFecTx


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetAesTx (LinkMap links, EponAesMode mode)
    {
    U8 link;
    bdmf_error_t drv_error = BDMF_ERR_OK;

    for (link = 0; link < EpnLlidCount; link++)
        {
        if (TestBitsSet(links, 1UL << link))
            {
            if (link < (EpnLlidCount/2))
                {
                drv_error += ag_drv_epn_aes_configuration_0_set(link, mode);
                }
            else
                {
                drv_error += ag_drv_epn_aes_configuration_1_set(link, mode);
                }
            }
        }
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U32 cfgl1sclracum = 0;

    drv_error += ag_drv_epn_reset_l1_accumulator_get(&cfgl1sclracum);
    cfgl1sclracum |= l1Map;
    drv_error += ag_drv_epn_reset_l1_accumulator_set(cfgl1sclracum);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U32 cfgl1sclracum = 0;

    drv_error += ag_drv_epn_reset_l1_accumulator_get(&cfgl1sclracum);
    cfgl1sclracum &= (~l1Map);
    drv_error += ag_drv_epn_reset_l1_accumulator_set(cfgl1sclracum);
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;

    for (link = 0; link < EpnQueueNum; link++)
        {
        if (TestBitsSet(l2Map, 1 << link))
            {
            drv_error += ag_drv_epn_reset_l2_rpt_fifo_set(link, TRUE);
            }
        }
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    U8 link;

    for (link = 0; link < EpnQueueNum; link++)
        {
        if (TestBitsSet(l2Map, 1 << link))
            {
            drv_error += ag_drv_epn_reset_l2_rpt_fifo_set(link, FALSE);
            }
        }
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
    } // EponDisableL1Sched


////////////////////////////////////////////////////////////////////////////////
//extern
void EponClearShaperConfig (EponShpElement shpr)
    {  
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgshpincovr;
    uint32_t cfgshprate;
    uint8_t cfgshpbstsize;
    uint32_t cfgshpen = 0;
    // before clear the rate & shpbstsize, must disable the shp for all associated queues.    
    drv_error += ag_drv_epn_tx_l1s_shp_que_en_set(shpr, cfgshpen);
    
    drv_error += ag_drv_epn_tx_l1s_shp_config_get((uint8_t)shpr, &cfgshpincovr, &cfgshprate, &cfgshpbstsize);
    cfgshprate = 0;
    cfgshpbstsize = 0;
    drv_error += ag_drv_epn_tx_l1s_shp_config_set((uint8_t)shpr, cfgshpincovr, cfgshprate, cfgshpbstsize);
    } // EponClearShaperConfig


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperRate (EponShpElement shpr, EponShaperRate shaperRate)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgshpincovr;
    uint32_t cfgshprate;
    uint8_t cfgshpbstsize;
    
    drv_error += ag_drv_epn_tx_l1s_shp_config_get((uint8_t)shpr, &cfgshpincovr, &cfgshprate, &cfgshpbstsize);
    cfgshprate = shaperRate;
    drv_error += ag_drv_epn_tx_l1s_shp_config_set((uint8_t)shpr, cfgshpincovr, cfgshprate, cfgshpbstsize);
    } // EponSetShaperRate


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperMbs (EponShpElement shpr, EponMaxBurstSize mbs)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgshpincovr;
    uint32_t cfgshprate;
    uint8_t cfgshpbstsize;
    
    drv_error += ag_drv_epn_tx_l1s_shp_config_get((uint8_t)shpr, &cfgshpincovr, &cfgshprate, &cfgshpbstsize);
    cfgshpbstsize = mbs;
    drv_error += ag_drv_epn_tx_l1s_shp_config_set((uint8_t)shpr, cfgshpincovr, cfgshprate, cfgshpbstsize);
    } // EponSetShaperMbs


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperIpg (EponShpElement shpr, BOOL ipg)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    bdmf_boolean cfgshpincovr;
    uint32_t cfgshprate;
    uint8_t cfgshpbstsize;
    
    drv_error += ag_drv_epn_tx_l1s_shp_config_get((uint8_t)shpr, &cfgshpincovr, &cfgshprate, &cfgshpbstsize);
    cfgshpincovr = ipg;
    drv_error += ag_drv_epn_tx_l1s_shp_config_set((uint8_t)shpr, cfgshpincovr, cfgshprate, cfgshpbstsize);
    } // EponSetShaperIpg


////////////////////////////////////////////////////////////////////////////////
//extern
void EponSetShaperL1Map (EponShpElement shpr, U32 map)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;

    drv_error += ag_drv_epn_tx_l1s_shp_que_en_set(shpr, map);
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

    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_control_0 control_0;
    epn_control_1 control_1;
    epn_multi_pri_cfg_0 multi_pri_cfg_0;

    drv_error += ag_drv_epn_control_0_get(&control_0);
    drv_error += ag_drv_epn_multi_pri_cfg_0_get(&multi_pri_cfg_0);

    numPri = (numPri - 1) & 0xF; // Now numPri it is between 0 and 7.
    if (numPri == 0) // Teknovus Mode
        {
        //OnuRegFieldWrite (&EpnCtl[0], EpnCtlRptSel, 1);
        control_0.rptselect = 1;
        multi_pri_cfg_0.cfgrptmultipri0 = FALSE;
        multi_pri_cfg_0.cfgrptswapqs0 = FALSE;
        multi_pri_cfg_0.cfgrptgntsoutst0 = FALSE;
        multi_pri_cfg_0.cfgsharedl2 = FALSE;
        multi_pri_cfg_0.cfgsharedburstcap = FALSE;
        }
    else
        {
        //OnuRegFieldWrite (&EpnCtl[0], EpnCtlRptSel, 0);
        control_0.rptselect = 0;
        multi_pri_cfg_0.cfgrptmultipri0 = TRUE;
        multi_pri_cfg_0.cfgrptswapqs0 = TRUE;
        multi_pri_cfg_0.cfgrptgntsoutst0 = TRUE;
        multi_pri_cfg_0.cfgsharedl2 = (mode!=PonSchModePriorityBased)?TRUE:FALSE;
        multi_pri_cfg_0.cfgsharedburstcap = (mode==PonSchMode3715CompatabilityMode)?TRUE:FALSE;
        }

    drv_error += ag_drv_epn_control_1_get(&control_1);
    control_1.cfgctcrpt = numPri2Mode[numPri];
    drv_error += ag_drv_epn_control_1_set(&control_1);

    drv_error += ag_drv_epn_control_0_set(&control_0);
    drv_error += ag_drv_epn_multi_pri_cfg_0_set(&multi_pri_cfg_0);
    } // EponSetNumPri


void EponDropUnmapLink (BOOL flag)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_control_0 control_0;

    drv_error += ag_drv_epn_control_0_get(&control_0);
    control_0.prvdropunmapppedllid = flag;
    drv_error += ag_drv_epn_control_0_set(&control_0);
    }


BOOL EponL1QueueEmptyCheck(U32 qIndex)
    {
    return 1;
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
    bdmf_error_t drv_error = BDMF_ERR_OK;
    
    if(maxFrameSize > EponMaxFrameSize)
        return;

    drv_error += ag_drv_epn_max_frame_size_set(maxFrameSize);
    } // EponSetMaxFrameSize

////////////////////////////////////////////////////////////////////////////////
/// \brief select which L1 Accumulator to report
///\input L1 Accumulator index
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponL1AccSel (U8 l1suvasizesel,U8 l1ssvasizesel)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    
    if(l1suvasizesel >= EponMaxL1AccNum || l1ssvasizesel >= EponMaxL1AccNum)
        return;

    drv_error += ag_drv_epn_l1_accumulator_sel_set(l1suvasizesel,l1ssvasizesel);
    } // EponSetMaxFrameSize


////////////////////////////////////////////////////////////////////////////////
/// \brief get Signed number of bytes in the selected L1S Shaped/UnShaped Virtual Accumulator
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponL1AccBytesGet(U32 *l1svasize,U32 *l1uvasize)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    
    if(l1svasize==NULL || l1uvasize == NULL)
        return;

    drv_error += ag_drv_epn_l1_sva_bytes_get(l1svasize);
    drv_error += ag_drv_epn_l1_uva_bytes_get(l1uvasize);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief get Epon mac main interrupt status
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////

void EponMainInrpStatusGet(epn_main_int_status *main_int_status)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    if(main_int_status == NULL)
        return;
    drv_error += ag_drv_epn_main_int_status_get(main_int_status);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief get Epon Mac L2 queue empty/full/stoped status
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponTxL2QueStatusGet(U8 l2QueIndex,U8 *l2QueEmpty,U8 *l2QueFull,U8 *l2QueStopped)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    if(l2QueIndex >= EponMaxL1AccNum || l2QueEmpty == NULL || l2QueFull == NULL || l2QueStopped == NULL)
        return;

    drv_error += ag_drv_epn_tx_l2s_que_empty_get(l2QueIndex, l2QueEmpty);
    drv_error += ag_drv_epn_tx_l2s_que_full_get(l2QueIndex, l2QueFull);
    drv_error += ag_drv_epn_tx_l2s_que_stopped_get(l2QueIndex, l2QueStopped);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief get Epon Mac L1 queue Shaped/Unshaped status
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponTxL1QueStatusGet(U8 l1AccIndex,U8 *l1sDquQueEmpty,U8 *l1sUnshapedQueEmpty)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    if(l1AccIndex >= EponMaxL1AccNum || l1sDquQueEmpty == NULL || l1sUnshapedQueEmpty == NULL)
        return;

    drv_error += ag_drv_epn_tx_l1s_shp_dqu_empty_get(l1AccIndex,l1sDquQueEmpty);
    drv_error += ag_drv_epn_tx_l1s_unshaped_empty_get(l1AccIndex,l1sUnshapedQueEmpty);
    
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief get Epon Mac L1 queue Shaped/Unshaped Overflow status
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EponTxL1QueOverflowStatusGet(U32 *l1shpOverflow,U32 *l1unshpOverflow)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;

    drv_error += ag_drv_epn_l1_sva_overflow_get(l1shpOverflow);
    drv_error += ag_drv_epn_l1_uva_overflow_get(l1unshpOverflow);
    
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
void EponEpnInit (U16 maxFrameSize, BOOL preDraft2Dot1, BOOL add16TqToFront,
               LaserRate upRate)
    {
    bdmf_error_t drv_error = BDMF_ERR_OK;
    epn_control_0 epn_ctrl_0 = {0};
    epn_control_1 epn_ctrl_1 = {0};	
	
    // Update variables
    iopPreDraft2Dot1 = preDraft2Dot1;
    iopNttReporting = add16TqToFront;

    endOfBurst = 0;
    // Re-adjust the data if 10G up. This implementation favors code size over
    // execution time. (a "if" is cheaper that "if-then-else")
    if (upRate == LaserRate10G)
        {
        endOfBurst = EndOfBurst10G;
        }
	
    // Bring Epon Out of Reset
    EponTopClearReset(EponTopEpnR, TRUE);
    mdelay(20);

    epn_ctrl_0.cfgreplaceupfcs = TRUE;
    epn_ctrl_0.cfgappendupfcs       = TRUE;
    epn_ctrl_0.rptselect       = 1;
    epn_ctrl_0.drxrst_pre_n     = TRUE;
    epn_ctrl_0.drxen            = TRUE;
    epn_ctrl_0.utxrst_pre_n     = TRUE;
    epn_ctrl_0.utxen            = TRUE;
    epn_ctrl_0.moduncappedreportlimit  = TRUE;
    epn_ctrl_0.modmpquesetfirst        = TRUE;
    drv_error += ag_drv_epn_control_0_set(&epn_ctrl_0);

    drv_error += ag_drv_epn_max_gnt_size_set(EpnMaxGrantSize);

    drv_error += ag_drv_epn_max_frame_size_set(maxFrameSize);

    // Default Time Stamp Differential
    drv_error += ag_drv_epn_time_stamp_diff_set(TimeStampDiff);

    drv_error += ag_drv_epn_main_int_status_set(&main_int_status);

    // Grant Start Time Delta
    drv_error += ag_drv_epn_gnt_time_start_delta_set(GrantStartTimeDelta);
    
    // Grant Time Margin
    drv_error += ag_drv_epn_dn_rd_gnt_margin_set(GrantStartTimeMargin);

    // Misalignement Count and Pause
    drv_error += ag_drv_epn_dn_gnt_misalign_thr_set(0, MisalignThreshold);
    drv_error += ag_drv_epn_dn_gnt_misalign_pause_set(MisalignPause); 

    // Grant Interval
    drv_error += ag_drv_epn_gnt_interval_set(MpcpNoGrantTime);

    drv_error += ag_drv_epn_report_byte_length_set(ReportByteLen);
    
    if (upRate == LaserRate10G)
        {
        drv_error += ag_drv_epn_bbh_que_status_delay_set(BbhQueStatDelay10G);
        drv_error += ag_drv_epn_up_packet_fetch_margin_set(UpPacketFetchMargin);
        }
    else
        {
        drv_error += ag_drv_epn_bbh_que_status_delay_set(BbhQueStatDelay1G);
        }
    

    drv_error += ag_drv_epn_minimum_grant_setup_set(0x64UL);

    EponSetL1Reset(EponAllL1Qs);
    EponSetL2Reset(EponAllL2Qs);

    EponGrantDisable(EponAllL1Qs);


    // Discovery scaling is unneeded since we don't use the Discovery increment
    // Remove stall grants
    // and clear epon stats on read.
    epn_ctrl_1.disablediscscale = TRUE;
    epn_ctrl_1.cfgstalegntchk   = TRUE;
    epn_ctrl_1.clronrd          = TRUE;
    drv_error += ag_drv_epn_control_1_set(&epn_ctrl_1);
    
    //Need to drop the unmapped traffic.
    EponDropUnmapLink(TRUE);
    //EponprvNoUnmappedFcs(TRUE);
    //EponUnmappedGates(TRUE);
    } // EponInit

// End of Epon.
