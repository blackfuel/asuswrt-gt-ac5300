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

////////////////////////////////////////////////////////////////////////////////
/// \file   ProtSwitch.c
/// \brief  ONU Protection Switching configuration
///
////////////////////////////////////////////////////////////////////////////////

#include "ProtSwitch.h"
#include "OptCtrl.h"
#include "OsAstMsgQ.h"
#include "PonConfigDb.h"
#include "PonManager.h"
#include "Holdover.h"
#include "OntDirector.h"
#include "OntmMpcp.h"
#include "CtcAlarms.h"
#include "EponUser.h"
#include "OsAstMsgQ.h"


static PsOnuState    psCurState;
static TkOsTick      psSwitchTm;
static BOOL          psScanning;
static PsConfig10G   psConfig;

PsReason  psReason;

#define PsPonA             0
#define PsPonB             1
#define PsIsPonAWorking     ((psCurState & PsStateSwitched) == 0)

#define PsDebugLevel        DebugProtectSwitch

typedef enum
    {
    PsRecHwType,
    PsRecState,
    PsRecCount
    } PsRecIds;


////////////////////////////////////////////////////////////////////////////////
/// PsPollXcvrs:  Update the signal state of the xcvrs
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void PsPollXcvrs(void)
    {
    // According to personality template
    // Signal Detect(Primary), also named GpioSigDet, is mapped to PON B
    // Signal Detect(Seconady), also named GpioSigDet2, is mapped to PON A
    //if (GpioNhcGet(GpioSigDet))
        {
        psCurState |= PsStateSigDetB;
        }
    //if (GpioNhcGet(GpioSigDet2))
        {
        psCurState |= PsStateSigDetA;
        }
    } // PsPollXcvrs


////////////////////////////////////////////////////////////////////////////////
/// PsPollState:  Update the current state
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void PsPollState(void)
    {
    // clear current state
    psCurState &= ~PsStateOutputMask;
    switch (psConfig.type)
        {
        case PsConfigDualXcvr:
            PsPollXcvrs();
            break;

        case PsConfigNone:
        default:
            break;
        }
    } // PsPollState


////////////////////////////////////////////////////////////////////////////////
/// PsSwitchXcvrs:  Switch working/standby xcvrs
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void PsSwitchXcvrs(void)
    {
    if (PsIsPonAWorking)
        {
        PonMgrActivePonSet(PsPonA);
        }
    else if (!PsIsPonAWorking)
        {
        PonMgrActivePonSet(PsPonB);
        }
    else
        {
        // standby transceiver detects no signal, don't switch
        }
    } // PsSwitchXcvrs


////////////////////////////////////////////////////////////////////////////////
/// PsSignalCheck:  Check if there has optcial signal in the standby xcvr
///
/// Parameters:
///     None
///
/// \return
/// TRUE if standby xcvr has optical signal, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
static
BOOL PsStandbySignalCheck(void)
    {
    PsPollState();

    if(((!PsIsPonAWorking) && TestBitsSet(psCurState, PsStateSigDetA)) ||
       (( PsIsPonAWorking) && TestBitsSet(psCurState, PsStateSigDetB)))
        {
        return TRUE;
        }
    return FALSE;
    } // PsStandbySignalCheck


////////////////////////////////////////////////////////////////////////////////
/// \brief  Perform a protection switch
///
/// \param wait     Time to wait before attempting next autonomous switch
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void PsPerformSwitch(U16 wait)
    {
    psCurState ^= PsStateSwitched;

    switch (psConfig.type)
        {
        case PsConfigDualXcvr:
            PsSwitchXcvrs();
            break;

        case PsConfigNone:
            return;

        default:
            // Invalid value
            break;
        }

    psSwitchTm = TkOsSysTick() + wait;
    PsPollState();

    eponUsrDebug(PsDebugLevel,
              ("%s\n", PsIsPonAWorking ? "PRI -> SEC" : "SEC -> PRI"));
    } // PsPerformSwitch



////////////////////////////////////////////////////////////////////////////////
/// PsSetState:  Set the protection switch state
///
/// Parameters:
/// \param state    Protection switch state to set
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsSetState(PsOnuState state)
    {
    if (((state ^ psCurState) & PsStateActiveMask) != 0)
        { // active PON is different
        PsStartScan(PsReasonHost, PsScanTimeHost);
        }
    } // PsSetState


////////////////////////////////////////////////////////////////////////////////
/// PsGetState:  Get the protection switch state
///
/// Parameters:
///     None
///
/// \return
/// The current protection switch state
////////////////////////////////////////////////////////////////////////////////
//extern
PsOnuState PsGetState(void)
    {
    PsPollState();
    return psCurState;
    } // PsGetState


////////////////////////////////////////////////////////////////////////////////
/// PsGetType:  Get the protection switch type
///
/// Parameters:
///     None
///
/// \return
/// The current protection switch tyoe
////////////////////////////////////////////////////////////////////////////////
//extern
PsConfig PsGetType(void)
    {
    return psConfig.type;
    } // PsGetState


////////////////////////////////////////////////////////////////////////////////
/// \brief  Notify thereason for switch
///
/// \param None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsNotifyReason(void)
    {
    if (psReason < PsReasonMax)
        {
        /*OamEventTkProtSwitch psTlv;

	        psTlv.ext.tlv.type = OamEventErrVendor;
	        psTlv.ext.tlv.length = sizeof(OamEventTkProtSwitch);
	        memcpy (psTlv.ext.oui.byte, &TeknovusOui, sizeof(IeeeOui));
	        psTlv.alm = OamAlmCodeProtSwitch;
	        psTlv.reason = psReason;
	        psTlv.state = PsGetState();

	        OntDirNotify(0, (U8*)&psTlv, sizeof(OamEventTkProtSwitch), OuiTeknovus);
	        */
        psReason = PsReasonMax;
        }
    } // PsNotifyReason


////////////////////////////////////////////////////////////////////////////////
/// PsHandleTimer:  Handle protection switching timer
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
static
void PsHandleTimer(EponTimerModuleId moduleId)
    {
    PsPollState();

    if (psConfig.type == PsConfigDualXcvr)
        {
        if (PsIsPonAWorking)
            {
            if (!TestBitsSet(psCurState, PsStateSigDetB))
                {
                OsAstMsgQSet(OsAstAlmEponStandbyLos, 0, 0);
                }
            else
                {
                OsAstMsgQClr(OsAstAlmEponStandbyLos, 0, 0);
                }
            }
        else
            {
            if (!TestBitsSet(psCurState, PsStateSigDetA))
                {
                OsAstMsgQSet(OsAstAlmEponStandbyLos, 0, 0);
                }
            else
                {
                OsAstMsgQClr(OsAstAlmEponStandbyLos, 0, 0);
                }
            }
        }
    } // PsHandleTimer


////////////////////////////////////////////////////////////////////////////////
/// PsStartScan:  Begin scanning for a good signal
///
/// Parameters:
/// \param reason   Reason we are scanning
/// \param time     Delay before next switch (system ticks)
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsStartScan(PsReason reason, U16 time)
    {
    psScanning = TRUE;
    psReason   = reason;
    eponUsrDebug(PsDebugLevel, ("SCAN: %d\n", reason));
    switch(psReason)
        {
        case PsReasonHost:
        case PsReasonNone:
            PsPerformSwitch(time);
            break;

        default:
            if(PsStandbySignalCheck())
                {
                PsPerformSwitch(time);
                }
            break;
        }
    } // PsStartScan


////////////////////////////////////////////////////////////////////////////////
/// PsScan:  Scan for a good signal
///
/// Parameters:
/// \param time   Time between switches (in system ticks)
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsScan(U16 time)
    {
    if (U32LessThan(psSwitchTm, TkOsSysTick()))
        {
        if(PsStandbySignalCheck())
            {
            PsPerformSwitch(time);
            }
        }
    } // PsScan


////////////////////////////////////////////////////////////////////////////////
/// PsStopScan:  Stop scanning for a good signal
///
/// Parameters:
/// None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsStopScan(void)
    {
    eponUsrDebug(PsDebugLevel, ("STOP\n"));

    psScanning = FALSE;

	PsNotifyReason();

	CtcAlmPonIfSwitchNotify();
	
    } // PsStopScan


////////////////////////////////////////////////////////////////////////////////
/// \brief  Callback to run the type C protection switch state machine
///
/// \param los          Loss of Signal
/// \param eponInSync   Is EPON in sync with OLT MPCP time
///
/// \return
/// Logical sync state of PON
////////////////////////////////////////////////////////////////////////////////
static
BOOL PsCallback(BOOL los, BOOL eponInSync)
    {
    if ((psConfig.type != PsConfigNone) &&
        (psConfig.mode == PsModeInternal) &&
        PonCfgDbGet(PsPonA)->available &&
        PonCfgDbGet(PsPonB)->available)
        {
        if (!psScanning)
            {
            if (los)
                {
                PsStartScan(PsReasonLos, PsScanTimeLos);
                }
            else if (!eponInSync)
                {
                if ((OntmMpcpLinksRegistered() != 0) ||
                    (mpcpGateNum == MaxMpcpGateCheck))
                    {
                    PsStartScan(PsReasonMpcp, PsScanTimeMpcp);
                    }
                }
            else
                {
                // in sync - nothing to do
                }
            }
        else
            {
            if (los)
                {
                PsScan(PsScanTimeLos);
                }
            else if (!eponInSync)
                {
                if ((OntmMpcpLinksRegistered() != 0) ||
                    (mpcpGateNum == MaxMpcpGateCheck))
                    {
                    PsScan(PsScanTimeMpcp);
                    }
                }
            else
                {
                PsStopScan();
                }
            }
        }

    return HoldoverCallback(los, eponInSync);
    }





////////////////////////////////////////////////////////////////////////////////
/// PsInit:  Initialize the protection switching module
///
/// Parameters:
///     None
///
/// \return
///     None
////////////////////////////////////////////////////////////////////////////////
//extern
void PsInit(void)
    {  
    psConfig.type = PsConfigNone;
    psConfig.mode = PsModeInternal; 

    psCurState = PsStateNone;
    psSwitchTm = 0;
    psScanning = FALSE;
    psReason = PsReasonMax;

    if ((psConfig.type != PsConfigNone) &&
        (psConfig.mode == PsModeInternal) &&
        PonCfgDbGet(PsPonA)->available &&
        PonCfgDbGet(PsPonB)->available)
        {
        PsPollState();
        PsStartScan(PsReasonNone, PsScanTimeInit);
        }
	
    PonCfgDbSetRegCb(PsCallback);
    //register timer.
    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
    	PsTimer,PsHandleTimer);
    } // PsInit


// End of file ProtSwitch.c
