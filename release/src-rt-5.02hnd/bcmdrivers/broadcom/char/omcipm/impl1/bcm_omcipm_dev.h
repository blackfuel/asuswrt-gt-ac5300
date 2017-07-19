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
#ifndef _BCM_OMCI_PM_DEV_
#define _BCM_OMCI_PM_DEV_

#include "bcm_omcipm_device.h"

/**
 * Functions are used by OMCI PM driver only
 **/

BCM_OMCI_PM_STATUS omcipm_dev_init(void);
void omcipm_dev_cleanup(void);
BOOL omcipm_dev_isRegistered(IN BCM_OMCI_PM_DEVICE_ID omcipmDeviceId);
BCM_OMCI_PM_STATUS omcipm_dev_getCounters(IN  BCM_OMCI_PM_DEVICE_ID omcipmDeviceId,
                                          IN  UINT16 physPortId,
                                          OUT void   *counters,
                                          IN  BOOL   reset);

#endif /*_BCM_OMCI_PM_DEV_*/
