INCLUDES := -I./ -I../ -I../include/
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: clean

all: utility.o print.o printf.o kclock.o env.o kernel_elfloader.o sched.o syscall_all.o

clean:
	rm -rf *~ *.o

include ../include.mk
