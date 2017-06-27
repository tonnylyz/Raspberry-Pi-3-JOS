#include "pmap.h"
struct Page *pages= (struct Page *)KERNEL_PAGES;
static struct Page_list page_free_list;

void page_init(void) {
    LIST_INIT(&page_free_list);
    u_long freemem = ROUND(TIMESTACKTOP, BY2PG);
    u_int used_index = VPN(freemem);
    u_int i;
    for (i = 0; i < used_index; i++) {
        pages[i].pp_ref = 1;
    }
    for (i = used_index; i < MAXPA / BY2PG; i++) {
        LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
    }
}

void page_bzero(size_t b, size_t len) {
    u_long max = (u_long)b + len;
    while ((u_long)b + 7 < max) {
        *(u_long *) b = 0;
        b += 8;
    }
    while ((u_long)b < max) {
        *(u_char *) b++ = 0;
    }
}

int page_alloc(struct Page **pp) {
    struct Page *ppage_temp;
    ppage_temp = LIST_FIRST(&page_free_list);
    if (ppage_temp == NULL) {
        return -E_NO_MEM;
    }
    LIST_REMOVE(ppage_temp, pp_link);
    page_bzero(page2kva(ppage_temp), BY2PG);
    ppage_temp->pp_ref = 0;
    *pp = ppage_temp;
    return 0;
}

void page_free(struct Page *pp) {
    if (pp->pp_ref > 0)
        return;
    if (pp->pp_ref == 0) {
        LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
        return;
    }
}

int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
    struct Page *ppage;
    u_long *pge;
    u_long *pde;
    u_long *pme;
    u_long *pte;

    pge = (u_long *)KADDR(&pgdir[PGX(va)]);
    pde = (u_long *)KADDR(PTE_ADDR(*pge)) + PDX(va);
    if (!(*pge & PTE_V)) {
        if (!create) {
            ppte = NULL;
            return 0;
        }
        if (page_alloc(&ppage) < 0) {
            return -E_NO_MEM;
        }
        pde = (u_long *)page2pa(ppage);
        *pge = (u_long)pde | PTE_V;
        pde += PDX(va);
        pde = (u_long *)KADDR(pde);
        ppage->pp_ref = 1;
    }

    pme = (u_long *)KADDR(PTE_ADDR(*pde)) + PMX(va);
    if (!(*pde & PTE_V)) {
        if (!create) {
            ppte = NULL;
            return 0;
        }
        if (page_alloc(&ppage) < 0) {
            return -E_NO_MEM;
        }
        pme = (u_long *)page2pa(ppage);
        *pde = (u_long)pme | PTE_V;
        pme += PMX(va);
        pme = (u_long *)KADDR(pme);
        ppage->pp_ref = 1;
    }

    pte = (u_long *)KADDR(PTE_ADDR(*pme)) + PTX(va);
    if (!(*pme & PTE_V)) {
        if (!create) {
            ppte = NULL;
            return 0;
        }
        if (page_alloc(&ppage) < 0) {
            return -E_NO_MEM;
        }
        pte = (u_long *)page2pa(ppage);
        *pme = (u_long)pte | PTE_V;
        pte += PTX(va);
        pte = (u_long *)KADDR(pte);
        ppage->pp_ref = 1;
    }
    *ppte = pte;
    return 0;
}

int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) {
    u_int PERM;
    Pte *pgtable_entry;
    PERM = perm | PTE_V | ATTRINDX_NORMAL | ATTRIB_SH_INNER_SHAREABLE | AF;

    pgdir_walk(pgdir, va, 0, &pgtable_entry);

    if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) {
        if (pa2page(*pgtable_entry) != pp) {
            page_remove(pgdir, va);
        } else {
            tlb_invalidate(pgdir, va);
            *pgtable_entry = (PTE_ADDR(page2pa(pp)) | PERM);
            return 0;
        }
    }
    tlb_invalidate(pgdir, va);
    if (pgdir_walk(pgdir, va, 1, &pgtable_entry) != 0) {
        return -E_NO_MEM;
    }
    *pgtable_entry = (PTE_ADDR(page2pa(pp)) | PERM);
    pp->pp_ref++;
    return 0;
}

struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
    struct Page *ppage;
    Pte *pte;
    pgdir_walk(pgdir, va, 0, &pte);
    if (pte == 0) {
        return 0;
    }
    if ((*pte & PTE_V) == 0) {
        return 0;
    }
    ppage = pa2page(PTE_ADDR(*pte));
    if (ppte) {
        *ppte = pte;
    }
    return ppage;
}

void page_decref(struct Page *pp) {
    if (--pp->pp_ref == 0) {
        page_free(pp);
    }
}

void page_remove(Pde *pgdir, u_long va) {
    Pte *pagetable_entry;
    struct Page *ppage;
    ppage = page_lookup(pgdir, va, &pagetable_entry);
    if (ppage == 0) {
        return;
    }
    ppage->pp_ref--;
    if (ppage->pp_ref == 0) {
        page_free(ppage);
    }
    *pagetable_entry = 0;
    tlb_invalidate(pgdir, va);
}


void tlb_invalidate(Pde *pgdir, u_long va) {
    __asm__ __volatile__ (
        "dsb ishst\n\t"
        "tlbi vmalle1is\n\t"
        "dsb ish\n\t"
        "isb\n\t"
    );
}
