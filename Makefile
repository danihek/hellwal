VERSION := $(shell cat VERSION)

CC       := gcc
CFLAGS   := -Wall -Wextra -O3
DEBUGFLAGS := -Wall -Wextra -ggdb
LDFLAGS  := -lm

PREFIX   ?= /usr/local
BINDIR   := $(PREFIX)/bin

TARGET   := hellwal

all: $(TARGET)

$(TARGET): hellwal.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) -DVERSION=\"$(VERSION)\"

debug: hellwal.c
	$(CC) $(DEBUGFLAGS) $< -o $(TARGET) $(LDFLAGS) -DVERSION=\"$(VERSION)\"

clean:
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f $(TARGET) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

release: $(TARGET)
	tar czf $(TARGET)-v$(VERSION).tar.gz $(TARGET)

.PHONY: all debug clean install uninstall release
