VERSION	= 1.11

all: build

-include config.mk
include scripts/lib.mk

CFLAGS	+= -g -I. $(XFT_CFLAGS) -DVERSION='"$(VERSION)"' -DDATADIR='"$(datadir)"'

objs	:= file.o main.o opt.o pager.o sconf.o x.o xmalloc.o

netwmpager: $(objs)
	$(call cmd,ld,$(XFT_LIBS))

clean		+= *.o netwmpager .install.log build-stamp debian/files debian/netwmpager*
distclean	+= config.mk

build: netwmpager

install: build
	$(INSTALL) -m755 $(prefix)/$(bindir) netwmpager
	$(INSTALL) -m644 $(prefix)/$(datadir)/netwmpager config-example

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
