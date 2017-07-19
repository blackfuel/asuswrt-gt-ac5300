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
 * DslFramer.h 
 *
 * Description:
 *	This file contains common DSL framer definitions
 *
 *
 * Copyright (c) 1993-1998 AltoCom, Inc. All rights reserved.
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.3 $
 *
 * $Id: DslFramer.h,v 1.3 2004/07/21 01:39:41 ilyas Exp $
 *
 * $Log: DslFramer.h,v $
 * Revision 1.3  2004/07/21 01:39:41  ilyas
 * Reset entire G.997 state on retrain. Timeout in G.997 if no ACK
 *
 * Revision 1.2  2004/04/12 23:41:10  ilyas
 * Added standard header for shared ADSL driver files
 *
 * Revision 1.1  2001/12/13 02:28:27  ilyas
 * Added common framer (DslPacket and G997) and G997 module
 *
 *
 *
 *****************************************************************************/

#ifndef	DslFramerHeader
#define	DslFramerHeader

#include "DList.h"

#define	kDslFramerInitialized			0x80000000

/* status codes */

#define	kDslFramerRxFrame				1
#define	kDslFramerRxFrameErr			2
#define kDslFramerTxFrame				3
#define	kDslFramerTxFrameErr			4

#define	kDslFramerRxFrameErrFlushed		1
#define	kDslFramerRxFrameErrAbort		2
#define	kDslFramerRxFrameErrPhy			3

#define	kDslFramerTxFrameErrFlushed		1


typedef	struct _dslFramerBufDesc {
	long		pkId;
	long		bufFlags;
	void		*bufPtr;
	long		bufLen;
} dslFramerBufDesc;

/* data bufDesc flags */

#define kDslFramerStartNewFrame			1
#define kDslFramerEndOfFrame			2
#define kDslFramerAbortFrame			4

#define kDslFramerExtraByteShift		3
#define kDslFramerExtraByteMask			(0x7 << kDslFramerExtraByteShift)

typedef struct _dslFramerControl {
	bitMap					setup;
	dslFrameHandlerType		rxIndicateHandlerPtr;
	dslFrameHandlerType		txCompleteHandlerPtr;
	dslStatusHandlerType	statusHandlerPtr;
	ulong					statusCode;
	ulong					statusOffset;

	int						nRxBuffers;
	int						nRxBufSize;
	int						nRxPackets;

	dslFrame				*freeBufListPtr;
	void					*freeBufPool;
	void					*pBufMemory;

	dslFrame				*freePacketListPtr;
	void					*freePacketPool;

	/* RX working data set */

	dslFrame				*pRxFrame;
	dslFrameBuffer			*pRxBuf;
	uchar					*pRxBufData;
	uchar					*pRxBufDataEnd;
	int						rxFrameLen;

	/* TX working data set */

	DListHeader				dlistTxWaiting;
	dslFrame				*pTxFrame;
	dslFrameBuffer			*pTxBuf;
	uchar					*pTxBufData;
	uchar					*pTxBufDataEnd;

	/* stats data */

	ulong					dslByteCntRxTotal;
	ulong					dslByteCntTxTotal;

	ulong					dslFrameCntRxTotal;
	ulong					dslFrameCntRxErr;
	ulong					dslFrameCntTxTotal;
	
} dslFramerControl;


extern Boolean  DslFramerInit(
			void					*gDslVars,
			dslFramerControl		*dfCtrl,
			bitMap					setup,
			ulong					statusCode,
			ulong					statusOffset,
			dslFrameHandlerType		rxIndicateHandlerPtr,
			dslFrameHandlerType		txCompleteHandlerPtr,
			dslStatusHandlerType	statusHandlerPtr,
			ulong					rxBufNum,
			ulong					rxBufSize,
			ulong					rxPacketNum);
extern void DslFramerClose(void *gDslVars, dslFramerControl *dfCtrl);
extern void DslFramerSendFrame(void *gDslVars, dslFramerControl *dfCtrl, dslFrame *pFrame);
extern void DslFramerReturnFrame(void *gDslVars, dslFramerControl *dfCtrl, dslFrame *pFrame);


extern Boolean DslFramerRxGetPtr(void *gDslVars, dslFramerControl *dfCtrl, dslFramerBufDesc *pBufDesc);
extern void	DslFramerRxDone  (void *gDslVars, dslFramerControl *dfCtrl, dslFramerBufDesc *pBufDesc);
extern Boolean	DslFramerTxGetPtr(void *gDslVars, dslFramerControl *dfCtrl, dslFramerBufDesc *pBufDesc);
extern void	DslFramerTxDone(void *gDslVars, dslFramerControl *dfCtrl, dslFramerBufDesc *pBufDesc);
extern Boolean DslFramerTxIdle (void *gDslVars, dslFramerControl *dfCtrl);
extern void DslFramerTxFlush(void *gDslVars, dslFramerControl *dfCtrl);

extern void * DslFramerGetFramePoolHandler(dslFramerControl *dfCtrl);
extern void DslFramerClearStat(dslFramerControl *dfCtrl);

extern void DslFramerRxFlushFrame (void *gDslVars, dslFramerControl *dfCtrl, int errCode);
extern void DslFramerRxFlush(void *gDslVars, dslFramerControl *dfCtrl);

#endif	/* DslFramerHeader */
