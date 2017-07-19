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

#include <linux/spinlock.h>
#include "EponMac.h"
#include "Lif.h"
#include "LifRegs.h"

U32 volatile __REGISTER_SPACE[MACREGSPACE];

//##############################################################################
// Local Type Definitions
//##############################################################################

typedef enum
    {
    LifDpRamSelDnStats  = 0,
    LifDpRamSelDnKey    = 1,
    LifDpRamSelUpKey    = 3,
    LifDpRamSelUpMulti  = 4
    } LifDpRamSel;

//##############################################################################
// Constant Definitions
//##############################################################################
#define PipeDelay           6
#define IpgCnt              2
#define PreambleLengthTq    4


//##############################################################################
// Macro Helper Definitions
//##############################################################################
// Build a 24bit key from 3 bytes.
#define Build24BitKey(a,b,c) (U32) (((a) << 16) | ((b) << 8) | (c))

//##############################################################################
// Static Variables
//##############################################################################
U8 laserOffTimeOffset;

static DEFINE_SPINLOCK(lif_spinlock);


//##############################################################################
// Function Definitions
//##############################################################################

////////////////////////////////////////////////////////////////////////////////
/// \brief Waits for LIF data port to be available
///
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void DpWait (void)
    {
    int timeout = 0x30;
    // make sure port is free. Note that we have no failsafe on this loop. The
    // reason is that DP should not be stuck unless the ASIC is dead.
    while ((OnuRegAnd (&LifDpCmd, LifDpCmdBusy) != 0) && (timeout--))
        {
        ;
        }
    } // DpWait


////////////////////////////////////////////////////////////////////////////////
/// \brief Writes a value to the LIF data port
///
/// Checks for data port busy, issues write command
///
/// \param ramSel   RAM to write
/// \param addr     Data port address to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void DpWrite (LifDpRamSel ramSel, U16 addr)
    {
    // wait for dp to be ready
    DpWait();
    // set up the address
    OnuRegWrite (&LifDpAddr, (U32) addr);
    // issue the write command
    OnuRegWrite (&LifDpCmd,
                 LifDpCmdRamWr |
                 TkPutField (LifDpCmdRamSel, ramSel));
    // wait again to be sure command had been taken
    DpWait();
    } // DpWrite

/*
////////////////////////////////////////////////////////////////////////////////
/// \brief Reads a value from the data port
///
/// Checks for data port busy, issues write command
///
/// \param ramSel   RAM to ream from
/// \param addr     Data port address to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void DpRead (LifDpRamSel ramSel, U16 addr)
    {
    // wait for dp to be ready
    DpWait();
    // set up the address
    OnuRegWrite (&LifDpAddr, (U32) addr);
    // issue the read command
    OnuRegWrite (&LifDpCmd, TkPutField (LifDpCmdRamSel, (U32)ramSel));
    // wait again for dp to be ready
    DpWait();
    } // DpRead
*/

////////////////////////////////////////////////////////////////////////////////
/// \brief Reads LIF statistic
///
/// \param statistic to read
///
/// \return
/// statistic value
////////////////////////////////////////////////////////////////////////////////
//extern
U32 LifReadStat (LifStatId stat)
    {
    if ( stat < LifFirstRxStat )
        {
        return OnuRegRead( &LifGetTxStatReg(stat) );
        }
    else
        {
        return OnuRegRead( &LifGetRxStatReg(stat-LifFirstRxStat) );
        }
    } // LifReadStat


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set/Clear encryption for a given link/key
///
/// \param mode security decrytion mode
/// \param direction direction of interest
/// \param link link of interest
/// \param keyIdx index of the key
/// \param key pointer to the key to set or NULL if encryption is disabled
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifKeySet (EncryptMode mode,
                Direction direction,
                LinkIndex link,
                U8 keyIdx,
                U32 const BULK *key,
                const U32 BULK* sci,
                U8 tci,
                U32 pn)
    {
    // Translation table
    
    U8 modeE2ITab[EncryptModeMax] = {
        LifEncryptionDisable,       // EncryptModeDisable
        LifEncryptionTeknovusMode,  // EncryptModeAes
        LifEncryptionDisable,       // EncryptModeZoh
        LifEncryptionTripleChurn,  // EncryptModeTripleChurn
        LifEncryptionDisable};  // EncryptMode8021AE
    
    U8  lifMode = modeE2ITab[mode];
    spin_lock_bh(&lif_spinlock);
    
    // If mode is set to disable, we clear the entry in the table
    if (mode == EncryptModeDisable)
        {
        OnuRegWrite (&LifDpData[4], LifDpDataNoEncryption);
    	OnuRegAndEqNot(&LifSecCtl, LifSecCtlEnTripleChurn);
        }
    else
        {
        // Set (or re-set) the global encryption scheme
        if (direction == Upstream)
            {
            OnuRegFieldWrite (&LifSecCtl, LifSecCtlUpScheme, lifMode);
            }
        else
            {
            OnuRegFieldWrite (&LifSecCtl, LifSecCtlDnScheme, lifMode);
            }
        if (mode == EncryptModeTripleChurn)
            {
            // For a 3Churned key, only 3 bytes are used: B0, B1 and B2
            // input key = [Bx] [B0] [B1] [B2]
            U8  b0 = (U8) (key[0] >> 16);
            U8  b1 = (U8) (key[0] >> 8);
            U8  b2 = (U8) (key[0] >> 0); 
    
            //enable ctc churning encryption
            OnuRegOrEq(&LifSecCtl, LifSecCtlEnTripleChurn);
            OnuRegWrite (&LifDpData[0], key[0]&0xFFFFFFUL);
            OnuRegWrite (&LifDpData[1], Build24BitKey(b2, b0, b1));
            OnuRegWrite (&LifDpData[2], Build24BitKey(b1, b2, b0));
            OnuRegWrite (&LifDpData[3], key[0]&0xFFFFUL);
            }
        else // All other encryption schemes are straight forward ...
            {
            // ... that means in reverse order.
            OnuRegWrite (&LifDpData[0], key[3]);
            OnuRegWrite (&LifDpData[1], key[2]);
            OnuRegWrite (&LifDpData[2], key[1]);
            OnuRegWrite (&LifDpData[3], key[0]);
            }
    
        OnuRegWrite (&LifDpData[4], LifDpDataEncryptWithKey0 | keyIdx);
        }
    
    // Send the command to data port
    if (direction == Upstream)
        {
        DpWrite (LifDpRamSelUpKey, link);
        }
    else
        {
        DpWrite (LifDpRamSelDnKey, (link << 1) | keyIdx);
        }
    
    spin_unlock_bh(&lif_spinlock);
    } // LifKeySet


////////////////////////////////////////////////////////////////////////////////
/// \brief Get the current security enable status for link
///
/// This function gets the secruity enable status for link
///
/// \param  None
///
/// \return
/// the current secruity enable status
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL LifEncryptEnGet(LinkIndex link, Direction dir)
    {
    return (Bool)(OnuRegAnd (&LifDnSec, (1UL << link)) >> link);
    } // LifRxEncryptEnBitmapGet


////////////////////////////////////////////////////////////////////////////////
/// LifKeyInUse - Get the current security key for a link
///
/// This function gets the active key for a given link.  It is used to detect a
/// key switch over.
///
 // Parameters:
/// \param  link Link index to check
///
/// \return
/// Active security key
////////////////////////////////////////////////////////////////////////////////
//extern
U8 LifKeyInUse (LinkIndex link, Direction dir)
    {
    return (U8)(OnuRegAnd (&LifSecKeySel, (1UL << link)) >> link);
    } // LifKeyInUse


////////////////////////////////////////////////////////////////////////////////
/// LifEncryptUpDisable - Disable upstream encryption global configuration
///
 // Parameters:
/// \none
///
/// \return
/// none
////////////////////////////////////////////////////////////////////////////////
//extern
void LifEncryptUpDisable (void)
    {
    
    } // LifEncryptUpDisable


////////////////////////////////////////////////////////////////////////////////
//extern
void LifFecTxSet(LinkMap linkMap)
    {
    OnuRegFieldWrite(&LifFecCtl, LifFecCtlTxFecLlidEn, linkMap);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set FEC Rx
///
/// \param Fec Rx mode
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifFecRxSet (BOOL mode)
    {
    if (mode)
        {
        OnuRegOrEq (&LifFecCtl, LifFecCtlCfRxEn);
        }
    else
        {
        OnuRegAndEqNot (&LifFecCtl, LifFecCtlCfRxEn);
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief Sets idle time for LIF
///
/// \param  front   Time to idle before burst,
///                 measured in 62.5M clocks (16 bit times)
/// \param  back    Time to idle after burst, 62.5M clocks
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifSetIdleTime (U16 front, U16 back)
    {
    OnuRegWrite (&LifLaserOffIdle,
                 TkPutField (LifLaserOffIdleCfTxDelta, back - laserOffTimeOffset) |
                 	LifLaserOffIdleBefore |
                 	TkPutField (LifLaserOffIdleCfTxInit, front));
	
    //Reg 0x138 must be set for FEC transmission;
    //If it is zero, the OLT will not be able to lock onto the upstream burst.
    OnuRegWrite (&LifLFecInitIdle, TkPutField (LifLFecInitIdleCfFecInit, front));
    
    OnuRegWrite (&LifSecUpMpcpOffset, front + PreambleLengthTq);
    } // LifSetIdleTime


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable a link form the lookup table
///
/// Logical links are numbered from 0;
///
/// \param link     Logical link number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifDisableLink (LinkIndex link)
    {
    OnuRegTableAndEqNot (LifLlid, link, LifLlidEn);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief Enable lookup for incoming traffic
///
/// Logical links are numbered from 0; the packet preamble has a 16-bit
/// LLID value within it that is not necessarily the same.
///
/// \param link     Logical link number
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifEnableLink (LinkIndex link)
    {
    OnuRegTableOrEq (LifLlid, link, LifLlidEn);
    } // LifEnableLink


////////////////////////////////////////////////////////////////////////////////
/// \brief sets physical LLID value associated with a link
///
/// Logical links are numbered from 0; the packet preamble has a 16-bit
/// LLID value within it that is not necessarily the same.
///
/// \param link     Logical link number
/// \param phy      Physical LLID value for this link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifCreateLink (LinkIndex link, PhyLlid phy)
    {
    OnuRegTableWrite (LifLlid, link, (U32)phy | LifLlidEn);
    } // LifEnableLink


////////////////////////////////////////////////////////////////////////////////
/// \brief Return a PhyLlid associated to a link index
///
/// \param link     Logical link number
///
/// \return Physical Llid
////////////////////////////////////////////////////////////////////////////////
//extern
PhyLlid LifGetPhyLlid (LinkIndex link)
    {
    return (PhyLlid) (OnuRegTableRead (LifLlid, link) & LifLlidLkup0Msk);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return a link index associated to a phy llid
///
/// \param phy     Phy LLID
///
/// \return Link index
////////////////////////////////////////////////////////////////////////////////
//extern
LinkIndex LifPhyLlidToIndex (PhyLlid phy)
    {
    LinkIndex  result;

    result = 0;
    while ((result < LifLlidCount) &&
           (LifGetPhyLlid(result) != phy))
        {
        ++result;
        }

    return result;
    } // LifPhyLlidToIndex


void LifLaserTxModeSet (rdpa_epon_laser_tx_mode mode)
    {
    // This is implemented as a multiple ifs instead of a switch to save code.
    if (mode == rdpa_epon_laser_tx_off)
        {
        OnuRegAndEqNot (&LifPonCtl, LifPonCtlCfTxLaserEn);
        }
    else
        {
        OnuRegOrEq (&LifPonCtl, LifPonCtlCfTxLaserEn);

        if (mode == rdpa_epon_laser_tx_continuous)
            {
            OnuRegOrEq (&LifPonCtl, LifPonCtlCfTxLaserOn);
            }
        else
            {
            OnuRegAndEqNot (&LifPonCtl, LifPonCtlCfTxLaserOn);
            }
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Does the LIF have code word lock?
///
/// \return
/// TRUE if LIF has code word lock, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL LifLocked(void)
    {
    BOOL  lock;

    // we are locked if the loss interrupt has not triggered
    lock = !OnuRegBitsSet(&LifInt, LifIntRxOutOfSync);

    // clear the loss interrupt after reading to prepare for next check
    OnuRegWrite(&LifInt, LifIntRxOutOfSync);

    return lock;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable LIF RX
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifRxDisable(void)
    {
    OnuRegAndEqNot(&LifPonCtl, LifPonCtlNotRxRst);
    OnuRegAndEqNot(&LifSecCtl, LifSecCtlNotDnRst);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Enable LIF RX
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifRxEnable(void)
    {
    OnuRegOrEq(&LifPonCtl, LifPonCtlNotRxRst);
    OnuRegOrEq(&LifSecCtl, LifSecCtlNotDnRst);
    }

////////////////////////////////////////////////////////////////////////////////
/// \brief  Setup TX-to-RX loopback
///
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifTxToRxLoopback (void)
    {
    OnuRegOrEq (&LifPonCtl, LifPonCtlCfTx2RxLpBack);
    OnuRegOrEq (&LifSecCtl, LifSecCtlNotUpRst | LifSecCtlNotDnRst);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void LifTransportTimeSet(U32 time)
    {
    OnuRegWrite(&LifTpTime, time);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
U32 LifMpcpTimeGet(void)
    {
    return OnuRegRead(&LifMpcpTime[0]);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void LifLaserMonSet(BOOL enable, U32 threshold)
    {
    if (enable)
        {
        OnuRegWrite(&LifLaserMonIntMask, 
            LifLaserMonLaserOffIntMask | LifLaserMonLaserOnMaxIntMask);
        OnuRegWrite(&LifLaserMonMaxThres, threshold);
        }

    OnuRegWrite(&LifLaserMonCtrl, (U32)enable);

    if (!enable)
        {
        OnuRegWrite(&LifLaserMonIntStat, 
            LifLaserMonLaserOffInt | LifLaserMonLaserOnMaxInt);
        }
    }


////////////////////////////////////////////////////////////////////////////////
//extern
U32 LifLaserMonLaserOnMaxIntGet(void)
    {
    return (OnuRegRead(&LifLaserMonIntStat) & LifLaserMonLaserOnMaxIntMask);
    }


////////////////////////////////////////////////////////////////////////////////
//extern
void LifLaserMonClrLaserOnMaxInt(void)
    {
    OnuRegWrite(&LifLaserMonIntStat, LifLaserMonLaserOnMaxInt);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Initialize module
///
/// \param  txOffTimeOffset     Delay normal laser Off (in TQ)
/// \param  txOffIdle           If TRUE we insert idle when laser is OFF
/// \param  upRate              Upstream laser rate
/// \param  dnRate              Downstream laser rate
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
//extern
void LifInit (U8 txOffTimeOffset, BOOL txOffIdle,
              Polarity polarity, LaserRate upRate, LaserRate dnRate)
    {
    U32 regVal;

    //release lif  
    VpbReset(VpbLifR,FALSE);
    // Store laser time offset. Needed later when setting up Idle Time.
    laserOffTimeOffset = txOffTimeOffset;
    
    regVal = 0;
    // Set LIF control register to 0
    OnuRegWrite (&LifPonCtl, regVal);

    regVal = LifPonCtlRxEn | LifPonCtlTxEn | LifPonCtlCfgRxDataBitFlip;
    
    if(polarity == ActiveHi)
        {
        regVal |= LifPonCtlCfTxLaserOnActHi;    
        }

    if(txOffIdle == FALSE)
        {
        regVal |= LifPonCtlCfTxZeroDurLOff;    
        }
                 
    if(dnRate == LaserRate1G)
        {
        regVal |= LifPonCtlCfOneGigPonDn;  
        }
    else if (dnRate == LaserRate2G)
        {
        regVal &= (~LifPonCtlCfOneGigPonDn);
        }

    // fix from fiberhome 1pps issue
    regVal |= LifPonCtlCfPpsClkRbc;
    
    // Clear reset
    OnuRegWrite (&LifPonCtl, regVal);

    // Interop register settings
    OnuRegWrite (&LifPonIopCtl,
                 TkPutField (LifPonIopCtlCfTxPipeDelay, PipeDelay) |
                 TkPutField (LifPonIopCtlCfTxIpgCnt, IpgCnt));

    // Enable Fec
    OnuRegWrite (&LifFecCtl,
                 LifFecCtlCfRxEn      |
                 LifFecCtlCfTxEn      |
                 LifFecCtlCfTxPerLlid );

	
    // Security Controls
    OnuRegWrite (&LifSecCtl,
                 LifSecCtlNotDnRst      |
                 LifSecCtlNotUpRst      |
                 LifSecCtlEnDn          |
                 LifSecCtlEnUp          |
                 LifSecCtlEnEpnMix      );

    if (dnRate != LaserRate10G)
        {
        LifRxEnable();
        }

    if (upRate == LaserRate1G)
        {
        OnuRegOrEq(&LifPonCtl, LifPonCtlNotTxRst);
        }
    }

void LifSetPonSpeed(LaserRate dnRate)
    {
    U32 val = 0;

    val = OnuRegRead(&LifPonCtl);
    
    if (dnRate == LaserRate1G)
        {
        val |= LifPonCtlCfOneGigPonDn;
        }
    else if (dnRate == LaserRate2G)
        {
        val &= (~LifPonCtlCfOneGigPonDn);
        }
    OnuRegWrite (&LifPonCtl, val);
    }
// End of Lif.c

