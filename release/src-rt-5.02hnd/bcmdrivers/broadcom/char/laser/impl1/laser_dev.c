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
#include <asm/uaccess.h>
#include <laser.h>
#include <gpon_i2c.h>
#include <boardparms.h>
#include <board.h>

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

#define LASER_DEV_NP_TX_PWR_MSB_OFFSET      102
#define LASER_DEV_NP_TX_PWR_LSB_OFFSET      103
#define LASER_DEV_NP_RX_PWR_MSB_OFFSET      104
#define LASER_DEV_NP_RX_PWR_LSB_OFFSET      105
#define LASER_DEV_NP_TEMP_LSB_OFFSET        97
#define LASER_DEV_NP_TEMP_MSB_OFFSET        96
#define LASER_DEV_NP_VOTAGE_LSB_OFFSET        99
#define LASER_DEV_NP_VOTAGE_MSB_OFFSET        98
#define LASER_DEV_NP_BIAS_CURRENT_LSB_OFFSET        101
#define LASER_DEV_NP_BIAS_CURRENT_MSB_OFFSET        100

#define LASER_DEV_MS_RX_PWR_MSB_OFFSET      0x50
#define LASER_DEV_MS_RX_PWR_LSB_OFFSET      0x51
#define LASER_DEV_MS_TX_PWR_MSB_OFFSET      0x56
#define LASER_DEV_MS_TX_PWR_LSB_OFFSET      0x57

struct mutex laser_mutex;

u16 (*Laser_Dev_Ioctl_Get_Tx_Optical_Pwr)(void);
u16 (*Laser_Dev_Ioctl_Get_Rx_Optical_Pwr)(void);
u16 (*Laser_Dev_Ioctl_Get_Temp)(void);
u16 (*Laser_Dev_Ioctl_Get_Votage)(void);
u16 (*Laser_Dev_Ioctl_Get_Bias_Current)(void);


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

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Set_M02098_Optical_Params
*             :
* Description :   This routine performs the function of loading the BOSA optical
*             :   params provided as input to the Mindspeed M02098 optics chip.
*             :
* Params      :   [IN] pUserData ulong address ofoptical params
*             :
* Returns     :   0 if successful, -1 otherwise
******************************************************************************/
static int Laser_Dev_Ioctl_Set_M02098_Optical_Params(unsigned char *pData,
    short Length)
{
    char ParamData[MAX_I2C_MSG_SIZE];
    int i2cLen = sizeof(ParamData) - 1;
    int Count = 0;
    short CurLen = 0;
    int ret = 0;
    
    while( CurLen < Length )
    {
        Count = ((Length-CurLen) < i2cLen) ? (Length-CurLen) : i2cLen;

        // Set index 0 to the length of the i2c transfer.
        ParamData[0] = (char) CurLen;

        // Copy parameter data to the i2c buffer.
        memcpy(ParamData + 1, pData + CurLen, Count);

        // Write the parameter data to the chip over i2c.
        if( laser_i2c_write(ParamData, (Count + 1)) != (Count + 1) )
        {
            printk("\nLaser_Dev: Error writing to optical device\n");
            ret = -1;
            break;
        }

        CurLen += Count;
    }

    return(ret);
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_M02098_Optical_Params
*             :
* Description :   This routine performs the function of getting the current BOSA 
*             :   optical params from the Mindspeed M02098 optics chip and
*             :   returning to the caller.
*             :
* Params      :   [IN] pBuf char pointer to optical params
*             :   [IN] Size integral value containing number of params
*             :
* Returns     :   0 or -1
******************************************************************************/
static int Laser_Dev_Ioctl_Get_M02098_Optical_Params(unsigned char *pData,
    short Length)
{
    char ParamData[MAX_I2C_MSG_SIZE];
    int i2cLen = sizeof(ParamData);
    int Count = 0;
    short CurLen = 0;
    int ret = 0;
    
    while( CurLen < Length )
    {
        Count = ((Length - CurLen) < i2cLen) ? (Length - CurLen) : i2cLen;

        // Set index 0 to the length of the i2c transfer.
        ParamData[0] = CurLen;

        // Write the parameter data to the chip over i2c.
        if( laser_i2c_read(ParamData, Count) != Count )
        {
            printk("\nLaser_Dev: Error reading from optical device\n");
            ret = -1;
            break;
        }

        // Copy parameter data to the i2c buffer.  The data starts at index 0.
        memcpy(pData + CurLen, ParamData, Count);

        CurLen += Count;
    }

    return(ret);
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Init_Tx_M02098_Power
*             :
* Description :   This routine retrieves the initial Tx power value and stores
*             :   it persistently.
*             :
* Params      :
*             : 
*             :
* Returns     :
******************************************************************************/
static u16 Laser_Dev_Ioctl_Init_Tx_M02098_Power(void)
{
    u16 ret = -1;
    unsigned char TxPwrBuf[3];

    // Get Tx power value.  Set offset to read from.
    TxPwrBuf[0] = LASER_DEV_MS_TX_PWR_MSB_OFFSET;            

    // Read the two Tx power registers.
    if(laser_i2c_read(TxPwrBuf, sizeof(TxPwrBuf) - 1) == sizeof(TxPwrBuf) - 1)
    {
        unsigned short InitTxPower = ((TxPwrBuf[2] >> 4) & 0x0f) |
            ((unsigned short) TxPwrBuf[1] << 4);
        unsigned short RxReading = 0xfff, RxOffset = 0xfff, NotUsed = 0xfff;

        // Store Tx power value in persistent storage.
        kerSysGetOpticalPowerValues(&RxReading,&RxOffset,&NotUsed);

        if((ret = kerSysSetOpticalPowerValues( RxReading, RxOffset,
            InitTxPower )) != 0)
        {
            printk("\nLaser_Dev: Failed to save Tx Power\n" );
        }
    }
    else
        printk("\nLaser_Dev: Failed to read Tx Power from M02098\n" );

    return( ret );
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Init_Rx_M02098_Power
*             :
* Description :   This routine is passed initial Rx power values and stores
*                 them persistently.
*             :
* Params      :   [IN] arg >> 16   : initial Rx reading
*             :   [IN] arg & 0xffff: Rx offset
*             :
* Returns     :
******************************************************************************/
static u16 Laser_Dev_Ioctl_Init_Rx_M02098_Power(unsigned long arg)
{
    u16 ret = -1;
    unsigned short RxReading = arg >> 16;
    unsigned short RxOffset = arg & 0xffff;
    unsigned short NotUsed1 = 0xfff, NotUsed2 = 0xfff, InitTxPower = 0xfff;

    // Get the initial Tx power value from persistent storage.
    kerSysGetOpticalPowerValues(&NotUsed1, &NotUsed2, &InitTxPower);

    // Store the Rx and Tx values in persistent storage.
    if( (ret = kerSysSetOpticalPowerValues(RxReading, RxOffset,
        InitTxPower)) != 0)
    {
        printk("\nLaser_Dev: Failed to save Rx Power values\n" );
    }

    return( ret );
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_M02098_Tx_Optical_Pwr
*             :
* Description :   This routine returns the Mindspeed M02098 Tx optical power.
*             :
* Params      :
*             :
* Returns     :   Tx optical power in micro-watts
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_M02098_Tx_Optical_Pwr(void)
{
    u16 ret = -1;
    unsigned short NotUsed1, NotUsed2;
    unsigned short InitTxPower;
    unsigned char TxPwrBuf[3];

    // Get the initial Tx power value from persistent storage.
    if(kerSysGetOpticalPowerValues(&NotUsed1, &NotUsed2, &InitTxPower) == 0)
    {
        // Get the current Tx power value from chip.  Set offset to read from.
        TxPwrBuf[0] = LASER_DEV_MS_TX_PWR_MSB_OFFSET;            

        // Read the two Tx power registers.
        if(laser_i2c_read(TxPwrBuf, sizeof(TxPwrBuf)-1) == sizeof(TxPwrBuf)-1)
        {
            unsigned short CurrTxPower = ((TxPwrBuf[2] >> 4) & 0x0f) |
                ((unsigned short) TxPwrBuf[1] << 4);

            // Tx Power uW calculation is:
            // Slope = 1000uW / InitTxPower
            // TxPower_uW = Slope * CurrTxPower
            ret = (1000 * CurrTxPower) / InitTxPower;
        }
        else
            printk("\nLaser_Dev: Failed to read Tx Power register\n" );
    }

    return( ret );
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_M02098_Rx_Optical_Pwr
*             :
* Description :   This routine returns the Mindspeed M02098 Rx optical power.
*             :
* Params      :
*             :
* Returns     :   Rx optical power in micro-watts
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_M02098_Rx_Optical_Pwr(void)
{
    u16 ret = -1;
    unsigned short NotUsed;
    unsigned short n20dBRxReading;
    unsigned short RxOffset;
    unsigned char RxPwrBuf[3];

    // Get the initial Rx power value from persistent storage.
    if(kerSysGetOpticalPowerValues(&n20dBRxReading, &RxOffset, &NotUsed) == 0)
    {
        // Get the current Rx power value from chip.  Set offset to read from.
        RxPwrBuf[0] = LASER_DEV_MS_RX_PWR_MSB_OFFSET;            

        // Read the two Rx power registers.
        if(laser_i2c_read(RxPwrBuf, sizeof(RxPwrBuf)-1) == sizeof(RxPwrBuf)-1)
        {
            unsigned short CurrRxReading = ((RxPwrBuf[1] >> 4) & 0x0f) |
                ((unsigned short) RxPwrBuf[0] << 4);

            // Rx Power uW calculation is:
            // Slope = 10uW / (20dBRxReading - RxOffset)
            // RxPower_uW = Slope * (CurrRxReading - RxOffset)
            unsigned long Slope = 10000000 / (n20dBRxReading - RxOffset);
            unsigned long ret32 = (Slope * (CurrRxReading - RxOffset)) / 1000000;
            ret = (u16) ret32;

#if 0 /* TESTING */
            printk("\n\nRxPwrBuf[] = 0x%2.2x 0x%2.2x 0x%2.2x\n",
                RxPwrBuf[0], RxPwrBuf[1], RxPwrBuf[2]);
            printk("Slope = 50000000 / (20dBRxReading %d - RxOffset "
                "%d) = %d\n", n20dBRxReading, RxOffset, Slope);
            printk("Reading = (Slope * (CurrRxReading %d - RxOffset)) / 1000000"
                "%d) = %d\n\n", CurrRxReading, RxOffset, ret);
#endif
        }
        else
            printk("\nLaser_Dev: Failed to read Rx Power register\n" );
    }
    else
        printk("\nLaser_Dev: Failed to read initial Rx Power values from "
            "persistent storage\n" );

    return( ret );
}


/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_M02098_Init_Params
*             :
* Description :   This routine returns the Mindspeed M02098 initial power
*             :   values that were saved persistently.
*             :
* Params      :
*             :
* Returns     :   Rx optical power in micro-watts
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_M02098_Init_Params(unsigned long pUserData)
{
    u16 ret = 0;
    LASER_INIT_PARAMS lip = {0xfff, 0xfff, 0xfff};

    // Get the initial Rx power value from persistent storage.
    if( kerSysGetOpticalPowerValues( &lip.initRxReading, &lip.initRxOffset,
        &lip.initTxReading ) == 0 )
    {
        copy_to_user((char *) pUserData, (char *) &lip, sizeof(lip));
    }
    else
        ret = -1;

    return( ret );
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_M02098_Laser_Temp
*             :
* Description :   This stub is intended to eventually be used to retrieve the
*             :   laser temp parameter provided by the mindspeed M02098 optics
*             :   chip.
*             :
* Params      :
*             :   None
*             :
* Returns     :   0
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_M02098_Laser_Temp(void)
{
    return 0;
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_Nphtncs_Tx_Optical_Pwr
*             :
* Description :   This routine performs the function of getting the tx optical 
*             :   power when the optics device is NEOPHOTONICS.  This device
*             :   uses internal calibration and stores the value in its final
*             :   form, so no calc or manipulation is required.
*             :
* Params      :   None
*             :
* Returns     :   Unsigned short representation of the Tx power value [micro-watts]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_Nphtncs_Tx_Optical_Pwr(void)
{
    u16 TxPwr = gponPhy_read_word(1, LASER_DEV_NP_TX_PWR_MSB_OFFSET);
        
    return ((TxPwr & 0xFF)<<8) | ((TxPwr & 0xFF00)>>8);  
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_Nphtncs_Rx_Optical_Pwr
*             :
* Description :   This routine performs the function of getting the rx optical 
*             :   power when the optics device is NEOPHOTONICS.  This device
*             :   uses internal calibration and stores the value in its final
*             :   form, so no calc or manipulation is required.
*             :
* Params      :   None
*             :
* Returns     :   Unsigned short representation of the Rx power value [micro-watts]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_Nphtncs_Rx_Optical_Pwr(void)
{    
    u16 RxPwr = gponPhy_read_word(1, LASER_DEV_NP_RX_PWR_MSB_OFFSET);
        
    return ((RxPwr & 0xFF)<<8) | ((RxPwr & 0xFF00)>>8);       
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_Nphtncs_Laser_Temp
*             :
* Description :   This routine performs the function of returning the laser 
*             :   temperature when the optics device is NEOPHOTONICS.  This 
*             :   device uses internal calibration and stores the value in its 
*             :   final form, so no calc or manipulation is required.
*             :
* Params      :   None
*             :
* Returns     :   Unsigned short representation of the Temperature [c]
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_Nphtncs_Laser_Temp(void)
{

    u16 Tmp = gponPhy_read_word(1, LASER_DEV_NP_TEMP_MSB_OFFSET);
        
    return ((Tmp & 0xFF)<<8) | ((Tmp & 0xFF00)>>8);          
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_Nphtncs_Laser_Votage
*             :
* Description :   This routine performs the function of returning the laser 
*             :   votage when the optics device is NEOPHOTONICS.  This 
*             :   device uses internal calibration and stores the value in its 
*             :   final form, so no calc or manipulation is required.
*             :
* Params      :   None
*             :
* Returns     :   unsigned short representation of the Voltage [mV].
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_Nphtncs_Laser_Votage(void)
{

    u16 Votage = gponPhy_read_word(1, LASER_DEV_NP_VOTAGE_MSB_OFFSET);
        
    return ((Votage & 0xFF)<<8) | ((Votage & 0xFF00)>>8);          
}

/******************************************************************************
* Function    :   Laser_Dev_Ioctl_Get_Nphtncs_Laser_Bias_Current
*             :
* Description :   This routine performs the function of returning the laser 
*             :   bias current when the optics device is NEOPHOTONICS.  This 
*             :   device uses internal calibration and stores the value in its 
*             :   final form, so no calc or manipulation is required.
*             :
* Params      :   None
*             :
* Returns     :   unsigned short representation of the Bias [uA].
******************************************************************************/
static u16 Laser_Dev_Ioctl_Get_Nphtncs_Laser_Bias_Current(void)
{

    u16 Bias = gponPhy_read_word(1, LASER_DEV_NP_BIAS_CURRENT_MSB_OFFSET);
        
    return ((Bias & 0xFF)<<8) | ((Bias & 0xFF00)>>8);          
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
static long Laser_Dev_File_Ioctl(struct file *file, unsigned int cmd,
    unsigned long arg)
{

    unsigned long * pPwr = (unsigned long *) arg;
    long Ret=0;
    
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
            LASER_OPTICAL_PARAMS lop;

            if( copy_from_user(&lop, (void *) arg, sizeof(lop)) == 0 )
            {
                unsigned char ucParams[LASER_TOTAL_OPTICAL_PARAMS_LEN];

                if( lop.opLength > LASER_TOTAL_OPTICAL_PARAMS_LEN )
                    lop.opLength = LASER_TOTAL_OPTICAL_PARAMS_LEN;

                if(copy_from_user(ucParams, lop.opRegisters, lop.opLength)==0) 
                {
                    Ret = Laser_Dev_Ioctl_Set_M02098_Optical_Params(ucParams,
                        lop.opLength);
                }
            }
            else
                Ret = -1;
            break;
        }
        case LASER_IOCTL_GET_OPTICAL_PARAMS:
        {
            LASER_OPTICAL_PARAMS lop;

            if( copy_from_user(&lop, (void *) arg, sizeof(lop)) == 0 )
            {
                unsigned char ucParams[LASER_TOTAL_OPTICAL_PARAMS_LEN];

                if( lop.opLength > LASER_TOTAL_OPTICAL_PARAMS_LEN )
                    lop.opLength = LASER_TOTAL_OPTICAL_PARAMS_LEN;

                if( (Ret = Laser_Dev_Ioctl_Get_M02098_Optical_Params(ucParams,
                    lop.opLength)) == 0 )
                {
                    copy_to_user(lop.opRegisters, ucParams, lop.opLength);
                }
            }
            else
                Ret = -1;
            break;
        }
        case LASER_IOCTL_INIT_TX_PWR:
        {
            Ret = Laser_Dev_Ioctl_Init_Tx_M02098_Power();
            break;
        }
        case LASER_IOCTL_INIT_RX_PWR:
        {
            Ret = Laser_Dev_Ioctl_Init_Rx_M02098_Power(arg);
            break;
        }
        case LASER_IOCTL_GET_INIT_PARAMS:
        {
            Ret = Laser_Dev_Ioctl_Get_M02098_Init_Params(arg);
            break;
        }
        case LASER_IOCTL_GET_DRV_INFO:
        {
            break;
        }
        default:
            printk("\nLaser_Dev: operation not supported\n" );
            Ret = -1;
           break;
    }
    mutex_unlock(&laser_mutex);

    return Ret;
}


static const struct file_operations laser_file_ops = {
    .owner =        THIS_MODULE,
    .open =         Laser_Dev_File_Open,
    .release =      Laser_Dev_File_Release,
    .unlocked_ioctl =   Laser_Dev_File_Ioctl,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = Laser_Dev_File_Ioctl,
#endif
};

/****************************************************************************
* Function    :   Laser_Dev_Init
*             :
* Description :   Performs a registration operation for the char device Laser 
*             :   Dev, plus initializes function pointers based on the
*             :
* Params      :   None
*             :
* Returns     :   status indicating whether error occurred getting gpon optics 
*             :   or during registration
****************************************************************************/
static int Laser_Dev_Init(void)
{
    int ret=0;

    u16 BosaFlag = 0;    

    if ( 0 == (ret = BpGetGponOpticsType(&BosaFlag)))
    {
    	/* exit the module if pmd device exist */
    	if (BP_GPON_OPTICS_TYPE_PMD == BosaFlag)
		{
    		printk(KERN_ERR "PMD exist on board \n");
			return 0;
	    }

    	ret = register_chrdev(LASER_DEV_MAJOR, LASER_DEV_CLASS, &laser_file_ops);
        if (ret >= 0) 
        {
            // assign the function pointers here...
            if (1 == BosaFlag)
            {
                unsigned char ucOpticalParams[BP_OPTICAL_PARAMS_LEN];

                Laser_Dev_Ioctl_Get_Tx_Optical_Pwr  = Laser_Dev_Ioctl_Get_M02098_Tx_Optical_Pwr;
                Laser_Dev_Ioctl_Get_Rx_Optical_Pwr  = Laser_Dev_Ioctl_Get_M02098_Rx_Optical_Pwr;
                Laser_Dev_Ioctl_Get_Temp            = Laser_Dev_Ioctl_Get_M02098_Laser_Temp;                                
                Laser_Dev_Ioctl_Get_Votage          = NULL;                                
                Laser_Dev_Ioctl_Get_Bias_Current    = NULL;

                if( BpGetDefaultOpticalParams(ucOpticalParams) == BP_SUCCESS )
                {
                    if( (ret = Laser_Dev_Ioctl_Set_M02098_Optical_Params(
                        ucOpticalParams, sizeof(ucOpticalParams))) != 0 )
                    {
                        printk(KERN_ERR "%s: cannot write optical parameters "\
                            "to laser device.\n", LASER_DEV_CLASS);
                    }
                }
                else
                {
                    printk(KERN_ERR "%s: cannot read default optical "
                        "parameters.\n", LASER_DEV_CLASS);
                }
            }
            else
            {                
                Laser_Dev_Ioctl_Get_Tx_Optical_Pwr  = Laser_Dev_Ioctl_Get_Nphtncs_Tx_Optical_Pwr;
                Laser_Dev_Ioctl_Get_Rx_Optical_Pwr  = Laser_Dev_Ioctl_Get_Nphtncs_Rx_Optical_Pwr;
                Laser_Dev_Ioctl_Get_Temp            = Laser_Dev_Ioctl_Get_Nphtncs_Laser_Temp;
                Laser_Dev_Ioctl_Get_Votage          = Laser_Dev_Ioctl_Get_Nphtncs_Laser_Votage;
                Laser_Dev_Ioctl_Get_Bias_Current    = Laser_Dev_Ioctl_Get_Nphtncs_Laser_Bias_Current;
            }
            mutex_init(&laser_mutex);
        }
        else
        {
            printk(KERN_ERR "%s: can't register major %d\n",LASER_DEV_CLASS, LASER_DEV_MAJOR);
        }
    }
    else
    {        
        printk(KERN_ERR "%s: board profile not configured and status of BOSA optics cannot be determined.\n", LASER_DEV_CLASS);
    }

    return ret;
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

MODULE_AUTHOR("Tim Sharp tsharp@broadcom.com");
MODULE_DESCRIPTION("Generic Laser Device driver");
MODULE_LICENSE("Proprietary");
