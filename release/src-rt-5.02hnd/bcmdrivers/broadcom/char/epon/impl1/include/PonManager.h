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



#if !defined(PonManager_h)
#define PonManager_h
////////////////////////////////////////////////////////////////////////////////
/// \file PonManager.h
/// \brief Manages PON specific Configuration and detection of PON status
///
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"
#include "PonMgrEncrypt.h"
#include "PonMgrFec.h"
#include "OsAstMsgQ.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
    {
    OnuAssertId regUp;
    OnuAssertId regDn;
    OnuAssertId actUp;
    OnuAssertId actDn;
    } PACK PonCfgAlarm;

//Per Register spec "Traffic is shaped in units of 2^-19 Gbps (~2 kbps) to
//any value up to line rate" however 2^-19 == 0.0000019073486328125 hence
//the use of 1907 here.
#define FifoRateFactor          1907UL
#define FifoBurstFactor         256U
#define FifBytesPerBlock        1024UL
#define FifMinFrameSize         64UL
#define FifPktOverhead          16UL
#define FifFifoMinBufferPerFrame (FifMinFrameSize + FifPktOverhead)


#define PonMgrOptToEpnOptMsk 0xC0

#define PriorityMask 0xF
#define PonMgrRptModeToPri(rptMode)     ((rptMode) & PriorityMask)

typedef enum
    {
    PonMgrRxOpticalLos       = 0,
    PonMgrRxOpticalUnLock    = 1,
    PonMgrRxOpticalLock      = 2,
    PonMgrRxOpticalStateNum
    } PonMgrRxOpticalState_e;

typedef U8 PonMgrRxOpticalState;


//Shaper types
typedef U8  PonShaper;
typedef U32 PonShaperRate;
typedef U16 PonMaxBurstSize;

//The value representing an unused shaper.
#define PonShaperBad                              0xFF
#define LinkIndexAll                              0xFF

#define PonPollingTimer   10//10MS
#define PonEventTimer     (1000) //1S
#define PonPmdPollingTimer 10//10MS

#define SLlidTotalQNum               9
#define DefaultL1ByteLimit           128
#define CtcL1DataQSize               1368
#define CtcL1OamQSize                16
#define CtcSLlidDataQMaxIndex        8
#define CtcSLlidDataQMinIndex        1

#define CtcSLlidTotalDataQNum        8
#define CtcSLlidTotalQNum            9
#define CtcL2DefSize                 192

#define DPOE_MAX_LINK_NUM            8
#define DPOE_L1_Q_NUM_PER_LINK       2
#define DPOE_L1_OAM_Q_SIZE           500
#define DPOE_L1_DATA_Q_SIZE          500
#define DPOE_L2_Def_Size             1950

#define BCM_MAX_LINK_NUM             8
#define BCM_L1_Q_NUM_PER_LINK        2
#define BCM_L1_OAM_Q_SIZE            500
#define BCM_L1_DATA_Q_SIZE           500
#define BCM_L2_Def_Size              1950


////////////////////////////////////////////////////////////////////////////////
/// \brief get the oam queue index of the link
///
/// \param link     Link index to get
///
/// \return
/// the oam queue index
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonMgrLinkOamQGet(LinkIndex link);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Control user traffic(mainly for llid data channel)
///
/// \returnn none.
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrUserTrafficSet(LinkIndex link, BOOL enable);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get user traffic Status(mainly for llid data channel)
///
/// \returnn none.
////////////////////////////////////////////////////////////////////////////////
//extern
void PonMgrUserTrafficGet(LinkIndex link, BOOL* state);

/* return 0 on success, -1 on fail */ 
int PonMgrActToWanState(LinkIndex link, BOOL enable);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get links that have been started
///
/// \return started links
////////////////////////////////////////////////////////////////////////////////
LinkMap PonMgrStartedLinksGet (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get reporting mode currently provisioned in HW
///
/// \return Report mode currently setup.
////////////////////////////////////////////////////////////////////////////////
extern
PonMgrRptMode PonMgrReportModeGet (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get number of priority in the reporting mode
///
/// \return How many priority
////////////////////////////////////////////////////////////////////////////////
#define PonMgrPriCountGet()     PonMgrRptModeToPri(PonMgrReportModeGet())


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get current maximum links based on reporting mode
///
/// \return
/// report mode
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonMgrGetCurrentMaxLinks(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get maximum number of links for reporting mode
///
/// \param  rptMode     Report mode to get max links for
///
/// \return Max links for reporting mode
////////////////////////////////////////////////////////////////////////////////
extern
U8 PonMgrGetMaxLinksForReportMode (PonMgrRptMode rptMode);


////////////////////////////////////////////////////////////////////////////////
/// PonMgrSetMpcpTimeTransfer - Set time to transmit next pulse
///
/// This function programs the LIF or XIF transmit time for passthrough and
/// tracked operation.
///
 // Parameters:
/// \param time Time to transmit pulse
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrSetMpcpTimeTransfer (U32 time);



////////////////////////////////////////////////////////////////////////////////
/// PonMgrGetCurMpcpTime - Get current MPCP time
///
/// This function returns the MPCP time of the most recently received downstream
/// packet. It is updated only when a downstream packet is received.
///
 // Parameters:
///
/// \return
/// Current MPCP time
////////////////////////////////////////////////////////////////////////////////
extern
U32 PonMgrGetCurMpcpTime (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Configure reporting settings for current report mode
///
/// This function sizes and sets sources for L2 fifos for all links based on the
/// current report mode.  In Teknovus mode all data and OAM queues for a link
/// are mapped to a single L2 fifo.  CTC reporting has one L2 per configured
/// report priority which is settable to 3, 4, or 8.  In this mode OAM is mapped
/// to the highest priority and data queues are assigned in descending priority.
/// If there are more data queues then report priorities all extra low priority
/// queues are mapped to the lowest priority L2 for the link.
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrL2Cfg(U8 link_num);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return a link index associated to a phy llid
///
/// \param link     Phy LLID
///
/// \return Link index
/// None
////////////////////////////////////////////////////////////////////////////////
extern
LinkIndex PonMgrPhyLlidToIndex (PhyLlid phy);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Retrieve the link index for a MAC address
///
/// \param  mac     MAC to get index for
///
/// \return Index of link that matches the MAC, TkOnuNumTxLlids if none
////////////////////////////////////////////////////////////////////////////////
extern
LinkIndex PonMgrGetLinkForMac(const MacAddr * mac);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Retrieve the MAC address of a link
///
/// \param  link    Index of the link
/// \param  mac     Where to store the MAC
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrGetMacForLink(LinkIndex link, MacAddr * mac);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Sets idle time with target ONU laser ON/OFF
///
/// \param  syncTime            sync time (in TQ)
/// \param  targetLaserOn       attempted laser ON time  (in TQ)
/// \param  targetLaserOff      attempted laser OFF time (in TQ)
///
/// \return Actual grant overhead provisionned in hardware
////////////////////////////////////////////////////////////////////////////////
extern
U16 PonMgrSetTargetIdleTime (MpcpInterval16 syncTime,
                             MpcpInterval16 targetLaserOn,
                             MpcpInterval16 targetLaserOff);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Sets idle time with default ONU laser ON/OFF
///
/// \param  syncTime    sync time (in TQ)
///
/// \return Actual grant overhead provisionned in hardware
////////////////////////////////////////////////////////////////////////////////
extern
U16 PonMgrSetDefaultIdleTime (MpcpInterval16 syncTime);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Enable L1/L2 queues
///
/// \param links    Bitmap of links to enable
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrEnableL1L2(LinkMap links);



////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable L1/L2 queues
///
/// \param links    Bitmap of links to disable
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrDisableL1L2(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable normal operation for a link
///
/// Specifically cause the ONU to go into a hibernate state.  Where we report no
/// data and send no non-report frames.  Specifically this is required for
/// certain kinds of on the fly reconfiguration.
///
/// \param links    Bitmap of links to pause
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrPauseLinks(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Enable normal operation for a link
///
/// \param links    Bitmap of the links to resume
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrResumeLinks(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Stop upstream traffic on a set of links
///
/// This is required before making any changes that affect reporting such as
/// overhead adjustments, report modes, queue configs, etc.
///
/// \param  links   Bitmap of links to stop
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrStopLinks(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief  reset some link's upstream path at MAC
///
/// NOTE: for detail of this function, please refer to JIRA SWBCACPE-18596
///
/// \param  links   Bitmap of links to operate
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrResetLinkUpPath(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Re-start upstream traffic on a set of links
///
/// NOTE: the links MUST be completely stopped prior to calling this function
///
/// \param  links   Bitmap of links to start
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrStartLinks(LinkMap links);


////////////////////////////////////////////////////////////////////////////////
/// \brief Configure all resources needed to register a link
///
/// \param link index of the link
/// \param llid physical LLID value
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrCreateLink (LinkIndex link, PhyLlid  llid);


////////////////////////////////////////////////////////////////////////////////
/// \brief Remove all resources needed to register a link
///
/// \param link index of the link to destroy
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrDestroyLink (LinkIndex link);


////////////////////////////////////////////////////////////////////////////////
/// \brief Applies a new burst cap to a link on the fly
///
/// \param link     Link index for burst cap
/// \param bcap     New burst cap values in a array of 16 bytes values
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrSetBurstCap(LinkIndex link, const U16 BULK* bcap);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Add shapers for queue map
///
/// \param  shapedQueueMap  The map of port queues to associate with the shaper
/// \param  rate            The shaper rate
/// \param  mbs             The shaper max burst size
///
/// \return A handle to the shaper element or PonShaperBad if failed
////////////////////////////////////////////////////////////////////////////////
extern
PonShaper PonMgrAddShaper (U32 shapedQueueMap,
                              PonShaperRate rate,
                              PonMaxBurstSize mbs);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Delete a shaper
///
/// \param  shp     Shaper to deactivate
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrDelChannelShaper (PonShaper shp);

////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable PON RX
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void PonMgrRxDisable(void);



////////////////////////////////////////////////////////////////////////////////
/// \brief  Enable PON RX
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void PonMgrRxEnable(void);

////////////////////////////////////////////////////////////////////////////////
/// \brief Do we have a signal?
///
/// \return
/// TRUE if loss of signal, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonMgrLos (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return a PhyLlid associated to a link index
///
/// \param link     Logical link number
///
/// \return
/// Physical Llid
////////////////////////////////////////////////////////////////////////////////
extern
PhyLlid PonMgrGetPhyLlid (LinkIndex link);

/* Set the Laser Tx mode */
void PonMgrSetLaserStatus (rdpa_epon_laser_tx_mode mode);

/* Get the Laser Tx mode status */
rdpa_epon_laser_tx_mode PonMgrGetLaserStatus (void);

/* Get the Laser Tx burst status */
rdpa_epon_laser_tx_mode PonMgrGetLaserBurstStatus (void);

/* Get the Laser Rx power status */
void PonMgrLaserRxPowerGet(bdmf_boolean *on);

////////////////////////////////////////////////////////////////////////////////
/// \brief Start definition of queue configuraton
///
/// \param none
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void pon_mgr_q_cfg_start (void);

////////////////////////////////////////////////////////////////////////////////
/// \brief  Config Link Queue
///
 /// \param *cfg    epon mac queue  config
 ///
/// \return void
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrLinkQueueConfig(epon_mac_q_cfg_t *cfg);

////////////////////////////////////////////////////////////////////////////////
/// \brief Tells if a given link keeps receiving gates
///
/// \param link Index of the link we query
///
/// \return
/// TRUE if link is alive
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonMgrLinkAlive (LinkIndex link);


////////////////////////////////////////////////////////////////////////////////
/// EponGetRxOptState : The overhead for AES depends on the AES mode
///
 // Parameters:
/// \param  None
///
/// \return
/// the state of the Rx Optical
////////////////////////////////////////////////////////////////////////////////
extern
PonMgrRxOpticalState PonMgrGetRxOptState (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the currently active PON
///
/// \param pon  New active PON
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrActivePonSet(PortInstance pon);


////////////////////////////////////////////////////////////////////////////////
/// \brief Initialization of Pon Configuration
///
/// Initial configuration for PON.
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrInit (void);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the weight of a priority
///
/// \param link     Index of link
/// \param pri      Priority on link
/// \param weight   Weight to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrSetWeight(LinkIndex link, U8 pri, U8 weight);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Flush all pending grants on the indicated links
///
/// \param linkMap  Bitmap of links to flush
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrFlushGrants(LinkMap linkMap);


/* set pon interval check time, never fails */
void PonLosCheckTimeSet(U16 time);


/* get pon interval check time, never fails */
U16 PonLosCheckTimeGet(void);


////////////////////////////////////////////////////////////////////////////////
/// \brief Gate los check handle
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////

extern
BOOL GateLosCheckHandle(BOOL sync);


////////////////////////////////////////////////////////////////////////////////
/// \brief Setup XIF/LIF for TX-to-RX loopback
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrTxToRxLoopback (void);

////////////////////////////////////////////////////////////////////////////////
/// \brief Setup PON reporting mode
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrSetReporting (PonMgrRptMode rptMode);


////////////////////////////////////////////////////////////////////////////////
/// \brief Set L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonMgrL1ByteLimitSet(U8 queue, U8 limit);


///////////////////////////////////////////////////////////////////////////////
/// \brief Get L1 queue byte limit.
///
///
/// \return
/// True:success; False:fail
////////////////////////////////////////////////////////////////////////////////
extern
BOOL PonMgrL1ByteLimitGet(U8 queue, U8* limit);

///////////////////////////////////////////////////////////////////////////////
/// \brief Get Data Queue L1 Map
///
///
/// \return L1 Map
/// 
////////////////////////////////////////////////////////////////////////////////
extern
U32 PonMgrDataQueueL1MapGet(LinkIndex link);

///////////////////////////////////////////////////////////////////////////////
/// \brief Get Link FlowId by link and queue
///
///
/// \return FlowId
/// 
////////////////////////////////////////////////////////////////////////////////
extern
U16 PonMgrLinkFlowGet(U32 link, U32 queue);

////////////////////////////////////////////////////////////////////////////////
/// \brief Set received max frame size of epon mac
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrMaxFrameSizeSet(U16 maxFrameSize);

////////////////////////////////////////////////////////////////////////////////
/// \brief Init Epon Mac Queue Config
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrEponMacQueueConfigInit(epon_mac_q_cfg_t *cfg);

////////////////////////////////////////////////////////////////////////////////
/// \brief set rdpa llid fec overhead 
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void PonMgrRdpaLlidFecSet(LinkIndex link,BOOL enable);

#if defined(__cplusplus)
}
#endif

#endif // PonManager.h
