

include ../makefile.rule

INC_PATH += -I../../include
INC_PATH += -I../common
INC_PATH += -I./inc
INC_PATH += -I../../extern_lib/include
INC_PATH += -I./

MACRO_FLAG = -DCHANNEL_8_MAC


LIBDIR=../../lib

SRCS    :=  $(wildcard *.c)
OBJS    :=  $(SRCS:%.c=%.o)


ifeq ($(SDK_CHIP_ID), 0x35190101)

SRCS_PLUS    :=  $(wildcard goke/*.cpp)
OBJS_PLUS    :=  $(SRCS_PLUS:%.cpp=%.o)

SRCS_CC    :=  $(wildcard goke/*.c)
OBJS_CC    :=  $(SRCS_CC:%.c=%.o)
INC_PATH += -I./goke
endif


ifeq ($(SDK_CHIP_ID), 0x3516A100)

SRCS_PLUS    :=  $(wildcard 3516/*.cpp)
OBJS_PLUS    :=  $(SRCS_PLUS:%.cpp=%.o)

SRCS_CC    :=  $(wildcard 3516/*.c)
OBJS_CC    :=  $(SRCS_CC:%.c=%.o)
INC_PATH += -I./3516

endif


CFLAGS +=   $(INC_PATH) $(MACRO_FLAG) -L../../extern_lib/lib/

Buid_objs  := $(OBJS)
Buid_objs += $(OBJS_PLUS)
Buid_objs += $(OBJS_CC)

TARGET :=  libdrvctrl.so


all:$(OBJS)
	$(C++)   -o $(TARGET) $(Buid_objs)  -fPIC -shared -lpthread -lm
	@cp $(TARGET) $(LIBDIR)

%.o:%.c 
	$(CC)   $(CFLAGS)  $(MPP_CFLAGS)  -c $< -o $@ 		

%.o:%.cpp 
	$(C++)   $(CFLAGS)  $(MPP_CFLAGS)  -c $< -o $@ 		
	
clean:
	@rm -f $(TARGET)
	@rm -f $(OBJS)

.PHONY : clean all
