CC = gcc
VERSION = 0.1
CFLAGS = -g -O2
LIBS = 
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin

all: 
	@cd libUseful-2.0; $(MAKE)
	$(CC) $(FLAGS) $(LIBS) -g -ogetlock main.c libUseful-2.0/libUseful-2.0.a

clean:
	@rm -f getlock libUseful-2.0/*.o libUseful-2.0/*.a libUseful-2.0/*.so

install:
	$(INSTALL) getlock $(bindir)
