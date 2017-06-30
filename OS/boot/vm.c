#include "mmu.h"

u_long freemem;

void boot_bzero(void* b, size_t len) {
    u_long max = (u_long)b + len;
    while ((u_long)b + 7 < max) {
        *(long *) b = 0;
        b += 8;
    }
    while ((u_long)b < max) {
        *(char *) b++ = 0;
    }
}

static void *boot_alloc(u_int n, u_int align, int clear) {
    u_long alloced_mem;
    freemem = ROUND(freemem, align);
    alloced_mem = freemem;
    freemem += n;
    if (clear) {
        boot_bzero((void *) alloced_mem, n);
    }
    return (void *) alloced_mem;
}


static Pte *boot_pgdir_walk(u_long *pgdir, u_long va) {
    // Use 40bit virtual address (4kB page and 4 levels page table)
    // [   2   |   9   |   9   |   9   |   12   ]
    //    Pge     Pde     Pme     Pte     Page
    Pte *pge;
    Pte *pde;
    Pte *pme;
    Pte *pte;
    pge = (Pte *)(&pgdir[PGX(va)]);
    pde = (Pte *)(PTE_ADDR(*pge)) + PDX(va);
    if (!(*pge & PTE_V))
    {
        pde = (Pte *)boot_alloc(BY2PG, BY2PG, 1);
        *pge = (u_long)pde | PTE_V;
        pde += PDX(va);
    }
    pme = (Pte *)(PTE_ADDR(*pde)) + PMX(va);
    if (!(*pde & PTE_V))
    {
        pme = (Pte *)boot_alloc(BY2PG, BY2PG, 1);
        *pde = (u_long)pme | PTE_V;
        pme += PMX(va);
    }
    pte = (Pte *)(PTE_ADDR(*pme)) + PTX(va);
    if (!(*pme & PTE_V))
    {
        pte = (Pte *)boot_alloc(BY2PG, BY2PG, 1);
        *pme = (u_long)pte | PTE_V;
        pte += PTX(va);
    }
    return pte;
}

void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
    Pte *pte;
    int i;
    for (i = 0; i < size; i += BY2PG) {
        pte = boot_pgdir_walk(pgdir, va + i);
        *pte = PTE_ADDR(pa + i) | perm | PTE_V | ATTRIB_AP_RW_EL1 | ATTRIB_SH_INNER_SHAREABLE | AF | UXN;
    }
}

void vm_init() {
    Pde *pgdir;
    u_int n;
    freemem = (u_long)KSTACKTOP;
    pgdir = boot_alloc(BY2PG, BY2PG, 1);
    n = ROUND(MAXPA, BY2PG);
    // 0x00000000 - 0x20000000 Normal memory
    boot_map_segment(pgdir, 0, n, 0, ATTRINDX_NORMAL);
    // 0x20000000 - 0x40000000 Device I/O
    boot_map_segment(pgdir, n, n, n, ATTRINDX_DEVICE);
}

