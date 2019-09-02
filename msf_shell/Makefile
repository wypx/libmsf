#
# Makefile for msf_shell process 
# by luotang 2018.11.15
#

ROOTDIR = ../../..
include $(ROOTDIR)/Rules.make

TMP_PATH= bin
TARGET	= $(BINPATH)/msf_shell

SRCS 	= $(wildcard src/*.c)
OBJS	= $(SRCS:%.c=$(TMP_PATH)/%.o)
cJSON_SRCS = ../msf_thirdparty/cJSON/cJSON.c
cJSON_OBJS = $(TMP_PATH)/src/cJSON.o

IFLAGS      += -I./inc -I $(ROOTDIR)/inc
IFLAGS	    += -I ../msf_thirdparty/cJSON
IFLAGS	    += -I ../msf/inc
IFLAGS	    += -I ../msf/inc/base
IFLAGS	    += -I ../msf/inc/log
IFLAGS	    += -I ../msf/inc/system 
IFLAGS	    += -I ../msf/inc/timer 
IFLAGS	    += -I ../msf/inc/event 
IFLAGS	    += -I ../msf/inc/mem 
IFLAGS	    += -I ../msf/inc/io  
IFLAGS	    += -I ../msf/inc/thread  
IFLAGS	    += -I ../msf/inc/encrypt  
IFLAGS	    += -I ../msf/inc/svc  
IFLAGS	    += -I ../msf/inc/network
CFLAGS 		= -Wall -W -O2 -Wswitch-default -Wpointer-arith -Wno-unused -g
DFLAGS 		= -D_GNU_SOURCE 
LD_FLAGS 	= -ldl -lpthread -lnuma -L$(LIBPATH) -lmsf

GFLAG 		= $(CFLAGS) $(DFLAGS) $(IFLAGS)

.PHONY: all clean

all : $(TARGET)

$(TARGET): $(cJSON_OBJS) $(OBJS)
	@echo $(LINK) $@
	@$(LINK) -o $@ $^ $(LD_FLAGS) 
	@echo $(STRIP) $@
	@$(STRIP) -x -R .note -R .comment $@
	@rm -rf $(TMP_PATH)
	@echo "====Makefile GFLAG==="
	@echo $(GFLAG)
	@echo "======================"
$(TMP_PATH)/%.o:%.c
	@mkdir -p $(BINPATH)
	@mkdir -p $(dir $@)
	@echo $(CC) $@
	@$(CC) $(GFLAG) -c $^ -o $@

$(cJSON_OBJS):$(cJSON_SRCS)
	@mkdir -p $(TMP_PATH)/src
	@echo $(CC) $@
	@$(CC) $(GFLAG) -c $^ -o $@

clean :
	rm -rf $(TMP_PATH)