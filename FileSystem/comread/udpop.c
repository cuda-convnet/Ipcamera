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

#include "udpop.h"
#include "devfile.h"


int recv_data(UDP_SOCKET * udp_fd)
{
	struct sockaddr_in cliaddr;
	int len;
	char buf[MAX_UDP_BUF_SIZE];

	len = recvfrom(udp_fd->socket_fd, buf, MAX_UDP_BUF_SIZE, 0, (struct sockaddr *)&cliaddr,sizeof(struct sockaddr));
	if( len < 0 )
	{
		DPRINTK("recvfrom len=%d\n",len);
		close_socket(udp_fd);
		return -1;
	}

	return 1;
}

void udp_send_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	UDP_SOCKET * p_udp = NULL;

	p_udp = (UDP_SOCKET *)value;

	while(p_udp->send_th)
	{
		sleep(1);
	}
	
	return ;
}

void udp_recv_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	UDP_SOCKET * p_udp = NULL;
	

	p_udp = (UDP_SOCKET *)value;

	while(p_udp->recv_th)
	{		
        if( recv_data(p_udp) <= 0 )
			usleep(10);
	}
	
	return ;
}



void close_socket(UDP_SOCKET * udp_fd)
{
	if( udp_fd->socket_fd )
	{
		udp_fd->send_th = 0;
		udp_fd->recv_th = 0;
		udp_fd->max_buf_count = 0;	
	
		close( udp_fd->socket_fd );
		udp_fd->socket_fd = 0;

		if( udp_fd->send_buf_ptr )
		{
			free(udp_fd->send_buf_ptr);
			udp_fd->send_buf_ptr = NULL;
		}

		if( udp_fd->recv_buf_ptr )
		{
			free(udp_fd->recv_buf_ptr);
			udp_fd->recv_buf_ptr = NULL;
		}		
	}
}

int ini_udp_socket(int svr_port,int time_out,int udp_mode,int buf_count,UDP_SOCKET * udp_fd )
{
	struct sockaddr_in server_addr;
	pthread_t id_send;
	pthread_t id_recv;
	int ret;
	int i;
	struct timeval stTimeoutVal;

	memset(udp_fd,0x00,sizeof(UDP_SOCKET));

	
	udp_fd->socket_fd = socket(PF_INET,SOCK_DGRAM,0);

	if( udp_fd->socket_fd == -1 )
		return UDP_ERROR;

	if( udp_mode == UDP_SERVER )
	{
		udp_fd->mode = udp_mode;

		memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
		server_addr.sin_family=AF_INET; 
		server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 		
		server_addr.sin_port=htons(svr_port); 

		if ( -1 == (bind(udp_fd->socket_fd,(struct sockaddr *)(&server_addr),
		sizeof(struct sockaddr))))
		{
			close_socket(udp_fd);
			DPRINTK( "socket bind error\n" );			
			return UDP_ERROR;
		}	
	}

	if (setsockopt(udp_fd->socket_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{				
		DPRINTK("setsockopt SO_REUSEADDR error \n");	
		close_socket(udp_fd);
		return UDP_ERROR;
	}	

	stTimeoutVal.tv_sec = 5;
	stTimeoutVal.tv_usec = 0;
	if ( setsockopt(udp_fd->socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
    {
		DPRINTK( "setsockopt SO_SNDTIMEO error\n" );
		close_socket(udp_fd);
		return UDP_ERROR;
	}
		 	
	if ( setsockopt(udp_fd->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
    {
		DPRINTK( "setsockopt SO_RCVTIMEO error\n" );
		close_socket(udp_fd);
		return UDP_ERROR;
	}

	udp_fd->max_buf_count = buf_count;
	udp_fd->send_buf_ptr = malloc(buf_count*sizeof(UDP_BUF_ARRAY));
	if( udp_fd->send_buf_ptr == NULL )
	{
		DPRINTK( "malloc error\n");
		close_socket(udp_fd);
		return UDP_ERROR;
	}
	memset(udp_fd->send_buf_ptr,0x00,buf_count*sizeof(UDP_BUF_ARRAY));
	

	udp_fd->max_buf_count = buf_count;
	udp_fd->recv_buf_ptr = malloc(buf_count*sizeof(UDP_BUF_ARRAY));
	if( udp_fd->recv_buf_ptr == NULL )
	{
		DPRINTK( "malloc error\n");
		close_socket(udp_fd);
		return UDP_ERROR;
	}
	memset(udp_fd->recv_buf_ptr,0x00,buf_count*sizeof(UDP_BUF_ARRAY));
	

	udp_fd->send_th = 1;

	ret = pthread_create(&id_send,NULL,(void*)udp_send_thread,(void*)udp_fd);
	if ( 0 != ret )
	{
		DPRINTK( "create udp_send_thread  thread error\n");
		close_socket(udp_fd);
		return UDP_ERROR;
	}

	udp_fd->recv_th = 1;

	ret = pthread_create(&id_recv,NULL,(void*)udp_recv_thread,(void*)udp_fd);
	if ( 0 != ret )
	{
		DPRINTK( "create udp_recv_thread  thread error\n");
		close_socket(udp_fd);
		return UDP_ERROR;
	}	

	return 1;	
}



