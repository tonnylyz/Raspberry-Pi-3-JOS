drivers_dir   := drivers
boot_dir      := boot
init_dir      := init
lib_dir       := lib
vmlinux_elf   := kernel.elf
vmlinux_img   := kernel7.img

link_script   := kernel.lds

modules       := boot init
objects       := $(boot_dir)/start.o \
                 $(init_dir)/asm.o \
                 $(init_dir)/main.o

.PHONY: all $(modules) clean

all: $(modules) vmlinux

vmlinux: $(modules)
	$(CROSS)ld -o $(vmlinux_elf) -e _start -T$(link_script) $(objects)
	$(CROSS)objcopy $(vmlinux_elf) -O binary $(vmlinux_img)

$(modules):
	$(MAKE) --directory=$@

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(vmlinux_elf) $(vmlinux_img)

include include.mk
