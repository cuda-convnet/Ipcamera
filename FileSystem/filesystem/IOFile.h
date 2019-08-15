#ifndef _IOFILE_H_
#define _IOFILE_H_

#include <sys/types.h>
#include <unistd.h>


#define IO_DISK_VER "IOFILE v1.01"
#define IO_DISK_INVALID "INVALID DISK v1.02"

#define IO_PAGESIZE  (4096)
#define IO_WRITEBUFMAXSIZE (368*1024)

#define IO_DISK_RE_START_OFFSET (0)
#define IO_DISK_FLAG_OFFSET  (IO_DISK_RE_START_OFFSET + 1*1024*1024 - 2*512)
#define IO_DISK_START_OFFSET (IO_DISK_RE_START_OFFSET + 1*1024*1024)
#define IO_DISK_FILE_INFO_OFFSET (IO_DISK_RE_START_OFFSET + 2*1024*1024)
#define IO_DISK_FILE_START_OFFSET ( IO_DISK_RE_START_OFFSET +100*1024*1024)


#define DF_SUCCESS 1
#define DF_FALSE  (-1)

#define DF_INVALID_DISK  2

#define FILE_CLOSE (0)
#define FILE_OPEN (1)



typedef struct  DISK_FILE_TOTAL_INFO_
{
	long long file_num;
	long long file_total_size;
}DISK_FILE_TOTAL_INFO;

typedef struct DISK_FILE_INFO_
{		
	long long disk_offset;
	long long file_max_size;
	long long file_size;
	long long file_create_time;
	long long file_edit_time;
	int file_pos;
	char file_name[100];
}DISK_FILE_INFO;

typedef struct DISK_IO_FILE_ST_
{
	long long fileoffset;
	long long file_size;
	long long diskoffset;
	DISK_FILE_TOTAL_INFO file_total_info;
	DISK_FILE_INFO file_info;
	int fd;
	int status;	
	char diskname[100];
	char filename[100];	
}DISK_IO_FILE;

	
/*
int IO_Open(char * filename,char * mode);
int IO_Close(int fd);
int IO_Read(int fd,void * buf,int size);
off_t IO_Lseek(int fildes, off_t offset, int whence);
int IO_Write(int fd,char * buf,int size);
*/

int file_io_open(char * dev,char * filename, char * mode,void * file_st,int file_pos);
int file_io_read(void * file_st,void * buf,int size);
off_t file_io_lseek(void * file_st,off_t offset, int whence);
int file_io_write(void * file_st,char * buf,int size);
int file_io_sync(void * file_st);
int file_io_close(void * file_st);
int file_io_file_get_length(void * file_st);
int file_io_file_get_pos(void * file_st);
int disk_io_check_disk_size(char * dev);
int check_io_file_version(char * dev);


#define DEBUG

#ifdef DEBUG
#  define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#define trace DPRINTK  
#else
#  define DPRINTK(fmt, args...)
#endif


#endif // !defined(_IOFILE_H_)

