/***********************************************************
 * <:copyright-BRCM:2009:DUAL/GPL:standard
 * 
 *    Copyright (c) 2009 Broadcom 
 *    All Rights Reserved
 * 
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed
 * to you under the terms of the GNU General Public License version 2
 * (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
 * with the following added to such license:
 * 
 *    As a special exception, the copyright holders of this software give
 *    you permission to link this software with independent modules, and
 *    to copy and distribute the resulting executable under terms of your
 *    choice, provided that you also meet, for each linked independent
 *    module, the terms and conditions of the license of that module.
 *    An independent module is a module which is not derived from this
 *    software.  The special exception does not apply to any modifications
 *    of the software.
 * 
 * Not withstanding the above, under no circumstances may you combine
 * this software in any way with any other Broadcom software provided
 * under a license other than the GPL, without Broadcom's express prior
 * written consent.
 * 
 * :>
 ************************************************************/

#ifndef __FAP4KE_INIT_H_INCLUDED__
#define __FAP4KE_INIT_H_INCLUDED__

/*
 *******************************************************************************
 * File Name  : fap4ke_init.h
 *
 * FAP_TBD: move this out
 * Description: This file contains some chip specific stuff... maybe move this...
 *
 *******************************************************************************
 */

//#define CC_FAP_6362A0

#ifdef CONFIG_BCM_FAP_IPV6
#define FAP_INIT_4KE_STACK_SIZE 1792
#else
#define FAP_INIT_4KE_STACK_SIZE 1536
#endif

#if defined(CONFIG_BCM96362)
#if defined(CC_FAP_6362A0)
#define DSPRAM_SIZE          (1<<12)
#define DSPRAM_BASE          0x00002000
#else /* CC_FAP_6362A0 */
#define DSPRAM_SIZE          (1<<13)
#define DSPRAM_BASE          0x00004000
#endif /* CC_FAP_6362A0 */

#define DSPRAM_VBASE         (DSPRAM_BASE | 0x80000000)

#define FAP_INIT_DSPRAM_STACK_POINTER_LOC ( DSPRAM_VBASE + FAP_INIT_4KE_STACK_SIZE )

#else /* defined(CONFIG_BCM96362) */
#define DSPRAM_SIZE          (1<<13)
#define DSPRAM_BASE          0x00000000
#define DSPRAM_VBASE         (DSPRAM_BASE | 0x80000000)
#define FAP_INIT_DSPRAM_STACK_POINTER_LOC ( DSPRAM_VBASE + FAP_INIT_4KE_STACK_SIZE )
#endif /* defined(CONFIG_BCM96362) */

#endif  /* defined(__FAP4KE_INIT_H_INCLUDED__) */
