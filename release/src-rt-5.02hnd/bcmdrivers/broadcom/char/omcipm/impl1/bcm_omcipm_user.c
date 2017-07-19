/*
* <:copyright-BRCM:2007:proprietary:standard
* 
*    Copyright (c) 2007 Broadcom 
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

#include <linux/module.h>
#include <linux/kernel.h> /* printk(), min() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>

#include "bcm_omcipm_api.h"         /* global definitions */
#include "bcm_omcipm_user.h"        /* local definitions */
#include "bcm_omcipm_llist.h"
#include "bcm_omcipm_threshold.h"
#include "bcm_omcipm_gpon.h"
#include "bcm_omcipm_enet.h"
#include <linux/bcm_log.h>

#ifndef OMCI_PM_NUM_CHRDEVS
  #define OMCI_PM_NUM_CHRDEVS  1  /* omcipm0 */
#endif

/* Typedefs. */
typedef void (*OMCIPM_FN_IOCTL) (unsigned long arg);

wait_queue_head_t omcipmEventWaitQueue;    /* OMCIPM event read wait queue */

/**
 * Local Types
 **/
typedef struct {
  struct cdev cdev;                   /* Char device structure */
  bool created;
} BCM_OmcipmChrDev;

/**
 * local variables
 **/
static int omcipmMajor = OMCI_PM_MAJOR;
static int omcipmMinor = 0;
static dev_t omcipmDevNum;    /* Our first device number */

static struct semaphore omcipmSemaphore;
static BCM_OmcipmChrDev *pChrDev = 0;
static bool chrDevRegionRegistered = 0;

/**
 * local functions
 **/

/***************************************************************************
 * Function Name: omcipm_reInitialize
 * Description  : all TCA evetns are unmasked.
 *                all entries of the PM Configuration table are unconfigured.
 *                all thresholds in the PM Threshold table are set to 0.
 * Returns      : None.
 ***************************************************************************/
static void omcipm_reInitialize(unsigned long arg)
{
    BCM_OMCI_PM_STATUS omcipmStatus = OMCI_PM_STATUS_SUCCESS;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    omcipm_reInit();
    put_user(omcipmStatus, &((PSTS_OMCI_PM_STATUS_ONLY)arg)->omcipmStatus);
}

/***************************************************************************
 * Function Name: omcipm_getDriverVersion
 * Description  : retrieve OMCI PM driver version information.
 * Returns      : None.
 ***************************************************************************/
static void omcipm_getDriverVersion(unsigned long arg)
{
    STS_OMCI_PM_DRIVER_VERSION KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_SUCCESS;
    put_user(KArg.omcipmStatus,
             &((PSTS_OMCI_PM_DRIVER_VERSION)arg)->omcipmStatus);
    KArg.version.driverMajor = 0;
    KArg.version.driverMinor = 1;
    KArg.version.driverFix = 0;
    KArg.version.apiMajor = 0;
    KArg.version.apiMinor = 1;
    copy_to_user(&((PSTS_OMCI_PM_DRIVER_VERSION)arg)->version,
                 &KArg.version, sizeof(KArg.version));
}

/***************************************************************************
 * Function Name: omcipm_isNotSupportedYet
 * Description  : return OMCI_PM_STATUS_NOT_SUPPORTED status.
 * Returns      : None.
 ***************************************************************************/
static void omcipm_isNotSupportedYet(unsigned long arg)
{
    BCM_OMCI_PM_STATUS omcipmStatus = OMCI_PM_STATUS_NOT_SUPPORTED;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    // this cast might be dangerous since there might be different PM types
    // but it is safe since all STS PM types have BCM_OMCI_PM_STATUS as 1st struct
    put_user(omcipmStatus, &((PSTS_OMCI_PM_STATUS_ONLY)arg)->omcipmStatus);
}

/**
 * omci fops
 **/

static int omcipm_ioctl(struct inode *ip, struct file *fp,
                        unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int cmdnr = _IOC_NR(cmd);
    OMCIPM_FN_IOCTL ioctlFuncs[] = {omcipm_getDriverVersion,
                                    omcipm_reInitialize,
                                    omcipm_createThreshold,
                                    omcipm_deleteThreshold,
                                    omcipm_getThreshold,
                                    omcipm_setThreshold,
                                    omcipm_createGemPort,
                                    omcipm_deleteGemPort,
                                    omcipm_getGemPort,
                                    omcipm_getCurrentGemPort,
                                    omcipm_setGemPort,
                                    omcipm_createFec,
                                    omcipm_deleteFec,
                                    omcipm_getFec,
                                    omcipm_getCurrentFec,
                                    omcipm_setFec,
                                    omcipm_createENet,
                                    omcipm_deleteENet,
                                    omcipm_getENet,
                                    omcipm_getCurrentENet,
                                    omcipm_setENet,
                                    omcipm_createENet2,
                                    omcipm_deleteENet2,
                                    omcipm_getENet2,
                                    omcipm_getCurrentENet2,
                                    omcipm_setENet2,
                                    omcipm_createENet3,
                                    omcipm_deleteENet3,
                                    omcipm_getENet3,
                                    omcipm_getCurrentENet3,
                                    omcipm_setENet3,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    omcipm_isNotSupportedYet,
                                    NULL} ;

    if (down_interruptible(&omcipmSemaphore))
    {
        return -ERESTARTSYS;
    }

    BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "IoCtl cmd : %d\n", cmdnr);

    if(cmdnr >= 0 && 
       cmdnr < MAX_BCM_OMCI_PM_IOCTL_COMMANDS &&
       ioctlFuncs[cmdnr] != NULL)
    {
        (*ioctlFuncs[cmdnr]) (arg);
    }
    else
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Invalid IoCtl Cmd: %d\n", cmdnr);
        ret = -EINVAL;
    }

    up(&omcipmSemaphore);

    return ret;
}

static unsigned int omcipm_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &omcipmEventWaitQueue, wait);

    if (omcipm_alertThresholdCrossing() == TRUE)
    {
        mask |= POLLIN | POLLRDNORM;  /* readable */
    }

    return mask;
}

static ssize_t omcipm_read(struct file *filp, char __user *buf,
                           size_t count, loff_t *f_pos)
{
    UINT8  data[count];

    if (down_interruptible(&omcipmSemaphore))
    {
        return -ERESTARTSYS;
    }

    memset(data, 0, count);

    count = omcipm_retrieveThresholdCrossing(data, count);
    if (count > 0)
    {
        copy_to_user(buf, (char *)data, count);
    }

    up(&omcipmSemaphore);

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"Read %d bytes", count);

    return count;
}

/*
 * The file operations for the pm device
 */
static struct file_operations omcipm_fops = {
  .owner = THIS_MODULE,
  .ioctl = omcipm_ioctl,
  .poll  = omcipm_poll,
  .read  = omcipm_read
};

/**
 * Local Functions:
 **/

/*
 * Set up the char_dev structure for this device.
 */
static int omcipm_setup_cdev(struct cdev *cdevp, int index, int major, int minor, struct                                      file_operations* fopsp)
{
    int err;
    int devno = MKDEV(major, minor + index);

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");
    BCM_ASSERT(cdevp);

    cdev_init(cdevp, fopsp);

    cdevp->owner = THIS_MODULE;
    /* XXX do we need to assign omci_fops manually ??? */
    //    dev->cdev.ops = &omci_fops;
    /* XXX once cdev_add is called, the driver must be ready to handle
       operations on the device */
    err = cdev_add (cdevp, devno, 1);
    /* Fail gracefully if need be */
    if (err)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to add pm%d device (rv %d)", index, err);
        return err;
    }

    return 0;
}

int __devinit omcipm_user_init(void)
{
    int rv = 0, i = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    omcipmDevNum = MKDEV(omcipmMajor, omcipmMinor);
    rv = register_chrdev_region(omcipmDevNum, OMCI_PM_NUM_CHRDEVS, OMCI_PM_DEVICE_NAME);

    if (rv)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Can't allocate major number %d", omcipmMajor);
        return rv;
    }

    BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,"Device 0x%x Major %d Minor %d", 
                 omcipmDevNum, omcipmMajor, omcipmMinor);

    chrDevRegionRegistered = 1;

    /*
     * allocate the devices -- we can't have them static, as the number
     * can be specified at load time
     * RL FIXME: Not sure whether it still makes sense to have more than 1 CHRDEV here.
     * -> probably OK to simplify to 1 static.
     */
    pChrDev = kmalloc(OMCI_PM_NUM_CHRDEVS * sizeof(BCM_OmcipmChrDev), GFP_KERNEL);
    if (pChrDev == NULL)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Can't alloc memory for BCM_OmcipmChrDev.");
        return -ENOMEM;
    }

    init_waitqueue_head(&omcipmEventWaitQueue);
    init_MUTEX(&omcipmSemaphore);

    memset(pChrDev, 0, OMCI_PM_NUM_CHRDEVS * sizeof(BCM_OmcipmChrDev));
    for (i = 0; i < OMCI_PM_NUM_CHRDEVS; i++)
    {
        rv = omcipm_setup_cdev(&pChrDev[i].cdev, i, omcipmMajor, omcipmMinor, &omcipm_fops);
        if (rv)
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_OMCIPM, "omcipm_setup_cdev failed: %d", rv);
            return rv;
        }
        pChrDev[i].created = 1;
    }

    rv = omcipm_threshold_init();
    rv = omcipm_init();

    return rv;
}

void __exit omcipm_user_cleanup(void) {
    int i = 0;
    dev_t devno = MKDEV(omcipmMajor, omcipmMinor);

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    omcipm_exit();
    omcipm_threshold_exit();

    /* Get rid of our char dev entries */
    if (pChrDev)
    {
        for (i = 0; i < OMCI_PM_NUM_CHRDEVS; i++)
        {
            if (pChrDev[i].created)
            {
                cdev_del(&pChrDev[i].cdev);
            }
        }
        /* free our device memory */
        kfree(pChrDev);
    }

    if (chrDevRegionRegistered)
    {
        unregister_chrdev_region(devno, OMCI_PM_NUM_CHRDEVS);
    }

    omcipmMajor = OMCI_PM_MAJOR;
}
