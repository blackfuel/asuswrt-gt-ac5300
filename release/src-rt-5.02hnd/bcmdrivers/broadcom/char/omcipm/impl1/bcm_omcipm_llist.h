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

#ifndef _BCM_OMCIPM_LLIST_H_
#define _BCM_OMCIPM_LLIST_H_

#include "bcm_common_llist.h"
#include "bcm_omcipm_api.h"

#define BCM_OMCI_PM_POLLING_RATE_15_MINS 15 * 3600 * HZ  /* 15 minutes to jiffies*/
#define BCM_OMCI_PM_POLLING_RATE_3_SEC 3 * HZ  /* 3 seconds is polling rate to prevent counter saturation*/
//#define BCM_OMCI_PM_POLLING_RATE_15_MINS 5 * HZ  /* 5 seconds to jiffies*/
//#define BCM_OMCI_PM_POLLING_RATE_3_SEC 1 * HZ  /* 1 seconds to jiffies*/

#define BCM_OMCI_PM_COUNTER_VALUE_MAX 0xFFFFFFFF  /*MAX value of 32 bit counter*/

/* OMCI PM Class. */
typedef enum OMCI_PM_Class
{
    OMCI_PM_CLASS_GEM_PORT = 0,
    OMCI_PM_CLASS_FEC,
    OMCI_PM_CLASS_ENET,
    OMCI_PM_CLASS_ENET_2,
    OMCI_PM_CLASS_ENET_3,
    OMCI_PM_CLASS_MOCA_ENET,
    OMCI_PM_CLASS_MOCA_IF,
    OMCI_PM_CLASS_GAL_ENET,
    OMCI_PM_CLASS_BRIDGE,
    OMCI_PM_CLASS_BRIDGE_PORT,
    OMCI_PM_CLASS_MAX
} BCM_OMCI_PM_CLASS;

typedef enum {
    OMCIPM_COUNTER_TYPE_CURRENT=0,
    OMCIPM_COUNTER_TYPE_15MINS,
    OMCIPM_COUNTER_TYPE_MAX
} BCM_OMCIPM_COUNTER_TYPE;

typedef struct {
    BCM_COMMON_DECLARE_LL_ENTRY ();
    BCM_OMCI_PM_CLASS omcipmClass;
    UINT16 crossingMaskPrev;
    UINT16 crossingMaskCurr;
    void *pm[OMCIPM_COUNTER_TYPE_MAX];
    struct timer_list timer15Mins;
} BCM_OMCIPM_ENTRY, *PBCM_OMCIPM_ENTRY;

/**
 * Global OMCI PM Link List functions
 **/

BCM_OMCI_PM_STATUS omcipm_create(BCM_OMCI_PM_CLASS omcipmClass,
                                 UINT16 physPortId,
                                 UINT16 thresholdId);
BCM_OMCI_PM_STATUS omcipm_remove(BCM_OMCI_PM_CLASS omcipmClass,
                                 UINT16 physPortId);
BCM_OMCI_PM_STATUS omcipm_get(BCM_OMCI_PM_CLASS omcipmClass,
                              UINT16 physPortId, void *counters);
BCM_OMCI_PM_STATUS omcipm_getCurrent(BCM_OMCI_PM_CLASS omcipmClass,
                                     UINT16 physPortId,
                                     void *counters);
BCM_OMCI_PM_STATUS omcipm_set(BCM_OMCI_PM_CLASS omcipmClass,
                              UINT16 physPortId,
                              UINT16 thresholdId);
int    omcipm_init(void);
void   omcipm_exit(void);
void   omcipm_reInit(void);
BOOL   omcipm_alertThresholdCrossing(void);
UINT16 omcipm_retrieveThresholdCrossing(UINT8 *buf, UINT16 size);

/**
 * Global GEM Port functions
 **/

int  omcipm_gemport_init(void);
void omcipm_gemport_exit(void);

/**
 * Global FEC functions
 **/

int  omcipm_fec_init(void);
void omcipm_fec_exit(void);

/**
 * Global Ethernet functions
 **/

int  omcipm_enet_init(void);
void omcipm_enet_exit(void);

/**
 * Global Ethernet 2 functions
 **/

int  omcipm_enet2_init(void);
void omcipm_enet2_exit(void);

/**
 * Global Ethernet 3 functions
 **/

int  omcipm_enet3_init(void);
void omcipm_enet3_exit(void);

#endif /* _BCM_OMCIPM_LLIST_H_ */
