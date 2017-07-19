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
 * AdslFile.h
 *
 * Description:
 *		This file contains definitions for Adsl File I/O 
 *
 *
 * Authors: Ilya Stomakhin
 *
 * $Revision: 1.1 $
 *
 * $Id: AdslFile.h,v 1.1 2004/04/14 21:11:59 ilyas Exp $
 *
 * $Log: AdslFile.h,v $
 * Revision 1.1  2004/04/14 21:11:59  ilyas
 * Inial CVS checkin
 *
 *
 *****************************************************************************/

typedef struct {
	char	imageId[4];
	long	lmemOffset;
	long	lmemSize;
	long	sdramOffset;
	long	sdramSize;
	char	reserved[12];
} adslPhyImageHdr;

int AdslFileLoadImage(char *fname, void *pAdslLMem, void *pAdslSDRAM);
