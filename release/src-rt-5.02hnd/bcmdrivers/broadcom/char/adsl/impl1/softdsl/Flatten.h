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
/****************************************************************************
 *
 * Flatten.h -- Header for Flatten/Unflatten command/status
 *
 * Copyright (c) 1998 AltoCom, Inc. All rights reserved.
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.14 $
 *
 * $Id: Flatten.h,v 1.14 2004/03/04 19:48:52 linyin Exp $
 *
 * $Log: Flatten.h,v $
 * Revision 1.14  2004/03/04 19:48:52  linyin
 * Support adsl2plus
 *
 * Revision 1.13  2003/10/17 22:45:14  yongbing
 * Increase buffer size for large B&G table of G992P3
 *
 * Revision 1.12  2003/08/12 23:16:26  khp
 * - for Haixiang: added support for ADSL_MARGIN_TWEAK_TEST
 *
 * Revision 1.11  2003/02/27 06:33:03  ilyas
 * Improved free space checking in command buffer (became a problem with
 * 2 commands SetXmtgain and StartPhy)
 *
 * Revision 1.10  2003/01/11 01:27:07  ilyas
 * Improved checking for available space in status buffer
 *
 * Revision 1.9  2002/09/07 01:43:59  ilyas
 * Added support for OEM parameters
 *
 * Revision 1.8  2002/05/16 00:01:52  khp
 * -added missing #endif
 *
 * Revision 1.7  2002/05/15 00:04:48  mprahlad
 * increase the status buffer size - prevent memory overflow for annexC cases
 *
 * Revision 1.6  2002/04/05 04:10:33  linyin
 * -hack to fit in Annex C firmware in LMEM
 *
 * Revision 1.5  2002/04/05 02:45:25  linyin
 * Make the buffer side larger for annexC
 *
 * Revision 1.4  2002/01/30 07:19:06  ilyas
 * Moved showtime code to LMEM
 *
 * Revision 1.3  2001/08/29 02:56:01  ilyas
 * Added tests for flattening/unflatenning command and statuses (dual mode)
 *
 * Revision 1.2  2001/04/25 00:30:54  ilyas
 * Adjusted MaxFrameLen
 *
 * Revision 1.1  2001/04/24 21:41:21  ilyas
 * Implemented status flattening/unflattaning to transfer statuses between
 * modules asynchronously through the circular buffer
 *
 *
 *****************************************************************************/

#ifndef _Flatten_H_
#define _Flatten_H_

#include	"CircBuf.h"

#ifdef ADSL_MARGIN_TWEAK_TEST
#define kMaxFlattenedCommandSize	272		/* maximum no. of bytes in flattened cmd */
#else
#define kMaxFlattenedCommandSize	128		/* maximum no. of bytes in flattened cmd */
#endif
#if	defined(G992_ANNEXC) || defined(G992P3)
#if defined(G992P5)
#define kMaxFlattenedStatusSize		2200   	/* maximum no. of bytes in flattened status */
#else
#define kMaxFlattenedStatusSize		1100   	/* maximum no. of bytes in flattened status */
#endif
#else
#define kMaxFlattenedStatusSize		 550   	/* maximum no. of bytes in flattened status */
#endif

#define	kMaxFlattenFramelength		(kMaxFlattenedStatusSize - (4*sizeof(long)) - 20)

extern int	SM_DECL FlattenCommand	(dslCommandStruct *cmd, ulong *dstPtr, ulong nAvail);
extern int	SM_DECL UnflattenCommand(ulong *srcPtr, dslCommandStruct *cmd);
extern int	SM_DECL FlattenStatus	(dslStatusStruct *status, ulong *dstPtr, ulong nAvail);
extern int	SM_DECL UnflattenStatus	(ulong *srcPtr, dslStatusStruct *status);

#define	FlattenBufferInit(fb,fbData,bufSize,itemSize)		\
	StretchBufferInit(fb, fbData, bufSize, itemSize)
	
extern int	SM_DECL FlattenBufferStatusWrite(stretchBufferStruct *fBuf, dslStatusStruct *status);
extern int	SM_DECL FlattenBufferStatusRead(stretchBufferStruct *fBuf, dslStatusStruct *status);

extern int	SM_DECL FlattenBufferCommandWrite(stretchBufferStruct *fBuf, dslCommandStruct *cmd);
extern int	SM_DECL FlattenBufferCommandRead(stretchBufferStruct *fBuf, dslCommandStruct *cmd);

#define FlattenBufferReadComplete(fb,nBytes)				\
	StretchBufferReadUpdate (fb, nBytes)

#endif /* _Flatten_H_ */

