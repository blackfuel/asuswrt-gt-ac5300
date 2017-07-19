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
#include <linux/types.h>
#include <linux/namei.h>
#include <bcm_OS_Deps.h>
#include <linux/ioctl.h>
#include <bcm_intr.h>
#include <laser.h>
#include <boardparms.h>
#if defined (CONFIG_BCM96838)
#include "shared_utils.h"
#else
#include "bcm_pinmux.h"
#include "bcm_gpio.h"
#define gpio_set_data bcm_gpio_set_data
#define gpio_set_dir bcm_gpio_set_dir
#define set_pinmux bcm_set_pinmux
#endif
#include <board.h>
#include <linux/bcm_log.h>
#include <linux/kthread.h>
#include <rdpa_api.h>
#include <rdpa_gpon.h>
#include <rdpa_epon.h>
#include "blocks.h"
#include "clkrst_testif.h"
#include "ld_lia.h"
#include "lrl.h"
#include "pmd_msg.h"
#include "HostCommon.h"
#include "pmd_op.h"
#include "pmd_cal.h"
#include "pmd_temp_cal.h"
#include "pmd_data_col.h"
#include "ru.h"
#include "shared_utils.h"
#include "wan_drv.h"
#include "pmd.h"

/* character major device number */
#define PMD_DEV_MAJOR   239

/* mutex */
struct mutex pmd_dev_lock;
#define LOCK() mutex_lock(&pmd_dev_lock)
#define UNLOCK() mutex_unlock(&pmd_dev_lock);

/* periodic timer */
struct timer_list periodic_timer;
#define PMD_TIMER_INTERVAL 1000 /*once in a sec*/

static struct task_struct *pmd_timer_thread;
wait_queue_head_t pmd_thread_event;
static int pmd_thread_sched;

static uint16_t pmd_state = pmd_tracking_state_disabled;
static bool pmd_fb_done = false;
static uint8_t num_of_ranging_attempts = 0;

/* board type */
typedef enum
{
    pmd_epon_board,
    pmd_gpon_board,
    pmd_gbe_board,
    pmd_auto_detect, /* gpon, epon auto detect */
}pmd_boards;

static uint16_t pmd_board_type;

uint8_t pmd_stack_state;
#define PMD_TRACKING_DELTA_TEMP 2
pmd_statistic_first_burst_callback pmd_statistic_callback;
pmd_math_first_burst_model_callback pmd_math_callback;

pmd_gpon_isr_callback gpon_isr_wrapper;

gpon_set_pmd_fb_done_callback gpon_set_pmd_fb_done;

uint16_t pmd_dev_first_burst_temp_dep(void);

/* indicate pmd fw is ready */
static wait_queue_head_t pmd_reset_event;
static int pmd_reset_normal;

/* holds latest read alarm */
uint16_t alarm_msg;
bool alarm_read_flag = 1;

struct tasklet_struct fault_tasklet;
#if defined(CONFIG_BCM96838)
struct tasklet_struct sd_tasklet;
#endif

/* for debugging and calibration */
#define DISABLE_TRACKING_CMD 60
uint8_t disable_tracking_flag;
/* debug print data collection */
#define PRINT_DATA_COLLECTION 61
uint8_t data_col_flag = 1;

uint32_t print_count;

#define TEMP_NOT_SET 0x555

int16_t pmd_ext_tracking_temp = TEMP_NOT_SET;
uint8_t pmd_gpon_stack_cur_state;
#define PMD_DATA_COL_INTERVAL 5;

#define EWAKE_MAC_CONTROL 3
#define EWAKE_CONSTANT_HIGH 5

#define PMD_CALIBRATION_INIT 1
#define PMD_CALIBRATION_STOP 13

#define MAX_NUM_RANGING_ATTEMPTS 4

void pmd_dev_assign_first_burst_temp_dep_func(pmd_statistic_first_burst_callback statistic_callback, pmd_math_first_burst_model_callback pmd_math)
{
    pmd_statistic_callback = statistic_callback;
    pmd_math_callback = pmd_math;
}
EXPORT_SYMBOL( pmd_dev_assign_first_burst_temp_dep_func );

void pmd_dev_assign_gpon_callback(pmd_gpon_isr_callback gpon_isr)
{
    gpon_isr_wrapper = gpon_isr;
}
EXPORT_SYMBOL( pmd_dev_assign_gpon_callback );

void pmd_dev_assign_gpon_stack_callback(gpon_set_pmd_fb_done_callback _gpon_set_pmd_fb_done)
{
    gpon_set_pmd_fb_done = _gpon_set_pmd_fb_done;
}
EXPORT_SYMBOL( pmd_dev_assign_gpon_stack_callback );

static void set_rstn(uint8_t gpio_state)
{
    unsigned short rstn;
    if (BpGetGpioPmdReset(&rstn) == BP_SUCCESS)
    {
        kerSysSetGpioDir(rstn);
        kerSysSetGpioState(rstn, gpio_state);
        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "rstn gpio set to %s \n", gpio_state ? "active" : "inactive");
    }
    else
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Fail to set rstn gpio to %s \n", gpio_state ? "active" : "inactive");
 
}

void pmd_dev_ewake_mac_control(uint16_t enable)
{
    unsigned short gpio;
    int gpio_state = EWAKE_MAC_CONTROL;

    BpGetPmdMACEwakeEn(&gpio);

    if (!enable)
        gpio_state = EWAKE_CONSTANT_HIGH;

    set_pinmux(gpio & BP_GPIO_NUM_MASK, gpio_state);
	
    /* effects only when gpio is in control of pin */
    gpio_set_dir(gpio & BP_GPIO_NUM_MASK, 1);
    gpio_set_data(gpio & BP_GPIO_NUM_MASK, 1);
}

void pmd_dev_diable_ewake_in_calibration(hm_msg_id msg_id, uint16_t type)
{
    if(msg_id != hmid_cal_type_set)
        return;

    if (type == PMD_CALIBRATION_INIT)
        pmd_dev_ewake_mac_control(0);

    if (type == PMD_CALIBRATION_STOP)
        pmd_dev_ewake_mac_control(1);
}

static long pmd_dev_file_ioctl(struct file *file, unsigned int cmd,
    unsigned long arg)
{
	unsigned long * pPwr = (unsigned long *) arg;
    unsigned long rc=0;
    LOCK();
    switch (cmd) 
    {
        case LASER_IOCTL_GET_RX_PWR:
        {           
            uint16_t op;

            rc = pmd_msg_handler(hmid_estimated_op_get, &op, sizeof(uint16_t));
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error reading PMD RX optical power \n");
            }
            *pPwr = ((unsigned long)op * 10 / 16); //PMD units is 1/16uW, converting to 0.1uW units
            break;
        }
        case LASER_IOCTL_GET_TX_PWR:
        {
            uint16_t tp;

            rc = pmd_msg_handler(hmid_level1_tracking_get, &tp, sizeof(uint16_t));
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error reading PMD TX optical power \n");
            }
            *pPwr = ((unsigned long)tp * 10); //Converting to 0.1uW
            break;
        }
        case LASER_IOCTL_GET_TEMPTURE:
        {
            uint16_t cur_temp = 0xFFFF;
            
            /* read and clear pmd interrupt */
            rc = pmd_msg_handler(hmid_temperature_get, &cur_temp, sizeof(uint16_t));
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error reading PMD temperature \n");
            }
            *pPwr = ((unsigned long)cur_temp << 8);
            break;
        }
        case LASER_IOCTL_GET_VOTAGE:
        {
            uint32_t pmd_voltage = 0;
            *pPwr = (unsigned long)pmd_voltage;
            break;
        }
        case LASER_IOCTL_GET_BIAS_CURRENT:
        {
            uint16_t cur_bias = 0;

            rc = pmd_msg_handler(hmid_bias_cur_get, &cur_bias, sizeof(uint16_t));
            if (rc)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error reading PMD current bias \n");
            }

            *pPwr = (unsigned long)cur_bias * 10;   //In 2 uA
            break;
        }
        case LASER_IOCTL_SET_OPTICAL_PARAMS:
        {

            break;
        }
        case LASER_IOCTL_GET_OPTICAL_PARAMS:
        {

            break;
        }
        case LASER_IOCTL_INIT_TX_PWR:
        {
            /* *pPwr = NULL; */
            break;
        }
        case LASER_IOCTL_INIT_RX_PWR:
        {
            /* *pPwr = NULL; */
            break;
        }
        case LASER_IOCTL_GET_INIT_PARAMS:
        {
            /* *pPwr = NULL; */
            break;
        }
        case LASER_IOCTL_GET_DRV_INFO:
        {
            break;
        }
        case PMD_IOCTL_SET_PARAMS:
        {
        	pmd_params pmd_op;
        	char buf[PMD_BUF_MAX_SIZE];

        	rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

        	if (pmd_op.len > PMD_BUF_MAX_SIZE)
        		pmd_op.len = PMD_BUF_MAX_SIZE;

        	rc = rc ? rc : copy_from_user(buf, pmd_op.buf, pmd_op.len);

        	rc = rc ? rc : pmd_op_i2c(pmd_op.client, pmd_op.offset, buf, pmd_op.len, PMD_WRITE_OP);
        	break;
        }
        case PMD_IOCTL_GET_PARAMS:
        {
        	pmd_params pmd_op;
        	char buf[PMD_BUF_MAX_SIZE];

        	rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

        	if( pmd_op.len > PMD_BUF_MAX_SIZE )
        		pmd_op.len = PMD_BUF_MAX_SIZE;

        	rc = rc ? rc : pmd_op_i2c(pmd_op.client, pmd_op.offset, buf, pmd_op.len, PMD_READ_OP);

        	rc = rc ? rc : copy_to_user(pmd_op.buf, buf, pmd_op.len);
        	break;
        }
        case PMD_IOCTL_CAL_FILE_WRITE:
        {
            pmd_params pmd_op;
            int32_t val;

            rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

            rc = rc ? rc : copy_from_user(&val, pmd_op.buf, sizeof(int32_t));

            if (pmd_op.offset == DISABLE_TRACKING_CMD)
                disable_tracking_flag = val;
            else if (pmd_op.offset == PRINT_DATA_COLLECTION)
            {
                data_col_flag = val;
                if (data_col_flag)
                    pmd_data_col_print();
            }
            else
                rc = rc ? rc : pmd_cal_param_set((pmd_calibration_param)pmd_op.offset, val);

        	break;
        }
        case PMD_IOCTL_CAL_FILE_READ:
        {
            pmd_params pmd_op;
            int32_t val;

            rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));
            if (rc)
                break;

            pmd_cal_param_get((pmd_calibration_param)pmd_op.offset, &val);

            rc = copy_to_user(pmd_op.buf, &val, sizeof(int32_t));

        	break;
        }
        case PMD_IOCTL_MSG_WRITE:
        {
            pmd_params pmd_op;
            char buf[PMD_BUF_MAX_SIZE];

            rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

            if (get_msg_type((hm_msg_id)pmd_op.offset) != pmd_set_msg)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " msg_id = %x  is not a set msg \n", pmd_op.offset);
                rc = -1;
                break;
            }

            if (pmd_op.len > PMD_BUF_MAX_SIZE)
                pmd_op.len = PMD_BUF_MAX_SIZE;

            rc = rc ? rc : copy_from_user(buf, pmd_op.buf, pmd_op.len);

            pmd_dev_diable_ewake_in_calibration((hm_msg_id)pmd_op.offset, *(uint16_t *)buf);
            rc = rc ? rc : pmd_msg_handler((hm_msg_id)pmd_op.offset, (uint16_t *)buf, pmd_op.len);

            break;
        }
        case PMD_IOCTL_MSG_READ:
        {
            pmd_params pmd_op;
            char buf[PMD_BUF_MAX_SIZE];

            rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

            if (get_msg_type((hm_msg_id)pmd_op.offset) != pmd_get_msg)
            {
                BCM_LOG_ERROR(BCM_LOG_ID_PMD, " msg_id = %x is not a get msg \n", pmd_op.offset);
                rc = -1;
                break;
            }

            if (pmd_op.len > PMD_BUF_MAX_SIZE)
                pmd_op.len = PMD_BUF_MAX_SIZE;

            rc = rc ? rc : pmd_msg_handler((hm_msg_id)pmd_op.offset, (uint16_t *)buf, pmd_op.len);

            rc = rc ? rc : copy_to_user(pmd_op.buf, buf, pmd_op.len);

            break;
        }
        case PMD_IOCTL_TEMP2APD_WRITE:
        {
            pmd_params pmd_op;
            uint16_t pmd_temp_apd_conv[APD_TEMP_TABLE_SIZE];

            rc = copy_from_user(&pmd_op, (void *)arg, sizeof(pmd_params));

            memcpy(pmd_temp_apd_conv, pmd_op.buf, sizeof(pmd_temp_apd_conv));

            pmd_temp2apd_file_op(pmd_temp_apd_conv);

            break;
        }
        case PMD_IOCTL_DUMP_DATA:
        {
            int rc;
            rc =  pmd_dump_data();
            if(rc)
            	BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "pmd_dumpdata command failed  \n");
            break;
        }
        default:
        	BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "pmd_dev IOCtl command unknown \n");
        	break;

    }
    UNLOCK();
    return rc;
}

static int pmd_dev_file_open(struct inode *inode, struct file *file)
{
    return 0;
}
static int pmd_dev_file_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations pmd_file_ops = {
    .owner =        THIS_MODULE,
    .open =         pmd_dev_file_open,
    .release =      pmd_dev_file_release,
    .unlocked_ioctl =   pmd_dev_file_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = pmd_dev_file_ioctl,
#endif
    
};

static int crt_reset_config(void)
{
    uint32_t rv;
    int rc = 0;

	rc = rc ? rc : RU_REG_READ(CRT, DIGITAL_BLOCK_RESET_CONFIG, &rv);
	RU_FIELD_SET(CRT, DIGITAL_BLOCK_RESET_CONFIG, CFG_CPU_RST, rv, 0);
	rc = rc ? rc : RU_REG_WRITE(CRT, DIGITAL_BLOCK_RESET_CONFIG, rv);
    return rc;
}

static int pmd_func_set(void)
{
    int rc = 0;
    unsigned short pmd_cap = 0;
    uint32_t rv;

    rc = RU_REG_READ(CRT, CRT_PARAM, &rv);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Error reading CRT register. rc = %d\n", rc);
    }

    rc = BpGetPmdFunc(&pmd_cap);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Error in BpGetPmdFunc. rc = %d\n", rc);
    }

    rv = pmd_cap;

    rc = RU_REG_WRITE(CRT, CRT_PARAM, rv);
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Error writing CRT register. rc = %d\n", rc);
    }

    return rc;
}

void ld_lia_set_cdr(void)
{
    RU_FIELD_WRITE(LD_LIA, LD_CDR_CONFIG_2, CFG_LD_CDR_RESET, 0); /* Take CDR out of reset */
    RU_FIELD_WRITE(LD_LIA, LD_CONTROL_1, CFG_LD_SIG_BIASADJ, 0x4222); /* Enable CDR signal path buffers */

    RU_FIELD_WRITE(LD_LIA, LD_CDR_RX_CONFIG, CFG_LD_RX0_PWRDN, 0);
    RU_FIELD_WRITE(LD_LIA, LD_CDR_RX_CONFIG, CFG_LD_RX0_SIGDET_PWRDN, 0);
}

int ld_pll_init(void)
{
    uint32_t rv;
    uint32_t retry = 1024;
    int rc = 0;

    /* RefClkCheck */
    RU_FIELD_WRITE(CRT, REF_CLOCK_MONITOR_CONFIG, CFG_REFCLK_MON_EN, 0);
    RU_FIELD_WRITE(CRT, REF_CLOCK_MONITOR_CONFIG, CFG_REFCLK_MON_EN, 1);
    rc = RU_REG_READ(CRT, REF_CLOCK_MONITOR_STATUS, &rv);
    if (RU_FIELD_GET(CRT, REF_CLOCK_MONITOR_STATUS, STAT_REFCLK_MON_RESULT, rv) < 10)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "pll error: External reference clock frequency failure");
        return -1;
    }

    rc = rc ? rc : RU_REG_READ(LD_LIA, LD_PLL_CONTROL_1, &rv);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_KA, rv, 3);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_KI, rv, 1);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_KP, rv, 9);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_MODE1P25, rv, 1);
    rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_PWRDN, rv, 0);
    rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_PDIV, rv, 1);

    if (pmd_board_type == pmd_gpon_board)
    {
        RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_NDIV_INT, rv, 99);
        rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);
        RU_FIELD_WRITE(LD_LIA, LD_PLL_CONTROL_2, CFG_LD_PLL_NDIV_FRAC, 558682);
    }
    else if (pmd_board_type == pmd_epon_board)
    {
        RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_NDIV_INT, rv, 100);
        rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);
        RU_FIELD_WRITE(LD_LIA, LD_PLL_CONTROL_2, CFG_LD_PLL_NDIV_FRAC, 0);
    }

    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_0, CFG_LD_PLL_MDIV_0, 2);
    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_1, CFG_LD_PLL_MDIV_1, 5);
    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_2, CFG_LD_PLL_MDIV_2, 12);
    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_3, CFG_LD_PLL_MDIV_3, 39);
    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_4, CFG_LD_PLL_MDIV_4, 5);
    RU_FIELD_WRITE(LD_LIA, LD_PLL_DIVIDER_5, CFG_LD_PLL_MDIV_5, 5);
   
    RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_RESETB, rv, 1);
    rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);

    while (--retry && !RU_FIELD_READ(LD_LIA, LD_PLL_STATUS, LD_PLL_LOCK))
        ;

    if (!retry)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "PLL lock has failed");
        return -1;
    }
    else
    {
        RU_FIELD_SET(LD_LIA, LD_PLL_CONTROL_1, CFG_LD_PLL_POST_RESETB, rv, 1);
        rc = rc ? rc : RU_REG_WRITE(LD_LIA, LD_PLL_CONTROL_1, rv);


        /* CrtBoot */
        RU_FIELD_WRITE(CRT, CLOCK_SOURCE_CONFIG, CFG_CORE_CLOCK_SOURCE, 1);
        RU_FIELD_WRITE(CRT, DIGITAL_BLOCK_RESET_CONFIG, CFG_GEN_CLOCKS_RST, 0);
        RU_FIELD_WRITE(CRT, CLOCK_SOURCE_CONFIG, CFG_BSL_DB_CLOCK_SOURCE, 1);
        RU_FIELD_WRITE(CRT, DIGITAL_BLOCK_RESET_CONFIG, CFG_LDC_RST, 0);
        RU_FIELD_WRITE(CRT, DIGITAL_BLOCK_RESET_CONFIG, CFG_DYN_MDIV_RST, 0);

        ld_lia_set_cdr();
    }
    return rc;
}

static int pmd_dev_periodic_thread(void *arg)
{
    uint16_t msg;
    int rc;

    while (1)
    {
        static uint8_t col_data_interval = PMD_DATA_COL_INTERVAL;

        wait_event_interruptible(pmd_thread_event, pmd_thread_sched);

        if (kthread_should_stop())
        {
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, "kthread_should_stop detected on pmd_dev\n");
            return 0;
        }

        if (!col_data_interval)
        {
            if (data_col_flag)
                pmd_data_collect();
            col_data_interval = PMD_DATA_COL_INTERVAL;
        }
        if (pmd_gpon_stack_cur_state == PMD_GPON_STATE_OPERATION_O5)
        {
            pmd_msg_handler(hmid_temperature_get, &pmd_ext_tracking_temp, sizeof(uint16_t));
        }
        if ((pmd_gpon_stack_cur_state == PMD_GPON_STATE_STANDBY_O2) && (pmd_ext_tracking_temp != TEMP_NOT_SET))
        {
            if (pmd_dev_first_burst_temp_dep() == pmd_tracking_state_first_burst)
            {
                pmd_state = pmd_tracking_state_first_burst;
                msg = pmd_state;

                rc = pmd_msg_handler(hmid_laser_tracking_set, &msg, sizeof(uint16_t));
                if (rc)
                    BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail to change tracking state \n");
            }
        }

        col_data_interval--;
        pmd_thread_sched = 0;
    }
    return 0;
}

static int pmd_dev_thread_init(void)
{
    /* Initialize thread */
    pmd_timer_thread = kthread_create(pmd_dev_periodic_thread, NULL, "pmd_thread");

    if(!pmd_timer_thread)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Timer thread creation failed \n");
        return -1;
    }

    pmd_thread_sched = 0;
    init_waitqueue_head(&pmd_thread_event);
    wake_up_process(pmd_timer_thread);
    return 0;
}

/* check if init val is already in the flash, if so use it */
int set_param_from_flash(pmd_calibration_param param_id, hm_msg_id msg_id)
{
    int32_t val;
    uint16_t msg_seq;
    int rc = 0;

    /* don't configure if it's not a valid parameter */
    if (pmd_cal_param_get(param_id, &val))
        return 0;

    if (msg_id == hmid_apd_config_set)
        val = val & 0xFFFFFC00;

    msg_seq = get_msg_seq_num(msg_id);
    switch (msg_seq)
    {
    case 1:
        rc = pmd_msg_handler(msg_id, &((int16_t *)&val)[1], msg_seq * 2);
        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "msg_id %x  val %x \n", (uint32_t)msg_id, (int32_t )val);
        break;
    case 2:
        rc = pmd_msg_handler(msg_id, (uint16_t *)&val, msg_seq * 2);
        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "msg_id %x  val %x \n", (uint32_t)msg_id, (int32_t )val);
        break;
    default :
        rc = -1;
        break;
    }

    return rc;
}

int pmd_set_init_val(void)
{
    int rc = 0;
    int32_t val = 0;

    pmd_cal_param_get(pmd_file_watermark, &val);
    if (val != CAL_FILE_WATERMARK)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "Flash file is corrupted\n");
        return 0;
    }

    rc = set_param_from_flash(pmd_level_0_dac, hmid_level_0_dac_ref);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_level_1_dac, hmid_level_1_dac_ref);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_bias, hmid_bias_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_mod, hmid_mod_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_apd, hmid_apd_config_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_mpd_config, hmid_mpd_config_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_mpd_gains, hmid_mpd_gains_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_apdoi_ctrl, hmid_apdoi_ctrl_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_rssi_a, hmid_rssi_a_factor_cal_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_rssi_b, hmid_rssi_b_factor_cal_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_rssi_c, hmid_rssi_c_factor_cal_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_temp_0, hmid_temp_0_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_temp_coff_h, hmid_temp_coff_h_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_temp_coff_l, hmid_temp_coff_l_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_esc_thr, hmid_esc_thr_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_rogue_thr, hmid_rogue_thr_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_avg_level_0_dac, hmid_avg_level_0_dac_ref);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_avg_level_1_dac, hmid_avg_level_1_dac_ref);
    if (rc)
        return rc;
   rc = set_param_from_flash(pmd_dacrange, hmid_dacrange_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_los_thr, hmid_lrl_los_thr_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_sat_pos, hmid_lia_rssi_peakpos_sat_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_sat_neg, hmid_lia_rssi_peakneg_sat_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_edge_rate, hmid_edge_rate_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_preemphasis_weight, hmid_preemphasis_weight_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_preemphasis_delay, hmid_preemphasis_delay_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_duty_cycle, hmid_duty_cycle_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_mpd_calibctrl, hmid_calibctrl_set);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_tx_power, hmid_cal_meas_tx_power);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_bias0, hmid_cal_meas_bias0);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_temp_offset, hmid_temp_offset_set);
    if (rc)
        return rc;
    /*rc = set_param_from_flash(pmd_bias_delta_i, hmid_cal_bias_delta_i);
    if (rc)
        return rc;
    rc = set_param_from_flash(pmd_slope_efficiency, hmid_cal_slope_efficiency);
    if (rc)
        return rc;*/
    /* set board type */
    rc = pmd_msg_handler(hmid_board_type_set, &pmd_board_type, 2);

    return rc;
}

static int pmd_dev_init_seq(void)
{
    int rc = 0;

    init_waitqueue_head(&pmd_reset_event);

    set_rstn(0);

    /* Set PLL */
    rc = ld_pll_init();
    if (rc)
        return -1;

    rc = pmd_op_sw_download();
    if (rc)
        return -1;
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "SW DOWNLOAD DONE \n");

    rc = pmd_func_set();
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, "Error in pmd_func_set \n");
        return -1;
    }

    rc = crt_reset_config();
    if (rc)
        return -1;

    /* wait till pmd finish init then continue with configuration*/
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "Going to sleep, waiting for PMD reset normal indication \n");

    wait_event_interruptible(pmd_reset_event, pmd_reset_normal != 0);
    pmd_reset_normal = 0;

    rc = pmd_set_init_val();
    if (rc)
        return -1;

    rc = pmd_op_temp_conv_table_download(&pmd_res_temp_conv[0], TEMP_TABLE_SIZE, TEMP_TABLE_LOWEST_TEMP);

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "temperature convert table DOWNLOAD DONE \n");
    
    return rc;
}

static int pmd_dev_get_esc_alarm(uint32_t *esc_msg)
{
    PMD_ALARM_INDICATION_PARAMETERS_DTE  *pmd_gpon_alarms = (PMD_ALARM_INDICATION_PARAMETERS_DTE *)esc_msg;
    uint16_t msg = 0;
    int rc = 0;
    
#ifdef PMD_FULL_DEBUG
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");
#endif
    /* read and clear pmd interrupt */
    rc = pmd_msg_handler(hmid_esc_alarm_int_get, &msg, sizeof(uint16_t));
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error clearing PMD ESC interrupt \n");
        return rc;
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "msg %x \n", msg);

    if (msg & PMD_ESC_BE_ALARM)
    {
        pmd_gpon_alarms->esc_be = 1;
        printk("ESC: burst enable alarm \r\n");
    }
    if (msg & PMD_ESC_ROGUE_ALARM)
    {
        pmd_gpon_alarms->esc_rogue = 1;
        printk("ESC: rogue alarm \r\n");
    }
    if (msg & PMD_ESC_MOD_OVR_CRNT)
    {
        pmd_gpon_alarms->esc_mod_over_current = 1;
        printk("ESC: modulation over current alarm \r\n");
    }
    if (msg & PMD_ESC_BIAS_OVR_CRNT)
    {
        pmd_gpon_alarms->esc_bias_over_current = 1;
        printk("ESC: bias over current alarm \r\n");
    }
    if (msg & PMD_ESC_ALARM_MPD_FAULT)
    {
        pmd_gpon_alarms->esc_mpd_fault = 1;
        printk("ESC: mpd fault alarm \r\n");
    }
    if (msg & PMD_ESC_ALARM_EYESAFE)
    {
        pmd_gpon_alarms->esc_eye_safety = 1;
        printk("ESC: eye safety alarm \r\n");
    }

    return 0;
}

static int pmd_dev_get_task_stuck_alarm(void)
{
    int rc = 0;
    uint16_t msg = 0;

    rc = pmd_msg_handler(hmid_task_stuck_get, &msg, sizeof(uint16_t));
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error reading which task is stuck \n");
        return rc;
    }
    if (msg & PMD_IDLE_TASK_STUCK_ALARM)
        printk("PMD: Idle task stuck\n");
    if (msg & PMD_LASER_TASK_STUCK_ALARM)
        printk("PMD: laser task stuck\n");
    if (msg & PMD_RSSI_TASK_STUCK_ALARM)
        printk("PMD: rssi task stuck\n");
    if (msg & PMD_TIMER_TASK_STUCK_ALARM)
        printk("PMD: timer task stuck\n");
    return rc;
}

static int pmd_dev_get_first_burst_done_alarm(void)
{
    int rc = 0;
    uint16_t msg = PMD_FIRST_BURST_COMPLETE;

    /* clear alarm */
    rc = pmd_msg_handler(hmid_alarm_clear, &msg, sizeof(uint16_t));
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error sending clear first burst alarm \n");
        return rc;
    }

    pmd_msg_handler(hmid_temperature_get, &pmd_ext_tracking_temp, sizeof(uint16_t));
    pmd_state = pmd_tracking_state_tracking;

    /* notify stack */
    pmd_fb_done = true;
    gpon_set_pmd_fb_done(1);

    return rc;
}

int pmd_dev_alarm(uint16_t alarm, uint32_t *pmd_ints)
{
    int rc = 0;

    if (alarm & PMD_RESET_NORMAL)
    {
        pmd_reset_normal = 1;
        wake_up_interruptible(&pmd_reset_event);
        printk("PMD: Reset normal\n");
    }
    if (alarm & PMD_RESET_WATCHDOD)
        printk("PMD: Reset watchdog \n");
    if (alarm & PMD_FATAL_ERROR)
        printk("PMD: Fatal error \n");
    if (alarm & PMD_EYE_SAFETY)
    {
        rc = pmd_dev_get_esc_alarm(pmd_ints);
        disable_tracking_flag = 1;
    }
    if (alarm & PMD_OTP_FAILURE)
        printk("PMD: OTP failure \n");
    if (alarm & PMD_LOSS_SIGNAL)
        printk("PMD: Signal loss \n");
    if (alarm & PMD_MEM_DATA_TEST_FAIL)
        printk("PMD: Memory data test error \n");
    if (alarm & PMD_MEM_ADDR_TEST_FAIL)
        printk("PMD: Memory address test error \n");
    if (alarm & PMD_MEM_CELL_TEST_FAIL)
        printk("PMD: Memory cell test error \n");
    if (alarm & ALM_MPD_POWER_UP_FAIL)
        printk("PMD: MPD power up failure \n");
    if (alarm & ALM_TASK_STUCK_ERROR)
        rc |= pmd_dev_get_task_stuck_alarm();
    if (alarm & ALM_CAL_STATE_OVER)
        printk("PMD: Calibration state over \n");
    if (alarm & PMD_FIRST_BURST_COMPLETE)
    {
        printk("PMD: First Burst Complete \n");
        rc |= pmd_dev_get_first_burst_done_alarm();
    }

    return rc;
}

/* return 1 if got valid new values to configure */
static int pmd_dev_get_first_burst_params(uint16_t *bias, uint16_t *mod)
{
    if (pmd_statistic_callback)
    {
        if (!pmd_statistic_callback(bias, mod))
            return 1;
    }
    if (pmd_math_callback)
    {
        if (!pmd_math_callback(bias, mod))
            return 1;
    }
    return 0;
}

/* this function changes the PMD bursten and eye safety mode according to the prbs, misc mode */
void pmd_dev_enable_prbs_or_misc_mode(uint16_t enable, uint8_t prbs_mode)
{
    uint16_t enable_esc = 0;

    if (prbs_mode)
    {
        /* eye safety can't work together with prbs */
        if (!enable)
            enable_esc = 1;
        pmd_msg_handler(hmid_esc_block_enable_set, &enable_esc, sizeof(uint16_t));
        pmd_dev_ewake_mac_control(!enable);
    }
    pmd_msg_handler(hmid_burst_en_dep_set, &enable, sizeof(uint16_t));

}
EXPORT_SYMBOL(pmd_dev_enable_prbs_or_misc_mode);

uint16_t pmd_dev_first_burst_temp_dep(void)
{
    int rc;
    uint16_t bias, mod;
    int16_t cur_temp;
    uint8_t delta_temp = 0;

    /* compare retrieved temp with cur temp */
    pmd_msg_handler(hmid_temperature_get, &cur_temp, 2);

    delta_temp = abs(cur_temp - pmd_ext_tracking_temp);

    if (delta_temp <= PMD_TRACKING_DELTA_TEMP )
    {
        /* temp didn't change much - continue with tracking */
        return pmd_tracking_state_tracking;
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "cur temperature %d last tracking temperature %d delta %d \n",cur_temp, pmd_ext_tracking_temp, delta_temp);

    printk("\n#### change to first_burst ####\n");
    /* temp change too much - to improve first burst, check in callbacks otherwise use the last tracking params*/
    if (pmd_dev_get_first_burst_params(&bias, &mod))
    {
        /* download new bias and mod values */
        rc = pmd_msg_handler(hmid_bias_set, &bias, sizeof(uint16_t));
        if (rc)
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail in hmid_bias_set. rc = %d \n", rc);

        rc = pmd_msg_handler(hmid_mod_set, &mod, sizeof(uint16_t));
        if (rc)
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail in hmid_mod_set, rc = %d \n", rc);

        BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "bias %x mod %x delta temperature %d \n",bias, mod, delta_temp);
    }

    pmd_ext_tracking_temp = TEMP_NOT_SET;
    pmd_fb_done = false;
    gpon_set_pmd_fb_done(0);

    return pmd_tracking_state_first_burst;
}

int pmd_dev_change_tracking_state(uint32_t old_state, uint32_t new_state)
{
    int rc = 0;
    static uint16_t msg;
    uint8_t change_state = 0;
    static uint16_t force_msg;

    pmd_gpon_stack_cur_state = new_state;

    printk("\n#### In pmd_dev_change_tracking_state. old = %d, new = %d ####\n", old_state, new_state);

    if (pmd_board_type == pmd_gpon_board)
    {
        switch(new_state)
        {
            case PMD_GPON_STATE_STANDBY_O2:
            {
                /* Disable force when ranging */
                force_msg = 0;
                rc = pmd_msg_handler(hmid_force_enable, &force_msg, sizeof(uint16_t));
                if (rc)
                    BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail in hmid_force_enable \n");

                if (pmd_fb_done)
                {
                    printk("\n#### pmd_fb_done is TRUE ####\n");

                    pmd_state = pmd_dev_first_burst_temp_dep();

                    if (pmd_state == pmd_tracking_state_tracking)
                    {
                        num_of_ranging_attempts++;
                        if (num_of_ranging_attempts == MAX_NUM_RANGING_ATTEMPTS)
                        {
                            printk("\n#### change to FB since MAX_NUM_RANGING_ATTEMPTS ####\n");
                            pmd_ext_tracking_temp = TEMP_NOT_SET;
                            pmd_fb_done = false;
                            gpon_set_pmd_fb_done(0);
                            pmd_state = pmd_tracking_state_first_burst;
                            num_of_ranging_attempts = 0;
                        }
                    }

                    msg = pmd_state;
                    change_state = 1;
                }
                else
                {
                    printk("\n#### pmd_fb_done is FALSE ####\n");
                    if (pmd_state != pmd_tracking_state_first_burst)
                    {
                        pmd_ext_tracking_temp = TEMP_NOT_SET;
                        pmd_fb_done = false;
                        gpon_set_pmd_fb_done(0);
                        msg = pmd_tracking_state_first_burst;
                        pmd_state = pmd_tracking_state_first_burst;
                        change_state = 1;
                    }
                }

                break;
            }
            case PMD_GPON_STATE_OPERATION_O5:
            {
                /* PMD moves to O5 by himself, just save the state */
                //pmd_state = pmd_tracking_state_tracking;

                /* Enable force after ranging */
                force_msg = 1;
                rc = pmd_msg_handler(hmid_force_enable, &force_msg, sizeof(uint16_t));
                if (rc)
                    BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail in hmid_force_enable \n");

                num_of_ranging_attempts = 0;

                break;
            }
            default:
            {
                switch(old_state)
                {
                    case PMD_GPON_STATE_STANDBY_O2:
                    case PMD_GPON_STATE_SERIAL_NUMBER_O3:
                    case PMD_GPON_STATE_RANGING_O4:
                    case PMD_GPON_STATE_POPUP_O6:
                    {
                        if (new_state == PMD_GPON_STATE_INIT_O1 || new_state == PMD_GPON_STATE_EMERGENCY_STOP_O7 )
                        {
                            msg = pmd_tracking_state_disabled;
                            pmd_state = pmd_tracking_state_disabled;
                            change_state = 1;
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
        }
    }
    else if(pmd_board_type == pmd_epon_board)
    {
        change_state = 1;

        switch((rdpa_epon_link_mpcp_state)new_state)
        {
        case rdpa_epon_link_unregistered:
            msg = pmd_tracking_state_disabled;
            break;
        case rdpa_epon_link_awaiting_register:
            msg = pmd_tracking_state_first_burst;
            break;
        case rdpa_epon_link_in_service:
            msg = pmd_tracking_state_tracking;
            break;
        default:
            break;
        }
    }

    pmd_stack_state = new_state;

    if (change_state && !disable_tracking_flag)
    {
        if (pmd_board_type == pmd_gpon_board)
            BCM_LOG_DEBUG(BCM_LOG_ID_PMD, " change gpon state : old  state %d new state %d msg %d\n",old_state, new_state, msg);
        else
            BCM_LOG_DEBUG(BCM_LOG_ID_PMD, " change epon state : new state %d msg %d\n",new_state, msg);

        rc = pmd_msg_handler(hmid_laser_tracking_set, &msg, sizeof(uint16_t));
        if (rc)
            BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Fail to change tracking state \n");
    }

    return rc;
}
EXPORT_SYMBOL(pmd_dev_change_tracking_state);

int pmd_dev_isr_callback(uint32_t *pmd_ints)
{
    uint16_t msg = 0;
    unsigned short pmd_irq;
    int rc = 0;

#ifdef PMD_FULL_DEBUG
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");
#endif
    /* read and clear pmd interrupt */
    rc = pmd_msg_handler(hmid_alarms_get, &msg, sizeof(uint16_t));
    if (rc)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD, " Error clearing PMD interrupt \n");
        goto exit;
    }

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "msg %x \n", msg);

    rc = pmd_dev_alarm(msg, pmd_ints);
    if (rc)
        goto exit;

    if (alarm_read_flag)
    {
        alarm_msg = msg;
        alarm_read_flag = 0;
    }
exit:
    BpGetPmdAlarmExtIntr(&pmd_irq);
    ext_irq_enable(pmd_irq, 1);
    return rc;
}
EXPORT_SYMBOL(pmd_dev_isr_callback);


int pmd_dev_poll_epon_alarm(void)
{
    alarm_read_flag = 1;
    return alarm_msg;
}
EXPORT_SYMBOL(pmd_dev_poll_epon_alarm);

static void pmd_dev_epon_alarm_handler(unsigned long arg)
{
    uint32_t pmd_ints;
    
    pmd_dev_isr_callback(&pmd_ints);
}

#if defined(CONFIG_BCM96838)
static void pmd_dev_sd_handler(unsigned long arg)
{
    unsigned short pmd_irq;
    uint32_t pkpos, pkneg;

    RU_REG_READ(LRL, LRL_RSSI_PEAKPOS, &pkpos);
    RU_REG_READ(LRL, LRL_RSSI_PEAKNEG, &pkneg);

    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "PMD: LOS Interrupt !! pkpos = %x pkneg = %x \n",pkpos, pkneg);

    BpGetWanSignalDetectedExtIntr(&pmd_irq);
    ext_irq_enable(pmd_irq, 1);
}
#endif

static int pmd_dev_get_board_type(void)
{
    int  count;
    char wan_type_buf[PSP_BUFLEN_16] = {};

    count = kerSysScratchPadGet((char*)RDPA_WAN_TYPE_PSP_KEY, (char*)wan_type_buf, (int)sizeof(wan_type_buf));
    if (count == 0)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_PMD,"ScratchPad does not contain RdpaWanType");
        return -1;
    }
    if(!strcmp(wan_type_buf ,"GPON"))
    {
        pmd_board_type = pmd_gpon_board;
        return 0;
    }
    else if (!strcmp(wan_type_buf, "EPON"))
    {
        pmd_board_type = pmd_epon_board;
        return 0;
    }
    else if (!strcmp(wan_type_buf, "GBE"))
    {
        pmd_board_type = pmd_gbe_board;
        return 0;
    }
    else if (!strcmp(wan_type_buf ,"AUTO"))
    {
        pmd_board_type = pmd_auto_detect;
        return 0;
    }

    return -1;
}

static int pmd_dev_fault_isr(int irq, void *priv)
{
    if (pmd_board_type == pmd_gpon_board)
    {
        if (gpon_isr_wrapper)
            gpon_isr_wrapper();
        else
            tasklet_hi_schedule(&fault_tasklet);  /* use the tasklet to call the isr directly */
    }
    else if (pmd_board_type == pmd_epon_board)
        tasklet_hi_schedule(&fault_tasklet);

    return IRQ_HANDLED;
}

#if defined(CONFIG_BCM96838)
static int pmd_dev_sd_isr(int irq, void *priv)
{
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");

    tasklet_hi_schedule(&sd_tasklet);

    return IRQ_HANDLED;
}
#endif

static int pmd_dev_int_connect(void)
{
    int rc;
    unsigned short pmd_irq;
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");

    /*connect fault interrupt, register pmd_dev_fault_isr */
    if (BpGetPmdAlarmExtIntr(&pmd_irq))
        return -1;

    rc = ext_irq_connect(pmd_irq, (void*)0, (FN_HANDLER)pmd_dev_fault_isr);
    if (rc)
        return rc;

    tasklet_init(&fault_tasklet, pmd_dev_epon_alarm_handler, 0);
	return 0;
}


#if defined(CONFIG_BCM96838)
static int pmd_dev_sd_int_connect(void)
{
    int rc = 0;
    unsigned short pmd_irq;
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");

    tasklet_init(&sd_tasklet, pmd_dev_sd_handler, 0);

    /*connect SD interrupt in B0, register pmd_sd_isr */
    if (BpGetWanSignalDetectedExtIntr(&pmd_irq))
        return -1;

    rc = ext_irq_connect(pmd_irq, (void*)3, (FN_HANDLER)pmd_dev_sd_isr);

    return rc;
}
#endif

static void pmd_dev_timer_handler(unsigned long data)
{
    /* call thread */
    pmd_thread_sched = 1;
    wake_up_interruptible(&pmd_thread_event);

    /*reset timer */
    mod_timer(&periodic_timer, jiffies + msecs_to_jiffies(PMD_TIMER_INTERVAL));
}

static void pmd_dev_timer_init(void)
{
    setup_timer(&periodic_timer, pmd_dev_timer_handler, 0);
    mod_timer(&periodic_timer, jiffies + msecs_to_jiffies(PMD_TIMER_INTERVAL));
}

static void pmd_dev_exit(void)
{
    unsigned short pmd_irq;
    BCM_LOG_DEBUG(BCM_LOG_ID_PMD, "\n");

    BpGetPmdAlarmExtIntr(&pmd_irq);
    ext_irq_enable(pmd_irq, 0);

    del_timer(&periodic_timer);

    unregister_chrdev(PMD_DEV_MAJOR, PMD_DEV_CLASS);

    tasklet_kill(&fault_tasklet);
    if (pmd_timer_thread)
        kthread_stop(pmd_timer_thread);

#if defined(CONFIG_BCM96838)
    BpGetWanSignalDetectedExtIntr(&pmd_irq);
    ext_irq_enable(pmd_irq, 0);
    tasklet_kill(&sd_tasklet);
#endif		
}

static int pmd_dev_init(void)
{
    int rc = 0;
    uint16_t OpticsTypeFlag = 0;

    if (BpGetGponOpticsType(&OpticsTypeFlag))
    {
        printk(KERN_ERR "PMD board profile not configured and optics type cannot be determined.\n");
        return -1;
    }
    else if (OpticsTypeFlag != BP_GPON_OPTICS_TYPE_PMD)
    {
        printk(KERN_ERR "Board not populated with PMD \n");
        return 0;
    }

    pmd_dev_get_board_type();

    msg_system_init();

    rc = pmd_cal_file_init();
    if (rc)
        goto exit;

    rc = pmd_dev_int_connect();
    if (rc)
        goto exit;

    wan_register_pmd_prbs_callback(pmd_dev_enable_prbs_or_misc_mode);

    rc = pmd_dev_init_seq();
    if (rc)
        goto exit;

    rc = pmd_temp2apd_file_op(NULL);
    if (rc)
        goto exit;

#if defined(CONFIG_BCM96838)
    rc = pmd_dev_sd_int_connect();
    if (rc)
        goto exit;
#endif

    /* allow chrdev only after init seq finished so that no locks are needed in init*/
    rc = register_chrdev(PMD_DEV_MAJOR, PMD_DEV_CLASS, &pmd_file_ops);
    if (rc)
        goto exit;

    pmd_dev_timer_init();

    rc = pmd_dev_thread_init();
    if (rc)
        goto exit;

    mutex_init(&pmd_dev_lock);
    return 0;

exit:
    pmd_dev_exit();
    return rc;
}

module_init(pmd_dev_init);
module_exit(pmd_dev_exit);


MODULE_DESCRIPTION("Pmd Device driver");
MODULE_LICENSE("Proprietary");
