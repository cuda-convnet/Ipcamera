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

#include <sys/shm.h>
#include <sys/ipc.h>

#include "share_mem.h"
#include "devfile.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif


MEM_MODULE_ST * MEM_init_share_mem(int chan,int buf_size,int is_svr,int (*func)(void *,char *,int,int))
{
	MEM_MODULE_ST * pst = NULL;
	MEM_START_ST start_st;
	MEM_CLIENT_ST client_st;
	MEM_SRV_ST	srv_st;	
	char * mem;
	int use_key;
	int msg_key;

	use_key = COMMEN_KEY+ chan;
	msg_key = COMMEN_MSG_KEY + chan;

	pst = (MEM_MODULE_ST *)malloc(sizeof(MEM_MODULE_ST));

	if( pst == NULL )
	{
		DPRINTK("malloc error!\n");
		goto ini_error;
	}

	memset(pst,0x00,sizeof(MEM_MODULE_ST));
	pst->client = -1;
	pst->msg_fd1 = -1;
	pst->msg_fd2 = -1;

	if( is_svr == 1 )
	{
		pst->client  = shmget(use_key ,1028,0640);     
	      if(pst->client != -1)
	      {
	         	mem = (char *)shmat(pst->client,NULL,0);
	        	if ( (int)mem != (void *)-1)
		      {	      		
		          	shmctl(pst->client,IPC_RMID,0) ;
		      }
	      }

		pst->client = shmget(use_key,buf_size,0640|IPC_CREAT|IPC_EXCL); 
		if(pst->client == -1)
		{
		     DPRINTK("shmget error\n");
		     goto ini_error;
		}
	}else
	{
		pst->client = shmget( use_key,0,0 );
		if(pst->client ==-1)
		{
		     DPRINTK("shmget error\n");
		     goto ini_error;
		}		
	}

	pst->buf_ptr = (char*)shmat( pst->client,( const void* )0,0 );

	if( (int)pst->buf_ptr == -1 )
	{
		DPRINTK("shmat error!\n");
		goto ini_error;
	}

	pst->totol_info_ptr = pst->buf_ptr;
	pst->srv_info_ptr = pst->buf_ptr+sizeof(MEM_START_ST);
	pst->client_info_ptr = pst->buf_ptr+sizeof(MEM_START_ST) + sizeof(MEM_SRV_ST);
	pst->head_ptr = pst->buf_ptr+sizeof(MEM_START_ST) + sizeof(MEM_SRV_ST) + sizeof(MEM_CLIENT_ST);
	pst->data_ptr = pst->buf_ptr+sizeof(MEM_START_ST) + sizeof(MEM_SRV_ST) + sizeof(MEM_CLIENT_ST) + sizeof(MEM_OP_ST);

	pst->dvr_cam_no = chan;

	
	pst->is_alread_work = 0;	
	pst->work_mode = 0;
	pst->recv_data_num = 0;
	pst->bind_enc_num[0] = -1;
	pst->bind_enc_num[1] = -1;
	pst->bind_enc_num[2] = -1;
	pst->bind_enc_num[3] = -1;
	pst->bind_enc_num[4] = -1;
	pst->bind_enc_num[5] = -1;
	pst->thread_stop_num = 0;	
	pst->is_svr = is_svr;
	pst->buf_max_size = buf_size - 1000;

	pthread_mutex_init(&pst->lock, NULL);

	pst->net_get_data = func;
	
	if( is_svr == 1 )
	{
		 if((pst->msg_fd1=msgget(msg_key,0640))==-1)
		 {
		 	
		 }

		 if((pst->msg_fd2=msgget(msg_key+100,0640))==-1)
		 {
		 	
		 }

	
		if( pst->msg_fd2 >= 0 )
		{
			msgctl( pst->msg_fd2, IPC_RMID,NULL);
		}
	
		if( pst->msg_fd1 >= 0 )
		{
			msgctl( pst->msg_fd1, IPC_RMID,NULL);
		}

		if((pst->msg_fd1=msgget(msg_key,IPC_CREAT|0666))==-1)
		 {
		 	DPRINTK("msgget error!\n");
			goto ini_error;
		 }

		 if((pst->msg_fd2=msgget(msg_key+100,IPC_CREAT|0666))==-1)
		 {
		 	DPRINTK("msgget error!\n");
			goto ini_error;
		 }
	}else
	{
		if((pst->msg_fd1=msgget(msg_key,0640))==-1)
		 {
		 	DPRINTK("msgget error! chan=%d\n",chan);
			goto ini_error;
		 }

		 if((pst->msg_fd2=msgget(msg_key+100,0640))==-1)
		 {
		 	DPRINTK("msgget error! chan=%d\n",chan);
			goto ini_error;
		 }

		 if(1)
		 {
			int ret;
			struct my_msg_st mst_st;
			mst_st.my_msg_type = 1;

			DPRINTK("check msg_fd2 have item\n");

			//这句话确保msg_fd2 通道中没有数据存在。
			if((ret=msgrcv(pst->msg_fd2,&mst_st,MAX_TEXT,1,IPC_NOWAIT)) < 0)      //msgsiz 为sizeof(mtext[]),而非sizeof(s_msg)
	            		DPRINTK("msgrcv error\n");
			
		 }
	}
	 

	DPRINTK("[%d] buf_size=%d  client=%d  msg(%d,%d) is_svr=%d\n",pst->dvr_cam_no,buf_size,pst->client,
		pst->msg_fd1,pst->msg_fd2,is_svr);
	

	if( is_svr == 1 )
	{
		if( MEM_create_work_thread(pst) < 0 )
		{
			DPRINTK("MEM_create_work_thread error! chan=%d\n",chan);
			goto ini_error;
		}
	}	

	return pst;



ini_error:

	if( pst )
	{
		if( pst->msg_fd2 >= 0 )
		{
			msgctl( pst->msg_fd2, IPC_RMID,NULL);
		}
	
		if( pst->msg_fd1 >= 0 )
		{
			msgctl( pst->msg_fd1, IPC_RMID,NULL);
		}
		
		if( pst->buf_ptr )
		{
			shmctl(pst->client,IPC_RMID,0) ;
		}
		
		free(pst);
	}

	return NULL;
}

int MEM_destroy_share_mem(void * value)
{
	MEM_MODULE_ST * pst = NULL;
	fd_set watch_set;
	struct  timeval timeout;
	int ret;
	
	pst = (MEM_MODULE_ST *)value;

	MEM_destroy_work_thread(pst);

	if( pst )
	{	
		if( pst->msg_fd2 >= 0 )
		{
			msgctl( pst->msg_fd2, IPC_RMID,NULL);
		}
	
		if( pst->msg_fd1 >= 0 )
		{
			msgctl( pst->msg_fd1, IPC_RMID,NULL);
		}
		
		if( pst->buf_ptr )
		{
			shmctl(pst->client,IPC_RMID,0) ;
		}
		
		free(pst);
	}

	return 1;
}


void MEM_net_recv_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	MEM_MODULE_ST * pst = NULL;
	fd_set watch_set;
	struct  timeval timeout;
	int ret;
	char buf[MEM_MAX_FRAME_BUF_SIZE];	
	int len;
	struct my_msg_st mst_st;
	
	pst = (MEM_MODULE_ST *)value;


	while( pst->recv_th_flag == 1)
	{
		if(msgrcv(pst->msg_fd1,&mst_st,MAX_TEXT,1,0) < 0)      //msgsiz 为sizeof(mtext[]),而非sizeof(s_msg)
            		DPRINTK("msgrcv error %d\n",pst->dvr_cam_no);
		
		ret = MEM_read_data_lock(pst,buf,&len,0);
		if( ret > 0 )
		{
			
		}else
		{
			DPRINTK("MEM_read_data error! %d\n",pst->dvr_cam_no);
		}
		

		mst_st.my_msg_type = 1;
		//sprintf(mst_st.msg_text,"2 send %d\n",*(int*)buf);
		if(msgsnd(pst->msg_fd2,&mst_st,MAX_TEXT,0) < 0)      //msgsiz 为sizeof(mtext[]),而非sizeof(s_msg)
           		DPRINTK("msgsnd error %d\n",pst->dvr_cam_no);		
		
	}


error_end:

	pst->recv_th_flag = -1;
	pthread_detach(pthread_self());
	
	return ;
}


int MEM_destroy_work_thread(MEM_MODULE_ST * pst)
{
	int count = 0;

	DPRINTK("no=%d  socket=%d 11\n",pst->dvr_cam_no,pst->client);

	if( pst->recv_th_flag == 1 )
	{	
		pst->recv_th_flag = 0;

		count = 0;

		while(1)
		{
			usleep(10000);

			if( pst->recv_th_flag == - 1 )
			{
				pst->recv_th_flag = 0;
			
				DPRINTK("NM_net_recv_thread out! %d\n",pst->client);

				break;
			}else
			{
				count++;

				if( count > 30 )
				{
					pst->recv_th_flag = 0;
					DPRINTK("NM_net_recv_thread force out! 1 %d\n",pst->client);				
					break;
				}
			}
		}
	}else
	{
		pst->recv_th_flag = 0;
	}

	DPRINTK("out\n");

	pthread_detach(pthread_self());

	return 1;
}


int MEM_create_work_thread(MEM_MODULE_ST * pst)
{
	pthread_t netsend_id;
	pthread_t netrecv_id;
	pthread_t netcheck_id;
	int ret;


	pst->recv_th_flag = 1;

	ret = pthread_create(&pst->netrecv_id,NULL,(void*)MEM_net_recv_thread,(void *)pst);
	if ( 0 != ret )
	{
		pst->recv_th_flag = 0;
		DPRINTK( "create NM_net_recv_thread thread error\n");	
		goto create_error;
	}	

	usleep(1000);
	

	DPRINTK("ok\n");

	pthread_detach(pthread_self());
	
	return 1;


create_error:

	MEM_destroy_work_thread(pst);

	pthread_detach(pthread_self());

	return -1;
}


int MEM_write_data(MEM_MODULE_ST * pst,char * buf, int size,int delay_time)
{
	MEM_OP_ST op_st;
	char msg_buf[40];
	int ret;
		
	if( pst->client < 0 )
	{
		DPRINTK("client =%d\n",pst->client);
		return -1;
	}

	if( pst->is_svr == 1 )
	{
		DPRINTK("only client can write. is_svr=%d\n",pst->is_svr);
		return -1;
	}

	if( size > pst->buf_max_size )
	{
		DPRINTK("size=%d > max=%d\n",size,pst->buf_max_size);
	}


	if(1)
	{			
		
		op_st.data_size = size;	

		memcpy(pst->data_ptr,buf,size);

		memcpy(pst->head_ptr,&op_st,sizeof(MEM_OP_ST));
	}else
	{
		memcpy(&op_st,pst->head_ptr,sizeof(MEM_OP_ST));
		if( op_st.mem_event != DATA_NO )
		{		
			usleep(1000);
			return -1;				
		}

		op_st.chan = pst->dvr_cam_no;
		
		if( pst->is_svr )
			op_st.type = IS_SRV;
		else
			op_st.type = IS_CLI;
		
		op_st.mem_event = DATA_OP;
		op_st.data_size = size;
		
		memcpy(pst->head_ptr,&op_st,sizeof(MEM_OP_ST));

		memcpy(pst->data_ptr,buf,size);

		op_st.mem_event = DATA_WRITE;

		memcpy(pst->head_ptr,&op_st,sizeof(MEM_OP_ST));	
	}

	{
		struct my_msg_st mst_st;
		mst_st.my_msg_type = 1;
		//sprintf(mst_st.msg_text,"send %d\n",*(int*)buf);
		if(msgsnd(pst->msg_fd1,&mst_st,MAX_TEXT,0) < 0)      //msgsiz 为sizeof(mtext[]),而非sizeof(s_msg)
	       {
	       	DPRINTK("msgsnd error\n");
			DPRINTK("%s\n",strerror(errno));
			return -1;	

		}

		//DPRINTK("send msg %d\n",*(int*)buf);

		if((ret=msgrcv(pst->msg_fd2,&mst_st,MAX_TEXT,1,0)) < 0)      //msgsiz 为sizeof(mtext[]),而非sizeof(s_msg)
            	{
            		DPRINTK("msgrcv error\n");
			return -1;	

		}

		//DPRINTK("recv msg %s  num=%d\n",mst_st.msg_text,ret);

	}

	
	
	
	return 1;
}

int MEM_write_data_lock(MEM_MODULE_ST * pst,char * buf, int size,int delay_time)
{
	int ret;
	pthread_mutex_lock(&pst->lock);
	ret = MEM_write_data(pst,buf,size,delay_time);
	pthread_mutex_unlock(&pst->lock);

	return ret;
}


int MEM_read_data(MEM_MODULE_ST * pst,char * buf, int *size,int delay_time)
{
	MEM_OP_ST op_st;
		
	if( pst->client < 0 )
	{
		DPRINTK("client =%d\n",pst->client);
		return -1;
	}

	if( pst->is_svr != 1 )
	{
		DPRINTK("only svr can read. is_svr=%d\n",pst->is_svr);
		return -1;
	}
		

	if(1)
	{
		memcpy(&op_st,pst->head_ptr,sizeof(MEM_OP_ST));
		pst->net_get_data(pst,pst->data_ptr,op_st.data_size,NET_DVR_NET_FRAME);		

	}else
	{
		memcpy(&op_st,pst->head_ptr,sizeof(MEM_OP_ST));	
		if( op_st.mem_event != DATA_WRITE )
		{		
			usleep(1000);
			return -1;			
		}

		pst->net_get_data(pst,pst->data_ptr,op_st.data_size,NET_DVR_NET_FRAME);

		op_st.mem_event = DATA_NO;

		memcpy(pst->head_ptr,&op_st,sizeof(MEM_OP_ST));	
	}

	*size = op_st.data_size;

	
	return 1;
}

int MEM_read_data_lock(MEM_MODULE_ST * pst,char * buf, int *size,int delay_time)
{	
	int ret;
	pthread_mutex_lock(&pst->lock);
	ret = MEM_read_data(pst,buf,size,delay_time);
	pthread_mutex_unlock(&pst->lock);

	return ret;
}




