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

void bzero(void* b, size_t len) {
    u_long max = (u_long)b + len;
    while ((u_long)b + 7 < max) {
        *(u_long *) b = 0;
        b += 8;
    }
    while ((u_long)b < max) {
        *(u_char *) b++ = 0;
    }
}

void bcopy(const void *src, void *dst, size_t len) {
    void *max;
    max = dst + len;
    while (dst + 3 < max) {
        *(int *) dst = *(int *) src;
        dst += 4;
        src += 4;
    }
    while (dst < max) {
        *(char *) dst = *(char *) src;
        dst += 1;
        src += 1;
    }
}

int page_alloc(struct Page **pp) {
    struct Page *page;
    page = LIST_FIRST(&page_free_list);
    if (page == NULL) {
        return -E_NO_MEM;
    }
    LIST_REMOVE(page, pp_link);
    bzero((void *)page2kva(page), BY2PG);
    page->pp_ref = 0;
    *pp = page;
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
    pgdir_walk(pgdir, va, 1, &pgtable_entry);
    if ((*pgtable_entry & PTE_V) != 0) {
        printf("[WARNING] page_insert : a page was already here.\n");
    }
    *pgtable_entry = (PTE_ADDR(page2pa(pp)) | PERM);
    tlb_invalidate();
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
    tlb_invalidate();
}


void map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
    Pte *pte;
    int i;
    for (i = 0; i < size; i += BY2PG) {
        pgdir_walk(pgdir, va + i, 1, &pte);
        *pte = PTE_ADDR(pa + i) | perm | PTE_V;
    }
}