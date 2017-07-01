INCLUDES = -I../include/ -I./

USERLIB := printf.o \
		print.o \
		libos.o \
		fork.o \
		pgfault.o \
		syscall_lib.o \
		ipc.o \
		string.o

CROSS  := /home/tonny/linaro-elf/bin/aarch64-elf-
CFLAGS := -Wall -ffreestanding
CC     := $(CROSS)gcc
LD     := $(CROSS)ld

CFLAGS += -nostdlib -static

all: fktest.x fktest.b pingpong.x pingpong.b $(USERLIB) entry.o

test: pitesta.o pitestb.o syscall_lib.o string.o print.o printf.o
	$(LD) -o pitesta.elf -e umain pitesta.o syscall_lib.o string.o print.o printf.o
	$(LD) -o pitestb.elf -e umain pitestb.o syscall_lib.o string.o print.o printf.o

%.x: %.b.c
	echo cc1 $<
	$(CC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	echo create $@
	echo bintoc $* $< > $@~
	chmod +x ./bintoc
	./bintoc $* $< > $@~ && mv -f $@~ $@
	
%.b: entry.o %.o $(USERLIB)
	echo ld $@
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib $^

%.o: %.c
	echo user1 cc $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	echo as $<
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: lib.h

.PHONY: clean

clean:
	rm -rf *~ *.o *.b.c *.x *.b

