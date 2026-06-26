PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CFLAGS ?= -Wall -Wextra -pedantic -O2
LDLIBS ?=

tempcap: tempcap.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tempcap.o: tempcap.c tempcap.h
	$(CC) $(CFLAGS) -c -o $@ $<

install: tempcap
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 tempcap $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/tempcap

clean:
	rm -f tempcap tempcap.o

.PHONY: install uninstall clean
