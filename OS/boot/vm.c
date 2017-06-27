#include "pmap.h"

static u_long freemem;

static void *alloc(u_int n, u_int align, int clear) {
    u_long alloced_mem;
    freemem = ROUND(freemem, align);
    alloced_mem = freemem;
    freemem = freemem + n;
    if (clear) {
        bzero((void *) alloced_mem, n);
    }
    return (void *) alloced_mem;
}

static Pte *boot_pgdir_walk(Pde *pgdir, u_long va, int create) {

    // Our convention:
    // Use 40bit virtual address (4kB page and 4 levels page table)
    // [   2   |   9   |   9   |   9   |   12   ]
    //    Pgd      Pde     Pme     Pte     Page
    Pge *pge;
    Pde *pde;
    Pme *pme;
    Pte *pte;

    pge = (Pge *)(&pgdir[PGX(va)]);

    pde = (Pde *)(PTE_ADDR(*pge) + PDX(va));
    if (!(*pde & PTE_V)) {
        if (!create) return NULL;

        pde = (Pme *)alloc(BY2PG, BY2PG, 1);
        *pge = pde | PTE_V;
        pde += PDX(va);
    }

    pme = (Pme *)(PTE_ADDR(*pde) + PMX(va));
    if (!(*pme & PTE_V)) {
        if (!create) return NULL;

        pme = (Pme *)alloc(BY2PG, BY2PG, 1);
        *pde = pme | PTE_V;
        pme += PMX(va);
    }

    pte = (Pte *)(PTE_ADDR(*pme) + PTX(va));
    if (!(*pte & PTE_V)) {
        if (!create) return NULL;

        pte = (Pte *)alloc(BY2PG, BY2PG, 1);
        *pme = pte | PTE_V;
        pte += PTX(va);
    }

    return pte;
}

void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
    Pte *pte;
    int i;
    for (i = 0; i < size; i += BY2PG) {
        pte = boot_pgdir_walk(pgdir, va + i, 1);
        *pte = PTE_ADDR(pa + i) | perm | PTE_V | ATTRIB_AP_RW_EL1 | ATTRIB_SH_INNER_SHAREABLE | AF | UXN;
    }
}

void vm_init() {
    Pde *pgdir;
    u_int n;

    freemem = KSTACKTOP;

    uart_init();
    printf("System started!\n");


    pgdir = alloc(BY2PG, BY2PG, 1);
    boot_pgdir = pgdir;

    n = ROUND(MAXPA, BY2PG);

    boot_map_segment(pgdir, 0, n, 0, ATTRINDX_NORMAL);
    boot_map_segment(pgdir, n, n, n, ATTRINDX_DEVICE);

}