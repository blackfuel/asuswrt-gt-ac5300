/*---------------------------------------------------------------------------

<:copyright-BRCM:2013:proprietary:standard 

   Copyright (c) 2013 Broadcom 
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
 ------------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <bcm_OS_Deps.h>
#include <linux/bcm_log.h>
#include "pmd_cal.h"
#include "pmd_op.h"
#include "pmd.h"

/* calibration file */
pmd_calibration calibration_file;
#define CAL_FILE_VERSION 4 /* should be incremented when changing the calibration file parameters */

int set_invalid(pmd_calibration_param cal_param)
{
    int rc = 0;

    switch (cal_param)
    {
    case pmd_file_watermark:
        calibration_file.watermark.valid = 0;
        break;
     case pmd_file_version:
        calibration_file.version.valid = 0;
        break;
    case pmd_level_0_dac:
        calibration_file.level_0_dac.valid = 0;
        break;
    case pmd_level_1_dac:
        calibration_file.level_1_dac.valid = 0;
        break;
    case pmd_bias:
        calibration_file.bias.valid = 0;
        break;
    case pmd_mod:
        calibration_file.mod.valid = 0;
        break;
    case pmd_apd:
        calibration_file.apd.valid = 0;
        break;
    case pmd_mpd_config:
        calibration_file.mpd_config.valid = 0;
        break;
    case pmd_mpd_gains:
        calibration_file.mpd_gains.valid = 0;
        break;
    case pmd_apdoi_ctrl:
        calibration_file.apdoi_ctrl.valid = 0;
        break;
    case pmd_rssi_a:
        calibration_file.rssi_a_cal.valid = 0;
        break;
    case pmd_rssi_b:
        calibration_file.rssi_b_cal.valid = 0;
        break;
    case pmd_rssi_c:
        calibration_file.rssi_c_cal.valid = 0;
        break;
    case pmd_temp_0:
        calibration_file.temp_0.valid = 0;
        break;
    case pmd_temp_coff_h:
        calibration_file.coff_h.valid = 0;
        break;
    case pmd_temp_coff_l:
        calibration_file.coff_l.valid = 0;
        break;
    case pmd_esc_thr:
        calibration_file.esc_th.valid = 0;
        break;
    case pmd_rogue_thr:
        calibration_file.rogue_th.valid = 0;
        break;
    case pmd_avg_level_0_dac:
        calibration_file.avg_level_0_dac.valid = 0;
        break;
    case pmd_avg_level_1_dac:
        calibration_file.avg_level_1_dac.valid = 0;
        break;
    case pmd_dacrange:
        calibration_file.dacrange.valid = 0;
        break;
    case pmd_los_thr:
        calibration_file.los_thr.valid = 0;
        break;
    case pmd_sat_pos:				
        calibration_file.sat_pos.valid = 0;
        break;
    case pmd_sat_neg:				
        calibration_file.sat_neg.valid = 0;
        break;
    case pmd_edge_rate:
        calibration_file.edge_rate.valid = 0;
        break;
    case pmd_preemphasis_weight:
        calibration_file.preemphasis_weight.valid = 0;
        break;
    case pmd_preemphasis_delay:
        calibration_file.preemphasis_delay.valid = 0;
        break;
    case pmd_duty_cycle:
    	calibration_file.duty_cycle.valid = 0;
    	break;
    case pmd_mpd_calibctrl:
        calibration_file.calibctrl.valid = 0;
        break;
    case pmd_tx_power:
        calibration_file.tx_power.valid = 0;
        break;
    case pmd_bias0:
        calibration_file.bias0.valid = 0;
        break;
    case pmd_temp_offset:
        calibration_file.temp_offset.valid = 0;
        break;
    case pmd_bias_delta_i:
        calibration_file.bias_delta_i.valid = 0;
        break;
    case pmd_slope_efficiency:
        calibration_file.slope_efficiency.valid = 0;
        break;

    default:
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Invalid calibration param \n");
        break;
    }

    rc = pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, sizeof(pmd_calibration), PMD_WRITE_OP);
    if (rc)
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Write invalid to calibration param number %d failed \n", (uint32_t)cal_param);

    return rc;
}

int pmd_cal_param_set(pmd_calibration_param cal_param, int32_t val)
{
    int rc =0;
    int16_t val_word = 0;
    val_word = val & 0xffff;

    if (val == CAL_FILE_INVALID_ENTRANCE)
        return set_invalid(cal_param);

    switch (cal_param)
    {
    case pmd_level_0_dac:
        calibration_file.level_0_dac.val = val_word;
        calibration_file.level_0_dac.valid = 1;
        break;
    case pmd_level_1_dac:
        calibration_file.level_1_dac.val = val_word;
        calibration_file.level_1_dac.valid = 1;
        break;
    case pmd_bias:
        calibration_file.bias.val = val_word;
        calibration_file.bias.valid = 1;
        break;
    case pmd_mod:
        calibration_file.mod.val = val_word;
        calibration_file.mod.valid = 1;
        break;
    case pmd_apd:
        calibration_file.apd.val = val_word;
        calibration_file.apd.valid = 1;
        break;
    case pmd_mpd_config:
        calibration_file.mpd_config.val = val_word;
        calibration_file.mpd_config.valid = 1;
        break;
    case pmd_mpd_gains:
        calibration_file.mpd_gains.val = val_word;
        calibration_file.mpd_gains.valid = 1;
        break;
    case pmd_apdoi_ctrl:
        calibration_file.apdoi_ctrl.val = val_word;
        calibration_file.apdoi_ctrl.valid = 1;
        break;
    case pmd_rssi_a:
        calibration_file.rssi_a_cal.val = val;
        calibration_file.rssi_a_cal.valid = 1;
        break;
    case pmd_rssi_b:
        calibration_file.rssi_b_cal.val = val;
        calibration_file.rssi_b_cal.valid = 1;
        break;
    case pmd_rssi_c:
        calibration_file.rssi_c_cal.val = val;
        calibration_file.rssi_c_cal.valid = 1;
        break;
    case pmd_temp_0:
        calibration_file.temp_0.val = val_word;
        calibration_file.temp_0.valid = 1;
        break;
    case pmd_temp_coff_h:
        calibration_file.coff_h.val = val_word;
        calibration_file.coff_h.valid = 1;
        break;
    case pmd_temp_coff_l:
        calibration_file.coff_l.val = val_word;
        calibration_file.coff_l.valid = 1;
        break;
    case pmd_esc_thr:
        calibration_file.esc_th.val = val_word;
        calibration_file.esc_th.valid = 1;
        break;
    case pmd_rogue_thr:
        calibration_file.rogue_th.val = val_word;
        calibration_file.rogue_th.valid = 1;
        break;
    case pmd_avg_level_0_dac:
        calibration_file.avg_level_0_dac.val = val_word;
        calibration_file.avg_level_0_dac.valid = 1;
        break;
    case pmd_avg_level_1_dac:
        calibration_file.avg_level_1_dac.val = val_word;
        calibration_file.avg_level_1_dac.valid = 1;
        break;
    case pmd_dacrange:
        calibration_file.dacrange.val = val_word;
        calibration_file.dacrange.valid = 1;
        break;
    case pmd_los_thr:
        calibration_file.los_thr.val = val_word;
        calibration_file.los_thr.valid = 1;
        break;
    case pmd_sat_pos:		
        calibration_file.sat_pos.val = val_word;
        calibration_file.sat_pos.valid = 1;
        break;
    case pmd_sat_neg:		
        calibration_file.sat_neg.val = val_word;
        calibration_file.sat_neg.valid = 1;
        break;
    case pmd_edge_rate:
        calibration_file.edge_rate.val = val_word;
        calibration_file.edge_rate.valid = 1;
        break;
    case pmd_preemphasis_weight:
        calibration_file.preemphasis_weight.val = val_word;
        calibration_file.preemphasis_weight.valid = 1;
        break;
    case pmd_preemphasis_delay:
        calibration_file.preemphasis_delay.val = val;
        calibration_file.preemphasis_delay.valid = 1;
        break;
    case pmd_duty_cycle:
    	calibration_file.duty_cycle.val = val;
    	calibration_file.duty_cycle.valid = 1;
    	break;
    case pmd_mpd_calibctrl:
        calibration_file.calibctrl.val = val;
        calibration_file.calibctrl.valid = 1;
        break;
    case pmd_tx_power:
        calibration_file.tx_power.val = val_word;
        calibration_file.tx_power.valid = 1;
        break;
    case pmd_bias0:
        calibration_file.bias0.val = val_word;
        calibration_file.bias0.valid = 1;
        break;		
    case pmd_temp_offset:
        calibration_file.temp_offset.val = val;
        calibration_file.temp_offset.valid = 1;
        break;
    case pmd_bias_delta_i:
        calibration_file.bias_delta_i.val = val;
        calibration_file.bias_delta_i.valid = 1;
        break;
    case pmd_slope_efficiency:
        calibration_file.slope_efficiency.val = val;
        calibration_file.slope_efficiency.valid = 1;
        break;
    default:
        rc = -1;
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Modification of calibration param number %d not supported \n", (uint32_t)cal_param);
        break;
    }

    rc = pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, sizeof(pmd_calibration), PMD_WRITE_OP);
    if (rc)
    {
        set_invalid(cal_param);
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Write of calibration param number %d failed \n", (uint32_t)cal_param);
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, " calibration_file[ %d ] = %x \n", cal_param, val_word);
    return rc;
}

int pmd_cal_param_get(pmd_calibration_param cal_param, int32_t *val)
{
    uint8_t valid;

    switch (cal_param)
    {
    case pmd_file_watermark:
        *val = calibration_file.watermark.val;
        valid = calibration_file.watermark.valid;
        break;
    case pmd_file_version:
        *val = calibration_file.version.val;
        valid = calibration_file.version.valid;
        break;
    case pmd_level_0_dac:
        *val = calibration_file.level_0_dac.val;
        valid = calibration_file.level_0_dac.valid;
        break;
    case pmd_level_1_dac:
        *val = calibration_file.level_1_dac.val;
        valid = calibration_file.level_1_dac.valid;
        break;
    case pmd_bias:
        *val = calibration_file.bias.val;
        valid = calibration_file.bias.valid;
        break;
    case pmd_mod:
        *val = calibration_file.mod.val;
        valid = calibration_file.mod.valid;
        break;
    case pmd_apd:
        *val = calibration_file.apd.val;
        valid = calibration_file.apd.valid;
        break;
    case pmd_mpd_config:
        *val = calibration_file.mpd_config.val;
        valid = calibration_file.mpd_config.valid;
        break;
    case pmd_mpd_gains:
        *val = calibration_file.mpd_gains.val;
        valid = calibration_file.mpd_gains.valid;
        break;
    case pmd_apdoi_ctrl:
        *val = calibration_file.apdoi_ctrl.val;
        valid = calibration_file.apdoi_ctrl.valid;
        break;
    case pmd_rssi_a:
        *val = calibration_file.rssi_a_cal.val;
        valid = calibration_file.rssi_a_cal.valid;
        break;
    case pmd_rssi_b:
        *val = calibration_file.rssi_b_cal.val;
        valid = calibration_file.rssi_b_cal.valid;
        break;
    case pmd_rssi_c:
        *val = calibration_file.rssi_c_cal.val;
        valid = calibration_file.rssi_c_cal.valid;
        break;
    case pmd_temp_0:
        *val = calibration_file.temp_0.val;
        valid = calibration_file.temp_0.valid;
        break;
    case pmd_temp_coff_h:
        *val = calibration_file.coff_h.val;
        valid = calibration_file.coff_h.valid;
        break;
    case pmd_temp_coff_l:
        *val = calibration_file.coff_l.val;
        valid = calibration_file.coff_l.valid;
        break;
    case pmd_esc_thr:
        *val = calibration_file.esc_th.val;
        valid = calibration_file.esc_th.valid;
        break;
    case pmd_rogue_thr:
        *val = calibration_file.rogue_th.val;
        valid = calibration_file.rogue_th.valid;
        break;
    case pmd_avg_level_0_dac:
        *val = calibration_file.avg_level_0_dac.val;
        valid = calibration_file.avg_level_0_dac.valid;
        break;
    case pmd_avg_level_1_dac:
        *val = calibration_file.avg_level_1_dac.val;
        valid = calibration_file.avg_level_1_dac.valid;
        break;
    case pmd_dacrange:
        *val = calibration_file.dacrange.val;
        valid = calibration_file.dacrange.valid;
        break;
    case pmd_los_thr:
        *val = calibration_file.los_thr.val;
        valid = calibration_file.los_thr.valid;
        break;
    case pmd_sat_pos:
        *val = calibration_file.sat_pos.val;
        valid = calibration_file.sat_pos.valid;
        break;
    case pmd_sat_neg:				
        *val = calibration_file.sat_neg.val;
        valid = calibration_file.sat_neg.valid;
        break;
    case pmd_edge_rate:
        *val = calibration_file.edge_rate.val;
        valid = calibration_file.edge_rate.valid;
        break;
    case pmd_preemphasis_weight:
        *val = calibration_file.preemphasis_weight.val;
        valid = calibration_file.preemphasis_weight.valid;
        break;
    case pmd_preemphasis_delay:
        *val = calibration_file.preemphasis_delay.val;
        valid = calibration_file.preemphasis_delay.valid;
        break;
    case pmd_duty_cycle:
    	*val = calibration_file.duty_cycle.val;
    	valid = calibration_file.duty_cycle.valid;
    	break;
    case pmd_mpd_calibctrl:
        *val = calibration_file.calibctrl.val;
        valid = calibration_file.calibctrl.valid;
        break;
    case pmd_tx_power:
        *val = calibration_file.tx_power.val;
        valid = calibration_file.tx_power.valid;
        break;
    case pmd_bias0:
        *val = calibration_file.bias0.val;
        valid = calibration_file.bias0.valid;
        break;
    case pmd_temp_offset:
        *val = calibration_file.temp_offset.val;
        valid = calibration_file.temp_offset.valid;
        break;
    case pmd_bias_delta_i:
        *val = calibration_file.bias_delta_i.val;
        valid = calibration_file.bias_delta_i.valid;
        break;
    case pmd_slope_efficiency:
        *val = calibration_file.slope_efficiency.val;
        valid = calibration_file.slope_efficiency.valid;
        break;
    default:
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Read of calibration param number %d not supported \n",(uint32_t)cal_param);
        return -1;
        break;
    }

    if (!valid)
    {
        *val = CAL_FILE_INVALID_ENTRANCE;
        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, " calibration param %d invalid \n", (uint32_t)cal_param);
        return -1;
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, " calibration_file[ %d ] = %x \n", (uint32_t)cal_param, *val);
    return 0;
}

int pmd_cal_file_init(void)
{
    int rc = 0;
    uint16_t len;

    pmd_op_get_file_len(pmd_cal_map, &len);

        /*create the calibration file if not already created*/
    if (pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, len, PMD_READ_OP))
    {
        /* create new calibration file */
        calibration_file.watermark.val = CAL_FILE_WATERMARK;
        calibration_file.watermark.valid = 1;
        calibration_file.version.val = CAL_FILE_VERSION;
        calibration_file.version.valid = 1;

        pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, sizeof(pmd_calibration), PMD_WRITE_OP);
    }
    else
    {
        if (calibration_file.version.val < 4)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, "We don't support this calibration file version anymore, please "
                    "delete the old calibration file and calibrate the board again \n");
            return -1;
        }

        /* file is corrupted create new one */
        if (calibration_file.watermark.val != CAL_FILE_WATERMARK)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Flash calibration file is corrupted, creating new empty one \n");

            calibration_file.watermark.val = CAL_FILE_WATERMARK;
            calibration_file.watermark.valid = 1;
            calibration_file.version.val = CAL_FILE_VERSION;
            calibration_file.version.valid = 1;

            rc = pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, sizeof(pmd_calibration), PMD_WRITE_OP);
            if (rc)
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Calibration file flash creation failed \n");
        }
        else
        {
            /* extend to new file size */
            if (len != sizeof(pmd_calibration))
            {
                calibration_file.version.val = CAL_FILE_VERSION;
                calibration_file.version.valid = 1;

                /* extend to new file size */
                if (len < sizeof(pmd_calibration))
                {
                    rc = pmd_op_file(pmd_cal_map, (unsigned char *)&calibration_file, sizeof(pmd_calibration), PMD_WRITE_OP);
                    if (rc)
                        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "failed to extend Calibration file in flash\n");
                }
            }
        }

    }
    return rc;
}

