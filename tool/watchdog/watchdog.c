#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>

int fd_watchdog = -1;

void feed_dog(int fd_watchdog)
{
	//feed the watchdog
	if(fd_watchdog >= 0) {
		static unsigned char food = 0;
		ssize_t eaten = write(fd_watchdog, &food, 1);
		if(eaten != 1) {
			puts("\n!!! FAILED feeding watchdog");
		}
	}

}


void close_dog(int signo)
{
	int ret = -1;
	printf("close dog\n");
#if 0
	//试过sdk 1.0.6 的不成功
	//内核配置"Disable watchdog shutdown on close" = no 才能关闭
	ret = close(fd_watchdog);
#else
	//driver supports "Magic Close"
	ssize_t eaten = write(fd_watchdog, "V", 1);
	if(eaten != 1) {
		puts("\n!!! FAILED feeding watchdog");
	}
#endif
	exit(0);
}
int main(void)
{
	int ret = -1;
	int timeout = 60;

	fd_watchdog = open("/dev/watchdog", O_WRONLY);
	if(fd_watchdog == -1) {
		int err = errno;
		printf("\n!!! FAILED to open /dev/watchdog, errno: %d, %s\n", err, strerror(err));
	}
	signal(SIGINT, close_dog);
	feed_dog(fd_watchdog);

	ret = ioctl(fd_watchdog, WDIOC_GETTIMEOUT, &timeout);
	printf("The timeout was is %d seconds\n", timeout);

	timeout = 60;
	ioctl(fd_watchdog, WDIOC_SETTIMEOUT, &timeout);
	printf("The timeout was set to %d seconds\n", timeout);

	int count =0;
	while(1) {
		count++;
		sleep(1);
		printf("%d\n", count);

		if (count >= timeout - 3) {
			count =0;
			feed_dog( fd_watchdog);
		}
	}

	close(fd_watchdog);
}
