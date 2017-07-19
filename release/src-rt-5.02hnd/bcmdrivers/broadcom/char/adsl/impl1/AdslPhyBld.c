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

#include <stdio.h>

#include "../AdslFile.h"

#include "adsl_lmem.h"
#include "adsl_sdram.h"

void AssignBigEndian (long *pLong, long val)
{
	char	*pMem = (char *) pLong;

	pMem[0] = (val >> 24) & 0xFF;
	pMem[1] = (val >> 16) & 0xFF;
	pMem[2] = (val >> 8)  & 0xFF;
	pMem[3] = val & 0xFF;
}

int main(int argc, char *argv[])
{
	FILE				*binFile;
	char				*fname;
	adslPhyImageHdr		phyHdr;
	int					i;

	fname = (argc < 2 ? "adslphy.bin" : argv[1]);
	binFile = fopen(fname, "wb");
	if (binFile == NULL) {
		printf("Failed to open input file %s\n", fname);
		return -1;
	}

#ifdef ADSL_ANNEXC
	phyHdr.imageId[0]	= 'C';
	phyHdr.imageId[1]	= ' ';
#elif defined(ADSL_ANNEXB)
	phyHdr.imageId[0]	= 'B';
	phyHdr.imageId[1]	= ' ';
#elif defined(ADSL_SADSL)
	phyHdr.imageId[0]	= 'S';
	phyHdr.imageId[1]	= 'A';
#else
	phyHdr.imageId[0]	= 'A';
	phyHdr.imageId[1]	= ' ';
#endif
	phyHdr.imageId[2]	= 0;
	phyHdr.imageId[3]	= 0;

	for (i = 0; i < sizeof(phyHdr.reserved); i++)
		phyHdr.reserved[i] = 0;

	AssignBigEndian (&phyHdr.lmemOffset, sizeof(phyHdr));
	AssignBigEndian (&phyHdr.lmemSize, sizeof(adsl_lmem));
	AssignBigEndian (&phyHdr.sdramOffset, sizeof(phyHdr) + sizeof(adsl_lmem));
	AssignBigEndian (&phyHdr.sdramSize, sizeof(adsl_sdram));
	fwrite(&phyHdr, 1, sizeof(phyHdr), binFile);
	fwrite(adsl_lmem, 1, sizeof(adsl_lmem), binFile);
	fwrite(adsl_sdram, 1, sizeof(adsl_sdram), binFile);
	fclose (binFile);
	
	return 0;
}
