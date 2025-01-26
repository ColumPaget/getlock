CC=gcc
VERSION=@VERSION@
CFLAGS=-g -O2
LIBS=-lz -lcrypto -lssl -lssl -lUseful-5 
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -D_FILE_OFFSET_BITS=64 -DHAVE_PRCTL=1 -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_PRCTL_H=1 -DHAVE_LIBUSEFUL_5_LIBUSEFUL_H=1 -DHAVE_LIBUSEFUL5=1 -DHAVE_LIBUSEFUL_5=1 -DHAVE_LIBSSL=1 -DHAVE_LIBSSL=1 -DHAVE_LIBCRYPTO=1 -DHAVE_LIBZ=1
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin
mandir=$(prefix)${prefix}/share/man

all: 
	$(CC) $(FLAGS) -g -ogetlock main.c  $(LIBS) 

libUseful-5/libUseful.a:
	$(MAKE) -C libUseful-5

clean:
	@rm -f getlock libUseful-5/*.o libUseful-5/*.a libUseful-5/*.so libUseful-5/*.so.*



install:
	mkdir -p $(bindir)
	$(INSTALL) getlock $(bindir)
	mkdir -p $(mandir)
	$(INSTALL) getlock.1 $(mandir)

test:
	-echo "no tests"
