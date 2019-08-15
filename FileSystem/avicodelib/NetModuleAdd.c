#include "NetModule.h"
#include <errno.h>
#include "P2pBaseDate.h"
#include "devfile.h"
#include "bufmanage.h"
#include "func_common.h"

#include "P2pClient.h"

extern NET_MODULE_ST * client_pst[MAX_CLIENT];


////////////////////////////////////////////////////////////////
//以下为dvr具体操作
static int g_listen_thread_work = 0;

int net_recv_data(void * pst,char * buf,int length,int cmd)
{
	DPRINTK("buf length = %d\n",length);
	return 1;
}





void dvr_net_nvr_listen_thread(void * value)
{
	int listen_port  = 0;
	int listen_fd = 0;
	struct sockaddr_in adTemp;
	struct  linger  linger_set;
	int i,sin_size;
	struct  timeval timeout;
	int ret;
	int tmp_fd = 0;	
	struct sockaddr_in server_addr;
	fd_set watch_set;
	int result;
	NET_MODULE_ST * p_net_client = NULL;


	listen_port  = *(int *)value;
	
	DPRINTK(" port = %d in\n",listen_port);

	if( -1 == (listen_fd =  socket(PF_INET,SOCK_STREAM,0)))
	{
		DPRINTK( "Socket initialize error\n" );
		
		goto error_end;
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		goto error_end;	
	}	

	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;
    if (setsockopt( listen_fd, SOL_SOCKET, SO_LINGER , (char *)&linger_set, 
sizeof(struct  linger) ) < 0 )
    {
		
		DPRINTK( "pNetSocket->sockSrvfd  SO_LINGER  error\n" );
		goto error_end;
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);	
	server_addr.sin_port=htons(listen_port); 

	if ( -1 == (bind(listen_fd,(struct sockaddr *)(&server_addr),
		sizeof(struct sockaddr))))
	{		
		DPRINTK( "DVR_NET_Initialize socket bind error\n" );
		goto error_end;
	}


	while( ipcam_is_now_dhcp() == 1 )
	{
		sleep(1);		
	}
	
	if ( -1 == (listen(listen_fd,5)))
	{
	
		DPRINTK( "DVR_NET_Initialize socket listen error\n" );
		goto error_end;
	}

	sin_size = sizeof(struct sockaddr_in);

	while( g_listen_thread_work==1 )
	{
		FD_ZERO(&watch_set); 
 		FD_SET(listen_fd,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( listen_fd + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "listen socket select error!\n" );
			goto error_end;
		}

		if ( ret > 0 && FD_ISSET( listen_fd, &watch_set ) )
		{
			if ( -1 == (tmp_fd = accept( listen_fd,
					(struct sockaddr *)&adTemp, &sin_size)))
			{
					DPRINTK( "DVR_NET_Initialize accept error 2\n" );
			}else
			{

				if( Net_dvr_have_client()  == 0 )
				{			
					//创建新的client_socket.				

					DPRINTK("tmp_fd =%d\n",tmp_fd);
				
					p_net_client = client_pst[0];

					if( p_net_client != NULL )
					{			
						if( ipcam_get_use_common_protocol_flag() == 1 )
						{
							p_net_client->connect_port = NORMAL_IPCAM_PORT;
							if( p_net_client->stop_work_by_user == 1 )
							{
								 p_net_client->stop_work_by_user = 0;
								 DPRINTK("wrong stop_work_by_user sign\n");
							}
						}
						else
							p_net_client->connect_port = NET_CAM_PORT;
						
						result = NM_connect_by_accept(p_net_client,tmp_fd);

						if( result < 0 )
						{
							DPRINTK("NM_connect_by_accept error: %s\n",inet_ntoa(adTemp.sin_addr));	
							NM_destroy_st(p_net_client);							
						}else
						{				
							DPRINTK("NM_connect_by_accept : %s\n",inet_ntoa(adTemp.sin_addr));								
						}
					}
				}else
				{
					DPRINTK("already have client ,kill tmp_fd =%d\n",tmp_fd);
					if( tmp_fd > 0 )
					{
						close(tmp_fd);
						tmp_fd = 0;
					}
				}
			}
		}	
	
	}


error_end:

	if( listen_fd > 0 )
	{
		close(listen_fd);
		listen_fd = 0;
	}

	g_listen_thread_work = -1;

	DPRINTK(" out \n");

	pthread_detach(pthread_self());
	
	
	return ;
}




void dvr_net_nvr_listen_thread_mult_client(void * value)
{
	int listen_port  = 0;
	int listen_fd = 0;
	struct sockaddr_in adTemp;
	struct  linger  linger_set;
	int i,sin_size;
	struct  timeval timeout;
	int ret;
	int tmp_fd = 0;	
	struct sockaddr_in server_addr;
	fd_set watch_set;
	int result;
	NET_MODULE_ST * p_net_client = NULL;
	int have_empty_client = 0;


	listen_port  = *(int *)value;
	
	DPRINTK(" port = %d in\n",listen_port);

	if( -1 == (listen_fd =  socket(PF_INET,SOCK_STREAM,0)))
	{
		DPRINTK( "Socket initialize error\n" );
		
		goto error_end;
	}

	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		goto error_end;	
	}	

	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;
    if (setsockopt( listen_fd, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
    {
		
		DPRINTK( "pNetSocket->sockSrvfd  SO_LINGER  error\n" );
		goto error_end;
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);	
	server_addr.sin_port=htons(listen_port); 

	if ( -1 == (bind(listen_fd,(struct sockaddr *)(&server_addr),
		sizeof(struct sockaddr))))
	{		
		DPRINTK( "DVR_NET_Initialize socket bind error\n" );
		goto error_end;
	}

	while( ipcam_is_now_dhcp() == 1 )
	{
		sleep(1);		
	}

	if ( -1 == (listen(listen_fd,5)))
	{
	
		DPRINTK( "DVR_NET_Initialize socket listen error\n" );
		goto error_end;
	}

	sin_size = sizeof(struct sockaddr_in);

	while( g_listen_thread_work==1 )
	{
		FD_ZERO(&watch_set); 
 		FD_SET(listen_fd,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( listen_fd + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "listen socket select error!\n" );
			goto error_end;
		}

		if ( ret > 0 && FD_ISSET( listen_fd, &watch_set ) )
		{
			if ( -1 == (tmp_fd = accept( listen_fd,
					(struct sockaddr *)&adTemp, &sin_size)))
			{
					DPRINTK( "DVR_NET_Initialize accept error 2\n" );
			}else
			{
				have_empty_client = 0;

				for( i = 1; i < MAX_CLIENT;i++)
				{
					p_net_client = client_pst[i];
					
					if( p_net_client->is_alread_work == 0)
					{
						have_empty_client= 1;
						break;
					}
				}

				if( have_empty_client  == 1 )
				{			
					//创建新的client_socket.	

					if( ipcam_get_use_common_protocol_flag() == 1 )
						p_net_client->connect_port = NORMAL_IPCAM_PORT;
					else
						p_net_client->connect_port = NET_CAM_PORT;

					DPRINTK("tmp_fd =%d\n",tmp_fd);					
					result = NM_connect_by_accept(p_net_client,tmp_fd);

					if( result < 0 )
					{
						DPRINTK("NM_connect_by_accept error: %s\n",inet_ntoa(adTemp.sin_addr));	
						NM_destroy_st(p_net_client);							
					}else
					{							
						DPRINTK("NM_connect_by_accept : %s\n",inet_ntoa(adTemp.sin_addr));					
					}
					
				}else
				{
					DPRINTK("mult already have client ,kill tmp_fd =%d\n",tmp_fd);
					if( tmp_fd > 0 )
					{
						close(tmp_fd);
						tmp_fd = 0;
					}
				}
			}
		}	
	
	}


error_end:

	if( listen_fd > 0 )
	{
		close(listen_fd);
		listen_fd = 0;
	}

	//g_listen_thread_work = -1;

	DPRINTK(" out \n");

	pthread_detach(pthread_self());
	
	
	return ;
}

int hvr_net_get_listen_thread_work()
{
	return g_listen_thread_work;
}



int dvr_net_create_nvr_listen(int listen_port )
{
	int i;
	pthread_t td_id;
	int ret;
	int count;
	static int s_listen_port = 0;

	//防止send失败造成pipe破裂，导致程序退出
	//signal(SIGPIPE,SIG_IGN);

	s_listen_port = listen_port;

	g_listen_thread_work = 1;

	
	ret = pthread_create(&td_id,NULL,(void*)dvr_net_nvr_listen_thread,(void *)&s_listen_port);
	if ( 0 != ret )
	{			
		DPRINTK( "create dvr_net_listen_thread port=%d  thread error\n",listen_port);	

		goto create_error;
	}	

	{
		static int s_mult_listen_port = 0;
		pthread_t td_id_mult;

		s_mult_listen_port = s_listen_port + 100;

		ret = pthread_create(&td_id_mult,NULL,(void*)dvr_net_nvr_listen_thread_mult_client,(void *)&s_mult_listen_port);
		if ( 0 != ret )
		{			
			DPRINTK( "create dvr_net_listen_thread port=%d  thread error\n",s_mult_listen_port);	

			goto create_error;
		}
	
	}


	return 1;

create_error:
	
	g_listen_thread_work = 0;

	return -1;
}

//额外加的

int Net_dvr_send_self_data(char * buf,int length)
{
	int ret;
	int i;
	NET_MODULE_ST * server_pst = NULL;
	
	if( length >  200*1024)
	{
		DPRINTK("length = %d too bigger!\n",length);
		return -1;
	}	
	
	for( i = 0; i < MAX_CLIENT;i++)	
	{		
		server_pst = client_pst[i];
		if( server_pst )
		{				
			if( server_pst->is_alread_work )
			{				
				if( NM_net_send_data(server_pst,buf,length,NET_DVR_NET_SELF_DATA) > 0 )
				{			
					
				}
			}
		}
	}

	
	return -1;	
}




int Net_dvr_send_cam_data(char * buf,int length,NET_MODULE_ST * server_pst)
{
	int ret;
	
	if( length >  200*1024)
	{
		DPRINTK("length = %d too bigger!\n",length);
		return -1;
	}

	if( server_pst )
	{
		if( NM_net_send_data(server_pst,buf,length,NET_DVR_NET_COMMAND) > 0 )
		{
			return 1;
		}
	}
	
	return -1;	
}

int Net_dvr_have_client()
{

	int i;
	int have_client = 0;
	NET_MODULE_ST * server_pst = NULL;
	static int show_count = 0;

	

	server_pst = client_pst[0];

	show_count++;

	if( show_count > 10 )
	{
		show_count = 0;

		DPRINTK("client_pst[0]->is_alread_work = %d\n",client_pst[0]->is_alread_work);
	}
	
	{
		if( server_pst->is_alread_work > 0  )
		{
			have_client = 1;
		}
	}

	return have_client;
}


int Net_have_fab_client()
{

	int i;
	int client_num = 0;
	NET_MODULE_ST * server_pst = NULL;


	for( i = 0; i < MAX_CLIENT;i++)	
	{		
		server_pst = client_pst[i];
		if( server_pst )
		{				
			if( server_pst->is_alread_work )
			{	
				client_num++;
			}
		}
	}

	return client_num;
}

int Net_have_onvif_app_connect()
{
	int i;
	int client_num = 0;
	NET_MODULE_ST * server_pst = NULL;


	for( i = 0; i < MAX_CLIENT;i++)	
	{		
		server_pst = client_pst[i];
		if( server_pst )
		{				
			if( (server_pst->is_alread_work == 1) && (server_pst->is_onvif_connect == 1))
			{	
				client_num++;
			}
		}
	}

	//DPRINTK("client_num=%d\n",client_num);

	return client_num;
}

int Net_cam_have_client_check_all_socket()
{
	int i;
	int have_client = 0;
	NET_MODULE_ST * server_pst = NULL;
	int client_num = 0;
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	client_num = Net_have_fab_client();

//	DPRINTK("client_num -%d-, %d\n", client_num, Net_have_onvif_app_connect());

	if( Net_have_onvif_app_connect() >  0)
	{
		if( client_num > 1 )
			have_client = 1;
	}else
	{
		if( client_num > 0 )
			have_client = 1;
	}
	

	if( dvr_is_recording() == 1 )
	{
		have_client = 1;
		//DPRINTK("is dvr_is_recording\n");
	}	

	if( netstat_have_rstp_user() > 0 )
	{
		have_client = 1;
		//DPRINTK("is netstat_have_rstp_user\n");
	}

	if (is_have_client() == 1)
	{
		have_client = 1;
	}

	return have_client;
}

void Net_dvr_stop_client()
{	
	int i;
	NET_MODULE_ST * server_pst = NULL;
	

	for( i = 0; i < MAX_CLIENT;i++)
	{
		server_pst = client_pst[i];
		
		if( server_pst->client > 0 )
		{
			server_pst->stop_work_by_user = 1;
			sleep(1);
		}
	}
}

int Net_dvr_get_send_frame_num()
{
	int ret;

	ret = Net_cam_get_send_video_frame();

	return ret;
	
}



void Net_dvr_check_client()
{	
	int i;
	NET_MODULE_ST * server_pst = NULL;
	static int send_data_flag[MAX_CLIENT] = {0};
	

	for( i = 0; i < 1;i++)
	{
		server_pst = client_pst[i];
		
		if( server_pst->client > 0 )
		{
			if( server_pst->send_data_flag == 0 )
			{
				DPRINTK("client_pst[%d] send_data_flag=%d\n",i,send_data_flag[i]);
				send_data_flag[i]++;

				if( send_data_flag[i] > 1 )
				{
					DPRINTK("client_pst[%d] send_data_flag=%d  close client\n",i,send_data_flag[i]);
					server_pst->stop_work_by_user = 1;
					sleep(1);
					send_data_flag[i] = 0;
				}
			}else
			{
				send_data_flag[i] = 0;
			}
		}
	}
}

int Net_dvr_is_main_client(NET_MODULE_ST * server_pst)
{
	int ret;

	if( server_pst == client_pst[0] )
		return 1;

	return 0;	
}


