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
#include <linux/mm.h>
#include <linux/in.h>
#include <linux/init.h>
#include <asm/uaccess.h> /* for copy_from_user */
#include <linux/version.h>

#include "bcm_omcipm_threshold.h"

/**
 * Local Threshold Data Table variables
 **/

static BCM_COMMON_DECLARE_LL(thresholdLL);
static struct kmem_cache *thresholdCache;

/**
 * Local Threshold Data Table functions
 **/

static inline PBCM_OMCIPM_THRESHOLD_ENTRY allocThresholdEntry(void)
{
    return kmem_cache_alloc(thresholdCache, GFP_KERNEL);
}

static inline void freeThresholdEntry(PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry)
{
    kmem_cache_free(thresholdCache, thresholdEntry);
}

static void freeAllThresholdEntry(void)
{
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry, nextThresholdEntry;

    thresholdEntry = BCM_COMMON_LL_GET_HEAD(thresholdLL);

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Cleaning-up Threshold Data Table (0x%08lX)", (uint32)thresholdEntry);

    while (thresholdEntry)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Cleaning-up Threshold Entry %d", thresholdEntry->thresholdData.thresholdId);
        nextThresholdEntry = BCM_COMMON_LL_GET_NEXT(thresholdEntry);
        BCM_COMMON_LL_REMOVE(&thresholdLL, thresholdEntry);
        freeThresholdEntry(thresholdEntry);
        thresholdEntry = nextThresholdEntry;
    };
}

static PBCM_OMCIPM_THRESHOLD_ENTRY findThresholdEntryById(UINT16 thresholdId)
{
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry = BCM_COMMON_LL_GET_HEAD(thresholdLL);

    while (thresholdEntry)
    {
        if (thresholdEntry->thresholdData.thresholdId == thresholdId)
        {
            break;
        }
        thresholdEntry = BCM_COMMON_LL_GET_NEXT(thresholdEntry);
    };

    return thresholdEntry;
}

static BCM_OMCI_PM_STATUS omcipm_threshold_createEntry(PBCM_OMCI_PM_THRESHOLD_DATA threshold)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry = NULL;

    /* allocate a new threshold entry */
    thresholdEntry = allocThresholdEntry();

    if (thresholdEntry != NULL)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Create Threshold Entry Id: %d", threshold->thresholdId);
        thresholdEntry->thresholdData.thresholdId      = threshold->thresholdId;
        thresholdEntry->thresholdData.thresholdValue1  = threshold->thresholdValue1;
        thresholdEntry->thresholdData.thresholdValue2  = threshold->thresholdValue2;
        thresholdEntry->thresholdData.thresholdValue3  = threshold->thresholdValue3;
        thresholdEntry->thresholdData.thresholdValue4  = threshold->thresholdValue4;
        thresholdEntry->thresholdData.thresholdValue5  = threshold->thresholdValue5;
        thresholdEntry->thresholdData.thresholdValue6  = threshold->thresholdValue6;
        thresholdEntry->thresholdData.thresholdValue7  = threshold->thresholdValue7;
        thresholdEntry->thresholdData.thresholdValue8  = threshold->thresholdValue8;
        thresholdEntry->thresholdData.thresholdValue9  = threshold->thresholdValue9;
        thresholdEntry->thresholdData.thresholdValue10 = threshold->thresholdValue10;
        thresholdEntry->thresholdData.thresholdValue11 = threshold->thresholdValue11;
        thresholdEntry->thresholdData.thresholdValue12 = threshold->thresholdValue12;
        thresholdEntry->thresholdData.thresholdValue13 = threshold->thresholdValue13;
        thresholdEntry->thresholdData.thresholdValue14 = threshold->thresholdValue14;
        BCM_COMMON_LL_APPEND(&thresholdLL, thresholdEntry);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not allocate Threshold Entry memory");
        ret = OMCI_PM_STATUS_ALLOC_ERROR;
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_threshold_deleteEntryById(UINT16 thresholdId)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry = NULL;

    thresholdEntry = findThresholdEntryById(thresholdId);

    if (thresholdEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Freeing Threshold Entry Id: %d", thresholdEntry->thresholdData.thresholdId);
        BCM_COMMON_LL_REMOVE(&thresholdLL, thresholdEntry);
        freeThresholdEntry(thresholdEntry);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find Threshold Id %d", thresholdId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_threshold_getEntry(PBCM_OMCI_PM_THRESHOLD_DATA threshold)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry = NULL;

    thresholdEntry = findThresholdEntryById(threshold->thresholdId);

    if (thresholdEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Get Threshold Entry Id: %d", threshold->thresholdId);
        threshold->thresholdValue1  = thresholdEntry->thresholdData.thresholdValue1;
        threshold->thresholdValue2  = thresholdEntry->thresholdData.thresholdValue2;
        threshold->thresholdValue3  = thresholdEntry->thresholdData.thresholdValue3;
        threshold->thresholdValue4  = thresholdEntry->thresholdData.thresholdValue4;
        threshold->thresholdValue5  = thresholdEntry->thresholdData.thresholdValue5;
        threshold->thresholdValue6  = thresholdEntry->thresholdData.thresholdValue6;
        threshold->thresholdValue7  = thresholdEntry->thresholdData.thresholdValue7;
        threshold->thresholdValue8  = thresholdEntry->thresholdData.thresholdValue8;
        threshold->thresholdValue9  = thresholdEntry->thresholdData.thresholdValue9;
        threshold->thresholdValue10 = thresholdEntry->thresholdData.thresholdValue10;
        threshold->thresholdValue11 = thresholdEntry->thresholdData.thresholdValue11;
        threshold->thresholdValue12 = thresholdEntry->thresholdData.thresholdValue12;
        threshold->thresholdValue13 = thresholdEntry->thresholdData.thresholdValue13;
        threshold->thresholdValue14 = thresholdEntry->thresholdData.thresholdValue14;
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find Threshold Id %d", threshold->thresholdId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_threshold_setEntry(PBCM_OMCI_PM_THRESHOLD_DATA threshold)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_THRESHOLD_ENTRY thresholdEntry = NULL;

    thresholdEntry = findThresholdEntryById(threshold->thresholdId);

    if (thresholdEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Set Threshold Entry Id: %d", threshold->thresholdId);
        thresholdEntry->thresholdData.thresholdValue1  = threshold->thresholdValue1;
        thresholdEntry->thresholdData.thresholdValue2  = threshold->thresholdValue2;
        thresholdEntry->thresholdData.thresholdValue3  = threshold->thresholdValue3;
        thresholdEntry->thresholdData.thresholdValue4  = threshold->thresholdValue4;
        thresholdEntry->thresholdData.thresholdValue5  = threshold->thresholdValue5;
        thresholdEntry->thresholdData.thresholdValue6  = threshold->thresholdValue6;
        thresholdEntry->thresholdData.thresholdValue7  = threshold->thresholdValue7;
        thresholdEntry->thresholdData.thresholdValue8  = threshold->thresholdValue8;
        thresholdEntry->thresholdData.thresholdValue9  = threshold->thresholdValue9;
        thresholdEntry->thresholdData.thresholdValue10 = threshold->thresholdValue10;
        thresholdEntry->thresholdData.thresholdValue11 = threshold->thresholdValue11;
        thresholdEntry->thresholdData.thresholdValue12 = threshold->thresholdValue12;
        thresholdEntry->thresholdData.thresholdValue13 = threshold->thresholdValue13;
        thresholdEntry->thresholdData.thresholdValue14 = threshold->thresholdValue14;
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find Threshold Id %d", threshold->thresholdId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

/**
 * Global Threshold Data Table functions
 **/

int omcipm_threshold_init(void)
{
    /* create a slab cache for threshold data entry */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
    thresholdCache = kmem_cache_create("omcipm_threshold_entry",
                                       sizeof(BCM_OMCIPM_THRESHOLD_ENTRY),
                                       0, /* align */
                                       0, /* flags */
                                       NULL); /* ctor */
#else
    thresholdCache = kmem_cache_create("omcipm_threshold_entry",
                                       sizeof(BCM_OMCIPM_THRESHOLD_ENTRY),
                                       0, /* align */
                                       0, /* flags */
                                       NULL, /* ctor */
                                       NULL); /* dtor */
#endif

    if (thresholdCache == NULL)
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create Threshold Data Entry Cache");
        return -ENOMEM;
    }

    BCM_COMMON_LL_INIT(&thresholdLL);

    return 0;
}

void omcipm_threshold_exit(void)
{
    freeAllThresholdEntry();
    kmem_cache_destroy(thresholdCache);
}

PBCM_OMCIPM_THRESHOLD_ENTRY omcipm_threshold_find(UINT16 thresholdId)
{
    return findThresholdEntryById(thresholdId);
}

/**
 * IOCTL Threshold functions
 **/

void omcipm_createThreshold(unsigned long arg)
{
    STS_OMCI_PM_THRESHOLD_DATA KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    copy_from_user(&KArg.thresholdData,
                   &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->thresholdData,
                   sizeof(KArg.thresholdData));

    KArg.omcipmStatus = omcipm_threshold_createEntry(&KArg.thresholdData);

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->omcipmStatus);
}

void omcipm_deleteThreshold(unsigned long arg)
{
    STS_OMCI_PM_THRESHOLD_DATA KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    copy_from_user(&KArg.thresholdData,
                   &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->thresholdData,
                   sizeof(KArg.thresholdData));

    KArg.omcipmStatus = omcipm_threshold_deleteEntryById(KArg.thresholdData.thresholdId);

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->omcipmStatus);
}

void omcipm_getThreshold(unsigned long arg)
{
    STS_OMCI_PM_THRESHOLD_DATA KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    copy_from_user(&KArg.thresholdData,
                   &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->thresholdData,
                   sizeof(KArg.thresholdData));

    KArg.omcipmStatus = omcipm_threshold_getEntry(&KArg.thresholdData);

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->omcipmStatus);

    copy_to_user(&((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->thresholdData,
                 &KArg.thresholdData,
                 sizeof(KArg.thresholdData));
}

void omcipm_setThreshold(unsigned long arg)
{
    STS_OMCI_PM_THRESHOLD_DATA KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    copy_from_user(&KArg.thresholdData,
                   &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->thresholdData,
                   sizeof(KArg.thresholdData));

    KArg.omcipmStatus = omcipm_threshold_setEntry(&KArg.thresholdData);

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_THRESHOLD_DATA)arg)->omcipmStatus);
}

