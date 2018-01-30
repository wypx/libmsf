#!/bin/sh
#################################
# Makefile for libplugin.so
#  by luotang
#  2018.1.7
#  project github: 
#################################
ROOTDIR = ../..
include $(ROOTDIR)/Rules.make
CROSS_COMPILE = $(TOOL_PREFIX)
CHAIN_HOST  ?= $(patsubst %-,%,$(TOOL_PREFIX))

BIN		 =bin
LIB      =lib
TARGET	 =$(LIB)/libplugin.so 

IFLAGS	 +=-I inc -I $(ROOTDIR)/inc -I../libipc/src/libgbase/inc
		 
CFLAGS	 +=-fPIC -Wall -Wextra -W -O2 -pipe -Wswitch-default -Wpointer-arith -Wno-unused  -ansi -ftree-vectorize -std=gnu11 #-std=c11
DFLAGS	 +=-D_GNU_SOURCE

LDFLAGS	 +=-shared 
GFLAG 	 += $(IFLAGS) $(DFLAGS) $(CFLAGS)

SRCS = $(wildcard  src/*.c)
OBJS = $(SRCS:%.c=$(LIB)/%.o)

.PHONY:clean

all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	$(STRIP)  -x -R .note -R .comment $@
	rm -rf $(LIB)/src
	@echo "====Makefile GFLAG==="
	@echo $(GFLAG)
	@echo "======================"
$(LIB)/%.o:%.c
	@mkdir -p $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(GFLAG) -c $< -o $@

plugins:
	$(CC) $(IFLAGS) -fPIC -shared doc/plugin_stun.c -o ./lib/plugin_stun.so -L../libipc/lib -lgbase
	$(CC) $(IFLAGS) -fPIC -shared doc/plugin_p2p.c -o ./lib/plugin_p2p.so -L../libipc/lib -lgbase
bin:
	$(CC) $(IFLAGS) -rdynamic -o lib/test doc/test.c -ldl -L./lib -lplugin

test:all plugins bin
	#cd lib && ./plug_test
clean :
	rm -rf $(LIB)/*
