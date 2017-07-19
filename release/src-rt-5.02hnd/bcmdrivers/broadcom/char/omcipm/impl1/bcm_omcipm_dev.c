/*
<:copyright-broadcom

 Copyright (c) 2007 Broadcom Corporation
 All Rights Reserved
 No portions of this material may be reproduced in any form without the
 written permission of:
          Broadcom Corporation
          5300 California Avenue
          Irvine, California 92617
 All information contained in this document is Broadcom Corporation
 company private, proprietary, and trade secret.

:>
*/

#include <linux/module.h>

#include "bcm_omcipm_dev.h"
#include "bcm_omcipm_llist.h"
#include <linux/bcm_log.h>

#include <linux/version.h>

static PBCM_OMCI_PM_DEVICE omcipmDevicesList[BCM_OMCI_PM_DEVICE_MAX];
static struct kmem_cache *devCache;

static PBCM_OMCI_PM_DEVICE allocOmciPmDevice(BCM_OMCI_PM_DEVICE_ID omcipmDeviceId)
{
    PBCM_OMCI_PM_DEVICE dev = kmem_cache_alloc(devCache, GFP_KERNEL);

    if (dev != NULL)
    {
        switch (omcipmDeviceId)
        {
            case BCM_OMCI_PM_DEVICE_GPON:
                omcipm_gemport_init();
                omcipm_fec_init();
                break;
            case BCM_OMCI_PM_DEVICE_ENET:
                omcipm_enet_init();
                omcipm_enet2_init();
                omcipm_enet3_init();
                break;
            default:
                break;
        }
    }

    return dev;
}

static void freeOmciPmDevice(PBCM_OMCI_PM_DEVICE dev)
{
    BCM_LOG_NOTICE(BCM_LOG_ID_OMCIPM, "Free OMCI PM Device ID %d", dev->omcipmDeviceId);

    // free memory for device table
    switch (dev->omcipmDeviceId)
    {
        case BCM_OMCI_PM_DEVICE_GPON:
             omcipm_gemport_exit();
             omcipm_fec_exit();
             break;
        case BCM_OMCI_PM_DEVICE_ENET:
             omcipm_enet_exit();
             omcipm_enet2_exit();
             omcipm_enet3_exit();
             break;
         default:
             break;
    }

    // free memory for this device
    kmem_cache_free(devCache, dev);
    dev = NULL;
}

/**
 * Functions are used by OMCI PM driver only
 **/

BCM_OMCI_PM_STATUS omcipm_dev_init(void)
{
    UINT16 i = 0;
    BCM_OMCI_PM_STATUS rv = OMCI_PM_STATUS_SUCCESS;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    for (i = 0; i < BCM_OMCI_PM_DEVICE_MAX; i++)
    {
        omcipmDevicesList[i] = NULL;
    }


    /* create a slab cache for OMCI PM device */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
devCache = kmem_cache_create("omcipm_device",
                                 sizeof(BCM_OMCI_PM_DEVICE),
                                 0, /* align */
                                 0, /* flags */
                                 NULL); /* ctor */
#else
    devCache = kmem_cache_create("omcipm_device",
                                 sizeof(BCM_OMCI_PM_DEVICE),
                                 0, /* align */
                                 0, /* flags */
                                 NULL, /* ctor */
                                 NULL); /* dtor */
#endif
    if (devCache == NULL)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create OMCI PM device Cache");
        rv = OMCI_PM_STATUS_RESOURCE_ERROR;
    }

    return rv;
}

void omcipm_dev_cleanup(void)
{
    UINT16 i = 0;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    for (i = 0; i < BCM_OMCI_PM_DEVICE_MAX; i++)
    {
        if (omcipmDevicesList[i] != NULL)
        {
            // free device table and its memory
            freeOmciPmDevice(omcipmDevicesList[i]);
        }
    }
    kmem_cache_destroy(devCache);
    devCache = NULL;
}

BOOL omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ID omcipmDeviceId)
{
    BOOL rv = FALSE;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    if (omcipmDeviceId < BCM_OMCI_PM_DEVICE_MAX &&
        omcipmDevicesList[omcipmDeviceId] != NULL)
    {
        //BCM_LOG_NOTICE(BCM_LOG_ID_OMCIPM, "OMCI PM Device ID %d is registered", omcipmDeviceId);
        rv = TRUE;
    }
    else
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "OMCI PM Device ID %d is NOT registered", omcipmDeviceId);
    }

    return rv;
}

BCM_OMCI_PM_STATUS omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_ID omcipmDeviceId,
                                          UINT16 physPortId,
                                          void   *counters,
                                          BOOL   reset)
{
    BCM_OMCI_PM_STATUS rv = OMCI_PM_STATUS_SUCCESS;

    if (omcipmDeviceId < BCM_OMCI_PM_DEVICE_MAX &&
        omcipmDevicesList[omcipmDeviceId] != NULL)
    {
        rv = omcipmDevicesList[omcipmDeviceId]->getCounters(physPortId, counters, reset);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "OMCI PM Device ID %d is NOT registered", omcipmDeviceId);
        rv = OMCI_PM_STATUS_NOT_REGISTERED;
    }

    return rv;
}

/**
 * Functions are exported by OMCI PM driver and used by other drivers
 **/

BCM_OMCI_PM_STATUS bcm_omcipm_dev_register(PBCM_OMCI_PM_DEVICE device)
{
    BCM_OMCI_PM_STATUS rv = OMCI_PM_STATUS_SUCCESS;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    if (device->omcipmDeviceId < BCM_OMCI_PM_DEVICE_MAX)
    {
        if (omcipmDevicesList[device->omcipmDeviceId] == NULL)
        {
            // allocate device table and its memory
            omcipmDevicesList[device->omcipmDeviceId] = allocOmciPmDevice(device->omcipmDeviceId);
        }

        if (omcipmDevicesList[device->omcipmDeviceId] != NULL)
        {
            BCM_LOG_NOTICE(BCM_LOG_ID_OMCIPM, "OMCI PM Device ID %d is registered", device->omcipmDeviceId);
            memcpy(omcipmDevicesList[device->omcipmDeviceId], device, sizeof(BCM_OMCI_PM_DEVICE));
        }
        else
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to allocate memory for device ID %d", device->omcipmDeviceId);
            rv = OMCI_PM_STATUS_ALLOC_ERROR;
        }
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to register device since its ID %d is invalid", device->omcipmDeviceId);
        rv = OMCI_PM_STATUS_NOT_REGISTERED;
    }

    return rv;
}

BCM_OMCI_PM_STATUS bcm_omcipm_dev_unregister(BCM_OMCI_PM_DEVICE_ID omcipmDeviceId)
{
    BCM_OMCI_PM_STATUS rv = OMCI_PM_STATUS_SUCCESS;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "");

    if (omcipmDeviceId < BCM_OMCI_PM_DEVICE_MAX)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_OMCIPM, "OMCI PM Device ID %d is unregistered", omcipmDeviceId);
        // free memory
        freeOmciPmDevice(omcipmDevicesList[omcipmDeviceId]);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to unregister device since its ID %d is invalid", omcipmDeviceId);
        rv = OMCI_PM_STATUS_NOT_REGISTERED;
    }

    return rv;
}

EXPORT_SYMBOL(bcm_omcipm_dev_register);
EXPORT_SYMBOL(bcm_omcipm_dev_unregister);
