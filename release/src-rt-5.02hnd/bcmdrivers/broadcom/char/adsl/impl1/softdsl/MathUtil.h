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
/************************************************************************
 *
 *	MathUtil.h:
 *
 *	Description:
 *	This file contains the exported interface for MathUtil.c module.
 *
 *
 * Copyright (c) 1993-1997 AltoCom, Inc. All rights reserved.
 * Authors: Mark Gonikberg, Haixiang Liang.
 *
 * $Revision: 1.6 $
 *
 * $Id: MathUtil.h,v 1.6 2004/04/13 00:21:13 ilyas Exp $
 *
 * $Log: MathUtil.h,v $
 * Revision 1.6  2004/04/13 00:21:13  ilyas
 * Added standard header for shared ADSL driver files
 *
 * Revision 1.5  2001/08/16 02:18:08  khp
 * - mark functions with FAST_TEXT to reduce cycle counts for QPROC targets
 *   (replaces use of LMEM_INSN)
 *
 * Revision 1.4  1999/10/06 04:55:22  liang
 * Added function to multiply two long values to save result as VeryLong.
 *
 * Revision 1.3  1999/08/05 19:42:52  liang
 * Merged with the softmodem top of the tree on 08/04/99 for assembly files.
 *
 * Revision 1.2  1999/03/26 03:29:59  liang
 * Export CosSin table.
 *
 * Revision 1.1  1998/10/28 01:28:07  liang
 * *** empty log message ***
 *
 * Revision 1.12  1998/02/10  17:19:49  scott
 * Changed MathVL routines to return arguments using pointers
 *
 * Revision 1.11  1997/12/13 06:12:07  mwg
 * Added more Atan2 flavors
 *
 * Revision 1.10  1997/11/18 01:11:48  mwg
 * Removed <CR> symbols which accidently slipped in.
 *
 * Revision 1.9  1997/11/03  19:07:52  scott
 * No longer redefine max() and min() if already defined
 *
 * Revision 1.8  1997/07/30 01:35:20  liang
 * Add more accurate atan2 function UtilLongLongAtan2.
 *
 * Revision 1.7  1997/07/21  20:23:19  mwg
 * Added new function: UtilBlockCos()
 *
 * Revision 1.6  1997/03/21  23:50:10  liang
 * Added initial version of V8bis module to CVS tree.
 *
 * Revision 1.5  1997/03/19  18:35:34  mwg
 * Changed copyright notice.
 *
 * Revision 1.4  1997/01/21  00:36:15  mwg
 * Added new function: UtilBlockCosSin()
 *
 * Revision 1.3  1996/06/18  21:14:45  mwg
 * Modified VLDivVL by allowing to specify the result scaling.
 *
 * Revision 1.2  1996/06/12  02:31:59  mwg
 * Added 64bit arithmetic functions.
 *
 * Revision 1.1.1.1  1996/02/14  02:35:15  mwg
 * Redesigned the project directory structure. Merged V.34 into the project.
 *
 * Revision 1.4  1995/12/04  23:08:15  liang
 * Add file Math/LinearToLog.c.
 *
 ************************************************************************/
#ifndef	MathUtilPh
#define	MathUtilPh

/* Exported tables */
extern	const short		UtilCosTable[];

/* Exported functions */
extern	ComplexShort	UtilCosSin(ushort angle);
extern	long			UtilBlockCosSin (int nValues, long angle, long delta, ComplexShort *dstPtr);
extern	long			UtilBlockCos (int nValues, long angle, long delta, short *dstPtr);
extern	ushort			UtilShortShortAtan2(ComplexShort point);
extern	ushort			UtilLongShortAtan2(ComplexLong point);
extern	ulong			UtilShortLongAtan2(ComplexShort point) FAST_TEXT;
extern	ulong			UtilLongLongAtan2(ComplexLong point) FAST_TEXT;
extern	ushort			UtilSqrt(ulong y);
extern	ushort			UtilMaxMagnitude(int blkSize, ComplexShort *dataPtr);
extern	short			UtilQ0LinearToQ4dB (ulong x);
extern	ulong			UtilQ4dBToQ12Linear (short x);
extern	void			UtilAdjustComplexMagnitude(ComplexShort	*srcPtr, short mag, short adjustment);

extern	void VLMultLongByLong(long x, long y, VeryLong *dst);
extern	void VLMultShort	(VeryLong x, short y, VeryLong *dst);
extern	void VLAddVL		(VeryLong x, VeryLong y, VeryLong *dst);
extern	void VLAddLong	(VeryLong x, long y, VeryLong *dst);
extern	void VLSubVL		(VeryLong x, VeryLong y, VeryLong *dst);
extern	void VLSubLong	(VeryLong x, long y, VeryLong *dst);
extern	void VLDivVL		(VeryLong x, VeryLong y, int scale, long *dst);
extern	void VLShiftLeft(VeryLong x, int shift, VeryLong *dst);
extern	void VLShiftRight(VeryLong x, int shift, VeryLong *dst);


#define	UtilAtan2		UtilShortShortAtan2
#define	UtilLongAtan2	UtilLongShortAtan2

/* Standard Macros	*/
#undef abs
#define		abs(x)			((x) >= 0   ? (x) : -(x))

#undef max
#define		max(x, y)		((x) >= (y) ? (x) : (y))

#undef min
#define		min(x, y)		((x) <= (y) ? (x) : (y))

#endif	/* MathUtilPh */
