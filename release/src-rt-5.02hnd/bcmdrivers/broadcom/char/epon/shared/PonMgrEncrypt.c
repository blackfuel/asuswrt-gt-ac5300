/*
*  Copyright 2011, Broadcom Corporation
*
* <:copyright-BRCM:2011:proprietary:standard
* 
*    Copyright (c) 2011 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

/*                   Copyright(c) 2002-2010 Broadcom, Inc.                    */

////////////////////////////////////////////////////////////////////////////////
/// \file PonMgrEncrypt.c
/// \brief Version 2 encryption implementation
///
/// Version 2 encryption is used to configure encrpytion hardware for all
/// products based off of the 4701 chip (55030).
///
////////////////////////////////////////////////////////////////////////////////
// generic includes
#include "PonMgrEncrypt.h"
#include "bcm_epon_cfg.h"
#ifdef CONFIG_EPON_10G_SUPPORT 
#include "Xif.h"
#endif
#include "Lif.h"

EncryptPonCfg encryptPonCfg;

// Tag Control Information bits
// bit 7: V=0
// bit 6: End Station = 0
#define EncryptTciExplicit       (1U << 5)
// bit 4: Single Copy Broadcast -  not used
#define EncryptTciEncrypt       (1U << 3)
#define EncryptTciChange        (1U << 2)
#define EncryptTciAuthEnc        (EncryptTciEncrypt | EncryptTciChange)

/*
static
void EncryptPrintCfg(void)
	{
	printk("\nmode: %d, opt: 0x%x\n", 
		encryptPonCfg.mode, encryptPonCfg.opts);
	printk("dnLinks: 0x%x, upLinks: 0x%x\n", 
		encryptPonCfg.dnEncryptLinks, encryptPonCfg.upEncryptLinks);
	}
*/

///////////////////////////////////////////////////////////////////////////////
/// \brief  Clear an encryption key
///
/// \param  link    Link to clear key
/// \param  dir     Direction to clear
/// \param  num     Key index to clear
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
static
void EncryptKeyClear (LinkIndex link, Direction dir, U8 num)
    {
    LifKeySet (EncryptModeDisable, dir, link, num, NULL, NULL, 0, 0);
#ifdef CONFIG_EPON_10G_SUPPORT 
    XifKeySet (EncryptModeDisable, dir, link, num, NULL, NULL, 0, 0);
#endif
    } // ZeroUpSecRam


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get link encrypted status
///
/// \return
/// TRUE is link is encrypted, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
//extern
BOOL EncryptGetLinkEncrypted(LinkIndex link, Direction dir)
    {
#ifdef CONFIG_EPON_10G_SUPPORT 
    if (((PonCfgDbGetDnRate() == LaserRate10G) && (dir == Dnstream)) || 
		((PonCfgDbGetUpRate() == LaserRate10G) && (dir == Upstream)))
        {
        return XifEncryptEnGet(link, dir);
        }
    else
#endif
        { 
        return LifEncryptEnGet(link, dir);
    	}
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Is a link encrypted
///
/// \return
/// TRUE is link is encrypted, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
static 
BOOL EncryptIsLinkEncrypted(LinkIndex link, Direction dir)
    {
    return (dir == Upstream) ? 
		TestBitsSet(encryptPonCfg.upEncryptLinks, 1U << link) : 
		TestBitsSet(encryptPonCfg.dnEncryptLinks, 1U << link);
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Gets the current encryption mode
///
/// \param link     Link to get mode for
///
/// \return
/// encryption mode
////////////////////////////////////////////////////////////////////////////////
EncryptMode EncryptModeGet(LinkIndex link)
    {
    return TestBitsSet(encryptPonCfg.dnEncryptLinks, 1U << link) ?
        encryptPonCfg.mode : EncryptModeDisable;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Gets the current encryption options
///
/// \param link     Link to get options for
///
/// \return
/// encryption options
////////////////////////////////////////////////////////////////////////////////
EncryptOptions EncryptOptsGet(LinkIndex link)
    {
    return TestBitsSet(encryptPonCfg.dnEncryptLinks, 1U << link) ?
        encryptPonCfg.opts : EncryptOptNone;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Apply new encryption configuration
///
/// \param mode New encryption mode
/// \param opts New encryption options
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void EncryptCfgApply(EncryptMode mode, EncryptOptions opts)
    {
    // if all links unencrypted, change mode to disabled
    if ((mode == EncryptModeDisable) && 
		(encryptPonCfg.dnEncryptLinks == 0))
    	{
    	encryptPonCfg.mode = mode;
    	encryptPonCfg.opts = EncryptOptNone;
		
#ifdef CONFIG_EPON_10G_SUPPORT 
		XifEncryptUpDisable();
#endif
		LifEncryptUpDisable();
		return;
    	}

	if((mode != EncryptModeDisable) && 
		(encryptPonCfg.dnEncryptLinks != 0))
		{
    	encryptPonCfg.mode = mode;
    	encryptPonCfg.opts = opts;
		}
    }


///////////////////////////////////////////////////////////////////////////////
/// \brief  Disable downstream encryption on a link
///
/// \param link     Link to disable encryption
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static
void EncryptLinkDisable(LinkIndex link)
    {
    U8  i;
	
    for (i = 0; i < 2; i++)
        {
        //EncryptKeyClear(link, Dnstream, i);
        if (link < TkOnuNumTxLlids)
            {
            EncryptKeyClear(link, Upstream, i);
            }
        }

	//update configuration db
    if (link < TkOnuNumTxLlids)
        {
        encryptPonCfg.dnEncryptLinks &= ~(1U << link);
        encryptPonCfg.upEncryptLinks &= ~(1U << link);
        }
    }


///////////////////////////////////////////////////////////////////////////////
/// \brief  Set encryption configuration on a link
///
/// \param  link    Link to configure encryption
/// \param  mode    Encryption mode to set
/// \param  opts    Encryption options to set
///
/// \return TRUE if configuration was successful
////////////////////////////////////////////////////////////////////////////////
BOOL EncryptLinkSet(LinkIndex link, EncryptMode mode, EncryptOptions opts)
    {
    //easy to apply encrypt disable mode.
    if (mode == EncryptModeDisable)
        {
        //if link not downsteam encrypted, just ignore
        if (!EncryptIsLinkEncrypted (link, Dnstream))
        	{
        	return TRUE;
        	}
		//downstream-only or bi-directional.
        EncryptLinkDisable(link);
		EncryptCfgApply(mode, opts);
		return TRUE;
        }

     if (((mode != encryptPonCfg.mode) || (opts != encryptPonCfg.opts)) &&
            (encryptPonCfg.dnEncryptLinks > 0))
        {//can't change encrypted mode or opts directly
        return FALSE;
        }

    if ((opts & EncryptOptBiDir) > 0)
    	{
    	encryptPonCfg.upEncryptLinks |= (1UL << link);     
    	}
  	encryptPonCfg.dnEncryptLinks |= (1UL << link); 
    EncryptCfgApply(mode, opts);

    return TRUE;
    }


////////////////////////////////////////////////////////////////////////////////
/// \brief  Gets the TCI for current encryption settings
///
/// \return
/// TCI
////////////////////////////////////////////////////////////////////////////////
static
U8 EncryptTciGet(U8 keyIdx)
    {
    U8 tci = 0;
    if (encryptPonCfg.mode == EncryptMode8021AE)
    	{
    	tci |= (!(encryptPonCfg.opts&EncryptOptImplicitSci)) ? EncryptTciExplicit : 0;
    	tci |= (!(encryptPonCfg.opts&EncryptOptAuthOnly)) ? EncryptTciAuthEnc : 0;
        tci |= keyIdx;
    	}

    return tci;
    }   // Encrypt8021aeSciSet


////////////////////////////////////////////////////////////////////////////////
/// EncryptKeyInUse - Get the active encryption key
///
/// This function gets the active key for a given link.  It is used to detect a
/// key switch over.
///
/// Parameters:
/// \param link Link to check
///
/// \return
/// Active encryption key index
////////////////////////////////////////////////////////////////////////////////
//extern
U8 EncryptKeyInUse (LinkIndex link)
    {   
#ifdef CONFIG_EPON_10G_SUPPORT 
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        return XifKeyInUse (link, Dnstream);
        }
    else
#endif 
        {
        return LifKeyInUse(link, Dnstream);
        }
    } // EncryptKeyInUse


///////////////////////////////////////////////////////////////////////////////
/// \brief  Sets the downstream encryption key for a link
///
/// \param link     Link to set key for
/// \param keyIdx   Key index to use
/// \param key      The encryption key to use
/// \param sci      The SCI of the link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static void EncryptDnKeySet(LinkIndex link,
                     U8 keyIdx,
                     const U32 * key,
                     const U32 * sci)
    {   
#ifdef CONFIG_EPON_10G_SUPPORT 
    if (PonCfgDbGetDnRate() == LaserRate10G)
        {
        XifKeySet (encryptPonCfg.mode, Dnstream, link, keyIdx, key, sci, 0,
                   AesInitialPacketNumber);
        }
    else
#endif
        { 
        LifKeySet (encryptPonCfg.mode, Dnstream, link, keyIdx, key, sci, 0,
		   AesInitialPacketNumber);
    	}
    }


///////////////////////////////////////////////////////////////////////////////
/// \brief  Sets the upstream encryption key for a link
///
/// \param link     Link to set key for
/// \param keyIdx   Key index to use
/// \param key      The encryption key to use
/// \param sci      The SCI of the link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
static void EncryptUpKeySet(LinkIndex link,
                     U8 keyIdx,
                     const U32 * key,
                     const U32 * sci)
    {    
#ifdef CONFIG_EPON_10G_SUPPORT 
    if (PonCfgDbGetUpRate() == LaserRate10G)
        {
        XifKeySet (encryptPonCfg.mode, Upstream, link, keyIdx, key, sci,
                   EncryptTciGet(keyIdx), AesInitialPacketNumber);
        }
    else
#endif
        {
        LifKeySet (encryptPonCfg.mode, Upstream, link, keyIdx, key, sci, 
                   EncryptTciGet(keyIdx), AesInitialPacketNumber);
        }
    } // EncryptKeySetUp



///////////////////////////////////////////////////////////////////////////////
/// \brief  check the key and set the key
///
/// \param dir     direction
/// \param link     Link to set key for
/// \param keyIdx   Key index to use
/// \param keyLen      Key length
/// \param key      The encryption key to use
/// \param sci      The SCI of the link
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
BOOL EncryptKeySet(LinkIndex link,
			Direction dir, 
                     U8 keyIdx,
                     U8 keyLen,
                     const U32* key,
                     const U32* sci)
    {
    if((EncryptModeGet(link) == EncryptModeDisable) ||
		(!EncryptIsLinkEncrypted(link, dir)))
    	{
        return FALSE;
    	}

    if (((EncryptModeGet(link) == EncryptModeTripleChurn) && (keyLen >= 3)) ||
		((EncryptModeGet(link) == EncryptModeAes) && (keyLen >= 16)) ||
		((EncryptModeGet(link) == EncryptModeZoh) && (keyLen >= 16)) ||
		((EncryptModeGet(link) == EncryptMode8021AE) && (keyLen >= 16)))
    	{
		if (dir == Upstream)
			{
			EncryptUpKeySet(link, keyIdx, key, sci);
			}
		else
			{
			EncryptDnKeySet(link, keyIdx, key, sci);
			}
	
		return TRUE;
    	}

    return FALSE;
    }


///////////////////////////////////////////////////////////////////////////////
/// \brief  encryption configuration initialization
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void EncryptCfgInit(void)
    {
    memset(&encryptPonCfg, 0, sizeof(EncryptPonCfg));
    }


// end PonMgrEncrypt.c

