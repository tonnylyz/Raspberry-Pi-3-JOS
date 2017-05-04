#include "pmap.h"

struct Page *pages;
static u_long freemem;

static struct Page_list page_free_list;    /* Free list of physical pages */

/* Overview:
 	Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, clear the
 	allocated memory.
 	This allocator is used only while setting up virtual memory system.

   Post-Condition:
	If we're out of memory, should panic, else return this address of memory we have allocated.*/
static void *alloc(u_int n, u_int align, int clear) {
    u_long alloced_mem;
    /* Initialize `freemem` if this is the first time. The first virtual address that the
     * linker did *not* assign to any kernel code or global variables. */
    if (freemem == 0) {
        freemem = KSTACKTOP;
    }

    /* Step 1: Round up `freemem` up to be aligned properly */
    freemem = ROUND(freemem, align);

    /* Step 2: Save current value of `freemem` as allocated chunk. */
    alloced_mem = freemem;

    /* Step 3: Increase `freemem` to record allocation. */
    freemem = freemem + n;

    /* Step 4: Clear allocated chunk if parameter `clear` is set. */
    if (clear) {
        bzero((void *) alloced_mem, n);
    }

    /* Step 5: return allocated chunk. */
    return (void *) alloced_mem;
}

/* Overview:
 	Get the page table entry for virtual address `va` in the given
 	page directory `pgdir`.
	If the page table is not exist and the parameter `create` is set to 1,
	then create it.*/
static Pte *boot_pgdir_walk(Pde *pgdir, u_long va, int create) {

    Pde *pgdir_entryp;
    Pte *pgtable, *pgtable_entry;

    /* Step 1: Get the corresponding page directory entry and page table. */
    /* Hint: Use KADDR and PTE_ADDR to get the page table from page directory
     * entry value. */

    pgdir_entryp = (Pde *) (&pgdir[PDX(va)]);
    pgtable = KADDR(PTE_ADDR(*pgdir_entryp));

    /* Step 2: If the corresponding page table is not exist and parameter `create`
     * is set, create one. And set the correct permission bits for this new page
     * table. */

    if (*pgdir_entryp == 0) {
        if (!create) return NULL;

        pgtable = alloc(BY2PG, BY2PG, 1);
        *pgdir_entryp = PADDR(pgtable) | PTE_V | PTE_R;
    }

    /* Step 3: Get the page table entry for `va`, and return it. */
    pgtable_entry = (Pte *) (&pgtable[PTX(va)]);
    return pgtable_entry;
}

/*Overview:
 	Map [va, va+size) of virtual address space to physical [pa, pa+size) in the page
	table rooted at pgdir.
	Use permission bits `perm|PTE_V` for the entries.
 	Use permission bits `perm` for the entries.

  Pre-Condition:
	Size is a multiple of BY2PG.*/
void boot_map_segment(Pde *pgdir, u_long va, u_long size, u_long pa, int perm) {
    int va_temp;
    Pte *pgtable_entry;

    /* Step 1: Check if `size` is a multiple of BY2PG. */
    if (size % BY2PG != 0)
        panic("Size is not a multiple of BY2PG.");

    /* Step 2: Map virtual address space to physical address. */
    int i;
    for (i = 0; i < size; i += BY2PG) {
        va_temp = va + i;
        /* Hint: Use `boot_pgdir_walk` to get the page table entry of virtual address `va`. */
        pgtable_entry = boot_pgdir_walk(pgdir, va_temp, 1);
        *pgtable_entry = (pa + i) | perm | PTE_V;
    }

    pgdir[PDX(va)] = pgdir[PDX(va)] | perm;
}

/* Overview:
    Set up two-level page table.

   Hint:
    You can get more details about `UPAGES` and `UENVS` in include/mmu.h. */
void vm_init() {
    //extern int mCONTEXT; // TODO: Place PGD (PDE Page Directory Entry)

    Pde *pgdir;
    u_int n;
    pgdir = alloc(BY2PG, BY2PG, 1);
    boot_pgdir = pgdir;
    pages = (struct Page *) alloc(NPAGE * sizeof(struct Page), BY2PG, 1);
    n = ROUND(NPAGE * sizeof(struct Page), BY2PG);
    //boot_map_segment(pgdir, UPAGES, n, PADDR(pages), PTE_R);

    printf("pmap.c:\t vm init success\n");
}

/*Overview:
 	Initialize page structure and memory free list.
 	The `pages` array has one `struct Page` entry per physical page. Pages
	are reference counted, and free pages are kept on a linked list.
  Hint:
	Use `LIST_INSERT_HEAD` to insert something to list.*/
void page_init(void) {
    /* Step 1: Initialize page_free_list. */
    /* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
    LIST_INIT(&page_free_list);

    /* Step 2: Align `freemem` up to multiple of BY2PG. */

    freemem = ROUND(freemem, BY2PG);
    /* Step 3: Mark all memory below `freemem` as used(set `pp_ref`
     * filed to 1) */

    u_int used_index = VPN(freemem);

    u_int i;
    for (i = 0; i < used_index; i++) {
        pages[i].pp_ref = 1;
    }
    /* Step 4: Mark the other memory as free. */
    for (i = used_index; i < NPAGE; i++) {
        pages[i].pp_ref = 0;
        LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
    }
}

/*Overview:
	Allocates a physical page from free memory, and clear this page.

  Post-Condition:
	If failed to allocate a new page(out of memory(there's no free page)),
 	return -E_NO_MEM.
	Else, set the address of allocated page to *pp, and returned 0.

  Note:
 	Does NOT increment the reference count of the page - the caller must do
 	these if necessary (either explicitly or via page_insert).

  Hint:
	Use LIST_FIRST and LIST_REMOVE defined in include/queue.h .*/
int page_alloc(struct Page **pp) {
    struct Page *ppage_temp;

    /* Step 1: Get a page from free memory. If fails, return the error code.*/
    ppage_temp = LIST_FIRST(&page_free_list);
    if (ppage_temp == NULL) {
        return -E_NO_MEM;
    }


    /* Step 2: Initialize this page.
     * Hint: use `bzero`. */
    LIST_REMOVE(ppage_temp, pp_link);

    bzero(page2kva(ppage_temp), BY2PG);
    ppage_temp->pp_ref = 0;
    *pp = ppage_temp;
    return 0;
}

/*Overview:
	Release a page, mark it as free if it's `pp_ref` reaches 0.
  Hint:
	When to free a page, just insert it to the page_free_list.*/
void page_free(struct Page *pp) {
    /* Step 1: If there's still virtual address refers to this page, do nothing. */
    if (pp->pp_ref > 0)
        return;

    /* Step 2: If the `pp_ref` reaches to 0, mark this page as free and return. */
    if (pp->pp_ref == 0) {
        LIST_INSERT_HEAD(&page_free_list, pp, pp_link);
        return;
    }

    /* If the value of `pp_ref` less than 0, some error must occurred before,
     * so PANIC !!! */
    panic("cgh:pp->pp_ref is less than zero\n");
}

/*Overview:
 	Given `pgdir`, a pointer to a page directory, pgdir_walk returns a pointer
 	to the page table entry (with permission PTE_R|PTE_V) for virtual address 'va'.

  Pre-Condition:
	The `pgdir` should be two-level page table structure.

  Post-Condition:
 	If we're out of memory, return -E_NO_MEM.
	Else, we get the page table entry successfully, store the value of page table
	entry to *ppte, and return 0, indicating success.

  Hint:
	We use a two-level pointer to store page table entry and return a state code to indicate
	whether this function execute successfully or not.
    This function have something in common with function `boot_pgdir_walk`.*/
int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
    Pde *pgdir_entryp;
    Pte *pgtable;
    struct Page *ppage;

    /* Step 1: Get the corresponding page directory entry and page table. */
    pgdir_entryp = (Pde *) (&pgdir[PDX(va)]);
    *ppte = 0;

    /* Step 2: If the corresponding page table is not exist(valid) and parameter `create`
     * is set, create one. And set the correct permission bits for this new page
     * table.
     * When creating new page table, maybe out of memory. */
    if (*pgdir_entryp & PTE_V) {
        pgtable = KADDR(PTE_ADDR(*pgdir_entryp));
    } else {
        if (!create) return 0;

        if (page_alloc(&ppage) < 0) {
            return -E_NO_MEM;
        }
        ppage->pp_ref = 1;
        pgtable = (Pte *) page2kva(ppage);
        *pgdir_entryp = page2pa(ppage) | PTE_R | PTE_V;
    }

    /* Step 3: Set the page table entry to `*ppte` as return value. */
    *ppte = (Pte *) (&pgtable[PTX(va)]);

    return 0;
}

/*Overview:
 	Map the physical page 'pp' at virtual address 'va'.
 	The permissions (the low 12 bits) of the page table entry should be set to 'perm|PTE_V'.

  Post-Condition:
    Return 0 on success
    Return -E_NO_MEM, if page table couldn't be allocated

  Hint:
	If there is already a page mapped at `va`, call page_remove() to release this mapping.
	The `pp_ref` should be incremented if the insertion succeeds.*/
int page_insert(Pde *pgdir, struct Page *pp, u_long va, u_int perm) {
    u_int PERM;
    Pte *pgtable_entry;
    PERM = perm | PTE_V;

    pgdir_walk(pgdir, va, 0, &pgtable_entry);

    if (pgtable_entry != 0 && (*pgtable_entry & PTE_V) != 0) {
        if (pa2page(*pgtable_entry) != pp) {
            page_remove(pgdir, va);
        } else {
            tlb_invalidate(pgdir, va);
            *pgtable_entry = (page2pa(pp) | PERM);
            return 0;
        }
    }
    tlb_invalidate(pgdir, va);
    if (pgdir_walk(pgdir, va, 1, &pgtable_entry) != 0) {
        return -E_NO_MEM;    // panic ("page insert failed .\n");
    }
    *pgtable_entry = (page2pa(pp) | PERM);
    pp->pp_ref++;
    return 0;
}

/*Overview:
	Look up the Page that virtual address `va` map to.

  Post-Condition:
	Return a pointer to corresponding Page, and store it's page table entry to *ppte.
	If `va` doesn't mapped to any Page, return NULL.*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
    struct Page *ppage;
    Pte *pte;
    pgdir_walk(pgdir, va, 0, &pte);
    if (pte == 0) {
        return 0;
    }
    if ((*pte & PTE_V) == 0) {
        return 0;    //the page is not in memory.
    }
    ppage = pa2page(*pte);
    if (ppte) {
        *ppte = pte;
    }
    return ppage;
}

// Overview:
// 	Decrease the `pp_ref` value of Page `*pp`, if `pp_ref` reaches to 0, free this page.
void page_decref(struct Page *pp) {
    if (--pp->pp_ref == 0) {
        page_free(pp);
    }
}

// Overview:
// 	Unmaps the physical page at virtual address `va`.
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
    return;
}

// Overview:
// 	Update TLB.
void
tlb_invalidate(Pde *pgdir, u_long va) {
    // TODO: Implement tlb_invalidate
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

void bzero(void *b, size_t len) {
    void *max;
    max = b + len;
    while (b + 3 < max){
        *(int *)b = 0;
        b+=4;
    }
    while (b < max){
        *(char *)b++ = 0;
    }
}