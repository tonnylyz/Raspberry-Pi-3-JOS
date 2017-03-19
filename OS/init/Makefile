INCLUDES := -I../include

main.o: asm.o
	$(CROSS)gcc $(CFLAGS) $(INCLUDES) -c main.c -o main.o

asm.o:
	$(CROSS)as asm.S -o asm.o

.PHONY: clean

all: main.o

clean:
	rm -rf *~ *.o
include ../include.mk
