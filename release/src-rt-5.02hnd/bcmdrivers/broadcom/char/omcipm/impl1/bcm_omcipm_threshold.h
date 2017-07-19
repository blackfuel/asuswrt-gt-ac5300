/*
<:copyright-broadcom 
 
 Copyright (c) 2002 Broadcom Corporation 
 All Rights Reserved 
 No portions of this material may be reproduced in any form without the 
 written permission of: 
          Broadcom Corporation 
          16215 Alton Parkway 
          Irvine, California 92619 
 All information contained in this document is Broadcom Corporation 
 company private, proprietary, and trade secret. 
 
:>
*/

#ifndef _BCM_OMCIPM_THRESHOLD_H_
#define _BCM_OMCIPM_THRESHOLD_H_

#include "bcm_common_llist.h"
#include "bcm_omcipm_api.h"


/*
 * Type definitions
 */

typedef struct
{
    BCM_COMMON_DECLARE_LL_ENTRY ();
    BCM_OMCI_PM_THRESHOLD_DATA thresholdData;
} BCM_OMCIPM_THRESHOLD_ENTRY, *PBCM_OMCIPM_THRESHOLD_ENTRY;

/**
 * Global Threshold Data Table functions
 **/

int  omcipm_threshold_init(void);
void omcipm_threshold_exit(void);
PBCM_OMCIPM_THRESHOLD_ENTRY omcipm_threshold_find(UINT16 thresholdId);

/**
 * IOCTL Threshold functions
 **/

void omcipm_createThreshold(unsigned long arg);
void omcipm_deleteThreshold(unsigned long arg);
void omcipm_getThreshold(unsigned long arg);
void omcipm_setThreshold(unsigned long arg);

#endif /* _BCM_OMCIPM_THRESHOLD_H_ */
