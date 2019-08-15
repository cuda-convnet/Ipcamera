#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <linux/fd.h>
#include <endian.h>
#include <mntent.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <asm/types.h>


#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <errno.h>


#define PACKET_SIZE     4096

#define COMMAND_FILE "/tmp/command.txt"
#define COMMAND_FAILD_FILE "/tmp/command_faild.txt"
#define RM_COMMAND_FILE "rm -rf /tmp/command.txt"
#define RM_COMMAND_FAILD_FILE "rm -rf /tmp/command_faild.txt"
#define SLEEP_TIME 100000


// 效验算法
unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft=len;
    int sum=0;
    unsigned short *w=addr;
    unsigned short answer=0;
    
    while(nleft > 1)
    {
           sum += *w++;
        nleft -= 2;
    }
    
    if( nleft == 1)
    {       
        *(unsigned char *)(&answer) = *(unsigned char *)w;
           sum += answer;
    }
    
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    
    return answer;
}

// Ping函数
int net_ping( char *ips)
{
    struct timeval timeo;
    int sockfd;
    struct sockaddr_in addr;
    struct sockaddr_in from;
    
    struct timeval *tval;
    struct ip *iph;
    struct icmp *icmp;
	int timeout = 1000;

    char sendpacket[PACKET_SIZE];
    char recvpacket[PACKET_SIZE];
    
    int n;
    pid_t pid;
    int maxfds = 0;
    fd_set readfds;
    
    // 设定Ip信息
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ips);  

    // 取得socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) 
    {
        printf("ip:%s,socket error\n",ips);
        goto ping_err;
    }
    
    // 设定TimeOut时间
    timeo.tv_sec = timeout / 1000;
    timeo.tv_usec = timeout % 1000;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1)
    {
        printf("ip:%s,setsockopt error\n",ips);
        goto ping_err;
    }
    
    // 设定Ping包
    memset(sendpacket, 0, sizeof(sendpacket));
    
    // 取得PID，作为Ping的Sequence ID
    pid=getpid();
    int i,packsize;
    icmp=(struct icmp*)sendpacket;
    icmp->icmp_type=ICMP_ECHO;
    icmp->icmp_code=0;
    icmp->icmp_cksum=0;
    icmp->icmp_seq=0;
    icmp->icmp_id=pid;
    packsize=8+56;
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);
    icmp->icmp_cksum=cal_chksum((unsigned short *)icmp,packsize);

    // 发包
    n = sendto(sockfd, (char *)&sendpacket, packsize, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 1)
    {
        printf("ip:%s,sendto error\n",ips);
        goto ping_err;
    }

    // 接受
    // 由于可能接受到其他Ping的应答消息，所以这里要用循环
    while(1)
    {
        // 设定TimeOut时间，这次才是真正起作用的
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        maxfds = sockfd + 1;
        n = select(maxfds, &readfds, NULL, NULL, &timeo);
        if (n <= 0)
        {
            printf("ip:%s,Time out error\n",ips);
            goto ping_err;
        }

        // 接受
        memset(recvpacket, 0, sizeof(recvpacket));
        int fromlen = sizeof(from);
        n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen);
        if (n < 1) {
            break;
        }
        
        // 判断是否是自己Ping的回复
        char *from_ip = (char *)inet_ntoa(from.sin_addr);
        // printf("fomr ip:%s\n",from_ip);
         if (strcmp(from_ip,ips) != 0)
         {
            printf("ip:%s,Ip wang\n",ips);
            continue;
         }
        
        iph = (struct ip *)recvpacket;
    
        icmp=(struct icmp *)(recvpacket + (iph->ip_hl<<2));

       // printf("ip:%s,icmp->icmp_type:%d,icmp->icmp_id:%d\n",ips,icmp->icmp_type,icmp->icmp_id);
       // 判断Ping回复包的状态
        if (icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == pid)
        {
            // 正常就退出循环
            break;
        } 
        else
        {
            // 否则继续等
            continue;
        }
    }
    
    // 关闭socket
     if( sockfd > 0 )
     	close(sockfd);

   // printf("ip:%s,Success\n",ips);
    return 1;

ping_err:
	 if( sockfd > 0 )
     	close(sockfd);
     return -1;
}



int process_send_command(char * cmd)
{
	FILE * fp = NULL;
	int rel;
	int sleep_time;
	long fileOffset = 0;	
	char cmd_buf[200];
	int  count = 0;

	return SendM2(cmd);


	fileOffset = strlen(cmd);

	if( fileOffset > 199 )
	{
		printf(" command %s is too long!\n",cmd);
	}

LAST_COMMAD_NOT_RUN:	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);
		count++;
		if( count > 10 )
		{
			printf("LAST_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto LAST_COMMAD_NOT_RUN;
	}
	

	fp = fopen(COMMAND_FILE,"wb");
	if( fp == NULL)
		return -1;

	strcpy(cmd_buf,cmd);

	rel = fwrite(cmd_buf,1,fileOffset+1,fp);

	if( rel != fileOffset+1)
	{
		printf("fwrite error!\n");
		fclose(fp);
		fp = NULL;
		return -1;
	}


	fclose(fp);
	fp = NULL;

	//等待命令执行程序处理
	sleep_time = SLEEP_TIME*2;	
	usleep(sleep_time);

END_COMMAD_NOT_RUN:	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);
		count++;

		if( count > 10 )
		{
			//printf("END_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto END_COMMAD_NOT_RUN;
	}


	fp = fopen(COMMAND_FAILD_FILE,"rb");
	if( fp != NULL )
	{
		fclose(fp);
		fp = NULL;
		return -1;
	}

	return 1;
	
}




int command_drv(char * command)
{	
	int	res = 1;	

	//DPRINTK("command: %s 1\n",command);
	//res = system(command);	
	res = -1;
	//DPRINTK("command: %s 2\n",command);
	if(res == 0x00)
	{				
		return res;
	}
	else
	{	
		//printf(" command_drv: %s error! code=%d\n",command,res);

		if( res < 0 )
		{
			res = process_send_command(command);
			if( res > 0 )
			{
				printf(" process_send_command : %s sucess!\n",command);
				return 1;
			}else
			{
				printf(" process_send_command : %s faild!\n",command);
				return -1;
			}
		}
		return -1;
	}	
	
}



int main3(int argc,char*argv[])
{
	int count = 0;
	char cmd[800];
   while(1)
{
	if( net_ping("192.168.1.1") < 0 )
	{
		strcpy(cmd,"ifconfig eth0 down;ifconfig eth0 hw ether 00:15:E6:89:B1:53;ifconfig eth0 192.168.1.100 netmask 255.255.255.0;ifconfig eth0 up;route add default gw 192.168.1.1;");
		command_drv(cmd);		
	}

	count++;

if( count % 1000  == 0)
{
	printf("count = %d\n",count);
}
   }
}

typedef long long ext2_loff_t;


static ext2_loff_t my_llseek (unsigned int fd, ext2_loff_t offset,
		unsigned int origin)
{
	ext2_loff_t result;
	int retval;

	retval = _llseek (fd, ((unsigned long long) offset) >> 32,
			((unsigned long long) offset) & 0xffffffff,
			&result, origin);

	
	return (retval == -1 ? (ext2_loff_t) retval : result);
}



int main2(int argc,char*argv[])
{
	int count = 0;
	char cmd[800];
	int fd;
	long long offset;
	int whence;


	offset = (long long)1024*1024*1024*180;


	
	fd = open("/dev/sda5",O_RDWR, 0666 );

	if( fd < 0 )
	{
		printf("open error!\n");
		return -1;
	}


	//if( my_llseek(fd, offset, SEEK_SET) == -1 )
	if( lseek(fd, offset, SEEK_SET) == -1 )
	{
		printf("seek error!\n");
		perror(strerror(errno));
		goto f_error;
	}

	cmd[0] = 0;
	cmd[1] = 0;
	cmd[2] = 0;
	cmd[3] = 0;
	

	if( read(fd, cmd, 4) != 4 )
	{
		printf("read error!\n");
		goto f_error;
	}

	printf("%d %d %d %d\n",cmd[0],cmd[1],cmd[2],cmd[3]);

	close(fd);
	fd = 0;


	fd = open("/dev/sda5",O_RDWR|O_CREAT|O_TRUNC, 0666 );

	if( fd < 0 )
	{
		printf("open error!\n");
		return -1;
	}
	

	printf("offset = %lld  fd = %d\n",offset,fd);

	//if( my_llseek(fd, offset, SEEK_SET) == -1 )
	if( lseek(fd, offset, SEEK_SET) == -1 )
	{
		printf("seek error!\n");
		perror(strerror(errno));
		goto f_error;
	} 
	
	

	cmd[0] = 9;
	cmd[1] = 10;
	cmd[2] = 11;
	cmd[3] = 12;
	

	if( write (fd, cmd, 4) != 4 )
	{
		printf("write error!\n");
		goto f_error;
	}

	close(fd);
	fd = 0;


 f_error:
	return -1;
  
}


int main(int argc,char*argv[])
{
	int count = 0;
	char cmd[800];
	int fd;
	long long offset;
	int whence;
	char * buf;
	char  * phy_buf;
	int pagesize;
	int num = 0;
	
	pagesize = getpagesize();



	offset = (long long)1024*1024*1024*180;


	buf = (char*)malloc(1024*1024*2);

	if( buf == NULL )
	{
		printf("open error!");
		return 1;
	}	
	
	phy_buf = (char*)((((int) buf+pagesize-1)/pagesize)*pagesize);
//	phy_buf = buf;

	printf("addr: %x %x\n",buf,phy_buf);
	
	fd = open("/dev/sda5",O_RDWR, 0666 );

	if( fd < 0 )
	{
		printf("open error!\n");
		return -1;
	}

	if( lseek(fd, offset, SEEK_SET) == -1 )
	{
		printf("seek error!\n");
		perror(strerror(errno));
		goto f_error;
	}

	for(num = 0; num < 1024;num++)
	{
		if( read (fd, phy_buf, 1024*1024) != 1024*1024)
		{
			printf("write error!\n");
			goto f_error;
		}		

		printf("num= %d\n",num);
	}

	close(fd);
	fd = 0;



 f_error:
	return -1;
  
}



