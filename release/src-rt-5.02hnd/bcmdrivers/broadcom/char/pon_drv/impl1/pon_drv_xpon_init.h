/*
  <:copyright-BRCM:2015:proprietary:standard
  
     Copyright (c) 2015 Broadcom 
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

/****************************************************************************
 *
 * pon_drv_xpon_init.h -- common head file for all wan type serdes init sequence
 *
 * Description:
 *	provide WAN_TOP and Serdes register access directly
 *
 * Authors: Fuguo Xu, Akiva Sadovski
 *
 * $Revision: 1.1 $
 *
 *
 ****************************************************************************/

#include "access_macros.h"
#include "ru_types.h"
#include "6858_map_part.h"

extern const ru_block_rec *WAN_TOP_BLOCKS[];

 
#define WAN_TOP_VIRTUAL_BASE ((long unsigned int) WAN_TOP_BLOCKS[0]->addr[0])
#define WAN_TOP_WRITE_32(a, r)   {uint32_t val=r; unsigned long _a=(WAN_TOP_VIRTUAL_BASE + (a - WAN_PHYS_BASE)); /*printk("--> 0x%lx 0x%x\n",_a, val);*/ WRITE_32(_a, val);}

/* writePONSerdesReg(lane, address, value, mask)
lane:     - register domain, such as Dev_Id, PLL_Id, Lane_Id
address:  - register offset
value:    - value to write
mask:     - value mask
*/
#define writePONSerdesReg(lane, address, value, mask)   {\
    uint16_t addr_major=lane; uint16_t addr_minor=address;\
    uint16_t _value=value; uint16_t _mask=(~mask) & 0xffff;\
    uint32_t _addr=(addr_major<<16) | addr_minor;\
    uint32_t masked_inv_val=(_value<<16) | _mask;\
    WAN_TOP_WRITE_32(0x80144060, _addr);\
    WAN_TOP_WRITE_32(0x80144064, masked_inv_val);\
    WAN_TOP_WRITE_32(0x8014405c,0x3);\
    udelay(1);\
    WAN_TOP_WRITE_32(0x8014405c,0x0);\
    }

