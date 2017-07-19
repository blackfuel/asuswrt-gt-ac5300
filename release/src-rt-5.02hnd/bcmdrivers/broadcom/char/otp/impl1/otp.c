/*
<:copyright-BRCM:2011:proprietary:standard

   Copyright (c) 2011 Broadcom 
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
/***************************************************************************
* File Name  : otp.c
*
* Description: provides IOCTLS to provide otp read/write to userspace
*
*
***************************************************************************/

/* Includes. */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>


#include <bcm_map.h>
#include <otp_ioctl.h>
#include <linux/bcm_log.h>

/* Typedefs. */

/* Prototypes. */
static int otp_open(struct inode *inode, struct file *filp);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
static int otp_ioctl(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg);
#endif
static int otp_release(struct inode *inode, struct file *filp);
static int __init brcm_otp_init(void);
void __exit brcm_otp_cleanup(void);


uint32_t otp_major = 0;

void __exit brcm_otp_cleanup(void)
{
}


static int otp_open(struct inode *inode, struct file *filp)
{

    return( 0 );
}

static int otp_release(struct inode *inode, struct file *filp)
{

    return( 0 );
}

static int otp_fuse_bootrom_enable(void);
static int otp_fuse_operator_enable(void);
static int otp_fuse_mid(int id);
static int otp_fuse_oid(int id);
static int otp_get_bootrom_enable(int *pRes);
static int otp_get_operator_enable(int *pRes);
static int otp_get_mid(int *pRes);
static int otp_get_oid(int *pRes);

//********************************************************************************************
// misc. ioctl calls come to here. (flash, led, reset, kernel memory access, etc.)
//********************************************************************************************
static int otp_ioctl(struct inode *inode, struct file *flip,
                     unsigned int command, unsigned long arg)
{
    int ret = -EINVAL;
    OTP_IOCTL_PARMS ctrlParms;

    switch (command) {
    case OTP_IOCTL_GET:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
        {
            switch (ctrlParms.action) {
            case OTP_BTRM_ENABLE_BIT:
                ret = otp_get_bootrom_enable(&ctrlParms.result);
		break;
            case OTP_OPERATOR_ENABLE_BIT:
                ret = otp_get_operator_enable(&ctrlParms.result);
		break;
            case OTP_MID_BITS:
                ret = otp_get_mid(&ctrlParms.result);
		break;
            case OTP_OID_BITS:
                ret = otp_get_oid(&ctrlParms.result);
		break;
	    default:
		ret = -1;
	    }
        }
        if (ret == 0) { 
             copy_to_user((void *)arg, (void *) &ctrlParms, sizeof(ctrlParms));
        }

        break;

    case OTP_IOCTL_SET:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
        {

            switch (ctrlParms.action) {
            case OTP_BTRM_ENABLE_BIT:
                ret = otp_fuse_bootrom_enable();
		break;
            case OTP_OPERATOR_ENABLE_BIT:
                ret = otp_fuse_operator_enable();
		break;
            case OTP_MID_BITS:
                ret = otp_fuse_mid(ctrlParms.id);
		break;
            case OTP_OID_BITS:
                ret = otp_fuse_oid(ctrlParms.id);
		break;
	    default:
		ret = -1;
	    }
        }
        if (ret == 0) { 
             ctrlParms.result = 0x0;
             copy_to_user((void *)arg, (void *) &ctrlParms, sizeof(ctrlParms));
        }

        break;
    default:
        break;
    }

    return (ret);

} /* board_ioctl */




#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
static DEFINE_MUTEX(otpIoctlMutex);

static long unlocked_otp_ioctl(struct file *filep, unsigned int cmd, 
                              unsigned long arg)
{
    struct inode *inode;
    long rt;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    inode = filep->f_dentry->d_inode;
#else
    inode = file_inode(filep);
#endif

    mutex_lock(&otpIoctlMutex);
    rt = otp_ioctl( inode, filep, cmd, arg );
    mutex_unlock(&otpIoctlMutex);
    
    return rt;
}
#endif

static struct file_operations otp_fops =
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 33)
    .unlocked_ioctl 	= unlocked_otp_ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = unlocked_otp_ioctl,
#endif
#else
    .ioctl   		= otp_ioctl,
#endif
    .open    		= otp_open,
    .release 		= otp_release,
};



#if defined(CONFIG_BCM96838)
static void otp_wait_done(void);
static void otp_wait_done()
{
   msleep(2);
   while ((OTP->Status & 0xfefff) != 
          (OTP_STATUS_CMD_DONE | OTP_STATUS_WRP_DATA_READY | 
	   OTP_STATUS_WRP_FDONE | OTP_STATUS_OTP_READY)) 
      msleep(2);

   OTP->Control = 0;
}

static void otp_prog_enable(int val);
static void otp_prog_enable(int val) 
{
   OTP->Bitsel = val;
   OTP->Control = OTP_CONTROL_CMD_PROG_ENAB | OTP_CONTROL_START;
   otp_wait_done();
}

static void otp_fuse(int addr, int data);
static void otp_fuse(int addr, int data)
{
   // poll for otp_ready, enable cpu mode, point the address register to the proper customer row
   while ((OTP->Status & OTP_STATUS_OTP_READY) != OTP_STATUS_OTP_READY)
      msleep(2);

   OTP->Config = OTP_CONFIG_CPU_MODE; 
   OTP->Addr = addr;

   // perform the prog enable sequence
   otp_prog_enable(0xf);
   otp_prog_enable(0x4);
   otp_prog_enable(0x8);
   otp_prog_enable(0xd);

   OTP->WriteData = data;
   OTP->Control = OTP_CONTROL_START | OTP_CONTROL_CMD_PROG_WORD | OTP_CONTROL_PROG_EN | OTP_CONTROL_ACCESS_MODE;

   while ((OTP->Status & OTP_STATUS_OTP_READY) != OTP_STATUS_OTP_READY)
      msleep(2);

   if ((OTP->Status & 0xffff0) == (OTP_STATUS_WRP_FDONE | OTP_STATUS_PROG_OK | OTP_STATUS_OTP_READY))
      printk("OTP fusing has completed as expected\n");
   else
      printk("OTP fusing didn't complete as expected\n");

   OTP->Control = 0;
}

#else

static void otp_wait_done(void);
static void otp_wait_done()
{
   while((JTAGOTP->status1 & 0x2) == 0x0)
      msleep(5);
}

static void otp_fuse(int addr, int data);
static void otp_fuse(int addr, int data)
{
   int i;
   int authVal[4] = {0xf,0x4,0x8,0xd};

   JTAGOTP->ctrl1 |= JTAG_OTP_CTRL_CPU_MODE;
   for (i=0;i<4;i++)
   {
      JTAGOTP->ctrl2 = authVal[i];
      JTAGOTP->ctrl0 = JTAG_OTP_CTRL_START | JTAG_OTP_CTRL_CMD_OTP_PROG_EN | JTAG_OTP_CTRL_PROG_EN;
      msleep(5);
      JTAGOTP->ctrl0 = 0x0;
      msleep(5);
   }
   otp_wait_done();
   if ((JTAGOTP->status1 & 0x4) == 0x4)
   {
      JTAGOTP->ctrl2 = data;
      JTAGOTP->ctrl3 = addr;
      JTAGOTP->ctrl0 = JTAG_OTP_CTRL_START | JTAG_OTP_CTRL_CMD_PROG | (0x2 << 22) | JTAG_OTP_CTRL_PROG_EN;
      otp_wait_done();
      JTAGOTP->ctrl0 = 0x0;
      printk("OTP fusing has completed as expected\n");
   }
   else
      printk("OTP fusing didn't complete as expected\n");

   JTAGOTP->ctrl1 &= ~JTAG_OTP_CTRL_CPU_MODE;
}

static int otp_read_row(int row);
static int otp_read_row(int row)
{
   int      rval = 0;
   uint32_t cntr = BTRM_OTP_READ_TIMEOUT_CNT;

   /* turn on cpu mode, set up row addr, activate read word */
   JTAGOTP->ctrl1 |= JTAG_OTP_CTRL_CPU_MODE;
   JTAGOTP->ctrl3 = row;
   JTAGOTP->ctrl0 = JTAG_OTP_CTRL_START | JTAG_OTP_CTRL_PROG_EN | JTAG_OTP_CTRL_ACCESS_MODE;

   /* Wait for low CMD_DONE (current operation has begun), reset countdown, wait for retrieval to complete */
   while((JTAGOTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE)
   {
      cntr--;
      if (cntr == 0)
         break;
   }
   cntr = BTRM_OTP_READ_TIMEOUT_CNT;
   while(!((JTAGOTP->status1) & JTAG_OTP_STATUS_1_CMD_DONE))
   {
      cntr--;
      if (cntr == 0)
         break;
   }

   /* If cntr nonzero, read was successful, retrieve data */
   if (cntr)
      rval = JTAGOTP->status0;

   /* zero out the ctrl_0 reg, turn off cpu mode, return results */
   JTAGOTP->ctrl0 = 0;
   JTAGOTP->ctrl1 &= ~JTAG_OTP_CTRL_CPU_MODE;
   return rval;
}

#endif


static int otp_fuse_bootrom_enable(void)
{
   int ret = 0;
   otp_fuse(OTP_CUST_BTRM_BOOT_ENABLE_FUSE_ROW, OTP_CUST_BTRM_BOOT_ENABLE_MASK);
   return ret;
}

static int otp_fuse_operator_enable(void)
{
   int ret = 0;
   otp_fuse(OTP_CUST_OP_INUSE_FUSE_ROW, OTP_CUST_OP_INUSE_MASK);
   return ret;
}

static int otp_fuse_mid(int id)
{
   int ret = 0;
   otp_fuse(OTP_CUST_MFG_MRKTID_FUSE_ROW, ((id << OTP_CUST_MFG_MRKTID_SHIFT) & OTP_CUST_MFG_MRKTID_MASK));
   return ret;
}

static int otp_fuse_oid(int id)
{
   int ret = 0;
   otp_fuse(OTP_CUST_OP_MRKTID_FUSE_ROW, ((id << OTP_CUST_OP_MRKTID_SHIFT) & OTP_CUST_OP_MRKTID_MASK));
   return ret;
}

static int otp_get_bootrom_enable(int *pRes)
{
   int ret = 0;
#if defined(CONFIG_BCM96838)
   *pRes = (*((uint32_t *)(OTP_BASE + OTP_CUST_BTRM_BOOT_ENABLE_ROW)));
#else
   *pRes = otp_read_row(OTP_CUST_BTRM_BOOT_ENABLE_ROW);
#endif
   *pRes = (*pRes & OTP_CUST_BTRM_BOOT_ENABLE_MASK) >> OTP_CUST_BTRM_BOOT_ENABLE_SHIFT;
   return ret;
}

static int otp_get_operator_enable(int *pRes)
{
   int ret = 0;
#if defined(CONFIG_BCM96838)
   *pRes = (*((uint32_t *)(OTP_BASE + OTP_CUST_OP_INUSE_ROW)));
#else
   *pRes = otp_read_row(OTP_CUST_OP_INUSE_ROW);
#endif
   *pRes = (*pRes & OTP_CUST_OP_INUSE_MASK) >> OTP_CUST_OP_INUSE_SHIFT;
   return ret;
}

static int otp_get_mid(int *pRes)
{
   int ret = 0;
#if defined(CONFIG_BCM96838)
   *pRes = (*((uint32_t *)(OTP_BASE + OTP_CUST_MFG_MRKTID_ROW)));
#else
   *pRes = otp_read_row(OTP_CUST_MFG_MRKTID_ROW);
#endif
   *pRes = (*pRes & OTP_CUST_MFG_MRKTID_MASK) >> OTP_CUST_MFG_MRKTID_SHIFT;
   return ret;
}

static int otp_get_oid(int *pRes)
{
   int ret = 0;
#if defined(CONFIG_BCM96838)
   *pRes = (*((uint32_t *)(OTP_BASE + OTP_CUST_OP_MRKTID_ROW)));
#else
   *pRes = otp_read_row(OTP_CUST_OP_MRKTID_ROW);
#endif
   *pRes = (*pRes & OTP_CUST_OP_MRKTID_MASK) >> OTP_CUST_OP_MRKTID_SHIFT;
   return ret;
}


static int __init brcm_otp_init(void)
{
    int ret;

    ret = register_chrdev(OTP_DRV_MAJOR, "otp", &otp_fops );
    if (ret < 0)
        printk( "brcm_otp_init(major %d): failed to register device.\n",OTP_DRV_MAJOR);
    else
    {
        printk("brcm_otp_init entry\n");
        otp_major = OTP_DRV_MAJOR;
    }

   return ret;
}

/***************************************************************************
* MACRO to call driver initialization and cleanup functions.
***************************************************************************/
module_init(brcm_otp_init);
module_exit(brcm_otp_cleanup);

MODULE_LICENSE("proprietary");

