CC=gcc
VERSION=@VERSION@
CFLAGS=-g -O2
LIBS=
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DHAVE_PRCTL=1 -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DSTDC_HEADERS=1 -DHAVE_SYS_PRCTL_H=1 -DHAVE_QUICK_EXIT=1 -DHAVE_LIBUSEFUL5=1
INSTALL=/usr/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
mandir=$(prefix)${prefix}/share/man

all: libUseful-bundled/libUseful.a
	$(CC) $(FLAGS) -g -ogetlock main.c libUseful-bundled/libUseful.a $(LIBS) 

libUseful-bundled/libUseful.a:
	$(MAKE) -C libUseful-bundled

clean:
	@rm -f getlock libUseful-bundled/*.o libUseful-bundled/*.a libUseful-bundled/*.so libUseful-bundled/*.so.*



install:
	mkdir -p $(bindir)
	$(INSTALL) getlock $(bindir)
	mkdir -p $(mandir)
	$(INSTALL) getlock.1 $(mandir)

test:
	-echo "no tests"
