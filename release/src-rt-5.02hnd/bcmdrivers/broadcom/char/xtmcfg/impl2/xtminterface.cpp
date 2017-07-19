/*
    <:copyright-BRCM:2011:proprietary:standard
    
       Copyright (c) 2011 Broadcom 
       All Rights Reserved
    
     This program is the proprietary software of Broadcom and/or its
     licensors, and may only be used, duplicated, modified or distributed pursuant
     to the terms and conditions of a separate, written license agreement executed
     between you and Broadcom (an "Authorized License").  Except as set forth in
     an Authorized License, Broadcom grants no license (express or implied), right
     to use, or waiver of any kind with respect to the Software, and Broadcom
     expressly reserves all rights in and to the Software and all intellectual
     property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
     NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
     BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
    
     Except as expressly set forth in the Authorized License,
    
     1. This program, including its structure, sequence and organization,
        constitutes the valuable trade secrets of Broadcom, and you shall use
        all reasonable efforts to protect the confidentiality thereof, and to
        use this information only in connection with your use of Broadcom
        integrated circuit products.
    
     2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
        AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
        WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
        RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
        ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
        FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
        COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
        TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
        PERFORMANCE OF THE SOFTWARE.
    
     3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
        ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
        INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
        WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
        IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
        OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
        SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
        SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
        LIMITED REMEDY.
    :>    


*/
/***************************************************************************
 * File Name  : xtminterface.cpp (impl2)
 *
 * Description: This file contains the implementation for the XTM interface
 *              class.  This class handles the processing that is associated
 *              with an ATM/PTM port.
 ***************************************************************************/

#include "bcmtypes.h"
#include "xtmcfgimpl.h"
#include "bcmadsl.h"

typedef int (*FN_ADSL_GET_OBJECT_VALUE) (unsigned char lineId, char *objId,
		                                   int objIdLen, char *dataBuf, long *dataBufLen);

static adslMibInfo MibInfo ;

extern "C" {

extern FN_ADSL_GET_OBJECT_VALUE g_pfnAdslGetObjValue;
}

/***************************************************************************
 * Function Name: XTM_INTERFACE
 * Description  : Constructor for the XTM interface class.
 * Returns      : None.
 ***************************************************************************/
XTM_INTERFACE::XTM_INTERFACE( void )
{
    m_ulPhysPort = 0;
    m_ulPhysBondingPort = MAX_INTERFACES ;
    m_ulDataStatus = DATA_STATUS_DISABLED ;
    m_ulErrTickCount = 0 ;
    m_ulIfInPacketsPtm = 0;
    m_ulAutoSenseATM = BC_ATM_AUTO_SENSE_ENABLE ;
    memset(&m_Cfg, 0x00, sizeof(m_Cfg));
    memset( &m_LinkInfo, 0x00, sizeof(m_LinkInfo));
    memset( &m_LinkDelay, 0x00, sizeof(m_LinkDelay));
    memset( &m_Stats, 0x00, sizeof(m_Stats));
    m_LinkInfo.ulLinkState = LINK_DOWN;
    m_Cfg.ulIfAdminStatus = ADMSTS_DOWN;

    // Log that we not initialized at this point
    m_bInitialized = false;

} /* XTM_INTERFACE */


/***************************************************************************
 * Function Name: ~XTM_INTERFACE
 * Description  : Destructor for the XTM interface class.
 * Returns      : None.
 ***************************************************************************/
XTM_INTERFACE::~XTM_INTERFACE( void )
{
    Uninitialize();
} /* ~XTM_INTERFACE */


/***************************************************************************
 * Function Name: Initialize
 * Description  : Initializes the object.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::Initialize( UINT32 ulPort, UINT32 ulInternalPort, UINT32 ulBondingPort,
                                         UINT32 autoSenseATM)
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    
    // Initialize as needed
    if(!m_bInitialized)
    {
        m_ulPhysPort = ulPort;
        m_ulInternalPort = ulInternalPort;
        m_ulPhysBondingPort = ulBondingPort;
        m_ulIfInPacketsPtm = 0;
        m_ulAutoSenseATM = autoSenseATM ;

        XP_REGS->ulTxSarCfg |= (1 << (m_ulPhysPort + TXSARA_CRC10_EN_SHIFT));
        XP_REGS->ulRxSarCfg |= (1 << (m_ulPhysPort + RXSARA_CRC10_EN_SHIFT));

        XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= RX_DOE_MASK ;
        XP_REGS->ulRxAtmCfg[m_ulPhysPort] &= ~RX_DOE_CT_ERROR ;

        SetRxUtopiaLinkInfo (LINK_UP) ;

        XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;

        m_Cfg.ulIfLastChange = XtmOsTickGet() / 10;
    }

    // Log the sucessful initialization
    m_bInitialized = true;

    return( bxStatus );
} /* Initialize */

/***************************************************************************
 * Function Name: ReInitialize
 * Description  : ReInitializes the object in terms of updating the bonding
 * port member.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::ReInitialize( UINT32 ulBondingPort )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    
    m_ulPhysBondingPort = ulBondingPort;
    return( bxStatus );
} /* ReInitialize */

/***************************************************************************
 * Function Name: GetStats
 * Description  : Returns statistics for this interface.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetStats( PXTM_INTERFACE_STATS pStats,
    UINT32 ulReset )
{
    UINT32 ulCurrMibCtrl = XP_REGS->ulMibCtrl;

    XP_REGS->ulMibCtrl = (ulReset) ? 1 : 0;

    pStats->ulIfInOctets  += XP_REGS->ulRxPortPktOctCnt[m_ulPhysPort];
    pStats->ulIfOutOctets += XP_REGS->ulTxPortPktOctCnt[m_ulPhysPort];
    
    if( m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_ATM ||
        m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_ATM_BONDED )
    {
        pStats->ulIfInPackets += XP_REGS->ulRxPortPktCnt[m_ulPhysPort];
    }
    else
    {
        pStats->ulIfInPackets = m_ulIfInPacketsPtm;
        if (m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_PTM_BONDED)
        {
            pStats->ulIfInPackets += (XP_REGS->ulRxPafFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafFragCount[m_ulPhysPort+MAX_PHY_PORTS]);

            /* ulRxPafFragCount[] includes dropped frag. Therefore, we have to
            * deduct the dropped frag count.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedFragCount[m_ulPhysPort+MAX_PHY_PORTS]);
                                  
            /* For PTM bonding, ulRxBondDroppedFragCount and ulRxPafDroppedPktCount
            * need to be counted.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort+MAX_PHY_PORTS]);
            pStats->ulIfInPackets -= (XP_REGS->ulRxBondDroppedFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxBondDroppedFragCount[m_ulPhysPort+MAX_PHY_PORTS]);
        }
        else
        {
            pStats->ulIfInPackets += (XP_REGS->ulRxPafPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafPktCount[m_ulPhysPort+MAX_PHY_PORTS]);

            /* ulRxPafPktCount[] includes dropped packets. Therefore, we have to
            * deduct the dropped packet count.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort+MAX_PHY_PORTS]);
                                  
        }
        m_ulIfInPacketsPtm = (ulReset) ? 0 : pStats->ulIfInPackets;
    }
    
    pStats->ulIfOutPackets += XP_REGS->ulTxPortPktCnt[m_ulPhysPort];
    pStats->ulIfInOamRmCells += (XP_REGS->ulTxRxPortOamCellCnt[m_ulPhysPort] &
        OAM_RX_CELL_COUNT_MASK) >> OAM_RX_CELL_COUNT_SHIFT;
    pStats->ulIfOutOamRmCells += (XP_REGS->ulTxRxPortOamCellCnt[m_ulPhysPort] &
        OAM_TX_CELL_COUNT_MASK) >> OAM_TX_CELL_COUNT_SHIFT;
    pStats->ulIfInAsmCells += (XP_REGS->ulTxRxPortAsmCellCnt[m_ulPhysPort] &
        ASM_RX_CELL_COUNT_MASK) >> ASM_RX_CELL_COUNT_SHIFT;
    pStats->ulIfOutAsmCells += (XP_REGS->ulTxRxPortAsmCellCnt[m_ulPhysPort] &
        ASM_TX_CELL_COUNT_MASK) >> ASM_TX_CELL_COUNT_SHIFT;
    pStats->ulIfInCellErrors +=
        (XP_REGS->ulRxPortErrorPktCellCnt[m_ulPhysPort] &
        ERROR_RX_CELL_COUNT_MASK) >> ERROR_RX_CELL_COUNT_SHIFT;
    pStats->ulIfInPacketErrors +=
        (XP_REGS->ulRxPortErrorPktCellCnt[m_ulPhysPort] &
        ERROR_RX_PKT_COUNT_MASK) >> ERROR_RX_PKT_COUNT_SHIFT;

    XP_REGS->ulMibCtrl = ulCurrMibCtrl;

    return( XTMSTS_SUCCESS );
} /* GetStats */


/***************************************************************************
 * Function Name: SetRxUtopiaLinkInfo
 * Description  : Enabled/Disabled Utopia Links on rx side.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetRxUtopiaLinkInfo ( UINT32 ulLinkState )
{
   UINT32 ulRxPortEnShift = 0, ulRxSlaveIntPortEnShift = 32 ;

   if( m_ulInternalPort )
      ulRxPortEnShift = RXUTO_INT_PORT_EN_SHIFT;
#if defined(CONFIG_BCM963268)
   else {
      if ((GPIO->GPIOBaseMode & GPIO_BASE_UTOPIA_OVERRIDE) != 0) {

         if ((MISC->miscStrapBus & MISC_STRAP_BUS_UTOPIA_MASTER_N_SLAVE) != 0)
            ulRxPortEnShift = RXUTO_EXT_IN_PORT_EN_SHIFT;
         else {
            ulRxPortEnShift = RXUTO_EXT_OUT_PORT_EN_SHIFT;
            ulRxSlaveIntPortEnShift = RXUTO_INT_PORT_EN_SHIFT ;
         }
      }
   }
#endif

   if( ulLinkState == LINK_UP ) {
      XP_REGS->ulRxUtopiaCfg |= 1 << (m_ulPhysPort + ulRxPortEnShift);
      if (ulRxSlaveIntPortEnShift != 32)
         XP_REGS->ulRxUtopiaCfg |= 1 << (m_ulPhysPort + ulRxSlaveIntPortEnShift);
   }
   else {
      XP_REGS->ulRxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulRxPortEnShift));
      if (ulRxSlaveIntPortEnShift != 32)
         XP_REGS->ulRxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulRxSlaveIntPortEnShift));
   }

   return( XTMSTS_SUCCESS );
} /* SetRxUtopiaLinkInfo */

void XTM_INTERFACE::UpdateLinkDelay()
{
	int ret ;
	long int size = sizeof (adslMibInfo) ;

	MibInfo.maxBondingDelay = 0 ;
	
	if (g_pfnAdslGetObjValue != NULL) {
		size = sizeof (MibInfo) ;
		ret  = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
		if (ret != 0)
			XtmOsPrintf ("Error g_pfnAdslGetObjValue port - %d return value - %d \n", m_ulPhysPort, ret) ;
	}

	m_LinkDelay.ulLinkDsBondingDelay = MibInfo.maxBondingDelay ;
}

/***************************************************************************
 * Function Name: SetLinkInfo
 * Description  : Call when there is a change in link status.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetLinkInfo( PXTM_INTERFACE_LINK_INFO pLinkInfo, UINT8 rxConfig,
		                                    UINT8 linkInfoConfig, UINT8 txConfig )
{
	int ret ;
   UINT32 ulTxPortEnShift=0, ulRxTeqPortShift=0, ulRxTeqPortMask=0, ulTxSlaveIntPortEnShift = 32 ;

   if( m_ulInternalPort )
   {
      ulTxPortEnShift  = TXUTO_INT_PORT_EN_SHIFT;
      ulRxTeqPortShift = RXUTO_INT_TEQ_PORT_SHIFT;
      ulRxTeqPortMask  = RXUTO_INT_TEQ_PORT_MASK;
   }
#if defined(CONFIG_BCM963268)
   else {
      /* Utopia override option will be set only for "all external"/ "internal
       * external" port modes.
       */
      if ((GPIO->GPIOBaseMode & GPIO_BASE_UTOPIA_OVERRIDE) != 0) {

         if (MISC->miscStrapBus & MISC_STRAP_BUS_UTOPIA_MASTER_N_SLAVE)
            ulTxPortEnShift = TXUTO_EXT_PORT_EN_SHIFT;
         else {
            ulTxPortEnShift = TXUTO_SLV_PORT_EN_SHIFT;
            ulTxSlaveIntPortEnShift = TXUTO_INT_PORT_EN_SHIFT ;
         }

         ulRxTeqPortShift = RXUTO_EXT_TEQ_PORT_SHIFT;
         ulRxTeqPortMask  = RXUTO_EXT_TEQ_PORT_MASK;
      }
   }
#endif

   if( pLinkInfo->ulLinkState == LINK_UP )
   {
      if( m_Cfg.ulIfAdminStatus == ADMSTS_UP )
      {
         if (txConfig == XTM_CONFIGURE) {
            XP_REGS->ulTxSchedCfg |=
               1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT);
            XP_REGS->ulTxUtopiaCfg |= 
               1 << (m_ulPhysPort + ulTxPortEnShift);
            if (ulTxSlaveIntPortEnShift != 32)
               XP_REGS->ulTxUtopiaCfg |= 1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift);
         }

         if (linkInfoConfig == XTM_CONFIGURE) {
            memcpy( &m_LinkInfo, pLinkInfo, sizeof(m_LinkInfo));
            m_Cfg.usIfTrafficType = pLinkInfo->ulLinkTrafficType;
         }

         if (rxConfig == XTM_CONFIGURE) {
            XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= RX_PORT_EN;

            if (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED)
               XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= (RXA_BONDING_VP_MASK |
                     (m_ulPhysPort << RX_PORT_MASK_SHIFT) |
                     RXA_HEC_CRC_IGNORE |
                     RXA_GFC_ERROR_IGNORE) ;

            if( m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_RAW )
            {
               m_Cfg.usIfTrafficType = TRAFFIC_TYPE_PTM;
               XP_REGS->ulRxUtopiaCfg &= ~ulRxTeqPortMask;
               XP_REGS->ulRxUtopiaCfg |= (m_ulPhysPort << ulRxTeqPortShift);
            }
         } /* rxConfig */
      }

      /* Read a current snapshot of the IF Error Counters */

      if (m_ulPhysBondingPort != MAX_INTERFACES) {
         long int size = sizeof (adslMibInfo) ;

         if (g_pfnAdslGetObjValue != NULL) {
				size = sizeof (MibInfo) ;
	         ret  = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
				if (ret != 0)
					XtmOsPrintf ("g_pfnAdslGetObjValue port - %d return value - %d \n", m_ulPhysPort, ret) ;
			}

         m_LinkDelay.ulLinkDsBondingDelay = MibInfo.maxBondingDelay ;
         m_LinkDelay.ulLinkUsDelay = MibInfo.xdslInfo.dirInfo[1].lpInfo[0].delay ;

         m_PrevIfMonitorInfo.rx_loss_of_sync     = MibInfo.adslPerfData.perfTotal.adslLOSS ;
         m_PrevIfMonitorInfo.rx_SES_count_change = MibInfo.adslPerfData.perfTotal.adslSES ;
         m_PrevIfMonitorInfo.tx_SES_count_change = MibInfo.adslTxPerfTotal.adslSES ;
         m_PrevIfMonitorInfo.tx_LOS_change       = MibInfo.adslTxPerfTotal.adslLOSS ;
         m_PrevIfMonitorInfo.rx_uncorrected      = MibInfo.xdslStat[0].rcvStat.cntRSUncor ;

         m_ulErrTickCount = 0 ;
      }

      while (XTM_IS_WRCHAN_FLUSH_BUSY)
         XtmOsDelay (100) ;

      XP_REGS->ulRxPafWriteChanFlush &= ~(0xFFFFFF11 << m_ulPhysPort) ;

      m_ulDataStatus = DATA_STATUS_ENABLED ;
   }
   else /* LINK_DOWN */
   {
      if (txConfig == XTM_CONFIGURE) {
         XP_REGS->ulTxUtopiaCfg &= 
            ~(1 << (m_ulPhysPort + ulTxPortEnShift));
         if (ulTxSlaveIntPortEnShift != 32)
            XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift));
         XP_REGS->ulTxSchedCfg &=
            ~(1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT));
      }
      else {
         /* Workaround for 68 atm bonding. Keep the scheduler enabled and the
          * utopia disabled (as the other bonding link may still be up.)
          */
         if( m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED ) {
            XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxPortEnShift));
            if (ulTxSlaveIntPortEnShift != 32)
               XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift));
         }
      }

      if (linkInfoConfig == XTM_CONFIGURE) {
         memcpy( &m_LinkInfo, pLinkInfo, sizeof(m_LinkInfo));
         m_Cfg.usIfTrafficType        = TRAFFIC_TYPE_NOT_CONNECTED;
         m_LinkInfo.ulLinkTrafficType = TRAFFIC_TYPE_NOT_CONNECTED;
         m_LinkDelay.ulLinkDsBondingDelay = 0 ;
         m_LinkDelay.ulLinkUsDelay = 0 ;
      }

      if (rxConfig == XTM_CONFIGURE) {
         XP_REGS->ulRxUtopiaCfg &= ~ulRxTeqPortMask;
         XP_REGS->ulRxUtopiaCfg |= (XP_MAX_PORTS << ulRxTeqPortShift);
         XP_REGS->ulRxAtmCfg[m_ulPhysPort] &= ~RX_PORT_EN;
      }

      while (XTM_IS_WRCHAN_FLUSH_BUSY)
         XtmOsDelay (100) ;

      XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;

      m_ulDataStatus = DATA_STATUS_DISABLED ;
      m_ulErrTickCount = 0 ;
   }

   m_Cfg.ulIfLastChange = XtmOsTickGet() / 10;

   return( XTMSTS_SUCCESS );
} /* SetLinkInfo */

/***************************************************************************
 * Function Name: UpdateLinkInfo
 * Description  : Call when there is a change in link traffic type.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::UpdateLinkInfo( UINT32 ulTrafficType )
{
	m_LinkInfo.ulLinkTrafficType = ulTrafficType ;
   m_Cfg.usIfTrafficType = ulTrafficType;
   XtmOsPrintf ("bcmxtmcfg: Auto-Sensed XTM Link Information, port = %d, State = %s, Service Support = %s \n",
                m_ulPhysPort,
                (m_LinkInfo.ulLinkState == LINK_UP ? "UP" :
			         (m_LinkInfo.ulLinkState == LINK_DOWN ? "DOWN":
				       (m_LinkInfo.ulLinkState == LINK_START_TEQ_DATA ? "START_TEQ": "STOP_TEQ"))),
                (ulTrafficType == TRAFFIC_TYPE_ATM ? "ATM" :
                 (ulTrafficType == TRAFFIC_TYPE_PTM ? "PTM" :
                  (ulTrafficType == TRAFFIC_TYPE_PTM_BONDED ? "PTM_BONDED" :
                   (ulTrafficType == TRAFFIC_TYPE_ATM_BONDED ? "ATM_BONDED" : "RAW"))))) ;

   return( XTMSTS_SUCCESS );
} /* UpdateLinkInfo */

/***************************************************************************
 * Function Name: GetLinkErrorStatus (63268)
 * Description  : Call for detecting Link Errors. Only called for 63268 bonding
 * through run-time configurations.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetLinkErrorStatus (UINT32 *pUsStatus, UINT32 *pDsStatus )
{
	int ret ;

   BCMXTM_STATUS nRet = XTMSTS_SUCCESS ;

   if (m_ulPhysBondingPort != MAX_INTERFACES) {

      if (m_ulErrTickCount == 0) {

         long int size = sizeof (adslMibInfo) ;
         IfMonitorErrorCounters CurrIfMonitorInfo ;

         m_ulUsStatus = m_ulDsStatus = XTMSTS_SUCCESS ;

         if (g_pfnAdslGetObjValue != NULL) {
				size = sizeof (MibInfo) ;
            ret = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
				if (ret != 0)
					XtmOsPrintf ("g_pfnAdslGetObjValue port - %d return value - %d \n", m_ulPhysPort, ret) ;
			}

         CurrIfMonitorInfo.rx_loss_of_sync = MibInfo.adslPerfData.perfTotal.adslLOSS ;
         CurrIfMonitorInfo.rx_SES_count_change  = MibInfo.adslPerfData.perfTotal.adslSES ;
         CurrIfMonitorInfo.tx_SES_count_change = MibInfo.adslTxPerfTotal.adslSES ;
         CurrIfMonitorInfo.tx_LOS_change = MibInfo.adslTxPerfTotal.adslLOSS ;
	 CurrIfMonitorInfo.rx_uncorrected = MibInfo.xdslStat[0].rcvStat.cntRSUncor ;

         if ((m_PrevIfMonitorInfo.rx_loss_of_sync != CurrIfMonitorInfo.rx_loss_of_sync) ||
             (m_PrevIfMonitorInfo.rx_SES_count_change != CurrIfMonitorInfo.rx_SES_count_change) ||
             (m_PrevIfMonitorInfo.rx_uncorrected != CurrIfMonitorInfo.rx_uncorrected)) {

				//XtmOsPrintf ("rxLos=%ul, rxSES=%ul, rxUnC=%ul \n",
						        //CurrIfMonitorInfo.rx_loss_of_sync, CurrIfMonitorInfo.rx_SES_count_change,
								  //CurrIfMonitorInfo.rx_uncorrected) ;
				nRet = XTMSTS_ERROR ;
				m_ulDsStatus = XTMSTS_ERROR ;
	 		}

         if ((m_PrevIfMonitorInfo.tx_SES_count_change != CurrIfMonitorInfo.tx_SES_count_change) ||
				 (m_PrevIfMonitorInfo.tx_LOS_change != CurrIfMonitorInfo.tx_LOS_change)) {

				//XtmOsPrintf ("txSES=%ul, txLos=%ul \n",
						        //CurrIfMonitorInfo.tx_SES_count_change, CurrIfMonitorInfo.tx_LOS_change) ;
				nRet = XTMSTS_ERROR ;
            m_ulUsStatus = XTMSTS_ERROR ;
			}

         if (nRet != XTMSTS_SUCCESS) {
            memcpy (&m_PrevIfMonitorInfo, &CurrIfMonitorInfo, sizeof (IfMonitorErrorCounters)) ;
            m_ulErrTickCount = XTM_BOND_DSL_ERR_DURATION_TIMEOUT_MS ;
         }
      }
      else {
         m_ulErrTickCount -= XTM_BOND_DSL_MONITOR_TIMEOUT_MS ;
         nRet = XTMSTS_ERROR ;
      }
   } /* if (m_ulPhysBondingPort != MAX_INTERFACES) */

   *pUsStatus = m_ulUsStatus ;
   *pDsStatus = m_ulDsStatus ;
   return (nRet) ;
}

/***************************************************************************
 * Function Name: SetPortDataStatus (63x68)
 * Description  : Call for setting the port data status.
 *                Also effects the port status in the SAR Rx registers.
 *                This is necessary to avoid SAR lockup from undesired traffic
 *                in DS direction during line micro-interruption.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_INTERFACE::SetPortDataStatus ( UINT32 status, UINT32 flush )
{
   /* if Phy does stop & start the flow, then we do not need this here in XTM.
    * Currently in experimentation phase
    */
   if ((m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED) ||
		 (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_BONDED)) {
      /* For PTM Bonding, XTM Network driver will take care in SW for Tx Path. This is for
       * HW control.
       * We only effect Rx direction, as there may be pending traffic for
       * concerned port in tx direction in the DMA queues, which should not be
       * blocked.
       */

      if (XTM_IS_WRCHAN_FLUSH_BUSY) {
         goto _End ;
      }

      if (status == DATA_STATUS_ENABLED)
         XP_REGS->ulRxPafWriteChanFlush &= ~(0xFFFFFF11 << m_ulPhysPort) ;
      else {
         if (flush == XTM_FLUSH)
              XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;
      }
#if 0
		XtmOsPrintf ("Paf Fl, P%d %x \n", m_ulPhysPort, XP_REGS->ulRxPafWriteChanFlush) ;
#endif
   } /* if (bonded traffic) */

   m_ulDataStatus = status;

_End :
   return ;
}


/***************************************************************************
 * Function Name: Uninitialize
 * Description  : Undo Initialize.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::Uninitialize( void )
{
    // Set link status as needed
    if(m_bInitialized)
        SetRxUtopiaLinkInfo (LINK_DOWN) ;

    // Log the sucessful uninitialization
    m_bInitialized = false;

    return( XTMSTS_SUCCESS );
} /* Uninitialize */


/***************************************************************************
 * Function Name: GetCfg
 * Description  : Returns the interface configuration record.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetCfg( PXTM_INTERFACE_CFG pCfg,
    XTM_CONNECTION_TABLE *pConnTbl )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XTM_CONNECTION *pConn;
    XTM_ADDR Addr;

    memcpy(pCfg, &m_Cfg, sizeof(XTM_INTERFACE_CFG));

    pCfg->ulIfOperStatus = (m_LinkInfo.ulLinkState == LINK_UP &&
        IsInterfaceUp()) ? OPRSTS_UP : OPRSTS_DOWN;

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
    if (m_ulPhysPort != XTM_RX_TEQ_PHY_PORT) {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED) ;
    }
    else {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED | SUPPORT_TRAFFIC_TYPE_TEQ) ;
    }
#else
    if (m_ulPhysPort != XTM_RX_TEQ_PHY_PORT) {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED) ;
    }
    else {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TEQ) ;
    }
#endif

    if (m_ulAutoSenseATM == BC_ATM_AUTO_SENSE_ENABLE)
        pCfg->usIfSupportedTrafficTypes |= SUPPORT_TRAFFIC_TYPE_ATM_BONDED ;

    /* Calculate the number of configured VCCs for this interface. */
    pCfg->ulAtmInterfaceConfVccs = 0;
    UINT32 i = 0;
    while( (pConn = pConnTbl->Enum( &i )) != NULL )
    {
        pConn->GetAddr( &Addr );
        if( ((Addr.ulTrafficType&TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM) &&
            (Addr.u.Vcc.ulPortMask & PORT_TO_PORTID(m_ulPhysPort)) ==
             PORT_TO_PORTID(m_ulPhysPort) )
        {
            pCfg->ulAtmInterfaceConfVccs++;
        }
    }

    return( bxStatus );
} /* GetCfg */


/***************************************************************************
 * Function Name: SetCfg
 * Description  : Sets the configuration record.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetCfg( PXTM_INTERFACE_CFG pCfg )
{
    m_Cfg.ulIfAdminStatus = pCfg->ulIfAdminStatus;
    return( XTMSTS_SUCCESS );
} /* SetCfg */
