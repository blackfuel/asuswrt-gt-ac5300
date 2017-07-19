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

/*                   Copyright(c) 2002-2013 Broadcom, Inc.                    */
////////////////////////////////////////////////////////////////////////////////
/// \file PonMgrFec.c
/// \brief Forward Error Correction configuration module
/// \author LiangW
/// \date 1/25 2013
///
/// This version provides implementations for BCM55030 based devices
///
/// This modules wraps the low level of the ONUs Forward Error Correction.  It
/// provided two interfaces, an advanced independent RX/TX configuration, and
/// an IEEE compliant simple interface.  The IEEE interface implements
/// functionality to support the following attributes:
///
///   - aFECAbility
///   - aFECmode
///   - aFECCorrectedBlocks
///   - aFECUncorrectableBlocks
///
/// Along with the IEEE standard FEC OAM there is also functionality to support
/// the following Teknovus extensions:
///
///   - OamExtAttrFECMode
////////////////////////////////////////////////////////////////////////////////
#include "PonMgrFec.h"
#include "bcm_epon_cfg.h"
#include "OntmMpcp.h"
#include "rdpa_api.h"

typedef struct
    {
    BOOL rx;
    BOOL tx;
    BOOL readOnly;
    } FecRecord;

typedef struct
    {
    U32 txEnable;
    U32 rxEnable;
    } FecProvisioning;


static FecProvisioning   fecProv;   // local config - not stored in FDS
static U32  oldFecTx;  // temp variable used when changing mode


//##############################################################################
// FEC Transmit Functions
//##############################################################################


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the current FEC Tx state for the EPON port
///
/// \return TRUE if upstream FEC is enabled on the port or any links
////////////////////////////////////////////////////////////////////////////////
BOOL FecTxState(void)
    {
    return fecProv.txEnable != 0;
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the current FEC Tx state for the given link
///
/// \param  link    Index of link
///
/// \return TRUE if upstream FEC is enabled on the given link
////////////////////////////////////////////////////////////////////////////////
BOOL FecTxLinkState(LinkIndex link)
    {
    return TestBitsSet(fecProv.txEnable,  1U << link);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set Tx FEC state in LIF/XIF
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void FecTxSet(LinkMap links)
    {
#ifdef CONFIG_EPON_10G_SUPPORT
    if (PonCfgDbGetUpRate() == LaserRate10G)
        {
        XifFecTxSet(links);
        }
    else
#endif
        {
        LifFecTxSet(links);
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set Tx FEC state
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void FecTxModeSet(void)
    {
    // disable any newly disabled links in the LIF/XIF
    FecTxSet(fecProv.txEnable & oldFecTx);
    // reconfigure links in the EPON
    EponSetFecTx(fecProv.txEnable);
    // enable any newly enabled links in the LIF/XIF
    FecTxSet(fecProv.txEnable);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set Rx FEC state
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void FecRxModeSet(void)
    {    
    BOOL rxFecMode = (0 != fecProv.rxEnable);

#ifdef CONFIG_EPON_10G_SUPPORT
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        XifFecRxSet(fecProv.rxEnable);
    	}
    else
#endif
    	{
        LifFecRxSet(rxFecMode);
    	}
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the current FEC Rx state for the EPON port
///
/// \return
/// TRUE if downstream FEC is enabled
////////////////////////////////////////////////////////////////////////////////
BOOL FecRxState (void)
    {
    return fecProv.rxEnable != 0;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the current FEC Rx state for the given link
///
/// \param  link    Index of link
///
/// \return TRUE if downstream FEC is enabled on the given link
////////////////////////////////////////////////////////////////////////////////
BOOL FecRxLinkState(LinkIndex link)
    {
    return TestBitsSet(fecProv.rxEnable,  1U << link);
    }


//##############################################################################
// FEC Global and Interface Functions
//##############################################################################


////////////////////////////////////////////////////////////////////////////////
/// \brief  Update the internal FEC mode
///
/// \param link     link to set FEC configuration on
/// \param rxState  should FEC decoding be enabled
/// \param txState  should FEC upstream encoding be enabled
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void FecModeIntSet (LinkIndex link, BOOL rxState, BOOL txState)
    {
    if (link >= TkOnuNumTxLlids)
        {
        return;
        }

    oldFecTx = fecProv.txEnable;
    if (txState)
        {
        fecProv.txEnable |= (U32)(1U << link);
        }
    else
        {
        fecProv.txEnable &= (U32)~(1U << link);
        }

    if (rxState)
        {
        fecProv.rxEnable |= (U32)(1U << link);
        }
    else
        {
        fecProv.rxEnable &= (U32)~(1U << link);
        }
    } // FecSetMode


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get the current FEC state for the EPON port
///
/// FEC is considered to be enabled on the EPON port if any link has:
/// 1G/1G: FEC enabled in either direction
/// 10G/1G: FEC enabled in upstream (downstream is always enabled)
/// 10G/10G: FEC is always enabled
///
/// \return TRUE if FEC is enabled on the EPON port
////////////////////////////////////////////////////////////////////////////////
BOOL FecState (void)
    {
    BOOL ret = FecTxState();    
    ret |= FecRxState();        
    return ret;
    }



////////////////////////////////////////////////////////////////////////////////
/// \brief  Apply the current FEC configuration
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void FecApply(void)
    {
    FecTxModeSet();
    FecRxModeSet();    
    } // FecApply


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the current FEC mode
///
/// \param link     Index of link to set FEC mode
/// \param rxState  FEC Rx enable/disable
/// \param txState  FEC Tx enable/disable
/// \param path accept 10G/1G FEC value flag
///
/// \return return True if set sucessful.
////////////////////////////////////////////////////////////////////////////////
BOOL FecModeSet (LinkIndex link,
                 BOOL rxState,
                 BOOL txState,
                 FecAppDirection path)
    {
    if (FecRxLinkState(link) != rxState)
        {
        if (PonCfgDbGetDnRate () == LaserRate10G)
            {
            if (!TestBitsSet(path,FecApp10G))
                {
                rxState = FecRxLinkState(link);
                }
            }

        if (PonCfgDbGetDnRate () == LaserRate1G)
            {
            if (!TestBitsSet(path,FecApp1G))
                {
                rxState = FecRxLinkState(link);
                }
            }
        }

    if (FecTxLinkState(link) != txState)
        {
        if (PonCfgDbGetUpRate () == LaserRate10G)
            {
            if (!TestBitsSet(path,FecApp10G))
                {
                txState = FecTxLinkState(link);
                }
            }

        if (PonCfgDbGetUpRate () == LaserRate1G)
            {
            if (!TestBitsSet(path,FecApp1G))
                {
                txState = FecTxLinkState(link);
                }
            }
        }

    FecModeIntSet (link, rxState, txState);
    FecApply();

    PonMgrRdpaLlidFecSet(link,txState);
    return TRUE;
    } // PonMgrSetFecMode



////////////////////////////////////////////////////////////////////////////////
/// FecCorrectedBlocks - Number of blocks that FEC has fixed
///
/// This function returns the number of downstream blocks that the FEC has
/// repaired since the last time this count was read.  The count clears on read.
///
/// \return
/// Number of corrected blocks
////////////////////////////////////////////////////////////////////////////////
//extern
U32 FecCorrectedBlocks (void)
    {    
#ifdef CONFIG_EPON_10G_SUPPORT
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        return XifXpcsRead40Stat(Xpcs40RxFecDecErrCorCnt);       
    	}
    else
#endif
        {
        return LifReadStat(LifRxFecCorrBlocks);        
        }
    } // FecCorrectedBlocks


////////////////////////////////////////////////////////////////////////////////
/// FecUncorrectableBlocks - Number of blocks that FEC has not fixed
///
/// This function returns the number of downstream blocks that the FEC could not
/// repair since the last time this count was read.  The count clears on read.
///
/// \return
/// Number of uncorrected blocks
////////////////////////////////////////////////////////////////////////////////
//extern
U32 FecUncorrectableBlocks (void)
    {       
#ifdef CONFIG_EPON_10G_SUPPORT
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        return XifXpcsRead32Stat(Xpcs32RxFecDecFailCnt);       
    	}
    else
#endif
        {
        return LifReadStat(LifRxFecUnCorrBlocks);        
        }
    } // FecUncorrectableBlocks


////////////////////////////////////////////////////////////////////////////////
/// FecInit - Initialize FEC
///
/// This function initializes FEC, restoring the previous configuration if it is
/// present or using the FEC configuration from personality if it is not.
///
/// \param up10G is the upstream 10G?
///
/// \return
/// Nothing
////////////////////////////////////////////////////////////////////////////////
void FecInit(void)
    {
    fecProv.rxEnable = 0;
    fecProv.txEnable = 0;
    }

// End of File PonMgrFec.c

