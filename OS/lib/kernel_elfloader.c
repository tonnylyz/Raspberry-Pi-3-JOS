#include "kerelf.h"

int is_elf_format(u_char *binary) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *) binary;

    if ((ehdr->e_ident[0] == ELFMAG0) &&
        (ehdr->e_ident[1] == ELFMAG1) &&
        (ehdr->e_ident[2] == ELFMAG2) &&
        (ehdr->e_ident[3] == ELFMAG3)) {
        return 1;
    }

    return 0;
}

int load_elf(u_char *binary, int size, u_long *entry_point, void *user_data,
             int (*map)(u_long va, u_int32_t sgsize,
                        u_char *bin, u_int32_t bin_size, void *user_data)) {
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *) binary;
    Elf64_Phdr *phdr = NULL;

    /* As a loader, we just care about segment,
     * so we just parse program headers.
     */
    u_char *ptr_ph_table = NULL;
    Elf64_Half ph_entry_count;
    Elf64_Half ph_entry_size;
    int r;

    // check whether `binary` is a ELF file.
    if ((size < 4) || !is_elf_format(binary)) {
        return -1;
    }

    ptr_ph_table = binary + ehdr->e_phoff;
    ph_entry_count = ehdr->e_phnum;
    ph_entry_size = ehdr->e_phentsize;

    while (ph_entry_count--) {
        phdr = (Elf64_Phdr *) ptr_ph_table;

        if (phdr->p_type == SHT_PROGBITS) {
            r = map(phdr->p_vaddr, phdr->p_memsz, binary + phdr->p_offset, phdr->p_filesz, user_data);

            if (r < 0) {
                return r;
            }
        }

        ptr_ph_table += ph_entry_size;
    }

    *entry_point = ehdr->e_entry;
    return 0;
}
