#ifndef OSLABPI_MMU_H
#define OSLABPI_MMU_H

#include "types.h"

#define BY2PG		4096
#define PDMAP		(4 * 1024 * 1024)	// bytes mapped by a page directory entry
#define PDSHIFT		30
#define PMSHIFT		21
#define PGSHIFT		12
#define PDX(va)		((((size_t)(va))>>30) & 0x1FF)
#define PMX(va)		((((size_t)(va))>>21) & 0x1FF)
#define PTX(va)		((((size_t)(va))>>12) & 0x1FF)
#define PTE_ADDR(pte)	((size_t)(pte)&~0xFFF)

#define PPN(va)		((((size_t)(va))>>12) & 0x7FFFFFF) // 27 bit
#define VPN(va)		PPN(va)

#define PTE_V		0x0200	// Valid bit
#define PTE_R		0x0400	// Dirty bit ,'0' means only read ,otherwise make interrupt

#define KERNBASE    0xffffff8000080000
#define KSTACKTOP   0xffffff8000400000
#define ULIM        0xffffff8000000000

#define NPAGE       0x40000


typedef u_long Pde;
typedef u_long Pme;
typedef u_long Pte;

// translates from kernel virtual address to physical address.
#define PADDR(kva)                      \
({                                      \
u_long __a = (u_long) (kva);              \
    if (__a < ULIM)                       \
        panic("PADDR called with invalid kva %08lx", __a); \
    __a - ULIM;                           \
})

// translates from physical address to kernel virtual address.
#define KADDR(pa)                       \
({                                      \
    u_long __ppn = PPN(pa);               \
    if (__ppn >= NPAGE)                   \
        panic("KADDR called with invalid pa %08lx", (u_long)pa); \
    (pa) + ULIM;                        \
})

#define assert(x)	                    \
do {	if (!(x)) panic("assertion failed: %s", #x); } while (0)

#define TRUP(_p)   						\
({								        \
    register typeof((_p)) __m_p = (_p); \
    (u_int) __m_p > ULIM ? (typeof(_p)) ULIM : __m_p; \
})

#endif //OSLABPI_MMU_H
