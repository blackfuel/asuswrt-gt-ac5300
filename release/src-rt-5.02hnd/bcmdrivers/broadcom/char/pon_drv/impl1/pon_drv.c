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
 * pon_drv.c -- Bcm Pon driver
 *
 * Description:
 *      This file contains BCM Wan Serdes, Wan Top, Wan Gearbox driver
 *
 * Authors: Fuguo Xu, Akiva Sadovski
 *
 * $Revision: 1.1 $
 *
 * $Id: pon_drv.c,v 1.1 2015/12/21 Fuguo Exp $
 *
 * $Log: pon_drv.c,v $
 * Revision 1.1  2015/12/21 Fuguo
 * Initial version.
 *
 ****************************************************************************/

#include <linux/delay.h>
#include "pmc_drv.h"
#include "pmc_wan.h"
#include "bcm_pinmux.h"
#include "bcm_gpio.h"
#include "board.h"
#include "wan_drv.h"
#include "pon_drv.h"
#include "gpon_pattern_gearbox_ag.h"
#include "ngpon_gearbox_ag.h"
#include "gpon_pattern_gearbox_ag.h"
#include "ten_g_gearbox_ag.h"
#include "ru_types.h"
#include "bcm_map_part.h"

#include "gpon_i2c.h"


#include "eagle_onu10g_interface.h"
#include "eagle_onu10g_ucode_image.h"
#include "eagle_onu10g_functions.c"
#include "tod_ag.h"
#include "misc_ag.h"
#include "wan_serdes_ag.h"
#include "serdes_status_ag.h"
#include "top_scratch_ag.h"
#include "pmi_ag.h"
#include "pon_drv_xpon_init.h"


#define TX_LASER_ON_OUT_P  53
#define TX_LASER_ON_OUT_N  54
#define A_WAN_EARLY_TXEN   55
#define B_WAN_EARLY_TXEN   56

#define FIRMWARE_READY_TIMEOUT_MS 1000


typedef enum {
    PLL_0 = 0,
    PLL_1,
    PLL_MAX,
} pll_id_t;

typedef struct cdr_ctrl_s
{
    uint8_t cdr_freq_en;
    uint8_t osx2p_pherr_gain;
    uint8_t cdr_integ_sat_sel;
    uint8_t cdr_bwsel_integ_acqcdr;
    uint8_t cdr_bwsel_integ_norm;
    uint8_t cdr_bwsel_prop_acqcdr;
    uint8_t cdr_bwsel_prop_norm;
    uint8_t cdr_freq_override_en;
    uint16_t cdr_freq_override_val;
    uint8_t phase_err_offset;
    uint8_t phase_err_offset_en;
    uint8_t cdr_phase_sat_ctrl;
    uint8_t phase_err_offset_mult_2;
    uint8_t pattern_sel;
}cdr_ctrl_t;

typedef struct osr_ctrl_s
{
    uint8_t rx_osr_mode_frc;
    uint8_t rx_osr_mode_frc_val;
    uint8_t tx_osr_mode_frc;
    uint8_t tx_osr_mode_frc_val;
}osr_ctrl_t;

typedef struct pon_mac_clk_and_sync_e_clk_s
{
    uint8_t sync_e_val;
    uint8_t tx_pon_mac_val;
    uint8_t rx_pon_mac_val;
}pon_mac_clk_and_sync_e_clk_t;

typedef struct pll_cfg_s
{
    pll_id_t tx_pll_id;
    pll_id_t rx_pll_id;
    enum eagle_onu10g_pll_enum tx_pll_cfg;
    enum eagle_onu10g_pll_enum rx_pll_cfg;
}pll_cfg_t;

typedef struct wan_info_s
{
    char * name;
}wan_info_t;
#ifdef USE_SERDES_LIB
static uint8_t *serdes_ucode = eagle_onu10g_ucode_image;
static uint16_t serdes_ucode_len = EAGLE_ONU10G_UCODE_IMAGE_SIZE;
#endif
static serdes_wan_type_t wan_serdes_type = SERDES_WAN_TYPE_NONE;
static PMD_DEV_ENABLE_PRBS pmd_prbs_callback;

extern const ru_block_rec *WAN_TOP_BLOCKS[];

static int wan_power_on(void);
static int wan_epon_zone_enable(void);
#ifdef USE_SERDES_LIB
static void wan_burst_config(serdes_wan_type_t wan_type);
#endif 
void remap_ru_block_addrs(uint32_t block_index, const ru_block_rec *ru_blocks[]);
void serdes_register_access_test(void);/* AAAAAA: Test SerDes register access.    To be remove!!  only for bring up */
#ifdef USE_SERDES_LIB
static err_code_t serdes_pll_calibration(serdes_wan_type_t wan_type);

static pon_mac_clk_and_sync_e_clk_t pon_mac_clk_and_sync_e_clk_cfg[] =
{
    [SERDES_WAN_TYPE_EPON_10G_SYM]          = {.tx_pon_mac_val=0b111, .sync_e_val=0b111, .rx_pon_mac_val=0b111},
    [SERDES_WAN_TYPE_EPON_10G_ASYM]         = {.tx_pon_mac_val=0b011, .sync_e_val=0b100, .rx_pon_mac_val=0b111},
    [SERDES_WAN_TYPE_EPON_1G]               = {.tx_pon_mac_val=0b011, .sync_e_val=0b100, .rx_pon_mac_val=0b111},
    [SERDES_WAN_TYPE_EPON_2G]               = {.tx_pon_mac_val=0b011, .sync_e_val=0b100, .rx_pon_mac_val=0b111},
    [SERDES_WAN_TYPE_AE_10G]                = {.tx_pon_mac_val=0b111, .sync_e_val=0b111, .rx_pon_mac_val=0b111},
    [SERDES_WAN_TYPE_AE]                    = {.tx_pon_mac_val=0b011, .sync_e_val=0b100, .rx_pon_mac_val=0b011},
    [SERDES_WAN_TYPE_XGPON_10G_2_5G]        = {.tx_pon_mac_val=0b010, .sync_e_val=0b011, .rx_pon_mac_val=0b100},
    [SERDES_WAN_TYPE_NGPON_10G_10G]         = {.tx_pon_mac_val=0b110, .sync_e_val=0b110, .rx_pon_mac_val=0b101},
    [SERDES_WAN_TYPE_NGPON_10G_10G_8B10B]   = {.tx_pon_mac_val=0b110, .sync_e_val=0b110, .rx_pon_mac_val=0b101},
    [SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B]  = {.tx_pon_mac_val=0b110, .sync_e_val=0b110, .rx_pon_mac_val=0b101},
    [SERDES_WAN_TYPE_GPON]                  = {.tx_pon_mac_val=0b010, .sync_e_val=0b011, .rx_pon_mac_val=0b010}
};

static pll_cfg_t pll_cfg[] =
{
    [SERDES_WAN_TYPE_GPON] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9
        },
    [SERDES_WAN_TYPE_EPON_1G] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_50x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_50x
        },
    [SERDES_WAN_TYPE_EPON_2G] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_50x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_50x
        },
    [SERDES_WAN_TYPE_AE] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_50x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_50x
        },
    [SERDES_WAN_TYPE_AE_10G] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_206p25x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_206p25x
        },
    [SERDES_WAN_TYPE_EPON_10G_SYM] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_206p25x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_206p25x
        },
    [SERDES_WAN_TYPE_EPON_10G_ASYM] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_1,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_50x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_206p25x
        },
    [SERDES_WAN_TYPE_XGPON_10G_2_5G] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_1,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9
        },
    [SERDES_WAN_TYPE_NGPON_10G_10G] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_0,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9
        },
    [SERDES_WAN_TYPE_NGPON_10G_10G_8B10B] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_1,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x_vco9p9,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_248p83_vco12p4x
        },
    [SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B] = 
        {
        .tx_pll_id  = PLL_0,
        .rx_pll_id  = PLL_1,
        .tx_pll_cfg = EAGLE_ONU10G_pll_div_199p06x,
        .rx_pll_cfg = EAGLE_ONU10G_pll_div_248p83_vco12p4x
        }
};
#endif
static wan_info_t wan_info [] =
{
    [SERDES_WAN_TYPE_NONE]                  = {.name="Unknown Wan type"},
    [SERDES_WAN_TYPE_GPON]                  = {.name="GPON"},
    [SERDES_WAN_TYPE_EPON_1G]               = {.name="EPON_1G"},
    [SERDES_WAN_TYPE_EPON_2G]               = {.name="EPON_2G"},
    [SERDES_WAN_TYPE_AE]                    = {.name="AE"},
    [SERDES_WAN_TYPE_AE_10G]                = {.name="AE_10G"},
    [SERDES_WAN_TYPE_EPON_10G_SYM]          = {.name="EPON_10G_SYM"},
    [SERDES_WAN_TYPE_EPON_10G_ASYM]         = {.name="EPON_10G_ASYM"},
    [SERDES_WAN_TYPE_XGPON_10G_2_5G]        = {.name="XGPON_10G_2_5G"},
    [SERDES_WAN_TYPE_NGPON_10G_10G]         = {.name="NGPON_10G_10G"},
    [SERDES_WAN_TYPE_NGPON_10G_10G_8B10B]   = {.name="NGPON_10G_10G_8B10B"},
    [SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B]  = {.name="NGPON_10G_2_5G_8B10B"}
};
#ifdef USE_SERDES_LIB
static int serdes_reset_by_port_signals(serdes_wan_type_t wan_type)
{
    bdmf_error_t rc =0;
    misc_misc_0 misc_0;
    uint16_t misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode;
    uint16_t misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode;
    misc_misc_2 misc_2;
    misc_misc_3 misc_3;
    wan_serdes_pll_ctl pll_ctl;
    wan_serdes_pram_ctl pram_ctl;
    

    memset(&misc_0, 0, sizeof(misc_misc_0));
    //misc_0.cr_xgwan_top_wan_misc_refin_en = 1;
    //misc_0.cr_xgwan_top_wan_misc_refout_en = 1;
    ag_drv_misc_misc_0_set(&misc_0);
        
    misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode = 0;
    misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode = 0;
    ag_drv_misc_misc_1_set(misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode, misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode);


    memset(&misc_2, 0, sizeof(misc_2));
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_EPON_1G:
        misc_2.cr_xgwan_top_wan_misc_pmd_rx_osr_mode = 1;
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_EPON_2G:
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_EPON_10G_SYM:
        /* 0 */
        break;
    case SERDES_WAN_TYPE_GPON:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 2;
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 1;
        break;
    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 2;
        break;
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
        /* 0 */
        break;
    default:
        __logNotice("%s: OSR mode not specified!", wan_info[wan_type].name);
        break;
    }
    ag_drv_misc_misc_2_set(&misc_2);

   

    memset(&misc_3, 0, sizeof(misc_3));
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_EPON_1G:
    case SERDES_WAN_TYPE_EPON_2G:
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
        /* 0 */
        break;
    case SERDES_WAN_TYPE_EPON_10G_SYM:
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select = 1;
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_laser_mode = 1;
        break;
    case SERDES_WAN_TYPE_GPON:
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_laser_oe = BDMF_TRUE;
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select = 2;
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_laser_mode = 2;
        break;
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_laser_oe = BDMF_TRUE;
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_wan_interface_select = 3;
        misc_3.cr_xgwan_top_wan_misc_wan_cfg_laser_mode = 2;
        break;
   
    default:
        __logNotice("%s: wan_interface and laser_mode not specified!", wan_info[wan_type].name);
        break;
    }
    ag_drv_misc_misc_3_set(&misc_3);

   

    memset(&pll_ctl, 0, sizeof(pll_ctl));
    pll_ctl.cfg_pll1_refin_en = 1;
    pll_ctl.cfg_pll0_lcref_sel = 1;
    pll_ctl.cfg_pll0_refin_en = 1;
    ag_drv_wan_serdes_pll_ctl_set(&pll_ctl);
    
    

    memset(&pram_ctl, 0, sizeof(pram_ctl));
    ag_drv_wan_serdes_pram_ctl_set(&pram_ctl);

   

    return rc;
}

static int serdes_de_reset_by_port_signals(serdes_wan_type_t wan_type)
{
    bdmf_error_t rc =0;
    misc_misc_2 misc_2;

    memset(&misc_2, 0, sizeof(misc_2));
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_EPON_1G:
        misc_2.cr_xgwan_top_wan_misc_pmd_rx_osr_mode = 1;
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_EPON_2G:
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_EPON_10G_SYM:
        /* 0 */
        break;
    case SERDES_WAN_TYPE_GPON:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 2;
        misc_2.cr_xgwan_top_wan_misc_pmd_tx_osr_mode = 1;
        break;
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 1;
        break;
    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
        misc_2.cfgngponrxclk = 2;
        misc_2.cfgngpontxclk = 2;
        break;
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
        /* 0 */
        break;
    default:
        __logNotice("%s: OSR mode not specified!", wan_info[wan_type].name);
        break;
    }
    misc_2.cr_xgwan_top_wan_misc_pmd_por_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb = 1;
    ag_drv_misc_misc_2_set(&misc_2);

    return rc;
}




static err_code_t serdes_poll_uc_dsc_ready_for_cmd(uint32_t timeout_ms)
{
    int32_t loop;
    uint8_t ready_for_cmd;
    uint8_t error_found;
    uint8_t gp_uc_req;
    uint8_t supp_info;
    uint8_t state;

    /* read quickly for 10 tries */
    for (loop = 0; loop < 100; loop++)
    {
        ESTM(ready_for_cmd = rd_uc_dsc_ready_for_cmd());
        if (ready_for_cmd)
        {
            ESTM(error_found = rd_uc_dsc_error_found());
            if (error_found)
            {
                ESTM(gp_uc_req = rd_uc_dsc_gp_uc_req());
                ESTM(supp_info = rd_uc_dsc_supp_info());
                __logError("DSC command returned error (after cmd) cmd = x%x, supp_info = x%x!", gp_uc_req, supp_info);
                return(_error(ERR_CODE_UC_CMD_RETURN_ERROR));
            }
            return (ERR_CODE_NONE);
        }
        if (loop > 10)
            udelay(10*timeout_ms);
    }
    __logError("DSC ready for command is not working, applying workaround and getting debug info!");
    ESTM(gp_uc_req = rd_uc_dsc_gp_uc_req());
    ESTM(supp_info = rd_uc_dsc_supp_info());
    ESTM(state = rd_dsc_state());
    __logError("DSC: supp_info = x%x", supp_info);
    __logError("DSC: gp_uc_req = x%x", gp_uc_req);
    __logError("DSC: state = x%x", state);
    /* artifically terminate the command to re-enable the command interface */
    wr_uc_dsc_ready_for_cmd(0x1);
    return (_error(ERR_CODE_POLLING_TIMEOUT));
}

static err_code_t serdes_firmware_download(void)
{
    err_code_t err=ERR_CODE_NONE;

    __logInfo("TMP SKIP ...");
    return err;
    
    __logInfo("ucode_len=%d", serdes_ucode_len);

    __logInfo("A");
    serdes_pll_calibration(wan_serdes_type);

    /* Assert micro reset */
    wrc_core_s_rstb(0);
    wrc_core_s_rstb(1);
    
    __logInfo("B");
    serdes_pll_calibration(wan_serdes_type);
    
    eagle_onu10g_uc_reset(1);
    
    __logInfo("C");
    serdes_pll_calibration(wan_serdes_type);

    /* Load a new firmware. */
    err = eagle_onu10g_ucode_mdio_load(serdes_ucode, serdes_ucode_len);
    if (err != ERR_CODE_NONE)
    {
        __logError("Firmware download failed");
        return err;
    }
    __logInfo("Firmware download succeeded");

    __logInfo("Firmware verify ...");
    err = eagle_onu10g_ucode_load_verify(serdes_ucode, serdes_ucode_len);
    if (err != ERR_CODE_NONE)
    {
        __logError("Firmware verify failed");
        return err;
    }

    __logInfo("Firmware verify ok");

    __logInfo("D");
    serdes_pll_calibration(wan_serdes_type);
    
    /* Activate the firmware.*/
    eagle_onu10g_uc_active_enable(1);

    __logInfo("E");
    serdes_pll_calibration(wan_serdes_type);
    
    /* De-assert micro reset */
    eagle_onu10g_uc_reset(0);

    __logInfo("F");
    serdes_pll_calibration(wan_serdes_type);

    /* Wait until the firmware is executing (ready). */
    err = serdes_poll_uc_dsc_ready_for_cmd(FIRMWARE_READY_TIMEOUT_MS);
    if (err != ERR_CODE_NONE)
    {
        __logError("Firmware not ready!");
        return err;
    }

    __logInfo("Firmware is ready");

    __logInfo("G");
    serdes_pll_calibration(wan_serdes_type);
    

    return ERR_CODE_NONE;
}

static err_code_t serdes_config_pll(serdes_wan_type_t wan_type)
{
    err_code_t err;
    pll_cfg_t *pll_cfg_p;

    pll_cfg_p = &pll_cfg[wan_type];
    if (pll_cfg_p->tx_pll_id != pll_cfg_p->rx_pll_id)
    {
        EFUN(eagle_onu10g_set_pll(pll_cfg_p->tx_pll_id));
        EFUN(wrc_ams_pll_pwrdn(0));
        EFUN(wr_tx_pll_select(pll_cfg_p->tx_pll_id));
        err = eagle_onu10g_configure_pll(pll_cfg_p->tx_pll_cfg);
        if (err != ERR_CODE_NONE)
        {
            __logError("configure Tx pll failed" );
            return err;
        }

        EFUN(eagle_onu10g_set_pll(pll_cfg_p->rx_pll_id));
        EFUN(wrc_ams_pll_pwrdn(0));
        EFUN(wr_rx_pll_select(pll_cfg_p->rx_pll_id));
        err = eagle_onu10g_configure_pll(pll_cfg_p->rx_pll_cfg);
        if (err != ERR_CODE_NONE)
        {
            __logError("configure Rx pll failed" );
            return err;
        }
    }
    else
    {
        EFUN(eagle_onu10g_set_pll(pll_cfg_p->tx_pll_id));
        EFUN(wrc_ams_pll_pwrdn(0));
        EFUN(wr_tx_pll_select(pll_cfg_p->tx_pll_id));
        EFUN(wr_rx_pll_select(pll_cfg_p->tx_pll_id));        
        err = eagle_onu10g_configure_pll(pll_cfg_p->tx_pll_cfg);
        if (err != ERR_CODE_NONE)
        {
            __logError("configure pll failed" );
            return err;
        }
    }
    
    return ERR_CODE_NONE;
}

static err_code_t serdes_config_cdr_vga_pf(serdes_wan_type_t wan_type)
{
    int8_t vga, pf1, pf2;
    cdr_ctrl_t cdr_ctrl;

    /* config VGA, PF1, PF2. Same to all wan type */
    vga = 32;
    pf1 = 7;
    pf2 = 3;
    eagle_onu10g_write_rx_afe(RX_AFE_VGA, vga);
    eagle_onu10g_write_rx_afe(RX_AFE_PF, pf1);
    eagle_onu10g_write_rx_afe(RX_AFE_PF2, pf2);

    /* config CDR */
    memset(&cdr_ctrl, 0, sizeof(cdr_ctrl_t));
    cdr_ctrl.cdr_freq_en = 1;
    cdr_ctrl.cdr_freq_override_val = 0x690;
    cdr_ctrl.cdr_phase_sat_ctrl = 1;
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_EPON_10G_SYM:
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
    case SERDES_WAN_TYPE_AE_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
        cdr_ctrl.pattern_sel = 3;
        break;

    case SERDES_WAN_TYPE_EPON_2G:
    case SERDES_WAN_TYPE_EPON_1G:
    case SERDES_WAN_TYPE_GPON:
    case SERDES_WAN_TYPE_AE:
        cdr_ctrl.pattern_sel = 0x15;
        break;

    default:
        __logError("%s", wan_info[SERDES_WAN_TYPE_NONE].name);
        return ERR_CODE_INVALID_RAM_ADDR;
    }

    EFUN(wr_cdr_freq_en(cdr_ctrl.cdr_freq_en));
    EFUN(wr_osx2p_pherr_gain(cdr_ctrl.osx2p_pherr_gain));
    EFUN(wr_cdr_integ_sat_sel(cdr_ctrl.cdr_integ_sat_sel));
    EFUN(wr_cdr_bwsel_integ_acqcdr(cdr_ctrl.cdr_bwsel_integ_acqcdr));
    EFUN(wr_cdr_bwsel_integ_norm(cdr_ctrl.cdr_bwsel_integ_norm));
    EFUN(wr_cdr_bwsel_prop_acqcdr(cdr_ctrl.cdr_bwsel_prop_acqcdr));
    EFUN(wr_cdr_bwsel_prop_norm(cdr_ctrl.cdr_bwsel_prop_norm));
    EFUN(wr_cdr_freq_override_en(cdr_ctrl.cdr_freq_override_en));
    EFUN(wr_cdr_freq_override_val(cdr_ctrl.cdr_freq_override_val));
    EFUN(wr_phase_err_offset(cdr_ctrl.phase_err_offset));
    EFUN(wr_phase_err_offset_en(cdr_ctrl.phase_err_offset_en));
    EFUN(wr_cdr_phase_sat_ctrl(cdr_ctrl.cdr_phase_sat_ctrl));
    EFUN(wr_phase_err_offset_mult_2(cdr_ctrl.phase_err_offset_mult_2));
    EFUN(wr_pattern_sel(cdr_ctrl.pattern_sel));

    return ERR_CODE_NONE;
}


static err_code_t serdes_config_osr(serdes_wan_type_t wan_type)
{
    osr_ctrl_t osr_ctrl;

    memset(&osr_ctrl, 0, sizeof(osr_ctrl_t));
    osr_ctrl.rx_osr_mode_frc = 1;/*  Same to all wan type */

    if ((SERDES_WAN_TYPE_EPON_1G == wan_type) ||
        (SERDES_WAN_TYPE_AE == wan_type))
        osr_ctrl.rx_osr_mode_frc_val = 1;

    osr_ctrl.tx_osr_mode_frc = 1;/*  Same to all wan type */
    
    if ((SERDES_WAN_TYPE_EPON_10G_ASYM == wan_type) ||
        (SERDES_WAN_TYPE_EPON_2G == wan_type)       ||
        (SERDES_WAN_TYPE_EPON_1G == wan_type)       ||
        (SERDES_WAN_TYPE_GPON == wan_type)          ||
        (SERDES_WAN_TYPE_AE == wan_type))
        osr_ctrl.tx_osr_mode_frc_val = 1;

    EFUN(wr_rx_osr_mode_frc(osr_ctrl.rx_osr_mode_frc));
    EFUN(wr_rx_osr_mode_frc_val(osr_ctrl.rx_osr_mode_frc_val));
    EFUN(wr_tx_osr_mode_frc(osr_ctrl.tx_osr_mode_frc));
    EFUN(wr_tx_osr_mode_frc_val(osr_ctrl.tx_osr_mode_frc_val));

    return ERR_CODE_NONE;
}


static err_code_t serdes_config_lane_polarity(void)
{
    unsigned short optics_type;

    if (BpGetGponOpticsType(&optics_type) == BP_SUCCESS && optics_type == BP_GPON_OPTICS_TYPE_PMD)
    {
        EFUN(wr_rx_pmd_dp_invert(1));
        EFUN(wr_tx_pmd_dp_invert(1));
    }
    return ERR_CODE_NONE;
}

static err_code_t serdes_config_lbe(serdes_wan_type_t wan_type)
{
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_GPON:
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
        /* 4bit  */
        EFUN(wrc_tx_lbe4_0(1));
        EFUN(wr_ams_tx_wclk4_en(1));
        break;

    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
        /* 1bit  */
        EFUN(wrc_tx_lbe4_0(0));
        EFUN(wr_ams_tx_wclk4_en(0));
        break;

    default:
        __logInfo("%s: LBE bit: NA", wan_info[wan_type].name);
        return ERR_CODE_NONE;

    }
    return ERR_CODE_NONE;
}

static err_code_t serdes_config_pon_mac_clk_and_sync_e_clk(serdes_wan_type_t wan_type)
{
    if (wan_type >= sizeof(pon_mac_clk_and_sync_e_clk_cfg)/sizeof(pon_mac_clk_and_sync_e_clk_t))
    {
        __logError("%s", wan_info[SERDES_WAN_TYPE_NONE].name);
        return ERR_CODE_INVALID_RAM_ADDR;
    }
    else
    {
        EFUN(wr_ams_tx_sync_e_ctrl(pon_mac_clk_and_sync_e_clk_cfg[wan_type].sync_e_val));
        EFUN(wr_ams_rx_pon_mac_ctrl(pon_mac_clk_and_sync_e_clk_cfg[wan_type].rx_pon_mac_val));
        EFUN(wr_ams_tx_pon_mac_ctrl(pon_mac_clk_and_sync_e_clk_cfg[wan_type].tx_pon_mac_val));
    } 
    return ERR_CODE_NONE;
}

static err_code_t serdes_check_pll_lock(pll_id_t pll_inst, int poll_cnt)
{
    uint8_t poll_lock = 0;
    int poll_lock_cnt = 0;

    EFUN(eagle_onu10g_set_pll(pll_inst));

    while((poll_lock == 0) && (poll_lock_cnt < poll_cnt))
    {
        ESTM(poll_lock = rdc_pll_lock());
        if (poll_lock)
        {
            __logInfo("PLL%d locked!", pll_inst);
            break;
        }
        udelay(10);
        poll_lock_cnt++;
    }

    if (!poll_lock)
    {
        __logError("PLL%d unlock, poll_cnt=%d", pll_inst, poll_cnt);
    }

    EFUN(eagle_onu10g_set_pll(0));
    
    return ERR_CODE_NONE;
}

static err_code_t serdes_check_wan_pll_lock(pll_id_t pll_inst)
{
    int poll_lock_cnt = 0;
    serdes_status_status stat;

    udelay(50);
    
    while(poll_lock_cnt < 10)
    {
        ag_drv_serdes_status_status_get(&stat);
        if (((PLL_0 == pll_inst) && stat.pmd_pll0_lock) || 
            ((PLL_1 == pll_inst) && stat.pmd_pll1_lock))
        {
            __logInfo("Wan Top PLL%d locked!", pll_inst);
            break;
        }

        poll_lock_cnt++;
        udelay(10);
    }

    if (poll_lock_cnt == 10)
    {
        __logError("Wan Top PLL%d unlock!", pll_inst);
    }

    return ERR_CODE_NONE;
}

static err_code_t serdes_pll_calibration(serdes_wan_type_t wan_type)
{
    pll_cfg_t *pll_cfg_p;

    pll_cfg_p = &pll_cfg[wan_type];
    if (pll_cfg_p->tx_pll_id != pll_cfg_p->rx_pll_id)
    {    
        EFUN(eagle_onu10g_set_pll(pll_cfg_p->tx_pll_id));
        EFUN(wrc_core_dp_s_rstb(1));

        EFUN(eagle_onu10g_set_pll(pll_cfg_p->rx_pll_id));
        EFUN(wrc_core_dp_s_rstb(1));
    }
    else
    {
        EFUN(eagle_onu10g_set_pll(pll_cfg_p->tx_pll_id));
        EFUN(wrc_core_dp_s_rstb(1));
    }
        
        EFUN(serdes_check_pll_lock(pll_cfg_p->tx_pll_id, 100));
    if (pll_cfg_p->tx_pll_id != pll_cfg_p->rx_pll_id)
    {
        EFUN(serdes_check_pll_lock(pll_cfg_p->rx_pll_id, 100));
    }
    EFUN(serdes_check_wan_pll_lock(pll_cfg_p->tx_pll_id)); 

    return ERR_CODE_NONE;
}

static err_code_t serdes_deactive_lane_soft_reset (void)
{
    EFUN(wr_ln_dp_s_rstb(1));
    EFUN(wr_ams_tx_ticksel(0x00));
    udelay(2);
    
    return ERR_CODE_NONE;
}

static err_code_t serdes_check_pmd_rx_lock_state(int32_t poll_cnt_max)
{

    err_code_t err;
    uint8_t pmd_rx_lock=0;
    uint8_t rx_dsc_lock=0;
    int32_t poll_cnt=0;


    serdes_status_status status;
    
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    udelay(1000);
    ag_drv_serdes_status_status_get(&status);
    mdelay(1000);
    ag_drv_serdes_status_status_get(&status);
    mdelay(1000);
    ag_drv_serdes_status_status_get(&status);
    mdelay(1000);


    while ((!rx_dsc_lock) && (poll_cnt < poll_cnt_max))
    {
        ESTM(rx_dsc_lock = rd_rx_dsc_lock());
        poll_cnt++;
        udelay(50);
    }
    if (0 == rx_dsc_lock)
    {
        __logError("rx_dsc_lock unlocked! poll_cnt=%d", poll_cnt);
        err = ERR_CODE_INVALID_RAM_ADDR;
    }
    else
    {
        __logInfo("rx_dsc_lock locked!");
    }

    while ((!pmd_rx_lock) && (poll_cnt < poll_cnt_max))
    {
        eagle_onu10g_pmd_lock_status(&pmd_rx_lock);
        poll_cnt++;
        udelay(50);
    }
    if (0 == pmd_rx_lock)
    {
        __logError("pmd_rx_lock unlocked! poll_cnt=%d", poll_cnt);
        err = ERR_CODE_INVALID_RAM_ADDR;
    }
    else
    {
        __logInfo("pmd_rx_lock locked!");
    }

    return err;

    return ERR_CODE_NONE;
}

static void wan_init(void)
{
    misc_misc_2 misc_2;
    uint32_t scratch = 0;
    misc_misc_0 misc_0;
    uint16_t misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode;
    uint16_t misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode;
    misc_misc_3 misc_3;
    wan_serdes_pll_ctl pll_ctl;
    wan_serdes_pram_ctl pram_ctl;
    

    //set misc to 0x3e
    memset(&misc_2, 0, sizeof(misc_2));
    misc_2.cr_xgwan_top_wan_misc_pmd_por_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb = 1;
    ag_drv_misc_misc_2_set(&misc_2);
    ag_drv_misc_misc_2_get(&misc_2);

    //set misc to 0x0
    memset(&misc_2, 0, sizeof(misc_2));
    ag_drv_misc_misc_2_set(&misc_2);
    ag_drv_misc_misc_2_get(&misc_2);

    //set misc to 0x3e
    memset(&misc_2, 0, sizeof(misc_2));
    misc_2.cr_xgwan_top_wan_misc_pmd_por_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_0_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_core_1_dp_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_h_rstb = 1;
    misc_2.cr_xgwan_top_wan_misc_pmd_ln_dp_h_rstb = 1;
    ag_drv_misc_misc_2_set(&misc_2);
    ag_drv_misc_misc_2_get(&misc_2);

    scratch = 0x01234567;
    ag_drv_top_scratch_scratch_set(scratch);
    ag_drv_top_scratch_scratch_get(&scratch);
    
    scratch = 0x89abcdef;
    ag_drv_top_scratch_scratch_set(scratch);
    ag_drv_top_scratch_scratch_get(&scratch);

    memset(&misc_0, 0, sizeof(misc_misc_0));
    ag_drv_misc_misc_0_set(&misc_0);
    memset(&misc_0, 0, sizeof(misc_misc_0));
    ag_drv_misc_misc_0_get(&misc_0);
        
    misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode = 0;
    misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode = 0;
    ag_drv_misc_misc_1_set(misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode, misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode);
    ag_drv_misc_misc_1_get(&misc_1_cr_xgwan_top_wan_misc_pmd_core_1_mode, &misc_1_cr_xgwan_top_wan_misc_pmd_core_0_mode);

    memset(&misc_2, 0, sizeof(misc_2));
    ag_drv_misc_misc_2_set(&misc_2);
    ag_drv_misc_misc_2_get(&misc_2);

    memset(&misc_3, 0, sizeof(misc_3));
    ag_drv_misc_misc_3_set(&misc_3);
    ag_drv_misc_misc_3_get(&misc_3);

    memset(&pll_ctl, 0, sizeof(pll_ctl));
    ag_drv_wan_serdes_pll_ctl_set(&pll_ctl);
    ag_drv_wan_serdes_pll_ctl_get(&pll_ctl);

    memset(&pram_ctl, 0, sizeof(pram_ctl));
    ag_drv_wan_serdes_pram_ctl_set(&pram_ctl);
    ag_drv_wan_serdes_pram_ctl_get(&pram_ctl);

    ag_drv_pmi_lp_0_set(0x00, 0x00);
    ag_drv_pmi_lp_1_set(0x00000000);
    ag_drv_pmi_lp_2_set(0x0000, 0x0000);
    
    
}

static void serdes_init(serdes_wan_type_t wan_type)
{
    serdes_register_access_test();/* AAAAAA: Test SerDes register access.    To be remove!!  only for bring up */
    
    /* Step 1. Assert POR reset by forcing pmd_por_h_rstb pin to 1'b0. Optionally if out of POR then core_s_rstb can be asserted by writing to 1'b0 to reset the whole core. */
    __logInfo("Step1");
    serdes_reset_by_port_signals(wan_type);
    //eagle_onu10g_core_pwrdn(PWRDN_DEEP);

    /* Step 2. Wait for stable refclk. */
    __logInfo("Step2");
    udelay(100);

    /* Step 3. De-assert pmd_por_h_rstb pin OR core_s_rstb register (only required if this register was manually written to 1'b0. Out of POR de-assertion, core_s_rstb is 1'b1) by making them 1'b1. */
    __logInfo("Step3");
    serdes_de_reset_by_port_signals(wan_type);
    udelay(10);
    //eagle_onu10g_core_pwrdn(PWR_ON);

    /* Step 4. Setup 2 PLL and 2 PLL calibration (common registers) */
    __logInfo("Step4");
    /* PLL configuration */
    serdes_config_pll(wan_type);

    /* Step 5. Speed up simulation for TX disable */
    /* only applicable for simulation, Skip */

    /* Step 6. For RX OS2 mode, the RX PI spacing needs to be adjusted as follows */
    /*  Done by the Serdes FW, Skip */

    /* Step 6A. CDR, VGA, and PF programming */
    __logInfo("Step6A");
    serdes_config_cdr_vga_pf(wan_type);

    /* Step 7. Set the over-sample mode ( per lane ) */
    __logInfo("Step7");
    serdes_config_osr(wan_type);

    /* Step 8. Set the lane polarity ( per lane )       // optional */
    __logInfo("Step8");
    serdes_config_lane_polarity();

    /* Step 9. Speed up values for pmd_rx_lock (only applicable for simulation to speed up PMD_LOCK time) */
    /* only applicable for simulation, Skip */

    /* Step 10. LBE set up. ( 4bit or 1bit LBE) */
    __logInfo("Step10");
    serdes_config_lbe(wan_type);

    /* Step 11. pon_mac_clk and sync_e_clk */
    __logInfo("Step11");
    serdes_config_pon_mac_clk_and_sync_e_clk(wan_type);

    /* Step 12. Set Tx FIR and amplitude settings ( per lane) (Optional) */
    /*  +++ open +++ */

    /* Step 13. De-assert core_dp_s_rstb bit which will start the PLL calibration */
    __logInfo("Step13");
    serdes_pll_calibration(wan_type);

    /* Step 14. De-assert ln_dp_s_rstb bit (per lane) */
    __logInfo("Step14");
    serdes_deactive_lane_soft_reset();

    /* Step 15. Check for pmd_rx_lock state */
    __logInfo("Step15");
    serdes_check_pmd_rx_lock_state(200);

    /* Load & verify microcode */
    __logInfo("Load microcode ...");
    serdes_firmware_download();

    return;
}

static int epon_wan_top_gearbox_config (serdes_wan_type_t wan_type)
{
    bdmf_error_t rc = 0;

    if ((SERDES_WAN_TYPE_EPON_10G_SYM == wan_type) || (SERDES_WAN_TYPE_EPON_10G_ASYM == wan_type))
    {
        ten_g_gearbox_gearbox ten_g_gearbox;

        memset(&ten_g_gearbox, 0, sizeof(ten_g_gearbox_gearbox));
        ten_g_gearbox.cfg_sgb_pon_10g_epon_clk_en = 1;
        ten_g_gearbox.cfg_sgb_pon_10g_epon_tx_gbox_rstn = 1;
        ten_g_gearbox.cfg_sgb_pon_10g_epon_rx_gbox_rstn = 1;
        ten_g_gearbox.cfg_sgb_pon_10g_epon_tx_cgen_rstn = 1;
        ten_g_gearbox.cfg_sgb_pon_10g_epon_rx_cgen_rstn = 1;

        rc = ag_drv_ten_g_gearbox_gearbox_set(&ten_g_gearbox);
        if (rc != BDMF_ERR_OK)
        {
            __logError("Cannot write ten_g_gearbox_gearbox rc=%d", rc);
            return rc ;
        }
    }
    return rc;
}

static int gpon_ngpon_wan_top_gearbox_config (serdes_wan_type_t wan_type) 
{
  bdmf_error_t            rc ;
  ngpon_gearbox_rx_ctl_0  rx_ctl_0 ;
  ngpon_gearbox_tx_ctl    tx_ctl ;
  gpon_gearbox_0          gearbox_0 ;

  uint8_t config_burst_delay_cyc ;
  uint8_t fifo_cfg_1_tx_wr_pointer ;
  uint8_t fifo_cfg_1_tx_rd_pointer ;


  rc = ag_drv_ngpon_gearbox_rx_ctl_0_get (&rx_ctl_0) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot read ngpon_gearbox_rx_ctl_0 rc=%d", rc);
    return rc ;
  }

  if ((wan_type == SERDES_WAN_TYPE_NGPON_10G_10G_8B10B) ||
      (wan_type == SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B))
  {
     rx_ctl_0.cfngpongboxrxfrchunt = 1  ;
     rx_ctl_0.cfngpongboxrxoutdataflip = 1 ;
  }
  else
  {
     rx_ctl_0.cfngpongboxrxfrchunt = 0  ;
     rx_ctl_0.cfngpongboxrxoutdataflip = 0 ;
  }

  rc = ag_drv_ngpon_gearbox_rx_ctl_0_set (&rx_ctl_0) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_rx_ctl_0 rc=%d", rc) ;
    return rc ;
  }

  rc = ag_drv_ngpon_gearbox_tx_ctl_get (&tx_ctl) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot read ngpon_gearbox_tx_ctl rc=%d", rc) ;
    return rc ;
  }

  tx_ctl.cfngpongboxtxen = 1;
  rc = ag_drv_ngpon_gearbox_tx_ctl_set (&tx_ctl) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_tx_ctl rc=%d", rc) ;
    return rc ;
  }

  if (wan_type == SERDES_WAN_TYPE_GPON)
  {
    rc = ag_drv_gpon_gearbox_0_get (&gearbox_0) ;
    if (rc != BDMF_ERR_OK)
    {  
       __logError("Cannot read gpon_gearbox_0 rc=%d", rc) ;
       return rc ;
     }
    gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_rx_16bit_order = 1 ;
    gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_8bit_order  = 0 ;
    gearbox_0.cr_xgwan_top_wan_misc_gpon_gearbox_fifo_cfg_0_tx_16bit_order = 1;

    rc = ag_drv_gpon_gearbox_0_set (&gearbox_0) ;
    if (rc != BDMF_ERR_OK)
    {  
       __logError("Cannot write gpon_gearbox_0 rc=%d", rc) ;
       return rc ;
     }
  
    rc = ag_drv_gpon_gearbox_2_get(&config_burst_delay_cyc, 
                                   &fifo_cfg_1_tx_wr_pointer, 
                                   &fifo_cfg_1_tx_rd_pointer) ;
    if (rc != BDMF_ERR_OK)
    {  
       __logError("Cannot read gearbox_2 rc=%d", rc) ;
       return rc ;
    }
    config_burst_delay_cyc = 3 ;
    rc = ag_drv_gpon_gearbox_2_set(config_burst_delay_cyc, 
                                   fifo_cfg_1_tx_wr_pointer, 
                                   fifo_cfg_1_tx_rd_pointer) ;
    if (rc != BDMF_ERR_OK)
    {  
       __logError("Cannot write gearbox_2 rc=%d", rc) ;
       return rc ;
    }
  }
    /*
     *   Rx FIFO control
     */
  rc = ag_drv_ngpon_gearbox_rx_ctl_0_get (&rx_ctl_0) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot read ngpon_gearbox_rx_ctl_0 rc=%d", rc);
    return rc ;
  }

  if ((wan_type == SERDES_WAN_TYPE_NGPON_10G_10G_8B10B) ||
      (wan_type == SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B))
  {
     rx_ctl_0.cfngpongboxrxfifordptr = 8  ;
     rx_ctl_0.cfngpongboxrxfifoptrld = 1 ;
     rx_ctl_0.cfngpongboxrxserdataflip = 0 ;
     rx_ctl_0.cfngpongboxrxmode = 0 ;
  }
  else
  {
     rx_ctl_0.cfngpongboxrxfifordptr = 8  ;
     rx_ctl_0.cfngpongboxrxfifoptrld = 1 ;
     rx_ctl_0.cfngpongboxrxserdataflip = 1 ;
     rx_ctl_0.cfngpongboxrxmode = 1 ;
  }

  rc = ag_drv_ngpon_gearbox_rx_ctl_0_set (&rx_ctl_0) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_rx_ctl_0 rc=%d", rc) ;
    return rc ;
  }

    /*
     *   Tx FIFO control
     */

  rc = ag_drv_ngpon_gearbox_tx_ctl_get (&tx_ctl) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot read ngpon_gearbox_tx_ctl rc=%d", rc) ;
    return rc ;
  }

  tx_ctl.cfngpongboxtxfifoptrld = 1 ;
  tx_ctl.cfngpongboxtxfifovldptrld = 1 ;
  tx_ctl.cfngpongboxtxfifovldoff = 0x1F ;

  rc = ag_drv_ngpon_gearbox_tx_ctl_set (&tx_ctl) ;
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_tx_ctl rc=%d", rc) ;
    return rc ;
  }

  return BDMF_ERR_OK ;
}

static void gpon_ngpon_pinmux_config (void)
{
  /*
      pChipEnv->pinmux->enable_TX_LASER_ON_OUT_P();

      set in wan_burst_config()
  */
  /*  
    pChipEnv->pinmux->enable_TX_LASER_ON_OUT_N();
  */
    bcm_set_pinmux(TX_LASER_ON_OUT_N,6);
  /*
    pChipEnv->pinmux->enable_A_WAN_EARLY_TXEN();
  */
    bcm_set_pinmux(A_WAN_EARLY_TXEN,6);

  /*
    pChipEnv->pinmux->enable_B_WAN_EARLY_TXEN();
  */
    bcm_set_pinmux(B_WAN_EARLY_TXEN,6);
}

static int active_eth_wan_top_gearbox_config (serdes_wan_type_t wan_type)
{
  /*
      To be defined
  */
  return 0 ;
}

static int gearbox_config(serdes_wan_type_t wan_type)
{
    bdmf_error_t rc = BDMF_ERR_OK;

    switch (wan_type) 
    {
    case SERDES_WAN_TYPE_AE:
      rc =  active_eth_wan_top_gearbox_config (wan_type) ;
      break ;

    /* XGPON/NGPON types */
    case SERDES_WAN_TYPE_EPON_1G:
    case SERDES_WAN_TYPE_AE_10G:
    case SERDES_WAN_TYPE_EPON_10G_SYM:
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
      rc = epon_wan_top_gearbox_config (wan_type) ;
      break;

    case SERDES_WAN_TYPE_GPON:
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
    case SERDES_WAN_TYPE_NGPON_10G_10G:
    case SERDES_WAN_TYPE_NGPON_10G_10G_8B10B:
    case SERDES_WAN_TYPE_NGPON_10G_2_5G_8B10B:
      rc = gpon_ngpon_wan_top_gearbox_config (wan_type) ;
      gpon_ngpon_pinmux_config () ;
      break ;

    case SERDES_WAN_TYPE_NONE:
    default:
      __logError("Unsupported WAN type wan_type=%d", wan_type) ;
    }
    return rc;
}
#endif


int rdp_post_init_fiber(void)
{
    unsigned short tx_gpio;

    /* Configure PON GPIO */
    if (BpGetPonTxEnGpio(&tx_gpio) == BP_SUCCESS)
    {
        kerSysSetGpioDir(tx_gpio & BP_GPIO_NUM_MASK);
        kerSysSetGpioState((tx_gpio & BP_GPIO_NUM_MASK), (tx_gpio & BP_ACTIVE_LOW) ? kGpioInactive : kGpioActive);
    }

    return 0;
}

#define LASER_TX_CHN_CFG    0x70
#define LASER_RX_CHN_CFG    0x71
#define LASER_PASS_BASE     0x7B

#define LASER_TX_POWER_1    0x66
#define LASER_TX_POWER_2    0x67

#define LASER_TX_BIAS_1     0x64
#define LASER_TX_BIAS_2     0x65


#define MAX_I2C_MSG_SIZE                    64 /* arbitrary limit */

void wan_enable_ngpon2_tunable_laser(void)
{
   uint8_t laser_pass[4] = {0x12, 0x34, 0x56, 0x78};
   int i;
   int rc;
   uint8_t i2c_byte ;


   /*
    *  1. Set password
    */
   for (i=0; i<sizeof(laser_pass); i++)
   {
       rc = gponPhy_write_byte(0, LASER_PASS_BASE + i, laser_pass[i]);
       if (rc < 0)
       {
          __logError("Failed to write value 0x%02x to addr 0x%02x", laser_pass[i], LASER_PASS_BASE + i) ;
       }
   }

   for (i=0; i<sizeof(laser_pass); i++)
   {
      i2c_byte = gponPhy_read_byte(0, LASER_PASS_BASE + i);
      __logError(">>>> Read 0x%02x from addr 0x%02x", i2c_byte, LASER_PASS_BASE + i) ;
   }
   /*
    * 2. Set Rx wavelength
    */
   rc = gponPhy_write_byte(0, LASER_RX_CHN_CFG, 1);
   if (rc < 0)
   {
      __logError("Failed to write value 0x%02x to addr 0x%02x", 1, LASER_RX_CHN_CFG) ;
   }

   i2c_byte = gponPhy_read_byte(0, LASER_RX_CHN_CFG);
   __logError(">>>> Read 0x%02x from addr 0x%02x", i2c_byte, LASER_RX_CHN_CFG) ;

   /*
    *  3. Set password
    */
   for (i=0; i<sizeof(laser_pass); i++)
   {
      rc = gponPhy_write_byte(0, LASER_PASS_BASE + i, laser_pass[i]);
      if (rc < 0)
      {
         __logError("Failed to write value 0x%02x to addr 0x%02x", laser_pass[i], LASER_PASS_BASE + i) ;
      }
   }
   for (i=0; i<sizeof(laser_pass); i++)
   {
      i2c_byte = gponPhy_read_byte(0, LASER_PASS_BASE + i);
      __logError(">>>> Read 0x%02x from addr 0x%02x", i2c_byte, LASER_PASS_BASE + i) ;
   }
   /*
    * 4. Set Tx wavelength
    */

   rc = gponPhy_write_byte(0, LASER_TX_CHN_CFG, 1);
   if (rc < 0)
   {
      __logError("Failed to write value 0x%02x to addr 0x%02x", 1, LASER_TX_CHN_CFG) ;
   }
   i2c_byte = gponPhy_read_byte(0, LASER_TX_CHN_CFG);
   __logInfo(">>>> Read 0x%02x from addr 0x%02x", i2c_byte, LASER_TX_CHN_CFG) ;
   /*
    * 5.  Read 10G Tx_Power
    */
   i2c_byte = gponPhy_read_byte(0, LASER_TX_POWER_1);
   __logInfo(">>>> Read 0x%02x from addr 0x%02x (10G Tx Power first byte)", i2c_byte, LASER_TX_POWER_1) ;

   i2c_byte = gponPhy_read_byte(0, LASER_TX_POWER_2);
   __logInfo(">>>> Read 0x%02x from addr 0x%02x  (10G Tx Power second byte)", i2c_byte, LASER_TX_POWER_2) ;
     
   /*
    * 6.  Read 10G Tx_Bias
    */
   i2c_byte = gponPhy_read_byte(0, LASER_TX_BIAS_1);
   __logInfo(">>>> Read 0x%02x from addr 0x%02x (10G Tx Bias first byte)", i2c_byte, LASER_TX_BIAS_1) ;

   i2c_byte = gponPhy_read_byte(0, LASER_TX_BIAS_2);
   __logInfo(">>>> Read 0x%02x from addr 0x%02x  (10G Tx Bias second byte)", i2c_byte, LASER_TX_BIAS_2) ;

}

int wan_serdes_config(serdes_wan_type_t wan_type)
{
    __logLevelSet(BCM_LOG_LEVEL_INFO);

    if (wan_type >= sizeof(wan_info)/sizeof(wan_info_t))
    {
        __logError("%s", wan_info[SERDES_WAN_TYPE_NONE].name);
        return ERR_CODE_INVALID_RAM_ADDR;
    }
    __logInfo("Wan type: %s", wan_info[wan_type].name);

    /* ioRemap virtual addresses of WAN */
    remap_ru_block_addrs(WAN_IDX, WAN_TOP_BLOCKS);

    
    if ((wan_type != SERDES_WAN_TYPE_EPON_1G) && 
         (wan_type != SERDES_WAN_TYPE_EPON_2G ) &&
         (wan_type != SERDES_WAN_TYPE_EPON_10G_ASYM) &&
         (wan_type != SERDES_WAN_TYPE_EPON_10G_SYM))
        { /* EPON Serdes init is using scripts temporally */
        if (wan_power_on() != 0)
            return -1;
        }
    else
    {
        if (wan_epon_zone_enable() !=0 )
            return -1;
    }

    serdes_debug_init();
    rdp_post_init_fiber();
#ifdef USE_SERDES_LIB
    wan_init();
    gearbox_config(wan_type);
    serdes_init(wan_type);
    wan_burst_config(wan_type);
#else
    switch (wan_type)
    {
    case SERDES_WAN_TYPE_GPON:
        gpon_wan_init();
        break;       
    case SERDES_WAN_TYPE_EPON_1G:
         epon_1g1g_wan_serdes_init();
        break;
    case SERDES_WAN_TYPE_EPON_2G:
        epon_2g1g_wan_serdes_init(); 
        break;
    case SERDES_WAN_TYPE_EPON_10G_ASYM:
        epon_10g1g_wan_serdes_init(); 
        break;
    case SERDES_WAN_TYPE_EPON_10G_SYM:
        epon_10g10g_wan_serdes_init(); 
        break;
    case SERDES_WAN_TYPE_XGPON_10G_2_5G:
        xgpon_wan_init();
        break;       
    case SERDES_WAN_TYPE_NGPON_10G_10G:
        wan_enable_ngpon2_tunable_laser();
        ngpon_10g10g_wan_serdes_init();
        break;       
        
    default:
        __logInfo("Wan type: %s -- not supported yet", wan_info[wan_type].name);
    }
#endif
    wan_serdes_type = wan_type;

    return 0;
}
EXPORT_SYMBOL(wan_serdes_config);

serdes_wan_type_t wan_serdes_type_get(void)
{
    return wan_serdes_type;
}
EXPORT_SYMBOL(wan_serdes_type_get);

void wan_register_pmd_prbs_callback(PMD_DEV_ENABLE_PRBS callback)
{
    pmd_prbs_callback = callback;
}
EXPORT_SYMBOL(wan_register_pmd_prbs_callback);



void wan_prbs_status(bdmf_boolean *valid, uint32_t *errors)
{
    err_code_t err;
    uint8_t lock_lost;
    uint8_t lock;

    /* Check for PRBS lock */
    err= eagle_onu10g_prbs_chk_lock_state(&lock);
    if (ERR_CODE_NONE != err)
    {
        __logError("Get prbs_chk_lock_state failed! err=%d", err);
        return;
    }

    /* Check for PRBS lock lost */
    err= eagle_onu10g_prbs_err_count_state(errors, &lock_lost);
    if (ERR_CODE_NONE != err)
    {
        __logError("Get prbs_err_count_state failed! err=%d", err);
        return;
    }

    if (lock && !lock_lost && *errors == 0)
        *valid = 1;
    else
        *valid = 0;
}
EXPORT_SYMBOL(wan_prbs_status);

void wan_prbs_gen(uint32_t enable, int enable_host_tracking, int mode, serdes_wan_type_t wan_type, bdmf_boolean *valid)
{
    enum srds_prbs_polynomial_enum prbs_poly_mode;
    enum srds_prbs_checker_mode_enum prbs_checker_mode;
    uint8_t prbs_inv;
    uint32_t errors;
    uint16_t transceiver;
    unsigned short polarity = 0;

    *valid = 0;
    BpGetGponOpticsType(&transceiver);
    if (transceiver == BP_GPON_OPTICS_TYPE_PMD && enable)
    {
        if (pmd_prbs_callback)
            pmd_prbs_callback((uint16_t)enable, 1);
    } 

    if (enable)
    {
        if ((SERDES_WAN_TYPE_AE == wan_type) || (SERDES_WAN_TYPE_AE_10G == wan_type))
        {
            if (BpGetAePolarity(&polarity) != BP_SUCCESS)
            {
                __logError("BpGetAePolarity failed!");
                return;
            }
        }        

        polarity = 1;
    }

    wrc_tx_laser_burst_polarity_inv_0(polarity);
    
    
    if (!enable)
    {
        /* Disable PRBS */
        eagle_onu10g_tx_prbs_en(0);
        eagle_onu10g_rx_prbs_en(0);

        if (transceiver == BP_GPON_OPTICS_TYPE_PMD)
        {
            if (pmd_prbs_callback)
                pmd_prbs_callback((uint16_t)enable, 1);
        }
        return;
    }
    else
    {
        if (mode == 0) /* PRBS 7 */
            prbs_poly_mode = PRBS_7;
        else if (mode == 1) /* PRBS 15 */
            prbs_poly_mode = PRBS_15;
        else if (mode == 2) /* PRBS 23 */
            prbs_poly_mode = PRBS_23;
        else if (mode == 3) /* PRBS 31 */
            prbs_poly_mode = PRBS_31;
        else
        {
            __logError("wan_prbs_gen unknown mode %d", mode);
            return;
        }

        /* config Generator and checker */
        prbs_inv = 0;
        prbs_checker_mode = PRBS_SELF_SYNC_HYSTERESIS;
        eagle_onu10g_config_tx_prbs(prbs_poly_mode, prbs_inv);
        eagle_onu10g_config_rx_prbs(prbs_poly_mode, prbs_checker_mode, prbs_inv);

        /* Enable PRBS Generator*/
        eagle_onu10g_tx_prbs_en(1);

        /* Enable PRBS checker */
        eagle_onu10g_rx_prbs_en(1);
        wan_prbs_status(valid, &errors); /* Clear counters */
        udelay(100);

        wan_prbs_status(valid, &errors);
    }
}
EXPORT_SYMBOL(wan_prbs_gen);

int ngpon_wan_post_init (uint32_t sync_frame_length)
{
  bdmf_error_t rc = 0;
  ngpon_gearbox_tx_ctl tx_ctl;

  rc = ag_drv_ngpon_gearbox_tx_ctl_get(&tx_ctl);
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot read ngpon_gearbox_tx_ctl rc=%d", rc) ;
  }


  /*
   * Disable Gearbox Tx
   */
  tx_ctl.cfngpongboxtxen = 0;

  rc = ag_drv_ngpon_gearbox_tx_ctl_set(&tx_ctl);
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_tx_ctl rc=%d", rc) ;
  }

  udelay(10); 

  /*
   * Re-enable Gearbox Tx thus syncing the FIFO
   */
  tx_ctl.cfngpongboxtxen = 1;

  rc = ag_drv_ngpon_gearbox_tx_ctl_set(&tx_ctl);
  if (rc != BDMF_ERR_OK)
  {  
    __logError("Cannot write ngpon_gearbox_tx_ctl rc=%d", rc) ;
  }
  return (int) rc ;
 
}
EXPORT_SYMBOL(ngpon_wan_post_init);

static int wan_power_on(void)
{
    int ret;

    ret = PowerOffDevice(PMB_ADDR_WAN, 0);
    if (ret != 0)
        __logError("Failed to PowerOffDevice PMB_ADDR_WAN block");
    else
        udelay(100);

    ret = PowerOnDevice(PMB_ADDR_WAN);
    if (ret != 0)
        __logError("Failed to PowerOnDevice PMB_ADDR_WAN block");
    else
        udelay(100);

    return ret;
}

static int wan_epon_zone_enable(void)
{
    /* fix EPON Block not work on new 6858 A0 chip with OTP flash */
    int ret;

    ret = PowerOnZone(PMB_ADDR_WAN, 0); /* WAN top level and common */
    (0  == ret) ? ret=PowerOnZone(PMB_ADDR_WAN, 4) : ret; /* 1G EPON */
    (0  == ret) ? ret=PowerOnZone(PMB_ADDR_WAN, 5) : ret; /* 10G EPON */
    (0  == ret) ? ret=PowerOnZone(PMB_ADDR_WAN, 6) : ret; /* Common EPON */
    return ret;
}

#if 0
static void wan_burst_config(serdes_wan_type_t wan_type)
{
    if ((SERDES_WAN_TYPE_AE == wan_type) || (SERDES_WAN_TYPE_AE_10G == wan_type))
    {
        unsigned short polarity;

        bcm_set_pinmux(TX_LASER_ON_OUT_P,5);
        bcm_gpio_set_dir(TX_LASER_ON_OUT_P,1);
        if (BpGetAePolarity(&polarity) == BP_SUCCESS)
            bcm_gpio_set_data(TX_LASER_ON_OUT_P, polarity);
    }
    else
    {
        bcm_set_pinmux(TX_LASER_ON_OUT_P,6);
    }
}
#endif

void wan_top_tod_ts48_get(uint16_t *ts48_msb, uint32_t *ts48_lsb)
{
    tod_config_0 config_0 = {} ;
    /*
     * 1. Set the interface and lock the timestamp value
     */

    /*
     * cfg_ts48_mac_select: this field selects the MAC that the timestamp comes from.
     *   0: EPON
     *   1: 10G EPON
     *   2: GPON
     *   3: NGPON
     *   4: Active Ethernet
     *   5-7: Reserved
     *   cfg_ts48_mac_selectReset value is 0x0.
     */
    switch (wan_serdes_type)
    {
       case SERDES_WAN_TYPE_GPON:
       case SERDES_WAN_TYPE_NONE:
           config_0.cfg_ts48_mac_select = 0x2;
           break;       

       case SERDES_WAN_TYPE_EPON_1G:
       case SERDES_WAN_TYPE_EPON_2G:
           config_0.cfg_ts48_mac_select = 0x0;
           break;       

       case SERDES_WAN_TYPE_EPON_10G_ASYM:
       case SERDES_WAN_TYPE_EPON_10G_SYM:
           config_0.cfg_ts48_mac_select = 0x1;
           break;       

       case SERDES_WAN_TYPE_XGPON_10G_2_5G:
       case SERDES_WAN_TYPE_NGPON_10G_10G:
       case SERDES_WAN_TYPE_NGPON_10G_2_5G:
           config_0.cfg_ts48_mac_select = 0x3;
           break;

       case SERDES_WAN_TYPE_AE:
       case SERDES_WAN_TYPE_AE_10G:
           config_0.cfg_ts48_mac_select = 0x4;
           break;       

       default:
           config_0.cfg_ts48_mac_select = 0x0;
           break;                  
    }
    config_0.cfg_ts48_enable = 1;
    config_0.cfg_ts48_read = 1;

    ag_drv_tod_config_0_set(&config_0);
    /*
     * 2. Read timestamp
     */
    ag_drv_tod_msb_get(ts48_msb);
    ag_drv_tod_lsb_get(ts48_lsb);

    /*
     * 3. Unlock
     */
    config_0.cfg_ts48_mac_select = 0x0; /* NGPON MAC */
    config_0.cfg_ts48_enable = 0;
    config_0.cfg_ts48_read = 0;
    ag_drv_tod_config_0_set(&config_0);
}
EXPORT_SYMBOL(wan_top_tod_ts48_get);

/*
 *  enabled = 0 - disable transmitter
 *  enabled = 1 - enable it
 */
void ngpon_wan_top_enable_transmitter(int enabled)
{ 
  if (enabled)
  {
     WAN_TOP_WRITE_32(0x8014404c,0xEC);
  }
  else
  {
     WAN_TOP_WRITE_32(0x8014404c,0x6C);
  }
     
}
EXPORT_SYMBOL(ngpon_wan_top_enable_transmitter);


MODULE_LICENSE("Proprietary");

