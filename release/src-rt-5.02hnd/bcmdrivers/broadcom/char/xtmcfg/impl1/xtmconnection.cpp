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
 * File Name  : xtmconnection.cpp
 *
 * Description: This file contains the implementation for the XTM connection
 *              and connection table classes.
 ***************************************************************************/

#include "xtmcfgimpl.h"
#include "board.h"
#include "bcmadsl.h"

#if defined(ADSLMIBDEF_H_VER) && (ADSLMIBDEF_H_VER  >  0)
typedef void (*FN_ADSL_WAN_DEV_NOTIFY) (unsigned char bDevUp);
extern "C" {
extern FN_ADSL_WAN_DEV_NOTIFY g_pfnAdslWanDevState ;
}
#endif


UINT32 XTM_CONNECTION::ms_ulSitUt = ADSL_SIT_UNIT_OF_TIME;
UINT32 XTM_CONNECTION::ms_ulBusSpeed = UBUS_BASE_FREQUENCY_IN_MHZ;

/***************************************************************************
 * Function Name: XTM_CONNECTION
 * Description  : Constructor for the XTM connection class.
 * Returns      : None.
 ***************************************************************************/
XTM_CONNECTION::XTM_CONNECTION( FN_XTMRT_REQ pfnXtmrtReq,
    XTM_CONNECTION_TABLE *pConnTable, UINT32 ulConnSem, void *pSoftSar,
    UINT32 *pulRxVpiVciCamShadow)
{
    m_pfnXtmrtReq = pfnXtmrtReq;
    m_pConnTable = pConnTable;
    m_ulConnSem = ulConnSem;
    m_pSoftSar = pSoftSar;
    m_pulRxVpiVciCamShadow = pulRxVpiVciCamShadow;
    m_hDev = INVALID_HANDLE;
    m_ulCreated = 0;
    m_ulConnected = 0;
    m_ulRemovePending = 0;
    m_ulTxQIdsSize = 0;
    m_pTrafficDescrEntries = NULL;
    m_ulNumTrafficDescrEntries = 0;
    m_ulPacketCmfShaperIdx = 0;
    m_ulPacketCmfTxChanIdx = 0;
    memset(m_szNetDeviceName, 0x00, sizeof(m_szNetDeviceName));
    memset(&m_Addr, 0x00, sizeof(m_Addr));
    memset(&m_Cfg, 0x00, sizeof(m_Cfg));
    memset(m_ucVcids, INVALID_VCID, sizeof(m_ucVcids));
    memset(m_ucVcidsTx, INVALID_VCID, sizeof(m_ucVcidsTx));
    memset(m_ulMpIdxs, INVALID_INDEX, sizeof(m_ulMpIdxs));
    memset(m_ulVcidsCount, 0x00, sizeof(m_ulVcidsCount));
    memset(m_TxQIds, 0x00, sizeof(m_TxQIds));

    m_ulHwAddHdr = (XtmOsChipRev() == CHIP_REV_BCM6368B0) ? 0 : 1;
    m_ulHwRemoveHdr = 0;
} /* XTM_CONNECTION */


/***************************************************************************
 * Function Name: ~XTM_CONNECTION
 * Description  : Destructor for the XTM connection class.
 * Returns      : None.
 ***************************************************************************/
XTM_CONNECTION::~XTM_CONNECTION( void )
{
    DeleteNetworkDevice();
} /* ~XTM_CONNECTION */


/***************************************************************************
 * Function Name: GetAddr
 * Description  : Returns the connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::GetAddr( PXTM_ADDR pAddr )
{
    memcpy(pAddr, &m_Addr, sizeof(m_Addr));
    return( XTMSTS_SUCCESS );
} /* GetAddr */


/***************************************************************************
 * Function Name: SetAddr
 * Description  : Sets the connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::SetAddr( PXTM_ADDR pAddr )
{
    memcpy(&m_Addr, pAddr, sizeof(m_Addr));
    return( XTMSTS_SUCCESS );
} /* SetAddr */


/***************************************************************************
 * Function Name: GetCfg
 * Description  : Returns the connection's configuration.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::GetCfg( PXTM_CONN_CFG pCfg )
{
    memcpy(pCfg, &m_Cfg, sizeof(m_Cfg));
    pCfg->ulOperStatus = OPRSTS_DOWN;

    /* The operational status of the connection depends on the link status and
     * bcmxtmrt device state.
     */
    if( m_ulConnected && m_Cfg.ulAdminStatus == ADMSTS_UP )
    {
        UINT32 ulOpenState = XTMRT_DEV_CLOSED;
        (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_GET_DEVICE_STATE, &ulOpenState);
        if( ulOpenState == XTMRT_DEV_OPENED )
            pCfg->ulOperStatus = OPRSTS_UP;
    }

    return( XTMSTS_SUCCESS );
} /* GetCfg */


/***************************************************************************
 * Function Name: SetCfg
 * Description  : Sets the connection's configuration.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::SetCfg( PXTM_CONN_CFG pCfg )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;

    /* A NULL configuration means the configuration is being deleted. */
    if( pCfg )
    {
        /* Validate transmit queue port id parameter. */
        for( UINT32 i = 0; i < pCfg->ulTransmitQParmsSize; i++ )
        {
            UINT32 ulPid = pCfg->TransmitQParms[i].ulPortId;
            UINT32 ulPortMask = m_Addr.u.Conn.ulPortMask ;
            if( (ulPid & ulPortMask) == 0 )
            {
                bxStatus = XTMSTS_PARAMETER_ERROR;
                break;
            }
        }

        if (bxStatus == XTMSTS_SUCCESS)
        {
            if(( m_Addr.ulTrafficType == TRAFFIC_TYPE_PTM ) ||
               ( m_Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED))
            {
                /* Validate PTM priority parameter. */
                for( UINT32 i = 0; i < pCfg->ulTransmitQParmsSize; i++ )
                {
                    UINT32 ulPtmPri = pCfg->TransmitQParms[i].ulPtmPriority;
                    if( (ulPtmPri & m_Addr.u.Flow.ulPtmPriority) == 0 ||
                         ulPtmPri == (PTM_PRI_LOW | PTM_PRI_HIGH) )
                    {
                        bxStatus = XTMSTS_PARAMETER_ERROR;
                        break;
                    }
                }
            }
        }

        if( bxStatus == XTMSTS_SUCCESS )
        {
            /*If admin status is not set to a valid value, set it to enabled.*/
            if( pCfg->ulAdminStatus != ADMSTS_DOWN )
                pCfg->ulAdminStatus = ADMSTS_UP;

            if( m_Cfg.ulAdminStatus != ADMSTS_DOWN )
                m_Cfg.ulAdminStatus = ADMSTS_UP;

            if( m_hDev != INVALID_HANDLE &&
                pCfg->ulAdminStatus != m_Cfg.ulAdminStatus )
            {
                (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_SET_ADMIN_STATUS,
                    (UINT32 *) pCfg->ulAdminStatus);
                m_Cfg.ulLastChange = XtmOsTickGet() / 10;
            }

            memcpy(&m_Cfg, pCfg, sizeof(m_Cfg));
            m_ulRemovePending = 0;

            if( m_ulConnected ) 
                CheckTransmitQueues();
        }
    }
    else
        m_ulRemovePending = 1;

    return( bxStatus );
} /* SetCfg */


/***************************************************************************
 * Function Name: SetTdt
 * Description  : Saves the Traffic Descriptor Table pointer and reconfigures
 *                the connection's shaper(s) if it is connected.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::SetTdt(
    PXTM_TRAFFIC_DESCR_PARM_ENTRY pTrafficDescrEntries,
    UINT32 ulNumTrafficDescrEntries )
{
    m_pTrafficDescrEntries = pTrafficDescrEntries;
    m_ulNumTrafficDescrEntries = ulNumTrafficDescrEntries;

    if( m_ulConnected )
    {
        UINT32 i;
        PXTM_TRANSMIT_QUEUE_PARMS pTxQParms;
        PXTMRT_TRANSMIT_QUEUE_ID pTxQId;
        for( i = 0, pTxQParms = m_Cfg.TransmitQParms, pTxQId = m_TxQIds;
             i < m_ulTxQIdsSize; i++, pTxQParms++, pTxQId++ )
        {
            ConfigureShaper( pTxQId->ulQueueIndex, pTxQParms ) ;
        }

        /* Update PacketCMF channel. */
        if( SPB_VCID_BEING_USED != INVALID_VCID &&
            m_ulVcidsCount[SPB_VCID_BEING_USED] )
        {
            CopyPacketCmfShaper( m_TxQIds[0].ulQueueIndex ) ;
        }
    }

    return( XTMSTS_SUCCESS );
} /* SetTdt */


/***************************************************************************
 * Function Name: SetLinkInfo
 * Description  : Processes a "link UP" or "link down" condition.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::SetLinkInfo( UINT32 ulLinkMask,
                                           PXTM_INTERFACE_LINK_INFO pLinkInfo,
                                           UINT32 ulBondingPort,
                                           PXTM_INTERFACE_LINK_INFO pOtherLinkInfo,
                                           UINT32 maxSeqDeviation)
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;

    if( m_hDev != INVALID_HANDLE )
    {
       XTMRT_LINK_STATUS_CHANGE Lsc;

       memset(&Lsc, 0x00, sizeof(Lsc));

       Lsc.ulLinkDataMask = ulLinkMask ;

       Lsc.ulDsSeqDeviation =  maxSeqDeviation ;
       Lsc.ulLinkState = pLinkInfo->ulLinkState;
       Lsc.ulTrafficType = pLinkInfo->ulLinkTrafficType ;

       if (pOtherLinkInfo == NULL) {
          Lsc.ulLinkUsRate = pLinkInfo->ulLinkUsRate;
          Lsc.ulLinkDsRate = pLinkInfo->ulLinkDsRate;
          Lsc.ulOtherLinkUsRate = 0;
          Lsc.ulOtherLinkDsRate = 0;
       }
       else {
          if (ulBondingPort != PHY_PORTID_0) {
             Lsc.ulLinkUsRate = pLinkInfo->ulLinkUsRate;
             Lsc.ulLinkDsRate = pLinkInfo->ulLinkDsRate;
             Lsc.ulOtherLinkUsRate = pOtherLinkInfo->ulLinkUsRate;
             Lsc.ulOtherLinkDsRate = pOtherLinkInfo->ulLinkDsRate;
          }
          else {
             Lsc.ulLinkUsRate = pOtherLinkInfo->ulLinkUsRate;
             Lsc.ulLinkDsRate = pOtherLinkInfo->ulLinkDsRate;
             Lsc.ulOtherLinkUsRate = pLinkInfo->ulLinkUsRate;
             Lsc.ulOtherLinkDsRate = pLinkInfo->ulLinkDsRate;
          }
       }

       if( pLinkInfo->ulLinkState == LINK_UP )
       {
          if (!m_ulConnected) {

             /* Link up. Reserve vcids and transmit queues. */
             bxStatus = ReserveVcidEntries();

             memset(m_ulMpIdxs, INVALID_INDEX, sizeof(m_ulMpIdxs));

             if( bxStatus == XTMSTS_SUCCESS )
             {
                m_ulTxQIdsSize = 0;
                bxStatus = ReserveTxShapers(m_Cfg.TransmitQParms,
                      m_Cfg.ulTransmitQParmsSize);
             }

             if( bxStatus == XTMSTS_SUCCESS )
             {
                UINT32 i;
                UINT32 ulBondingPort = MAX_INTERFACES ;

                if (m_Cfg.TransmitQParms[0].ulBondingPortId  != PORT_PHY_INVALID) {

                   /* Reserve HW Shapers */
                   ReserveTxHwShapers(m_Cfg.TransmitQParms, m_Cfg.ulTransmitQParmsSize) ;
                   ulBondingPort = PORTID_TO_PORT(m_Cfg.TransmitQParms[0].ulBondingPortId) ;
                }

                for( i = 0; i < MAX_VCIDS && bxStatus == XTMSTS_SUCCESS; i++ ) {
                   if( m_ulVcidsCount[i] > 1 ) {
                      bxStatus = ReserveMultiPriorityIndex( i, ulBondingPort );
                   }
                }
             }

             if( bxStatus == XTMSTS_SUCCESS )
             {
                Lsc.ulTransmitQueueIdsSize = m_ulTxQIdsSize;
                memcpy(Lsc.TransmitQueueIds, m_TxQIds, m_ulTxQIdsSize *
                      sizeof(XTMRT_TRANSMIT_QUEUE_ID));
                Lsc.ulRxVcidsSize = CopyVcids(Lsc.ucRxVcids);
                Lsc.ucTxVcid = Lsc.ucRxVcids[0];

                if( pLinkInfo->ulLinkTrafficType == TRAFFIC_TYPE_PTM_RAW )
                   Lsc.ulLinkState |= LSC_RAW_ENET_MODE;

                if( (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_LINK_STATUS_CHANGED,
                         &Lsc) == 0 )
                {
                   m_ulConnected = 1;
                }
                else
                   bxStatus = XTMSTS_ERROR;
             }

             if( bxStatus != XTMSTS_SUCCESS ) {
                if (m_Cfg.TransmitQParms[0].ulBondingPortId  != PORT_PHY_INVALID)
                   UnreserveTxHwShapers () ;
                UnreserveVcidsAndTxShapers();
                m_ulConnected = 0 ;
             }
          }
          else {
             /* Connection is already connected */
             if( (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_LINK_STATUS_CHANGED,
                      &Lsc) != 0 )
             {
                bxStatus = XTMSTS_ERROR;
             }
          }
       } /* if (LINK_UP) */
       else {
          if( m_ulConnected )
          {
             if (ulLinkMask == 0) {
                /* Link down. Unreserve vcids and transmit queues. */
                Lsc.ulTransmitQueueIdsSize = m_ulTxQIdsSize;
                memcpy(Lsc.TransmitQueueIds, m_TxQIds, m_ulTxQIdsSize *
                      sizeof(XTMRT_TRANSMIT_QUEUE_ID));
                Lsc.ulRxVcidsSize = CopyVcids(Lsc.ucRxVcids);
                Lsc.ucTxVcid = Lsc.ucRxVcids[0];

                (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_LINK_STATUS_CHANGED, &Lsc);
                m_ulConnected = 0;

                if (m_Cfg.TransmitQParms[0].ulBondingPortId  != PORT_PHY_INVALID)
                   UnreserveTxHwShapers () ;
                UnreserveVcidsAndTxShapers();
             }
             else {
                /* One or more links are UP in the group */
                (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_LINK_STATUS_CHANGED, &Lsc);
             }

             bxStatus = XTMSTS_SUCCESS;
          }
          else
             bxStatus = XTMSTS_STATE_ERROR;
       } /* if (LINK_DOWN) */

		 if (m_ulConnected)
       	XtmOsPrintf ("bcmxtmcfg: Connection UP, LinkActiveStatus=0x%x, US=%d, DS=%d \n",
                   	 ulLinkMask,
                      (pOtherLinkInfo ? (pLinkInfo->ulLinkUsRate+pOtherLinkInfo->ulLinkUsRate) :
                       pLinkInfo->ulLinkUsRate), 
                      (pOtherLinkInfo?(pLinkInfo->ulLinkDsRate+pOtherLinkInfo->ulLinkDsRate) :
                       pLinkInfo->ulLinkDsRate)) ;
		 else
       	XtmOsPrintf ("bcmxtmcfg: Connection DOWN, LinkActiveStatus=0x%x \n",
                   	 ulLinkMask) ;

    } /* if( m_hDev != INVALID_HANDLE ) */

    return( bxStatus );
} /* SetLinkInfo */


/***************************************************************************
 * Function Name: CreateNetworkDevice
 * Description  : Calls the bcmxtmrt driver to create an operating system
 *                network device instance.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::CreateNetworkDevice( char *pszNetDeviceName, UINT32 ulTxPafEnabled )
{
    BCMXTM_STATUS bxStatus;

    if( !m_ulCreated )
    {
        XTMRT_CREATE_NETWORK_DEVICE Cni;

        memcpy(&Cni.ConnAddr, &m_Addr, sizeof(m_Addr));
        strcpy(Cni.szNetworkDeviceName, pszNetDeviceName);

        Cni.ulHeaderType = m_Cfg.ulHeaderType;
        Cni.ulFlags = 0;
        Cni.ulTxPafEnabled = ulTxPafEnabled;
        if( m_ulHwAddHdr == 1 )
            Cni.ulFlags |= CNI_HW_ADD_HEADER;
        if( m_ulHwRemoveHdr == 1 )
            Cni.ulFlags |= CNI_HW_REMOVE_HEADER;
        if( (XP_REGS->ulTxSarCfg & TXSAR_USE_ALT_FSTAT) == TXSAR_USE_ALT_FSTAT )
            Cni.ulFlags |= CNI_USE_ALT_FSTAT;
        if( ((XP_REGS->ulRxAalCfg & RXAALP_CELL_LENGTH_MASK) >>
            RXAALP_CELL_LENGTH_SHIFT) > 0 )
            Cni.ulFlags |= CNI_HW_REMOVE_TRAILER;

        if( (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_CREATE_DEVICE, &Cni) == 0 )
        {
            strcpy(m_szNetDeviceName, pszNetDeviceName);
            m_hDev = Cni.hDev;
            m_ulCreated = 1;
            bxStatus = XTMSTS_SUCCESS;

            /* Inform the DSL driver if the "ptm0" interface goes up */

            if (!strcmp (m_szNetDeviceName, "ptm0")) {

               if ( g_pfnAdslWanDevState != NULL ) {
                  g_pfnAdslWanDevState (1) ; /* 1 for up */
               }
            }
        }
        else
            bxStatus = XTMSTS_RESOURCE_ERROR;
    }
    else
        bxStatus = XTMSTS_STATE_ERROR;

    return( bxStatus );
} /* CreateNetworkDevice */


/***************************************************************************
 * Function Name: DeleteNetworkDevice
 * Description  : Calls the bcmxtmrt driver to delete an operating system
 *                network device.
 *                Also calls the DSL driver to notify WAN "ptm0" interface down
 *                condition.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::DeleteNetworkDevice( void )
{
    BCMXTM_STATUS bxStatus;

    if( m_ulConnected )
    {
        XTM_INTERFACE_LINK_INFO LinkInfo;

        memset(&LinkInfo, 0x00, sizeof(LinkInfo));
        LinkInfo.ulLinkState = LINK_DOWN;
        SetLinkInfo( 0x0, &LinkInfo, 0, NULL, XTM_DEF_DS_MAX_DEVIATION );
    }

    if( m_ulCreated )
    {

        /* Inform the DSL driver if the "ptm0" interface goes down */

        if (!strcmp (m_szNetDeviceName, "ptm0")) {

           if ( g_pfnAdslWanDevState != NULL ) {
              g_pfnAdslWanDevState (0) ; /* 0 for down */
           }
        }

        (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_DELETE_DEVICE, NULL);
        m_szNetDeviceName[0] = '\0';
        m_hDev = INVALID_HANDLE;
        m_ulCreated = 0;
        bxStatus = XTMSTS_SUCCESS;
    }
    else
        bxStatus = XTMSTS_STATE_ERROR;

    return( bxStatus );
} /* DeleteNetworkDevice */

BCMXTM_STATUS XTM_CONNECTION::ReserveTxQueue (UINT32 *pulQueueIndex)
{
   BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;

   *pulQueueIndex = INVALID_INDEX ;

   bxStatus = m_pConnTable->ReserveTxQIndex (pulQueueIndex) ;

   if (bxStatus == XTMSTS_SUCCESS)
      m_ulTxQIdsSize++ ;

   return( bxStatus );
} /* ReserveTxQueue */

/***************************************************************************
 * Function Name: ReserveVcidEntries (6368, 6362, 6328)
 * Description  : Reserves Rx CAM and, for ATM, Tx Table entries.  The index
 *                into the Rx CAM and Tx Table is called a "vcid".  The same
 *                vcid indicies are used for both the Rx CAM and Tx Table.
 *
 *                The Rx CAM uses vcid for both ATM and PTM, and includes port
 *                in the CAM entry.  Tx Table only uses vcid for ATM and does
 *                not include port in a Tx Table entry.  Since the same vcids
 *                are used for both the Rx CAM and Tx Table, the Tx Table may
 *                have multiple entries with the same value (same VPI/VCI).
 *                This is redundant but does (should) not create problems.
 *
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReserveVcidEntries( void )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    UINT32 ulPort;
    UINT32 ulPortMask = m_Addr.u.Conn.ulPortMask ;

    for( ulPort = PHY_PORTID_0;
         (ulPort < MAX_INTERFACES) && (bxStatus == XTMSTS_SUCCESS);
         ulPort++ )
    {
        if( (ulPortMask & PORT_TO_PORTID(ulPort)) != 0 )
        {
            /* The connection uses this port.  Find an available vcid. */
            if( (m_Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
            {
                if (m_Addr.ulTrafficType == TRAFFIC_TYPE_ATM_BONDED)
                {
                    /* For atm bonded ports, we dont need duplicate vcid configuration
                     * for odd ports (bonded pairs...) on the Rx/Tx side. 
                     * HW does not work with this kind of configuration.
                     * Dual latency - todo
                     */
                    if (ulPort == PHY_PORTID_0)
                        bxStatus = ReserveAtmVcidEntry( ulPort, XTM_CONFIGURE );
                }
                else
                {
                    bxStatus = ReserveAtmVcidEntry( ulPort, XTM_CONFIGURE );
                }
            }
            else
            {
                /* If the port is 0 and the TEQ port is 0, then configure
                 * receive for "raw" mode.
                 */
                if( ulPort == 0 &&
                    ((XP_REGS->ulRxUtopiaCfg & RXUTO_TEQ_PORT_MASK) >>
                    RXUTO_TEQ_PORT_SHIFT) == 0 )
                {
                    bxStatus = ReserveRawVcidEntry( ulPort );
                }
                else
                {
                    if (m_Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)
                    {
                        /* For atm bonded ports, we dont need duplicate vcid configuration
                         * for odd ports (bonded pairs...) on the Rx/Tx side. 
                         * HW does not work with this kind of configuration.
                         * Dual latency - todo
                         */
#ifdef PTMBOND_DS_UNI_CHANNEL
                        if (ulPort == DS_PTMBOND_CHANNEL)
#endif
                            bxStatus = ReservePtmVcidEntry( ulPort );
                    }
                    else 
                        bxStatus = ReservePtmVcidEntry( ulPort );
                }
            } /* else part of if (atm traffic type) */
        } /* if( (ulPortMask & PORT_TO_PORTID(ulPort)) != 0 ) */
    } /* for (ulPort) */

    return( bxStatus );
} /* ReserveVcidEntries */


/***************************************************************************
 * Function Name: ReserveAtmVcidEntry (6368, 6362, 6328)
 * Description  : Finds an available vcid and configures the Rx CAM and Tx
 *                Table entries.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReserveAtmVcidEntry( UINT32 ulPort, UINT8 rxCamConfigure )
{
    BCMXTM_STATUS bxStatus = XTMSTS_RESOURCE_ERROR;
    UINT32 ulVcid, ulCam, ulRam;

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);
    for( ulVcid = 0; ulVcid < MAX_VCIDS; ulVcid++ )
    {
        if( (m_pulRxVpiVciCamShadow[ulVcid] & RXCAM_VALID) == 0 )
        {
            /* An available vcid is found. */
            UINT32 ulRxPhysPort = PORT_LOG_TO_PHYS(ulPort);
            ulRxPhysPort = PORT_PHYS_TX_TO_PHYS_RX(ulRxPhysPort);

            ulCam = ulRxPhysPort | (m_Addr.u.Vcc.usVci << RXCAMA_VCI_SHIFT) |
                (m_Addr.u.Vcc.usVpi << RXCAMA_VPI_SHIFT) | RXCAM_VALID;

            ulRam = ulVcid << RXCAM_VCID_SHIFT;

            switch( m_Cfg.ulAtmAalType )
            {
            case AAL_5:
                ulRam |= RXCAM_CT_AAL5;

                /* Strip RFC2684 header on receive. */
                if( HT_LEN(m_Cfg.ulHeaderType) != 0 && m_ulHwRemoveHdr == 1 )
                {
                    ulRam |= RXCAM_STRIP_EN |
                        (HT_LEN(m_Cfg.ulHeaderType) << RXCAM_STRIP_BYTE_SHIFT);
                }
                break;

            case AAL_0_PACKET:
                ulRam |= RXCAM_CT_AAL0_PKT;
                break;

            case AAL_0_CELL:
                ulRam |= RXCAM_CT_AAL0_CELL;
                break;
            }

            m_ucVcids[ulPort][0] = ulVcid;
            m_ucVcidsTx[ulVcid]  = ulVcid;

            if (rxCamConfigure == XTM_CONFIGURE)
            {
               /* Write the values to the Rx VPI/VCI/Port CAM. */
               XP_REGS->ulRxVpiVciCam[(ulVcid * 2) + 1] = ulRam;
               XP_REGS->ulRxVpiVciCam[ulVcid * 2]       = ulCam;
            }

            m_pulRxVpiVciCamShadow[ulVcid] = ulCam;

            /* Also use the same vcid for the Tx Table. */
            XP_REGS->ulTxVpiVciTable[ulVcid] = 
                (m_Addr.u.Vcc.usVci << TXTBL_VCI_SHIFT) |
                (m_Addr.u.Vcc.usVpi << TXTBL_VPI_SHIFT);

            /* Set F4 OAM VPI to this VCC.
             * Note: This will override any previous F4 OAM VPI value.
             *       Since there is only one register for F4 OAM VPI,
             *       we just have to set it to the last configured VCC.
             *       This patch only works well if the CPE has only one
             *       ATM path (VPI) to the ISP. Normally it is the case.
             */
            XP_REGS->ulTxOamCfg &=
                ~(TXOAM_F4_SEG_VPI_MASK | TXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulRxOamCfg &=
                ~(RXOAM_F4_SEG_VPI_MASK | RXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulTxOamCfg |=
                (m_Addr.u.Vcc.usVpi << TXOAM_F4_E2E_VPI_SHIFT) | m_Addr.u.Vcc.usVpi;
            XP_REGS->ulRxOamCfg |=
                (m_Addr.u.Vcc.usVpi << RXOAM_F4_E2E_VPI_SHIFT) | m_Addr.u.Vcc.usVpi;
                
            bxStatus = XTMSTS_SUCCESS;
            break;
        }
    }
    XtmOsReleaseSem(m_ulConnSem);

    return( bxStatus );
} /* ReserveAtmVcidEntry */


/***************************************************************************
 * Function Name: ReservePtmVcidEntry (6368, 6362, 6328)
 * Description  : Finds an available vcid and configures an Rx CAM entry for
 *                each PTM priority that is in the connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReservePtmVcidEntry( UINT32 ulPort )
{
    BCMXTM_STATUS bxStatus = XTMSTS_RESOURCE_ERROR;
    UINT32 ulPtmPri, ulVcid, ulCam, ulRam;
    UINT8  ucVcidTx = INVALID_VCID;

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);
    for( ulPtmPri = PTM_PRI_LOW; ulPtmPri <= PTM_PRI_HIGH; ulPtmPri++ )
    {
        /* Each PTM priority will have a separate rx vcid. */
        if( (m_Addr.u.Flow.ulPtmPriority & ulPtmPri) != 0 )
        {
            for( ulVcid = 0; ulVcid < MAX_VCIDS; ulVcid++ )
            {
                if( (m_pulRxVpiVciCamShadow[ulVcid] & RXCAM_VALID) == 0 )
                {
                    UINT32 ulRxPhysPort = PORT_LOG_TO_PHYS(ulPort);
                    ulRxPhysPort = PORT_PHYS_TX_TO_PHYS_RX(ulRxPhysPort);

                    /* An available vcid is found. */
                    ulCam = ulRxPhysPort | ((ulPtmPri == PTM_PRI_LOW)
                        ? RXCAMP_PTM_PRI_LOW : RXCAMP_PTM_PRI_HIGH) |
                        RXCAM_VALID;

                    ulRam = (ulVcid << RXCAM_VCID_SHIFT) |
                        (RXCAM_CT_PTM << RXCAM_CT_SHIFT);
                    m_ucVcids[ulPort][ulPtmPri - PTM_PRI_LOW] = ulVcid;

                    /* Set the tx vcid of this flow as follows:
                     *    - For a single priority interface, the tx vcid of the flow is the same
                     *      as its rx vcid.
                     *    - For a dual priority interface, the tx vcid of the LOW priority flow is
                     *      the same as its rx vcid.  The tx vcid of the HIGH priority flow will
                     *      be set to the tx vcid of the LOW priority flow.
                     */ 
                    if (ulPtmPri == PTM_PRI_LOW)
                    {
                       m_ucVcidsTx[ulVcid] = ulVcid;
                       ucVcidTx = ulVcid;
                    }
                    else
                    {
                       /* If ucVcidTx is INVALID, we know that this is a single priority interface.
                        * Otherwise, this is a dual priority interface.
                        */
                       m_ucVcidsTx[ulVcid] = (ucVcidTx == INVALID_VCID)? ulVcid : ucVcidTx;
                    }

                    /* Write the values to the Rx VPI/VCI/Port CAM. */
                    XP_REGS->ulRxVpiVciCam[(ulVcid * 2) + 1] = ulRam;
                    XP_REGS->ulRxVpiVciCam[ulVcid * 2] = ulCam;
                    m_pulRxVpiVciCamShadow[ulVcid] = ulCam;

                    bxStatus = XTMSTS_SUCCESS;
                    break;
                }
            }

            if( bxStatus != XTMSTS_SUCCESS )
                break;
        }
    }
    XtmOsReleaseSem(m_ulConnSem);

    return( bxStatus );
} /* ReservePtmVcidEntry */


/***************************************************************************
 * Function Name: ReserveRawVcidEntry (6368, 6362, 6328)
 * Description  : Finds an available vcid and configures an Rx CAM entry for
 *                each PTM priority that is in the connection address.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReserveRawVcidEntry( UINT32 ulPort )
{
    BCMXTM_STATUS bxStatus = XTMSTS_RESOURCE_ERROR;
    UINT32 ulVcid, ulCam, ulRam;

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);
    for( ulVcid = 0; ulVcid < MAX_VCIDS; ulVcid++ )
    {
        if( (m_pulRxVpiVciCamShadow[ulVcid] & RXCAM_VALID) == 0 )
        {
            /* An available vcid is found. */
            UINT32 ulRxPhysPort = PORT_LOG_TO_PHYS(ulPort);
            ulRxPhysPort = PORT_PHYS_TX_TO_PHYS_RX(ulRxPhysPort);

            ulCam = ulRxPhysPort | RXCAM_TEQ_CELL | RXCAM_VALID;
            ulRam = (ulVcid << RXCAM_VCID_SHIFT) | RXCAM_CT_TEQ;

            m_ucVcids[ulPort][0] = ulVcid;

            /* Write the values to the Rx VPI/VCI/Port CAM. */
            XP_REGS->ulRxVpiVciCam[(ulVcid * 2) + 1] = ulRam;
            XP_REGS->ulRxVpiVciCam[ulVcid * 2] = ulCam;
            m_pulRxVpiVciCamShadow[ulVcid] = ulCam;

            bxStatus = XTMSTS_SUCCESS;
            break;
        }
    }
    XtmOsReleaseSem(m_ulConnSem);

    return( bxStatus );
} /* ReserveRawVcidEntry */


/***************************************************************************
 * Function Name: ReserveTxHwShapers (6368, PTM Bonding)
 * Description  : Reserves and configures HW shapers for PTM bonding mode in
 *                6368.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_CONNECTION::ReserveTxHwShapers(PXTM_TRANSMIT_QUEUE_PARMS
    pTransmitQParms, UINT32 ulTransmitQParmsSize)
{
   PXTM_TRANSMIT_QUEUE_PARMS pTxQParms;
   UINT32 i ;

   for( i = 0, pTxQParms = pTransmitQParms; i < ulTransmitQParmsSize;
         i++, pTxQParms++ )
   {
      ConfigureHwShaper ( pTxQParms ) ;
   }
}

/***************************************************************************
 * Function Name: ReserveTxShapers (6368, 6362, 6328)
 * Description  : Reserves and configures available shapers.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReserveTxShapers(PXTM_TRANSMIT_QUEUE_PARMS
    pTransmitQParms, UINT32 ulTransmitQParmsSize)
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    PXTM_TRANSMIT_QUEUE_PARMS pTxQParms;
    PXTMRT_TRANSMIT_QUEUE_ID pTxQId;
    UINT32 ulShaperIdx = INVALID_INDEX;
    UINT32 ulTxQueueIndex = INVALID_INDEX;
    UINT32 i, j, ulVcid, ulPort, ulPtmPriIdx;

    for( i = 0, pTxQParms = pTransmitQParms; i < ulTransmitQParmsSize;
         i++, pTxQParms++ )
    {
        /* Find an unused shaper.  If it's not enabled, it's unused. */
        for( j = 0, ulShaperIdx = INVALID_INDEX; j < MAX_SHAPERS; j++ )
        {
           UINT32 *pulSstCtrl ;
           pulSstCtrl = PSSTCTRL(j, pTxQParms->ulBondingPortId) ;
           if( (*pulSstCtrl & SST_EN) == 0 )
            {
                ulShaperIdx = j;
                break;
            }
        }

        if( ulShaperIdx == INVALID_INDEX )
        {
            bxStatus = XTMSTS_RESOURCE_ERROR;
            break;
        }

        /* Get an unused Transmit Q as well */
        if ((bxStatus = ReserveTxQueue (&ulTxQueueIndex)) == XTMSTS_RESOURCE_ERROR)
           break ;

        bxStatus = ConfigureShaper( ulShaperIdx, pTxQParms ) ;
        if( bxStatus != XTMSTS_SUCCESS )
            break;

        ulPort = PORTID_TO_PORT(pTxQParms->ulPortId);
        ulPtmPriIdx = (pTxQParms->ulPtmPriority)
            ? pTxQParms->ulPtmPriority - PTM_PRI_LOW : 0;
        ulVcid = m_ucVcids[ulPort][ulPtmPriIdx];
        m_ulVcidsCount[ulVcid]++;

        if( (pTxQParms->ulPtmPriority & PTM_PRI_HIGH) == PTM_PRI_HIGH &&
            m_ucVcidsTx[ulVcid] != INVALID_VCID )
        {
            m_ulVcidsCount[m_ucVcidsTx[ulVcid]]++;
        }

        /* Fill in Channel Index Table. */
	     if( (m_Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
        {
            j = ulVcid << TXCHA_VCID_SHIFT;

            switch( m_Cfg.ulAtmAalType )
            {
            case AAL_5:
                j |= TXCHA_CT_AAL5;

                if( HT_LEN(m_Cfg.ulHeaderType) != 0 && m_ulHwAddHdr == 1 )
                {
                    j |= TXCHA_HDR_EN |
                        (HT_TYPE(m_Cfg.ulHeaderType)-PKT_HDRS_START_IDX) <<
                        TXCHA_HDR_IDX_SHIFT;
                }

                break;

            case AAL_0_PACKET:
                j |= TXCHA_CT_AAL0_PKT;
                break;

            case AAL_0_CELL:
                j |= TXCHA_CT_AAL0_CELL;
                break;
            }
        }
        else /* PTM */
            j = TXCHP_FCS_EN | TXCHP_CRC_EN;

        if (pTxQParms->ulBondingPortId  == PORT_PHY_INVALID)
        XP_REGS->ulTxChannelCfg[ulTxQueueIndex] = j;

        /* Fill in the transmit queue id structure that is passed to the
         * bcmxtmrt driver.
         */
        pTxQId = &m_TxQIds[m_ulTxQIdsSize-1];
        pTxQId->ucQosQId        = pTxQParms->ucQosQId;
        pTxQId->ulPortId        = pTxQParms->ulPortId;
        pTxQId->ulPtmPriority   = pTxQParms->ulPtmPriority;
        pTxQId->ucWeightAlg     = pTxQParms->ucWeightAlg;
        pTxQId->ulWeightValue   = pTxQParms->ulWeightValue;
        pTxQId->ucSubPriority   = pTxQParms->ucSubPriority;
        pTxQId->usQueueSize     = pTxQParms->usSize;
        pTxQId->ulQueueIndex    = ulTxQueueIndex;
        pTxQId->ulBondingPortId = pTxQParms->ulBondingPortId;
    } /* for (i, txqparams) */

    if( bxStatus == XTMSTS_SUCCESS )
    {
        if( SPB_VCID_BEING_USED != INVALID_VCID &&
            m_ulVcidsCount[SPB_VCID_BEING_USED] )
        {
            /* Add one additional channel which is used by PacketCMF */
            if( m_ulMpIdxs[SPB_VCID_BEING_USED] == INVALID_INDEX )
            {
                bxStatus = ReserveMultiPriorityIndex(SPB_VCID_BEING_USED, MAX_INTERFACES);
            }

            if( bxStatus == XTMSTS_SUCCESS &&
                m_ulPacketCmfShaperIdx != INVALID_INDEX )
            {
                CopyPacketCmfShaper( m_ulPacketCmfShaperIdx ) ;
            }
        }
    }
    else
    {
        for( i = 0, pTxQId = m_TxQIds; i < m_ulTxQIdsSize; i++, pTxQId++ ) {
          UINT32 *pulSstCtrl ;
          pulSstCtrl = PSSTCTRL(pTxQId->ulQueueIndex,pTransmitQParms->ulBondingPortId);
          *pulSstCtrl &= ~SST_EN;

          m_pConnTable->UnReserveTxQIndex (pTxQId->ulQueueIndex) ;
        }

        memset(m_ulMpIdxs, INVALID_INDEX, sizeof(m_ulMpIdxs));
        m_ulTxQIdsSize = 0;
    }

    return( bxStatus );
} /* ReserveTxShapers */


/***************************************************************************
 * Function Name: ReserveMultiPriorityIndex (6368, 6362, 6328)
 * Description  : Reserves a multi priority index.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ReserveMultiPriorityIndex( UINT32 ulVcid, UINT32 ulBondingPort )
{
    BCMXTM_STATUS bxStatus = XTMSTS_RESOURCE_ERROR;
    PXTMRT_TRANSMIT_QUEUE_ID pTxQId;
    UINT32 ulMpAalCfg = MPAALCFG(ulBondingPort) ;
    UINT32 ulMpAalIdx, j, k;
    UINT32 ulVcidTx;

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);

    /* Use its tx vcid to determine which MPAAL group it belongs. */
    ulVcidTx = m_ucVcidsTx[ulVcid];

    if( (ulMpAalIdx = m_ulMpIdxs[ulVcidTx]) == INVALID_INDEX )
    {
        /* Find an available MPAAL index. */
        for( UINT32 i = 0, ulShift = 0; i < TXMP_NUM_GROUPS; i++,
             ulShift += TXMP_GROUP_SIZE )
        {
            if( (ulMpAalCfg & (TXMP_GROUP_EN << ulShift)) == 0 )
            {
                m_ulMpIdxs[ulVcidTx] = ulMpAalIdx = i;
                break;
            }
        }
    }

    m_ulMpIdxs[ulVcid] = m_ulMpIdxs[ulVcidTx];

    XtmOsReleaseSem(m_ulConnSem);

    if( ulMpAalIdx != INVALID_INDEX )
    {
        UINT32 ulPortId = INVALID_INDEX;
        UINT32 ulShaperIdx = 0;

        /* Find port for vcid. */
        for( j = 0; j < MAX_INTERFACES && ulPortId == INVALID_INDEX; j++ )
            for( k = 0; k < MAX_PTM_PRIORITIES; k++ )
            {
                if( m_ucVcids[j][k] == ulVcid )
                {
                    ulPortId = PORT_TO_PORTID(j);
                    break;
                }
            }

        /* Check if any of the transmit DMA channels are configured for PTM
         * preemption.
         */
        UINT32 ulPtmHighPriQ = 0;
        for( j = 0, pTxQId = m_TxQIds; j < m_ulTxQIdsSize; j++, pTxQId++ )
            if( pTxQId->ulPortId == ulPortId &&
                pTxQId->ulPtmPriority == PTM_PRI_HIGH )
            {
                ulPtmHighPriQ = 1;
                break;
            }

        /* Set all shapers with same port to group. */
        for( j = 0, pTxQId = m_TxQIds; j < m_ulTxQIdsSize; j++, pTxQId++ ) {

           UINT32 *pulSstCtrl, *pulSstBt ;

           pulSstCtrl = PSSTCTRL(pTxQId->ulQueueIndex, pTxQId->ulBondingPortId) ;
           pulSstBt   = PSSTBT(pTxQId->ulQueueIndex, pTxQId->ulBondingPortId) ;

           if( pTxQId->ulPortId == ulPortId ) {

              if ((*pulSstCtrl & SST_MP_EN) == 0) {

                 *pulSstCtrl &= ~SST_MP_GROUP_MASK ;
                 *pulSstCtrl |= SST_MP_EN | (ulMpAalIdx << SST_MP_GROUP_SHIFT);
                 *pulSstBt &= ~(SST_WEIGHT_VALUE_MASK | SST_ALG_WEIGHT_MASK);

                 if (pTxQId->ucWeightAlg == WA_WFQ)
                 {
                    *pulSstBt |= SST_ALG_WFQ | (pTxQId->ulWeightValue << SST_WEIGHT_VALUE_SHIFT);
                 }
                 else
                 {
                    /* For SP or WRR, set WGHT_ALG of the SST BT register to CWRR
                     * with WGHT_VAL=1.
                     */
                    *pulSstBt |= SST_ALG_CWRR | (1 << SST_WEIGHT_VALUE_SHIFT) ;
                 }
              }

              if( ulPtmHighPriQ == 1 && pTxQId->ulPtmPriority == PTM_PRI_LOW )
              {
                 /* For the PTM preemptive flow to preempt the normal flow,
                  * the normal flow DMA channels must not be in the MPAAL
                  * group but the preemptive flow DMA channels must be in.
                  */
                 *pulSstCtrl &= ~(SST_MP_EN | SST_MP_GROUP_MASK);
              }
              else
              {
                 if( pTxQId->ulQueueIndex > ulShaperIdx )
                    ulShaperIdx = pTxQId->ulQueueIndex;
              }
           } /* if( pTxQId->ulPortId == ulPortId )  */
        } /* for (j) */

        /* Set MPAAL register here with new group. */
        {
           UINT32 ulMpAalCfg ;

           ulMpAalCfg = MPAALCFG(ulBondingPort) ;
           ulMpAalCfg &= ~(TXMP_GROUP_SHAPER_MASK << (ulMpAalIdx * TXMP_GROUP_SIZE));
           ulMpAalCfg |= (ulShaperIdx <<
                 ((ulMpAalIdx * TXMP_GROUP_SIZE) + TXMP_GROUP_SHAPER_SHIFT));
           ulMpAalCfg |= (TXMP_GROUP_EN << (ulMpAalIdx * TXMP_GROUP_SIZE));
           SET_MPAALCFG(ulBondingPort,ulMpAalCfg) ;
        }

        bxStatus = XTMSTS_SUCCESS;

    } /* if( ulMpAalIdx != INVALID_INDEX ) */

    return( bxStatus );
} /* ReserveMultiPriorityIndex */

/***************************************************************************
 * Function Name: ConfigureHwShaper (6368, PTM Bonding mode)
 * Description  : This is used only in PTM bonding mode. Ref - data sheet 6368
 *                Q 0 is for low priority path 0.
 *                Q 1 is for high priority path 0.
 *                Q 2 is for low prio path 1
 *                Q 3 is for high prio path 1.
 *                2 shapers are used for each Q. In this case, we will
 *                configure 2 shapers for Q 0 and 2 for Q 1.
 *                same shaping values as that of the current one along with the
 *                Also, the tx channel configuration will be configured.
 *                corresponding port Id.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_CONNECTION::ConfigureHwShaper( PXTM_TRANSMIT_QUEUE_PARMS pTxQParms )
{
   UINT32 ulPhysPort;
   UINT32 ulPtmPriIdx ;
   UINT32 ulAlg = SST_ALG_UBR_NO_PCR ;

   ulPtmPriIdx = pTxQParms->ulPtmPriority?
         (pTxQParms->ulPtmPriority - PTM_PRI_LOW) : 0 ;

   for (ulPhysPort = 0 ; ulPhysPort < MAX_BONDED_LINES ; ulPhysPort++) {

      UINT32 ulHwShaperIdx = ulPtmPriIdx + ulPhysPort ;
      UINT32 *pulSstCtrl   = XP_REGS->ulSstCtrl + ulHwShaperIdx ;
      UINT32 *pulSstPcrScr = XP_REGS->ulSstPcrScr + ulHwShaperIdx ;
      UINT32 *pulSstBt     = XP_REGS->ulSstBt + ulHwShaperIdx ;
      UINT32 ulSstCtrl, ulSstPcrScr, ulSstBt     = 0;

      /* Convert from cell rates to rate percentages which is required by
       * the ATM Processor. The unit of time, ulUt, is defined 10 times
       * larger than its actual value and is, therefore, divided by 10.
       * This is done to reduce rounding errors.  In addition, the result
       * is rounded up.
       */

      if( (*pulSstCtrl & SST_EN) == 0 ) {

         /* Calculate new shaper register values. */
         ulSstCtrl  = (ulPhysPort << SST_PORT_SHIFT) | ulAlg;
         ulSstCtrl |= ((UINT32)(pTxQParms->ucSubPriority) << SST_SUB_PRIORITY_SHIFT);

         if (ulPtmPriIdx != 0)
            ulSstCtrl |= SST_PTM_PREEMPT ;

         ulSstPcrScr = 0 ;
         ulSstBt = 0 ;

         /* For queues not belonging to a MPAAL group, set WGHT_ALG of the
          * SST BT register to CWRR with WGHT_VAL=1, regardless the weight
          * algorithm of this connection is SP or WFQ.
          */
         ulSstBt |= SST_ALG_CWRR | (1 << SST_WEIGHT_VALUE_SHIFT);

         /* Change shaper registers if they are different than current values.*/
         if( *pulSstCtrl != ulSstCtrl || *pulSstPcrScr != ulSstPcrScr ||
               *pulSstBt != ulSstBt )
         {
            /* Disable shaper. */
            *pulSstCtrl &= ~SST_EN;

            /* Set new shaper values. */
            *pulSstCtrl = ulSstCtrl;
            *pulSstPcrScr = ulSstPcrScr;
            *pulSstBt = ulSstBt;
         }

         /* Enable shaper.*/
         *pulSstCtrl |= SST_EN;
      } /* if (shaper control is NOT enabled) */

   } /* for (ulPhysPort) */

   {
      UINT32 j ;

      j = TXCHP_FCS_EN | TXCHP_CRC_EN;
      XP_REGS->ulTxChannelCfg[ulPtmPriIdx] = j;
   }
}

/***************************************************************************
 * Function Name: UnconfigureHwShaper (6368, PTM Bonding mode)
 * Description  : This is used only in PTM bonding mode. Ref - data sheet 6368
 *                Q 0 is for low priority path 0.
 *                Q 1 is for high priority path 0.
 *                Q 2 is for low prio path 1
 *                Q 3 is for high prio path 1.
 *                2 shapers are used for each Q. In this case, we will
 *                unconfigure 2 shapers for Q 0 and 2 for Q 1.
 *                Shapers will be set to 0.
 *                Also, the tx channel configuration will be unconfigured.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_CONNECTION::UnconfigureHwShaper( PXTM_TRANSMIT_QUEUE_PARMS pTxQParms )
{
   UINT32 ulPhysPort ;
   UINT32 ulPtmPriIdx ;

   ulPtmPriIdx = pTxQParms->ulPtmPriority?
         (pTxQParms->ulPtmPriority - PTM_PRI_LOW) : 0 ;

   for (ulPhysPort = 0 ; ulPhysPort < MAX_BONDED_LINES ; ulPhysPort++) {

      UINT32 ulHwShaperIdx = ulPtmPriIdx + ulPhysPort ;
      UINT32 *pulSstCtrl   = XP_REGS->ulSstCtrl + ulHwShaperIdx ;
      UINT32 *pulSstPcrScr = XP_REGS->ulSstPcrScr + ulHwShaperIdx ;
      UINT32 *pulSstBt     = XP_REGS->ulSstBt + ulHwShaperIdx ;

      if( (*pulSstCtrl & SST_EN) == 0x1 ) {

         /* Disable shaper. */
         *pulSstCtrl = *pulSstPcrScr = *pulSstBt = 0 ;
      }
   } /* for (ulPhysPort) */
 
   XP_REGS->ulTxChannelCfg [ulPtmPriIdx] = 0 ;
}

/***************************************************************************
 * Function Name: ConfigureShaper (6368, 6362, 6328)
 * Description  : Reserves and configures available shapers.
 *                For bonding, the successive shaper Id is configured with the
 *                same shaping values as that of the current one along with the
 *                corresponding port Id.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ConfigureShaper( UINT32 ulShaperIdx,
    PXTM_TRANSMIT_QUEUE_PARMS pTxQParms )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    UINT32 ulTdi = m_Cfg.ulTransmitTrafficDescrIndex;
    PXTM_TRAFFIC_DESCR_PARM_ENTRY pEntry = GetTdtEntry( ulTdi );
    UINT32 ulAlg = SST_ALG_UBR_NO_PCR;
    UINT32 ulPcr = 0;
    UINT32 ulScr = 0;
    UINT32 ulMbs = 1;
    UINT32 ulMcr = 0;
    UINT32 ulMcrEn = 0;

    /* Determine shaper register values from the traffic descriptor table
     * parameter entry.
     */
    switch( pEntry->ulTrafficDescrType )
    {
    case TDT_ATM_NO_TRAFFIC_DESCRIPTOR:
        switch( pEntry->ulServiceCategory )
        {
        case SC_UBR:
            ulAlg = SST_ALG_UBR_NO_PCR;
            break;

        default:
            bxStatus = XTMSTS_PARAMETER_ERROR;
            break;
        }
        break;

    case TDT_ATM_NO_CLP_NO_SCR:
    case TDT_ATM_NO_CLP_TAGGING_NO_SCR:
    case TDT_ATM_CLP_TRANSPARENT_NO_SCR:
        ulPcr = pEntry->ulPcr;
        switch( pEntry->ulServiceCategory )
        {
        case SC_UBR:
            ulAlg = SST_ALG_UBR_PCR;
            break;

        case SC_CBR:
            ulAlg = SST_ALG_CBR;
            break;

        default:
            bxStatus = XTMSTS_PARAMETER_ERROR;
            break;
        }
        break;

    case TDT_ATM_NO_CLP_SCR:
    case TDT_ATM_CLP_TRANSPARENT_SCR:
        ulPcr = pEntry->ulPcr;
        ulScr = pEntry->ulScr;
        ulMbs = pEntry->ulMbs;
        switch( pEntry->ulServiceCategory )
        {
        case SC_RT_VBR:
        case SC_NRT_VBR:
            ulAlg = SST_ALG_VBR_1;
            break;

        default:
            bxStatus = XTMSTS_PARAMETER_ERROR;
            break;
        }
        break;

    case TDT_ATM_CLP_NO_TAGGING_MCR:
        ulPcr = pEntry->ulPcr;
        ulMcr = pEntry->ulMcr;
        ulMcrEn = 1;
        switch( pEntry->ulServiceCategory )
        {
        case SC_CBR:
            ulAlg = SST_ALG_CBR;
            break;

        case SC_RT_VBR:
        case SC_NRT_VBR:
            ulAlg = SST_ALG_VBR_1;
            break;

        case SC_UBR:
            ulAlg = SST_ALG_UBR_PCR;
            break;

        default:
            bxStatus = XTMSTS_PARAMETER_ERROR;
            break;
        }
        break;

    case TDT_ATM_PTM_MAX_BIT_RATE_SCR:
        ulScr = pEntry->ulScr;
        switch( pEntry->ulServiceCategory )
        {
        case SC_MBR:
            ulAlg = SST_ALG_MBR;
            break;

        default:
            bxStatus = XTMSTS_PARAMETER_ERROR;
            break;
        }
        break;

    default:
        bxStatus = XTMSTS_PARAMETER_ERROR;
        break;
    }

    if( pEntry->ulMcr )
    {
        ulMcr = pEntry->ulMcr;
        ulMcrEn = 1;
    }

    if( bxStatus == XTMSTS_SUCCESS )
    {
        UINT32 *pulSstCtrl = PSSTCTRL(ulShaperIdx, pTxQParms->ulBondingPortId) ;
        UINT32 *pulSstPcrScr = XP_REGS->ulSstPcrScr + ulShaperIdx;
        UINT32 *pulSstBt = PSSTBT(ulShaperIdx, pTxQParms->ulBondingPortId) ;
        UINT32 ulSstCtrl = 0;
        UINT32 ulSstPcrScr = 0;
        UINT32 ulSstBt = 0;
        UINT32 ulPort;
        UINT32 ulPhysPort;
        UINT32 ulPtmPriIdx;
        UINT32 ulVcid;

        const UINT32 ulUt = ms_ulSitUt;   /* SIT_CNT_VAL */

        /* PCR is calculated based on the desired cell rate by the equation
         * below (see 6368, 6362, 6328 data sheet):
         *    PCR = roundup(1/(c * s)) - 1
         *    where c is the desired pcr in cells/s
         *          s is the shaping interval time in seconds.
         *               s = SIT * 0.0000001  (SIT * 100ns)
         * i.e.,
         *    PCR = roundup(10000000/(c * SIT)) - 1
         *
         * Similar equation is used for SCR or MCR calculation.
         */
        UINT32 ulSitCr;
        UINT32 ulIp;
        UINT32 ulIs;
        UINT32 ulIm;
        UINT32 ulBt;

        ulSitCr = ulUt * ulPcr;
        ulIp    = ulPcr ? (10000000+(ulSitCr/2)) / ulSitCr : 0;
        ulSitCr = ulUt * ulScr;
        ulIs    = ulScr ? (10000000+(ulSitCr/2)) / ulSitCr : 0;
        ulSitCr = ulUt * ulMcr;
        ulIm    = (ulMcr && ulMcrEn) ? (10000000+(ulSitCr/2)) / ulSitCr : 0;
        ulBt    = (ulMbs - 1) * (ulIs - ulIp);
 
#ifdef CONFIG_BCM96368
        /* Readjust SCR to fit SAR scheduling algorithm. Works in some, but
         * not all cases.
         */
        if( ulIs )
        {
            UINT32 ulLrt = (XP_REGS->ulTxLineRateTimer & 0x0001fffe) >>
                TXLRT_COUNT_VALUE_SHIFT;
            ulIs -= (ulLrt / ulUt);
        }
#endif

        /* Adjust MCR to fit SAR scheduling algorithm. */
        if( ulIm )
        {
            UINT32 ulLrt = (XP_REGS->ulTxLineRateTimer & 0x0001fffe) >>
                TXLRT_COUNT_VALUE_SHIFT;
            ulIm -= (ulLrt / ulUt);
            ulIm *= 2;
        }

        /* adjust for ubus frequency deviation from the base frequency */
        ulIp = (ulIp * ms_ulBusSpeed) / UBUS_BASE_FREQUENCY_IN_MHZ;
        ulIs = (ulIs * ms_ulBusSpeed) / UBUS_BASE_FREQUENCY_IN_MHZ;
        ulIm = (ulIm * ms_ulBusSpeed) / UBUS_BASE_FREQUENCY_IN_MHZ;

        /* The ATM Processor expects IP, IS and IM values to be one less than
         * the computed value.
         */
        if( ulIp )
            ulIp--;

        if( ulIs )
            ulIs--;

        if( ulIm )
            ulIm--;

        if( ulIp > 0xFFFF )
        {
            XtmOsPrintf("Error: Calculated PCR=0x%x is too big. Reset to max 0xFFFF\n", ulIp);
            bxStatus = XTMSTS_PARAMETER_ERROR;
            ulIp = 0xFFFF; /* set to max so that shaper can be configured */
        }
        if( ulIs > 0xFFFF )
        {
            XtmOsPrintf("Error: Calculated SCR=0x%x is too big. Reset to max 0xFFFF\n", ulIs);
            bxStatus = XTMSTS_PARAMETER_ERROR;
            ulIs = 0xFFFF; /* set to max so that shaper can be configured */
        }
        if( ulIm > 0xFFFF )
        {
            XtmOsPrintf("Error: Calculated MCR=0x%x is too big. Reset to max 0xFFFF\n", ulIm);
            bxStatus = XTMSTS_PARAMETER_ERROR;
            ulIm = 0xFFFF; /* set to max so that shaper can be configured */
        }
        
        /* Calculate new shaper register values. */
        ulPort = PORTID_TO_PORT(pTxQParms->ulPortId);
        ulPhysPort = PORT_LOG_TO_PHYS(ulPort);
        ulPtmPriIdx = pTxQParms->ulPtmPriority?
                           (pTxQParms->ulPtmPriority - PTM_PRI_LOW) : 0;
        ulVcid = m_ucVcids[ulPort][ulPtmPriIdx];
        if( SPB_VCID_BEING_USED != INVALID_VCID &&
            ulVcid == SPB_VCID_BEING_USED )
        {
            m_ulPacketCmfShaperIdx = ulShaperIdx;
            m_ulPacketCmfTxChanIdx = ulShaperIdx;
        }
        ulSstCtrl  = (ulPhysPort << SST_PORT_SHIFT) | ulAlg;
        ulSstCtrl |= ((UINT32)(pTxQParms->ucSubPriority) << SST_SUB_PRIORITY_SHIFT);

        if( m_ulMpIdxs[ulVcid] != INVALID_INDEX )
            ulSstCtrl |= (m_ulMpIdxs[ulVcid]<<SST_MP_GROUP_SHIFT) | SST_MP_EN;
        if( ulMcrEn )
            ulSstCtrl |= SST_MCR_EN | (ulIm << SST_RATE_MCR_SHIFT);
        if( ((m_Addr.ulTrafficType == TRAFFIC_TYPE_PTM) ||
             (m_Addr.ulTrafficType == TRAFFIC_TYPE_PTM_BONDED)) &&
            pTxQParms->ulPtmPriority == PTM_PRI_HIGH )
        {
            ulSstCtrl |= SST_PTM_PREEMPT;
        }

        ulSstPcrScr = (ulIp<<SST_RATE_PCR_SHIFT) | (ulIs<<SST_RATE_SCR_SHIFT);

        ulSstBt = (ulBt << SST_RATE_BT_SHIFT);

        if (ulSstCtrl & SST_MP_EN)
        {
            /* This queue belongs to a MPAAL group */
            if (pTxQParms->ucWeightAlg == WA_WFQ)
            {
                ulSstBt |= SST_ALG_WFQ | (pTxQParms->ulWeightValue << SST_WEIGHT_VALUE_SHIFT);
            }
            else
            {
                /* For SP or WRR connection, set WGHT_ALG of the SST BT register to CWRR
                 * with WGHT_VAL=1.
                 */
                ulSstBt |= SST_ALG_CWRR | (1 << SST_WEIGHT_VALUE_SHIFT);
            }
        }
        else
        {
            /* For queues not belonging to a MPAAL group, set WGHT_ALG of the
             * SST BT register to CWRR with WGHT_VAL=1, regardless the weight
             * algorithm of this connection is SP or WFQ.
             */
            ulSstBt |= SST_ALG_CWRR | (1 << SST_WEIGHT_VALUE_SHIFT);
        }

        /* Change shaper registers if they are different than current values.*/
        if( *pulSstCtrl != ulSstCtrl || *pulSstPcrScr != ulSstPcrScr ||
            *pulSstBt != ulSstBt )
        {
            /* Disable shaper. */
            *pulSstCtrl &= ~SST_EN;

            /* Set new shaper values. */
            *pulSstCtrl = ulSstCtrl;
            *pulSstPcrScr = ulSstPcrScr;
            *pulSstBt = ulSstBt;
        }

        /* Enable shaper.*/
        *pulSstCtrl |= SST_EN;

    } /* if( bxStatus == XTMSTS_SUCCESS ) */

    return( bxStatus );
} /* ConfigureShaper */


/***************************************************************************
 * Function Name: CopyPacketCmfShaper (6368, 6362, 6328)
 * Description  : Copies the values from one shaper connection index to another
 *                shaper connection index. The new shpaer is used by PacketCMF.
 * Returns      : None.
 ***************************************************************************/
void XTM_CONNECTION::CopyPacketCmfShaper( UINT32 ulSrcShaperIdx )
{
    UINT32 ulIdx;

    /* CMF will not be used for PTM bonding , so no need to map the SST here */
    if( (m_Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
        ulIdx = SPB_ATM_CHANNEL;
    else
        ulIdx = SPB_PTM_CHANNEL;

    XP_REGS->ulSstPcrScr[ulIdx] = XP_REGS->ulSstPcrScr[ulSrcShaperIdx];
    XP_REGS->ulSstBt[ulIdx] = XP_REGS->ulSstBt[ulSrcShaperIdx];
    XP_REGS->ulSstCtrl[ulIdx] = XP_REGS->ulSstCtrl[ulSrcShaperIdx];
    XP_REGS->ulTxChannelCfg[ulIdx] = XP_REGS->ulTxChannelCfg[ulSrcShaperIdx];
} /* CopyPacketCmfShaper */


/***************************************************************************
 * Function Name: UnreserveVcidsAndTxShapers (6368, 6362, 6328)
 * Description  : Clear VCID from Tx Table and Rx CAM and clear shapers used
 *                by this connection.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::UnreserveVcidsAndTxShapers( void )
{
    UINT32 i, j;
    UINT8 ucVcid;

    for( i = 0; i < m_ulTxQIdsSize; i++ )
    {
       UINT32 *pulSstCtrl ;

       pulSstCtrl = PSSTCTRL(m_TxQIds[i].ulQueueIndex, m_TxQIds[i].ulBondingPortId) ;
       *pulSstCtrl = 0;

       if (m_Cfg.TransmitQParms[i].ulBondingPortId  == PORT_PHY_INVALID)
        XP_REGS->ulTxChannelCfg[m_TxQIds[i].ulQueueIndex] = 0;

       m_pConnTable->UnReserveTxQIndex (m_TxQIds[i].ulQueueIndex) ;
    }

    m_ulTxQIdsSize = 0;

    if( SPB_VCID_BEING_USED != INVALID_VCID &&
        m_ulVcidsCount[SPB_VCID_BEING_USED] )
    {
        /* Remove PacketCMF channel. */
	if( (m_Addr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
            XP_REGS->ulSstCtrl[SPB_ATM_CHANNEL] = 0;
        else
            XP_REGS->ulSstCtrl[SPB_PTM_CHANNEL] = 0;
    }

    memset(m_TxQIds, 0x00, sizeof(m_TxQIds));

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);
    for( i = 0; i < MAX_VCIDS; i++ )
    {
        if( m_ulMpIdxs[i] != INVALID_INDEX )
        {
           UINT32 ulTxMpAalCfg ;
           UINT32 ulBondingPort = PORTID_TO_PORT(m_Cfg.TransmitQParms[0].ulBondingPortId) ;

           ulTxMpAalCfg = MPAALCFG(ulBondingPort) ;
           ulTxMpAalCfg &= ~((TXMP_GROUP_EN | TXMP_GROUP_SHAPER_MASK) <<
                (m_ulMpIdxs[i] * TXMP_GROUP_SIZE));
           SET_MPAALCFG(ulBondingPort,ulTxMpAalCfg) ;
    
           m_ulMpIdxs[i] = INVALID_INDEX;
        }
    }

    for( i = 0; i < MAX_INTERFACES; i++ )
        for( j = 0; j < MAX_PTM_PRIORITIES; j++ )
        {
            if( (ucVcid = m_ucVcids[i][j]) != INVALID_VCID)
            {
                XP_REGS->ulRxVpiVciCam[(ucVcid * 2) + 1] = 0;
                XP_REGS->ulRxVpiVciCam[ucVcid * 2] = 0;
                XP_REGS->ulTxVpiVciTable[ucVcid] = 0;
                m_ucVcids[i][j] = INVALID_VCID;
                m_ulVcidsCount[ucVcid] = 0;
                m_pulRxVpiVciCamShadow[ucVcid] = 0;
            }
        }
    XtmOsReleaseSem(m_ulConnSem);

    return( XTMSTS_SUCCESS );
} /* UnreserveVcidsAndTxShapers */


/***************************************************************************
 * Function Name: UnreserveTxHwShapers (6368, PTM Bonding)
 * Description  : Unreserves and Unconfigures HW shapers for PTM bonding mode in
 *                6368.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_CONNECTION::UnreserveTxHwShapers ( void )
{
   UINT32 i ;
   PXTM_TRANSMIT_QUEUE_PARMS pTxQParms ;

   for( i = 0, pTxQParms = &m_Cfg.TransmitQParms[0]; i < m_Cfg.ulTransmitQParmsSize;
         i++, pTxQParms++ )
      UnconfigureHwShaper ( pTxQParms ) ;
}


/***************************************************************************
 * Function Name: ShareExistingTxQueue (6368, 6362, 6328)
 * Description  : Not used by ths chip.
 * Returns      : XTMSTS_SUCCESS
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::ShareExistingTxQueue( UINT32 ulSubPriority )
{
    return( XTMSTS_SUCCESS );
} /* ShareExistingTxQueue */


/***************************************************************************
 * Function Name: FindTxQueue (6368, 6362, 6328)
 * Description  : Not used by ths chip.
 * Returns      : XTMSTS_SUCCESS
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION::FindTxQueue( UINT32 ulQueueIndex )
{
    return( XTMSTS_SUCCESS );
} /* FindTxQueue */

/***************************************************************************
 * Function Name: CheckTransmitQueues
 * Description  : Adds and deletes transmit queues from the current
 *                configuration in order to match the new configuration.
 * Returns      : XTMSTS_SUCCESS
 ***************************************************************************/
void XTM_CONNECTION::CheckTransmitQueues( void )
{
    XTMRT_TRANSMIT_QUEUE_ID TxQIdsSave[MAX_TRANSMIT_QUEUES];
    UINT32 i, j, k, ulFound, ulTxQIdsSize, ulTxQIdsSizeSave = 0;
    PXTM_TRANSMIT_QUEUE_PARMS pTxQParms;
    PXTMRT_TRANSMIT_QUEUE_ID pTxQId;

    XtmOsRequestSem(m_ulConnSem, MAX_TIMEOUT);

    /* Check transmit queues that have been removed.*/
    for( i = 0, pTxQId = m_TxQIds; i < m_ulTxQIdsSize; i++, pTxQId++ )
    {
        for( j = 0, pTxQParms = m_Cfg.TransmitQParms, ulFound = 0;
             j < m_Cfg.ulTransmitQParmsSize; j++, pTxQParms++ )
        {
            if( pTxQId->ucQosQId      == pTxQParms->ucQosQId &&
                pTxQId->ulPortId      == pTxQParms->ulPortId &&
                pTxQId->ucSubPriority == pTxQParms->ucSubPriority &&
                pTxQId->ulPtmPriority == pTxQParms->ulPtmPriority &&
                pTxQId->ucWeightAlg   == pTxQParms->ucWeightAlg &&
                pTxQId->ulWeightValue == pTxQParms->ulWeightValue )
            {
                memcpy(&TxQIdsSave[ulTxQIdsSizeSave++], pTxQId,
                    sizeof(XTMRT_TRANSMIT_QUEUE_ID));
                ulFound = 1;
                break;
            }
        }

        if( ulFound == 0 )
        {
           UINT32 *pulSstCtrl, *pulSstBt ;

           /* Delete the transmit queue. */
           UINT32 ulPort = PORTID_TO_PORT(pTxQId->ulPortId);
           UINT32 ulPtmPriIdx = pTxQId->ulPtmPriority?
              (pTxQId->ulPtmPriority - PTM_PRI_LOW) : 0;
           UINT32 ulVcid = m_ucVcids[ulPort][ulPtmPriIdx];

           (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_UNSET_TX_QUEUE, pTxQId);
           
           if (m_ulVcidsCount[ulVcid] > 1)
              m_ulVcidsCount[ulVcid]--;

           pulSstCtrl = PSSTCTRL(pTxQId->ulQueueIndex,pTxQId->ulBondingPortId) ;
           pulSstBt   = PSSTBT(pTxQId->ulQueueIndex,pTxQId->ulBondingPortId) ;
           *pulSstCtrl = *pulSstBt = 0 ;

           if (pTxQId->ulBondingPortId  == PORT_PHY_INVALID)
              XP_REGS->ulTxChannelCfg[pTxQId->ulQueueIndex] = 0;

           m_pConnTable->UnReserveTxQIndex (pTxQId->ulQueueIndex) ;

           {
              PXTMRT_TRANSMIT_QUEUE_ID pTxQId2;
              UINT32 ulShaperIdx;

              switch (m_ulVcidsCount[ulVcid])
              {
                 case 0:
                 case 1:
                    for (j = 0, pTxQId2 = m_TxQIds; j < m_ulTxQIdsSize; j++, pTxQId2++)
                    {

                       pulSstCtrl = PSSTCTRL(pTxQId2->ulQueueIndex, pTxQId2->ulBondingPortId);
                       pulSstBt   = PSSTBT(pTxQId2->ulQueueIndex, pTxQId2->ulBondingPortId);

                       if (pTxQId2->ulPortId == pTxQId->ulPortId && (*pulSstCtrl & SST_MP_EN))
                       {
                          /* Disable MPAAL for the queue */
                          *pulSstCtrl &= ~(SST_MP_EN | SST_MP_GROUP_MASK);

                          /* For queues not belonging to a MPAAL group,
                           * set WGHT_ALG of the SST BT register to CWRR with
                           * WGHT_VAL=1, regardless the weight algorithm of
                           * this connection is SP or WFQ.
                           */
                          *pulSstBt &= ~(SST_ALG_WEIGHT_MASK | SST_WEIGHT_VALUE_MASK);
                          *pulSstBt |= SST_ALG_CWRR | (1 << SST_WEIGHT_VALUE_SHIFT);
                       }
                    }

                    /* Reset MPAAL entry. */
                    if( m_ulMpIdxs[ulVcid] != INVALID_INDEX )
                    {
                       UINT32 ulBondingPort = PORTID_TO_PORT (m_TxQIds[0].ulBondingPortId) ;
                       UINT32 txMpAalCfg = MPAALCFG(ulBondingPort) ;

                       txMpAalCfg &= ~((TXMP_GROUP_EN | TXMP_GROUP_SHAPER_MASK) <<
                             (m_ulMpIdxs[ulVcid] * TXMP_GROUP_SIZE));
                       SET_MPAALCFG(ulBondingPort, txMpAalCfg) ;
                       m_ulMpIdxs[ulVcid] = INVALID_INDEX;
                    }
                    break;

                 default: {

                    UINT32 ulBondingPort = PORTID_TO_PORT (m_TxQIds[0].ulBondingPortId) ;
                    /* Update the group number in the MPAAL register to be the
                     * the highest shaper index.  Shapers in the same group will
                     * have the same port number for this XTM_CONNECTION object.
                     */
                    for( j = 0, pTxQId2 = m_TxQIds, ulShaperIdx = 0;
                          j < m_ulTxQIdsSize; j++, pTxQId2++ )
                    {
                       if(pTxQId2!=pTxQId && pTxQId2->ulPortId==pTxQId->ulPortId)
                       {
                          if( pTxQId2->ulQueueIndex > ulShaperIdx )
                             ulShaperIdx = pTxQId2->ulQueueIndex;
                       }
                    }

                    k = MPAALCFG(ulBondingPort) ;
                    k &= ~(TXMP_GROUP_SHAPER_MASK <<
                          (m_ulMpIdxs[ulVcid] * TXMP_GROUP_SIZE));
                    k |= (ulShaperIdx << ((m_ulMpIdxs[ulVcid] * TXMP_GROUP_SIZE) + TXMP_GROUP_SHAPER_SHIFT));
                    SET_MPAALCFG(ulBondingPort,k) ;

                    break;
                 }
              } /* switch (m_ulVcidsCount[ulVcid]) */
           }
        } /* ulFound */
    } /* for (i) */

    m_ulTxQIdsSize = ulTxQIdsSizeSave;
    memcpy(m_TxQIds, TxQIdsSave, m_ulTxQIdsSize * sizeof(XTMRT_TRANSMIT_QUEUE_ID));

    /* Check transmit queues that have been added. */
    for( j = 0, pTxQParms = m_Cfg.TransmitQParms;
         j < m_Cfg.ulTransmitQParmsSize; j++, pTxQParms++ )
    {
        for( i = 0, pTxQId = m_TxQIds, ulTxQIdsSize=m_ulTxQIdsSize, ulFound=0;
             i < ulTxQIdsSize; i++, pTxQId++ )
        {
            if( pTxQId->ucQosQId      == pTxQParms->ucQosQId &&
                pTxQId->ulPortId      == pTxQParms->ulPortId &&
                pTxQId->ucSubPriority == pTxQParms->ucSubPriority &&
                pTxQId->ulPtmPriority == pTxQParms->ulPtmPriority &&
                pTxQId->ucWeightAlg   == pTxQParms->ucWeightAlg &&
                pTxQId->ulWeightValue == pTxQParms->ulWeightValue )
            {
                ulFound = 1;
                break;
            }
        }

        if( ulFound == 0 )
        {
           /* Add the transmit queue. */
           BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;

           UINT32 ulPort = PORTID_TO_PORT(pTxQParms->ulPortId);
           UINT32 ulPtmPriIdx = pTxQParms->ulPtmPriority?
              (pTxQParms->ulPtmPriority - PTM_PRI_LOW) : 0;
           UINT32 ulVcid = m_ucVcids[ulPort][ulPtmPriIdx];
           UINT32 ulBondingPort = MAX_INTERFACES ;

           if (pTxQParms->ulBondingPortId != PORT_PHY_INVALID) {
              ulBondingPort = PORTID_TO_PORT(pTxQParms->ulBondingPortId);
           }

           if( pTxQParms->ulPtmPriority == PTM_PRI_HIGH ) {
              ulVcid = m_ucVcidsTx[ulVcid];
           }

           bxStatus = ReserveTxShapers(pTxQParms, 1);

           if (bxStatus == XTMSTS_SUCCESS) {

              if (pTxQParms->ulBondingPortId  != PORT_PHY_INVALID) {
                 /* Reserve HW Shapers */
                 ReserveTxHwShapers(pTxQParms, 1) ;
              }
           }

           if( bxStatus == XTMSTS_SUCCESS && m_ulVcidsCount[ulVcid] > 1 ) {
              bxStatus = ReserveMultiPriorityIndex( ulVcid, ulBondingPort );
           }

           if( bxStatus == XTMSTS_SUCCESS )
           {
              /* ReserveTxShapers created a new XTMRT_TRANSMIT_QUEUE_ID
               * array entry.  Call the bcmxtmrt driver to setup the DMA.
               */
              (*m_pfnXtmrtReq) (m_hDev, XTMRT_CMD_SET_TX_QUEUE,
                                &m_TxQIds[m_ulTxQIdsSize - 1]);
           }
        }
    }

    XtmOsReleaseSem(m_ulConnSem);
} /* CheckTransmitQueues */


/***************************************************************************
 * Function Name: GetTdtEntry
 * Description  : Returns a traffic descriptor table entry for the specified
 *                index.
 * Returns      : Traffic descriptor table entry.
 ***************************************************************************/
PXTM_TRAFFIC_DESCR_PARM_ENTRY XTM_CONNECTION::GetTdtEntry( UINT32 ulTdtIdx )
{
    static XTM_TRAFFIC_DESCR_PARM_ENTRY Entry =
        {0, TDT_ATM_NO_TRAFFIC_DESCRIPTOR, 0, 0, 0, 0, SC_UBR};
    PXTM_TRAFFIC_DESCR_PARM_ENTRY pFound = &Entry;
    PXTM_TRAFFIC_DESCR_PARM_ENTRY pEntry;
    UINT32 i;

    for( i = 0, pEntry = m_pTrafficDescrEntries; i<m_ulNumTrafficDescrEntries;
        i++, pEntry++ )
    {
        if( pEntry->ulTrafficDescrIndex == ulTdtIdx )
        {
            pFound = pEntry;
            break;
        }
    }

    return( pFound );
} /* GetTdtEntry */


/***************************************************************************
 * Function Name: CopyVcids
 * Description  : Copies VCIDs for this connection to an array.
 * Returns      : number of VCIDs copied
 ***************************************************************************/
UINT32 XTM_CONNECTION::CopyVcids( UINT8 *pucVcids )
{
    UINT32 ulNum = 0;

    for( UINT32 i = 0; i < MAX_INTERFACES; i++ )
        for( UINT32 j = 0; j < MAX_PTM_PRIORITIES; j++ )
            if( m_ucVcids[i][j] != INVALID_VCID)
                pucVcids[ulNum++] = m_ucVcids[i][j];

    return( ulNum );
}


/***************************************************************************
 * XTM_CONNECTION_TABLE Class
 ***************************************************************************/


/***************************************************************************
 * Function Name: XTM_CONNECTION_TABLE
 * Description  : Constructor for the XTM connection table class.
 * Returns      : None.
 ***************************************************************************/
XTM_CONNECTION_TABLE::XTM_CONNECTION_TABLE( void )
{
   UINT32 j;
   UINT32 txQIndex ;

    m_ulMuSem = XtmOsCreateSem(1);
    memset(m_pConnTable, 0x00, sizeof(m_pConnTable));
   for (txQIndex = 0; txQIndex < MAX_TRANSMIT_QUEUES; txQIndex++)
      m_TxQs [txQIndex] = XTM_TX_Q_AVAILABLE ;

   for( j = 0; j < MAX_SHAPERS; j++ )
   {
      ulSPP_SstCtrl[j] = 0 ;
      ulSPP_SstBt[j] = 0 ;
   }

   ulSPP_MpAalCfg = 0 ;
} /* XTM_CONNECTION_TABLE */


/***************************************************************************
 * Function Name: ~XTM_CONNECTION_TABLE
 * Description  : Destructor for the XTM connection table class.
 * Returns      : None.
 ***************************************************************************/
XTM_CONNECTION_TABLE::~XTM_CONNECTION_TABLE( void )
{
   UINT32 j;
   UINT32 txQIndex ;

   for (txQIndex = 0; txQIndex < MAX_TRANSMIT_QUEUES; txQIndex++)
      m_TxQs [txQIndex] = XTM_TX_Q_AVAILABLE ;

   for( j = 0; j < MAX_SHAPERS; j++ )
   {
      ulSPP_SstCtrl[j] = 0 ;
      ulSPP_SstBt[j] = 0 ;
   }

   ulSPP_MpAalCfg = 0 ;
    XtmOsDeleteSem(m_ulMuSem);
} /* ~XTM_CONNECTION_TABLE */


/***************************************************************************
 * Function Name: Add
 * Description  : Adds a connection object to the connection table.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION_TABLE::Add( XTM_CONNECTION *pConn )
{
    BCMXTM_STATUS bxStatus = XTMSTS_NOT_FOUND;
    XTM_ADDR ConnAddr;
    UINT32 i, ulPm;
    UINT32 ulUnused = (UINT32) -1;

    XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

    pConn->GetAddr( &ConnAddr );
    ulPm = ConnAddr.u.Conn.ulPortMask ;

    /* Validate the portmask. */
    if( ulPm < PORT_PHY0_FAST || ulPm > (PORT_PHY0_FAST |
        PORT_PHY0_INTERLEAVED | PORT_PHY1_FAST | PORT_PHY1_INTERLEAVED) )
    {
        bxStatus = XTMSTS_PARAMETER_ERROR;
    }
    else
    {
        /* Check if the connection is already in the table. */
        for( i = 0; i < MAX_CONN_TABLE_ENTRIES; i++ )
        {
            if( m_pConnTable[i] )
            {
                if( pConn == m_pConnTable[i] )
                {
                    bxStatus = XTMSTS_SUCCESS;
                    break;
                }
                else
                {
                    XTM_ADDR TblAddr;
                    m_pConnTable[i]->GetAddr( &TblAddr );
                    UINT32 ulPortMatch, ulAddrEqual;

                    /* Mask the port bits.  If the ATM/PTM addresses then
                     * match, this is an invalid entry because it conflicts
                     * with an existing ATM/PTM address.  It has the same
                     * VPI/VCI or PTM flow and at least one of the ports match.
                     */
		              if( (TblAddr.ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM )
                    {
                        ulPortMatch = (TblAddr.u.Vcc.ulPortMask &
                            ConnAddr.u.Vcc.ulPortMask);
                        ulAddrEqual =
                            (TblAddr.u.Vcc.usVpi == ConnAddr.u.Vcc.usVpi) &&
                            (TblAddr.u.Vcc.usVci == ConnAddr.u.Vcc.usVci);
                    }
                    else
                    {
                        ulPortMatch = (TblAddr.u.Flow.ulPortMask &
                            ConnAddr.u.Flow.ulPortMask);
                        ulAddrEqual =
                            (TblAddr.u.Flow.ulPtmPriority ==
                             ConnAddr.u.Flow.ulPtmPriority);
                    }
                    if( ulPortMatch && ulAddrEqual )
                    {
                        /* Address conflict. */
                        bxStatus = XTMSTS_PARAMETER_ERROR;
                        break;
                    }
                }
            }
            else
                if( ulUnused == (UINT32) -1 )
                    ulUnused = i;
        }

        /* Add the connection to the table if it is not already in the table and
         * there is space.
         */
        if( bxStatus == XTMSTS_NOT_FOUND && ulUnused != (UINT32) -1 )
        {
            m_pConnTable[ulUnused] = pConn;
            bxStatus = XTMSTS_SUCCESS;
        }
    }

    XtmOsReleaseSem(m_ulMuSem);

    return( bxStatus );
} /* Add */


/***************************************************************************
 * Function Name: Remove
 * Description  : Removes a connection object from the connection table.
 * Returns      : None.
 ***************************************************************************/
void XTM_CONNECTION_TABLE::Remove( XTM_CONNECTION *pConn )
{
    UINT32 i;

    XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

    for( i = 0; i < MAX_CONN_TABLE_ENTRIES; i++ )
    {
        if( pConn == m_pConnTable[i] )
        {
            m_pConnTable[i] = NULL;
            break;
        }
    }

    XtmOsReleaseSem(m_ulMuSem);
} /* Remove */


/***************************************************************************
 * Function Name: Get
 * Description  : Returns a connection object from the connection table.
 * Returns      : pointer to an XTM_CONNECTION object or NULL
 ***************************************************************************/
XTM_CONNECTION *XTM_CONNECTION_TABLE::Get( PXTM_ADDR pAddr )
{
    XTM_CONNECTION *pConn = NULL;
    XTM_ADDR Addr;
    UINT32 i;

    XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

    for( i = 0; i < MAX_CONN_TABLE_ENTRIES; i++ )
    {
        if( m_pConnTable[i] )
        {
            m_pConnTable[i]->GetAddr( &Addr );
            if( XTM_ADDR_CMP(pAddr, &Addr) )
            {
                /* Connection found. */
                pConn = m_pConnTable[i];
                break;
            }
        }
    }

    XtmOsReleaseSem(m_ulMuSem);

    return( pConn );
} /* Get */


/***************************************************************************
 * Function Name: GetForPortId
 * Description  : Returns a connection object from the connection table.
 *                The ulPortMask in the supplied parameter can contain
 *                a subset of ports.
 * Returns      : pointer to an XTM_CONNECTION object or NULL
 ***************************************************************************/
XTM_CONNECTION *XTM_CONNECTION_TABLE::GetForPortId( PXTM_ADDR pAddr )
{
    XTM_CONNECTION *pConn = NULL;
    XTM_ADDR Addr;
    UINT32 i;

    XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

    for( i = 0; i < MAX_CONN_TABLE_ENTRIES; i++ )
    {
        if( m_pConnTable[i] )
        {
            m_pConnTable[i]->GetAddr( &Addr );
            Addr.u.Conn.ulPortMask &= pAddr->u.Conn.ulPortMask;
            if( XTM_ADDR_CMP(pAddr, &Addr) )
            {
                /* Connection found. */
                pConn = m_pConnTable[i];
                break;
            }
        }
    }

    XtmOsReleaseSem(m_ulMuSem);

    return( pConn );
} /* GetForPortId */


/***************************************************************************
 * Function Name: Enum
 * Description  : Returns a connection object from the connection table for
 *                the specified index.
 * Returns      : pointer to an XTM_CONNECTION object or NULL
 ***************************************************************************/
XTM_CONNECTION *XTM_CONNECTION_TABLE::Enum( UINT32 *pulIdx )
{
    XTM_CONNECTION *pConn = NULL;
    UINT32 i;

    XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

    for( i = *pulIdx; i < MAX_CONN_TABLE_ENTRIES; i++ )
    {
        if( m_pConnTable[i] )
        {
            pConn = m_pConnTable[i];
            break;
        }
    }

    *pulIdx = i + 1;

    XtmOsReleaseSem(m_ulMuSem);

    return( pConn );
} /* Enum */

/***************************************************************************
 * Function Name: ReserveTxQIndex
 * Description  : Reserves an available tx Q.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION_TABLE::ReserveTxQIndex( UINT32 *pTxQIndex )
{
   UINT32        TxQIdSize ;
   BCMXTM_STATUS status = XTMSTS_SUCCESS ;

   XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

   for (TxQIdSize = 0 ; TxQIdSize < MAX_TRANSMIT_QUEUES ; TxQIdSize++) {
      if (m_TxQs [TxQIdSize] == XTM_TX_Q_AVAILABLE)
         break ;
   }

   if (TxQIdSize == MAX_TRANSMIT_QUEUES)
      status = XTMSTS_RESOURCE_ERROR ;
   else {
      m_TxQs [TxQIdSize] = XTM_TX_Q_NOT_AVAILABLE ;
      *pTxQIndex = TxQIdSize ;
   }

   XtmOsReleaseSem(m_ulMuSem);

   return( status );
} /* ReserveTxQIndex */

/***************************************************************************
 * Function Name: UnReserveTxQIndex
 * Description  : Unreserves a valid Tx Q given.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_CONNECTION_TABLE::UnReserveTxQIndex( UINT32 TxQIndex )
{
   BCMXTM_STATUS status = XTMSTS_SUCCESS ;

   XtmOsRequestSem(m_ulMuSem, MAX_TIMEOUT);

   if (TxQIndex < MAX_TRANSMIT_QUEUES &&
       m_TxQs [TxQIndex] == XTM_TX_Q_NOT_AVAILABLE) {

      m_TxQs [TxQIndex] = XTM_TX_Q_AVAILABLE ;
   }
   else
      status = XTMSTS_PARAMETER_ERROR ;

   XtmOsReleaseSem(m_ulMuSem);

   return( status );
} /* UnReserveTxQIndex */
