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
 * File Name  : xtmprocessor.cpp
 *
 * Description: This file contains the implementation for the XTM processor
 *              class.  This is the topmost class that contains or points to
 *              all objects needed to handle the processing of the XTM processor.
 ***************************************************************************/

#include "xtmcfgimpl.h"
#include "bcm_map.h"
#include "board.h"

static UINT32	gulBondDslMonitorValid = 0 ;

/***************************************************************************
 * Function Name: XTM_PROCESSOR
 * Description  : Constructor for the XTM processor class.
 * Returns      : None.
 ***************************************************************************/
XTM_PROCESSOR::XTM_PROCESSOR( void )
{
   UINT32 ulPort ;
    m_pTrafficDescrEntries = NULL;
    m_ulNumTrafficDescrEntries = 0;
    memset(&m_InitParms, 0x00, sizeof(m_InitParms));
    m_ulConnSem = XtmOsCreateSem(1);
    m_pSoftSar = NULL;
   m_ulPrevRelatedUpEvtValid = 0 ;
   for (ulPort=PHY_PORTID_0; ulPort<MAX_BONDED_LINES; ulPort++) {
      m_ulPendingEvtValid[ulPort] = 0 ;
   }
    memset(m_ulRxVpiVciCamShadow, 0x00, sizeof(m_ulRxVpiVciCamShadow));
} /* XTM_PROCESSOR */


/***************************************************************************
 * Function Name: ~XTM_PROCESSOR
 * Description  : Destructor for the XTM processor class.
 * Returns      : None.
 ***************************************************************************/
XTM_PROCESSOR::~XTM_PROCESSOR( void )
{
   UINT32 ulPort ;

   m_ulPrevRelatedUpEvtValid = 0 ;
   for (ulPort=PHY_PORTID_0; ulPort<MAX_BONDED_LINES; ulPort++) {
      m_ulPendingEvtValid[ulPort] = 0 ;
   }
    Uninitialize(bcmxtmrt_request);
} /* ~XTM_PROCESSOR */

/***************************************************************************
 * Function Name: Initialize (6368, 6362, 6328)
 * Description  : Initializes the object.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::Initialize( PXTM_INITIALIZATION_PARMS pInitParms,
    UINT32 ulBusSpeed, FN_XTMRT_REQ pfnXtmrtReq )

{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    UINT32 i, j, k;
    UINT32 *p;

    if( pInitParms )
        memcpy(&m_InitParms, pInitParms, sizeof(m_InitParms));

    if( pfnXtmrtReq )
        m_pfnXtmrtReq = pfnXtmrtReq;
    
    if( ulBusSpeed )
    {
        m_ulBusSpeed = ulBusSpeed;
        XTM_CONNECTION::SetBusSpeed( ulBusSpeed );
    }

    i = TXSARP_ENET_FCS_INSERT | TXSARP_CRC16_EN | TXSARP_PREEMPT |
        TXSAR_USE_ALT_FSTAT | TXSARP_SOF_WHILE_TX;
    j = XP_REGS->ulRxSarCfg;

	 XP_REGS->ulRxAtmCfg[0] |= RX_DOE_IDLE_CELL ;
	 XP_REGS->ulRxAtmCfg[1] |= RX_DOE_IDLE_CELL ;
	 XP_REGS->ulRxAtmCfg[2] |= RX_DOE_IDLE_CELL ;
	 XP_REGS->ulRxAtmCfg[3] |= RX_DOE_IDLE_CELL ;

    XP_REGS->ulRxAalCfg &= ~RXAALP_CELL_LENGTH_MASK;

#if defined(CONFIG_BCM_JUMBO_FRAME)
   /* Set the packet buffer length to 9*0x100 bytes */
   XP_REGS->ulRxPktBufThreshold &= ~PBTHRESH_MAX_COUNT_MASK;
   XP_REGS->ulRxPktBufThreshold |= 0x9;
#endif

    XP_REGS->ulTxSarCfg |= i;
    XP_REGS->ulRxSarCfg = j;

    m_ulTrafficType = TRAFFIC_TYPE_NOT_CONNECTED;

    XP_REGS->ulTxSchedCfg = TXSCH_SHAPER_RESET;
    for( i = 0; i < 1000; i++ )
        ;

   XP_REGS->ulTxLineRateTimer = 0 ;
    XP_REGS->ulTxSchedCfg = 0;

    for( i = 0; i < XP_MAX_CONNS; i++ )
        XP_REGS->ulSstCtrl[i] = 0;

    for( i = 0; i < XP_MAX_CONNS; i++ )
        XP_REGS->ulSstPcrScr[i] = 0;

    for( i = 0; i < XP_MAX_CONNS; i++ )
        XP_REGS->ulSstBt[i] = 0;

    for( i = 0; i < XP_MAX_CONNS; i++ )
        XP_REGS->ulSstBucketCnt[i] = 0;

    XP_REGS->ulTxUtopiaCfg &= ~TXUTO_MODE_INT_EXT_MASK;

    switch( (m_InitParms.ulPortConfig & PC_INTERNAL_EXTERNAL_MASK) )
    {
    case PC_INTERNAL_EXTERNAL:
        XP_REGS->ulTxUtopiaCfg |= TXUTO_MODE_INT_EXT;
        XP_REGS->ulRxUtopiaCfg |= RXUTO_INTERNAL_BUF0_EN|RXUTO_EXTERNAL_BUF1_EN;
        break;

    case PC_ALL_EXTERNAL:
        XP_REGS->ulRxUtopiaCfg |= RXUTO_EXTERNAL_BUF1_EN;
		  XP_REGS->ulTxUtopiaCfg |= TXUTO_MODE_ALL_EXT;
        break;

    /* case PC_ALL_INTERNAL: */
    default:
		  /* Utopia slave mode only. Master mode in ALL internal is possible. */

        XP_REGS->ulTxUtopiaCfg |= TXUTO_MODE_ALL_INT;
        XP_REGS->ulRxUtopiaCfg |= RXUTO_INTERNAL_BUF0_EN;
        break;
    }

    if( (m_InitParms.ulPortConfig & PC_NEG_EDGE) == PC_NEG_EDGE )
    {
        XP_REGS->ulTxUtopiaCfg |= TXUTO_NEG_EDGE;
        XP_REGS->ulRxUtopiaCfg |= RXUTO_NEG_EDGE;
    }
    else
    {
        XP_REGS->ulTxUtopiaCfg &= ~TXUTO_NEG_EDGE;
        XP_REGS->ulRxUtopiaCfg &= ~RXUTO_NEG_EDGE;
    }

    XP_REGS->ulRxAalMaxSdu = 0xffff;

    /* Initialize transmit and receive connection tables. */
    for( i = 0; i < XP_MAX_CONNS; i++ )
        XP_REGS->ulTxVpiVciTable[i] = 0;

    for( i = 0; i < XP_MAX_CONNS * 2; i++ )
        XP_REGS->ulRxVpiVciCam[i] = 0;

    /* Add headers. */
    UINT32 Hdrs[XP_MAX_TX_HDRS][XP_TX_HDR_WORDS] = PKT_HDRS;
    UINT32 ulHdrOffset = 0; /* header offset in packet */

    for( i = 0, k = PKT_HDRS_START_IDX; Hdrs[k][0] != PKT_HDRS_END; i++, k++ )
    {
        /* p[0] = header length, p[1]... = header contents */
        p = Hdrs[k];
        XP_REGS->ulTxHdrInsert[i] =
            ((p[0] << TXHDR_COUNT_SHIFT) & TXHDR_COUNT_MASK) |
            ((ulHdrOffset << TXHDR_OFFSET_SHIFT) & TXHDR_OFFSET_MASK);

        for( j = 0; j < p[0] / sizeof(UINT32); j++ )
            XP_REGS->ulTxHdrValues[i][j] = p[1 + j];

        if( p[0] % sizeof(UINT32) != 0 )
            XP_REGS->ulTxHdrValues[i][j] = p[1 + (p[0] / sizeof(UINT32))];
    }

    /* Read to clear MIB counters. Set "clear on read" as default. */
    XP_REGS->ulMibCtrl = 1;

    for(p = XP_REGS->ulTxPortPktOctCnt; p <= &XP_REGS->ulBondOutputCellCnt; p++)
        i = *(volatile UINT32 *)p;

    for(p = XP_REGS->ulTxVcPktOctCnt; p < XP_REGS->ulTxVcPktOctCnt+XP_MAX_CONNS; p++)
        i = *(volatile UINT32 *)p;

    XP_REGS->ulRxPktBufMibMatch = 0;
    for( i = 0; i < XP_RX_MIB_MATCH_ENTRIES; i++ )
    {
        j = XP_REGS->ulRxPktBufMibRxOctet;
        k = XP_REGS->ulRxPktBufMibRxPkt;
    }

    /* Configure Rx Filter Mib control register. */
#if defined(CONFIG_BCM96362)
    /* Reset the MIB counter after read */
    XP_REGS->ulRxPktBufMibCtrl = PBMIB_CLEAR;
#else
    XP_REGS->ulRxPktBufMibCtrl = 0;
#endif

    /* Mask and clear interrupts. */
    XP_REGS->ulIrqMask = ~INTR_MASK;
    XP_REGS->ulIrqStatus = ~0;

    /* Initialize interfaces. */
    for( i = PHY_PORTID_0; i < MAX_INTERFACES; i++ ) {
       if ((m_InitParms.bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) ||
           (m_InitParms.bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE)) {
          if (m_InitParms.bondConfig.sConfig.dualLat == BC_DUAL_LATENCY_ENABLE) {
             if (i < PHY_PORTID_2)
                m_Interfaces[i].Initialize(i, i+PHY_PORTID_2);
             else
                m_Interfaces[i].Initialize(i, i-PHY_PORTID_2);
          }
          else {
             /* No dual latency. Hard code it for the interfaces as per chip
              * definitions
              */
             UINT32 ulBondingPort ;
             switch (i) {
                case PHY_PORTID_0  :
                   ulBondingPort = PHY_PORTID_1 ;
                   break ;
                case PHY_PORTID_1  :
                   ulBondingPort = PHY_PORTID_0 ;
                   break ;
                default    :
                   ulBondingPort = MAX_INTERFACES ;
                   break ;
             }
             m_Interfaces[i].Initialize(i, ulBondingPort) ;
          }
       }
       else {
          m_Interfaces[i].Initialize(i, MAX_INTERFACES);
       }
    }

    if( pInitParms)
    {
        /* First time that this Initialize function has been called.  Call the
         * bcmxtmrt driver initialization function.
         */
        XTMRT_GLOBAL_INIT_PARMS GlobInit;
        memcpy(GlobInit.ulReceiveQueueSizes, m_InitParms.ulReceiveQueueSizes,
            sizeof(GlobInit.ulReceiveQueueSizes));
        GlobInit.bondConfig.uConfig = m_InitParms.bondConfig.uConfig;
        GlobInit.ulMibRxClrOnRead   = PBMIB_CLEAR;
        GlobInit.pulMibTxOctetCountBase =  XP_REGS->ulTxVcPktOctCnt;
        GlobInit.pulMibRxCtrl           = &XP_REGS->ulRxPktBufMibCtrl;
        GlobInit.pulMibRxMatch          = &XP_REGS->ulRxPktBufMibMatch;
        GlobInit.pulMibRxOctetCount     = &XP_REGS->ulRxPktBufMibRxOctet;
        GlobInit.pulMibRxPacketCount    = &XP_REGS->ulRxPktBufMibRxPkt;
        GlobInit.pulRxCamBase           = &XP_REGS->ulRxVpiVciCam [0];
        (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_GLOBAL_INITIALIZATION, &GlobInit);

        bxStatus = m_OamHandler.Initialize( m_pfnXtmrtReq );

    }

	 m_pAsmHandler = NULL ;

    return( bxStatus );
} /* Initialize */



/***************************************************************************
 * Function Name: ReInitialize (6362/6328/6358/6338)
 * Description  : Initializes the bonding aspect for the chipsets.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_PROCESSOR::ReInitialize ( UINT32 ulLogicalPort, UINT32 ulTrafficType )
{
	/* Nothing to do, as bonding is not available in these chipsets */
	return ;
}


/***************************************************************************
 * Function Name: Uninitialize
 * Description  : Undo Initialize.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::Uninitialize( FN_XTMRT_REQ pfnXtmrtReq )
{
    UINT32 i;
    XTM_CONNECTION *pConn;

	 gulBondDslMonitorValid = 0 ;
    XtmOsPrintf ("bcmxtmcfg: gulBondDslMonitorValid = %d \n", (int) gulBondDslMonitorValid) ;
	 
    /* Set interface link state to down to disable SAR receive */
    for( i = PHY_PORTID_0; i < MAX_INTERFACES; i++ )
    {
        PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[i].GetLinkInfo();

        if (pLi->ulLinkState == LINK_UP)
        {
            XTM_INTERFACE_LINK_INFO linkInfo;

            linkInfo = *pLi;
            linkInfo.ulLinkState = LINK_DOWN;
            SetInterfaceLinkInfo( PORT_TO_PORTID(i), &linkInfo );
        }
    }

    /* Delete all network device instances. */
    i = 0;
    while( (pConn = m_ConnTable.Enum( &i )) != NULL )
    {
        m_ConnTable.Remove( pConn );
        delete pConn;
    }

    /* Uninitialize interfaces. */
    for( i = PHY_PORTID_0; i < MAX_INTERFACES; i++ )
        m_Interfaces[i].Uninitialize();

    if( m_pTrafficDescrEntries )
    {
        XtmOsFree( (char *) m_pTrafficDescrEntries );
        m_pTrafficDescrEntries = NULL;
    }

    m_OamHandler.Uninitialize();

	 if (m_pAsmHandler) {
		 m_pAsmHandler->Uninitialize() ;
		 XtmOsDelay (20) ;
		 delete m_pAsmHandler ;
		 m_pAsmHandler = NULL ;
	 }

    m_pfnXtmrtReq = pfnXtmrtReq;
    (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_GLOBAL_UNINITIALIZATION, NULL);

    return( XTMSTS_SUCCESS );

} /* Uninitialize */


/***************************************************************************
 * Function Name: GetTrafficDescrTable
 * Description  : Returns the Traffic Descriptor Table.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetTrafficDescrTable(PXTM_TRAFFIC_DESCR_PARM_ENTRY
    pTrafficDescrTable, UINT32 *pulTrafficDescrTableSize)
{
    BCMXTM_STATUS bxStatus = XTMSTS_PARAMETER_ERROR;

    if( *pulTrafficDescrTableSize >= m_ulNumTrafficDescrEntries )
    {
        if( m_pTrafficDescrEntries && m_ulNumTrafficDescrEntries )
        {
            memcpy(pTrafficDescrTable, m_pTrafficDescrEntries,
              m_ulNumTrafficDescrEntries*sizeof(XTM_TRAFFIC_DESCR_PARM_ENTRY));
            bxStatus = XTMSTS_SUCCESS;
        }
    }

    *pulTrafficDescrTableSize = m_ulNumTrafficDescrEntries;

    return( bxStatus );
} /* GetTrafficDescrTable */


/***************************************************************************
 * Function Name: SetTrafficDescrTable
 * Description  : Saves the supplied Traffic Descriptor Table.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SetTrafficDescrTable(PXTM_TRAFFIC_DESCR_PARM_ENTRY
    pTrafficDescrTable, UINT32  ulTrafficDescrTableSize)
{
    BCMXTM_STATUS bxStatus;

    if( ulTrafficDescrTableSize )
    {
        /* Free an existing table if it exists. */
        if( m_pTrafficDescrEntries )
        {
            XtmOsFree( (char *) m_pTrafficDescrEntries );
            m_pTrafficDescrEntries = NULL;
            m_ulNumTrafficDescrEntries = 0;
        }

        UINT32 ulSize =
            ulTrafficDescrTableSize * sizeof(XTM_TRAFFIC_DESCR_PARM_ENTRY);

        /* Allocate memory for the new table. */
        m_pTrafficDescrEntries =
            (PXTM_TRAFFIC_DESCR_PARM_ENTRY) XtmOsAlloc(ulSize);

        /* Copy the table. */
        if( m_pTrafficDescrEntries )
        {
            m_ulNumTrafficDescrEntries = ulTrafficDescrTableSize;
            memcpy( m_pTrafficDescrEntries, pTrafficDescrTable, ulSize );

            /* Update connections with new values. */
            UINT32 i = 0;
            XTM_CONNECTION *pConn;
            while( (pConn = m_ConnTable.Enum( &i )) != NULL )
            {
                pConn->SetTdt( m_pTrafficDescrEntries,
                    m_ulNumTrafficDescrEntries );
            }

            bxStatus = XTMSTS_SUCCESS;
        }
        else
            bxStatus = XTMSTS_ALLOC_ERROR;
    }
    else
        bxStatus = XTMSTS_PARAMETER_ERROR;

    return( bxStatus );
} /* SetTrafficDescrTable */


/***************************************************************************
 * Function Name: GetInterfaceCfg
 * Description  : Returns the interface configuration record for the specified
 *                port id.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetInterfaceCfg( UINT32 ulPortId,
    PXTM_INTERFACE_CFG pInterfaceCfg )
{
    BCMXTM_STATUS bxStatus;
    UINT32 ulPort = PORTID_TO_PORT(ulPortId);

    if( ulPort < MAX_INTERFACES )
        bxStatus = m_Interfaces[ulPort].GetCfg( pInterfaceCfg, &m_ConnTable );
    else
        bxStatus = XTMSTS_PARAMETER_ERROR;

    return( bxStatus ) ;
} /* GetInterfaceCfg */


/***************************************************************************
 * Function Name: SetInterfaceCfg
 * Description  : Sets the interface configuration record for the specified
 *                port.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SetInterfaceCfg( UINT32 ulPortId,
    PXTM_INTERFACE_CFG pInterfaceCfg )
{
    BCMXTM_STATUS bxStatus;
    UINT32 ulPort = PORTID_TO_PORT(ulPortId);
    UINT32 ulBondingPort ;

    if( ulPort < MAX_INTERFACES ) {
       bxStatus = m_Interfaces[ulPort].SetCfg( pInterfaceCfg );
       ulBondingPort = m_Interfaces[ulPort].GetBondingPortNum () ;
       if (ulBondingPort != MAX_INTERFACES)
          bxStatus = m_Interfaces[ulBondingPort].SetCfg( pInterfaceCfg );
    }
    else
       bxStatus = XTMSTS_PARAMETER_ERROR;

    return( bxStatus );
} /* SetInterfaceCfg */

/* The following routine updates (add/removal) the port mask for the connection
 * for bonding.
 * For ex., if a given port is part of the port mask, its bonding member will
 * also be updated to be part of the same port mask.
 * This happens transparent to the user layer.
 * It also updates (add/removal) the traffic type for the connection
 * for bonding.
 * For ex., if a given connection is PTM type and bonding is enabled, then
 * the traffic type is changed from/to PTM_BONDED type.
 * This happens transparent to the user layer.
 */
void XTM_PROCESSOR::UpdateConnAddrForBonding ( PXTM_ADDR pConnAddr, UINT32 operation )
{
   UINT32 ulPortId, ulPort, ulBondingPort ;
   UINT32 *pulPortMask = &pConnAddr->u.Conn.ulPortMask ;
   UINT32 *pulTrafficType = &pConnAddr->ulTrafficType ;

   if (( m_InitParms.bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE) ||
       ( m_InitParms.bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE)) {
      for (ulPortId = PORT_PHY0_PATH0 ; ulPortId <= PORT_PHY1_PATH1 ; ulPortId <<= 0x1) {
         if (*pulPortMask & ulPortId) {
            ulPort = PORTID_TO_PORT (ulPortId) ;
            ulBondingPort = m_Interfaces[ulPort].GetBondingPortNum () ;
            if (ulBondingPort != MAX_INTERFACES) {
               if (operation == XTM_ADD)
                  *pulPortMask |= PORT_TO_PORTID (ulBondingPort) ;
               else
                  *pulPortMask &= ~PORT_TO_PORTID (ulBondingPort) ;
            }
         }
      }

		if( (*pulTrafficType & TRAFFIC_TYPE_ATM_MASK) != TRAFFIC_TYPE_ATM ) {
         if (operation == XTM_ADD)
            *pulTrafficType = TRAFFIC_TYPE_PTM_BONDED ;
         else
            *pulTrafficType = TRAFFIC_TYPE_PTM ;
      }
		else {
         if (operation == XTM_ADD)
            *pulTrafficType = TRAFFIC_TYPE_ATM_BONDED ;
         else
            *pulTrafficType = TRAFFIC_TYPE_ATM ;
		}
   }
}

/* The following routine updates (add/removal) the bonding port ID for the
 * transmit Q Parameters section of the connection configuration.
 * For a given portId in the transmit Q Params information, the corresponding
 * bonding member also will be updated.
 * This happens transparent to the user layer.
 */
void XTM_PROCESSOR::UpdateTransmitQParamsForBonding ( XTM_TRANSMIT_QUEUE_PARMS *pTxQParms,
                                           UINT32 ulTxQParmsSize, UINT32 operation)
{
   UINT32 ulPort, ulBondingPort, ulLogicalBondingPort, ulLogicalPort ;
   UINT32 txQIndex ;
   PXTM_INTERFACE_LINK_INFO pLiPort, pLiBondingPort ;

   for (txQIndex = 0 ; txQIndex < ulTxQParmsSize; txQIndex++) {

      ulPort = PORTID_TO_PORT (pTxQParms [txQIndex].ulPortId ) ;
      ulLogicalPort = PORT_PHYS_TO_LOG (ulPort) ;
      ulLogicalBondingPort = m_Interfaces[ulLogicalPort].GetBondingPortNum () ;

      pLiPort =  m_Interfaces[ulLogicalPort].GetLinkInfo();

		if (ulLogicalBondingPort != MAX_INTERFACES) {

			pLiBondingPort =  m_Interfaces[ulLogicalBondingPort].GetLinkInfo();
			ulBondingPort = PORT_LOG_TO_PHYS (ulLogicalBondingPort) ;

			if ((pLiPort->ulLinkTrafficType == TRAFFIC_TYPE_PTM_BONDED) ||
					(pLiBondingPort->ulLinkTrafficType == TRAFFIC_TYPE_PTM_BONDED)) {

				if (operation == XTM_ADD)
					pTxQParms [txQIndex].ulBondingPortId = PORT_TO_PORTID (ulBondingPort) ;
				else
					pTxQParms [txQIndex].ulBondingPortId = PORT_PHY_INVALID ;
			}
			else
				pTxQParms [txQIndex].ulBondingPortId = PORT_PHY_INVALID ;
		}
      else
         pTxQParms [txQIndex].ulBondingPortId = PORT_PHY_INVALID ;
   } /* txQIndex */
}


/***************************************************************************
 * Function Name: GetConnCfg
 * Description  : Returns the connection configuration record for the
 *                specified connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetConnCfg( PXTM_ADDR pConnAddr,
    PXTM_CONN_CFG pConnCfg )
{
    BCMXTM_STATUS bxStatus;
    XTM_CONNECTION *pConn ;
    
    UpdateConnAddrForBonding ( pConnAddr, XTM_ADD ) ;

    pConn = m_ConnTable.Get( pConnAddr );

    if( pConn )
    {
        UINT32 ulPortMask = pConnAddr->u.Conn.ulPortMask ;

        pConn->GetCfg( pConnCfg );

        /* The operational status of the connection is determined by checking
         * the link status.  A connection can use more than one link.  In order
         * for the operational status of the connection to be "up", atleast one
         * of the links it uses must be "up".
         * If all the links are "down", the connection will be operationally
         * down.
         */
        pConnCfg->ulOperStatus = OPRSTS_DOWN;
        for( UINT32 i = PHY_PORTID_0; i < MAX_INTERFACES; i++ )
        {
            if( (ulPortMask & PORT_TO_PORTID(i)) == PORT_TO_PORTID(i) )
            {
                PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[i].GetLinkInfo();
                if( pLi->ulLinkState == LINK_UP &&
                    pLi->ulLinkTrafficType == pConnAddr->ulTrafficType )
                {
                    /* At least one link is UP. */
                    pConnCfg->ulOperStatus = OPRSTS_UP;
                    break;
                }
            }
        }

        bxStatus = XTMSTS_SUCCESS;
    }
    else
        bxStatus = XTMSTS_NOT_FOUND;

    UpdateConnAddrForBonding ( pConnAddr, XTM_REMOVE ) ;

    return( bxStatus );
} /* GetConnCfg */


/***************************************************************************
 * Function Name: SetConnCfg
 * Description  : Sets the connection configuration record for the specified
 *                connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SetConnCfg( PXTM_ADDR pConnAddr,
    PXTM_CONN_CFG pConnCfg )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XTM_CONNECTION *pConn ;

    /* Make a port mask taking into account of bonding as well */
    UpdateConnAddrForBonding ( pConnAddr, XTM_ADD ) ;

    /* Make a port mask for xmit params of the conncfg taking into account of bonding as well */
    if (pConnCfg)
      UpdateTransmitQParamsForBonding ( pConnCfg->TransmitQParms, 
                                        pConnCfg->ulTransmitQParmsSize, XTM_ADD) ;
    pConn = m_ConnTable.Get( pConnAddr );

    if( pConn == NULL )
    {
        /* The connection does not exist. Create it and add it to the table. */
        if( (pConn = new XTM_CONNECTION(m_pfnXtmrtReq, &m_ConnTable,
            m_ulConnSem, m_pSoftSar, m_ulRxVpiVciCamShadow)) != NULL )
        {
            pConn->SetTdt(m_pTrafficDescrEntries, m_ulNumTrafficDescrEntries);
            pConn->SetAddr( pConnAddr );
            if( (bxStatus = m_ConnTable.Add( pConn )) != XTMSTS_SUCCESS )
            {
                delete pConn;
                pConn = NULL;
            }
        }
        else
            bxStatus = XTMSTS_ALLOC_ERROR;
    }

    if( pConn )
    {
        bxStatus = pConn->SetCfg( pConnCfg );
        if( pConnCfg == NULL )
        {
            /* A NULL configuration pointer means to remove the connection.
             * However, an active connection (and its configuration will stay
             * active until its network device instance is deleted.
             */
            if( !pConn->Connected() )
            {
                m_ConnTable.Remove( pConn );
                delete pConn;
            }
        }
    }

    if (pConnCfg)
      UpdateTransmitQParamsForBonding ( pConnCfg->TransmitQParms, 
                                        pConnCfg->ulTransmitQParmsSize, XTM_REMOVE) ;

    UpdateConnAddrForBonding ( pConnAddr, XTM_REMOVE ) ;
    return( bxStatus );
} /* SetConnCfg */


/***************************************************************************
 * Function Name: GetConnAddrs
 * Description  : Returns an array of all configured connection addresses.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetConnAddrs( PXTM_ADDR pConnAddrs,
    UINT32 *pulNumConns )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    UINT32 ulInNum = *pulNumConns;
    UINT32 ulOutNum = 0;
    UINT32 i = 0;
    XTM_CONNECTION *pConn;
    XTM_ADDR Addr;

    while( (pConn = m_ConnTable.Enum( &i )) != NULL )
    {
        if( ulOutNum < ulInNum )
        {
            pConn->GetAddr( &Addr );
            UpdateConnAddrForBonding ( &Addr, XTM_REMOVE ) ;
            memcpy(&pConnAddrs[ulOutNum++], &Addr, sizeof(Addr));
        }
        else
        {
            bxStatus = XTMSTS_PARAMETER_ERROR;
            ulOutNum++;
        }
    }

    *pulNumConns = ulOutNum;
    return( bxStatus );
} /* GetConnAddrs */


/***************************************************************************
 * Function Name: GetInterfaceStatistics
 * Description  : Returns the interface statistics for the specified port.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetInterfaceStatistics( UINT32 ulPortId,
    PXTM_INTERFACE_STATS pIntfStats, UINT32 ulReset )
{
    BCMXTM_STATUS bxStatus;
    UINT32 ulPort = PORTID_TO_PORT(ulPortId);
    UINT32 ulBondingPort ;

    memset (pIntfStats, 0, sizeof (XTM_INTERFACE_STATS)) ;

    if( ulPort < MAX_INTERFACES ) {
       bxStatus = m_Interfaces[ulPort].GetStats( pIntfStats, ulReset );
       ulBondingPort = m_Interfaces[ulPort].GetBondingPortNum () ;
       if (ulBondingPort != MAX_INTERFACES)
          bxStatus = m_Interfaces[ulBondingPort].GetStats( pIntfStats, ulReset );
    }
    else
        bxStatus = XTMSTS_PARAMETER_ERROR;

    return( bxStatus );
} /* GetInterfaceStatistics */



/***************************************************************************
 * Function Name: GetErrorStatistics
 * Description  : Returns the error statistics for the device.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetErrorStatistics( PXTM_ERROR_STATS pErrStats )
{
    BCMXTM_STATUS bxStatus;

    /* Do we have a good pointer? */
    if(pErrStats != NULL)
    {
        /* Zero out return structure */
        memset (pErrStats, 0, sizeof (XTM_ERROR_STATS)) ;

        /* Zero out fields that are unsupported in this processor */
        pErrStats->ulPafLostFragments = 0;
        pErrStats->ulOverflowErrorsRx = 0;
        pErrStats->ulPafErrs = 0;

        /* Sum errors to get frame drop count */
        for(int i=0; i<XP_MAX_PORTS; i++)
            pErrStats->ulFramesDropped += XP_REGS->ulRxPortErrorPktCellCnt[i];

        /* Flag success */
        bxStatus = XTMSTS_SUCCESS;
    }
    else
    {
        /* Flag bad parameter */
        bxStatus = XTMSTS_PARAMETER_ERROR;
    }

    return( bxStatus );
} /* GetErrorStatistics */

/***************************************************************************
 * Function Name: ComputeSeqDeviation (6368, 6362, 6328)
 * Description  : To compute max sequence deviation to facilitate bonding
 * processing.
 *
 * Returns      : Default value or difference in rates/delays computed.
 ***************************************************************************/
UINT32 XTM_PROCESSOR::ComputeSeqDeviation (UINT32 ulLogicalPort, 
                                           PXTM_INTERFACE_LINK_INFO pLinkInfo,
                                           UINT32 ulBondingPort,
                                           PXTM_INTERFACE_LINK_INFO pOtherLinkInfo)
{
   UINT32 maxSeqDeviation = XTM_DEF_DS_MAX_DEVIATION ;
   UINT32 diffDelay ;

   PXTM_INTERFACE_LINK_DELAY pLinkDelay, pOtherLinkDelay ;

   if (ulBondingPort != MAX_INTERFACES) {

      pLinkDelay       = m_Interfaces[ulLogicalPort].GetLinkDelay () ;
      pOtherLinkDelay  = m_Interfaces[ulBondingPort].GetLinkDelay () ;

      if (pLinkDelay->ulLinkDsBondingDelay == 0) {
         m_Interfaces[ulLogicalPort].UpdateLinkDelay () ;
         pLinkDelay = m_Interfaces [ulLogicalPort].GetLinkDelay () ;
      }

      if (pOtherLinkDelay->ulLinkDsBondingDelay == 0) {
         m_Interfaces[ulBondingPort].UpdateLinkDelay () ;
         pOtherLinkDelay = m_Interfaces [ulBondingPort].GetLinkDelay () ;
      }

      /* Difference in rates */
      /* Difference in Delays */
      if ((pLinkInfo->ulLinkDsRate != 0) && (pOtherLinkInfo->ulLinkDsRate != 0)) {
         if (pLinkInfo->ulLinkDsRate > pOtherLinkInfo->ulLinkDsRate) {
            maxSeqDeviation += (pLinkInfo->ulLinkDsRate/pOtherLinkInfo->ulLinkDsRate) ;
         }
         else if (pLinkInfo->ulLinkDsRate < pOtherLinkInfo->ulLinkDsRate) {
            maxSeqDeviation += (pOtherLinkInfo->ulLinkDsRate/pLinkInfo->ulLinkDsRate) ;
         }

         if (pLinkDelay->ulLinkDsBondingDelay > pOtherLinkDelay->ulLinkDsBondingDelay) {
            diffDelay = pLinkDelay->ulLinkDsBondingDelay-pOtherLinkDelay->ulLinkDsBondingDelay ;
            maxSeqDeviation += ((pLinkInfo->ulLinkDsRate/512*8)*diffDelay)/1000 ;
         }
         else if (pLinkDelay->ulLinkDsBondingDelay < pOtherLinkDelay->ulLinkDsBondingDelay) {
            diffDelay = pOtherLinkDelay->ulLinkDsBondingDelay-pLinkDelay->ulLinkDsBondingDelay ;
            maxSeqDeviation += ((pOtherLinkInfo->ulLinkDsRate/512*8)*diffDelay)/1000 ;
         }

         XtmOsPrintf ("Port    :%u\t DS Rate:%u\t DS Delay:%u \n", ulLogicalPort,
                      pLinkInfo->ulLinkDsRate, pLinkDelay->ulLinkDsBondingDelay) ;
         XtmOsPrintf ("BondPort:%u\t DS Rate:%u\t DS Delay:%u \n", ulBondingPort,
                      pOtherLinkInfo->ulLinkDsRate, pOtherLinkDelay->ulLinkDsBondingDelay) ;
      }

   } /* ulBondingPort */

   return (maxSeqDeviation) ;
}

/***************************************************************************
 * Function Name: SetInterfaceLinkInfo (6368, 6362, 6328)
 * Description  : Called when an ADSL/VDSL connection has come up or gone down.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SetInterfaceLinkInfo( UINT32 ulPortId,
    PXTM_INTERFACE_LINK_INFO pLinkInfo )
{
	BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
	UINT32 ulPhysPort = PORTID_TO_PORT(ulPortId);
	UINT32 ulLogicalPort = PORT_PHYS_TO_LOG(ulPhysPort);
	UINT32 ulBondingPort;
	UINT32 ulTrafficType = (pLinkInfo->ulLinkTrafficType==TRAFFIC_TYPE_PTM_RAW)
		? TRAFFIC_TYPE_PTM : pLinkInfo->ulLinkTrafficType;
	PXTM_INTERFACE_LINK_INFO pOtherLinkInfo = NULL ;
	PXTM_INTERFACE_LINK_INFO pCurrLinkInfo ;

   /* Check if the usermode operations (which are currently checked is the
    * creation of Network devices) are on right now, then defer this event and
    * come back later.
    */
   if (pLinkInfo->ulLinkState == LINK_UP) {
      if (m_ulPrevRelatedUpEvtValid == 0) {
         m_ulPrevRelatedUpEvtValid = (XTM_BOND_DSL_MONITOR_TIMEOUT_MS*200) ; /* Has to be a multiple of monitor timeout */
         /* Fall through to process this event */
      }
      else {
         /* If prev related event is valid, (concurrent events happening), Save
          * this and come back later.
          */
         m_ulPendingEvtValid[ulLogicalPort] = 1 ;
         m_ulPendingEvtPortId[ulLogicalPort] = ulPortId ;
         memcpy (&m_PendingEvtLinkInfo[ulLogicalPort], pLinkInfo, sizeof (XTM_INTERFACE_LINK_INFO)) ;
         XtmOsPrintf ("bcmxtmcfg: PendEvt1 LI, P=%d, S=%d \n", ulLogicalPort, pLinkInfo->ulLinkState) ;
         return (bxStatus) ;
      }
   }
   else {
      if (m_ulPendingEvtValid[ulLogicalPort] == 1) {
         m_ulPrevRelatedUpEvtValid = (XTM_BOND_DSL_MONITOR_TIMEOUT_MS*200) ; /* Has to be a multiple of monitor timeout */
         m_ulPendingEvtPortId[ulLogicalPort] = ulPortId ;
         memcpy (&m_PendingEvtLinkInfo[ulLogicalPort], pLinkInfo, sizeof (XTM_INTERFACE_LINK_INFO)) ;
         XtmOsPrintf ("bcmxtmcfg: PendEvt2 LI, P=%d, S=%d \n", ulLogicalPort, pLinkInfo->ulLinkState) ;
         return (bxStatus) ;
      }
      else {
         m_ulPrevRelatedUpEvtValid = 0 ;
      }
   }

   if ((m_InitParms.bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE) &&
	    (ulTrafficType == TRAFFIC_TYPE_ATM)) {
		/* To support legacy DSLAMs which dont support G.994 handshake
		 * information for bonding.
		 */
		ulTrafficType = pLinkInfo->ulLinkTrafficType = TRAFFIC_TYPE_ATM_BONDED ;
	}

	XtmOsPrintf ("bcmxtmcfg: XTM Link Information, port = %d, State = %s, Service Support = %s \n", 
			ulLogicalPort, 
         (pLinkInfo->ulLinkState == LINK_UP ? "UP" :
          (pLinkInfo->ulLinkState == LINK_DOWN ? "DOWN":
	        (pLinkInfo->ulLinkState == LINK_START_TEQ_DATA ? "START_TEQ": "STOP_TEQ"))),
			(ulTrafficType == TRAFFIC_TYPE_ATM ? "ATM" :
			 (ulTrafficType == TRAFFIC_TYPE_PTM ? "PTM" :
			  (ulTrafficType == TRAFFIC_TYPE_PTM_BONDED ? "PTM_BONDED" :
				(ulTrafficType == TRAFFIC_TYPE_ATM_BONDED ? "ATM_BONDED" : "RAW"))))) ;

#ifdef SUPPORT_EXT_DSL_BONDING
	if (ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)
		ulTrafficType = pLinkInfo->ulLinkTrafficType = TRAFFIC_TYPE_PTM ;
#endif

	pCurrLinkInfo = m_Interfaces[ulLogicalPort].GetLinkInfo() ;

   if ((pCurrLinkInfo) && (pCurrLinkInfo->ulLinkState == pLinkInfo->ulLinkState)) /* duplicate */ {
      return (bxStatus) ;
   }

	/* Reinitialize SAR based on the line traffic type.
	 * We support Bonding modes (PTM-ATM) change on the fly.
	 * Bonding to Normal mode requires reboot, stipulated by the Configuration
	 * stack on the system.
	 **/
	if (pLinkInfo->ulLinkState == LINK_UP) {
      if (CheckTrafficCompatibility (ulTrafficType) != XTMSTS_SUCCESS) {
         XtmOsPrintf ("\nbcmxtmcfg: Traffic Type incompatibility within BONDING & NON-BONDING between"
               " CO & CPE. Rebooting is necessary to match the traffic types.!!!! \n") ;
         XtmOsSendSysEvent (XTM_EVT_TRAFFIC_TYPE_MISMATCH_AND_RESTART) ;
         return (bxStatus) ;
      }
		ReInitialize (ulLogicalPort, ulTrafficType);
   }

	switch( pLinkInfo->ulLinkState )
	{
		case LINK_UP:
		case LINK_DOWN:

			ulBondingPort = m_Interfaces[ulLogicalPort].GetBondingPortNum () ;

			if (pLinkInfo->ulLinkState == LINK_DOWN) {
				pLinkInfo->ulLinkUsRate = pLinkInfo->ulLinkDsRate = 0 ;
			}

			if (ulBondingPort != MAX_INTERFACES)
				pOtherLinkInfo = m_Interfaces[ulBondingPort].GetLinkInfo() ;

         /* In case of normal mode, this will default to 0, but will have the
          * effect in bonded mode.
          */
			{
				const UINT32 ulUsPerSec = 1000000;
				const UINT32 ul100NsPerUs = 10;
				const UINT32 ulBitsPerCell = (53 * 8);
				const UINT32 ulBitsPerCodeWord = (65 * 8);

				UINT32 ulBits = ((ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM)
					? ulBitsPerCell : ulBitsPerCodeWord;
				UINT32 ulLinkRate100Ns = (pOtherLinkInfo) ? 
					(pLinkInfo->ulLinkUsRate + pOtherLinkInfo->ulLinkUsRate)/ ul100NsPerUs :
					pLinkInfo->ulLinkUsRate/ul100NsPerUs ;
				UINT32 ulSlr ;

				ulSlr = (ulLinkRate100Ns != 0) ? ((ulUsPerSec * ulBits) / ulLinkRate100Ns) : 0 ;

				/* The shaper interval timer (SIT) can be set to a fix value rather than a
				 * calculated value based on the link rate. In ATM mode, with SIT value
				 * of 2, we will be able to specify minimum peak cell rate of 32kbps and
				 * it will give us very good accuracy when computing PCR for cell rates
				 * close to line rate.
				 * 
				 * For PTM mode, uses the "fast mode" setting.
				 */
				const UINT32 ulSit = 2;

				XTM_CONNECTION::SetSitUt( ulSit );

				if( pLinkInfo->ulLinkState == LINK_UP )
				{
					XP_REGS->ulTxLineRateTimer = TXLRT_EN |
						(ulSlr << TXLRT_COUNT_VALUE_SHIFT);

					/* Set SIT value.  For PTM mode, use "fast mode". */
					UINT32 ulTxSchedCfg = XP_REGS->ulTxSchedCfg & ~(TXSCH_SIT_COUNT_EN |
							TXSCH_SIT_COUNT_VALUE_MASK | TXSCHP_FAST_SCHED | TXSCH_SOFWT_PRIORITY_EN |
							TXSCHA_ALT_SHAPER_MODE);
					if( (ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
					{
						ulTxSchedCfg |= TXSCH_SIT_COUNT_EN | TXSCHA_ALT_SHAPER_MODE |
							(ulSit<< TXSCH_SIT_COUNT_VALUE_SHIFT);
					}
					else
						ulTxSchedCfg |= TXSCHP_FAST_SCHED | TXSCH_SOFWT_PRIORITY_EN;
					XP_REGS->ulTxSchedCfg = ulTxSchedCfg;

					UINT32 ulSwitchPktTxChan;
					if( (ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
					{
						XP_REGS->ulTxSarCfg &= ~TXSAR_MODE_PTM;
						XP_REGS->ulRxSarCfg &= ~RXSAR_MODE_PTM;
						XP_REGS->ulTxSarCfg |= TXSAR_MODE_ATM;
						XP_REGS->ulRxSarCfg |= RXSAR_MODE_ATM;
						ulSwitchPktTxChan = SPB_ATM_CHANNEL;
					}
					else
					{
						XP_REGS->ulTxSarCfg &= ~TXSAR_MODE_ATM;
						XP_REGS->ulRxSarCfg &= ~RXSAR_MODE_ATM;
						XP_REGS->ulTxSarCfg |= TXSAR_MODE_PTM;
						XP_REGS->ulRxSarCfg |= RXSAR_MODE_PTM;
						ulSwitchPktTxChan = SPB_PTM_CHANNEL;
					}


#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM96328)
					*(UINT32 *) (PKTCMF_OFFSET_ENGINE_SAR + PKTCMF_OFFSET_RXFILT +
							RXFILT_REG_MATCH3_DEF_ID) = 0x0f0e0d0c;
#endif

					//*(UINT32 *) (PKTCMF_OFFSET_ENGINE_SAR + PKTCMF_OFFSET_RXFILT +
					//RXFILT_REG_VCID0_QID) = 0x00000004 ;  
					/* Needed for multiple DMAs each per vcid. */
				}

				if( ulPhysPort < MAX_INTERFACES )
				{
					UINT32 i;
					UINT32 ulPortMask, ulLinkMask;
					XTM_ADDR Addr;

					/* Create a "link up" bit mask of connected interfaces. */
					for( i = PHY_PORTID_0, ulLinkMask = 0; i < MAX_INTERFACES; i++ )
					{
						PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[i].GetLinkInfo();
						if( pLi->ulLinkState == LINK_UP )
							ulLinkMask |= PORT_TO_PORTID(i);
					}

					/* If link down, subtract the port id of the interface that just
					 * changed.  If link up, add the port id of the interface.
					 */
					if( pLinkInfo->ulLinkState == LINK_DOWN )
						ulLinkMask &= ~ulPortId;
					else
						ulLinkMask |= ulPortId;

					/* Call connections that are affected by the link change. A connection
					 * is affected if it is connected and none of its interfaces are
					 * currently up or it is not connected and atleast one of its
					 * interfaces is currently up.
					 * In addition, if the link status changes either way as long as
					 * the link is part of the port mask of the connection.
					 */
					i = 0;
					XTM_CONNECTION *pConn;

					while( (pConn = m_ConnTable.Enum( &i )) != NULL )
					{
						UINT32 ulDoConnSetLinkInfo = 0;

						pConn->GetAddr( &Addr );
						ulPortMask = Addr.u.Conn.ulPortMask ;

						if( ulBondingPort != MAX_INTERFACES )
							ulDoConnSetLinkInfo = (ulPortId & ulPortMask) ? 1 : 0;
						else
							ulDoConnSetLinkInfo =
								(((ulPortMask & ulLinkMask) == 0 &&
								  pConn->Connected()) ||
								 (ulTrafficType == Addr.ulTrafficType &&
								  (ulPortMask & ulLinkMask) == ulPortMask &&
								  !pConn->Connected())) ? 1 : 0;

						if (ulDoConnSetLinkInfo)
						{
							UINT32 maxSeqDeviation ;

							maxSeqDeviation = ComputeSeqDeviation (ulLogicalPort, pLinkInfo,
									ulBondingPort, pOtherLinkInfo) ;
							bxStatus = pConn->SetLinkInfo( (ulLinkMask & ulPortMask),
									pLinkInfo, ulBondingPort, pOtherLinkInfo, maxSeqDeviation );

							if( bxStatus != XTMSTS_SUCCESS )
								break;
						}
					}

					/* Call the interface that there is a link change. */
					CheckAndSetIfLinkInfo (ulLogicalPort, pLinkInfo, ulBondingPort,
							pOtherLinkInfo) ;

					if ((m_InitParms.bondConfig.sConfig.atmBond == BC_ATM_BONDING_ENABLE)
							&&
							(m_pAsmHandler))
						m_pAsmHandler->LinkStateNotify(ulLogicalPort) ;

					if( pLinkInfo->ulLinkState == LINK_DOWN )
					{
						/* Call bcmxtmrt driver with NULL device context to stop the
						 * receive DMA.
						 */
						(*m_pfnXtmrtReq) (NULL, XTMRT_CMD_LINK_STATUS_CHANGED, 
								(char *) ulPortId);
						if (!pOtherLinkInfo ||
								((pOtherLinkInfo->ulLinkState == LINK_DOWN))) {

							XP_REGS->ulTxLineRateTimer = 0;

							ReInitialize (ulLogicalPort, ulTrafficType);

							if (m_pAsmHandler) {
								m_pAsmHandler->Uninitialize() ;
								XtmOsDelay (20) ;
								delete m_pAsmHandler ;
								m_pAsmHandler = NULL ;
							}
						}
						else
							XP_REGS->ulTxLineRateTimer = TXLRT_EN | (ulSlr << TXLRT_COUNT_VALUE_SHIFT) ;
					} /* if (Link is down) */
				}
				else
					bxStatus = XTMSTS_PARAMETER_ERROR;
			}
			break;

		case LINK_START_TEQ_DATA:
			XP_REGS->ulRxAtmCfg[ulPhysPort] = RX_PORT_EN |
				RXA_VCI_MASK | RXA_VC_BIT_MASK;
			XP_REGS->ulRxUtopiaCfg &= ~RXUTO_TEQ_PORT_MASK;
			XP_REGS->ulRxUtopiaCfg |= (ulPhysPort << RXUTO_TEQ_PORT_SHIFT) |
				(1 << ulPhysPort);
			XP_REGS->ulRxVpiVciCam[(TEQ_DATA_VCID * 2) + 1] =
				RXCAM_CT_TEQ | (TEQ_DATA_VCID << RXCAM_VCID_SHIFT);
			XP_REGS->ulRxVpiVciCam[TEQ_DATA_VCID * 2] =
				ulPhysPort | RXCAM_TEQ_CELL | RXCAM_VALID;

			/* This is hardcoded in SetInterfaceLinkInfo.  Set the same value here for TEQ_VCID
				in case link is down */
			/* Set default Match IDs in Packet CMF Rx Filter block. */
			*(UINT32 *) (PKTCMF_OFFSET_ENGINE_SAR + PKTCMF_OFFSET_RXFILT +
					RXFILT_REG_MATCH3_DEF_ID) = 0x0f0e0d0c;
         
         (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_SET_TEQ_DEVCTX,
            (char *) pLinkInfo->ulLinkUsRate);
			break;

		case LINK_STOP_TEQ_DATA:
			XP_REGS->ulRxAtmCfg[ulPhysPort] = 0;
			XP_REGS->ulRxUtopiaCfg &= ~(RXUTO_TEQ_PORT_MASK |
					(1 << ulPhysPort));
			XP_REGS->ulRxUtopiaCfg |= (XP_MAX_PORTS << RXUTO_TEQ_PORT_SHIFT);
			XP_REGS->ulRxVpiVciCam[(TEQ_DATA_VCID * 2) + 1] = 0;
			XP_REGS->ulRxVpiVciCam[TEQ_DATA_VCID * 2] = 0;
			break;
	}

	return( bxStatus );
} /* SetInterfaceLinkInfo */


/***************************************************************************
 * Function Name: SendOamCell
 * Description  : Sends an OAM loopback cell.  For request cells, it waits for
 *                the OAM loopback response.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SendOamCell( PXTM_ADDR pConnAddr,
    PXTM_OAM_CELL_INFO pOamCellInfo )
{
    BCMXTM_STATUS bxStatus = XTMSTS_STATE_ERROR;
    UINT32 ulPort = PORTID_TO_PORT(pConnAddr->u.Vcc.ulPortMask);
    PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[ulPort].GetLinkInfo();

    if( ulPort < MAX_INTERFACES && pLi->ulLinkState == LINK_UP )
        bxStatus = m_OamHandler.SendCell(pConnAddr, pOamCellInfo, &m_ConnTable);

    return( bxStatus );
} /* SendOamCell */


/***************************************************************************
 * Function Name: CreateNetworkDevice
 * Description  : Call the bcmxtmrt driver to create a network device instance.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::CreateNetworkDevice( PXTM_ADDR pConnAddr,
                                                  char *pszNetworkDeviceName )
{
    BCMXTM_STATUS bxStatus;
    XTM_CONNECTION *pConn ;
    XTM_INTERFACE_LINK_INFO Li ;
    PXTM_INTERFACE_LINK_INFO pOtherLi = NULL ;
    UINT32 ulPort, ulBondingPort = MAX_INTERFACES;
    UINT32 ulPortMask;

    UpdateConnAddrForBonding ( pConnAddr, XTM_ADD ) ;
    pConn = m_ConnTable.Get( pConnAddr );

    if( pConn )
    {
        UINT32 ulLinkMask = 0 ;
        
        pConn->CreateNetworkDevice( pszNetworkDeviceName, 0 );

        /* Call connection to indicate the link is up where appropriate. */
        memset (&Li, 0, sizeof (XTM_INTERFACE_LINK_INFO)) ;
        
        ulPortMask = pConnAddr->u.Conn.ulPortMask ;
        
        for( ulPort = PHY_PORTID_0; ulPort < MAX_INTERFACES; ulPort++ )
        {
            if( ulPortMask & PORT_TO_PORTID(ulPort) )
            {
                PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[ulPort].GetLinkInfo();
                ulBondingPort = m_Interfaces[ulPort].GetBondingPortNum() ;
                if( pLi->ulLinkState == LINK_UP &&
                    pLi->ulLinkTrafficType == pConnAddr->ulTrafficType )
                {
                   ulLinkMask |= PORT_TO_PORTID(ulPort) ;
                   Li.ulLinkUsRate += pLi->ulLinkUsRate ;
                   Li.ulLinkDsRate += pLi->ulLinkDsRate ;
                   Li.ulLinkTrafficType = pLi->ulLinkTrafficType ;

                   if (ulBondingPort != MAX_INTERFACES)
                   {
                      pOtherLi = m_Interfaces[ulBondingPort].GetLinkInfo();
                      if (pOtherLi->ulLinkState == LINK_UP)
                         ulLinkMask |= PORT_TO_PORTID(ulBondingPort) ;
                      break ;   /* More than 2 ports in bonding not supported, so exiting */
                   }
                }
            }
        }

        if (ulLinkMask != 0)
        {
           UINT32 maxSeqDeviation ;
           
           Li.ulLinkState = LINK_UP ;
           maxSeqDeviation = ComputeSeqDeviation (ulPort, &Li, ulBondingPort, pOtherLi) ;
           pConn->SetLinkInfo( ulLinkMask, &Li, ulBondingPort, pOtherLi, maxSeqDeviation ) ;
        }

        bxStatus = XTMSTS_SUCCESS;
    }
    else
        bxStatus = XTMSTS_NOT_FOUND;

    UpdateConnAddrForBonding ( pConnAddr, XTM_REMOVE ) ;

    {
       /* Compile Out for Loopback */
       UINT32 ulLinkState = LINK_UP;
       ulPort = PORTID_TO_PORT(pConnAddr->u.Vcc.ulPortMask);

       if( ulPort < MAX_INTERFACES )
       {
          PXTM_INTERFACE_LINK_INFO pLi = m_Interfaces[ulPort].GetLinkInfo();
          ulLinkState = pLi->ulLinkState;
       }

       /* All external ports */
       if(bxStatus == XTMSTS_SUCCESS && ulLinkState == LINK_DOWN &&
          (XP_REGS->ulTxUtopiaCfg & TXUTO_MODE_INT_EXT_MASK) == TXUTO_MODE_ALL_EXT)
       {
          XTM_INTERFACE_LINK_INFO Li;

          Li.ulLinkState  = LINK_UP;
          Li.ulLinkUsRate = 54000000;
          Li.ulLinkDsRate = 99000000;
          Li.ulLinkTrafficType = pConnAddr->ulTrafficType ;

          ulPortMask = pConnAddr->u.Conn.ulPortMask ;

          for( ulPort = PHY_PORTID_0; ulPort < MAX_INTERFACES; ulPort++ )
          {
             if( ulPortMask & PORT_TO_PORTID(ulPort) )
             {
                XtmOsPrintf("bcmxtmcfg: Force link up, port=%lu, traffictype=%lu\n",
                            ulPort, Li.ulLinkTrafficType);
                BcmXtm_SetInterfaceLinkInfo( (ulPortMask & PORT_TO_PORTID(ulPort)), &Li );
             }
          }
          if( m_InitParms.bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE)
          {
             Li.ulLinkState  = LINK_UP;
             Li.ulLinkUsRate = 54000000;
             Li.ulLinkDsRate = 99000000;
             Li.ulLinkTrafficType = pConnAddr->ulTrafficType ;

             XtmOsPrintf("bcmxtmcfg: Force link up, port=%lu, traffictype=%lu\n",
                   ulPort, Li.ulLinkTrafficType);
             BcmXtm_SetInterfaceLinkInfo( 0x2, &Li );
          }
       }
    }

    return( bxStatus );
} /* CreateNetworkDevice */


/***************************************************************************
 * Function Name: SetDsPtmBondingDeviation
 * Description  : Call the bcmxtmrt driver to set the DS max deviation, which
 *                will take effect during the adaptiva windowing in the
 *                presence of data errors.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::SetDsPtmBondingDeviation ( UINT32 ulDeviation )
{
    (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_SET_DS_SEQ_DEVIATION, (UINT32 *) ulDeviation) ;
    return( XTMSTS_SUCCESS );
} /* SetDsPtmBondingDeviation */

/***************************************************************************
 * Function Name: DeleteNetworkDevice
 * Description  : Remove a bcmxtmrt network device.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::DeleteNetworkDevice( PXTM_ADDR pConnAddr )
{
    BCMXTM_STATUS bxStatus;
    XTM_CONNECTION *pConn ;

    UpdateConnAddrForBonding ( pConnAddr, XTM_ADD ) ;

    pConn = m_ConnTable.Get( pConnAddr );

    if( pConn )
    {
        pConn->DeleteNetworkDevice();

        /* The connection's configuration was previously removed. */
        if( pConn->RemovePending() )
        {
            m_ConnTable.Remove( pConn );
            delete pConn;
        }
        bxStatus = XTMSTS_SUCCESS;
    }
    else
        bxStatus = XTMSTS_NOT_FOUND;

    UpdateConnAddrForBonding ( pConnAddr, XTM_REMOVE ) ;
    return( bxStatus );
} /* DeleteNetworkDevice */

/***************************************************************************
 * Function Name: GetBondingInfo
 * Description  : Returns the bonding information for the system, if applicable.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::GetBondingInfo ( PXTM_BOND_INFO pBondInfo )
{
    BCMXTM_STATUS bxStatus;
    PXTM_INTERFACE_LINK_INFO pLinkInfo [MAX_BOND_PORTS] ;
    UINT32 portSet[MAX_BOND_PORTS], ulPort ;

    /* Currently, single latency - single group information is supported and
     * returned, if available.
     * BACP for PTM bonding is not supported yet.
     */
    memset (pBondInfo, 0, sizeof (XTM_BOND_INFO)) ;

    pBondInfo->u8MajorVersion = XTM_BOND_MAJ_VER ;
    pBondInfo->u8MinorVersion = XTM_BOND_MIN_VER ;
    pBondInfo->u8BuildVersion = XTM_BOND_BUILD_VER ;

    portSet[0] = 0x0 ;
    ulPort = portSet[0] ;

    pLinkInfo[0] = m_Interfaces [ulPort].GetLinkInfo () ;
    if (pLinkInfo[0]->ulLinkState == LINK_UP)
	    pBondInfo->ulTrafficType = pLinkInfo[0]->ulLinkTrafficType ;
    else
	    pBondInfo->ulTrafficType = TRAFFIC_TYPE_NOT_CONNECTED ;
    bxStatus = XTMSTS_NOT_SUPPORTED ;
    return( bxStatus ) ;
}

/***************************************************************************
 * Function Name: CheckTrafficCompatibility (6368/62/28)
 * Description  : Called for 6368 as currently we cannot support on the fly
 *                conversion from non-bonded to bonded and vice versa
 *                traffic types. This function enforces this condition.
 * Returns      : None.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::CheckTrafficCompatibility ( UINT32 ulTrafficType )
{
	BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS ;

   switch (ulTrafficType) {
      case TRAFFIC_TYPE_PTM_BONDED     :
         if ( m_InitParms.bondConfig.sConfig.ptmBond != BC_PTM_BONDING_ENABLE)
            bxStatus = XTMSTS_PROTO_ERROR ;
         break ;
      case TRAFFIC_TYPE_ATM_BONDED     :
         if ( m_InitParms.bondConfig.sConfig.atmBond != BC_ATM_BONDING_ENABLE)
            bxStatus = XTMSTS_PROTO_ERROR ;
         break ;
      case TRAFFIC_TYPE_PTM            :
         if ( m_InitParms.bondConfig.sConfig.ptmBond == BC_PTM_BONDING_ENABLE)
            bxStatus = XTMSTS_PROTO_ERROR ;
         break ;
      case TRAFFIC_TYPE_ATM            :
         if ( m_InitParms.bondConfig.sConfig.atmBond == BC_PTM_BONDING_ENABLE)
            bxStatus = XTMSTS_PROTO_ERROR ;
         break ;
      default                          :
         break ;
   }

	return (bxStatus) ;
} /* CheckTrafficCompatibility */


/***************************************************************************
 * Function Name: CheckAndSetIfLinkInfo (6368/62/28)
 * Description  : Called for 6368 as bonding requires rx side to be configured 
 *                intelligently based on the other bonding pair's activeness.
 *		            If bonding enabled, (only for 6368), set the rx port status only
 *			         f all the bonded ports are down, otherwise, do not disable any
 *			         ort. This is due to the SAR workaround we have in the phy for
 *			         ptm bonding that all traffic will always come on channel 0.
 *			         Also a case for atm bonding, where port 0 goes down, the
 *			         entire traffic stops as the scheduler gets disabled on the
 *			         port 0. We cannot disable the scheduler for atm bonding
 *			         operating port until all the ports are down.
 *			   
 * Returns      : None.
 ***************************************************************************/
BCMXTM_STATUS XTM_PROCESSOR::CheckAndSetIfLinkInfo ( UINT32 ulLogicalPort,
	 			PXTM_INTERFACE_LINK_INFO pLinkInfo, UINT32 ulBondingPort,
	 			PXTM_INTERFACE_LINK_INFO pOtherLinkInfo)
{
	UINT32 j ;
	BCMXTM_STATUS bxStatus ;

	if (ulBondingPort != MAX_INTERFACES) {
      if (pLinkInfo->ulLinkTrafficType != TRAFFIC_TYPE_ATM_BONDED) {

#ifdef PTMBOND_DS_UNI_CHANNEL
			if( pLinkInfo->ulLinkState == LINK_UP ) {

				if (ulLogicalPort == DS_PTMBOND_CHANNEL) {
					bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, 
							XTM_CONFIGURE, XTM_CONFIGURE, XTM_CONFIGURE );
				}
				else {

					bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_NO_CONFIGURE,
							XTM_NO_CONFIGURE, XTM_CONFIGURE, XTM_CONFIGURE );

					/* Use the same link information (spoof for rx side config) */
					if (pOtherLinkInfo->ulLinkState == LINK_DOWN)
						bxStatus = m_Interfaces[ulBondingPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, 
								XTM_CONFIGURE, XTM_NO_CONFIGURE, XTM_NO_CONFIGURE );
				}
			}
         else {
            /* Link down */
            bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, 
                  XTM_CONFIGURE, XTM_CONFIGURE, XTM_CONFIGURE );
            if ((ulLogicalPort == DS_PTMBOND_CHANNEL) && (pOtherLinkInfo->ulLinkState == LINK_UP)) {
               bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pOtherLinkInfo, XTM_CONFIGURE, 
                     XTM_CONFIGURE, XTM_NO_CONFIGURE, XTM_NO_CONFIGURE );
            }
         }
#else
      bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_CONFIGURE,
                                                          XTM_CONFIGURE, XTM_CONFIGURE );
#endif
      }
      else {
         j = XP_REGS->ulRxSarCfg ;
         j &= ~RXSARA_BOND_CELL_COUNT_MASK ;

         if( pLinkInfo->ulLinkState == LINK_UP ) {
            if (pOtherLinkInfo->ulLinkState == LINK_DOWN) {
               j |= 0x00010000 ; /* Start at SID=0, 16 outstanding cells for missing cells */
               XP_REGS->ulRxSarCfg = j ;
            bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_CONFIGURE, 
                     XTM_CONFIGURE, XTM_CONFIGURE );
               bxStatus = m_Interfaces[ulBondingPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_NO_CONFIGURE,
                     XTM_NO_CONFIGURE, XTM_NO_CONFIGURE );
            }
            else {
               m_Interfaces[ulBondingPort].SetUTStatus (LINK_DOWN) ;
               bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_NO_CONFIGURE,
                     XTM_CONFIGURE, XTM_CONFIGURE );
               bxStatus = m_Interfaces[ulBondingPort].SetLinkInfo( pLinkInfo, XTM_NO_CONFIGURE, XTM_NO_CONFIGURE,
                     XTM_CONFIGURE, XTM_CONFIGURE );
               m_Interfaces[ulLogicalPort].SetUTStatus (LINK_DOWN) ;
               j |= 0x01800000 ; /* Start at SID=0, 16 outstanding cells for missing cells */
               XP_REGS->ulRxSarCfg = j ;
               m_Interfaces[ulBondingPort].SetUTStatus (LINK_UP) ;
               m_Interfaces[ulLogicalPort].SetUTStatus (LINK_UP) ;
            }
            XtmOsPrintf ("bcmxtmcfg: ATMB RxSARCfg = %x \n", j) ;
         }
         else {
            /* Only for single latency bonding.
             * For dual latency bonding, we need to pair 0 & 2 for a group and
             * 1 & 3 for another group.
             * For single latency, 0 & 1 are paired. These are derived from
             * HW design/assumptions.
             */

            if (pOtherLinkInfo->ulLinkState == LINK_UP) {

               m_Interfaces[ulBondingPort].SetUTStatus (LINK_DOWN) ;
               m_Interfaces[ulLogicalPort].SetUTStatus (LINK_DOWN) ;
               j |= 0x00010000 ; /* Start at SID=Write SID, 16 outstanding cells for missing cells */
				   //j &= ~RXSARA_BOND_EN ;
               //XP_REGS->ulRxSarCfg = j ;
               //j |= RXSARA_BOND_EN ;
               XP_REGS->ulRxSarCfg = j ;

               bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_NO_CONFIGURE, 
                                            XTM_CONFIGURE, XTM_CONFIGURE, XTM_NO_CONFIGURE );
               m_Interfaces[ulBondingPort].SetUTStatus (LINK_UP) ;
				   //XtmOsStartTimer ((void *) BondXtmRestartTimerCb, (UINT32) this,
						              //XTM_BOND_XTM_RESTART_TIMEOUT_MS) ;
            }
            else {
               m_Interfaces[ulLogicalPort].SetUTStatus (LINK_DOWN) ;
               j |= 0x00010000 ; /* Start at SID=Write SID, 16 outstanding cells for missing cells */
               XP_REGS->ulRxSarCfg = j ;
               m_Interfaces[ulLogicalPort].SetUTStatus (LINK_UP) ;

               bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_CONFIGURE,
                                          XTM_CONFIGURE, XTM_CONFIGURE);
            }

            XtmOsPrintf ("bcmxtmcfg: ATMB RxSARCfg = %x \n", j) ;
			}
		}

      if( pOtherLinkInfo->ulLinkState == LINK_UP )
         m_Interfaces[ulBondingPort].UpdateLinkDelay () ; 
	}
	else {
      bxStatus = m_Interfaces[ulLogicalPort].SetLinkInfo( pLinkInfo, XTM_CONFIGURE, XTM_CONFIGURE,
                                                          XTM_CONFIGURE, XTM_CONFIGURE );
	}

	return (bxStatus) ;
} /* CheckAndSetIfLinkInfo */

void XTM_PROCESSOR::CheckAndResetSAR( UINT32 ulPortId,
    PXTM_INTERFACE_LINK_INFO pLinkInfo )
{
   int i;

    if( pLinkInfo->ulLinkState == LINK_DOWN )
    {
        /* Reset in order to clear internal buffers. */
        PERF->softResetB &= ~SOFT_RST_SAR;

        for (i=50 ; i >0 ; i--)
           ;

        PERF->softResetB |= SOFT_RST_SAR;
        Initialize( NULL, 0, NULL );
    }
}


/* Reinitializes the SAR with configuration. Used in debugging SAR issue(s)
 * with errorneous traffic in US/DS direction from/to phy.
 * Assumes, SAR with one/two ports, 1 PTM LOW priority interface with a
 * corresponding connection.
 * Number of Tx Qs can be any. (from pre-existing)
 */
BCMXTM_STATUS XTM_PROCESSOR::DebugReInitialize ()
{
   UINT32 i ;
   UINT32 ulPort = 0x0, ulOtherPort, ulOrigLinkState, ulOrigOtherLinkState ;
   PXTM_INTERFACE_LINK_INFO pLiPort, pLiOtherPort ;
   XTM_INTERFACE_LINK_INFO origLiPort, origLiOtherPort ;
   XTM_ADDR xtmAddr ;
   XTM_CONN_CFG origConnCfg ;
   BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS ;

   pLiPort      = m_Interfaces [ulPort].GetLinkInfo() ;

   if (pLiPort) {

      memcpy (&origLiPort, pLiPort, sizeof (XTM_INTERFACE_LINK_INFO)) ;
      ulOrigLinkState      = pLiPort->ulLinkState ;
      ulOtherPort = m_Interfaces [ulPort].GetBondingPortNum() ;
      if (ulOtherPort != MAX_INTERFACES) {
         pLiOtherPort = m_Interfaces [ulOtherPort].GetLinkInfo() ;
         memcpy (&origLiOtherPort, pLiOtherPort, sizeof (XTM_INTERFACE_LINK_INFO)) ;
         ulOrigOtherLinkState = pLiOtherPort->ulLinkState ;
      }
      else {
         pLiOtherPort = NULL ;
         ulOrigOtherLinkState = LINK_DOWN ;
      }
   }
   else {
      ulOrigLinkState = LINK_DOWN ;
      ulOtherPort = MAX_INTERFACES ;
      pLiOtherPort = NULL ;
      ulOrigOtherLinkState = LINK_DOWN ;
   }

   if (ulOrigLinkState == LINK_UP) {
      pLiPort->ulLinkState = LINK_DOWN ;
      pLiPort->ulLinkTrafficType = TRAFFIC_TYPE_PTM ;
      bxStatus = BcmXtm_SetInterfaceLinkInfo (PORT_TO_PORTID(ulPort), pLiPort) ;
   }

   if ((bxStatus == XTMSTS_SUCCESS) && (ulOrigOtherLinkState == LINK_UP)) {
      pLiOtherPort->ulLinkState = LINK_DOWN ;
      pLiOtherPort->ulLinkTrafficType = TRAFFIC_TYPE_PTM ;
      bxStatus = BcmXtm_SetInterfaceLinkInfo (PORT_TO_PORTID(ulOtherPort), pLiOtherPort) ;
   }

   xtmAddr.ulTrafficType =  TRAFFIC_TYPE_PTM ;
   xtmAddr.u.Flow.ulPortMask = PORT_PHY0_PATH0 ;
   xtmAddr.u.Flow.ulPtmPriority = PTM_PRI_LOW ;

   if (bxStatus == XTMSTS_SUCCESS)
      bxStatus = BcmXtm_DeleteNetworkDevice (&xtmAddr) ;

   if (bxStatus == XTMSTS_SUCCESS) {
      bxStatus = BcmXtm_GetConnCfg (&xtmAddr, &origConnCfg) ;
      if (bxStatus == XTMSTS_SUCCESS) 
         bxStatus = BcmXtm_SetConnCfg (&xtmAddr, NULL) ;  /* Remove the connection */
   }

   for (i=99999999; i > 0 ; i--)
      ;

   if ((bxStatus == XTMSTS_SUCCESS) && (ulOrigOtherLinkState == LINK_UP)) {

      pLiOtherPort->ulLinkState = LINK_UP ;
      pLiOtherPort->ulLinkTrafficType = TRAFFIC_TYPE_PTM_BONDED ;
      bxStatus = BcmXtm_SetInterfaceLinkInfo (PORT_TO_PORTID(ulOtherPort), &origLiOtherPort) ;

      if (bxStatus == XTMSTS_SUCCESS) 
         bxStatus = BcmXtm_SetConnCfg (&xtmAddr, &origConnCfg) ; /* ReConfigure the connection */

      if (bxStatus == XTMSTS_SUCCESS)
         bxStatus = BcmXtm_CreateNetworkDevice (&xtmAddr, (char *) "ptm0") ;
   }

   for (i=99999; i > 0 ; i--)
      ;

   if ((bxStatus == XTMSTS_SUCCESS) && (ulOrigLinkState == LINK_UP)) {

      pLiPort->ulLinkState = LINK_UP ;
      pLiPort->ulLinkTrafficType = TRAFFIC_TYPE_PTM_BONDED ;
      bxStatus = BcmXtm_SetInterfaceLinkInfo (PORT_TO_PORTID(ulPort), &origLiPort) ;

      if (ulOrigOtherLinkState != LINK_UP) {

         if (bxStatus == XTMSTS_SUCCESS) 
            bxStatus = BcmXtm_SetConnCfg (&xtmAddr, &origConnCfg) ; /* ReConfigure the connection */

         if (bxStatus == XTMSTS_SUCCESS)
            bxStatus = BcmXtm_CreateNetworkDevice (&xtmAddr, (char *) "ptm0") ;
      }
   }

   return (bxStatus) ;
}
