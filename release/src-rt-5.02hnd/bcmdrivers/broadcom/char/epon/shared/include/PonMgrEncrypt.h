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

#if !defined(PonMgrEncrypt_h)
#define PonMgrEncrypt_h
////////////////////////////////////////////////////////////////////////////////
/// \file PonMgrEncrypt.h
/// \brief Public encryption API
///
/// This module manages the encryption hardware.  It does not itself contain
/// any crypto algorithms; rather it manages key exchange with the OLT and
/// provisions the security hardware in the chip.
///
////////////////////////////////////////////////////////////////////////////////
#include "Teknovus.h"
#include "Security.h"
#include "PonManager.h"
#include "EponMac.h"
#include "Lif.h"
#include "Stream.h"


#if defined(__cplusplus)
extern "C" {
#endif

// define AES key and SCI for 802.1ae security algorithm
typedef struct
    {
    MacAddr mac;
    U16     port;
    } PACK  AesSci;

typedef union
    {
    AesSci  sci;
    U8      byte[8];
    U32     dword[2];
    } PACK AesSciInfo;

typedef struct
    {
    MultiByte128    key;    // AES 128 bytes key
    AesSciInfo      sci;    // the SCI field for 802.1ae AES algorithm
    } PACK AesKeyInfo;


typedef struct
	{
	EncryptMode     mode;
	EncryptOptions	opts;
	U32		dnEncryptLinks;
	U32		upEncryptLinks;
	} PACK EncryptPonCfg;


#define AesInitialPacketNumber  1
#define AesPktNumThreshold      0xC0000000UL


////////////////////////////////////////////////////////////////////////////////
/// \brief  Get link encrypted status
///
/// \return
/// TRUE is link is encrypted, FALSE otherwise
////////////////////////////////////////////////////////////////////////////////
extern
BOOL EncryptGetLinkEncrypted(LinkIndex link, Direction dir);


///////////////////////////////////////////////////////////////////////////////
/// \brief  Set encryption configuration on a link
///
/// \param  link    Link to configure encryption
/// \param  mode    Encryption mode to set
/// \param  opts    Encryption options to set
///
/// \return TRUE if configuration was successful
////////////////////////////////////////////////////////////////////////////////
extern
BOOL EncryptLinkSet(LinkIndex link, EncryptMode mode, EncryptOptions opts);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Gets the current encryption mode
///
/// \param link     Link to get mode for
///
/// \return
/// encryption mode
////////////////////////////////////////////////////////////////////////////////
extern
EncryptMode EncryptModeGet(LinkIndex link);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Gets the current encryption options
///
/// \param link     Link to get options for
///
/// \return
/// encryption options
////////////////////////////////////////////////////////////////////////////////
extern
EncryptOptions EncryptOptsGet(LinkIndex link);

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
extern 
BOOL EncryptKeySet(LinkIndex link,
			Direction dir, 
                     U8 keyIdx,
                     U8 keyLen,
                     const U32 * key,
                     const U32 * sci);


////////////////////////////////////////////////////////////////////////////////
/// EncryptKeyInUse - Get the active encryption key
///
/// This function gets the active key for a given link.  It is used to detect a
/// key switch over.
///
/// \param link Link to check
///
/// \return
/// Active encryption key index
////////////////////////////////////////////////////////////////////////////////
extern
U8 EncryptKeyInUse (LinkIndex link);


///////////////////////////////////////////////////////////////////////////////
/// \brief  encryption configuration initialization
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern 
void EncryptCfgInit(void);

#if defined(__cplusplus)
}
#endif


#endif // Encryption.h
