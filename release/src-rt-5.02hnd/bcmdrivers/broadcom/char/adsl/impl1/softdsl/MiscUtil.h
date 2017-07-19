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
 * MiscUtil.h -- Miscellaneous utilities
 *
 * Description:
 *
 *
 * Copyright (c) 1993-1997 AltoCom, Inc. All rights reserved.
 * Authors: Mark Gonikberg Haixiang Liang
 *
 * $Revision: 1.4 $
 *
 * $Id: MiscUtil.h,v 1.4 2004/04/13 00:21:46 ilyas Exp $
 *
 * $Log: MiscUtil.h,v $
 * Revision 1.4  2004/04/13 00:21:46  ilyas
 * Added standard header for shared ADSL driver files
 *
 * Revision 1.3  2001/07/21 01:21:06  ilyas
 * Added more functions for int to string conversion used by log file
 *
 * Revision 1.2  1999/08/05 19:42:56  liang
 * Merged with the softmodem top of the tree on 08/04/99 for assembly files.
 *
 * Revision 1.1  1999/01/27 22:10:12  liang
 * Initial version.
 *
 * Revision 1.1  1997/07/10 01:18:45  mwg
 * Initial revision.
 *
 *
 *
 *****************************************************************************/
#ifndef _MISC_UTIL_H_
#define _MISC_UTIL_H_

extern long		SM_DECL	GetRateValue(dataRateMap rate);
extern int 		SM_DECL	DecToString(ulong value, uchar *dstPtr, uint nDigits);
extern int 		SM_DECL	HexToString(ulong value, uchar *dstPtr, uint nDigits);
extern char *	SM_DECL	DecToStr(char *s, ulong num);
extern char *	SM_DECL	SignedToStr(char *s, long num);
extern char *	SM_DECL	HexToStr(char *s, ulong num);

#define	EvenParityBit(x)	((z = (y = x ^ (x >> 4)) ^ (y >> 2)) ^ (z >> 1))
#define	OddParityBit(x)		(EvenParityBit(x) ^ 1)

extern void	ParityApply(int nBytes, int nDataBits, int parity, uchar *srcPtr, uchar *dstPtr);
extern void	ParityStrip(int nBytes, int nDataBits, int parity, uchar *srcPtr, uchar *dstPtr, statusHandlerType	statusHandler);

#endif
