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

/*                Copyright(c) 2002-2010 Broadcom Corporation                 */

#if !defined(Mpcp_h)
#define Mpcp_h
////////////////////////////////////////////////////////////////////////////////
/// \file Mpcp.h
/// \brief Definitions for the IEEE 802.3ah Multi-Point Control Protocol
///
/// This header defines structures and constants useful for handling
/// MPCP messages
///
/// These definitions match draft 1.414 of IEEE 802.3ah
///
////////////////////////////////////////////////////////////////////////////////

#include "Teknovus.h"

#if defined(__cplusplus)
extern "C" {
#endif


/// Global broadcast physical LLID value
#define MpcpBroadcastLlid      0x7fff
#define MpcpBroadcastLlid1G    0x7fff
#define MpcpBroadcastLlid10G   0x7ffe
#define MpcpLlidSleBit         0x8000

// Hardware index values for broadcast.  Note that these can only be used in
// specific instances.
#if defined(PioneerFpga) && PioneerFpga
#define MpcpBroadcastHwIdx1G   (MpcpBroadcastLlid1G & 0x1f)
#define MpcpBroadcastHwIdx10G  (MpcpBroadcastLlid10G & 0x1f)
#else
#define MpcpBroadcastHwIdx1G   (MpcpBroadcastLlid1G & 0xff)
#define MpcpBroadcastHwIdx10G  (MpcpBroadcastLlid10G & 0xff)
#endif


/// MPCP message type opcodes
typedef enum
    {
    MpcpGate            = 0x0002,
    MpcpReport          = 0x0003,
    MpcpRegisterReq     = 0x0004,
    MpcpRegister        = 0x0005,
    MpcpRegisterAck     = 0x0006,

    MpcpNumOpcodes      = 5,
    MpcpOpcodeForce16   = 0x7fff
    } MpcpOpcode_e;


typedef U16 MpcpOpcode;

#define MpcpEthertype 0x8808


/// All MPCP messages have a 32-bit timestamp field, which
/// counts time in units of 16 bit times.
typedef U32 MpcpTimestamp;

/// MPCP commonly measures time intervals with a 16-bit width,
/// again measured in units of 16 bit times
typedef U16 MpcpInterval16;


////////////////////////////////////////////////////////////////////////////////
// Gate
////////////////////////////////////////////////////////////////////////////////

/// Gate message layout
typedef struct
    {
    MpcpOpcode      opcode;
    MpcpTimestamp   timestamp;
    U8              flags;
    // variable payload follows this header, as specified
    // by the flags field
    } PACK MpcpGateMsg;

#define MpcpGateFlagNumGrantsMsk    0x07
#define MpcpGateFlagNumGrantsSft    0
#define MpcpGateFlagDiscovery       0x08
#define MpcpGateFlagForceGrant1     0x10
#define MpcpGateFlagForceGrant2     0x20
#define MpcpGateFlagForceGrant3     0x40
#define MpcpGateFlagForceGrant4     0x80

// payload consists of 0-4 grants, as specified in flags field,
typedef struct
    {
    MpcpTimestamp   startTime;
    MpcpInterval16  length;
    } PACK MpcpGateGrantInfo;

/// MPCP dicovery info (802.3 av)
typedef enum
    {
    MpcpGateInfoNotSupported  = 0x0000,
    MpcpGateInfo1GCapable     = 0x0001,
    MpcpGateInfo10GCapable    = 0x0002,
    MpcpGateInfo1GWindow      = 0x0010,
    MpcpGateInfo1GAttempt     = 0x0010,
    MpcpGateInfo10GWindow     = 0x0020,
    MpcpGateInfo10GAttempt    = 0x0020,
    MpcpGateInfoForce16       = 0x7fff
    } MpcpDiscoveryInfo_e;


typedef U16 MpcpDiscoveryInfo;

#define MpcpGateInfoCapabilityMsk (MpcpGateInfo1GCapable|MpcpGateInfo10GCapable)
#define MpcpGateInfoCapabilitySft 0
#define MpcpGateInfoWindowMsk     (MpcpGateInfo1GWindow|MpcpGateInfo10GWindow)
#define MpcpGateInfoWindowSft     4


// discovery gates have grants followed by OLT receiver sync time
typedef struct
    {
    MpcpInterval16     syncTime;
    } PACK MpcpGateDiscoveryInfo1G;


typedef struct
    {
    MpcpInterval16     nonDiscoverySyncTime;
    MpcpDiscoveryInfo  discoveryInfo;
    } PACK MpcpGateDiscoveryInfo10G;


////////////////////////////////////////////////////////////////////////////////
/// MpcpGateGrant:  locates grant #n in a gate msg
///
/// The caller is responsible for ensuring that grant is actually in the
/// message (that is, grant < (msg->flags & NumGrantsMsk)
///
/// \param m        Gate message to parse
/// \param grant    Which grant to return
///
/// \return
/// pointer to gate grant
////////////////////////////////////////////////////////////////////////////////
extern
MpcpGateGrantInfo* MpcpGateGrant (const MpcpGateMsg* msg, U8 grant);


////////////////////////////////////////////////////////////////////////////////
/// MpcpGateDiscovery1G:  locates discovery information in a 1G gate message
///
/// The caller should ensure that the message is actually a discovery gate
/// (msg->flags & Discovery)
/// \param m    Message containing discovery info
///
/// \return
/// Pointer to discovery info
////////////////////////////////////////////////////////////////////////////////
extern
MpcpGateDiscoveryInfo1G* MpcpGateDiscovery1G (const MpcpGateMsg* msg);


////////////////////////////////////////////////////////////////////////////////
/// MpcpGateDiscovery:  locates discovery information in a 10G gate message
///
/// The caller should ensure that the message is actually a discovery gate
/// (msg->flags & Discovery)
///
/// \param m    Message containing discovery info
///
/// \return
/// Pointer to discovery info
////////////////////////////////////////////////////////////////////////////////
extern
MpcpGateDiscoveryInfo10G* MpcpGateDiscovery10G (const MpcpGateMsg* msg);


////////////////////////////////////////////////////////////////////////////////
// Report
////////////////////////////////////////////////////////////////////////////////

/// Report message layout
typedef struct
    {
    MpcpOpcode      opcode;
    MpcpTimestamp   timestamp;
    U8              numReports;
    } PACK MpcpReportMsg;

/// A report message has a number of queue reports
typedef struct
    {
    U8              bitmap;
    // followed by 1-8 queue values
    } PACK MpcpQReport;

// each report has 1-8 queue values, as specified by the bitmap
typedef U16 MpcpQReportValue;


////////////////////////////////////////////////////////////////////////////////
/// MpcpReportGetReport:  gets the given report structure from a report
///
/// The caller is responsible for ensuring that the request report is
/// within the message (less than msg->numReports)
///
/// \return
/// Pointer to Mpcp report Q
////////////////////////////////////////////////////////////////////////////////
extern
MpcpQReport* MpcpReportGetQReport (const MpcpReportMsg* msg, U8 report);


////////////////////////////////////////////////////////////////////////////////
/// MpcpReportGetQValue:  Gets a queue value from a queue report
///
/// This routine actually checks whether or not the requested queue value
/// is actually in the report, unlike most of these routines
///
/// \return
/// Reported queue value; 0 if not present
////////////////////////////////////////////////////////////////////////////////
extern
MpcpQReportValue MpcpReportGetQValue (const MpcpQReport* report, U8 queue);


////////////////////////////////////////////////////////////////////////////////
// Register Req
////////////////////////////////////////////////////////////////////////////////

/// Register Req messasge layout
typedef struct
    {
    MpcpOpcode        opcode;
    MpcpTimestamp     timestamp;
    U8                flags;
    U8                pendGrants;
    } PACK MpcpRegisterReq1GMsg;


typedef struct
    {
    MpcpOpcode        opcode;
    MpcpTimestamp     timestamp;
    U8                flags;
    U8                pendGrants;
    MpcpDiscoveryInfo discoveryInfo;
    U8                laserOn;
    U8                laserOff;
    } PACK MpcpRegisterReq10GMsg;


#define MpcpRegReqFlagRegister     1
#define MpcpRegReqFlagDeregister   3

#define MpcpMaxAllowedLaserOn     32
#define MpcpMaxAllowedLaserOff    32


////////////////////////////////////////////////////////////////////////////////
// Register
////////////////////////////////////////////////////////////////////////////////

typedef U16 MpcpPhyLlid;


/// Register message layout
typedef struct
    {
    MpcpOpcode      opcode;
    MpcpTimestamp   timestamp;
    MpcpPhyLlid     assignedPort;
    U8              flags;
    MpcpInterval16  nonDiscoverySyncTime;
    U8              pendGrants;
    } PACK MpcpRegister1GMsg;


typedef struct
    {
    MpcpOpcode      opcode;
    MpcpTimestamp   timestamp;
    MpcpPhyLlid     assignedPort;
    U8              flags;
    MpcpInterval16  nonDiscoverySyncTime;
    U8              pendGrants;
    U8              targetLaserOn;
    U8              targetLaserOff;
    } PACK MpcpRegister10GMsg;

#define MpcpRegFlagReregister       1
#define MpcpRegFlagDeallocate       2
#define MpcpRegFlagSuccess          3
#define MpcpRegFlagNack             4


////////////////////////////////////////////////////////////////////////////////
// Register Ack
////////////////////////////////////////////////////////////////////////////////

/// Register Ack message layout
typedef struct
    {
    MpcpOpcode      opcode;
    MpcpTimestamp   timestamp;
    U8              flags;
    MpcpPhyLlid     assignedPort;
    MpcpInterval16  nonDiscoverySyncTime;
    } PACK MpcpRegisterAckMsg;

#define MpcpRegAckFlagsSuccess      0x01
#define MpcpRegAckFlagsFailure      0x00

#if defined(__cplusplus)
}
#endif

#endif // Mpcp.h
