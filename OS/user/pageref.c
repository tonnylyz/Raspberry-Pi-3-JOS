#include "lib.h"

typedef LIST_ENTRY(Page) Page_LIST_entry_t;
struct Page {
    Page_LIST_entry_t pp_link;
    unsigned short pp_ref;
};

int pageref(void *va) {
    u_long pte;
    pte = syscall_pgtable_entry((u_long) va);
    if (pte == 0)
        return 0;
    return pages[PTE_ADDR(pte) >> PGSHIFT].pp_ref;
}
