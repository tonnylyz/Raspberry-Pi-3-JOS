#ifndef OSLABPI_PMAP_H
#define OSLABPI_PMAP_H

#include "mmu.h"
#include "types.h"
#include "queue.h"
#include "printf.h"
#include "error.h"

Pde *boot_pgdir;

LIST_HEAD(Page_list, Page);
typedef LIST_ENTRY(Page) Page_LIST_entry_t;
struct Page {
    Page_LIST_entry_t pp_link;
    u_short pp_ref;
};

extern struct Page *pages;

static inline u_long page2ppn(struct Page *pp) {
    return pp - pages;
}

static inline u_long pten2va(u_int pten0, u_int pten1, u_int pten2, u_int pten3) {
    return (((u_long) pten0 << 39) | ((u_long) pten1 << 30) | ((u_long) pten2 << 21) | ((u_long) pten3 << 12));
}

/* Get the physical address of Page 'pp'. */
static inline u_long page2pa(struct Page *pp) {
    return (page2ppn(pp) << PGSHIFT);
}

static inline u_long page2kva(struct Page *pp) {
    return KADDR((u_long)(page2ppn(pp) << PGSHIFT));
}

/* Get the Page struct whose physical address is 'pa'. */
static inline struct Page *pa2page(u_long pa) {
    if (PPN(pa) >= MAXPA / BY2PG) {
        panic("pa2page called with invalid pa: %lx", pa);
    }
    return &pages[PPN(pa)];
}

void page_init(void);

int page_alloc(struct Page **pp);

void page_free(struct Page *pp);

void page_decref(struct Page *pp);

int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte);

int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm);

struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte);

void page_remove(Pde *pgdir, u_long va);

void tlb_invalidate();

void bcopy(const void *src, void *dst, size_t len);
void bzero(void* b, size_t len);

#endif //OSLABPI_PMAP_H