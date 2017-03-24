SECTIONS
{
    . = 0x00080000;
    .text : { *(.text*) }
    .bss  : { *(.bss*)  }
}
