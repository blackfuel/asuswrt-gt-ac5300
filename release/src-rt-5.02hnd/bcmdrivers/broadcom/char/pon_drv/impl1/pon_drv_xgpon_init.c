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
 * pon_xgpon_init.c -- Bcm Pon driver: init sequence for XGPON mode (with SFP+)
 *
 * Description:
 *	Built from initialization script of VLSI team
 *
 * Authors: Fuguo Xu, Akiva Sadovski
 *
 * $Revision: 1.1 $
 *
 *
 ****************************************************************************/

#include <linux/delay.h>
#include "pon_drv.h"
#include "gpon_pattern_gearbox_ag.h"
#include "ru_types.h"


/*
 *   Ugly way to translate physical to virtual for one particular block - WAN TOP
 */
#define WAN_TOP_WRITE_32(a, r)   {uint32_t val=r; unsigned long _a=(0xffffff8000002000 + (a - 0x80144000)); /*printk("--> 0x%lx 0x%x\n",_a, val);*/ WRITE_32(_a, val);}

static void xgpon_wan_top_init(void)
{
  WAN_TOP_WRITE_32(0x80144034,0x8000);
  WAN_TOP_WRITE_32(0x80144014,0x08d300d7);
  WAN_TOP_WRITE_32(0x80144014,0x08d300c7);
  WAN_TOP_WRITE_32(0x80144040,0x2);
  WAN_TOP_WRITE_32(0x80144044, 0x0);
  WAN_TOP_WRITE_32(0x80144048,0x12000000);
  WAN_TOP_WRITE_32(0x80144024,0x80067);
  WAN_TOP_WRITE_32(0x80144024,0x80061);
  WAN_TOP_WRITE_32(0x80144050,0x105);
  WAN_TOP_WRITE_32(0x80144048,0x12000038);
  WAN_TOP_WRITE_32(0x80144048,0x1200003e);
  WAN_TOP_WRITE_32(0x8014404c,0x6C);
  WAN_TOP_WRITE_32(0x80144058,0x0);
}


static void set_serdes_reg(uint32_t addr, uint32_t masked_inv_val)
{
  WAN_TOP_WRITE_32(0x80144060, (0x8000000 | addr));
  WAN_TOP_WRITE_32(0x80144064, masked_inv_val);
  WAN_TOP_WRITE_32(0x8014405c,0x3);
  udelay(1);
  WAN_TOP_WRITE_32(0x8014405c,0x0);
}



static void xgpon_serdes_init (void)
{
  set_serdes_reg(0x800d0b9, 0xfffb);
  set_serdes_reg(0x900d0b9, 0xfffb);
  set_serdes_reg(0x800d0b1, 0x80ff7f);
  set_serdes_reg(0x800d0b1, 0xffbf);
  set_serdes_reg(0x800d0b4, 0x300fcff);
  set_serdes_reg(0x800d0b4, 0x2000cfff);
  set_serdes_reg(0x800d0b4, 0x53770000);
  set_serdes_reg(0x800d0b7, 0x432c0000);
  set_serdes_reg(0x800d0b8, 0xfffc);
  set_serdes_reg(0x800d0b8, 0xfffb);
  set_serdes_reg(0x800d0b8, 0xc70c00f);
  set_serdes_reg(0x800d0b6, 0x1fffe);
  set_serdes_reg(0x800d0b8, 0x80007fff);
  set_serdes_reg(0x800d0b8, 0x4000ffff);
  set_serdes_reg(0x800d0b8, 0x8c700000);
  set_serdes_reg(0x800d08d, 0xfffe);
  set_serdes_reg(0x900d0b1, 0xff7f);
  set_serdes_reg(0x900d0b1, 0xffbf);
  set_serdes_reg(0x900d0b4, 0xfcff);
  set_serdes_reg(0x900d0b4, 0x2000cfff);
  set_serdes_reg(0x900d0b4, 0x50770000);
  set_serdes_reg(0x900d0b7, 0x432c0000);
  set_serdes_reg(0x900d0b8, 0xfffc);
  set_serdes_reg(0x900d0b8, 0xfffb);
  set_serdes_reg(0x900d0b8, 0xc70c00f);
  set_serdes_reg(0x900d0b6, 0x1fffe);
  set_serdes_reg(0x900d0b8, 0x80007fff);
  set_serdes_reg(0x900d0b8, 0x4000bfff);
  set_serdes_reg(0x900d0b8, 0x8c700000);
  set_serdes_reg(0x800d08d, 0x2fffc);
  set_serdes_reg(0x800d050, 0x3fff8);
  set_serdes_reg(0x800d001, 0x4fffb);
  set_serdes_reg(0x800d003, 0xfcff);
  set_serdes_reg(0x800d001, 0xffbf);
  set_serdes_reg(0x800d017, 0xfff0);
  set_serdes_reg(0x800d017, 0xf0ff);
  set_serdes_reg(0x800d017, 0xcfff);
  set_serdes_reg(0x800d017, 0x3fff);
  set_serdes_reg(0x800d001, 0xff7f);
  set_serdes_reg(0x800d002, 0x2b20000);
  set_serdes_reg(0x800d018, 0xfff0);
  set_serdes_reg(0x800d018, 0xfcff);
  set_serdes_reg(0x800d001, 0x1fffe);
  set_serdes_reg(0x800d003, 0xfffd);
  set_serdes_reg(0x800d003, 0x30ff0f);
  set_serdes_reg(0x800d041, 0x7fff0);
  set_serdes_reg(0x800d042, 0x3fff8);
  set_serdes_reg(0x800d02b, 0xc1ff);
  set_serdes_reg(0x800d02b, 0x100fe00);
  set_serdes_reg(0x800d02b, 0x80007fff);
  set_serdes_reg(0x800d004, 0x40008fff);
  set_serdes_reg(0x800d004, 0x400fbff);
  set_serdes_reg(0x800d004, 0x1ff80);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004, 0x200fdff);
  set_serdes_reg(0x800d004,  0x20008fff);
  set_serdes_reg(0x800d004, 0x400fbff);
  set_serdes_reg(0x800d004, 0x1ff80);
  set_serdes_reg(0x800d07c,  0x160ffff);
  set_serdes_reg(0x800d0a1, 0x1fffe);
  set_serdes_reg(0x800d0a1, 0x6fff1);
  set_serdes_reg(0x800d0a1, 0x20ff8f);
  set_serdes_reg(0x800d092, 0xA8040000);
  set_serdes_reg(0x900d092, 0xA8040000);
  set_serdes_reg(0x800d154, 0x2000dfff);
  set_serdes_reg(0x900d154, 0x2000dfff);
  set_serdes_reg(0x800d081, 0x2fffd);
  set_serdes_reg(0x800d0d1, 0xafff1);
  set_serdes_reg(0x800d0e1, 0xfff0);
  set_serdes_reg(0x800d0a0, 0x0);
  set_serdes_reg(0x800d0e0, 0xfffe);
  set_serdes_reg(0x800d0e1, 0x0fffe);
  set_serdes_reg(0x800d070, 0x2003dffc);
  set_serdes_reg(0x800d0e2, 0x0fff0);
  set_serdes_reg(0x800d0e4, 0x3ffff);
  set_serdes_reg(0x800d0a2, 0x9fff);
  set_serdes_reg(0x800d0a2, 0xdfff0);
  set_serdes_reg(0x800d00d, 0x0);
  set_serdes_reg(0x800d00d, 0);
  set_serdes_reg(0x800d0e0, 0xfffe);
  set_serdes_reg(0x800d070, 0x2003dffc);
  set_serdes_reg(0x800d050, 0x3fff8);
  set_serdes_reg(0x800d0e4, 0x3ffff);
  set_serdes_reg(0x800d0e2, 0x3ffff);
}

static void xgpon_gearbox_rx_ctrl (void)
{
  WAN_TOP_WRITE_32(0x80144014,0x08d300d7);
  WAN_TOP_WRITE_32(0x80144014,0x08d300c7);

}

void  xgpon_wan_init(void)
{
    xgpon_wan_top_init ();
    xgpon_serdes_init ();
    xgpon_gearbox_rx_ctrl ();
} 
