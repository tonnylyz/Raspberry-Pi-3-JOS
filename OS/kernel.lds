SECTIONS
{
    . = 0x00080000;
    init : {
        boot/init.o
        boot/vm.o
    }
    _init_end = ABSOLUTE(.);
    .text 0xFFFFFF0000000000 + _init_end : AT(_init_end) {
        init/main.o
        *(.text*)
    }
    .bss  : { *(.bss*)  }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
}
