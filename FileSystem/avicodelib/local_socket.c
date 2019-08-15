//#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

 #ifndef USING_MINGW
#include <sys/shm.h>
#include <sys/ipc.h>
#endif


#include <errno.h>
#include "local_socket.h"
#include "devfile.h"


#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

//#define debug_log (1)

//#define test_lsot_send_packet_app (1)

static int mem_id = LS_COMMEN_KEY;
static pthread_mutex_t key_lock;

int compare_sockaddr(struct sockaddr_in *s1, struct sockaddr_in *s2) {
  char sender[16];
  char recipient[16];
  strcpy(sender, inet_ntoa(s1->sin_addr));
  strcpy(recipient, inet_ntoa(s2->sin_addr));
  
  return ((s1->sin_family == s2->sin_family) && (strcmp(sender, recipient) == 0) && (s1->sin_port == s2->sin_port));
}


int ls_socket_read_data( int idSock, char* pBuf, int ulLen )
{
	int ulReadLen = 0;
	int iRet = 0;
	int eagain_num = 0;

	if ( idSock <= 0 )
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
				if( errno == EAGAIN )         
				{
					eagain_num++;
					if( eagain_num < 2 )
	                        		continue;
				}
			}
			
			return -1;
		}
		ulReadLen += iRet;
		pBuf += iRet;
	}	
	
	return 1;
}


int ls_socket_send_data( int idSock, char* pBuf, int ulLen )
{
	int lSendLen;
	int lStotalLen;
	int lRecLen;
	int eagain_num = 0;
	char* pData = pBuf;

	lSendLen = ulLen;
	lStotalLen = lSendLen;
	lRecLen = 0;

	if ( idSock <= 0 )
	{
		DPRINTK("idSock=%d no stop 1\n",idSock);
		return -1;
	}	
	
	while( lRecLen < lStotalLen )
	{	
		lSendLen = send( idSock, pData, lStotalLen - lRecLen, 0 );			
		if ( lSendLen <= 0 )
		{
			DPRINTK("lSendLen=%d %d eagain_num=%d\n",lSendLen,idSock,eagain_num);
			DPRINTK("%s\n", strerror(errno));

			if( errno == EAGAIN )            
			{
			    eagain_num++;
			    if( eagain_num < 2 )	
                        	continue;
			}
			
			return -1;
		}
		lRecLen += lSendLen;
		pData += lSendLen;
	}

	

	return 1;
}

int ls_set_init_key_id(int num)
{
	static int first = 1;
	if( first == 1 )
	{
		pthread_mutex_init(&key_lock, NULL);
		first = 0;
	}
	
	pthread_mutex_lock(&key_lock);
	mem_id = num;
	pthread_mutex_unlock(&key_lock);
	return mem_id;
}

int ls_init_socket(LOCAL_SOCKET * pLsocket,int socket_type,int connect_object,int mem_max_size)
{	
	int ret = 0;
	int use_key;
	char * mem;


	memset(pLsocket,0x00,sizeof(LOCAL_SOCKET));

	
	pthread_mutex_init(&pLsocket->socket_lock, NULL);	
	pthread_mutex_init(&pLsocket->mem_send_mutex, NULL);
      pthread_cond_init(&pLsocket->mem_send_block_cond, NULL);
	pthread_mutex_init(&pLsocket->need_response_lock, NULL);	
	pthread_mutex_init(&pLsocket->socket_recv_lock, NULL);

	pLsocket->socket_type = socket_type;
	pLsocket->connect_object = connect_object;
	pLsocket->mem_fd = -1; // 有可能创建后得到的值为0，所以赋值-1.
	pLsocket->mem_max_size = mem_max_size;
	pLsocket->remote_mem_fd = -1;	

	if(pLsocket->connect_object == LS_CONNECT_OBJECT_LOCAL)
	{
		pthread_mutex_lock(&key_lock);
		use_key = mem_id;
		mem_id++;
		pthread_mutex_unlock(&key_lock);	

		 #ifndef USING_MINGW
	
		pLsocket->mem_fd= shmget(use_key ,1028,0640);     
	      if(pLsocket->mem_fd != -1)
	      {
	         	mem = (char *)shmat(pLsocket->mem_fd,NULL,0);
	        	if ( (int)mem != (void *)-1)
		      {	      		
		          	shmctl(pLsocket->mem_fd,IPC_RMID,0) ;
		      }
	      }	

		pLsocket->mem_fd = shmget(use_key,pLsocket->mem_max_size,0640|IPC_CREAT|IPC_EXCL); 
		if(pLsocket->mem_fd == -1)
		{
		     DPRINTK("shmget error\n");
		     goto create_err;
		}		

		pLsocket->mem_buf_ptr= (char*)shmat( pLsocket->mem_fd ,( const void* )0,0 );

		if( (int)pLsocket->mem_buf_ptr == -1 )
		{
			DPRINTK("shmat error!\n");
			goto create_err;
		}

		pLsocket->mem_key = use_key;		

		DPRINTK("mem_key=0x%x\n",pLsocket->mem_key);

		#endif
		
	}

	pLsocket->init_flag = 1;

	return 1;

create_err:
	
	return ret;
}

int ls_close_socket(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;	

	if( pLsocket->socket > 0 )
	{
		
		pthread_mutex_lock(&pLsocket->socket_lock);		
		
		close(pLsocket->socket);
		pLsocket->socket = 0;			

		pthread_mutex_unlock(&pLsocket->socket_lock);
	}

	return 1;	
}

int ls_destroy_socket(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	if( pLsocket->init_flag == 0 )	
		return LS_SOCKET_NOT_INIT_ERR;	

	if( ls_check_socket_is_not_clear(pLsocket) > 0 )
	{		
		ls_stop_socket_work(pLsocket);
	}	
	
	ls_close_socket(pLsocket);
	
	pthread_mutex_destroy(&pLsocket->socket_lock);	
	pthread_mutex_destroy(&pLsocket->mem_send_mutex);	
	pthread_cond_destroy(&pLsocket->mem_send_block_cond);	
	pthread_mutex_destroy(&pLsocket->need_response_lock);	
	pthread_mutex_destroy(&pLsocket->socket_recv_lock);	

	 #ifndef USING_MINGW
	if( pLsocket->mem_fd >= 0 )
	{
		shmdt(pLsocket->mem_buf_ptr );
		pLsocket->mem_buf_ptr  = NULL;
		msgctl( pLsocket->mem_fd, IPC_RMID,NULL);
		pLsocket->mem_fd = -1;
		pLsocket->mem_buf_ptr = NULL;
		pLsocket->mem_max_size = 0;
	}

	if( pLsocket->remote_mem_fd >= 0 )
	{
		shmdt(pLsocket->remote_mem_buf_ptr );
		pLsocket->remote_mem_buf_ptr  = NULL;
		msgctl( pLsocket->remote_mem_fd, IPC_RMID,NULL);
		pLsocket->remote_mem_fd = -1;
		pLsocket->remote_mem_key = 0;
		pLsocket->remote_mem_buf_ptr  = NULL;
	}
	#endif

	pLsocket->init_flag = 0;

	return 1;
}

int ls_reset_socket(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;	

	ls_stop_socket_work(pLsocket);

	pthread_mutex_lock( &pLsocket->socket_lock);	
	pLsocket->send_err_flag = 0;	

	#ifndef USING_MINGW
	if( pLsocket->remote_mem_fd >= 0 )
	{
		shmdt(pLsocket->remote_mem_buf_ptr );
		pLsocket->remote_mem_buf_ptr  = NULL;
		msgctl( pLsocket->remote_mem_fd, IPC_RMID,NULL);
		pLsocket->remote_mem_fd = -1;
		pLsocket->remote_mem_key = 0;
		
	}	
	#endif
	pthread_mutex_unlock( &pLsocket->socket_lock);

	return 1;
}

void ls_tcp_listen_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	LOCAL_SOCKET * pLsocket = NULL;
	int i,sin_size;
	fd_set watch_set;
	int ret;
	struct  timeval timeout;
	struct sockaddr_in adTemp;
	int tmp_fd = 0;	

	pLsocket = (LOCAL_SOCKET *)value;	
	if ( -1 == (listen(pLsocket->socket,5)))
	{	
		DPRINTK( "socket listen error\n" );
		return ;
	}

	sin_size = sizeof(struct sockaddr_in);

	pLsocket->is_listen_work = 1;
	pLsocket->all_thread_flag = 1;

	while( pLsocket->is_listen_work )
	{
		FD_ZERO(&watch_set); 
 		FD_SET(pLsocket->socket,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( pLsocket->socket + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "listen socket select error!\n" );
			goto error_end;
		}

		if ( ret > 0 && FD_ISSET( pLsocket->socket , &watch_set ) )
		{			
			tmp_fd = accept( pLsocket->socket,(struct sockaddr *)&adTemp, &sin_size);			
		
			if ( -1 == tmp_fd )
			{
				//	DPRINTK( "DVR_NET_Initialize accept error 2\n" );
			}else
			{
				int allow_accept = 1;
				if(pLsocket->connect_object == LS_CONNECT_OBJECT_LOCAL )
				{
					if( strcmp("127.0.0.1",inet_ntoa(adTemp.sin_addr)) !=  0 )
					{
						close(tmp_fd);
						tmp_fd = 0;
						allow_accept = 0;
						DPRINTK("socket is LS_CONNECT_OBJECT_LOCAL mode, %s is not allow connect\n",
							inet_ntoa(adTemp.sin_addr));
					}
				}				

				if( allow_accept )
				{
					//创建新的client_socket.	
					strcpy(pLsocket->connect_addr,inet_ntoa(adTemp.sin_addr));
					pLsocket->connect_port = adTemp.sin_port;
					pLsocket->address = adTemp;
					pLsocket->accpet_socket(pLsocket,tmp_fd,pLsocket->connect_addr,pLsocket->connect_port);			
				}
			}
		}	
	
	}

error_end:
	ls_close_socket(pLsocket);	

	
	pLsocket->all_thread_flag = 0;
	pthread_exit(2);	

	return ;
}
 
static int ls_recv_udp_data(void * plsocket,void * udp_st,char * buf,int length)
{
	int last_num = 0;
	int count_num = 0;
	LOCAL_SOCKET * pLsock = (LOCAL_SOCKET *)plsocket;
	LS_UDP_ST *cur= (LS_UDP_ST *)udp_st;
	LS_NET_DATA_ST st_data;

	memcpy(&st_data,buf,sizeof(LS_NET_DATA_ST));

	//DPRINTK("%d %d %d\n",st_data.data_length,st_data.is_mult_data,st_data.mem_key);

	buf += sizeof(LS_NET_DATA_ST);
	length -= sizeof(LS_NET_DATA_ST);
	
	
	if( st_data.is_mult_data == 1 )
	{
		int recv_num = 0;
		int total_num = 0;
		char * pBuf = NULL;

		if( st_data.data_length > LS_RECV_MAX_SIZE || st_data.data_length <= 0)
		{
			DPRINTK("mult length=%d error  \n",st_data.data_length);
			goto read_err;
		}	

		if( cur->start_recv_packet_no == 0)
		{
			//开始了一个新的接受任务。
			if( cur->big_recv_buf )
			{
				free(cur->big_recv_buf);
				cur->big_recv_buf = NULL;
				cur->big_recv_len = 0;
			}
		}

		if( cur->big_recv_buf == NULL)
		{			
			cur->big_recv_buf = (char*)malloc(st_data.data_length);
			if( cur->big_recv_buf  == NULL )
			{
				DPRINTK("malloc err\n");				
				goto read_err;
			}

			cur->big_recv_len = 0;
		}

		total_num = st_data.data_length;

		pBuf = cur->big_recv_buf;
					
		memcpy(pBuf + cur->big_recv_len,buf,st_data.split_length);			

		cur->big_recv_len = cur->big_recv_len + st_data.split_length;			

		if( cur->big_recv_len >=  st_data.data_length)
		{			
			cur->recv_data(plsocket,cur,cur->big_recv_buf,cur->big_recv_len);

			if( cur->big_recv_buf )
			{
				free(cur->big_recv_buf);
				cur->big_recv_buf = NULL;
				cur->big_recv_len = 0;
			}
		}
		
	}else
	{
		cur->recv_data(plsocket,cur,buf,length);
	}

	
	return 1;

read_err:
	return -1;
}



void ls_udp_listen_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	LOCAL_SOCKET * pLsocket = NULL;
	int i,sin_size;
	fd_set watch_set;
	int ret;
	struct  timeval timeout;
	struct sockaddr_in adTemp;
	int tmp_fd = 0;
	struct rudp_packet recv_packet;
	struct sockaddr_in recver;
  	size_t recver_length = sizeof(struct sockaddr_in);

	pLsocket = (LOCAL_SOCKET *)value;		

	sin_size = sizeof(struct sockaddr_in);

	pLsocket->is_listen_work = 1;
	pLsocket->all_thread_flag = 1;

	while( pLsocket->is_listen_work )
	{
		FD_ZERO(&watch_set); 
 		FD_SET(pLsocket->socket,&watch_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1000;

		ret  = select( pLsocket->socket + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "listen socket select error!\n" );
			goto error_end;
		}

		if ( ret > 0 && FD_ISSET( pLsocket->socket , &watch_set ) )
		{	
			ret = recvfrom(pLsocket->socket, &recv_packet, sizeof(struct rudp_packet), 0, (struct sockaddr *)&recver, &recver_length);

			if( ret > 0 )
			{		
				ls_recv_thread_lock(pLsocket);
				
				#ifdef debug_log
				DPRINTK("recv socket=%d startno=%d type=%d pack_num_once=%d seqno=%d\n",pLsocket->socket,recv_packet.header.startno,recv_packet.header.type,
						recv_packet.header.pack_num_once,recv_packet.header.seqno);
				#endif
				
				if(pLsocket->udp_list == NULL) 
				{
			    		pLsocket->recv_udp_data(pLsocket,&recver,&recv_packet,ret);
			  	}else 
			  	{
			  		int find_udp_st = 0;
				    	LS_UDP_ST *cur= pLsocket->udp_list;					
					do
					{
					  	if( compare_sockaddr(&cur->address,&recver))				     
					  	{
					  		find_udp_st = 1;
					       	break;
					  	}
					   	cur = cur->next;
					}while(cur != NULL );
					
					
					if( find_udp_st )		
					{				
					
						if( recv_packet.header.type ==  RUDP_DATA )
						{
							int get_all_data = 1;

							if(recv_packet.header.startno  == 1 )
							{
								struct sockaddr_in *s1 = (struct sockaddr_in *)&cur->address;	
								
								for( i = 0;i < RUDP_WINDOW; i++)
								{
									cur->now_recv_data[i].header.type = 0;									
								}
								cur->start_recv_packet_no = 0;

								#ifdef debug_log
								DPRINTK(" udp recv (%s:%d) reset \n",inet_ntoa(s1->sin_addr),ntohs(s1->sin_port));
								#endif
							}

							if(  (recv_packet.header.startno > cur->start_recv_packet_no )
								&&  (recv_packet.header.startno <= cur->start_recv_packet_no+ RUDP_WINDOW)
								&& ( cur->now_recv_data[recv_packet.header.seqno].header.type == 0 ) )
							{
								cur->now_recv_data[recv_packet.header.seqno] = recv_packet;	

								#ifdef debug_log
								DPRINTK("recv socket=%d startno=%d type=%d pack_num_once=%d seqno=%d  in array\n",pLsocket->socket,recv_packet.header.startno,recv_packet.header.type,
										recv_packet.header.pack_num_once,recv_packet.header.seqno);
								#endif
							}

							if( recv_packet.header.startno <= cur->start_recv_packet_no+ RUDP_WINDOW )
							{
								//如过接受包的序列号超出了估计值范围则不响应。
								recv_packet.header.type = RUDP_ACK;
								ls_send_udp_packet(pLsocket,&recv_packet,&recver);
							}
							
							
							for( i = 0;i < recv_packet.header.pack_num_once; i++)
							{
								if( cur->now_recv_data[i].header.type != RUDP_DATA )
								{
									get_all_data = 0;
								}
							}

							if( get_all_data )
							{
								char buf[RUDP_MAXPKTSIZE*RUDP_WINDOW];
								int data_len = 0;	
								u_int32_t start_recv_packet_no = 0;

								for( i = 0;i < recv_packet.header.pack_num_once; i++)
								{
									memcpy(buf + data_len,cur->now_recv_data[i].payload,cur->now_recv_data[i].payload_length);
									data_len += cur->now_recv_data[i].payload_length;
									cur->now_recv_data[i].header.type = 0;		
									start_recv_packet_no = cur->now_recv_data[i].header.startno;
								}	

								#ifdef debug_log
								DPRINTK("recv all data\n");
								#endif
								ls_recv_udp_data(pLsocket,cur,buf,data_len);	

								#ifdef debug_log
								DPRINTK("recv all data over\n");
								#endif

								cur->start_recv_packet_no = start_recv_packet_no;
								
							}
						}else if( recv_packet.header.type ==  RUDP_ACK )
						{
							int get_all_data = 1;
							
							if( cur->send_data[recv_packet.header.seqno].header.type == RUDP_DATA )
							{
								cur->send_data[recv_packet.header.seqno].header.type = 0;							
							}

							for( i = 0;i < recv_packet.header.pack_num_once; i++)
							{
								if( cur->send_data[i].header.type == RUDP_DATA )
								{
									get_all_data = 0;
								}
							}

							if( get_all_data )
							{
								cur->mem_send_status = LS_MEM_SEND_STATUS_ARRIVE;
								#ifdef debug_log
								DPRINTK("Get all data\n");
								#endif
								pthread_mutex_lock(&cur->mem_send_mutex);			      
							      pthread_cond_signal(&cur->mem_send_block_cond);
							      pthread_mutex_unlock(&cur->mem_send_mutex);
							}

						}else
							{
							}
					}
					else
						pLsocket->recv_udp_data(pLsocket,&recver,&recv_packet,ret);
					
			  	}

				ls_recv_thread_unlock(pLsocket);
			}else
			{
				DPRINTK("time out!\n");					
			}
			
		}	
	
	}

error_end:
	ls_close_socket(pLsocket);	

	
	pLsocket->all_thread_flag = 0;
	pthread_exit(2);	

	return ;
}



int ls_listen(LOCAL_SOCKET * pLsocket,int listen_port,int (*accpet_socket)(void *,int ,char *,int),int (*recv_udp_data)(void *,void *,char *,int))
{
	int ret_no = 0;
	int ret = 0;
	pthread_t td_id;
	struct sockaddr_in server_addr;
	int time_out_num = 0;
	int socket_fd =0 ;
	if( pLsocket == NULL )
	{
		ret_no = LS_SOCKET_NULL_ERR;
		goto listen_err;
	}
	
	if(pLsocket->socket > 0 )
	{
		DPRINTK("This socket is work\n");
		ret_no = LS_SOCKET_ALREADY_WORK_ERR;
		goto listen_err;
	}

	ls_reset_socket(pLsocket);

	pLsocket->socket_port = listen_port;
	pLsocket->accpet_socket = accpet_socket;		
	pLsocket->recv_udp_data = recv_udp_data;

	if(pLsocket->socket_type == SOCK_STREAM)
	{
		ret = ls_socket_create_set(&socket_fd,0,30,1,0,0);
		if( ret < 0 )
		{
			DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
			return ret;
		}	
	}else
	{		
		 socket_fd = socket(AF_INET,SOCK_DGRAM,0);
		 if(socket_fd == -1) 
		 {
		    	DPRINTK( "socket create error socket_fd=%d\n",socket_fd);
			ret_no = LS_SOCKET_BIND_ERR;
			goto listen_err;
		 }
	}	

	pLsocket->socket = socket_fd;
	memset(&server_addr, 0, sizeof(struct sockaddr_in)); 
	server_addr.sin_family=AF_INET; 
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);	
	server_addr.sin_port=htons(listen_port); 

	if ( -1 == (bind(pLsocket->socket,(struct sockaddr *)(&server_addr),
			sizeof(struct sockaddr))))
	{		
		DPRINTK( "socket bind error\n" );
		ret_no = LS_SOCKET_BIND_ERR;
		goto listen_err;
	}


	if(pLsocket->socket_type == SOCK_STREAM)
	{
		ret = pthread_create(&pLsocket->netlisten_id,NULL,(void*)ls_tcp_listen_thread,(void *)pLsocket);
		if ( 0 != ret )
		{	
			DPRINTK( "socket create listen thread error\n" );
			ret_no = LS_SOCKET_CREATE_LISTEN_THREAD_ERR;
			goto listen_err;
		}
	}else
	{
		ret = pthread_create(&pLsocket->netlisten_id,NULL,(void*)ls_udp_listen_thread,(void *)pLsocket);
		if ( 0 != ret )
		{	
			DPRINTK( "socket create listen thread error\n" );
			ret_no = LS_SOCKET_CREATE_LISTEN_THREAD_ERR;
			goto listen_err;
		}
	}

	while(pLsocket->is_listen_work == 0)
	{
		ls_usleep(100000);		
		time_out_num++;

		if( time_out_num > 20)
		{
			DPRINTK("listen err\n");
			ret_no = LS_SOCKET_LISTEN_THREAD_ERR;
			goto listen_err;
		}
	}


	return 1;

listen_err:
	ls_close_socket(pLsocket);

	return ret_no;
}


int ls_send_data_direct(LOCAL_SOCKET * pLsocket,char * buf,int length)
{
	int ret;
	LS_NET_DATA_ST st_data;
	char dest_buf[LS_MAX_BUF_SIZE];
	int send_len = 0;
	int send_once_len = 0;
	
	send_once_len = LS_MAX_BUF_SIZE - sizeof(LS_NET_DATA_ST) - 20;
	

	if(  pLsocket->recv_th_flag != 1 )
	{	
		return LS_SOCKET_WRITE_ERR;
	}

	if( length + sizeof(LS_NET_DATA_ST) >= LS_RECV_MAX_SIZE )	
	{
		DPRINTK("len=%d > %d\n",length,LS_RECV_MAX_SIZE);
		return LS_SOCKET_WRITE_ERR;
	}

	memset(&st_data,0x00,sizeof(LS_NET_DATA_ST));

	st_data.data_length = length;
	st_data.send_mode = LS_DATA_SEND_MODE_NET;
	
	if( length + sizeof(LS_NET_DATA_ST) > LS_MAX_BUF_SIZE )	
	{
		st_data.is_mult_data = 1;

		while(send_len <  st_data.data_length )
		{
			if( send_len + send_once_len <  st_data.data_length )
				st_data.split_length = send_once_len;
			else
				st_data.split_length = st_data.data_length - send_len;

			memcpy(dest_buf,&st_data,sizeof(LS_NET_DATA_ST));
			memcpy(dest_buf + sizeof(LS_NET_DATA_ST),buf + send_len, st_data.split_length);			

			ret = ls_socket_send_data( pLsocket->socket, dest_buf, st_data.split_length + sizeof(LS_NET_DATA_ST));
			if( ret < 0 )
			{		
				DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
				goto err;
			}	

			send_len += st_data.split_length;
		}

	}else
	{		
		memcpy(dest_buf,&st_data,sizeof(LS_NET_DATA_ST));
		memcpy(dest_buf + sizeof(LS_NET_DATA_ST),buf,length);	      

		ret = ls_socket_send_data( pLsocket->socket, dest_buf, length + sizeof(LS_NET_DATA_ST));
		if( ret < 0 )
		{		
			DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
			goto err;
		}
	}

	return 1;

err:

	return LS_SOCKET_WRITE_ERR;
}



int ls_send_data_by_mem(LOCAL_SOCKET * pLsocket,char * buf,int length)
{
	int ret;
	LS_NET_DATA_ST st_data;
	char * dest_buf;
	int send_len = 0;
	int send_once_len = 0;	
	
	if( pLsocket->mem_fd < 0 )
		return LS_SOCKET_WRITE_ERR;		

	send_once_len = pLsocket->mem_max_size - sizeof(LS_NET_DATA_ST) - 20;
	dest_buf = pLsocket->mem_buf_ptr;

	if(  pLsocket->recv_th_flag != 1 )
	{	
		return LS_SOCKET_WRITE_ERR;
	}

	if( length + sizeof(LS_NET_DATA_ST) >= LS_RECV_MAX_SIZE )	
	{
		DPRINTK("len=%d > %d\n",length,LS_RECV_MAX_SIZE);
		return LS_SOCKET_WRITE_ERR;
	}

	memset(&st_data,0x00,sizeof(LS_NET_DATA_ST));

	st_data.data_length = length;
	st_data.send_mode = LS_DATA_SEND_MEM_NET;
	st_data.mem_key = pLsocket->mem_key;

	
	if( length + sizeof(LS_NET_DATA_ST) > LS_MAX_BUF_SIZE )	
	{
		st_data.is_mult_data = 1;		

		
		while(send_len <  st_data.data_length )
		{
			if( send_len + send_once_len <  st_data.data_length )
				st_data.split_length = send_once_len;
			else
				st_data.split_length = st_data.data_length - send_len;
			
			memcpy(dest_buf,buf + send_len, st_data.split_length);		

			pLsocket->mem_send_status = LS_MEM_SEND_STATUS_SENDING;		

			//DPRINTK("data=%d %d\n",st_data.data_length,send_len);

			ret = ls_socket_send_data( pLsocket->socket, &st_data,sizeof(LS_NET_DATA_ST));
			if( ret < 0 )
			{		
				DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
				goto err;
			}	

			send_len += st_data.split_length;

			pthread_mutex_lock(&pLsocket->mem_send_mutex);
			{
				struct timeval now;
				struct timespec locktime;					
				gettimeofday(&now, NULL);
				locktime.tv_sec = now.tv_sec + 1;
				locktime.tv_nsec = now.tv_usec * 1000;	

				if(pLsocket->mem_send_status == LS_MEM_SEND_STATUS_SENDING )
					pthread_cond_timedwait(&pLsocket->mem_send_block_cond, &pLsocket->mem_send_mutex, &locktime);
				
			}
			pthread_mutex_unlock(&pLsocket->mem_send_mutex);			
			if(pLsocket->mem_send_status != LS_MEM_SEND_STATUS_ARRIVE )
			{
				DPRINTK("send mem timeout err\n");
				goto err;
			}		
		}		

	}else
	{				
		memcpy(dest_buf,buf,length);	      
		
		pLsocket->mem_send_status = LS_MEM_SEND_STATUS_SENDING;
		ret = ls_socket_send_data( pLsocket->socket, &st_data,sizeof(LS_NET_DATA_ST));
		if( ret < 0 )
		{		
			DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
			goto err;
		}	

		pthread_mutex_lock(&pLsocket->mem_send_mutex);
		{
			struct timeval now;
			struct timespec locktime;				
			gettimeofday(&now, NULL);
			locktime.tv_sec = now.tv_sec + 1;
			locktime.tv_nsec = now.tv_usec * 1000;
			
			if(pLsocket->mem_send_status == LS_MEM_SEND_STATUS_SENDING )
				pthread_cond_timedwait(&pLsocket->mem_send_block_cond, &pLsocket->mem_send_mutex, &locktime);
		}
		pthread_mutex_unlock(&pLsocket->mem_send_mutex);
		if(pLsocket->mem_send_status != LS_MEM_SEND_STATUS_ARRIVE )
		{
			DPRINTK("send mem timeout err\n");
			goto err;
		}
	}

	return 1;

err:

	return LS_SOCKET_WRITE_ERR;
}


int ls_send_data(LOCAL_SOCKET * pLsocket,char * buf,int length)
{
	int ret = 0;

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	pthread_mutex_lock( &pLsocket->socket_lock);

	if( pLsocket->send_err_flag == 0 )
	{
		if(pLsocket->connect_object == LS_CONNECT_OBJECT_LOCAL )
		{
			if( length < 10*1024 )
				ret = ls_send_data_direct(pLsocket,buf,length);
			else
				ret = ls_send_data_by_mem(pLsocket,buf,length);

			if( ret <  0 )
				pLsocket->send_err_flag = -1;
		}else
		{		
			ret = ls_send_data_direct(pLsocket,buf,length);
			if( ret <  0 )
				pLsocket->send_err_flag = -1;
		}
	}else
	{
		ret = LS_SOCKET_WRITE_ERR;
	}

	pthread_mutex_unlock( &pLsocket->socket_lock);	

	return ret;
}




int ls_send_udp_data(LS_UDP_ST * udp_st,char * buf,int length)
{
	int ret = 0;
	LOCAL_SOCKET * pLsocket = NULL;

	pLsocket = (LOCAL_SOCKET *)udp_st->parent_socket;

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	pthread_mutex_lock( &udp_st->socket_lock);

	if( udp_st->send_err_flag == 0 )
	{			
		ret = ls_send_udp_data_lock(pLsocket,udp_st,buf,length);
		//if( ret <  0 )
		//	udp_st->send_err_flag = -1;
		
	}else
	{
		ret = LS_SOCKET_WRITE_ERR;
	}

	pthread_mutex_unlock( &udp_st->socket_lock);	

	return ret;
}


int ls_send_alive_packet(LOCAL_SOCKET * pLsocket)
{
	int cmd = LS_ALIVE_PACKET_MAC;
	ls_send_data(pLsocket,&cmd,sizeof(int));
}

int ls_open_client_mem(LOCAL_SOCKET * pLsocket,int mem_key)
{
	 #ifndef USING_MINGW
	if( pLsocket->remote_mem_key != mem_key)
	{
		if( pLsocket->remote_mem_fd >= 0 )
		{
			msgctl( pLsocket->remote_mem_fd, IPC_RMID,NULL);
			pLsocket->remote_mem_fd = -1;
		}
	}

	if( pLsocket->remote_mem_fd < 0 )
	{
		pLsocket->remote_mem_fd = shmget( mem_key,0,0 );
		if(pLsocket->remote_mem_fd == -1)
		{
		     DPRINTK("shmget error\n");
		     goto ini_error;
		}	

		pLsocket->remote_mem_buf_ptr = (char*)shmat( pLsocket->remote_mem_fd,( const void* )0,0 );
		if( (int)pLsocket->remote_mem_buf_ptr == -1 )
		{
			DPRINTK("shmat error!\n");
			goto ini_error;
		}

		pLsocket->remote_mem_key = mem_key;

		DPRINTK("remote_mem_key = 0x%x\n",pLsocket->remote_mem_key);
	}
	#endif

	return 1;

ini_error:

	return -1;
}


int ls_command_recv_data(LOCAL_SOCKET * pLsocket)
{
	LS_NET_DATA_ST st_data;
	int ret;
	char buf[LS_MAX_BUF_SIZE];
	char * pBuf = NULL;
	int convert_length = 0;
	int packet_num = 0;
	
	ret = ls_socket_read_data(pLsocket->socket,&st_data,sizeof(LS_NET_DATA_ST));
	if( ret < 0 )
		goto read_err;		
			
	if( st_data.send_mode == LS_DATA_SEND_MEM_NET)
	{
		ret = ls_open_client_mem(pLsocket,st_data.mem_key);
		if( ret < 0 )
			goto read_err;
	}
	
	if( st_data.is_mult_data == 1 )
	{
		int recv_num = 0;
		int total_num = 0;

		if( st_data.data_length > LS_RECV_MAX_SIZE || st_data.data_length <= 0)
		{
			DPRINTK("mult length=%d error  \n",st_data.data_length);
			goto read_err;
		}			
		
		pBuf = (char*)malloc(st_data.data_length);
		if( pBuf == NULL )
		{
			DPRINTK("malloc err\n");
			
			goto read_err;
		}

		total_num = st_data.data_length;

		while( recv_num < total_num)
		{
			if( st_data.send_mode == LS_DATA_SEND_MEM_NET)
			{				
				memcpy(pBuf + recv_num,pLsocket->remote_mem_buf_ptr,st_data.split_length);					

				{
					int cmd = LS_GET_MEM_DATA_OVER;
					ret =ls_send_data(pLsocket,&cmd,sizeof(int));
					if( ret < 0 )
					{
						DPRINTK("send LS_GET_MEM_DATA_OVER err\n");
						goto read_err;
					}
				}
			}
			else
			{
				ret = ls_socket_read_data(pLsocket->socket,pBuf + recv_num,st_data.split_length);
				if( ret < 0 )
					goto read_err;
			}

			recv_num = recv_num + st_data.split_length;

			if( recv_num < total_num )
			{				
				ret = ls_socket_read_data(pLsocket->socket,&st_data,sizeof(LS_NET_DATA_ST));
				if( ret < 0 )
					goto read_err;
			}
		}

		pLsocket->recv_data((void*)pLsocket,pBuf,st_data.data_length);
		
	}else
	{
		if( st_data.data_length > LS_MAX_BUF_SIZE || st_data.data_length <= 0)
		{
			DPRINTK(" length=%d error  \n",st_data.data_length);
			goto read_err;
		}	

		if( st_data.send_mode == LS_DATA_SEND_MEM_NET)
		{
			memcpy(buf,pLsocket->remote_mem_buf_ptr,st_data.data_length);
			{
				int cmd = LS_GET_MEM_DATA_OVER;
				ret =ls_send_data(pLsocket,&cmd,sizeof(int));
				if( ret < 0 )
				{
					DPRINTK("send LS_GET_MEM_DATA_OVER err\n");
					goto read_err;
				}
			}
		}else
		{
			ret = ls_socket_read_data(pLsocket->socket,buf,st_data.data_length);
			if( ret < 0 )
				goto read_err;			
		}

		
		if(st_data.data_length == sizeof(int) )
		{	
			memcpy(&packet_num,buf,sizeof(int));	
			
			if( packet_num == LS_ALIVE_PACKET_MAC)
			{
				//DPRINTK("recv alive packet\n");
			}else if( packet_num == LS_GET_MEM_DATA_OVER)
			{
				//DPRINTK("recv LS_GET_MEM_DATA_OVER packet\n");
				pLsocket->mem_send_status = LS_MEM_SEND_STATUS_ARRIVE;

				pthread_mutex_lock(&pLsocket->mem_send_mutex);			      
			      pthread_cond_signal(&pLsocket->mem_send_block_cond);
			      pthread_mutex_unlock(&pLsocket->mem_send_mutex);
			}
			else
			{
				pLsocket->recv_data((void*)pLsocket,buf,st_data.data_length);
			}
		}else
		{
			pLsocket->recv_data((void*)pLsocket,buf,st_data.data_length);
		}

	}

	if( pBuf )
	{
		free(pBuf);
		pBuf = NULL;
	}

	return 1;	

read_err:
	if( pBuf )
	{
		free(pBuf);
		pBuf = NULL;
	}

	return -1;
}


void ls_net_recv_thread(void * value)
{
	LOCAL_SOCKET * pLsocket = NULL;
	fd_set watch_set;
	struct  timeval timeout;
	int ret;
	int time_out_num = 0;

	DPRINTK("in\n");

	pLsocket = (LOCAL_SOCKET *)value;

	DPRINTK("client = %d\n",pLsocket->socket);

	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);


	while( pLsocket->recv_th_flag == 1)
	{
		FD_ZERO(&watch_set); 
 		FD_SET(pLsocket->socket,&watch_set);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		ret  = select( pLsocket->socket + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "recv socket select error!\n" );
			goto error_end;
		}

		if ( ret > 0 && FD_ISSET( pLsocket->socket, &watch_set ) )
		{
			ret = ls_command_recv_data(pLsocket);
			if( ret < 0 )
			{							
				DPRINTK("NM_command_recv_data error! no=%d\n",pLsocket->socket);
				goto error_end;
			}

			time_out_num = 0;
		}

		if( ret == 0 )
		{
			time_out_num++;
			if( time_out_num > 30 )
			{
				DPRINTK("Can't recv alive packet more than 30 sec\n");
				goto error_end;
			}
		}	
		
	}


error_end:
	ls_close_socket(pLsocket);
	
	DPRINTK("out %d  1\n",pLsocket->socket);
	
	pLsocket->all_thread_flag = 0;
	pthread_exit(2);	
	
	return ;
}


void ls_net_check_thread(void * value)
{	
	LOCAL_SOCKET * pLsocket = (LOCAL_SOCKET *)value;
	int  recv_num = 0;
	int connect_lost_time = 15 * 1000000 / 100000;
	int recv_time = 0;
	int send_alive_time_num = 0;	

	while( ( pLsocket->recv_th_flag == 1 ) && ( pLsocket->check_th_flag == 1)
		&& (pLsocket->all_thread_flag == 1))
	{
		ls_usleep(100000);
		send_alive_time_num++;
		
		if( send_alive_time_num % 20 == 0 )   // 2秒一次。
			ls_send_alive_packet(pLsocket);
	}	
		
	ls_destroy_work_thread(pLsocket);				
	ls_close_socket(pLsocket);

	DPRINTK("out \n");
	pthread_exit(2);
	
	return ;
}



int ls_create_work_thread(LOCAL_SOCKET * pLsocket)
{
	pthread_t netrecv_id;
	pthread_t netcheck_id;
	int ret;

	pLsocket->all_thread_flag = 1;
	pLsocket->recv_th_flag = 1;

	ret = pthread_create(&pLsocket->netrecv_id,NULL,(void*)ls_net_recv_thread,(void *)pLsocket);
	if ( 0 != ret )
	{
		pLsocket->recv_th_flag = 0;
		DPRINTK( "create NM_net_recv_thread thread error\n");	
		goto create_error;
	}

	pLsocket->check_th_flag = 1;
	ret = pthread_create(&pLsocket->netcheck_id,NULL,(void*)ls_net_check_thread,(void *)pLsocket);
	if ( 0 != ret )
	{		
		pLsocket->check_th_flag = 0;
		DPRINTK( "create NM_net_check_thread thread error\n");
		goto create_error;
	}

	ls_usleep(100*1000);

	return 1;

create_error:
	return -1;
	
}


int ls_destroy_work_thread(LOCAL_SOCKET * pLsocket)
{
	int count = 0;	

	if( pLsocket->recv_th_flag == 1 )
	{	
		pLsocket->recv_th_flag = 0;
		DPRINTK("pthread_join netrecv_id start\n");
		pthread_join(pLsocket->netrecv_id,NULL);	
		DPRINTK("pthread_join netrecv_id end\n");		
			
	}
	
	if( pLsocket->is_listen_work == 1 )
	{		
		pLsocket->is_listen_work = 0;	
		DPRINTK("pthread_join netlisten_id start\n");
		pthread_join(pLsocket->netlisten_id,NULL);	
		DPRINTK("pthread_join netlisten_id end\n");		
	}		

	return 1;
}





int ls_socket_create_set(int * client_socket,int client_port,int delaytime,int create_socket,int connect_mode,int socket_buf_size)
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
		if( -1 == (*client_socket =  socket(PF_INET,SOCK_STREAM,0)))
		{
			DPRINTK( "socket initialize error\n" );	
			return LS_SOCKET_INIT_ERR;
		}		
	}

  #ifndef USING_MINGW

	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;

    if (setsockopt( *client_socket, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
    {
		printf( "setsockopt SO_LINGER  error\n" );
		return LS_SOCKET_SET_ERR;	
	}

	
    if (setsockopt(*client_socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(int) ) < 0 )
    {
		printf( "setsockopt SO_KEEPALIVE error\n" );
		return LS_SOCKET_SET_ERR;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle)) < -1)
	{
		printf( "setsockopt TCP_KEEPIDLE error\n" );
		return LS_SOCKET_SET_ERR;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval)) < -1)
	{
		printf( "setsockopt TCP_KEEPINTVL error\n" );
		return LS_SOCKET_SET_ERR;	
	}

	
	if(setsockopt( *client_socket,SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount)) < -1)
	{
		printf( "setsockopt TCP_KEEPCNT error\n" );
		return LS_SOCKET_SET_ERR;	
	}



	i = 1;

	if (setsockopt(*client_socket, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		close( *client_socket );
		*client_socket = 0;
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		return LS_SOCKET_SET_ERR;	
	}		


	  if( delaytime != 0 )
	  {
		stTimeoutVal.tv_sec = delaytime;
		stTimeoutVal.tv_usec = 0;
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	      		close( *client_socket );
			*client_socket = 0;
			printf( "setsockopt SO_SNDTIMEO error\n" );
			return LS_SOCKET_SET_ERR;
		}
			 	
		if ( setsockopt( *client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	       {
	       	close( *client_socket );
			*client_socket = 0;
			printf( "setsockopt SO_RCVTIMEO error\n" );
			return LS_SOCKET_SET_ERR;
		}
	  }

	  if(socket_buf_size != 0)
	  {
	    int recvbuf_len = 100*1024; //实际缓冲区大小的一半。
	    int sndbuf_len = 100*1024; //实际缓冲区大小的一半。
	    int len = sizeof(recvbuf_len);
		
	    recvbuf_len = socket_buf_size;
	    sndbuf_len = socket_buf_size;     
	    DPRINTK("set socket buf size %d\n",sndbuf_len);
			
	    if( setsockopt( *client_socket, SOL_SOCKET, SO_RCVBUF, (void *)&recvbuf_len, len ) < 0 )
	    {
	        	perror("getsockopt: SO_RCVBUF\n");	   
			return LS_SOCKET_SET_ERR;
	    }


	     if( setsockopt( *client_socket, SOL_SOCKET, SO_SNDBUF, (void *)&sndbuf_len, len ) < 0 )
	    {
	        	perror("getsockopt: SO_SNDBUF\n");	  
			return LS_SOCKET_SET_ERR;
	    }
	}

	#endif

	return 1;
	   
}



int ls_connect(LOCAL_SOCKET * pLsocket,char * ip,int port,int (*recv_data)(void *,char *,int))
{
	
	int a;
	int socket_fd = -1;
	int ret;
	struct sockaddr_in addr;	

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;
	
	if(pLsocket->socket > 0 )
	{
		DPRINTK("This socket is work\n");		
		return LS_SOCKET_ALREADY_WORK_ERR;
	}

	#ifndef USING_MINGW
	signal(SIGPIPE,SIG_IGN); 
	#endif
		

	ret = ls_socket_create_set(&socket_fd,0,30,1,0,0);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		return ret;
	}
	
	ls_reset_socket(pLsocket);
	pLsocket->socket= socket_fd;
	pLsocket->recv_data = recv_data;
	strcpy(pLsocket->connect_addr,ip);
	pLsocket->connect_port = port;
	
	addr.sin_family = AF_INET;	
  	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);	

	pLsocket->address = addr;

	DPRINTK("pLsocket->socket=%d ip=%s  port=%d \n",pLsocket->socket,ip,port);
	
	if (connect(pLsocket->socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
		perror(strerror(errno));

		#ifndef USING_MINGW
	  	if (errno == EINPROGRESS) 
		{			
	       	fprintf(stderr, "connect timeout\n");			
		}  
		#endif

		DPRINTK("connect error 123!\n");
		
		goto connect_error;
	 }
	
	
	if( ls_create_work_thread(pLsocket) < 0 )
		goto connect_error;

	
	return 1;

connect_error:
	ls_close_socket(pLsocket);
	
	return -1;
}


int ls_connect_by_accept(LOCAL_SOCKET * pLsocket,int socket_fd,char * connect_ip,int port,int (*recv_data)(void * ,char * ,int ))
{
	char ip_buf[200];
	int a;	
	int ret;
	struct sockaddr_in addr;

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;


	ret = ls_socket_create_set(&socket_fd,0,30,0,0,0);
	if( ret < 0 )
	{
		DPRINTK("NM_client_socket_set error,ret=%d\n",ret);
		return ret;
	}

	DPRINTK("accpet socket =%d  \n",socket_fd);
	ls_reset_socket(pLsocket);
	pLsocket->socket= socket_fd;	
	strcpy(pLsocket->connect_addr,connect_ip);
	pLsocket->connect_port=port;
	pLsocket->recv_data = recv_data;

	addr.sin_family = AF_INET;	
  	addr.sin_addr.s_addr = inet_addr(connect_ip);
	addr.sin_port = htons(port);	

	pLsocket->address = addr;

	if( ls_create_work_thread(pLsocket) < 0 )
		goto connect_error;


	return 1;

connect_error:
	ls_close_socket(pLsocket);	
	return -1;
}

int ls_check_socket_iswork(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	if( pLsocket->init_flag == 0 )
		return LS_SOCKET_NOT_INIT_ERR;
	
	if( (pLsocket->is_listen_work == 1 || pLsocket->check_th_flag == 1)  &&
		(pLsocket->all_thread_flag == 1))
		return 1;

	return 0;
}

int ls_check_socket_is_not_clear(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	if( pLsocket->init_flag == 0 )
		return LS_SOCKET_NOT_INIT_ERR;
	
	if( pLsocket->is_listen_work == 1 || pLsocket->check_th_flag == 1  || 
		pLsocket->recv_th_flag == 1 )
		return 1;

	return 0;
}

int ls_stop_socket_work(LOCAL_SOCKET * pLsocket)
{
	int count = 0;

	#ifdef debug_log
	DPRINTK("is_listen_work=%d recv_th_flag=%d check_th_flag=%d\n",
		pLsocket->is_listen_work,pLsocket->recv_th_flag ,pLsocket->check_th_flag);
	#endif
	
	if( pLsocket->is_listen_work == 1 || pLsocket->recv_th_flag == 1  )
	{		
		ls_destroy_work_thread(pLsocket);		
	}	

	if( pLsocket->check_th_flag == 1 )
	{
		pLsocket->check_th_flag = 0;	

		#ifdef debug_log
		DPRINTK("pthread_join netcheck_id start\n");
		#endif
		pthread_join(pLsocket->netcheck_id,NULL);		

		#ifdef debug_log
		DPRINTK("pthread_join netcheck_id end\n");	
		#endif
	}	

	return 1;
}

int  ls_ini_udp_st(LS_UDP_ST * udp_st,char * ip,int port,int (*recv_data)(void *,void *,char *,int))
{
	struct hostent* hp;
  	struct in_addr *addr;	

	if( udp_st == NULL )
	{
		DPRINTK("NULL err\n");
		return -1;
	}

	memset(udp_st,0x00,sizeof(LS_UDP_ST));

	pthread_mutex_init(&udp_st->socket_lock, NULL);
	pthread_mutex_init(&udp_st->mem_send_mutex, NULL);
      pthread_cond_init(&udp_st->mem_send_block_cond, NULL);
	pthread_mutex_init(&udp_st->need_response_lock, NULL);
	
	
	
	 if ((hp = gethostbyname(ip)) == NULL || 
	   (addr = (struct in_addr *)hp->h_addr) == NULL) 
	 {
	   		DPRINTK("Can't locate host \"%s\"\n", ip); 
	   		return NULL;
	 }
 
    	udp_st->id_key = rand();
    	udp_st->address.sin_family = AF_INET;
      udp_st->address.sin_port = htons(port);
    	memcpy(&udp_st->address.sin_addr, addr, sizeof(struct in_addr));
	udp_st->recv_data = recv_data;
	udp_st->is_init = 1;

	return 1;
}

int ls_destroy_udp_st(LS_UDP_ST * udp_st)
{
	if( udp_st->is_init == 1)
	{
		pthread_mutex_destroy(&udp_st->socket_lock);	
		pthread_mutex_destroy(&udp_st->mem_send_mutex);	
		pthread_cond_destroy(&udp_st->mem_send_block_cond);
		pthread_mutex_destroy(&udp_st->need_response_lock);		

		if( udp_st->self_data)
		{
			free(udp_st->self_data);	
		}

		udp_st->is_init = 0;
	}

	return 1;
}

int ls_recv_thread_lock(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket->init_flag == 1)
	{
		pthread_mutex_lock( &pLsocket->socket_recv_lock);
	}

	return 1;
}

int ls_recv_thread_unlock(LOCAL_SOCKET * pLsocket)
{
	if( pLsocket->init_flag == 1)
	{
		pthread_mutex_unlock( &pLsocket->socket_recv_lock);
	}

	return 1;
}


int ls_add_udp_task_to_socket(LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st)	
{
	int ret = 0;

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;

	udp_st->parent_socket = pLsocket;	

	if(pLsocket->udp_list == NULL) 
	{
    		pLsocket->udp_list = udp_st;			
  	}else 
  	{
	    	LS_UDP_ST *curr = pLsocket->udp_list;		
	
		 while(curr->next != NULL) 
		 {
		 	if( curr == udp_st )
			{
				DPRINTK("aready in task\n");
				return 1;
			}
			
		   	curr = curr->next;
	      }		
		 
	    	curr->next = udp_st;
  	}	
	
	return 1;
}

int ls_del_udp_task_to_socket(LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st)	
{
	int ret = 0;

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;	

	if(pLsocket->udp_list == NULL) 
	{
    		
  	}else 
  	{
	    LS_UDP_ST *curr = pLsocket->udp_list;
	    LS_UDP_ST * pre = NULL;
	    int find_item = 0;
	    	
	    do	
	    {
		 	if( curr->id_key == udp_st->id_key)
		 	{
		 		find_item = 1;
		 		break;
		 	}
				
			pre = curr;
		   	curr = curr->next;
	     } while(curr != NULL);
	    

	    if( find_item == 1)
	    {
	    	 if( pre == NULL )
	    	 {
	    	 	pLsocket->udp_list = curr->next;
	    	 }else
	    	 {
	    	 	pre->next = curr->next;
	    	 }
	    }	   
    	 
  	}	
	
	return 1;
}

int ls_del_udp_task(LS_UDP_ST * udp_st)	
{
	int ret = 0;
	LOCAL_SOCKET * pLsocket = NULL;

	pLsocket = udp_st->parent_socket;	

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;	

	if(pLsocket->udp_list == NULL) 
	{
    		
  	}else 
  	{
	    LS_UDP_ST *curr = pLsocket->udp_list;
	    LS_UDP_ST * pre = NULL;
	    int find_item = 0;
	    	
	    do	
	    {
		 	if( curr->id_key == udp_st->id_key)
		 	{
		 		find_item = 1;
		 		break;
		 	}
				
			pre = curr;
		   	curr = curr->next;
	     } while(curr != NULL);
	    

	    if( find_item == 1)
	    {
	    	 if( pre == NULL )
	    	 {
	    	 	pLsocket->udp_list = curr->next;
	    	 }else
	    	 {
	    	 	pre->next = curr->next;
	    	 }
	    }	   
    	 
  	}	
	
	return 1;
}



int ls_send_udp_packet( LOCAL_SOCKET * pLsocket,struct rudp_packet *p, struct sockaddr_in *recipient) 
{
	#ifdef debug_log
	DPRINTK("socket=%d startno=%d type=%d pack_num_once=%d seqno=%d \n",pLsocket->socket,p->header.startno,p->header.type,
	p->header.pack_num_once,p->header.seqno);
	#endif
	
    	if (sendto(pLsocket->socket, p, sizeof(struct rudp_packet), 0, (struct sockaddr*)recipient, sizeof(struct sockaddr_in)) < 0) 
	{
      		fprintf(stderr, "rudp_sendto: sendto failed\n");
      		return -1;
    	}		
  
  	return 1;
}

int ls_send_udp_data_direct( LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st,void * buf, int length) 
{
	int send_num = 0;
	u_int32_t once_send_num = 0;
	int i = 0;	
	int ret = 1;
	int retrans_data_num = RUDP_MAXRETRANS;
	int send_total_num = 0;	
	

	for( i = 0; i < RUDP_WINDOW;i++ )
	{	
		if( udp_st->start_send_packet_no ==  0 )
		{			
			struct sockaddr_in *s1 = (struct sockaddr_in *)&udp_st->address;	
			udp_st->start_send_packet_no = 1;
			#ifdef debug_log
			DPRINTK("send udp (%s:%d) reset \n",inet_ntoa(s1->sin_addr),ntohs(s1->sin_port),length);	
			#endif
		}
	
		udp_st->send_data[i].header.type = RUDP_DATA;
		udp_st->send_data[i].header.seqno = i;
		udp_st->send_data[i].header.startno = udp_st->start_send_packet_no;

		if( send_num + RUDP_MAXPKTSIZE > length)
			once_send_num = length - send_num;
		else
			once_send_num = RUDP_MAXPKTSIZE;
		
		memcpy(udp_st->send_data[i].payload,buf+ send_num,once_send_num);
		udp_st->send_data[i].payload_length = once_send_num;
		
		send_total_num++;
		send_num += once_send_num;		
		udp_st->start_send_packet_no++;
		
		if( send_num >= length)
			break;
	}



	for( i = 0; i < send_total_num;i++ )
	{
		udp_st->send_data[i].header.pack_num_once = send_total_num;
	}	

	udp_st->mem_send_status = LS_MEM_SEND_STATUS_SENDING;	
send_data:
	
	for( i = 0; i < RUDP_WINDOW;i++ )
	{
		if( udp_st->send_data[i].header.type == RUDP_DATA )
		{				
			#ifdef test_lsot_send_packet_app
			int ran_num = rand() / 10;

			if( ran_num % 8 ==  0 )
			{
			}else
			{
				if( ls_send_udp_packet(pLsocket,&udp_st->send_data[i],&udp_st->address) < 0 )
				{	
					ret = LS_SOCKET_WRITE_ERR;
					goto err;
				}	
			}

			#else
			if( ls_send_udp_packet(pLsocket,&udp_st->send_data[i],&udp_st->address) < 0 )
			{	
				ret = LS_SOCKET_WRITE_ERR;
				goto err;
			}		
			#endif
		}			
	}	
	

	
	pthread_mutex_lock(&udp_st->mem_send_mutex);
	{
		struct timeval now;
		struct timespec locktime;	
		struct timeval delay;   
		struct timeval timeout_time;
		
		gettimeofday(&now, NULL);	
		delay.tv_sec = RUDP_TIMEOUT/1000;
    		delay.tv_usec= (RUDP_TIMEOUT%1000) * 1000;			
    		timeradd(&now, &delay, &timeout_time);
		
		locktime.tv_sec = timeout_time.tv_sec;			
		locktime.tv_nsec = timeout_time.tv_usec * 1000;		

		if(udp_st->mem_send_status == LS_MEM_SEND_STATUS_SENDING )
			pthread_cond_timedwait(&udp_st->mem_send_block_cond, &udp_st->mem_send_mutex, &locktime);
		
	}
	pthread_mutex_unlock(&udp_st->mem_send_mutex);			
	if(udp_st->mem_send_status != LS_MEM_SEND_STATUS_ARRIVE  && 
		retrans_data_num > 0)
	{
		#ifdef debug_log
		DPRINTK("send mem timeout err retrans_data_num=%d\n",retrans_data_num);
		#endif
		retrans_data_num--;
		goto send_data;
	}	

	if( retrans_data_num <= 0 )
	{
		ret = LS_SOCKET_TIME_OUT_ERR;
	}		
	
	return ret;

err:
	return ret;
}


int ls_send_udp_data_lock( LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st,void * buf, int length) 
{
	int ret;
	LS_NET_DATA_ST st_data;
	char dest_buf[LS_MAX_BUF_SIZE];
	int send_len = 0;
	int send_once_len = 0;	

	if( pLsocket == NULL )
		return LS_SOCKET_NULL_ERR;
	
	send_once_len = RUDP_MAXPKTSIZE*RUDP_WINDOW  - sizeof(LS_NET_DATA_ST);

	if( length + sizeof(LS_NET_DATA_ST) >= LS_RECV_MAX_SIZE )	
	{
		DPRINTK("len=%d > %d\n",length,LS_RECV_MAX_SIZE);
		return LS_SOCKET_WRITE_ERR;
	}

	memset(&st_data,0x00,sizeof(LS_NET_DATA_ST));

	st_data.data_length = length;
	st_data.send_mode = LS_DATA_SEND_MODE_NET;	

	
	if( length + sizeof(LS_NET_DATA_ST) > RUDP_MAXPKTSIZE*RUDP_WINDOW )	
	{
		st_data.is_mult_data = 1;

		while(send_len <  st_data.data_length )
		{
			if( send_len + send_once_len <  st_data.data_length )
				st_data.split_length = send_once_len;
			else
				st_data.split_length = st_data.data_length - send_len;

			memcpy(dest_buf,&st_data,sizeof(LS_NET_DATA_ST));
			memcpy(dest_buf + sizeof(LS_NET_DATA_ST),buf + send_len, st_data.split_length);			

			ret = ls_send_udp_data_direct( pLsocket, udp_st, dest_buf, st_data.split_length + sizeof(LS_NET_DATA_ST));
			if( ret < 0 )
			{		
				DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
				goto err;
			}	

			send_len += st_data.split_length;
		}

	}else
	{		
		memcpy(dest_buf,&st_data,sizeof(LS_NET_DATA_ST));
		memcpy(dest_buf + sizeof(LS_NET_DATA_ST),buf,length);	      

		ret = ls_send_udp_data_direct( pLsocket,udp_st, dest_buf, length + sizeof(LS_NET_DATA_ST));
		if( ret < 0 )
		{		
			DPRINTK("NM_socket_send_data error! no=%d\n",pLsocket->socket);
			goto err;
		}
	}

	return 1;

err:

	return LS_SOCKET_WRITE_ERR;
}


void ls_usleep(int million_sec)
{	
	#ifdef _WIN32
		int sleep_sec = 0;
		sleep_sec = million_sec/1000;
		
		if( sleep_sec <= 0 )
			sleep_sec = 1;		
		Sleep(sleep_sec);
	#else
		usleep(million_sec);
	#endif
}

