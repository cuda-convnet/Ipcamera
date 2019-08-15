#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>


#include <errno.h>
#include "local_socket.h"
#include "yun_base.h"
#include "yun_svr.h"
#include "netserver.h"

#include "devfile.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

#define debug_log (1)

#define YS_SERVER_PORT (11888)

static LOCAL_SOCKET  main_lsock;
static LOCAL_SOCKET  cli_lsock;

static int client_listen_port = CLIENT_START_PORT;
static pthread_mutex_t get_availble_port_mutex;
static int bind_port = 22999;
static int yun_transmit_port = 22266;
static int open_yun_flag = 0;
static int is_connect_yun_svr = 0;


int net_command_send_data_direct(int  client_socket, char *pBuf, int ulLen)
{
	return ls_socket_send_data(client_socket,pBuf,ulLen);
}

int net_command_recv_data_direct(int  client_socket,char * buf,int max_length,int time_out)
{
	fd_set fdread,watch_set;	
	struct timeval tv = {1, 0};
	int ret;
	int result = 1;
	int time_out_count = time_out;


	while( 1 )
	{
		FD_ZERO( &watch_set);
		FD_SET( client_socket, &watch_set);

		ret = select(client_socket+1, &watch_set, NULL, NULL, &tv);

		if (ret == 0)
		{		  
		  time_out_count--;
		  if( time_out_count < 0 )
		  {
				DPRINTK("recv timeout\n");
				result = LS_SOCKET_TIME_OUT_ERR;
				break;
		  }
		  continue;
		}

		if( ret < 0 )
		{		
			goto END_WORK;
		}
	  
		// accept socket
		if (ret > 0 && FD_ISSET(client_socket, &watch_set))
		{			
			result = ls_socket_read_data(client_socket,buf,max_length);
			if( result < 0 )
			{				
				result =LS_SOCKET_READ_ERR;
				goto END_WORK;
			}else if( result == sizeof(int)) 
			{
			}else
			{
				break;
			}
		}	
	}
	

END_WORK:
	return result;
}


int ys_get_yun_transmit_port()
{
	return yun_transmit_port;
}


int ys_check_port_availble(int local_port)
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
	socket_fd = -1;


	socket_fd = socket(AF_INET,SOCK_STREAM,0);
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
	socket_fd = -1;

	return 1;

err_step:

	if( socket_fd > 0 )
		close(socket_fd);

	return -1;
		
}


int ys_get_availble_port()
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
	
	port = client_listen_port;

	client_listen_port++;

	if( client_listen_port >= (CLIENT_START_PORT+MAX_CLIENT_NUM))
		client_listen_port = CLIENT_START_PORT;	


	if( ys_check_port_availble(port) < 0 )
	{
		try_num++;

		if( try_num < 100 )
			goto retry;
		else
			port = -1;
	}

	pthread_mutex_unlock( &get_availble_port_mutex);


	return port;
}




int ys_send_data(LOCAL_SOCKET * pLsocket,int cmd,char * send_buf, int buf_len)
{
	YUN_CMD_ST * cmd_st = NULL;
	char * tmp = NULL;	
	int ret = 0;

	tmp = (char *)malloc(sizeof(YUN_CMD_ST)+buf_len);
	if( tmp == NULL )
		goto err;

	cmd_st = (YUN_CMD_ST *)tmp;
	cmd_st->flag = MAC_ID;
	cmd_st->cmd = cmd;
	cmd_st->buf_len = buf_len;

	memcpy(tmp+sizeof(YUN_CMD_ST),send_buf,buf_len);	

	ret = ls_send_data(pLsocket,tmp,sizeof(YUN_CMD_ST)+buf_len);
	
	free(tmp);

	return ret;

err:

	if( tmp )	
		free(tmp);
	
	return -1;
}


static int ys_tcp_connect(char * ip, int port,int * tcp_socket_fd)
{

	int a;
	int socket_fd = -1;
	int ret;
	struct sockaddr_in addr;	
	
	ret = ls_socket_create_set(&socket_fd,0,8,1,0,0);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		return ret;
	}
	
	
	addr.sin_family = AF_INET;	
  	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);	

	DPRINTK("socket_fd=%d ip=%s port=%d\n",socket_fd,ip,port);
	
	if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
		perror(strerror(errno));	
		DPRINTK("connect error !\n");		
		goto connect_error;
	 }else
	 {
	 	DPRINTK("success: fd=%d\n",socket_fd);
	 }


	*tcp_socket_fd = socket_fd;


	return 1;

connect_error:

	if( socket_fd > 0 )
	{
		close(socket_fd);
		socket_fd = -1;
	}

	return -1;
	
}


static int ys_tcp_st_connect(char * ip, int port,YS_TCP_CONNECT_ST * tcp_st)
{

	int i = 0;
	int ret = 0;
	char ip_buf[100];
	strcpy(ip_buf,ip);

	if(  tcp_st->need_tcp_connect_socket_num > YS_MAX_TCP_SOCKET_NUM )
	{
		DPRINTK("need_tcp_connect_socket_num=%d > YS_MAX_TCP_SOCKET_NUM \n",tcp_st->need_tcp_connect_socket_num);
		return -1;
	}

	for( i = 0; i < tcp_st->need_tcp_connect_socket_num; i++)
	{
		ret = ys_tcp_connect(ip_buf,port,&tcp_st->tcp_socket[i]);
	}

	return 1;

connect_error:

	for( i = 0; i < tcp_st->need_tcp_connect_socket_num; i++)
	{
		if( tcp_st->tcp_socket[i] > 0 )
		{
			close(tcp_st->tcp_socket[i]);
			tcp_st->tcp_socket[i] = -1;
		}
	}
	return -1;
	
}

static int ys_tcp_connect_yun_svr(char * ip, int port,int * tcp_socket_fd,char * mac_id,int socket_id)
{

	int a;
	int socket_fd = -1;
	int ret;
	struct sockaddr_in addr;
	YUN_SERVER_HEAD_ST head_st;
	
	ret = ls_socket_create_set(&socket_fd,0,8,1,0,0);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		return ret;
	}
	
	
	addr.sin_family = AF_INET;	
  	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);	

	DPRINTK("socket_fd=%d ip=%s port=%d  mac_id=%s\n",socket_fd,ip,port,mac_id);
	
	if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
		perror(strerror(errno));	
		DPRINTK("connect error !\n");		
		goto connect_error;
	 }

	head_st.flag = MSG_YUN_HEAD_FLAG;
	strcpy(head_st.mac_id,mac_id);
	head_st.socket_id = socket_id;

	ret = net_command_send_data_direct(socket_fd,(char*)&head_st,sizeof(YUN_SERVER_HEAD_ST));
	if( ret < 0 )
	{			
		DPRINTK("net_command_send_data_direct err\n");
		goto connect_error;
	}

	ret = net_command_recv_data_direct(socket_fd,(char *)&head_st,sizeof(YUN_SERVER_HEAD_ST),5);
	if( ret < 0 )
	{
		DPRINTK("net_command_recv_data_direct err\n");
		goto connect_error;
	}

	if(head_st.socket_id != 1 || head_st.flag != MSG_YUN_HEAD_FLAG )
	{
		DPRINTK("%d %x err\n",head_st.socket_id,head_st.flag);
		goto connect_error;
	}


	*tcp_socket_fd = socket_fd;


	return 1;

connect_error:

	if( socket_fd > 0 )
	{
		close(socket_fd);
		socket_fd = -1;
	}

	return -1;
	
}


static int ys_tcp_st_connect_yun_svr(char * ip, int port,YS_TCP_CONNECT_ST * tcp_st)
{

	int i = 0;
	int ret = 0;
	char ip_buf[100];

	strcpy(ip_buf,ip);

	if(  tcp_st->need_tcp_connect_socket_num > YS_MAX_TCP_SOCKET_NUM )
	{
		DPRINTK("need_tcp_connect_socket_num=%d > YS_MAX_TCP_SOCKET_NUM \n",tcp_st->need_tcp_connect_socket_num);
		return -1;
	}

	for( i = 0; i < tcp_st->need_tcp_connect_socket_num; i++)
	{
		ret = ys_tcp_connect_yun_svr(ip_buf,port,&tcp_st->tcp_socket[i],tcp_st->mac_id,i);
		if( ret <  0 )
			goto connect_error;
	}

	return 1;

connect_error:

	for( i = 0; i < tcp_st->need_tcp_connect_socket_num; i++)
	{
		if( tcp_st->tcp_socket[i] > 0 )
		{
			close(tcp_st->tcp_socket[i]);
			tcp_st->tcp_socket[i] = 0;
		}
	}
	return -1;
	
}


static int svr_recv_data(void * plsocket,char * buf,int length)
{
	int last_num = 0;
	int count_num = 0;
	int ret = 0;
	YUN_CMD_ST * cmd_st = NULL;
	LOCAL_SOCKET * pLsock = (LOCAL_SOCKET *)plsocket;	

	#ifdef debug_log
	DPRINTK("(%s:%d)recv: len=%d \n",pLsock->connect_addr,pLsock->connect_port,length);
	#endif

	cmd_st = (YUN_CMD_ST * )buf;
	if( cmd_st->flag != MAC_ID )
	{
		DPRINTK("unknow data\n");
		return -1;
	}

	switch(cmd_st->cmd)
	{
		case YS_GET_NVR_ALL_INFO:
			{
				UDP_MSG msg;		
				NET_UPNP_ST_INFO * upnp = NULL;
				char server_ip[512];
				char mac_id[512];
				memset(&msg,0x00,sizeof(msg));
				get_svr_mac_id(server_ip,msg.mac_id);
				upnp = net_get_server_info();
				if( upnp->upnp_flag == 1 )
					msg.client_port = upnp->external_port;
				else
					msg.client_port = 0;
				
				upnp = net_get_yun_server_info();

				if( upnp->upnp_flag == 1 )
					msg.yun_port = upnp->external_port;
				else
					msg.yun_port = 0;				
				

				#ifdef debug_log
				DPRINTK("YS_GET_NVR_ALL_INFO mac_id=%s client_port=%d  yun_port=%d\n",
				msg.mac_id,msg.client_port,msg.yun_port);
				#endif
				
				ret = ys_send_data(pLsock,YS_GET_NVR_ALL_INFO,&msg,sizeof(msg));
				if( ret < 0 )
				{
					DPRINTK("ys_send_data err\n");
				}
				
			}			
			break;
		case YS_NVR_CONNECT_TO_PC:
			{
				UDP_MSG msg;
				YS_TCP_CONNECT_ST tcp_st;
				int i = 0;				

				memcpy(&msg,buf+sizeof(YUN_CMD_ST),sizeof(UDP_MSG));
				DPRINTK("YS_NVR_CONNECT_TO_PC %s:%d\n",inet_ntoa(msg.nvr_address.sin_addr),ntohs(msg.nvr_address.sin_port));

				memset(&tcp_st,0x00,sizeof(YS_TCP_CONNECT_ST));
				tcp_st.need_tcp_connect_socket_num = msg.yun_session_num;

				ret = ys_tcp_st_connect(inet_ntoa(msg.nvr_address.sin_addr),ntohs(msg.nvr_address.sin_port),&tcp_st);
				
				if( ret < 0 )				
					msg.request_cmd = TCP_REQUEST_FAILED;
				else
				{
					msg.request_cmd = TCP_REQUEST_SUCCESS;
				
					for( i = 0; i < tcp_st.need_tcp_connect_socket_num; i++)
					{
						net_server_get_client_socket(net_get_sever_ctrl(),tcp_st.tcp_socket[i],&msg.nvr_address);
					}
				}

				ret = ys_send_data(pLsock,YS_NVR_CONNECT_TO_PC,&msg,sizeof(msg));
				if( ret < 0 )
				{
					DPRINTK("ys_send_data err\n");
				}					
				
			}
			break;
		case YS_NVR_CONNECT_TO_YUN_SERVER:
			{
				UDP_MSG msg;
				YS_TCP_CONNECT_ST tcp_st;
				int i = 0;			
				char server_ip[512];
				char mac_id[512];		
				get_svr_mac_id(server_ip,mac_id);

				memcpy(&msg,buf+sizeof(YUN_CMD_ST),sizeof(UDP_MSG));
				DPRINTK("YS_NVR_CONNECT_TO_YUN_SERVER %s:%d  mac_id:%s\n",
					inet_ntoa(msg.nvr_address.sin_addr),ntohs(msg.nvr_address.sin_port),mac_id);

				strcpy(server_ip,inet_ntoa(msg.nvr_address.sin_addr));
				
				memset(&tcp_st,0x00,sizeof(YS_TCP_CONNECT_ST));
				strcpy(tcp_st.mac_id,mac_id);				
				tcp_st.need_tcp_connect_socket_num = msg.yun_session_num;

				ret = ys_tcp_st_connect_yun_svr(server_ip,ntohs(msg.nvr_address.sin_port),&tcp_st);
				
				if( ret < 0 )				
					msg.request_cmd = TCP_REQUEST_FAILED;
				else
				{
					msg.request_cmd = TCP_REQUEST_SUCCESS;
				
					for( i = 0; i < tcp_st.need_tcp_connect_socket_num; i++)
					{
						net_server_get_client_socket(net_get_sever_ctrl(),tcp_st.tcp_socket[i],&msg.nvr_address);
					}
				}

				ret = ys_send_data(pLsock,YS_NVR_CONNECT_TO_PC,&msg,sizeof(msg));
				if( ret < 0 )
				{
					DPRINTK("ys_send_data err\n");
				}					
				
			}
			break;
		default:
			break;
	}	

	
	return 1;
}



static int accpet_socket(void * psvr_Lsock,int accept_socket,char * connect_ip,int port)
{	
	LOCAL_SOCKET * pLsock_svr = NULL;	
	int ret = 0;
	static int count = 0;

	pLsock_svr = &cli_lsock;	
	
	if( ls_check_socket_is_not_clear(pLsock_svr) == 1 )
	{
		ls_destroy_socket(pLsock_svr);		
	}

	ret = ls_init_socket(pLsock_svr,SOCK_STREAM,LS_CONNECT_OBJECT_REMOTE,LS_MAX_SEND_MEMORY_SIZE);
	if( ret < 0  )
	{
		printf("ls_init_socket err=%d\n",ret);
		goto main_err;
	}
			

	ret = ls_connect_by_accept(pLsock_svr,accept_socket,connect_ip,port,svr_recv_data);
	if( ret < 0 )
	{
		printf("ls_connect_by_accept = %d\n",ret);
		goto main_err;
	}

	
	return 1;

main_err:

	ls_destroy_socket(pLsock_svr);
	printf("out ok\n");
	return -1;
}



int ys_start_server_th()
{	
	SET_PTHREAD_NAME(NULL);
	LOCAL_SOCKET * pSvrsock = NULL;
	LOCAL_SOCKET * pLsock_svr = NULL;	
	int err_no = 0;
	int ret = 0;		
	NET_UPNP_ST_INFO * upnp = NULL;
	NET_UPNP_ST_INFO * upnp_server = NULL;
	int is_online_num = 0;

	pSvrsock = &main_lsock;	

	while( ipcam_is_now_dhcp() == 1 )
	{
		sleep(1);
	}

	ret = ls_init_socket(pSvrsock,SOCK_STREAM,LS_CONNECT_OBJECT_REMOTE,LS_MAX_SEND_MEMORY_SIZE);
	if( ret < 0)
	{
		DPRINTK("ls_init_socket err=%d\n",ret);
		goto main_err;
	}


	ret = ls_listen(pSvrsock,YS_SERVER_PORT,accpet_socket,NULL);
	if( ret < 0 )
	{
		DPRINTK("ls_listen err=%d\n",ret);
		goto main_err;
	}	
	

	while(ls_check_socket_iswork(pSvrsock))	
	{				
		usleep(5000000);	

		//DPRINTK("open_yun_flag=%d  %d\n",open_yun_flag, ls_check_socket_iswork(&cli_lsock));
		if( open_yun_flag == 0 &&  ls_check_socket_iswork(&cli_lsock) == 1 )
		{
			DPRINTK("close yun server\n");
			SendM2("killall yun_svr");			
		}		

		if( ls_check_socket_iswork(&cli_lsock) == 1 )
		{
			is_online_num++;

			if( is_online_num >= 2 )			
				is_connect_yun_svr = 1;			
		}else
		{
			is_online_num = 0;
			is_connect_yun_svr = 0;
		}

		if( ls_check_socket_iswork(&cli_lsock) != 1  &&  (open_yun_flag == 1))
		{
			char cmd[200];
			char ip_buf[200];
			char svr_ip[200];
			char mac_id[50];
			int a;
			int url_get_ip = 1;
			
			memset(ip_buf,0,200);			
			get_svr_mac_id(svr_ip,mac_id);
			DPRINTK("get_dns_url_ip\n");
			get_dns_url_ip(svr_ip,ip_buf);
			
			// 没有得到dns.
			if( ip_buf[0] == 0 )
			{
				for(a = 0; a < 8;a++)
				{
					if( svr_ip[a] == (char)NULL )
						break;

					if( (svr_ip[a] < '0' || svr_ip[a] > '9') 
								&& (svr_ip[a] != '.')  )
					{			
						DPRINTK("url is %s,but can not get ip\n",svr_ip);
						url_get_ip = 0;
						break;
					}
				}
			}

			if(url_get_ip == 0 )
				continue;
			
			if( ip_buf[0] == 0 )
		      		strcpy(ip_buf,svr_ip);

			upnp = net_get_yun_server_info();
			upnp_server = net_get_server_info();

			if( upnp->upnp_over == 0 || upnp_server->upnp_over == 0 )
				continue;

			//if( ys_check_port_availble(bind_port) < 0 )
			//	bind_port = ys_get_availble_port();

			//if( ys_check_port_availble(yun_transmit_port) < 0 )
			//	yun_transmit_port =  ys_get_availble_port();

			if( bind_port != -1 && yun_transmit_port != -1 )
			{				
				sprintf(cmd,"/mnt/mtd/yun_svr cli %s %d %d %d &",ip_buf,YUN_SERVER_PORT,bind_port,yun_transmit_port);
				DPRINTK("cmd:%s\n",cmd);
				command_drv(cmd);

				upnp = net_get_yun_server_info();
				if( yun_transmit_port != upnp->internal_port )
				{
					DPRINTK("yun port change %d->%d\n",upnp->internal_port,yun_transmit_port);
					upnp->internal_port = yun_transmit_port;
					Lock_Set_UPnP(upnp,0);	
				}
			}			
		}
		
		
	}	

	

	ls_destroy_socket(pSvrsock);
	ls_destroy_socket(pLsock_svr);

	return 1;	

main_err:

	ls_destroy_socket(pSvrsock);
	ls_destroy_socket(pLsock_svr);

	return -1;
}

int ys_start_server()
{
	int ret;
	pthread_t id_ys_server;	
	
	ret = pthread_create(&id_ys_server,NULL,(void*)ys_start_server_th,NULL);
	if ( 0 != ret )
	{
		DPRINTK( "create ys_start_server_th  thread error\n");
		return -1;
	}	

	return 1;
}

int yun_server_restart()
{
	SendM2("killall yun_svr");
	DPRINTK(" restart yun svr\n");
	return 1;
}

int get_yun_server_flag()
{
	return open_yun_flag;
}


int open_yun_server(int flag)
{
	if(flag == 0 )
		open_yun_flag = 0;
	else
		open_yun_flag = 1;


	//open_yun_flag = 1;

	DPRINTK("open_yun_flag=%d allways\n",open_yun_flag);
	
	return 1;
}

int get_yun_server_online()
{
	return is_connect_yun_svr;
}




