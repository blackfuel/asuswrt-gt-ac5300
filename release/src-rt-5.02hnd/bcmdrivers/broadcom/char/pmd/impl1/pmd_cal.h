/*
   Copyright (c) 2013 Broadcom
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/
#ifndef _PMD_CAL_H_
#define _PMD_CAL_H_

#include "pmd.h"

#define CAL_FILE_WATERMARK 0xcafe2bed
#define CAL_FILE_INVALID_ENTRANCE 0xbadbad

typedef struct
{
    int32_t val;
    bool valid;
}flash_int;

typedef struct
{
    int16_t val;
    bool valid;
}flash_word;

/* Calibration file version 1 */
typedef struct
{
    flash_int  watermark;       
    flash_int  version;
    flash_word level_0_dac;     /* used for first burst + CID tracking */
    flash_word level_1_dac;
    flash_word bias;
    flash_word mod;
    flash_word apd;
    flash_word mpd_config;
    flash_word mpd_gains;
    flash_word apdoi_ctrl;
    flash_int rssi_a_cal;      /* optic_power = a * rssi + b */
    flash_int rssi_b_cal;
    flash_int rssi_c_cal;
    flash_word temp_0;
    flash_word coff_h;    /* Temperate coefficient high */
    flash_word coff_l;    /* Temperate coefficient low */
    flash_word esc_th;
    flash_word rogue_th;
    flash_word avg_level_0_dac; /* used for force tracking */
    flash_word avg_level_1_dac;
    flash_word dacrange;
    flash_word los_thr;
    flash_word sat_pos;
    flash_word sat_neg;
    flash_word edge_rate;
    flash_word preemphasis_weight;
    flash_int  preemphasis_delay;
    flash_word duty_cycle;
    flash_word calibctrl;
    flash_word tx_power;
    flash_word bias0;
    flash_word temp_offset;
    flash_word bias_delta_i;
    flash_word slope_efficiency;
}pmd_calibration;

int pmd_cal_param_set(pmd_calibration_param cal_param, int32_t val);
int pmd_cal_param_get(pmd_calibration_param cal_param, int32_t *val);
int pmd_cal_file_init(void);

#endif /* _PMD_CAL_H_ */
