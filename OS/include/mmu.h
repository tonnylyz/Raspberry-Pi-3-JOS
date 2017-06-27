#ifndef OSLABPI_MMU_H
#define OSLABPI_MMU_H

#include "types.h"

#define BY2PG		4096
#define PDMAP		(4 * 1024 * 1024)	// bytes mapped by a page directory entry
#define PDSHIFT		30
#define PMSHIFT		21
#define PGSHIFT		12

#define PGX(va)		((((size_t)(va))>>39) & 0x01)
#define PDX(va)		((((size_t)(va))>>30) & 0x1FF)
#define PMX(va)		((((size_t)(va))>>21) & 0x1FF)
#define PTX(va)		((((size_t)(va))>>12) & 0x1FF)
#define PTE_ADDR(pte)	((size_t)(pte)& 0xFFFFFFF000)

#define PPN(va)		(((size_t)(va)) >> 12)
#define VPN(va)		(((size_t)(va) & 0xFFFFFFFFFF) >> 12)

#define PTE_V                     0x3 << 0    // Table Entry Valid bit
#define PBE_V                     0x1 << 0    // Block Entry Valid bit
#define ATTRIB_AP_RW_EL1          0x0 << 6
#define ATTRIB_AP_RW_ALL          0x1 << 6
#define ATTRIB_AP_RO_EL1          0x2 << 6
#define ATTRIB_AP_RO_ALL          0x3 << 6
#define ATTRIB_SH_NON_SHAREABLE   0x0 << 8
#define ATTRIB_SH_OUTER_SHAREABLE 0x2 << 8
#define ATTRIB_SH_INNER_SHAREABLE 0x3 << 8
#define AF                        0x1 << 10
#define PXN                       0x0 << 53
#define UXN                       0x1UL << 54

#define ATTRINDX_NORMAL           0 << 2    // inner/outer write-back non-transient, non-allocating
#define ATTRINDX_DEVICE           1 << 2    // Device-nGnRE
#define ATTRINDX_COHERENT           2 << 2 // Device-nGnRnE


#define KERNBASE    0xffffff0000080000
#define KSTACKTOP   0xffffff0001000000
#define ULIM        0xffffff0000000000

#define NPAGE       0x40000
#define MAXPA       0x40000000


typedef u_long Pge;
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
