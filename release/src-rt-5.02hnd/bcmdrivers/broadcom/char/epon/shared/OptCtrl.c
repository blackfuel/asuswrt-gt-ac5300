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


#include "board.h"
#include "boardparms.h"
#include "OptCtrl.h"
#include "PonConfigDb.h"
#include "PonManager.h"
#include "EponUser.h"
#include "OsAstMsgQ.h"

typedef enum
    {
    RecOptTimerActive,
    RecOptTimerStandby,

    RecordsCount
    } RecordIds;


typedef enum
    {
    LaserGpioTxDis,
    LaserGpioRxDis,
    LaserGpioOptShut,
    LaserGpioSigDet,

    LaserGpioCount
    } LaserGpioFuncs;


#define MaxPonIf    1


static U16                  BULK activeTimer;
static U16                  BULK standbyTimer;
static OptCtrlLaserState    BULK txLaserState[NumPonIf];
static OptCtrlLaserState    BULK rxLaserState[NumPonIf];
static rdpa_epon_laser_tx_mode curLaserTxMode;

#define OptCtrlDebug(lvl, args) eponUsrDebug(lvl, args)

////////////////////////////////////////////////////////////////////////////////
/// \brief  Update the state of the laser
///
/// \param pon  Instance to update
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void LaserStateUpdate(U8 pon)
    {
    if (pon < MaxPonIf)
        {
        unsigned short rx_gpio;
        unsigned short tx_gpio;
        int rc = 0;
        BOOL FAST tx = (txLaserState[pon] == LaserStateEnable);
        BOOL FAST rx = (rxLaserState[pon] == LaserStateEnable);
		
        rc = BpGetPonRxEnGpio(&rx_gpio);
        if (rc < 0)
            {
            OptCtrlDebug(DebugOptCtrl,("set rx:%d fail\n",(U8)rx));
            }
        else
            {
            kerSysSetGpioState(rx_gpio, rx ? kGpioActive : kGpioInactive);
            }

        rc = BpGetPonTxEnGpio(&tx_gpio);
        if (rc < 0)
            {
            OptCtrlDebug(DebugOptCtrl,("set tx:%d fail\n",(U8)tx));
            }
        else
            {
            kerSysSetGpioState(tx_gpio, tx ? kGpioActive : kGpioInactive);
            }

        OptCtrlDebug(DebugOptCtrl,("tx:%d rx:%d\n",(U8)tx,(U8)rx));
        DelayMs(PonCfgDbGetPowerupTime(pon));
        }
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Set the state of the Tx/Rx laser
///
/// \param pon  Instance to set
/// \param tx   New Tx state for the laser
/// \param rx   New Rx state for the laser
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlSetLaserState(U8 pon, OptCtrlLaserState tx, OptCtrlLaserState rx)
    {
    if (pon < MaxPonIf)
        {
        txLaserState[pon] |= tx;
        rxLaserState[pon] |= rx;

        LaserStateUpdate(pon);
        }
    } // OptCtrlSetLaserState


////////////////////////////////////////////////////////////////////////////////
/// \brief  Clear state from the Tx/Rx laser
///
/// \param pon  Instance to clear
/// \param tx   New Tx state for the laser
/// \param rx   New Rx state for the laser
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlClearLaserState(U8 pon, OptCtrlLaserState tx, OptCtrlLaserState rx)
    {
    if (pon < MaxPonIf)
        {
        txLaserState[pon] &= (OptCtrlLaserState)~tx;
        rxLaserState[pon] &= (OptCtrlLaserState)~rx;

        LaserStateUpdate(pon);
        }
    } // OptCtrlClearLaserState


////////////////////////////////////////////////////////////////////////////////
/// \brief  Enable laser transmitter
///
/// Enables the laser transmitter and clears associated alarms and NVS records
///
/// \param actOpt Active optical or standby optical
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void OptCtrlTxDiagClear(BOOL actOpt)
    {
    PortInstance ponPort = PonCfgDbGetActivePon();
    if ((MaxPonIf == 1) && (!actOpt))
        {
        return ;
        }
    
    if(!actOpt)
        {
        ponPort = PonCfgDbGetStandbyPon();
        }
    OptCtrlClearLaserState(ponPort, LaserStateDisOptFail, LaserStateUnchanged);

    //OsAstMsgQClr(OsAstAlmEponLaserShutdownTemp, ponPort, 0);
    //OsAstMsgQClr(OsAstAlmEponLaserShutdownPerm, ponPort, 0);
    if(actOpt)
        {
        //resume the TX status
        PonMgrSetLaserStatus(curLaserTxMode);
        }
    } // OptCtrlEnableLaser


////////////////////////////////////////////////////////////////////////////////
/// OptCtrlHandleTimerOut - Timer handler routine for the optics control module
///
/// Parameters:
///     None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlHandleTimerOut (void)
    {
    if (activeTimer != OptCtrlDisableForeverTime)
        {
        // enable laser when timer reaches zero
        if (activeTimer == 0)
            {
            curLaserTxMode = PonMgrGetLaserBurstStatus();
            OptCtrlTxDiagClear(TRUE);
            OptCtrlDebug(DebugOptCtrl,("Timerout\n"));
            }

        // When enableTimer is zero this line evaluates to
        // OptCtrlDisableForeverTime, effectively stopping the timer countdown
        --activeTimer;
        }
	
	if (standbyTimer != OptCtrlDisableForeverTime)
        {
        // enable laser when timer reaches zero
        if (standbyTimer == 0)
            {
            OptCtrlTxDiagClear(FALSE);
            }

        // When enableTimer is zero this line evaluates to
        // OptCtrlDisableForeverTime, effectively stopping the timer countdown
        --standbyTimer;
        }
    } // OptCtrlHandleTimerOut


////////////////////////////////////////////////////////////////////////////////
/// \brief  Disable laser transmitter
///
/// Disables the laser transmitter for the specified length of time
///
/// \param actOpt       Active optical or standby optical
/// \param enableTime   Time in seconds to reenable
/// \param preserve     Whether to store disable time in NVS
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlTxDiagSet(BOOL actOpt, U16 enableTime, BOOL preserve)
    {
    PortInstance ponPort = PonCfgDbGetActivePon();
    //Only One PON interface. do not support standby.
    if ((MaxPonIf == 1) && (!actOpt))
        {
        return ;
        }
    if(!actOpt)
        {
        ponPort = PonCfgDbGetStandbyPon();
        }

    //If laser is already on, no need to re-enable.
    if((PonMgrGetLaserStatus() != rdpa_epon_laser_tx_off)
        && 
        (0 == enableTime))
        {
        return;
        }
     
    OptCtrlSetLaserState(ponPort, LaserStateDisOptFail, LaserStateUnchanged);

    if (enableTime == OptCtrlDisableForeverTime)
        {
        OsAstMsgQSet (OsAstAlmEponLaserShutdownPerm, ponPort, 0);
        OsAstMsgQClr (OsAstAlmEponLaserShutdownTemp, ponPort, 0);
        }
    else
        {
        OsAstMsgQSet (OsAstAlmEponLaserShutdownTemp, ponPort, 0);
        OsAstMsgQClr (OsAstAlmEponLaserShutdownPerm, ponPort, 0);
        }

    if(actOpt)
        {
        activeTimer = enableTime;
        if (enableTime != 0)
            {
            PonMgrSetLaserStatus(rdpa_epon_laser_tx_off);
            }
        else
            {
            PonMgrSetLaserStatus (PonMgrGetLaserBurstStatus());
            }
        curLaserTxMode = PonMgrGetLaserStatus();
        }
    else
        {
        standbyTimer = enableTime;
        }

    //Re-enable laser directly, other than waiting for timer times out.
    if(0 == enableTime)
        {
        OptCtrlHandleTimerOut();
        }
    } // OptCtrlDisableLaser


////////////////////////////////////////////////////////////////////////////////
//extern
OptCtrlLaserCaps OptCtrlGetLaserCaps(void)
    {
    OptCtrlLaserCaps caps = LaserCapsNone;

    //if (GpioNhcExist(GpioLaserTxDis))
        {
        caps |= LaserCapsTxPower;
        }
    //if (GpioNhcExist(GpioLaserRxDis))
        {
        caps |= LaserCapsRxPower;
        }
    //if (GpioNhcExist(GpioLaserTxDis2))
        {
        caps |= LaserCapsTxPower2;
        }
    //if (GpioNhcExist(GpioLaserRxDis2))
        {
        caps |= LaserCapsRxPower2;
        }

    return caps;
    } // OptCtrlGetLaserCaps


////////////////////////////////////////////////////////////////////////////////
/// OptCtrlHandle1sTimer - Timer handler routine for the optics control module
///
/// Parameters:
///     None
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlHandle1sTimer (EponTimerModuleId moduleId)
    {
    OptCtrlHandleTimerOut();
    } // OptCtrlHandle1sTimer


////////////////////////////////////////////////////////////////////////////////
/// OptCtrlInit - Initial laser power settings
///
/// This function initializes the laser power settings, namely if the laser was
/// determined to be insane it will shut it off at startup.
///
/// Parameters:
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
//extern
void OptCtrlInit (void)
    {
    U8 FAST i;

    activeTimer = OptCtrlDisableForeverTime;
    standbyTimer = OptCtrlDisableForeverTime;
	
    for (i = 0; i < NumPonIf; i++)
        {
        txLaserState[i] = LaserStateEnable;
        rxLaserState[i] = LaserStateEnable;
        }
	
	// Active optical
	//if(FdsRecRead(FdsGrpIdOptCtrl, RecOptTimerActive,
	//                               &activeTimer, sizeof(activeTimer)) == 0)
	if (!PonGetActOptCtrl(&activeTimer))
        {
        curLaserTxMode = rdpa_epon_laser_tx_burst;
        OptCtrlTxDiagClear(TRUE);
        }
    else
        {
        OptCtrlTxDiagSet(TRUE, activeTimer, FALSE);
        }
	
	// Standby optical
	if(!PonGetStdOptCtrl(&standbyTimer))	
        {
        OptCtrlTxDiagClear(FALSE);
        }
    else
        {
        OptCtrlTxDiagSet(FALSE, standbyTimer, FALSE);
        }

    //register timer.
    EponUsrModuleTimerRegister(EPON_TIMER_TO1,
		OptCtrlTimer,OptCtrlHandle1sTimer);
    } // OptCtrlInit

// End of File OptCtrl.c
