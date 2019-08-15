
#include "NetModule.h"
#include <errno.h>
#include "P2pBaseDate.h"
#include "devfile.h"
#include "bufmanage.h"
#include "func_common.h"

#define  ONLY_USE_FOR_GOKE  (1)


#ifdef ONLY_USE_FOR_GOKE

static int connect_but_no_recv_data_count = 0;

#endif


//用这个函数可以节省内存，但会增加cpu的使用率
int NM_command_recv_data2(NET_MODULE_ST * pst)
{
	NET_DATA_ST st_data;
	int ret;
	char * buf = NULL;
	char * dest_buf = NULL;
	int convert_length = 0;
	BUF_FRAMEHEADER * pheader = NULL;
	
	ret = NM_socket_read_data(pst->client,&st_data,sizeof(NET_DATA_ST));	
	if( ret < 0 )
		goto error_end; 	

	if( st_data.data_length > MAX_FRAME_BUF_SIZE || st_data.data_length <= 0)
	{
		DPRINTK(" length=%d error  no=%d\n",st_data.data_length,pst->dvr_cam_no);
		goto error_end; 
	}

	buf  = (char*)malloc(st_data.data_length+1000);
	if( buf == NULL  )
	{
		DPRINTK("malloc error\n");
		goto error_end; 
	}	

	ret = NM_socket_read_data(pst->client,buf,st_data.data_length);
	if( ret < 0 )
		goto error_end; 

	dest_buf  =  (char*)malloc(st_data.data_length+1000);
	if( dest_buf == NULL  )
	{
		DPRINTK("malloc error\n");
		goto error_end; 
	}
	

	if(pst->connect_port == NORMAL_IPCAM_PORT )
	{
		convert_length = NIP_nvr_recv_command_convert(dest_buf,MAX_FRAME_BUF_SIZE,
			buf,st_data.data_length,st_data.cmd);
	}else
		convert_length = 0;

	if( convert_length < 0 )
	{
		DPRINTK("NIP_command_convert_to_nvr error!\n");
		goto skip_ok; 
	}

	pst->recv_data_num++;
	if( pst->recv_data_num > 1000)
		pst->recv_data_num = 0;

	if( dest_buf )
	{
		pheader = (BUF_FRAMEHEADER*)dest_buf;

		if( pheader->index >= 0 && pheader->index < 6 )
		{
			//DPRINTK("index=%d\n",pheader->index);
			pst->cam_chan_recv_num[pheader->index]++;
			if( pst->cam_chan_recv_num[pheader->index] > 1000)
				pst->cam_chan_recv_num[pheader->index] = 0;
		}
	}

	if( convert_length > 0 )
		pst->net_get_data((void*)pst,dest_buf,convert_length,st_data.cmd);
	else
		pst->net_get_data((void*)pst,buf,st_data.data_length,st_data.cmd);

skip_ok:

	if( buf )
		free(buf);

	if( dest_buf )
		free(dest_buf);

	return 1;	

error_end:

	if( buf )
		free(buf);

	if( dest_buf )
		free(dest_buf);

	return -1;
}


int NM_command_recv_data(NET_MODULE_ST * pst)
{
	NET_DATA_ST st_data;
	int ret;
	char buf[MAX_FRAME_BUF_SIZE];
	char dest_buf[MAX_FRAME_BUF_SIZE];
	int convert_length = 0;
	BUF_FRAMEHEADER * pheader = NULL;
	
	ret = NM_socket_read_data(pst->client,&st_data,sizeof(NET_DATA_ST));	
	if( ret < 0 )
		return -1;	

	if( st_data.data_length > MAX_FRAME_BUF_SIZE || st_data.data_length <= 0)
	{
		DPRINTK(" length=%d error  no=%d\n",st_data.data_length,pst->dvr_cam_no);
		return -1;
	}

	ret = NM_socket_read_data(pst->client,buf,st_data.data_length);
	if( ret < 0 )
		return -1;	
	

	if(pst->connect_port == NORMAL_IPCAM_PORT )
	{	
		convert_length = NIP_nvr_recv_command_convert(dest_buf,MAX_FRAME_BUF_SIZE,
			buf,st_data.data_length,st_data.cmd);
	}else
		convert_length = 0;

	if( convert_length < 0 )
	{
		DPRINTK("NIP_command_convert_to_nvr error!\n");
		return 1;
	}

	pst->recv_data_num++;
	if( pst->recv_data_num > 1000)
		pst->recv_data_num = 0;	

	pst->is_check_goke_recv_data_num++;

	pheader = (BUF_FRAMEHEADER*)dest_buf;

	if( pheader->index >= 0 && pheader->index < 6 )
	{
		//DPRINTK("index=%d\n",pheader->index);
		pst->cam_chan_recv_num[pheader->index]++;
		if( pst->cam_chan_recv_num[pheader->index] > 1000)
			pst->cam_chan_recv_num[pheader->index] = 0;
	}

	if( convert_length > 0 )
		pst->net_get_data((void*)pst,dest_buf,convert_length,st_data.cmd);
	else
		pst->net_get_data((void*)pst,buf,st_data.data_length,st_data.cmd);

	return 1;	
}



int NM_net_send_data_direct(NET_MODULE_ST * pst,char * buf,int length,int cmd)
{
	int ret;
	NET_DATA_ST st_data;
	char dest_buf[MAX_FRAME_BUF_SIZE];
	char src_buf[MAX_FRAME_BUF_SIZE];
	int data_length;
	int convert_length;

	if(  pst->recv_th_flag != 1 )
	{
		DPRINTK("recv_th_flag=%d  no=%d\n",pst->recv_th_flag,pst->dvr_cam_no);
		return -1;
	}

	if( pst->stop_work_by_user != 0 )
	{
		DPRINTK("stop_work_by_user=%d  no=%d\n",pst->stop_work_by_user,pst->dvr_cam_no);
		return -1;
	}
	
	if( length + sizeof(NET_DATA_ST) >= MAX_FRAME_BUF_SIZE )
	{
		return -1;
	}	

	st_data.cmd = cmd;
	st_data.data_length = length;


	fifo_lock_buf(pst->cycle_send_buf);	

	memcpy(src_buf,&st_data,sizeof(NET_DATA_ST));
	memcpy(src_buf + sizeof(NET_DATA_ST),buf,length);
	data_length = sizeof(NET_DATA_ST) + length;	
	
	 if(pst->connect_port == NORMAL_IPCAM_PORT )		
		convert_length = NIP_nvr_send_command_convert(dest_buf,MAX_FRAME_BUF_SIZE,src_buf,data_length);
	else
		convert_length = 0;
	
	
	if( convert_length >  0 )
	{
		ret = NM_socket_send_data( pst->client, dest_buf, convert_length);
		if( ret < 0 )
		{		
			DPRINTK("NM_socket_send_data error11! no=%d\n",pst->dvr_cam_no);	
			if( pst->stop_work_by_user == 0 && pst->is_alread_work == 1)
				pst->stop_work_by_user = 1;
		}
	}else
	{
		if( data_length > 0 )
		{
			ret = NM_socket_send_data( pst->client, src_buf, data_length);
			if( ret < 0 )
			{	
				DPRINTK("NM_socket_send_data error11! no=%d\n",pst->dvr_cam_no);
				if( pst->stop_work_by_user == 0 && pst->is_alread_work == 1)
					pst->stop_work_by_user = 1;
			}
		}	
	}	
	
	fifo_unlock_buf(pst->cycle_send_buf);

	
	pst->recv_data_num++;
	if( pst->recv_data_num > 1000)
		pst->recv_data_num = 0;

	#ifdef  ONLY_USE_FOR_GOKE
	if( st_data.cmd ==  NET_DVR_NET_FRAME )
	{
		if( ret > 0 && pst->recv_data_num > 100)
			connect_but_no_recv_data_count = 0;
	}
	#endif

	return 1;

err:
	fifo_reset(pst->cycle_send_buf);
	fifo_unlock_buf(pst->cycle_send_buf);

	return -1;
}



int NM_net_send_data(NET_MODULE_ST * pst,char * buf,int length,int cmd)
{
	int ret;
	NET_DATA_ST st_data;

	if( pst->client < 0 )
	{
		return -1;
	}

	if( pst->send_data_mode == NET_SEND_DIRECT_MODE)
		return NM_net_send_data_direct(pst,buf,length,cmd);

	if( pst->send_th_flag != 1 )
		return -1;

	if(  pst->recv_th_flag != 1 )
		return -1;
	
	if( length + sizeof(NET_DATA_ST) >= MAX_FRAME_BUF_SIZE )
	{
		return -1;
	}

	st_data.cmd = cmd;
	st_data.data_length = length;


	fifo_lock_buf(pst->cycle_send_buf);		
	
	ret = fifo_put(pst->cycle_send_buf,&st_data,sizeof(NET_DATA_ST));
	if( ret != sizeof(NET_DATA_ST) )
	{
		DPRINTK("%d insert error!\n",ret);		
		goto err;
	}	

	ret = fifo_put(pst->cycle_send_buf,buf,length);
	if( ret != length )
	{
		DPRINTK("%d %d insert error!\n",ret,length);		
		goto err;
	}
	
	fifo_unlock_buf(pst->cycle_send_buf);

	return 1;

err:
	fifo_reset(pst->cycle_send_buf);
	fifo_unlock_buf(pst->cycle_send_buf);

	return -1;
}

int NM_get_buf(NET_MODULE_ST * pst,char * buf)
{
	int ret;
	int length;
	NET_DATA_ST st_data;
	CYCLE_BUFFER * cycle_enc_buf_ptr = NULL;

	cycle_enc_buf_ptr = pst->cycle_send_buf;		

	fifo_lock_buf(pst->cycle_send_buf);	
	
	ret = fifo_get(cycle_enc_buf_ptr,&st_data,sizeof(NET_DATA_ST));
	if( ret != sizeof(NET_DATA_ST) )
	{
		DPRINTK("%d get error!\n",ret);		
		goto err2;
	}	

	length = st_data.data_length;

	memcpy(buf,&st_data,sizeof(NET_DATA_ST));
	
	ret = fifo_get(cycle_enc_buf_ptr,buf + sizeof(NET_DATA_ST),length);
	if( ret != length )
	{
		DPRINTK("%d %d get error! %d\n",ret,length,sizeof(NET_DATA_ST));		
		goto err2;
	}
	fifo_unlock_buf(pst->cycle_send_buf);


	length = sizeof(NET_DATA_ST) + st_data.data_length;

	return length;

err2:
	fifo_reset(cycle_enc_buf_ptr);
	fifo_unlock_buf(pst->cycle_send_buf);

	return -1;
}


int NM_socket_read_data( int idSock, char* pBuf, int ulLen )
{
	int ulReadLen = 0;
	int iRet = 0;
	int eagain_num = 0;

	if ( idSock < 0 )
	{
		return -1;
	}	

	while( ulReadLen < ulLen )
	{		
		iRet = recv( idSock, pBuf, ulLen - ulReadLen, 0 );
		if ( iRet <= 0 )
		{
			DPRINTK("ulReadLen=%d eagain_num=%d\n",iRet,eagain_num);
			DPRINTK("%s\n", strerror(errno));

			if( iRet < 0 )
			{
				/*
				if( errno == EAGAIN )         
				{
					eagain_num++;
					if( eagain_num < 2 )
	                        		continue;
				}
				*/
			}
			
			return -1;
		}
		ulReadLen += iRet;
		pBuf += iRet;
	}	
	
	return 1;
}

int NM_socket_send_data( int idSock, char* pBuf, int ulLen )
{
	int lSendLen;
	int lStotalLen;
	int lRecLen;
	int eagain_num = 0;
	char* pData = pBuf;

	lSendLen = ulLen;
	lStotalLen = lSendLen;
	lRecLen = 0;

	if ( idSock < 0 )
	{
		DPRINTK("idSock=%d no stop 1\n",idSock);
		return -1;
	}	
	
	while( lRecLen < lStotalLen )
	{
		//
		lSendLen = send( idSock, pData, lStotalLen - lRecLen, 0 );			
		if ( lSendLen <= 0 )
		{
			DPRINTK("lSendLen=%d %d eagain_num=%d\n",lSendLen,idSock,eagain_num);
			DPRINTK("%s\n", strerror(errno));

			if( errno == EAGAIN )            
			{
			/*
			    eagain_num++;
			    if( eagain_num < 2 )	
                        	continue;
                        	*/
			}
			
			return -1;
		}
		lRecLen += lSendLen;
		pData += lSendLen;
	}

	

	return 1;
}


NET_MODULE_ST * NM_ini_st(int work_mode,int send_mode,int buf_size,int (*func)(void *,char *,int,int))
{
	NET_MODULE_ST * pst = NULL;

	pst = (NET_MODULE_ST *)malloc(sizeof(NET_MODULE_ST));

	if( pst == NULL )
	{
		printf("malloc error!\n");
		goto ini_error;
	}

	memset(pst,0x00,sizeof(NET_MODULE_ST));

	pst->cycle_send_buf = NULL;	
	pst->is_alread_work = 0;
	pst->build_once = work_mode;
	pst->work_mode = 0;
	pst->recv_data_num = 0;
	pst->send_data_mode = send_mode;
	pst->bind_enc_num[0] = -1;
	pst->bind_enc_num[1] = -1;
	pst->bind_enc_num[2] = -1;
	pst->bind_enc_num[3] = -1;
	pst->bind_enc_num[4] = -1;
	pst->bind_enc_num[5] = -1;
	pst->thread_stop_num = 0;	
	pst->is_yb_nvr = 0;
	pst->is_onvif_connect = 0;
	
	
	pthread_mutex_init(&pst->control_mutex,NULL);

	if( pst->send_data_mode == NET_SEND_BUF_MODE )
	{
		if( (pst->cycle_send_buf = (CYCLE_BUFFER *)init_cycle_buffer(buf_size)) == NULL )
		{
			DPRINTK("init_cycle_buffer error!\n");
			goto ini_error;
		}
	}else
	{
		if( (pst->cycle_send_buf = (CYCLE_BUFFER *)init_cycle_buffer(0x100000/8/8)) == NULL )
		{
			DPRINTK("init_cycle_buffer error!\n");
			goto ini_error;
		}
	}

	DPRINTK("cycle_send_buf = 0x%x\n",pst->cycle_send_buf);

	filo_set_save_num(pst->cycle_send_buf,0);

	pst->net_get_data = func;

	DPRINTK("work_mode=%d send_data_mode=%d buf_size=%d  init mutex\n",work_mode,send_mode,buf_size);
	

	return pst;



ini_error:

	if( pst )
	{
		if( pst->cycle_send_buf )
			destroy_cycle_buffer(pst->cycle_send_buf);

		free(pst);
	}

	return NULL;
}



int  NM_reset(NET_MODULE_ST * pst)
{
	int i;
	
	if( pst == NULL )
	{
		DPRINTK("NET_MODULE_ST is NULL!\n");
		goto reset_error;
	}

	filo_set_save_num(pst->cycle_send_buf,0);
	fifo_reset(pst->cycle_send_buf);	

	pst->bind_enc_num[0] = -1;
	pst->bind_enc_num[1] = -1;
	pst->bind_enc_num[2] = -1;
	pst->bind_enc_num[3] = -1;
	pst->bind_enc_num[4] = -1;
	pst->bind_enc_num[5] = -1;
	pst->mac[0] = 0;
	pst->is_alread_work = 0;
	pst->recv_data_num = 0;	
	pst->stop_work_by_user = 0;
	for( i = 0; i < 8; i++)
		pst->enc_info[i] = 0;
	for( i = 0; i < 6; i++)
		pst->cam_chan_recv_num[i] = 0;
	pst->have_d1_enginer = 0;
	pst->ipcam_type = 0;
	pst->thread_stop_num = 0;	
	pst->send_cam = 0;
	pst->is_yb_nvr = 0;
	pst->is_onvif_connect = 0;
	pst->client = -1;
	pst->is_check_goke_recv_data_num = 0;
	pst->send_data_flag = 0;

	return 1;


reset_error:
	return -1;
}


int NM_destroy_st(NET_MODULE_ST * pst)
{
	if( pst )
	{
		if( pst->build_once == BUILD_MODE_ALLWAY )
		{
			if( pst->cycle_send_buf )
				destroy_cycle_buffer(pst->cycle_send_buf);

			pthread_mutex_destroy( &pst->control_mutex);

			free(pst);
		}else
		{
			if( NM_reset(pst) < 0 )
				DPRINTK("NM_reset error \n");
		}		
	}

	return 1;
}





void NM_get_host_name(char * pServerIp,char * buf)
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
			
			 DPRINTK("tcp pServerIp=%s\n",pServerIp);
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




int NM_client_socket_set(int * client_socket,int client_port,int delaytime,int create_socket,int connect_mode)
{
	struct  linger  linger_set;
//	int nNetTimeout;
	struct timeval stTimeoutVal;
//	struct sockaddr_in server_addr;
	int i;
	int keepalive = 1;
	int keepIdle = 10;
	int keepInterval = 3;
	int keepCount = 3;

	if( create_socket == 1 )
	{	
		if( connect_mode == NET_MODE_NORMAL )
		{
			if( -1 == (*client_socket =  socket(PF_INET,SOCK_STREAM,0)))
			{
				DPRINTK( "DVR_NET_socket initialize error\n" );	
				return ERR_NET_INIT1;
			}
		}else
		{
			if( -1 == (*client_socket =  socket(PF_LOCAL,SOCK_STREAM,0)))
			{
				DPRINTK( "DVR_NET_socket initialize error\n" );	
				return ERR_NET_INIT1;
			}
		}
	}

	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;

    if (setsockopt( *client_socket, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
    {
		printf( "setsockopt SO_LINGER  error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}

	
    if (setsockopt(*client_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(int) ) < 0 )
    {
		printf( "setsockopt SO_KEEPALIVE error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle)) < -1)
	{
		printf( "setsockopt TCP_KEEPIDLE error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval)) < -1)
	{
		printf( "setsockopt TCP_KEEPINTVL error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount)) < -1)
	{
		printf( "setsockopt TCP_KEEPCNT error\n" );
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}



	i = 1;

	if (setsockopt(*client_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		close( *client_socket );
		*client_socket = -1;
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		return ERR_NET_SET_ACCEPT_SOCKET;	
	}		


	  if( delaytime != 0 )
	  {
		stTimeoutVal.tv_sec = delaytime;
		stTimeoutVal.tv_usec = 0;
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	      		close( *client_socket );
			*client_socket  = -1;
			printf( "setsockopt SO_SNDTIMEO error\n" );
			return ERR_NET_SET_ACCEPT_SOCKET;
		}
			 	
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	       		close( *client_socket );
			*client_socket = -1;
			printf( "setsockopt SO_RCVTIMEO error\n" );
			return ERR_NET_SET_ACCEPT_SOCKET;
		}
	  }

	  {
	    int recvbuf_len = 10*1024; //实际缓冲区大小的一半。
	    int sndbuf_len = 10*1024; //实际缓冲区大小的一半。
     	    int len = sizeof(recvbuf_len);
	    if( setsockopt( *client_socket, SOL_SOCKET, SO_RCVBUF, (void *)&recvbuf_len, len ) < 0 )
	    {
	        perror("getsockopt: SO_RCVBUF\n");	      
	    }


	     if( setsockopt( *client_socket, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf_len, len ) < 0 )
	    {
	        perror("getsockopt: SO_SNDBUF\n");	      
	    }
	}



	return 1;
	   
}



void NM_net_send_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	NET_MODULE_ST * pst = NULL;
	char buf[MAX_FRAME_BUF_SIZE];
	char dest_buf[MAX_FRAME_BUF_SIZE];
	int data_length;
	int convert_length;
	int ret;

	pst = (NET_MODULE_ST *)value;

	DPRINTK("in  client=%d\n",pst->client);	


	while( pst->send_th_flag == 1)
	{
	
		if( fifo_enable_get(pst->cycle_send_buf) == 0 )
		{
			usleep(100);
			continue;
		}

		data_length = NM_get_buf(pst,buf);

		 if(pst->connect_port == NORMAL_IPCAM_PORT )
			convert_length = NIP_nvr_send_command_convert(dest_buf,MAX_FRAME_BUF_SIZE,buf,data_length);
		else
			convert_length = 0;
		
		if( convert_length >  0 )
		{
			ret = NM_socket_send_data( pst->client, dest_buf, convert_length);
			if( ret < 0 )
			{
				break;
			}
		}else
		{
			if( data_length > 0 )
			{
				ret = NM_socket_send_data( pst->client, buf, data_length);
				if( ret < 0 )
				{
					break;
				}
			}	
		}

		pst->recv_data_num++;
		if( pst->recv_data_num > 1000)
			pst->recv_data_num = 0;
		
	}
	

	DPRINTK("out %d  no=%d  \n",pst->client,pst->dvr_cam_no);	
	pthread_exit(2);
	
	return ;
}

void NM_net_recv_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	NET_MODULE_ST * pst = NULL;
	fd_set watch_set;
	struct  timeval timeout;
	int ret;
	int show_count = 0;

	DPRINTK("in\n");

	pst = (NET_MODULE_ST *)value;

	DPRINTK("client = %d\n",pst->client);

	while( pst->recv_th_flag == 1)
	{
		FD_ZERO(&watch_set); 
 		FD_SET(pst->client,&watch_set);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		ret  = select( pst->client + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "recv socket select error!\n" );
			goto error_end;
		}

		if( ret == 0) 
		{
			//show_count++;

			//if( show_count % 5 == 0 )
			//	DPRINTK( "recv socket select timeout! client = %d\n",pst->client);
		}

		if ( ret > 0 && FD_ISSET( pst->client, &watch_set ) )
		{
			ret = NM_command_recv_data(pst);
			if( ret < 0 )
			{					
				DPRINTK("NM_command_recv_data error! no=%d\n",pst->dvr_cam_no);
				goto error_end;
			}
		}	
	
		
	}


error_end:
	#ifdef  ONLY_USE_FOR_GOKE

	if( pst->is_check_goke_recv_data_num == 0)
	{
		connect_but_no_recv_data_count++;
		DPRINTK("connect_but_no_recv_data_count = %d\n",connect_but_no_recv_data_count);
		if( connect_but_no_recv_data_count > 10 )
		{
			connect_but_no_recv_data_count = 0;
			DPRINTK("connect_but_no_recv_data_count > 10 , need reboot\n");

			command_drv("/mnt/mtd/fast_run.sh");
		}
	}else
	{
		
		connect_but_no_recv_data_count = 0;
	}

	#endif
	
	if( pst->stop_work_by_user == 0 )
		pst->stop_work_by_user = 1;
	
	DPRINTK("out %d  no=%d  \n",pst->client,pst->dvr_cam_no);	
	pthread_exit(2);	
	return ;
}

int NM_net_get_status(void * value)
{
	NET_MODULE_ST * pst = (NET_MODULE_ST *)value;
	static int frame_num = 0;

	if( pst  == NULL )
		return 0;

	if( pst->client > 0 )
	{

		if( frame_num != pst->recv_data_num )
		{
			//DPRINTK("[%d] %d %d  1\n",chan,frame_num[chan],pNetModule[chan]->work_mode);
			frame_num = pst->recv_data_num;
			return 1;
		}else
		{
			//DPRINTK("[%d] %d %d  0\n",chan,frame_num[chan],pNetModule[chan]->work_mode);
			return 0;
		}	
		
	}
	else
		return 0;	
}


void NM_net_check_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	NET_MODULE_ST * pst = (NET_MODULE_ST *)value;
	int  recv_num = 0;
	int connect_lost_time = 15 * 1000000 / 100000;
	int recv_time = 0;
	int count = 0;
	int step_count = 0;

	DPRINTK("in\n");

	while( ( pst->recv_th_flag == 1 ) && 
		( pst->send_th_flag == 1 || pst->send_data_mode == NET_SEND_DIRECT_MODE) 
		&& ( pst->stop_work_by_user == 0) )
	{
		usleep(500000);

		step_count++;

		//printf(" step_count count = %d\n",step_count);

		if( (step_count % 2) == 0 )
		{
			if( NM_net_get_status((void*)pst) == 1)
			{			
				count = 0;		
				//printf(" send count = %d\n",count);
			}else
			{
				count++;

				//printf("not send count = %d\n",count);

				if( count > 30 )
				{
					DPRINTK("Net not send data!");
					break;
				}
			}
		}
	}



	
	DPRINTK("no=%d  1\n",pst->dvr_cam_no);		
	NM_destroy_work_thread(pst);
	DPRINTK("no=%d  2\n",pst->dvr_cam_no);
	

	DPRINTK("out \n");

	pthread_detach(pthread_self());
	
	return ;
}



int NM_create_work_thread(NET_MODULE_ST * pst)
{
	pthread_t netsend_id;
	pthread_t netrecv_id;
	pthread_t netcheck_id;
	int ret;


	if( pst->send_data_mode == NET_SEND_BUF_MODE )
	{
		pst->send_th_flag = 1;

		ret = pthread_create(&pst->netsend_id,NULL,(void*)NM_net_send_thread,(void *)pst);
		if ( 0 != ret )
		{
			pst->send_th_flag = 0;
			DPRINTK( "create NM_net_send_thread thread error\n");
			goto create_error;
		}
	}

	pst->recv_th_flag = 1;

	ret = pthread_create(&pst->netrecv_id,NULL,(void*)NM_net_recv_thread,(void *)pst);
	if ( 0 != ret )
	{
		pst->recv_th_flag = 0;
		DPRINTK( "create NM_net_recv_thread thread error\n");	
		goto create_error;
	}

	ret = pthread_create(&pst->netcheck_id,NULL,(void*)NM_net_check_thread,(void *)pst);
	if ( 0 != ret )
	{		
		DPRINTK( "create NM_net_check_thread thread error\n");
		goto create_error;
	}


	pst->is_alread_work = 1;

	usleep(100*1000);	

	DPRINTK("ok\n");	
	
	return 1;


create_error:

	NM_destroy_work_thread(pst);	

	return -1;
}

int NM_destroy_work_thread(NET_MODULE_ST * pst)
{
	int count = 0;

	pthread_mutex_lock(&pst->control_mutex);

	DPRINTK("no=%d  socket=%d  send_th_flag=%d recv_th_flag=%d no lock\n",pst->dvr_cam_no,\
		pst->client,pst->send_th_flag,pst->recv_th_flag );	

	if( pst->send_data_mode == NET_SEND_BUF_MODE )
	{
		if( pst->send_th_flag == 1 )
		{
			pst->send_th_flag = 0;
			DPRINTK("wait send_th_flag start no=%d  socket=%d\n",pst->dvr_cam_no,pst->client);		
			pthread_join(pst->netsend_id,NULL);		
			DPRINTK("wait send_th_flag end no=%d  socket=%d\n",pst->dvr_cam_no,pst->client);		
		}	
	}
	
	if( pst->recv_th_flag == 1 )
	{
		pst->recv_th_flag = 0;
		DPRINTK("wait recv_th_flag start no=%d  socket=%d\n",pst->dvr_cam_no,pst->client);		
		pthread_join(pst->netrecv_id,NULL);		
		DPRINTK("wait recv_th_flag end no=%d  socket=%d\n",pst->dvr_cam_no,pst->client);		
	}

	DPRINTK("no=%d  socket=%d 13\n",pst->dvr_cam_no,pst->client);

	if( pst->client >= 0 )
	{		
		DPRINTK("no=%d  close socket=%d 1\n",pst->dvr_cam_no,pst->client);
		close(pst->client);
		DPRINTK("no=%d  close socket=%d 2\n",pst->dvr_cam_no,pst->client);
		pst->client = -1;
		NM_destroy_st(pst);
		DPRINTK("no=%d   NM_destroy_st 3\n",pst->dvr_cam_no);
	}

	DPRINTK("out\n");

	pthread_mutex_unlock(&pst->control_mutex);
	return 1;
}





int NM_connect(NET_MODULE_ST * pst,char * ip,int port)
{
	char ip_buf[200];
	int a;
	int socket_fd = -1;
	int ret;
	struct sockaddr_in addr;
	struct sockaddr_un sa = {
                .sun_family = AF_LOCAL,
                .sun_path = "11"
        };
	
	pthread_mutex_lock( &pst->control_mutex);

	if( pst->is_alread_work == 1 )
	{
		DPRINTK("is_alread_work = 1  not allow connect!\n");
		goto connect_error;
	}

	sprintf(sa.sun_path,"/dev/%d",port);	
 
	memset(ip_buf,0,200);
	NM_get_host_name(ip,ip_buf);

	// 没有得到dns.
	if( ip_buf[0] == 0 )
	{
		for(a = 0; a < 8;a++)
		{
			if( ip[a] == (char)NULL )
				break;

			if( (ip[a] < '0' || ip[a] > '9') 
						&& (ip[a] != '.')  )
			{			
				DPRINTK("url is %s,but can not get ip\n",ip);
				goto connect_error;
			}
		}
	}

	ret = NM_client_socket_set(&socket_fd,0,7,1,pst->connect_mode);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		goto connect_error;
	}

	pst->client = socket_fd;

	DPRINTK("pst->client = %d\n",pst->client);

	addr.sin_family = AF_INET;
	if( ip_buf[0] == 0 )
  		addr.sin_addr.s_addr = inet_addr(ip);
	else
		addr.sin_addr.s_addr = inet_addr(ip_buf);
	addr.sin_port = htons(port);	

	DPRINTK("ip=%s  port=%d \n",ip,port);

	pst->thread_stop_num = 0;


	if( pst->connect_mode == NET_MODE_NORMAL )
	{
		if (connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
		{
			perror(strerror(errno));
			
	        if (errno == EINPROGRESS) 
			{			
	             	fprintf(stderr, "connect timeout\n");	
				if(0)
				{
					unsigned long ul = 1;
	   				ioctl( socket_fd, FIONBIO, &ul );
	             	}
			}  

			DPRINTK("connect error 123!\n");
			
			goto connect_error;
	       }
	}else
	{
		if (connect(socket_fd, (struct sockaddr*)&sa, sizeof(sa)) == -1) 
		{
			perror(strerror(errno));
			
	        	if (errno == EINPROGRESS) 
			{			
	             		fprintf(stderr, "connect timeout\n");	
			}  

			DPRINTK("connect error 123!\n");			
			goto connect_error;
	    	}
	}
	
	if( NM_create_work_thread(pst) < 0 )
		goto connect_error;

	pthread_mutex_unlock( &pst->control_mutex);

	return 1;

connect_error:

	if(  pst->client >= 0)
	{			
		close( pst->client);
		pst->client = -1;		
	}	

	 pthread_mutex_unlock( &pst->control_mutex);
	
	return -1;
}



int NM_disconnect(NET_MODULE_ST * pst)
{
	DPRINTK("1\n");
	NM_destroy_work_thread(pst);
	DPRINTK("2\n");


	return 1;
}

int NM_connect_by_accept(NET_MODULE_ST * pst,int socket_fd)
{
	char ip_buf[200];
	int a;	
	int ret;


	ret = NM_client_socket_set(&socket_fd,0,15,0,pst->connect_mode);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		return ret;
	}

	DPRINTK("tmp_fd =%d\n",socket_fd);

	pst->client = socket_fd;	

	if( NM_create_work_thread(pst) < 0 )
		goto connect_error;


	return 1;

connect_error:

	if(  pst->client >= 0 )
	{			
		close( pst->client);
		pst->client = -1;		
	}
	
	return -1;
}


////////////////////////////////////////////////////////////////
//以下为dvr具体操作




