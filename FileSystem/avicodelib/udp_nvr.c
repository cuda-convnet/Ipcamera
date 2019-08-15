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
 
#include "udp_nvr.h"
#include "udp_srv.h"

#include "devfile.h"

#define FAB_MAX_NVR_ARRAY_NUM (100000)
#define FAB_CONNECT_PORT_START (13000)
#define FAB_CONNECT_PORT_END (15000)
#define CROSS_TIME_NUM (30)
#define CROSS_BETWEEN_TIME (100)

static pthread_mutex_t get_availble_port_mutex;
static int fab_connect_port = FAB_CONNECT_PORT_START;
static char udp_server_ip[100] = {0};
static char udp_server_real_ip[100] = {0};
static char udp_mac_name[100] = {0};
static int nvr_local_port = 0;
static int nvr_get_server_answer_num = 0;

int get_server_answer_block()
{
	static int answer_num = 0;
	static int lost_num = 0;

	if( answer_num != nvr_get_server_answer_num)
	{
		answer_num = nvr_get_server_answer_num;
		lost_num = 0;
	}

	lost_num++;

	if( lost_num > 3 )
	{
		lost_num = 0;
		return 1;
	}

	return 0;
}

int check_port_availble(int local_port)
{
	int socket_fd = -1;
	int ret;
	struct sockaddr_in local;
	int fromlen = sizeof(local);

	
	socket_fd = socket(AF_INET,SOCK_DGRAM,0);
	if (socket_fd == -1) 
	{ 
		DPRINTK("check_port_availble: socket create err\n"); 
		goto err_step;
	}

	local.sin_family=AF_INET;
	local.sin_port=htons(local_port);  ///监听端口
	local.sin_addr.s_addr=INADDR_ANY;       ///本机
	
	ret = bind(socket_fd,(struct sockaddr*)&local,fromlen);
	if( ret == - 1)
	{				
		DPRINTK("check_port_availble: socket bind err\n"); 
		goto err_step;
	}	

	close(socket_fd);

	return 1;

err_step:

	if( socket_fd > 0 )
		close(socket_fd);

	return -1;
		
}

int fab_get_availble_port()
{
	int port;
	int try_num = 0;
	static int first = 0;

	if(first == 0 )
	{
		first = 1;
		pthread_mutex_init(&get_availble_port_mutex,NULL);	
	}

	pthread_mutex_lock( &get_availble_port_mutex);

retry:
	
	port = fab_connect_port;

	fab_connect_port++;

	if( fab_connect_port > FAB_CONNECT_PORT_END)
		fab_connect_port = FAB_CONNECT_PORT_START;	


	if( check_port_availble(port) < 0 )
	{
		try_num++;

		if( try_num < 50 )
			goto retry;
		else
			port = -1;
	}

	pthread_mutex_unlock( &get_availble_port_mutex);

	return port;
}


int fab_nvr_send_data_to_client(FAB_UDP_CMD_ST * pCmd,FAB_UDP_SOCKET * pSocket)
{
	int i;
	FAB_UDP_CMD_ST cmd;
	cmd = *pCmd;
	cmd.cmd = NET_NVR_SEND_DATA_TO_CLIENT;	
	
	fab_SendUdp2(cmd.source_ip,cmd.source_port,(char*)&cmd,sizeof(cmd),pSocket);	
		
	DPRINTK("mac:%s ip:%s port:%d fab_nvr_send_data_to_client\r\n",pCmd->nvr_mac_id,cmd.source_ip,cmd.source_port);		
	
	return 1;
}

int fab_nvr_send_alive(FAB_UDP_CMD_ST * pCmd,FAB_UDP_SOCKET * pSocket)
{
	FAB_UDP_CMD_ST cmd;
	cmd = *pCmd;
	cmd.verify_flag = VERIFY_FLAG;
	cmd.cmd = NET_ALIVE;	
	//cmd.array_id = -1;	

	//DPRINTK("%s:%d  array_id=%d\n",cmd.dst_ip,cmd.dst_port,cmd.array_id );
	fab_SendUdp2(cmd.dst_ip,cmd.dst_port,(char*)&cmd,sizeof(cmd),pSocket);

	return 1;
}

int fab_nvr_send_data_to_srv_new_port(FAB_UDP_CMD_ST * pCmd,FAB_UDP_SOCKET * pSocket)
{
	FAB_UDP_CMD_ST cmd;
	cmd = *pCmd;
	cmd.cmd = NET_NVR_CONNECT_SERVER_NEW_PORT;	
	
	fab_SendUdp2(cmd.dst_ip,cmd.dst_port,(char*)&cmd,sizeof(cmd),pSocket);		
	
	return 1;
}

int fab_nvr_recv_call_back(char * buf,int len, void * p,void * psocket)
{
	struct sockaddr_in * pfrom;
	FAB_UDP_SOCKET * pSocket = (FAB_UDP_SOCKET*)psocket;
	FAB_UDP_CMD_ST * pCmd = (FAB_UDP_CMD_ST*)buf;
	FAB_UDP_CMD_ST * pSelfCmd = NULL;

	pfrom = (struct sockaddr_in *)p;

	pSelfCmd = (FAB_UDP_CMD_ST*)pSocket->self_data;

	if( pCmd->verify_flag != VERIFY_FLAG )
		return 0;

	switch(pCmd->cmd)
	{		
		case NET_ALIVE:
			{					
				//*pSelfCmd = *pCmd;	

				pSelfCmd->array_id = pCmd->array_id;

				nvr_get_server_answer_num++;
					
				//DPRINTK("mac:%s ip:%s port:%d id=%d NET_ALIVE %d\r\n",
				//	pCmd->nvr_mac_id,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port),pCmd->array_id,nvr_get_server_answer_num);						

				
				
			}
			break;
		case NET_NVR_CONNECT_SERVER_NEW_PORT:
			{				
				//*pSelfCmd = *pCmd;

				//fab_nvr_start_udp_cross_th(pCmd);
					
				DPRINTK("mac:%s ip:%s port:%d NET_NVR_CONNECT_SERVER_NEW_PORT\r\n",pCmd->nvr_mac_id,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));						

				{
					char cmdbuf[200];
				//	sprintf(cmdbuf,"/mnt/mtd/appserver %d %d &",pSocket->port,nvr_local_port);
					sprintf(cmdbuf,"/mnt/mtd/appserver %s %d %d &",inet_ntoa(pfrom->sin_addr),pCmd->server_port,nvr_local_port);
					DPRINTK("cmd:%s\n",cmdbuf);
					system(cmdbuf);
				}
			}
			break;		
		default:
			break;
	}

	return 1;
}



int fab_nvr_recv_call_back2(char * buf,int len, void * p,void * psocket)
{
	struct sockaddr_in * pfrom;
	FAB_UDP_SOCKET * pSocket = (FAB_UDP_SOCKET*)psocket;
	FAB_UDP_CMD_ST * pCmd = (FAB_UDP_CMD_ST*)buf;
	FAB_UDP_CMD_ST * pSelfCmd = NULL;

	pfrom = (struct sockaddr_in *)p;

	pSelfCmd = (FAB_UDP_CMD_ST*)pSocket->self_data;

	if( pCmd->verify_flag != VERIFY_FLAG )
		return 0;

	switch(pCmd->cmd)
	{
		
		case NET_SEND_DATA_NVR_OR_CLIENT:
			{
				*pSelfCmd = *pCmd;

				fab_nvr_send_data_to_client( pSelfCmd,pSocket);
				
				DPRINTK("mac:%s ip:%s port:%d NET_SEND_DATA_NVR_OR_CLIENT\r\n",pCmd->nvr_mac_id,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));						
						
			}
			break;
		case NET_CLIENT_SEND_DATA_TO_NVR:
			{	
									
				DPRINTK("mac:%s ip:%s port:%d NET_CLIENT_SEND_DATA_TO_NVR\r\n",pCmd->nvr_mac_id,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));						
				
			}
			break;		
		case NET_NVR_START_UDP_LISTEN:
			{
				DPRINTK("mac:%s ip:%s port:%d NET_NVR_START_UDP_LISTEN\r\n",pCmd->nvr_mac_id,inet_ntoa(pfrom->sin_addr),ntohs(pfrom->sin_port));						
				if( pSocket->socket_fd > 0 )
				{
					close(pSocket->socket_fd);
					pSocket->socket_fd = 0;
				} 
				
				
				pSocket->m_hExit = 0;
				DPRINTK("connect th out\n");
			
				
				{
					char cmdbuf[200];
				//	sprintf(cmdbuf,"/mnt/mtd/appserver %d %d &",pSocket->port,nvr_local_port);
					sprintf(cmdbuf,"/mnt/mtd/appserver %d %d %s %d &",pSocket->port,nvr_local_port,pCmd->source_ip,pCmd->source_port);
					DPRINTK("cmd:%s\n",cmdbuf);
					SendM2(cmdbuf);
				}

						

				
			}
			break;
		case NET_CLIENT_START_UDP_CONNECT:
			break;			
		default:
			break;
	}

	return 1;
}



int Fab_nvr_start_udp_cross_func(void * lpParam)
{
	SET_PTHREAD_NAME(NULL);
	FAB_UDP_CMD_ST * pCmd = (FAB_UDP_CMD_ST*)lpParam;
	FAB_UDP_CMD_ST cmd;
	FAB_UDP_SOCKET udp_socket;	

	cmd = *pCmd;

	pCmd->cmd = 0;

	DPRINTK("in \n");

	udp_socket.port =  fab_get_availble_port();	

	if( udp_socket.port < 0 )
	{
		DPRINTK("no avalible port use err\n");
		return -1;
	}

	DPRINTK("port = %d\n",udp_socket.port);

	udp_socket.self_data  = &cmd;

	udp_socket.recv_call_back = fab_nvr_recv_call_back2;
	
	fab_udp_start_recv_th(&udp_socket);

	usleep(10000);

	if( udp_socket.open_socket == 0 )
	{
		DPRINTK("fab_udp_start_recv_th error\n");
		return -1;
	}

	strcpy(cmd.dst_ip,udp_server_real_ip);
	cmd.dst_port = pCmd->server_port;
	DPRINTK("send port=%d \n",cmd.dst_port );
	fab_nvr_send_data_to_srv_new_port( &cmd,&udp_socket);	

	sleep(10);

	DPRINTK("out \n");
	return 1;

}


int fab_nvr_start_udp_cross_th(FAB_UDP_CMD_ST * pCmd)
{	
	pthread_t netrecv_id;
	int ret;
	int count = 0;

	ret = pthread_create(&netrecv_id,NULL,(void*)Fab_nvr_start_udp_cross_func,(void *)pCmd);
	if ( 0 != ret )
	{
		
		DPRINTK( "create Fab_nvr_start_udp_cross_func thread error\n");	
		return -1;
	}

	while(pCmd->cmd != 0)
	{
		sleep(1);
		count++;

		if( count > 3 )
		{
			DPRINTK( "create Fab_nvr_start_udp_cross_func thread error 2\n");	
			return -1;
		}
	}	
 
	return 1;	
}

static void udp_get_host_name(char * pServerIp,char * buf)
{

	int a;
	struct hostent *myhost;
	char ** pp;
	struct in_addr addr;
	int is_url = 0;
	
	for(a = 0; a < 8;a++)
	{
		if( pServerIp[a] == (char)NULL )
			break;

		if( (pServerIp[a] < '0' || pServerIp[a] > '9') 
					&& (pServerIp[a] != '.')  )
		{
			is_url = 1;
			
			 DPRINTK("udp pServerIp=%s\n",pServerIp);
			//不知道什么原因解析不了域名。
			myhost = gethostbyname(pServerIp); 

			if( myhost == NULL )
			{
				DPRINTK("gethostbyname can't get %s ip\n",pServerIp);
				//herror(pServerIp);
			/*
				printf("use ping to get ip\n");
				get_host_name_by_ping(pServerIp,buf);
				if( buf[0] == 0 )
					printf("get_host_name_by_ping also can't get %s ip!\n",pServerIp);
			*/
				 break;
			}
			 
			DPRINTK("host name is %s\n",myhost->h_name);		 
		 
			//for (pp = myhost->h_aliases;*pp!=NULL;pp++)
			//	printf("%02X is %s\n",*pp,*pp);
			
			pp = myhost->h_addr_list;
			while(*pp!=NULL)
			{
				addr.s_addr = *((unsigned int *)*pp);
				DPRINTK("address is %s\n",inet_ntoa(addr));
				sprintf(buf,"%s",inet_ntoa(addr));
				pp++;
			}
			
			break;
		}
	}

	if( is_url == 0 )
	{
		strcpy(buf,pServerIp);

		DPRINTK("is not url ,set ip buf %s\n",buf);
	}
}


int nvr_start()
{
	FAB_UDP_SOCKET udp_socket1;	
	FAB_UDP_CMD_ST cmd;
	int count = 0;
	char ip_buf[200];
	int get_host_name_num = 0;

	udp_socket1.port = fab_get_availble_port();		

	udp_socket1.self_data  = &cmd;

	udp_socket1.recv_call_back = fab_nvr_recv_call_back;
	
	fab_udp_start_recv_th(&udp_socket1);

	DPRINTK("udp port:%d\n",udp_socket1.port);

	usleep(100000);

	if( udp_socket1.open_socket == 0 )
	{
		DPRINTK("fab_udp_start_recv_th error\n");
		return -1;
	}
	
	memset(ip_buf,0,200);
	while(1)
	{ 		
		if( get_host_name_num % 20 == 0 )
		{
			udp_get_host_name(udp_server_ip,ip_buf);	
			if( ip_buf[0] == 0 )
			{
				sleep(UDP_CONNECT_ALIVE_TIME);
				continue;
			}
		}

		get_host_name_num++;

		strcpy(udp_server_real_ip,ip_buf);
		
		strcpy(cmd.dst_ip,ip_buf);
		cmd.dst_port = SVR_LISTEN_UDP_PORT;
		strcpy(cmd.nvr_mac_id,udp_mac_name);
		fab_nvr_send_alive( &cmd,&udp_socket1);
		sleep(UDP_CONNECT_ALIVE_TIME);

		if( get_server_answer_block() == 1 )
		{
			DPRINTK("Can't get server answer\n");
			break;
		}
	}

	fab_udp_stop_recv_th(&udp_socket1);

	return 1;

}

int start_udp_server_func(void * lpParam)
{
	SET_PTHREAD_NAME(NULL);
	while(1)
	{
		nvr_start();
	}
}

int start_udp_server(char * server_ip,char * mac_name,int local_port)
{
	pthread_t netrecv_id;
	int ret;
	int count = 0;
	static int first = 0;

	strcpy(udp_server_ip,server_ip);
	strcpy(udp_mac_name,mac_name);
	nvr_local_port = local_port;
	DPRINTK("%s %s %d\n",udp_server_ip,udp_mac_name,nvr_local_port);	

	if(first > 0)
		return 1;	
	
	ret = pthread_create(&netrecv_id,NULL,(void*)start_udp_server_func,(void *)NULL);
	if ( 0 != ret )
	{
		
		DPRINTK( "create start_udp_server_func thread error\n");	
		return -1;
	}

	first = 1;
	
	return 1;
}



