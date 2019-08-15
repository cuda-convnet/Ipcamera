

include ../../makefile.rule

INC_PATH += -I../../../include
INC_PATH += -I../../common
INC_PATH += -I./inc
INC_PATH += -I../
INC_PATH += -I./ctlserver


INC_PATH += -I../../../../sunriselib/include

MACRO_FLAG = -DCHANNEL_8_MAC

CFLAGS +=   $(INC_PATH) $(MACRO_FLAG)  -L../../../extern_lib/lib/ -lyuv 

LIBDIR=../../lib

SRCS    :=  $(wildcard *.c)
OBJS    :=  $(SRCS:%.c=%.o)

SRCS_PLUS    :=  $(wildcard *.cpp)
OBJS_PLUS    :=  $(SRCS_PLUS:%.cpp=%.o)

OBJS += $(OBJS_PLUS)

Buid_objs  := $(OBJS)
Buid_objs += $(OBJS_PLUS)


all:$(OBJS)	

%.o:%.c 
	$(CC)   $(CFLAGS)  $(MPP_CFLAGS)  -c $< -o $@ 		

%.o:%.cpp 
	$(C++)   $(CFLAGS)  $(MPP_CFLAGS)  -c $< -o $@ 	
	
clean:	
	@rm -f $(OBJS)

.PHONY : clean all
