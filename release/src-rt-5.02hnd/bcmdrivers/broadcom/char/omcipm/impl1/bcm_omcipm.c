/*
<:copyright-BRCM:2015:proprietary:standard

   Copyright (c) 2015 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:>
*/

#include <linux/module.h>

#include "bcm_omcipm_dev.h"
#include "bcm_omcipm_user.h"
#include <linux/bcm_log.h>

#define VERSION     "0.1"
#define VER_STR     "v" VERSION

#define OMCIPM_PROC_ENTRY_NAME "omcipm"

static struct file_operations omcipm_proc_fops = {
    owner: THIS_MODULE
};

static int __init bcm_omcipm_init(void) 
{
    int rv = 0;
    struct proc_dir_entry *p;

    p = create_proc_entry(OMCIPM_PROC_ENTRY_NAME, 0, 0);
    if (!p) 
    {
        BCM_LOG_ERROR(BCM_LOG_ID_OMCIPM, "unable to create /proc/%s!", OMCIPM_PROC_ENTRY_NAME);
        return -EIO;
    }
    p->proc_fops = &omcipm_proc_fops;

    rv = omcipm_user_init();
    if (rv)
    {
        omcipm_user_cleanup();
    }
    else
    {
        rv = omcipm_dev_init();
        if (rv)
        {
            omcipm_user_cleanup();
            omcipm_dev_cleanup();
        }
        else
        {
            BCM_LOG_DEBUG(BCM_LOG_ID_OMCIPM, "Broadcom Performance Monitoring %s\n", VER_STR);
        }
    }

    return rv;
}

static void __exit bcm_omcipm_exit(void) 
{
    omcipm_dev_cleanup();
    omcipm_user_cleanup();
    remove_proc_entry(OMCIPM_PROC_ENTRY_NAME, NULL);
}

MODULE_DESCRIPTION("Broadcom Performance Monitoring Module");
MODULE_LICENSE("Proprietary");

module_init(bcm_omcipm_init);
module_exit(bcm_omcipm_exit);
