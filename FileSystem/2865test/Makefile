
include ../makefile.rule

INCLUDE += -I../../include
INCLUDE+=-I../avicodelib
INCLUDE+=-I../filesystem
INCLUDE += -I../common

LIBDIR=../../lib

USE_FILE = 2865test.c 

EXEFILE = 2865test


#all: $(EXEFILE)
#$(EXEFILE) : $(USE_FILE)

all:
	$(CC)  -o $(EXEFILE) $(USE_FILE) $(LIBFILE) $(INCLUDE)
	cp $(EXEFILE) /nfs/yb

clean:
	rm -f  $(EXEFILE)

