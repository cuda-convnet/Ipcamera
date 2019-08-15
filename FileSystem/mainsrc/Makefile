
include ../makefile.rule

INCLUDE += -I../../include
INCLUDE+=-I../avicodelib
INCLUDE+=-I../filesystem
INCLUDE += -I../common

LIBDIR=../../lib

EXTER_LIB_DIR=../../extern_lib/lib


USE_FILE = main.c

EXEFILE = Hisi_9x

//LIBFILE = -lpthread -lstdc++
LIBFILE = -lpthread -lm -lstdc++
LIBFILE += -L$(LIBDIR) -L$(EXTER_LIB_DIR) -lfilesystem -L$(LIBDIR) -ldrvctrl $(LIBDIR)/fdisk.a   
LIBFILE += $(EXTER_LIB_DIR)/libscene_auto.a $(MPI_LIBS) $(AUDIO_LIBA)  $(SENSOR_LIBS)
#LIBFILE +=  $(EXTER_LIB_DIR)/libselfrtsp.a $(EXTER_LIB_DIR)/libliveMedia.a $(EXTER_LIB_DIR)/libgroupsock.a  $(EXTER_LIB_DIR)/libBasicUsageEnvironment.a  $(EXTER_LIB_DIR)/libUsageEnvironment.a  
LIBFILE +=  $(EXTER_LIB_DIR)/librtsp.a  -lyuv

#all: $(EXEFILE)
#$(EXEFILE) : $(USE_FILE)

all:
	$(CC)  -o $(EXEFILE) $(USE_FILE) $(LIBFILE) $(INCLUDE) $(LIBS)  -O0 -rdynamic  -ldl 
	cp $(EXEFILE) /nfs/yb

clean:
	rm -f  $(EXEFILE)

