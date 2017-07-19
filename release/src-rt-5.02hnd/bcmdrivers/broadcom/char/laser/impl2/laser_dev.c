/*---------------------------------------------------------------------------

<:copyright-BRCM:2013:proprietary:standard 

   Copyright (c) 2013 Broadcom 
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
 ------------------------------------------------------------------------- */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <laser.h>
#include <gpon_i2c.h>
#include <boardparms.h>
#include <board.h>
#include "bl_lilac_i2c.h"

#if defined(CONFIG_BCM_I2C_BUS) || defined(CONFIG_BCM_I2C_BUS_MODULE)
// 6816 limits i2c data blocks to a max of 32 bytes, but requires   
// the first byte to be the offset.
#define MAX_I2C_MSG_SIZE                    32 /* BCM6816 limit */
#else
#define MAX_I2C_MSG_SIZE                    64 /* arbitrary limit */
#endif

/* character major device number */

#define LASER_DEV_MAJOR   239
#define LASER_DEV_CLASS   "laser_dev"

#define LASER_DEV_LILAC_STATIC_PARAMETERS_ADDRESS    0xA0    
#define LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS 0xA2   
#define LASER_DEV_LILAC_TX_PWR_MSB_OFFSET       102 /* Measured Output Power */
#define LASER_DEV_LILAC_TX_PWR_LSB_OFFSET       103
#define LASER_DEV_LILAC_RX_PWR_MSB_OFFSET       104 /* Measured Input Power */
#define LASER_DEV_LILAC_RX_PWR_LSB_OFFSET       105
#define LASER_DEV_LILAC_TEMP_LSB_OFFSET         97  /* Internally measured module temperature */
#define LASER_DEV_LILAC_TEMP_MSB_OFFSET         96
#define LASER_DEV_LILAC_VOTAGE_LSB_OFFSET       99  /* Internally measured supply voltage in transceiver */
#define LASER_DEV_LILAC_VOTAGE_MSB_OFFSET       98
#define LASER_DEV_LILAC_BIAS_CURRENT_LSB_OFFSET 101 /* Internally measured TX Bias Current */
#define LASER_DEV_LILAC_BIAS_CURRENT_MSB_OFFSET 100

 
struct mutex laser_mutex;

u16 (*Laser_Dev_Ioctl_Get_Tx_Optical_Pwr)(void);
u16 (*Laser_Dev_Ioctl_Get_Rx_Optical_Pwr)(void);
u16 (*Laser_Dev_Ioctl_Get_Temp)(void);
u16 (*Laser_Dev_Ioctl_Get_Votage)(void);
u16 (*Laser_Dev_Ioctl_Get_Bias_Current)(void);


/* Global definition */
BL_LILAC_I2C_CONF i2c_params;


/******************************************************************************
* Function    :   Laser_Dev_File_Open
*             :
* Description :   This routine implements the device open function but currently
*             :   is just a stub.
*             :
* Params      :   None
*             :
* Returns     :   0
******************************************************************************/
static int Laser_Dev_File_Open(struct inode *inode, struct file *file)
{
    return 0;
}

/******************************************************************************
* Function    :   Laser_Dev_File_Release
*             :
* Description :   This routine implements the device release function but 
*             :   currently is just a stub.
*             :
* Params      :   None
*             :
* Returns     :   0
******************************************************************************/
static int Laser_Dev_File_Release(struct inode *inode, struct file *file)
{
    return 0;
}

/*******************************************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_lilac_Tx_Optical_Pwr
*             :
* Description :   This routine returns the tranceiver Tx optical power based on SFF-8472 Specification.
*             :
* Params      :
*             :
* Returns     :   Tx optical power in micro-watts
*********************************************************************************************************/
static u16 Laser_Dev_Ioctl_Get_lilac_Tx_Optical_Pwr(void)
{   
    BL_LILAC_I2C_STATUS i2cStatus;
    u16 txPwr ;
    uint32_t data_len = 2;
    
    i2cStatus = bl_lilac_i2c_read ( &i2c_params, 
                                    (unsigned char)LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS, 
                                    LASER_DEV_LILAC_TX_PWR_MSB_OFFSET,
                                    I2C_REG_WIDTH_8,
                                    (uint8_t*)&txPwr, 
                                    data_len);
    if ( i2cStatus != I2C_OK )
    {
        printk ("I2C: Error reading from device.\n");
        return (-1);
    }

    return ( txPwr );   
}

/*******************************************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_lilac_Rx_Optical_Pwr
*             :
* Description :   This routine returns tranceiver Rx optical power based on SFF-8472 Specification.
*             :
* Params      :
*             :
* Returns     :   Rx optical power in micro-watts
*********************************************************************************************************/
static u16 Laser_Dev_Ioctl_Get_lilac_Rx_Optical_Pwr(void)
{
    BL_LILAC_I2C_STATUS i2cStatus;
    u16 rxPwr ;
    uint32_t data_len = 2;
    
    i2cStatus = bl_lilac_i2c_read ( &i2c_params, 
                                    (unsigned char)LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS, 
                                    LASER_DEV_LILAC_RX_PWR_MSB_OFFSET,
                                    I2C_REG_WIDTH_8,
                                    (uint8_t*)&rxPwr, 
                                    data_len);
    if ( i2cStatus != I2C_OK )
    {
        printk ("I2C: Error reading from device.\n");
        return (-1);
    }

    return ( rxPwr );   

}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_lilac_Laser_Temp
*             :
* Description :   This routine returns internally measured module temperature 
*             :   based on SFF-8472 Specification.
*             :
* Params      :   None
*             :
* Returns     :   Temperature [c]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_lilac_Laser_Temp(void)
{
    BL_LILAC_I2C_STATUS i2cStatus;
    u16 temp ;
    uint32_t data_len = 2;
    
    i2cStatus = bl_lilac_i2c_read ( &i2c_params, 
                                    (unsigned char)LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS, 
                                    LASER_DEV_LILAC_TEMP_MSB_OFFSET,
                                    I2C_REG_WIDTH_8,
                                    (uint8_t*)&temp, 
                                    data_len);
    if ( i2cStatus != I2C_OK )
    {
        printk ("I2C: Error reading from device.\n");
        return (-1);
    }

    return ( temp );
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_lilac_Laser_Votage
*             :
* Description :   This routine returns internally measured supply voltage in transceiver 
*             :   based on SFF-8472 Specification.
*             :   Internally measured transceiver supply voltage represented as a 16 bit 
*             :   unsigned integer with the voltage defined as the full 16 bit value 
*             :   (0 - 65535) with LSB equal to 100 uVolt, yielding a total range of 0 to +6.55 Volts.
*             :
* Params      :   None
*             :   
* Returns     :   Voltage [mV]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_lilac_Laser_Votage(void)
{
    BL_LILAC_I2C_STATUS i2cStatus;
    u16 voltage ;
    uint32_t data_len = 2;
    
    i2cStatus = bl_lilac_i2c_read ( &i2c_params, 
                                    (unsigned char)LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS, 
                                    LASER_DEV_LILAC_VOTAGE_MSB_OFFSET,
                                    I2C_REG_WIDTH_8,
                                    (uint8_t*)&voltage, 
                                    data_len);
    if ( i2cStatus != I2C_OK )
    {
        printk ("I2C: Error reading from device.\n");
        return (-1);
    }

    return ( voltage );   
}
/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_lilac_Laser_Bias_Current
*             :
*             : 
* Description :   This routine returns internally measured TX Bias current in transceiver 
*             :   based on SFF-8472 Specification.
*             :   Measured TX bias current represented as a 16 bit unsigned integer with the 
*             :   current defined as the full 16 bit value (0 - 65535) with LSB equal to 2 uA, 
*             :   yielding a total range of 0 to 131 mA.
*             :
* Params      :   None
*             :   
* Returns     :   Bias [uA]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_lilac_Laser_Bias_Current(void)
{
    BL_LILAC_I2C_STATUS i2cStatus;
    u16 bias ;
    uint32_t data_len = 2;
    
    i2cStatus = bl_lilac_i2c_read ( &i2c_params, 
                                    (unsigned char)LASER_DEV_LILAC_REAL_TIME_DIAGNOSTIC_ADDRESS, 
                                    LASER_DEV_LILAC_BIAS_CURRENT_MSB_OFFSET,
                                    I2C_REG_WIDTH_8,
                                    (uint8_t*)&bias, 
                                    data_len);
    if ( i2cStatus != I2C_OK )
    {
        printk ("I2C: Error reading from device.\n");
        return (-1);
    }

    return ( bias );   
}


/******************************************************************************
* Function    :   Laser_Dev_File_Ioctl
*             :
* Description :   This routine catches all of the IOCTL calls made to this char
*             :   device after it has been opened.
*             :
* Params      :   struct file *file           :
*             :   [IN]unsigned int cmd        : indicates the operation to be 
*             :                                 performed
*             :   [IN/OUT]unsigned long arg   : pointer to user args for i/o
*             :
* Returns     :   Result of IOCTL call
******************************************************************************/
static long Laser_Dev_File_Ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

    unsigned long * pPwr = (unsigned long *) arg;
    unsigned long Ret=0;
    
    mutex_lock(&laser_mutex);

    switch (cmd) 
    {
        case LASER_IOCTL_GET_RX_PWR:
        {           
            *pPwr = Laser_Dev_Ioctl_Get_Rx_Optical_Pwr();
            break;
        }
        case LASER_IOCTL_GET_TX_PWR:
        {
            *pPwr = Laser_Dev_Ioctl_Get_Tx_Optical_Pwr();
            break;
        }
        case LASER_IOCTL_GET_TEMPTURE:
        {
            *pPwr = Laser_Dev_Ioctl_Get_Temp();
            break;
        }
        case LASER_IOCTL_GET_VOTAGE:
        {
            if (Laser_Dev_Ioctl_Get_Votage)
                *pPwr = Laser_Dev_Ioctl_Get_Votage();
            break;
        }
        case LASER_IOCTL_GET_BIAS_CURRENT:
        {
            if (Laser_Dev_Ioctl_Get_Bias_Current)
                *pPwr = Laser_Dev_Ioctl_Get_Bias_Current();
            break;
        }
        case LASER_IOCTL_SET_OPTICAL_PARAMS:
        {
            break;
        }
        case LASER_IOCTL_GET_OPTICAL_PARAMS:
        {
            break;
        }
        case LASER_IOCTL_INIT_TX_PWR:
        {
            break;
        }
        case LASER_IOCTL_INIT_RX_PWR:
        {
            break;
        }
        case LASER_IOCTL_GET_INIT_PARAMS:
        {
            break;
        }
        case LASER_IOCTL_GET_DRV_INFO:
        {
            break;
        }
    }
    mutex_unlock(&laser_mutex);

    return Ret;
}


static const struct file_operations laser_file_ops = {
    .owner          = THIS_MODULE,
    .open           = Laser_Dev_File_Open,
    .release        = Laser_Dev_File_Release,
    .unlocked_ioctl = Laser_Dev_File_Ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = Laser_Dev_File_Ioctl,
#endif
    
};

/****************************************************************************
* Function    :   Laser_Dev_Init
*             :
* Description :   Performs a registration operation for the char device Laser 
*             :   Dev, initialization for i2c driver and initialization of
*             :   function pointers based on.
*             :
* Params      :   None
*             :
* Returns     :   status indicating whether error occurred getting i2c driver 
*             :   or during registration
****************************************************************************/
static int Laser_Dev_Init(void)
{
    BL_LILAC_SOC_STATUS socStatus = 0;
    int scl     = -1;
    int sda_out = -1;
    int rate    = 100;
    int sda_in  = -1;
    
    if ( register_chrdev(LASER_DEV_MAJOR, LASER_DEV_CLASS, &laser_file_ops) >= 0) 
    {
        /* Init i2c driver */
        i2c_params.rate = rate;
        i2c_params.scl_pin = scl;
        i2c_params.sda_in_pin = sda_in;
        i2c_params.sda_out_pin = sda_out;

        
        socStatus = bl_lilac_i2c_init(&i2c_params);
       
        if (socStatus != BL_LILAC_SOC_OK )
        {
            printk("Error initializing i2c: Error=%d \n", (int)socStatus);
            return ( 1 );
        }
       

        /* Assign the function pointers */
        Laser_Dev_Ioctl_Get_Tx_Optical_Pwr  = Laser_Dev_Ioctl_Get_lilac_Tx_Optical_Pwr;
        Laser_Dev_Ioctl_Get_Rx_Optical_Pwr  = Laser_Dev_Ioctl_Get_lilac_Rx_Optical_Pwr;
        Laser_Dev_Ioctl_Get_Temp            = Laser_Dev_Ioctl_Get_lilac_Laser_Temp; 
        Laser_Dev_Ioctl_Get_Votage          = Laser_Dev_Ioctl_Get_lilac_Laser_Votage;
        Laser_Dev_Ioctl_Get_Bias_Current    = Laser_Dev_Ioctl_Get_lilac_Laser_Bias_Current;

        /* Init mutex for driver usage */
        mutex_init(&laser_mutex);    
    }
    else
    {
        printk(KERN_ERR "%s: can't register major %d\n",LASER_DEV_CLASS, LASER_DEV_MAJOR);
        return ( 2 );
    }
     
    return ( 0 );
}

/****************************************************************************
* Function    :   Laser_Dev_Exit
*             :
* Description :   Performs an un register operation for the char device Laser 
*             :   Dev
*             :
* Params      :   None
*             :
* Returns     :   None
****************************************************************************/
static void Laser_Dev_Exit(void)
{
    unregister_chrdev(LASER_DEV_MAJOR, LASER_DEV_CLASS);
}

module_init(Laser_Dev_Init);
module_exit(Laser_Dev_Exit);

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Tim Sharp tsharp@broadcom.com");
MODULE_DESCRIPTION("Generic Laser Device driver");
