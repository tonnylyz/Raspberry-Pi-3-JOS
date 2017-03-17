CROSS_COMPILE := /home/tonny/linaro/bin/aarch64-elf-
CC			  := $(CROSS_COMPILE)gcc
CFLAGS		  := -Wall -O -ffreestanding
LD			  := $(CROSS_COMPILE)ld
