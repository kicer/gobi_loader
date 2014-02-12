VERSION = 0.7

gobi_loader: gobi_loader.c
	gcc -Wall gobi_loader.c -o gobi_loader

all: gobi_loader

install: gobi_loader
	install -D gobi_loader ${prefix}/lib/udev/gobi_loader
	install -D 60-gobi.rules ${prefix}/lib/udev/rules.d/60-gobi.rules
	mkdir -p ${prefix}/lib/firmware
	-udevadm control --reload-rules

uninstall:
	-rm $(prefix)/lib/udev/gobi_loader
	-rm $(prefix)/lib/udev/rules.d/60-gobi.rules

clean:
	-rm gobi_loader
	-rm *~

dist:
	mkdir gobi_loader-$(VERSION)
	cp gobi_loader.c README Makefile 60-gobi.rules gobi_loader-$(VERSION)
	tar zcf gobi_loader-$(VERSION).tar.gz gobi_loader-$(VERSION)
	rm -rf gobi_loader-$(VERSION)
