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
 * AdslCore.c -- Bcm ADSL core driver
 *
 * Description:
 *	This file contains BCM ADSL core driver 
 *
 *
 * Copyright (c) 2000-2001  Broadcom Corporation
 * All Rights Reserved
 * No portions of this material may be reproduced in any form without the
 * written permission of:
 *          Broadcom Corporation
 *          16215 Alton Parkway
 *          Irvine, California 92619
 * All information contained in this document is Broadcom Corporation
 * company private, proprietary, and trade secret.
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.4 $
 *
 * $Id: AdslCoreDefs.h,v 1.4 2004/07/20 23:45:48 ilyas Exp $
 *
 * $Log: AdslCoreDefs.h,v $
 * Revision 1.4  2004/07/20 23:45:48  ilyas
 * Added driver version info, SoftDslPrintf support. Fixed G.997 related issues
 *
 * Revision 1.3  2004/06/10 00:20:33  ilyas
 * Added L2/L3 and SRA
 *
 * Revision 1.2  2004/04/12 23:24:38  ilyas
 * Added default G992P5 PHY definition
 *
 * Revision 1.1  2004/04/08 23:59:15  ilyas
 * Initial CVS checkin
 *
 ****************************************************************************/

#ifndef _ADSL_CORE_DEFS_H
#define _ADSL_CORE_DEFS_H

#if defined(__KERNEL__)
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
#include <generated/autoconf.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
#include <linux/autoconf.h>
#else
#include <linux/config.h>
#endif
#endif
#ifdef CONFIG_ARM
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
#include <asm/io.h>
#else
#include <linux/memory.h>
#endif
#endif

#if (defined(BOARD_bcm96348) || defined(_BCM96348_))
#define CONFIG_BCM96348	1
#endif

#if (defined(BOARD_bcm96338) || defined(_BCM96338_))
#define	CONFIG_BCM96338	1
#endif



#if !defined(CONFIG_BCM96362) && (defined(BOARD_bcm96362) || defined(_BCM96362_))
#define CONFIG_BCM96362	1
#endif

#if !defined(CONFIG_BCM96328) && (defined(BOARD_bcm96328) || defined(_BCM96328_))
#define CONFIG_BCM96328	1
#endif

#if !defined(CONFIG_BCM963268) && (defined(BOARD_bcm963268) || defined(_BCM963268_))
#define	CONFIG_BCM963268	1
#endif

#if !defined(CONFIG_BCM96318) && (defined(BOARD_bcm96318) || defined(_BCM96318_))
#define	CONFIG_BCM96318	1
#endif

#if !defined(CONFIG_BCM963138) && (defined(BOARD_bcm963138) || defined(_BCM963138_))
#define	CONFIG_BCM963138	1
#endif

#if !defined(CONFIG_BCM963381) && (defined(BOARD_bcm963381) || defined(_BCM963381_))
#define	CONFIG_BCM963381	1
#endif

#if !defined(CONFIG_BCM963148) && (defined(BOARD_bcm963148) || defined(_BCM963148_))
#define	CONFIG_BCM963148	1
#endif

#if defined(CONFIG_BCM96362) ||      \
	defined(CONFIG_BCM96328) ||      \
	 defined(CONFIG_BCM963268) ||      \
	  defined(CONFIG_BCM96318) ||      \
	defined(CONFIG_BCM963138) ||      \
	 defined(CONFIG_BCM963381) ||      \
	  defined(CONFIG_BCM963148)
#define	CONFIG_BCM963x8
#endif

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148)
#define	CONFIG_PHY_PARAM
#endif

#if defined(CONFIG_BCM963x8)

#if !defined( __GNUC__) && defined(__arm)
#pragma anon_unions
#define    __inline__    __inline
#endif


#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM96328)

#ifdef ADSL_ANNEXB
#include "../adslcore6362B/adsl_defs.h"
#else
#include "../adslcore6362/adsl_defs.h"
#endif

#elif defined(CONFIG_BCM963268)

#ifdef PHY_LOOPBACK
#include "../adslcore63268LB/adsl_defs.h"
#else
#ifdef ADSL_ANNEXB
#include "../adslcore63268B/adsl_defs.h"
#elif defined(SUPPORT_DSL_BONDING) && !defined(SUPPORT_2CHIP_BONDING)
#ifdef SUPPORT_DSL_BONDING5B
#include "../adslcore63268bnd5/adsl_defs.h"
#else
#include "../adslcore63268bnd/adsl_defs.h"
#endif
#else
#include "../adslcore63268/adsl_defs.h"
#endif
#endif

#elif defined(CONFIG_BCM96318)

#ifdef ADSL_ANNEXB
#include "../adslcore6318B/adsl_defs.h"
#else
#include "../adslcore6318/adsl_defs.h"
#endif

#elif defined(CONFIG_BCM963138)

#if defined(PHY_LOOPBACK)
#include "../adslcore63138LB/adsl_defs.h"
#elif defined(ADSL_ANNEXB)
#include "../adslcore63138B/adsl_defs.h"
#else
#include "../adslcore63138/adsl_defs.h"
#endif

#elif defined(CONFIG_BCM963381)

#ifdef PHY_LOOPBACK
#include "../adslcore63381LB/adsl_defs.h"
#else
#ifdef ADSL_ANNEXB
#include "../adslcore63381B/adsl_defs.h"
#else
#include "../adslcore63381/adsl_defs.h"
#endif
#endif

#elif defined(CONFIG_BCM963148)

#ifdef PHY_LOOPBACK
#include "../adslcore63148LB/adsl_defs.h"
#else
#ifdef ADSL_ANNEXB
#include "../adslcore63148B/adsl_defs.h"
#else
#include "../adslcore63148/adsl_defs.h"
#endif
#endif

#else

#ifdef ADSL_ANNEXC
#include "../adslcore6348C/adsl_defs.h"
#elif defined(ADSL_ANNEXB)
#include "../adslcore6348B/adsl_defs.h"
#elif defined(ADSL_SADSL)
#include "../adslcore6348SA/adsl_defs.h"
#else
#include "../adslcore6348/adsl_defs.h"
#endif

#endif
#endif /* defined(CONFIG_BCM963x8) */

#ifdef _WIN32_WCE
#define	ASSERT(a)
#endif

#include "AdslXfaceData.h"
#ifdef XDSL_DRV
#include "AdslMibDef.h"
#include "DiagDef.h"
int DslGetLineId(void *gDslVars);
#ifndef __SoftDslPrintf
#if defined(XDSLDRV_ENABLE_PARSER)
#define __SoftDslPrintf(gv,fmt,an, ...)  DiagWriteString(DslGetLineId(gv),DIAG_DSL_CLIENT,fmt, ## __VA_ARGS__)
#else
#define __SoftDslPrintf(gv,fmt,an, ...)  DiagStrPrintf(DslGetLineId(gv),DIAG_DSL_CLIENT,fmt, ## __VA_ARGS__)
#endif
#endif
#endif

/* adjust some definitions for the HOST */

#if !defined(CONFIG_VDSL_SUPPORTED)
#undef	G993
#undef	BCM6368_SRC
#else
#undef VECTORING
#define VECTORING
#undef G993P5_OVERHEAD_MESSAGING
#define G993P5_OVERHEAD_MESSAGING
#undef DSL_EXTERNAL_BONDING_DISCOVERY
#define DSL_EXTERNAL_BONDING_DISCOVERY
#endif
#undef	DSLVARS_GLOBAL_REG
#undef	GLOBAL_PTR_BIAS
#undef	ADSLCORE_ONLY
#undef	USE_SLOW_DATA 
#undef	USE_FAST_TEXT 
#undef	VP_SIMULATOR 
#undef	bcm47xx 
#undef	ADSL_FRAMER
#undef	ATM
#undef	ATM_I432 
#undef	DSL_OS
#undef	PTM

#define HOST_ONLY
#define G997_1_FRAMER
#define ADSL_MIB

//#define ADSL_MIBOBJ_PLN

/* definitions for combo PHY (AnnexA(ADSL2) and AnnexB) */
 
#if !(defined(ADSL_SINGLE_PHY) || defined(G992_ANNEXC))


#undef	G992P1_ANNEX_A
#define	G992P1_ANNEX_A
#undef	G992P3
#define	G992P3
#undef	G992P5
#define	G992P5
#define	G992P1_ANNEX_A
#undef	READSL2
#define	READSL2
#undef	G992P1_ANNEX_A_USED_FOR_G992P2
#define	G992P1_ANNEX_A_USED_FOR_G992P2
#undef	T1P413
#define	T1P413

#undef	G992P1_ANNEX_B
#define	G992P1_ANNEX_B

#endif

/* ADSL PHY definition */

typedef struct {
	unsigned long	sdramPageAddr;
	unsigned long	sdramImageAddr;
	unsigned long	sdramImageSize;
	unsigned long	sdramPhyImageAddr;
	unsigned short	fwType;
	unsigned short	chipType;
	unsigned short	mjVerNum;
	unsigned short	mnVerNum;
	char			*pVerStr;
	unsigned long	features[4];
	unsigned long	sdramPageSize;
} adslPhyInfo;
extern adslPhyInfo	adslCorePhyDesc;
extern unsigned long	adslPhyXfaceOffset;

/* chip list */ 

#define	kAdslPhyChipMjMask			0xFF00
#define	kAdslPhyChipMnMask			0x00FF
#define	kAdslPhyChipUnknown			0
#define	kAdslPhyChip6345			0x100
#define	kAdslPhyChip6348			0x200
#define	kAdslPhyChip6368			0x300
#define	kAdslPhyChip6362			0x400
#define	kAdslPhyChipRev0			0
#define	kAdslPhyChipRev1			1
#define	kAdslPhyChipRev2			2
#define	kAdslPhyChipRev3			3
#define	kAdslPhyChipRev4			4
#define	kAdslPhyChipRev5			5

#define	ADSL_PHY_SUPPORT(f)			AdslFeatureSupported(adslCorePhyDesc.features,f)
#define	ADSL_PHY_SET_SUPPORT(p,f)	AdslFeatureSet((p)->features,f)

#ifndef HOST_LMEM_BASE
#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM96328)
 #define HOST_LMEM_BASE					0xB0D80000L
#elif defined(CONFIG_BCM963268)
 #define HOST_LMEM_BASE					0xB0780000L
#elif defined(CONFIG_BCM96318)
 #define HOST_LMEM_BASE					0xB0180000
#elif defined(CONFIG_BCM963138) || defined(CONFIG_BCM963381) || defined(CONFIG_BCM963148)
 #define HOST_LMEM_BASE					DSLLMEM_BASE
 #else
 #define HOST_LMEM_BASE					0xFFF00000L
 #endif
#endif

#ifndef DSL_PHY_LMEM_BASE
#define DSL_PHY_LMEM_BASE				0x19000000L
#endif

#ifndef DSL_PHY_SDRAM_BASE
#define DSL_PHY_SDRAM_BASE				0x10000000L
#define DSL_PHY_SDRAM_ADDR_MASK			0xF0000000
#endif

#ifndef FLATTEN_ADDR_ADJUST
#define FLATTEN_ADDR_ADJUST				(HOST_LMEM_BASE - DSL_PHY_LMEM_BASE)
#endif

#ifndef ADSL_PHY_XFACE_OFFSET
#define ADSL_PHY_XFACE_OFFSET			0x00017F90
#endif
#define ADSL_LMEM_XFACE_DATA			(HOST_LMEM_BASE | adslPhyXfaceOffset)

#ifndef ADSL_PHY_SDRAM_START
#define ADSL_PHY_SDRAM_START			0x10000000
#endif
#ifndef ADSL_PHY_SDRAM_BIAS
#define ADSL_PHY_SDRAM_BIAS				0x00040000
#endif
#define	ADSL_PHY_SDRAM_START_4			(ADSL_PHY_SDRAM_START + ADSL_PHY_SDRAM_BIAS)

#ifndef	ADSL_PHY_SDRAM_PAGE_SIZE
#define ADSL_PHY_SDRAM_PAGE_SIZE		0x00080000
#endif

#ifdef ADSL_PHY_SDRAM_BIAS
#define ADSL_SDRAM_IMAGE_SIZE			(ADSL_PHY_SDRAM_PAGE_SIZE - ADSL_PHY_SDRAM_BIAS)
#else
#define ADSL_SDRAM_IMAGE_SIZE			(256*1024)
#endif


#ifndef	ADSL_PHY_SDRAM_LINK_OFFSET
#define ADSL_PHY_SDRAM_LINK_OFFSET		0x00040000
#endif

#define ADSL_SDRAM_TOTAL_SIZE			0x00800000
#ifdef CONFIG_ARM
#define ADSL_SDRAM_HOST_ADDR_DEFAULT	(ADSL_SDRAM_TOTAL_SIZE - ADSL_PHY_SDRAM_PAGE_SIZE + ADSL_PHY_SDRAM_BIAS)
#else
#define ADSL_SDRAM_HOST_ADDR_DEFAULT	(0xA0000000 | (ADSL_SDRAM_TOTAL_SIZE - ADSL_PHY_SDRAM_PAGE_SIZE + ADSL_PHY_SDRAM_BIAS))
#endif
#define ADSLXF							((AdslXfaceData *) ADSL_LMEM_XFACE_DATA)

#define ADSL_MIPS_LMEM_ADDR(a)			(((ulong)(a) & DSL_PHY_LMEM_BASE) == DSL_PHY_LMEM_BASE)
#define ADSL_MIPS_SDRAM_ADDR(a)			(((ulong)(a) & DSL_PHY_SDRAM_ADDR_MASK) == DSL_PHY_SDRAM_BASE)
#ifndef CONFIG_ARM
#define	SDRAM_ADDR_TO_HOST(a)			((void *) ((ulong)(a) - adslCorePhyDesc.sdramPhyImageAddr + \
										(ADSLXF->sdramBaseAddr ? (unsigned long) ADSLXF->sdramBaseAddr : ADSL_SDRAM_HOST_ADDR_DEFAULT)))
#else
#define	SDRAM_ADDR_TO_HOST(a)			((void *) ((ulong)(a) - adslCorePhyDesc.sdramPhyImageAddr + \
										(adslCorePhyDesc.sdramImageAddr ? (unsigned long) adslCorePhyDesc.sdramImageAddr : ADSL_SDRAM_HOST_ADDR_DEFAULT)))
#endif
#define LMEM_ADDR_TO_HOST(a)			((void *) ((ulong) (a) + (HOST_LMEM_BASE - ((ulong)(a) & 0xff000000))))
#ifdef CONFIG_ARM
#define SDRAM_ADDR_TO_ADSL(a)			(virt_to_phys((void *)(a)) | 0xA0000000)
#else
#define SDRAM_ADDR_TO_ADSL(a)			(a)
#endif
#define ADSL_ADDR_TO_HOST(addr)			ADSL_MIPS_LMEM_ADDR(addr) ?  LMEM_ADDR_TO_HOST(addr) : (ADSL_MIPS_SDRAM_ADDR(addr) ?  SDRAM_ADDR_TO_HOST(addr) : (void *) (addr))

#ifndef DEBUG
#define DEBUG	0
#endif

#endif

