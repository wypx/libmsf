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

IFLAGS 		= -I./inc -I $(ROOTDIR)/inc -I../msf_thirdparty/cJSON -I../msf/inc 
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