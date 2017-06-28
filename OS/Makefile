drivers_dir   := drivers
boot_dir      := boot
init_dir      := init
lib_dir       := lib
mm_dir        := mm
vmlinux_elf   := kernel.elf
vmlinux_img   := kernel8.img

link_script   := kernel.lds

modules       := boot drivers init lib mm
objects       := $(boot_dir)/*.o \
                 $(init_dir)/*.o \
                 $(drivers_dir)/*.o \
                 $(lib_dir)/*.o \
                 $(mm_dir)/pmap.o

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
	rm -rf *.o *~ $(vmlinux_elf)

include include.mk
