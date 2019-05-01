#############################
# Makefile for libjpg lib
# by luotang
# 2017.4.26
#############################
ROOTDIR = ../../../..
include $(ROOTDIR)/Rules.make

SUBDIRS = numactrl

INCPATH	=$(ROOTDIR)/inc
LIBPATH	=$(ROOTDIR)/lib

all: make_libnuma

make_libnuma:
	cd $(SUBDIRS)  && chmod +x ./configure
	cd $(SUBDIRS)  && \
	./configure --prefix="$(PWD)/lib" --host=$(CHAIN_HOST) CFLAGS="-fPIC" \
            --disable-FEATURE --enable-static --enable-shared && \
	make && \
	make install
	$(STRIP) -x -R .note -R .comment ./lib/lib/libnuma.a
	cp -rf ./lib/lib/* $(TOOL_LIB_DIR)
	cp -rf ./lib/include/* $(TOOL_INC_DIR)
clean:
	cd $(SUBDIRS)  && make clean
	#rm -f Makefile
