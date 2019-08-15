
# Makefile -- Makefile for util-linux Linux utilities
# Created: Sat Dec 26 20:09:40 1992
# Revised: Fri Oct  6 21:02:21 1995 by r.faith@ieee.org
# Copyright 1992, 1993, 1994, 1995 Rickard E. Faith (faith@cs.unc.edu)
#
PREFIX  := $(shell /bin/pwd | sed -e 's/initrd.*/initrd\/stage/')

include ../makefile.rule


INCLUDE+=-I../../include

#LINUX_ROOT=-I/opt/Hi3531_SDK_V1.0.7.1/osdrv/kernel/linux-3.0.y
LINUX_INC=-I$(LINUX_ROOT)/include

MFLAG = -Os -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D_GNU_SOURCE -DHAVE_LLSEEK -DHAS_LONG_LONG -D__ia64__

#CC = arm-hisiv100nptl-linux-gcc
#AR= arm-hisiv100nptl-linux-ar

EXE_APP=myfdisk
SRCEXE = test.c 
SRCLIB = fdisk.c fdiskaixlabel.c fdiskbsdlabel.c fdisksgilabel.c fdisksunlabel.c \
i386_sys_types.c  llseek.c mkdosfs.c partitiontype.c partname.c 

SRCOBJ = fdisk.o fdiskaixlabel.o fdiskbsdlabel.o fdisksgilabel.o fdisksunlabel.o \
i386_sys_types.o  llseek.o mkdosfs.o partitiontype.o partname.o 

my_objects  :=   fdisk.a

all :  
	$(CC)  -c  $(SRCLIB) $(INCLUDE) $(LINUX_INC) $(MFLAG)
	$(AR)  -r  fdisk.a $(SRCOBJ)
	$(CC)  -o $(EXE_APP) $(SRCEXE) $(INCLUDE) $(LINUX_INC) $(MFLAG) fdisk.a

	cp fdisk.a ../../lib
	cp fdisk.a ../filesystem


clean:
	rm -f *.so test *.o *.a
