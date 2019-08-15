#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <asm/page.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>

#include "DFile.h"


static int pagesize = 0;

int DF_Open(char * filename,char * mode)
{
	int i = 0;
	int read_and_write = 0;
	int create_file_cut = 0;
	int flag = 0;
	int fd;
	

	if( pagesize == 0 )
	{
		if(DF_PAGESIZE == getpagesize() )
		{
			pagesize = DF_PAGESIZE;
		}else
		{
			printf("pagesize is %d error!\n",getpagesize());
			return DF_FALSE;
		}
		
	}

	for( i = 0; i < strlen(mode);i++)
	{
		if( mode[i] == '+' )
			read_and_write = 1;

		if( mode[i] == 'w' )
			create_file_cut = 1;
	}

	if( (read_and_write == 0) && (create_file_cut == 0) )
	{
		printf("mode error; mode=%s\n",mode);
		return DF_FALSE;
	}

	if( create_file_cut )
	flag = flag | O_CREAT | O_TRUNC;

	if( read_and_write )
	flag = flag | O_RDWR;

	flag = flag | O_DIRECT;

	fd = open(filename,flag);


	return fd;
}

int DF_Close(int fd)
{
	return close(fd);
}

int DF_Read(int fd,void * buf,int size)
{
	return read(fd,buf,size);	
}

off_t DF_Lseek(int fildes, off_t offset, int whence)
{
	return lseek(fildes,offset,whence);	
}

int DF_Write(int fd,char * buf,int size)
{
	return write(fd,buf,size);	
}


int DF_Write2(int fd,char * buf,int size)
{
	static char * write_buf2=NULL;
	char * write_buf;
	int ret;
	int write_data_num = 0;
	off_t cur_offset = 0;
	int data_size = size;

	if( write_buf2 == NULL )
	{
		write_buf2 = malloc(DF_PAGESIZE*2);
		if(write_buf2 == NULL )
		{
			printf("malloc error!123\n");
			return -1;
		}
	}

	write_buf = (char*)((((int) write_buf2+DF_PAGESIZE-1)/DF_PAGESIZE)*DF_PAGESIZE);
	

	cur_offset = DF_Lseek(fd,0,SEEK_CUR);

	printf("%x %x %x %x\n",write_buf2,write_buf,cur_offset,&size);

	if( cur_offset % DF_PAGESIZE ==  0 )
	{
		if( size % DF_PAGESIZE != 0 )
		{
			write_data_num = (size / DF_PAGESIZE) * DF_PAGESIZE ;		
			
			memset(write_buf,0x00,DF_PAGESIZE);			
			memcpy(write_buf,buf,size % DF_PAGESIZE);
			
			
		}else
		{
			write_data_num = size;
		}		

		if( write_data_num > 0 )
		{
			ret = write(fd,buf,write_data_num);

			if( ret < 0 )
				return DF_FALSE;
		}
		

		if( size != write_data_num )
			ret = write(fd,write_buf,DF_PAGESIZE);

		ret = DF_Lseek(fd,cur_offset+size,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;
		

		return ret;	

	}else
	{
		off_t read_offset;
		int num = 0;

		num = cur_offset % DF_PAGESIZE;
		read_offset = (cur_offset / DF_PAGESIZE) * DF_PAGESIZE ;

		printf("%x %x %x %x 3\n",write_buf2,write_buf,cur_offset,read_offset);

		ret = DF_Lseek(fd,read_offset,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;

		printf("1f\n");

		

		memset(write_buf,0x00,DF_PAGESIZE);
		ret = DF_Read(fd,write_buf,DF_PAGESIZE);
		if( ret != DF_PAGESIZE  )
			return DF_FALSE;		

		ret = DF_Lseek(fd,read_offset,SEEK_SET);
		if( ret < 0 )
			return ret;	
		
		
		memcpy(&write_buf[num],buf,(DF_PAGESIZE - num));	

		printf("4f\n");
		
		ret = write(fd,write_buf,DF_PAGESIZE);
		if( ret < 0 )
			return DF_FALSE;

		buf = &buf[DF_PAGESIZE - num];

		size = size - (DF_PAGESIZE - num);

		if( size % DF_PAGESIZE != 0 )
		{
			write_data_num = (size / DF_PAGESIZE) * DF_PAGESIZE ;

			memset(write_buf,0x00,DF_PAGESIZE);
			memcpy(write_buf,buf,size % DF_PAGESIZE);
		}else
		{
			write_data_num = size;
		}

		if( write_data_num > 0 )
		{
			ret = write(fd,buf,write_data_num);
			if( ret < 0 )
				return DF_FALSE;
		}	

		ret = write(fd,write_buf,DF_PAGESIZE);	

		printf("6f %d %d\n",ret,DF_Lseek(fd,0,SEEK_CUR));

		ret = DF_Lseek(fd,cur_offset+data_size,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;

		printf("5f %d\n",ret);

		return ret;	

		
	}
}


int DF_Write3(int fd,char * buf,int size)
{
	static char * write_buf2=NULL;
	char * write_buf;
	int ret;
	int write_data_num = 0;
	off_t cur_offset = 0;
	int data_size = size;

	if( write_buf2 == NULL )
	{
		write_buf2 = malloc(DF_WRITEBUFMAXSIZE);
		if(write_buf2 == NULL )
		{
			printf("malloc error!123\n");
			return DF_FALSE;
		}
	}

	if( size > (DF_WRITEBUFMAXSIZE - DF_PAGESIZE*2) )
	{
		printf("write size too large size=%d\n",size);
		return DF_FALSE;
	}

	write_buf = (char*)((((int) write_buf2+DF_PAGESIZE-1)/DF_PAGESIZE)*DF_PAGESIZE);
	

	cur_offset = DF_Lseek(fd,0,SEEK_CUR);

//	printf("%x %x %x %x\n",write_buf2,write_buf,cur_offset,&size);

	if( cur_offset % DF_PAGESIZE ==  0 )
	{
		//memset(write_buf,0x00,DF_WRITEBUFMAXSIZE - DF_PAGESIZE);
		memcpy(write_buf,buf,size);	

		write_data_num = (size + DF_PAGESIZE - 1 )/DF_PAGESIZE * DF_PAGESIZE;
	
		ret = write(fd,write_buf,write_data_num);
		if( ret < 0 )
			return DF_FALSE;		

		ret = DF_Lseek(fd,cur_offset+size,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;
		

		return ret;	

	}else
	{
		off_t read_offset;
		int num = 0;

		num = cur_offset % DF_PAGESIZE;
		read_offset = (cur_offset / DF_PAGESIZE) * DF_PAGESIZE ;

	//	printf("%x %x %x %x 3\n",write_buf2,write_buf,cur_offset,read_offset);

		ret = DF_Lseek(fd,read_offset,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;
		

		memset(write_buf,0x00,DF_PAGESIZE);
		ret = DF_Read(fd,write_buf,DF_PAGESIZE);
		if( ret != DF_PAGESIZE  )
			return DF_FALSE;		

		ret = DF_Lseek(fd,read_offset,SEEK_SET);
		if( ret < 0 )
			return ret;	
		
		
		memcpy(&write_buf[num],buf,(DF_PAGESIZE - num));	
	
		
		ret = write(fd,write_buf,DF_PAGESIZE);
		if( ret < 0 )
			return DF_FALSE;

		buf = &buf[DF_PAGESIZE - num];

		size = size - (DF_PAGESIZE - num);

		memset(write_buf,0x00,DF_WRITEBUFMAXSIZE - DF_PAGESIZE);
		memcpy(write_buf,buf,size);	

		write_data_num = (size + DF_PAGESIZE - 1 )/DF_PAGESIZE * DF_PAGESIZE;
	
		ret = write(fd,write_buf,write_data_num);
		if( ret < 0 )
			return DF_FALSE;		

		ret = DF_Lseek(fd,cur_offset+data_size,SEEK_SET);
		if( ret < 0 )
			return DF_FALSE;

	//	printf("5f %d\n",ret);

		return ret;	

		
	}
}


