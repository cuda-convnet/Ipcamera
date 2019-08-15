include ../makefile.rule

USE_FILE = systemcall.c 
EXEFILE = systemcall


#all: $(EXEFILE)
#$(EXEFILE) : $(USE_FILE)

all:
	$(CC)  -o $(EXEFILE) $(USE_FILE) -lpthread
	cp $(EXEFILE) /nfs/yb

clean:
	rm -f  $(EXEFILE)

