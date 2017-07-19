/*
* <:copyright-BRCM:2013:proprietary:standard
* 
*    Copyright (c) 2013 Broadcom 
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
:>
*/

/*----- Includes -----*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/notifier.h>
#include "fbond.h"

/*----- Defines -----*/

/* Override global debug and/or assert compilation for Driver layer */
#define CLRsys              CLRy
#define DBGsys              "FBOND:DRV"
#if !defined(CC_CONFIG_FBOND_DRV_DBGLVL)
#undef PKT_DBG_SUPPORTED
#endif
#if defined(PKT_DBG_SUPPORTED)
static int pktDbgLvl = CC_CONFIG_FBOND_DRV_DBGLVL;
#endif
#if !defined(CC_FBOND_DRV_ASSERT)
#undef PKT_ASSERT_SUPPORTED
#endif
#if defined(CC_CONFIG_FBOND_COLOR)
#define PKT_DBG_COLOR_SUPPORTED
#endif
#include "pktDbg.h"
#if defined(CC_CONFIG_FBOND_DEBUG)    /* Runtime debug level setting */
int fbondDrvDebug(int lvl) { dbg_config( lvl ); return lvl; }
#endif


/*----- Exported callbacks from fbond.c ONLY to be invoked by fbonddrv */
extern int fbond_dev_add(uint32_t groupindex, uint32_t ifindex);
extern int fbond_dev_delete(uint32_t groupindex, uint32_t ifindex);
extern int fbond_dev_remove(void *dev_p);
extern int fbond_dev_token_update(uint32_t groupindex, uint32_t ifindex, uint32_t tokens, uint32_t max_tokens);
extern void fbond_status(void);
extern void fbond_slice(unsigned long data, uint32_t slice_ix);
extern int  fbond_max_ent(void);
extern void fbond_test(uint32_t num_flows);

extern int  fbond_construct(void);
extern void fbond_destruct(void);
extern int  fbondDebug(int debug_level);

/*----- Forward declarations -----*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
static int  fbondDrvIoctl(struct inode *inode, struct file *filep,
                           unsigned int command, unsigned long arg);
#else
static long fbondDrvIoctl(struct file *filep, unsigned int command, 
                           unsigned long arg);
#endif

static int  fbondDrvOpen(struct inode *inode, struct file *filp);

static void fbondDrvTimer(unsigned long data);

static int  fbondDrvNetDevNotifier(struct notifier_block *this,
                                    unsigned long event, void *ptr);


/*----- Globals -----*/
typedef struct {
    uint32_t slice_ix;          /* slice number processed */
    uint32_t num_slices;        /* total number of slice timers */
    uint32_t slice_period;      /* timer period of each slice timer */

    struct timer_list timer;

    struct file_operations fops;
    struct notifier_block netdev_notifier;

    int     interval;           /* Timer interval */

    spinlock_t  locks[FLOWBOND_MAX_GROUP];
    
} __attribute__((aligned(16))) FBondDrv_t;


static FBondDrv_t fbondDrv_g = {
    .fops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
        .ioctl          = fbondDrvIoctl,
#else
        .unlocked_ioctl = fbondDrvIoctl,
#if defined(CONFIG_COMPAT)
        .compat_ioctl = fbondDrvIoctl,
#endif
#endif
        .open           = fbondDrvOpen
    },

    .netdev_notifier = {
        .notifier_call  = fbondDrvNetDevNotifier,
    },

    .interval           = FBOND_REFRESH_INTERVAL,
};


/*
 *------------------------------------------------------------------------------
 * Function Name: fbondDrvIoctl
 * Description  : Main entry point to handle user applications IOCTL requests
 *                Flow Bond Utility.
 * Returns      : 0 - success or error
 *------------------------------------------------------------------------------
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
static int fbondDrvIoctl(struct inode *inode, struct file *filep,
                          unsigned int command, unsigned long arg)
#else
static long fbondDrvIoctl(struct file *filep, unsigned int command, 
                           unsigned long arg)
#endif
{
    FbondIoctl_t cmd;
    int ret = FBOND_SUCCESS;

    if ( command > FBOND_IOCTL_INVALID )
        cmd = FBOND_IOCTL_INVALID;
    else
        cmd = (FbondIoctl_t)command;

    switch ( cmd )
    {
        case FBOND_IOCTL_STATUS :
        {
            fbond_status();
            break;
        }

        case FBOND_IOCTL_ENABLE :
        {
            break;
        }

        case FBOND_IOCTL_DISABLE :
        {
            break;
        }

        case FBOND_IOCTL_INTERVAL :
        {
            del_timer( &fbondDrv_g.timer );
            fbondDrv_g.interval = arg;
            fbondDrv_g.slice_period = (fbondDrv_g.interval/fbondDrv_g.num_slices);
            fbondDrv_g.timer.expires = jiffies + fbondDrv_g.slice_period;
            add_timer( &fbondDrv_g.timer );
            break;
        }

        case FBOND_IOCTL_ADDIF :
        {
            FbondIoctlAddDeleteIf_t addIfArgs;

            copy_from_user((void*) &addIfArgs, (void*) arg, sizeof(FbondIoctlAddDeleteIf_t));
            ret = fbond_dev_add(addIfArgs.groupindex, addIfArgs.ifindex);
            break;
        }

        case FBOND_IOCTL_DELETEIF :
        {
            FbondIoctlAddDeleteIf_t deleteIfArgs;

            copy_from_user((void*) &deleteIfArgs, (void*) arg, sizeof(FbondIoctlAddDeleteIf_t));
            ret = fbond_dev_delete(deleteIfArgs.groupindex, deleteIfArgs.ifindex);
            break;
        }

        case FBOND_IOCTL_TOKENS :
        {
            FbondIoctlTokens_t tokensArgs;

            copy_from_user((void*) &tokensArgs, (void*) arg, sizeof(FbondIoctlTokens_t));
            ret = fbond_dev_token_update(tokensArgs.groupindex, tokensArgs.ifindex, tokensArgs.tokens, tokensArgs.max_tokens);
            break;
        }

        case FBOND_IOCTL_TEST :
        {
            fbond_test(arg);
            fbond_status();
            break;
        }

        case FBOND_IOCTL_DEBUG :
        {
#if defined(CC_CONFIG_FBOND_DEBUG)
            int layer = (arg>>8) & 0xFF;
            int level = arg & 0xFF;

            switch ( layer )
            {
                case FBOND_DBG_DRV_LAYER: 
                    ret = fbondDrvDebug( level );
                    break;

                case FBOND_DBG_FB_LAYER:  
                    ret = fbondDebug( level );
                    break;

                default: 
                    ret = FBOND_ERROR;
            }
#else
            fb_error( "CC_CONFIG_FBOND_DEBUG not defined");
            ret = FBOND_ERROR;
#endif
            break;
        }

        default :
        {
            fb_error( "Invalid cmd[%u]", command );
            ret = FBOND_ERROR;
        }
    }

    return ret;

} /* fbondDrvIoctl */

/*
 *------------------------------------------------------------------------------
 * Function Name: fbondDrvOpen
 * Description  : Called when a user application opens this device.
 * Returns      : 0 - success
 *------------------------------------------------------------------------------
 */
int fbondDrvOpen(struct inode *inode, struct file *filp)
{
    dbgl_print( DBG_EXTIF, "Access Flow Bond Char Device" );
    return FBOND_SUCCESS;
} /* fbondDrvOpen */


/*
 *------------------------------------------------------------------------------
 * Function Name: fbondDrvTimer
 * Description  : Periodic slice timer callback. Passes timeout to fbond.o
 * Flow Bond timer interval is divided into a number of slice intervals.
 * The distribution of fbond entries to a slice interval is based on slice
 * mask, which is number of slices minus 1. The number of slices is max 
 * flow bond entries divided by number of entries served in one slice timer.
 *------------------------------------------------------------------------------
 */
void fbondDrvTimer(unsigned long data)
{
    fbondDrv_g.timer.data++;

    fbond_slice(fbondDrv_g.timer.data, fbondDrv_g.slice_ix);

    fbondDrv_g.slice_ix++;
    if (fbondDrv_g.slice_ix >= fbondDrv_g.num_slices)
        fbondDrv_g.slice_ix = 0;

    fbondDrv_g.timer.expires = jiffies + fbondDrv_g.slice_period;
    add_timer( &fbondDrv_g.timer );
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fbondDrvNetDevNotifier
 * Description  : Receive notifications of link state changes and device down
 *                and forward them to fbond.o
 *------------------------------------------------------------------------------
 */
int fbondDrvNetDevNotifier(struct notifier_block *this,
                            unsigned long event, void *dev_p)
{
    struct net_device *dev = NETDEV_NOTIFIER_GET_DEV(dev_p);

    dbgl_print( DBG_INTIF, "dev <%p> event<%lu>", dev, event); 

    switch (event) {
        case NETDEV_CHANGE:
            break;

        case NETDEV_GOING_DOWN:
            fbond_dev_remove(dev);
            break;

        case NETDEV_CHANGEMTU:
            break;
            
        default:
            break;
    }

    return NOTIFY_DONE;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: flowbond_construct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : Error or FBOND_DRV_MAJOR
 *------------------------------------------------------------------------------
 */

static int __init flowbond_construct(void)
{
    int groupIdx;
    
    dbg_config( CC_CONFIG_FBOND_DRV_DBGLVL );

    dbgl_func( DBG_BASIC );

    /*
     * ========================
     * Initialize fbond state
     * ========================
     */
    if ( fbond_construct() == FBOND_ERROR )
        return FBOND_ERROR;

    /* Register a character device for Ioctl handling */
    if ( register_chrdev(FBOND_DRV_MAJOR, FBOND_DRV_NAME, &fbondDrv_g.fops) )
    {
        print( CLRerr "%s Unable to get major number <%d>" CLRnl,
                  __FUNCTION__, FBOND_DRV_MAJOR);
        return FBOND_ERROR;
    }

    print( CLRbold FLOWBOND_MODNAME " Char Driver " FLOWBOND_VER_STR
                   " Registered<%d>" CLRnl, FBOND_DRV_MAJOR );

    /* Start a periodic OS refresh timer  */
    init_timer( &fbondDrv_g.timer );       /* initialize periodic timer */

    fbondDrv_g.slice_ix = 0;
    fbondDrv_g.num_slices = fbond_max_ent()/FBOND_SLICE_MAX_ENT_COUNT;
    fbondDrv_g.slice_period = (fbondDrv_g.interval/fbondDrv_g.num_slices);
    fbondDrv_g.timer.expires = jiffies + fbondDrv_g.slice_period;
    fbondDrv_g.timer.function = fbondDrvTimer;
    fbondDrv_g.timer.data = (unsigned long)0;

    add_timer( &fbondDrv_g.timer );        /* kick start timer */

    /* Register handler for network device notifications */
    register_netdevice_notifier( &fbondDrv_g.netdev_notifier );
    printk( CLRbold FLOWBOND_MODNAME " registered with netdev chain" CLRnl );

    printk( CLRbold "Constructed " FLOWBOND_MODNAME FLOWBOND_VER_STR CLRnl );

    for (groupIdx = 0; groupIdx < FLOWBOND_MAX_GROUP; groupIdx++) {
       spin_lock_init(&fbondDrv_g.locks[groupIdx]);
    }

    return 0;
}

/*
 *------------------------------------------------------------------------------
 * Function Name: flowbond_destruct
 * Description  : Initial function that is called at system startup that
 *                registers this device.
 * Returns      : None
 *
 * CAUTION      : MODULE UNLOADING ONLY FOR LAB !!!
 *
 *------------------------------------------------------------------------------
 */
void flowbond_destruct(void)
{
    dbgl_func( DBG_BASIC );

    del_timer( &fbondDrv_g.timer );/* Delete timer */

    fbond_destruct();              /* fbond.o: reset all flow state */

    /* Un register for network device notifications */
    unregister_netdevice_notifier( &fbondDrv_g.netdev_notifier );
    printk( CLRbold2 FLOWBOND_MODNAME "unregistered with netdev chain" CLRnl );

    /* Un register character device */
    unregister_chrdev( FBOND_DRV_MAJOR, FBOND_DRV_NAME );

    printk( CLRbold2 FLOWBOND_MODNAME " Char Driver " FLOWBOND_VER_STR
                    " Unregistered<%d>" CLRnl, FBOND_DRV_MAJOR );

    printk( CLRbold2 "Destructed " FLOWBOND_MODNAME FLOWBOND_VER_STR CLRnl );
    /*----- OK, Safe to unload now -----*/
}

/*
 *------------------------------------------------------------------------------
 * Function Name: fbond_dev_put
 * Description  : device independent device put
 * Returns      : None
 *------------------------------------------------------------------------------
 */
void flowbond_dev_put(struct net_device *dev)
{
    dev_put(dev);
}


/*
 *------------------------------------------------------------------------------
 * Function Name: flowbond_lockGroup
 * Description  : lock a group.
 * Returns      : None
 *------------------------------------------------------------------------------
 */
void flowbond_lockGroup(uint16_t groupIdx, unsigned long *pFlags)
{
    if (groupIdx >= FLOWBOND_MAX_GROUP) {        
        WARN_ONCE(1, "Invalid groupIdx %d\n", groupIdx);
        groupIdx = 0;
    }
    spin_lock_irqsave(&fbondDrv_g.locks[groupIdx], *pFlags);
}

/*
 *------------------------------------------------------------------------------
 * Function Name: flowbond_unlockGroup
 * Description  : unlock a group
 * Returns      : None
 *------------------------------------------------------------------------------
 */
void flowbond_unlockGroup(uint16_t groupIdx, unsigned long *pFlags)
{
    if (groupIdx >= FLOWBOND_MAX_GROUP) {        
        WARN_ONCE(1, "Invalid groupIdx %d\n", groupIdx);
        groupIdx = 0;
    }
    spin_unlock_irqrestore(&fbondDrv_g.locks[groupIdx], *pFlags);
}



module_init(flowbond_construct);
module_exit(flowbond_destruct);

MODULE_DESCRIPTION(FLOWBOND_MODNAME);
MODULE_VERSION(FLOWBOND_VERSION);

MODULE_LICENSE("Proprietary");
