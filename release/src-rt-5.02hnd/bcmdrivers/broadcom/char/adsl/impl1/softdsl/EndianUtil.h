#ifndef _ADSL_CORE_ENDIAN_CONV_H
#define _ADSL_CORE_ENDIAN_CONV_H

#if defined(WINNT) || defined(LINUX_DRIVER)
static __inline short DiagEndianCl2Srv(short shNum)
{
#ifdef WINNT
	__asm {
		mov		ax, shNum
		xchg	al, ah
	}
#elif LINUX_DRIVER
	register  short val  asm ("ax");
	__asm volatile (
		"movw	%1, %0;\n\t"
		"xchg	%%al, %%ah;\n\t"
		: "=r"(val): "d"(shNum) 
	);
	return val;
#endif
}

static __inline long DiagEndianCl2SrvLong(long lNum)
{
#ifdef WINNT
	__asm {
		mov		eax, lNum
		bswap   eax
	}
#elif LINUX_DRIVER
	register  long  val  asm ("eax");
	__asm (
		"movl	%1, %0 \n\t"
		"bswap  %%eax  \n\t"
		: "=r"(val): "d"(lNum) 
	);
	return val;
#endif
}

#define	ADSL_ENDIAN_CONV_LONG(x)	DiagEndianCl2SrvLong(x)
#define	ADSL_ENDIAN_CONV_SHORT(x)	DiagEndianCl2Srv(x)
#define	ADSL_ENDIAN_CONV_ULONG(x)	( (unsigned long)DiagEndianCl2SrvLong(x) )
#define	ADSL_ENDIAN_CONV_USHORT(x)	( (unsigned short)DiagEndianCl2Srv(x) )

#elif defined(ADSLDRV_LITTLE_ENDIAN)
/* ADSL Driver to/from PHY address and data conversion macros */
#if defined(__arm) || defined(CONFIG_ARM)
static __inline unsigned long brev32(unsigned long x)
{
#if defined(__GNUC__)
	asm volatile("rev %0, %1" : "=r" (x) : "r" (x));
#else
	__asm { rev  x, x };
#endif
	return x;
}

static __inline unsigned short brev16(unsigned short x)
{
#if defined(__GNUC__)
	asm volatile("revsh %0, %1" : "=r" (x) : "r" (x));
#else
	__asm { revsh  x, x };
#endif
	return x;
}

static __inline unsigned long brev1616(unsigned long x)
{
#if defined(__GNUC__)
	asm volatile("rev16 %0, %1" : "=r" (x) : "r" (x));
#else
	__asm { rev16  x, x };
#endif
	return x;
}
#define	ADSL_ENDIAN_CONV_LONG(x)	( (long)brev32(x) )
#define	ADSL_ENDIAN_CONV_SHORT(x)	( (short)brev16(x) )
#define	ADSL_ENDIAN_CONV_2SHORTS(x)	( (long)brev1616(x) )
#define	ADSL_ENDIAN_CONV_ULONG(x)	brev32(x)
#define	ADSL_ENDIAN_CONV_USHORT(x)	brev16(x)
#define	ADSL_ENDIAN_CONV_2USHORTS(x)	brev1616(x)
#else
#define	ADSL_ENDIAN_CONV_LONG(x)	( (long)(((x) << 24) | (((x) << 8) & 0x00FF0000) | (((x) >> 8) & 0x0000FF00) | ((unsigned long)(x) >> 24)) )
#define	ADSL_ENDIAN_CONV_SHORT(x)	( (short)(((x) << 8) | ((unsigned short)(x) >> 8)) )
#define	ADSL_ENDIAN_CONV_2SHORTS(x)	( (long)(((x) << 16) | ((unsigned long)(x) >> 16)) )
#define	ADSL_ENDIAN_CONV_ULONG(x)	( ((x) << 24) | (((x) << 8) & 0x00FF0000) | (((x) >> 8) & 0x0000FF00) | ((unsigned long)(x) >> 24) )
#define	ADSL_ENDIAN_CONV_USHORT(x)	(((x) << 8) | ((unsigned short)(x) >> 8))
#define	ADSL_ENDIAN_CONV_2USHORTS(x)	( ((x) << 16) | ((unsigned long)(x) >> 16) )
#endif

#else /* !ADSLDRV_LITTLE_ENDIAN */
#define	ADSL_ENDIAN_CONV_LONG(x)	( (long)(x) )
#define	ADSL_ENDIAN_CONV_SHORT(x)	( (short)(x) )
#define	ADSL_ENDIAN_CONV_2SHORTS(x)	( (long)(x) )
#define	ADSL_ENDIAN_CONV_ULONG(x)	( (unsigned long)(x) )
#define	ADSL_ENDIAN_CONV_USHORT(x)	( (unsigned short)(x) )
#define	ADSL_ENDIAN_CONV_2USHORTS(x)	( (unsigned long)(x) )
#endif

#endif
