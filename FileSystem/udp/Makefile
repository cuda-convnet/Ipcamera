export CROSS:=arm-none-linux-gnueabi-
export CC:=$(CROSS)gcc
export AR:=$(CROSS)ar
export STRIP:=$(CROSS)strip

LINUX_SRC=/usr/src/arm-linux-2.6.28/linux-2.6.28-fa
LINUX_INC=$(LINUX_SRC)/include

INC_PATH = -I/$(LINUX_SRC)/../module/include/dvr
INC_PATH +=-I/usr/src/arm-linux-2.6.28/module/include/vcap200_v3
INC_PATH +=-I/usr/src/arm-linux-2.6.28/module/include/security
INC_PATH +=-I/usr/src/arm-linux-2.6.28/module/include/ahb_dma
INC_PATH += -I../inc
INC_PATH +=-I/usr/src/arm-linux-2.6.28/module/include/isp


#USE_FILE = udptest.c udpop.c
#EXEFILE = udptest
USE_FILE = udptest.c audioctrl.c
EXEFILE = audio_test
CFLAGS +=  -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES $(INC_PATH)

#all: $(EXEFILE)
#$(EXEFILE) : $(USE_FILE)

all:
	$(CC)  $(CFLAGS) -o $(EXEFILE) $(USE_FILE) 
	cp $(EXEFILE) /nfs/yb

clean:
	rm -f  $(EXEFILE)

