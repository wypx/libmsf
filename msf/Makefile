#!/bin/sh
########################################################
# Makefile for libmsf.so
#  by luotang
#  2018.1.7
#  project github: https://github.com/wypx/libmsf
########################################################
ROOTDIR = ../../..
include $(ROOTDIR)/Rules.make

TMP_PATH =lib
TARGET	 =$(LIBPATH)/libmsf.so

IFLAGS	 +=-I inc -I $(ROOTDIR)/inc 
IFLAGS	 += -I inc/base
IFLAGS	 += -I inc/log
IFLAGS	 += -I inc/system 
IFLAGS	 += -I inc/timer 
IFLAGS	 += -I inc/event 
IFLAGS	 += -I inc/mem 
IFLAGS	 += -I inc/io  
IFLAGS	 += -I inc/thread  
IFLAGS	 += -I inc/encrypt  
IFLAGS	 += -I inc/svc  
IFLAGS	 += -I inc/network

CFLAGS	 +=-fPIC -Wall -Wextra -W -O2 -pipe -Wswitch-default -Wpointer-arith -Wno-unused  -ansi -ftree-vectorize -std=gnu11 #-std=c11
DFLAGS	 +=-D_GNU_SOURCE

LDFLAGS	 +=-shared 
GFLAG 	 += $(IFLAGS) $(DFLAGS) $(CFLAGS)

SRCS = $(wildcard src/*.c src/base/*.c src/log/*.c src/system/*.c src/event/*.c src/timer/*.c src/mem/*.c src/io/*.c src/thread/*.c src/encrypt/*.c src/network/*.c src/svc/*.c)
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
	@mkdir -pv $(TMP_PATH)
	@mkdir -p $(dir $@)
	@echo $(CC) $@
	@$(CC) $(GFLAG) -c $< -o $@

clean :
	rm -rf $(TMP_PATH)
