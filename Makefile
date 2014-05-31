VERSION	= 1.11.1

all: build

-include config.mk
include scripts/lib.mk

CFLAGS	+= -g -I. $(XFT_CFLAGS) -DVERSION='"$(VERSION)"' -DDATADIR='"$(datadir)"'

objs	:= file.o main.o opt.o pager.o sconf.o x.o xmalloc.o

netwmpager: $(objs)
	$(call cmd,ld,$(XFT_LIBS))

clean		+= *.o netwmpager .install.log build-stamp debian/files debian/netwmpager* doc/netwmpager.1.gz
distclean	+= config.mk

build: netwmpager doc/netwmpager.1.gz

doc/netwmpager.1.gz: doc/netwmpager.1
	gzip --keep doc/netwmpager.1

install: build
	$(INSTALL) -d $(prefix)/$(bindir)
	$(INSTALL) -m755 $(prefix)/$(bindir) netwmpager
	$(INSTALL) -d $(prefix)/$(datadir)
	$(INSTALL) -m644 $(prefix)/$(datadir)/netwmpager config-example
	$(INSTALL) -d $(prefix)/$(mandir)
	$(INSTALL) -m644 $(prefix)/$(mandir)/man1 doc/netwmpager.1.gz

tags:
	exuberant-ctags *.[ch]

REV     = $(shell git-rev-parse HEAD)
RELEASE	= netwmpager-$(REV)
TARBALL	= $(RELEASE).tar.bz2

release:
	git-tar-tree $(REV) $(RELEASE) | bzip2 -9 > $(TARBALL)

.PHONY: all build install release

main.o: Makefile config.mk
pager.o x.o: config.mk
