
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include  <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <linux/watchdog.h>

#include "audioctrl.h"

	
int main(int argc,char *argv[] )
{
	int ret=0;
	int i = 0,j = 0;
	char cmd[50];
	char buf[50000];
	int  count = 0;
	char channel_buf[4096];
	FILE * fp = NULL;
      int fd;
      int sec = 10;
      char wd = 0;
      int check_time = 0;     

	sleep(5); 

	fp = fopen("/tmp/use_wdog","rb");
	if( fp == NULL )
	{
		printf("can't  fopen use_wdog!not use wdog \n");
		return 1;
	}

	fclose(fp);
	fp = NULL;

	printf("use watchdog\n");


	fp = fopen("/ramdisk/watchdog","wb");
	if( fp == NULL )
	{
		printf("can't create watchdog! reboot;\n");
		system("reboot");
	}

	wd = 0;

	ret = fwrite(&wd,1,1,fp);
	if( ret != 1 )
	{
		printf("fwrite watchdog! reboot;\n");
		system("reboot");
	}

	fclose(fp);
	fp = NULL;



	while(1)
	{		

		sleep(10);

		
		fp = fopen("/ramdisk/watchdog","r+b");
		if( fp == NULL )
		{
			printf("can't  fopen watchdog! reboot;\n");
			system("reboot");
		}

		wd = 0;

		ret = fread(&wd,1,1,fp);
		if( ret != 1 )
		{
			printf("fread watchdog! reboot;\n");
			system("reboot");
		}

		if( wd == 1 )
		{
			fseek( fp , 0 , SEEK_SET );
			
			wd = 0;
			
			ret = fwrite(&wd,1,1,fp);
			if( ret != 1 )
			{
				printf("fwrite watchdog! reboot;\n");
				system("reboot");
			}

			check_time = 0;
		}else
		{
			check_time++;

			if( check_time > 1 )
			{
				printf("check_time > 1 !echo  reboot;\n");
				system("echo 1 > /dev/wdt");
				sleep(12);
				printf("check_time > 1 !system  reboot;\n");
				system("reboot");
			}
		}

		fclose(fp);
		fp = NULL;
	}


	return 1;


	if( strncmp(argv[1], "r", 1) == 0 )
	{

	if( open_audio_dev(NULL) == ERROR )
	{
		printf("open audio error!\n");
		return 1;
	}

	fp = fopen("1.pcm","wb");

	if( fp )
	{
		for( i = 0; i < 20; i++)
		{	
			
			if( read_audio_buf(buf,4096) == ERROR )
			{
				printf("read audio error!\n");
				return 1;
			}

/*
			count = 0;

			for( j = 0;j < 4096;j = j+4)
			{
				channel_buf[count] = buf[j];
				channel_buf[count+1] = buf[j+1];
				count = count + 2;
			}

			fwrite(channel_buf,1,count,fp);
			*/;

			fwrite(buf,1,4096,fp);

			if( write_audio_buf(buf,4096) == ERROR )
			{
				printf("read audio error!\n");
				return 1;
			}

			
		}
	}

	fclose(fp);

	close_audio_dev();

	}else
	{
		if( open_audio_dev(NULL) == ERROR )
		{
			printf("open audio error!\n");
			return 1;
		}

		fp = fopen("1.pcm","rb");

		if( fp )
		{
			for( i = 0; i < 20; i++)
			{				

			/*
				fread(buf,1,4096/2,fp);					
				count = 0;
				for( j = 0;j < 4096/2;j=j+2)
				{
					channel_buf[count] = 0;
					channel_buf[count+1] = 0;
					channel_buf[count+2] = buf[j];
					channel_buf[count+3] = buf[j+1];
					count = count + 4;
				}
				

				if( write_audio_buf(channel_buf,count) == ERROR )
				{
					printf("read audio error!\n");
					return 1;
				} */

				fread(buf,1,4096,fp);		
				

				if( write_audio_buf(buf,4096) == ERROR )
				{
					printf("read audio error!\n");
					return 1;
				} 
				
			}
		}

		fclose(fp);

		close_audio_dev();
	}

	

	printf("test ok!");
	

	return 0;
	
}

