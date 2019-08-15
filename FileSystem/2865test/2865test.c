#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <asm/page.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include "tw2865.h"


#define TW2865_FILE  "/dev/misc/tw2865dev"

int main(int argc, char *argv[])
{

	int fd ;
	unsigned char  addr;
	unsigned char  value;
	registerrw_t RegisterReadWrite;
	int chipno = 0;

	
	fd = open(TW2865_FILE,O_RDWR);
	if(fd <0)
	{
		printf("can't open tw2865\n");
		return -1;
	}

	if (strcmp(argv[1], "r") == 0) 
	{
		addr = atoi(argv[2]);
		//value = atoi(argv[3]);

		printf("r %d  %d\n");

		RegisterReadWrite.addr = (unsigned char)addr;
		RegisterReadWrite.ChipNo = chipno;
		RegisterReadWrite.value = 0;

		

		if(0!=ioctl(fd,TW2865CMD_READ_REG,&RegisterReadWrite))
		{
			printf("2815 errno %d\r\n",errno);			
		}

		printf("read addr:%x value:%x\n",addr,RegisterReadWrite.value);
	}


	if (strcmp(argv[1], "w") == 0) 
	{
		addr = atoi(argv[2]);
		value = atoi(argv[3]);

		RegisterReadWrite.addr = (unsigned char)addr;
		RegisterReadWrite.ChipNo = chipno;
		RegisterReadWrite.value = value;

		

		if(0!=ioctl(fd,TW2865CMD_WRITE_REG,&RegisterReadWrite))
		{
			printf("2815 errno %d\r\n",errno);			
		}

		printf("write addr:%x value:%x\n",addr,RegisterReadWrite.value);
	}



	if( fd)
		close(fd);


	 chipno = 1;

	fd = open(TW2865_FILE,O_RDWR);
	if(fd <0)
	{
		printf("can't open tw2865\n");
		return -1;
	}

	if (strcmp(argv[1], "r") == 0) 
	{
		addr = atoi(argv[2]);
		//value = atoi(argv[3]);

		printf("r %d  %d\n");

		RegisterReadWrite.addr = (unsigned char)addr;
		RegisterReadWrite.ChipNo = chipno;
		RegisterReadWrite.value = 0;

		

		if(0!=ioctl(fd,TW2865CMD_READ_REG,&RegisterReadWrite))
		{
			printf("2815 errno %d\r\n",errno);			
		}

		printf("read addr:%x value:%x\n",addr,RegisterReadWrite.value);
	}


	if (strcmp(argv[1], "w") == 0) 
	{
		addr = atoi(argv[2]);
		value = atoi(argv[3]);

		RegisterReadWrite.addr = (unsigned char)addr;
		RegisterReadWrite.ChipNo = chipno;
		RegisterReadWrite.value = value;

		

		if(0!=ioctl(fd,TW2865CMD_WRITE_REG,&RegisterReadWrite))
		{
			printf("2815 errno %d\r\n",errno);			
		}

		printf("write addr:%x value:%x\n",addr,RegisterReadWrite.value);
	}



	if( fd)
		close(fd);
	
	
}


