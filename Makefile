VERSION := $(shell cat VERSION)

CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lm

DESTDIR = /usr/local/bin

hellwal: hellwal.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) hellwal.c -o hellwal $(LDFLAGS) -DVERSION=\"$(VERSION)\"

debug: hellwal.c $(wildcard src/*.h)
	$(CC) $(CFLAGS) -ggdb hellwal.c -o hellwal $(LDFLAGS) -DVERSION=\"$(VERSION)\"

clean:
	rm hellwal

install: hellwal
	mkdir -p $(DESTDIR)
	cp -f hellwal $(DESTDIR)
	chmod 755 $(DESTDIR)/hellwal # chmod u=rwx,g=rx,o=rx

uninstall:
	rm -f $(DESTDIR)/hellwal

release: hellwal
	tar czf hellwal-v$(VERSION).tar.gz hellwal

.PHONY: hellwal debug release clean install uninstall
