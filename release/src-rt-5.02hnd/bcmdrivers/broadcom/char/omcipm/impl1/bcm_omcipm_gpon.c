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
#include <asm/uaccess.h>   /* for copy_from_user */

#include "bcm_omcipm_gpon.h"

/**
 * GEM Port local functions
 **/

static BCM_OMCI_PM_STATUS omcipm_gemport_create(PBCM_OMCI_PM_GEM_PORT gemPort)
{
    BCM_OMCI_PM_STATUS ret = omcipm_create(OMCI_PM_CLASS_GEM_PORT,
                                           gemPort->omcipm.omcipmId,
                                           gemPort->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully create GEM Port Id: %d, Threshold Id: %d",
                     gemPort->omcipm.omcipmId, gemPort->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete GEM Port Id: %d, Status: %d",
                      gemPort->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_gemport_delete(UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = omcipm_remove(OMCI_PM_CLASS_GEM_PORT, physPortId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully delete GEM Port Id: %d",
                     physPortId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete GEM Port Id: %d, Status: %d",
                      physPortId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_gemport_get(PBCM_OMCI_PM_GEM_PORT gemPort)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_get(OMCI_PM_CLASS_GEM_PORT,
                     gemPort->omcipm.omcipmId,
                     (void *)gemPort);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get GEM Port Id: %d",
                     gemPort->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get GEM Port Id: %d, Status: %d",
                      gemPort->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_gemport_getCurrent(PBCM_OMCI_PM_GEM_PORT gemPort)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_getCurrent(OMCI_PM_CLASS_GEM_PORT,
                            gemPort->omcipm.omcipmId,
                            (void *)gemPort);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get current GEM Port Id: %d",
                     gemPort->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get current GEM Port Id: %d, Status: %d",
                      gemPort->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_gemport_set(PBCM_OMCI_PM_GEM_PORT gemPort)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_set(OMCI_PM_CLASS_GEM_PORT,
                     gemPort->omcipm.omcipmId,
                     gemPort->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully set GEM Port Id: %d, Threshold Id: %d",
                     gemPort->omcipm.omcipmId, gemPort->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to set GEM Port Id: %d, Status: %d",
                      gemPort->omcipm.omcipmId, ret);
    }

    return ret;
}

/**
 * FEC local functions
 **/

static BCM_OMCI_PM_STATUS omcipm_fec_create(PBCM_OMCI_PM_FEC fec)
{
    BCM_OMCI_PM_STATUS ret = omcipm_create(OMCI_PM_CLASS_FEC,
                                           fec->omcipm.omcipmId,
                                           fec->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully create FEC Id: %d, Threshold Id: %d",
                     fec->omcipm.omcipmId, fec->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete FEC Id: %d, Status: %d",
                      fec->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_fec_delete(UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = omcipm_remove(OMCI_PM_CLASS_FEC, physPortId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully delete FEC Id: %d",
                     physPortId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete FEC Id: %d, Status: %d",
                      physPortId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_fec_get(PBCM_OMCI_PM_FEC fec)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_get(OMCI_PM_CLASS_FEC,
                     fec->omcipm.omcipmId,
                     (void *)fec);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get FEC Id: %d",
                     fec->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get FEC Id: %d, Status: %d",
                      fec->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_fec_getCurrent(PBCM_OMCI_PM_FEC fec)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_getCurrent(OMCI_PM_CLASS_FEC,
                            fec->omcipm.omcipmId,
                            (void *)fec);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get current FEC Id: %d",
                     fec->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get current FEC Id: %d,  Status: %d",
                      fec->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_fec_set(PBCM_OMCI_PM_FEC fec)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_set(OMCI_PM_CLASS_FEC,
                     fec->omcipm.omcipmId,
                     fec->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully set FEC Id: %d, Threshold Id: %d",
                     fec->omcipm.omcipmId, fec->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to set FEC Id: %d, Status: %d",
                      fec->omcipm.omcipmId, ret);
    }

    return ret;
}

/**
 * IOCTL GEM Port functions
 **/

void omcipm_createGemPort(unsigned long arg)
{
    STS_OMCI_PM_GEM_PORT KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmGemPort,
                       &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                       sizeof(KArg.omcipmGemPort));

        KArg.omcipmStatus = omcipm_gemport_create(&KArg.omcipmGemPort);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmStatus);
}

void omcipm_deleteGemPort(unsigned long arg)
{
    STS_OMCI_PM_GEM_PORT KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmGemPort,
                       &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                       sizeof(KArg.omcipmGemPort));

        KArg.omcipmStatus = omcipm_gemport_delete(KArg.omcipmGemPort.omcipm.omcipmId);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmStatus);
}

void omcipm_getGemPort(unsigned long arg)
{
    STS_OMCI_PM_GEM_PORT KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmGemPort,
                       &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                       sizeof(KArg.omcipmGemPort));

        KArg.omcipmStatus = omcipm_gemport_get(&KArg.omcipmGemPort);

        copy_to_user(&((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                     &KArg.omcipmGemPort,
                     sizeof(KArg.omcipmGemPort));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmStatus);
}

void omcipm_getCurrentGemPort(unsigned long arg)
{
    STS_OMCI_PM_GEM_PORT KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmGemPort,
                       &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                       sizeof(KArg.omcipmGemPort));

        KArg.omcipmStatus = omcipm_gemport_getCurrent(&KArg.omcipmGemPort);

        copy_to_user(&((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                     &KArg.omcipmGemPort,
                     sizeof(KArg.omcipmGemPort));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmStatus);
}

void omcipm_setGemPort(unsigned long arg)
{
    STS_OMCI_PM_GEM_PORT KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmGemPort,
                       &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmGemPort,
                       sizeof(KArg.omcipmGemPort));

        KArg.omcipmStatus = omcipm_gemport_set(&KArg.omcipmGemPort);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_GEM_PORT)arg)->omcipmStatus);
}

/**
 * IOCTL FEC functions
 **/

void omcipm_createFec(unsigned long arg)
{
    STS_OMCI_PM_FEC KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmFec,
                       &((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                       sizeof(KArg.omcipmFec));

        KArg.omcipmStatus = omcipm_fec_create(&KArg.omcipmFec);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_FEC)arg)->omcipmStatus);
}

void omcipm_deleteFec(unsigned long arg)
{
    STS_OMCI_PM_FEC KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmFec,
                       &((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                       sizeof(KArg.omcipmFec));

        KArg.omcipmStatus = omcipm_fec_delete(KArg.omcipmFec.omcipm.omcipmId);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_FEC)arg)->omcipmStatus);
}

void omcipm_getFec(unsigned long arg)
{
    STS_OMCI_PM_FEC KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmFec,
                       &((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                       sizeof(KArg.omcipmFec));

        KArg.omcipmStatus = omcipm_fec_get(&KArg.omcipmFec);

        copy_to_user(&((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                     &KArg.omcipmFec,
                     sizeof(KArg.omcipmFec));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_FEC)arg)->omcipmStatus);
}

void omcipm_getCurrentFec(unsigned long arg)
{
    STS_OMCI_PM_FEC KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmFec,
                       &((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                       sizeof(KArg.omcipmFec));

        KArg.omcipmStatus = omcipm_fec_getCurrent(&KArg.omcipmFec);

        copy_to_user(&((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                     &KArg.omcipmFec,
                     sizeof(KArg.omcipmFec));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_FEC)arg)->omcipmStatus);
}

void omcipm_setFec(unsigned long arg)
{
    STS_OMCI_PM_FEC KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_GPON) == TRUE)
    {
        copy_from_user(&KArg.omcipmFec,
                       &((PSTS_OMCI_PM_FEC)arg)->omcipmFec,
                       sizeof(KArg.omcipmFec));

        KArg.omcipmStatus = omcipm_fec_set(&KArg.omcipmFec);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_FEC)arg)->omcipmStatus);
}
