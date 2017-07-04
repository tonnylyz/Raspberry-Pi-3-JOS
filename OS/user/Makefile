INCLUDES = -I../include/ -I./

USERLIB := printf.o \
		print.o \
		libos.o \
		fork.o \
		syscall.o \
		syscall_lib.o \
		ipc.o \
		string.o \
		fd.o \
		file.o \
		fs.o \
		fsipc.o \
		pageref.o

CROSS  := /home/tonny/linaro/bin/aarch64-none-elf-
CFLAGS := -Wall -ffreestanding
CC     := $(CROSS)gcc
LD     := $(CROSS)ld

all: lab5

lab5: serv.elf fstest.elf $(USERLIB) entry.o

lab4: fktest.elf pingpong.elf $(USERLIB) entry.o

lab3: pitesta.o pitestb.o pitestc.o syscall.o syscall_lib.o string.o print.o printf.o
	$(LD) -o pitesta.elf -e umain pitesta.o syscall.o syscall_lib.o string.o print.o printf.o
	$(LD) -o pitestb.elf -e umain pitestb.o syscall.o syscall_lib.o string.o print.o printf.o
	$(LD) -o pitestc.elf -e umain pitestc.o syscall.o syscall_lib.o string.o print.o printf.o

%.elf: entry.o %.o $(USERLIB)
	echo ld $@
	$(LD) -o $@ -e _start $^

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	echo as $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: lib.h

.PHONY: clean

clean:
	rm -rf *~ *.o *.b.c *.x *.b *.elf

