#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <linux/videodev.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
 #include <pthread.h>
 
#include "udp_nvr.h"

#include "devfile.h"

#define TRACE DPRINTK



int fab_SendUdp3(char * ip,int port,char * buf,int length,int local_port)
{
	char * host_name = ip;	
	char * pVersion = NULL;
	int so_broadcast;
	char server_ip[100];
	char mac_id[100];
	int mac_hex_id = 0;
	int * pmac_hex_id = NULL;
	struct sockaddr_in serv_addr; 
	#ifdef WIN32
	SOCKET sockfd = 0;
	BOOL bReuseaddr=TRUE;	
	#else
	int  sockfd = 0;
	int  bReuseaddr=1;
	#endif
	int ret;
	
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen =sizeof(from);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) 
	{ 
		perror("Error: socket"); 
		goto send_error;
	}

	#ifdef WIN32
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr,sizeof(BOOL)) < 0) 
	{			
		TRACE("setsockopt SO_REUSEADDR error \n");		
		goto send_error;
	}
	#else
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr,sizeof(int)) < 0) 
	{			
		TRACE("setsockopt SO_REUSEADDR error \n");		
		goto send_error;
	}
	#endif

	if( strcmp(ip,"255.255.255.255") == 0 )
	{
		#ifdef WIN32
		 BOOL bBroadcast = TRUE;
		 int num = 0; 		
		 int ret_val = setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,( const char* )&bBroadcast,sizeof(bBroadcast));
		 if(ret_val == -1)
		 {
			 TRACE("Failed in setsockfd \n");
			goto send_error;
		 } 
		 #else
		  int bBroadcast = 1;
		 int num = 0; 		
		 int ret_val = setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&bBroadcast,sizeof(bBroadcast));
		 if(ret_val == -1)
		 {
			 TRACE("Failed in setsockfd \n");
			goto send_error;
		 } 
		 #endif
	}

	
	local.sin_family=AF_INET;
	local.sin_port=htons(local_port);  ///监听端口
	local.sin_addr.s_addr=INADDR_ANY;       ///本机

	ret = bind(sockfd,(struct sockaddr*)&local,fromlen);
	if( ret == -1 )
	{
		TRACE("bind to error\n");
		goto send_error;
	}

	
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr.s_addr = inet_addr(host_name); 	
	

	ret = sendto(sockfd,buf,length,0,(struct sockaddr*)&serv_addr,fromlen);		

	if( ret <= 0 )
	{
		TRACE("send to error\n");
		goto send_error;
	}
	
	if( sockfd > 0 )
	{
		close(sockfd);
		sockfd = 0;
	}

	return 1;

send_error:
	
	if( sockfd > 0 )
	{
		close(sockfd);
		sockfd = 0;
	}


	return -1;
}


int fab_SendUdp2(char * ip,int port,char * buf,int length,FAB_UDP_SOCKET * pSocket)
{
	char * host_name = ip;	
	char * pVersion = NULL;
	int so_broadcast;
	char server_ip[100];
	char mac_id[100];
	int mac_hex_id = 0;
	int * pmac_hex_id = NULL;
	struct sockaddr_in serv_addr; 
	int ret;
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen =sizeof(from);	

	
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr.s_addr = inet_addr(ip); 		

	ret = sendto(pSocket->socket_fd,buf,length,0,(struct sockaddr*)&serv_addr,fromlen);		

	if( ret <= 0 )
	{
		TRACE("send to error\n");
		goto send_error;
	}

	return 1;

send_error:

	return -1;
}

#ifdef WIN32
DWORD WINAPI Fab_NetUdpRecvThread(LPVOID lpParam)
{
	SET_PTHREAD_NAME(NULL);
	FAB_UDP_SOCKET * pUdpSocket;
	char buffer[UDP_DATA_LENGTH];
	int ret=0;
	BOOL bReuseaddr=TRUE;
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen = 0;
	int nNetTimeout=1000;// 1秒


	pUdpSocket = (FAB_UDP_SOCKET *)lpParam;		
	
	fromlen =sizeof(from);
	local.sin_family=AF_INET;
	local.sin_port=htons(pUdpSocket->port);  ///监听端口
	local.sin_addr.s_addr=INADDR_ANY;       ///本机

	pUdpSocket->socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	bind(pUdpSocket->socket_fd,(struct sockaddr*)&local,fromlen);
	

	if (setsockopt(pUdpSocket->socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr,sizeof(BOOL)) < 0) 
	{		
		closesocket( pUdpSocket->socket_fd );
		pUdpSocket->socket_fd = 0;
		TRACE("setsockopt SO_REUSEADDR error \n");		
		return -1;	
	}		

	if(setsockopt( pUdpSocket->socket_fd ,SOL_SOCKET,SO_SNDTIMEO,(char *)&nNetTimeout,sizeof(int)) < 0 )
	{
		closesocket( pUdpSocket->socket_fd );
		pUdpSocket->socket_fd = 0;
		TRACE( "listen socket  SO_SNDTIMEO  error\n" );
		return -1;
	}

	if(setsockopt( pUdpSocket->socket_fd ,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int))< 0 )
	{
		closesocket( pUdpSocket->socket_fd );
		pUdpSocket->socket_fd = 0;
		TRACE( "listen socket  SO_SNDTIMEO  error\n" );
		return -1;
	}

	pUdpSocket->open_socket = 1;

	while (WaitForSingleObject(pUdpSocket->m_hExit,0)!=WAIT_OBJECT_0)
	{
		memset(buffer,0x00,UDP_DATA_LENGTH);
		
		ret = recvfrom(pUdpSocket->socket_fd,buffer,UDP_DATA_LENGTH,0,(struct sockaddr*)&from,&fromlen);

		if( ret > 0 )
		{		
			pUdpSocket->recv_call_back(buffer,ret,&from,pUdpSocket);			
		}
	}

	closesocket(pUdpSocket->socket_fd);
	pUdpSocket->socket_fd = 0;
 
  return 0;
}

int  fab_udp_stop_recv_th(FAB_UDP_SOCKET * pSocket)
{
	SetEvent(pSocket->m_hExit);

	if(WaitForSingleObject(pSocket->m_NetUdpRecvHandle,3000)==WAIT_TIMEOUT)
	{
		 //AfxMessageBox("close m_NetCheckHandle thread by force!");
		 TerminateThread(pSocket->m_NetUdpRecvHandle,0);
	}
	CloseHandle(pSocket->m_NetUdpRecvHandle);

	CloseHandle(pSocket->m_hExit);

	return 0;
}

int fab_udp_start_recv_th(FAB_UDP_SOCKET * pSocket)
{
	DWORD       dwThreadId3;	

	pSocket->m_hExit=CreateEvent(NULL,TRUE,FALSE,NULL);

	ResetEvent(pSocket->m_hExit);

	pSocket->m_NetUdpRecvHandle = CreateThread(NULL, 0,  Fab_NetUdpRecvThread,(LPVOID)pSocket, 0, &dwThreadId3); 
	if( pSocket->m_NetUdpRecvHandle == NULL)
		goto START_FAILD;

	return 0;

START_FAILD:
	 fab_udp_stop_recv_th(pSocket);
	return -1;	
}
#else

int Fab_NetUdpRecvThread(void * lpParam)
{
	SET_PTHREAD_NAME(NULL);
	FAB_UDP_SOCKET * pUdpSocket;
	char buffer[UDP_DATA_LENGTH];
	int ret=0;
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen = 0;
	int nNetTimeout=1000;
	int i;
	
	pUdpSocket = (FAB_UDP_SOCKET *)lpParam;		
	
	fromlen =sizeof(from);
	local.sin_family=AF_INET;
	local.sin_port=htons(pUdpSocket->port);  ///监听端口
	local.sin_addr.s_addr=INADDR_ANY;       ///本机

	pUdpSocket->socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	
	ret = bind(pUdpSocket->socket_fd,(struct sockaddr*)&local,fromlen);
	if( ret == - 1)
	{		
		close( pUdpSocket->socket_fd );
		pUdpSocket->socket_fd = 0;
		TRACE("setsockopt bind error \n");		
		return -1;	
	}	
	
	/*
	i = 1;
	if (setsockopt(pUdpSocket->socket_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{				
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		return -1; 
	}	*/


	{	
		struct timeval stTimeoutVal;	

		stTimeoutVal.tv_sec = 0;
		stTimeoutVal.tv_usec = 10000;
		if ( setsockopt( pUdpSocket->socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	    	{				
			DPRINTK("setsockopt SO_SNDTIMEO error \n");		
			return -1; 
		}	

			 	
		if ( setsockopt( pUdpSocket->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	    	{				
			DPRINTK("setsockopt SO_RCVTIMEO error \n");		
			return -1; 
		}
	}



	pUdpSocket->open_socket = 1;

	while (pUdpSocket->m_hExit)
	{
		memset(buffer,0x00,UDP_DATA_LENGTH);
		
		ret = recvfrom(pUdpSocket->socket_fd,buffer,UDP_DATA_LENGTH,0,(struct sockaddr*)&from,&fromlen);

		if( ret > 0 )
		{		
			pUdpSocket->recv_call_back(buffer,ret,&from,pUdpSocket);			
		}
	}

	if( pUdpSocket->socket_fd > 0 )
	{
		close(pUdpSocket->socket_fd);
		pUdpSocket->socket_fd = 0;
	}

	pUdpSocket->open_socket = 0;

	pthread_detach(pthread_self());
 
  return 0;
}

int  fab_udp_stop_recv_th(FAB_UDP_SOCKET * pSocket)
{
	pSocket->m_hExit = 0;	

	if( pSocket->open_socket == 1 )
	{
		sleep(1);		
	}

	if( pSocket->socket_fd > 0)
	{
		close(pSocket->socket_fd);
		pSocket->socket_fd = 0;
	}	

	pSocket->open_socket = 0;

	return 0;
}

int fab_udp_start_recv_th(FAB_UDP_SOCKET * pSocket)
{	
	pthread_t netrecv_id;
	int ret;

	pSocket->m_hExit = 1;

	ret = pthread_create(&pSocket->netrecv_id,NULL,(void*)Fab_NetUdpRecvThread,(void *)pSocket);
	if ( 0 != ret )
	{
		
		DPRINTK( "create Fab_NetUdpRecvThread thread error\n");	
		goto START_FAILD;
	}

	return 0;

START_FAILD:
	 fab_udp_stop_recv_th(pSocket);
	return -1;	
}

#endif

