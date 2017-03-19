SECTIONS
{
    . = 0x00080000;
    _bootstrap : {
        boot/start.o
        init/asm.o
        init/main.o
        *(.text*)
    }
    /*
    Reserved for vm map
    
          //1234567812345678
    .text 0xFFFFFFC000000000 + SIZEOF(_bootstrap) : {
        init/main.o
        *(.text*)
    }
    */
    .data   : { *(.data*)   }
    .rodata : { *(.rodata*) }
    .bss    : { *(.bss*)    }
}
