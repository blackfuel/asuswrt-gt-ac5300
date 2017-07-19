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

#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/version.h>

#include "bcm_omcipm_dev.h"
#include "bcm_omcipm_llist.h"
#include "bcm_omcipm_threshold.h"

// omcipmEventWaitQueue is declared in bcm_omcipm_user.c
extern wait_queue_head_t omcipmEventWaitQueue;

/**
 * Local OMCI PM variables
 **/

static BCM_COMMON_DECLARE_LL(omcipmLL);
static struct timer_list timerCurrent;
static struct kmem_cache *omcipmCache = NULL;
static struct kmem_cache *gemPortCache = NULL;
static struct kmem_cache *fecCache = NULL;
static struct kmem_cache *enetCache = NULL;
static struct kmem_cache *enet2Cache = NULL;
static struct kmem_cache *enet3Cache = NULL;


/**
 * Local OMCI PM functions
 **/

static void addTimer15Mins(PBCM_OMCIPM_ENTRY omcipmEntry);
static void addTimerCurrent(void);

static UINT16 getOmcipmIdFromEntry(PBCM_OMCIPM_ENTRY omcipmEntry)
{
    UINT16 omcipmId = 0;
    PBCM_OMCI_PM_ID_ONLY id = NULL;

    if (omcipmEntry != NULL)
    {
        // this cast might be dangerous since there might be different PM types
        // but it is safe since all PM types have BCM_OMCIPM_PM as 1st struct
        id = (PBCM_OMCI_PM_ID_ONLY)(omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT]);
        if (id != NULL)
        {
            omcipmId = id->omcipm.omcipmId;
        }
    }

    return omcipmId;
}

static UINT16 getThresholdIdFromEntry(PBCM_OMCIPM_ENTRY omcipmEntry)
{
    UINT16 thresholdId = 0;
    PBCM_OMCI_PM_ID_ONLY id = NULL;

    if (omcipmEntry != NULL)
    {
        // this cast might be dangerous since there might be different PM types
        // but it is safe since all PM types have BCM_OMCIPM_PM as 1st struct
        id = (PBCM_OMCI_PM_ID_ONLY)(omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT]);
        if (id != NULL)
        {
            thresholdId = id->omcipm.thresholdId;
        }
    }

    return thresholdId;
}

static BCM_OMCI_PM_STATUS setThresholdIdInEntry(PBCM_OMCIPM_ENTRY omcipmEntry,
                                                UINT16 thresholdId)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCI_PM_ID_ONLY id = NULL;

    if (omcipmEntry != NULL)
    {
        // this cast might be dangerous since there might be different PM types
        // but it is safe since all PM types have BCM_OMCIPM_PM as 1st struct
        id = (PBCM_OMCI_PM_ID_ONLY)(omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT]);
        if (id != NULL)
        {
            id->omcipm.thresholdId = thresholdId;
        }
        id = (PBCM_OMCI_PM_ID_ONLY)(omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS]);
        if (id != NULL)
        {
            id->omcipm.thresholdId = thresholdId;
        }
    }
    else
    {
        ret = OMCI_PM_STATUS_PARAMETER_ERROR;
    }

    return ret;
}

static BCM_OMCI_PM_STATUS copyCounters(BCM_OMCI_PM_CLASS omcipmClass, void *src, void *dst)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    switch (omcipmClass)
    {
        case OMCI_PM_CLASS_GEM_PORT:
            memcpy(dst, src, sizeof(BCM_OMCI_PM_GEM_PORT));
            break;
        case OMCI_PM_CLASS_FEC:
            memcpy(dst, src, sizeof(BCM_OMCI_PM_FEC));
            break;
        case OMCI_PM_CLASS_ENET:
            memcpy(dst, src, sizeof(BCM_OMCI_PM_ETHERNET));
            break;
        case OMCI_PM_CLASS_ENET_2:
            memcpy(dst, src, sizeof(BCM_OMCI_PM_ETHERNET_2));
            break;
        case OMCI_PM_CLASS_ENET_3:
            memcpy(dst, src, sizeof(BCM_OMCI_PM_ETHERNET_3));
            break;
        case OMCI_PM_CLASS_MOCA_ENET:
            break;
        case OMCI_PM_CLASS_MOCA_IF:
            break;
        case OMCI_PM_CLASS_GAL_ENET:
            break;
        case OMCI_PM_CLASS_BRIDGE:
            break;
        case OMCI_PM_CLASS_BRIDGE_PORT:
            break;
        default:
            break;
    }

    return ret;
}

static BCM_OMCI_PM_STATUS reInitCounters(PBCM_OMCIPM_ENTRY omcipmEntry,
                                         BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 i = 0, omcipmId = 0, thresholdId = 0;
    PBCM_OMCI_PM_ID_ONLY id = NULL;
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    if (omcipmEntry == NULL)
    {
        return OMCI_PM_STATUS_PARAMETER_ERROR;
    }

    // this cast might be dangerous since there might be different PM types
    // but it is safe since all PM types have BCM_OMCIPM_PM as 1st struct
    id = (PBCM_OMCI_PM_ID_ONLY)omcipmEntry->pm[counterType];
    if (id != NULL)
    {
        // keep current omcipmId and thresholdId
        omcipmId = id->omcipm.omcipmId;
        thresholdId = id->omcipm.thresholdId;
        // only reset all threshold crossing mask when 15 Mins is expired
        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            omcipmEntry->crossingMaskPrev = omcipmEntry->crossingMaskCurr = 0;
        }
        // reset all counters to 0
        switch (omcipmEntry->omcipmClass)
        {
            case OMCI_PM_CLASS_GEM_PORT:
                memset(omcipmEntry->pm[counterType], 0, sizeof(BCM_OMCI_PM_GEM_PORT));
                // only call reset counters 1 time when device is registered
                if (i == OMCIPM_COUNTER_TYPE_CURRENT &&
                    omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
                {
                    BCM_OMCI_PM_GEM_PORT_COUNTER counters;
                    // use omcipm_dev_getCounters to reset counters
                    // by assign TRUE to reset parameter
                    ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_GPON,
                                                 id->omcipm.omcipmId,
                                                 (void *)&counters,
                                                 TRUE);
                }
                break;
            case OMCI_PM_CLASS_FEC:
                memset(omcipmEntry->pm[counterType], 0, sizeof(BCM_OMCI_PM_FEC));
                break;
            case OMCI_PM_CLASS_ENET:
                memset(omcipmEntry->pm[counterType], 0, sizeof(BCM_OMCI_PM_ETHERNET));
                break;
            case OMCI_PM_CLASS_ENET_2:
                memset(omcipmEntry->pm[counterType], 0, sizeof(BCM_OMCI_PM_ETHERNET_2));
                break;
            case OMCI_PM_CLASS_ENET_3:
                memset(omcipmEntry->pm[counterType], 0, sizeof(BCM_OMCI_PM_ETHERNET_3));
                break;
            case OMCI_PM_CLASS_MOCA_ENET:
                break;
            case OMCI_PM_CLASS_MOCA_IF:
                break;
            case OMCI_PM_CLASS_GAL_ENET:
                break;
            case OMCI_PM_CLASS_BRIDGE:
                break;
            case OMCI_PM_CLASS_BRIDGE_PORT:
                break;
            default:
                break;
        }
        // restore current omcipmId and thresholdId
        id->omcipm.omcipmId = omcipmId;
        id->omcipm.thresholdId = thresholdId;
    }
    else
    {
        ret = OMCI_PM_STATUS_PARAMETER_ERROR;
    }

    return ret;
}

static void freeOmcipmEntry(PBCM_OMCIPM_ENTRY omcipmEntry)
{
    UINT16 i = 0;

    if (omcipmEntry == NULL)
    {
        return;
    }

    for (i = OMCIPM_COUNTER_TYPE_CURRENT; i < OMCIPM_COUNTER_TYPE_MAX; i++)
    {
        switch (omcipmEntry->omcipmClass)
        {
            case OMCI_PM_CLASS_GEM_PORT:
                kmem_cache_free(gemPortCache, omcipmEntry->pm[i]);
                break;
            case OMCI_PM_CLASS_FEC:
                kmem_cache_free(fecCache, omcipmEntry->pm[i]);
                break;
            case OMCI_PM_CLASS_ENET:
                kmem_cache_free(enetCache, omcipmEntry->pm[i]);
                break;
            case OMCI_PM_CLASS_ENET_2:
                kmem_cache_free(enet2Cache, omcipmEntry->pm[i]);
                break;
            case OMCI_PM_CLASS_ENET_3:
                kmem_cache_free(enet3Cache, omcipmEntry->pm[i]);
                break;
            case OMCI_PM_CLASS_MOCA_ENET:
                break;
            case OMCI_PM_CLASS_MOCA_IF:
                break;
            case OMCI_PM_CLASS_GAL_ENET:
                break;
            case OMCI_PM_CLASS_BRIDGE:
                break;
            case OMCI_PM_CLASS_BRIDGE_PORT:
                break;
            default:
                break;
        }
    }

    // delete timer15Mins for this entry
    del_timer(&(omcipmEntry->timer15Mins));
    // free memory for this entry
    kmem_cache_free(omcipmCache, omcipmEntry);
    omcipmEntry = NULL;
}

static PBCM_OMCIPM_ENTRY allocOmcipmEntry(BCM_OMCI_PM_CLASS omcipmClass,
                                          UINT16 physPortId,
                                          UINT16 thresholdId)
{
    UINT16 i = 0;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;
    PBCM_OMCI_PM_ID_ONLY id = NULL;

    omcipmEntry = kmem_cache_alloc(omcipmCache, GFP_KERNEL);

    if (omcipmEntry != NULL)
    {
        omcipmEntry->omcipmClass = omcipmClass;
        omcipmEntry->crossingMaskPrev = omcipmEntry->crossingMaskCurr = 0;
        for (i = OMCIPM_COUNTER_TYPE_CURRENT; i < OMCIPM_COUNTER_TYPE_MAX; i++)
        {
            switch (omcipmEntry->omcipmClass)
            {
                case OMCI_PM_CLASS_GEM_PORT:
                    omcipmEntry->pm[i] = kmem_cache_alloc(gemPortCache, GFP_KERNEL);
                    if (omcipmEntry->pm[i] != NULL)
                    {
                        memset(omcipmEntry->pm[i], 0, sizeof(BCM_OMCI_PM_GEM_PORT));
                    }
                    break;
                case OMCI_PM_CLASS_FEC:
                    omcipmEntry->pm[i] = kmem_cache_alloc(fecCache, GFP_KERNEL);
                    if (omcipmEntry->pm[i] != NULL)
                    {
                        memset(omcipmEntry->pm[i], 0, sizeof(BCM_OMCI_PM_FEC));
                    }
                    break;
                case OMCI_PM_CLASS_ENET:
                    omcipmEntry->pm[i] = kmem_cache_alloc(enetCache, GFP_KERNEL);
                    if (omcipmEntry->pm[i] != NULL)
                    {
                        memset(omcipmEntry->pm[i], 0, sizeof(BCM_OMCI_PM_ETHERNET));
                    }
                    break;
                case OMCI_PM_CLASS_ENET_2:
                    omcipmEntry->pm[i] = kmem_cache_alloc(enet2Cache, GFP_KERNEL);
                    if (omcipmEntry->pm[i] != NULL)
                    {
                        memset(omcipmEntry->pm[i], 0, sizeof(BCM_OMCI_PM_ETHERNET_2));
                    }
                    break;
                case OMCI_PM_CLASS_ENET_3:
                    omcipmEntry->pm[i] = kmem_cache_alloc(enet3Cache, GFP_KERNEL);
                    if (omcipmEntry->pm[i] != NULL)
                    {
                        memset(omcipmEntry->pm[i], 0, sizeof(BCM_OMCI_PM_ETHERNET_3));
                    }
                    break;
                case OMCI_PM_CLASS_MOCA_ENET:
                    break;
                case OMCI_PM_CLASS_MOCA_IF:
                    break;
                case OMCI_PM_CLASS_GAL_ENET:
                    break;
                case OMCI_PM_CLASS_BRIDGE:
                    break;
                case OMCI_PM_CLASS_BRIDGE_PORT:
                    break;
                default:
                    break;
            }
            // this cast might be dangerous since there might be different PM types
            // but it is safe since all PM types have BCM_OMCIPM_PM as 1st struct
            id = (PBCM_OMCI_PM_ID_ONLY)omcipmEntry->pm[i];
            // assign current omcipmId and thresholdId
            if (id != NULL)
            {
                id->omcipm.omcipmId = physPortId;
                id->omcipm.thresholdId = thresholdId;
            }
            else
            {
                // free OMCI PM memory
                freeOmcipmEntry(omcipmEntry);
                break;
            }
        }

        // omcipmEntry might be NULL if freeOmcipmEntry is called above
        if (omcipmEntry != NULL)
        {
            // init timer15Mins for this entry
            init_timer(&(omcipmEntry->timer15Mins));
            // add timer15Mins for this entry
            addTimer15Mins(omcipmEntry);
        }
    }

    return omcipmEntry;
}

static void freeAllOmcipmEntry(void)
{
    PBCM_OMCIPM_ENTRY omcipmEntry, nextOmcipmEntry;

    omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,
                  "Cleaning-up OMCI PM Table (0x%08lX)",
                  (uint32)omcipmEntry);

    while (omcipmEntry)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Cleaning-up OMCI PM Entry %d",
                     getOmcipmIdFromEntry(omcipmEntry));
        nextOmcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
        BCM_COMMON_LL_REMOVE(&omcipmLL, omcipmEntry);
        freeOmcipmEntry(omcipmEntry);
        omcipmEntry = nextOmcipmEntry;
    };
}

static PBCM_OMCIPM_ENTRY findOmcipmEntryById(BCM_OMCI_PM_CLASS omcipmClass,
                                             UINT16 physPortId)
{
    PBCM_OMCIPM_ENTRY omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    while (omcipmEntry)
    {
        if ((omcipmEntry->omcipmClass == omcipmClass) &&
            (getOmcipmIdFromEntry(omcipmEntry) == physPortId))
        {
            break;
        }
        omcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
    };

    return omcipmEntry;
}

static BOOL alertThresholdCrossing(void)
{
    BOOL alert = FALSE;
    PBCM_OMCIPM_ENTRY omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    while (omcipmEntry)
    {
        if (omcipmEntry->crossingMaskPrev != omcipmEntry->crossingMaskCurr)
        {
            alert = TRUE;
            break;
        }
        omcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
    };

    return alert;
}

static UINT16 controlCrossingMask(UINT16 mask, UINT8 pos, BOOL enable)
{
    switch (pos)
    {
        case 0:
            mask = (enable == TRUE) ? mask | 0x8000 : mask & 0x7FFF;
            break;
        case 1:
            mask = (enable == TRUE) ? mask | 0x4000 : mask & 0xBFFF;
            break;
        case 2:
            mask = (enable == TRUE) ? mask | 0x2000 : mask & 0xDFFF;
            break;
        case 3:
            mask = (enable == TRUE) ? mask | 0x1000 : mask & 0xEFFF;
            break;
        case 4:
            mask = (enable == TRUE) ? mask | 0x0800 : mask & 0xF7FF;
            break;
        case 5:
            mask = (enable == TRUE) ? mask | 0x0400 : mask & 0xFBFF;
            break;
        case 6:
            mask = (enable == TRUE) ? mask | 0x0200 : mask & 0xFDFF;
            break;
        case 7:
            mask = (enable == TRUE) ? mask | 0x0100 : mask & 0xFEFF;
            break;
        case 8:
            mask = (enable == TRUE) ? mask | 0x0080 : mask & 0xFF7F;
            break;
        case 9:
            mask = (enable == TRUE) ? mask | 0x0040 : mask & 0xFFBF;
            break;
        case 10:
            mask = (enable == TRUE) ? mask | 0x0020 : mask & 0xFFDF;
            break;
        case 11:
            mask = (enable == TRUE) ? mask | 0x0010 : mask & 0xFFEF;
            break;
        case 12:
            mask = (enable == TRUE) ? mask | 0x0008 : mask & 0xFFF7;
            break;
        case 13:
            mask = (enable == TRUE) ? mask | 0x0004 : mask & 0xFFFB;
            break;
        case 14:
            mask = (enable == TRUE) ? mask | 0x0002 : mask & 0xFFFD;
            break;
        default:
            mask = (enable == TRUE) ? mask | 0x8000 : mask & 0x7FFF;
            break;
    }

    return mask;
}

static void checkGemPortCrossing(PBCM_OMCIPM_ENTRY omcipmEntry,
                                 BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 thresholdId = getThresholdIdFromEntry(omcipmEntry);
    PBCM_OMCIPM_THRESHOLD_ENTRY threshold = omcipm_threshold_find(thresholdId);
    PBCM_OMCI_PM_GEM_PORT gemCurrent = (PBCM_OMCI_PM_GEM_PORT)
                          omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];

    if (threshold == NULL || gemCurrent == NULL)
    {
        return;
    }

    if (gemCurrent->lostPackets >= threshold->thresholdData.thresholdValue1)
    {
        // set 1st bit (which is lostPackets) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 0, TRUE);
    }


    if (gemCurrent->misinsertedPackets >= threshold->thresholdData.thresholdValue2)
    {
        // set 2nd bit (which is misinsertedPackets) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 1, TRUE);
    }

    if (gemCurrent->impairedBlocks >= threshold->thresholdData.thresholdValue3)
    {
        // set 3rd bit (which is impairedBlocks) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 2, TRUE);
    }
}

static void checkFecCrossing(PBCM_OMCIPM_ENTRY omcipmEntry,
                             BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 thresholdId = getThresholdIdFromEntry(omcipmEntry);
    PBCM_OMCIPM_THRESHOLD_ENTRY threshold = omcipm_threshold_find(thresholdId);
    PBCM_OMCI_PM_FEC fecCurrent = (PBCM_OMCI_PM_FEC)
                          omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];

    if (threshold == NULL || fecCurrent == NULL)
    {
        return;
    }

    if (fecCurrent->correctedBytes >= threshold->thresholdData.thresholdValue1)
    {
        // set 1st bit (which is correctedBytes) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 0, TRUE);
    }

    if (fecCurrent->correctedCodeWords >= threshold->thresholdData.thresholdValue2)
    {
        // set 2nd bit (which is correctedCodeWords) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 1, TRUE);
    }

    if (fecCurrent->uncorrectedCodeWords >= threshold->thresholdData.thresholdValue3)
    {
        // set 3rd bit (which is uncorrectedCodeWords) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 2, TRUE);
    }

    if (fecCurrent->fecSeconds >= threshold->thresholdData.thresholdValue4)
    {
        // set 5th bit (which is fecSeconds) counter to 1
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 4, TRUE);
    }
}

static void checkENetCrossing(PBCM_OMCIPM_ENTRY omcipmEntry,
                              BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 thresholdId = getThresholdIdFromEntry(omcipmEntry);
    PBCM_OMCIPM_THRESHOLD_ENTRY threshold = omcipm_threshold_find(thresholdId);
    PBCM_OMCI_PM_ETHERNET enetCurrent = (PBCM_OMCI_PM_ETHERNET)
                          omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];

    if (threshold == NULL || enetCurrent == NULL)
    {
        return;
    }

    if (enetCurrent->fcsErrors >= threshold->thresholdData.thresholdValue1)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 0, TRUE);
    }

    if (enetCurrent->excessiveCollisionCounter >= threshold->thresholdData.thresholdValue2)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 1, TRUE);
    }

    if (enetCurrent->lateCollisionCounter >= threshold->thresholdData.thresholdValue3)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 2, TRUE);
    }

    if (enetCurrent->frameTooLongs >= threshold->thresholdData.thresholdValue4)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 3, TRUE);
    }

    if (enetCurrent->bufferOverflowsOnReceive >= threshold->thresholdData.thresholdValue5)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 4, TRUE);
    }

    if (enetCurrent->bufferOverflowsOnTransmit >= threshold->thresholdData.thresholdValue6)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 5, TRUE);
    }

    if (enetCurrent->singleCollisionFrameCounter >= threshold->thresholdData.thresholdValue7)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 6, TRUE);
    }

    if (enetCurrent->multipleCollisionsFrameCounter >= threshold->thresholdData.thresholdValue8)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 7, TRUE);
    }

    if (enetCurrent->sqeCounter >= threshold->thresholdData.thresholdValue9)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 8, TRUE);
    }

    if (enetCurrent->deferredTransmissionCounter >= threshold->thresholdData.thresholdValue10)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 9, TRUE);
    }

    if (enetCurrent->internalMacTransmitErrorCounter >= threshold->thresholdData.thresholdValue11)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 10, TRUE);
    }

    if (enetCurrent->carrierSenseErrorCounter >= threshold->thresholdData.thresholdValue12)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 11, TRUE);
    }

    if (enetCurrent->alignmentErrorCounter >= threshold->thresholdData.thresholdValue13)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 12, TRUE);
    }

    if (enetCurrent->internalMacReceiveErrorCounter >= threshold->thresholdData.thresholdValue14)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 13, TRUE);
    }
}

static void checkENet2Crossing(PBCM_OMCIPM_ENTRY omcipmEntry,
                               BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 thresholdId = getThresholdIdFromEntry(omcipmEntry);
    PBCM_OMCIPM_THRESHOLD_ENTRY threshold = omcipm_threshold_find(thresholdId);
    PBCM_OMCI_PM_ETHERNET_2 enet2Current = (PBCM_OMCI_PM_ETHERNET_2)
                          omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];

    if (threshold == NULL || enet2Current == NULL)
    {
        return;
    }

    if (enet2Current->pppoeFilterFrameCounter >= threshold->thresholdData.thresholdValue1)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 0, TRUE);
    }
}

static void checkENet3Crossing(PBCM_OMCIPM_ENTRY omcipmEntry,
                               BCM_OMCIPM_COUNTER_TYPE counterType)
{
    UINT16 thresholdId = getThresholdIdFromEntry(omcipmEntry);
    PBCM_OMCIPM_THRESHOLD_ENTRY threshold = omcipm_threshold_find(thresholdId);
    PBCM_OMCI_PM_ETHERNET_3 enet3Current = (PBCM_OMCI_PM_ETHERNET_3)
                          omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];

    if (threshold == NULL || enet3Current == NULL)
    {
        return;
    }

    if (enet3Current->dropEvents >= threshold->thresholdData.thresholdValue1)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 0, TRUE);
    }

    if (enet3Current->undersizePackets >= threshold->thresholdData.thresholdValue2)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 1, TRUE);
    }

    if (enet3Current->fragments >= threshold->thresholdData.thresholdValue3)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 2, TRUE);
    }

    if (enet3Current->jabbers >= threshold->thresholdData.thresholdValue4)
    {
        omcipmEntry->crossingMaskCurr = controlCrossingMask
                                        (omcipmEntry->crossingMaskCurr, 3, TRUE);
    }
}

/* handle saturation of counter:
 * when counter reach its max value then it keeps this value
 * until its value is reset
 */
static UINT32 updateCounter(UINT32 counter, UINT32 curr)
{
    UINT32 val = counter + curr;

    val = (val > counter) ? val : BCM_OMCI_PM_COUNTER_VALUE_MAX;

    return val;
}

/*
 * omcipm_dev_getCounters() return current counters in device.
 * To avoid saturation, after this call, counters in device
 * are reset to 0 (since reset parameter is TRUE).
 * Current counters accumulate all counters from device
 * until their values are saturated or 15 minutes later.
 * When 15 minutes are expired, 15 minutes counters copy
 * current counters, and reset current counters to 0.
*/

static void pollGemPort(PBCM_OMCIPM_ENTRY omcipmEntry,
                        BCM_OMCIPM_COUNTER_TYPE counterType)
{
    PBCM_OMCI_PM_GEM_PORT gemCurrent = NULL, gem15Mins = NULL;

    gemCurrent = (PBCM_OMCI_PM_GEM_PORT)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];
    gem15Mins = (PBCM_OMCI_PM_GEM_PORT)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS];

    if (gemCurrent != NULL &&
        gem15Mins != NULL &&
        omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        // keeps 15 mins interval number
        UINT8  intervalEndTime = gem15Mins->omcipm.intervalEndTime;
        BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
        BCM_OMCI_PM_GEM_PORT_COUNTER counters;
        // init counters
        memset(&counters, 0, sizeof(BCM_OMCI_PM_GEM_PORT_COUNTER));
        // get counters from GPON driver
        ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_GPON,
                                     gemCurrent->omcipm.omcipmId,
                                     (void *)&counters,
                                     TRUE);
        // update counters for this entry
        if (ret == OMCI_PM_STATUS_SUCCESS)
        {
            gemCurrent->lostPackets = updateCounter
                (gemCurrent->lostPackets, counters.lostPackets);
            gemCurrent->misinsertedPackets = updateCounter
                (gemCurrent->misinsertedPackets, counters.misinsertedPackets);
            gemCurrent->receivedPackets = updateCounter
                (gemCurrent->receivedPackets, counters.receivedPackets);
            gemCurrent->receivedBlocks = updateCounter
                (gemCurrent->receivedBlocks, counters.receivedBlocks);
            gemCurrent->transmittedBlocks = updateCounter
                (gemCurrent->transmittedBlocks, counters.transmittedBlocks);
            gemCurrent->impairedBlocks = updateCounter
                (gemCurrent->impairedBlocks, counters.impairedBlocks);
            gemCurrent->transmittedPackets = updateCounter
                (gemCurrent->transmittedPackets, counters.transmittedPackets);
        }

        // current interval is previous 15 Mins interval + 1
        gemCurrent->omcipm.intervalEndTime = ((intervalEndTime + 1) < 255) ? (intervalEndTime + 1) : 0;

#if 0
// BEGIN TEST CODES
        gemCurrent->lostPackets        += (gemCurrent->omcipm.intervalEndTime*10) + 1;
        gemCurrent->misinsertedPackets += (gemCurrent->omcipm.intervalEndTime*10) + 2;
        gemCurrent->receivedPackets    += (gemCurrent->omcipm.intervalEndTime*10) + 3;
        gemCurrent->receivedBlocks     += (gemCurrent->omcipm.intervalEndTime*10) + 4;
        gemCurrent->transmittedBlocks  += (gemCurrent->omcipm.intervalEndTime*10) + 5;
        gemCurrent->impairedBlocks     += (gemCurrent->omcipm.intervalEndTime*10) + 6;
        gemCurrent->transmittedPackets += (gemCurrent->omcipm.intervalEndTime*10) + 7;
         // increase interval counter which has value from 0 to 255
        (gemCurrent->omcipm.intervalEndTime < 255) ? ++gemCurrent->omcipm.intervalEndTime : 0;
//printk("===> pollGemPort -- Current counters, id=%d, thresholdId=%d, interval=%d, lostPackets=%lu, misinsertedPackets=%lu, receivedPackets=%lu, receivedBlocks=%lu, transmittedBlocks=%lu, impairedBlocks=%lu, transmittedPackets=%lu.\n\n", gemCurrent->omcipm.omcipmId, gemCurrent->omcipm.thresholdId, gemCurrent->omcipm.intervalEndTime, gemCurrent->lostPackets, gemCurrent->misinsertedPackets, gemCurrent->receivedPackets, gemCurrent->receivedBlocks, gemCurrent->transmittedBlocks, gemCurrent->impairedBlocks, gemCurrent->transmittedPackets);
// END TEST CODES
#endif

        // are there any GEM port counters that cross over threshold?
        // need to call before the code below since crossingMaskCurr
        // is set to 0 when 15 Mins is expired.
        checkGemPortCrossing(omcipmEntry, counterType);

        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            // copy all current counters to 15 mins counters
            copyCounters(omcipmEntry->omcipmClass, gemCurrent, gem15Mins);
            // reset all current counters
            reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
            // reset current crossing mask
            omcipmEntry->crossingMaskCurr = 0;
            // increase interval counter which has value from 0 to 255
            gem15Mins->omcipm.intervalEndTime = (intervalEndTime < 255) ? ++intervalEndTime : 0;
//printk("===> pollGemPort -- 15 MINS counters, id=%d, thresholdId=%d, interval=%d, lostPackets=%lu, misinsertedPackets=%lu, receivedPackets=%lu, receivedBlocks=%lu, transmittedBlocks=%lu, impairedBlocks=%lu, transmittedPackets=%lu.\n\n", gem15Mins->omcipm.omcipmId, gem15Mins->omcipm.thresholdId, gem15Mins->omcipm.intervalEndTime, gem15Mins->lostPackets, gem15Mins->misinsertedPackets, gem15Mins->receivedPackets, gem15Mins->receivedBlocks, gem15Mins->transmittedBlocks, gem15Mins->impairedBlocks, gem15Mins->transmittedPackets);
        }
    }
}

static void pollFec(PBCM_OMCIPM_ENTRY omcipmEntry,
                    BCM_OMCIPM_COUNTER_TYPE counterType)
{
    PBCM_OMCI_PM_FEC fecCurrent = NULL, fec15Mins = NULL;

    fecCurrent = (PBCM_OMCI_PM_FEC)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];
    fec15Mins = (PBCM_OMCI_PM_FEC)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS];
    if (fecCurrent != NULL &&
        fec15Mins != NULL &&
        omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        // keeps 15 mins interval number
        UINT8  intervalEndTime = fec15Mins->omcipm.intervalEndTime;
#if 0
        BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
        BCM_OMCI_PM_FEC_COUNTER counters;
        // init counters
        memset(&counters, 0, sizeof(BCM_OMCI_PM_FEC_COUNTER));
        // get counters from GPON driver
        ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_GPON,
                                     fecCurrent->omcipm.omcipmId,
                                     (void *)&counters,
                                     TRUE);
        // update counters for this entry
        if (ret == OMCI_PM_STATUS_SUCCESS)
        {
            fecCurrent->correctedBytes = updateCounter
                (fecCurrent->correctedBytes, counters.correctedBytes);
            fecCurrent->correctedCodeWords = updateCounter
                (fecCurrent->correctedCodeWords, counters.correctedCodeWords);
            fecCurrent->uncorrectedCodeWords = updateCounter
                (fecCurrent->uncorrectedCodeWords, counters.uncorrectedCodeWords);
            fecCurrent->totalCodeWords = updateCounter
                (fecCurrent->totalCodeWords, counters.totalCodeWords);
            fecCurrent->fecSeconds = updateCounter
                (fecCurrent->fecSeconds, counters.fecSeconds);
        }

        // current interval is previous 15 Mins interval + 1
        fecCurrent->omcipm.intervalEndTime = ((intervalEndTime + 1) < 255) ? (intervalEndTime + 1) : 0;

// BEGIN TEST CODES
        fecCurrent->correctedBytes       += (fecCurrent->omcipm.intervalEndTime*100) + 1;
        fecCurrent->correctedCodeWords   += (fecCurrent->omcipm.intervalEndTime*200) + 2;
        fecCurrent->uncorrectedCodeWords += (fecCurrent->omcipm.intervalEndTime*300) + 3;
        fecCurrent->totalCodeWords       += (fecCurrent->omcipm.intervalEndTime*400) + 4;
        fecCurrent->fecSeconds           += (fecCurrent->omcipm.intervalEndTime*500) + 5;
        // increase interval counter which has value from 0 to 255
        // current interval is previous 15 Mins interval + 1
        (fecCurrent->omcipm.intervalEndTime < 255) ? ++fecCurrent->omcipm.intervalEndTime : 0;
//printk("===> pollFec, id=%d, thresholdId=%d, interval=%d, correctedBytes=%lu, correctedCodeWords=%lu, uncorrectedCodeWords=%lu, totalCodeWords=%lu, fecSeconds=%lu.\n\n", fecCurrent->omcipm.omcipmId, fecCurrent->omcipm.thresholdId, fecCurrent->omcipm.intervalEndTime, fecCurrent->correctedBytes, fecCurrent->correctedCodeWords, fecCurrent->uncorrectedCodeWords, fecCurrent->totalCodeWords, fecCurrent->fecSeconds);
// END TEST CODES
#endif

        // are there any FEC counters that cross over threshold?
        // need to call before the code below since crossingMaskCurr
        // is set to 0 when 15 Mins is expired.
        checkFecCrossing(omcipmEntry, counterType);

        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            // copy all current counters to 15 mins counters
            copyCounters(omcipmEntry->omcipmClass, fecCurrent, fec15Mins);
            // reset all current counters
            reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
            // reset current crossing mask
            omcipmEntry->crossingMaskCurr = 0;
            // increase interval counter which has value from 0 to 255
            fec15Mins->omcipm.intervalEndTime = (intervalEndTime < 255) ? ++intervalEndTime : 0;
//printk("===> pollFec, id=%d, thresholdId=%d, interval=%d, correctedBytes=%lu, correctedCodeWords=%lu, uncorrectedCodeWords=%lu, totalCodeWords=%lu, fecSeconds=%lu.\n\n", fec15Mins->omcipm.omcipmId, fec15Mins->omcipm.thresholdId, fec15Mins->omcipm.intervalEndTime, fec15Mins->correctedBytes, fec15Mins->correctedCodeWords, fec15Mins->uncorrectedCodeWords, fec15Mins->totalCodeWords, fec15Mins->fecSeconds);
        }
    }
}

static void pollENet(PBCM_OMCIPM_ENTRY omcipmEntry,
                     BCM_OMCIPM_COUNTER_TYPE counterType)
{
    PBCM_OMCI_PM_ETHERNET enetCurrent = NULL, enet15Mins = NULL;

    enetCurrent = (PBCM_OMCI_PM_ETHERNET)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];
    enet15Mins = (PBCM_OMCI_PM_ETHERNET)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS];
    if (enetCurrent != NULL &&
        enet15Mins != NULL &&
        omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        // keeps 15 mins interval number
        UINT8  intervalEndTime = enet15Mins->omcipm.intervalEndTime;
#if 0
        BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
        BCM_OMCI_PM_ETHERNET_COUNTER counters;
        // init counters
        memset(&counters, 0, sizeof(BCM_OMCI_PM_ETHERNET_COUNTER));
        // get counters from ENET driver
        ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_ENET,
                                     enetCurrent->omcipm.omcipmId,
                                     (void *)&counters,
                                     TRUE);
        // update counters for this entry
        if (ret == OMCI_PM_STATUS_SUCCESS)
        {
            enetCurrent->fcsErrors = updateCounter
                (enetCurrent->fcsErrors, counters.fcsErrors);
            enetCurrent->excessiveCollisionCounter = updateCounter
                (enetCurrent->excessiveCollisionCounter, counters.excessiveCollisionCounter);
            enetCurrent->lateCollisionCounter = updateCounter
                (enetCurrent->lateCollisionCounter, counters.lateCollisionCounter);
            enetCurrent->frameTooLongs = updateCounter
                (enetCurrent->frameTooLongs, counters.frameTooLongs);
            enetCurrent->bufferOverflowsOnReceive = updateCounter
                (enetCurrent->bufferOverflowsOnReceive, counters.bufferOverflowsOnReceive);
            enetCurrent->bufferOverflowsOnTransmit = updateCounter
                (enetCurrent->bufferOverflowsOnTransmit, counters.bufferOverflowsOnTransmit);
            enetCurrent->singleCollisionFrameCounter = updateCounter
                (enetCurrent->singleCollisionFrameCounter, counters.singleCollisionFrameCounter);
            enetCurrent->multipleCollisionsFrameCounter = updateCounter
                (enetCurrent->multipleCollisionsFrameCounter, counters.multipleCollisionsFrameCounter);
            enetCurrent->sqeCounter = updateCounter
                (enetCurrent->sqeCounter, counters.sqeCounter);
            enetCurrent->deferredTransmissionCounter = updateCounter
                (enetCurrent->deferredTransmissionCounter, counters.deferredTransmissionCounter);
            enetCurrent->internalMacTransmitErrorCounter = updateCounter
                (enetCurrent->internalMacTransmitErrorCounter, counters.internalMacTransmitErrorCounter);
            enetCurrent->carrierSenseErrorCounter = updateCounter
                (enetCurrent->carrierSenseErrorCounter, counters.carrierSenseErrorCounter);
            enetCurrent->alignmentErrorCounter = updateCounter
                (enetCurrent->alignmentErrorCounter, counters.alignmentErrorCounter);
            enetCurrent->internalMacReceiveErrorCounter = updateCounter
                (enetCurrent->internalMacReceiveErrorCounter, counters.internalMacReceiveErrorCounter);
        }

        // current interval is previous 15 Mins interval + 1
        enetCurrent->omcipm.intervalEndTime = ((intervalEndTime + 1) < 255) ? (intervalEndTime + 1) : 0;
#endif

        // are there any Ethernet counters that cross over threshold?
        // need to call before the code below since crossingMaskCurr
        // is set to 0 when 15 Mins is expired.
        checkENetCrossing(omcipmEntry, counterType);

        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            // copy all current counters to 15 mins counters
            copyCounters(omcipmEntry->omcipmClass, enetCurrent, enet15Mins);
            // reset all current counters
            reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
            // reset current crossing mask
            omcipmEntry->crossingMaskCurr = 0;
            // increase interval counter which has value from 0 to 255
            enet15Mins->omcipm.intervalEndTime = (intervalEndTime < 255) ? ++intervalEndTime : 0;
        }
    }
}

static void pollENet2(PBCM_OMCIPM_ENTRY omcipmEntry,
                      BCM_OMCIPM_COUNTER_TYPE counterType)
{
    PBCM_OMCI_PM_ETHERNET_2 enetCurrent = NULL, enet15Mins = NULL;

    enetCurrent = (PBCM_OMCI_PM_ETHERNET_2)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];
    enet15Mins = (PBCM_OMCI_PM_ETHERNET_2)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS];
    if (enetCurrent != NULL &&
        enet15Mins != NULL &&
        omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        // keeps 15 mins interval number
        UINT8  intervalEndTime = enet15Mins->omcipm.intervalEndTime;
#if 0
        BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
        BCM_OMCI_PM_ETHERNET_2_COUNTER counters;
        // init counters
        memset(&counters, 0, sizeof(BCM_OMCI_PM_ETHERNET_2_COUNTER));
        // get counters from ENET driver
        ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_ENET,
                                     enetCurrent->omcipm.omcipmId,
                                     (void *)&counters,
                                     TRUE);
        // update counters for this entry
        if (ret == OMCI_PM_STATUS_SUCCESS)
        {
            enetCurrent->pppoeFilterFrameCounter = updateCounter
                (enetCurrent->pppoeFilterFrameCounter, counters.pppoeFilterFrameCounter);
        }

        // current interval is previous 15 Mins interval + 1
        enetCurrent->omcipm.intervalEndTime = ((intervalEndTime + 1) < 255) ? (intervalEndTime + 1) : 0;
#endif

        // are there any Ethernet 2 counters that cross over threshold?
        // need to call before the code below since crossingMaskCurr
        // is set to 0 when 15 Mins is expired.
        checkENet2Crossing(omcipmEntry, counterType);

        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            // copy all current counters to 15 mins counters
            copyCounters(omcipmEntry->omcipmClass, enetCurrent, enet15Mins);
            // reset all current counters
            reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
            // reset current crossing mask
            omcipmEntry->crossingMaskCurr = 0;
            // increase interval counter which has value from 0 to 255
            enet15Mins->omcipm.intervalEndTime = (intervalEndTime < 255) ? ++intervalEndTime : 0;
        }
    }
}

static void pollENet3(PBCM_OMCIPM_ENTRY omcipmEntry,
                      BCM_OMCIPM_COUNTER_TYPE counterType)
{
    PBCM_OMCI_PM_ETHERNET_3 enetCurrent = NULL, enet15Mins = NULL;

    enetCurrent = (PBCM_OMCI_PM_ETHERNET_3)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT];
    enet15Mins = (PBCM_OMCI_PM_ETHERNET_3)omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS];
    if (enetCurrent != NULL &&
        enet15Mins != NULL &&
        omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        // keeps 15 mins interval number
        UINT8  intervalEndTime = enet15Mins->omcipm.intervalEndTime;
#if 0
        BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
        BCM_OMCI_PM_ETHERNET_3_COUNTER counters;
        // init counters
        memset(&counters, 0, sizeof(BCM_OMCI_PM_ETHERNET_3_COUNTER));
        // get counters from ENET driver
        ret = omcipm_dev_getCounters(BCM_OMCI_PM_DEVICE_ENET,
                                     enetCurrent->omcipm.omcipmId,
                                     (void *)&counters,
                                     TRUE);
        // update counters for this entry
        if (ret == OMCI_PM_STATUS_SUCCESS)
        {
            enetCurrent->dropEvents = updateCounter
                (enetCurrent->dropEvents, counters.dropEvents);
            enetCurrent->octets = updateCounter
                (enetCurrent->octets, counters.octets);
            enetCurrent->packets = updateCounter
                (enetCurrent->packets, counters.packets);
            enetCurrent->broadcastPackets = updateCounter
                (enetCurrent->broadcastPackets, counters.broadcastPackets);
            enetCurrent->multicastPackets = updateCounter
                (enetCurrent->multicastPackets, counters.multicastPackets);
            enetCurrent->undersizePackets = updateCounter
                (enetCurrent->undersizePackets, counters.undersizePackets);
            enetCurrent->fragments = updateCounter
                (enetCurrent->fragments, counters.fragments);
            enetCurrent->jabbers = updateCounter
                (enetCurrent->jabbers, counters.jabbers);
            enetCurrent->packets64Octets = updateCounter
                (enetCurrent->packets64Octets, counters.packets64Octets);
            enetCurrent->packets127Octets = updateCounter
                (enetCurrent->packets127Octets, counters.packets127Octets);
            enetCurrent->packets255Octets = updateCounter
                (enetCurrent->packets255Octets, counters.packets255Octets);
            enetCurrent->packets511Octets = updateCounter
                (enetCurrent->packets511Octets, counters.packets511Octets);
            enetCurrent->packets1023Octets = updateCounter
                (enetCurrent->packets1023Octets, counters.packets1023Octets);
            enetCurrent->packets1518Octets = updateCounter
                (enetCurrent->packets1518Octets, counters.packets1518Octets);
        }

        // current interval is previous 15 Mins interval + 1
        enetCurrent->omcipm.intervalEndTime = ((intervalEndTime + 1) < 255) ? (intervalEndTime + 1) : 0;
#endif

        // are there any Ethernet 3 counters that cross over threshold?
        // need to call before the code below since crossingMaskCurr
        // is set to 0 when 15 Mins is expired.
        checkENet3Crossing(omcipmEntry, counterType);

        if (counterType == OMCIPM_COUNTER_TYPE_15MINS)
        {
            // copy all current counters to 15 mins counters
            copyCounters(omcipmEntry->omcipmClass, enetCurrent, enet15Mins);
            // reset all current counters
            reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
            // reset current crossing mask
            omcipmEntry->crossingMaskCurr = 0;
            // increase interval counter which has value from 0 to 255
            enet15Mins->omcipm.intervalEndTime = (intervalEndTime < 255) ? ++intervalEndTime : 0;
        }
    }
}

static void poll15Mins(UINT32 handle)
{
    PBCM_OMCIPM_ENTRY omcipmEntry = (PBCM_OMCIPM_ENTRY)handle;

    if (omcipmEntry == NULL)
    {
        return;
    }

    switch (omcipmEntry->omcipmClass)
    {
        case OMCI_PM_CLASS_GEM_PORT:
            pollGemPort(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
            break;
        case OMCI_PM_CLASS_FEC:
            pollFec(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
            break;
        case OMCI_PM_CLASS_ENET:
            pollENet(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
            break;
        case OMCI_PM_CLASS_ENET_2:
            pollENet2(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
            break;
        case OMCI_PM_CLASS_ENET_3:
            pollENet3(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
            break;
        case OMCI_PM_CLASS_MOCA_ENET:
            break;
        case OMCI_PM_CLASS_MOCA_IF:
            break;
        case OMCI_PM_CLASS_GAL_ENET:
            break;
        case OMCI_PM_CLASS_BRIDGE:
            break;
        case OMCI_PM_CLASS_BRIDGE_PORT:
            break;
        default:
            break;
    }

    // call addTimer15Mins() so that poll_15Mins() is called again
    // when timer15Mins is expired
    addTimer15Mins(omcipmEntry);
}

static void addTimer15Mins(PBCM_OMCIPM_ENTRY omcipmEntry)
{
    if (omcipmEntry != NULL)
    {
        // setup timer15Mins for this entry
        omcipmEntry->timer15Mins.expires = jiffies + BCM_OMCI_PM_POLLING_RATE_15_MINS;
        omcipmEntry->timer15Mins.function = poll15Mins;
        omcipmEntry->timer15Mins.data = (UINT32) omcipmEntry;

        // add timer15Mins for this entry
        add_timer(&(omcipmEntry->timer15Mins));
    }
}

static void pollCurrentEntry(PBCM_OMCIPM_ENTRY omcipmEntry)
{
    if (omcipmEntry != NULL)
    {
        // Poll current counters for specific OMCI PM entry
        switch (omcipmEntry->omcipmClass)
        {
            case OMCI_PM_CLASS_GEM_PORT:
                pollGemPort(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
                break;
            case OMCI_PM_CLASS_FEC:
                pollFec(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
                break;
            case OMCI_PM_CLASS_ENET:
                pollENet(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
                break;
            case OMCI_PM_CLASS_ENET_2:
                pollENet2(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
                break;
            case OMCI_PM_CLASS_ENET_3:
                pollENet3(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
                break;
            case OMCI_PM_CLASS_MOCA_ENET:
                break;
            case OMCI_PM_CLASS_MOCA_IF:
                break;
            case OMCI_PM_CLASS_GAL_ENET:
                break;
            case OMCI_PM_CLASS_BRIDGE:
                break;
            case OMCI_PM_CLASS_BRIDGE_PORT:
                break;
            default:
                break;
        }
    }
}

static void pollCurrent(UINT32 handle)
{
    PBCM_OMCIPM_ENTRY omcipmEntry;

    omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    // Poll current counters for all OMCI PM entry
    while (omcipmEntry)
    {
        pollCurrentEntry(omcipmEntry);
        // get next entry
        omcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
    };

    // call addTimerCurrent() so that pollCurrent() is called again
    // when timerCurrent is expired
    addTimerCurrent();
}

static void addTimerCurrent(void)
{
    // setup timerCurrent for OMCI PM llist
    // 3 seconds is polling rate to prevent counter saturation
    timerCurrent.expires = jiffies + BCM_OMCI_PM_POLLING_RATE_3_SEC;
    timerCurrent.function = pollCurrent;
    timerCurrent.data = 0;

    // add timerCurrent for OMCI PM llist
    add_timer(&timerCurrent);

    // wake up event queue (in bcm_omcipm_user)
    if (alertThresholdCrossing() == TRUE)
    {
        wake_up_interruptible(&omcipmEventWaitQueue);
    }
}

/**
 * Global OMCI PM functions
 **/

BCM_OMCI_PM_STATUS omcipm_create(BCM_OMCI_PM_CLASS omcipmClass,
                                 UINT16 physPortId,
                                 UINT16 thresholdId)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;

    /* allocate a new OMCI PM entry */
    omcipmEntry = allocOmcipmEntry(omcipmClass, physPortId, thresholdId);

    if (omcipmEntry != NULL)
    {
        BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,
                      "Create OMCI PM Entry Id: %d, Threshold Id: %d",
                      physPortId, thresholdId);
        BCM_COMMON_LL_APPEND(&omcipmLL, omcipmEntry);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not allocate OMCI PM Entry memory");
        ret = OMCI_PM_STATUS_ALLOC_ERROR;
    }

    return ret;
}

BCM_OMCI_PM_STATUS omcipm_remove(BCM_OMCI_PM_CLASS omcipmClass, UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;

    omcipmEntry = findOmcipmEntryById(omcipmClass, physPortId);

    if (omcipmEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Freeing OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        BCM_COMMON_LL_REMOVE(&omcipmLL, omcipmEntry);
        freeOmcipmEntry(omcipmEntry);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find OMCI PM Entry Class %d, Id %d", omcipmClass, physPortId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

BCM_OMCI_PM_STATUS omcipm_get(BCM_OMCI_PM_CLASS omcipmClass, UINT16 physPortId, void *counters)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;

    omcipmEntry = findOmcipmEntryById(omcipmClass, physPortId);

    if (omcipmEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Get OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        ret = copyCounters(omcipmClass,
                           omcipmEntry->pm[OMCIPM_COUNTER_TYPE_15MINS],
                           counters);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

BCM_OMCI_PM_STATUS omcipm_getCurrent(BCM_OMCI_PM_CLASS omcipmClass, UINT16 physPortId, void *counters)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;

    omcipmEntry = findOmcipmEntryById(omcipmClass, physPortId);

    if (omcipmEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Get Current OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        pollCurrentEntry(omcipmEntry);
        ret = copyCounters(omcipmClass,
                           omcipmEntry->pm[OMCIPM_COUNTER_TYPE_CURRENT],
                           counters);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

BCM_OMCI_PM_STATUS omcipm_set(BCM_OMCI_PM_CLASS omcipmClass, UINT16 physPortId, UINT16 thresholdId)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;
    PBCM_OMCIPM_ENTRY omcipmEntry = NULL;

    omcipmEntry = findOmcipmEntryById(omcipmClass, physPortId);

    if (omcipmEntry != NULL)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, "Set OMCI PM Entry Class: %d, Id: %d, Threshod: %d", omcipmClass, physPortId, thresholdId);
        ret = setThresholdIdInEntry(omcipmEntry, thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Could not find OMCI PM Entry Class: %d, Id: %d", omcipmClass, physPortId);
        ret = OMCI_PM_STATUS_NOT_FOUND;
    }

    return ret;
}

int omcipm_init(void)
{
    /* create a slab cache for OMCI PM entry */
    if (omcipmCache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
	    omcipmCache = kmem_cache_create("omcipm_entry",
                                        sizeof(BCM_OMCIPM_ENTRY),
                                        0, /* align */
                                        0, /* flags */
                                        NULL); /* ctor */
#else
	    omcipmCache = kmem_cache_create("omcipm_entry",
                                        sizeof(BCM_OMCIPM_ENTRY),
                                        0, /* align */
                                        0, /* flags */
                                        NULL, /* ctor */
	                                 	NULL); /* dtor */
#endif
        if (omcipmCache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create OMCI PM Entry Cache");
            return -ENOMEM;
        }

        BCM_COMMON_LL_INIT(&omcipmLL);
    }

    // init timerCurrent for all PMs in llist
    init_timer(&timerCurrent);

    // setup timerCurrent for all PMs in llist
    addTimerCurrent();

    return 0;
}

void omcipm_exit(void)
{
    // delete timerCurrent for all OMCI PMs in llist
    del_timer(&timerCurrent);

    // free all PMs memory
    freeAllOmcipmEntry();

    // destroy llist cache for OMCI PM entry
    kmem_cache_destroy(omcipmCache);
    omcipmCache = NULL;
}

void omcipm_reInit(void)
{
    PBCM_OMCIPM_ENTRY omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    while (omcipmEntry)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM, 
                     "Re-Intialize OMCI PM Entry Class: %d, Id: %d",
                     omcipmEntry->omcipmClass,
                     getOmcipmIdFromEntry(omcipmEntry));
        // reinitialize PM info
        reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_CURRENT);
        reInitCounters(omcipmEntry, OMCIPM_COUNTER_TYPE_15MINS);
        // delete timer15Mins for this entry
        del_timer(&(omcipmEntry->timer15Mins));
        // add timer15Mins for this entry
        addTimer15Mins(omcipmEntry);
        // get next entry
        omcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
    };
}

BOOL omcipm_alertThresholdCrossing(void)
{
    return alertThresholdCrossing();
}

UINT16 omcipm_retrieveThresholdCrossing(UINT8 *buf, UINT16 size)
{
    UINT16 len = 0;
    BCM_OMCI_PM_TCA tca;
    PBCM_OMCIPM_ENTRY omcipmEntry = BCM_COMMON_LL_GET_HEAD(omcipmLL);

    while (omcipmEntry)
    {
        if (omcipmEntry->crossingMaskPrev != omcipmEntry->crossingMaskCurr)
        {
            // set prev mask equal to curr mask to mark this
            // threshold crossing is already alerted so that
            // it's not reported in the next 15 mins
            omcipmEntry->crossingMaskPrev = omcipmEntry->crossingMaskCurr;

            memset(&tca, 0, sizeof(BCM_OMCI_PM_TCA));
            // Class ID is 2 bytes
            switch (omcipmEntry->omcipmClass)
            {
                case OMCI_PM_CLASS_GEM_PORT:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_GEM_PORT;
                    break;
                case OMCI_PM_CLASS_FEC:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_FEC;
                    break;
                case OMCI_PM_CLASS_ENET:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_ENET;
                    break;
                case OMCI_PM_CLASS_ENET_2:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_ENET_2;
                    break;
                case OMCI_PM_CLASS_ENET_3:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_ENET_3;
                    break;
                case OMCI_PM_CLASS_MOCA_ENET:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_MOCA_ENET;
                    break;
                case OMCI_PM_CLASS_MOCA_IF:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_MOCA_IF;
                    break;
                case OMCI_PM_CLASS_GAL_ENET:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_GAL_ENET;
                    break;
                case OMCI_PM_CLASS_BRIDGE:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_BRIDGE;
                    break;
                case OMCI_PM_CLASS_BRIDGE_PORT:
                    tca.classId = (UINT16)BCM_OMCI_PM_CLASS_BRIDGE_PORT;
                    break;
                default:
                    break;
            }
            memcpy(&buf[len], &(tca.classId), sizeof(tca.classId));
            len += sizeof(tca.classId);
            // Managed Entity ID is 2 bytes
            tca.instanceId = getOmcipmIdFromEntry(omcipmEntry);
            memcpy(&buf[len], &(tca.instanceId), sizeof(tca.instanceId));
            len += sizeof(tca.instanceId);
            // Threshold Crossing Mask is 2 bytes
            tca.tcaMask = omcipmEntry->crossingMaskCurr;
            memcpy(&buf[len], &(tca.tcaMask), sizeof(tca.tcaMask));
            len += sizeof(tca.tcaMask);
            // each entry uses 6 bytes in buf
            if ((len + sizeof(BCM_OMCI_PM_TCA)) > size)
            {
                break;
            }
        }
        omcipmEntry = BCM_COMMON_LL_GET_NEXT(omcipmEntry);
    };

    return len;
}

/**
 * Global GEM Port functions
 **/

int omcipm_gemport_init(void)
{
    /* create a slab cache for gemPort entry */
    if (gemPortCache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        gemPortCache = kmem_cache_create("omcipm_gemport",
                                         sizeof(BCM_OMCI_PM_GEM_PORT),
                                         0, /* align */
                                         0, /* flags */
                                         NULL); /* ctor */
#else
        gemPortCache = kmem_cache_create("omcipm_gemport",
                                         sizeof(BCM_OMCI_PM_GEM_PORT),
                                         0, /* align */
                                         0, /* flags */
                                         NULL, /* ctor */
                                         NULL); /* dtor */
#endif

        if (gemPortCache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create GEM Port Cache");
            return -ENOMEM;
        }
    }

    return 0;
}

void omcipm_gemport_exit(void)
{
    kmem_cache_destroy(gemPortCache);
    gemPortCache = NULL;
}

/**
 * Global FEC functions
 **/

int omcipm_fec_init(void)
{
    /* create a slab cache for Forward Error Correction entry */
    if (fecCache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        fecCache = kmem_cache_create("omcipm_fec",
                                     sizeof(BCM_OMCI_PM_FEC),
                                     0, /* align */
                                     0, /* flags */
                                     NULL); /* ctor */
#else
        fecCache = kmem_cache_create("omcipm_fec",
                                     sizeof(BCM_OMCI_PM_FEC),
                                     0, /* align */
                                     0, /* flags */
                                     NULL, /* ctor */
                                     NULL); /* dtor */
#endif
        if (fecCache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create FEC Cache");
            return -ENOMEM;
        }
    }

    return 0;
}

void omcipm_fec_exit(void)
{
    kmem_cache_destroy(fecCache);
    fecCache = NULL;
}

/**
 * Global Ethernet functions
 **/

int omcipm_enet_init(void)
{
    /* create a slab cache for Ethernet entry */
    if (enetCache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        enetCache = kmem_cache_create("omcipm_enet",
                                      sizeof(BCM_OMCI_PM_ETHERNET),
                                      0, /* align */
                                      0, /* flags */
                                      NULL); /* ctor */
#else
        enetCache = kmem_cache_create("omcipm_enet",
                                      sizeof(BCM_OMCI_PM_ETHERNET),
                                      0, /* align */
                                      0, /* flags */
                                      NULL, /* ctor */
                                      NULL); /* dtor */
#endif
        if (enetCache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create Ethernet Cache");
            return -ENOMEM;
        }
    }

    return 0;
}

void omcipm_enet_exit(void)
{
    kmem_cache_destroy(enetCache);
    enetCache = NULL;
}

/**
 * Global Ethernet 2 functions
 **/

int omcipm_enet2_init(void)
{
    /* create a slab cache for Ethernet 2 entry */
    if (enet2Cache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        enet2Cache = kmem_cache_create("omcipm_enet2",
                                       sizeof(BCM_OMCI_PM_ETHERNET_2),
                                       0, /* align */
                                       0, /* flags */
                                       NULL); /* ctor */
#else
        enet2Cache = kmem_cache_create("omcipm_enet2",
                                       sizeof(BCM_OMCI_PM_ETHERNET_2),
                                       0, /* align */
                                       0, /* flags */
                                       NULL, /* ctor */
                                       NULL); /* dtor */
#endif
        if (enet2Cache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create Ethernet 2 Cache");
            return -ENOMEM;
        }
    }

    return 0;
}

void omcipm_enet2_exit(void)
{
    kmem_cache_destroy(enet2Cache);
    enet2Cache = NULL;
}

/**
 * Global Ethernet 3 functions
 **/

int omcipm_enet3_init(void)
{
    /* create a slab cache for Ethernet 3 entry */
    if (enet3Cache == NULL)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        enet3Cache = kmem_cache_create("omcipm_enet3",
                                       sizeof(BCM_OMCI_PM_ETHERNET_3),
                                       0, /* align */
                                       0, /* flags */
                                       NULL); /* ctor */
#else
        enet3Cache = kmem_cache_create("omcipm_enet3",
                                       sizeof(BCM_OMCI_PM_ETHERNET_3),
                                       0, /* align */
                                       0, /* flags */
                                       NULL, /* ctor */
                                       NULL); /* dtor */
#endif
        if (enet3Cache == NULL)
        {
            BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "Failed to create Ethernet 3 Cache");
            return -ENOMEM;
        }
    }

    return 0;
}

void omcipm_enet3_exit(void)
{
    kmem_cache_destroy(enet3Cache);
    enet3Cache = NULL;
}

