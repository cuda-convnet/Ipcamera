#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

main()
{
	int fd,rel;	
	fd = open("1.txt",O_RDWR);
	
	write(fd,"sdfsdf",7);

	rel = ftruncate(fd,500*1024);
	if( rel == -1 )
	{
		printf(" ftruncate error! rel=%d\n",errno);
	}
	
	close(fd);
}	