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

#ifndef _BCM_OMCIPM_ENET_H_
#define _BCM_OMCIPM_ENET_H_

#include "bcm_omcipm_llist.h"
#include "bcm_omcipm_dev.h"

/**
 * IOCTL Ethernet functions
 **/

void omcipm_createENet(unsigned long arg);
void omcipm_deleteENet(unsigned long arg);
void omcipm_getENet(unsigned long arg);
void omcipm_getCurrentENet(unsigned long arg);
void omcipm_setENet(unsigned long arg);

/**
 * IOCTL Ethernet 2 functions
 **/

void omcipm_createENet2(unsigned long arg);
void omcipm_deleteENet2(unsigned long arg);
void omcipm_getENet2(unsigned long arg);
void omcipm_getCurrentENet2(unsigned long arg);
void omcipm_setENet2(unsigned long arg);

/**
 * IOCTL Ethernet 3 functions
 **/

void omcipm_createENet3(unsigned long arg);
void omcipm_deleteENet3(unsigned long arg);
void omcipm_getENet3(unsigned long arg);
void omcipm_getCurrentENet3(unsigned long arg);
void omcipm_setENet3(unsigned long arg);

#endif /* _BCM_OMCIPM_ENET_H_ */
