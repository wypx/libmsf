#!/bin/sh
#################################
# Makefile for libmsf.so
#  by luotang
#  2018.1.7
#  project github: 
#################################
ROOTDIR = ../../..
include $(ROOTDIR)/Rules.make

TMP_PATH =lib
TARGET	 =$(LIBPATH)/libmsf.so

IFLAGS	 +=-I inc -I $(ROOTDIR)/inc 
		 
CFLAGS	 +=-fPIC -Wall -Wextra -W -O2 -pipe -Wswitch-default -Wpointer-arith -Wno-unused  -ansi -ftree-vectorize -std=gnu11 #-std=c11
DFLAGS	 +=-D_GNU_SOURCE

LDFLAGS	 +=-shared 
GFLAG 	 += $(IFLAGS) $(DFLAGS) $(CFLAGS)

SRCS = $(wildcard  src/*.c)
OBJS = $(SRCS:%.c=$(TMP_PATH)/%.o)

.PHONY:clean

all:$(TARGET)

$(TARGET): $(OBJS)
	@$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@echo $(STRIP) $@
	@$(STRIP)  -x -R .note -R .comment $@
	@cp -f $@ /lib
	@rm -rf $(TMP_PATH)
	@echo "====Makefile GFLAG==="
	@echo $(GFLAG)
	@echo "======================"
$(TMP_PATH)/%.o:%.c
	@mkdir -p $(TMP_PATH)
	@mkdir -p $(dir $@)
	@echo $(CC) $@
	@$(CC) $(GFLAG) -c $< -o $@

clean :
	rm -rf $(TMP_PATH)
