/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
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

/****************************************************************************
 *
 * BcmCoreTest.c -- Bcm ADSL core driver main
 *
 * Description:
 *	This file contains BCM ADSL core driver system interface functions
 *
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.6 $
 *
 * $Id: BcmAdslCore.c,v 1.6 2004/07/20 23:45:48 ilyas Exp $
 *
 * $Log: BcmAdslCore.c,v $
 * Revision 1.6  2004/07/20 23:45:48  ilyas
 * Added driver version info, SoftDslPrintf support. Fixed G.997 related issues
 *
 * Revision 1.5  2004/06/10 00:20:33  ilyas
 * Added L2/L3 and SRA
 *
 * Revision 1.4  2004/04/30 17:58:01  ilyas
 * Added framework for GDB communication with ADSL PHY
 *
 * Revision 1.3  2004/04/28 20:30:38  ilyas
 * Test code for GDB frame processing
 *
 * Revision 1.2  2004/04/12 23:20:03  ilyas
 * Merged RTEMS changes
 *
 * Revision 1.1  2004/04/08 21:24:49  ilyas
 * Initial CVS checkin. Version A2p014
 *
 ****************************************************************************/

#ifdef VXWORKS
#define RTOS	VXWORKS
#endif

#ifdef _WIN32_WCE
#include <windows.h>
#include <types.h>
#endif

#include "softdsl/AdslCoreDefs.h"

#if defined(_CFE_)
#include "lib_types.h"
#include "lib_string.h"
#ifdef SUPPORT_XDSLDRV_GDB
void setGdbOn(void){}
char isGdbOn(void) {return(0);}
void BcmAdslCoreGdbTask(void){}
void BcmAdslCoreGdbCmd(void *pCmdData, int len){}
#endif
#endif

/* Includes. */
#ifdef __KERNEL__
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/vmalloc.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
#include <linux/slab.h>
#endif
#include <linux/module.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
#define	LINUX_KERNEL_2_6
#endif
#endif /* __KERNEL__ */

#include "BcmOs.h"
#if defined(CONFIG_BCM96338)
#include "6338_common.h"
#include "6338_map.h"
#define	XDSL_CLK_EN	ADSL_CLK_EN
#elif defined(CONFIG_BCM96348)
#include "6348_common.h"
#include "6348_map.h"
#elif defined(CONFIG_BCM96362)
#include "6362_common.h"
#include "6362_map.h"
#define	XDSL_CLK_EN	(ADSL_CLK_EN | PHYMIPS_CLK_EN | ADSL_AFE_EN | ADSL_QPROC_EN)
#elif defined(CONFIG_BCM96328)
#include "6328_common.h"
#include "6328_map.h"
#define	XDSL_CLK_EN	(ADSL_CLK_EN | PHYMIPS_CLK_EN | ADSL_AFE_EN | ADSL_QPROC_EN)
#elif defined(CONFIG_BCM963268)
#include "63268_common.h"
#include "63268_map.h"
#define	XDSL_CLK_EN	(VDSL_CLK_EN | PHYMIPS_CLK_EN | VDSL_AFE_EN | VDSL_QPROC_EN)
#elif defined(CONFIG_BCM96318)
#include "6318_common.h"
#include "6318_map.h"
#ifndef ADSL_QPROC_EN
#define	ADSL_QPROC_EN   QPROC_CLK_EN   /* original definition in older code base */
#endif
#ifndef ADSL_AFE_EN
#define	ADSL_AFE_EN   AFE_CLK_EN   /* original definition in older code base */
#endif
#define	XDSL_CLK_EN	(ADSL_CLK_EN | PHYMIPS_CLK_EN | ADSL_AFE_EN | ADSL_QPROC_EN)
#elif defined(CONFIG_BCM963138)
#include "63138_map.h"
#elif defined(CONFIG_BCM963381)
#include "63381_map.h"
#elif defined(CONFIG_BCM963148)
#include "63148_map.h"
#endif

#ifdef VXWORKS
#include "interrupt.h"
#ifdef ADSL_IRQ
#undef	INTERRUPT_ID_ADSL
#define INTERRUPT_ID_ADSL   ADSL_IRQ
#endif
#elif defined(TARG_OS_RTEMS)
#include <stdarg.h>
#include "bspcfg.h"
#ifndef BD_BCM63XX_TIMER_CLOCK_INPUT
#define BD_BCM63XX_TIMER_CLOCK_INPUT BD_BCM6345_TIMER_CLOCK_INPUT
#endif
#define FPERIPH BD_BCM63XX_TIMER_CLOCK_INPUT
#else
#include "boardparms.h"
#endif /* VXWORKS */

#include "board.h"

#if !defined(TARG_OS_RTEMS) && !defined(VXWORKS) && !defined(_NOOS)
#include "bcm_intr.h"
#if !defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338)
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define INTERRUPT_ID_ADSL   INTERRUPT_ID_VDSL
#else
#define INTERRUPT_ID_ADSL   INTERRUPT_ID_XDSL
#endif	/* CONFIG_BCM963138/CONFIG_BCM963148 */
#endif
#endif

#include "bcmadsl.h"
#include "BcmAdslCore.h"
#include "AdslCore.h"
#include "AdslCoreMap.h"

#define EXCLUDE_CYGWIN32_TYPES
#include "softdsl/SoftDsl.h"
#include "softdsl/SoftDsl.gh"

#include "BcmAdslDiag.h"
#include "DiagDef.h"
#include "BcmAdslDiagCommon.h"

#include "AdslDrvVersion.h"

#if defined(SUPPORT_2CHIP_BONDING)
#include "softdsl/Flatten.h"
#include "bcm_ext_bonding_comm.h"
unsigned long extBondingDbgFlag = 0;	//TO DO: Remove after debugging
#endif

#ifdef XDSLDRV_ENABLE_MIBPRINT
#ifdef _NOOS
#include <string.h>
#else
#include <linux/string.h>
#endif
#endif

#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
#include <bcmxtmcfg.h>
#include "softdsl/SoftDslG993p2.h"

#if defined(NO_XTM_MODULE)
BCMXTM_STATUS BcmXtm_GetInterfaceCfg( UINT32 ulPortId, PXTM_INTERFACE_CFG pInterfaceCfg )
{
	return XTMSTS_STATE_ERROR;
}
#endif

#ifdef XDSLDRV_ENABLE_PARSER
void BcmAdslCoreDiagStatusParseAndPrint(dslStatusStruct *status);
#endif

#ifndef SUPPORT_TRAFFIC_TYPE_PTM_RAW
#define SUPPORT_TRAFFIC_TYPE_PTM_RAW	0
#endif
#ifndef SUPPORT_TRAFFIC_TYPE_PTM_BONDED
#define SUPPORT_TRAFFIC_TYPE_PTM_BONDED	0
#endif
#ifndef SUPPORT_TRAFFIC_TYPE_ATM_BONDED
#define SUPPORT_TRAFFIC_TYPE_ATM_BONDED	0
#endif

#ifdef BP_AFE_DEFAULT
#if (BP_AFE_CHIP_INT != (AFE_CHIP_INT << AFE_CHIP_SHIFT))
#error Inconsistent BP_AFE_CHIP_INT definition
#endif
#if (BP_AFE_LD_ISIL1556 != (AFE_LD_ISIL1556 << AFE_LD_SHIFT))
#error Inconsistent BP_AFE_LD_ISIL1556 definition
#endif
#if (BP_AFE_FE_ANNEXA != (AFE_FE_ANNEXA << AFE_FE_ANNEX_SHIFT))
#error Inconsistent BP_AFE_FE_ANNEXA definition
#endif
#if (BP_AFE_FE_REV_ISIL_REV1 != (AFE_FE_REV_ISIL_REV1 << AFE_FE_REV_SHIFT))
#error Inconsistent BP_AFE_FE_REV_ISIL_REV1 definition
#endif
#endif	/* BP_AFE_DEFAULT */
#endif	/* CONFIG_BCM96368 and newer chips */

#ifdef CONFIG_BCM96348
#define BCM6348_PLL_WORKAROUND
#endif

#undef	ADSLCORE_ENABLE_LONG_REACH
/* #define  DTAG_UR2 */
/* #define SAVE_STAT_LOCAL_AUTOSTART */

#define ADSL_MIPS_STATUS_INIT_TIMEOUT_MS  5000
#define ADSL_MIPS_STATUS_TIMEOUT_MS		60000

#if (ADSL_OEM_G994_VENDOR_ID != kDslOemG994VendorId)
#error Inconsistent ADSL_OEM_G994_VENDOR_ID definition
#endif
#if (ADSL_OEM_G994_XMT_NS_INFO != kDslOemG994XmtNSInfo)
#error Inconsistent ADSL_OEM_G994_XMT_NS_INFO definition
#endif
#if (ADSL_OEM_G994_RCV_NS_INFO != kDslOemG994RcvNSInfo)
#error Inconsistent ADSL_OEM_G994_RCV_NS_INFOdefinition
#endif
#if (ADSL_OEM_EOC_VENDOR_ID != kDslOemEocVendorId)
#error Inconsistent ADSL_OEM_EOC_VENDOR_ID definition
#endif
#if (ADSL_OEM_EOC_VERSION != kDslOemEocVersion)
#error Inconsistent ADSL_OEM_EOC_VERSION definition
#endif
#if (ADSL_OEM_EOC_SERIAL_NUMBER != kDslOemEocSerNum)
#error Inconsistent ADSL_OEM_EOC_SERIAL_NUMBER definition
#endif
#if (ADSL_OEM_T1413_VENDOR_ID != kDslOemT1413VendorId)
#error Inconsistent ADSL_OEM_T1413_VENDOR_ID definition
#endif
#if (ADSL_OEM_T1413_EOC_VENDOR_ID != kDslOemT1413EocVendorId)
#error Inconsistent ADSL_OEM_T1413_EOC_VENDOR_ID definition
#endif
#if (ADSL_XMT_GAIN_AUTO != kDslXmtGainAuto)
#error Inconsistent ADSL_XMT_GAIN_AUTO definition
#endif

#ifdef NTR_SUPPORT
#if (kNtrOperModeInt != NTR_OPER_MODE_INT)
#error Inconsistent kNtrOperModeInt definition
#endif
#if (kNtrStop != kDslTestNtrStop)
#error Inconsistent kNtrStop definition
#endif
#if (kNtrStart != kDslTestNtrStart)
#error Inconsistent kNtrStart definition
#endif
#endif

/* External vars */
extern adslPhyInfo	adslCorePhyDesc;
extern int		g_nAdslExit;
extern long	adslCoreEcUpdateMask;
extern void AdslCoreSetTime(ulong timeMs);
extern void *XdslCoreGetDslVars(unsigned char lineId);
extern void BcmXdsl_RequestIoCtlCallBack(void);
#if defined(SUPPORT_MULTI_PHY) || defined(CONFIG_BCM_DSL_GFAST)
extern void BcmXdsl_RequestSavePreferredLine(void);
#endif
extern void BcmAdslCoreDiagSetupRawData(int flag);
#if defined(SUPPORT_TEQ_FAKE_LINKUP)
extern int AdslDrvIsFakeLinkUp(unsigned char lineId);
#endif

//LGD_FOR_TR098
extern unsigned long g_ShowtimeStartTicks[];

#ifdef SUPPORT_STATUS_BACKUP
extern void AdslCoreBkupSbStat(void);
extern AC_BOOL AdslCoreSwitchCurSbStatToSharedSbStat(void);
#endif
#if defined(ADSLDRV_STATUS_PROFILING) || defined(SUPPORT_STATUS_BACKUP)
extern int AdslCoreStatusAvailTotal (void);
#endif
extern AC_BOOL AdslCoreStatusAvail (void);

#if defined(__KERNEL__)

#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)) && \
	(defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)))
#if defined(CONFIG_BCM_ATM_BONDING_ETH) || defined(CONFIG_BCM_ATM_BONDING_ETH_MODULE)
extern int atmbonding_enabled ;
#endif
#endif

#endif

/* Local vars */

#if defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
LOCAL Bool	xdslCorePhyImageJustSwitch = AC_FALSE;
LOCAL Bool	switchGfastToVdslImagePending = AC_FALSE;
#endif

#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
LOCAL OS_SEMID	irqSemId = (OS_SEMID)  0;
LOCAL OS_TASKID	IrqTid	 = (OS_TASKID) 0;
#else
LOCAL void *	irqDpcId = NULL;
LOCAL AC_BOOL	dpcScheduled=AC_FALSE;
OS_SEMID	syncPhyMipsSemId = (OS_SEMID)0;
#endif

LOCAL OS_TICKS	pingLastTick, statLastTick, initTick;
#if defined(SUPPORT_SELT) && !defined(PHY_BLOCK_TEST)
LOCAL OS_TICKS seltLastTick;
#endif
LOCAL ulong	noStatTimeout = ADSL_MIPS_STATUS_INIT_TIMEOUT_MS;

#ifdef ADSLDRV_STATUS_PROFILING
LOCAL OS_TICKS	printTicks = 0;
LOCAL ulong	intrDurTotal=0, intrDurMin=(ulong)-1, intrDurMax=0;
LOCAL ulong	intrDurMaxAtIntrCnt=0;
LOCAL ulong	intrDuringSchedCnt=0;
LOCAL ulong	dpcDurTotal=0, dpcDurMin=(ulong)-1, dpcDurMax=0;
LOCAL ulong	dpcDurMaxAtDpcCnt=0;
LOCAL int	dpcDurMaxAtByteAvail=0;

LOCAL ulong	dpcDelayTotal=0, dpcDelayMin=(ulong)-1, dpcDelayMax=0;
LOCAL ulong	dpcDelayMaxAtDpcCnt=0;
LOCAL int	dpcDelayMaxAtByteAvail=0;

LOCAL int	byteAvailMax=0, byteAvailMaxAtByteProc=0, byteAvailMaxAtNumStatProc=0;
LOCAL ulong	byteAvailMaxAtDpcCnt=0, byteAvailMaxAtDpcDur=0, byteAvailMaxAtDpcDelay=0;

LOCAL int	byteProcMax=0, byteProcMaxAtNumStatProc=0;
LOCAL ulong	byteProcMaxAtDpcCnt=0, byteProcMaxAtDpcDur=0, byteProcMaxAtDpcDelay=0;

LOCAL int	nStatProcMax=0, nStatProcMaxAtDpcCnt=0, nStatProcMaxAtByteAvail=0, nStatProcMaxAtByteProc=0;

LOCAL ulong	dpcScheduleTimeStamp=0;
int			gByteAvail=0;
#endif

#if defined(SUPPORT_STATUS_BACKUP) || defined(ADSLDRV_STATUS_PROFILING)
int			gByteProc=0;
ulong		gBkupStartAtIntrCnt=0, gBkupStartAtdpcCnt=0;
#endif

#ifdef SUPPORT_DSL_BONDING
LOCAL long		acPendingEvents[MAX_DSL_LINE] = {0, 0};
LOCAL long		acL3Requested[MAX_DSL_LINE] = {0, 0};
LOCAL OS_TICKS	acL3StartTick[MAX_DSL_LINE] = {0, 0};
#else
LOCAL long		acPendingEvents[MAX_DSL_LINE] = {0};
LOCAL long		acL3Requested[MAX_DSL_LINE] = {0};
LOCAL OS_TICKS	acL3StartTick[MAX_DSL_LINE] = {0};
#endif
#ifdef SUPPORT_SELT
#ifdef SUPPORT_DSL_BONDING
LOCAL OS_TICKS  acTestModeWait[MAX_DSL_LINE] = {0,0};
#ifdef SUPPORT_MULTI_PHY
LOCAL Bool      acSELTSuspendMediaSearch[MAX_DSL_LINE] = {AC_FALSE, AC_FALSE};
#endif
#else
LOCAL OS_TICKS  acTestModeWait[MAX_DSL_LINE] = {0};
#endif
#endif /* SUPPORT_SELT */
void (*bcmNotify)(unsigned char) = BcmAdsl_Notify;

dslCommandStruct	adslCoreConnectionParam[MAX_DSL_LINE];
dslCommandStruct	adslCoreCfgCmd[MAX_DSL_LINE]; 
adslCfgProfile		adslCoreCfgProfile[MAX_DSL_LINE];
#ifdef SUPPORT_DSL_BONDING
adslCfgProfile		*pAdslCoreCfgProfile[MAX_DSL_LINE] = {NULL, NULL};
#else
adslCfgProfile		*pAdslCoreCfgProfile[MAX_DSL_LINE] = {NULL};
#endif
#ifdef G992P3
g992p3DataPumpCapabilities	g992p3Param[MAX_DSL_LINE];
#endif
#ifdef G992P5
g992p3DataPumpCapabilities	g992p5Param[MAX_DSL_LINE];
#endif
#ifdef G993
g993p2DataPumpCapabilities g993p2Param[MAX_DSL_LINE];
#endif
Bool			adslCoreInitialized = AC_FALSE;
#ifdef SUPPORT_DSL_BONDING
Bool			adslCoreConnectionMode[MAX_DSL_LINE] = {AC_FALSE, AC_FALSE};
#else
Bool			adslCoreConnectionMode[MAX_DSL_LINE] = {AC_FALSE};
#endif
ulong			adslCoreIntrCnt		= 0;
ulong			adslCoreIsrTaskCnt	= 0;
#ifdef SUPPORT_DSL_BONDING
long			adslCoreXmtGain [MAX_DSL_LINE] = {ADSL_XMT_GAIN_AUTO, ADSL_XMT_GAIN_AUTO};
Bool			adslCoreXmtGainChanged[MAX_DSL_LINE] = {AC_FALSE, AC_FALSE};
#else
long			adslCoreXmtGain	[MAX_DSL_LINE] = {ADSL_XMT_GAIN_AUTO};
Bool			adslCoreXmtGainChanged[MAX_DSL_LINE] = {AC_FALSE};
#endif
#ifdef VXWORKS
Bool			adslCoreAlwaysReset = AC_FALSE;
#elif defined(SUPPORT_PHY_BIN_FROM_SDRAM)
Bool			adslCoreAlwaysReset = AC_FALSE;
#else
Bool			adslCoreAlwaysReset = AC_TRUE;
#endif
Bool			adslCoreStarted = AC_FALSE;
Bool			adslCoreResetPending = AC_FALSE;
ulong			adslCoreCyclesPerMs = 0;
Bool			adslCoreMuteDiagStatus = AC_FALSE;
#if defined(CONFIG_BCM963138)
Bool			adslCoreResetDelay = AC_FALSE;
#endif
#ifdef SUPPORT_DSL_BONDING
ulong			adslCoreHsModeSwitchTime[MAX_DSL_LINE] = {0, 0};
#else
ulong			adslCoreHsModeSwitchTime[MAX_DSL_LINE] = {0};
#endif
int			gConsoleOutputEnable = 1;
Bool		gDiagDataLog = AC_FALSE;
OS_TICKS	gDiagLastCmdTime = 0;
OS_TICKS	gDiagLastPingTime = 0;
ulong		gPhyPCAddr =HOST_LMEM_BASE + 0x5B0;

Bool			gSharedMemAllocFromUserContext = 0;

ulong		gDiagSyncTimeMs = 0;
ulong		gDiagSyncTimeLastCycleCount = 0;

#if defined(DSL_EXTERNAL_BONDING_DISCOVERY)
bonDiscExchangeStruct  gBndExch;
#endif

#if defined(PHY_LOOPBACK)
uchar			adslNewImg = 0;
#endif

struct {
	ulong		rcvCtrl, rcvAddr, rcvPtr;
	ulong		xmtCtrl, xmtAddr, xmtPtr;
} adslCoreDiagState;

typedef struct {
	int	nStatus;
	void	*pAddr;
	int	len;
	int	maxLen;
	int	readLen;
} xdslSavedStatInfo;

typedef struct {
	Bool	saveStatInitialized;
	Bool	saveStatStarted;
	Bool	saveStatContinous;
	Bool saveStatDisableOnRetrain;
	int	bufSize;
	void	*pBuf;
	int	curSplitStatEndLen;
	xdslSavedStatInfo	*pCurrentSavedStatInfo;
	xdslSavedStatInfo	savedStatInfo[2];
	xdslSavedStatInfo	*pCurrentReadStatInfo;
} xdslSavedStatCtrl;

DiagDebugData	diagDebugCmd = { 0, 0 };
#ifdef __kerSysGetAfeId
unsigned int	nvramAfeId[2] = {0};
#endif

#if defined(__KERNEL__) && (!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
char				*pBkupImage = NULL;
ulong			bkupSdramSize = 0;
ulong			bkupLmemSize = 0;
LOCAL Bool	pBkupImageEnable = FALSE;
#endif

LOCAL Bool	phyInExceptionState = FALSE;

#ifdef CONFIG_PHY_PARAM
LOCAL void *pLowMemAddr = NULL;
ulong       lmSize = 0;
dma_addr_t  lmAddrPhys;
struct device *pXdslDummyDevice = NULL;
#endif

#if defined(DSL_EXTERNAL_BONDING_DISCOVERY) && !defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
void BcmXdslCoreProcessBondDiscExchStat(dslStatusStruct *pStatus);
#endif

#if defined(SUPPORT_2CHIP_BONDING)
#if defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
void BcmXdslCoreProcessCmdFromExtBondDev(EXT_BONDING_COMM_BufferDesc *pBufDesc, void *devContext);
void BcmXdslCoreSendStatToExtBondDev(struct sk_buff *skbDiag, ulong cmd, char *buf0, int len0, char *buf1, int len1);
#elif defined(SUPPORT_EXT_DSL_BONDING_MASTER)
extern int BcmXdslCoreExtBondDiagWriteData(EXT_BONDING_COMM_BufferDesc *pBufDesc);
void BcmXdslCoreProcessStatFromExtBondDev(EXT_BONDING_COMM_BufferDesc *pBufDesc, void *devContext);
void BcmXdslCoreSendCmdToExtBondDev(dslCommandStruct *cmdPtr);
#endif
#endif

LOCAL void BcmAdslCoreSetConnectionParam(unsigned char lineId, dslCommandStruct *pDslCmd, dslCommandStruct *pCfgCmd, adslCfgProfile *pAdslCfg);
Bool BcmXdslDiagStatSaveLocalIsActive(void);
void BcmXdslDiagStatSaveLocal(ulong cmd, char *statStr, long n, char *p1, long n1);
int BcmXdslDiagStatSaveLocalRead(void *pBuf, int bufLen);

BCMADSL_STATUS BcmAdslDiagStatSaveInit(void *pAddr, int len);
BCMADSL_STATUS BcmAdslDiagStatSaveContinous(void);
BCMADSL_STATUS BcmAdslDiagStatSaveStart(void);
BCMADSL_STATUS BcmAdslDiagStatSaveStop(void);
BCMADSL_STATUS BcmAdslDiagStatSaveUnInit(void);
BCMADSL_STATUS BcmAdslDiagStatSaveGet(PADSL_SAVEDSTATUS_INFO pSavedStatInfo);

void BcmAdslCoreDiagWriteStatus(dslStatusStruct *status, char *pBuf, int len);
void BcmAdslCoreReset(int diagDataMap);

#ifdef PHY_PROFILE_SUPPORT
LOCAL void BcmAdslCoreProfilingStart(void);
void BcmAdslCoreProfilingStop(void);
#endif

LOCAL Bool BcmAdslCoreCanReset(void);
LOCAL void BcmAdslCoreEnableSnrMarginData(unsigned char lineId);
LOCAL void BcmAdslCoreDisableSnrMarginData(unsigned char lineId);
ulong BcmAdslCoreStatusSnooper(dslStatusStruct *status, char *pBuf, int len);
void BcmAdslCoreAfeTestStatus(dslStatusStruct *status);
void BcmAdslCoreSendBuffer(ulong statusCode, uchar *bufPtr, ulong bufLen);
void BcmAdslCoreSendDmaBuffers(ulong statusCode, int bufNum);

#if 1 || defined(PHY_BLOCK_TEST)
void BcmAdslCoreProcessTestCommand(void);
Bool BcmAdslCoreIsTestCommand(void);
void BcmAdslCoreDebugTimer(void);
void BcmAdslCoreDebugSendCommand(char *fileName, ushort cmd, ulong offset, ulong len, ulong bufAddr);
void BcmAdslCoreDebugPlaybackStop(void);
void BcmAdslCoreDebugPlaybackResume(void);
void BcmAdslCoreDebugReset(void);
void BcmAdslCoreDebugReadEndOfFile(void);
#endif

#ifndef _NOOS
#ifdef LINUX_KERNEL_2_6
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
LOCAL irqreturn_t BcmCoreInterruptHandler(int irq, void * dev_id);
#else
LOCAL irqreturn_t BcmCoreInterruptHandler(int irq, void * dev_id, struct pt_regs *ptregs);
#endif
#else
UINT32 BcmCoreInterruptHandler (void);
#endif
#endif /* !_NOOS */

void BcmCoreAtmVcInit(void);
void BcmCoreAtmVcSet(void);

#if defined(PHY_LOOPBACK) || defined(SUPPORT_TEQ_FAKE_LINKUP)
void AdslDrvXtmLinkUp(unsigned char lineId, unsigned char tpsTc);
void AdslDrvXtmLinkDown(unsigned char lineId);
#endif

#ifdef ADSLDRV_LITTLE_ENDIAN
#define BCM_STATHDR_SWAP32(val)	(val)
#else
#define BCM_STATHDR_SWAP32(val)	\
	((uint32)( \
		(((uint32)(val) & (uint32)0x000000ffUL) << 24) | \
		(((uint32)(val) & (uint32)0x0000ff00UL) <<  8) | \
		(((uint32)(val) & (uint32)0x00ff0000UL) >>  8) | \
		(((uint32)(val) & (uint32)0xff000000UL) >> 24) ))
#endif

#define	STAT_REC_HEADER		0x55AA1234
#define	DIAG_STRING_STATUS	1
#define	DIAG_BINARY_STATUS	2
#define	DIAG_STATTYPE_LINEID_SHIFT	13
#define	DIAG_STATTYPE_CLIENTTYPE_SHIFT	11
#define	DIAG_MIN_BUF_SIZE	2048

static struct {
	ulong	statSign;
	ulong	statId;
} statHdr = { STAT_REC_HEADER, 0 };

static xdslSavedStatCtrl	gSaveStatCtrl = {AC_FALSE, AC_FALSE, AC_FALSE, AC_FALSE, 0, NULL, 0, NULL, {{0,NULL,0,0,0}, {0,NULL,0,0,0}}};

#if defined(CONFIG_SMP) && defined(__KERNEL__)
spinlock_t	gSpinLock;
static ulong	spinCount[2] = {0,0};
#ifdef SPINLOCK_CALL_STAT
static ulong	spinLockNestedCallEnter[2] = {0,0};
static ulong	spinLockCalledEnter[2] = {0,0};
static ulong	spinLockNestedCallExit[2] = {0,0};
static ulong	spinLockCalledExit[2] = {0,0};
#endif
void SMP_DpcSyncEnter(void)
{
	int cpuId;
	
	local_bh_disable();
	cpuId = smp_processor_id();
	if (cpuId !=0 && cpuId != 1) {
		printk("unexpected CPU value! %d\n", cpuId);
		return;
	}
	spinCount[cpuId]++;
	if (spinCount[cpuId] != 1) {
#ifdef SPINLOCK_CALL_STAT
		spinLockNestedCallEnter[cpuId]++;
#endif
		return;	/* Prevent nested calls */
	}
#ifdef SPINLOCK_CALL_STAT
	spinLockCalledEnter[cpuId]++;
#endif
	spin_lock_bh(&gSpinLock);
}

void SMP_DpcSyncExit(void)
{
	int cpuId = smp_processor_id();
	
	if (cpuId !=0 && cpuId != 1) {
		printk("unexpected CPU value! %d\n", cpuId);
		local_bh_enable();
		return;
	}
	spinCount[cpuId]--;
	if (spinCount[cpuId] != 0) {
		local_bh_enable();
#ifdef SPINLOCK_CALL_STAT
		spinLockNestedCallExit[cpuId]++;
#endif
		return;
	}
#ifdef SPINLOCK_CALL_STAT
	spinLockCalledExit[cpuId]++;
#endif
	spin_unlock_bh(&gSpinLock);
	local_bh_enable();
}
#endif

#ifdef SUPPORT_DSL_BONDING
Bool BcmXdslCorePtmBondingEnable(unsigned char lineId)
{
	return ((adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures & kDslPtmBondingSupported) != 0);
}

Bool BcmXdslCoreAtmBondingEnable(unsigned char lineId)
{
	return ((adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures & kDslAtmBondingSupported) != 0);
}
#endif

#if defined(SUPPORT_2CHIP_BONDING)
Bool BcmXdslCoreExtBondEnable(unsigned char lineId)
{
#ifdef DSL_EXTERNAL_BONDING_DISCOVERY
	dslExtraCfgCommand  *pExtraCfg = (void *) ((char *)& adslCoreCfgCmd[lineId].param + sizeof(adslCoreCfgCmd[lineId].param.dslClearEocMsg));
	return((pExtraCfg->phyExtraCfg[0]&kPhyCfg1ExternalBondingDiscovery) != 0);
#else
	return AC_FALSE;
#endif
}

#ifdef SUPPORT_EXT_DSL_BONDING_MASTER
#define CMD_BUF_SIZE	1400
#define STAT_BUF_SIZE	0x4000
static ulong flattenCmdBuf[CMD_BUF_SIZE>>2];
static ulong xdslStatusBuffer[STAT_BUF_SIZE>>2];
static int xdslStatusBufferSize=0;
static Bool bWaitNextStatus=FALSE;

void BcmXdslCoreSendCmdToExtBondDev(dslCommandStruct *cmdPtr)
{
	int cmdLen;
	EXT_BONDING_COMM_BufferDesc *pBufDesc;
	dslCommandCode cmdCode = cmdPtr->command;
	
	cmdPtr->command &= ~(1 << DSL_LINE_SHIFT);
	cmdLen = FlattenCommand(cmdPtr, &flattenCmdBuf, CMD_BUF_SIZE);
	cmdPtr->command= cmdCode;
	
	if(0 == cmdLen) {
		printk("%s: Fail flatten cmdCode=%x, value=%x\n",
			__FUNCTION__, cmdPtr->command, cmdPtr->param.value);
		return;
	}
	
	if( NULL == (pBufDesc = EXT_BONDING_COMM_allocateBuffer(cmdLen)) ) {
		printk("%s: Fail allocating BufferDesc for cmdCode=%x, value=%x cmdLen=%d\n",
			__FUNCTION__, cmdPtr->command, cmdPtr->param.value, cmdLen);
		return;
	}
	
	memcpy(EXT_BONDING_COMM_getData(pBufDesc), (void*)&flattenCmdBuf, cmdLen);
	EXT_BONDING_COMM_sendFrame(pBufDesc, EXT_BONDING_DSL_CMD);
}

void BcmXdslCoreProcessStatFromExtBondDev(EXT_BONDING_COMM_BufferDesc *pBufDesc, void *devContext)
{
	LogProtoHeader *pBuf = EXT_BONDING_COMM_getData(pBufDesc);
	int bufLen = EXT_BONDING_COMM_getBufferSize(pBufDesc);
	unsigned char logCommmand = pBuf->logCommmand;
	
	if(extBondingDbgFlag & 1)
		printk("%s: bufLen=%d, logCommmand=%d\n", __FUNCTION__, bufLen, pBuf->logCommmand);
	
	if((statusInfoData == logCommmand) && (LOG_PROTO_ID[1] == (pBuf->logProtoId[1]-1)))
		logCommmand++;		/* statusInfoData + 1: first splitted status */
	bufLen -= sizeof(LogProtoHeader);
	
	switch(logCommmand) {
		case statusInfoData + 1:
			/* FIRST SPLITTED STATUS */
			memcpy ((char *)&xdslStatusBuffer, pBuf+1, bufLen);
			xdslStatusBufferSize = bufLen;
			bWaitNextStatus = TRUE;
			break;
		case statusInfoData-2:
			/* INTERMEDIATE  SPLITTED STATUS */
			if(bWaitNextStatus && ((xdslStatusBufferSize + bufLen) < STAT_BUF_SIZE)) {
				memcpy ((char *)&xdslStatusBuffer+xdslStatusBufferSize, pBuf+1, bufLen);
				xdslStatusBufferSize += bufLen;
			}
			else {
				printk("%s: bufLen=%d, logCommmand=%d bWaitNextStatus=%d xdslStatusBufferSize=%d\n",
				__FUNCTION__, bufLen, logCommmand, bWaitNextStatus, xdslStatusBufferSize);
			}
			break;
		case statusInfoData-3:
			/* LAST SPLITTED STATUS */
			if(bWaitNextStatus && ((xdslStatusBufferSize + bufLen) < STAT_BUF_SIZE)) {
				memcpy ((char *)&xdslStatusBuffer+xdslStatusBufferSize, pBuf+1, bufLen);
				bufLen += xdslStatusBufferSize;
				pBuf = (void *) xdslStatusBuffer;
				pBuf--;
			}
			else {
				printk("%s: bufLen=%d, logCommmand=%d bWaitNextStatus=%d xdslStatusBufferSize=%d\n",
				__FUNCTION__, bufLen, logCommmand, bWaitNextStatus, xdslStatusBufferSize);
				break;
			}
			xdslStatusBufferSize = 0;
			bWaitNextStatus=FALSE;
			/* fall through */
		case statusInfoData:
		case statusInfoData-1:
			{
			dslStatusStruct status;
			int unflattenLen = UnflattenStatus((ulong *)(pBuf+1), &status);
			
			if(extBondingDbgFlag & 2)
				printk("%s: statLen=%d unflattenLen=%d statusCode=%d, value=%d\n",
					__FUNCTION__, bufLen, unflattenLen, (int)status.code, status.param.value);
			
			if( (statusInfoData-3) == logCommmand ) {
				if(extBondingDbgFlag & 4)
					printk("*** %s: Splitted statLen=%d unflattenLen=%d statusCode=%d, value=%d\n",
						__FUNCTION__, bufLen, unflattenLen, (int)status.code, status.param.value);	//TO DO: forward to DslDiags.
			}
			else
				BcmXdslCoreExtBondDiagWriteData(pBufDesc);
			
			if( (kDslReceivedEocCommand == DSL_STATUS_CODE(status.code)) &&
				(kDslBondDiscExchange == status.param.value))
				BcmXdslCoreProcessBondDiscExchStat(&status);
			else
				AdslMibStatusSnooper(XdslCoreGetDslVars(1), &status);
			break;
			}
		default:
			BcmXdslCoreExtBondDiagWriteData(pBufDesc);
			break;
	}
	EXT_BONDING_COMM_freeFrameBuffer(pBufDesc);
}

#endif

#ifdef SUPPORT_EXT_DSL_BONDING_SLAVE
void BcmXdslCoreProcessCmdFromExtBondDev(EXT_BONDING_COMM_BufferDesc *pBufDesc, void *devContext)
{
	dslCommandStruct	cmd;
	
	UnflattenCommand((ulong *)EXT_BONDING_COMM_getData(pBufDesc), &cmd);
	AdslCoreCommandHandler(&cmd);
	EXT_BONDING_COMM_freeFrameBuffer(pBufDesc);
}

extern int BcmXdslCoreDiagGetFrHdrLen(void);
void BcmXdslCoreSendStatToExtBondDev(struct sk_buff *skbDiag, ulong cmd, char *buf0, int len0, char *buf1, int len1)
{
	int bufDescLen;
	EXT_BONDING_COMM_BufferDesc *pBufDesc;
	LogProtoHeader *pLogProtHdr;

	if(NULL == skbDiag)
		bufDescLen = len0 + len1 + sizeof(LogProtoHeader);
	else
		bufDescLen = skbDiag->len - BcmXdslCoreDiagGetFrHdrLen() + sizeof(LogProtoHeader);
	
	if( NULL == (pBufDesc = EXT_BONDING_COMM_allocateBuffer(bufDescLen)) ) {
		printk("%s: Fail allocating BufferDesc len=%d\n", __FUNCTION__, bufDescLen);
		return;
	}
	
	pLogProtHdr = (LogProtoHeader *)EXT_BONDING_COMM_getData(pBufDesc);

	if(NULL == skbDiag) {
		int n = *(short *) LOG_PROTO_ID;
		
		if (cmd & DIAG_SPLIT_MSG)
			n++;
		*(short *)pLogProtHdr->logProtoId = n;
		pLogProtHdr->logCommmand = cmd & 0xFF;
		memcpy ((char *)(pLogProtHdr+1), buf0, len0);
		if (NULL != buf1)
			memcpy ((char *)(pLogProtHdr+1)+len0, buf1, len1);
	}
	else
		memcpy((char *)pLogProtHdr, (char *)skbDiag->data+BcmXdslCoreDiagGetFrHdrLen()-sizeof(LogProtoHeader), bufDescLen);
	
	pLogProtHdr->logPartyId = LOG_PARTY_CLIENT | (1 << DIAG_PARTY_LINEID_SHIFT);
	if(statusInfoData == pLogProtHdr->logCommmand) {
		ulong *pStatCode = (ulong *)(pLogProtHdr+1);
		*pStatCode |= (1 << DSL_LINE_SHIFT);
	}
	
	if(extBondingDbgFlag & 1)
		printk("%s: bufLen=%d, logCommmand=%d\n", __FUNCTION__, bufDescLen, pLogProtHdr->logCommmand);
	
	EXT_BONDING_COMM_sendFrame(pBufDesc, EXT_BONDING_DSL_STAT);
}
#endif

#endif	/* SUPPORT_2CHIP_BONDING */

#if defined(DSL_EXTERNAL_BONDING_DISCOVERY) && !defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
void BcmXdslCoreProcessBondDiscExchStat(dslStatusStruct *pStatus)
{
	bonDiscExchangeStruct *pBD = (bonDiscExchangeStruct *) (pStatus->param.dslClearEocMsg.dataPtr); 
	short bdCmd = pBD->bdCmd & (~kBondDiscCmdReqReply);
	unsigned int aggDiscLow=pBD->bondDisc.pmeRemoteDiscoveryLow & 0xffffffff, aggDiscHigh=pBD->bondDisc.pmeRemoteDiscoveryHigh & 0xffffff;
	int clearIfSame = bdCmd & kBondDiscCmdClearIfSame;
	int isSame,isClear;
	
	uchar lineId = DSL_LINE_ID(pStatus->code);

	if (bdCmd & kBondDiscCmdSet) {
		if (bdCmd & kBondDiscCmdDiscovery) {
			isSame = (gBndExch.bondDisc.pmeRemoteDiscoveryLow==aggDiscLow) && (gBndExch.bondDisc.pmeRemoteDiscoveryHigh==aggDiscHigh);
			isClear = (gBndExch.bondDisc.pmeRemoteDiscoveryLow==0) && (gBndExch.bondDisc.pmeRemoteDiscoveryHigh==0);
			if (clearIfSame) {
				if (isSame) {
					aggDiscLow  = 0;
					aggDiscHigh = 0;
					gBndExch.bondDisc.pmeAggregationReg = 0;
				}
				else {
					aggDiscLow  = gBndExch.bondDisc.pmeRemoteDiscoveryLow; 
					aggDiscHigh = gBndExch.bondDisc.pmeRemoteDiscoveryHigh;
				}
			}
			else {
				/* this means SetIfClear*/
				if (!isClear) {
					aggDiscLow  = gBndExch.bondDisc.pmeRemoteDiscoveryLow; 
					aggDiscHigh = gBndExch.bondDisc.pmeRemoteDiscoveryHigh;
				}
			}

			gBndExch.bondDisc.pmeRemoteDiscoveryLow   = aggDiscLow;
			gBndExch.bondDisc.pmeRemoteDiscoveryHigh  = aggDiscHigh;
		}
		
		if (bdCmd & kBondDiscCmdAggregate) 
			gBndExch.bondDisc.pmeAggregationReg |= (1 << lineId);
	}

	if (pBD->bdCmd & kBondDiscCmdReqReply) {
		dslCommandStruct	cmd;
		dslStatusStruct	status;

		gBndExch.bdCmd &= ~kBondDiscCmdReqReply;
		cmd.command = kDslSendEocCommand | (lineId << DSL_LINE_SHIFT);
		cmd.param.dslClearEocMsg.msgId = kDslBondDiscExchange;
		cmd.param.dslClearEocMsg.msgType = sizeof(bonDiscExchangeStruct) | kDslClearEocMsgDataVolatile;
		cmd.param.dslClearEocMsg.dataPtr = (void *) &gBndExch;
#ifdef SUPPORT_EXT_DSL_BONDING_MASTER
		if(0 != lineId)
			BcmXdslCoreSendCmdToExtBondDev(&cmd);
		else
			AdslCoreCommandHandler(&cmd);
#else
		AdslCoreCommandHandler(&cmd);
#endif
		status.code = kDslReceivedEocCommand | (lineId << DSL_LINE_SHIFT);
		status.param.dslClearEocMsg.msgId = kDslBondDiscExchangeDrv;
		status.param.dslClearEocMsg.msgType = sizeof(bonDiscExchangeStruct);
		status.param.dslClearEocMsg.dataPtr = (void *) &gBndExch;
		BcmAdslCoreDiagWriteStatus(&status, (char *)&status, sizeof(status.code) + sizeof(status.param.dslClearEocMsg));
	}
}
#endif

#ifdef CONFIG_BCM_DSL_GFAST
void BcmXdslCoreSetPreferredPhyType(void);
AC_BOOL BcmXdslCoreClearPreferredPhyType(void);

void BcmXdslCoreSetPreferredPhyType(void)
{
	if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERPHY_DISABLED) &&
		!(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDPHY_FOUND)) {
		adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_PHYTYPE_MSK;
		adslCoreCfgProfile[0].xdslMiscCfgParam |= (BCM_PHYTYPE_NON_GFAST | BCM_PREFERREDPHY_FOUND);
		DiagWriteString(0, DIAG_DSL_CLIENT, "Set preferred PHY: Non Gfast\n");
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("Set preferred PHY: Non Gfast\n")));
		BcmXdsl_RequestSavePreferredLine();
	}
}

AC_BOOL BcmXdslCoreClearPreferredPhyType(void)
{
	AC_BOOL res = FALSE;
	
	if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDPHY_FOUND) {
		adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_PREFERREDPHY_FOUND;
		adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_PHYTYPE_MSK;
		adslCoreCfgProfile[0].xdslMiscCfgParam |= BCM_PHYTYPE_GFAST;
		DiagWriteString(0, DIAG_DSL_CLIENT, "Clear preferred PHY: Non Gfast\n");
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("Clear preferred PHY: Non Gfast\n")));
		if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERPHY_DISABLED))
			res = TRUE;
	}
	
	return res;
}
#endif

#ifdef SUPPORT_MULTI_PHY
int	bcmConfiguredImageType=BCM_IMAGETYPE_BONDING;
extern void BcmXdsl_NotifyMisMatchTrafficType(void);
void BcmXdslCoreMediaSearchSetPreferredMedia(void);
AC_BOOL BcmXdslCoreMediaSearchClearPreferredMedia(void);

void BcmXdslCoreSwitchPhyImage(int imageType)
{
	if( (imageType & (~BCM_IMAGETYPE_MSK)) || (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED))
		return;
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
	adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_IMAGETYPE_MSK;
	adslCoreCfgProfile[0].xdslMiscCfgParam |= imageType;
	adslCoreCfgProfile[1].xdslMiscCfgParam = adslCoreCfgProfile[0].xdslMiscCfgParam;
	XdslCoreSetPhyImageType(imageType);
	
	if(BCM_IMAGETYPE_BONDING == imageType) {
		if(BcmXdslCoreMediaSearchClearPreferredMedia())
			BcmXdsl_RequestSavePreferredLine();
	}
	
	DiagWriteString(0, DIAG_DSL_CLIENT, "*** Drv:  Switching to %s image ***\n",
		(BCM_IMAGETYPE_BONDING == imageType) ? "bonding": "single line");
	
	if(imageType != bcmConfiguredImageType)
		BcmXdsl_NotifyMisMatchTrafficType();
#endif
	adslCoreResetPending = AC_TRUE;
#if !defined(__ECOS)
	BcmXdsl_RequestIoCtlCallBack();
#endif /* !defined(__ECOS) */
	BcmXdslCoreMediaSearchInit();
}

/* Time out in seconds */
#define	MEDIASEARCH_HSTONE_TIMEOUT	30
#define	MEDIASEARCH_TRAINING_TIMEOUT	120
#define	MEDIASEARCH_VECT_TRAINING_TIMEOUT	300
#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
static int	mediaSearchTrainingTimeout = MEDIASEARCH_TRAINING_TIMEOUT;
#endif

typedef enum {
	DSL_LINE_INNERPAIR = 0,	/* Mapped to primary path afeId */
	DSL_LINE_OUTERPAIR,	/* Mapped to external path afeId */
	DSL_LINE_MAX_MEDIA
} xdslMedia;

typedef struct _xdslSearchEnt {
	Bool	searched;		/* This media has been searched */
	Bool	signalPresent;	/* This media has found HS tones */
	Bool	showTime;		/* This media has reached showtime */
	Bool	timeOut;		/* This media has timed out in searching */
	ulong	searchTime;	/* time used so far */
	ulong	rxAtten;		/* Rx attenuation */
} xdslSearchEnt;

typedef enum {
	MEDIASEARCH_UNKNOWN_S = 0,
	MEDIASEARCH_INITIALIZED_S,
	MEDIASEARCH_INPROGRESS_S,
	MEDIASEARCH_RESTARTPHY_S,
	MEDIASEARCH_DONE_S
} xdslMediaSearchState;

typedef struct _xdslSearchMedia {
	xdslMediaSearchState	searchState;	/* Searching-started or searching-done */
	xdslMedia	mediaInUse;
	xdslMedia	mediaPreferred;
	xdslSearchEnt	searchEnt[DSL_LINE_MAX_MEDIA];
} xdslSearchMedia;

xdslSearchMedia	xdslSearchMediaCfg;

#define MEDIA_SIGNAL_PRESENT(x)	xdslSearchMediaCfg.searchEnt[(x)].signalPresent
#define MEDIA_RX_LNATTNAVG(x)	xdslSearchMediaCfg.searchEnt[(x)].rxAtten
#define MEDIA_IN_SHOWTIME(x)	xdslSearchMediaCfg.searchEnt[(x)].showTime
#define MEDIA_SIGNALDETECT_TIMEOUT(x)	(xdslSearchMediaCfg.searchEnt[(x)].searchTime >= MEDIASEARCH_HSTONE_TIMEOUT)
#define MEDIA_TRAIN_TIMEOUT(x)	(xdslSearchMediaCfg.searchEnt[(x)].searchTime >= mediaSearchTrainingTimeout)
#define MEDIA_SEARCHTIME(x)	xdslSearchMediaCfg.searchEnt[(x)].searchTime
#define MEDIA_SEARCHED(x)	xdslSearchMediaCfg.searchEnt[(x)].searched

static AC_BOOL	bMediaSearchSuspended = AC_FALSE;
static AC_BOOL	bMediaSearchBndIndicated[2] = { AC_FALSE, AC_FALSE };
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
AC_BOOL	bMediaSearchLine0WakeupCmdPending = AC_FALSE;
#endif

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
void BcmXdslCorePrintCurrentMedia(void)
{
	unsigned int	mediaInUse[2];
	if(BCM_IMAGETYPE_BONDING != XdslCoreGetPhyImageType()) {
		BcmXdslCoreGetAfeBoardId(&mediaInUse[0]);
		mediaInUse[0] = mediaInUse[xdslSearchMediaCfg.mediaInUse];		/* Non-bonding PHY will use mediaInUse[0] */
		DiagWriteString(0, DIAG_DSL_CLIENT, "XdslMediaSearch: current media(%d) --> 0x%08X\n",\
			xdslSearchMediaCfg.mediaInUse, (unsigned int)mediaInUse[0]);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("XdslMediaSearch: current media(%d) --> 0x%08X\n"),\
			xdslSearchMediaCfg.mediaInUse, (unsigned int)mediaInUse[0]));
	}
}

LOCAL void BcmXdslCoreSetDefaultMediaType(long bcmMediaType)
{
	bcmMediaType &= BCM_MEDIATYPE_MSK;
	adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_MEDIATYPE_MSK;
	adslCoreCfgProfile[0].xdslMiscCfgParam |= bcmMediaType;
	adslCoreCfgProfile[1].xdslMiscCfgParam = adslCoreCfgProfile[0].xdslMiscCfgParam;
	DiagWriteString(0, DIAG_DSL_CLIENT, "BcmXdslCoreSetDefaultMediaType: mediaType (%d)\n",
		(BCM_MEDIATYPE_EXTERNALAFE==bcmMediaType)? DSL_LINE_OUTERPAIR: DSL_LINE_INNERPAIR);
	BCMOS_EVENT_LOG((KERN_CRIT TEXT("BcmXdslCoreSetDefaultMediaType: mediaType (%d)\n"),
		(BCM_MEDIATYPE_EXTERNALAFE==bcmMediaType)? DSL_LINE_OUTERPAIR: DSL_LINE_INNERPAIR));
}

void BcmXdslCoreGetCurrentMedia(unsigned int *pMediaInUse)
{
	BcmXdslCoreGetAfeBoardId(pMediaInUse);
	
	if(!ADSL_PHY_SUPPORT(kAdslPhyBonding))
		pMediaInUse[0] = pMediaInUse[xdslSearchMediaCfg.mediaInUse];		/* Non-bonding PHY will use pMediaInUse[0] */
}
#endif

LOCAL Bool BcmXdslCoreSelectPreferredMedia(xdslMedia * pPreferredMedia)
{
	char str[128];
	Bool res = FALSE;
	xdslMedia i = DSL_LINE_INNERPAIR;	/* DSL_LINE_INNERPAIR == 0 */
	xdslMedia preferredMedia = i;
	xdslMedia lastMediaSearch = DSL_LINE_MAX_MEDIA -1;
	
	while( i < lastMediaSearch ) {
		if(MEDIA_SIGNAL_PRESENT(preferredMedia) == MEDIA_SIGNAL_PRESENT(i+1)) {
			if(MEDIA_SIGNAL_PRESENT(preferredMedia)) {
				xdslMedia otherMedia;
				res = TRUE;	/* Signal is present on both media, pick one with smaller rx attenuation */
				if( MEDIA_RX_LNATTNAVG(i+1) < MEDIA_RX_LNATTNAVG(preferredMedia) ) {
					otherMedia = preferredMedia;
					preferredMedia = i+1;	/* if rxAtten of preferredMedia and (i+1) are equal, keep preferredMedia */
				}
				else
					otherMedia = preferredMedia + 1;
				sprintf(str, "XdslMediaSearch: Signal found on both media, preferredMedia(%d) rxAtten(%d), otherMedia(%d) rxAtten(%d)\n",
					preferredMedia, (int)MEDIA_RX_LNATTNAVG(preferredMedia), otherMedia, (int)MEDIA_RX_LNATTNAVG(otherMedia));
				DiagWriteString(0, DIAG_DSL_CLIENT, str);
				BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
			}
			else {
				sprintf(str, "XdslMediaSearch: No signal found on both media\n");
				DiagWriteString(0, DIAG_DSL_CLIENT, str);
				BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
			}
		}
		else {
			res = TRUE;	/* Only one media detected signal */
			if(MEDIA_SIGNAL_PRESENT(i+1))
				preferredMedia = i+1;
			sprintf(str, "XdslMediaSearch: Only one media detected signal, preferredMedia(%d) rxAtten(%d)\n",
				preferredMedia, (int)MEDIA_RX_LNATTNAVG(preferredMedia));
			DiagWriteString(0, DIAG_DSL_CLIENT, str);
			BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
		}
		i++;
	}
	
	if(TRUE == res)
		*pPreferredMedia = preferredMedia;
	
	return res;
}

LOCAL void BcmXdslCoreMediaSearchEntSetDefault(xdslSearchEnt *pSearchEnt)
{
	pSearchEnt->searched = FALSE;
	pSearchEnt->signalPresent = FALSE;
	pSearchEnt->showTime = FALSE;
	pSearchEnt->timeOut = FALSE;
	pSearchEnt->searchTime = 0;
	pSearchEnt->rxAtten = (ulong)-1;
}

#if !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
LOCAL void BcmXdslCoreMediaSearchNext(void)
{
	char str[64];
	xdslMedia nextMediaType = (xdslSearchMediaCfg.mediaInUse==DSL_LINE_INNERPAIR)? DSL_LINE_OUTERPAIR: DSL_LINE_INNERPAIR;
	
	if( MEDIA_SEARCHED(nextMediaType)) {
		/* ALL media have been searched, select the preferred media */
		sprintf(str, "XdslMediaSearch: All media have been searched\n");
		DiagWriteString(0, DIAG_DSL_CLIENT, str);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
		if(BcmXdslCoreSelectPreferredMedia(&xdslSearchMediaCfg.mediaPreferred)) {
			xdslSearchMediaCfg.searchState = MEDIASEARCH_DONE_S;
		}
		else {
			/* Restart the search */
			sprintf(str, "XdslMediaSearch: No preferred media - redo search\n");
			DiagWriteString(0, DIAG_DSL_CLIENT, str);
			BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
			BcmXdslCoreMediaSearchInit();
			xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
		}
	}
	else {
		BcmXdslCoreMediaSearchEntSetDefault(&xdslSearchMediaCfg.searchEnt[nextMediaType]);
		xdslSearchMediaCfg.mediaInUse = nextMediaType;
		sprintf(str, "XdslMediaSearch: Next search media(%d)\n", nextMediaType);
		DiagWriteString(0, DIAG_DSL_CLIENT, str);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
	}
	
	if ( MEDIASEARCH_INPROGRESS_S == xdslSearchMediaCfg.searchState )
		BcmXdslCoreMediaSearchReStartPhy();
	else {
		/* Found preferred media */
		/* restart PHY if the preferred media is not the last searched media */
		if(xdslSearchMediaCfg.mediaInUse != xdslSearchMediaCfg.mediaPreferred) {
			ulong rxAtten;
			xdslSearchMediaCfg.mediaInUse = xdslSearchMediaCfg.mediaPreferred;
			rxAtten = xdslSearchMediaCfg.searchEnt[xdslSearchMediaCfg.mediaInUse].rxAtten;
			BcmXdslCoreMediaSearchEntSetDefault(&xdslSearchMediaCfg.searchEnt[xdslSearchMediaCfg.mediaInUse]);
			xdslSearchMediaCfg.searchEnt[xdslSearchMediaCfg.mediaInUse].rxAtten = rxAtten;
			BcmXdslCoreMediaSearchReStartPhy();
		}
		else
			BcmXdslCoreMediaSearchSetPreferredMedia();
	}
}
#endif

unsigned char BcmXdslCoreSelectAndReturnPreferredMedia(void)
{
	unsigned lineId = (unsigned char)-1;
	
	if(BcmXdslCoreSelectPreferredMedia(&xdslSearchMediaCfg.mediaPreferred)) {
		lineId = (unsigned char)xdslSearchMediaCfg.mediaPreferred;
		xdslSearchMediaCfg.searchState = MEDIASEARCH_DONE_S;
#ifdef CONFIG_BCM963268
		xdslSearchMediaCfg.mediaInUse = xdslSearchMediaCfg.mediaPreferred;
		BcmXdslCoreMediaSearchSetPreferredMedia();
#endif
	}
	return lineId;
}

void BcmXdslCoreMediaSearchInit(void)
{
	int i;
	
	BcmCoreDpcSyncEnter();
	memset((void *)&xdslSearchMediaCfg, 0, sizeof(xdslSearchMedia));
	for(i= 0; i < DSL_LINE_MAX_MEDIA; i++)
		BcmXdslCoreMediaSearchEntSetDefault(&xdslSearchMediaCfg.searchEnt[i]);
	if((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIATYPE_MSK) == BCM_MEDIATYPE_INTERNALAFE)
		xdslSearchMediaCfg.mediaInUse = DSL_LINE_INNERPAIR;
	else
		xdslSearchMediaCfg.mediaInUse = DSL_LINE_OUTERPAIR;
#if defined(CONFIG_BCM963268)
	if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERMEDIA_DISABLED) && 
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDTYPE_FOUND))
		xdslSearchMediaCfg.searchState = MEDIASEARCH_DONE_S;
	else
#endif
		xdslSearchMediaCfg.searchState = MEDIASEARCH_INITIALIZED_S;
	BcmCoreDpcSyncExit();
}

AC_BOOL BcmXdslCoreMediaSearchInInitState(void)
{
	return (MEDIASEARCH_INITIALIZED_S == xdslSearchMediaCfg.searchState);
}

void BcmXdslCoreMediaSearchSetPreferredMedia(void)
{
	if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERMEDIA_DISABLED) &&
		!(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDTYPE_FOUND)) {
#ifdef CONFIG_BCM963268
		adslCoreCfgProfile[0].xdslMiscCfgParam |= (BCM_IMAGETYPE_SINGLELINE | BCM_PREFERREDTYPE_FOUND);
#else
		adslCoreCfgProfile[0].xdslMiscCfgParam |= BCM_PREFERREDTYPE_FOUND;
#endif
		adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_MEDIATYPE_MSK;
		adslCoreCfgProfile[0].xdslMiscCfgParam |= ((DSL_LINE_INNERPAIR==xdslSearchMediaCfg.mediaInUse) ? BCM_MEDIATYPE_INTERNALAFE: BCM_MEDIATYPE_EXTERNALAFE);
		DiagWriteString(0, DIAG_DSL_CLIENT, "XdslMediaSearch: Set preferred media(%d)\n", xdslSearchMediaCfg.mediaInUse);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("XdslMediaSearch: Set preferred media(%d)\n"), xdslSearchMediaCfg.mediaInUse));
		BcmXdsl_RequestSavePreferredLine();
	}
}

AC_BOOL BcmXdslCoreMediaSearchClearPreferredMedia(void)
{
	AC_BOOL res = FALSE;
	
	if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDTYPE_FOUND) {
		adslCoreCfgProfile[0].xdslMiscCfgParam &= ~BCM_PREFERREDTYPE_FOUND;
		DiagWriteString(0, DIAG_DSL_CLIENT, "XdslMediaSearch: Clear preferred media(%d)\n", xdslSearchMediaCfg.mediaInUse);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("XdslMediaSearch: Clear preferred media(%d)\n"), xdslSearchMediaCfg.mediaInUse));
		if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERMEDIA_DISABLED))
			res = TRUE;
	}
	
	return res;
}

void BcmXdslCoreMediaSearchSM(xdslMediaSearchEvent mediaSearchEvent, ulong param)
{
	xdslMedia curMedia;
	xdslMediaSearchState	curState;
	Boolean	skipMediaSearchSM;
	char	str[128];
	
	if(ADSL_PHY_SUPPORT(kAdslPhyBonding))
		curMedia = XdslCoreGetCurLineId();
	else
		curMedia = xdslSearchMediaCfg.mediaInUse;
	
#if defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
	if(ADSL_PHY_SUPPORT(kAdslPhyGfast))
		skipMediaSearchSM = TRUE;
	else
#endif
	if(TRUE == bMediaSearchBndIndicated[curMedia]) {
		long	size = sizeof(adslMibInfo);
		adslMibInfo	*adslMib = (void *)AdslCoreGetObjectValue (curMedia, NULL, 0, NULL, &size);
		skipMediaSearchSM = (adslMib->xdslStat[0].bondingStat.status != 0) ? TRUE: FALSE;
	}
	else
		skipMediaSearchSM = FALSE;
	
	if((AC_TRUE == skipMediaSearchSM) || (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIASEARCH_DISABLED) ||
		(AC_TRUE == bMediaSearchSuspended))
		return;
	
	BcmCoreDpcSyncEnter();
	curState = xdslSearchMediaCfg.searchState;
	
	switch(curState) {
		case MEDIASEARCH_INITIALIZED_S:
			switch(mediaSearchEvent) {
				case MEDIASEARCH_START_E:
					sprintf(str, "XdslMediaSearch: INIT_S received start event, media(%d)\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
					break;
				default:
					break;
			}
			break;
		case MEDIASEARCH_INPROGRESS_S:
			switch(mediaSearchEvent) {
				case MEDIASEARCH_START_E:
					/* Either from a PHY switch or Diags download or new media search which require PHY image reload */
					sprintf(str, "XdslMediaSearch: INPROGRESS_S received start event, media(%d)\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					break;
#ifdef CONFIG_BCM963268
				case MEDIASEARCH_TIMER_E:
					if((AC_FALSE == adslCoreStarted) || ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
						BcmCoreDpcSyncExit();
						return;
					}
					if (MEDIA_IN_SHOWTIME(curMedia))
						BcmXdslCoreMediaSearchNext();
					else
					if( (!MEDIA_SIGNAL_PRESENT(curMedia) && MEDIA_SIGNALDETECT_TIMEOUT(curMedia)) ||
						MEDIA_TRAIN_TIMEOUT(curMedia) ) {
						sprintf(str, "XdslMediaSearch: %s timeout(%d secs), media(%d)\n", \
							(!MEDIA_SIGNAL_PRESENT(curMedia)) ? "detect signal": "train",\
							(int)MEDIA_SEARCHTIME(curMedia), curMedia);
						DiagWriteString(0, DIAG_DSL_CLIENT, str);
						BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"),str));
						MEDIA_SEARCHED(curMedia) = TRUE;
						BcmXdslCoreMediaSearchNext();
					}
					else 
						MEDIA_SEARCHTIME(curMedia)++;

					break;
#endif
				case MEDIASEARCH_SIGNAL_E:
					if(!MEDIA_SIGNAL_PRESENT(curMedia)) {
						sprintf(str, "XdslMediaSearch: received signal event, media(%d)\n", curMedia);
						DiagWriteString(0, DIAG_DSL_CLIENT, str);
						BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					}
					MEDIA_SIGNAL_PRESENT(curMedia) = TRUE;
					break;
				case MEDIASEARCH_RXLNATTNAVG_E:
				{
					xdslMedia otherMedia = (DSL_LINE_INNERPAIR==curMedia) ? DSL_LINE_OUTERPAIR: DSL_LINE_INNERPAIR;
					sprintf(str, "XdslMediaSearch: received rx attenuation(%d) event, media(%d)\n", (int)param, curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					MEDIA_RX_LNATTNAVG(curMedia) = param;
					if(MEDIA_IN_SHOWTIME(otherMedia)  && (MEDIA_RX_LNATTNAVG(otherMedia) <= MEDIA_RX_LNATTNAVG(curMedia))) {
						MEDIA_SEARCHED(curMedia) = TRUE;
						MEDIA_SEARCHTIME(curMedia)=0;
#ifdef CONFIG_BCM963268
						if(!ADSL_PHY_SUPPORT(kAdslPhyBonding))
							BcmXdslCoreMediaSearchNext();
#endif
					}
				}
					break;
				case MEDIASEARCH_LINKUP_E:
					/* The TIMER event will do the next search */
					sprintf(str, "XdslMediaSearch: received linkup event, media(%d)\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					MEDIA_IN_SHOWTIME(curMedia) = TRUE;
					MEDIA_SEARCHED(curMedia) = TRUE;
					MEDIA_SEARCHTIME(curMedia)=0;
					break;
				case MEDIASEARCH_LINKDOWN_E:
					sprintf(str, "XdslMediaSearch: received linkdown event, media(%d)\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					MEDIA_IN_SHOWTIME(curMedia) = FALSE;
					MEDIA_SIGNAL_PRESENT(curMedia) = FALSE;
					MEDIA_SEARCHTIME(curMedia) = 0;
					break;
				default:
					break;
			}
			break;
		case MEDIASEARCH_RESTARTPHY_S:
			switch(mediaSearchEvent) {
#ifdef CONFIG_BCM963268
				case MEDIASEARCH_TIMER_E:
					if(!ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
						sprintf(str, "XdslMediaSearch: restart PHY, media(%d)\n", curMedia);
						DiagWriteString(0, DIAG_DSL_CLIENT, str);
						BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
						xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
						BcmXdslCoreMediaSearchReStartPhy();
					}
					break;
#endif
				default:
					break;
			}
			break;
		case MEDIASEARCH_DONE_S:
			switch(mediaSearchEvent) {
				case MEDIASEARCH_START_E:
					// Either from a PHY switch or Diags download or new media search which require PHY image reload
					sprintf(str, "XdslMediaSearch: DONE_S received start event, media(%d)\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					break;
#ifdef CONFIG_BCM963268
				case MEDIASEARCH_TIMER_E:
					if((AC_FALSE == adslCoreStarted) || ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
						BcmCoreDpcSyncExit();
						return;
					}
					if(!MEDIA_IN_SHOWTIME(curMedia)) {
						MEDIA_SEARCHTIME(curMedia)++;
						if( (!MEDIA_SIGNAL_PRESENT(curMedia) && MEDIA_SIGNALDETECT_TIMEOUT(curMedia)) ||
							MEDIA_TRAIN_TIMEOUT(curMedia) ) {
							/* preferred media has problem connecting, redo search */
							xdslMedia nextMediaType = (curMedia==DSL_LINE_INNERPAIR)? DSL_LINE_OUTERPAIR: DSL_LINE_INNERPAIR;
							sprintf(str, "XdslMediaSearch: preferred media(%d) %s timeout(%d secs), redo search\n",\
								curMedia, (!MEDIA_SIGNAL_PRESENT(curMedia)) ? "detect signal": "train",\
								(int)MEDIA_SEARCHTIME(curMedia));
							DiagWriteString(0, DIAG_DSL_CLIENT, str);
							BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
							MEDIA_SEARCHED(curMedia) = TRUE;
							MEDIA_SEARCHED(nextMediaType) = FALSE;
							if(BcmXdslCoreMediaSearchClearPreferredMedia())
								BcmXdsl_RequestSavePreferredLine();
							xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
							BcmXdslCoreMediaSearchNext();
						}
					}
					break;
#endif
				case MEDIASEARCH_SIGNAL_E:
					if(!MEDIA_SIGNAL_PRESENT(curMedia)) {
						sprintf(str, "XdslMediaSearch: preferred media(%d) received signal event\n", curMedia);
						DiagWriteString(0, DIAG_DSL_CLIENT, str);
						BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					}
					MEDIA_SIGNAL_PRESENT(curMedia) = TRUE;
					break;
				case MEDIASEARCH_RXLNATTNAVG_E:
					sprintf(str, "XdslMediaSearch: preferred media(%d) received rx attenuation(%d) event\n", curMedia, (int)param);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
#ifdef CONFIG_BCM963268
					if(!ADSL_PHY_SUPPORT(kAdslPhyBonding) && ((ulong)-1 != MEDIA_RX_LNATTNAVG(curMedia)) && (param > (MEDIA_RX_LNATTNAVG(curMedia) + 640))) {
						/* retrain attenuation larger than 2.5dB, redo media search */
						sprintf(str, "XdslMediaSearch: preferred media(%d) rx atten is 2.5dB larger then previous connection(%d > %d), redo search\n",\
							curMedia, (int)param, (int)MEDIA_RX_LNATTNAVG(curMedia));
						DiagWriteString(0, DIAG_DSL_CLIENT, str);
						BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
						if(BcmXdslCoreMediaSearchClearPreferredMedia())
							BcmXdsl_RequestSavePreferredLine();
						BcmXdslCoreMediaSearchInit();
						xdslSearchMediaCfg.searchState = MEDIASEARCH_RESTARTPHY_S;
					}
#endif
					break;
				case MEDIASEARCH_LINKUP_E:
					/* Reset searchTime in case there will be retrain, the MEDIA_TRAIN_TIMEOUT will be monitored correctly to redo search */
					sprintf(str, "XdslMediaSearch: preferred media(%d) received linkup event\n", curMedia);
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					MEDIA_IN_SHOWTIME(curMedia) = TRUE;
					MEDIA_SEARCHED(curMedia) = TRUE;
					MEDIA_SEARCHTIME(curMedia)=0;
#ifdef CONFIG_BCM963268
					if(!ADSL_PHY_SUPPORT(kAdslPhyBonding))
						BcmXdslCoreMediaSearchSetPreferredMedia();
#endif
					break;
				case MEDIASEARCH_LINKDOWN_E:
#ifndef CONFIG_BCM963268
					sprintf(str, "XdslMediaSearch: %s(%d) received linkdown event\n", (xdslSearchMediaCfg.mediaPreferred==curMedia)? "preferred media":"media", curMedia);
					if(xdslSearchMediaCfg.mediaPreferred==curMedia)
						xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
#else
					sprintf(str, "XdslMediaSearch: %s(%d) received linkdown event\n", !ADSL_PHY_SUPPORT(kAdslPhyBonding)? "preferred media": "media", curMedia);
					if(ADSL_PHY_SUPPORT(kAdslPhyBonding) && !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDTYPE_FOUND))
						xdslSearchMediaCfg.searchState = MEDIASEARCH_INPROGRESS_S;
#endif
					DiagWriteString(0, DIAG_DSL_CLIENT, str);
					BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
					MEDIA_IN_SHOWTIME(curMedia) = FALSE;	/* Will redo search if no signal/retrain timeout */
					MEDIA_SIGNAL_PRESENT(curMedia) = FALSE;
					MEDIA_SEARCHTIME(curMedia) = 0;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	BcmCoreDpcSyncExit();
}

void BcmXdslCoreMediaSearchReStartPhy(void)
{
	char str[64];
	if (!in_softirq()) {
		sprintf(str, "XdslMediaSearch: State(%d) reset XdslCore\n", xdslSearchMediaCfg.searchState);
		DiagWriteString(0, DIAG_DSL_CLIENT, str);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
		BcmAdslCoreReset(DIAG_DATA_EYE);
	}
	else {
		adslCoreResetPending = AC_TRUE;
		BcmXdsl_RequestIoCtlCallBack();
		sprintf(str, "XdslMediaSearch: State(%d) reset XdslCore pending\n", xdslSearchMediaCfg.searchState);
		DiagWriteString(0, DIAG_DSL_CLIENT, str);
		BCMOS_EVENT_LOG((KERN_CRIT TEXT("%s"), str));
	}
}

AC_BOOL BcmXdslCoreProcessMediaSearchCfgCmd(ulong mediaSrchCfg, AC_BOOL phySwitchOnly)
{
	int newVal, curVal;
	AC_BOOL phySwitchAllowed;
	AC_BOOL saveCfg = AC_FALSE, resetPhy = AC_FALSE;
	
	BcmCoreDpcSyncEnter();
	if(AC_TRUE == phySwitchOnly)
		bcmConfiguredImageType = mediaSrchCfg & BCM_IMAGETYPE_MSK;
	
	newVal = (mediaSrchCfg & MS_SWITCH_SEARCH_MSK)  >> MS_SWITCH_SEARCH_SHIFT;
	curVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCH_SEARCH_MSK) >> BCM_SWITCH_SEARCH_SHIFT;
	if ((newVal != curVal) && (!phySwitchOnly)) {
		adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_SWITCH_SEARCH_MSK) |
			(newVal << BCM_SWITCH_SEARCH_SHIFT);
	}
#if 0 && (defined(CONFIG_BCM963268) || defined(CONFIG_BCM_DSL_GFAST))
	newVal = (mediaSrchCfg & MS_SAVE_PREFERMEDIA_MSK) >> MS_SAVE_PREFERMEDIA_SHIFT;
	curVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERMEDIA_MSK) >> BCM_SAVEPREFERMEDIA_SHIFT;
	if ((newVal != curVal) && (!phySwitchOnly)) {
		if(BCM_SAVEPREFERMEDIA_DISABLED == (newVal << BCM_SAVEPREFERMEDIA_SHIFT))
			BcmXdslCoreMediaSearchClearPreferredMedia();
		adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_SAVEPREFERMEDIA_MSK) |
			(newVal << BCM_SAVEPREFERMEDIA_SHIFT);
	}
#endif
	if( mediaSrchCfg & MS_USE_NEW_CFG ) {
		newVal = (mediaSrchCfg & MS_USE_AFE_MSK) >> MS_USE_AFE_SHIFT;
		curVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIATYPE_MSK) >> BCM_MEDIATYPE_SHIFT;
		
		if ((newVal != curVal) && (!phySwitchOnly)) {
			adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_MEDIATYPE_MSK) |
				(newVal << BCM_MEDIATYPE_SHIFT);
			resetPhy = AC_TRUE;	/* Changing AFE path requires reset/reload PHY */
		}
		
		newVal = (mediaSrchCfg & MS_USE_IMG_MSK) >> MS_USE_IMG_SHIFT;
		curVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK) >> BCM_IMAGETYPE_SHIFT;
		phySwitchAllowed = !(phySwitchOnly && (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED));
		
		if ((newVal != curVal) && phySwitchAllowed) {
			adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_IMAGETYPE_MSK) |
				(newVal << BCM_IMAGETYPE_SHIFT);
#ifndef CONFIG_BCM963268
			resetPhy = AC_TRUE;	/* 63138/63148 */
#endif
		}
#if defined(CONFIG_BCM963268) || (defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO))
#ifdef CONFIG_BCM_DSL_GFAST
		newVal = (mediaSrchCfg & PHY_TYPE_MSK) >> PHY_TYPE_SHIFT;
		curVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PHYTYPE_MSK) >> BCM_PHYTYPE_SHIFT;
		if ((newVal != curVal) && phySwitchAllowed) {
			adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_PHYTYPE_MSK) | (newVal << BCM_PHYTYPE_SHIFT);
			resetPhy = AC_TRUE;
		}
#endif
		if (phySwitchAllowed && (XdslCoreGetPhyImageType() != newVal)) {
			XdslCoreSetPhyImageType(newVal);
			resetPhy = AC_TRUE;
		}
		if(!phySwitchOnly) {
			saveCfg |= BcmXdslCoreMediaSearchClearPreferredMedia();
			resetPhy = TRUE;
		}
#ifdef CONFIG_BCM963268
		else if(BCM_IMAGETYPE_BONDING == XdslCoreGetPhyImageType())
			saveCfg |= BcmXdslCoreMediaSearchClearPreferredMedia();
#endif
#endif
		if( TRUE == resetPhy )
			BcmXdslCoreMediaSearchInit();
	}

	adslCoreCfgProfile[1].xdslMiscCfgParam = adslCoreCfgProfile[0].xdslMiscCfgParam;
	BcmCoreDpcSyncExit();
	
	if(saveCfg)
		BcmXdsl_RequestSavePreferredLine();
	
	return resetPhy;
}

#endif

#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
void BcmAdslDiagEnablePrintStatCmd(void)
{
	dslCommandStruct	cmd;

	cmd.command = kDslDiagFrameHdrCmd;
	cmd.param.dslStatusBufSpec.pBuf = 0;
	cmd.param.dslStatusBufSpec.bufSize = 0;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslDiagDisablePrintStatCmd(void)
{
	dslCommandStruct	cmd;
	cmd.command = kDslDiagSetupCmd;
	cmd.param.dslDiagSpec.setup = 0;
	cmd.param.dslDiagSpec.eyeConstIndex1 = 0;
	cmd.param.dslDiagSpec.eyeConstIndex2 = 0;
	cmd.param.dslDiagSpec.logTime = 0;
	BcmCoreCommandHandlerSync(&cmd);

}
#endif

BCMADSL_STATUS BcmAdslDiagStatSaveInit(void *pAddr, int len)
{
	if((NULL == pAddr) || (len < DIAG_MIN_BUF_SIZE)) {
#ifdef SAVE_STAT_LOCAL_DBG
		AdslDrvPrintf(0, "%: Failed pAddr=%X len=%d\n", __FUNCTION__, (ulong)pAddr, len);
#endif
		return BCMADSL_STATUS_ERROR;
	}
	
	BcmCoreDpcSyncEnter();
	statHdr.statSign = BCM_STATHDR_SWAP32(STAT_REC_HEADER);
	memset((void*)&gSaveStatCtrl, 0, sizeof(gSaveStatCtrl));
	gSaveStatCtrl.pBuf = pAddr;
	gSaveStatCtrl.bufSize = len;
	gSaveStatCtrl.saveStatStarted = AC_FALSE;
	gSaveStatCtrl.saveStatContinous = AC_FALSE;
	gSaveStatCtrl.saveStatDisableOnRetrain = AC_FALSE;
	gSaveStatCtrl.savedStatInfo[0].pAddr = pAddr;
	gSaveStatCtrl.savedStatInfo[0].maxLen = len >> 1;
	gSaveStatCtrl.savedStatInfo[1].pAddr = (void *)((int)pAddr + (len >> 1));
	gSaveStatCtrl.savedStatInfo[1].maxLen = len >> 1;
	gSaveStatCtrl.pCurrentSavedStatInfo = &gSaveStatCtrl.savedStatInfo[0];
	gSaveStatCtrl.pCurrentReadStatInfo = &gSaveStatCtrl.savedStatInfo[0];
	gSaveStatCtrl.saveStatInitialized = AC_TRUE;
	BcmCoreDpcSyncExit();
	
	return BCMADSL_STATUS_SUCCESS;
}

BCMADSL_STATUS BcmAdslDiagStatSaveContinous(void)
{
	if((AC_TRUE == gSaveStatCtrl.saveStatInitialized) && (AC_FALSE == gSaveStatCtrl.saveStatStarted) ) {
		gSaveStatCtrl.saveStatContinous = AC_TRUE;
		return BCMADSL_STATUS_SUCCESS;
	}
	
	return BCMADSL_STATUS_ERROR;
}

BCMADSL_STATUS BcmAdslDiagStatSaveStart(void)
{
	adslVersionInfo	verInfo;
	
	if(AC_TRUE != gSaveStatCtrl.saveStatInitialized)
		return BCMADSL_STATUS_ERROR;
	
#ifdef __KERNEL__
	BcmXdslCoreDiagProcFileCreate();
#endif

	gSaveStatCtrl.saveStatStarted = AC_TRUE;
#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
#ifndef SAVE_STAT_LOCAL_AUTOSTART
	BcmAdslDiagEnablePrintStatCmd();
#endif
#endif
	BcmAdslCoreGetVersion(&verInfo);
	BcmAdslCoreDiagSaveStatusString("ADSL version info: PHY=%s, Drv=%s",
		verInfo.phyVerStr, verInfo.drvVerStr);
	
	return BCMADSL_STATUS_SUCCESS;
}

BCMADSL_STATUS BcmAdslDiagStatSaveStop(void)
{
#ifdef SAVE_STAT_LOCAL_DBG
	AdslDrvPrintf("**%s:**\n", __FUNCTION__);
#endif
	gSaveStatCtrl.saveStatStarted = AC_FALSE;
#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
#ifndef SAVE_STAT_LOCAL_AUTOSTART
	BcmAdslDiagDisablePrintStatCmd();
#endif
#endif
	return BCMADSL_STATUS_SUCCESS;
}

BCMADSL_STATUS BcmAdslDiagStatSaveUnInit(void)
{
	gSaveStatCtrl.saveStatStarted = AC_FALSE;
	gSaveStatCtrl.saveStatInitialized = AC_FALSE;
	gSaveStatCtrl.saveStatContinous = AC_FALSE;
	gSaveStatCtrl.saveStatDisableOnRetrain = AC_FALSE;
#ifdef __KERNEL__
	BcmXdslCoreDiagProcFileRemove();
#endif
	return BCMADSL_STATUS_SUCCESS;
}

BCMADSL_STATUS BcmAdslDiagStatSaveGet(PADSL_SAVEDSTATUS_INFO pSavedStatInfo)
{
	xdslSavedStatInfo *p, *p1;
	
	if(AC_TRUE != gSaveStatCtrl.saveStatInitialized) {
#ifdef SAVE_STAT_LOCAL_DBG
		AdslDrvPrintf("TEXT(%s: gSaveStatCtrl has not been initialized"), __FUNCTION__);
#endif
		return BCMADSL_STATUS_ERROR;
	}
	
	BcmCoreDpcSyncEnter();
#if 1
	BcmAdslCoreDiagWriteStatusString(0, "pCurAddr=%x pAddr0=%x len0=%d nStatus0=%d, pAddr1=%x len1=%d nStatus1=%d\n",
		(uint)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr,
		(uint)gSaveStatCtrl.savedStatInfo[0].pAddr,
		gSaveStatCtrl.savedStatInfo[0].len,
		gSaveStatCtrl.savedStatInfo[0].nStatus,
		(uint)gSaveStatCtrl.savedStatInfo[1].pAddr,
		gSaveStatCtrl.savedStatInfo[1].len,
		gSaveStatCtrl.savedStatInfo[1].nStatus);
	AdslDrvPrintf(TEXT("pCurAddr=%x pAddr0=%x len0=%d nStatus0=%d, pAddr1=%x len1=%d nStatus1=%d\n"),
		(uint)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr,
		(uint)gSaveStatCtrl.savedStatInfo[0].pAddr,
		gSaveStatCtrl.savedStatInfo[0].len,
		gSaveStatCtrl.savedStatInfo[0].nStatus,
		(uint)gSaveStatCtrl.savedStatInfo[1].pAddr,
		gSaveStatCtrl.savedStatInfo[1].len,
		gSaveStatCtrl.savedStatInfo[1].nStatus);
#endif
	
	if(gSaveStatCtrl.pCurrentSavedStatInfo == &gSaveStatCtrl.savedStatInfo[1]) {
		p = &gSaveStatCtrl.savedStatInfo[0];
		p1 = &gSaveStatCtrl.savedStatInfo[1];
	}
	else {
		if(gSaveStatCtrl.savedStatInfo[1].len > 0) {
			p = &gSaveStatCtrl.savedStatInfo[1];
			p1= &gSaveStatCtrl.savedStatInfo[0];
		}
		else {
			p = &gSaveStatCtrl.savedStatInfo[0];
			p1= &gSaveStatCtrl.savedStatInfo[1];
		}
	}
	pSavedStatInfo->nStatus = p->nStatus;
	pSavedStatInfo->pAddr = p->pAddr;
	pSavedStatInfo->len = p->len;
	pSavedStatInfo->nStatus += p1->nStatus;
	pSavedStatInfo->pAddr1 = p1->pAddr;
	pSavedStatInfo->len1 = p1->len;
	BcmCoreDpcSyncExit();
	
	return BCMADSL_STATUS_SUCCESS;
}

int BcmXdslDiagStatSaveLocalRead(void *pBuf, int bufLen)
{
	int len=0;
	
	if(AC_TRUE != gSaveStatCtrl.saveStatInitialized) {
#ifdef SAVE_STAT_LOCAL_DBG
		AdslDrvPrintf(TEXT("%s: gSaveStatCtrl has not been initialized"), __FUNCTION__);
#endif
		return 0;
	}
	
	BcmCoreDpcSyncEnter();
	if(gSaveStatCtrl.pCurrentReadStatInfo->len != gSaveStatCtrl.pCurrentReadStatInfo->readLen)
		len = gSaveStatCtrl.pCurrentReadStatInfo->len -gSaveStatCtrl.pCurrentReadStatInfo->readLen;
	else if(gSaveStatCtrl.pCurrentReadStatInfo != gSaveStatCtrl.pCurrentSavedStatInfo) {
		gSaveStatCtrl.pCurrentReadStatInfo = gSaveStatCtrl.pCurrentSavedStatInfo;
		len = gSaveStatCtrl.pCurrentReadStatInfo->len -gSaveStatCtrl.pCurrentReadStatInfo->readLen;
	}
	
	if(len > bufLen)
		len = bufLen;
	
	if(len) {
		memcpy(pBuf, (void *)((int)gSaveStatCtrl.pCurrentReadStatInfo->pAddr+gSaveStatCtrl.pCurrentReadStatInfo->readLen), len);
		gSaveStatCtrl.pCurrentReadStatInfo->readLen += len;
	}
	BcmCoreDpcSyncExit();
	
	return len;
}

void BcmXdslCoreDiagStatSaveDisableOnRetrainSet(void)
{
	gSaveStatCtrl.saveStatDisableOnRetrain = TRUE;
}

Bool BcmXdslDiagStatSaveLocalIsActive(void)
{
	return( AC_TRUE == gSaveStatCtrl.saveStatStarted );
}

#define	BUF_SPACE_AVAILABLE(x)	((x).pCurrentSavedStatInfo->maxLen - (x).pCurrentSavedStatInfo->len)

#ifdef SAVE_STAT_LOCAL_DBG
LOCAL void BcmXdslDiagPrintStatSaveLocalInfo(int switchToBufNum, int curStatLen)
{
	int prevBufNum = (1 == switchToBufNum) ? 0: 1;
	
	AdslDrvPrintf("**Switch to buf%d: nStatus%d=%d len%d=%d, nStatus%d=%d len%d=%d buf%dSpaceAvail=%d, curStatLen=%d**\n",
		switchToBufNum, switchToBufNum, gSaveStatCtrl.pCurrentSavedStatInfo->nStatus,
		switchToBufNum, gSaveStatCtrl.pCurrentSavedStatInfo->len,
		prevBufNum, gSaveStatCtrl.savedStatInfo[prevBufNum].nStatus,
		prevBufNum, gSaveStatCtrl.savedStatInfo[prevBufNum].len,
		prevBufNum, gSaveStatCtrl.savedStatInfo[prevBufNum].maxLen - gSaveStatCtrl.savedStatInfo[prevBufNum].len,
		curStatLen);
}
#endif

void BcmXdslDiagStatSaveLocal(ulong cmd, char *statStr, long n, char *p1, long n1)
{
#ifdef SUPPORT_DSL_BONDING
	ulong lineId;
#endif
	ulong clientType;
	ulong statId;
	int len = (int)(n+n1);

	if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (len + sizeof(statHdr)) ) {
		if(gSaveStatCtrl.pCurrentSavedStatInfo == &gSaveStatCtrl.savedStatInfo[0]) {
			gSaveStatCtrl.pCurrentSavedStatInfo = &gSaveStatCtrl.savedStatInfo[1];
			if(gSaveStatCtrl.pCurrentReadStatInfo != &gSaveStatCtrl.savedStatInfo[0])
				gSaveStatCtrl.pCurrentReadStatInfo = &gSaveStatCtrl.savedStatInfo[0];
#ifdef SAVE_STAT_LOCAL_DBG
			BcmXdslDiagPrintStatSaveLocalInfo(1, len);
#endif
			gSaveStatCtrl.pCurrentSavedStatInfo->nStatus = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->len = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->readLen = 0;
			if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (len + sizeof(statHdr)) )
				return;
		}
		else {
			/* The second buffer is full */
			if(AC_FALSE == gSaveStatCtrl.saveStatContinous)
				return;
			
			gSaveStatCtrl.pCurrentSavedStatInfo = &gSaveStatCtrl.savedStatInfo[0];
			if(gSaveStatCtrl.pCurrentReadStatInfo != &gSaveStatCtrl.savedStatInfo[1])
				gSaveStatCtrl.pCurrentReadStatInfo = &gSaveStatCtrl.savedStatInfo[1];
#ifdef SAVE_STAT_LOCAL_DBG
			BcmXdslDiagPrintStatSaveLocalInfo(0, len);
#endif
			gSaveStatCtrl.pCurrentSavedStatInfo->nStatus = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->len = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->readLen = 0;
			if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (len + sizeof(statHdr)) )
				return;
		}
	}
#ifdef SUPPORT_DSL_BONDING
	lineId = (cmd & DIAG_LINE_MASK) >> DIAG_LINE_SHIFT;
	if ( statusInfoData == (cmd & ~(DIAG_LINE_MASK | DIAG_TYPE_MASK)) )
		statId = (DIAG_BINARY_STATUS | (lineId << DIAG_STATTYPE_LINEID_SHIFT)) << 16;
	else
		statId = (DIAG_STRING_STATUS | (lineId << DIAG_STATTYPE_LINEID_SHIFT)) << 16;
#else
	if ( statusInfoData == (cmd & ~DIAG_TYPE_MASK))
		statId = DIAG_BINARY_STATUS << 16;
	else
		statId = DIAG_STRING_STATUS << 16;
#endif
	clientType = (cmd & DIAG_TYPE_MASK) >> DIAG_TYPE_SHIFT;
	statId |= (clientType << DIAG_STATTYPE_CLIENTTYPE_SHIFT) << 16;

	statHdr.statId = statId | (len & 0xFFFF);
	statHdr.statId = BCM_STATHDR_SWAP32(statHdr.statId);
	
	memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
		(void*)&statHdr, sizeof(statHdr));
	gSaveStatCtrl.pCurrentSavedStatInfo->len += sizeof(statHdr);
	
	memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
		(void*)statStr, n);
	gSaveStatCtrl.pCurrentSavedStatInfo->len += n;
	
	if( NULL != p1) {
		memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
			(void*)p1, n1);
		gSaveStatCtrl.pCurrentSavedStatInfo->len += n1;
	}
	gSaveStatCtrl.pCurrentSavedStatInfo->nStatus++;
}

/* NOTE: Call to BcmXdslDiagFirstSplitStatSaveLocal(), BcmXdslDiagContSplitStatSaveLocal(), and BcmXdslDiagLastSplitStatSaveLocal()
* need to be protected if not from the bottom half context
*/
LOCAL void BcmXdslDiagFirstSplitStatSaveLocal(ulong cmd, int totalLen, void *statStr, int n, void *p1, int n1)
{
#ifdef SUPPORT_DSL_BONDING
	ulong lineId;
#endif
	ulong statId;
	ulong clientType;

	if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (totalLen + sizeof(statHdr)) ) {
		if(gSaveStatCtrl.pCurrentSavedStatInfo == &gSaveStatCtrl.savedStatInfo[0]) {
			gSaveStatCtrl.pCurrentSavedStatInfo = &gSaveStatCtrl.savedStatInfo[1];
			if(gSaveStatCtrl.pCurrentReadStatInfo != &gSaveStatCtrl.savedStatInfo[0])
				gSaveStatCtrl.pCurrentReadStatInfo = &gSaveStatCtrl.savedStatInfo[0];
#ifdef SAVE_STAT_LOCAL_DBG
			BcmXdslDiagPrintStatSaveLocalInfo(1, totalLen);
#endif
			gSaveStatCtrl.pCurrentSavedStatInfo->nStatus = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->len = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->readLen = 0;
			if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (totalLen + sizeof(statHdr)) ) {
				gSaveStatCtrl.curSplitStatEndLen = 0;
				return;
			}
		}
		else {
			/* The second buffer is full */
			if(AC_FALSE == gSaveStatCtrl.saveStatContinous) {
				gSaveStatCtrl.curSplitStatEndLen = 0;
				return;
			}
			
			gSaveStatCtrl.pCurrentSavedStatInfo = &gSaveStatCtrl.savedStatInfo[0];
			if(gSaveStatCtrl.pCurrentReadStatInfo != &gSaveStatCtrl.savedStatInfo[1])
				gSaveStatCtrl.pCurrentReadStatInfo = &gSaveStatCtrl.savedStatInfo[1];
#ifdef SAVE_STAT_LOCAL_DBG
			BcmXdslDiagPrintStatSaveLocalInfo(0, totalLen);
#endif
			gSaveStatCtrl.pCurrentSavedStatInfo->nStatus = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->len = 0;
			gSaveStatCtrl.pCurrentSavedStatInfo->readLen = 0;
			if( BUF_SPACE_AVAILABLE(gSaveStatCtrl) < (totalLen + sizeof(statHdr)) ) {
				gSaveStatCtrl.curSplitStatEndLen = 0;
				return;
			}
		}
	}
#ifdef SUPPORT_DSL_BONDING
	lineId = (cmd & DIAG_LINE_MASK) >> DIAG_LINE_SHIFT;
	if ( statusInfoData == (cmd & ~(DIAG_LINE_MASK | DIAG_TYPE_MASK)) )
		statId = (DIAG_BINARY_STATUS | (lineId << DIAG_STATTYPE_LINEID_SHIFT)) << 16;
	else
		statId = (DIAG_STRING_STATUS | (lineId << DIAG_STATTYPE_LINEID_SHIFT)) << 16;
#else
	if ( statusInfoData == (cmd & ~DIAG_TYPE_MASK))
		statId = DIAG_BINARY_STATUS << 16;
	else
		statId = DIAG_STRING_STATUS << 16;
#endif
	clientType = (cmd & DIAG_TYPE_MASK) >> DIAG_TYPE_SHIFT;
	statId |= (clientType << DIAG_STATTYPE_CLIENTTYPE_SHIFT) << 16;

	gSaveStatCtrl.curSplitStatEndLen = gSaveStatCtrl.pCurrentSavedStatInfo->len + sizeof(statHdr) + totalLen;
	statHdr.statId = statId | (totalLen & 0xFFFF);
	statHdr.statId = BCM_STATHDR_SWAP32(statHdr.statId);
	
	memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
		(void*)&statHdr, sizeof(statHdr));
	gSaveStatCtrl.pCurrentSavedStatInfo->len += sizeof(statHdr);
	
	memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
		statStr, n);
	gSaveStatCtrl.pCurrentSavedStatInfo->len += n;
	
	if( NULL != p1) {
		memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
			p1, n1);
		gSaveStatCtrl.pCurrentSavedStatInfo->len += n1;
	}
	gSaveStatCtrl.pCurrentSavedStatInfo->nStatus++;
}

LOCAL void BcmXdslDiagContSplitStatSaveLocal(void *p, int n)
{
	if(gSaveStatCtrl.curSplitStatEndLen) {
		memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
			p, n);
		gSaveStatCtrl.pCurrentSavedStatInfo->len += n;
	}
}

LOCAL void BcmXdslDiagLastSplitStatSaveLocal(void *p, int n)
{
	if(gSaveStatCtrl.curSplitStatEndLen) {
		memcpy((void*)((int)gSaveStatCtrl.pCurrentSavedStatInfo->pAddr+gSaveStatCtrl.pCurrentSavedStatInfo->len),
			p, n);
		gSaveStatCtrl.pCurrentSavedStatInfo->len += n;
		if(gSaveStatCtrl.curSplitStatEndLen != gSaveStatCtrl.pCurrentSavedStatInfo->len) {
#ifdef SAVE_STAT_LOCAL_DBG
			AdslDrvPrintf("*** %s: curSplitStatEndLen=%d pCurrentSavedStatInfo->len=%d lastStatLen=%d ***\n",
				__FUNCTION__, gSaveStatCtrl.curSplitStatEndLen,
				gSaveStatCtrl.pCurrentSavedStatInfo->len, n);
#endif
			gSaveStatCtrl.pCurrentSavedStatInfo->len = gSaveStatCtrl.curSplitStatEndLen;
		}
		gSaveStatCtrl.curSplitStatEndLen = 0;
	}
}

#ifdef ADSLDRV_STATUS_PROFILING
void BcmXdslCoreDrvProfileInfoClear(void)
{
	BcmCoreDpcSyncEnter();	/* Should disable interrupt b/w the first 2 lines, but it's OK for experimenting */
	adslCoreIntrCnt=intrDurTotal=intrDurMax=intrDurMaxAtIntrCnt=intrDuringSchedCnt=0;
	intrDurMin=(ulong)-1;
	adslCoreIsrTaskCnt=dpcDurTotal=dpcDurMax=dpcDurMaxAtDpcCnt=dpcDurMaxAtByteAvail=0;
	dpcDurMin=(ulong)-1;
	dpcDelayTotal=dpcDelayMax=dpcDelayMaxAtDpcCnt=dpcDelayMaxAtByteAvail=0;
	dpcDelayMin=(ulong)-1;
	byteAvailMax=byteAvailMaxAtDpcCnt=byteAvailMaxAtDpcDur=byteAvailMaxAtDpcDelay=byteAvailMaxAtNumStatProc=byteAvailMaxAtByteProc=0;
	byteProcMax=byteProcMaxAtDpcCnt=byteProcMaxAtDpcDur=byteProcMaxAtDpcDelay=byteProcMaxAtNumStatProc=0;
	nStatProcMax=nStatProcMaxAtDpcCnt=nStatProcMaxAtByteAvail=nStatProcMaxAtByteProc=0;
	gByteAvail=gByteProc=gBkupStartAtIntrCnt=gBkupStartAtdpcCnt=0;
	BcmCoreDpcSyncExit();
}

void BcmXdslCoreDrvProfileInfoPrint(void)
{
	OS_TICKS	ticks;
	int	tMs;
	
	if(!adslCoreIntrCnt || !adslCoreIsrTaskCnt)
		return;
	
	bcmOsGetTime(&ticks);
	tMs = (ticks - printTicks) * BCMOS_MSEC_PER_TICK;
	
	BcmAdslCoreDiagWriteStatusString(0,
		"DrvProfileInfoPrintInterval=%lums, syncTime=%lusec: intrCnt=%lu, durShedIntrCnt=%lu dpcCnt=%lu\n"
		"  intrDurMin=%lu, intrDurAvg=%lu, intrDurMax=%lu at intrCnt=%lu\n"
		"  dpcDurMin=%lu, dpcDurAvg=%lu, dpcDurMax=%lu at dpcCnt=%lu byteAvail=%d\n"
		"  dpcDelayMin=%lu, dpcDelayAvg=%lu, dpcDelayMax=%lu at dpcCnt=%ld byteAvail=%d\n"
		"  byteAvailMax=%d at dpcCnt=%lu dpcDuration=%lu dpcDelay=%lu nStatProc=%d byteProc=%d\n"
		"  byteProcMax =%d at dpcCnt=%lu dpcDuration=%lu dpcDelay=%lu nStatProc=%d\n"
		"  nStatProcMax=%d at dpcCnt=%lu byteAvail=%d byteProc=%d\n"
		"  last bkup at: intrCnt=%lu dpcCnt=%lu, snapshot of: byteAvail=%d byteProc=%d\n",
		tMs, BcmAdslCoreDiagGetSyncTime(), adslCoreIntrCnt, intrDuringSchedCnt, adslCoreIsrTaskCnt,
		intrDurMin, intrDurTotal/adslCoreIntrCnt, intrDurMax, intrDurMaxAtIntrCnt,
		dpcDurMin, dpcDurTotal/adslCoreIsrTaskCnt, dpcDurMax, dpcDurMaxAtDpcCnt, dpcDurMaxAtByteAvail,
		dpcDelayMin, dpcDelayTotal/adslCoreIsrTaskCnt, dpcDelayMax, dpcDelayMaxAtDpcCnt, dpcDelayMaxAtByteAvail,
		byteAvailMax, byteAvailMaxAtDpcCnt, byteAvailMaxAtDpcDur, byteAvailMaxAtDpcDelay, byteAvailMaxAtNumStatProc, byteAvailMaxAtByteProc,
		byteProcMax, byteProcMaxAtDpcCnt, byteProcMaxAtDpcDur, byteProcMaxAtDpcDelay, byteProcMaxAtNumStatProc,
		nStatProcMax, nStatProcMaxAtDpcCnt, nStatProcMaxAtByteAvail, nStatProcMaxAtByteProc,
		gBkupStartAtIntrCnt, gBkupStartAtdpcCnt, gByteAvail, gByteProc);
	
	printTicks=ticks;
}
#endif

#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
#ifdef _NOOS
int BcmXdslCoreGetAfeBoardId(unsigned int *pAfeIds)
{
	return BP_SUCCESS;
}
#else
Boolean IsValidAfeId(ulong afeId)
{
	ulong  n;

	if (0 == afeId)
		return false;

	n = (afeId & AFE_CHIP_MASK) >> AFE_CHIP_SHIFT;
	if ((0 == n) || (n > AFE_CHIP_MAX))
		return false;
	n = (afeId & AFE_LD_MASK) >> AFE_LD_SHIFT;
	if ((0 == n) || (n > AFE_LD_MAX))
		return false;

	return true;
}

int BcmXdslCoreGetAfeBoardId(unsigned int *pAfeIds)
{
	int				i, res;
	ulong			mibLen;
	adslMibInfo		*pMibInfo;

#ifdef BP_AFE_DEFAULT
	pAfeIds[0] = BP_AFE_DEFAULT;
	pAfeIds[1] = BP_AFE_DEFAULT;
#else
	char	boardIdStr[BP_BOARD_ID_LEN];
	pAfeIds[0] = 0;
	pAfeIds[1] = 0;
#endif

#ifdef BP_AFE_DEFAULT
	res = BpGetDslPhyAfeIds((UINT32 *)pAfeIds);
	if( res  != BP_SUCCESS )
		DiagWriteString(0, DIAG_DSL_CLIENT, "%s: BpGetDslPhyAfeIds() Error(%d)!\n", __FUNCTION__, res);
#ifdef __kerSysGetAfeId
	for (i = 0;i < 2; i++)
		if (IsValidAfeId(nvramAfeId[i]))
			pAfeIds[i] = nvramAfeId[i];
#endif
#else
	res = BpGetBoardId(boardIdStr);
	if( BP_SUCCESS == res ) {
		if( 0 == strcmp("96368MVWG", boardIdStr) ) {
			pAfeIds[0] = (AFE_CHIP_INT << AFE_CHIP_SHIFT) |
						(AFE_LD_6302 << AFE_LD_SHIFT) |
						(AFE_FE_REV_6302_REV1 << AFE_FE_REV_SHIFT);
		}
		else if( (0 == strcmp("96368VVW", boardIdStr)) || (0 == strcmp("96368SV2", boardIdStr)) ) {
			pAfeIds[0] = (AFE_CHIP_INT << AFE_CHIP_SHIFT) |
						(AFE_LD_ISIL1556 << AFE_LD_SHIFT) |
						(AFE_FE_REV_ISIL_REV1 << AFE_FE_REV_SHIFT);
		}
		else
			DiagWriteString(0, DIAG_DSL_CLIENT, "%s: Unknown boardId \"%s\"\n", __FUNCTION__, boardIdStr);
		pAfeIds[0] |= (AFE_FE_ANNEXA << AFE_FE_ANNEX_SHIFT);
	}
	else
		DiagWriteString(0, "%s: BpGetBoardId() Error(%d)!\n", __FUNCTION__, res);
#endif
	for(i = 0; i < MAX_DSL_LINE; i++) {
		mibLen = sizeof(adslMibInfo);
		pMibInfo = (void *) AdslCoreGetObjectValue (i, NULL, 0, NULL, &mibLen);
		pMibInfo->afeId[0] = pAfeIds[0];
		pMibInfo->afeId[1] = pAfeIds[1];
	}
	
	return res;
}
#endif

#ifdef CONFIG_BCM963268
#define AFE_PATH_INTERNAL	0
#define AFE_PATH_EXTERNAL	1

Boolean Is63268AFEPathRemap(int path, unsigned short gpio)
{
	Boolean res = FALSE;
	if(AFE_PATH_INTERNAL == path) {
		switch (gpio & BP_GPIO_NUM_MASK) {
			case (BP_GPIO_12_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_13_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_26_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_27_AH & BP_GPIO_NUM_MASK):
				res = TRUE;
				break;
			default:
				break;
		}
	}
	else
	if(AFE_PATH_EXTERNAL == path) {
		switch (gpio & BP_GPIO_NUM_MASK) {
			case (BP_GPIO_10_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_11_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_24_AH & BP_GPIO_NUM_MASK):
			case (BP_GPIO_25_AH & BP_GPIO_NUM_MASK):
				res = TRUE;
				break;
			default:
				break;
			}
	}
	return res;
}

int GetGpioPhyCode(unsigned short gpio)
{
	int phyCode = -1;
	unsigned short gpioNum = gpio & BP_GPIO_NUM_MASK;

	if ((gpio & ~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_4)
		phyCode = kDslAfeLdPinCtl4;
	else if ((gpio & ~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_5)
		phyCode = kDslAfeLdPinCtl5;
	else if (gpioNum == BP_GPIO_10_AH)
		phyCode = kDslAfeLdPinCtl0; 
	else if (gpioNum == BP_GPIO_11_AH)
		phyCode = kDslAfeLdPinCtl1; 
	else if ((gpioNum == BP_GPIO_8_AH)  ||
			 (gpioNum == BP_GPIO_12_AH) ||
			 (gpioNum == BP_GPIO_26_AH))
		phyCode = kDslAfeLdPinCtl2;
	else if ((gpioNum == BP_GPIO_9_AH)  ||
			 (gpioNum == BP_GPIO_13_AH) ||
			 (gpioNum == BP_GPIO_27_AH))
		phyCode = kDslAfeLdPinCtl3;
	DiagWriteString(0, DIAG_DSL_CLIENT, "GetGpioPhyCode: gpio=%d phyCode=%d\n", gpio, phyCode);
	return phyCode;
}

int GetPhy6303LDCfg(int path)
{
	unsigned short bpGpio;
	int            phyPwr, phyClk, phyData;

	if (AFE_PATH_INTERNAL == path) {
	  //bpGpio = BP_GPIO_27_AH;
	  if ((BP_SUCCESS != BpGetIntAFELDPwrGpio(&bpGpio)) || (-1 == (phyPwr = GetGpioPhyCode(bpGpio))))
		phyPwr = kDslAfeLdPinCtl0;
	  /* no BpGetIntAFELDClkGpio() function use default */
	  phyClk  = kDslAfeLdPinCtl5;
	  phyData = kDslAfeLdPinCtl4;
	}
	else {
	  if ((BP_SUCCESS != BpGetExtAFELDPwrGpio(&bpGpio)) || (-1 == (phyPwr = GetGpioPhyCode(bpGpio))))
		phyPwr = kDslAfeLdPinCtl3;
	  if ((BP_SUCCESS != BpGetExtAFELDClkGpio(&bpGpio)) || (-1 == (phyClk = GetGpioPhyCode(bpGpio))))
		phyClk = kDslAfeLdPinCtl2;
	  if ((BP_SUCCESS != BpGetExtAFELDDataGpio(&bpGpio)) || (-1 == (phyData = GetGpioPhyCode(bpGpio))))
		phyData = kDslAfeLdPinCtl1;
	}

	DiagWriteString(0, DIAG_DSL_CLIENT, "GetPhy6303LDCfg: pwr=%d clk=%d data=%d; cfg=0x%X\n", phyPwr, phyClk, phyData,
		(phyPwr << (16+((kDslAfeLdPinPwrIdx) << 2))) | 
		(phyClk << (16+((kDslAfeLdPinClkIdx) << 2))) | 
		(phyData << (16+((kDslAfeLdPinDataIdx) << 2)))
		);
	return
		(phyPwr << (16+((kDslAfeLdPinPwrIdx) << 2))) | 
		(phyClk << (16+((kDslAfeLdPinClkIdx) << 2))) | 
		(phyData << (16+((kDslAfeLdPinDataIdx) << 2)));
}


Boolean Is63268LDPwrGpioPinSwap(unsigned short gpio)
{
	Boolean res = FALSE;
	switch (gpio & BP_GPIO_NUM_MASK) {
		case (BP_GPIO_8_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_12_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_26_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_10_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_24_AH & BP_GPIO_NUM_MASK):
			res = TRUE;
			break;
		default:
			break;
	}
	return res;
}

Boolean Is63268LDModeGpioPinSwap(unsigned short gpio)
{
	Boolean res = FALSE;
	switch (gpio & BP_GPIO_NUM_MASK) {
		case (BP_GPIO_9_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_13_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_27_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_11_AH & BP_GPIO_NUM_MASK):
		case (BP_GPIO_25_AH & BP_GPIO_NUM_MASK):
			res = TRUE;
			break;
		default:
			break;
	}
	return res;
}

#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
Boolean Is63268DSLCtrlPinUsed(unsigned short gpio)
{
	return(((gpio&~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_4) || ((gpio&~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_5));
}

Boolean Is63268LDPwrDSLCtrlPinSwap(unsigned short gpio)
{
	return((gpio&~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_5);
}

Boolean Is63268LDModeDSLCtrlPinSwap(unsigned short gpio)
{
	return((gpio&~BP_ACTIVE_MASK) == BP_PIN_DSL_CTRL_4);
}
#endif

#endif

#ifdef ADSLDRV_LITTLE_ENDIAN
void AfeDescCopy(afeDescStruct *dst, afeDescStruct *src)
{
	int		i;

	dst->verId = ADSL_ENDIAN_CONV_SHORT(src->verId);
	dst->size  = ADSL_ENDIAN_CONV_SHORT(src->size);
	dst->chipId = ADSL_ENDIAN_CONV_LONG(src->chipId);
	dst->boardAfeId = ADSL_ENDIAN_CONV_LONG(src->boardAfeId);

	dst->afeChidIdConfig0 = ADSL_ENDIAN_CONV_LONG(src->afeChidIdConfig0);
	dst->afeChidIdConfig1 = ADSL_ENDIAN_CONV_LONG(src->afeChidIdConfig1);
	dst->afeTuningControl = ADSL_ENDIAN_CONV_LONG(src->afeTuningControl);

	dst->meanTxGain = ADSL_ENDIAN_CONV_LONG(src->meanTxGain);
	dst->meanRxGain = ADSL_ENDIAN_CONV_LONG(src->meanRxGain);

	for (i = 0; i < MAX_AFE_BANDS; i++) {
	  dst->txToneIndex[i] = ADSL_ENDIAN_CONV_SHORT(src->txToneIndex[i]);
	  dst->rxToneIndex[i] = ADSL_ENDIAN_CONV_SHORT(src->rxToneIndex[i]);
	}
	for (i = 0; i < TX_POWER_MODES; i++) {
	  dst->maxTxPower[i] = ADSL_ENDIAN_CONV_SHORT(src->maxTxPower[i]);
	  dst->maxRxPower[i] = ADSL_ENDIAN_CONV_SHORT(src->maxRxPower[i]);
	}
}
#endif

void BcmXdslCoreSendAfeInfo(int toPhy)
{
	int	i;
	union {
		dslStatusStruct status;
		dslCommandStruct cmd;
	} t;
	
#ifdef CONFIG_BCM963268
	unsigned int	afeConfigExt = 0;
#endif
	unsigned int	afeIds[2];
	afeDescStruct	afeInfo;
#if defined(CONFIG_BCM963268) || defined(USE_PINMUX_DSLCTL_API)
	unsigned short bpGpioPwr, bpGpioMode;
#endif
#ifdef CONFIG_BCM_DSL_GFAST
	int res;
	unsigned short bpGpioAFELDRelay;
#endif

	memset((void*)&afeInfo, 0, sizeof(afeInfo));
	afeInfo.verId = AFE_DESC_VERSION(AFE_DESC_VER_MJ,AFE_DESC_VER_MN);
	afeInfo.size = sizeof(afeDescStruct);
#ifdef CONFIG_BCM963268
	afeInfo.chipId = (PERF->RevID & REV_ID_MASK) 
		| (0x63268 << CHIP_ID_SHIFT); 
#elif defined(CONFIG_BCM963381)
	afeInfo.chipId = PERF->RevID;
	if(0x633810A0 == afeInfo.chipId) {
		char	boardIdStr[BP_BOARD_ID_LEN];
		if( BP_SUCCESS == BpGetBoardId(boardIdStr) ) {
			if((NULL != strstr(boardIdStr, "REF1")) && (NULL == strstr(boardIdStr, "A0")))
				afeInfo.chipId = 0x633810A1;
		}
	}
#else
	afeInfo.chipId = PERF->RevID;
#endif

#if defined(SUPPORT_MULTI_PHY) && !defined(CONFIG_BCM963138) && !defined(CONFIG_BCM963148)
	BcmXdslCoreGetCurrentMedia(&afeIds[0]);
#else
	BcmXdslCoreGetAfeBoardId(&afeIds[0]);
#endif

#ifdef CONFIG_BCM963268
#ifdef BP_GET_INT_AFE_DEFINED
	if ((afeIds[0] & AFE_LD_MASK) == (AFE_LD_6303 << AFE_LD_SHIFT)) {
		afeInfo.afeChidIdConfig0 |= GetPhy6303LDCfg(AFE_PATH_INTERNAL);
		if (BP_SUCCESS == BpGetIntAFELDPwrGpio(&bpGpioPwr))
		  if(BP_ACTIVE_LOW == (bpGpioPwr & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_PWR_POL;
	}
	else if((BP_SUCCESS == BpGetIntAFELDPwrGpio(&bpGpioPwr)) && (BP_SUCCESS == BpGetIntAFELDModeGpio(&bpGpioMode))) {
		if(Is63268AFEPathRemap(AFE_PATH_INTERNAL, bpGpioPwr)) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_PWR_MAP;
		}
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
		else if( ((PERF->RevID & REV_ID_MASK) >= 0xD0) && Is63268DSLCtrlPinUsed(bpGpioPwr) ) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT6302_PAD;
		}
#endif
		if(Is63268AFEPathRemap(AFE_PATH_INTERNAL, bpGpioMode)) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_MODE_MAP;
		}
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
		else if( ((PERF->RevID & REV_ID_MASK) >= 0xD0) && Is63268DSLCtrlPinUsed(bpGpioMode) ) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT6302_PAD;
		}
#endif
		if(Is63268LDPwrGpioPinSwap(bpGpioPwr) || Is63268LDModeGpioPinSwap(bpGpioMode)
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
			|| Is63268LDPwrDSLCtrlPinSwap(bpGpioPwr) || Is63268LDModeDSLCtrlPinSwap(bpGpioMode)
#endif
			)
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_PIN_SWAP;

		if(BP_ACTIVE_LOW == (bpGpioPwr & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_PWR_POL;
		if(BP_ACTIVE_LOW == (bpGpioMode & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_MODE_POL;
	}
#endif /* BP_GET_INT_AFE_DEFINED */
	/* External AFE path */
	if ((afeIds[0] & AFE_LD_MASK) == (AFE_LD_6303 << AFE_LD_SHIFT)) {
		afeConfigExt |= GetPhy6303LDCfg(AFE_PATH_EXTERNAL);
		if (BP_SUCCESS == BpGetExtAFELDPwrGpio(&bpGpioPwr))
		  if(BP_ACTIVE_LOW == (bpGpioPwr & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_PWR_POL;
	}
	else if((BP_SUCCESS == BpGetExtAFELDPwrGpio(&bpGpioPwr)) && (BP_SUCCESS == BpGetExtAFELDModeGpio(&bpGpioMode))) {
		if(Is63268AFEPathRemap(AFE_PATH_EXTERNAL, bpGpioPwr)) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_PWR_MAP;
		}
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
		else if( ((PERF->RevID & REV_ID_MASK) >= 0xD0) && Is63268DSLCtrlPinUsed(bpGpioPwr) ) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT6302_PAD;
		}
#endif
		if(Is63268AFEPathRemap(AFE_PATH_EXTERNAL, bpGpioMode)) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_MODE_MAP;
		}
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
		else if( ((PERF->RevID & REV_ID_MASK) >= 0xD0) && Is63268DSLCtrlPinUsed(bpGpioMode) ) {
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT6302_PAD;
		}
#endif
		if(Is63268LDPwrGpioPinSwap(bpGpioPwr) || Is63268LDModeGpioPinSwap(bpGpioMode)
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 2)
			||Is63268LDPwrDSLCtrlPinSwap(bpGpioPwr) || Is63268LDModeDSLCtrlPinSwap(bpGpioMode)
#endif
			)
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_PIN_SWAP;

		if(BP_ACTIVE_LOW == (bpGpioPwr & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_PWR_POL;
		if(BP_ACTIVE_LOW == (bpGpioMode & BP_ACTIVE_MASK))
			afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_MODE_POL;
	}
#endif /* CONFIG_BCM963268 */

#ifdef USE_PINMUX_DSLCTL_API
	if ((afeIds[0] & AFE_LD_MASK) == (AFE_LD_6303 << AFE_LD_SHIFT)) {
		if (BP_SUCCESS == BpGetIntAFELDPwrDslCtl(&bpGpioPwr)) {
			if (((BP_VDSLCTL_0 & BP_PINMUX_ARG_MASK) >>  BP_PINMUX_ARG_SHIFT) != bpGpioPwr)
				afeInfo.afeChidIdConfig0 |= AFELD_CTL_INT_PWR_MAP;
		}
	}
	/* external line */
	if ((afeIds[1] & AFE_LD_MASK) == (AFE_LD_6303 << AFE_LD_SHIFT)) {
		if (BP_SUCCESS == BpGetExtAFELDPwrDslCtl(&bpGpioPwr)) {
			if (((BP_VDSLCTL_3 & BP_PINMUX_ARG_MASK) >>  BP_PINMUX_ARG_SHIFT) != bpGpioPwr)
				afeInfo.afeChidIdConfig0 |= AFELD_CTL_EXT_PWR_MAP;
		}
	}
	bpGpioMode = 0;
#endif /* USE_PINMUX_DSLCTL_API */

#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 9) && \
	(defined(CONFIG_BCM963381) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148))
	afeInfo.afeChidIdConfig1 |= DG_PHY_CONFIG_BYPASS_SHIFT_MASK;
#endif

#ifdef CONFIG_BCM_DSL_GFASTCOMBO
	if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERPHY_DISABLED) &&
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDPHY_FOUND) )
		afeInfo.afeChidIdConfig1 |= AFECTL1_LAST_MODE_VDSL;
	if( toPhy ) {
		t.cmd.command = kDslVdslAfeIdCmd;
		if(AFE_CHIP_GFCH0 == ((afeIds[0] & AFE_CHIP_MASK) >> AFE_CHIP_SHIFT))	/* Gfast2 board */
			t.cmd.param.value = (afeIds[1] & ~AFE_CHIP_MASK) | (AFE_CHIP_CH0 << AFE_CHIP_SHIFT);
		else
			t.cmd.param.value = afeIds[1];
		
		DiagWriteString(0, DIAG_DSL_CLIENT, "*** Drv: Sending VDSL afeId(0x%08X) to PHY\n", t.cmd.param.value);
		BcmCoreCommandHandlerSync(&t.cmd);
	}
#endif

#ifdef CONFIG_BCM_DSL_GFAST
	res = BpGetAFELDRelayGpio(&bpGpioAFELDRelay);
#endif

	for(i = 0; i < MAX_DSL_LINE; i++) {
#ifdef SUPPORT_DSL_BONDING
		if( (1 == i) && !ADSL_PHY_SUPPORT(kAdslPhyBonding))
			break;	/* Bonding driver with non bonding PHY */
#endif
#ifdef CONFIG_BCM963268
		if ((afeConfigExt != 0) &&
			((1 == i) || (!ADSL_PHY_SUPPORT(kAdslPhyBonding) && (AFE_CHIP_INT_BITMAP != (afeIds[0] & AFE_CHIP_MASK)))))
			afeInfo.afeChidIdConfig0 = afeConfigExt;
#endif
#ifndef CONFIG_BCM_DSL_GFAST
		afeInfo.boardAfeId = afeIds[i];
#else
		if(ADSL_PHY_SUPPORT(kAdslPhyGfast) && (0 == i)) {
			afeInfo.boardAfeId = afeIds[0];
			afeInfo.boardAfeId = (afeInfo.boardAfeId & ~(AFE_CHIP_MASK|AFE_FE_ANNEX_MASK)) | AFE_CHIP_GFAST_BITMAP | AFE_FE_ANNEXX_BITMAP;
			if(toPhy && (BP_SUCCESS == res)) {
				DiagWriteString(i, DIAG_DSL_CLIENT, "*** Drv: De-activate LD Relay (bpGpio 0x%04X)\n",bpGpioAFELDRelay);
				kerSysSetGpio(bpGpioAFELDRelay, kGpioInactive);	/* De-activate the switch relay to use the internal AFE path */
			}
		}
		else {
			afeInfo.boardAfeId = afeIds[i];
			if(0 == i) {
				if(AFE_CHIP_GFCH0 == ((afeInfo.boardAfeId & AFE_CHIP_MASK) >> AFE_CHIP_SHIFT))
					afeInfo.boardAfeId = (afeIds[1] & ~AFE_CHIP_MASK) | (AFE_CHIP_CH0 << AFE_CHIP_SHIFT);
				else
					afeInfo.boardAfeId = afeIds[1];
				if(toPhy && (BP_SUCCESS == res)) {
					DiagWriteString(i, DIAG_DSL_CLIENT, "*** Drv: Activate LD Relay (bpGpio 0x%04X)\n",bpGpioAFELDRelay);
					kerSysSetGpio(bpGpioAFELDRelay, kGpioActive);	/* Activate the switch relay to use the external AFE path */
				}
			}
		}
#endif
		if( toPhy ) {
			/* Send info to PHY */
			if( 0 == afeInfo.boardAfeId ) {
#ifdef CONFIG_VDSL_SUPPORTED
#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
				afeInfo.boardAfeId = 0x40808100;	/* A.12.3.30 */
#elif defined(CONFIG_BCM963381)
				afeInfo.boardAfeId = 0x40808200;
#else
				afeInfo.boardAfeId =
					(AFE_CHIP_INT << AFE_CHIP_SHIFT) |
					(AFE_LD_ISIL1556 << AFE_LD_SHIFT) |
					(AFE_FE_REV_ISIL_REV1 << AFE_FE_REV_SHIFT) |
					(AFE_FE_ANNEXA << AFE_FE_ANNEX_SHIFT);
#endif
				DiagWriteString(i, DIAG_DSL_CLIENT, "*** Drv:  AfeId is not defined.  Using driver's default afeId(0x%08X)\n",
					afeInfo.boardAfeId);
#else
				DiagWriteString(i, DIAG_DSL_CLIENT, "*** Drv:  AfeId is not defined.  Skip sending to PHY\n");
				continue;
#endif
			}
			t.cmd.command = kDslSendEocCommand | (i << DSL_LINE_SHIFT);
			t.cmd.param.dslClearEocMsg.msgId = kDslAfeInfoCmd;
			gSharedMemAllocFromUserContext=1;
			t.cmd.param.dslClearEocMsg.dataPtr = AdslCoreSharedMemAlloc(sizeof(afeDescStruct));
#ifdef ADSLDRV_LITTLE_ENDIAN
			AfeDescCopy((afeDescStruct *)t.cmd.param.dslClearEocMsg.dataPtr, &afeInfo);
#else
			memcpy(t.cmd.param.dslClearEocMsg.dataPtr,	(void *)&afeInfo, sizeof(afeDescStruct));
#endif
			t.cmd.param.dslClearEocMsg.msgType = sizeof(afeDescStruct);
			BcmCoreCommandHandlerSync(&t.cmd);
			gSharedMemAllocFromUserContext=0;
		}
		else {
			/* Send info to DslDiags */
#ifndef ADSLDRV_LITTLE_ENDIAN
			afeDescStruct	*pAfeInfo = &afeInfo;
#else
			afeDescStruct	afeInfo1;
			afeDescStruct	*pAfeInfo = &afeInfo1;
			AfeDescCopy(&afeInfo1, &afeInfo);
#endif
			t.status.code = kDslReceivedEocCommand | (i << DSL_LINE_SHIFT);
			t.status.param.dslClearEocMsg.msgId = kDslAfeInfoCmd;
			t.status.param.dslClearEocMsg.dataPtr = (void *)pAfeInfo;
			t.status.param.dslClearEocMsg.msgType = sizeof(afeDescStruct);
			DiagWriteStatus(&t.status, (char *)&t.status, sizeof(t.status.code) + sizeof(t.status.param.dslClearEocMsg));
		}
		DiagWriteString(i, DIAG_DSL_CLIENT, "*** Drv: Sending afeId(0x%08X) afeChidIdConfig0(0x%08X) to %s\n",
			afeInfo.boardAfeId, afeInfo.afeChidIdConfig0, (toPhy) ? "PHY": "DslDiags");
	}
}
#endif

void BcmXdslCoreDiagSendPhyInfo(void)
{
	dslStatusStruct status;
	long *pPhyInfo = (long *)AdslCoreGetPhyInfo();
#ifdef ADSLDRV_LITTLE_ENDIAN
	adslPhyInfo phyInfo;
	BlockLongMoveReverse(4, pPhyInfo, (long *)&phyInfo);	/* sdramPageAddr - sdramPhyImageAddr */
	BlockShortMoveReverse(4,(short *)(pPhyInfo+4), (short *)(((long *)&phyInfo)+4));	/* fwType - mnVerNum */
	BlockLongMoveReverse(4, pPhyInfo+7, ((long *)&phyInfo)+7);	/* features[0 -3] */
	pPhyInfo = (long *)&phyInfo;
#endif
	status.code = kDslReceivedEocCommand;
	status.param.dslClearEocMsg.msgId = kDslPhyInfoCmd;
	status.param.dslClearEocMsg.dataPtr = (void *)pPhyInfo;
	status.param.dslClearEocMsg.msgType = sizeof(adslPhyInfo);
	DiagWriteStatus(&status, (char *)&status, sizeof(status.code) + sizeof(status.param.dslClearEocMsg));

}

void BcmXdslCoreMaintenanceTask(void)
{
	OS_TICKS	ticks;

	bcmOsGetTime(&ticks);
#if !defined(PHY_BLOCK_TEST) && !defined(PHY_LOOPBACK)
	if ( adslCoreStarted && !ADSL_PHY_SUPPORT(kAdslPhyPlayback) && !phyInExceptionState
#ifdef SUPPORT_XDSLDRV_GDB
		&& !isGdbOn()
#endif
		)
	{
		int	tPingMs, tMs;
#ifdef ADSLDRV_STATUS_PROFILING
		tMs = (ticks - printTicks) * BCMOS_MSEC_PER_TICK;
		if (tMs >= 3000)
			BcmXdslCoreDrvProfileInfoPrint();
#endif
#ifdef SUPPORT_SELT
        {
          int i0;
          for(i0=0;i0<MAX_DSL_LINE;i0++)
          {
              if (acTestModeWait[i0])
              {
                  tPingMs = (ticks - seltLastTick) * BCMOS_MSEC_PER_TICK;
                  if (tPingMs>1000)
                  {
                      seltLastTick = ticks;
                  }

                  if (ticks>acTestModeWait[i0])
                  {
                      acTestModeWait[i0] = 0;
                      BcmAdslCoreSetSeltNextMode(i0);
                  }
              }
          }
        }
#endif /* SUPPORT_SELT */

		tMs = (ticks - statLastTick) * BCMOS_MSEC_PER_TICK;
		tPingMs = (ticks - pingLastTick) * BCMOS_MSEC_PER_TICK;
		if (ADSL_MIPS_STATUS_INIT_TIMEOUT_MS == noStatTimeout) {
			if ((ticks - initTick) > 20000)
				noStatTimeout = ADSL_MIPS_STATUS_TIMEOUT_MS;
		}
		if ((tMs > noStatTimeout) && (tPingMs > 1000)) {
			dslCommandStruct	cmd;
			BcmAdslCoreDiagWriteStatusString(0, "ADSL MIPS inactive. Sending Ping\n");
			cmd.command = kDslPingCmd;
			BcmCoreCommandHandlerSync(&cmd);
			pingLastTick = ticks;
			if (tMs > (noStatTimeout + 5000)) {
				int i; OS_TICKS	osTime0;
				BcmAdslCoreDiagWriteStatusString(0, "Resetting ADSL MIPS\n");
				for(i = 0; i < 20; i++) {
					BcmAdslCoreDiagWriteStatusString(0, "PC = 0x%08x\n", ADSL_ENDIAN_CONV_LONG(*(volatile ulong *)gPhyPCAddr)); // (HOST_LMEM_BASE + 0x5E8)
					bcmOsGetTime(&osTime0);
					while (BcmAdslCoreOsTimeElapsedMs((ulong)osTime0) < 1);
				}
				if (BcmAdslCoreCanReset()) {
					adslCoreResetPending = AC_TRUE;
					BcmAdslCoreNotify(0, ACEV_LINK_DOWN|BCM_ADSL_EVENT);
#ifdef SUPPORT_DSL_BONDING
					if(ADSL_PHY_SUPPORT(kAdslPhyBonding))
						BcmAdslCoreNotify(1, ACEV_LINK_DOWN|BCM_ADSL_EVENT);
#endif
#if !defined(__ECOS)
					BcmXdsl_RequestIoCtlCallBack();
#endif
				}
			}
		}
	
		if (acL3StartTick[0] != 0) {
			tMs = (ticks - acL3StartTick[0]) * BCMOS_MSEC_PER_TICK;
			if (tMs > 20000) {
				acL3StartTick[0] = 0;
				BcmAdslCoreConnectionStart(0);
			}
		}
#ifdef SUPPORT_DSL_BONDING
		if(ADSL_PHY_SUPPORT(kAdslPhyBonding) && (acL3StartTick[1] != 0)) {
			tMs = (ticks - acL3StartTick[1]) * BCMOS_MSEC_PER_TICK;
			if (tMs > 20000) {
				acL3StartTick[1] = 0;
				BcmAdslCoreConnectionStart(1);
			}
		}
#endif
	}
#endif /*!defined(PHY_BLOCK_TEST) && !defined(PHY_LOOPBACK) */
	
#ifdef SUPPORT_XDSLDRV_GDB
	BcmAdslCoreGdbTask();
#endif

#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
	BcmCoreDiagZeroCopyTxDoneHandler();
#endif

	if (adslCoreResetPending) {
#ifdef __KERNEL__
		if (!in_softirq())
#endif
		{
#if defined(CONFIG_BCM963138)
			adslCoreResetDelay = AC_FALSE;
#endif
			adslCoreResetPending = AC_FALSE;
			AdslDrvPrintf(TEXT("*** %s: Resetting XdslCore\n"), __FUNCTION__);
			BcmAdslCoreReset(DIAG_DATA_EYE);
		}
	}
#if defined(CONFIG_BCM963138)
	if(adslCoreResetDelay) {
		adslCoreResetPending = AC_TRUE;
		adslCoreResetDelay = AC_FALSE;
#ifndef __ECOS
		BcmXdsl_RequestIoCtlCallBack();
#endif
	}
#endif
}

void BcmAdslCoreDiagDataLogNotify(int set)
{
#if defined(CONFIG_BCM96338) || defined(CONFIG_BCM96348)
	if(set) {
		gConsoleOutputEnable = 0;
		gDiagDataLog = AC_TRUE;
	}
	else {
		gConsoleOutputEnable = 1;
		gDiagDataLog = AC_FALSE;
	}
#endif
}

LOCAL Bool BcmAdslCoreCanReset(void)
{
#ifdef VXWORKS
	if (adslCoreAlwaysReset)
		return 1;
	return (BcmAdslCoreStatusSnooper == AdslCoreGetStatusCallback());
#else
	return adslCoreAlwaysReset;
#endif
}

unsigned long BcmAdslCoreOsTimeElapsedMs(unsigned long  osTime0)
{
	OS_TICKS	osTime1;
	
	bcmOsGetTime(&osTime1);
	
	return ((osTime1 - (OS_TICKS)osTime0) * BCMOS_MSEC_PER_TICK);
}

ulong BcmAdslCoreCycleTimeElapsedUs(ulong cnt1, ulong cnt0)
{
	cnt1 -= cnt0;

	if (cnt1 < ((ulong) -1) / 1000)
		return (cnt1 * 1000 / adslCoreCyclesPerMs);
	else
		return (cnt1 / (adslCoreCyclesPerMs / 1000));
}

#ifdef _NOOS
ulong BcmAdslCoreCalibrate(void)
{
	if (adslCoreCyclesPerMs != 0)
		return adslCoreCyclesPerMs;
	adslCoreCyclesPerMs = 1000000;
	return adslCoreCyclesPerMs;
}
#else
#if defined(CONFIG_ARM)
//TO DO: Remove after verified with 63138 boards
static inline unsigned long read_pmccntr(void)
{
	unsigned long x;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r" (x));
	return x;
}

static inline void write_pmccntr(unsigned long x)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r" (x));
}

static inline unsigned long read_pmcntenset(void)
{
	unsigned long x;
	asm volatile("mrc p15, 0, %0, c9, c12, 1" : "=r" (x));
	return x;
}

static inline void write_pmcntenset(unsigned long x)
{
	asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r" (x));
}

static void check_pmccntr_is_up(void)
{
	unsigned long x, y;

	x = read_pmccntr();
	y = read_pmcntenset();
	
	if (!(x & 0x1) || !(y & 0x80000000)) {
		/* pmc_cntr is not enabled */
		x |= 0x00000001;
		write_pmccntr(x);
		y |= 0x80000000;
		write_pmcntenset(y);
#if defined(CONFIG_SMP)
		AdslDrvPrintf(TEXT("%s: cpu%d pmccntr=0x%x, pmcntenset=0x%x\n"),
			__FUNCTION__, smp_processor_id(), (uint)read_pmccntr(), (uint)read_pmcntenset());
#else
		AdslDrvPrintf(TEXT("%s: pmccntr=0x%x, pmcntenset=0x%x\n"),
			__FUNCTION__, (uint)read_pmccntr(), (uint)read_pmcntenset());
#endif
	}
}
#endif

ulong BcmAdslCoreGetCycleCount(void)
{
	ulong	cnt;
#ifdef _WIN32_WCE
	cnt = 0;
#elif defined(__arm) || defined(CONFIG_ARM)
#ifdef _NOOS
	cnt = read_PMCCNTR();
#else
#if defined(BOARD_H_API_VER) && (BOARD_H_API_VER < 8)
	check_pmccntr_is_up();
#endif
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r" (cnt));
#endif
#else	/* MIPS targets */
	__asm volatile("mfc0 %0, $9":"=d"(cnt));
#endif

	return(cnt);
}

ulong BcmAdslCoreCalibrate(void)
{
	OS_TICKS	tick0, tick1;
	ulong		cnt;

	if (adslCoreCyclesPerMs != 0)
		return adslCoreCyclesPerMs;
#if defined(CONFIG_ARM)
	check_pmccntr_is_up();
#endif
	bcmOsGetTime(&tick1);
	do {
		bcmOsGetTime(&tick0);
	} while (tick0 == tick1);
	
	BcmCoreDpcSyncEnter();
	cnt = BcmAdslCoreGetCycleCount();
	do {
		bcmOsGetTime(&tick1);
		tick1 = (tick1 - tick0) * BCMOS_MSEC_PER_TICK;
	} while (tick1 < 60);
	cnt = BcmAdslCoreGetCycleCount() - cnt;
	BcmCoreDpcSyncExit();
	
	adslCoreCyclesPerMs = cnt / tick1;
#if defined(CONFIG_ARM)
	AdslDrvPrintf(TEXT("%s: (cnt1-cnt)=0x%08x, adslCoreCyclesPerMs=%lu, BCMOS_MSEC_PER_TICK =%d\n"),
		__FUNCTION__, (uint)cnt, adslCoreCyclesPerMs, BCMOS_MSEC_PER_TICK);
	if(0 == adslCoreCyclesPerMs)
		adslCoreCyclesPerMs = 1000000;
#endif
	return adslCoreCyclesPerMs;
}
#endif

void BcmAdslCoreDelay(ulong timeMs)
{
	OS_TICKS	tick0, tick1;

	bcmOsGetTime(&tick0);
	do {
		bcmOsGetTime(&tick1);
		tick1 = (tick1 - tick0) * BCMOS_MSEC_PER_TICK;
	} while (tick1 < timeMs);
}

#ifndef _NOOS
void BcmAdslCoreSetWdTimer(ulong timeUs)
{
	TIMER->WatchDogDefCount = timeUs * (FPERIPH/1000000);
	TIMER->WatchDogCtl = 0xFF00;
	TIMER->WatchDogCtl = 0x00FF;
}
#endif

/***************************************************************************
** Function Name: BcmAdslCoreCheckBoard
** Description  : Checks the presense of Bcm ADSL core
** Returns      : 1
***************************************************************************/
Bool BcmAdslCoreCheckBoard()
{
	return 1;
}

#ifndef USE_PMC_API
#define	MAX_DOWNCMD_DELAY_MS	5
#else
#define	MAX_DOWNCMD_DELAY_MS	50
#endif

void BcmAdslCoreStop(void)
{
	OS_TICKS osTime;
#ifdef USE_PMC_API
	OS_STATUS nRet;
#endif

#ifdef USE_PMC_API
	DiagWriteString(0, DIAG_DSL_CLIENT, "%s: Sending kDslDownCmd\n", __FUNCTION__);
	sema_init((struct semaphore *)syncPhyMipsSemId, 0);
#endif
	BcmXdslCoreSendCmd(0, kDslDownCmd, kDslIdleNone);
#ifdef SUPPORT_DSL_BONDING
	if(ADSL_PHY_SUPPORT(kAdslPhyBonding))
		BcmXdslCoreSendCmd(1, kDslDownCmd, kDslIdleNone);
#endif
	bcmOsGetTime(&osTime);
#ifndef USE_PMC_API
	while(BcmAdslCoreOsTimeElapsedMs(osTime) < MAX_DOWNCMD_DELAY_MS);
#else
	nRet = bcmOsSemTake(syncPhyMipsSemId, (MAX_DOWNCMD_DELAY_MS/BCMOS_MSEC_PER_TICK));
	if(OS_STATUS_FAIL == nRet)
		bcmOsSleep((MAX_DOWNCMD_DELAY_MS/BCMOS_MSEC_PER_TICK));
	DiagWriteString(0, DIAG_DSL_CLIENT, "%s: delayed %d ms, nRet(%d)\n", __FUNCTION__, (int)BcmAdslCoreOsTimeElapsedMs(osTime), (int)nRet);
#endif
	
	adslCoreStarted = AC_FALSE;
#ifdef CONFIG_BCM_PWRMNGT_DDR_SR_API
	BcmPwrMngtRegisterLmemAddr(NULL);
#endif
	bcmOsGetTime(&statLastTick);

	AdslCoreStop();
#if defined(__KERNEL__)
	dpcScheduled = AC_FALSE;
#endif
}

void __BcmAdslCoreStart(int diagDataMap, Bool bRestoreImage)
{
	int	i;
	volatile ulong	*pAdslEnum = (ulong *) XDSL_ENUM_BASE;

	phyInExceptionState = FALSE;
	gPhyPCAddr = HOST_LMEM_BASE + 0x5B0;
	adslCoreResetPending = AC_FALSE;
	bcmOsGetTime(&statLastTick);
	pingLastTick = statLastTick;
	AdslCoreHwReset(bRestoreImage);
	bcmOsGetTime(&initTick);
	noStatTimeout = ADSL_MIPS_STATUS_INIT_TIMEOUT_MS;
#if defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)
	pAdslEnum[RCV_ADDR_INTR] = adslCoreDiagState.rcvAddr;
	pAdslEnum[RCV_CTL_INTR]  = adslCoreDiagState.rcvCtrl;
	pAdslEnum[RCV_PTR_INTR]  = adslCoreDiagState.rcvPtr;
	pAdslEnum[XMT_ADDR_INTR] = adslCoreDiagState.xmtAddr;
	pAdslEnum[XMT_CTL_INTR]  = adslCoreDiagState.xmtCtrl;
	pAdslEnum[XMT_PTR_INTR]  = adslCoreDiagState.xmtPtr;
#endif
	for(i = 0; i < MAX_DSL_LINE; i++) {
#ifdef SUPPORT_DSL_BONDING
		if( (1 == i) && !ADSL_PHY_SUPPORT(kAdslPhyBonding))
			break;	/* Bonding driver with non bonding PHY */
#endif
		BcmAdslCoreSetConnectionParam((uchar)i, &adslCoreConnectionParam[i], &adslCoreCfgCmd[i], pAdslCoreCfgProfile[i]);
		BcmCoreCommandHandlerSync(&adslCoreCfgCmd[i]);
		BcmCoreCommandHandlerSync(&adslCoreConnectionParam[i]);
	}
#if !defined(CONFIG_BCM963268) && defined(SUPPORT_MULTI_PHY)
	if((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) &&
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIASEARCH_DISABLED) &&
		(BCM_XDSLMODE_SINGLELINE==(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK))
#if defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
		&& !ADSL_PHY_SUPPORT(kAdslPhyGfast)
#endif
		) {
		unsigned char lineToStayDown = 1^(((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIATYPE_MSK) >> BCM_MEDIATYPE_SHIFT) & 1);
		BcmXdslCoreSendCmd(lineToStayDown, kDslDownCmd, 0);
	}
#endif
	adslCoreStarted = AC_TRUE;
	/* Clear pending status interrupt, and enable it */
	pAdslEnum[ADSL_INTSTATUS_I] = pAdslEnum[ADSL_INTSTATUS_I];
	pAdslEnum[ADSL_INTMASK_I] |= MSGINT_MASK_BIT;
#if defined(__KERNEL__)
	bcmOsDpcEnqueue(irqDpcId);
#endif
}

#ifdef  BCM6348_PLL_WORKAROUND

Bool			adslCoreReverbPwrRcv = AC_FALSE;
ulong			adslCoreReverbPwr = 0;
void AdslCoreSetPllClock(void);

void BcmAdslCoreReverbPwrClear(void)
{
	adslCoreReverbPwrRcv = AC_FALSE;
	adslCoreReverbPwr = 0;
}

void BcmAdslCoreReverbPwrSet(ulong pwr)
{
	adslCoreReverbPwrRcv = AC_TRUE;
	adslCoreReverbPwr = pwr;
}

Bool BcmAdslCoreReverbPwrGet(ulong *pPwr)
{
	*pPwr = adslCoreReverbPwr;
	return adslCoreReverbPwrRcv;
}

void BcmAdslCoreHandlePLL(ulong to)
{
	ulong		pwr, tryCnt;
	OS_TICKS	tick0, tick1;

	tryCnt = 0;
	do {
		BcmAdslCoreDelay(10);
		BcmAdslCoreSetTestMode(0, kDslTestEstimatePllPhase);
		BcmAdslCoreDelay(300);
		BcmAdslCoreReverbPwrClear();
		BcmAdslCoreSetTestMode(0, kDslTestReportPllPhaseStatus);
		bcmOsGetTime(&tick0);
		do {
			bcmOsSleep (10/BCMOS_MSEC_PER_TICK);
			bcmOsGetTime(&tick1);
			tick1 = (tick1 - tick0) * BCMOS_MSEC_PER_TICK;
			if (tick1 > to)
				break;
		} while (!BcmAdslCoreReverbPwrGet(&pwr));
		if (!BcmAdslCoreReverbPwrGet(&pwr)) {
			BcmAdslCoreDiagWriteStatusString(0, "No PLL test status");
			break;
		}

		if (pwr < 5000) {
			BcmAdslCoreDiagWriteStatusString(0, "PLL test passed, pwr = %d", pwr);
			break;
		}
		
		BcmAdslCoreDiagWriteStatusString(
			0,
			"PLL test failed, pwr = %d\n"
			"Resetting PLL and ADSL PHY", pwr);
		AdslCoreSetPllClock();

		BcmAdslCoreStop();
		__BcmAdslCoreStart(-1, AC_TRUE);
	} while (++tryCnt < 5);
}

void BcmAdslCoreHandlePLLReset(void)
{
	if (PERF->RevID >= 0x634800A1)
		return;
	adslCoreMuteDiagStatus = AC_TRUE;
	BcmAdslCoreDiagWriteStatusString(0, "<<<Start 6348 PLL check");
	BcmAdslCoreHandlePLL(20);
	adslCoreMuteDiagStatus = AC_FALSE;
	BcmAdslCoreDiagWriteStatusString(0, ">>>6348 PLL check complete");
}

void BcmAdslCoreHandlePLLInit(void)
{
	if (PERF->RevID >= 0x634800A1)
		return;
	BcmCoreCommandHandlerSync(&adslCoreCfgCmd[0]);
	BcmCoreCommandHandlerSync(&adslCoreConnectionParam[0]);
	BcmAdslCoreHandlePLLReset();
}

#endif

void BcmAdslCoreStart(int diagDataMap, Bool bRestoreImage)
{
#ifdef PHY_BLOCK_TEST
	BcmAdslCoreDebugReset();
#endif
	__BcmAdslCoreStart(diagDataMap, bRestoreImage);
#if 1 && defined(BCM6348_PLL_WORKAROUND)
	BcmAdslCoreHandlePLLReset();
#endif
#ifndef PHY_BLOCK_TEST	/* PHY features are populated after __BcmAdslCoreStart() */
	if (ADSL_PHY_SUPPORT(kAdslPhyPlayback))
		BcmAdslCoreDebugReset();
#endif
	BcmAdslDiagReset(diagDataMap);
#if defined(CONFIG_BCM96338) || defined(CONFIG_BCM96348)
	BcmAdslDiagEnable();
#endif
	BcmAdslCoreLogWriteConnectionParam(&adslCoreConnectionParam[0]);
#ifdef SUPPORT_DSL_BONDING
	if (ADSL_PHY_SUPPORT(kAdslPhyBonding))
		BcmAdslCoreLogWriteConnectionParam(&adslCoreConnectionParam[1]);
#endif
#if defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)
	BcmCoreAtmVcSet();
#endif
}

/***************************************************************************
** Function Name: BcmAdslCoreReset
** Description  : Completely resets ADSL MIPS 
** Returns      : None.
***************************************************************************/
void BcmAdslCoreReset(int diagDataMap)
{
	BcmAdslCoreStop();
	BcmAdslCoreStart(diagDataMap, AC_TRUE);
	AdslDrvPrintf(TEXT("VersionInfo: %s.d%s\n"),(NULL != adslCorePhyDesc.pVerStr)? adslCorePhyDesc.pVerStr: "UnknownPHY", ADSL_DRV_VER_STR);
#if defined(SUPPORT_MULTI_PHY)
	if((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) &&
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIASEARCH_DISABLED) &&
		(BCM_IMAGETYPE_SINGLELINE==(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK))) {
		unsigned char lineToStayDown = 1^(((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIATYPE_MSK) >> BCM_MEDIATYPE_SHIFT) & 1);
		kerSysLedCtrl((1==lineToStayDown)? kLedSecAdsl: kLedAdsl, kLedStateOff);
	}
#endif
}

/***************************************************************************
** Function Name: BcmAdslCoreDiagCmdAdsl
** Description  : ADSL command handler
** Returns      : None.
***************************************************************************/
void BcmAdslCoreDiagCmdAdsl(unsigned char lineId, int diagCmd, int len, void *pCmdData) {

	switch (diagCmd) {
		case LOG_CMD_DEBUG:
		{
			DiagDebugData	*pDbgCmd = pCmdData;
			switch (pDbgCmd->cmd) {
#if defined(CONFIG_BCM963268)
#ifdef TEST_CMD_BUF_RING
				case DIAG_DEBUG_CMD_START_BUF_TEST:
					BcmAdslCoreTestCmdStart(pDbgCmd->param1);
					break;
				case DIAG_DEBUG_CMD_STOP_BUF_TEST:
					BcmAdslCoreTestCmdStop();
					break;
#endif
				case DIAG_DEBUG_CMD_SET_RAW_DATA_MODE:
					BcmAdslCoreDiagSetupRawData(pDbgCmd->param1);
					break;
#endif
#if defined(PHY_LOOPBACK) || defined(SUPPORT_TEQ_FAKE_LINKUP)
				case DIAG_DEBUG_CMD_SET_XTM_LINKUP:
					AdslDrvXtmLinkUp(pDbgCmd->param1, pDbgCmd->param2);
					break;
				case DIAG_DEBUG_CMD_SET_XTM_LINKDOWN:
					AdslDrvXtmLinkDown(pDbgCmd->param1);
					break;
#endif
				default:
					BcmAdslCoreDebugCmd(lineId, pDbgCmd);
					break;
			}
			break;
		}
		default:
			BcmAdslCoreDiagCmdCommon(lineId, diagCmd, len, pCmdData);
			break;
	}
}

/***************************************************************************
** Function Name: BcmAdslCoreStatusSnooper
** Description  : Some DSL status processing
** Returns      : 1
***************************************************************************/
ulong BcmAdslCoreStatusSnooper(dslStatusStruct *status, char *pBuf, int len)
{
	long				val;
	adslMibInfo			*pMibInfo;
	ulong				mibLen;
	static	Bool		bCheckBertEx = false;
	dslStatusCode		statusCode;
	uchar			lineId;
#if defined(SUPPORT_MULTI_PHY) && defined(SUPPORT_VECTORING) && defined(CONFIG_BCM963268)
	static	int	mediaSearchTrainingTimeoutPrev = -1;
#endif
	bcmOsGetTime(&statLastTick);
	pingLastTick = statLastTick;
	
	statusCode = DSL_STATUS_CODE(status->code);
	lineId = DSL_LINE_ID(status->code);

	if (!adslCoreMuteDiagStatus || (kDslExceptionStatus == statusCode))
		BcmAdslCoreDiagStatusSnooper(status, pBuf, len);

	if (bCheckBertEx) {
		bCheckBertEx = false;
		mibLen = sizeof(adslMibInfo);
		pMibInfo = (void *) AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &mibLen);
		if (0 == pMibInfo->adslBertStatus.bertSecCur)
			BcmAdslCoreDiagWriteStatusString(
				lineId,
				"BERT_EX results: totalBits=0x%08lX%08lX, errBits=0x%08lX%08lX",
				pMibInfo->adslBertStatus.bertTotalBits.cntHi, 
				pMibInfo->adslBertStatus.bertTotalBits.cntLo,
				pMibInfo->adslBertStatus.bertErrBits.cntHi, 
				pMibInfo->adslBertStatus.bertErrBits.cntLo);
	}

	switch (statusCode) {
#ifdef USE_PMC_API
		case kDslWakeupRequest:
			bcmOsSemGive(syncPhyMipsSemId);
			break;
#endif
		case kDslEpcAddrStatus:
			gPhyPCAddr = (ulong)(ADSL_ADDR_TO_HOST(status->param.value));
			break;
		case kDslEscapeToG994p1Status:
		{
#ifdef SUPPORT_MULTI_PHY
			xdslMedia curMedia;
#if !defined(CONFIG_BCM963268)
			curMedia = lineId;
#else
			curMedia = ADSL_PHY_SUPPORT(kAdslPhyBonding)? lineId: xdslSearchMediaCfg.mediaInUse;
#endif
#ifdef SUPPORT_SELT
			if(!acSELTSuspendMediaSearch[lineId])
#endif
			bMediaSearchSuspended = AC_FALSE;
			bMediaSearchBndIndicated[lineId] = AC_FALSE;
			if(TRUE == MEDIA_SIGNAL_PRESENT(curMedia))
				BcmXdslCoreMediaSearchSM(MEDIASEARCH_LINKDOWN_E, 0);
#endif	/* SUPPORT_MULTI_PHY */
			if(gSaveStatCtrl.saveStatDisableOnRetrain)
				BcmAdslDiagStatSaveStop();
			break;
		}
		case kDslExceptionStatus:
			phyInExceptionState = TRUE;
#if 0 || defined(ADSL_SELF_TEST)
			{
			ulong	*sp, spAddr;
			int		i, stackSize;

			AdslDrvPrintf (TEXT("DSL Exception:\n"));
			sp = (ulong*) status->param.dslException.sp;
			for (i = 0; i < 28; i += 4)
				AdslDrvPrintf (TEXT("R%d=0x%08lX\tR%d=0x%08lX\tR%d=0x%08lX\tR%d=0x%08lX\n"),
					i+1, sp[i], i+2, sp[i+1], i+3, sp[i+2], i+4, sp[i+3]);
			AdslDrvPrintf (TEXT("R29=0x%08lX\tR30=0x%08lX\tR31=0x%08lX\n"), sp[28], sp[29], sp[30]);

			sp = (ulong*) status->param.dslException.argv;
			AdslDrvPrintf (TEXT("argv[0] (EPC) = 0x%08lX\n"), sp[0]);
			for (i = 1; i < status->param.dslException.argc; i++)
				AdslDrvPrintf (TEXT("argv[%d]=0x%08lX\n"), i, sp[i]);
			AdslDrvPrintf (TEXT("Exception stack dump:\n"));

			sp = (ulong*) status->param.dslException.sp;
			spAddr = sp[28];
#ifdef FLATTEN_ADDR_ADJUST
			if ((spAddr & 0x50000000) == 0x50000000)
			  spAddr = spAddr & ~0x40000000;
			sp = (ulong *)ADSL_ADDR_TO_HOST(spAddr);
			stackSize = 64;
#else
			sp = (ulong *) status->param.dslException.stackPtr;
			stackSize = status->param.dslException.stackLen;
#endif
			for (i = 0; i < stackSize; i += 8)
				{
				AdslDrvPrintf (TEXT("%08lX: %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX\n"),
					spAddr + (i*4), sp[0], sp[1], sp[2], sp[3], sp[4], sp[5], sp[6], sp[7]);
				sp += 8;
				}
			}
#endif
			if (BcmAdslCoreCanReset()) {
#if defined(CONFIG_BCM963138)
				if((0xb0 == (PERF->RevID & REV_ID_MASK)) && ADSL_PHY_SUPPORT(kAdslPhyDualMips)) {
					if(AC_FALSE == adslCoreResetPending)
						adslCoreResetDelay = AC_TRUE;
				}
				else
#endif
				{
				adslCoreResetPending = AC_TRUE;
#if !defined(__ECOS)
				BcmXdsl_RequestIoCtlCallBack();
#endif
				}
				BcmAdslCoreNotify(lineId, ACEV_LINK_DOWN|BCM_ADSL_EVENT);
			}
			else {
				adslCoreConnectionMode[lineId] = AC_FALSE;
#if defined(SUPPORT_DSL_BONDING) && !defined(SUPPORT_2CHIP_BONDING)
				adslCoreConnectionMode[lineId^1] = AC_FALSE;
#endif
			}
			break;
#ifdef SUPPORT_MULTI_PHY
		case kDslConnectInfoStatus:
			val = status->param.dslConnectInfo.value;
			switch (status->param.dslConnectInfo.code) {
				case kG994MessageExchangeRcvInfo:
					BcmXdslCoreMediaSearchSM(MEDIASEARCH_SIGNAL_E, 0);
					break;
				case kDslATUAvgLoopAttenuationInfo:
					BcmXdslCoreMediaSearchSM(MEDIASEARCH_RXLNATTNAVG_E, (ulong)val << 4);
					break;
#if 0 && defined(CONFIG_BCM963268)
				case kDslFramingModeInfo:
					if(ADSL_PHY_SUPPORT(kAdslPhyBonding) && (val & kAtmHeaderCompression)) {
						long otherBcmMediaType = (0 == lineId) ? BCM_MEDIATYPE_EXTERNALAFE: BCM_MEDIATYPE_INTERNALAFE;
						BcmXdslCoreSetDefaultMediaType(otherBcmMediaType);
						BcmXdslCoreSwitchPhyImage(BCM_IMAGETYPE_SINGLELINE);	/* Switch to non-bonding PHY */
					}
					break;
#endif
			}
			break;
#endif	/* SUPPORT_MULTI_PHY */
		case kDslReceivedEocCommand:
			switch (status->param.value) {
#ifdef SUPPORT_MULTI_PHY
				case kDsl993p2FeQlnLD:
					bMediaSearchSuspended = AC_TRUE;
					break;
				//case kDsl993p2LnAttnAvg:
				case kDsl993p2LnAttnRaw:
					val = *(short*)status->param.dslClearEocMsg.dataPtr;
					BcmXdslCoreMediaSearchSM(MEDIASEARCH_RXLNATTNAVG_E, ADSL_ENDIAN_CONV_SHORT(val));
					break;
#endif
#if defined(DSL_EXTERNAL_BONDING_DISCOVERY) && !defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
				case kDslBondDiscExchange:
					BcmXdslCoreProcessBondDiscExchStat(status);
					break;
#endif
				default:
					break;
			}
			break;
		case kDslTrainingStatus:
			val = status->param.dslTrainingInfo.value;
			switch (status->param.dslTrainingInfo.code) {
#if defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
				case kDslGfastCOSupport:
				{
					long modCfg;
					
					if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PHYSWITCH_DISABLED)
						break;
					
					modCfg = adslCoreCfgProfile[0].adslAnnexAParam;
					if((kAdslCfgModAny == (modCfg & kAdslCfgModMask)) && !(adslCoreCfgProfile[0].adsl2Param & kAdsl2CfgAnnexMOnly))
						modCfg = kAdslCfgModMask;
					
					if(val) {
						if(!ADSL_PHY_SUPPORT(kAdslPhyGfast) && (modCfg & kDslCfgModGfastOnly)) {
							XdslCoreSetPhyImageType(BCM_PHYTYPE_GFAST >> BCM_PHYTYPE_SHIFT);
							xdslCorePhyImageJustSwitch = AC_TRUE;
							adslCoreResetPending = AC_TRUE;
#if !defined(__ECOS)
							BcmXdsl_RequestIoCtlCallBack();
#endif
						}
					}
					else if(ADSL_PHY_SUPPORT(kAdslPhyGfast)) {
						XdslCoreSetPhyImageType(BCM_PHYTYPE_NON_GFAST >> BCM_PHYTYPE_SHIFT);
						if(AC_FALSE == switchGfastToVdslImagePending) {
							switchGfastToVdslImagePending = AC_TRUE;
							BcmAdslCoreConnectionStart(lineId);
							xdslCorePhyImageJustSwitch = AC_TRUE;
						}
					}
					break;
				}
				case kDslG994p1ToneDet:
				case kDslG994p1ReturntoStartup:
				case kDslT1p413ReturntoStartup:
					if(switchGfastToVdslImagePending) {
						BcmXdslCoreSendCmd(lineId, kDslDownCmd, kDslIdleNone);
						adslCoreResetPending = AC_TRUE;
#if !defined(__ECOS)
						BcmXdsl_RequestIoCtlCallBack();
#endif
						switchGfastToVdslImagePending = AC_FALSE;
					}
					break;
#endif	/* CONFIG_BCM_DSL_GFAST && !defined(CONFIG_BCM_DSL_GFASTCOMBO) */
#ifdef SUPPORT_MULTI_PHY
				case kG992LDStartMode:
					if (val)
					  bMediaSearchSuspended = AC_TRUE;
					break;
				case kDslBondingState:
					bMediaSearchBndIndicated[lineId] = AC_TRUE;
#if defined(CONFIG_BCM963268)
					if((kDslBondingPTM == val) || (kDslBondingATM == val)) {
						if(!ADSL_PHY_SUPPORT(kAdslPhyBonding))
							BcmXdslCoreSwitchPhyImage(BCM_IMAGETYPE_BONDING);  /* Switch to bonding PHY */
					}
					else {
						if(ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
							long otherBcmMediaType = (0 == lineId) ? BCM_MEDIATYPE_EXTERNALAFE: BCM_MEDIATYPE_INTERNALAFE;
							
							BcmXdslCoreSetDefaultMediaType(otherBcmMediaType);
							BcmXdslCoreSwitchPhyImage(BCM_IMAGETYPE_SINGLELINE);  /* Switch to non-bonding PHY */
						}
					}
#endif
					break;
				case kDslFinishedG994p1:
				case kDslFinishedT1p413:
					if(kDslFinishedT1p413 == status->param.dslTrainingInfo.code)
						BcmXdslCoreMediaSearchSM(MEDIASEARCH_SIGNAL_E, 0);
#if defined(CONFIG_BCM963268)
#if defined(SUPPORT_DSL_BONDING)
					if(ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
						mibLen = sizeof(adslMibInfo);
						pMibInfo = (void *) AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &mibLen);
						pMibInfo->xdslInfo.dirInfo[US_DIRECTION].lpInfo[0].ahifChanId[0] = lineId;
					}
#endif
#ifdef SUPPORT_VECTORING
					if(kG993p2AnnexA == val) {
						char vectState, oidStr[] = { kOidAdslPrivate, kOIdAdslPrivGetVectState };
						long vectStateLen = sizeof(vectState);
						if((kAdslMibStatusSuccess == AdslCoreGetObjectValue(lineId, oidStr, sizeof(oidStr), &vectState, &vectStateLen)) &&
							(VECT_DISABLED != vectState))
							mediaSearchTrainingTimeout = MEDIASEARCH_VECT_TRAINING_TIMEOUT;
						else
							mediaSearchTrainingTimeout = MEDIASEARCH_TRAINING_TIMEOUT;
					}
					else
						mediaSearchTrainingTimeout = MEDIASEARCH_TRAINING_TIMEOUT;
					if(mediaSearchTrainingTimeoutPrev != mediaSearchTrainingTimeout) {
						AdslDrvPrintf (TEXT("mediaSearchTrainingTimeout = %d seconds\n"), mediaSearchTrainingTimeout);
						mediaSearchTrainingTimeoutPrev = mediaSearchTrainingTimeout;
					}
#endif
					if((ADSL_PHY_SUPPORT(kAdslPhyBonding)) && !bMediaSearchBndIndicated[lineId] &&
						(0 == (adslCoreConnectionParam[0].param.dslModeSpec.capabilities.auxFeatures & kDslAtmBondingSupported))) {
						long otherBcmMediaType = (0 == lineId) ? BCM_MEDIATYPE_EXTERNALAFE: BCM_MEDIATYPE_INTERNALAFE;
						
						BcmXdslCoreSetDefaultMediaType(otherBcmMediaType);
						BcmXdslCoreSwitchPhyImage(BCM_IMAGETYPE_SINGLELINE);  /* Switch to non-bonding PHY */
					}
#endif /* CONFIG_BCM963268 */
					break;
#endif /* SUPPORT_MULTI_PHY */
				case kDslG992p2RxShowtimeActive:
				{
#ifdef CONFIG_BCM_DSL_GFAST
					long modCfg = adslCoreCfgProfile[0].adslAnnexAParam;

					if((kAdslCfgModAny == (modCfg & kAdslCfgModMask)) && !(adslCoreCfgProfile[0].adsl2Param & kAdsl2CfgAnnexMOnly))
						modCfg = kAdslCfgModMask;
					
					if(!XdslMibIsGfastMod(XdslCoreGetDslVars(lineId))
#ifndef CONFIG_BCM_DSL_GFASTCOMBO
						&& (modCfg & kDslCfgModGfastOnly)
#endif
						) {
							BcmXdslCoreSetPreferredPhyType();
					}
					else {
						if(BcmXdslCoreClearPreferredPhyType())
							BcmXdsl_RequestSavePreferredLine();
					}
#endif
					BcmAdslCoreEnableSnrMarginData(lineId);
					break;
				}
				case kDslG994p1RcvNonStandardInfo:
					BcmAdslCoreNotify(lineId, ACEV_G994_NONSTDINFO_RECEIVED);
					break;
				default:
					break;
			}
			break;

		case kDslShowtimeSNRMarginInfo:
			if (status->param.dslShowtimeSNRMarginInfo.nCarriers != 0)
				BcmAdslCoreDisableSnrMarginData(lineId);
			break;
		case kAtmStatus: 
			switch (status->param.atmStatus.code) {
				case kAtmStatBertResult:
					bCheckBertEx = true;
					break;
#if 0 && defined(SUPPORT_MULTI_PHY) && defined(CONFIG_BCM963268)
				case kAtmStatHdrCompr:
					if(ADSL_PHY_SUPPORT(kAdslPhyBonding) && (0 != status->param.atmStatus.param.value)) {
						long otherBcmMediaType = (0 == lineId) ? BCM_MEDIATYPE_EXTERNALAFE: BCM_MEDIATYPE_INTERNALAFE;
						BcmXdslCoreSetDefaultMediaType(otherBcmMediaType);
						BcmXdslCoreSwitchPhyImage(BCM_IMAGETYPE_SINGLELINE);	/* Switch to non-bonding PHY */
					}
					break;
#endif
				default:
					break;
			}
			break;
#ifdef BCM6348_PLL_WORKAROUND
		case kDslTestPllPhaseResult:
			BcmAdslCoreReverbPwrSet(status->param.value);
			break;
#endif
		default:
			break;
	}
	return statLastTick * BCMOS_MSEC_PER_TICK;
}

#ifdef CONFIG_PHY_PARAM
/***************************************************************************
** Function Name: BcmCoreAllocLowMem
** Description  : Allocates memory in the first 16MB of memory for PHY
** Returns      : pointer to allocated memory
***************************************************************************/
void *BcmCoreAllocLowMem(unsigned long size)
{
	if ((NULL != pLowMemAddr) && (lmSize < size)) {
		dma_free_coherent(pXdslDummyDevice, lmSize, pLowMemAddr, lmAddrPhys);
		pLowMemAddr = NULL;
	}
	if (NULL == pLowMemAddr) {
		pLowMemAddr = dma_alloc_coherent(pXdslDummyDevice, size, &lmAddrPhys, GFP_KERNEL);
		DiagStrPrintf(0,DIAG_DSL_CLIENT, "dma_alloc_coherent: pVM=0x%08lX, sz=%ld, pAddr=0x%08lX\n", (ulong) pLowMemAddr , size, (ulong) lmAddrPhys);
		if (NULL == pLowMemAddr)
			return NULL;
#ifdef CONFIG_BCM963148
		if ((lmAddrPhys+size) > 0x01000000) {
			dma_free_coherent(pXdslDummyDevice, size, pLowMemAddr, lmAddrPhys);
			pLowMemAddr = NULL;
			DiagStrPrintf(0,DIAG_DSL_CLIENT, "dma_alloc_coherent: physAddr above 16MB: 0x%08lX\n", (ulong) lmAddrPhys);
			return NULL;
		}
#elif defined(CONFIG_BCM963138)
#define PAD_BLK_NUM    16
#define PAD_BLK_SIZE   0x40000
		if ((lmAddrPhys & 0x00200000) | ((lmAddrPhys+size) & 0x00200000)) {
			void        *padAddr[PAD_BLK_NUM];
			dma_addr_t  padAddrPhys[PAD_BLK_NUM];
			int         i;

			DiagStrPrintf(0,DIAG_DSL_CLIENT, "dma_alloc_coherent: physAddr bit[21] set: 0x%08lX\n", (ulong) lmAddrPhys);
			dma_free_coherent(pXdslDummyDevice, size, pLowMemAddr, lmAddrPhys);
			pLowMemAddr = NULL;
			for (i = 0; i < PAD_BLK_NUM; i++) {
			  padAddr[i] = dma_alloc_coherent(pXdslDummyDevice, PAD_BLK_SIZE, padAddrPhys+i, GFP_KERNEL);
			  DiagStrPrintf(0,DIAG_DSL_CLIENT, "dma_alloc_coherent[%d]: p=0x%08lX, sz=%d, pAddr=0x%08lX\n", i, 
			    (ulong) padAddr[i], PAD_BLK_SIZE, (ulong) padAddrPhys[i]);
			  if (NULL == padAddr[i]) {
				i--;
				break;
			  }
			  if (0 == ((padAddrPhys[i] & 0x00200000) | ((padAddrPhys[i]+size) & 0x00200000))) {
			    pLowMemAddr = padAddr[i];
				lmAddrPhys  = padAddrPhys[i];
				size        = PAD_BLK_SIZE;
				i--;
			    break;
			  }
			}
			DiagStrPrintf(0,DIAG_DSL_CLIENT, "dma_alloc_coherent_%d: pVM=0x%08lX, sz=%ld, pAddr=0x%08lX\n", i+1, (ulong) pLowMemAddr , size, (ulong) lmAddrPhys);
			printk("dma_alloc_coherent_%d: pVM=0x%08lX, sz=%ld, pAddr=0x%08lX\n", i+1, (ulong) pLowMemAddr , size, (ulong) lmAddrPhys);
			while (i >= 0) {
			  dma_free_coherent(pXdslDummyDevice, PAD_BLK_SIZE, padAddr[i], padAddrPhys[i]);
			  i--;
			}
			if (NULL == pLowMemAddr)
			  return NULL;
		}
#endif
		lmSize = size;
	}
	printk("BcmCoreAllocLowMem: physAddr=0x%08lX size=%ld\n", (ulong) lmAddrPhys, lmSize);
	return (void *) lmAddrPhys;
}
#endif /* CONFIG_PHY_PARAM */

/***************************************************************************
** Function Name: BcmCoreInterruptHandler
** Description  : Interrupt service routine that is called when there is an
**                core ADSL PHY interrupt.  Signals event to the task to
**                process the interrupt condition.
** Returns      : 1
***************************************************************************/
#ifndef _NOOS
#ifdef LINUX_KERNEL_2_6
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
LOCAL irqreturn_t BcmCoreInterruptHandler(int irq, void * dev_id)
#else
LOCAL irqreturn_t BcmCoreInterruptHandler(int irq, void * dev_id, struct pt_regs *ptregs)
#endif
#else
UINT32 BcmCoreInterruptHandler (void)
#endif
{
	Bool	bRunTask = AC_FALSE;
	
#ifdef	ADSLDRV_STATUS_PROFILING
	ulong	cycleCnt0, timeElasped;
	cycleCnt0=BcmAdslCoreGetCycleCount();
#endif

	adslCoreIntrCnt++;
#if defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)
	BcmAdslCoreDiagIntrHandler();
#endif

	if(AdslCoreIntHandler())
		bRunTask = AC_TRUE;
#if (!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338)) && defined(__KERNEL__)
	else
	if(BcmCoreDiagZeroCopyStatActive())
		bRunTask = AC_TRUE;
#endif
#ifdef SUPPORT_XDSLDRV_GDB
	else
	if (isGdbOn())
		bRunTask = AC_TRUE;
#endif
#ifdef PHY_BLOCK_TEST
	else
	if (BcmAdslCoreIsTestCommand())
		bRunTask = AC_TRUE;
#else
	else
	if (ADSL_PHY_SUPPORT(kAdslPhyPlayback))
		if (BcmAdslCoreIsTestCommand())
			bRunTask = AC_TRUE;
#endif

#if defined(__KERNEL__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))
	bRunTask = AC_TRUE;
#endif

	if (bRunTask) {
#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
		bcmOsSemGive (irqSemId);
#else
		if(AC_FALSE==dpcScheduled) {
			bcmOsDpcEnqueue(irqDpcId);
			dpcScheduled=AC_TRUE;
#ifdef ADSLDRV_STATUS_PROFILING
			dpcScheduleTimeStamp=cycleCnt0;
#endif
		}
#ifdef SUPPORT_STATUS_BACKUP
		else {
			AdslCoreBkupSbStat();
#ifdef ADSLDRV_STATUS_PROFILING
			intrDuringSchedCnt++;
#endif
		}
#endif /* SUPPORT_STATUS_BACKUP */
#endif	/* Linux */
	}
	
#if !defined(BCM_CORE_TEST) && !defined(KERNEL_UNMASK_DSL_INTR) && !defined(CONFIG_ARM)
	BcmHalInterruptEnable(INTERRUPT_ID_ADSL);
#endif

#ifdef ADSLDRV_STATUS_PROFILING
	timeElasped=BcmAdslCoreCycleTimeElapsedUs(BcmAdslCoreGetCycleCount(), cycleCnt0);
	if(timeElasped < intrDurMin)
		intrDurMin=timeElasped;
	if(timeElasped > intrDurMax) {
		intrDurMax=timeElasped;
		intrDurMaxAtIntrCnt=adslCoreIntrCnt;
	}
	intrDurTotal += timeElasped;
#endif

#ifdef LINUX_KERNEL_2_6
	return IRQ_HANDLED;
#else
	return 1;
#endif
}

/***************************************************************************
** Function Name: BcmCoreDpc
** Description  : Processing of ADSL PHY interrupt 
** Returns      : None.
***************************************************************************/
LOCAL void BcmCoreDpc(void * arg)
{
	int	numStatProc, numStatToProcMax;
	volatile ulong	*pAdslEnum;
#ifdef ADSLDRV_STATUS_PROFILING
	ulong	cycleCnt0, timeElapsed;
#endif

	if( g_nAdslExit == 1 )
		return;
	
#if defined(CONFIG_SMP) && defined(__KERNEL__)
	BcmCoreDpcSyncEnter();
#endif
	pAdslEnum = (ulong *) XDSL_ENUM_BASE;
	if(AC_FALSE == adslCoreStarted) {
		bcmOsSemGive(syncPhyMipsSemId);
#if !defined(__ECOS)
		dpcScheduled=AC_FALSE;
#endif
		/* Disable and clear pending status interrupt */
		pAdslEnum[ADSL_INTMASK_I] &= (~MSGINT_MASK_BIT);
		pAdslEnum[ADSL_INTSTATUS_I] = pAdslEnum[ADSL_INTSTATUS_I];
#if defined(CONFIG_SMP) && defined(__KERNEL__)
		BcmCoreDpcSyncExit();
#endif
#if !defined(BCM_CORE_TEST) && (defined(__KERNEL__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))) && !defined(KERNEL_UNMASK_DSL_INTR) && !defined(CONFIG_ARM)
		BcmHalInterruptEnable(INTERRUPT_ID_ADSL);
#endif
		return;
	}
	/* Disable and clear pending status interrupt */
	pAdslEnum[ADSL_INTMASK_I] &= (~MSGINT_MASK_BIT);
	pAdslEnum[ADSL_INTSTATUS_I] = pAdslEnum[ADSL_INTSTATUS_I];
#ifdef ADSLDRV_STATUS_PROFILING
	cycleCnt0=BcmAdslCoreGetCycleCount();
	timeElapsed=BcmAdslCoreCycleTimeElapsedUs(cycleCnt0, dpcScheduleTimeStamp);
	gByteAvail=0;
#endif
#if defined(ADSLDRV_STATUS_PROFILING) || defined(SUPPORT_STATUS_BACKUP)
	gByteProc=0;
#endif
#if !defined(__ECOS)
	dpcScheduled=AC_FALSE;
#endif /* !defined(__ECOS) */
	
	adslCoreIsrTaskCnt++;

	BcmAdslCoreDiagIsrTask();
#ifdef SUPPORT_XDSLDRV_GDB
	BcmAdslCoreGdbTask();
#endif
#ifdef PHY_BLOCK_TEST
	BcmAdslCoreProcessTestCommand();
#else
	if (ADSL_PHY_SUPPORT(kAdslPhyPlayback))
		BcmAdslCoreProcessTestCommand();
#endif

#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
	if(BcmCoreDiagZeroCopyStatActive())
		BcmCoreDiagZeroCopyStatHandler();
#endif

#if defined(__KERNEL__) && (defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338))
	if( gDiagDataLog )
		numStatToProcMax = 2;
	else
		numStatToProcMax = 0xffff;
#else
	numStatToProcMax = 0xffff;
#endif

	numStatProc=0;
#ifdef SUPPORT_STATUS_BACKUP
processStat:
#endif
#ifdef ADSLDRV_STATUS_PROFILING
	gByteAvail+=AdslCoreStatusAvailTotal();
#endif

	numStatProc += AdslCoreIntTaskHandler(numStatToProcMax);

#ifdef SUPPORT_STATUS_BACKUP
	if(AdslCoreSwitchCurSbStatToSharedSbStat()) {
		DiagWriteString(0, DIAG_DSL_CLIENT, "%s: bkup SB processed at intrCnt=%lu dpcCnt=%lu, byteProc=%d nStatProc=%d shByteAvail=%d\n",
			__FUNCTION__, adslCoreIntrCnt, adslCoreIsrTaskCnt, gByteProc, numStatProc, AdslCoreStatusAvailTotal());
#ifdef ADSLDRV_STATUS_PROFILING
		BcmXdslCoreDrvProfileInfoPrint();
#endif
		goto processStat;
	}
#endif

	pAdslEnum[ADSL_INTSTATUS_I] = pAdslEnum[ADSL_INTSTATUS_I];	/* Clear pending status interrupt, and enable it */
	pAdslEnum[ADSL_INTMASK_I] |= MSGINT_MASK_BIT;

#if defined( __KERNEL__)
	if( AdslCoreStatusAvail()
#if defined(PHY_BLOCK_TEST)
		|| BcmAdslCoreIsTestCommand()
#endif
#if (!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
		|| BcmCoreDiagZeroCopyStatAvail()
#endif
		)
		bcmOsDpcEnqueue(irqDpcId);
#endif
#if !defined(BCM_CORE_TEST) && (defined(__KERNEL__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0))) && !defined(KERNEL_UNMASK_DSL_INTR) && !defined(CONFIG_ARM)
	BcmHalInterruptEnable(INTERRUPT_ID_ADSL);
#endif

#ifdef ADSLDRV_STATUS_PROFILING
	if(timeElapsed < dpcDelayMin)
		dpcDelayMin=timeElapsed;
	if(timeElapsed > dpcDelayMax) {
		dpcDelayMax=timeElapsed;
		dpcDelayMaxAtDpcCnt=adslCoreIsrTaskCnt;
		dpcDelayMaxAtByteAvail=gByteAvail;
	}
	
	if(numStatProc > nStatProcMax) {
		nStatProcMax = numStatProc;
		nStatProcMaxAtDpcCnt=adslCoreIsrTaskCnt;
		nStatProcMaxAtByteAvail=gByteAvail;
		nStatProcMaxAtByteProc=gByteProc;
	}
	
	if(gByteAvail > byteAvailMax) {
		byteAvailMaxAtDpcCnt=adslCoreIsrTaskCnt;
		byteAvailMaxAtDpcDelay=timeElapsed;
		byteAvailMaxAtNumStatProc=numStatProc;
		byteAvailMaxAtByteProc=gByteProc;
	}
	
	if(gByteProc > byteProcMax) {
		byteProcMaxAtDpcCnt=adslCoreIsrTaskCnt;
		byteProcMaxAtDpcDelay=timeElapsed;
		byteProcMaxAtNumStatProc=numStatProc;
	}
	dpcDelayTotal += timeElapsed;
	
	timeElapsed=BcmAdslCoreCycleTimeElapsedUs(BcmAdslCoreGetCycleCount(), cycleCnt0);
	/* timeElapsed = dpcDuration */
	if(gByteAvail > byteAvailMax) {
		byteAvailMax=gByteAvail;
		byteAvailMaxAtDpcDur=timeElapsed;
	}
	
	if(gByteProc > byteProcMax) {
		byteProcMax = gByteProc;
		byteProcMaxAtDpcDur=timeElapsed;
	}
	
	if(timeElapsed < dpcDurMin)
		dpcDurMin=timeElapsed;
	if(timeElapsed > dpcDurMax) {
		dpcDurMax=timeElapsed;
		dpcDurMaxAtDpcCnt=adslCoreIsrTaskCnt;
		dpcDurMaxAtByteAvail=gByteAvail;
	}
	
	dpcDurTotal += timeElapsed;
#endif
#if defined(CONFIG_SMP) && defined(__KERNEL__)
	BcmCoreDpcSyncExit();
#endif
}
#else
LOCAL void BcmCoreDpc(void * arg)
{
}
#endif /* !_NOOS */

#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
/***************************************************************************
** Function Name: BcmCoreIsrTask
** Description  : Runs in a separate thread of execution. Returns from blocking
**                on an event when an core ADSL PHY interrupt.  
** Returns      : None.
***************************************************************************/
LOCAL void BcmCoreIsrTask(void)
{
	while (TRUE) {
		bcmOsSemTake(irqSemId, OS_WAIT_FOREVER);

        if( g_nAdslExit == 1 )
            break;

		BcmCoreDpc(NULL);
	}
}
#endif

#if defined(G992P3) || defined(G992P5)
int BcmAdslCoreLogSaveAdsl2Capabilities(long *cmdData, g992p3DataPumpCapabilities *pCap)
{
	cmdData[0]  = (long)pCap->rcvNTREnabled;
	cmdData[1]  = (long)pCap->shortInitEnabled;
	cmdData[2]  = (long)pCap->diagnosticsModeEnabled;
	cmdData[3]  = (long)pCap->featureSpectrum;
	cmdData[4]  = (long)pCap->featureOverhead;
	cmdData[5]  = (long)pCap->featureTPS_TC[0];
	cmdData[6]  = (long)pCap->featurePMS_TC[0];
	cmdData[7]  = (long)pCap->featureTPS_TC[1];
	cmdData[8]  = (long)pCap->featurePMS_TC[1];
	cmdData[9]  = (long)pCap->featureTPS_TC[2];
	cmdData[10] = (long)pCap->featurePMS_TC[2];
	cmdData[11] = (long)pCap->featureTPS_TC[3];
	cmdData[12] = (long)pCap->featurePMS_TC[3];
	cmdData[13] = (long)pCap->readsl2Upstream;
	cmdData[14] = (long)pCap->readsl2Downstream;
	cmdData[15] = (long)pCap->sizeIDFT;
	cmdData[16] = (long)pCap->fillIFFT;
	cmdData[17] = (long)pCap->minDownOverheadDataRate;
	cmdData[18] = (long)pCap->minUpOverheadDataRate;
	cmdData[19] = (long)pCap->maxDownATM_TPSTC;
	cmdData[20] = (long)pCap->maxUpATM_TPSTC;
	cmdData[21] = (long)pCap->minDownATM_TPS_TC[0];
	cmdData[22] = (long)pCap->maxDownATM_TPS_TC[0];
	cmdData[23] = (long)pCap->minRevDownATM_TPS_TC[0];
	cmdData[24] = (long)pCap->maxDelayDownATM_TPS_TC[0];
	cmdData[25] = (long)pCap->maxErrorDownATM_TPS_TC[0];
	cmdData[26] = (long)pCap->minINPDownATM_TPS_TC[0];
	cmdData[27] = (long)pCap->minUpATM_TPS_TC[0];
	cmdData[28] = (long)pCap->maxUpATM_TPS_TC[0];
	cmdData[29] = (long)pCap->minRevUpATM_TPS_TC[0];
	cmdData[30] = (long)pCap->maxDelayUpATM_TPS_TC[0];
	cmdData[31] = (long)pCap->maxErrorUpATM_TPS_TC[0];
	cmdData[32] = (long)pCap->minINPUpATM_TPS_TC[0];
	cmdData[33] = (long)pCap->maxDownPMS_TC_Latency[0];
	cmdData[34] = (long)pCap->maxUpPMS_TC_Latency[0];
	return 35;
}
#endif

#ifdef G993
int BcmAdslCoreLogSaveVdsl2Capabilities(long *cmdData, g993p2DataPumpCapabilities *pCap)
{
	cmdData[0]  = (long)pCap->verId;
	cmdData[1]  = (long)pCap->size;
	cmdData[2]  = (long)pCap->profileSel;
	cmdData[3]  = (long)pCap->maskUS0;
	cmdData[4]  = (long)pCap->cfgFlags;
	cmdData[5]  = (long)pCap->maskEU;
	cmdData[6]  = (long)pCap->maskADLU;
	return 7;
}
#endif

void BcmAdslCoreLogWriteConnectionParam(dslCommandStruct *pDslCmd)
{
	long	cmdData[120];	/* Worse case was 100 ulongs */
	int		n;

	if (0 == (n = AdslCoreFlattenCommand(pDslCmd, cmdData, sizeof(cmdData))))
		return;
	n >>= 2;

#ifdef	G993
	if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2))
		n--;		/* undo the pointer to carrierInfoG993p2AnnexA */
#endif

#ifdef	G992P3
#ifdef	G992P5
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2p))
		n--;		/* undo the pointer to carrierInfoG992p5AnnexA */
#endif
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2))
		n--;		/* undo the pointer to carrierInfoG992p3AnnexA */
#endif

#ifdef	G992P3
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2))
		n += BcmAdslCoreLogSaveAdsl2Capabilities(cmdData+n, pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p3AnnexA);
#endif
#ifdef	G992P5
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2p))
		n += BcmAdslCoreLogSaveAdsl2Capabilities(cmdData+n, pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA);
#endif

#ifdef	G993
	if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2))
		n += BcmAdslCoreLogSaveVdsl2Capabilities(cmdData+n, pDslCmd->param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA);
#endif

	BcmAdslCoreDiagWriteLog(commandInfoData, cmdData, n);
}

/***************************************************************************
** Function Name: BcmAdslCoreSetConnectionParam
** Description  : Sets default connection parameters
** Returns      : None.
***************************************************************************/

#if defined(G992P3) || defined(G992P5)
LOCAL void SetAdsl2Caps(Boolean bP5Cap, dslCommandStruct *pDslCmd, adslCfgProfile *pAdslCfg)
{
	g992p3DataPumpCapabilities	*pG992p3Cap;
	ulong						phyCfg, modCfg;
	ulong						adsl2Cfg;

	pG992p3Cap = bP5Cap ? 
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA : 
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p3AnnexA;

	if (NULL == pAdslCfg) {
		phyCfg = 0;
		modCfg = kAdslCfgModAny;
		adsl2Cfg = 0;
	}
	else {
		phyCfg = pAdslCfg->adslAnnexAParam;
		modCfg = phyCfg & kAdslCfgModMask;
		adsl2Cfg = pAdslCfg->adsl2Param;
	}

	if (kAdslCfgModAny == (modCfg & kAdslCfgModMask)) {
		modCfg = kAdslCfgModMask;	/* all enabled */
		adsl2Cfg |= kAdsl2CfgReachExOn;
	}

	if (bP5Cap) {
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.downstreamMinCarr = 33;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.downstreamMaxCarr = 511;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.upstreamMinCarr = 6;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.upstreamMaxCarr = 31;
#ifdef G992P1_ANNEX_B
		if (ADSL_PHY_SUPPORT(kAdslPhyAnnexB)) {
#ifdef DTAG_UR2
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.upstreamMinCarr = 33;
#endif
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5.upstreamMaxCarr = 59;
		}
#endif
	}
	
	if (modCfg & kAdslCfgModAdsl2Only)
		pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p3AnnexA | kG994p1;
	
	if (bP5Cap && (modCfg & kAdslCfgModAdsl2pOnly))
		pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p5AnnexA | kG994p1;
	
	pG992p3Cap->rcvNTREnabled = 0;
	pG992p3Cap->shortInitEnabled = 0;
	pG992p3Cap->diagnosticsModeEnabled = 0;
	pG992p3Cap->featureSpectrum = 0x10;
	pG992p3Cap->featureOverhead = 0x0f;
	
	if (0 == (phyCfg & kAdslCfgTpsTcMask))
		pG992p3Cap->featureTPS_TC[0] = kG994p1G992p3AnnexADownATM_TPS_TC | kG994p1G992p3AnnexAUpATM_TPS_TC;
	else {
		pG992p3Cap->featureTPS_TC[0] = 0;
		if (phyCfg & kAdslCfgTpsTcAtmAdsl)
			pG992p3Cap->featureTPS_TC[0] |= kG994p1G992p3AnnexADownATM_TPS_TC | kG994p1G992p3AnnexAUpATM_TPS_TC;
		if (phyCfg & kAdslCfgTpsTcPtmAdsl)
			pG992p3Cap->featureTPS_TC[0] |= kG994p1G992p3AnnexADownPTM_TPS_TC | kG994p1G992p3AnnexAUpPTM_TPS_TC;
	}
	if (phyCfg & kAdslCfgTpsTcPtmPreMask)
		pDslCmd->param.dslModeSpec.capabilities.auxFeatures |= kDslPTMPreemptionDisable;

	pG992p3Cap->featurePMS_TC[0] = 0x03;
	pG992p3Cap->featureTPS_TC[1] = 0;
	pG992p3Cap->featurePMS_TC[1] = 0;
	pG992p3Cap->featureTPS_TC[2] = 0;
	pG992p3Cap->featurePMS_TC[2] = 0;
	pG992p3Cap->featureTPS_TC[3] = 0;
	pG992p3Cap->featurePMS_TC[3] = 0;
		
#ifdef	READSL2
	if (ADSL_PHY_SUPPORT(kAdslPhyAdslReAdsl2) && (modCfg & kAdslCfgModAdsl2Only) && (adsl2Cfg & kAdsl2CfgReachExOn)) {
		pG992p3Cap->featureSpectrum |= kG994p1G992p3AnnexLReachExtended;
		if (0 == (adsl2Cfg & kAdsl2CfgAnnexLMask))
			adsl2Cfg |= kAdsl2CfgAnnexLUpWide | kAdsl2CfgAnnexLUpNarrow;
		pG992p3Cap->readsl2Upstream   = 0;
		pG992p3Cap->readsl2Downstream = kG994p1G992p3AnnexLDownNonoverlap;
		if (adsl2Cfg & kAdsl2CfgAnnexLUpWide)
			pG992p3Cap->readsl2Upstream |= kG994p1G992p3AnnexLUpWideband;
		if (adsl2Cfg & kAdsl2CfgAnnexLUpNarrow)
			pG992p3Cap->readsl2Upstream |= kG994p1G992p3AnnexLUpNarrowband;
		if (adsl2Cfg & kAdsl2CfgAnnexLDnOvlap)
			pG992p3Cap->readsl2Downstream &= ~kG994p1G992p3AnnexLDownNonoverlap;
	}
#endif

	pG992p3Cap->sizeIDFT = bP5Cap ? 9 : 8;
	pG992p3Cap->fillIFFT = 2;
	pG992p3Cap->minDownOverheadDataRate = 3;
	pG992p3Cap->minUpOverheadDataRate = 3;
	pG992p3Cap->maxDownATM_TPSTC = 1;
	pG992p3Cap->maxUpATM_TPSTC = 1;
	pG992p3Cap->minDownATM_TPS_TC[0] = 1;
#if !defined(_CFE_) && !defined(__ECOS) && !defined(_NOOS)
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)) && \
	(defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)))
#if (defined(CONFIG_BCM_ATM_BONDING_ETH) || defined(CONFIG_BCM_ATM_BONDING_ETH_MODULE)) && !defined(NO_BITRATE_LIMIT)
	if (atmbonding_enabled == 1)
		pG992p3Cap->maxDownATM_TPS_TC[0] = bP5Cap ? 2600 : 3800;
	else
		pG992p3Cap->maxDownATM_TPS_TC[0] = 4095;
#else
	pG992p3Cap->maxDownATM_TPS_TC[0] = 4095;
#endif
#else
	pG992p3Cap->maxDownATM_TPS_TC[0] = 4095;
#endif
#else
	pG992p3Cap->maxDownATM_TPS_TC[0] = 4095;
#endif
	pG992p3Cap->minRevDownATM_TPS_TC[0] = 8;
	pG992p3Cap->maxDelayDownATM_TPS_TC[0] = 32;
	pG992p3Cap->maxErrorDownATM_TPS_TC[0] = 2;
	pG992p3Cap->minINPDownATM_TPS_TC[0] = 0;
	pG992p3Cap->minUpATM_TPS_TC[0] = 1;
	pG992p3Cap->maxUpATM_TPS_TC[0] = 2000;
	pG992p3Cap->minRevUpATM_TPS_TC[0] = 8;
	pG992p3Cap->maxDelayUpATM_TPS_TC[0] = 20;
	pG992p3Cap->maxErrorUpATM_TPS_TC[0] = 2;
	pG992p3Cap->minINPUpATM_TPS_TC[0] = 0;
	pG992p3Cap->maxDownPMS_TC_Latency[0] = 4095;
	pG992p3Cap->maxUpPMS_TC_Latency[0] = 4095;
}
#endif /* defined(G992P3) || defined(G992P5) */

ulong BcmXdslCoreGetPhyExtraCfg(unsigned char lineId, unsigned char phyExtraCfgNum)
{
	dslExtraCfgCommand  *pExtraCfg;
	
	pExtraCfg = (void *) ((char *)&adslCoreCfgCmd[lineId].param + sizeof(adslCoreCfgCmd[lineId].param.dslClearEocMsg));
	
	return pExtraCfg->phyExtraCfg[phyExtraCfgNum&3];
}

void BcmXdslCoreSetPhyExtraCfg(unsigned char lineId, unsigned char phyExtraCfgNum, ulong phyExtraCfg)
{
	dslExtraCfgCommand  *pExtraCfg;
	
	pExtraCfg = (void *) ((char *)&adslCoreCfgCmd[lineId].param + sizeof(adslCoreCfgCmd[lineId].param.dslClearEocMsg));
	
	pExtraCfg->phyExtraCfg[phyExtraCfgNum&3] |= phyExtraCfg;
}

void BcmXdslCoreClearPhyExtraCfg(unsigned char lineId, unsigned char phyExtraCfgNum, ulong phyExtraCfg)
{
	dslExtraCfgCommand  *pExtraCfg;
	
	pExtraCfg = (void *) ((char *)&adslCoreCfgCmd[lineId].param + sizeof(adslCoreCfgCmd[lineId].param.dslClearEocMsg));
	
	pExtraCfg->phyExtraCfg[phyExtraCfgNum&3] &= ~phyExtraCfg;
}

#ifdef CONFIG_VDSL_SUPPORTED
LOCAL void BcmAdslCoreUpdateConnectionParamBasedOnXtmFeatures(unsigned char lineId)
{
	XTM_INTERFACE_CFG	interfaceCfg;
	ushort		*pUsIfSupportedTrafficTypes = NULL;
	g993p2DataPumpCapabilities	 *pG993p2Cap =NULL;

	if(XTMSTS_SUCCESS == BcmXtm_GetInterfaceCfg(PORT_PHY0_PATH0, &interfaceCfg))
		pUsIfSupportedTrafficTypes = (ushort *)((int)&interfaceCfg.ulIfLastChange + 4);
	else
		return;

	if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2))
	{
		pG993p2Cap = adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA;
		if((*pUsIfSupportedTrafficTypes & SUPPORT_TRAFFIC_TYPE_PTM_RAW) != 0)
			pG993p2Cap->cfgFlags |= CfgFlagsRawEthernetDS;
		else
			pG993p2Cap->cfgFlags &= ~CfgFlagsRawEthernetDS;
		if(adslCoreCfgProfile[lineId].vdslCfgFlagsMask & CfgFlagsRawEthernetDS)
			pG993p2Cap->cfgFlags &= ~((adslCoreCfgProfile[lineId].vdslCfgFlagsMask & CfgFlagsRawEthernetDS) & ~(adslCoreCfgProfile[lineId].vdslCfgFlagsValue));
	}
	
#if defined(SUPPORT_DSL_BONDING)
	if(
#if !defined(SUPPORT_MULTI_PHY)
		ADSL_PHY_SUPPORT(kAdslPhyBonding)
#elif defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
		!((adslCoreCfgProfile[lineId].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) &&
		  (BCM_IMAGETYPE_SINGLELINE==(adslCoreCfgProfile[lineId].xdslMiscCfgParam & BCM_IMAGETYPE_MSK)))
#else
		!(!ADSL_PHY_SUPPORT(kAdslPhyBonding) && (adslCoreCfgProfile[lineId].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED))
#endif
	) {
		if( (*pUsIfSupportedTrafficTypes & SUPPORT_TRAFFIC_TYPE_PTM_BONDED) != 0 )
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures |= kDslPtmBondingSupported;
		else
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures &= ~kDslPtmBondingSupported;
		
		if( (*pUsIfSupportedTrafficTypes & SUPPORT_TRAFFIC_TYPE_ATM_BONDED) != 0 )
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures |= kDslAtmBondingSupported;
		else
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures &= ~kDslAtmBondingSupported;
		
		if(adslCoreCfgProfile[lineId].xdslAuxFeaturesMask & (kDslPtmBondingSupported | kDslAtmBondingSupported))
		{
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures &=
				~((adslCoreCfgProfile[lineId].xdslAuxFeaturesMask & (kDslPtmBondingSupported | kDslAtmBondingSupported)) &~(adslCoreCfgProfile[lineId].xdslAuxFeaturesValue));
		}
	}
	else
	{
		/* If can not obtain upperlayer feature info, assume they are not supported */
		adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures &= ~kDslPtmBondingSupported;
		adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures &= ~kDslAtmBondingSupported;
	}

#ifdef  SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED
	if( (*pUsIfSupportedTrafficTypes & (SUPPORT_TRAFFIC_TYPE_PTM_BONDED | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED)) == (SUPPORT_TRAFFIC_TYPE_PTM_BONDED | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED) )
	{
		if((adslCoreCfgProfile[lineId].xdslCfg2Mask & kPhyCfg2TxPafEnabled) && !(adslCoreCfgProfile[lineId].xdslCfg2Value& kPhyCfg2TxPafEnabled))
			BcmXdslCoreClearPhyExtraCfg(lineId, 1, kPhyCfg2TxPafEnabled);
		else
			BcmXdslCoreSetPhyExtraCfg(lineId, 1, kPhyCfg2TxPafEnabled);
	}
#endif
#endif	/* defined(SUPPORT_DSL_BONDING) */
}
#endif /* #ifdef CONFIG_VDSL_SUPPORTED */

LOCAL void BcmAdslCoreSetConnectionParam(unsigned char lineId, dslCommandStruct *pDslCmd, dslCommandStruct *pCfgCmd, adslCfgProfile *pAdslCfg)
{
	ulong	phyCfg, modCfg;
#ifdef CONFIG_VDSL_SUPPORTED
	g993p2DataPumpCapabilities	*pG993p2Cap = NULL;
#endif

	if (NULL != pAdslCfg) {
		memcpy((void *)&adslCoreCfgProfile[lineId], (void *)pAdslCfg, sizeof(adslCfgProfile));
#ifdef CONFIG_VDSL_SUPPORTED
		BcmAdslCoreDiagWriteStatusString (lineId, "Line %d: AnnexCParam=0x%08X AnnexAParam=0x%08X adsl2=0x%08X vdslParam=0x%08X vdslParam1=0x%08X\n",
			lineId, (uint)pAdslCfg->adslAnnexCParam, (uint)pAdslCfg->adslAnnexAParam, (uint) pAdslCfg->adsl2Param,
			(uint)pAdslCfg->vdslParam, (uint)pAdslCfg->vdslParam1);
#else
		BcmAdslCoreDiagWriteStatusString (lineId, "AnnexCParam=0x%08X AnnexAParam=0x%08X adsl2=0x%08X\n",
			(uint)pAdslCfg->adslAnnexCParam, (uint)pAdslCfg->adslAnnexAParam, (uint) pAdslCfg->adsl2Param);
#endif
	}
	else
		BcmAdslCoreDiagWriteStatusString (lineId, "BcmAdslCoreSetConnectionParam: no profile\n");

	if (NULL != pAdslCfg) {
#ifdef G992_ANNEXC
		phyCfg = pAdslCfg->adslAnnexCParam;
#else
		phyCfg = pAdslCfg->adslAnnexAParam;
#endif
		adslCoreHsModeSwitchTime[lineId] = pAdslCfg->adslHsModeSwitchTime;
		modCfg = phyCfg & kAdslCfgModMask;
	}
	else {
		phyCfg = 0;
		modCfg = kAdslCfgModAny;
	}
	
#ifdef SUPPORT_DSL_BONDING
	pDslCmd->command = kDslStartPhysicalLayerCmd | (lineId << DSL_LINE_SHIFT);
#else
	pDslCmd->command = kDslStartPhysicalLayerCmd;
#endif
	pDslCmd->param.dslModeSpec.direction = kATU_R;
	pDslCmd->param.dslModeSpec.capabilities.modulations = kG994p1;
	pDslCmd->param.dslModeSpec.capabilities.minDataRate = 1;
	pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 48;
	pDslCmd->param.dslModeSpec.capabilities.features = 0;
	pDslCmd->param.dslModeSpec.capabilities.auxFeatures = (0x00
				| kG994p1PreferToExchangeCaps
				| kG994p1PreferToDecideMode
				| kDslGinpDsSupported
				| kDslGinpUsSupported
				);
	pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 = 0;

	if ((NULL != pAdslCfg) && (phyCfg & kAdslCfgPwmSyncClockOn))
		AfePwmSetSyncClockFreq(pDslCmd->param.dslModeSpec.capabilities.auxFeatures, pAdslCfg->adslPwmSyncClockFreq);

	pDslCmd->param.dslModeSpec.capabilities.demodCapabilities = (
				kSoftwareTimeErrorDetectionEnabled |
				kHardwareAGCEnabled |
				kDigitalEchoCancellorEnabled |
				kHardwareTimeTrackingEnabled);

	if((phyCfg & kAdslCfgDemodCapMask) || (phyCfg & kAdslCfgDemodCap2Mask))
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |=  kDslBitSwapEnabled;

	if (phyCfg & kAdslCfgExtraMask) {
		if (kAdslCfgTrellisOn == (phyCfg & kAdslCfgTrellisMask))
			pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslTrellisEnabled;
		if (0 == (phyCfg & kAdslCfgTrellisMask))
			pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslTrellisEnabled;
		if (kAdslCfgLOSMonitoringOff == (phyCfg & kAdslCfgLOSMonitoringMask))
			pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslAutoRetrainDisabled;

		pDslCmd->param.dslModeSpec.capabilities.noiseMargin = 
			(kAdslCfgDefaultTrainingMargin != pAdslCfg->adslTrainingMarginQ4) ? 
				pAdslCfg->adslTrainingMarginQ4 : 0x60;
		AdslCoreSetShowtimeMarginMonitoring(
			lineId,
			phyCfg & kAdslCfgMarginMonitoringOn ? AC_TRUE : AC_FALSE,
			pAdslCfg->adslShowtimeMarginQ4,
			pAdslCfg->adslLOMTimeThldSec);
	}
	else {
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslTrellisEnabled;
		pDslCmd->param.dslModeSpec.capabilities.noiseMargin = 0x60;
	}

#ifdef G992P1

#ifdef G992P1_ANNEX_A
	if (ADSL_PHY_SUPPORT(kAdslPhyAnnexA)) {
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslG994AnnexAMultimodeEnabled;
		pDslCmd->param.dslModeSpec.capabilities.modulations &= ~(kG994p1 | kT1p413);
		if (kAdslCfgModAny == (modCfg & kAdslCfgModMask)) {
#ifdef CONFIG_VDSL_SUPPORTED
			if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2) || ADSL_PHY_SUPPORT(kAdslPhyBonding))
				modCfg = kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly;
			else
				modCfg = kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly | kAdslCfgModT1413Only; /* 6367-ADSL only */
#else
			modCfg = kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly | kAdslCfgModT1413Only;
#endif
		}
#ifdef G994P1
		if (modCfg & (kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly))
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG994p1;
		if (modCfg & kAdslCfgModGdmtOnly)
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p1AnnexA;
#endif

#ifdef T1P413
		if (ADSL_PHY_SUPPORT(kAdslPhyT1P413) && (modCfg & kAdslCfgModT1413Only))
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kT1p413 /* | kG992p1AnnexA */;
#endif
		pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 255;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1.downstreamMinCarr = 33;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1.downstreamMaxCarr = 254;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1.upstreamMinCarr = 6;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1.upstreamMaxCarr = 31;
		pDslCmd->param.dslModeSpec.capabilities.features |= (kG992p1ATM | kG992p1RACK1);
		pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 400;
		pDslCmd->param.dslModeSpec.capabilities.features |= kG992p1HigherBitRates;
		
		if((phyCfg & kAdslCfgDemodCapMask) || (phyCfg & kAdslCfgDemodCap2Mask))
			pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslAturXmtPowerCutbackEnabled;
		pDslCmd->param.dslModeSpec.capabilities.subChannelInfo = (
					kSubChannelLS0Upstream | kSubChannelASODownstream);
#ifdef G992P1_ANNEX_A_USED_FOR_G992P2
		if (ADSL_PHY_SUPPORT(kAdslPhyG992p2Init)) {
			if (modCfg & kAdslCfgModGliteOnly)
				pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p2AnnexAB;
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p2.downstreamMinCarr = 33;
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p2.downstreamMaxCarr = 126;
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p2.upstreamMinCarr = 6;
			pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p2.upstreamMaxCarr = 31;
			pDslCmd->param.dslModeSpec.capabilities.features |= kG992p2RACK1;
		}
#endif
	}
#endif /* G992P1_ANNEX_A */

#ifdef G992P1_ANNEX_B
	if (ADSL_PHY_SUPPORT(kAdslPhyAnnexB) || ADSL_PHY_SUPPORT(kAdslPhySADSL)) {
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslG994AnnexAMultimodeEnabled;
		if (kAdslCfgModAny == (modCfg & kAdslCfgModMask))
			modCfg = kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly | kAdslCfgModT1413Only;
		pDslCmd->param.dslModeSpec.capabilities.modulations &= ~(kG994p1 | kT1p413);
		if (modCfg & kAdslCfgModGdmtOnly)
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p1AnnexB;
		if (modCfg & (kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly))
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG994p1;
		if (modCfg & kAdslCfgModT1413Only)
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kT1p413;
		pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 255;
#ifdef SADSL_DEFINES
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.downstreamMinCarr = 59;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.downstreamMaxCarr = 254;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMinCarr = 7;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMaxCarr = 58;
#else
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.downstreamMinCarr = 61;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.downstreamMaxCarr = 254;
#ifdef DTAG_UR2
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMinCarr = 33;
#else
		pDslCmd->param.dslModeSpec.capabilities.features |= kG992p1AnnexBUpstreamTones1to32;
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMinCarr = 28;
#endif
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMaxCarr = 59;
#endif
		pDslCmd->param.dslModeSpec.capabilities.features |= (kG992p1AnnexBATM | kG992p1AnnexBRACK1) ; 
#if 1
		pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 400;
		pDslCmd->param.dslModeSpec.capabilities.features |= kG992p1HigherBitRates;
#endif
		if((phyCfg & kAdslCfgDemodCapMask) || (phyCfg & kAdslCfgDemodCap2Mask))
			pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslAturXmtPowerCutbackEnabled;

		pDslCmd->param.dslModeSpec.capabilities.subChannelInfoAnnexB = (
					kSubChannelLS0Upstream | kSubChannelASODownstream);
#ifdef G992P1_ANNEX_A_USED_FOR_G992P2
		if (ADSL_PHY_SUPPORT(kAdslPhyG992p2Init)) {
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p2AnnexAB;
			pDslCmd->param.dslModeSpec.capabilities.features |= kG992p2RACK1;
		}
#endif
	}
#endif	/* G992P1_ANNEX_B */

#ifdef G992_ANNEXC
	if (kAdslCfgModAny == (modCfg & kAdslCfgModMask))
		modCfg = kAdslCfgModGdmtOnly | kAdslCfgModGliteOnly;
	pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p1AnnexC;
	if (modCfg & kAdslCfgModGliteOnly)
		pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p2AnnexC;
	pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 400;
	/* pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 255; */

	if (pAdslCfg && (pAdslCfg->adslAnnexCParam & kAdslCfgCentilliumCRCWorkAroundMask))
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslCentilliumCRCWorkAroundEnabled;
	
#ifndef G992P1_ANNEX_I
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexC.downstreamMinCarr = 33;
#else
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexC.downstreamMinCarr = 6;
#endif

	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexC.downstreamMaxCarr = 254;
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexC.upstreamMinCarr = 6;
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexC.upstreamMaxCarr = 31;
	pDslCmd->param.dslModeSpec.capabilities.features |= (kG992p1ATM | kG992p1RACK1);
	if (pAdslCfg && (kAdslCfgFBM == (pAdslCfg->adslAnnexCParam & kAdslCfgBitmapMask)))
		pDslCmd->param.dslModeSpec.capabilities.features |= kG992p1AnnexCDBM;

	pDslCmd->param.dslModeSpec.capabilities.features |= kG992p1HigherBitRates;
	pDslCmd->param.dslModeSpec.capabilities.subChannelInfoAnnexC = (
				kSubChannelLS0Upstream | kSubChannelASODownstream);
#if defined(ADSLCORE_ENABLE_LONG_REACH) && defined(G992_ANNEXC_LONG_REACH)
	pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= kDslAnnexCPilot48 | kDslAnnexCReverb33_63; 
#endif
#endif /* G992_ANNEXC */

#ifdef G992P1_ANNEX_I
	pDslCmd->param.dslModeSpec.capabilities.auxFeatures &= ~kG994p1PreferToDecideMode;
	pDslCmd->param.dslModeSpec.capabilities.auxFeatures |= kG994p1PreferToMPMode;
	pDslCmd->param.dslModeSpec.capabilities.modulations |= (kG992p1AnnexI>>4);
	/* pDslCmd->param.dslModeSpec.capabilities.modulations &= ~kG992p2AnnexC; */
	pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 1023;
	/* pDslCmd->param.dslModeSpec.capabilities.maxDataRate = 255; */

	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.downstreamMinCarr = 33;
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.downstreamMaxCarr = 511;
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.upstreamMinCarr = 6;
	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.upstreamMaxCarr = 31;
	if (NULL == pAdslCfg)
		modCfg |= kAdslCfgUpstreamMax;

	if (kAdslCfgUpstreamMax == (modCfg & kAdslCfgUpstreamModeMask))
		modCfg |= kAdslCfgUpstreamDouble;

	if (modCfg & kAdslCfgUpstreamDouble)
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.upstreamMaxCarr = 
			(modCfg & kAdslCfgNoSpectrumOverlap) ? 62 : 63;
	else
		pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.upstreamMaxCarr = 31;

	pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.downstreamMinCarr = 
		(modCfg & kAdslCfgNoSpectrumOverlap ? 
					pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p1AnnexI.upstreamMaxCarr + 2 :
					7);

	pDslCmd->param.dslModeSpec.capabilities.subChannelInfoAnnexI = (
				kSubChannelLS0Upstream | kSubChannelASODownstream);
#endif /* G992P1_ANNEX_I */

#ifdef G992P3
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2)) 
		SetAdsl2Caps(false, pDslCmd, pAdslCfg);
#endif /* G992P3 */

#ifdef G992P5
	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2p)) 
		SetAdsl2Caps(true, pDslCmd, pAdslCfg);
#endif /* G992P5 */

#endif /* G992P1 */

#ifdef CONFIG_VDSL_SUPPORTED

	if (NULL == pAdslCfg)
		modCfg = kAdslCfgModAny;
	else
		modCfg = pAdslCfg->adslAnnexAParam & kAdslCfgModMask;
	if (kAdslCfgModAny == (modCfg & kAdslCfgModMask))
		modCfg = kAdslCfgModMask;				/* all enabled */
	
#ifdef CONFIG_BCM_DSL_GFAST
	if (modCfg & kDslCfgModGfastOnly)
		pDslCmd->param.dslModeSpec.capabilities.modulations |= kGfastAnnexA;
#ifndef CONFIG_BCM_DSL_GFASTCOMBO
	if (modCfg & kDslCfgModGfastOnly) {
		if( !xdslCorePhyImageJustSwitch && (0 == lineId) &&
			(!ADSL_PHY_SUPPORT(kAdslPhyGfast) && (NULL != pAdslCfg) &&
			!(pAdslCfg->xdslMiscCfgParam & BCM_PHYSWITCH_DISABLED) &&
			((pAdslCfg->xdslMiscCfgParam & BCM_SAVEPREFERPHY_DISABLED) ||
			!(pAdslCfg->xdslMiscCfgParam & BCM_PREFERREDPHY_FOUND))) ) {
			XdslCoreSetPhyImageType(BCM_PHYTYPE_GFAST >> BCM_PHYTYPE_SHIFT);
			adslCoreResetPending = AC_TRUE;
#if !defined(__ECOS)
			BcmXdsl_RequestIoCtlCallBack();
#endif
		}
	}
	else if( !xdslCorePhyImageJustSwitch && ADSL_PHY_SUPPORT(kAdslPhyGfast) && (NULL != pAdslCfg) &&
		!(pAdslCfg->xdslMiscCfgParam & BCM_PHYSWITCH_DISABLED) && (0 == lineId)) {
		XdslCoreSetPhyImageType(BCM_PHYTYPE_NON_GFAST >> BCM_PHYTYPE_SHIFT);
		if(AC_FALSE == switchGfastToVdslImagePending) {
			BcmAdslCoreConnectionStart(lineId);
			switchGfastToVdslImagePending = AC_TRUE;
		}
	}
	xdslCorePhyImageJustSwitch = AC_FALSE;
#endif /* !CONFIG_BCM_DSL_GFASTCOMBO */
#endif

	if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2)) {
		pG993p2Cap = pDslCmd->param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA;
		if (modCfg & kDslCfgModVdsl2Only)
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG993p2AnnexA;
		
		pG993p2Cap->verId = 0;
		pG993p2Cap->size = sizeof(g993p2DataPumpCapabilities);

		if((NULL != pAdslCfg) && (pAdslCfg->vdslParam & (kVdslUS0Mask | kVdslProfileMask2)) ) {
			pG993p2Cap->profileSel = pAdslCfg->vdslParam & (ADSL_PHY_SUPPORT(kAdslPhyVdslBrcmPriv1) ? kVdslProfileMask2: ADSL_PHY_SUPPORT(kAdslPhyVdsl30a) ? kVdslProfileMask1 : kVdslProfileMask);
			pG993p2Cap->maskUS0 = (pAdslCfg->vdslParam & kVdslUS0Mask) >> kVdslUS0MaskShift;
		}

		pG993p2Cap->maskUS0 &= pG993p2Cap->profileSel;
		pG993p2Cap->cfgFlags = (CfgFlagsDynamicFFeatureDisable | CfgFlagsEnableErrorSamplePacketsCounter);
		
		if (0 != (phyCfg & kAdslCfgTpsTcMask)) {
			if (0 == (phyCfg & kAdslCfgTpsTcAtmVdsl))
				pG993p2Cap->cfgFlags |= CfgFlagsVdslNoAtmSupport;
			if (0 == (phyCfg & kAdslCfgTpsTcPtmVdsl))
				pG993p2Cap->cfgFlags |= CfgFlagsVdslNoPtmSupport;
		}

		if (ADSL_PHY_SUPPORT(kAdslPhyAttnDrAmd1))
			pG993p2Cap->cfgFlags |= CfgFlagsAttnDrAmd1Enabled;
		
#ifdef G992P1_ANNEX_B
		if (ADSL_PHY_SUPPORT(kAdslPhyAnnexB))
			pG993p2Cap->maskEU = 0x00060000;
		else
#endif
			pG993p2Cap->maskEU = 0x000703FF ;

		pG993p2Cap->maskADLU = 0x0 ;
	}
#endif	/* #ifdef CONFIG_VDSL_SUPPORTED */

	if ((NULL != pAdslCfg) && (pAdslCfg->adsl2Param & kAdsl2CfgAnnexMEnabled)) {
		if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMOnly)
			pDslCmd->param.dslModeSpec.capabilities.modulations = 0;
		pDslCmd->param.dslModeSpec.capabilities.modulations |= kG994p1;
		pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 = (pAdslCfg->adsl2Param & kAdsl2CfgAnnexMPsdMask) >> kAdsl2CfgAnnexMPsdShift;
		if(pAdslCfg->adsl2Param & kAdsl2CfgAnnexMpXMask) {
			if (pAdslCfg->adsl2Param & kAdsl2CfgAnnexMp3)
				pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p3AnnexM;
			if (pAdslCfg->adsl2Param & kAdsl2CfgAnnexMp5)
				pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p5AnnexM;
		}
		else
			pDslCmd->param.dslModeSpec.capabilities.modulations |= kG992p3AnnexM | kG992p5AnnexM;
	}

	if((phyCfg & kAdslCfgDemodCapMask) || (phyCfg & kAdslCfgDemodCap2Mask) )
		pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 |= (kDslFireDsSupported | kDsl24kbyteInterleavingEnabled);

	if ((NULL != pAdslCfg) && (phyCfg & kAdslCfgDemodCapMask)) {
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities |= 
			pAdslCfg->adslDemodCapMask & pAdslCfg->adslDemodCapValue;
		pDslCmd->param.dslModeSpec.capabilities.demodCapabilities &= 
			~(pAdslCfg->adslDemodCapMask & ~pAdslCfg->adslDemodCapValue);
	}

	pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 |= adslCoreEcUpdateMask;

	if ((NULL != pAdslCfg) && (phyCfg & kAdslCfgDemodCap2Mask)) {
		pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 |= 
			pAdslCfg->adslDemodCap2Mask & pAdslCfg->adslDemodCap2Value;
		pDslCmd->param.dslModeSpec.capabilities.subChannelInfop5 &= 
			~(pAdslCfg->adslDemodCap2Mask & ~pAdslCfg->adslDemodCap2Value);
	}
	
	if (NULL != pAdslCfg) {
		if(0 != pAdslCfg->xdslAuxFeaturesMask) {
			pDslCmd->param.dslModeSpec.capabilities.auxFeatures |= 
				pAdslCfg->xdslAuxFeaturesMask & pAdslCfg->xdslAuxFeaturesValue;
			pDslCmd->param.dslModeSpec.capabilities.auxFeatures &= 
				~(pAdslCfg->xdslAuxFeaturesMask & ~pAdslCfg->xdslAuxFeaturesValue);
		}
#ifdef G993
		if ((ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2)) && (0 != pAdslCfg->vdslCfgFlagsMask)) {
			pG993p2Cap->cfgFlags |= pAdslCfg->vdslCfgFlagsMask & pAdslCfg->vdslCfgFlagsValue;
			pG993p2Cap->cfgFlags &= ~(pAdslCfg->vdslCfgFlagsMask & ~pAdslCfg->vdslCfgFlagsValue);
		}
#endif
	}
	
	AdslCoreSetSupportedNumTones(lineId);
	
	/* initialize extra configuration command. The code assumes extra cfg. is less that sizeof(dslCommandStruct) */
	if (NULL != pAdslCfg)
	{
		long				*pProfCfg, i;
		dslExtraCfgCommand  *pExtraCfg;
#ifdef CONFIG_BCM_DSL_GFAST
		unsigned short	bpGpioAFELDRelay;
#endif
		pCfgCmd->command = kDslSendEocCommand | (lineId << DSL_LINE_SHIFT);
		pCfgCmd->param.dslClearEocMsg.msgId = kDslExtraPhyCfgCmd;
		pCfgCmd->param.dslClearEocMsg.msgType = 16 | (0 << 16) | kDslClearEocMsgDataVolatile;
		pExtraCfg = (void *) ((char *)&pCfgCmd->param + sizeof(pCfgCmd->param.dslClearEocMsg));
		pCfgCmd->param.dslClearEocMsg.dataPtr = (void *) pExtraCfg;
		pProfCfg = &pAdslCfg->xdslCfg1Mask;
#if defined(SUPPORT_2CHIP_BONDING)
		pExtraCfg->phyExtraCfg[0] = kPhyCfg1ExternalBondingDiscovery;
#if defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
		pExtraCfg->phyExtraCfg[0] |= kPhyCfg1ExternalBondingSlave;
#endif
#else
		pExtraCfg->phyExtraCfg[0] = 0;
#endif
		pExtraCfg->phyExtraCfg[1] = 0;
#ifdef CONFIG_BCM_DSL_GFAST
		if(BP_SUCCESS == BpGetAFELDRelayGpio(&bpGpioAFELDRelay))
			pExtraCfg->phyExtraCfg[1] |= kPhyCfg2EnableGfastVdslMMode;
#endif
#if !defined(CONFIG_BCM963268) && defined(SUPPORT_MULTI_PHY)
		if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIASEARCH_DISABLED)
			pExtraCfg->phyExtraCfg[1] &= ~kPhyCfg2DisablePtmNonBondingConnection;
		else
		if( !((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) &&
			  (BCM_IMAGETYPE_BONDING==(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK))) )
			pExtraCfg->phyExtraCfg[1] |= kPhyCfg2DisablePtmNonBondingConnection;
#endif
		pExtraCfg->phyExtraCfg[2] = 0;
		pExtraCfg->phyExtraCfg[3] = 0;
		/* assumes xdslCfg?Mask/Value are contiguous in adslCfgProfile */
		for (i= 0; i < 4; i++) {
			pExtraCfg->phyExtraCfg[i] |= pProfCfg[0] & pProfCfg[1];
			pExtraCfg->phyExtraCfg[i] &= ~(pProfCfg[0] & ~pProfCfg[1]);
			pProfCfg += 2;
		}
#ifdef CONFIG_VDSL_SUPPORTED
		BcmAdslCoreUpdateConnectionParamBasedOnXtmFeatures(lineId);
#endif
	}
	
#ifdef G993
	if (NULL != pAdslCfg) {
		if( NULL != pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA ) {
			g992p3DataPumpCapabilities	*pG992p5Cap = pDslCmd->param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA;
			if( (pAdslCfg->maxDsDataRateKbps <= 0x3FFFC) && ((pAdslCfg->maxDsDataRateKbps >> 2) > 0) &&
				(pAdslCfg->maxUsDataRateKbps <= 0x3FFFC) && ((pAdslCfg->maxUsDataRateKbps >> 2) > 0) &&
				(pAdslCfg->maxAggrDataRateKbps <= 0x7FFF8) && ((pAdslCfg->maxAggrDataRateKbps >> 3) > 0) ) {
				pG992p5Cap->maxUpPTM_TPS_TC[2] = pAdslCfg->maxUsDataRateKbps >> 2;	/* 4000 BPS */
				pG992p5Cap->maxDownPTM_TPS_TC[2] = pAdslCfg->maxDsDataRateKbps >> 2;
				pG992p5Cap->maxDownPTM_TPS_TC[3] = pAdslCfg->maxAggrDataRateKbps >> 3;	/* 8000 BPS */
			}
			else {
				/* Invalid configuration.  Let PHY use it's default value */
				pG992p5Cap->maxUpPTM_TPS_TC[2] = 0;
				pG992p5Cap->maxDownPTM_TPS_TC[2] = 0;
				pG992p5Cap->maxDownPTM_TPS_TC[3] = 0;
			}
		}
	}
#endif
}

void BcmXdslCoreSendCmd(unsigned char lineId, ulong cmd, ulong value)
{
	dslCommandStruct	dslCmd;

	dslCmd.command = cmd | (lineId << DSL_LINE_SHIFT);
	dslCmd.param.value = value;
	BcmCoreCommandHandlerSync(&dslCmd);
}

/***************************************************************************
** Function Name: BcmAdslCoreConnectionReset
** Description  : Restarts ADSL connection if core is initialized
** Returns      : None.
***************************************************************************/
void BcmAdslCoreConnectionReset(unsigned char lineId)
{
	if (adslCoreConnectionMode[lineId]) {
#if 0
		BcmAdslCoreReset(-1);
#elif defined(_NOOS)
		bcmOsDelay(40);
#else
		BcmAdslCoreConnectionStop(lineId);
		bcmOsDelay(40);
		BcmAdslCoreConnectionStart(lineId);
#endif
	}
}

void BcmAdslCoreSendXmtGainCmd(unsigned char lineId, int gain)
{
	dslCommandStruct	cmd;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslSetXmtGainCmd | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslSetXmtGainCmd;
#endif
	cmd.param.value = gain;
	BcmCoreCommandHandlerSync(&cmd);
}

/**************************************************************************
** Function Name: BcmAdslCoreConfigure
** Description  : This function is called by ADSL driver change ADSL PHY
** Returns      : None.
**************************************************************************/
void BcmAdslCoreConfigure(unsigned char lineId, adslCfgProfile *pAdslCfg)
{
	long	pair;

	if (NULL == pAdslCfg)
		pair = kAdslCfgLineInnerPair;
	else {
#if defined(G992P1_ANNEX_A)
		pair = pAdslCfg->adslAnnexAParam & kAdslCfgLinePairMask;
#elif defined(G992_ANNEXC)
		pair = pAdslCfg->adslAnnexCParam & kAdslCfgLinePairMask;
#else
		pair = kAdslCfgLineInnerPair;
#endif
	}
	BcmAdsl_ConfigureRj11Pair(pair);

#ifdef G992P3
	adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p3AnnexA = &g992p3Param[lineId];
#endif
#ifdef G992P5
	adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA = &g992p5Param[lineId];
#endif
#ifdef G993
	adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA = &g993p2Param[lineId];
#endif
	if (NULL != pAdslCfg) {
		adslCoreCfgProfile[lineId] = *pAdslCfg;
		pAdslCoreCfgProfile[lineId] = &adslCoreCfgProfile[lineId];
	}
	else 
		pAdslCoreCfgProfile[lineId] = NULL;
	
	BcmAdslCoreSetConnectionParam(lineId, &adslCoreConnectionParam[lineId], &adslCoreCfgCmd[lineId], pAdslCfg);
	BcmAdslCoreConnectionReset(lineId);
}


LOCAL int StrCpy(char *dst, char *src)
{
	char	*dst0 = dst;

	while (*src != 0)
		*dst++ = *src++;
	*dst = 0;
	return (dst - dst0);
}

/**************************************************************************
** Function Name: BcmAdslCoreGetVersion
** Description  : Changes ADSL version information
** Returns      : STS_SUCCESS 
**************************************************************************/
void BcmAdslCoreGetVersion(adslVersionInfo *pAdslVer)
{
	adslPhyInfo		*pInfo = AdslCoreGetPhyInfo();
	int				phyVerLen = 0, n;

	pAdslVer->phyMjVerNum = pInfo->mjVerNum;
	pAdslVer->phyMnVerNum = pInfo->mnVerNum;
	
	pAdslVer->phyVerStr[0] = 0;
	if (NULL != pInfo->pVerStr)
		phyVerLen = StrCpy(pAdslVer->phyVerStr, pInfo->pVerStr);
	
	if (ADSL_PHY_SUPPORT(kAdslPhyAnnexA))
		pAdslVer->phyType = kAdslTypeAnnexA;
	else if (ADSL_PHY_SUPPORT(kAdslPhyAnnexB))
		pAdslVer->phyType = kAdslTypeAnnexB;
	else if (ADSL_PHY_SUPPORT(kAdslPhySADSL))
		pAdslVer->phyType = kAdslPhySADSL;
	else if (ADSL_PHY_SUPPORT(kAdslPhyAnnexC))
		pAdslVer->phyType = kAdslTypeAnnexC;
	else
		pAdslVer->phyType = kAdslTypeUnknown;

	pAdslVer->drvMjVerNum = ADSL_DRV_MJ_VERNUM;
	pAdslVer->drvMnVerNum = (ushort) ((ADSL_DRV_MN_VERSYM - 'a') << (16 - 5)) | ADSL_DRV_MN_VERNUM;
	StrCpy(pAdslVer->drvVerStr, ADSL_DRV_VER_STR);
#if 1
	n = StrCpy(pAdslVer->phyVerStr + phyVerLen, ".d");
	StrCpy(pAdslVer->phyVerStr + phyVerLen + n, pAdslVer->drvVerStr);
#endif
}

/**************************************************************************
** Function Name: BcmAdslCoreInit
** Description  : This function is called by ADSL driver to init core
**                ADSL PHY
** Returns      : None.
**************************************************************************/
unsigned char ctlmG994p1ControlXmtNonStdInfoMsg[] = {
    0x02, 0x12, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x50, 0xC0, 0xA8, 0x01, 0x01,
    0x03, 0x0E, 0x00, 0x00, 0x0E, 0xB5, 0x00, 0x50,
    0x00, 0x00, 0x00, 0x03, 0x02, 0x01, 0x07, 0x02,
    0x09, 
#if defined(ADSLCORE_ENABLE_LONG_REACH) && defined(G992_ANNEXC_LONG_REACH)
	0x01,
#else
	0x00,
#endif
	0x00
};

void BcmAdslCoreInit(void)
{
	int	diagMap;
	int	i;
	volatile ulong	*pAdslEnum = (ulong *) XDSL_ENUM_BASE;
#if defined(CONFIG_SMP) && defined(__KERNEL__) && defined(SPINLOCK_CALL_STAT)
	printk("spinLockCalledEnter=0x%08X\n", (int)spinLockCalledEnter);
	printk("spinLockNestedCallEnter=0x%08X\n", (int)spinLockNestedCallEnter);
	printk("spinLockCalledExit=0x%08X\n", (int)spinLockCalledExit);
	printk("spinLockNestedCallExit=0x%08X\n", (int)spinLockNestedCallExit);
#endif

	BcmAdslCoreCalibrate();

	for(i = 0; i < MAX_DSL_LINE; i++) {
		adslCoreConnectionMode[i] = AC_FALSE;
		acPendingEvents[i] = 0;
	}
#ifdef SUPPORT_EXT_DSL_BONDING_MASTER
	adslCoreConnectionMode[1] = AC_TRUE;
#endif
	adslCoreMuteDiagStatus = AC_FALSE;
	
	bcmOsGetTime(&statLastTick);
	
#ifdef ADSLDRV_STATUS_PROFILING
	printTicks=statLastTick;
#endif

#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
	bcmOsSemCreate(NULL, &irqSemId);
#if defined(TARG_OS_RTEMS)
	bcmOsTaskCreate("DSLC", 20*1024, 255, BcmCoreIsrTask, 0, &IrqTid);
#else
	bcmOsTaskCreate("BcmCoreIrq", 20*1024, 6, BcmCoreIsrTask, 0, &IrqTid);
#endif
#else /* Linux */
	irqDpcId = bcmOsDpcCreate(BcmCoreDpc, NULL);
	if( OS_STATUS_OK != bcmOsSemCreate(NULL, &syncPhyMipsSemId))
		AdslDrvPrintf(TEXT("%s: bcmOsSemCreate for syncPhyMipsSemId Fail!!!\n"), __FUNCTION__);
#if defined(CONFIG_PHY_PARAM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	if(NULL == pXdslDummyDevice) {
		pXdslDummyDevice = kzalloc(sizeof(struct device), GFP_KERNEL);
		if(NULL != pXdslDummyDevice)
			dma_set_coherent_mask(pXdslDummyDevice, DMA_BIT_MASK(24));
		else
			AdslDrvPrintf(TEXT("%s: kzalloc for pXdslDummyDevice Fail!!!\n"), __FUNCTION__);
	}
#endif
#endif

#if !defined(BCM_CORE_TEST) && !defined(_NOOS)
#if !defined(__ECOS)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define FN_PARAM    ((void *)0)
#else
#define FN_PARAM    (0)
#endif
#ifndef KERNEL_UNMASK_DSL_INTR
#if (LINUX_FW_VERSION >= 410)
	BcmHalMapInterruptEx((void*)BcmCoreInterruptHandler, FN_PARAM, INTERRUPT_ID_ADSL,
	                     "dsl", INTR_REARM_NO, INTR_AFFINITY_DEFAULT);
#else
	BcmHalMapInterrupt((void*)BcmCoreInterruptHandler, FN_PARAM, INTERRUPT_ID_ADSL);
#endif
#else /* KERNEL_UNMASK_DSL_INTR */
	BcmHalMapInterruptEx((void*)BcmCoreInterruptHandler, FN_PARAM, INTERRUPT_ID_ADSL,
	                     "dsl", INTR_REARM_YES, INTR_AFFINITY_DEFAULT);
#endif
#endif /* !defined(__ECOS) */
#ifndef CONFIG_ARM
	BcmHalInterruptEnable (INTERRUPT_ID_ADSL);
#endif
#endif

#if !defined(CONFIG_BCM96348) && !defined(USE_PMC_API)
	PERF->blkEnables |= XDSL_CLK_EN;
#endif
	
	diagMap = BcmAdslDiagTaskInit();
	BcmCoreAtmVcInit();
	
	if (NULL == AdslCoreGetStatusCallback())
		AdslCoreSetStatusCallback(BcmAdslCoreStatusSnooper);

	adslCoreResetPending = AC_FALSE;

	AdslCoreSetTime(statLastTick * BCMOS_MSEC_PER_TICK);
	
#if defined(__kerSysGetAfeId) && !defined(_NOOS)
	kerSysGetAfeId((UINT32 *)nvramAfeId);
#endif

#if defined(CONFIG_BCM_DSL_GFAST)
#ifndef CONFIG_BCM_DSL_GFASTCOMBO
	if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PHYSWITCH_DISABLED)
		XdslCoreSetPhyImageType((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PHYTYPE_MSK) >> BCM_PHYTYPE_SHIFT);
	else if(!(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERPHY_DISABLED) &&
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDPHY_FOUND))
		XdslCoreSetPhyImageType(BCM_PHYTYPE_NON_GFAST >> BCM_PHYTYPE_SHIFT);
	else if((adslCoreCfgProfile[0].adslAnnexAParam & kDslCfgModGfastOnly) ||
		((kAdslCfgModAny == (adslCoreCfgProfile[0].adslAnnexAParam & kAdslCfgModMask)) &&
		!(adslCoreCfgProfile[0].adsl2Param & kAdsl2CfgAnnexMOnly)))
		XdslCoreSetPhyImageType(BCM_PHYTYPE_GFAST >> BCM_PHYTYPE_SHIFT);
	else
		XdslCoreSetPhyImageType(BCM_PHYTYPE_NON_GFAST >> BCM_PHYTYPE_SHIFT);
#endif /* !CONFIG_BCM_DSL_GFASTCOMBO */
#if defined(SUPPORT_MULTI_PHY)
	BcmXdslCoreMediaSearchInit();
#endif
#elif defined(SUPPORT_MULTI_PHY)
	{
#if defined(CONFIG_BCM963268)
		int imageType;
#endif
		bcmConfiguredImageType = adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK;
#if defined(CONFIG_BCM963268)
		if(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED)
			imageType = adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK;
		else if( !(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERMEDIA_DISABLED) &&
			(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_PREFERREDTYPE_FOUND))
			imageType = BCM_IMAGETYPE_SINGLELINE;
		else
			imageType = BCM_IMAGETYPE_BONDING;
		XdslCoreSetPhyImageType(imageType);
#endif
		BcmXdslCoreMediaSearchInit();
	}
#elif defined(SUPPORT_DSL_BONDING5B) && defined(SUPPORT_DSL_BONDING_C0)
	XdslCoreSetPhyImageType(0);
#endif

#if defined(SUPPORT_2CHIP_BONDING)
#if defined(SUPPORT_EXT_DSL_BONDING_MASTER)
	EXT_BONDING_COMM_registerPacketType(EXT_BONDING_DSL_STAT, BcmXdslCoreProcessStatFromExtBondDev, NULL);
#elif defined(SUPPORT_EXT_DSL_BONDING_SLAVE)
	EXT_BONDING_COMM_registerPacketType(EXT_BONDING_DSL_CMD, BcmXdslCoreProcessCmdFromExtBondDev, NULL);
#endif
#endif

#if defined(CONFIG_SMP) && defined(__KERNEL__)
	spin_lock_init(&gSpinLock);
#endif
#ifdef SAVE_STAT_LOCAL_AUTOSTART
	{
		ADSL_DIAG     dslDiags;
		DiagDebugData diagDebugData;

		dslDiags.diagCmd = LOG_CMD_DEBUG;
		dslDiags.diagMap = (int) &diagDebugData;
		diagDebugData.cmd = DIAG_DEBUG_CMD_STAT_SAVE_LOCAL;
		diagDebugData.param1 = 8;  /* save until buffer full */
		diagDebugData.param2 = 600000;
		BcmAdslCoreDiagCmd(0, &dslDiags);
	}
#endif

	if ( !AdslCoreInit() ) {
		BcmAdslCoreUninit();
		return;
	}
	AdslDrvPrintf(TEXT("VersionInfo: %s.d%s\n"),(NULL != adslCorePhyDesc.pVerStr)? adslCorePhyDesc.pVerStr: "UnknownPHY", ADSL_DRV_VER_STR);
	
	bcmOsGetTime(&initTick);
	noStatTimeout = ADSL_MIPS_STATUS_INIT_TIMEOUT_MS;
	
	for(i = 0; i < MAX_DSL_LINE; i++)
		BcmAdslCoreConfigure((uchar)i, pAdslCoreCfgProfile[i]);

#if 1 && defined(BCM6348_PLL_WORKAROUND)
	BcmAdslCoreHandlePLLInit();
#endif

	BcmAdslDiagInit(diagMap);

#if 0 || defined(ADSLCORE_ENABLE_LONG_REACH)
	/* BcmAdslCoreSetOemParameter (ADSL_OEM_G994_VENDOR_ID, "\x00""\x00""\x00""\xF0""\x00\x00""\x00\x00", 8); */
	BcmAdslCoreSetOemParameter (
		ADSL_OEM_G994_XMT_NS_INFO, 
		ctlmG994p1ControlXmtNonStdInfoMsg, 
		sizeof(ctlmG994p1ControlXmtNonStdInfoMsg));
#endif
#ifdef G992P1_ANNEX_I
	BcmAdslCoreSetOemParameter (ADSL_OEM_G994_VENDOR_ID, "\xB5""\x00""BDCM""\x54\x4D", 8);
	//BcmAdslCoreSetOemParameter (ADSL_OEM_G994_VENDOR_ID, "\xB5""\x00""BDCM""\x00\x01", 8);
#endif

	adslCoreInitialized = AC_TRUE;
	adslCoreStarted = AC_TRUE;
	/* Clear pending status interrupt, and enable it */
	pAdslEnum[ADSL_INTSTATUS_I] = pAdslEnum[ADSL_INTSTATUS_I];
	pAdslEnum[ADSL_INTMASK_I] |= MSGINT_MASK_BIT;
#if defined(__KERNEL__)
	bcmOsDpcEnqueue(irqDpcId);
#endif
	BcmDiagsMgrRegisterClient(DIAG_DSL_CLIENT, BcmAdslCoreDiagCmdAdsl);
}

/**************************************************************************
** Function Name: BcmAdslCoreConnectionStart
** Description  : This function starts ADSL PHY normal connection operations
** Returns      : None.
**************************************************************************/
void BcmAdslCoreConnectionStart(unsigned char lineId)
{
	if (!adslCoreInitialized)
		return;

	BcmAdslCoreLogWriteConnectionParam(&adslCoreConnectionParam[lineId]);
	if (adslCoreXmtGainChanged[lineId]) {
		adslCoreXmtGainChanged[lineId] = AC_FALSE;
		BcmAdslCoreSendXmtGainCmd(lineId, adslCoreXmtGain[lineId]);
	}

	BcmCoreDpcSyncEnter();
	if ((adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.modulations & kT1p413) && 
		!(adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities & kDslG994AnnexAMultimodeEnabled))
	{
		dslCommandStruct cmd;
#ifdef SUPPORT_DSL_BONDING
		cmd.command = kDslSetG994p1T1p413SwitchTimerCmd | (lineId << DSL_LINE_SHIFT);
#else
		cmd.command = kDslSetG994p1T1p413SwitchTimerCmd;
#endif
		cmd.param.value = adslCoreHsModeSwitchTime[lineId];
		AdslCoreCommandHandler(&cmd);
	}
	
#ifdef CONFIG_VDSL_SUPPORTED
	BcmAdslCoreUpdateConnectionParamBasedOnXtmFeatures(lineId);
#endif

	AdslCoreCommandHandler(&adslCoreCfgCmd[lineId]);
	AdslCoreCommandHandler(&adslCoreConnectionParam[lineId]);
	
#if defined(SUPPORT_MULTI_PHY)
	if((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SWITCHPHY_DISABLED) &&
		(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIASEARCH_DISABLED) &&
		(BCM_IMAGETYPE_SINGLELINE==(adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_IMAGETYPE_MSK))) {
		unsigned char lineToStayDown = 1^(((adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_MEDIATYPE_MSK) >> BCM_MEDIATYPE_SHIFT) & 1);
		kerSysLedCtrl((1==lineToStayDown)? kLedSecAdsl: kLedAdsl, kLedStateOff);
#ifndef CONFIG_BCM963268
		if(!ADSL_PHY_SUPPORT(kAdslPhyGfast))
			BcmXdslCoreSendCmd(lineToStayDown, kDslDownCmd, 0);
#endif
	}
#endif

#if defined(DTAG_UR2) && !defined(ANNEX_J)
	{
	dslCommandStruct	cmd;
	uchar xmtToneMap[8] = {0,0,0,0, 0xFE, 0xFF, 0xFF, 0xFF};
	uchar rcvToneMap = 0xFF;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslTestCmd | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslTestCmd;
#endif
	cmd.param.dslTestSpec.type = kDslTestToneSelection;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtStartTone = 0;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones = 60;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvStartTone = 60;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvNumOfTones = 8;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap = xmtToneMap;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvMap = &rcvToneMap;
	AdslCoreCommandHandler(&cmd);
	}
#endif
	adslCoreConnectionMode[lineId] = AC_TRUE;

	/* Update driver variables to include some of the driver default values that are sent to PHY */
	adslCoreCfgProfile[lineId].adslDemodCapValue |= adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities;
	adslCoreCfgProfile[lineId].adslDemodCap2Value |= adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5;
	adslCoreCfgProfile[lineId].xdslAuxFeaturesValue |= adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.auxFeatures;
#ifdef CONFIG_VDSL_SUPPORTED
	if (ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2)) {
		g993p2DataPumpCapabilities	*pG993p2Cap = adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA;
		adslCoreCfgProfile[lineId].vdslCfgFlagsValue |= pG993p2Cap->cfgFlags;
	}
#endif
	{
	dslExtraCfgCommand *pExtraCfg = (void *) ((char *)&adslCoreCfgCmd[lineId].param + sizeof(adslCoreCfgCmd[lineId].param.dslClearEocMsg));
	adslCoreCfgProfile[lineId].xdslCfg1Value |= pExtraCfg->phyExtraCfg[0];
	adslCoreCfgProfile[lineId].xdslCfg2Value |= pExtraCfg->phyExtraCfg[1];
	adslCoreCfgProfile[lineId].xdslCfg3Value |= pExtraCfg->phyExtraCfg[2];
	adslCoreCfgProfile[lineId].xdslCfg4Value |= pExtraCfg->phyExtraCfg[3];
	}

#if defined(SUPPORT_SELT) && defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY)
	if(acSELTSuspendMediaSearch[lineId]) {
		/* Either SELT was forced to stop or SELT was started from the WebUI and is completed */
		acSELTSuspendMediaSearch[lineId] = AC_FALSE;
#ifdef CONFIG_BCM963268
		bMediaSearchSuspended = AC_FALSE;
#else
		BcmAdslCoreConnectionStart(lineId^1);
#endif
	}
#endif
	BcmCoreDpcSyncExit();
}

/**************************************************************************
** Function Name: BcmAdslCoreConnectionStop
** Description  : This function stops ADSL PHY connection operations and 
**				  puts ADSL PHY in idle mode
** Returns      : None.
**************************************************************************/
void BcmAdslCoreConnectionStop(unsigned char lineId)
{
	dslCommandStruct	cmd;

	if (!adslCoreInitialized)
		return;
	
#if defined(SUPPORT_DSL_BONDING)
	if((1 == lineId) && !ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
		return;
	}
#endif

	adslCoreConnectionMode[lineId] = FALSE;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslDownCmd | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslDownCmd;
#endif
	cmd.param.value = kDslIdleNone;

	BcmCoreCommandHandlerSync(&cmd);
}

/**************************************************************************
** Function Name: BcmAdslCoreConnectionUninit
** Description  : This function disables ADSL PHY
** Returns      : None.
**************************************************************************/
void BcmAdslCoreUninit(void)
{
#ifdef CONFIG_BCM_PWRMNGT_DDR_SR_API
	BcmPwrMngtRegisterLmemAddr(NULL);
#endif

#if !defined(BCM_CORE_TEST) && !defined(_NOOS)
	BcmHalInterruptDisable (INTERRUPT_ID_ADSL);
#ifdef __KERNEL__
	free_irq(INTERRUPT_ID_ADSL, 0);
#endif
#endif

#if defined(CONFIG_SMP) && defined(__KERNEL__)
	BcmCoreDpcSyncEnter();
#endif
	if (adslCoreInitialized)
		AdslCoreUninit();

	BcmAdslDiagTaskUninit();

#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
	if (irqSemId != 0 )
		bcmOsSemGive (irqSemId);
#else
#if defined(CONFIG_PHY_PARAM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
	if(NULL != pXdslDummyDevice) {
		kfree(pXdslDummyDevice);
		pXdslDummyDevice = NULL;
	}
#endif
#endif
	adslCoreInitialized = AC_FALSE;
	adslCoreStarted = AC_FALSE;
	
#if defined(CONFIG_SMP) && defined(__KERNEL__)
	BcmCoreDpcSyncExit();
#endif
}

/***************************************************************************
** Function Name: BcmAdslCoreGetConnectionInfo
** Description  : This function is called by ADSL driver to obtain
**                connection info from core ADSL PHY
** Returns      : None.
***************************************************************************/
void BcmAdslCoreGetConnectionInfo(unsigned char lineId, PADSL_CONNECTION_INFO pConnectionInfo)
{
	AdslCoreConnectionRates	acRates;
	OS_TICKS				ticks;

	bcmOsGetTime(&ticks);
	
#ifndef PHY_LOOPBACK

	if (!adslCoreConnectionMode[lineId]) {
		pConnectionInfo->LinkState = BCM_ADSL_LINK_DOWN;
		pConnectionInfo->ulFastDnStreamRate = 0;
		pConnectionInfo->ulInterleavedDnStreamRate = 0;
		pConnectionInfo->ulFastUpStreamRate = 0;
		pConnectionInfo->ulInterleavedUpStreamRate = 0;
		return;
	}
#ifdef SUPPORT_TEQ_FAKE_LINKUP
	if(AdslDrvIsFakeLinkUp(lineId) || AdslCoreLinkState(lineId))
#else
	if (AdslCoreLinkState(lineId))
#endif
	{
		pConnectionInfo->LinkState = BCM_ADSL_LINK_UP;
		AdslCoreGetConnectionRates (lineId, &acRates);
		pConnectionInfo->ulFastDnStreamRate = acRates.fastDnRate;
		pConnectionInfo->ulInterleavedDnStreamRate = acRates.intlDnRate;
		pConnectionInfo->ulFastUpStreamRate = acRates.fastUpRate;
		pConnectionInfo->ulInterleavedUpStreamRate = acRates.intlUpRate;
	}
	else {
		pConnectionInfo->LinkState = BCM_ADSL_LINK_DOWN;
		pConnectionInfo->ulFastDnStreamRate = 0;
		pConnectionInfo->ulInterleavedDnStreamRate = 0;
		pConnectionInfo->ulFastUpStreamRate = 0;
		pConnectionInfo->ulInterleavedUpStreamRate = 0;
		switch (AdslCoreLinkStateEx(lineId)) {
			case kAdslTrainingG994:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G994;
				break;
			case kAdslTrainingG992Started:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G992_STARTED;
				break;
			case kAdslTrainingG992ChanAnalysis:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G992_CHANNEL_ANALYSIS;
				break;
			case kAdslTrainingG992Exchange:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G992_EXCHANGE;
				break;
			case kAdslTrainingG993Started:
#ifdef CONFIG_BCM_DSL_GFAST
				if(ADSL_PHY_SUPPORT(kAdslPhyGfast)
#ifdef CONFIG_BCM_DSL_GFASTCOMBO
					&& XdslMibIsGfastMod(XdslCoreGetDslVars(lineId))
#endif
					)
					pConnectionInfo->LinkState = BCM_ADSL_TRAINING_GFAST_STARTED;
				else
#endif
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G993_STARTED;
				break;
			case kAdslTrainingG993ChanAnalysis:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G993_CHANNEL_ANALYSIS;
				break;
			case kAdslTrainingG993Exchange:
				pConnectionInfo->LinkState = BCM_ADSL_TRAINING_G993_EXCHANGE;
				break;
		}
	}

	//LGD_FOR_TR098
	pConnectionInfo->ShowtimeStart = (ticks - g_ShowtimeStartTicks[lineId])*BCMOS_MSEC_PER_TICK/1000;
	
#ifdef SUPPORT_EXT_DSL_BONDING_MASTER
	if( 0 != lineId )
		return;
#endif

#else	/* PHY_LOOPBACK */

	if (kAdslTrainingConnected == (AdslCoreLinkStateEx(lineId))) {
		pConnectionInfo->LinkState = BCM_ADSL_LINK_UP;
		AdslCoreGetConnectionRates (lineId, &acRates);
		pConnectionInfo->ulFastDnStreamRate = acRates.fastDnRate;
		pConnectionInfo->ulInterleavedDnStreamRate = acRates.intlDnRate;
		pConnectionInfo->ulFastUpStreamRate = acRates.fastUpRate;
		pConnectionInfo->ulInterleavedUpStreamRate = acRates.intlUpRate;
	}
	else {
		pConnectionInfo->LinkState = BCM_ADSL_LINK_DOWN;
		pConnectionInfo->ulFastDnStreamRate = 0;
		pConnectionInfo->ulInterleavedDnStreamRate = 0;
		pConnectionInfo->ulFastUpStreamRate = 0;
		pConnectionInfo->ulInterleavedUpStreamRate = 0;
	}
#endif /* PHY_LOOPBACK */

}

LOCAL void BcmAdslCoreEnableSnrMarginData(unsigned char lineId)
{
	dslCommandStruct	cmd;
	cmd.command = kDslFilterSNRMarginCmd | (lineId << DSL_LINE_SHIFT);
	cmd.param.value = 0;
	AdslCoreCommandHandler(&cmd);
}

LOCAL void BcmAdslCoreDisableSnrMarginData(unsigned char lineId)
{
	dslCommandStruct	cmd;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslFilterSNRMarginCmd | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslFilterSNRMarginCmd;
#endif
	cmd.param.value = 1;
	AdslCoreCommandHandler(&cmd);
}

int	BcmAdslCoreSetObjectValue (unsigned char lineId, char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
	int		res;
	BcmCoreDpcSyncEnter();
	res = AdslCoreSetObjectValue (lineId, objId, objIdLen, dataBuf, dataBufLen);
	BcmCoreDpcSyncExit();
	return res;
}

int	BcmAdslCoreGetObjectValue (unsigned char lineId, char *objId, int objIdLen, char *dataBuf, long *dataBufLen)
{
	uchar	*oid = (uchar *) objId;
	int		res;

	BcmCoreDpcSyncEnter();
	if ((objIdLen > 1) && (kOidAdslPrivate == oid[0]) && (kOidAdslPrivShowtimeMargin == oid[1]))
		BcmAdslCoreEnableSnrMarginData(lineId);
	res = AdslCoreGetObjectValue (lineId, objId, objIdLen, dataBuf, dataBufLen);
	BcmCoreDpcSyncExit();
	return res;
}

void BcmAdslCoreStartBERT(unsigned char lineId, unsigned long totalBits)
{
	dslCommandStruct	cmd;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslDiagStartBERT | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslDiagStartBERT;
#endif
	cmd.param.value = totalBits;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreStopBERT(unsigned char lineId)
{
	dslCommandStruct	cmd;
#ifdef SUPPORT_DSL_BONDING
	cmd.command = kDslDiagStopBERT | (lineId << DSL_LINE_SHIFT);
#else
	cmd.command = kDslDiagStopBERT;
#endif
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreBertStartEx(unsigned char lineId, unsigned long bertSec)
{
	BcmCoreDpcSyncEnter();
	AdslCoreBertStartEx(lineId, bertSec);
	BcmCoreDpcSyncExit();
}

void BcmAdslCoreBertStopEx(unsigned char lineId)
{
	BcmCoreDpcSyncEnter();
	AdslCoreBertStopEx(lineId);
	BcmCoreDpcSyncExit();
}

void BcmXdslCoreMiscIoCtlFunc(unsigned char lineId)
{
	if (adslCoreResetPending) {
		adslCoreResetPending = AC_FALSE;
		BcmAdslCoreReset(DIAG_DATA_EYE);
	}
}

#ifndef DYING_GASP_API

#if defined(CONFIG_BCM963x8)
/* The BCM6348 cycles per microsecond is really variable since the BCM6348
 * MIPS speed can vary depending on the PLL settings.  However, an appoximate
 * value of 120 will still work OK for the test being done.
 */
#define	CYCLE_PER_US	120
#endif
#define	DG_GLITCH_TO	(100*CYCLE_PER_US)

#if !defined(__KERNEL__) && !defined(_CFE_)
#define BpGetDyingGaspExtIntr(pIntrNum)		*(unsigned int *) (pIntrNum) = 0
#endif

Bool BcmAdslCoreCheckPowerLoss(void)
{
	ulong	clk0;
	ulong	ulIntr;

	ulIntr = 0;
	clk0 = BcmAdslCoreGetCycleCount();

	UART->Data = 'D';
	UART->Data = '%';
	UART->Data = 'G';

#if defined(CONFIG_BCM963x8) && !defined(VXWORKS) && !defined(__ECOS)
	do {
		ulong		clk1;

		clk1 = BcmAdslCoreGetCycleCount();		/* time cleared */
		/* wait a little to get new reading */
		while ((BcmAdslCoreGetCycleCount()-clk1) < CYCLE_PER_US*2)
			;
	} while ((PERF->IrqStatus & (1 << (INTERRUPT_ID_DG - INTERNAL_ISR_TABLE_OFFSET))) && ((BcmAdslCoreGetCycleCount() - clk0) < DG_GLITCH_TO));

	if (!(PERF->IrqStatus & (1 << (INTERRUPT_ID_DG - INTERNAL_ISR_TABLE_OFFSET)))) {
#ifndef CONFIG_ARM
		BcmHalInterruptEnable( INTERRUPT_ID_DG );
#endif
		AdslDrvPrintf (TEXT(" - Power glitch detected. Duration: %ld us\n"), (BcmAdslCoreGetCycleCount() - clk0)/CYCLE_PER_US);
		return AC_FALSE;
	}
#endif
	return AC_TRUE;
}
#endif /* DYING_GASP_API */

void BcmAdslCoreSendDyingGasp(int powerCtl)
{
    int    i;
    dslCommandStruct    cmd;

    for(i = 0; i < MAX_DSL_LINE; i++)
    if (kAdslTrainingConnected == AdslCoreLinkStateEx(i)) {
        cmd.command = kDslDyingGaspCmd | (i << DSL_LINE_SHIFT);
        cmd.param.value = powerCtl != 0 ? 1 : 0;
#ifdef __KERNEL__
        if (!in_irq())
#endif
            BcmCoreDpcSyncEnter();

        AdslCoreCommandHandler(&cmd);
        AdslCoreCommandHandler(&cmd);

#ifdef __KERNEL__
        if (!in_irq())
#endif
            BcmCoreDpcSyncExit();
    }
    else {
        AdslDrvPrintf (TEXT(" - Power failure detected. ADSL Link down.\n"));
    }

#ifndef DYING_GASP_API
    BcmAdslCoreSetWdTimer(1000000);
#if defined(CONFIG_BCM963x8)
#if !defined(__ECOS)
    PERF->blkEnables &= ~(EMAC_CLK_EN | USBS_CLK_EN | SAR_CLK_EN);
#endif /* !defined(__ECOS) */
#endif
#endif
}

extern void XdslCoreSetOvhMsgPrintFlag(uchar lineId, Boolean flag);
#ifdef SUPPORT_PHY_BIN_FROM_SDRAM
extern int loadImageFromSdram;
#endif
int ClearEOCLoopBackEnabled=0;
#ifdef GFAST_TESTMODE_TEST
extern ulong gFastTestModeReq;
#endif

void BcmAdslCoreDebugCmd(unsigned char lineId, void *pMsg)
{
	int	res;
	DiagDebugData	*pDbgCmd = (DiagDebugData *) pMsg;

	switch (pDbgCmd->cmd) {
#if defined(SUPPORT_2CHIP_BONDING)
		case DIAG_DEBUG_CMD_SET_EXTBONDINGDBG_PRINT:	//TO DO: Remove after debugging
			extBondingDbgFlag = pDbgCmd->param1;
			break;
#endif
#if defined(SUPPORT_MULTI_PHY) || defined(CONFIG_BCM_DSL_GFAST)
		case DIAG_DEBUG_CMD_SAVE_PREFERRED_LINE:
			BcmXdsl_RequestSavePreferredLine();
			break;
#ifdef SUPPORT_MULTI_PHY
#ifdef CONFIG_BCM963268
		case DIAG_DEBUG_CMD_SWITCH_PHY_IMAGE:
			if( XdslCoreGetPhyImageType() != pDbgCmd->param1 )
				BcmXdslCoreSwitchPhyImage(pDbgCmd->param1);
			break;
#endif
		case DIAG_DEBUG_CMD_MEDIASEARCH_CFG:
		{
			Bool resetPhy = BcmXdslCoreProcessMediaSearchCfgCmd(pDbgCmd->param1, AC_FALSE);
			
			if(resetPhy)
				BcmXdslCoreMediaSearchReStartPhy();

			if( BcmXdslCoreMediaSearchInInitState() )
				BcmXdslCoreMediaSearchSM(MEDIASEARCH_START_E, 0);
			break;
		}
#elif defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
		case DIAG_DEBUG_CMD_PHY_TYPE_CFG:
		{
			int newVal;
			AC_BOOL restartPhy = AC_FALSE;
			
			newVal = (pDbgCmd->param1 & PHY_SWITCH_MSK) >> PHY_SWITCH_SHIFT;
			adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_PHYSWITCH_MSK) |
				(newVal << BCM_PHYSWITCH_SHIFT);
			newVal = (pDbgCmd->param1 & PHY_TYPE_MSK) >> PHY_TYPE_SHIFT;
			adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_PHYTYPE_MSK) |
				(newVal << BCM_PHYTYPE_SHIFT);
			if((((PHY_TYPE_GFAST >> PHY_TYPE_SHIFT) == newVal) && !ADSL_PHY_SUPPORT(kAdslPhyGfast)) ||
				(((PHY_TYPE_NON_GFAST >> PHY_TYPE_SHIFT) == newVal) && ADSL_PHY_SUPPORT(kAdslPhyGfast))) {
				XdslCoreSetPhyImageType(newVal);
				restartPhy = AC_TRUE;
			}
#if 0
			newVal = (pDbgCmd->param1 & PHY_TYPE_PREFER_SAVE_MSK) >> PHY_TYPE_PREFER_SAVE_SHIFT;
			oldVal = (adslCoreCfgProfile[0].xdslMiscCfgParam & BCM_SAVEPREFERPHY_MSK) >> BCM_SAVEPREFERPHY_SHIFT;
			if(newVal != oldVal) {
				if((PHY_TYPE_PREFER_SAVE_DISABLED >> PHY_TYPE_PREFER_SAVE_SHIFT) == newVal)
					BcmXdslCoreClearPreferredPhyType();
				adslCoreCfgProfile[0].xdslMiscCfgParam = (adslCoreCfgProfile[0].xdslMiscCfgParam & ~BCM_SAVEPREFERPHY_MSK) |
					(newVal << BCM_SAVEPREFERPHY_SHIFT);
			}
#endif
			if(restartPhy) {
				if(ADSL_PHY_SUPPORT(kAdslPhyGfast)) {
					if(AC_FALSE == switchGfastToVdslImagePending) {
						BcmAdslCoreConnectionStart(lineId);
						switchGfastToVdslImagePending = AC_TRUE;
					}
				}
				else {
					switchGfastToVdslImagePending = AC_FALSE;
					BcmAdslCoreReset(DIAG_DATA_EYE);
				}
			}
			
			break;
		}
#endif
#if defined(CONFIG_BCM_DSL_GFAST)
		case DIAG_DEBUG_CMD_MICRO_INTERRUPT:
		{
			unsigned short bpGpioAFELDRelay;
			OS_TICKS	osTime0, osTime1;
			
			if(BP_SUCCESS != BpGetAFELDRelayGpio(&bpGpioAFELDRelay)) {
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "*** bpGpioAFELDRelay is not configured!!! ***\n");
				break;
			}
			
			if(ADSL_PHY_SUPPORT(kAdslPhyGfast)) {
				kerSysSetGpio(bpGpioAFELDRelay, kGpioActive);	/* Activate the switch relay to use the external AFE path */
				bcmOsGetTime(&osTime0);
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "*** Activate LD Relay for %d ms (tick0 %08x) ***\n",(int)pDbgCmd->param1, osTime0);
				while (BcmAdslCoreOsTimeElapsedMs(osTime0) < pDbgCmd->param1);
				bcmOsGetTime(&osTime1);
				kerSysSetGpio(bpGpioAFELDRelay, kGpioInactive);	/* De-activate the switch relay to use the internal AFE path */
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "*** De-activate LD Relay (tick1 %08x -> %lu ms) ***\n",osTime1,(osTime1-osTime0)*BCMOS_MSEC_PER_TICK);
			}
			else {
				kerSysSetGpio(bpGpioAFELDRelay, kGpioInactive);	/* De-activate the switch relay to use the internal AFE path */
				bcmOsGetTime(&osTime0);
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "*** De-activate LD Relay for %d ms (tick0 %08x) ***\n",(int)pDbgCmd->param1, osTime0);
				while (BcmAdslCoreOsTimeElapsedMs(osTime0) < pDbgCmd->param1);
				bcmOsGetTime(&osTime1);
				kerSysSetGpio(bpGpioAFELDRelay, kGpioActive);	/* Activate the switch relay to use the external AFE path */
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "*** Activate LD Relay (tick1 %08x -> %lu ms) ***\n", osTime1, (osTime1-osTime0)*BCMOS_MSEC_PER_TICK);
			}
		}
			break;
#endif
#endif
		case DIAG_DEBUG_CMD_SYNC_CPE_TIME:
			BcmAdslCoreDiagSetSyncTime(pDbgCmd->param1);
			break;
#if defined(USE_6306_CHIP)
		case DIAG_DEBUG_CMD_IND_READ_6306:
			res = spiTo6306IndirectRead((int)pDbgCmd->param1);
			BcmAdslCoreDiagWriteStatusString(lineId, "Indirect Read 0x%08x = 0x%08x\n",
				(uint)pDbgCmd->param1, (uint)res);
			break;
		case DIAG_DEBUG_CMD_IND_WRITE_6306:
			spiTo6306IndirectWrite((int)pDbgCmd->param1, (int)pDbgCmd->param2);
			BcmAdslCoreDiagWriteStatusString(lineId, "Indirect Write addr: 0x%08x data: 0x%08x\n",
				(uint)pDbgCmd->param1, (uint)pDbgCmd->param2);
			break;
		case DIAG_DEBUG_CMD_READ_6306:
			res = spiTo6306Read((int)pDbgCmd->param1);
			BcmAdslCoreDiagWriteStatusString(lineId, "Direct Read 0x%08x = 0x%08x\n",
				(uint)pDbgCmd->param1, (uint)res);
			break;
		case DIAG_DEBUG_CMD_WRITE_6306:
			spiTo6306Write((int)pDbgCmd->param1, (int)pDbgCmd->param2);
			BcmAdslCoreDiagWriteStatusString(lineId, "Direct Write addr: 0x%08x data: 0x%08x\n",
				(uint)pDbgCmd->param1, (uint)pDbgCmd->param2);
			break;
#endif
		case DIAG_DEBUG_CMD_RESET_CONNECTION:
			BcmAdslCoreConnectionReset(lineId);
			break;

		case DIAG_DEBUG_CMD_RESET_PHY:
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
			if(pBkupImage) {
				vfree(pBkupImage);
				pBkupImage = NULL;
				bkupLmemSize = 0;
				bkupSdramSize = 0;
			}
#endif
#ifdef SUPPORT_PHY_BIN_FROM_SDRAM
			loadImageFromSdram = (0 != pDbgCmd->param1)? 1: 0;
			adslCoreAlwaysReset = FALSE;
#endif
			BcmAdslCoreReset(DIAG_DATA_EYE);
#if defined(PHY_LOOPBACK)
			adslNewImg = 1;
#endif
			break;

#if 1 && defined(BCM6348_PLL_WORKAROUND)
		case 99:
			AdslCoreSetPllClock();
			BcmAdslCoreReset(DIAG_DATA_EYE);
			break;
#endif

#if defined(CONFIG_BCM96348) ||  defined(CONFIG_BCM96338)
		case DIAG_DEBUG_CMD_LOG_SAMPLES:
			{
			dslCommandStruct	cmd;
			int					nDmaBlk;

			BcmAdslCoreDiagDmaInit();

			nDmaBlk = BcmAdslCoreDiagGetDmaBlockNum();
			if (nDmaBlk < 3)
				break;
			bcmOsSleep (100/BCMOS_MSEC_PER_TICK);
			cmd.command = kDslDiagSetupCmd;
			cmd.param.dslDiagSpec.setup = kDslDiagEnableEyeData | kDslDiagEnableLogData;
			cmd.param.dslDiagSpec.eyeConstIndex1 = 63;
			cmd.param.dslDiagSpec.eyeConstIndex2 = 64;
			cmd.param.dslDiagSpec.logTime = (ulong) (1 - nDmaBlk);
			BcmCoreCommandHandlerSync(&cmd);
			bcmOsSleep (100/BCMOS_MSEC_PER_TICK);
			BcmAdslCoreSendDmaBuffers(kDslDataAvailStatus, nDmaBlk - 2);
			}
			break;
#endif
		case DIAG_DEBUG_CMD_LOG_DATA:
			BcmAdslCoreDiagStartLog_1(lineId, pDbgCmd->param1, pDbgCmd->param2);
			if (pDbgCmd->cmdId == DIAG_DEBUG_CMD_LOG_AFTER_RESET)
				diagDebugCmd = *pDbgCmd;
			else
				BcmAdslCoreDiagStartLog_2(lineId, pDbgCmd->param1, pDbgCmd->param2);
			break;

#if 1 || defined(PHY_BLOCK_TEST)
		case DIAG_DEBUG_CMD_PLAYBACK_STOP:
			BcmAdslCoreDebugPlaybackStop();
			break;

		case DIAG_DEBUG_CMD_PLAYBACK_RESUME:
			BcmAdslCoreDebugPlaybackResume();
			break;
#endif

		case DIAG_DEBUG_CMD_G992P3_DEBUG:
			XdslCoreSetOvhMsgPrintFlag(lineId, (pDbgCmd->param1 != 0));
			break;
		case DIAG_DEBUG_CMD_CLEAREOC_LOOPBACK:
			{
			ClearEOCLoopBackEnabled= (pDbgCmd->param1 != 0);
			}
			break;
		case DIAG_DEBUG_CMD_SET_OEM:
		{
			char *str = (char *)pDbgCmd + sizeof(DiagDebugData);
			AdslDrvPrintf(TEXT("DIAG_DEBUG_CMD_SET_OEM: paramId = %lu len = %lu\n"), pDbgCmd->param1,pDbgCmd->param2);
			str[pDbgCmd->param2] = 0;
			AdslDrvPrintf(TEXT("str: %s\n"), str);
			res=AdslCoreSetOemParameter(pDbgCmd->param1,str,(int)pDbgCmd->param2);
			AdslDrvPrintf(TEXT("Set len = %d\n"), res);
		}
			break;
		case DIAG_DEBUG_CMD_G992P3_DIAG_MODE: 
			BcmAdslCoreSetAdslDiagMode(lineId, pDbgCmd->param1);
			break;
		case DIAG_DEBUG_CMD_ANNEXM_CFG:
			if (pDbgCmd->param2 != 0) {
				if (-1 == (long) pDbgCmd->param2)
					pDbgCmd->param2 = 0;
				adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p3AnnexA->readsl2Upstream = 
				adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA->readsl2Upstream =
					pDbgCmd->param2 & 0xFF;
				adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p3AnnexA->readsl2Downstream = 
				adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p5AnnexA->readsl2Downstream =
					(pDbgCmd->param2 >> 8) & 0xFF;
			}
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5 = pDbgCmd->param1 & (kAdsl2CfgAnnexMPsdMask >> kAdsl2CfgAnnexMPsdShift);
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5 |= adslCoreEcUpdateMask;
			BcmAdslCoreConnectionReset(lineId);
			break;
		case DIAG_DEBUG_CMD_SET_L2_TIMEOUT:
			AdslCoreSetL2Timeout(pDbgCmd->param1);
			break;
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
		case DIAG_DEBUG_CMD_CONFIG_BKUPIMAGE:
			pBkupImageEnable = (0 == pDbgCmd->param1)? FALSE: TRUE;
			if((FALSE == pBkupImageEnable) && (NULL != pBkupImage)) {
				vfree(pBkupImage);
				pBkupImage = NULL;
				bkupLmemSize = 0;
				bkupSdramSize = 0;
			}
			break;
#endif
#ifdef GFAST_TESTMODE_TEST
		case DIAG_DEBUG_CMD_GFAST_TESTMODE:
			AdslDrvPrintf(TEXT("DIAG_DEBUG_CMD_GFAST_TESTMODE: Current gFastTestModeReq = %lu pDbgCmd->param1 = %lu\n"), gFastTestModeReq, pDbgCmd->param1);
			gFastTestModeReq = pDbgCmd->param1;
			break;
#endif
	}
}

void BcmAdslCoreResetStatCounters(unsigned char lineId)
{
	BcmCoreDpcSyncEnter();
	AdslCoreResetStatCounters(lineId);
	BcmCoreDpcSyncExit();
}

void BcmAdslCoreSetTestMode(unsigned char lineId, int testMode)
{
	dslCommandStruct	cmd;

	if (ADSL_TEST_DIAGMODE == testMode) {
		BcmAdslCoreSetAdslDiagMode(lineId, 1);
		return;
	}
#ifdef SUPPORT_SELT
	if (ADSL_TEST_NEXT_SELT == testMode) {
		BcmAdslCoreSetSeltNextMode(lineId);
		return;
	}
#endif
	BcmCoreDpcSyncEnter();
	if (ADSL_TEST_L3 == testMode) {
		acL3Requested[lineId]= 1;
		AdslCoreSetL3(lineId);
	}
	else if (ADSL_TEST_L0 == testMode) {
		AdslCoreSetL0(lineId);
	}
	else {
#ifdef NTR_SUPPORT
		if( kDslTestNtrStart == testMode ) {
			long len;
			adslMibInfo *pMib = (adslMibInfo *)AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &len);
			len = sizeof(dslNtrCfg);
			cmd.command = kDslSendEocCommand | (lineId << DSL_LINE_SHIFT);
			cmd.param.dslClearEocMsg.msgId = kDslNtrConfig;
			cmd.param.dslClearEocMsg.msgType = len;
			gSharedMemAllocFromUserContext=1;
			cmd.param.dslClearEocMsg.dataPtr = (char *) AdslCoreSharedMemAlloc(len);
			memcpy(cmd.param.dslClearEocMsg.dataPtr, &pMib->ntrCfg, len);
#ifdef  ADSLDRV_LITTLE_ENDIAN
			BlockLongMoveReverse(5, (long *)cmd.param.dslClearEocMsg.dataPtr, (long *)cmd.param.dslClearEocMsg.dataPtr);
#endif
			AdslCoreCommandHandler(&cmd);
			gSharedMemAllocFromUserContext=0;
			AdslDrvPrintf (TEXT("### Sent kDslNtrConfig to Phy, operMode=%d , output freq=%lu ###\n"), pMib->ntrCfg.operMode, pMib->ntrCfg.intModeDivRatio);
		}
#endif
#ifdef SUPPORT_SELT
        if (kDslTestHybridSelt == testMode) {
			long len;
			adslMibInfo *pMib = (adslMibInfo *)AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &len);

            cmd.command = kDslSeltConfiguration | (lineId << DSL_LINE_SHIFT);
            cmd.param.value  = pMib->selt.seltCfg;
            AdslCoreCommandHandler(&cmd);

            cmd.command = kDslSetRcvGainCmd | (lineId << DSL_LINE_SHIFT);
            cmd.param.value = pMib->selt.seltAgc;
            AdslCoreCommandHandler(&cmd);
        }
#endif
		cmd.command = kDslTestCmd | (lineId << DSL_LINE_SHIFT);
		cmd.param.dslTestSpec.type = testMode;
		AdslCoreCommandHandler(&cmd);
	}
	BcmCoreDpcSyncExit();
}
void BcmAdslCoreSetTestExecutionDelay(unsigned char lineId, int testMode, ulong value)
{
	dslCommandStruct	cmd;

	BcmCoreDpcSyncEnter();
	cmd.command = kDslTestCmd | (lineId << DSL_LINE_SHIFT);
	cmd.param.dslTestSpec.type =kDslTestExecuteDelay ;
	cmd.param.dslTestSpec.param.value=value;
	AdslCoreCommandHandler(&cmd);
	BcmCoreDpcSyncExit();
}

#ifdef SUPPORT_SELT
void BcmAdslCoreSetSeltNextMode(unsigned char lineId)
{
	//dslCommandStruct	cmd;
	adslMibInfo			*pMibInfo;
	ulong				mibLen;
    unsigned char       nextSteps;
#if defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY) && !defined(CONFIG_BCM963268)
    Bool                bondingStat;
    adslMibInfo         *pMibInfo1;
#endif

	if (!adslCoreStarted)
		return;

    pMibInfo = (void *) AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &mibLen);

    if (((pMibInfo->selt.seltState&SELT_STATE_MASK)==SELT_STATE_IDLE) || ((pMibInfo->selt.seltState&SELT_STATE_MASK)==SELT_STATE_COMPLETE)) {
#if defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY)
        if(acSELTSuspendMediaSearch[lineId]) {
            /* For the case SELT started from the CLI, data is retrieved and processed */
            acSELTSuspendMediaSearch[lineId] = AC_FALSE;
            BcmAdslCoreSetTestMode(lineId, kDslTestBackToNormal);
#ifdef CONFIG_BCM963268
            bMediaSearchSuspended = AC_FALSE;
#else
            BcmAdslCoreConnectionStart(lineId^1);
#endif
        }
#endif
        return;
    }

    /* SELT is measuring here */
    nextSteps  = pMibInfo->selt.seltSteps;
    nextSteps &= (unsigned char)(~((pMibInfo->selt.seltState>>SELT_STATE_MEASUREMENT_SHIFT)&SELT_STATE_MASK));

    if (nextSteps&SELT_STATE_STEP_WAIT)
    {
        OS_TICKS	ticks, waitTimeInTicks;
        bcmOsGetTime(&ticks);

        /* kick-off wait period if in showtime */
        if (kAdslTrainingConnected == pMibInfo->adslTrainingState)
        {
            waitTimeInTicks = 10*HZ;
            BcmAdslCoreConnectionStop(lineId);
#if defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY)
#ifdef CONFIG_BCM963268
            if(!ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
                bMediaSearchSuspended = AC_TRUE;
                acSELTSuspendMediaSearch[lineId] = AC_TRUE;
            }
#else
            bondingStat = ((BcmXdslCorePtmBondingEnable(lineId) && (kDslBondingPTM == pMibInfo->xdslStat[0].bondingStat.status)) ||
                (BcmXdslCoreAtmBondingEnable(lineId) && (kDslBondingATM == pMibInfo->xdslStat[0].bondingStat.status)));
            pMibInfo1 = (void *) AdslCoreGetObjectValue (lineId^1, NULL, 0, NULL, &mibLen);
            if(!bondingStat && (kAdslTrainingConnected != pMibInfo1->adslTrainingState)) {
                BcmAdslCoreConnectionStop(lineId^1);
                acSELTSuspendMediaSearch[lineId] = AC_TRUE;
            }
#endif
#endif
        }
        else
            waitTimeInTicks = 1*HZ;

        acTestModeWait[lineId] = ticks+waitTimeInTicks;
        pMibInfo->selt.seltState |= (SELT_STATE_STEP_WAIT<<SELT_STATE_MEASUREMENT_SHIFT);
    }
    else if (nextSteps&SELT_STATE_STEP_QLN)
    {
        /* kick-off QLN test*/
        BcmAdslCoreSetTestMode(lineId, kDslTestQLN);
        pMibInfo->selt.seltState |= (SELT_STATE_STEP_QLN<<SELT_STATE_MEASUREMENT_SHIFT);
    }
    else if (nextSteps&SELT_STATE_STEP_ENR)
    {
        /* kick-off ENR test*/
        pMibInfo->selt.seltState |= (SELT_STATE_STEP_ENR<<SELT_STATE_MEASUREMENT_SHIFT);
    }
    else if (nextSteps&SELT_STATE_STEP_SELT) {
#if defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY)
        if(kAdslTrainingConnected == pMibInfo->adslTrainingState) {
            /* SELT started without SELT_STATE_STEP_WAIT */
#ifdef CONFIG_BCM963268
            if(!ADSL_PHY_SUPPORT(kAdslPhyBonding)) {
                bMediaSearchSuspended = AC_TRUE;
                acSELTSuspendMediaSearch[lineId] = AC_TRUE;
            }
#else
            bondingStat = ((BcmXdslCorePtmBondingEnable(lineId) && (kDslBondingPTM == pMibInfo->xdslStat[0].bondingStat.status)) ||
                (BcmXdslCoreAtmBondingEnable(lineId) && (kDslBondingATM == pMibInfo->xdslStat[0].bondingStat.status)));
            pMibInfo1 = (void *) AdslCoreGetObjectValue (lineId^1, NULL, 0, NULL, &mibLen);
            if(!bondingStat && (kAdslTrainingConnected != pMibInfo1->adslTrainingState)) {
                BcmAdslCoreConnectionStop(lineId^1);
                acSELTSuspendMediaSearch[lineId] = AC_TRUE;
            }
#endif
        }
#endif
        /* kick-off SELT test*/
        pMibInfo->selt.seltState |= (SELT_STATE_STEP_SELT<<SELT_STATE_MEASUREMENT_SHIFT);
        BcmAdslCoreSetTestMode(lineId, kDslTestHybridSelt);
    }
    else if (nextSteps&SELT_STATE_STEP_POSTPROCESSING)
    {
        /* enable post-processing */
        pMibInfo->selt.seltState = SELT_STATE_POSTPROCESSING;
        BcmAdslCoreSetTestMode(lineId, kDslTestBackToNormal);
#if !defined(CONFIG_BCM963268) && defined(SUPPORT_DSL_BONDING) && defined(SUPPORT_MULTI_PHY)
        if(acSELTSuspendMediaSearch[lineId])
            BcmXdslCoreSendCmd(lineId^1, kDslDownCmd, kDslIdleNone);
#endif
    }
}
#endif

void BcmAdslCoreSetAdslDiagMode(unsigned char lineId, int diagMode)
{
#ifdef G992P3
	dslCommandStruct	cmd;
	adslMibInfo			*pMibInfo;
	ulong				mibLen;

	if (!adslCoreStarted)
		return;

	if (ADSL_PHY_SUPPORT(kAdslPhyAdsl2) || ADSL_PHY_SUPPORT(kAdslPhyVdslG993p2)) {
		cmd.command = kDslIdleCmd | (lineId << DSL_LINE_SHIFT);
		BcmCoreCommandHandlerSync(&cmd);

		BcmAdslCoreDelay(40);

		pMibInfo = (void *) AdslCoreGetObjectValue (lineId, NULL, 0, NULL, &mibLen);
		if (diagMode != 0)
			pMibInfo->adslPhys.adslLDCompleted = 1;		/* 1-Started, 2-Completed, -1-Failed */
		g992p5Param[lineId].diagnosticsModeEnabled = 
		g992p3Param[lineId].diagnosticsModeEnabled = (diagMode != 0) ? 1 : 0;
		BcmCoreCommandHandlerSync(&adslCoreCfgCmd[lineId]);
		BcmCoreCommandHandlerSync(&adslCoreConnectionParam[lineId]);
		g992p3Param[lineId].diagnosticsModeEnabled = 0;
		g992p5Param[lineId].diagnosticsModeEnabled = 0;
	}
#endif
}

void BcmAdslCoreSelectTones(
	unsigned char	lineId,
	int		xmtStartTone,
	int		xmtNumTones,
	int		rcvStartTone,
	int		rcvNumTones,
	char	*xmtToneMap,
	char	*rcvToneMap)
{
	dslCommandStruct	cmd;

	cmd.command = kDslTestCmd | (lineId << DSL_LINE_SHIFT);
	cmd.param.dslTestSpec.type = kDslTestToneSelection;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtStartTone = xmtStartTone;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones = xmtNumTones;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvStartTone = rcvStartTone;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvNumOfTones = rcvNumTones;
	cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap = xmtToneMap;
	cmd.param.dslTestSpec.param.toneSelectSpec.rcvMap = rcvToneMap;
#if 1
	{
		int		i;

		AdslDrvPrintf(TEXT("xmtStartTone=%ld, xmtNumTones=%ld, rcvStartTone=%ld, rcvNumTones=%ld \nxmtToneMap="),
			cmd.param.dslTestSpec.param.toneSelectSpec.xmtStartTone,
			cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones,
			cmd.param.dslTestSpec.param.toneSelectSpec.rcvStartTone,
			cmd.param.dslTestSpec.param.toneSelectSpec.rcvNumOfTones);
		for (i = 0; i < ((cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones+7)>>3); i++)
			AdslDrvPrintf(TEXT("%02X "), cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap[i]);
		AdslDrvPrintf(TEXT("\nrcvToneMap="));
		for (i = 0; i < ((cmd.param.dslTestSpec.param.toneSelectSpec.rcvNumOfTones+7)>>3); i++)
			AdslDrvPrintf(TEXT("%02X "), cmd.param.dslTestSpec.param.toneSelectSpec.rcvMap[i]);
	}
	AdslDrvPrintf(TEXT("\n"));
#endif

	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreDiagRearrangeSelectTones(int *xmtStartTone, int *xmtNumTones,int *rcvStartTone, int *rcvNumTones, char *rcvToneMap, char *xmtToneMap)
{
	*rcvStartTone = *xmtNumTones - 8;
	*rcvNumTones  = 512 - *rcvStartTone;
	if ((*rcvToneMap - *xmtToneMap) != (32 >> 3)) {
		int		i;
			for (i = (32 >> 3); i < (*xmtNumTones >> 3); i++)
			xmtToneMap[i] = rcvToneMap[i - (32 >> 3)];
	}
	//rcvToneMap   += (*rcvStartTone - 32) >> 3;
	return;
}

void BcmAdslCoreDiagSelectTones(
	unsigned char	lineId,
	int		xmtStartTone,
	int		xmtNumTones,
	int		rcvStartTone,
	int		rcvNumTones,
	char	*xmtToneMap,
	char	*rcvToneMap)
{
#ifdef G992P1_ANNEX_B
	if (ADSL_PHY_SUPPORT(kAdslPhyAnnexB)) {
		xmtNumTones  = (adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG992p1AnnexB.upstreamMaxCarr + 7) & ~7;
		BcmAdslCoreDiagRearrangeSelectTones(&xmtStartTone, &xmtNumTones, &rcvStartTone, &rcvNumTones,rcvToneMap,xmtToneMap);
	}
#endif
	if((adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.modulations & kG992p3AnnexM) ||
		(adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.modulations & kG992p5AnnexM) )
	{
		xmtNumTones  = 64;
		BcmAdslCoreDiagRearrangeSelectTones(&xmtStartTone, &xmtNumTones, &rcvStartTone, &rcvNumTones, rcvToneMap, xmtToneMap);
		rcvToneMap+=(rcvStartTone - 32) >> 3;
	}
	BcmAdslCoreSelectTones(lineId, xmtStartTone,xmtNumTones, rcvStartTone,rcvNumTones, xmtToneMap,rcvToneMap);
}

Bool BcmAdslCoreSetSDRAMBaseAddr(void *pAddr)
{
	return AdslCoreSetSDRAMBaseAddr(pAddr);
}

Bool BcmAdslCoreSetVcEntry (int gfc, int port, int vpi, int vci, int pti_clp)
{
	return AdslCoreSetVcEntry(gfc, port, vpi, vci, pti_clp);
}

Bool BcmAdslCoreSetGfc2VcMapping(Bool bOn)
{
	dslCommandStruct	cmd;

	cmd.command = kDslAtmGfcMappingCmd;
	cmd.param.value = bOn;
	BcmCoreCommandHandlerSync(&cmd);
	return AC_TRUE;
}

Bool BcmAdslCoreSetAtmLoopbackMode(void)
{
	dslCommandStruct	cmd;

#if 1
	BcmAdslCoreReset(DIAG_DATA_EYE);
#else
	BcmAdslCoreConnectionStop();
#endif
	BcmAdslCoreDelay(100);

	cmd.command = kDslLoopbackCmd;
	BcmCoreCommandHandlerSync(&cmd);
	return AC_TRUE;
}

int BcmAdslCoreGetOemParameter (int paramId, void *buf, int len)
{
	return AdslCoreGetOemParameter (paramId, buf, len);
}

int BcmAdslCoreSetOemParameter (int paramId, void *buf, int len)
{
	return AdslCoreSetOemParameter (paramId, buf, len);
}

int BcmAdslCoreSetXmtGain(unsigned char lineId, int gain)
{
	if ((gain != ADSL_XMT_GAIN_AUTO) && ((gain < -22) || (gain > 2)))
		return BCMADSL_STATUS_ERROR;

#if 1
	if (gain != adslCoreXmtGain[lineId]) {
		adslCoreXmtGain[lineId] = gain;
		adslCoreXmtGainChanged[lineId] = AC_TRUE;
		if (adslCoreConnectionMode[lineId])
			BcmAdslCoreConnectionReset(lineId);
	}
#else
	adslCoreXmtGain = gain;
	BcmAdslCoreSendXmtGainCmd(gain);
#endif
	return BCMADSL_STATUS_SUCCESS;
}


int  BcmAdslCoreGetSelfTestMode(void)
{
	return AdslCoreGetSelfTestMode();
}

void BcmAdslCoreSetSelfTestMode(int stMode)
{
	AdslCoreSetSelfTestMode(stMode);
}

int  BcmAdslCoreGetSelfTestResults(void)
{
	return AdslCoreGetSelfTestResults();
}

int BcmXdslCoreGetLineActive(int lineId)
{
	return adslCoreConnectionMode[lineId];
}


ADSL_LINK_STATE BcmAdslCoreGetEvent (unsigned char lineId)
{
	int		adslState;

	if (0 == acPendingEvents[lineId])
		adslState = -1;
	else if (acPendingEvents[lineId] & ACEV_LINK_UP) {
		adslState = BCM_ADSL_LINK_UP;
		acPendingEvents[lineId] &= ~ACEV_LINK_UP;
	}
	else if (acPendingEvents[lineId] & ACEV_LINK_DOWN) {
		adslState = BCM_ADSL_LINK_DOWN;
		acPendingEvents[lineId] &= ~ACEV_LINK_DOWN;
	}
	else if (acPendingEvents[lineId] & ACEV_G997_FRAME_RCV) {
		adslState = BCM_ADSL_G997_FRAME_RECEIVED;
		acPendingEvents[lineId] &= ~ACEV_G997_FRAME_RCV;
	}
	else if (acPendingEvents[lineId] & ACEV_G997_FRAME_SENT) {
		adslState = BCM_ADSL_G997_FRAME_SENT;
		acPendingEvents[lineId] &= ~ACEV_G997_FRAME_SENT;
	}
	else if (acPendingEvents[lineId] & ACEV_SWITCH_RJ11_PAIR) {
		adslState = ADSL_SWITCH_RJ11_PAIR;
		acPendingEvents[lineId] &= ~ACEV_SWITCH_RJ11_PAIR;
	}
	else if (acPendingEvents[lineId] & ACEV_G994_NONSTDINFO_RECEIVED) {
		adslState = BCM_ADSL_G994_NONSTDINFO_RECEIVED;
		acPendingEvents[lineId] &= ~ACEV_G994_NONSTDINFO_RECEIVED;
	}
	else {
		adslState = BCM_ADSL_EVENT;
		acPendingEvents[lineId] = 0;
	}

	return adslState;
}


Bool BcmAdslCoreG997SendData(unsigned char lineId, void *buf, int len)
{
	Bool	bRet;

	BcmCoreDpcSyncEnter();
	bRet = AdslCoreG997SendData(lineId, buf, len);
	BcmCoreDpcSyncExit();
	return bRet;
}

void *BcmAdslCoreG997FrameGet(unsigned char lineId, int *pLen)
{
	return AdslCoreG997FrameGet(lineId, pLen);
}

void *BcmAdslCoreG997FrameGetNext(unsigned char lineId, int *pLen)
{
	return AdslCoreG997FrameGetNext(lineId, pLen);
}

void BcmAdslCoreG997FrameFinished(unsigned char lineId)
{
	BcmCoreDpcSyncEnter();
	AdslCoreG997FrameFinished(lineId);
	BcmCoreDpcSyncExit();
}



void BcmAdslCoreNotify(unsigned char lineId, long acEvent)
{
	if (ACEV_LINK_POWER_L3 == acEvent) {
#ifdef CONFIG_BCM_DSL_GFAST
		if (XdslMibIsGfastMod(XdslCoreGetDslVars(lineId)) && !acL3Requested[lineId])
			BcmAdslCoreConnectionStart(lineId);
		else
#endif
			BcmXdslCoreSendCmd(lineId, kDslDownCmd, kDslIdleNone);
		if (!acL3Requested[lineId]
#ifdef CONFIG_BCM_DSL_GFAST
			&& !XdslMibIsGfastMod(XdslCoreGetDslVars(lineId))
#endif
			)
		{
			bcmOsGetTime(&acL3StartTick[lineId]);
			if (0 == acL3StartTick[lineId])
				acL3StartTick[lineId]= 1;
		}
		acL3Requested[lineId] = 0;
		return;
	}
	else if (ACEV_RATE_CHANGE == acEvent) {
		BcmXdslNotifyRateChange(lineId);
		return;
	}
	else if (ACEV_REINIT_PHY == acEvent) {
		DiagWriteString(lineId, DIAG_DSL_CLIENT, "BcmAdslCoreNotify: Re-init PHY");
		BcmAdslCoreConnectionStart(lineId);
		return;
	}
	else if(ACEV_FAST_RETRAIN == acEvent) {
		AdslDrvPrintf(TEXT("Line %d: fast retrain started\n"), lineId);
		return;
	}
	
	acPendingEvents[lineId] |= acEvent;
	(*bcmNotify)(lineId);
}

int BcmAdslCoreGetConstellationPoints (int toneId, ADSL_CONSTELLATION_POINT *pointBuf, int numPoints)
{
	return BcmAdslDiagGetConstellationPoints (toneId, pointBuf, numPoints);
}

/*
**
**	ATM EOP workaround functions
**
*/


void BcmAdslCoreAtmSetPortId(int path, int portId)
{
	dslCommandStruct	cmd;
	
	cmd.command = kDslAtmVcControlCmd;
	if(path==0)
		cmd.param.value = kDslAtmSetIntlPortId | portId;
	else if(path==1)
		cmd.param.value = kDslAtmSetFastPortId | portId;
	BcmCoreCommandHandlerSync(&cmd);
}

ulong   atmVcTable[16] = { 0 };
ulong	atmVcCnt = 0;
#define ATM_VC_SIZE		sizeof(atmVcTable)/sizeof(atmVcTable[0])

void BcmCoreAtmVcInit(void)
{
	atmVcCnt = 0;
	AdslMibByteClear(ATM_VC_SIZE, atmVcTable);
}

void BcmCoreAtmVcSet(void)
{
	dslCommandStruct	cmd;
	int					i;

	cmd.command = kDslAtmVcControlCmd;
	cmd.param.value = kDslAtmVcClear;
	BcmCoreCommandHandlerSync(&cmd);

	for (i = 0; i < atmVcCnt; i++) {
		cmd.command = kDslAtmVcControlCmd;
		cmd.param.value = kDslAtmVcAddEntry | atmVcTable[i];
		BcmCoreCommandHandlerSync(&cmd);
	}
}

void BcmAdslCoreAtmClearVcTable(void)
{
	dslCommandStruct	cmd;

	atmVcCnt = 0;
	AdslMibByteClear(ATM_VC_SIZE, atmVcTable);

	cmd.command = kDslAtmVcControlCmd;
	cmd.param.value = kDslAtmVcClear;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreAtmAddVc(int vpi, int vci)
{
	dslCommandStruct	cmd;
	int					i;
	ulong				vc;

	vc = (vpi << 16) | vci;
	for (i = 0; i < atmVcCnt; i++) {
	  if (vc == atmVcTable[i])
		break;
	}
	if ((i == atmVcCnt) && (atmVcCnt < ATM_VC_SIZE)) {
	  atmVcTable[atmVcCnt] = vc;
	  atmVcCnt++;
	}

	cmd.command = kDslAtmVcControlCmd;
	cmd.param.value = kDslAtmVcAddEntry | vc;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreAtmDeleteVc(int vpi, int vci)
{
	dslCommandStruct	cmd;
	int					i;
	ulong				vc;

	vc = (vpi << 16) | vci;
	for (i = 0; i < atmVcCnt; i++) {
	  if (vc == atmVcTable[i]) {
		atmVcTable[i] = atmVcTable[atmVcCnt];
		atmVcCnt--;
		atmVcTable[atmVcCnt] = 0;
		break;
	  }
	}

	cmd.command = kDslAtmVcControlCmd;
	cmd.param.value = kDslAtmVcDeleteEntry | vc;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreAtmSetMaxSdu(unsigned short maxsdu)
{
	dslCommandStruct	cmd;

	cmd.command = kDslAtmVcControlCmd;
	cmd.param.value = kDslAtmSetMaxSDU | maxsdu;
	BcmCoreCommandHandlerSync(&cmd);
}

#if 0
void AddVcTest(ulong vci)
{
	BcmAdslCoreAtmAddVc(vci >> 16, vci & 0xFFFF);
}

void DelVcTest(ulong vci)
{
	BcmAdslCoreAtmDeleteVc(vci >> 16, vci & 0xFFFF);
}
#endif

/*
**
**	DslDiags common functions
**
*/
#ifdef SUPPORT_DSL_BONDING
void	*diagStatDataPtr[MAX_DSL_LINE] = {NULL, NULL};
#else
void	*diagStatDataPtr[MAX_DSL_LINE] = {NULL};
#endif

void BcmAdslCoreDiagWriteStatus(dslStatusStruct *status, char *pBuf, int len)
{
	char			*statStr, *p1;
	ulong			cmd;
	long			n, n1;
	uchar			lineId;
	
	if (!BcmAdslDiagIsActive() && !BcmXdslDiagStatSaveLocalIsActive())
		return;

	statStr = pBuf;
	n = len;
	lineId = DSL_LINE_ID(status->code);

#ifdef SUPPORT_DSL_BONDING
	cmd = statusInfoData | (lineId << DIAG_LINE_SHIFT);
#else
	cmd = statusInfoData;
#endif
	p1 = NULL;
	n1 = 0;

	switch (DSL_STATUS_CODE(status->code)) {
	  case kDslExceptionStatus:
		{
		ulong	*sp;
		sp = (ulong*) status->param.dslException.sp;
		sp = (ulong *) ADSL_ENDIAN_CONV_LONG(sp[28]);
#ifdef FLATTEN_ADDR_ADJUST
		if (((ulong)sp & 0x50000000) == 0x50000000)
		  sp = (ulong *) ((ulong)sp & ~0x40000000);
		sp = (ulong *)ADSL_ADDR_TO_HOST(sp);
#endif
		p1 = (void *) sp;
		n1 = 64 * sizeof(long);
		}
		break;
	  case kDslConnectInfoStatus:
		{
		ulong	bufLen = status->param.dslConnectInfo.value;

		  switch (status->param.dslConnectInfo.code) {
			case kDslChannelResponseLog:
			case kDslChannelResponseLinear:
				bufLen <<= 1;
				/* */
			case kDslChannelQuietLineNoise:
				p1 = status->param.dslConnectInfo.buffPtr;
				n1 = bufLen;
				break;
			case kDslRcvCarrierSNRInfo:
				if (NULL != diagStatDataPtr[lineId]) {
					n  = 3 * 4;
					p1 = diagStatDataPtr[lineId];
					n1 = bufLen << 1;
				}
				break;
			case kG992p2XmtToneOrderingInfo:
			case kG992p2RcvToneOrderingInfo:
			case kG994MessageExchangeRcvInfo:
			case kG994MessageExchangeXmtInfo:
			case kG992MessageExchangeRcvInfo:
			case kG992MessageExchangeXmtInfo:
				if (NULL != diagStatDataPtr[lineId]) {
					n  = 3 * 4;
					p1 = diagStatDataPtr[lineId];
					n1 = bufLen;
				}
				break;
			default:
				break;
		  }
		}
		break;
	  case kDslDspControlStatus:
		switch (status->param.dslConnectInfo.code) {
			case kDslG992RcvShowtimeUpdateGainPtr:
				p1 = status->param.dslConnectInfo.buffPtr;
				n1 = status->param.dslConnectInfo.value<<1;
				((dslStatusStruct *)pBuf)->param.dslConnectInfo.value = ADSL_ENDIAN_CONV_LONG(n1);
				break;
			case kDslPLNPeakNoiseTablePtr:
			case kDslPerBinThldViolationTablePtr:
			case kDslImpulseNoiseDurationTablePtr:
			case kDslImpulseNoiseTimeTablePtr:
			case kDslInpBinTablePtr:
			case kDslItaBinTablePtr:
			case kDslNLNoise:
			case kDslInitializationSNRMarginInfo:
			case kFireMonitoringCounters:
				p1 = status->param.dslConnectInfo.buffPtr;
				n1 = status->param.dslConnectInfo.value;
				break;
		}
		break;

	  case kDslShowtimeSNRMarginInfo:
		if (NULL != diagStatDataPtr[lineId]) {
			n  = sizeof(status->param.dslShowtimeSNRMarginInfo) + 4 - 4;
			p1 = diagStatDataPtr[lineId];
			n1 = status->param.dslShowtimeSNRMarginInfo.nCarriers << 1;
		}
		break;
	  case kAtmStatus:
		if (kAtmStatCounters == status->param.atmStatus.code) {
			n = 3*4;
			((dslStatusStruct *)pBuf)->param.atmStatus.code = ADSL_ENDIAN_CONV_LONG(kAtmStatCounters1);
			p1 = (void *) status->param.atmStatus.param.value;
			n1 = ADSL_ENDIAN_CONV_SHORT(((atmPhyCounters *) p1)->id);
			((dslStatusStruct *)pBuf)->param.atmStatus.param.value = ADSL_ENDIAN_CONV_LONG(n1);
		}
		break;
	}

	if (NULL == p1) {  /* default processing */
		DiagWriteStatus(status, pBuf, len);
		return;
	}


	BcmCoreDpcSyncEnter();

	if(BcmXdslDiagStatSaveLocalIsActive())
		BcmXdslDiagStatSaveLocal(cmd, statStr, n, p1, n1);

	if (!BcmAdslDiagIsActive()) {
		BcmCoreDpcSyncExit();
		return;
	}

	DiagWriteStatusInfo(cmd, statStr, n, p1, n1);
	BcmCoreDpcSyncExit();
}

void BcmAdslCoreWriteOvhMsg(void *gDslVars, char *hdr, dslFrame *pFrame)
{
	dslStatusStruct status;
	dslFrameBuffer	*pBuf, *pBufNext;
	uchar		*pData;
	int			dataLen, i = 0;
	ulong		cmd;
	Boolean		bFirstBuf = true;
#ifndef _NOOS
	if (!BcmAdslDiagIsActive() && !BcmXdslDiagStatSaveLocalIsActive())
		return;
#endif
#ifdef SUPPORT_DSL_BONDING
	cmd = statusInfoData | (gLineId(gDslVars) << DIAG_LINE_SHIFT);
	status.code = ADSL_ENDIAN_CONV_LONG(kDslDspControlStatus | (gLineId(gDslVars) << DSL_LINE_SHIFT));
#else
	cmd = statusInfoData;
	status.code = ADSL_ENDIAN_CONV_LONG(kDslDspControlStatus);
#endif
	if( 'T' == hdr[0] )
		status.param.dslConnectInfo.code = ADSL_ENDIAN_CONV_LONG(kDslTxOvhMsg);
	else
		status.param.dslConnectInfo.code = ADSL_ENDIAN_CONV_LONG(kDslRxOvhMsg);
	status.param.dslConnectInfo.value = ADSL_ENDIAN_CONV_LONG(DslFrameGetLength(gDslVars, pFrame));
	pBuf = DslFrameGetFirstBuffer(gDslVars, pFrame);
#ifdef _NOOS
	{
		static uchar buf[1024];
		if(status.param.dslConnectInfo.value <= sizeof(buf)) {
			while (NULL != pBuf) {
				dataLen   = DslFrameBufferGetLength(gDslVars, pBuf);
				pData = DslFrameBufferGetAddress(gDslVars, pBuf);
				memcpy((void *)(buf+i), pData, dataLen);
				i += dataLen;
				pBuf = DslFrameGetNextBuffer(gDslVars, pBuf);
			}
			status.param.dslConnectInfo.buffPtr = (void *)buf;
			BcmAdslCoreDiagStatusParseAndPrint(&status);
		}
		else {
			AdslDrvPrintf(TEXT("*** BcmAdslCoreWriteOvhMsg: Skip %s, framelen=%d\n"), hdr, (int)status.param.dslConnectInfo.value);
		}

		if(BcmXdslDiagStatSaveLocalIsActive())
					BcmXdslDiagFirstSplitStatSaveLocal(cmd, status.param.dslConnectInfo.value + sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						(char *)&status, sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						pData, dataLen);
	}
#else
#ifdef XDSLDRV_ENABLE_PARSER
	{
		static uchar buf[1024];
		if(status.param.dslConnectInfo.value <= sizeof(buf)) {
			while (NULL != pBuf) {
				dataLen   = DslFrameBufferGetLength(gDslVars, pBuf);
				pData = DslFrameBufferGetAddress(gDslVars, pBuf);
				memcpy((void *)(buf+i), pData, dataLen);
				i += dataLen;
				pBuf = DslFrameGetNextBuffer(gDslVars, pBuf);
			}
			status.param.dslConnectInfo.buffPtr = (void *)buf;
			BcmAdslCoreDiagStatusParseAndPrint(&status);
		}
		else {
			AdslDrvPrintf(TEXT("*** BcmAdslCoreWriteOvhMsg: Skip %s, framelen=%d\n"), hdr, (int)status.param.dslConnectInfo.value);
		}
		return;
	}
#endif
	while (NULL != pBuf) {
		dataLen   = DslFrameBufferGetLength(gDslVars, pBuf);
		pData = DslFrameBufferGetAddress(gDslVars, pBuf);
		pBufNext = DslFrameGetNextBuffer(gDslVars, pBuf);
		if( ++i >= 20 && pBufNext) {
			if(BcmAdslDiagIsActive()) {
				BcmAdslCoreDiagWriteStatusData(cmd -3, pData, dataLen, NULL, 0);
				DiagWriteString(gLineId(gDslVars), DIAG_DSL_CLIENT, " G.997 frame %s: pFr = 0x%X, len = %ld; too many buffer(> %d) in this frame",
					hdr, (int) pFrame, DslFrameGetLength(gDslVars, pFrame), i);
			}
			if(BcmXdslDiagStatSaveLocalIsActive()) {
				BcmXdslDiagLastSplitStatSaveLocal(pData, dataLen);
				DiagWriteString(gLineId(gDslVars), DIAG_DSL_CLIENT, " G.997 frame %s: pFr = 0x%X, len = %ld; too many buffer(> %d) in this frame",
					hdr, (int) pFrame, DslFrameGetLength(gDslVars, pFrame), i);
			}
			break;
		}
		
		if( !bFirstBuf ) {
			if(pBufNext) {
				if(BcmAdslDiagIsActive())
					BcmAdslCoreDiagWriteStatusData(cmd -2, pData, dataLen, NULL, 0);
				if(BcmXdslDiagStatSaveLocalIsActive())
					BcmXdslDiagContSplitStatSaveLocal(pData, dataLen);
			}
			else {
				if(BcmAdslDiagIsActive())
					BcmAdslCoreDiagWriteStatusData(cmd -3, pData, dataLen, NULL, 0);
				if(BcmXdslDiagStatSaveLocalIsActive())
					BcmXdslDiagLastSplitStatSaveLocal(pData, dataLen);
			}
		}
		else {
			if(pBufNext) {
				if(BcmAdslDiagIsActive())
					BcmAdslCoreDiagWriteStatusData(cmd | DIAG_SPLIT_MSG,
						(char *)&status, sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						pData, dataLen);
				if(BcmXdslDiagStatSaveLocalIsActive())
					BcmXdslDiagFirstSplitStatSaveLocal(cmd, status.param.dslConnectInfo.value + sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						(char *)&status, sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						pData, dataLen);
			}
			else {
				if(BcmAdslDiagIsActive())
					BcmAdslCoreDiagWriteStatusData(cmd,
						(char *)&status, sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						pData, dataLen);
				if(BcmXdslDiagStatSaveLocalIsActive())
					BcmXdslDiagStatSaveLocal(cmd,
						(char *)&status, sizeof(status.code) + sizeof(status.param.dslConnectInfo),
						pData, dataLen);
			}
			bFirstBuf = FALSE;
		}
		pBuf = pBufNext;
	}	
#endif /* !_NOOS */
}

#define DIAG_NOCMD_TO	(5000 / BCMOS_MSEC_PER_TICK)
#define DIAG_PING_TIME	(1000 / BCMOS_MSEC_PER_TICK)
#define DIAG_DISC_TIME	(10000 / BCMOS_MSEC_PER_TICK)

void BcmAdslCoreDiagSetSyncTime(ulong syncTime)
{
	BcmCoreDpcSyncEnter();
	gDiagSyncTimeLastCycleCount=BcmAdslCoreGetCycleCount();
	gDiagSyncTimeMs = syncTime+1;
	BcmCoreDpcSyncExit();
	AdslDrvPrintf (TEXT("%s: syncTime=%lu ms %lu sec\n"), __FUNCTION__, syncTime, syncTime/1000);
}

ulong BcmAdslCoreDiagGetSyncTime(void)
{
	ulong	curCycleCount;
	
	BcmCoreDpcSyncEnter();
	curCycleCount = BcmAdslCoreGetCycleCount();
	gDiagSyncTimeMs += ((curCycleCount - gDiagSyncTimeLastCycleCount + (adslCoreCyclesPerMs/2))/adslCoreCyclesPerMs);
	gDiagSyncTimeLastCycleCount = curCycleCount;
	BcmCoreDpcSyncExit();
	
	return gDiagSyncTimeMs/1000;
}

void BcmAdslCoreDiagCmdNotify(void)
{
	bcmOsGetTime(&gDiagLastCmdTime);
}

#ifdef SUPPORT_EXT_DSL_BONDING_SLAVE
#define BCM_DIAGS_IS_CONNECTED	BcmAdslDiagIsConnected
#else
#define BCM_DIAGS_IS_CONNECTED	BcmAdslDiagIsActive
#endif

void BcmAdslCoreDiagConnectionCheck(void)
{
	char	buf[4];
	OS_TICKS	tick;
	ulong	syncTime;
	
	if(BCM_DIAGS_IS_CONNECTED()) {
		bcmOsGetTime(&tick);
		syncTime = BcmAdslCoreDiagGetSyncTime();	/* Also update syncTime */
		if ((tick - gDiagLastCmdTime) > DIAG_NOCMD_TO) {
			if ((tick - gDiagLastCmdTime) > DIAG_DISC_TIME) {
				AdslDrvPrintf (TEXT("DslDiags timeout disconnect: syncTime= %lu sec, to=%lu, currTick=%lu, gDiagLastCmdTime=%lu\n"),
						syncTime, (tick - gDiagLastCmdTime)*BCMOS_MSEC_PER_TICK, tick, gDiagLastCmdTime);
				BcmAdslDiagDisconnect();
			}
			else if ((tick - gDiagLastPingTime) > DIAG_PING_TIME) {
				AdslDrvPrintf (TEXT("DslDiags ping request: syncTime= %lu sec, currTick=%lu, gDiagLastPingTime=%lu gDiagLastCmdTime=%lu\n"),
						syncTime, tick, gDiagLastPingTime, gDiagLastCmdTime);
				gDiagLastPingTime = tick;
				BcmAdslCoreDiagWriteStatusData(LOG_CMD_PING_REQ, buf, 2, NULL, 0);
			}
		}
	}
}

#ifdef XDSLDRV_ENABLE_PARSER

#ifdef XDSLDRV_ENABLE_MIBPRINT
#define abs(n)    ( ((n) ^ ((n) >> 31)) - ((n) >> 31) )
#define DEC_POINT(n)    (n) < 0 ? '-' : ' ', (int)(abs(n) / 10), (int)(abs(n) % 10)

#ifndef CONFIG_VDSL_SUPPORTED
static short scratchBuf[kAdslMibAnnexAToneNum*2*2];
#else
static short scratchBuf[kVdslMibToneNum*2];
#endif

static long f2DecI(long val, int q)
{
   return (val/q);
}

static long f2DecF(long val, int q)
{
   int      sn = val >> 31;
   return (((val ^ sn) - sn) % q);
}

static int GetAdsl2Perq(xdslFramingInfo *p2, int q)
{
	return (0 == p2->M*p2->L) ? -1 : (2*q*p2->T*(p2->M*(p2->B[0]+1)+p2->R)*(p2->U*p2->G))/(p2->M*p2->L);
}

static int GetAdsl2Orq(xdslFramingInfo *p2, int q)
{
	int den = p2->T*(p2->M*(p2->B[0]+1)+p2->R);
	return (0 == den) ? -1 : (4*q*p2->M*p2->L/den);
}

static int GetAdsl2AggrRate(xdslFramingInfo *p, int q)
{
	int       phyrAdj = (1 == p->rtxMode) ? 1 : 0;
//	long long num = (long long) 1024*q*p->L*(p->N - p->R - phyrAdj);
	long num = (long long) 1024*q*p->L*(p->N - p->R - phyrAdj);
	return (0 == p->N) ? -1 : num/(p->N*257);
}

#ifdef CONFIG_VDSL_SUPPORTED
static int GetVdsl2PERp(int vdslProf, xdslFramingInfo *p, int q)
{
//	long long num = (long long) (257*q*p->N*p->T*p->U);
	long num = (long long) (257*q*p->N*p->T*p->U);
	if (kVdslProfile30a == vdslProf)
		num >>= 1;  /* Fs = 8Khz */
	return (0 == p->M*p->L) ? -1 : num/(4*32*p->M*p->L);
}

static int GetVdsl2ORp(int vdslProf, xdslFramingInfo *p, int q)
{
//	long long num = (long long) 1024*q*p->M*p->L*p->G;
	long num = (long long) 1024*q*p->M*p->L*p->G;
	int den = p->T*p->N;

	if (kVdslProfile30a == vdslProf)
		num <<= 1;  /* Fs = 8Khz */
	return (0 == den) ? -1 : num/(den*257);
}

static int GetVdsl2AggrRate(int vdslProf, xdslFramingInfo *p, int q)
{
	int       phyrAdj = (1 == p->rtxMode) ? 1 : 0;
//	long long num = (long long) 1024*q*p->L*(p->N - p->R - phyrAdj);
	long num = (long long) 1024*q*p->L*(p->N - p->R - phyrAdj);

	if (kVdslProfile30a == vdslProf)
		num <<= 1;  /* Fs = 8Khz */
	return (0 == p->N) ? -1 : num/(p->N*257);
}
#endif

static int GetXdsl2PERp(int modType, int vdslProf, xdslFramingInfo *p, int q)
{
#ifdef CONFIG_VDSL_SUPPORTED
	if(kVdslModVdsl2 == modType)
		return GetVdsl2PERp(vdslProf, p, q);
	else
#endif
	return GetAdsl2Perq(p, q);
}

static int GetXdsl2ORp(int modType, int vdslProf, xdslFramingInfo *p, int q)
{
#ifdef CONFIG_VDSL_SUPPORTED
	if(kVdslModVdsl2 == modType)
		return GetVdsl2ORp(vdslProf, p, q);
	else
#endif
	return GetAdsl2Orq(p, q);
}

static int GetXdsl2AgR(int modType, int vdslProf, xdslFramingInfo *p, int q)
{
#ifdef CONFIG_VDSL_SUPPORTED
	if(kVdslModVdsl2 == modType)
		return GetVdsl2AggrRate(vdslProf, p, q);
	else
#endif
	return GetAdsl2AggrRate(p, q);
}

static char * GetStateStatusStr(int status)
{
   char *pBuf = (char *)&scratchBuf[0];
   pBuf[0] = '\0';
   if(kAdslPhysStatusNoDefect == status)
      strcpy(pBuf, "No Defect");
   else {
      if (kAdslPhysStatusLOM & status)
        strcat(pBuf, "Loss Of Margin  ");
      if (kAdslPhysStatusLOF & status)
        strcat(pBuf, "Loss Of Frame  ");
      if (kAdslPhysStatusLOS & status)
        strcat(pBuf, "Loss Of Signal  ");
      if (kAdslPhysStatusLOSQ & status)
        strcat(pBuf, "Loss Of Signal Quality  ");
      if (kAdslPhysStatusLPR & status)
        strcat(pBuf, "Loss Of Power  ");
      if(0 == strlen(pBuf))
         strcpy(pBuf, "Unknown");
   }
   return pBuf;
}

static char * GetTrainingStatusStr(int status)
{
   switch (status) 
   {
   case kAdslTrainingIdle:
      return "Idle";
   case kAdslTrainingG994:
      return "G.994 Training";
   case kAdslTrainingG992Started:
      return "G.992 Started";
   case kAdslTrainingG992ChanAnalysis:
      return "G.922 Channel Analysis";
   case kAdslTrainingG992Exchange:
      return "G.992 Message Exchange";
   case kAdslTrainingG993Started:
      return "G.993 Started";
   case kAdslTrainingG993ChanAnalysis:
      return "G.993 Channel Analysis";
   case kAdslTrainingG993Exchange:
      return "G.993 Message Exchange";
   case kAdslTrainingConnected:
      return "Showtime";
   default:
      return "Unknown";
   }
}

static char * GetModulationStr(int modType)
{
   switch (modType)
   {
   case kAdslModGdmt:
      return "G.DMT";
   case kAdslModT1413:
      return "T1.413";
   case kAdslModGlite:
      return "G.lite";
   case kAdslModAnnexI:
      return "AnnexI";
   case kAdslModAdsl2:
      return "ADSL2";
   case kAdslModAdsl2p:
      return "ADSL2+";
   case kAdslModReAdsl2:
      return "RE-ADSL2";
   case kVdslModVdsl2:
      return "VDSL2";
  case kXdslModGfast:
      return "G.fast";
   default:
      return "Unknown";
   }
}

static unsigned char IsXdsl2(int modType)
{
	unsigned char res = 0;
	
	switch (modType) {
		case kAdslModAdsl2:
		case kAdslModAdsl2p:
		case kAdslModReAdsl2:
		case kVdslModVdsl2:
			res = 1;
			break;
		default:
			break;
	}
	return res;
}

static char *GetAnnexTypeStr(int annexType)
{
	char *res = "";
	
	switch(annexType) {
		case kAdslTypeAnnexA:
			res ="Annex A";
			break;
		case kAdslTypeAnnexB:
			res ="Annex B";
			break;
		case kAdslTypeAnnexC:
			res ="Annex C";
			break;
		case kAdslTypeSADSL:
			res ="Annex SADSL";
			break;
		case kAdslTypeAnnexI:
			res ="Annex I";
			break;
		case kAdslTypeAnnexAB:
			res ="Annex AB";
			break;
	}
	
	return res;
}

#ifdef CONFIG_VDSL_SUPPORTED
static char * GetSelectedProfileStr(unsigned short vdsl2Profile)
{
	char *res = "Unknown";
	
	switch(vdsl2Profile) {
		case kVdslProfile8a:
			res = "Profile 8a";
			break;
		case kVdslProfile8b:
			res = "Profile 8b";
			break;
		case kVdslProfile8c:
			res = "Profile 8c";
			break;
		case kVdslProfile8d:
			res = "Profile 8d";
			break;
		case kVdslProfile12a:
			res = "Profile 12a";
			break;
		case kVdslProfile12b:
			res = "Profile 12b";
			break;
		case kVdslProfile17a:
			res = "Profile 17a";
			break;
		case kVdslProfile30a:
			res = "Profile 30a";
			break;
	}
	return res;
}

#endif

static char *GetConnectionTypeStr(unsigned char tmType)
{
	return ( kXdslDataPtm == tmType ) ? "PTM Mode":
			((kXdslDataAtm == tmType) || (kXdslDataNitro == tmType)) ? "ATM Mode":
			(kXdslDataRaw == tmType) ? "RAW Data Mode": "Not connected";
}

static char * GetModulationStr2(int modType, int xdslType)
{
   static char annexMStr[] = "AnnexM EU-  ";

#ifdef CONFIG_VDSL_SUPPORTED
    if(kVdslModVdsl2 == modType)
        return GetAnnexTypeStr(xdslType >> kXdslModeAnnexShift);
#endif

   if (xdslType & kAdsl2ModeAnnexMask)
   {
      adslVersionInfo adslVer;
      int mVal = 32 + (((xdslType & kAdsl2ModeAnnexMask) - 1) << 2);
      BcmAdslCoreGetVersion(&adslVer);
      annexMStr[5] = (kAdslTypeAnnexB == adslVer.phyType) ? 'J': 'M';
      annexMStr[sizeof(annexMStr)-3] = '0' + mVal/10;
      annexMStr[sizeof(annexMStr)-2] = '0' + mVal%10;
      return annexMStr;
   }
   else
      return GetAnnexTypeStr(xdslType >> kXdslModeAnnexShift);
}

void BcmCoreAdslMibPrint(void)
{
	xdslFramingInfo	*pRxFramingParam, *pTxFramingParam;
	adslMibInfo		*adslMib = NULL;
	long				size = sizeof(adslMibInfo);
	int				modType, vdslProf, modVdsl2=0, pathId=0;

	adslMib = (void *) BcmAdslCoreGetObjectValue (0, NULL, 0, NULL, &size);

	modType = adslMib->adslConnection.modType;
#ifdef CONFIG_VDSL_SUPPORTED
	vdslProf = adslMib->xdslInfo.vdsl2Profile;
	if(kVdslModVdsl2 == modType)
		modVdsl2 = 1;
#else
	vdslProf = 0;
#endif

	DiagWriteString(0, DIAG_DSL_CLIENT,"Link Power State:\tL%d\n", adslMib->xdslInfo.pwrState);

	if (kAdslTrainingConnected == adslMib->adslTrainingState) {
		int	rK = adslMib->adslConnection.rcvInfo.K;
		int	xK = adslMib->adslConnection.xmtInfo.K;
		char	trellisString[80];
		if(adslMib->adslConnection.modType<kAdslModAdsl2 ) {
			if(adslMib->adslConnection.trellisCoding ==kAdslTrellisOn)
				strcpy(trellisString,"ON");
			else strcpy(trellisString,"OFF");
		}
		else {
			if(0 == (adslMib->adslConnection.trellisCoding2 & kAdsl2TrellisTxEnabled))
				strcpy(trellisString,"U:OFF");
			else
				strcpy(trellisString,"U:ON");
			if(0 == (adslMib->adslConnection.trellisCoding2 & kAdsl2TrellisRxEnabled))
				strcat(trellisString," /D:OFF");
			else
				strcat(trellisString," /D:ON");
		}

		DiagWriteString(0, DIAG_DSL_CLIENT,"Mode:\t\t\t%s %s\n",
			GetModulationStr(adslMib->adslConnection.modType),
			GetModulationStr2(adslMib->adslConnection.modType, adslMib->xdslInfo.xdslMode));
#ifdef CONFIG_VDSL_SUPPORTED
		if(modVdsl2)
			DiagWriteString(0, DIAG_DSL_CLIENT,	"VDSL2 Profile:\t\t%s\n", GetSelectedProfileStr(vdslProf));
#endif
		DiagWriteString(0, DIAG_DSL_CLIENT,"TPS-TC:\t\t\t%s(0x%X)\n", GetConnectionTypeStr(adslMib->xdslInfo.dirInfo[1].lpInfo[0].tmType[0]), adslMib->xdslInfo.dirInfo[1].lpInfo[0].tpsTcOptions);
		DiagWriteString(0, DIAG_DSL_CLIENT,
			"Trellis:\t\t%s\n"
			"Line Status:\t\t%s\n"
			"Training Status:\t%s\n"
			"\t\tDown\t\tUp\n"
			"SNR (dB):\t%c%d.%d\t\t%c%d.%d\n"
			"Attn(dB):\t%c%d.%d\t\t%c%d.%d\n"
			"Pwr(dBm):\t%c%d.%d\t\t%c%d.%d\n",

			trellisString,
			GetStateStatusStr(adslMib->adslPhys.adslCurrStatus),
			GetTrainingStatusStr(adslMib->adslTrainingState),

			DEC_POINT(adslMib->adslPhys.adslCurrSnrMgn), DEC_POINT(adslMib->adslAtucPhys.adslCurrSnrMgn),
			DEC_POINT(adslMib->adslPhys.adslCurrAtn), DEC_POINT(adslMib->adslAtucPhys.adslCurrAtn),
			DEC_POINT(adslMib->adslAtucPhys.adslCurrOutputPwr), DEC_POINT(adslMib->adslPhys.adslCurrOutputPwr));

		if(!IsXdsl2(adslMib->adslConnection.modType)) {
			pRxFramingParam = &adslMib->xdslInfo.dirInfo[0].lpInfo[0];
			pTxFramingParam = &adslMib->xdslInfo.dirInfo[1].lpInfo[0];
			DiagWriteString(0, DIAG_DSL_CLIENT,
				"\t\t\tG.dmt framing\n"
				"K:\t\t%d(%d)\t\t%d\n"
				"R:\t\t%d\t\t%d\n"
				"S:\t\t%ld.%04ld\t\t%ld.%04ld\n"
				"D:\t\t%d\t\t%d\n",
				rK, adslMib->adslRxNonStdFramingAdjustK, xK,
				pRxFramingParam->R, pTxFramingParam->R,
#if 1
				(pRxFramingParam->S.denom) ? f2DecI(pRxFramingParam->S.num, pRxFramingParam->S.denom), f2DecF(pRxFramingParam->S.num, pRxFramingParam->S.denom) : 0, 0,
				(pTxFramingParam->S.denom) ? f2DecI(pTxFramingParam->S.num, pTxFramingParam->S.denom), f2DecF(pTxFramingParam->S.num, pTxFramingParam->S.denom) : 0, 0,
#else
				0, 0, 0, 0,
#endif
				pRxFramingParam->D, pTxFramingParam->D);
		}
		else {
			if( modVdsl2 )
				DiagWriteString(0, DIAG_DSL_CLIENT,"\n\t\t\tVDSL2 framing\n");
			else
				DiagWriteString(0, DIAG_DSL_CLIENT,"\n\t\t\tADSL2 framing\n");
			for(pathId = 0; pathId < MAX_LP_NUM; pathId++) {
				if((0==pathId) || (adslMib->lp2Active || adslMib->lp2TxActive) ) {
					pRxFramingParam = &adslMib->xdslInfo.dirInfo[0].lpInfo[pathId];
					pTxFramingParam = &adslMib->xdslInfo.dirInfo[1].lpInfo[pathId];
#if 0
					if(0 == pRxFramingParam->S.denom) {
						pRxFramingParam->S.denom = 1;
						pRxFramingParam->S.num 0;
					}
					if(0 == pTxFramingParam->S.denom) {
						pTxFramingParam->S.denom = 1;
						pTxFramingParam->S.num = 0;
					}
#endif
					DiagWriteString(0, DIAG_DSL_CLIENT,
					"\t\t\tBearer %d\n"
					"MSGc:\t\t%d\t\t%d\n"
					"B:\t\t%d\t\t%d\n"
					"M:\t\t%d\t\t%d\n"
					"T:\t\t%d\t\t%d\n"
					"R:\t\t%d\t\t%d\n"
					"S:\t\t%ld.%04ld\t\t%ld.%04ld\n"
					"L:\t\t%d\t\t%d\n"
					"D:\t\t%d\t\t%d\n",
					pathId,
					pRxFramingParam->U*pRxFramingParam->G-6, pTxFramingParam->U*pTxFramingParam->G-6,
					pRxFramingParam->B[0], pTxFramingParam->B[0],
					pRxFramingParam->M, pTxFramingParam->M,
					pRxFramingParam->T, pTxFramingParam->T,
					pRxFramingParam->R, pTxFramingParam->R,
#if 1
					(pRxFramingParam->S.denom) ? f2DecI(pRxFramingParam->S.num, pRxFramingParam->S.denom), f2DecF(pRxFramingParam->S.num, pRxFramingParam->S.denom) : 0, 0,
					(pTxFramingParam->S.denom) ? f2DecI(pTxFramingParam->S.num, pTxFramingParam->S.denom), f2DecF(pTxFramingParam->S.num, pTxFramingParam->S.denom) : 0, 0,
#else
					0, 0, 0, 0,
#endif
					pRxFramingParam->L, pTxFramingParam->L,
					pRxFramingParam->D, pTxFramingParam->D);

					if( modVdsl2 ) {
						DiagWriteString(0, DIAG_DSL_CLIENT,
						"I:\t\t%d\t\t%d\n"
						"N:\t\t%d\t\t%d\n",
						pRxFramingParam->I, pTxFramingParam->I,
						pRxFramingParam->N, pTxFramingParam->N);
					}
					if(adslMib->xdslStat[0].ginpStat.status & 0xC) {
						DiagWriteString(0, DIAG_DSL_CLIENT,
						"Q:\t\t%d\t\t%d\n"
						"V:\t\t%d\t\t%d\n"
						"RxQueue:\t\t%d\t\t%d\n"
						"TxQueue:\t\t%d\t\t%d\n"
						"G.INP Framing:\t\t%d\t\t%d\n"
						"G.INP lookback:\t\t%d\t\t%d\n"
						"RRC bits:\t\t%d\t\t%d\n",
						pRxFramingParam->Q, pTxFramingParam->Q,
						pRxFramingParam->V, pTxFramingParam->V,
						pRxFramingParam->rxQueue, pTxFramingParam->rxQueue,
						pRxFramingParam->txQueue, pTxFramingParam->txQueue,
						pRxFramingParam->rtxMode, pTxFramingParam->rtxMode,
						pRxFramingParam->ginpLookBack, pTxFramingParam->ginpLookBack,
						pRxFramingParam->rrcBits, pTxFramingParam->rrcBits);
					}
				}
			}
		}

		DiagWriteString(0, DIAG_DSL_CLIENT,"\n\t\t\tCounters\n");
		for(pathId = 0; pathId < MAX_LP_NUM; pathId++) {
			if((0==pathId) || (adslMib->lp2Active || adslMib->lp2TxActive) ) {
				pRxFramingParam = &adslMib->xdslInfo.dirInfo[0].lpInfo[pathId];
				pTxFramingParam = &adslMib->xdslInfo.dirInfo[1].lpInfo[pathId];
				DiagWriteString(0, DIAG_DSL_CLIENT,
				"\t\t\tBearer %d\n"
				"%s:\t\t%u\t\t%u\n"
				"%s:\t\t%u\t\t%u\n",
				pathId,
				(modVdsl2)? "OHF" : "SF",
				(unsigned int)adslMib->xdslStat[pathId].rcvStat.cntSF,(unsigned int)adslMib->xdslStat[pathId].xmtStat.cntSF,
				(modVdsl2)? "OHFErr" : "SFErr",
				(unsigned int)adslMib->xdslStat[pathId].rcvStat.cntSFErr, (unsigned int)adslMib->xdslStat[pathId].xmtStat.cntSFErr);
				
				DiagWriteString(0, DIAG_DSL_CLIENT,
					"RS:\t\t%u\t\t%u\n"
					"RSCorr:\t\t%u\t\t%u\n"
					"RSUnCorr:\t%u\t\t%u\n",
					(unsigned int)adslMib->xdslStat[pathId].rcvStat.cntRS, (unsigned int)adslMib->xdslStat[pathId].xmtStat.cntRS,
					(unsigned int) adslMib->xdslStat[pathId].rcvStat.cntRSCor,(unsigned int) adslMib->xdslStat[pathId].xmtStat.cntRSCor,
					(unsigned int)adslMib->xdslStat[pathId].rcvStat.cntRSUncor, (unsigned int)adslMib->xdslStat[pathId].xmtStat.cntRSUncor);
			}
		}

		if(adslMib->xdslStat[0].ginpStat.status & 0xC) {
			DiagWriteString(0, DIAG_DSL_CLIENT,
				"\n\t\t\tRetransmit Counters\n"
				"rtx_tx:\t\t%u\t\t%u\n"
				"rtx_c:\t\t%u\t\t%u\n"
				"rtx_uc:\t\t%u\t\t%u\n",
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.rtx_tx, (unsigned int)adslMib->adslStat.ginpStat.cntUS.rtx_tx,
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.rtx_c, (unsigned int)adslMib->adslStat.ginpStat.cntUS.rtx_c,
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.rtx_uc, (unsigned int)adslMib->adslStat.ginpStat.cntUS.rtx_uc);
			DiagWriteString(0, DIAG_DSL_CLIENT,
				"\n\t\t\tG.INP Counters\n"
				"LEFTRS:\t\t%u\t\t%u\n"
				"minEFTR:\t%u\t\t%u\n"
				"errFreeBits:\t%u\t\t%u\n",
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.LEFTRS, (unsigned int)adslMib->adslStat.ginpStat.cntUS.LEFTRS,
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.minEFTR, (unsigned int)adslMib->adslStat.ginpStat.cntUS.minEFTR,
				(unsigned int)adslMib->adslStat.ginpStat.cntDS.errFreeBits, (unsigned int)adslMib->adslStat.ginpStat.cntUS.errFreeBits);
		}
		else
		/* 1 - kFireDsEnabled, 2 -kFireUsEnabled */
		if(adslMib->xdslStat[0].fireStat.status & 0x3) {
			DiagWriteString(0, DIAG_DSL_CLIENT,
				"\n"
				"ReXmt:\t\t%u\t\t%u\n"
				"ReXmtCorr:\t%u\t\t%u\n"
				"ReXmtUnCorr:\t%u\t\t%u\n",
				(unsigned int)adslMib->adslStat.fireStat.reXmtRSCodewordsRcved, (unsigned int)adslMib->adslStat.fireStat.reXmtRSCodewordsRcvedUS,
				(unsigned int)adslMib->adslStat.fireStat.reXmtCorrectedRSCodewords, (unsigned int)adslMib->adslStat.fireStat.reXmtCorrectedRSCodewordsUS,
				(unsigned int)adslMib->adslStat.fireStat.reXmtUncorrectedRSCodewords, (unsigned int)adslMib->adslStat.fireStat.reXmtUncorrectedRSCodewordsUS);
		}

		for(pathId = 0; pathId < MAX_LP_NUM; pathId++) {
			if((0==pathId) || (adslMib->lp2Active || adslMib->lp2TxActive) ) {
				DiagWriteString(0, DIAG_DSL_CLIENT,
				"\n"
				"\t\t\tBearer %d\n"
				"HEC:\t\t%u\t\t%u\n"
				"OCD:\t\t%u\t\t%u\n"
				"LCD:\t\t%u\t\t%u\n"
				"Total Cells:\t%u\t\t%u\n"
				"Data Cells:\t%u\t\t%u\n"
				"Drop Cells:\t%u\n"
				"Bit Errors:\t%u\t\t%u\n",
				pathId,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntHEC, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntHEC,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntOCD, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntOCD,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntLCD, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntLCD,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntCellTotal, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntCellTotal,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntCellData, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntCellData,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntCellDrop,
				(unsigned int)adslMib->atmStat2lp[pathId].rcvStat.cntBitErrs, (unsigned int)adslMib->atmStat2lp[pathId].xmtStat.cntBitErrs);
			}
		}
		
		DiagWriteString(0, DIAG_DSL_CLIENT,
			"\n"
			"ES:\t\t%u\t\t%u\n"
			"SES:\t\t%u\t\t%u\n"
			"UAS:\t\t%u\t\t%u\n"
			"AS:\t\t%u\n"
			"\n",
			(unsigned int)adslMib->adslPerfData.perfTotal.adslESs, (unsigned int)adslMib->adslTxPerfTotal.adslESs,
			(unsigned int)adslMib->adslPerfData.perfTotal.adslSES,(unsigned int) adslMib->adslTxPerfTotal.adslSES,
			(unsigned int)adslMib->adslPerfData.perfTotal.adslUAS, (unsigned int)adslMib->adslTxPerfTotal.adslUAS,
			(unsigned int)adslMib->adslPerfData.perfSinceShowTime.adslAS);
		
		for(pathId = 0; pathId < MAX_LP_NUM; pathId++) {
			if((0==pathId) || (adslMib->lp2Active || adslMib->lp2TxActive) ) {
				pRxFramingParam = &adslMib->xdslInfo.dirInfo[0].lpInfo[pathId];
				pTxFramingParam = &adslMib->xdslInfo.dirInfo[1].lpInfo[pathId];
				DiagWriteString(0, DIAG_DSL_CLIENT,
					"\t\t\tBearer %d\n"
					"INP:\t\t%ld.%02ld\t\t%ld.%02ld\n"
					"INPRein:\t%ld.%02ld\t\t%ld.%02ld\n"
					"delay:\t\t%ld\t\t%ld\n"
					"PER:\t\t%ld.%02ld\t\t%ld.%02ld\n"
					"OR:\t\t%ld.%02ld\t\t%ld.%02ld\n"
					"AgR:\t\t%ld.%02ld\t%ld.%02ld\n"
					"\n",
					pathId,
					f2DecI(pRxFramingParam->INP, 2), f2DecF(pRxFramingParam->INP, 2), f2DecI(pTxFramingParam->INP, 2), f2DecF(pTxFramingParam->INP, 2),
					f2DecI(pRxFramingParam->INPrein, 2), f2DecF(pRxFramingParam->INPrein, 2), f2DecI(pTxFramingParam->INPrein, 2), f2DecF(pTxFramingParam->INPrein, 2),
					(long)pRxFramingParam->delay, (long)pTxFramingParam->delay,
					f2DecI(GetXdsl2PERp(modType, vdslProf, pRxFramingParam,100),100), f2DecF(GetXdsl2PERp(modType, vdslProf, pRxFramingParam,100),100),
					f2DecI(GetXdsl2PERp(modType, vdslProf, pTxFramingParam,100),100), f2DecF(GetXdsl2PERp(modType, vdslProf, pTxFramingParam,100),100),
					f2DecI(GetXdsl2ORp(modType, vdslProf, pRxFramingParam,100),100), f2DecF(GetXdsl2ORp(modType, vdslProf, pRxFramingParam,100),100),
					f2DecI(GetXdsl2ORp(modType, vdslProf, pTxFramingParam,100),100), f2DecF(GetXdsl2ORp(modType, vdslProf, pTxFramingParam,100),100),
					f2DecI(GetXdsl2AgR(modType, vdslProf, pRxFramingParam,100),100), f2DecF(GetXdsl2AgR(modType, vdslProf, pRxFramingParam,100),100),
					f2DecI(GetXdsl2AgR(modType, vdslProf, pTxFramingParam,100),100), f2DecF(GetXdsl2AgR(modType, vdslProf, pTxFramingParam,100),100)
					);
			}
		}
		DiagWriteString(0, DIAG_DSL_CLIENT,"Bitswap:\t%u/%u\t\t%u/%u\n\n", (unsigned int)adslMib->adslStat.bitswapStat.rcvCnt, (unsigned int)adslMib->adslStat.bitswapStat.rcvCntReq,
			(unsigned int)adslMib->adslStat.bitswapStat.xmtCnt, (unsigned int)adslMib->adslStat.bitswapStat.xmtCntReq);
	}
}
#endif

#if defined(_NOOS) || defined(XDSL_DRV_STATUS_POLLING)
extern void stop_TEST(unsigned int cpuId);
extern unsigned int get_cpuid(void);
#endif
extern void DiagUnflattenStatus1(long *pFlatStatus, int statusLen, void *pStat, void *pOutBuf, BOOL diagClientLE);
extern void StatusParser(modemStatusStruct *status, char *dstPtr);
int parsedCnt = 0;
char parsedBuf[4096<<4];

void BcmAdslCoreDiagStatusParseAndPrint(dslStatusStruct *status)
{
	int	parsedStrLen;
	parsedBuf[0] = 0;
	StatusParser((modemStatusStruct *)status, parsedBuf);
	parsedStrLen = strlen(parsedBuf);
	if(parsedStrLen > 0) {
		if(parsedStrLen >= 3072) {
			AdslDrvPrintf(TEXT("Truncate parsed string len(%d) to 3072 bytes\n"), parsedStrLen);
			parsedBuf[3072] = 0;
		}
		AdslDrvPrintf(TEXT("StatusParser: %s\n"), parsedBuf);
	}
}

void BcmAdslCoreDiagStatusSnooper(dslStatusStruct *status, char *pBuf, int len)
{
	int	parsedStrLen;
	if((kFirstDslStatusCode + 200) == status->code) {
		AdslDrvPrintf(TEXT("StatusParser: Status playback completed\n"));
#if defined(_NOOS) || defined(XDSL_DRV_STATUS_POLLING)
		stop_TEST(get_cpuid());
#endif
		return;
	}
	else if((kFirstDslStatusCode + 201) == status->code) {
#ifdef XDSLDRV_ENABLE_MIBPRINT
		BcmCoreAdslMibPrint();
#endif
		return;
	}
	
	parsedBuf[0] = 0;
	DiagUnflattenStatus1((long *)pBuf, len, (void *)status, parsedBuf, 1);
	StatusParser((modemStatusStruct *)status, parsedBuf);
	parsedStrLen = strlen(parsedBuf);
	if(parsedStrLen > 0) {
		if(parsedStrLen >= 3072) {
			AdslDrvPrintf(TEXT("Truncate parsed string len(%d) to 3072 bytes\n"), parsedStrLen);
			parsedBuf[3072] = 0;
		}
		AdslDrvPrintf(TEXT("StatusParser: %s\n"), parsedBuf);
	}
}

#else

void BcmAdslCoreDiagStatusSnooper(dslStatusStruct *status, char *pBuf, int len)
{
	unsigned char lineId;
	Bool bStatPtrSet = false;

	if (!BcmAdslDiagIsActive() && !BcmXdslDiagStatSaveLocalIsActive())
		return;
#ifdef XDSLDRV_ENABLE_MIBPRINT
	if((kFirstDslStatusCode + 201) == status->code) {
		BcmCoreAdslMibPrint();
		return;
	}
#endif

	BcmAdslCoreDiagWriteStatus(status, pBuf, len);
	
	lineId = DSL_LINE_ID(status->code);
	
	switch (DSL_STATUS_CODE(status->code)) {
#ifdef ADSLDRV_STATUS_PROFILING
		case kDslReceivedEocCommand:
			if ( kStatusBufferHistogram==status->param.dslClearEocMsg.msgId )
				BcmXdslCoreDrvProfileInfoPrint();
			break;
#endif
		case kDslDspControlStatus:
			if (kDslStatusBufferInfo == status->param.dslConnectInfo.code) {
				diagStatDataPtr[lineId] = status->param.dslConnectInfo.buffPtr;
				bStatPtrSet = true;
			}
			break;
		case kDslExceptionStatus:
			BcmAdslCoreDiagWriteStatusString(lineId, "Resetting ADSL MIPS\n");
			break;
		default:
			break;
	}
	if (!bStatPtrSet)
		diagStatDataPtr[lineId] = NULL;
}
#endif

void BcmAdslCoreDiagSaveStatusString(char *fmt, ...)
{
#ifndef _NOOS
	dslStatusStruct 	status;
	va_list 			ap;
	
	if(!BcmXdslDiagStatSaveLocalIsActive())
		return;
	
	va_start(ap, fmt);

	status.code = kDslPrintfStatusSaveLocalOnly;
	status.param.dslPrintfMsg.fmt = fmt;
	status.param.dslPrintfMsg.argNum = 0;
#if defined(__arm) || defined(CONFIG_ARM)
	status.param.dslPrintfMsg.argPtr = (void *)ap.__ap;
#else
	status.param.dslPrintfMsg.argPtr = (void *)ap;
#endif
	va_end(ap);

	DiagWriteStatus(&status, NULL, 0);
#endif
}

#ifndef BcmAdslCoreDiagWriteStatusString
void BcmAdslCoreDiagWriteStatusString(unsigned char lineId, char *fmt, ...)
{
	dslStatusStruct 	status;
	va_list 			ap;
	
	va_start(ap, fmt);
	
	status.code = kDslPrintfStatus | (lineId << DSL_LINE_SHIFT);
	status.param.dslPrintfMsg.fmt = fmt;
	status.param.dslPrintfMsg.argNum = 0;
	status.param.dslPrintfMsg.argPtr = (void *)ap;
	va_end(ap);
	
	BcmAdslCoreDiagWriteStatus(&status, NULL, 0);
}
#endif

/* common diag command handler */

static void BcmDiagPrintConfigCmd (char *hdr, adslCfgProfile *pCfg)
{
	ulong	cfg;

	AdslDrvPrintf (TEXT("DrvDiag: %s: CCFG=0x%lX ACFG=0x%lX"), hdr, pCfg->adslAnnexCParam, pCfg->adslAnnexAParam);
#ifndef G992_ANNEXC
	cfg = pCfg->adslAnnexAParam;
#else
	cfg = pCfg->adslAnnexCParam;
#endif
	if (cfg & kAdslCfgExtraData) {
		AdslDrvPrintf (TEXT(" TrM=0x%lX ShM=0x%lX MgTo=%ld"), pCfg->adslTrainingMarginQ4, pCfg->adslShowtimeMarginQ4, pCfg->adslLOMTimeThldSec);
		if (cfg & kAdslCfgDemodCapOn)
			AdslDrvPrintf (TEXT(", DemodMask=0x%lX DemodVal=0x%lX\n"), pCfg->adslDemodCapMask, pCfg->adslDemodCapValue);
		else
			AdslDrvPrintf (TEXT("\n"));
	}
	else
		AdslDrvPrintf (TEXT("\n"));
}

typedef struct _diagEyeCfgStruct {
	short		toneMain;
	short		tonePilot;
	long			reserved;
	long			reserved1;
	BOOL		enabled;
} diagEyeCfgStruct;

void BcmAdslCoreDiagCmdCommon(unsigned char lineId, int diagCmd, int len, void *pCmdData)
{
	dslCommandStruct	cmd;
	char		*pInpBins;
	char		*pItaBins;
	long		size;
	int		res, n;
	uchar		*pOid;
	uchar		*pObj;
	/* AdslDrvPrintf (TEXT("DrvDiagCmd: %d\n"), diagCmd); */
	memset((void*)&cmd, 0, sizeof(cmd));

	switch (diagCmd) {
		case LOG_CMD_MIB_GET1:
		{
			char *pName = (char *)pCmdData;
			pOid = (uchar *)((int)pCmdData + strlen(pName) + 1);
			n = ((int)pCmdData + len) - (int)pOid;	/* oid length */
			if( n < 2 ) {
				BcmAdslCoreDiagWriteStatusString(lineId, "LOG_CMD_MIB_GET1: Invalid oid len(%d)\n", n);
				break;
			}
			res = BcmAdslCoreGetObjectValue (lineId, pOid, n, NULL, &size);
			if (kAdslMibStatusNoObject != res) {
				DiagWriteString(lineId, DIAG_DSL_CLIENT, "LOG_CMD_MIB_GET1: filename=%s dataLen=%d 0x%X 0x%X 0x%X\n",
					pName, size,*(ulong*)res,*((ulong*)res+1),*((ulong*)res+2));
				BcmAdslCoreDiagWriteFile(lineId, pName, (void *)res, size);
			}
			break;
		}
		case LOG_CMD_MIB_GET:
			if (len != 0)
				memmove((void *)((int)pCmdData+2), pCmdData, len);
			((uchar *)pCmdData)[0] = len & 0xFF;
			((uchar *)pCmdData)[1] = (len >> 8) & 0xFF;
			pOid = (uchar *)pCmdData+2;
			size = LOG_MAX_DATA_SIZE - (2+len+2);  /* Make sure pCmdData size is at least LOG_MAX_DATA_SIZE => DoDiagCommand() */
			res = BcmAdslCoreGetObjectValue (lineId, (0 == len ? NULL : pOid), len, pOid+len+2, &size);
			if (kAdslMibStatusSuccess == res) {
				pOid[len+0] = size & 0xFF;
				pOid[len+1] = (size >> 8) & 0xFF;
				BcmAdslCoreDiagWriteStatusData(LOG_CMD_MIB_GET | (lineId << DIAG_LINE_SHIFT), pCmdData, 2+len+2+size, NULL, 0);
			}
			else if ((kAdslMibStatusBufferTooSmall == res) && (kOidAdslPrivate == pOid[0])) {
				int	i=0;
				pObj = (void *) BcmAdslCoreGetObjectValue (lineId, (0 == len ? NULL : pOid), len, NULL, (long *)&size);
				
				n = LOG_MAX_DATA_SIZE - (2+len+2);	/* Available buffer size for MIB data */
				pOid[0] = kOidAdslPrivatePartial;
				pOid[len+0] = n & 0xFF;
				pOid[len+1] = (n >> 8) & 0xFF;
				
				BcmCoreDpcSyncEnter();
				while( size> n )
				{
					memcpy (pOid+len+2, pObj+i*n, n);
					BcmAdslCoreDiagWriteStatusData(LOG_CMD_MIB_GET | (lineId << DIAG_LINE_SHIFT), pCmdData, 2+len+2+n, NULL, 0);
					size -= n;
					i++;
				}
				
				if(size) {
					pOid[len+0] = size & 0xFF;
					pOid[len+1] = (size >> 8) & 0xFF;
					memcpy (pOid+len+2, pObj+i*n, size);
					BcmAdslCoreDiagWriteStatusData(LOG_CMD_MIB_GET | (lineId << DIAG_LINE_SHIFT), pCmdData, 2+len+2+size, NULL, 0);
				}
				
				pOid[len+0] = 0;
				pOid[len+1] = 0;
				BcmAdslCoreDiagWriteStatusData(LOG_CMD_MIB_GET | (lineId << DIAG_LINE_SHIFT), pCmdData, 2+len+2, NULL, 0);
				BcmCoreDpcSyncExit();
			}
			else {
				BcmAdslCoreDiagWriteStatusString(lineId, "LOG_CMD_MIB_GET Error %d: oidLen=%d, oid = %d %d %d %d %d", 
					res, len, pOid[0], pOid[1], pOid[2], pOid[3], pOid[4]);
				pOid[len+0] = 0;
				pOid[len+1] = 0;
				BcmAdslCoreDiagWriteStatusData(LOG_CMD_MIB_GET | (lineId << DIAG_LINE_SHIFT), pCmdData, 2+len+2, NULL, 0);
			}
			break;

		case LOG_CMD_EYE_CFG:
			{
			diagEyeCfgStruct *pEyeCfg = (diagEyeCfgStruct *)pCmdData;

#if !defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338)
			DiagWriteString(lineId, DIAG_DSL_CLIENT, "%s: LOG_CMD_EYE_CFG - toneMain=%d tonePilot=%d enabled=%d",
				__FUNCTION__, pEyeCfg->toneMain, pEyeCfg->tonePilot, pEyeCfg->enabled);
			if(pEyeCfg->enabled)
				BcmAdslCoreDiagSetBufDesc(lineId);
#endif
#ifdef SUPPORT_DSL_BONDING
			cmd.command = kDslDiagSetupCmd | (lineId << DSL_LINE_SHIFT);
#else
			cmd.command = kDslDiagSetupCmd;
#endif
			cmd.param.dslDiagSpec.setup = pEyeCfg->enabled;
			cmd.param.dslDiagSpec.eyeConstIndex1 = (pEyeCfg->reserved & 0xFFFF0000) | pEyeCfg->toneMain;
			cmd.param.dslDiagSpec.eyeConstIndex2 = (pEyeCfg->reserved1 & 0xFFFF0000) | pEyeCfg->tonePilot;
			cmd.param.dslDiagSpec.logTime = 0;
			BcmCoreCommandHandlerSync(&cmd);
#if !defined(CONFIG_BCM96338) && !defined(CONFIG_BCM96348)
			if(0 == pEyeCfg->enabled)
				BcmAdslDiagEnablePrintStatCmd();
#endif
			}
			break;

		case LOG_CMD_SWITCH_RJ11_PAIR:
			AdslDrvPrintf (TEXT("DrvDiag: LOG_CMD_SWITCH_RJ11_PAIR\n"));
			BcmAdslCoreNotify(lineId, ACEV_SWITCH_RJ11_PAIR);
			break;

		case LOG_CMD_CFG_PHY:
		{
			adslPhyCfg	*pCfg = pCmdData;

			AdslDrvPrintf (TEXT("CFG_PHY CMD: demodCapabilities=0x%08X  mask=0x%08X  demodCap=0x%08X\n"),
				(unsigned int)adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities,
				(unsigned int)pCfg->demodCapMask, (unsigned int)pCfg->demodCap);
			if(adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities == pCfg->demodCap) {
				AdslDrvPrintf (TEXT("CFG_PHY CMD: No change, do nothing\n"));
				break;
			}
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities |= 
				pCfg->demodCapMask & pCfg->demodCap;
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities &= 
				~(pCfg->demodCapMask & ~pCfg->demodCap);

			adslCoreCfgProfile[lineId].adslDemodCapValue=adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.demodCapabilities;
			adslCoreCfgProfile[lineId].adslDemodCapMask=adslCoreCfgProfile[lineId].adslDemodCapValue | pCfg->demodCapMask;
		}
			goto _cmd_reset;

		case LOG_CMD_CFG_PHY2:
		{
			adslPhyCfg	*pCfg = pCmdData;

			AdslDrvPrintf (TEXT("CFG_PHY2 CMD: subChannelInfop5=0x%08X  mask=0x%08X  demodCap=0x%08X\n"),
				(unsigned int)adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5,
				(unsigned int)pCfg->demodCapMask, (unsigned int)pCfg->demodCap);
			
			if(adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5 == pCfg->demodCap) {
				AdslDrvPrintf (TEXT("CFG_PHY2 CMD: No change, do nothing\n"));
				break;
			}

			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5 |= 
				pCfg->demodCapMask & pCfg->demodCap;
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5 &= 
				~(pCfg->demodCapMask & ~pCfg->demodCap);

			adslCoreCfgProfile[lineId].adslDemodCap2Value=adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.subChannelInfop5;
			adslCoreCfgProfile[lineId].adslDemodCap2Mask=adslCoreCfgProfile[lineId].adslDemodCap2Value | pCfg->demodCapMask;
		}
			goto _cmd_reset;
#ifdef G993
		case LOG_CMD_CFG_PHY3:
		{
			adslPhyCfg	*pCfg = pCmdData;
			#if 0
			ulong	vdslParam = 
				(adslCoreConnectionParam.param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA->profileSel & kVdslProfileMask1) |
				((adslCoreConnectionParam.param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA->maskUS0 << kVdslUS0MaskShift) & kVdslUS0Mask);
			#endif
			AdslDrvPrintf (TEXT("CFG_PHY3 CMD: ((maskUS0<<16)|profileSel) - Current=0x%08X  New=0x%08X\n"),
				(uint)adslCoreCfgProfile[lineId].vdslParam, (uint)pCfg->demodCap);
			
			if(adslCoreCfgProfile[lineId].vdslParam == pCfg->demodCap) {
				AdslDrvPrintf (TEXT("CFG_PHY3 CMD: No change, do nothing\n"));
				break;
			}
			
			adslCoreCfgProfile[lineId].vdslParam = pCfg->demodCap;
			
			if (!ADSL_PHY_SUPPORT(kAdslPhyVdsl30a))
				pCfg->demodCap &= ~kVdslProfile30a;
			if (!ADSL_PHY_SUPPORT(kAdslPhyVdslBrcmPriv1))
				pCfg->demodCap &= ~kVdslProfileBrcmPriv1;
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA->profileSel = 
				pCfg->demodCap & kVdslProfileMask2;
			adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.carrierInfoG993p2AnnexA->maskUS0 =
				(pCfg->demodCap & kVdslUS0Mask) >> kVdslUS0MaskShift;
		}
#endif
_cmd_reset:

		case LOG_CMD_RESET:
			BcmAdslCoreConnectionReset(lineId);
			break;

		case LOG_CMD_TEST_DATA:
			adslCoreAlwaysReset = (0 != *(long*) pCmdData) ? AC_TRUE : AC_FALSE;
			break;

		case LOG_CMD_LOG_STOP:
			cmd.command = kDslDiagStopLogCmd;
			BcmCoreCommandHandlerSync(&cmd);
			break;

		case LOG_CMD_CONFIG_A:
#ifndef G992_ANNEXC
			{
			adslCfgProfile *pAdslCfg = (adslCfgProfile *) pCmdData;

			BcmDiagPrintConfigCmd  ("LOG_CMD_CONFIG_A", pCmdData);

			if ( (kAdslCfgModAny == (pAdslCfg->adslAnnexAParam & kAdslCfgModMask)) &&
				 (pAdslCfg->adsl2Param == (kAdsl2CfgReachExOn | kAdsl2CfgAnnexLUpWide | kAdsl2CfgAnnexLUpNarrow)) )
				pAdslCfg->adsl2Param = 0;
			BcmAdslCoreConfigure(lineId, (adslCfgProfile *) pCmdData);
			}
#else
			AdslDrvPrintf (TEXT("DrvDiag: LOG_CMD_CONFIG_A - Can't configure AnnexC modem for AnnexA\n"));
#endif
			break;

		case LOG_CMD_CONFIG_C:
#ifdef G992_ANNEXC
			BcmDiagPrintConfigCmd  ("LOG_CMD_CONFIG_C", pCmdData);
			BcmAdslCoreConfigure(lineId, (adslCfgProfile *) pCmdData);
#else
			AdslDrvPrintf (TEXT("DrvDiag: LOG_CMD_CONFIG_C - Can't configure AnnexA modem for AnnexC\n"));
#endif
			break;

		case LOG_CMD_BERT_EX:
			BcmAdslCoreBertStartEx(lineId, *(long*)pCmdData);
			break;
#ifdef SUPPORT_XDSLDRV_GDB
		case LOG_CMD_GDB:
		{
#ifdef DEBUG_GDB_STUB
			int i;
			printk("BcmAdslCoreDiagCmdCommon::LOG_CMD_GDB '");
			for (i = 0; i < len; i++)
			printk("%c", *((char *)pCmdData + i));
			printk("'\n");
#endif
			BcmCoreDpcSyncEnter();
			BcmAdslCoreGdbCmd(pCmdData, len);
			BcmCoreDpcSyncExit(); 
			break;
		}
#endif
		case (kDslAfelbTestCmd - kFirstDslCommandCode):
		case (kDslTestQuietCmd - kFirstDslCommandCode):
		case (kDslDiagStartBERT - kFirstDslCommandCode):
		case (kDslFilterSNRMarginCmd - kFirstDslCommandCode):
		case (kDslDyingGaspCmd - kFirstDslCommandCode):
		case (kDslStartRetrainCmd - kFirstDslCommandCode):
		case (kDslIdleCmd - kFirstDslCommandCode):
		case (kDslIdleRcvCmd - kFirstDslCommandCode):
		case (kDslIdleXmtCmd - kFirstDslCommandCode):
			cmd.param.value = *(long*)pCmdData;
			/* fall through */
		case (kDslLoopbackCmd - kFirstDslCommandCode):
		case (kDslDiagStopBERT - kFirstDslCommandCode):
		case (kDslDiagStopLogCmd - kFirstDslCommandCode):
		case (kDslPingCmd - kFirstDslCommandCode):
			cmd.command = (diagCmd + kFirstDslCommandCode) | (lineId << DSL_LINE_SHIFT);
			AdslDrvPrintf (TEXT("DrvDiag: cmd=%ld, val= %ld\n"), cmd.command, cmd.param.value);
			BcmCoreCommandHandlerSync(&cmd);

			if ((kDslDyingGaspCmd - kFirstDslCommandCode) == diagCmd) {
				bcmOsSleep (1000/BCMOS_MSEC_PER_TICK);
				cmd.command = kDslIdleCmd | (lineId << DSL_LINE_SHIFT);
				BcmCoreCommandHandlerSync(&cmd);
			}
			break;

#ifdef SUPPORT_SELT
        case (kDslSetRcvGainCmd - kFirstDslCommandCode):
        case (kDslSeltConfiguration - kFirstDslCommandCode):
            {
                /* copy value to seltState */
                adslMibInfo *pMibInfo;
                ulong mibLen = sizeof(adslMibInfo);
                pMibInfo = (void *) AdslCoreGetObjectValue(lineId, NULL, 0, NULL, &mibLen);
                if (diagCmd==(kDslSetRcvGainCmd - kFirstDslCommandCode))
                {
                    pMibInfo->selt.seltAgc = *(int *)pCmdData;
                    BcmAdslCoreDiagWriteStatusString(lineId, "SELT AGC, set value %d in diags state\n",pMibInfo->selt.seltAgc);
                }
                else
                {
                    pMibInfo->selt.seltCfg = *(unsigned long *)pCmdData;
                    BcmAdslCoreDiagWriteStatusString(lineId, "SELT CFG, set value %ld in diags state\n",pMibInfo->selt.seltCfg);
                }
            }
            /* now relay the command*/
            cmd.param.value = *(long*)pCmdData;
			cmd.command = (diagCmd + kFirstDslCommandCode) | (lineId << DSL_LINE_SHIFT);
			BcmCoreCommandHandlerSync(&cmd);
            break;
#endif

		case (kDslTestCmd - kFirstDslCommandCode):
			cmd.command = kDslTestCmd | (lineId << DSL_LINE_SHIFT);
			memcpy((void *)&cmd.param, pCmdData, sizeof(cmd.param.dslTestSpec));
			/* AdslDrvPrintf (TEXT("DrvDslTestCmd: testCmd=%ld\n"), cmd.param.dslTestSpec.type); */
			if (kDslTestToneSelection == cmd.param.dslTestSpec.type) {
				cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap = ((char*) pCmdData) +
					FLD_OFFSET(dslCommandStruct,param.dslTestSpec.param.toneSelectSpec.xmtMap) -
					FLD_OFFSET(dslCommandStruct,param);
				cmd.param.dslTestSpec.param.toneSelectSpec.rcvMap = 
					cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap +
					((cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones + 7) >> 3);
				BcmAdslCoreDiagSelectTones(
					lineId,
					cmd.param.dslTestSpec.param.toneSelectSpec.xmtStartTone,
					cmd.param.dslTestSpec.param.toneSelectSpec.xmtNumOfTones,
					cmd.param.dslTestSpec.param.toneSelectSpec.rcvStartTone,
					cmd.param.dslTestSpec.param.toneSelectSpec.rcvNumOfTones,
					cmd.param.dslTestSpec.param.toneSelectSpec.xmtMap,
					cmd.param.dslTestSpec.param.toneSelectSpec.rcvMap);
			}
			else if (kDslTestExecuteDelay== cmd.param.dslTestSpec.type) {
				BcmAdslCoreSetTestExecutionDelay(lineId, cmd.param.dslTestSpec.type,cmd.param.dslTestSpec.param.value);
			}
			else {
				BcmAdslCoreSetTestMode(lineId, cmd.param.dslTestSpec.type);
#ifdef ADSLDRV_STATUS_PROFILING
				if(101 == cmd.param.dslTestSpec.type)	/* Retrieve Histogram of status buffer and reset */
					BcmXdslCoreDrvProfileInfoClear();
#endif
			}
			break;

		case (kDslStartPhysicalLayerCmd - kFirstDslCommandCode):
			cmd.command = kDslStartPhysicalLayerCmd | (lineId << DSL_LINE_SHIFT);
			memcpy((void *)&cmd.param, pCmdData, sizeof(cmd.param.dslModeSpec));
			BcmCoreCommandHandlerSync(&cmd);
			break;

		case (kDslSetXmtGainCmd - kFirstDslCommandCode):
			cmd.param.value = *(long*)pCmdData;
			BcmAdslCoreSetXmtGain (lineId, cmd.param.value);
			break;

		case (kDslAfeTestCmd - kFirstDslCommandCode):
			BcmAdslCoreAfeTestMsg(pCmdData);
			break;
			
		case (kDslAfeTestCmd1 - kFirstDslCommandCode):
			cmd.command = kDslAfeTestCmd1 | (lineId << DSL_LINE_SHIFT);
			memcpy((void *)&cmd.param, pCmdData, sizeof(cmd.param.dslAfeTestSpec1));
			BcmCoreCommandHandlerSync(&cmd);
			break;
			
		case (kDslPLNControlCmd - kFirstDslCommandCode):
			cmd.command = kDslPLNControlCmd | (lineId << DSL_LINE_SHIFT);
			cmd.param.dslPlnSpec.plnCmd = ((long*)pCmdData)[0];
			if (kDslPLNControlStart == cmd.param.dslPlnSpec.plnCmd) {
				cmd.param.dslPlnSpec.mgnDescreaseLevelPerBin = ((long*)pCmdData)[1];
				cmd.param.dslPlnSpec.mgnDescreaseLevelBand   = ((long*)pCmdData)[2];
			}
			gSharedMemAllocFromUserContext=1;
			if (kDslPLNControlDefineInpBinTable == cmd.param.dslPlnSpec.plnCmd) {
				cmd.param.dslPlnSpec.nInpBin = ((ulong*)pCmdData)[3];
				if(NULL!=(pInpBins=AdslCoreSharedMemAlloc(2*cmd.param.dslPlnSpec.nInpBin))){
#ifdef ADSLDRV_LITTLE_ENDIAN
					BlockShortMoveReverse(cmd.param.dslPlnSpec.nInpBin,
						(short *)(((char *)pCmdData) + FLD_OFFSET(dslCommandStruct, param.dslPlnSpec.itaBinPtr) - 0*FLD_OFFSET(dslCommandStruct, param)),
						(short *)pInpBins);
					
#else
					memcpy(pInpBins,((char *)pCmdData) + FLD_OFFSET(dslCommandStruct, param.dslPlnSpec.itaBinPtr) - 0*FLD_OFFSET(dslCommandStruct, param),2*cmd.param.dslPlnSpec.nInpBin);
#endif
					cmd.param.dslPlnSpec.inpBinPtr = (ushort *)pInpBins;
				}
				AdslDrvPrintf (TEXT("cmd.param.dslPlnSpec.inpBinPtr: 0x%x, data= 0x%X 0x%X 0x%X\n"), 
					(unsigned int)cmd.param.dslPlnSpec.inpBinPtr,
					(unsigned int)cmd.param.dslPlnSpec.inpBinPtr[0],
					(unsigned int)cmd.param.dslPlnSpec.inpBinPtr[1],
					(unsigned int)cmd.param.dslPlnSpec.inpBinPtr[2]);
			}
			if (kDslPLNControlDefineItaBinTable == cmd.param.dslPlnSpec.plnCmd) {
				cmd.param.dslPlnSpec.nItaBin = ((long*)pCmdData)[5];
				if(NULL!=(pItaBins=AdslCoreSharedMemAlloc(2*cmd.param.dslPlnSpec.nItaBin))){
#ifdef ADSLDRV_LITTLE_ENDIAN
					BlockShortMoveReverse(cmd.param.dslPlnSpec.nItaBin,
						(short *)(((char *)pCmdData) + FLD_OFFSET(dslCommandStruct, param.dslPlnSpec.itaBinPtr) - 0*FLD_OFFSET(dslCommandStruct, param)),
						(short *)pItaBins);
#else
					memcpy(pItaBins,((char *)pCmdData) + FLD_OFFSET(dslCommandStruct, param.dslPlnSpec.itaBinPtr) - 0*FLD_OFFSET(dslCommandStruct, param),2*cmd.param.dslPlnSpec.nItaBin);
#endif
					cmd.param.dslPlnSpec.itaBinPtr = (ushort *)pItaBins;
				}
				AdslDrvPrintf (TEXT("DrvDslPLNControlCmd: itaBin=%d\n"), (int)cmd.param.dslPlnSpec.nItaBin);
			}
			BcmCoreCommandHandlerSync(&cmd);
			gSharedMemAllocFromUserContext=0;
			break;
#ifdef PHY_PROFILE_SUPPORT
		case (kDslProfileControlCmd - kFirstDslCommandCode):

			cmd.param.value = *(long*)pCmdData;
			cmd.command = diagCmd + kFirstDslCommandCode;
			
			if( kDslProfileEnable == cmd.param.value )
				BcmAdslCoreProfilingStart();
			else
				BcmAdslCoreProfilingStop();
			
			BcmCoreCommandHandlerSync(&cmd);
			break;
#endif
		case (kDslSendEocCommand - kFirstDslCommandCode):
		{
			void *pEocMsgDataPtr;
			cmd.command = kDslSendEocCommand | (lineId << DSL_LINE_SHIFT);
			memcpy((void *)&cmd.param, pCmdData, sizeof(cmd.param.dslClearEocMsg));
			size = cmd.param.dslClearEocMsg.msgType & 0xFFFF;
			cmd.param.dslClearEocMsg.msgType &= ~kDslClearEocMsgDataVolatileMask;
			gSharedMemAllocFromUserContext=1;
			cmd.param.dslClearEocMsg.dataPtr = AdslCoreSharedMemAlloc(size);
			pEocMsgDataPtr = (char *)pCmdData+FLD_OFFSET(dslCommandStruct, param.dslClearEocMsg.dataPtr) - FLD_OFFSET(dslCommandStruct,param);
#ifdef ADSLDRV_LITTLE_ENDIAN
			if((kDsl993p2PsdDescriptorUs == cmd.param.dslClearEocMsg.msgId) || (kDsl993p2PsdDescriptorDs == cmd.param.dslClearEocMsg.msgId)) {
				Psd993p2 *pPsdDescDst = (Psd993p2 *)cmd.param.dslClearEocMsg.dataPtr;
				Psd993p2 *pPsdDescSrc = (Psd993p2 *)pEocMsgDataPtr;
				pPsdDescDst->maxNbBP = pPsdDescSrc->maxNbBP;
				pPsdDescDst->n = pPsdDescSrc->n;
				size = (size > sizeof(Psd993p2)) ? sizeof(Psd993p2): size;
				size -= 2;
				BlockShortMoveReverse(size>>1, (short *)pPsdDescSrc->bp, (short *)pPsdDescDst->bp);
			}
			else if((kDsl993p2BpDescriptorUs == cmd.param.dslClearEocMsg.msgId) || (kDsl993p2BpDescriptorDs == cmd.param.dslClearEocMsg.msgId)) {
				Bp993p2 *pBp993p2Dst = (Bp993p2 *)cmd.param.dslClearEocMsg.dataPtr;
				Bp993p2 *pBp993p2Src = (Bp993p2 *)pEocMsgDataPtr;
				pBp993p2Dst->noOfToneGroups = pBp993p2Src->noOfToneGroups;
				pBp993p2Dst->reserved = pBp993p2Src->reserved;
				size = (size > sizeof(Bp993p2)) ? sizeof(Bp993p2): size;
				size -= 2;
				BlockShortMoveReverse(size>>1, (short *)pBp993p2Src->toneGroups, (short *)pBp993p2Dst->toneGroups);
			}
			else
				memcpy(cmd.param.dslClearEocMsg.dataPtr, pEocMsgDataPtr, size);
#else
			memcpy(cmd.param.dslClearEocMsg.dataPtr, pEocMsgDataPtr, size);
#endif
			BcmCoreCommandHandlerSync(&cmd);
			gSharedMemAllocFromUserContext=0;
		}
			break;
		default:
			if (diagCmd < 200) {
				cmd.param.value = *(long*)pCmdData;
				cmd.command = (diagCmd + kFirstDslCommandCode) | (lineId << DSL_LINE_SHIFT);
				BcmCoreCommandHandlerSync(&cmd);
			}
			break;
	}	
}

/*
**
**	Support for AFE test
**
*/

#define 		UN_INIT_HDR_MARK	0x80

typedef struct {
	char		*pSdram;
	ulong		size;
	ulong		cnt;
	ulong		frameCnt;
} afeTestData;

afeTestData		afeParam = { NULL, 0, 0, UN_INIT_HDR_MARK };
afeTestData		afeImage = { NULL, 0, 0, UN_INIT_HDR_MARK };

char  *rdFileName = NULL;

void BcmAdslCoreIdle(int size)
{
	BcmAdslCoreDiagWriteStatusString(0, "ReadFile Done");
#ifdef PHY_BLOCK_TEST
	AdslDrvPrintf (TEXT("ReadFile Done: addr=0x%X size=%d\n"), (uint)afeParam.pSdram, (int)afeParam.size);
#endif
}
void (*bcmLoadBufferCompleteFunc)(int size) = BcmAdslCoreIdle;

void BcmAdslCoreSetLoadBufferCompleteFunc(void *funcPtr)
{
	bcmLoadBufferCompleteFunc = (NULL != funcPtr) ? funcPtr : BcmAdslCoreIdle;
}

void *BcmAdslCoreGetLoadBufferCompleteFunc(void)
{
	return (BcmAdslCoreIdle == bcmLoadBufferCompleteFunc) ? bcmLoadBufferCompleteFunc : NULL;
}

void BcmAdslCoreAfeTestInit(afeTestData *pAfe, void *pSdram, ulong size)
{
	pAfe->pSdram = pSdram;
	pAfe->size	= size;
	pAfe->cnt	= 0;
	pAfe->frameCnt = 0;
}

void BcmAdslCoreAfeTestStart(afeTestData *pAfe, afeTestData *pImage)
{
	dslCommandStruct	cmd;

	cmd.command = kDslAfeTestCmd;
	cmd.param.dslAfeTestSpec.type = kDslAfeTestPatternSend;
	cmd.param.dslAfeTestSpec.afeParamPtr = afeParam.pSdram;
	cmd.param.dslAfeTestSpec.afeParamSize = afeParam.size;
	cmd.param.dslAfeTestSpec.imagePtr = afeImage.pSdram;
	cmd.param.dslAfeTestSpec.imageSize = afeImage.size;
	BcmCoreCommandHandlerSync(&cmd);
}

void BcmAdslCoreAfeTestAck (afeTestData	*pAfeData, ulong frNum, ulong frRcv)
{
	dslStatusStruct		status;

	// AdslDrvPrintf (TEXT("BcmAdslCoreAfeTestAck. frNumA = %d frNumR = %d\n"), frNum, frRcv);
	status.code = kDslDataAvailStatus;
#if 1
	status.param.dslDataAvail.dataPtr = (void *) (frNum | (frRcv << 8) | DIAG_ACK_FRAME_RCV_PRESENT);
#else
	status.param.dslDataAvail.dataPtr = (void *) frNum;
#endif
	status.param.dslDataAvail.dataLen = DIAG_ACK_LEN_INDICATION;
	BcmAdslCoreDiagStatusSnooper(&status, (void *)&status, sizeof(status.code) + sizeof(status.param.dslDataAvail));
}

#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
void BcmAdslCoreResetPhy(int copyImage)
{
	void		*pSdramImageAddr;

	if(!copyImage) {
		DiagWriteString(0, DIAG_DSL_CLIENT, "%s: Restting PHY without re-copying image\n", __FUNCTION__);
		BcmAdslCoreStop();
		BcmAdslCoreStart(DIAG_DATA_EYE, AC_FALSE);
	}
	else if(pBkupImageEnable && pBkupImage) {
		BcmAdslCoreStop();
		DiagWriteString(0, DIAG_DSL_CLIENT, "%s: Re-copying backup image before resetting PHY\n", __FUNCTION__);
		memcpy((void*)HOST_LMEM_BASE, pBkupImage, bkupLmemSize);
		pSdramImageAddr = AdslCoreSetSdramImageAddr(ADSL_ENDIAN_CONV_LONG(((ulong*)HOST_LMEM_BASE)[2]), ADSL_ENDIAN_CONV_LONG(((ulong*)HOST_LMEM_BASE)[3]), bkupSdramSize);
		memcpy(pSdramImageAddr, pBkupImage + bkupLmemSize, bkupSdramSize);
		BcmAdslCoreStart(DIAG_DATA_EYE, AC_FALSE);
	}
	else {
		/* NOTE: We cannot re-copy image from the flash as reading from a file can not be done in bottom half */
		DiagWriteString(0, DIAG_DSL_CLIENT, "%s: Backup image is not available, will re-copy PHY image from the flash\n", __FUNCTION__);
		adslCoreResetPending = AC_TRUE;
	}
}
#endif

#ifdef LMEM_ACCESS_WORKAROUND
extern void AdslCoreRunPhyMipsInSDRAM(void);
extern AC_BOOL AdslCoreIsPhyMipsRunning(void);
#endif

void BcmAdslCoreAfeTestMsg(void *pMsg)
{
	dslCommandStruct	*pCmd = (void *) (((char *) pMsg) - sizeof(pCmd->command));
	char				*pSdram;
	afeTestData		*pAfeData;
	ulong			frNum = 0, dataLen = 0, n;
	int				i;
#ifdef LMEM_ACCESS_WORKAROUND
	int chipRevId = (PERF->RevID & REV_ID_MASK);
#endif
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
	char				*pBkupAddr=NULL;
#endif

	if (kDslAfeTestPhyRun == pCmd->param.dslAfeTestSpec.type) {
#ifdef PHY_BLOCK_TEST
		AdslDrvPrintf (TEXT("PHY RUN START\n"));
#endif
		BcmAdslCoreStart(DIAG_DATA_EYE, AC_FALSE);
#ifdef PHY_BLOCK_TEST
		if(BcmAdslCoreIsTestCommand() && (NULL != irqDpcId))
			bcmOsDpcEnqueue(irqDpcId);
#endif
		return;
	}
	else if (kDslAfeTestRdEndOfFile == pCmd->param.dslAfeTestSpec.type) {
#ifdef PHY_BLOCK_TEST
		BcmAdslCoreDebugReadEndOfFile();
#else
		if (ADSL_PHY_SUPPORT(kAdslPhyPlayback))
			BcmAdslCoreDebugReadEndOfFile();
#endif
		afeParam.cnt = 0;
		afeImage.cnt = 0;
		return;
	}

	if ( ((void *) -1 == pCmd->param.dslAfeTestSpec.afeParamPtr) &&
		 ((void *) -1 == pCmd->param.dslAfeTestSpec.imagePtr) ) {

		if (-1 == (pCmd->param.dslAfeTestSpec.afeParamSize & pCmd->param.dslAfeTestSpec.imageSize)) {
			BcmAdslCoreAfeTestStart(&afeParam, &afeImage);
			return;
		}
		
		switch (pCmd->param.dslAfeTestSpec.type) {
			case kDslAfeTestLoadImage:
			case kDslAfeTestLoadImageOnly:
				
				BcmAdslCoreAfeTestInit(&afeParam, (void *)HOST_LMEM_BASE, pCmd->param.dslAfeTestSpec.afeParamSize);
				BcmAdslCoreAfeTestInit(&afeImage, NULL, pCmd->param.dslAfeTestSpec.imageSize);
				BcmAdslCoreStop();
				if ((kDslAfeTestLoadImage == pCmd->param.dslAfeTestSpec.type) &&
					(0 == afeParam.size) && (0 == afeImage.size)) {
					BcmAdslCoreStart(DIAG_DATA_EYE, AC_TRUE);
#if 0
					if (DIAG_DEBUG_CMD_LOG_DATA == diagDebugCmd.cmd) {
						BcmAdslCoreDiagStartLog(diagDebugCmd.param1, diagDebugCmd.param2);
						diagDebugCmd.cmd = 0;
					}
#endif
				}
#if 1 && defined(__KERNEL__) &&  \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
				if((TRUE == pBkupImageEnable) && (0 != afeParam.size) && (0 != afeImage.size)) {
					ulong combineImageLen = afeParam.size+afeImage.size;
					if((NULL != pBkupImage) && (combineImageLen > (bkupLmemSize+bkupSdramSize))) {
						vfree(pBkupImage);
						pBkupImage = (char *)vmalloc(combineImageLen);
						if(NULL == pBkupImage) {
							bkupLmemSize = 0;
							bkupSdramSize = 0;
							BcmAdslCoreDiagWriteStatusString(0, "*** vmalloc(%d) for backing up downloading image failed\n", (int)combineImageLen);
						}
						else {
							bkupLmemSize = afeParam.size;
							bkupSdramSize = afeImage.size;
							BcmAdslCoreDiagWriteStatusString(0, "*** vmalloc(%d) for backing up downloading image sucessful\n", (int)combineImageLen);
						}
					}
					else {
						bkupLmemSize = afeParam.size;
						bkupSdramSize = afeImage.size;
						if(NULL == pBkupImage) {
							pBkupImage = (char *)vmalloc(combineImageLen);
							if(NULL == pBkupImage) {
								bkupLmemSize = 0;
								bkupSdramSize = 0;
								BcmAdslCoreDiagWriteStatusString(0, "*** vmalloc(%d) for backing up downloading image failed\n", (int)combineImageLen);
							}
							else {
								BcmAdslCoreDiagWriteStatusString(0, "*** vmalloc(%d) for backing up downloading image sucessful\n", (int)combineImageLen);
							}
						}
					}
				}
#endif
				break;
			case kDslAfeTestPatternSend:
				pSdram = (char*) AdslCoreGetSdramImageStart() + AdslCoreGetSdramImageSize();
				BcmAdslCoreAfeTestInit(&afeParam, pSdram, pCmd->param.dslAfeTestSpec.afeParamSize);
				pSdram += (pCmd->param.dslAfeTestSpec.afeParamSize + 0xF) & ~0xF;
				BcmAdslCoreAfeTestInit(&afeImage, pSdram, pCmd->param.dslAfeTestSpec.imageSize);
				break;

			case kDslAfeTestLoadBuffer:
				n = pCmd->param.dslAfeTestSpec.imageSize;	/* buffer address is here */
				pSdram = (((int) n & 0xF0000000) == 0x10000000) ? ADSL_ADDR_TO_HOST(n) : (void *) n;
				BcmAdslCoreAfeTestInit(&afeParam, pSdram, pCmd->param.dslAfeTestSpec.afeParamSize);
				BcmAdslCoreAfeTestInit(&afeImage, 0, 0);
				if (0 == afeParam.size)
					(*bcmLoadBufferCompleteFunc)(0);
				break;
		}
		return;
	}

	if ((0 == afeParam.size) && (0 == afeImage.size))
		return;

	if (pCmd->param.dslAfeTestSpec.afeParamSize != 0) {
#ifdef LMEM_ACCESS_WORKAROUND
		if( ((0xA0 == chipRevId) || (0xB0 == chipRevId)) &&
			(afeParam.cnt == 0) && (AC_FALSE == AdslCoreIsPhyMipsRunning()) &&
			(((uint)afeParam.pSdram & HOST_LMEM_BASE) == HOST_LMEM_BASE) )
			AdslCoreRunPhyMipsInSDRAM();
#endif
		pAfeData = &afeParam;
		frNum = (ulong) (pCmd->param.dslAfeTestSpec.afeParamPtr) & 0xFF;
		dataLen = pCmd->param.dslAfeTestSpec.afeParamSize;
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
		pBkupAddr = pBkupImage;
#endif
	}
	else if (pCmd->param.dslAfeTestSpec.imageSize != 0) {
		pAfeData = &afeImage;
		if (NULL == pAfeData->pSdram) {
			pAfeData->pSdram = AdslCoreSetSdramImageAddr(ADSL_ENDIAN_CONV_LONG(((ulong*)HOST_LMEM_BASE)[2]), ADSL_ENDIAN_CONV_LONG(((ulong*)HOST_LMEM_BASE)[3]), pAfeData->size);
			if(NULL == pAfeData->pSdram) {
				if(0 == pAfeData->frameCnt) {
					BcmAdslCoreDiagWriteStatusString(0, "Can not load image! SDRAM image offset(0x%x) is below the Host reserved memory offset(0x%x)",
						(((uint*)HOST_LMEM_BASE)[2] & (ADSL_PHY_SDRAM_PAGE_SIZE-1)), ADSL_PHY_SDRAM_BIAS);
					AdslDrvPrintf (TEXT("Can not load image! SDRAM image offset(0x%x) is below the Host reserved memory offset(0x%x)\n"),
						(((uint*)HOST_LMEM_BASE)[2] & (ADSL_PHY_SDRAM_PAGE_SIZE-1)), ADSL_PHY_SDRAM_BIAS);
				}
				BcmAdslCoreAfeTestAck (pAfeData, pAfeData->frameCnt, pAfeData->frameCnt);
				pAfeData->frameCnt = (pAfeData->frameCnt + 1) & 0xFF;
				return;
			}
		}
		frNum = (ulong) (pCmd->param.dslAfeTestSpec.imagePtr) & 0xFF;
		dataLen = pCmd->param.dslAfeTestSpec.imageSize;
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
		if(pBkupImage)
			pBkupAddr = pBkupImage+bkupLmemSize;
#endif
	}
	else
		pAfeData = NULL;

	if (NULL != pAfeData) {
		
		if (frNum != pAfeData->frameCnt) {
			BcmAdslCoreAfeTestAck (pAfeData, (pAfeData->frameCnt - 1) & 0xFF, frNum);
			return;
		}
		n = pAfeData->size - pAfeData->cnt;
		if (dataLen > n)
			dataLen = n;
		pSdram = pAfeData->pSdram + pAfeData->cnt;
		memcpy (
			pSdram, 
			((char *) pCmd) + sizeof(pCmd->command) + sizeof(pCmd->param.dslAfeTestSpec),
			dataLen);
#if defined(__KERNEL__) && \
	(!defined(CONFIG_BCM96348) &&  !defined(CONFIG_BCM96338))
		if(pBkupAddr) {
			pBkupAddr += pAfeData->cnt;
			memcpy (
				pBkupAddr, 
				((char *) pCmd) + sizeof(pCmd->command) + sizeof(pCmd->param.dslAfeTestSpec),
				dataLen);
		}
#endif
		pAfeData->cnt += dataLen;
		pAfeData->frameCnt = (pAfeData->frameCnt + 1) & 0xFF;
		BcmAdslCoreAfeTestAck (pAfeData, frNum, frNum);
	}

	if ((afeParam.cnt == afeParam.size) && (afeImage.cnt == afeImage.size)) {
		switch (pCmd->param.dslAfeTestSpec.type) {
			case kDslAfeTestLoadImageOnly:
				BcmAdslCoreDiagWriteStatusString(0, "LoadImageOnly Done");
				AdslCoreSetXfaceOffset(HOST_LMEM_BASE, afeParam.size);
#ifdef PHY_BLOCK_TEST
				AdslDrvPrintf (TEXT("\nLMEM & SDRAM images load done\nLMEM addr=0x%X size=%d  SDRAM addr=0x%X size=%d\n"),
					(uint)afeParam.pSdram, (int)afeParam.size, (uint)afeImage.pSdram, (int)afeImage.size);
#endif
				adslCoreAlwaysReset = AC_FALSE;
				for(i = 0; i < MAX_DSL_LINE; i++)
					adslCoreConnectionMode[i] = AC_TRUE;
				break;
			case kDslAfeTestLoadImage:
				BcmAdslCoreDiagWriteStatusString(0, "LoadImage Done");
				AdslCoreSetXfaceOffset(HOST_LMEM_BASE, afeParam.size);
				adslCoreAlwaysReset = AC_FALSE;
				
				for(i = 0; i < MAX_DSL_LINE; i++)
					adslCoreConnectionMode[i] = AC_TRUE;
				
#if defined(PHY_LOOPBACK)
				adslNewImg = 1;
#endif
#ifdef ADSLDRV_STATUS_PROFILING
				BcmXdslCoreDrvProfileInfoClear();
#endif
#if defined(CONFIG_BCM_DSL_GFAST) && !defined(CONFIG_BCM_DSL_GFASTCOMBO)
				xdslCorePhyImageJustSwitch = AC_TRUE;
#endif
				BcmAdslCoreStart(DIAG_DATA_EYE, AC_FALSE);
				break;
			case kDslAfeTestPatternSend:
				BcmAdslCoreAfeTestStart(&afeParam, &afeImage);
				break;
			case kDslAfeTestLoadBuffer:
				(*bcmLoadBufferCompleteFunc)(afeParam.size);
				break;
		}
		afeParam.cnt = 0;
		afeParam.frameCnt = UN_INIT_HDR_MARK;
		afeImage.cnt = 0;
		afeImage.frameCnt = UN_INIT_HDR_MARK;
	}
}

void BcmAdslCoreSendBuffer(ulong statusCode, uchar *bufPtr, ulong bufLen)
{
	ULONG				n, frCnt, dataCnt, dataSize;
	dslStatusStruct		*pStatus;
	char			testFrame[1000], *pData;

	pStatus = (void *) testFrame;
	dataSize = sizeof(testFrame) - sizeof(pStatus->code) - sizeof(pStatus->param.dslDataAvail) - 16;
	pData = testFrame + sizeof(pStatus->code) + sizeof(pStatus->param.dslDataAvail);

	dataCnt  = 0;
	frCnt	 = 0;
	while (dataCnt < bufLen) {
		n = bufLen - dataCnt;
		if (n > dataSize)
			n = dataSize;

		pStatus->code = statusCode;
		pStatus->param.dslDataAvail.dataPtr = (void *) frCnt;
		pStatus->param.dslDataAvail.dataLen	= n;
		memcpy (pData, bufPtr + dataCnt, n);
		DiagWriteStatus(pStatus, testFrame, (pData - testFrame) + n);

		dataCnt += n;
		frCnt = (frCnt + 1) & 0xFF;

		if (0 == (frCnt & 7))
			BcmAdslCoreDelay(40);
	}

	pStatus->code = statusCode;
	pStatus->param.dslDataAvail.dataPtr = (void *) frCnt;
	pStatus->param.dslDataAvail.dataLen	= 0;
	DiagWriteStatus(pStatus, testFrame, pData - testFrame);
}

#if defined(CONFIG_BCM96338) || defined(CONFIG_BCM96348)
void BcmAdslCoreSendDmaBuffers(ulong statusCode, int bufNum)
{
	int					len, i;
	dslStatusStruct		*pStatus;
	static char			testFrame[LOG_MAX_DATA_SIZE], *pFrameData;
	LogProtoHeader		*pLogHdr;

	/* 
	**	Send the first 2 DMA buffers as raw/DMA data 
	**  If DMA'd block aren't sent in interrupt handler DslDiags needs to 
	**	see this to know the sampling rate 512 vs. 1024
	*/

	pLogHdr = ((LogProtoHeader *) BcmAdslCoreDiagGetDmaDataAddr(0)) - 1;
	BcmAdslCoreDiagWriteStatusData(
		pLogHdr->logCommmand, 
		(void *) (pLogHdr + 1),
		BcmAdslCoreDiagGetDmaDataSize(0), 
		NULL, 0);

	pLogHdr = ((LogProtoHeader *) BcmAdslCoreDiagGetDmaDataAddr(1)) - 1;
	BcmAdslCoreDiagWriteStatusData(
		pLogHdr->logCommmand, 
		(void *) (pLogHdr + 1),
		BcmAdslCoreDiagGetDmaDataSize(1), NULL, 0);

	/*	Send captured samples as statuses for loopback test */

	pStatus = (void *) testFrame;
	pFrameData = testFrame + sizeof(pStatus->code) + sizeof(pStatus->param.dslDataAvail);
	len	  = BcmAdslCoreDiagGetDmaDataSize(0);

	pStatus->code = kDslDataAvailStatus;
	pStatus->param.dslDataAvail.dataPtr = (void *) 0xFFF00000;
	pStatus->param.dslDataAvail.dataLen = len * bufNum;
	BcmAdslCoreDiagStatusSnooper(pStatus, testFrame, pFrameData - testFrame);

	for (i = 0; i < bufNum; i++) {
		pStatus->code = statusCode;
		pStatus->param.dslDataAvail.dataPtr = (void *) (i & 0xFF);
		pStatus->param.dslDataAvail.dataLen = len;
		len	  = BcmAdslCoreDiagGetDmaDataSize(i);
		memcpy (pFrameData, BcmAdslCoreDiagGetDmaDataAddr(i), len);
		BcmAdslCoreDiagStatusSnooper(pStatus, testFrame, (pFrameData - testFrame) + len);

		if (7 == (i & 7))
			BcmAdslCoreDelay(40);
	}

	pStatus->code = statusCode;
	pStatus->param.dslDataAvail.dataPtr = (void *) bufNum;
	pStatus->param.dslDataAvail.dataLen	= 0;
	BcmAdslCoreDiagStatusSnooper(pStatus, testFrame, pFrameData - testFrame);
}
#endif

void BcmAdslCoreAfeTestStatus(dslStatusStruct *status)
{
	/* Original kDslDataAvailStatus message has already gone to BcmAdslCoreDiagStatusSnooper */

	BcmAdslCoreSendBuffer(status->code, status->param.dslDataAvail.dataPtr, status->param.dslDataAvail.dataLen);
}

ulong BcmAdslCoreGetConfiguredMod(uchar lineId)
{
	return adslCoreConnectionParam[lineId].param.dslModeSpec.capabilities.modulations;
}

/*
**
**	Support for chip test (not standard PHY DSL interface
**
*/

#if 1 || defined(PHY_BLOCK_TEST)

/* Memory map for Monitor (System Calls) */

#define MEM_MONITOR_ID              0
#define MEM_MONITOR_PARAM_1         1
#define MEM_MONITOR_PARAM_2         2
#define MEM_MONITOR_PARAM_3         3
#define MEM_MONITOR_PARAM_4         4

#define GLOBAL_EVENT_ZERO_BSS       0  /* bssStart, bssEnd */
#define GLOBAL_EVENT_LOAD_IMAGE     2   /* imageId, imageStart */
#define GLOBAL_EVENT_DUMP_IMAGE     3   /* imageId, imageStart, imageEnd */
#define GLOBAL_EVENT_PR_MSG         4   /* num, msgFormat, msgData */
#define GLOBAL_EVENT_PROBE_IMAGE    5   /* imageId, imageStart, imageEnd */
#define GLOBAL_EVENT_READ_FILE      20  /* filename, bufferStart, size */
#define GLOBAL_EVENT_SIM_STOP       8   /* N/A */
#define GLOBAL_EVENT_RESET_6306     60

Bool		testCmdInProgress = AC_FALSE;
Bool		testFileReadCmdInProgress = AC_FALSE;
Bool		testPlaybackStopRq = AC_FALSE;
OS_TICKS	tFileReadCmd, tTestCmd;

#if defined(PHY_BLOCK_TEST) && defined(USE_6306_CHIP)
extern void XdslCoreHwReset6306(void);
#endif
/*
**	File cache control data
*/

// #define	READ_FILE_CACHE_SIZE		0x4000

void  BcmAdslCoreDebugReadFile(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen);
void  __BcmAdslCoreDebugReadFileCached(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen);
void  BcmAdslCoreDebugReadFileCached(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen);
#ifdef READ_FILE_CACHE_SIZE
void  (*bcmDebugReadFileFunc)(char *fName, ulong rcmd, uchar *pBuf, ulong bLen) = BcmAdslCoreDebugReadFileCached;
#else
void  (*bcmDebugReadFileFunc)(char *fName, ulong rcmd, uchar *pBuf, ulong bLen) = BcmAdslCoreDebugReadFile;
#endif

char	*dirFileName = NULL;
ulong	dirFileOffset = 0;
char	*cacheFileName = NULL;
uchar	*cacheBufPtr = NULL;
ulong	cacheSize = 0;
circBufferStruct	cacheCircBuf;

uchar	*rdBufPtr = NULL;
ulong	rdBufLen = 0;

void BcmAdslCoreDebugSendCommand(char *fileName, ushort cmd, ulong offset, ulong len, ulong bufAddr)
{
	DiagProtoFrame		*pDiagFrame;
	DiagTestData		*pTestCmd;
	char				testFrame[sizeof(LogProtoHeader)+sizeof(DiagTestData)];

	pDiagFrame	= (void *) testFrame;
	pTestCmd	= (void *) pDiagFrame->diagData;
	*(short *)pDiagFrame->diagHdr.logProtoId = *(short *) LOG_PROTO_ID;
	pDiagFrame->diagHdr.logPartyId	= LOG_PARTY_CLIENT;
	pDiagFrame->diagHdr.logCommmand = LOG_CMD_DEBUG;

	pTestCmd->cmd		= ADSL_ENDIAN_CONV_SHORT(cmd);
	pTestCmd->cmdId		= 0;
	pTestCmd->offset	= ADSL_ENDIAN_CONV_LONG(offset);
	pTestCmd->len		= ADSL_ENDIAN_CONV_LONG(len);
	pTestCmd->bufPtr	= ADSL_ENDIAN_CONV_LONG(bufAddr);
	memcpy (pTestCmd->fileName, fileName, DIAG_TEST_FILENAME_LEN);
	BcmAdslCoreDiagWrite(testFrame, sizeof(LogProtoHeader)+sizeof(DiagTestData));
}

void BcmAdslCoreDebugWriteFile(char *fileName, char cmd, uchar *bufPtr, ulong bufLen)
{
	BcmAdslCoreDebugSendCommand(fileName, cmd, 0, bufLen, (ulong) bufPtr);
	BcmAdslCoreSendBuffer(kDslDataAvailStatus, bufPtr, bufLen);
}

void BcmAdslCoreDebugCompleteCommand(ulong rdSize)
{
	char	strBuf[16];
	ulong	monitorId, reqAddr, reqSize;
	volatile ulong	*pAdslLmem = (void *) HOST_LMEM_BASE;
	
	monitorId = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_ID]);
	reqAddr = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_2]);
	reqSize = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_3]);
	
	if ( (GLOBAL_EVENT_READ_FILE == monitorId) || (GLOBAL_EVENT_LOAD_IMAGE == monitorId) ) {
		pAdslLmem[MEM_MONITOR_PARAM_2] = ADSL_ENDIAN_CONV_LONG(rdSize);
		if(GLOBAL_EVENT_READ_FILE == monitorId ) {
			if( rdSize < reqSize ) {
				DiagWriteString(0, DIAG_DSL_CLIENT, "READ_EOF: fn=%s addr=0x%X size=%d reqSize=%d dstAddr=0x%X\n",
					(NULL != rdFileName) ? rdFileName: "Unknown",
					(uint)afeParam.pSdram, (int)rdSize, (int)reqSize, (uint)reqAddr);
				dirFileName = NULL;
				dirFileOffset = 0;
				rdFileName = NULL;
				BcmAdslDiagStatSaveStop();
			}
		}
		else {
			BcmAdslCoreDiagSaveStatusString("LOAD_DONE: fn=%s addr=0x%X size=%d\n",
				(NULL != rdFileName) ? rdFileName: "Unknown",
				(uint)afeParam.pSdram, (int)afeParam.size);
			dirFileName = NULL;
			dirFileOffset = 0;
			rdFileName = NULL;
		}
	}
	
	testCmdInProgress = AC_FALSE;
	adslCoreStarted = AC_TRUE;
	
	sprintf(strBuf, "0x%X", (uint)monitorId);
	BcmAdslCoreDiagSaveStatusString("%s: %s fn=%s rdSize=%lu reqSize/param3=%lu reqAddr/param2=0x%X\n",
		__FUNCTION__,
		(GLOBAL_EVENT_READ_FILE==monitorId) ? "READ": (GLOBAL_EVENT_LOAD_IMAGE==monitorId) ? "LOAD": strBuf,
		(NULL != rdFileName) ? rdFileName: "Unknown",
		rdSize, reqSize, reqAddr);
	
	pAdslLmem[MEM_MONITOR_ID] ^= (ulong) -1;
}

void BcmAdslCoreDebugReadEndOfFile(void)
{
	if (testPlaybackStopRq)
		testPlaybackStopRq = AC_FALSE;
	BcmAdslCoreSetLoadBufferCompleteFunc(NULL);
	BcmCoreDpcSyncEnter();
	BcmAdslCoreDebugCompleteCommand(0);
	BcmCoreDpcSyncExit();
}

int __BcmAdslCoreDebugReadFileComplete(int rdSize, Bool bCompleteCmd)
{
	testFileReadCmdInProgress = AC_FALSE;
	if (testPlaybackStopRq) {
		testPlaybackStopRq = AC_FALSE;
		rdSize = 0;
	}

	if (rdFileName == dirFileName)
		dirFileOffset += rdSize;

	/* 
	** if read problem (rdSize == 0) leave command pending and be ready for
	** DslDiags retransmissions (i.e. leave complete function pointer
	*/

	if (rdSize != 0) {
		BcmAdslCoreSetLoadBufferCompleteFunc(NULL);
		if (bCompleteCmd) {
			BcmCoreDpcSyncEnter();
			BcmAdslCoreDebugCompleteCommand(rdSize);
			BcmCoreDpcSyncExit();
		}
	}
	return rdSize;
}

void BcmAdslCoreDebugReadFileComplete(int rdSize)
{
	__BcmAdslCoreDebugReadFileComplete(rdSize, true);
}

#ifdef READ_FILE_CACHE_SIZE

void BcmAdslCoreDebugReadFileCacheComplete(int rdSize)
{
	CircBufferWriteUpdateContig(&cacheCircBuf, rdSize);
	rdSize = __BcmAdslCoreDebugReadFileComplete(rdSize, false);

	if ((rdBufLen != 0) && (rdSize != 0))
		__BcmAdslCoreDebugReadFileCached(cacheFileName, DIAG_TEST_CMD_READ, rdBufPtr, rdBufLen);
}

#endif

void __BcmAdslCoreDebugReadFile(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen, void *fnComplPtr)
{
	ulong	offset = 0;

	if (NULL == dirFileName) {
		dirFileName = fileName;
		dirFileOffset = 0;
	}
	
	if (fileName == dirFileName)
		offset = dirFileOffset;
	rdFileName = fileName;
	
	BcmAdslCoreDiagSaveStatusString("CMD_REQ: %s fn=%s offset=%d len=%d addr=0x%X\n",
		(readCmd == DIAG_TEST_CMD_READ) ? "READ": "LOAD",
		fileName, (int)offset , (int)bufLen, (uint)bufPtr);
	
	bcmOsGetTime(&tFileReadCmd);
	testFileReadCmdInProgress = AC_TRUE;
	BcmAdslCoreSetLoadBufferCompleteFunc(fnComplPtr);
	BcmAdslCoreAfeTestInit(&afeParam, bufPtr, bufLen);
	BcmAdslCoreAfeTestInit(&afeImage, 0, 0);
	BcmAdslCoreDebugSendCommand(fileName, readCmd, offset, bufLen, (ulong) bufPtr);
}

void BcmAdslCoreDebugReadFile(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen)
{
	__BcmAdslCoreDebugReadFile(fileName, readCmd, bufPtr, bufLen, BcmAdslCoreDebugReadFileComplete);
}

/*
**
**	File cache control data
**
*/

#ifdef READ_FILE_CACHE_SIZE

void *BcmAdslCoreDebugCacheAlloc(ulong *pBufSize)
{
	void	*pMem;
	ulong	bufSize = *pBufSize;

	do { 
#if defined(VXWORKS)
	    pMem = (void *) malloc(bufSize);
#elif defined(__KERNEL__)
		pMem = (void *) kmalloc(bufSize, GFP_KERNEL);
#endif
	} while ((NULL == pMem) && ((bufSize >> 1) > 0x2000));
	*pBufSize = bufSize;
	return pMem;
}

ulong BcmAdslCoreDebugReadCacheContig(uchar *bufPtr, ulong bufLen)
{
	ulong		n;

	if (0 == bufLen)
		return 0;
	if (0 == (n = CircBufferGetReadContig(&cacheCircBuf))) 
		return 0;

	if (n > bufLen)
		n = bufLen;

	memcpy (ADSL_ADDR_TO_HOST(bufPtr), CircBufferGetReadPtr(&cacheCircBuf), n);
	CircBufferReadUpdateContig(&cacheCircBuf, n);
	return n;
}

void __BcmAdslCoreDebugReadFileCached(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen)
{
	ulong		n;

	do {
	  n = BcmAdslCoreDebugReadCacheContig(bufPtr, bufLen);
	  bufPtr += n;
	  bufLen -= n;
	  if (0 == bufLen)
		  break;

	  n = BcmAdslCoreDebugReadCacheContig(bufPtr, bufLen);
	  bufPtr += n;
	  bufLen -= n;
	} while (0);

	rdBufPtr = bufPtr;
	rdBufLen = bufLen;

	if (!testFileReadCmdInProgress && ((n = CircBufferGetWriteContig(&cacheCircBuf)) != 0))
		__BcmAdslCoreDebugReadFile(fileName, readCmd, CircBufferGetWritePtr(&cacheCircBuf), n, BcmAdslCoreDebugReadFileCacheComplete);
	if (0 == bufLen)
		BcmAdslCoreDebugCompleteCommand(0);
}

void BcmAdslCoreDebugReadFileCached(char *fileName, ulong readCmd, uchar *bufPtr, ulong bufLen)
{
	if (NULL == cacheFileName) {
		cacheSize = READ_FILE_CACHE_SIZE;
		cacheBufPtr = BcmAdslCoreDebugCacheAlloc(&cacheSize);
		BcmAdslCoreDiagWriteStatusString(0, "AllocateCache: ptr=0x%X, size=%d", (ulong) cacheBufPtr, cacheSize);
		if (NULL == cacheBufPtr) {
			bcmDebugReadFileFunc = BcmAdslCoreDebugReadFile;
			return BcmAdslCoreDebugReadFile(fileName, readCmd, bufPtr, bufLen);
		}
		cacheFileName = fileName;
		CircBufferInit(&cacheCircBuf, cacheBufPtr, cacheSize);
	}
	else if (cacheFileName != fileName)
		return BcmAdslCoreDebugReadFile(fileName, readCmd, bufPtr, bufLen);

	__BcmAdslCoreDebugReadFileCached(fileName, readCmd, bufPtr, bufLen);
}
#endif

void BcmAdslCoreDebugTestComplete(void)
{
	BcmAdslCoreDebugSendCommand("", DIAG_TEST_CMD_TEST_COMPLETE, 0, 0, 0);
}

void BcmAdslCoreDebugPlaybackStop(void)
{
	testPlaybackStopRq = AC_TRUE;
}

void BcmAdslCoreDebugPlaybackResume(void)
{
	if (testCmdInProgress) {
		BcmAdslCoreSetLoadBufferCompleteFunc(NULL);
#ifdef READ_FILE_CACHE_SIZE
		__BcmAdslCoreDebugReadFileCached(cacheFileName, DIAG_TEST_CMD_READ, rdBufPtr, rdBufLen);
#else
		BcmAdslCoreDebugCompleteCommand(0);
#endif
	}
}

void BcmAdslCoreDebugReset(void)
{
	if (NULL != cacheBufPtr) {
#if defined(VXWORKS)
		free (cacheBufPtr);
#elif defined(__KERNEL__)
		kfree(cacheBufPtr);
#endif
		cacheBufPtr = NULL;
	}
	cacheFileName = NULL;
#ifdef READ_FILE_CACHE_SIZE
	bcmDebugReadFileFunc = BcmAdslCoreDebugReadFileCached;
#else
	bcmDebugReadFileFunc = BcmAdslCoreDebugReadFile;
#endif
	testCmdInProgress = AC_FALSE;
	dirFileName = NULL;
	dirFileOffset = 0;
	rdFileName = NULL;
	BcmAdslCoreDebugSendCommand("", DIAG_TEST_CMD_TEST_COMPLETE, 0, 0, 0);
}

void BcmAdslCoreDebugTimer(void)
{
	volatile ulong	*pAdslLmem = (void *) HOST_LMEM_BASE;
	char		*pName;
	OS_TICKS	tMs;

	if (testCmdInProgress && testFileReadCmdInProgress) {
		bcmOsGetTime(&tMs);
		tMs = (tMs - tFileReadCmd) * BCMOS_MSEC_PER_TICK;
		if (tMs > 5000) {
			BcmAdslCoreDiagWriteStatusString(0, "BcmAdslCoreDebugTimer: FileReadCmd timeout time=%d", tMs);
			pName = ADSL_ADDR_TO_HOST(ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_1]));
			(*bcmDebugReadFileFunc)(
				pName,
				DIAG_TEST_CMD_READ,
				(uchar *) ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_2]),
				ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_3]));
		}
	}
	else if (!testCmdInProgress && BcmAdslCoreIsTestCommand()) {
		bcmOsGetTime(&tMs);
		tMs = (tMs - tTestCmd) * BCMOS_MSEC_PER_TICK;
		if (tMs > 100) {
			BcmAdslCoreDiagWriteStatusString(0, "BcmAdslCoreDebugTimer: TestCmd=%d time=%d", ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_ID]), tMs);
#if defined(VXWORKS) || defined(TARG_OS_RTEMS) || defined(__ECOS)
			if (irqSemId != 0 )
				bcmOsSemGive (irqSemId);
#else
			if (irqDpcId != NULL)
				bcmOsDpcEnqueue(irqDpcId);
#endif
		}
	}

}

Bool BcmAdslCoreIsTestCommand(void)
{
	volatile ulong	*pAdslLmem = (void *) HOST_LMEM_BASE;
	
	if (testCmdInProgress)
		return AC_FALSE;
	
	return ((ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_ID]) & 0xFFFFFF00) == 0);
}

void BcmAdslCoreProcessTestCommand(void)
{
	volatile ulong	*pAdslLmem = (void *) HOST_LMEM_BASE;
	char	*pName;
	ulong	cmdId, param1, param2, param3, param4;
#ifdef PHY_BLOCK_TEST
	uchar	cmd;
#endif
	if (!BcmAdslCoreIsTestCommand())
		return;
	bcmOsGetTime(&tTestCmd);
	
	adslCoreStarted = AC_FALSE;
	
	cmdId = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_ID]);
	param1 = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_1]);
	param2 = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_2]);
	param3 = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_3]);
	param4 = ADSL_ENDIAN_CONV_LONG(pAdslLmem[MEM_MONITOR_PARAM_4]);

	BcmAdslCoreDiagSaveStatusString("%s: cmdId=0x%X\n\t\tparam1=0x%X param2=0x%X(0x%X) param3=0x%X param4=0x%X\n",
		__FUNCTION__, (uint)cmdId, (uint)param1, (uint)param2, (uint)(ADSL_ADDR_TO_HOST(param2)), (uint)param3, (uint)param4);
	
	switch (cmdId) {
		case GLOBAL_EVENT_LOAD_IMAGE:
			pName = ADSL_ADDR_TO_HOST(param1);
			testCmdInProgress = AC_TRUE;
			BcmAdslCoreDebugReadFile(
				pName,
				DIAG_TEST_CMD_LOAD,
				(uchar *) ADSL_ADDR_TO_HOST(param2),
				param3);
			testCmdInProgress = AC_TRUE;
			break;
		case GLOBAL_EVENT_READ_FILE:
			pName = ADSL_ADDR_TO_HOST(param1);
			testCmdInProgress = AC_TRUE;
			(*bcmDebugReadFileFunc)(
				pName,
				DIAG_TEST_CMD_READ,
				(uchar *) ADSL_ADDR_TO_HOST(param2),
				param3);
			testCmdInProgress = AC_TRUE;
			break;
#ifdef PHY_BLOCK_TEST
		case GLOBAL_EVENT_DUMP_IMAGE:
			cmd = DIAG_TEST_CMD_WRITE;
			goto _write_file;
		case GLOBAL_EVENT_PROBE_IMAGE:
			cmd = DIAG_TEST_CMD_APPEND;
_write_file:
			pName = ADSL_ADDR_TO_HOST(param1);
			
			BcmAdslCoreDebugWriteFile(
				pName,
				cmd,
				ADSL_ADDR_TO_HOST(param2),
				param3 - param2 + 1);
			// BcmAdslCoreDelay(40);
			BcmAdslCoreDebugCompleteCommand(0);
			break;

		case GLOBAL_EVENT_PR_MSG:
#ifdef PHY_LOOPBACK
			if (1 == adslNewImg) {
				AdslDrvXtmLinkDown(0);
#ifdef SUPPORT_DSL_BONDING
				AdslDrvXtmLinkDown(1);
#endif
				adslNewImg = 2;
			}
			else if (2 == adslNewImg) {
				int ctrl = 0, tcMode = kXdslDataAtm;
#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM963138) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963148)
				ctrl   = ((ulong *) XDSL_ENUM_BASE)[0];
				tcMode = ((ulong *) XDSL_ENUM_BASE)[3];
#endif
				if ((ctrl != 0) && (tcMode != 0)) {

				  tcMode &= 3;
				  AdslDrvXtmLinkUp(0, tcMode);
#ifdef SUPPORT_DSL_BONDING
				  AdslDrvXtmLinkUp(1, tcMode);
#endif
				  adslNewImg = 0;
				}
			}
#endif
			pName = ADSL_ADDR_TO_HOST(param2);
			DiagWriteString(
				0,
				DIAG_DSL_CLIENT,
				pName,
				param3,
				param4);
			BcmAdslCoreDebugCompleteCommand(0);
			break;
		case GLOBAL_EVENT_SIM_STOP:
#ifdef PHY_LOOPBACK
			AdslDrvXtmLinkDown(0);
#ifdef SUPPORT_DSL_BONDING
			AdslDrvXtmLinkDown(1);
#endif
#endif
#ifdef _NOOS
			stop_TEST(get_cpuid());
#else
			BcmAdslCoreDebugSendCommand("", DIAG_TEST_CMD_TEST_COMPLETE, 0, 0, 0);
#endif
			BcmAdslCoreDebugCompleteCommand(0);
			break;
#if defined(USE_6306_CHIP)
		case GLOBAL_EVENT_RESET_6306:
			XdslCoreHwReset6306();
			BcmAdslCoreDebugCompleteCommand(0);
			break;
#endif
#endif	/* PHY_BLOCK_TEST */
		case GLOBAL_EVENT_ZERO_BSS:
			BcmAdslCoreDebugCompleteCommand(0);
			break;
		default:
			AdslDrvPrintf (TEXT("*** %s: Unknown cmd = 0x%lX\n"), __FUNCTION__, cmdId);
			break;
	}
	
	if (!testCmdInProgress)
		adslCoreStarted = AC_TRUE;
}

#endif /* PHY_BLOCK_TEST */

#ifdef PHY_PROFILE_SUPPORT

#ifdef SYS_RANDOM_GEN
#define PROF_TIMER_SEED_INIT	srand(jiffies)
#define PROF_RANDOM32_GEN 		rand()
#else
LOCAL uint32 profileTimerSeed = 0;
LOCAL uint32 __random32(uint32 *seed);

LOCAL uint32 __random32(uint32 *seed)	/* FIXME: will get a more sophiscated algorithm later */
{
	*seed = 16807*(*seed);
	*seed += (((*seed) >> 16) & 0xffff);
	return *seed;
}

#define PROF_TIMER_SEED_INIT	(profileTimerSeed = jiffies)
#define PROF_RANDOM32_GEN 		__random32(&profileTimerSeed)
#endif

LOCAL void BcmAdslCoreProfileTimerFn(ulong arg);

LOCAL int profileStarted = 0;
LOCAL struct timer_list profileTimer;

LOCAL void BcmAdslCoreProfilingStart(void)
{
	init_timer(&profileTimer);
	profileTimer.function = BcmAdslCoreProfileTimerFn;
	profileTimer.expires = 2;	/* 10ms */
	profileTimer.data = 0;
	PROF_TIMER_SEED_INIT;
	add_timer(&profileTimer);
	profileStarted = 1;
}

void BcmAdslCoreProfilingStop(void)
{
	if(profileStarted) {
		del_timer(&profileTimer);
		profileStarted = 0;
	}
}


LOCAL void BcmAdslCoreProfileTimerFn(ulong arg)
{
	ulong 		cycleCnt0;
	ulong 		randomDelay;
	volatile ulong	*pAdslEnum = (ulong *) XDSL_ENUM_BASE;
	
	if(!profileStarted)
		return;
	
	/* Wait some random time within 250us or 250* CyclesPerUs */
	randomDelay = PROF_RANDOM32_GEN;
	
#ifdef PROF_RES_IN_US
	randomDelay = randomDelay % 250;
	cycleCnt0 = BcmAdslCoreGetCycleCount();
	while (BcmAdslCoreCycleTimeElapsedUs(BcmAdslCoreGetCycleCount(), cycleCnt0) < randomDelay);
#else
	randomDelay = randomDelay % (250 * ((adslCoreCyclesPerMs+500)/1000));
	cycleCnt0 = BcmAdslCoreGetCycleCount();
	while ((BcmAdslCoreGetCycleCount() - cycleCnt0) < randomDelay);
#endif
	
	/* Initiate PHY interupt */
#if 1
	pAdslEnum[ADSL_HOSTMESSAGE] = 1;
#else
	printk("%s: randomDelay - %d\n", __FUNCTION__, randomDelay);
#endif

	/* Re-schedule the timer */
	mod_timer(&profileTimer, 1 /* 5ms */);
}
#endif /* PHY_PROFILE_SUPPORT */

#if defined(USE_6306_CHIP)

/* CTRL_CONFIG Register */
#define CTRL_CONFIG_ENABLE_INDIRECT_MODE    1
#define CTRL_CONFIG_MEMORY_WRITE           (0 << 13)
#define CTRL_CONFIG_MEMORY_READ            (1 << 13)
#define CTRL_CONFIG_FFT_DMA_FIFO_WR        (2 << 13)
#define CTRL_CONFIG_FFT_PROC_FIFO_WR       (3 << 13)
#define CTRL_CONFIG_IO_WRITE               (4 << 13)
#define CTRL_CONFIG_IO_READ                (5 << 13)

/* SPI related */
#define MSG_TYPE_HALF_DUPLEX_WRITE  1
#define MSG_TYPE_HALF_DUPLEX_READ   2

#define SPI_CMD_READ                1
#define SPI_CMD_WRITE               0

/* 6306 Control Interface registers */
#define     RBUS_6306_CTRL_CONTROL                 0x000
#define     RBUS_6306_CTRL_CONFIG                   0x004
#define     RBUS_6306_CTRL_CPU_ADDR              0x008
#define     RBUS_6306_CTRL_CPU_READ_DATA    0x00c
#define     RBUS_6306_CTRL_CHIP_ID1                0x038
#define     RBUS_6306_CTRL_CHIP_ID2                0x03C
#define     RBUS_6306_CTRL_ANA_CONFIG_PLL_REG0  0x400
#define     RBUS_6306_CTRL_ANA_PLL_STATUS            0x47c

#define     RBUS_AFE_PRIMARY_BASE            0xB0757000
#define     RBUS_AFE_BONDING_BASE            0xB0758000
#define     RBUS_AFE_CONFIG                         0x8
#define     RBUS_AFE_BUS_READ_ADDR          0xf00
#define     RBUS_AFE_PRIMARY_INDIRECT   (RBUS_AFE_PRIMARY_BASE | RBUS_AFE_BUS_READ_ADDR)
#define     RBUS_AFE_BONDING_INDIRECT   (RBUS_AFE_BONDING_BASE | RBUS_AFE_BUS_READ_ADDR)

#define     RBUS_AFE_X_REG_CONTROL          0x030
#define     RBUS_AFE_X_REG_DATA                0x034

#define     POS_AFE_X_REG_CTRL_START       0x00
#define     POS_AFE_X_REG_CTRL_WRITE       0x02
#define     POS_AFE_X_REG_ADDR                  0x03
#define     POS_AFE_X_REG_SELECT               0x08

#define     RBUS_AFE_SPI_CONTROL1             0x800
#define     RBUS_AFE_SPI_CONTROL2             0x804
#define     RBUS_AFE_SPI_TAIL                      0x808
#define     RBUS_AFE_SPI_TX_MSG_FIFO       0x840
#define     RBUS_AFE_SPI_RX_MSG_FIFO       0x880

#define     vpApiWriteReg(x, y)     (*(unsigned int *)(x) = (y))
#define     __vpApiReadReg(x)         (*(volatile unsigned int *)(x))
unsigned int  vpApiReadReg(unsigned int x)
{
   if((x & 0xfff) > 0x7ff)
      return __vpApiReadReg(x);    /* Direct read; BusClk */
   else {
      /* Indirect read; SysClk */
      if ((x & 0xfffff000) == RBUS_AFE_PRIMARY_BASE) {
         vpApiWriteReg(RBUS_AFE_PRIMARY_INDIRECT, x);
         return __vpApiReadReg(RBUS_AFE_PRIMARY_INDIRECT);
      }
      else {
         vpApiWriteReg(RBUS_AFE_BONDING_INDIRECT, x);
         return __vpApiReadReg(RBUS_AFE_BONDING_INDIRECT);
      }
   }
}


#define     CTRL_CONTROL_ADSL_MODE          0
#define     CTRL_CONTROL_VDSL_MODE          1

static int  vsb6306VdslMode = CTRL_CONTROL_ADSL_MODE;

static int  spiWriteInProgress = 0;

static void spiWaitForWriteDone(void);

int IsAfe6306ChipUsed(void)
{
    unsigned int afeIds[2];
    
    BcmXdslCoreGetAfeBoardId(&afeIds[0]);
    
    return(((afeIds[0] & AFE_CHIP_MASK) == (AFE_CHIP_6306 << AFE_CHIP_SHIFT)) ||
            ((afeIds[1] & AFE_CHIP_MASK) == (AFE_CHIP_6306 << AFE_CHIP_SHIFT)));
}

static void writeToSPITxFIFO(unsigned int *pData, int tailBytes, int prendCount, int numWords) 
{
    int i;
    unsigned int wdata, address, spiControl2;
    
    vsb6306VdslMode = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_CONFIG) & 1;
    
/*	PR2("writeToSPITxFIFO: tailBytes = %d, prendCount = %d\n", tailBytes, prendCount); */
/*	PR1("writeToSPITxFIFO: numWords = %d\n", numWords); */
	/* program SPI master */
	/* SPIMsgData address*/
	for(i=0; i< numWords; i++) {
		address = RBUS_AFE_SPI_TX_MSG_FIFO + i*4;
		vpApiWriteReg(RBUS_AFE_BONDING_BASE | address, pData[i]);
/*		PR2("writeToSPITxFIFO i = %d, data = 0x%08x\n", i, pData[i]); */
	}
	spiControl2 = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_CONTROL2);
	if (vsb6306VdslMode == CTRL_CONTROL_VDSL_MODE) {
		/* In case of VDSL divide the clock further */
		spiControl2 = (spiControl2 & 0xfffff800) | (5 << 8) | 0xff;
	} else {
		/* In case of ADSL don't divide the clock further */
		spiControl2 = (spiControl2 & 0xfffff800) | (6 << 8) | 0x00;
	}
	/* SPI control2 register */
	vpApiWriteReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_CONTROL2, spiControl2);
	/* configure tail register to make it exactly bytes 
	   which should be byte_count + prend_bytes */
	wdata = (tailBytes) << 16 ;
	vpApiWriteReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_TAIL, wdata);	
	/* SPI control1 register */
	wdata = (prendCount << 24) | (0x3 << 16);
	vpApiWriteReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_CONTROL1, wdata);

}

static void waitForSPICmdDone(void)
{
    unsigned int rdata;
    unsigned int command_done = 0x0;
    while (command_done == 0x0){
        rdata = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_CONTROL1);
        command_done = (rdata >> 8) & 0x00000001;
    };
    /* clear the interrupt and other bits */
    vpApiWriteReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_CONTROL1, 0xffff);
    /* clean SPI tail register */ 
    vpApiWriteReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_TAIL, 0x0);
}

__inline__ unsigned int reverseBits(unsigned int input, int numBits)
{
    unsigned int output = 0;
    int i;
    for (i = 0; i < numBits -1; i++) {
        output |= input & 0x1;
        output <<= 1;
        input >>= 1;
    }
    output |= input;
    return output;
}

static void spiWaitForWriteDone(void)
{
    if (spiWriteInProgress) {
        waitForSPICmdDone();
        spiWriteInProgress = 0;
    }
}
#ifdef CONFIG_BCM963268
#define RDATA0_MASK	0xfffff
#define RDATA1_MASK	0xfff
#define	RDATA0_SHIFT	12
#define	RDATA1_SHIFT	20
#else
#define RDATA0_MASK	0x1fffff
#define RDATA1_MASK	0x7ff
#define	RDATA0_SHIFT	11
#define	RDATA1_SHIFT	21
#endif

int spiTo6306Read(int address)
{
    unsigned int byte_count, prend_count;
    unsigned int spi_command0;
    unsigned int msgType, reverse_value;
    unsigned int rdata, rdata0, rdata1;
    unsigned int start_bit;
    unsigned int default_value_vector32;
    unsigned int default_value_vector8;
    unsigned int txdata[8];

    /* Wait for completion of previous SPI Write command */
    spiWaitForWriteDone();

    vsb6306VdslMode = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_CONFIG) & 1;

    if (vsb6306VdslMode == CTRL_CONTROL_VDSL_MODE) {
        default_value_vector32 = 0x7fffffff;   
        default_value_vector8 = 0xff; 
        start_bit = 0x0;
    } else {
        default_value_vector32 = 0x0; 
        default_value_vector8 = 0x0;
        start_bit = 0x1;
    }     

    /* PR1("spiTo6306Read: vsb6306VdslMode = %d\n", vsb6306VdslMode); */

    /* the address is only 16-bit */
    address &= 0xffff;    
    msgType = MSG_TYPE_HALF_DUPLEX_READ; 
    byte_count = 0x6; 
    prend_count = 0x3;
    reverse_value = 0x0;

    /* address need to reverse bits because 6306 is LSB first SPI is MSB first */
    /* reverse address bits */
    address >>= 2;
    reverse_value = reverseBits(address, 14);
    spi_command0 = (start_bit << 15) | ((reverse_value & 0x3fff) << 1); 

    txdata[0] = (msgType << 30 |  byte_count << 16 | spi_command0);
    txdata[1] = (SPI_CMD_READ << 31 ) | default_value_vector32;
    writeToSPITxFIFO(txdata, 5, prend_count, 2);
    
    waitForSPICmdDone();
    
    /* read back the data from receive fifo */
    rdata0 = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_SPI_RX_MSG_FIFO);
    rdata1 = vpApiReadReg(RBUS_AFE_BONDING_BASE | (RBUS_AFE_SPI_RX_MSG_FIFO + 4));

    /* The number of bits that is valid in rdata0 is a result of the hardware design and */
    /* provided by the HW designer based on the design and the waveform. If the design changes */
    /* this may change and need to be found out looking at the waveform */
    /* for rdata0, only [20:0] contain useful information */
    /* for rdata1, only [31:21] contain useful information */
    /* read back data should be {rdata0[20:0], rdata1[31:21]} */
    rdata = ((rdata0 & RDATA0_MASK) << RDATA0_SHIFT) | ((rdata1 >> RDATA1_SHIFT) & RDATA1_MASK);
    
    /* revers bit order due to 6306 LSB first SPI MSB first */
    reverse_value = reverseBits(rdata, 32);

    /* PR2("SPI to 6306 Read operation with address = %08x, data = %08x\n", address, reverse_value); */
    
    return reverse_value;
}

int spiTo6306IndirectRead(int address)
{
    int returnValue;
    unsigned int spi_address, spi_data;
    unsigned int msgType;
    unsigned int byte_count, prend_count;
    unsigned int spi_command0, spi_command1;
    unsigned int reverse_value, start_bit, default_value16, stop_bit;
    unsigned int txdata[8];
    unsigned int indirectLength, incommand, cmdConfig;
    int unit;

    /* Wait for completion of previous SPI Write command */
    spiWaitForWriteDone();
    
    vsb6306VdslMode = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_CONFIG) & 1;
    
    if (vsb6306VdslMode == CTRL_CONTROL_VDSL_MODE) {
        default_value16 = 0x7fff;
        start_bit = 0x0;
        stop_bit  = 0x1;
    } else {
        default_value16 = 0x0;
        start_bit = 0x1;
        stop_bit  = 0x0;
    }

    msgType = MSG_TYPE_HALF_DUPLEX_WRITE;
    byte_count = 9;
    prend_count = 0x2;

    /* write cpu_addr */
    spi_address = RBUS_6306_CTRL_CPU_ADDR >> 2;
    spi_data    = (address & 0xffff);
    reverse_value = reverseBits(spi_address, 14);
    txdata[0] = (msgType << 30 |  byte_count << 16 | (start_bit << 15) | ((reverse_value & 0x3fff) << 1));

    reverse_value = reverseBits(spi_data, 16);
    spi_command1 = (SPI_CMD_WRITE   << 31)| ((reverse_value&0xffff) << 15) | (stop_bit << 14) | (start_bit << 13);   

    /* Config */
    spi_address = RBUS_6306_CTRL_CONFIG >> 2;
    indirectLength = 0;
    unit = address & 0x000f0000;
    if (unit == 0x50000) {
        incommand = CTRL_CONFIG_IO_READ;
    } else
        incommand = CTRL_CONFIG_MEMORY_READ;

    cmdConfig = (incommand | indirectLength << 1 | CTRL_CONFIG_ENABLE_INDIRECT_MODE);

    reverse_value = reverseBits(spi_address, 14);
    txdata[1] = spi_command1 | ((reverse_value>>1) & 0x1fff);

    spi_command0 = (reverse_value << 31) | (SPI_CMD_WRITE << 29);
    spi_data  = cmdConfig & 0xffff;
    reverse_value = reverseBits(spi_data, 16);
    spi_command0 |= ((reverse_value << 13) | (default_value16 & 0x1fff));
    txdata[2] = spi_command0;
    writeToSPITxFIFO(txdata, (byte_count+prend_count), prend_count, 3);
    
    waitForSPICmdDone();

    /* Write the data */
    returnValue = spiTo6306Read(RBUS_6306_CTRL_CPU_READ_DATA);
    /* PR2("SPI to 6306 IndirectRead operation with address = %08x, data = %08x\n", address, returnValue); */
    /* PR2("incommand = %08x, indirectLength = %08x\n", incommand, indirectLength); */
    return returnValue;
}

void spiTo6306Write(int address, unsigned int data)
{
    unsigned int spi_address;
    unsigned int msgType;
    unsigned int byte_count, prend_count;
    unsigned int spi_command0, spi_command1, spi_command2;
    unsigned int reverse_value, start_bit, default_value16;
    unsigned int txdata[8];

    /* Wait for completion of previous SPI Write command */
    spiWaitForWriteDone();
    
    vsb6306VdslMode = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_CONFIG) & 1;
    
    if (vsb6306VdslMode == CTRL_CONTROL_VDSL_MODE) {
        default_value16 = 0x7fff;  
        start_bit = 0x0; 
    } else {
        default_value16 = 0x0;
        start_bit = 0x1;
    }

    /* both address and data are only 16 bit */
    address &= 0xffff;    
    data    &= 0xffff;

    msgType = MSG_TYPE_HALF_DUPLEX_WRITE;
    byte_count = 0x5; 
    spi_address = address >> 2;
    prend_count = 0x2;
    reverse_value = 0x0;

    /* PR2("SPI to 6306 Write operation with address = %08x, data = %08x\n", address, data); */

    /* both address and data need to revers bits because 6306 is LSB first SPI is MSB first */
    /* reverse address bits */
    reverse_value = reverseBits(spi_address, 14);
    
    /* add starting bit at the beginning, 
       then the address to form the first command for SPI interface*/
    spi_command0 = (start_bit << 15) | ((reverse_value & 0x3fff) << 1);
    
    /* reverse data bits */
    reverse_value = reverseBits(data, 16);

    /* read_write direction need to be after the address,
       Then the 14:0 of the data bits. the bit 15 data will
       be on the next transfer */
    spi_command1 = (SPI_CMD_WRITE   << 15)| (reverse_value >> 1 & 0x7fff);
    /* bit 15 data with default value following base on what mode 6306 is */
    spi_command2 =  ((reverse_value << 15) & 0x8000) | default_value16;

    txdata[0] = (msgType << 30 |  byte_count << 16 | spi_command0);
    txdata[1] = spi_command1 << 16 | spi_command2;
    writeToSPITxFIFO(txdata, (byte_count+prend_count), prend_count, 2);


    spiWriteInProgress = 1;
}


void spiTo6306IndirectWrite(int address, unsigned int data)
{
    unsigned int spi_address, spi_data;
    unsigned int msgType;
    unsigned int byte_count, prend_count;
    unsigned int spi_command0, spi_command1;
    unsigned int reverse_value, start_bit, default_value16, stop_bit;
    unsigned int txdata[8];
    unsigned int indirectLength, incommand, cmdConfig;
    int unit;

    /* PR2("spiTo6306IndirectWrite: addr = 0x%08x, data = 0x%08x\n", address, data); */

    /* Wait for completion of previous SPI Write command */
    spiWaitForWriteDone();
    
    vsb6306VdslMode = vpApiReadReg(RBUS_AFE_BONDING_BASE | RBUS_AFE_CONFIG) & 1;
    
    if (vsb6306VdslMode == CTRL_CONTROL_VDSL_MODE) {
        default_value16 = 0x7fff;  
        start_bit = 0x0; 
        stop_bit  = 0x1; 
    } else {
        default_value16 = 0x0; 
        start_bit = 0x1;
        stop_bit  = 0x0;
    }   

    msgType = MSG_TYPE_HALF_DUPLEX_WRITE;
    byte_count = 13; 
    prend_count = 0x2;

    /* write cpu_addr */
    spi_address = RBUS_6306_CTRL_CPU_ADDR >> 2;
    spi_data    = (address & 0xffff);
    reverse_value = reverseBits(spi_address, 14);
    txdata[0] = (msgType << 30 |  byte_count << 16 | (start_bit << 15) | ((reverse_value & 0x3fff) << 1));

    reverse_value = reverseBits(spi_data, 16);
    spi_command1 = (SPI_CMD_WRITE   << 31)| ((reverse_value&0xffff) << 15) | (stop_bit << 14) | (start_bit << 13);   

    /* Config */
    spi_address = RBUS_6306_CTRL_CONFIG >> 2;
    indirectLength = 0;

    unit = address & 0x000f0000;
    if (unit == 0) {
        incommand = CTRL_CONFIG_MEMORY_WRITE;
    } 
    else if (unit  == 0x50000) {
        incommand = CTRL_CONFIG_IO_WRITE;
    } else if (unit  == 0x60000) {
        incommand = CTRL_CONFIG_FFT_PROC_FIFO_WR;
    } else if (unit  == 0x70000) {
        incommand = CTRL_CONFIG_FFT_DMA_FIFO_WR;
    }
    else {
        incommand = CTRL_CONFIG_MEMORY_WRITE; /* Default */
    }
    cmdConfig = (incommand | indirectLength << 1 | CTRL_CONFIG_ENABLE_INDIRECT_MODE);

    reverse_value = reverseBits(spi_address, 14);
    txdata[1] = spi_command1 | ((reverse_value>>1) & 0x1fff);

    spi_command0 = (reverse_value << 31) | (SPI_CMD_WRITE << 29);
    spi_data  = cmdConfig & 0xffff;
    reverse_value = reverseBits(spi_data, 16);
    spi_command0 |= ((reverse_value << 13) | (stop_bit << 12) | (start_bit << 11));
    reverse_value = reverseBits(data, 32);
    txdata[2] = spi_command0 | ((reverse_value >> 21) & 0x7ff);
    txdata[3] = (reverse_value << 11) | (default_value16 & 0x7ff);
    writeToSPITxFIFO(txdata, (byte_count+prend_count), prend_count, 4);
    spiWriteInProgress = 1;
}


void SetupReferenceClockTo6306(void)
{
#ifdef CONFIG_BCM963268
	int chipRevId = (PERF->RevID & REV_ID_MASK);
#endif
    AdslDrvPrintf(TEXT("**** SetupReferenceClockTo6306 ****\n"));
    if(0 ==(MISC->miscPllLock_stat & MISC_LCPLL_LOCK))
        AdslDrvPrintf(TEXT("SetupReferenceClockTo6306: LCPLL is not lock!\n"));
    if( (0xA0 == chipRevId) || (0xB0 == chipRevId) )
        MISC->miscLcpll_ctrl |= MISC_CLK64_EXOUT_EN | MISC_CLK64_ENABLE;
    else
        MISC->miscLcpll_ctrl |= MISC_CLK64_EXOUT_EN;
}
#endif /* CONFIG_VDSL_SUPPORTED */

#if defined(CONFIG_VDSL_SUPPORTED) && defined(BOARD_H_API_VER) && (BOARD_H_API_VER > 6)
int IsLD6303VR5p3Used(void)
{
    unsigned int afeIds[2];
    
    BcmXdslCoreGetAfeBoardId(&afeIds[0]);
    
    return((((afeIds[0] & AFE_LD_MASK) == AFE_LD_6303_BITMAP) && ((afeIds[0] & AFE_LD_REV_MASK) == AFE_LD_REV_6303_VR5P3_BITMAP)) ||
            (((afeIds[1] & AFE_LD_MASK) == AFE_LD_6303_BITMAP) && ((afeIds[1] & AFE_LD_REV_MASK) == AFE_LD_REV_6303_VR5P3_BITMAP)));
}
#endif

#ifdef __KERNEL__
MODULE_LICENSE("Proprietary");
#endif
