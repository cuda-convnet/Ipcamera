#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "systemcall.h"

#include <pthread.h>

#include<sys/socket.h>
#include <unistd.h>// for close function
#include <string.h> // for bzero function

#include<sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<netinet/in.h>
#include <linux/watchdog.h>

#include <netdb.h>
#include  <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/tcp.h>

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

#define SERV_PORT 12302
#define SERV_IP "xxx.xxx.xxx.xxx"//mx27 board ip
#define BACKLOG 10 //the counts of connect can keep in wait queen
#define DEST_IP "xxx.xxx.xxx.xxx"

static int g_listen_thread_work = 0;

static pthread_t g_secondsystemcallpro = -1;
static pthread_t g_executecommandpro = -1;

int Wdgid = -1;

void executeCommand(int value)
{
	char buf[500];
	int sockfd_client;
	int on = 1;
	sockfd_client = (int)value;
	
	//printf("sockfd_client:%d--value:%d-\n", sockfd_client, value);

	fd_set watch_set;
	struct  timeval timeout;
	int ret;

	ret = setsockopt( sockfd_client, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(on)); 
	if( ret != 0 )
	{
		printf("set socket TCP_NODELAY failed!\n");
	}

	FD_ZERO(&watch_set); 
 	FD_SET(sockfd_client,&watch_set);
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	ret  = select( sockfd_client + 1, &watch_set, NULL, NULL, &timeout );
	if ( ret < 0 )
	{
		printf( "recv socket select error!\n" );
		goto error_end;
	}

	if ( ret > 0 && FD_ISSET(sockfd_client, &watch_set ) )
	{
		int recvbytes;//the number of bytes receive from socket
		memset(buf,0,sizeof(buf));
		recvbytes = recv(sockfd_client,buf,500,0);
		if(recvbytes <= 0 )//changed here			
		{
			printf("recv_data error!\n");
			goto error_end;
		}
		//printf("%dbytes receive from connect:%s\n",recvbytes,buf);

		int n_send_len;

		if (strstr(buf, "reboot") != NULL)
		{
			memset(buf,0,sizeof(buf));
			strcpy(buf, "/mnt/mtd/fast_run.sh");
		}
		
		ret = system(buf);
		DPRINTK("%s  ret=%d  max=500 use TCP_NODELAY\n",buf,ret);
		if( ret < 0  || ret >= 256)
		{
			printf("execute command -%s- error! ret=%d\n", buf,ret);
			n_send_len = send(sockfd_client, buf,strlen(buf),0);
			//printf("%d bytes sent\n",n_send_len);
			goto error_end;
		}
		n_send_len = send(sockfd_client, "OK",strlen("OK"),0);
		//printf("%d bytes sent\n",n_send_len);

		if (strstr(buf, "fast_run.sh") != NULL)
		{
			printf("%s - use watchdog reboot\n", buf);
			system("sync");
			unsigned int time_sec = 0;
			Wdgid = open("/dev/watchdog", O_RDWR);
			if(Wdgid < 0)
				printf("OPEN WDG FAIL! %x.\n", Wdgid);
			else
			{
				time_sec = 1;
				if(Wdgid > 0) ioctl(Wdgid, WDIOC_SETTIMEOUT, &time_sec);
			}
		}
	}
	
	
error_end:

	close(sockfd_client);

	pthread_detach(pthread_self());

	return;
}

int execute_command_thread_create(int sockfd)
{
	//printf("start to create execute_command thread\n");
	int ret = 0;
	ret = pthread_create(&g_executecommandpro, NULL,(void *)executeCommand, sockfd);
	if ( ret < 0 )
	{
		printf("cam_recv_broadcast_thread create error\n");
		return -1;
	}
		//printf("cam_recv_broadcast_thread create success\n");
	return 1;
}

void RecvM(int argc, char **argv)
{
	//printf("into RecvM\n");
	int listen_fd,i;
	socklen_t sin_size; // used in accept(),but i don't know what it means
	struct sockaddr_in my_addr;//local ip info
	struct sockaddr_in serv_addr,client_sockaddr; //server ip info
	struct  timeval timeout;
	fd_set watch_set;
	int tmp_fd = 0;
	int serverport, ret = 0;
	//if(argc == 2)
	//{
	//	serverport = atoi(argv[1]);
	//}
	//else
	{
		serverport = SERV_PORT;
	}

	//printf("serverport:%d\n", serverport);
	
	if(-1 == (listen_fd = socket(AF_INET,SOCK_STREAM,0)) )
	{
		perror("error in create socket\n");
		exit(0);
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		
		printf("setsockopt SO_REUSEADDR error \n");		
		goto error_recv;	
	}
	
	//set the sockaddr_in struct
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(serverport);//server listening port
	serv_addr.sin_addr.s_addr = INADDR_ANY;//here is the specia in listening tcp connect
	bzero(&serv_addr.sin_zero,8);
//bind , the ip and port information is aready in the sockaddr
	if(-1 == bind(listen_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr)))
	{
		perror("bind error\n");
		exit(0);
	}
	//printf("bind seccessful\n");
	
	if(-1 == listen(listen_fd,BACKLOG))
	{
		perror("lisenning");
		exit(1);
	}

	sin_size = sizeof(struct sockaddr_in);

	printf("the server is listenning...\n");

	while(1)
	{
		
		FD_ZERO(&watch_set); 
 		FD_SET(listen_fd,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( listen_fd + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			printf( "listen socket select error!\n" );
			goto error_recv;
		}

		if ( ret > 0 && FD_ISSET( listen_fd, &watch_set ) )
		{
			//accept
			//printf("before accept:sockfd_client is %d\n",tmp_fd);
			tmp_fd = accept( listen_fd,(struct sockaddr *)&client_sockaddr, &sin_size);
			if(-1 == tmp_fd)
			{
				printf("accept error.\n");
				goto error_recv;
			}

			//printf("accept connect.\n");
			execute_command_thread_create(tmp_fd);
		}
	}
error_recv:
	if( listen_fd > 0 )
	{
		close(listen_fd);
		listen_fd = 0;
	}

	pthread_detach(pthread_self());
	return;
}

int SendM(char *argv)
{
	char buf[200];
	printf("into SendM\n");
	int sockfd,sock_dt, ret;
	struct  timeval timeout;
	fd_set watch_set;
	struct sockaddr_in my_addr;//local ip info
	struct sockaddr_in dest_addr; //destnation ip info

	int destport = 12302;//atoi(argv[2]);

	printf("destport:%d\n", destport);

	if(-1 == (sockfd = socket(AF_INET,SOCK_STREAM,0)) )
	{
		perror("error in create socket\n");
		goto error_Send;
	}

    	dest_addr.sin_family = AF_INET;
    	dest_addr.sin_port = htons(destport);
    	dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");//argv[1]);
    	memset(&dest_addr.sin_zero,0,8);
//connect
	if(-1 == connect(sockfd,(struct sockaddr*)&dest_addr,sizeof(struct sockaddr)))
	{
	        perror("connect error\n");
	        goto error_Send;
	}

	int n_send_len;
	n_send_len = send(sockfd, argv,strlen(argv),0);
	printf("%d bytes sent\n",n_send_len);

	while(1)
	{
		FD_ZERO(&watch_set); 
	 	FD_SET(sockfd,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( sockfd + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			printf( "listen socket select error!\n" );
			goto error_Send;
		}

		if ( ret > 0 && FD_ISSET( sockfd, &watch_set ) )
		{
			int recvbytes;//the number of bytes receive from socket
			memset(buf,0,sizeof(buf));
			recvbytes = recv(sockfd,buf,200,0);
			if(recvbytes <= 0)//changed here
			{		
				printf("recv_data error!\n");
				goto error_Send;			
			}
			printf("%dbytes receive from connect:%s\n",recvbytes,buf);

			if(strcmp(buf, "OK") == 0)
			{
				printf("execute command success! ret:%s\n", buf);
				break;
			}else
				goto error_Send;
		}
	}

	if( sockfd >= 0 )
	{
		close(sockfd);
		sockfd = -1;
	}
	return 1;

error_Send:

	{
		printf("execute command error! command:-%s-\n", buf);
	}
	if( sockfd >= 0 )
	{
		close(sockfd);
		sockfd = -1;
	}
	return -1;
}

void del_command_faild_file()
{
	int rel;
	rel = system(RM_COMMAND_FAILD_FILE);
	if( rel < 0 )
	{
		printf("systemcall: %s error = %d!\n",RM_COMMAND_FAILD_FILE,rel);
	}
}

void del_command_file()
{
	int rel;
	rel = system(RM_COMMAND_FILE);
	if( rel < 0 )
	{
		printf("systemcall: %s error = %d!\n",RM_COMMAND_FILE,rel);
	}
}

void comman_faild(char * cmd)
{
	FILE * fp = NULL;
	fp = fopen(COMMAND_FAILD_FILE,"wb");
	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}	

	if( cmd != NULL)
	printf("comman_faild:%s\n",cmd);
}


int   my_system(const   char   *command)   
{   

	 int   pid,   status;
	 char * envp[ ]={"PATH=/bin",0};
    
         if   (command   ==   0)   
                 return   1;

	 printf("execve: %s\n",command);
		 
         pid   =   fork();   
         if   (pid   ==   -1)   
                 return   -1;   
		 
         if   (pid   ==   0)   {   
                 char   *argv[4];   
                 argv[0]   =   "sh";   
                 argv[1]   =   "-c";   
                 argv[2]   =   (char*)command;   
                 argv[3]   =   0;   
                 execve("/bin/sh",   argv,   envp);   
                 exit(127);   
         }   
		 
         do{   
                 if(waitpid(pid,   &status,   0)   ==   -1) 
		   {   
                         if   (errno   !=   EINTR)   
                                 return   -1;   
                 }   else   
                         return   status;   
         }   while(1);   
}  

int secondsystemcall_thread_create()
{
	printf("start to create secondsystemcall_thread\n");
	int ret = 0;
	ret = pthread_create(&g_secondsystemcallpro, NULL,(void *)RecvM, NULL);
	if ( ret < 0 )
	{
		printf("secondsystemcall_thread create error\n");
		return -1;
	}
		printf("secondsystemcall_thread create success\n");
	return 1;
}

int  main(int argc, char **argv)
{
	 if(argc == 2)
	 	SendM(argv[1]);
	 else{
	secondsystemcall_thread_create();

	printf("into default\n");
	FILE * fp = NULL;
	int rel;
	long fileOffset = 0;
	char cmd_buf[200];
	int use_execve;

	del_command_file();


	while( 1 )
	{
		usleep(SLEEP_TIME);

		fp = fopen(COMMAND_FILE,"rb");
		if( fp != NULL )
		{

			//清除上一次失败的标示文件。
			del_command_faild_file();
			
			rel = fseek(fp,0L,SEEK_END);
			if( rel != 0 )
			{
				printf("systemcall: fseek error!!\n");
				fclose(fp);
				fp = NULL;
				del_command_file();
				comman_faild(NULL);
				continue;
			}

			fileOffset = ftell(fp);
			if( fileOffset == -1 )
			{
				printf(" systemcall: ftell error!\n");
				fclose(fp);
				fp = NULL;
				del_command_file();
				comman_faild(NULL);
				continue;
			}

			rewind(fp);	

			if( fileOffset > 200 )
			{
				printf("systemcall: command is too long\n");
				fclose(fp);
				fp = NULL;
				del_command_file();
				comman_faild(NULL);
				continue;
			}

			rel = fread(cmd_buf,1,fileOffset,fp);
			if( rel != fileOffset )
			{
				printf("systemcall: fread Error!\n");
				fclose(fp);
				fp = NULL;
				del_command_file();
				comman_faild(NULL);
				continue;
			}	

			fclose(fp);
			fp = NULL;

			cmd_buf[fileOffset] = '\0';

			printf("cmd_buf:%s\n",cmd_buf);			
		
			rel = system(cmd_buf);

			//printf("cmd:rel = %d\n",rel);

			//rel = my_system(cmd_buf);
			
			if( rel < 0 )
			{
				printf("systemcall: %s error = %d!\n",cmd_buf,rel);
				comman_faild(cmd_buf);	
				del_command_file();													
			}else
			{
				printf("systemcall: %s ok rel=%d!\n",cmd_buf,rel);		

				if( rel > 256 )
				{
					printf("systemcall: %s error = %d!\n",cmd_buf,rel);
					comman_faild(cmd_buf);	
				}

				del_command_file();
					
			}				

			sync();			
		}
	}
	
	 	}
	return 1;
}
