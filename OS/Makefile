drivers_dir	  := drivers
boot_dir	  := boot
init_dir	  := init
lib_dir		  := lib
vmlinux_elf	  := kernel7.img

link_script   := kernel.lds

modules		  := boot init
objects		  := $(boot_dir)/start.o \
				 $(init_dir)/main.o

.PHONY: all $(modules) clean

all: $(modules) vmlinux

vmlinux: $(modules)
	$(LD) -o $(vmlinux_elf) -T $(link_script) $(objects)

$(modules):
	$(MAKE) --directory=$@

clean:
	for d in $(modules);	\
		do					\
			$(MAKE) --directory=$$d clean; \
		done; \
	rm -rf *.o *~ $(vmlinux_elf)

include include.mk
