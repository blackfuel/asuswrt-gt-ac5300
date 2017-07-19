/*
<:copyright-broadcom 
 
 Copyright (c) 2007 Broadcom Corporation 
 All Rights Reserved 
 No portions of this material may be reproduced in any form without the 
 written permission of: 
          Broadcom Corporation 
          5300 California Avenue
          Irvine, California 92617 
 All information contained in this document is Broadcom Corporation 
 company private, proprietary, and trade secret. 
 
:>
*/
/***************************************************************************
 * File Name  : xtmoamhandler.cpp
 *
 * Description: This file contains the implementation for the XTM OAM handler
 *              class.  This class handles sending an ATM OAM F4 or OAM F5
 *              loopback request and waiting for the loopback response.
 *              It also handles responding to a received ATM OAM F4 or OAM F5
 *              loopback request.
 ***************************************************************************/

#include "xtmcfgimpl.h"

#define VCI_OAM_F4_SEGMENT                  3
#define VCI_OAM_F4_END_TO_END               4
#define OAM_TYPE_FUNCTION_BYTE_OFS          0
#define OAM_LB_INDICATION_BYTE_OFS          1
#define OAM_LB_CORRELATION_TAG_BYTE_OFS     2
#define OAM_LB_LOCATION_ID_BYTE_OFS         6
#define OAM_LB_SRC_ID_BYTE_OFS              22
#define OAM_LB_UNUSED_BYTE_OFS              38
#define OAM_LB_CRC_BYTE_OFS                 46
#define OAM_LB_CORRELATION_TAG_LEN          4
#define OAM_LB_LOCATION_ID_LEN              16
#define OAM_LB_SRC_ID_LEN                   16
#define OAM_LB_UNUSED_BYTE_LEN              8
#define OAM_LB_CRC_BYTE_LEN                 2
#define OAM_FAULT_MGMT_AIS                  0x10
#define OAM_FAULT_MGMT_RDI                  0x11
#define OAM_FAULT_MGMT_LB                   0x18
#define OAM_FAULT_MGMT_LB_REQUEST           1
#define OAM_FAULT_MGMT_LB_RESPONSE          0
#define OAM_LB_UNUSED_BYTE_DEFAULT          0x6a
#define OAM_RSP_RECEIVED                    0x80000000

#define CRC10_TABLE_NOT_INITIALIZED         0x1234
UINT16 XTM_OAM_HANDLER::ms_usByteCrc10Table[] = {CRC10_TABLE_NOT_INITIALIZED};

/***************************************************************************
 * Function Name: XTM_OAM_HANDLER
 * Description  : Constructor for the XTM processor class.
 * Returns      : None.
 ***************************************************************************/
XTM_OAM_HANDLER::XTM_OAM_HANDLER( void )
{
    m_pfnXtmrtReq = NULL;
    m_ulRegistered = 0;
    memset(m_PendingOamReqs, 0x00, sizeof(m_PendingOamReqs));
    memset(m_ulRspTimes, 0x00, sizeof(m_ulRspTimes));

    GenByteCrc10Table();
} /* XTM_OAM_HANDLER */


/***************************************************************************
 * Function Name: ~XTM_OAM_HANDLER
 * Description  : Destructor for the XTM processor class.
 * Returns      : None.
 ***************************************************************************/
XTM_OAM_HANDLER::~XTM_OAM_HANDLER( void )
{
    Uninitialize();
} /* ~XTM_OAM_HANDLER */


/***************************************************************************
 * Function Name: Initialize
 * Description  : Initializes the object.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_OAM_HANDLER::Initialize( FN_XTMRT_REQ pfnXtmrtReq )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XTMRT_CELL_HDLR CellHdlr;

    m_pfnXtmrtReq = pfnXtmrtReq;

    if( !m_ulRegistered )
    {
        /* Register an OAM cell handler with the bcmxtmrt network driver. */
        CellHdlr.ulCellHandlerType = CELL_HDLR_OAM;
        CellHdlr.pfnCellHandler = XTM_OAM_HANDLER::ReceiveOamCb;
        CellHdlr.pContext = this;
        if( (*m_pfnXtmrtReq) (NULL, XTMRT_CMD_REGISTER_CELL_HANDLER,
            &CellHdlr) == 0 )
        {
            m_ulRegistered = 1;
        }
        else
            XtmOsPrintf("bcmxtmcfg: Error registering OAM handler\n");
    }
    else
        bxStatus = XTMSTS_STATE_ERROR;

    return( bxStatus );
} /* Initialize */


/***************************************************************************
 * Function Name: Uninitialize
 * Description  : Uninitializes the object.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_OAM_HANDLER::Uninitialize( void )
{
    if( m_ulRegistered )
    {
        XTMRT_CELL_HDLR CellHdlr;

        /* Unregister the OAM cell handler with the bcmxtmrt network driver. */
        CellHdlr.ulCellHandlerType = CELL_HDLR_OAM;
        CellHdlr.pfnCellHandler = XTM_OAM_HANDLER::ReceiveOamCb;
        CellHdlr.pContext = this;
        (*m_pfnXtmrtReq)(NULL, XTMRT_CMD_UNREGISTER_CELL_HANDLER, &CellHdlr);
        m_ulRegistered = 0;
    }

    return( XTMSTS_SUCCESS );
} /* Uninitialize */


/***************************************************************************
 * Function Name: SendCell
 * Description  : Sends an OAM cell.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_OAM_HANDLER::SendCell( PXTM_ADDR pConnAddr,
    PXTM_OAM_CELL_INFO pOamCellInfo, XTM_CONNECTION_TABLE *pConnTable )
{
    BCMXTM_STATUS bxStatus = XTMSTS_PARAMETER_ERROR;
    XTM_CONNECTION *pConn = NULL;
    XTMRT_HANDLE hDev = INVALID_HANDLE;
    PXTM_ADDR pReqAddr = NULL;
    UINT32 *pulRspTime = NULL;
    UINT32 i;
    UINT8 ucCircuitType = pOamCellInfo->ucCircuitType;

    /* Validate the OAM parameters and find the bcmxtmrt driver instance to
     * send the OAM cell to.
     */
	 if( ( (pConnAddr->ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM ) &&
        (ucCircuitType == CTYPE_OAM_F5_SEGMENT ||
         ucCircuitType == CTYPE_OAM_F5_END_TO_END) )
    {
        /* OAM F5 cell to send. Get the bcmxtmrt driver instance handle. */
        if( (pConn = pConnTable->GetForPortId( pConnAddr )) != NULL )
        {
            if( (hDev = pConn->GetHandle()) != INVALID_HANDLE )
                bxStatus = XTMSTS_SUCCESS;
        }
    }
    else
    {
	 	  if( ( (pConnAddr->ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM ) &&
            ((ucCircuitType == CTYPE_OAM_F4_SEGMENT &&
              pConnAddr->u.Vcc.usVci == VCI_OAM_F4_SEGMENT) ||
             (ucCircuitType == CTYPE_OAM_F4_END_TO_END &&
              pConnAddr->u.Vcc.usVci == VCI_OAM_F4_END_TO_END)) )
        {
            /* OAM F4 cell.  Send it on the first connection that has a valid
             * bcmxtmrt driver instance handle and the port matches.
             */
            XTM_ADDR Addr;
            i = 0;

#if defined(CONFIG_BCM96362) ||  defined(CONFIG_BCM96328)
            XP_REGS->ulTxOamCfg &=
                ~(TXOAM_F4_SEG_VPI_MASK | TXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulRxOamCfg &=
                ~(RXOAM_F4_SEG_VPI_MASK | RXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulTxOamCfg |=
                (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
            XP_REGS->ulRxOamCfg |=
                (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
#endif

            while( (pConn = pConnTable->Enum( &i )) != NULL )
            {
                pConn->GetAddr( &Addr );
                Addr.u.Vcc.ulPortMask &= pConnAddr->u.Vcc.ulPortMask;
                if( pConnAddr->u.Vcc.ulPortMask == Addr.u.Vcc.ulPortMask &&
                    (hDev = pConn->GetHandle()) != INVALID_HANDLE )
                {
                    bxStatus = XTMSTS_SUCCESS;
                    break;
                }
            }
        }
    }

    if( bxStatus == XTMSTS_SUCCESS )
    {
        XTMRT_CELL Cell;
        UINT8 *pData = Cell.ucData;

        memcpy(&Cell.ConnAddr, pConnAddr, sizeof(XTM_ADDR));
        Cell.ucCircuitType = ucCircuitType;

        pData[OAM_TYPE_FUNCTION_BYTE_OFS] = OAM_FAULT_MGMT_LB;
        pData[OAM_LB_INDICATION_BYTE_OFS] = OAM_FAULT_MGMT_LB_REQUEST;
        memset( &pData[OAM_LB_CORRELATION_TAG_BYTE_OFS], 0xbc, sizeof(int) );
        if( ucCircuitType == CTYPE_OAM_F5_END_TO_END ||
            ucCircuitType == CTYPE_OAM_F4_END_TO_END )
        {
            /* 16 bytes location ID and 16 bytes source ID */
            memset( &pData[OAM_LB_LOCATION_ID_BYTE_OFS], 0xff,
                OAM_LB_LOCATION_ID_LEN + OAM_LB_SRC_ID_LEN ); 
        }
        else
        {
            /* for segment one, we need to have NODE ID  agreement between
             * our modem and the network nodes; for now, I just assume
             * we are node src_id_3-0 now and the other end of the segment is
             * locationId_3-0
             */
            memset(&pData[OAM_LB_LOCATION_ID_BYTE_OFS], 0xff, sizeof(int));
            memset(&pData[OAM_LB_LOCATION_ID_BYTE_OFS+4], 0xff, sizeof(int));
            memset(&pData[OAM_LB_LOCATION_ID_BYTE_OFS+8], 0xff, sizeof(int));
            memset(&pData[OAM_LB_LOCATION_ID_BYTE_OFS+12],0xff, sizeof(int));

            memset(&pData[OAM_LB_SRC_ID_BYTE_OFS], 0xff, sizeof(int));
            memset(&pData[OAM_LB_SRC_ID_BYTE_OFS+4], 0xff, sizeof(int));
            memset(&pData[OAM_LB_SRC_ID_BYTE_OFS+8], 0xff, sizeof(int));
            memset(&pData[OAM_LB_SRC_ID_BYTE_OFS+12], 0xff, sizeof(int));
        }

        memset( &pData[OAM_LB_UNUSED_BYTE_OFS], OAM_LB_UNUSED_BYTE_DEFAULT,
            OAM_LB_UNUSED_BYTE_LEN );
        memset( &pData[OAM_LB_CRC_BYTE_OFS], 0, OAM_LB_CRC_BYTE_LEN );

        if( XtmOsChipRev() < CHIP_REV_BCM6368B0 )
        {
            UINT16 usCrc10 = UpdateCrc10ByBytes( 0, pData, CELL_PAYLOAD_SIZE );
            pData[OAM_LB_CRC_BYTE_OFS]     = (UINT8) (usCrc10 >> 8);
            pData[OAM_LB_CRC_BYTE_OFS + 1] = (UINT8) (usCrc10 & 0xff);
        }

        /* Save OAM circuit type and VCC address to match with response.
         * Save the OAM circuit type in the "ulTrafficType" field.
         */
        for( i = 0; i < sizeof(m_PendingOamReqs) / sizeof(XTM_ADDR); i++ )
        {
            if( m_PendingOamReqs[i].ulTrafficType == 0 )
            {
                pReqAddr = &m_PendingOamReqs[i];
                pulRspTime = &m_ulRspTimes[i];
                break;
            }
        }

        if( pReqAddr )
        {
            UINT32 ulTotalTime = 0;
            UINT32 ulIterations = 0;
            UINT32 ulTimeout = (pOamCellInfo->ulTimeout) ?
                pOamCellInfo->ulTimeout : 1;

            pOamCellInfo->ulSent = pOamCellInfo->ulReceived = 0;
            pOamCellInfo->ulMinRspTime = ulTimeout;
            for( i = 0; i < pOamCellInfo->ulRepetition; i++ )
            {
                memcpy(pReqAddr, pConnAddr, sizeof(XTM_ADDR));
                pReqAddr->ulTrafficType = ucCircuitType;
                *pulRspTime = XtmOsTickGet();

                /* Send to OAM cell to the bcmxtmrt driver to send. */
                if( (*m_pfnXtmrtReq) (hDev, XTMRT_CMD_SEND_CELL, &Cell) == 0 )
                {
                    /* Wait for the corresponding OAM response. */
                    const UINT32 ulOamIntervalMs = 100;

                    pOamCellInfo->ulSent++;
                    bxStatus = XTMSTS_TIMEOUT;
                    ulIterations = ulTimeout / ulOamIntervalMs;
                    while( ulIterations-- )
                    {
                        if( (pReqAddr->ulTrafficType & OAM_RSP_RECEIVED) != 0 )
                        {
                            pOamCellInfo->ulReceived++;
                            ulTotalTime += *pulRspTime;
                            if( *pulRspTime < pOamCellInfo->ulMinRspTime )
                                pOamCellInfo->ulMinRspTime = *pulRspTime;
                            if( *pulRspTime > pOamCellInfo->ulMaxRspTime )
                                pOamCellInfo->ulMaxRspTime = *pulRspTime;
                            bxStatus = XTMSTS_SUCCESS;
                            break;
                        }

                        XtmOsDelay( ulOamIntervalMs );
                    }
                    pReqAddr->ulTrafficType = 0;

                    if( bxStatus == XTMSTS_TIMEOUT )
                    {
                        XtmOsPrintf("bcmxtmcfg: OAM loopback response not "
                            "received on VCC %d.%d.%d\n",
                            pConnAddr->u.Vcc.ulPortMask,
                            pConnAddr->u.Vcc.usVpi, pConnAddr->u.Vcc.usVci);
                    }
                }
                else
                {
                    if( pReqAddr )
                        pReqAddr->ulTrafficType = 0;
                    bxStatus = XTMSTS_ERROR;
                }
            }

            if( pOamCellInfo->ulReceived )
            {
                bxStatus = XTMSTS_SUCCESS;
                pOamCellInfo->ulAvgRspTime =
                    ulTotalTime / pOamCellInfo->ulReceived;
            }
            else
            {
                bxStatus = XTMSTS_TIMEOUT;
                pOamCellInfo->ulAvgRspTime = 0;
                pOamCellInfo->ulMinRspTime = 0;
            }
        }
        else
            bxStatus = XTMSTS_RESOURCE_ERROR;
    }

    return( bxStatus );
} /* SendCell */


/***************************************************************************
 * Function Name: ReceiveOamCb
 * Description  : Processes a received OAM cell.
 * Returns      : 0 if successful, non-0 if not
 ***************************************************************************/
int XTM_OAM_HANDLER::ReceiveOamCb( XTMRT_HANDLE hDev, UINT32 ulCommand,
    void *pParam, void *pContext )
{
    const UINT32 ulAtmHdrSize = 4; /* no HEC */
    XTM_OAM_HANDLER *pThis = (XTM_OAM_HANDLER *) pContext;
    PXTMRT_CELL pCell = (PXTMRT_CELL) pParam;
    UINT8 *pData = pCell->ucData;
    PXTM_ADDR pConnAddr = &pCell->ConnAddr;
    UINT16 usCrc10;
    UINT32 i;

    /* Remove ATM header. */
    for( i = 0; i < CELL_PAYLOAD_SIZE; i++ )
        pCell->ucData[i] = pCell->ucData[i + ulAtmHdrSize];

    pConnAddr->ulTrafficType = pCell->ucCircuitType;
    switch( pData[OAM_TYPE_FUNCTION_BYTE_OFS] )
    {
    case OAM_FAULT_MGMT_LB:
        if( pData[OAM_LB_INDICATION_BYTE_OFS] == OAM_FAULT_MGMT_LB_RESPONSE )
        {
            /* An OAM loopback response has been received.  Try to find the
             * associated request.
             */
            UINT32 ulSize = sizeof(pThis->m_PendingOamReqs) / sizeof(XTM_ADDR);
            for( i = 0; i < ulSize; i++ )
            {
                if(XTM_ADDR_CMP(&pThis->m_PendingOamReqs[i], pConnAddr))
                {
                    /* Found. */
                    pThis->m_ulRspTimes[i] =
                        XtmOsTickGet() - pThis->m_ulRspTimes[i];
                    pThis->m_PendingOamReqs[i].ulTrafficType|=OAM_RSP_RECEIVED;
                    break;
                }
            }
        }
        else
        {
            /* Send an OAM loopback response. */
            pData[OAM_LB_INDICATION_BYTE_OFS] = OAM_FAULT_MGMT_LB_RESPONSE;
            memset( &pData[OAM_LB_CRC_BYTE_OFS], 0, OAM_LB_CRC_BYTE_LEN );
            if( XtmOsChipRev() < CHIP_REV_BCM6368B0 )
            {
                usCrc10 = pThis->UpdateCrc10ByBytes(0,pData,CELL_PAYLOAD_SIZE);
                pData[OAM_LB_CRC_BYTE_OFS]     = (UINT8) (usCrc10 >> 8);
                pData[OAM_LB_CRC_BYTE_OFS + 1] = (UINT8) (usCrc10 & 0xff);
            }

#if defined(CONFIG_BCM96362) ||  defined(CONFIG_BCM96328)
            if( pCell->ucCircuitType == CTYPE_OAM_F4_SEGMENT ||
                pCell->ucCircuitType == CTYPE_OAM_F4_END_TO_END )
            {
                XP_REGS->ulTxOamCfg &=
                    ~(TXOAM_F4_SEG_VPI_MASK | TXOAM_F4_E2E_VPI_MASK);
                XP_REGS->ulRxOamCfg &=
                    ~(RXOAM_F4_SEG_VPI_MASK | RXOAM_F4_E2E_VPI_MASK);
                XP_REGS->ulTxOamCfg |=
                    (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
                XP_REGS->ulRxOamCfg |=
                    (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
            }
#endif

            (*pThis->m_pfnXtmrtReq) (hDev, XTMRT_CMD_SEND_CELL, pCell);
        }
        break;

    case OAM_FAULT_MGMT_AIS:
        /* Send an OAM RDI. */
        pData[OAM_TYPE_FUNCTION_BYTE_OFS] = OAM_FAULT_MGMT_RDI;
        memset( &pData[OAM_LB_CRC_BYTE_OFS], 0, OAM_LB_CRC_BYTE_LEN );
        if( XtmOsChipRev() < CHIP_REV_BCM6368B0 )
        {
            usCrc10 = pThis->UpdateCrc10ByBytes( 0, pData, CELL_PAYLOAD_SIZE );
            pData[OAM_LB_CRC_BYTE_OFS]     = (UINT8) (usCrc10 >> 8);
            pData[OAM_LB_CRC_BYTE_OFS + 1] = (UINT8) (usCrc10 & 0xff);
        }

#if defined(CONFIG_BCM96362) ||  defined(CONFIG_BCM96328)
        if( pCell->ucCircuitType == CTYPE_OAM_F4_SEGMENT ||
            pCell->ucCircuitType == CTYPE_OAM_F4_END_TO_END )
        {
            XP_REGS->ulTxOamCfg &=
                ~(TXOAM_F4_SEG_VPI_MASK | TXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulRxOamCfg &=
                ~(RXOAM_F4_SEG_VPI_MASK | RXOAM_F4_E2E_VPI_MASK);
            XP_REGS->ulTxOamCfg |=
                (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
            XP_REGS->ulRxOamCfg |=
                (pConnAddr->u.Vcc.usVpi << 8) | pConnAddr->u.Vcc.usVpi;
        }
#endif

        (*pThis->m_pfnXtmrtReq) (hDev, XTMRT_CMD_SEND_CELL, pCell);
        break;
    }

    return( 0 );
} /* ReceiveOamCb */


/***************************************************************************
 * Function Name: GenByteCrc10Table
 * Description  : Generate the table of CRC-10 remainders for all possible
 *                bytes.
 * Returns      : None.
 ***************************************************************************/
void XTM_OAM_HANDLER::GenByteCrc10Table( void )
{
    if( ms_usByteCrc10Table[0] == CRC10_TABLE_NOT_INITIALIZED )
    {
        int i, j;
        unsigned short crc10_accum;

        for( i = 0;  i < 256;  i++ )
        {
            crc10_accum = ((unsigned short) i << 2);
            for ( j = 0;  j < 8;  j++ )
                if ((crc10_accum <<= 1) & 0x400) crc10_accum ^= 0x633;

            ms_usByteCrc10Table[i] = crc10_accum;
        }
    }
} // GenByteCrc10Table


/***************************************************************************
 * Function Name: UpdateCrc10ByBytes
 * Description  : Update the data block CRC-10 remainder one byte at a time.
 * Returns      : None.
 ***************************************************************************/
UINT16 XTM_OAM_HANDLER::UpdateCrc10ByBytes( UINT16 usCrc10Accum, UINT8 *pBuf,
    int nDataBlkSize )
{
    int i;

    for( i = 0;  i < nDataBlkSize;  i++ )
    {
        usCrc10Accum = ((usCrc10Accum << 8) & 0x3ff)
            ^ ms_usByteCrc10Table[(usCrc10Accum >> 2) & 0xff]
            ^ *pBuf++;
    }

    return( usCrc10Accum );
} // UpdateCrc10ByBytes

