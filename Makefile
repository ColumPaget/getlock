CC = gcc
VERSION = @VERSION@
CFLAGS = -g -O2
LIBS = 
FLAGS=$(CFLAGS) -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1
INSTALL=/bin/install -c
prefix=/usr/local
bindir=$(prefix)${exec_prefix}/bin

all: 
	@cd libUseful-4; $(MAKE)
	$(CC) $(FLAGS) $(LIBS) -g -ogetlock main.c libUseful-4/libUseful-4.a

clean:
	@rm -f getlock libUseful-4/*.o libUseful-4/*.a libUseful-4/*.so

install:
	$(INSTALL) getlock $(bindir)

test:
	-echo "no tests"
