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
 * File Name  : xtmcfgmain.cpp (impl2)
 *
 * Description: This file contains the implementation for the XTM kernel entry
 *              point functions.
 ***************************************************************************/

/* Includes. */
#include "xtmcfgimpl.h"
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
#include "bcm_intr.h"
#endif


/* Globals. */
static XTM_PROCESSOR *g_pXtmProcessor = NULL;


/* Prototypes. */
extern "C" {
BCMXTM_STATUS BcmXtm_ChipInitialize( PXTM_INITIALIZATION_PARMS pInitParms,
    UINT32 *pulBusSpeed );
}


/***************************************************************************
 * Function Name: BcmXtm_Initialize
 * Description  : ATM/PTM processor initialization entry point function.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_Initialize( PXTM_INITIALIZATION_PARMS pInitParms )
{
    BCMXTM_STATUS bxStatus;

    if( g_pXtmProcessor == NULL )
    {
        XtmOsInitialize();

        if( (g_pXtmProcessor = new XTM_PROCESSOR) != NULL )
        {
            UINT32 ulBusSpeed = 0;

            bxStatus = BcmXtm_ChipInitialize( pInitParms, &ulBusSpeed );
            if( bxStatus == XTMSTS_SUCCESS )
            {
                if( (bxStatus = g_pXtmProcessor->Initialize( pInitParms,
                    bcmxtmrt_request )) != XTMSTS_SUCCESS )
                {
                    BcmXtm_Uninitialize();
                }
            }
        }
        else
            bxStatus = XTMSTS_ALLOC_ERROR;
    }
    else
        bxStatus = XTMSTS_STATE_ERROR;

    return( bxStatus );
} /* BcmXtm_Initialize */


/***************************************************************************
 * Function Name: BcmXtm_Uninitialize
 * Description  : ATM/PTM processor uninitialization entry point function.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_Uninitialize( void )
{
    XTM_PROCESSOR *pXtmProc = g_pXtmProcessor;

    g_pXtmProcessor = NULL;

    if( pXtmProc )
        delete pXtmProc;

    return( XTMSTS_SUCCESS );
} /* BcmXtm_Uninitialize */

/***************************************************************************
 * Function Name: BcmXtm_Configure
 * Description  : ATM/PTM processor configuration entry point function.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_Configure( PXTM_CONFIGURATION_PARMS pConfigParms )
{
    BCMXTM_STATUS bxStatus;

    if( g_pXtmProcessor != NULL )
    {
        /* Pass configuration data to XtmProcessor */
        bxStatus = g_pXtmProcessor->Configure(pConfigParms);
    }
    else
        bxStatus = XTMSTS_STATE_ERROR;

    return( bxStatus );
} /* BcmXtm_Configure */


/***************************************************************************
 * Function Name: BcmXtm_GetTrafficDescrTable
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetTrafficDescrTable( PXTM_TRAFFIC_DESCR_PARM_ENTRY
    pTrafficDescrTable, UINT32 *pulTrafficDescrTableSize )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetTrafficDescrTable( pTrafficDescrTable,
            pulTrafficDescrTableSize )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetTrafficDescrTable */


/***************************************************************************
 * Function Name: BcmXtm_SetTrafficDescrTable
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_SetTrafficDescrTable( PXTM_TRAFFIC_DESCR_PARM_ENTRY
    pTrafficDescrTable, UINT32  ulTrafficDescrTableSize )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->SetTrafficDescrTable( pTrafficDescrTable,
            ulTrafficDescrTableSize )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_SetTrafficDescrTable */


/***************************************************************************
 * Function Name: BcmXtm_GetInterfaceCfg
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetInterfaceCfg( UINT32 ulPortId, PXTM_INTERFACE_CFG
    pInterfaceCfg )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetInterfaceCfg( ulPortId, pInterfaceCfg )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetInterfaceCfg */


/***************************************************************************
 * Function Name: BcmXtm_SetInterfaceCfg
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_SetInterfaceCfg( UINT32 ulPortId, PXTM_INTERFACE_CFG
    pInterfaceCfg )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->SetInterfaceCfg( ulPortId, pInterfaceCfg )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_SetInterfaceCfg */


/***************************************************************************
 * Function Name: BcmXtm_GetConnCfg
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetConnCfg( PXTM_ADDR pConnAddr, PXTM_CONN_CFG pConnCfg )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetConnCfg( pConnAddr, pConnCfg )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetConnCfg */


/***************************************************************************
 * Function Name: BcmXtm_SetConnCfg
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_SetConnCfg( PXTM_ADDR pConnAddr, PXTM_CONN_CFG pConnCfg )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->SetConnCfg( pConnAddr, pConnCfg )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_SetConnCfg */


/***************************************************************************
 * Function Name: BcmXtm_GetConnAddrs
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetConnAddrs( PXTM_ADDR pConnAddrs, UINT32 *pulNumConns )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetConnAddrs( pConnAddrs, pulNumConns )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetConnAddrs */


/***************************************************************************
 * Function Name: BcmXtm_GetInterfaceStatistics
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetInterfaceStatistics( UINT32 ulPortId,
    PXTM_INTERFACE_STATS pStatistics, UINT32 ulReset )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetInterfaceStatistics(ulPortId, pStatistics, ulReset)
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetInterfaceStatistics */


/***************************************************************************
 * Function Name: BcmXtm_GetErrorStatistics
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetErrorStatistics( PXTM_ERROR_STATS pStatistics )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetErrorStatistics(pStatistics)
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_GetInterfaceStatistics */

/***************************************************************************
 * Function Name: BcmXtm_SetInterfaceLinkInfo
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_SetInterfaceLinkInfo( UINT32 ulPortId,
    PXTM_INTERFACE_LINK_INFO pLinkInfo )
{
    BCMXTM_STATUS bxStatus = XTMSTS_STATE_ERROR;

    if( g_pXtmProcessor )
    {
        bxStatus = g_pXtmProcessor->SetInterfaceLinkInfo(ulPortId, pLinkInfo); 
        /* If all the available link(s) are down, the SAR will have to be reset
         * to accommodate possible different traffic types.
         */
        g_pXtmProcessor->CheckAndResetSAR(ulPortId, pLinkInfo);
    }

    return( bxStatus );
} /* BcmXtm_SetInterfaceLinkInfo */


/***************************************************************************
 * Function Name: BcmAtm_SetInterfaceLinkInfo
 * Description  : Backward compatibility function.
 * Returns      : STS_SUCCESS if successful or error status.
 ***************************************************************************/

typedef struct AtmInterfaceLinkInfo
{
    UINT32 ulStructureId;
    UINT32 ulLinkState;
    UINT32 ulLineRate;
    UINT32 ulReserved[2];
} ATM_INTERFACE_LINK_INFO, *PATM_INTERFACE_LINK_INFO;

extern "C"
BCMXTM_STATUS BcmAtm_SetInterfaceLinkInfo( UINT32 ulInterfaceId,
    PATM_INTERFACE_LINK_INFO pInterfaceLinkInfo )
{
    XTM_INTERFACE_LINK_INFO LinkInfo;

    LinkInfo.ulLinkState = pInterfaceLinkInfo->ulLinkState;
    LinkInfo.ulLinkUsRate = pInterfaceLinkInfo->ulLineRate;
    LinkInfo.ulLinkDsRate = pInterfaceLinkInfo->ulLineRate;
    LinkInfo.ulLinkTrafficType = TRAFFIC_TYPE_ATM;

    return( BcmXtm_SetInterfaceLinkInfo( PORT_TO_PORTID(ulInterfaceId),
        &LinkInfo ) );
} /* BcmAtm_SetInterfaceLinkInfo */

extern "C"
BCMXTM_STATUS BcmAtm_GetInterfaceId( UINT8 ucPhyPort, UINT32 *pulInterfaceId )
{
    *pulInterfaceId = 0;
    return( XTMSTS_SUCCESS );
} // BcmAtm_GetInterfaceId

/***************************************************************************
 * Function Name: BcmXtm_SendOamCell
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_SendOamCell( PXTM_ADDR pConnAddr,
    PXTM_OAM_CELL_INFO pOamCellInfo)
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->SendOamCell( pConnAddr, pOamCellInfo )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_SendOamCell */


/***************************************************************************
 * Function Name: BcmXtm_CreateNetworkDevice
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_CreateNetworkDevice( PXTM_ADDR pConnAddr,
    char *pszNetworkDeviceName )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->CreateNetworkDevice( pConnAddr,
            pszNetworkDeviceName )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_CreateNetworkDevice */


/***************************************************************************
 * Function Name: BcmXtm_DeleteNetworkDevice
 * Description  : 
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_DeleteNetworkDevice( PXTM_ADDR pConnAddr )
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->DeleteNetworkDevice( pConnAddr )
        : XTMSTS_STATE_ERROR );
} /* BcmXtm_DeleteNetworkDevice */

/***************************************************************************
 * Function Name: BcmXtm_GetBondingInfo
 * Description  : If bonding enabled, return success with info.
 *                If non-bonding, return Non-Success message.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_GetBondingInfo ( PXTM_BOND_INFO pBondingInfo)
{
    return( (g_pXtmProcessor)
        ? g_pXtmProcessor->GetBondingInfo( pBondingInfo )
        : XTMSTS_STATE_ERROR ) ;
} /* BcmXtm_GetBondingInfo */


/***************************************************************************
 * Function Name: BcmXtm_ChipInitialization (63268/6318/63138/63381/63148)
 * Description  : Chip specific initialization.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
extern "C"
BCMXTM_STATUS BcmXtm_ChipInitialize( PXTM_INITIALIZATION_PARMS pInitParms,
    UINT32 *pulBusSpeed )
{
    return( XTMSTS_SUCCESS );
}  /* BcmXtm_ChipInitialize */

