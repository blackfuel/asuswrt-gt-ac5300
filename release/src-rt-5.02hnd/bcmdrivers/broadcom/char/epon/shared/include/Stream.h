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

/*                   Copyright(c) 2010 Broadcom, Inc.                         */

#if !defined(Stream_h)
#define Stream_h
////////////////////////////////////////////////////////////////////////////////
/// \file Stream.h
/// \brief API for reading/writing packed bytes streams
///
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"

typedef struct
    {
    U8* start;
    U8* cur;
    }  PACK Stream;

typedef struct
    {
    Stream      src;
    Stream      reply;
    U8 *        endReply;
    U16         curOffset;
    U16         seqNum;
    BOOL        isLastItem;
    Port        dest;
    LinkIndex   link;
    } PACK OamParserState;

typedef enum
    {
    LacpSlowProtocolSubtype        = 0x01,  // Link Agregation Control Protocol
    LampSlowProtocolSubtype        = 0x02,  // Link Agregation Marker Protocol
    OamSlowProtocolSubtype         = 0x03,
    Oam8021asProtocolSubtype       = 0x0A,
    OrgSpecSlowProtocolSubtype     = 0x0A   // Organization Specific Subtype
    } SlowProtocolSubtype;

#define MAX_MPCP_SIZE  100
#define MAX_ALM_SIZE   64

extern U8 rxPktBuffer[MAX_MPCP_SIZE];
extern U8 txPktBuffer[MAX_MPCP_SIZE];
extern U8 txAlmBuffer[MAX_ALM_SIZE];
#define TxAlarm ((EthernetFrame *)txAlmBuffer)
#define TxFrame ((EthernetFrame *)txPktBuffer)
#define RxFrame ((EthernetFrame *)rxPktBuffer)

extern OamParserState oamParser;;

////////////////////////////////////////////////////////////////////////////////
/// \brief  Write an IEEE OUI to a packed stream
///
/// \param strm     Stream to write to
/// \param oui      Pointer to OUI
///
/// \return None
////////////////////////////////////////////////////////////////////////////////
#define StreamWriteOui(strm, oui)   \
    StreamWriteBytes(strm, (oui)->byte, sizeof(IeeeOui))


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a byte from a packed stream
///
/// \param strm     Stream to read from
///
/// \return
/// Byte read
////////////////////////////////////////////////////////////////////////////////
U8 StreamReadU8(Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a word from a packed stream
///
/// \param strm     Stream to read from
///
/// \return
/// Word read
////////////////////////////////////////////////////////////////////////////////
U16 StreamReadU16(Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a dword from a packed stream
///
/// \param strm     Stream to read from
///
/// \return
/// Dword read
////////////////////////////////////////////////////////////////////////////////
U32 StreamReadU32(Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a qword from a packed stream
///
/// \param strm     Stream to read from
///
/// \return
/// Qword read
////////////////////////////////////////////////////////////////////////////////
U64 StreamReadU64(Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a byte from a packed stream without advancing
///
/// \param strm     Stream to read from
///
/// \return
/// Byte read
////////////////////////////////////////////////////////////////////////////////
U8 StreamPeekU8(const Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a word from a packed stream without advancing
///
/// \param strm     Stream to read from
///
/// \return
/// Word read
////////////////////////////////////////////////////////////////////////////////
U16 StreamPeekU16(const Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a dword from a packed stream without advancing
///
/// \param strm     Stream to read from
///
/// \return
/// Dword read
////////////////////////////////////////////////////////////////////////////////
U32 StreamPeekU32(const Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Read a qword from a packed stream without advancing
///
/// \param strm     Stream to read from
///
/// \return
/// Qword read
////////////////////////////////////////////////////////////////////////////////
U64 StreamPeekU64(const Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Write a sequence of bytes to a packed stream
///
/// \param strm     Stream to write to
/// \param byte     Pointer to bytes to write
/// \param numBytes Number of bytes to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StreamWriteBytes(Stream* strm, const U8 * bytes, U16 numBytes);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Copy a sequence of packed bytes to a packed stream
///
/// \param strm     Stream to copy to
/// \param src      Pointer to source data to copy
/// \param numBytes Number of bytes to copy
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
extern
void StreamCopy(Stream* strm, const U8 * src, U16 numBytes);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Write a byte to a packed stream
///
/// \param strm     Stream to write to
/// \param byte     Byte to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamWriteU8(Stream* strm, U8 byte);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Write a word to a packed stream
///
/// \param strm     Stream to write to
/// \param word     Word to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamWriteU16(Stream* strm, U16 word);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Write a dword to a packed stream
///
/// \param strm     Stream to write to
/// \param dword    Dword to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamWriteU32(Stream* strm, U32 dword);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Write a qword to a packed stream
///
/// \param strm     Stream to write to
/// \param qword    Qword to write
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamWriteU64(Stream* strm, U64 qword);




////////////////////////////////////////////////////////////////////////////////
/// \brief  Skip bytes in a stream
///
/// \param strm         Stream to skip
/// \param byteCount    Number of bytes to skip
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamSkip(Stream * strm, U32 byteCount);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Rewind a stream
///
/// \param strm         Stream to rewind
/// \param byteCount    Number of bytes to Rewind
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamRewind(Stream * strm, U32 byteCount);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Return length of stream in bytes (from start to current location)
///
/// \param strm         Stream to get byte length for
///
/// \return
/// Count in bytes of the stream
////////////////////////////////////////////////////////////////////////////////
U32 StreamLengthInBytes(const Stream* strm);


////////////////////////////////////////////////////////////////////////////////
/// \brief  Initialize a stream
///
/// \param strm     Stream to init
/// \param start    Pointer to beginning of stream
///
/// \return
/// None
////////////////////////////////////////////////////////////////////////////////
void StreamInit(Stream* strm, U8* start);



U16 PbiRand16(void);


#endif // Stream.h
