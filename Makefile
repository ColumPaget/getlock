CC = gcc
VERSION = @VERSION@
CFLAGS = -g -O2
LIBS = 
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -D_FILE_OFFSET_BITS=64 -DHAVE_LIBUSEFUL5=1
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin

all: libUseful-5/libUseful.a
	$(CC) $(FLAGS) -g -ogetlock main.c $(LIBS) libUseful-5/libUseful.a

libUseful-5/libUseful.a:
	$(MAKE) -C libUseful-5

clean:
	@rm -f getlock libUseful-5/*.o libUseful-5/*.a libUseful-5/*.so

install:
	$(INSTALL) getlock $(bindir)

test:
	-echo "no tests"
