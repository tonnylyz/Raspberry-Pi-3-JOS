#ifndef OSLABPI_MMU_H
#define OSLABPI_MMU_H

#ifndef USER_PROGRAM
#include "types.h"
#endif

#define BY2PG        4096
#define PDMAP        (4 * 1024 * 1024)    // bytes mapped by a page directory entry
#define PDSHIFT        30
#define PMSHIFT        21
#define PGSHIFT        12


#define PGX(va) ((((u_long)(va)) >> 39) & 0x01)
#define PDX(va) ((((u_long)(va)) >> 30) & 0x01FF)
#define PMX(va) ((((u_long)(va)) >> 21) & 0x01FF)
#define PTX(va) ((((u_long)(va)) >> 12) & 0x01FF)

#define PTE_ADDR(pte) ((u_long)(pte)& 0xFFFFFFF000)

#define PPN(va) (((u_long)(va)) >> 12)
#define VPN(va) (((u_long)(va) & 0xFFFFFFFFFF) >> 12)

#define KADDR(pa) ((u_long)(pa) | 0xFFFFFF0000000000uL)

#define KTOP         KADDR(0x02000000)
#define TIMESTACKTOP KADDR(0x01800000)
#define KERNEL_ENVS  KADDR(0x01700000)
#define KERNEL_PAGES KADDR(0x01400000)
#define KERNEL_PGDIR KADDR(0x01000000)
#define KSTACKTOP    KADDR(0x01000000)
#define KERNBASE     KADDR(0x00080000)

#define UTOP         (0xffffffff)
#define USTACKTOP    (0x01000000)
#define UENVS        (0x90000000)
#define UPAGES       (0xa0000000)

#define PTE_V                     (0x3 << 0)    // Table Entry Valid bit
#define PBE_V                     (0x1 << 0)    // Block Entry Valid bit
#define ATTRIB_AP_RW_EL1          (0x0 << 6)
#define ATTRIB_AP_RW_ALL          (0x1 << 6)
#define ATTRIB_AP_RO_EL1          (0x2 << 6)
#define ATTRIB_AP_RO_ALL          (0x3 << 6)
#define ATTRIB_SH_NON_SHAREABLE   (0x0 << 8)
#define ATTRIB_SH_OUTER_SHAREABLE (0x2 << 8)
#define ATTRIB_SH_INNER_SHAREABLE (0x3 << 8)
#define AF                        (0x1 << 10)
#define PXN                       (0x0 << 53)
#define UXN                       (0x1UL << 54)

#define ATTRINDX_NORMAL           (0 << 2)    // inner/outer write-back non-transient, non-allocating
#define ATTRINDX_DEVICE           (1 << 2)    // Device-nGnRE
#define ATTRINDX_COHERENT         (2 << 2) // Device-nGnRnE


#define NPAGE       0x20000
#define MAXPA       0x20000000



typedef u_long Pge;
typedef u_long Pde;
typedef u_long Pme;
typedef u_long Pte;



#define assert(x)                        \
do {    if (!(x)) panic("assertion failed: %s", #x); } while (0)

#endif //OSLABPI_MMU_H
