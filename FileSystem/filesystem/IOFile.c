#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <linux/fb.h>
//#include <asm/page.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "IOFile.h"


static int pagesize = 0;

static int IO_Open(char * filename,char * mode)
{
	int i = 0;
	int read_and_write = 0;
	int read_only = 0;
	int create_file_cut = 0;
	int flag = 0;
	int fd;
	

	if( pagesize == 0 )
	{
		if(IO_PAGESIZE == getpagesize() )
		{
			pagesize = IO_PAGESIZE;
		}else
		{
			DPRINTK("pagesize is %d error!\n",getpagesize());
			return DF_FALSE;
		}
		
	}

	for( i = 0; i < strlen(mode);i++)
	{
		if( mode[i] == '+' )
			read_and_write = 1;

		if( mode[i] == 'w' )
			create_file_cut = 1;

		if( mode[i] == 'r' )
			read_only = 1;
	}

	if( (read_and_write == 0) && (create_file_cut == 0) && (read_only ==0) )
	{
		DPRINTK("mode error; mode=%s\n",mode);
		return DF_FALSE;
	}

	//if( create_file_cut )
	//flag = flag | O_CREAT | O_TRUNC;

	//if( read_and_write )
	flag = flag | O_RDWR;

	//flag = flag | O_DIRECT;

	fd = open(filename,flag);


	return fd;
}

static int IO_Close(int fd)
{
	return close(fd);
}

static int IO_Read(int fd,void * buf,int size)
{
	int ulReadLen = 0;
	int iRet = 0;
	int eagain_num = 0;

	//return read(fd,buf,size);


	while( ulReadLen < size )
	{		
		iRet = read( fd, buf, size - ulReadLen);	
		if ( iRet <= 0 )
		{
			DPRINTK("ulReadLen=%d %d  eagain_num=%d\n",iRet,size,eagain_num);
			DPRINTK("%s\n", strerror(errno));

			if( iRet < 0 )
			{
				if( errno == EAGAIN )         
				{
					eagain_num++;
					if( eagain_num < 2 )
	                        		continue;
				}
			}
			
			return -1;
		}
		ulReadLen += iRet;
		buf += iRet;
	}	
	
	return size;	
	
}

static off_t IO_Lseek(int fildes, off_t offset, int whence)
{
	return lseek(fildes,offset,whence);	
}

static int IO_Write(int fd,char * buf,int size)
{	
	int lSendLen;
	int lStotalLen;
	int lRecLen;
	int eagain_num = 0;
	char* pData = buf;

	//return write(fd,buf,size);


	lSendLen = size;
	lStotalLen = lSendLen;
	lRecLen = 0;
	
	
	while( lRecLen < lStotalLen )
	{		
		lSendLen = write(fd,pData,lStotalLen - lRecLen);				
		if ( lSendLen <= 0 )
		{
			DPRINTK("write=%d %d eagain_num=%d\n",lSendLen,lStotalLen,eagain_num);
			DPRINTK("%s\n", strerror(errno));

			if( errno == EAGAIN )            
			{
			    eagain_num++;
			    if( eagain_num < 2 )	
                        	continue;
			}
			
			return -1;
		}
		lRecLen += lSendLen;
		pData += lSendLen;
	}
	
	return lStotalLen;	
	
}


/*//////////////////////////////////////////////////////
DISK FILE NAME: diskname + filename: /dev/sda5 + video_0.avd : /dev/sda5/video_0.avd


*/////////////////////////////////////////////////////////////

long long get_64time(int year,int month,int day,int hour,int min,int sec)
{
	long long mytime;
	long long tmp[6];

	tmp[0] = sec;
	tmp[1] = min;
	tmp[2] = hour;
	tmp[3] = day;
	tmp[4] = month;
	tmp[5] = year;

	mytime = tmp[0] + tmp[1] * 100 +tmp[2] * 10000 + tmp[3] * 1000000  
		+ tmp[4] * 100000000	+ tmp[5] * 10000000000;

	return mytime;
}

int disk_save_file_info(void * file_st)
{
	DISK_IO_FILE * pfile;
	DISK_FILE_INFO * pfile_info;
	off_t offset;
	off_t old_offset;
	int ret;

	
	pfile = (DISK_IO_FILE *)file_st;

	pfile_info = &pfile->file_info;

	offset = IO_DISK_FILE_INFO_OFFSET + pfile_info->file_pos * sizeof(DISK_FILE_INFO);

	ret = IO_Lseek(pfile->fd,offset,SEEK_SET);
	if( ret == (off_t)-1 )
	{
		DPRINTK("leek error!  %lld %lld\n",old_offset,offset);
		return -1;
	}

	ret = IO_Write(pfile->fd,(char *)pfile_info, sizeof(DISK_FILE_INFO));
	if( ret != sizeof(DISK_FILE_INFO) )
		return -1;

	ret = fsync(pfile->fd);
	if( ret < 0 )
	{
		DPRINTK(" error %s %d\n", strerror(errno),errno);
	}

	//DPRINTK("%s pfile_info->file_pos = %d , filesize=%lld\n",pfile_info->file_name,pfile_info->file_pos,pfile_info->file_size);	


	return  1;
}

// disk_size is  M bytes.


int format_disk_to_io_sys(char * dev,int disk_size)
{
	int fd = 0;
	off_t offset;
	DISK_FILE_TOTAL_INFO file_total_info;
	DISK_FILE_INFO file_info;
	int ret;
	int total_file_num = 0;
	int i;
	char tmp_buf[512];
	
	int file_pos = 0;

	if(0)
	{
		char buf[1024*5];
		FILE * fp;

		fd = IO_Open(dev,"w+");

		offset = IO_Lseek(fd,IO_DISK_START_OFFSET,SEEK_SET);

		ret = IO_Read(fd,buf,1024*5);

		IO_Close(fd);

		printf("sizeof(%d)\n",sizeof(DISK_FILE_TOTAL_INFO));

		fp = fopen("/nfs/yb/disk.dat","wb");

		fwrite(buf,1024*5,1,fp);

		fclose(fp);

		return 1;

		
		
		
	}
	

	fd = IO_Open(dev,"w+");

	if( fd <= 0 )
		goto  open_dev_error;

	
	offset = IO_Lseek(fd,IO_DISK_FLAG_OFFSET,SEEK_SET);
	if( ret == (off_t)-1 )
		goto file_op_error;

	memset(tmp_buf,0x00,sizeof(tmp_buf));

	strcpy(tmp_buf,IO_DISK_VER);	

	ret = IO_Write(fd,tmp_buf, 512);
	if( ret != 512 )
		goto file_op_error;

	offset = IO_Lseek(fd,IO_DISK_START_OFFSET,SEEK_SET);

	if( ret == (off_t)-1 )
		goto file_op_error;

	file_total_info.file_num = 0;
	file_total_info.file_total_size = 0;

	ret = IO_Write(fd,(char *)&file_total_info, sizeof(DISK_FILE_TOTAL_INFO));
	if( ret != sizeof(DISK_FILE_TOTAL_INFO) )
		goto file_op_error;

	//build file system.	
	
	offset = IO_Lseek(fd,IO_DISK_FILE_INFO_OFFSET,SEEK_SET);

	if( ret == (off_t)-1 )
		goto file_op_error;

	total_file_num = disk_size / 128;

	if( (total_file_num * sizeof(DISK_FILE_INFO) + IO_DISK_FILE_INFO_OFFSET)
		> IO_DISK_FILE_START_OFFSET)
	{
		DPRINTK(" too many file large than file start: %d %d\n",
			total_file_num *2* sizeof(DISK_FILE_INFO) + IO_DISK_FILE_INFO_OFFSET,
			IO_DISK_FILE_START_OFFSET);
		
		goto file_op_error;
	}else
	{
		DPRINTK("files start: %d %d\n",
			total_file_num *2* sizeof(DISK_FILE_INFO) + IO_DISK_FILE_INFO_OFFSET,
			IO_DISK_FILE_START_OFFSET);
	}

	offset = IO_DISK_FILE_START_OFFSET;
	file_pos = 0;

	for( i = 0; i < total_file_num ; i++)
	{
		memset(tmp_buf,0x00,200);

		sprintf(tmp_buf,"video_%d.avh",i);

		strcpy(file_info.file_name,tmp_buf);
		file_info.file_max_size = 3 * 1024 *1024;
		file_info.file_pos = file_pos;		
		file_info.disk_offset = offset;
		file_info.file_size = 0;

		{
			struct timeval tv;
			struct tm tp;
			gettimeofday( &tv,NULL );	
			gmtime_r(&tv.tv_sec,&tp);	
			file_info.file_create_time = (long long)get_64time(tp.tm_year,tp.tm_mon,tp.tm_mday,tp.tm_hour,tp.tm_min,tp.tm_sec);
			file_info.file_edit_time = file_info.file_create_time;
		}

		ret = IO_Write(fd,(char *)&file_info, sizeof(DISK_FILE_INFO));
		if( ret != sizeof(DISK_FILE_INFO) )
			goto file_op_error;

		if( file_pos == 0 )
			DPRINTK("%s pos=%d  disk_offet=%lld  file_max_size=%lld\n",file_info.file_name,file_info.file_pos,file_info.disk_offset,file_info.file_max_size);

		offset += file_info.file_max_size;
		file_pos++;

		memset(tmp_buf,0x00,200);

		sprintf(tmp_buf,"video_%d.avd",i);

		strcpy(file_info.file_name,tmp_buf);
		file_info.file_max_size = 125 * 1024 *1024;
		file_info.file_pos = file_pos;
		file_info.disk_offset = offset;		

		ret = IO_Write(fd,(char *)&file_info, sizeof(DISK_FILE_INFO));
		if( ret != sizeof(DISK_FILE_INFO) )
			goto file_op_error;

		if( file_pos == 1 )
		DPRINTK("%s pos=%d  disk_offet=%lld  file_max_size=%lld\n",file_info.file_name,file_info.file_pos,file_info.disk_offset,file_info.file_max_size);

		offset += file_info.file_max_size;
		file_pos++;

		
	}	

	file_total_info.file_num = total_file_num*2;
	file_total_info.file_total_size = offset;

	offset = IO_Lseek(fd,IO_DISK_START_OFFSET,SEEK_SET);

	if( ret == (off_t)-1 )
		goto file_op_error;	
	
	ret = IO_Write(fd,(char *)&file_total_info, sizeof(DISK_FILE_TOTAL_INFO));
	if( ret != sizeof(DISK_FILE_TOTAL_INFO) )
		goto file_op_error;
	

	IO_Close(fd);
	fd = 0;

	printf("filenum=%lld file_total_size=%lld \n",file_total_info.file_num,file_total_info.file_total_size);
	
	return 1;


open_dev_error:
	DPRINTK(" dev:%s open error!\n");
	return -1;
	
file_op_error:
	DPRINTK("error: diskname %s\n",dev);
	if( fd > 0 )
	{
		IO_Close(fd);
		fd = 0;
	}	
	return -1;
	
}

int check_io_file_version(char * dev)
{
	int fd = 0;
	off_t offset;
	DISK_FILE_TOTAL_INFO file_total_info;
	DISK_FILE_INFO file_info;
	int ret;
	int total_file_num = 0;
	int i;
	char tmp_buf[512];


	fd = IO_Open(dev,"w+");

	if( fd <= 0 )
		goto  file_check_error;

	
	offset = IO_Lseek(fd,IO_DISK_FLAG_OFFSET,SEEK_SET);
	if( ret == (off_t)-1 )
		goto file_check_error;

	memset(tmp_buf,0x00,sizeof(tmp_buf));	

	ret = IO_Read(fd,tmp_buf, 512);
	if( ret != 512 )
		goto file_check_error;

	DPRINTK("info: %s\n",tmp_buf);

	if( strcmp(tmp_buf,IO_DISK_INVALID) == 0 )
	{
		DPRINTK("%s  %s is invalid\n",IO_DISK_INVALID,tmp_buf);
		goto diks_is_invalid_ok;
	}


	if( strcmp(tmp_buf,IO_DISK_VER) != 0 )
	{
		DPRINTK("%s  %s is not same\n",IO_DISK_VER,tmp_buf);
		goto file_check_error;
	}

	DPRINTK("file system ver: %s\n",IO_DISK_VER);


	IO_Close(fd);
	fd = 0;
	
	return 1;
	
file_check_error:
	DPRINTK("error: diskname %s\n",dev);
	if( fd > 0 )
	{
		IO_Close(fd);
		fd = 0;
	}	
	return -1;

diks_is_invalid_ok:

	IO_Close(fd);
	fd = 0;
	return DF_INVALID_DISK;
}


int file_io_open(char * dev,char * filename, char * mode,void * file_st,int file_pos)
{
	DISK_IO_FILE * pfile;
	char splitpath[2][100];
	off_t offset;
	int ret;
	int i;
	int find_file= 0;
	int create_new_file = 0;
	int read_file = 0;
	int edit_file = 0;

	pfile = (DISK_IO_FILE *)file_st;

	memset(pfile,0x00,sizeof(DISK_IO_FILE));
	
	splitpath[0][0] = 0;
	splitpath[1][0] = 0;
	

	strcpy(splitpath[0],dev);
	strcpy(splitpath[1],filename);
	

	if( (splitpath[0][0] == 0) ||(splitpath[1][0] == 0)  )
		goto wrong_file_name;

	sprintf(pfile->diskname,"%s",splitpath[0]);
	sprintf(pfile->filename,"%s",splitpath[1]);


	for( i = 0; i < strlen(mode);i++)
	{	
		if( mode[i] == 'r' )
			read_file = 1;

		if( mode[i] == 'w' )
			create_new_file = 1;

		if( mode[i] == '+' )
			edit_file = 1;
	}



	pfile->fd = IO_Open(pfile->diskname,mode);

	if( pfile->fd <= 0 )	
		goto open_disk_error;

	offset = IO_Lseek(pfile->fd,IO_DISK_START_OFFSET,SEEK_SET);

	if( ret == (off_t)-1 )
		goto file_op_error;	
	
	ret = IO_Read(pfile->fd,&pfile->file_total_info,sizeof(DISK_FILE_TOTAL_INFO));
	if( ret != sizeof(DISK_FILE_TOTAL_INFO))
		goto file_op_error;

	//DPRINTK(" file_total_size=%lld filenum=%d\n",pfile->file_total_info.file_total_size,pfile->file_total_info.file_num);

	
	offset = IO_Lseek(pfile->fd,IO_DISK_FILE_INFO_OFFSET,SEEK_SET);

	if( ret == (off_t)-1 )
		goto file_op_error;

	find_file= 0;

	//ÎªÁË¿ìËÙÕÒµ½ÎÄ¼þ£¬¸Ä½øµÄ¡
	{
		sscanf(pfile->filename,"video_%d.av",&file_pos);

		//DPRINTK("file_pos=%d \n",file_pos);
		file_pos = file_pos*2;

		offset = IO_Lseek(pfile->fd,IO_DISK_FILE_INFO_OFFSET+ file_pos* sizeof(DISK_FILE_INFO),SEEK_SET);
		if( ret == (off_t)-1 )
			goto file_op_error;		
	}	

		
	for(i = file_pos; i <  pfile->file_total_info.file_num; i++)
	{		
		ret = IO_Read(pfile->fd,&pfile->file_info,sizeof(DISK_FILE_INFO));
		if( ret != sizeof(DISK_FILE_INFO))
			goto file_op_error;

		if( strcmp(pfile->file_info.file_name,pfile->filename) == 0 )
		{
			find_file = 1;
			break;
		}
	}

	if( find_file == 0 )
		goto file_open_error;

	{
		struct timeval tv;
		struct tm tp;

		gettimeofday( &tv,NULL );		
	
		gmtime_r(&tv.tv_sec,&tp);
		
		if( create_new_file == 1 )
		{
			pfile->file_info.file_edit_time = (long long)get_64time(tp.tm_year,tp.tm_mon,tp.tm_mday,tp.tm_hour,tp.tm_min,tp.tm_sec);
			pfile->file_info.file_create_time = pfile->file_info.file_edit_time;
			pfile->file_info.file_size = 0;
		}

		if( edit_file == 1 )
		{
			pfile->file_info.file_edit_time = (long long)get_64time(tp.tm_year,tp.tm_mon,tp.tm_mday,tp.tm_hour,tp.tm_min,tp.tm_sec);
		}

		if( read_file == 1 )
		{
			//do nothing.
		}
	
		ret = disk_save_file_info(file_st);
		if( ret < 0 )
		{
			DPRINTK("disk_save_file_info error! fd= %d\n",pfile->fd);
			goto file_op_error;
		}
	}

	pfile->diskoffset = pfile->file_info.disk_offset;
	pfile->fileoffset = 0;
	pfile->file_size = pfile->file_info.file_size;
	pfile->status = FILE_OPEN;

	offset = IO_Lseek(pfile->fd,pfile->diskoffset,SEEK_SET);
	if( ret == (off_t)-1 )
		goto file_op_error;

	DPRINTK("open file %s  max=%lld\n",filename,pfile->file_info.file_max_size);
	
	return 1;

wrong_file_name:
	DPRINTK("error: filename %s\n",filename);
	return -1;

open_disk_error:
	DPRINTK("error: diskname %s\n",pfile->diskname);
	return -1;

file_op_error:
	DPRINTK("error: diskname %s\n",pfile->diskname);
	if( pfile->fd > 0 )
	{
		IO_Close(pfile->fd);
		pfile->fd = 0;
	}	
	return -1;

file_open_error:
	DPRINTK("error: filename %s\n",pfile->filename);
	if( pfile->fd > 0 )
	{
		IO_Close(pfile->fd);
		pfile->fd = 0;
	}	
	
	return -1;
	
}




 int file_io_read(void * file_st,void * buf,int size)
{
	int ret;
	DISK_IO_FILE * pfile;	
	pfile = (DISK_IO_FILE *)file_st;


	if( pfile->fileoffset + size > pfile->file_info.file_size )
	{
		DPRINTK("write data over file max size  %lld %d %lld\n",
			pfile->fileoffset,size, pfile->file_info.file_size);		
		return 0;
	}

	//printf("1 offset = %lld\n",IO_Lseek(pfile->fd,0,SEEK_CUR));

	ret = IO_Read(pfile->fd,buf,size);
	if( ret > 0 )
	{
		pfile->fileoffset = pfile->fileoffset + size;		
	}

	//printf("2 offset = %lld\n",IO_Lseek(pfile->fd,0,SEEK_CUR));

	return ret;	
}

off_t file_io_lseek(void * file_st,off_t offset, int whence)
{
	DISK_IO_FILE * pfile;
	off_t disk_offset;	
	off_t ret = 0;
	
	pfile = (DISK_IO_FILE *)file_st;

	disk_offset = pfile->diskoffset + offset;

	if( (disk_offset < pfile->file_info.disk_offset) ||
		(disk_offset > (pfile->file_info.disk_offset + pfile->file_size) ) )
	{
		DPRINTK("offset = %lld disk_offset=%lld file_size=%lld\n",
			offset,pfile->file_info.disk_offset,pfile->file_size);
		return -1;
	}

	if(disk_offset > (pfile->file_info.disk_offset + pfile->file_info.file_max_size)  )
	{
		DPRINTK("offset = %lld disk_offset=%lld file_max_size=%lld\n",
			offset,pfile->file_info.disk_offset,pfile->file_info.file_max_size);
		return -1;
	}

	ret = IO_Lseek(pfile->fd,disk_offset,SEEK_SET);

	if( ret != (off_t)-1 )
	{
		pfile->fileoffset = offset;
	}

	return ret;
}

 int file_io_write(void * file_st,char * buf,int size)
{
	int ret;
	DISK_IO_FILE * pfile;	
	pfile = (DISK_IO_FILE *)file_st;

	if( pfile->fileoffset + size > pfile->file_info.file_max_size )
	{
		DPRINTK("write data over file max size  %lld %d %lld\n",
			pfile->fileoffset,size, pfile->file_info.file_max_size);
		
		return 0;
	}	

	ret = IO_Write(pfile->fd,buf,size);
	if( ret > 0 )
	{
		pfile->fileoffset = pfile->fileoffset + size;	

		if( pfile->fileoffset > pfile->file_size )
			 pfile->file_size = pfile->fileoffset;
		
	}		
	
	return ret ;	
}

 int file_io_sync(void * file_st)
 {
 	int ret = 0;
 	DISK_IO_FILE * pfile;	
	pfile = (DISK_IO_FILE *)file_st;
	
 	if( pfile->file_size > pfile->file_info.file_size )
	{
		//DPRINTK("%s file_size=%lld  org_file_size=%lld\n",pfile->filename,pfile->file_size,pfile->file_info.file_size);
		
		pfile->file_info.file_size = pfile->file_size;
		ret = disk_save_file_info(file_st);
		if( ret < 0 )
		{
			DPRINTK("disk_save_file_info error! fd= %d\n",pfile->fd);
			return -1;
		}
	}

	return 1;
 }

 int file_io_close(void * file_st)
{
	DISK_IO_FILE * pfile;

	pfile = (DISK_IO_FILE *)file_st;

	if( pfile->fd > 0 )
	{
		file_io_sync(file_st);
	
		IO_Close(pfile->fd);		

		memset(pfile,0x00,sizeof(DISK_IO_FILE));
	}	

	return 1;
}

 int file_io_file_get_length(void * file_st)
 {
 	DISK_IO_FILE * pfile;

	pfile = (DISK_IO_FILE *)file_st;

	if( pfile->fd > 0 )
	{
		return pfile->file_size;
	}

	return -1;
 }

  int file_io_file_get_pos(void * file_st)
 {
 	DISK_IO_FILE * pfile;

	pfile = (DISK_IO_FILE *)file_st;

	if( pfile->fd > 0 )
	{
		return pfile->fileoffset;
	}

	return -1;
 }

  
static void set_watchdog_func1()
{
	FILE * fp = NULL;
      int fd;
      int sec = 10;
      char wd = 0;
      int check_time = 0;  
	 int ret;

		fp = fopen("/ramdisk/watchdog","r+b");
		if( fp == NULL )
		{
			//DPRINTK("can't  fopen watchdog!\n");
			return;
		}

	
		fseek( fp , 0 , SEEK_SET );
		
		wd = 1;		
		ret = fwrite(&wd,1,1,fp);
		if( ret != 1 )
		{			
			DPRINTK("fwrite watchdog error!\n");			
		}		

		fclose(fp);
		fp = NULL;
		
}


  int disk_io_check_disk_size(char * dev)
  {
  	int fd = 0;
	off_t i  = 0;
	off_t j = 0;
	off_t cur_offset = 0;
	off_t rel = 0;
	char buf[1*1024*1024];	
	char buf2[1*1024*1024];
	int max_size_support = 64;	
	int availability_size = 0;

	
	fd = IO_Open(dev,"w+");

	if( fd <= 0 )
	{
		printf("open %s error!\n",dev);
		return -1;
	}		

	for( i = 0; i < max_size_support; i++)
	{

		memset(buf,0x32,1024*1024);
		memset(buf2,0x31,1024*1024);
		
		cur_offset =  (off_t)i * 1024*1024*1024;

		if( i == 0 )
			cur_offset =  (off_t)(i+1) * 1024*1024*1024 - 20*1024*1024;

		//printf("lseek %lld write  i=%d\n",cur_offset,i);

		rel = IO_Lseek(fd,cur_offset,SEEK_SET);
		if( rel == -1 )
		{
			printf("lseek error! %lld ret=%lld\n",cur_offset,rel);
			break;
		}

		rel = IO_Write(fd,(char *)buf, sizeof(buf));
		if( rel != sizeof(buf) )
		{
			printf("IO_Write error!\n");
			break;
		}	

		rel = IO_Write(fd,(char *)buf2, sizeof(buf2));
		if( rel != sizeof(buf2) )
		{
			printf("IO_Write error!\n");
			break;
		}

		if( i == (4-1) || i == (8-1) || i == (16-1)|| i == (32-1))
		{
			rel = IO_Write(fd,(char *)buf, sizeof(buf));
			if( rel != sizeof(buf) )
			{
				printf("IO_Write error!\n");
				break;
			}	

			rel = IO_Write(fd,(char *)buf2, sizeof(buf2));
			if( rel != sizeof(buf) )
			{
				printf("IO_Write error!\n");
				break;
			}	

			rel = IO_Write(fd,(char *)buf2, sizeof(buf2));
			if( rel != sizeof(buf) )
			{
				printf("IO_Write error!\n");
				break;
			}	

			rel = IO_Write(fd,(char *)buf2, sizeof(buf2));
			if( rel != sizeof(buf) )
			{
				printf("IO_Write error!\n");
				break;
			}	

			
		}

		fsync(fd);	
	
		set_watchdog_func1();
		printf("%lld write %d\n",cur_offset,sizeof(buf));
		
	}

	IO_Close(fd);


	fd = IO_Open(dev,"w+");

	if( fd <= 0 )
	{
		printf("open error!\n");
		return -1;
	}

	for( i = 0; i < max_size_support; i++)
	{
		cur_offset =  (off_t)i * 1024*1024*1024;

		if( i == 0 )
			cur_offset =  (off_t)(i+1) * 1024*1024*1024 - 20*1024*1024;

		//printf(" lseek %lld read i=%d\n",cur_offset,i);

		rel = IO_Lseek(fd,cur_offset,SEEK_SET);
		if( rel == -1 )
		{
			printf("lseek error! %lld ret=%lld\n",cur_offset,rel);
			break;
		}

		rel = IO_Read(fd,(char *)buf, sizeof(buf));
		if( rel != sizeof(buf) )
		{
			printf("IO_Read error!\n");
			break;
		}

		printf("%lld read  %d %d %d %d\n",cur_offset,buf[0],buf[1],buf[2],buf[3]);

		if( buf[0] == 0x32 &&  buf[1] == 0x32 &&  buf[2] == 0x32 && 
			 buf[3] == 0x32 )
		{
			availability_size++;
		}else
		{			
			break;
		}
		
	}

	IO_Close(fd);

	printf("availability_size = %d\n",availability_size);

	return availability_size;	
  }




int set_disk_is_invalid(char * dev)
{
	int fd = 0;
	off_t offset;
	DISK_FILE_TOTAL_INFO file_total_info;
	DISK_FILE_INFO file_info;
	int ret;
	int total_file_num = 0;
	int i;
	char tmp_buf[512];
	
	int file_pos = 0;
	

	fd = IO_Open(dev,"w+");

	if( fd <= 0 )
		goto  file_op_error;

	
	offset = IO_Lseek(fd,IO_DISK_FLAG_OFFSET,SEEK_SET);
	if( ret == (off_t)-1 )
		goto file_op_error;

	memset(tmp_buf,0x00,sizeof(tmp_buf));

	strcpy(tmp_buf,IO_DISK_INVALID);	

	ret = IO_Write(fd,tmp_buf, 512);
	if( ret != 512 )
		goto file_op_error;

	IO_Close(fd);
	fd = 0;

	DPRINTK("set disk %s invalid\n",dev);
	
	return 1;

	
file_op_error:
	DPRINTK("error: diskname %s\n",dev);
	if( fd > 0 )
	{
		IO_Close(fd);
		fd = 0;
	}	
	return -1;
	
}



