include ../makefile.rule


MACRO_FLAG =  -DCHANNEL_8_MAC -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D_GNU_SOURCE



INCLUDE += -I../../include
LIBDIR=../../lib


SRCEXE = test.c 
SRCLIB = FSfileoperate.c FSdiskoperate.c  FSfileinfoop.c FSrecordop.c FSalarmop.c  FSdiskinfoop.c  FSpreviouview.c FSfilecreate.c \
	FSEventInfo.c FTCsaveplayctrl.c FTCfileplay.c FTCfilerec.c FTCCustomctrl.c IOFile.c

OBJFILE = FSfileoperate.o FSdiskoperate.o FSfileinfoop.o FSrecordop.o FSalarmop.o  FSdiskinfoop.o  FSpreviouview.o FSfilecreate.o \
	FSEventInfo.o FTCsaveplayctrl.o FTCfileplay.o FTCfilerec.o FTCCustomctrl.o IOFile.o

SHARELIB = libfilesystem.so 
STATICLIB = libfilesystem.a



all :
	$(CC) $(CFLAGS)  $(MPP_CFLAGS)  $(MACRO_FLAG) -o $(SHARELIB) $(SRCLIB)  -fPIC -shared $(INCLUDE)

ifeq ($(HIGDB), n)	
	$(STRIP) $(SHARELIB)
endif
	
	#$(CC) $(CFLAGS)  $(MPP_CFLAGS) $(MACRO_FLAG) -c  $(SRCLIB)  $(INCLUDE) 
	
	#$(AR) -r $(STATICLIB) $(OBJFILE) 
	
	@cp $(SHARELIB)  $(LIBDIR)

clean:
	rm  -rf $(SHARELIB) $(STATICLIB) *.o
