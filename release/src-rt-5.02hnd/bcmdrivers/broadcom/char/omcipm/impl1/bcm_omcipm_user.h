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

#ifndef _BCM_OMCIPM_USER_H_
#define _BCM_OMCIPM_USER_H_

#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/types.h>

int __devinit omcipm_user_init(void);
void __exit omcipm_user_cleanup(void);

#endif /* _BCM_OMCIPM_USER_H_ */
