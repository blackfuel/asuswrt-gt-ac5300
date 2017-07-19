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
 * serdes_debug.c -- Bcm Wan Serdes Debug Method
 *
 * Description:
 *	This file contains the debug implementation for Wan Serdes
 *    work with the script "serdesctrl"
 *
 * Authors: Fuguo Xu, Akiva Sadovski
 *
 * $Revision: 1.1 $
 *
 * $Id: serdes_debug.c,v 1.1 2015/12/30 Fuguo Exp $
 *
 * $Log: serdes_debug.c,v $
 * Revision 1.1  2015/12/30 Fuguo
 * Initial version.
 *
 ****************************************************************************/

#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include "wan_drv.h"
#include "pon_drv.h"
#include "eagle_onu10g_dependencies.h"
#include "eagle_onu10g_interface.h"

#define PROC_CMD_MAX_LEN 64

static ssize_t proc_get_serdes_reg(struct file *, char *, size_t, loff_t *);
static ssize_t proc_set_serdes_reg(struct file *, const char *, size_t, loff_t *);
static ssize_t proc_get_serdes_prbs(struct file *, char *, size_t, loff_t *);
static ssize_t proc_set_serdes_prbs(struct file *, const char *, size_t, loff_t *);
static ssize_t proc_set_serdes_display(struct file *, const char *, size_t, loff_t *);
static ssize_t proc_get_pll_sel(struct file *, char *, size_t, loff_t *);
static ssize_t proc_set_pll_sel(struct file *, const char *, size_t, loff_t *);


static uint16_t g_reg_read_addr=0;

static struct file_operations serdes_reg_proc =
{
    .read  = proc_get_serdes_reg,
    .write = proc_set_serdes_reg,
};

static struct file_operations serdes_prbs_proc =
{
    .read  = proc_get_serdes_prbs,
    .write = proc_set_serdes_prbs,
};

static struct file_operations serdes_display_proc =
{
    .write = proc_set_serdes_display,
};


static struct file_operations serdes_pllsel_proc =
{
    .read  = proc_get_pll_sel,
    .write = proc_set_pll_sel,
};


static ssize_t proc_get_pll_sel(struct file *file, char *buff, size_t len, loff_t *offset)
{
    int ret=0;
    uint8_t val;

    __logDebug("get_pll_sel \n");
    if (0 == *offset)
    {
        val = eagle_onu10g_get_pll();
        *offset = sprintf(buff, "%hhX", val);
        ret = *offset;
    }
    return ret;
}

static ssize_t proc_set_pll_sel(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    char input[PROC_CMD_MAX_LEN];
    uint8_t val;
    int ret=0;

    void help(void)
    {
        printk("    Params: pllsel(0|1)\n");
    }
    
    if (len > PROC_CMD_MAX_LEN)
        len = PROC_CMD_MAX_LEN;

    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;

    if (strncasecmp(input, "help", 4) == 0)
    {
        help();
        return len;
    }

    ret = sscanf(input, "%hhX", &val);

    if(1 == ret)
    {
        __logDebug("val=%u\n", val);
        eagle_onu10g_set_pll(val);
    }
    else
    {
        __logError("Error format!");
        help();
        return -EFAULT;
    }

    return len;
}



static ssize_t proc_get_serdes_reg(struct file *file, char *buff, size_t len, loff_t *offset)
{
    int ret=0;
    uint16_t val;

    __logDebug("addr=0x%04X\n", g_reg_read_addr);
    if (0 == *offset)
    {
        eagle_onu10g_pmd_rdt_reg(g_reg_read_addr, &val);
        *offset = sprintf(buff, "%04X", val);
        ret = *offset;
    }
    return ret;
}

static ssize_t proc_set_serdes_reg(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    char input[PROC_CMD_MAX_LEN];
    uint32_t addr, val, mask;
    int ret=0;

    void help(void)
    {
        printk("    Params: address(Hex) value(Hex) mask(Hex)\n");
    }
    
    if (len > PROC_CMD_MAX_LEN)
        len = PROC_CMD_MAX_LEN;

    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;

    if (strncasecmp(input, "help", 4) == 0)
    {
        help();
        return len;
    }

    ret = sscanf(input, "%x %x %x", &addr, &val, &mask);
    if (1 == ret)
    {
        g_reg_read_addr = addr;
    }
    else if (2 == ret)
    {
        __logDebug("addr=0x%04X, val=0x%04X\n", addr, val);
        eagle_onu10g_pmd_wr_reg(addr&0xffff, val&0xffff);
    }
    else if (3 == ret)
    {
      __logDebug("addr=0x%04X, val=0x%04X, mask=0x%04X\n", addr, val, mask);
        eagle_onu10g_pmd_wr_reg(addr&0xffff, val&mask);
    }
    else
    {
        __logError("Error format!");
        help();
        return -EFAULT;
    }

    return len;
}

static ssize_t proc_get_serdes_prbs(struct file *file, char *buff, size_t len, loff_t *offset)
{
    int ret=0;
    bdmf_boolean valid=0;
    uint32_t errors;

    if (0 == *offset)
    {
        wan_prbs_status(&valid, &errors);
        *offset = sprintf(buff, "%d", valid);
        ret = *offset;
    }
    return ret;
}

static ssize_t proc_set_serdes_prbs(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    char input[PROC_CMD_MAX_LEN];
    bdmf_boolean valid;
    int wan_type;
    int mode;
    int enable_host_tracking;
    uint32_t enable;

    void help(void)
    {
        printk("    Params: enable enable_host_tracking mode wan_type\n");
        printk("    - enable: <0|1>\n"
               "    - enable_host_tracking: <0|1>\n"
               "    - mode: 0:PRBS_7, 1:PRBS_15, 2:PRBS_23, 3:PRBS_31\n"
               "    - wan_type: %d:GPON, %d:EPON_1G, %d:EPON_2G, %d:AE, %d:AE_10G, %d:EPON_10G_SYM, %d:EPON_10G_ASYM,\n" \
               "                %d:XGPON_10G_2_5G, %d:NGPON_10G_10G, %d:NGPON_10G_10G_8B10B, %d:NGPON_10G_2_5G_8B10B\n",
                                SERDES_WAN_TYPE_GPON, SERDES_WAN_TYPE_EPON_1G, SERDES_WAN_TYPE_EPON_2G,
                                SERDES_WAN_TYPE_AE, SERDES_WAN_TYPE_AE_10G, SERDES_WAN_TYPE_EPON_10G_SYM,
                                SERDES_WAN_TYPE_EPON_10G_ASYM, SERDES_WAN_TYPE_XGPON_10G_2_5G, SERDES_WAN_TYPE_NGPON_10G_10G,
                                SERDES_WAN_TYPE_NGPON_10G_10G_8B10B, SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B);
    }

    if (len > PROC_CMD_MAX_LEN)
        len = PROC_CMD_MAX_LEN;

    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;

    if (strncasecmp(input, "help", 4) == 0)
    {
        help();
        return len;
    }

    if (sscanf(input, "%d %d %d %d", &enable, &enable_host_tracking, &mode, &wan_type) != 4)
    {
        __logError("Error format!");
        help();
        return -EFAULT;
    }

    __logDebug("enable=%d, enable_host_tracking=%d, mode=%d, wan_type=%d\n", enable, enable_host_tracking, mode, wan_type);
    wan_prbs_gen(enable, enable_host_tracking, mode, wan_type, &valid);

    return len;
}

static ssize_t proc_set_serdes_display(struct file *file, const char *buff, size_t len, loff_t *offset)
{
    char input[PROC_CMD_MAX_LEN]={0}, *ptr;

    void help(void)
    {
        printk("    Params: <diag diag_level> | <ln_st_legd>\n");
        printk("    - diag: display diag data\n"
               "    - diag_level: <0 - 19>\n"
               "    - ln_st_legd: display lane state legend\n");
    }

    if (len > PROC_CMD_MAX_LEN)
        len = PROC_CMD_MAX_LEN;

    if (copy_from_user(input, buff, len) != 0)
        return -EFAULT;

    if (strncasecmp(input, "help", 4) == 0)
    {
        help();
        return len;
    }

    if (strncasecmp(input, "diag", 4) == 0)
    {
        uint32_t  diag_level;
        ptr = &input[4];
        if (sscanf(ptr, "%d", &diag_level) != 1)
        {
            __logError("Error format!");
            help();
            return -EFAULT;
        }
        __logDebug("diag_level=%d\n", diag_level);
        eagle_onu10g_display_diag_data(diag_level);
    }
    else if (strncasecmp(input, "ln_st_legd", 10) == 0)
    {
        eagle_onu10g_display_lane_state_legend();
    }
    else
    {
        __logError("Unknow cmds: %s\n", input);
        help();
        return -EFAULT;
    }

    return len;
}


int serdes_debug_init(void)
{
    struct proc_dir_entry *p0;
    struct proc_dir_entry *p1;
    struct proc_dir_entry *p2;

    p0 = proc_mkdir("serdes", NULL);
    if (!p0)
    {
        __logError("failed to create /proc/serdes !");
        return -1;
    }

    p1 = proc_mkdir("wan", p0);
    if (!p1)
    {
        __logError("failed to create /proc/serdes/wan !");
        return -1;
    }

    p2 = proc_create("reg", S_IWUSR | S_IRUSR, p1, &serdes_reg_proc);
    if (!p2)
    {
        __logError("failed to create /proc/serdes/wan/reg !");
        return -1;
    }

    p2 = proc_create("prbs", S_IWUSR | S_IRUSR, p1, &serdes_prbs_proc);
    if (!p2)
    {
        __logError("failed to create /proc/serdes/wan/prbs !");
        return -1;
    }

    p2 = proc_create("display", S_IWUSR | S_IRUSR, p1, &serdes_display_proc);
    if (!p2)
    {
        __logError("failed to create /proc/serdes/wan/display !");
        return -1;
    }

    p2 = proc_create("pllsel", S_IWUSR | S_IRUSR, p1, &serdes_pllsel_proc);
    if (!p2)
    {
        __logError("failed to create /proc/serdes/wan/pllsel !");
        return -1;
    }

    return 0;
}

