#ifndef OSLABPI_MMU_H
#define OSLABPI_MMU_H

#include "types.h"

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

#define TIMESTACKTOP (0xFFFFFF0000000000uL + 0x01751000uL)
#define KSTACKTOP    (0xFFFFFF0000000000uL + 0x01000000uL)
#define KERNBASE     (0xFFFFFF0000000000uL + 0x00080000uL)
#define KERNEL_PGDIR (0xFFFFFF0000000000uL + 0x01000000uL)
#define KERNEL_PAGES (0xFFFFFF0000000000uL + 0x01400000uL)
#define KERNEL_ENVS  (0xFFFFFF0000000000uL + 0x01700000uL)

#define USTACKTOP    (0x80000000)

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


#define NPAGE       0x40000
#define MAXPA       0x20000000


typedef u_long Pge;
typedef u_long Pde;
typedef u_long Pme;
typedef u_long Pte;


#define KADDR(pa) ((u_long)(pa) | 0xFFFFFF0000000000)

#define assert(x)                        \
do {    if (!(x)) panic("assertion failed: %s", #x); } while (0)

#endif //OSLABPI_MMU_H
