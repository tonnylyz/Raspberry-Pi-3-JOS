FSIMGFILES := motd newmotd

all: fs.img

fs.img: $(FSIMGFILES)
	dd if=/dev/zero of=fs.img bs=4096 count=1024 2>/dev/null
	chmod +x ./fsformat
	./fsformat fs.img $(FSIMGFILES)

.PHONY: clean

clean:
	rm -rf fs.img

include ../include.mk
