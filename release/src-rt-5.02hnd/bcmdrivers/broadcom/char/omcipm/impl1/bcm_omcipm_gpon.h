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

#ifndef _BCM_OMCIPM_GPON_H_
#define _BCM_OMCIPM_GPON_H_

#include "bcm_omcipm_llist.h"
#include "bcm_omcipm_dev.h"

/**
 * IOCTL GEM Port functions
 **/

void omcipm_createGemPort(unsigned long arg);
void omcipm_deleteGemPort(unsigned long arg);
void omcipm_getGemPort(unsigned long arg);
void omcipm_getCurrentGemPort(unsigned long arg);
void omcipm_setGemPort(unsigned long arg);

/**
 * IOCTL FEC functions
 **/

void omcipm_createFec(unsigned long arg);
void omcipm_deleteFec(unsigned long arg);
void omcipm_getFec(unsigned long arg);
void omcipm_getCurrentFec(unsigned long arg);
void omcipm_setFec(unsigned long arg);

#endif /* _BCM_OMCIPM_GPON_H_ */
