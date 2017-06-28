ENTRY(_start)
SECTIONS
{
    . = 0x00000000;
	.text : {
		*(.text*)
	}
	.init_array : {
		*(.init_array*)
	}
  	.rodata : {
  		*(.rodata*)
  	}
	.data : {
		*(.data*)
	}
    .bss : {
        *(.bss*)
    }
}
