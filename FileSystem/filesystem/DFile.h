#ifndef _DFILE_H_
#define _DFILE_H_

#include <sys/types.h>
#include <unistd.h>

#define DF_PAGESIZE  (4096)
#define DF_WRITEBUFMAXSIZE (368*1024)


#define DF_SUCCESS 1
#define DF_FALSE  (-1)

int DF_Open(char * filename,char * mode);
int DF_Close(int fd);
int DF_Read(int fd,void * buf,int size);
off_t DF_Lseek(int fildes, off_t offset, int whence);
int DF_Write(int fd,char * buf,int size);

#endif // !defined(_DFILE_H_)

