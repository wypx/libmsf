#############################
# Makefile for libmsf
# by luotang.me
# 2018.11.16
#############################
ROOTDIR = ../..
include $(ROOTDIR)/Rules.make

SUBDIRS = msf msf_shell msf_daemon  #msf_mobile

all:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
					echo "Make Subdirs $$subdir";\
					cd $$subdir && make all && cd ..;\
	done
clean:
	@list='$(SUBDIRS)'; for subdir in $$list; do \
					echo "Clean Subdirs $$subdir";\
					make clean -C $$subdir;\
	done
