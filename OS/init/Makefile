INCLUDES := -I../include -I../ -I../drivers/include

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: clean

all: main.o

clean:
	rm -rf *~ *.o

include ../include.mk
