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

#include "bcm_omcipm_enet.h"

/**
 * Ethernet local functions
 **/

static BCM_OMCI_PM_STATUS omcipm_enet_create(PBCM_OMCI_PM_ETHERNET enet)
{
    BCM_OMCI_PM_STATUS ret = omcipm_create(OMCI_PM_CLASS_ENET,
                                           enet->omcipm.omcipmId,
                                           enet->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully create Ethernet Id: %d, Threshold Id: %d",
                     enet->omcipm.omcipmId, enet->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet Id: %d, Status: %d",
                      enet->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_delete(UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = omcipm_remove(OMCI_PM_CLASS_ENET, physPortId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully delete Ethernet Id: %d",
                     physPortId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet Id: %d, Status: %d",
                      physPortId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_get(PBCM_OMCI_PM_ETHERNET enet)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_get(OMCI_PM_CLASS_ENET,
                     enet->omcipm.omcipmId,
                     (void *)enet);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get Ethernet Id: %d",
                     enet->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get Ethernet Id: %d, Status: %d",
                      enet->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_getCurrent(PBCM_OMCI_PM_ETHERNET enet)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_getCurrent(OMCI_PM_CLASS_ENET,
                            enet->omcipm.omcipmId,
                            (void *)enet);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get current Ethernet Id: %d",
                     enet->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get current Ethernet Id: %d, Status: %d",
                      enet->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_set(PBCM_OMCI_PM_ETHERNET enet)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_set(OMCI_PM_CLASS_ENET,
                     enet->omcipm.omcipmId,
                     enet->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully set Ethernet Id: %d, Threshold Id: %d",
                     enet->omcipm.omcipmId, enet->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to set Ethernet Id: %d, Status: %d",
                      enet->omcipm.omcipmId, ret);
    }

    return ret;
}

/**
 * Ethernet 2 local functions
 **/

static BCM_OMCI_PM_STATUS omcipm_enet_2_create(PBCM_OMCI_PM_ETHERNET_2 enet_2)
{
    BCM_OMCI_PM_STATUS ret = omcipm_create(OMCI_PM_CLASS_ENET_2,
                                           enet_2->omcipm.omcipmId,
                                           enet_2->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully create Ethernet 2 Id: %d, Threshold Id: %d",
                     enet_2->omcipm.omcipmId, enet_2->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet 2 Id: %d, Status: %d",
                      enet_2->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_2_delete(UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = omcipm_remove(OMCI_PM_CLASS_ENET_2, physPortId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully delete Ethernet 2 Id: %d",
                     physPortId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet 2 Id: %d, Status: %d",
                      physPortId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_2_get(PBCM_OMCI_PM_ETHERNET_2 enet_2)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_get(OMCI_PM_CLASS_ENET_2,
                     enet_2->omcipm.omcipmId,
                     (void *)enet_2);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get Ethernet 2 Id: %d",
                     enet_2->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get Ethernet 2 Id: %d, Status: %d",
                      enet_2->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_2_getCurrent(PBCM_OMCI_PM_ETHERNET_2 enet_2)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_getCurrent(OMCI_PM_CLASS_ENET_2,
                            enet_2->omcipm.omcipmId,
                            (void *)enet_2);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get current Ethernet 2 Id: %d",
                     enet_2->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get current Ethernet 2 Id: %d,  Status: %d",
                      enet_2->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_2_set(PBCM_OMCI_PM_ETHERNET_2 enet_2)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_set(OMCI_PM_CLASS_ENET_2,
                     enet_2->omcipm.omcipmId,
                     enet_2->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully set Ethernet 2 Id: %d, Threshold Id: %d",
                     enet_2->omcipm.omcipmId, enet_2->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to set Ethernet 2 Id: %d, Status: %d",
                      enet_2->omcipm.omcipmId, ret);
    }

    return ret;
}

/**
 * Ethernet 3 local functions
 **/

static BCM_OMCI_PM_STATUS omcipm_enet_3_create(PBCM_OMCI_PM_ETHERNET_3 enet_3)
{
    BCM_OMCI_PM_STATUS ret = omcipm_create(OMCI_PM_CLASS_ENET_3,
                                           enet_3->omcipm.omcipmId,
                                           enet_3->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully create Ethernet 3 Id: %d, Threshold Id: %d",
                     enet_3->omcipm.omcipmId, enet_3->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet 3 Id: %d, Status: %d",
                      enet_3->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_3_delete(UINT16 physPortId)
{
    BCM_OMCI_PM_STATUS ret = omcipm_remove(OMCI_PM_CLASS_ENET_3, physPortId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully delete Ethernet 3 Id: %d",
                     physPortId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to delete Ethernet 3 Id: %d, Status: %d",
                      physPortId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_3_get(PBCM_OMCI_PM_ETHERNET_3 enet_3)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_get(OMCI_PM_CLASS_ENET_3,
                     enet_3->omcipm.omcipmId,
                     (void *)enet_3);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get Ethernet 3 Id: %d",
                     enet_3->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get Ethernet 3 Id: %d, Status: %d",
                      enet_3->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_3_getCurrent(PBCM_OMCI_PM_ETHERNET_3 enet_3)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_getCurrent(OMCI_PM_CLASS_ENET_3,
                            enet_3->omcipm.omcipmId,
                            (void *)enet_3);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully get current Ethernet 3 Id: %d",
                     enet_3->omcipm.omcipmId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to get current Ethernet 3 Id: %d, Status: %d",
                      enet_3->omcipm.omcipmId, ret);
    }

    return ret;
}

static BCM_OMCI_PM_STATUS omcipm_enet_3_set(PBCM_OMCI_PM_ETHERNET_3 enet_3)
{
    BCM_OMCI_PM_STATUS ret = OMCI_PM_STATUS_SUCCESS;

    ret = omcipm_set(OMCI_PM_CLASS_ENET_3,
                     enet_3->omcipm.omcipmId,
                     enet_3->omcipm.thresholdId);

    if (ret == OMCI_PM_STATUS_SUCCESS)
    {
        BCM_LOG_INFO(BCM_LOG_ID_OMCIPM,
                     "Successfully set Ethernet 3 Id: %d, Threshold Id: %d",
                     enet_3->omcipm.omcipmId, enet_3->omcipm.thresholdId);
    }
    else
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM,
                      "Fail to set Ethernet 3 Id: %d, Status: %d",
                      enet_3->omcipm.omcipmId, ret);
    }

    return ret;
}

/**
 * IOCTL Ethernet functions
 **/

void omcipm_createENet(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet,
                       &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                       sizeof(KArg.omcipmEthernet));

        KArg.omcipmStatus = omcipm_enet_create(&KArg.omcipmEthernet);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmStatus);
}

void omcipm_deleteENet(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet,
                       &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                       sizeof(KArg.omcipmEthernet));

        KArg.omcipmStatus = omcipm_enet_delete(KArg.omcipmEthernet.omcipm.omcipmId);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmStatus);
}

void omcipm_getENet(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet,
                       &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                       sizeof(KArg.omcipmEthernet));

        KArg.omcipmStatus = omcipm_enet_get(&KArg.omcipmEthernet);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                     &KArg.omcipmEthernet,
                     sizeof(KArg.omcipmEthernet));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmStatus);
}

void omcipm_getCurrentENet(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet,
                       &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                       sizeof(KArg.omcipmEthernet));

        KArg.omcipmStatus = omcipm_enet_getCurrent(&KArg.omcipmEthernet);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                     &KArg.omcipmEthernet,
                     sizeof(KArg.omcipmEthernet));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmStatus);
}

void omcipm_setENet(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet,
                       &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmEthernet,
                       sizeof(KArg.omcipmEthernet));

        KArg.omcipmStatus = omcipm_enet_set(&KArg.omcipmEthernet);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET)arg)->omcipmStatus);
}

/**
 * IOCTL Ethernet 2 functions
 **/

void omcipm_createENet2(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_2 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet2,
                       &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                       sizeof(KArg.omcipmEthernet2));

        KArg.omcipmStatus = omcipm_enet_2_create(&KArg.omcipmEthernet2);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmStatus);
}

void omcipm_deleteENet2(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_2 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet2,
                       &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                       sizeof(KArg.omcipmEthernet2));

        KArg.omcipmStatus = omcipm_enet_2_delete(KArg.omcipmEthernet2.omcipm.omcipmId);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmStatus);
}

void omcipm_getENet2(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_2 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet2,
                       &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                       sizeof(KArg.omcipmEthernet2));

        KArg.omcipmStatus = omcipm_enet_2_get(&KArg.omcipmEthernet2);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                     &KArg.omcipmEthernet2,
                     sizeof(KArg.omcipmEthernet2));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmStatus);
}

void omcipm_getCurrentENet2(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_2 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet2,
                       &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                       sizeof(KArg.omcipmEthernet2));

        KArg.omcipmStatus = omcipm_enet_2_getCurrent(&KArg.omcipmEthernet2);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                     &KArg.omcipmEthernet2,
                     sizeof(KArg.omcipmEthernet2));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmStatus);
}

void omcipm_setENet2(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_2 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet2,
                       &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmEthernet2,
                       sizeof(KArg.omcipmEthernet2));

        KArg.omcipmStatus = omcipm_enet_2_set(&KArg.omcipmEthernet2);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_2)arg)->omcipmStatus);
}

/**
 * IOCTL Ethernet 3 functions
 **/

void omcipm_createENet3(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_3 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet3,
                       &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                       sizeof(KArg.omcipmEthernet3));

        KArg.omcipmStatus = omcipm_enet_3_create(&KArg.omcipmEthernet3);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmStatus);
}

void omcipm_deleteENet3(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_3 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet3,
                       &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                       sizeof(KArg.omcipmEthernet3));

        KArg.omcipmStatus = omcipm_enet_3_delete(KArg.omcipmEthernet3.omcipm.omcipmId);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmStatus);
}

void omcipm_getENet3(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_3 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet3,
                       &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                       sizeof(KArg.omcipmEthernet3));

        KArg.omcipmStatus = omcipm_enet_3_get(&KArg.omcipmEthernet3);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                     &KArg.omcipmEthernet3,
                     sizeof(KArg.omcipmEthernet3));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmStatus);
}

void omcipm_getCurrentENet3(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_3 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet3,
                       &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                       sizeof(KArg.omcipmEthernet3));

        KArg.omcipmStatus = omcipm_enet_3_getCurrent(&KArg.omcipmEthernet3);

        copy_to_user(&((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                     &KArg.omcipmEthernet3,
                     sizeof(KArg.omcipmEthernet3));
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmStatus);
}

void omcipm_setENet3(unsigned long arg)
{
    STS_OMCI_PM_ETHERNET_3 KArg;

    BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM,"");

    KArg.omcipmStatus = OMCI_PM_STATUS_NOT_REGISTERED;

    if (omcipm_dev_isRegistered(BCM_OMCI_PM_DEVICE_ENET) == TRUE)
    {
        copy_from_user(&KArg.omcipmEthernet3,
                       &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmEthernet3,
                       sizeof(KArg.omcipmEthernet3));

        KArg.omcipmStatus = omcipm_enet_3_set(&KArg.omcipmEthernet3);
    }

    put_user(KArg.omcipmStatus, &((PSTS_OMCI_PM_ETHERNET_3)arg)->omcipmStatus);
}
