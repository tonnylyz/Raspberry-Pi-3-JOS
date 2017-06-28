#ifndef OSLABPI_TYPE_H
#define OSLABPI_TYPE_H

typedef unsigned char u8;
typedef	unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef unsigned long u_int64_t;
typedef unsigned short u_int16_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned int u_int32_t;
typedef signed long int64_t;

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

typedef unsigned long long size_t;

#ifndef NULL
#define NULL ((void *) 0)
#endif /* !NULL */

#define MIN(_a, _b)	\
	({		\
		typeof(_a) __a = (_a);	\
		typeof(_b) __b = (_b);	\
		__a <= __b ? __a : __b;	\
	})

/* Static assert, for compile-time assertion checking */
#define static_assert(c) switch (c) case 0: case(c):

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))

/* Rounding; only works for n = power of two */
#define ROUND(a, n)	(((((u_long)(a))+(n)-1)) & ~((n)-1))
#define ROUNDDOWN(a, n)	(((u_long)(a)) & ~((n)-1))

#endif //OSLABPI_TYPE_H
