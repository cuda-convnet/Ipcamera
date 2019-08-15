include  makefile.rule

all:	
	@cd fdiskdll; make
	
ifeq ($(SDK_CHIP),hi3519v101)
	
	@cd avicodelib/goke; make 
	
endif


ifeq ($(SDK_CHIP),hi3516a)
	
	@cd avicodelib/3516; make 
	
endif
	
	@cd avicodelib; make 
	@cd filesystem; make 
		
clean:
	@cd fdiskdll; make clean
	

ifeq ($(SDK_CHIP), hi3519v101)
	
	@cd avicodelib/goke; make clean 
	
endif


ifeq ($(SDK_CHIP), hi3516a)
	
	@cd avicodelib/3516; make clean 
	
endif
	@cd avicodelib; make clean
	@cd filesystem; make clean
	@rm -f ../lib/libfilectrl.so ../lib/libdrvctrl.so ../lib/libfdisk.so ../lib/libfilesystem.so
