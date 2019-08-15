#include "netserver.h"
#include <errno.h>
#include "P2pClient.h"
#include "Hisi_dev.h"
#include "G711_lib.h"


#define SEND_DELAY_TIME 30

extern BUFLIST g_othreinfolist;
NETSOCKET socket_ctrl;
NET_MAC_SET_PARA dvr_mac_para;
int change_ip_flag = 0;
int close_client = 0;
time_t g_search_time = 0;
NET_COMMAND g_cmd;
char gui_msg[GUI_MSG_MAX_NUM];
static int g_net_file_play_change_play_pos = 0;
static int g_mac_random_num = 0;
static int g_net_mode = TD_DRV_VIDEO_SIZE_CIF;
char upnp_ip[200]={0};
int upnp_port = 0;
int enable_upnp = 0;

pthread_mutex_t socket_accept_mutex;

void  lock_accept_mutex()
{
	int ret = 0;
	static int first = 0;	
	if( first == 0 )
	{
		pthread_mutex_init(&socket_accept_mutex,NULL);			
		first = 1;
	}	
	
	pthread_mutex_lock(&socket_accept_mutex);	
}

void  unlock_accept_mutex()
{
	pthread_mutex_unlock(&socket_accept_mutex);
}

int Net_dvr_get_net_mode()
{
	return g_net_mode;
}

int get_max_fd(NETSOCKET * server_ctrl)
{
	int i = 0;
	int maxfd = 0;
	
	for ( i = 0; i < MAXCLIENT; i++ )
	{
		if ( server_ctrl->clientfd[i] > 0 )
		{
			maxfd = server_ctrl->clientfd[i] > maxfd ? server_ctrl->clientfd[i] : maxfd;
		}
	}

	maxfd = server_ctrl->listenfd > maxfd ? server_ctrl->listenfd : maxfd;

	return maxfd;
}

int show_led_info(int flag)
{
	KEYBOARD_LED_INFO * p_info = NULL;
	static char led1,led2;

	if( flag == 1 )
	{
		return 1;
	}
	else
	{
		p_info = get_keyboard_led_info();	
		if( (led1 != p_info->led1) || (led2 != p_info->led2) )
		{
			led1 = p_info->led1;
			led2 = p_info->led2;
			return 1;
		}
		else
			return 0;
	}

	return 0;
}

int socket_read_data( int idSock, char* pBuf, int ulLen )
{
	int ulReadLen = 0;
	int iRet = 0;

	if ( idSock <= 0 )
	{
		return SOCKET_ERROR;
	}	

	while( ulReadLen < ulLen )
	{		
		iRet = recv( idSock, pBuf, ulLen - ulReadLen, 0 );
		if ( iRet <= 0 )
		{
			DPRINTK("ulReadLen=%d fd=%d\n",iRet,idSock);
			return SOCKET_READ_ERROR;
		}
		ulReadLen += iRet;
		pBuf += iRet;
	}

	
	
	return 1;
}

int socket_send_data( int idSock, char* pBuf, int ulLen )
{
	int lSendLen;
	int lStotalLen;
	int lRecLen;
	char* pData = pBuf;

	lSendLen = ulLen;
	lStotalLen = lSendLen;
	lRecLen = 0;

	if ( idSock <= 0 )
	{
		return SOCKET_ERROR;
	}	

	
	
	while( lRecLen < lStotalLen )
	{
		
		lSendLen = send( idSock, pData, lStotalLen - lRecLen, 0 );			
		if ( lSendLen <= 0 )
		{
			DPRINTK("lSendLen=%d\n",lSendLen);
			return SOCKET_SEND_ERROR;
		}
		lRecLen += lSendLen;
		pData += lSendLen;
	}

	

	return 1;
}



int safe_socket_send_data( NETSOCKET * server_ctrl, int idx, char* pBuf, int ulLen )
{
	int ret,wait_sec;

	while(server_ctrl->close_socket_flag[idx] == 1 )
	{
		DPRINTK("socket close now,wait \n");
		sleep(1);
		wait_sec++;
		if( wait_sec > 20 )
		{
			DPRINTK("time out break\n");
			return -1;;
		}
	}

	pthread_mutex_lock(&server_ctrl->send_mutex[idx]);

	server_ctrl->send_flag[idx] = 1;

	ret = socket_send_data(server_ctrl->clientfd[idx],pBuf, ulLen);

	server_ctrl->send_flag[idx] = 0;

	pthread_mutex_unlock(&server_ctrl->send_mutex[idx]);	

	return ret;
}


 void  net_send_timeout()
 {   
 	 printf("send time out! 30 %d\n",SEND_DELAY_TIME);	

 	 #ifdef NET_USE_NEW_FUN
 	 	 DPRINTK("use new fun net\n");
     	 close_client = 1;
     #else     	
		 clear_client(socket_ctrl.cur_client_idx, &socket_ctrl);  
     #endif
 }  

 int check_play_status(NETSOCKET * server_ctrl)
 {
 	int idx;
	NET_COMMAND cmd;
	int ret;
	static int play_count = 0;

	if( g_net_file_play_change_play_pos == 1 )
	{
		DPRINTK("change play pos ,not stop!\n");
		return 1;
	}
	
 	if( server_ctrl->net_play_mode == NET_FILE_PLAYING &&
		(g_pstcommand->GST_FTC_get_play_status() == 1 ||play_count > 0 ) )
 	{
 		play_count = 1;
		
 		if( g_pstcommand->GST_FTC_get_play_status() == 0  )
		{
			g_pstcommand->GST_FTC_StopPlay();
			g_pstcommand->GST_FTC_AudioPlayCam(-1);
			printf(" net close play file!\n");
			
			for( idx = 0; idx < MAXCLIENT; idx++ )
			{
				if( server_ctrl->client_send_data[idx] == 0 )
					continue;
				
				server_ctrl->net_play_mode = 0;
				server_ctrl->client_send_data[idx] = 0;

				play_count = 0;

				cmd.command = PLAY_STOP;

				start_stop_net_file_view(0);

				ret = safe_socket_send_data(server_ctrl,idx, (char*)&cmd, sizeof(NET_COMMAND));
				if ( ret <= 0 )
				{
					clear_client(idx,server_ctrl);
					return SOCKET_SEND_ERROR;
				}				
			}			
			
		}
 	}else
 	{
 		play_count = 0;
 	}

	return 1;
 }

 int send_other_info(NETSOCKET * server_ctrl)
 {
 	int idx = 0;
 	for( idx = 0; idx < MAXCLIENT; idx++ )
	{
		if( server_ctrl->info_cmd_to_send[idx] > 0 )
		{
			switch(server_ctrl->info_cmd_to_send[idx])
			{
				case MACHINE_INFO:
					break;
				default:
					break;
			}
		}
 	}
 	return 1;
 }

 int send_disk_info(int idx,NETSOCKET * server_ctrl)
 {
 	NET_COMMAND cmd;
	int ret;
	int disk_num = 0;
	GST_DISKINFO * disk_info;
	NET_SEND_INFO net_info[8];
	char send_buf[1000];
	int send_len = 0;
	int i;

	cmd.command = OTHER_INFO;
	cmd.length      = MAC_DISK_INFO;

	disk_num = g_pstcommand->GST_DISK_GetDiskNum();

	cmd.page       = disk_num;

	for( i = 0; i < disk_num; i++ )
	{
		disk_info = g_pstcommand->GST_FS_GetDiskInfoPoint(i);
		net_info[i].disk_size =disk_info->size / 1024 /1024;
		net_info[i].disk_remain = g_pstcommand->GST_FS_get_disk_remain_rate(i);
		net_info[i].disk_cover = g_pstcommand->GST_FS_get_cover_rate(i);
		net_info[i].disk_bad = g_pstcommand->GST_FS_GetBadBlockSize(i);
		g_pstcommand->GST_FTC_get_disk_rec_start_end_time(&net_info[i].start_rec_time,
			&net_info[i].end_rec_time,i);
	}
	

//	alarm(SEND_DELAY_TIME);

	server_ctrl->cur_client_idx = idx;

	memcpy(send_buf+send_len,(char*)&cmd, sizeof(NET_COMMAND));
	send_len += sizeof(NET_COMMAND);

	for( i = 0; i < disk_num; i++ )
	{
		printf(" disk_size = %d\n",net_info[i].disk_size);

		memcpy(send_buf+send_len,(char*)&net_info[i], sizeof(NET_SEND_INFO));
		send_len += sizeof(NET_SEND_INFO);	
	}


	ret = safe_socket_send_data(server_ctrl,idx, send_buf, send_len);
	if ( ret <= 0 )
	{
		clear_client(idx,server_ctrl);
		return SOCKET_SEND_ERROR;
	}

	server_ctrl->info_cmd_to_send[idx] = 0;

//	alarm(0);

 	return 1;
 }

  int send_led_info(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;
//	int disk_num = 0;		
//	int i;

	cmd.command = OTHER_INFO;
	cmd.length      = LED_INFO;
	cmd.page        = get_keyboard_led_info()->led1;
	cmd.play_fileId= get_keyboard_led_info()->led2;

//	alarm(SEND_DELAY_TIME);

	server_ctrl->cur_client_idx = idx;

	ret = safe_socket_send_data(server_ctrl,idx, (char*)&cmd, sizeof(NET_COMMAND));
	if ( ret <= 0 )
	{
		clear_client(idx,server_ctrl);
		return SOCKET_SEND_ERROR;
	}
	
	server_ctrl->info_cmd_to_send[idx] = 0;

//	alarm(0);
	
 	return 1;
 }

 int send_file_list(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;
	char  send_buf[1000000];
	int send_len = 0;

	cmd = g_cmd;
	

//	alarm(SEND_DELAY_TIME);

	server_ctrl->cur_client_idx = idx;

	memcpy(send_buf+send_len,(char*)&cmd, sizeof(NET_COMMAND));
	send_len += sizeof(NET_COMMAND);			

	if( cmd.page  >0 )
	{		
		memcpy(send_buf+send_len, (char *)(server_ctrl->rec_list_info), cmd.play_fileId);
		send_len += cmd.play_fileId;		
	}

	ret = safe_socket_send_data(server_ctrl,idx, send_buf, send_len);
	if ( ret <= 0 )
	{		
		DPRINTK( "socket_send_data() send 2 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}

	server_ctrl->info_cmd_to_send[idx] = 0;

//	alarm(0);
	
 	return 1;
 }

 int send_play_file_result(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;
//	int disk_num = 0;		
//	int i;

	cmd = g_cmd;
	

//	alarm(SEND_DELAY_TIME);

	server_ctrl->cur_client_idx = idx;
	printf(" den 2  %d %d %d %d\n",cmd.command,cmd.length,cmd.page,cmd.play_fileId);
	ret = safe_socket_send_data(server_ctrl,idx,(char*)&cmd, sizeof(NET_COMMAND));
	if ( ret <= 0 )
	{			
		DPRINTK( "socket_send_data() send 3 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}	

	if( 	cmd.page == NET_FILE_OPEN_SUCCESS )
	{
		server_ctrl->net_play_mode = NET_FILE_PLAYING;
		server_ctrl->client_send_data[idx] = 1;
	}		

	server_ctrl->info_cmd_to_send[idx] = 0;

//	alarm(0);
	
 	return 1;
 }


 int send_dvr_mac_para(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;
	char send_buf[1000000];
	int send_len = 0;

	cmd = g_cmd;

	server_ctrl->cur_client_idx = idx;

	memcpy(send_buf + send_len,(char*)&cmd, sizeof(NET_COMMAND));
	send_len += sizeof(NET_COMMAND);

	memcpy(send_buf + send_len,(char*)&dvr_mac_para, cmd.play_fileId);
	send_len += cmd.play_fileId;	
	
	ret = safe_socket_send_data(server_ctrl,idx,send_buf,send_len);
	if ( ret <= 0 )
	{			
		DPRINTK( "socket_send_data() send 3 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}	

	server_ctrl->info_cmd_to_send[idx] = 0;


	
 	return 1;
 }

 int send_gui_msg(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;
	char send_buf[1000000];
	int send_len = 0;
	
	cmd = g_cmd;

	server_ctrl->cur_client_idx = idx;

	memcpy(send_buf + send_len,(char*)&cmd, sizeof(NET_COMMAND));
	send_len += sizeof(NET_COMMAND);

	memcpy(send_buf + send_len,(char*)gui_msg,cmd.play_fileId);
	send_len += cmd.play_fileId;	
	
	ret = safe_socket_send_data(server_ctrl,idx,send_buf,send_len);
	if ( ret <= 0 )
	{			
		DPRINTK( "socket_send_data() send 3 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}

	server_ctrl->info_cmd_to_send[idx] = 0;
	
 	return 1;
 }

  int send_max_video_size(int idx,NETSOCKET * server_ctrl)
 {
  	NET_COMMAND cmd;
	int ret;

	
	cmd.command = OTHER_INFO;
	cmd.length = SEND_MAX_VIDEO_SIZE;
	cmd.page= Net_cam_get_support_max_video();
	DPRINTK("Net_dvr_get_support_max_video = %d\n",cmd.page);
		
	server_ctrl->cur_client_idx = idx;
	ret = safe_socket_send_data(server_ctrl,idx,(char*)&cmd, sizeof(NET_COMMAND));
	if ( ret <= 0 )
	{			
		DPRINTK( "socket_send_data() send 3 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}	

	server_ctrl->info_cmd_to_send[idx] = 0;


	
 	return 1;
 }


 int send_other_info_data(NETSOCKET * server_ctrl)
{
//	GST_DRV_BUF_INFO * buf_info = NULL;
	int ret;
	int idx = 0;

	for( idx = 0; idx < MAXCLIENT; idx++ )
	{
	  //	DPRINTK("command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);

		if( server_ctrl->info_cmd_to_send[idx] != 0)
			server_ctrl->socket_send_num[idx]++;

	  	switch( server_ctrl->info_cmd_to_send[idx] )
	  	{
	  		case MACHINE_INFO:
				break;			
			case MAC_DISK_INFO:	
				ret = send_disk_info(idx,server_ctrl);
				break;
			case LED_INFO:
				ret = send_led_info(idx,server_ctrl);
				break;
			case DISK_FILE_LIST:				
				ret = send_file_list(idx,server_ctrl);
				break;
			case PLAY_FILE_RESULT:
				ret = send_play_file_result(idx,server_ctrl);
				break;
			case SEND_MAC_PARA:
				ret = send_dvr_mac_para(idx,server_ctrl);
				break;
			case SEND_GUI_MSG:
				ret = send_gui_msg(idx,server_ctrl);
				break;
			case SEND_MAX_VIDEO_SIZE:
				ret = send_max_video_size(idx,server_ctrl);
				break;
			default:
				ret = 0;
			//	printf(" wrong other info!\n");
				break;
	  	}		

		
	}

	return ret;
	

}


 int send_other_info_data_msg(NETSOCKET * server_ctrl)
{

	int ret;
	int idx = 0;
	BUF_FRAMEHEADER pheadr;
	int save_buf[2];	

	if( get_one_buf(&pheadr,save_buf,&g_othreinfolist) > 0 )
	{
		idx = save_buf[0];
		
		switch(  save_buf[1] )
	  	{
	  		case MACHINE_INFO:
				break;			
			case MAC_DISK_INFO:	
				ret = send_disk_info(idx,server_ctrl);
				break;
			case LED_INFO:
				ret = send_led_info(idx,server_ctrl);
				break;
			case DISK_FILE_LIST:				
				ret = send_file_list(idx,server_ctrl);
				break;
			case PLAY_FILE_RESULT:
				ret = send_play_file_result(idx,server_ctrl);
				break;
			case SEND_MAC_PARA:
				ret = send_dvr_mac_para(idx,server_ctrl);
				break;
			case SEND_GUI_MSG:
				ret = send_gui_msg(idx,server_ctrl);
				break;
			case SEND_MAX_VIDEO_SIZE:
				ret = send_max_video_size(idx,server_ctrl);
				break;
			default:
				ret = 0;
			//	printf(" wrong other info!\n");
				break;
	  	}		

	}

	return ret;

}


 int is_time_to_net_send()
 {
	static struct timeval playTime;
	static struct timeval oldTime;
	// 传送两帧数据之间的时间间隔。
	long between_time = 15000;
	long value;
	
	get_sys_time( &playTime, NULL );
	
	value = labs(playTime.tv_usec -oldTime.tv_usec);

	//DPRINTK("value = %ld\nn",value);

	oldTime.tv_usec = playTime.tv_usec; 

	if( value > between_time )
	{
		//DPRINTK("%ld %ld %ld\n",playTime.tv_usec,oldTime.tv_usec,value);
		return 1;
	}
	else
	{
		return 0;
	}
 }

int normal_user_net_send(int chan)
{
	NETSOCKET * server_ctrl;
	int idx;
	int is_send = 0;

	server_ctrl = &socket_ctrl;
	
	if( server_ctrl->super_user_id  != -1 )
	 	return 1;


	for( idx = 0; idx < MAXCLIENT; idx++ )
	{
		if( server_ctrl->client_send_data[idx] == 0 )
			continue;	

		if( get_bit(server_ctrl->client_use_cam[idx],chan)== 1 )
		{
			is_send =1;
		}
		
	}
	
	return is_send;
}

int normal_user_net_data_change_to_send(int idx,NETSOCKET * server_ctrl,FRAMEHEADERINFO * s_frame,FRAMEHEADERINFO * dst_frame,
					char * s_buf, char * dst_buf)
{	
	int is_send = 0;
	int i;	
	FRAMEHEADERINFO * tmp_pf = NULL;
	char * tmp_data_pf = NULL;
	char * data_buf = NULL;
	int new_combine_num = 0;
	int new_buf_length = 0;
	int write_buf_length = 0;

	// If dvr have super user than not send normal user data. 
	if( server_ctrl->super_user_id  != -1 )
	 	return is_send;
	
	if( server_ctrl->client_send_data[idx] == 0 )
		return is_send;	

	if( (s_frame->ulTimeSec == 0)&&( s_frame->ulTimeUsec == 0 ) )
	{
		// 是多帧合并特殊帧

		tmp_pf = (FRAMEHEADERINFO*)s_buf;
		tmp_data_pf = s_buf + sizeof(FRAMEHEADERINFO);
		data_buf = tmp_data_pf;		
		
		for( i = 0 ; i < s_frame->ulFrameNumber;i++)
		{
			
			if( get_bit(server_ctrl->client_use_cam[idx],tmp_pf->iChannel)== 1 )
			{
				memcpy( dst_buf + new_buf_length,tmp_pf,sizeof(FRAMEHEADERINFO));
				new_buf_length += sizeof(FRAMEHEADERINFO);
				memcpy( dst_buf + new_buf_length,data_buf,tmp_pf->ulDataLen);
				new_buf_length += tmp_pf->ulDataLen;

				new_buf_length = (new_buf_length + 3) /4 * 4;
				new_combine_num = new_combine_num + 1;				
			}
		
			write_buf_length += sizeof(FRAMEHEADERINFO);
			write_buf_length += tmp_pf->ulDataLen;
			write_buf_length = (write_buf_length + 3) /4 * 4;

			tmp_pf = (FRAMEHEADERINFO*)(s_buf + write_buf_length);
			data_buf = tmp_data_pf + write_buf_length;
		}

		if( new_combine_num > 0 )
		{
			memcpy(dst_frame,s_frame,sizeof(FRAMEHEADERINFO));

			dst_frame->ulDataLen = new_buf_length;
			dst_frame->ulFrameNumber  = new_combine_num;

			//DPRINTK("2 idx=%d length=%d  cam=%x new_combine_num=%d\n",idx,
			//	new_buf_length,server_ctrl->client_use_cam[idx],new_combine_num);				

			is_send = 1;
		}		
		
	}else
	{
		if( server_ctrl->is_super_user[idx] == 1 )
		{
			*dst_frame = *s_frame;
			memcpy(dst_buf,s_buf,dst_frame->ulDataLen);
			is_send = 1;
		}
	}

	
	return is_send;
}
   	
   	


int send_img_data(NETSOCKET * server_ctrl)
{
	GST_DRV_BUF_INFO * buf_info = NULL;
	static char data[NET_BUF_MAX_SIZE];
	static char data1[NET_BUF_MAX_SIZE];
	FRAMEHEADERINFO stFrameHeaderInfo;
	FRAMEHEADERINFO stFrameHeaderInfo1;
	int index;
	int ret;
	int idx = 0;
	NET_COMMAND cmd;
	char * tmp_data;
	char * tmp_data2;
	int buf_pos = 0;
//	static int frame_count = 0;	


	tmp_data = &data[0];
	tmp_data2 = &data1[0];
	
	
	buf_info = (GST_DRV_BUF_INFO * )get_net_buf_info(tmp_data);	

	if( buf_info )
	{
		
		
		if( VIDEO_FRAME == buf_info->iBufDataType )
		{				
			stFrameHeaderInfo.type       = 'V';					
		}
		else
		{
			stFrameHeaderInfo.type       = 'A';	
		}		

		//DPRINTK(" iChannelMask = %x  \n",buf_info->iChannelMask);	
		
		for ( index = 0; index <get_dvr_max_chan(1); index++ )			
		{
			//if (!( buf_info->iChannelMask & ( 1 << index )))
			if( get_bit(buf_info->iChannelMask,index) )
					break;	
		}		

	
		stFrameHeaderInfo.iChannel = index;			
		stFrameHeaderInfo.ulDataLen = buf_info->iBufLen[index];
		stFrameHeaderInfo.ulFrameType = buf_info->iFrameType[index];
		stFrameHeaderInfo.ulTimeSec = buf_info->tv.tv_sec;
		stFrameHeaderInfo.ulTimeUsec = buf_info->tv.tv_usec;
		stFrameHeaderInfo.videoStandard = get_cur_standard();	
		stFrameHeaderInfo.imageSize	= get_cur_size();
		stFrameHeaderInfo.ulFrameNumber = buf_info->iFrameCountNumber[index];

		if( stFrameHeaderInfo.ulDataLen > NET_BUF_MAX_SIZE )
			DPRINTK("data larger is %ld\n",stFrameHeaderInfo.ulDataLen);
		

		if( g_pstcommand->GST_FTC_get_play_status() == 0  )
		{
			if( Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_D1)
					stFrameHeaderInfo.imageSize	= TD_DRV_VIDEO_SIZE_D1;
			else
					stFrameHeaderInfo.imageSize	= TD_DRV_VIDEO_SIZE_CIF;
		}

		for( idx = 0; idx < MAXCLIENT; idx++ )
		{
		 	//DPRINTK("command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);
			if( server_ctrl->client_send_data[idx] == 0 )
				continue;	
			
			server_ctrl->socket_send_num[idx]++;

			cmd.command = COMMAND_GET_FRAME;	

			//DPRINTK("net 2 %d\n",stFrameHeaderInfo.iChannel);		

			if( (server_ctrl->super_user_id  != -1) || (stFrameHeaderInfo.type == 'A')  )
			{

				server_ctrl->cur_client_idx = idx;	

				memcpy(tmp_data2 + buf_pos,(char*)&cmd, sizeof(NET_COMMAND));
				buf_pos += sizeof(NET_COMMAND);
				memcpy(tmp_data2 + buf_pos,(char*)&stFrameHeaderInfo, sizeof(FRAMEHEADERINFO));
				buf_pos += sizeof(FRAMEHEADERINFO);
				memcpy(tmp_data2 + buf_pos,tmp_data, stFrameHeaderInfo.ulDataLen);
				buf_pos += stFrameHeaderInfo.ulDataLen;
			
				ret = safe_socket_send_data(server_ctrl,idx, tmp_data2, buf_pos);
				if ( ret <= 0 )
				{
					printf("1error %s %d\n", strerror(errno),errno);
					clear_client(idx,server_ctrl);
					return SOCKET_SEND_ERROR;
				}
							

			}else
			{			
				//以上的是发送合并帧的情况，ipcam不需要这种情况。

				server_ctrl->cur_client_idx = idx;						
			
				memcpy(tmp_data2 + buf_pos,(char*)&cmd, sizeof(NET_COMMAND));
				buf_pos += sizeof(NET_COMMAND);
				memcpy(tmp_data2 + buf_pos,(char*)&stFrameHeaderInfo, sizeof(FRAMEHEADERINFO));
				buf_pos += sizeof(FRAMEHEADERINFO);
				memcpy(tmp_data2 + buf_pos,tmp_data, stFrameHeaderInfo.ulDataLen);
				buf_pos += stFrameHeaderInfo.ulDataLen;
			
				ret = safe_socket_send_data(server_ctrl,idx, tmp_data2, buf_pos);
				if ( ret <= 0 )
				{
					printf("1error %s %d\n", strerror(errno),errno);
					clear_client(idx,server_ctrl);
					return SOCKET_SEND_ERROR;
				}				
				
			}			

		
			//DPRINTK("net %d\n",stFrameHeaderInfo.iChannel);				
			 

			//if( stFrameHeaderInfo.iChannel == 0 )
			//	DPRINTK(" channel = %d type=%c frametype=%d imgsize=%d len=%d num=%d sec=%ld usec=%ld\n",stFrameHeaderInfo.iChannel,
			//	stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,stFrameHeaderInfo.imageSize,stFrameHeaderInfo.ulDataLen,
			//	buf_info->iFrameCountNumber[index],stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec);

			//printf("net %d 111 %x %x %x %x  chan=%d\n",stFrameHeaderInfo.iChannel,data[1000],data[1001],data[1002],data[1003],stFrameHeaderInfo.iChannel);
			
		}

		return 1;
	}
	
	return 0;
}

int clear_client(int idx, NETSOCKET * server_ctrl)
{
	printf(" super_user_id = %d  idx=%d\n",server_ctrl->super_user_id, idx);

	if( server_ctrl->clientfd[idx] == -1 )
		return ALLRIGHT;


	if( server_ctrl->net_play_mode != 0 )
	{
		if( server_ctrl->net_play_mode == GET_LOCAL_VIEW  )
		{			
			if( server_ctrl->super_user_id  == -1 ) // 普通用户
			{
				if( server_ctrl->client_num <= 1 )
				{
					start_stop_net_view(0,0xffff);
					server_ctrl->net_play_mode = 0; 
				}
			}else
			{
				if( server_ctrl->super_user_id  == idx )
				{
					//超级用户
					start_stop_net_view(0,0xffff);			
					server_ctrl->net_play_mode = 0; 
				}else
				{
					//有超级用户时，断开普通用户?
				}
				
			}
		}

		//if( server_ctrl->net_play_mode == NET_FILE_PLAYING  )
	
		// 这有问题
		
	}	

	if( (idx == server_ctrl->super_user_id) || (server_ctrl->client_num == 1) )
	{	
		if( g_pstcommand->GST_FTC_get_play_status() == 1 )
			g_pstcommand->GST_FTC_StopPlay();			
		start_stop_net_file_view(0);
		server_ctrl->net_play_mode = 0;	
	
		open_net_audio(0);
		g_pstcommand->GST_FTC_AudioPlayCam(0);

		clear_buf(&g_othreinfolist);

	}

	if( server_ctrl->clientfd[idx] > 0 )
	{
		int wait_sec = 0;
		
		DPRINTK("close socket %d idx=%d\n",server_ctrl->clientfd[idx],idx);

		server_ctrl->close_socket_flag[idx] = 1;

		while(server_ctrl->send_flag[idx] == 1 )
		{
			DPRINTK("socket send data now,wait \n");
			sleep(1);
			wait_sec++;
			if( wait_sec > 30 )
			{
				DPRINTK("time out break\n");
				break;
			}
		}

		DPRINTK("send flag:%d %d socket=%d\n",
			server_ctrl->close_socket_flag[idx],server_ctrl->send_flag[idx],
			server_ctrl->clientfd[idx]);
		
		if(  server_ctrl->clientfd[idx] <= 0 )
		{
			DPRINTK("socket is %d ,is close already,so return\n",
				server_ctrl->clientfd[idx] );
			return ALLRIGHT;
		}
		
		//shutdown( server_ctrl->clientfd[idx], SHUT_RDWR );
		if(  server_ctrl->clientfd[idx] >  0 )
		{
			DPRINTK("here 1\n");
			FD_CLR( server_ctrl->clientfd[idx], &server_ctrl->watch_set );	
			DPRINTK("here 2\n");
		}
		if( server_ctrl->clientfd[idx] > 0 )
		{
			close( server_ctrl->clientfd[idx] );
			server_ctrl->clientfd[idx] = -1;
		}else
		{
			DPRINTK("socket = %d, have an error,not close\n");
		}
		DPRINTK("close socket %d idx=%d over\n",server_ctrl->clientfd[idx],idx);		
		server_ctrl->client_num--;	

		if( server_ctrl->super_user_id == idx )
			server_ctrl->super_user_id = -1;	

		server_ctrl->close_socket_flag[idx] = 0;
	}	

	
	server_ctrl->clientfd[idx] = -1;
	server_ctrl->client_flag[idx]  = 0;	
	server_ctrl->client_send_data[idx] = 0;
	server_ctrl->info_cmd_to_send[idx] = 0;
	server_ctrl->send_flag[idx] = 0;
	server_ctrl->close_socket_flag[idx]= 0 ;
	server_ctrl->have_check_passwd[idx] = 0;
	server_ctrl->is_super_user[idx] = 0;
	server_ctrl->maxfd = get_max_fd( server_ctrl );

	if(server_ctrl->client_num <= 0 )
	{
		server_ctrl->client_num = 0;
		server_ctrl->super_user_id = -1;
		g_net_mode = TD_DRV_VIDEO_SIZE_CIF;
		DPRINTK(" set g_net_mode = TD_DRV_VIDEO_SIZE_CIF\n");
	}

	printf(" idx=%d client error! client_num=%d  super_user_id=%d\n",idx,server_ctrl->client_num,
		server_ctrl->super_user_id );



	return ALLRIGHT;
}


int op_other_info(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	int ret;
	NET_COMMAND * pcmd;
	pcmd = (NET_COMMAND*)cmd;

	DPRINTK("info %d\n",pcmd->length);
	ret = 1;

	switch( pcmd->length )
	{	
		case MACHINE_INFO:
			server_ctrl->info_cmd_to_send[idx] = MACHINE_INFO;			
			break;
		case MAC_DISK_INFO:
			server_ctrl->info_cmd_to_send[idx] = MAC_DISK_INFO;
			//ret = send_disk_info(idx,server_ctrl);
			break;
		case LED_INFO:
			server_ctrl->info_cmd_to_send[idx] = LED_INFO;
			//ret = send_led_info(idx,server_ctrl);
			break;
		case DISK_FILE_LIST:
			server_ctrl->info_cmd_to_send[idx] = DISK_FILE_LIST;
			break;
		case PLAY_FILE_RESULT:		
			server_ctrl->info_cmd_to_send[idx] = PLAY_FILE_RESULT;
			break;
		case SEND_MAC_PARA:
			server_ctrl->info_cmd_to_send[idx] = SEND_MAC_PARA;
			break;
		case SEND_GUI_MSG:
			server_ctrl->info_cmd_to_send[idx] = SEND_GUI_MSG;
			break;		
		case SEND_MAX_VIDEO_SIZE:
			server_ctrl->info_cmd_to_send[idx] = SEND_MAX_VIDEO_SIZE;
			break;
		default:
			break;
	}
	
	return ret;
}


int op_other_info_msg(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	int ret;
	int available_cmd = 1;
	NET_COMMAND * pcmd;
	pcmd = (NET_COMMAND*)cmd;

	//printf("info %d\n",pcmd->length);
	ret = 1;

	switch( pcmd->length )
	{	
		case MACHINE_INFO:
			server_ctrl->info_cmd_to_send[idx] = MACHINE_INFO;			
			break;
		case MAC_DISK_INFO:
			server_ctrl->info_cmd_to_send[idx] = MAC_DISK_INFO;
			//ret = send_disk_info(idx,server_ctrl);
			break;
		case LED_INFO:
			server_ctrl->info_cmd_to_send[idx] = LED_INFO;
			//ret = send_led_info(idx,server_ctrl);
			break;
		case DISK_FILE_LIST:
			server_ctrl->info_cmd_to_send[idx] = DISK_FILE_LIST;
			break;
		case PLAY_FILE_RESULT:		
			server_ctrl->info_cmd_to_send[idx] = PLAY_FILE_RESULT;
			break;
		case SEND_MAC_PARA:
			server_ctrl->info_cmd_to_send[idx] = SEND_MAC_PARA;
			break;
		case SEND_GUI_MSG:
			server_ctrl->info_cmd_to_send[idx] = SEND_GUI_MSG;
			break;		
		case SEND_MAX_VIDEO_SIZE:
			server_ctrl->info_cmd_to_send[idx] = SEND_MAX_VIDEO_SIZE;
			break;
		case NET_CAM_SUPPORT_512k_BUFF:
			DPRINTK("SET NET_CAM_SUPPORT_512k_BUFF \n");
			Net_cam_set_support_512K_buf(1);
			break;
		default:
			available_cmd = 0;
			break;
	}

	if( available_cmd == 1 )
	{
		BUF_FRAMEHEADER pheadr;
		int save_buf[2];

		pheadr.iLength = sizeof(int) *2;

		save_buf[0] = idx;
		save_buf[1] =  pcmd->length;		
		
		insert_data(&pheadr,(char*)save_buf,&g_othreinfolist);
	}
	
	return ret;
}

int op_user_info(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	NET_COMMAND * pcmd;
	char tempbuf[30];
	char user[10], pass[10];
	char aes_pass[32];
	int ret;
	int i;
	int j;
	int have_super_user = 0;
	
	pcmd = (NET_COMMAND*)cmd;

	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)tempbuf, pcmd->length) )
	{
		return -1;
	}
	
	strcpy(user,tempbuf);
	strcpy(pass,&tempbuf[10]);

	printf("%s:%s\n",user,pass);
	printf("super_user_id = %d\n",server_ctrl->super_user_id);

	for( i = 0; i < 4; i++ )
	{
		if( server_ctrl->user_name[i][0] == '\0' )
			continue;
		
		printf("user  %s:%s\n",server_ctrl->user_name[i],server_ctrl->user_password[i]);

		{
			char sAESKey[32];
			char tmp_access_passwd[64] = "";
			strcpy(sAESKey, "szyat!@");	
			strcpy(tmp_access_passwd,server_ctrl->user_password[i]);
			memset(aes_pass,0x00,32);
		      	AES_Init();				
			AES_Encrypt(128,(unsigned char *)tmp_access_passwd, strlen(tmp_access_passwd), (unsigned char *)sAESKey, 7, (unsigned char * )aes_pass);			
		      	AES_Deinit();		

			//aes_pass的长度会有16位，但网传的buf只有10，所以aes_pass[9] = 0;
			aes_pass[9] = 0;				

			printf("pass=%s    aes_pass=%s    cmp=%d\n",pass,aes_pass,strcmp(pass,aes_pass));
		}

		printf("name cmd=%d cmp=%d\n",strcmp(user,server_ctrl->user_name[i]),
			strcmp(pass,server_ctrl->user_password[i]) );
	
		if( (strcmp(user,server_ctrl->user_name[i]) == 0 ) && 
			(strcmp(pass,server_ctrl->user_password[i]) == 0  || strcmp(pass,aes_pass) == 0) )
		{
			if( i== 0 )
			{
				pcmd->length = RIGHT_USER_PASS;
				pcmd->page = SUPER_USER;	


				for(j = 0; j < 4; j++)
				{
					if ( server_ctrl->clientfd[j] > 0 )
					{
						if( server_ctrl->is_super_user[j] == 1 )
							have_super_user = 1;
					}
				}
				
				//if( server_ctrl->super_user_id != -1 )
				if( have_super_user == 1 )
				{
					pcmd->length = BAD_USER_PASS;
					pcmd->page = SUPER_USER;
					printf(" ddd==1\n");
				}else
				{			
					//server_ctrl->super_user_id = idx;
					server_ctrl->have_check_passwd[idx] = 1;
					server_ctrl->is_super_user[idx] = 1;
					printf("super user %d\n",idx);
					printf(" ddd==2");
					//kick_normal_client(server_ctrl);
				}
			}else
			{
				pcmd->length = RIGHT_USER_PASS;
				pcmd->page = NORMAL_USER;	
				server_ctrl->have_check_passwd[idx] = 1;
				
				if( server_ctrl->super_user_id != -1 )
				{
					pcmd->length = BAD_USER_PASS;
					pcmd->page = SUPER_USER;
					server_ctrl->have_check_passwd[idx] = 0;
					printf(" ddd==3");
				}

				if( server_ctrl->client_num >= MAX_CONNECT_NUM -1 )
				{
					pcmd->command = USER_TOO_MUCH;
					server_ctrl->have_check_passwd[idx] = 0;
				}
				printf("normal user %d\n",idx);
			}

			break;		
		}
	}

	if( i >= 4 )
	{
		pcmd->length = BAD_USER_PASS;
		pcmd->page = NORMAL_USER;	
		server_ctrl->have_check_passwd[idx] = 0;
		printf("bad user %d\n",idx);
		printf(" ddd==4");
	}

	//printf("ss=%d\n",pcmd->length);

	#ifdef USE_H264_ENC_DEC
		pcmd->play_fileId = H264_CODE;
	#else
		pcmd->play_fileId = MPEG4_CODE;
	#endif


	pcmd->searchDate = get_dvr_max_chan(1) + MAC_MODEL;	

//	alarm(SEND_DELAY_TIME);
	server_ctrl->cur_client_idx = idx;

	ret = safe_socket_send_data(server_ctrl,idx, (char*)pcmd, sizeof(NET_COMMAND));
	if ( ret <= 0 )
	{
		clear_client(idx,server_ctrl);
		return SOCKET_SEND_ERROR;
	}
//	alarm(0);
	
	
	return 1;
}

int get_net_local_play_cam()
{
	if( socket_ctrl.client_send_data[0] )
		return socket_ctrl.play_cam;

	return 0x0f;
}

int get_update_soft(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	char file_name[256];
	NET_COMMAND * pcmd;
	char * file_buf = NULL;
	FILE * fp = NULL;
	int ret;
	pcmd = (NET_COMMAND*)cmd;

	memset(file_name,0x00,256);

	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)file_name, pcmd->length) )
	{		
		return -1;
	}

	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)pcmd, sizeof( NET_COMMAND )) )
	{
		return -1;
	}

	if( pcmd->length > 5*1024*1024 || pcmd->length <= 0 )
	{
		printf("file is too larger %d\n",pcmd->length);
		return -1;
	}

	printf(" %s file length is %d\n",file_name,pcmd->length);

	

	file_buf = (char*)malloc(pcmd->length);

	if( file_buf == NULL )
	{
		printf(" not have enough memory!\n");
		return -1;
	}	

	
	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)file_buf, pcmd->length) )
	{
		goto GETUPDATESOFTERROR;
	}

	fp = fopen(file_name,"wb");

	if( fp == NULL )
	{
		printf("create %s file error!\n",file_name);
		goto GETUPDATESOFTERROR;
	}	

	ret = fwrite(file_buf,1,pcmd->length,fp);

	if( ret != pcmd->length )
	{
		printf("write %s file error!\n",file_name);
		goto GETUPDATESOFTERROR;
	}

	if( fp )
		fclose(fp);

	if( file_buf )
		free(file_buf);
	
	return 1;

GETUPDATESOFTERROR:
	if( fp )
		fclose(fp);

	if( file_buf )
		free(file_buf);
	
	return -1;
}


int execute_shell_file(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	char file_name[256];
	NET_COMMAND * pcmd;
	int ret;
	pcmd = (NET_COMMAND*)cmd;

	memset(file_name,0x00,256);

	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)file_name, pcmd->length) )
	{
		return -1;
	}

	printf("execute shell file %s\n",file_name);

	ret = command_drv(file_name);

	printf("ret = %d\n",ret);
	
	return 1;
}

int net_play_file(char * scmd,int idx, NETSOCKET * server_ctrl)
{
	time_t startTime;
	time_t endTime;
	int cam,sound,time_delay;
	int play_mode = RECORDPLAY;
	int ret;
	NET_COMMAND cmd;
	NET_COMMAND * pcmd;
	pcmd = (NET_COMMAND*)scmd;

	startTime = g_search_time + pcmd->searchDate;
	if( pcmd->length == 0 )
		endTime = 0;
	else
		endTime = g_search_time + pcmd->length;
	cam = pcmd->play_fileId;
	sound = pcmd->page / 10;
	time_delay = pcmd->page % 10;

	if( time_delay ==  1 )
		play_mode = RECORDPLAY + 10;
	

	if( g_pstcommand->GST_FTC_get_play_status() == 1 && get_net_file_view_flag() == 0)
	{
		cmd.command = NET_FILE_OPEN_ERROR;
		cmd.length     = 0x11;
	}
	else
	{
		{
			if( g_pstcommand->GST_FTC_get_play_status() == 1 )
			{
				//g_pstcommand->GST_FTC_AudioPlayCam(0);
				//g_pstcommand->GST_FTC_StopPlay();

				g_net_file_play_change_play_pos = 1;
			}
		}
	
		start_stop_net_file_view(1);
		ret = g_pstcommand->GST_FTC_SetTimeToPlay(startTime,endTime,cam,play_mode);
		g_net_file_play_change_play_pos = 0;
		if( ret < 0 )
		{
			start_stop_net_file_view(0);
			cmd.command = NET_FILE_OPEN_ERROR;
			cmd.length     = 0x00;
		}
		else
		{		
			
			cmd.command = NET_FILE_OPEN_SUCCESS;

			//g_pstcommand->GST_FTC_AudioPlayCam(0x01);
			
			if( sound == 1 )  
			{
				int sound_cam = 0,num=0;

				for(num=0;num< 16;num++)
				{
					if( get_bit(cam,num)==1 )
					{
						sound_cam = set_bit(sound_cam,num);
						g_pstcommand->GST_FTC_AudioPlayCam(sound_cam);
						break;
					}
				}
				
				
			}else
			{
				g_pstcommand->GST_FTC_AudioPlayCam(0);
			}
		}
	}

	g_cmd.command = OTHER_INFO;
	g_cmd.length = PLAY_FILE_RESULT;
	g_cmd.page = cmd.command;
	g_cmd.play_fileId = cmd.length;

	cmd = g_cmd;
	
	ret = op_other_info((char*)&cmd,idx, server_ctrl);
	if ( ret <= 0 )
	{			
		DPRINTK( "socket_send_data() send 3 cmd error \n" );
		return SOCKET_SEND_ERROR;
	}
			
	
	return 1;
}

int net_set_dvr_mac_para(char * cmd,int idx, NETSOCKET * server_ctrl)
{
//	char file_name[256];
	NET_COMMAND * pcmd;
//	int ret;
	pcmd = (NET_COMMAND*)cmd;


	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)&dvr_mac_para, pcmd->length) )
	{
		return -1;
	}

	g_pstcommand->GST_MENU_cam_parameter_set(&dvr_mac_para.cam_set);
	g_pstcommand->GST_MENU_ptz_parameter_set(&dvr_mac_para.ptz_set);
	g_pstcommand->GST_MENU_rec_parameter_set(&dvr_mac_para.rec_set);
	
	return 1;
}

int net_press_dvr_key(char * cmd,int idx, NETSOCKET * server_ctrl)
{
//	char file_name[256];
	NET_COMMAND * pcmd;
//	int ret;
	pcmd = (NET_COMMAND*)cmd;

	switch(pcmd->length)
	{
		case PRESS_REC_KEY:
			g_pstcommand->GST_MENU_net_key(NET_KEY_VAL_REC);
			break;
		case PRESS_ESC_KEY:
			g_pstcommand->GST_MENU_net_key(NET_KEY_VAL_ESC);
			break;
	}	
	
	return 1;
}

int net_set_gui_msg(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	//char msg_buf[GUI_MSG_MAX_NUM];
	NET_COMMAND * pcmd;
	pcmd = (NET_COMMAND*)cmd;


	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)gui_msg, pcmd->length) )
	{
		return -1;
	}

	//DPRINTK("msg:%s\n",msg_buf);

	if( g_pstcommand->GST_MENU_gui_msg_set != 0 )
		g_pstcommand->GST_MENU_gui_msg_set(gui_msg);	
	else
	{
		DPRINTK("GST_MENU_gui_msg_set is empty\n");
	}
	
	return 1;
}

int net_send_audio_data(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	char audio_buf[2048];
	NET_COMMAND * pcmd;
	int i;
//	int ret;
	pcmd = (NET_COMMAND*)cmd;

	if( pcmd->length >=  2048 )
	{
		DPRINTK("length=%d larger than 1024.\n",pcmd->length);
		return -1;
	}

	if( 0 > socket_read_data( server_ctrl->clientfd [idx], (char*)audio_buf, pcmd->length) )
	{
		return -1;
	}
	

	if( machine_use_audio() == 0 )
	{
		
		if( pcmd->length > 168 )
		{
			for( i = 0; i < pcmd->length;i += 168)
			{
				Hisi_write_audio_buf((unsigned char *)(audio_buf+i),168,0);			
			}
		}else		
			Hisi_write_audio_buf((unsigned char *)audio_buf,pcmd->length,0);
	}else
	{
		DPRINTK("machine_use_audio == 1\n");
	}
	
	return 1;
}


static int net_listen_audio_chan = 0;
static int net_speak_audio_chan = 0;


int net_dvr_get_net_listen_chan()
{
	return net_listen_audio_chan;
}

int net_dvr_get_net_speak_chan()
{
	return net_speak_audio_chan;
}

int net_audio_set_fuc(char * cmd,int idx, NETSOCKET * server_ctrl)
{
	//char msg_buf[GUI_MSG_MAX_NUM];
	NET_COMMAND * pcmd;	
	pcmd = (NET_COMMAND*)cmd;

	if( pcmd->page == 1	)
	{
		open_net_audio(1);
		
		net_listen_audio_chan = pcmd->length;
	}

	if( pcmd->page == 0	)
	{
		net_speak_audio_chan = pcmd->length;
	}	
	

	DPRINTK("(L %x,S %x)\n",net_listen_audio_chan,net_speak_audio_chan);
	
	return 1;
}

int set_audio_socket_mode(int idx, NETSOCKET * server_ctrl)
{
	int fdsock = 0;

	DPRINTK("idx=%d fd=%d\n",idx,server_ctrl->clientfd[idx]);

	fdsock = server_ctrl->clientfd[idx];
	if( server_ctrl->clientfd[idx] > 0 )
	{		
		FD_CLR( server_ctrl->clientfd[idx], &server_ctrl->watch_set );			
		server_ctrl->client_num--;			

		if( server_ctrl->super_user_id == idx )
			server_ctrl->super_user_id = -1;		
	}	
	
	server_ctrl->clientfd[idx] = -1;
	server_ctrl->client_flag[idx]  = 0;	
	server_ctrl->client_send_data[idx] = 0;
	server_ctrl->info_cmd_to_send[idx] = 0;
	server_ctrl->send_flag[idx] = 0;
	server_ctrl->close_socket_flag[idx]= 0 ;
	server_ctrl->maxfd = get_max_fd( server_ctrl );

	audio_socket_inarray(fdsock);	

	//DPRINTK("start33\n");
	//usleep(300000);

	return 1;
}

int process_command(int idx, NETSOCKET * server_ctrl)
{
	NET_COMMAND cmd;
	int fd;
	int ret;
	int count = 0;
	int send_msg_length;

	fd = server_ctrl->clientfd[idx];

	memset( &cmd, 0, sizeof( NET_COMMAND ) );
	
	if ( (fd == -1) || 0 > socket_read_data( fd, (char*)&cmd, sizeof( NET_COMMAND )) )
	{
		DPRINTK( "process_command() read cmd error \n" );
		return SOCKET_READ_ERROR;
	}

	server_ctrl->socket_recv_num[idx]++;

	switch(cmd.command)
	{	
		//NVR协议中的NET_CAM_IS_SUPPORT_YUN_SERVER 与摄像枪中 NET_CAM_SET_WIFI_SSID_PASS 值冲突了。
		// 所以在摄像枪NVR 协议中NET_CAM_SET_WIFI_SSID_PASS 等同于 NET_CAM_IS_SUPPORT_YUN_SERVER
		// case NET_CAM_IS_SUPPORT_YUN_SERVER:
		case NET_CAM_IS_SUPPORT_YUN_SERVER:
			DPRINTK("NET_CAM_IS_SUPPORT_YUN_SERVER command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);
			cmd.page = 1;
			cmd.play_fileId = ipcam_get_hex_id();
			ret = safe_socket_send_data(server_ctrl,idx,(char*)&cmd, sizeof(NET_COMMAND));
			if ( ret <= 0 )
			{			
				DPRINTK( "socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}	
			break;
		case COMMAND_GET_FRAME:		
			break;
		case GET_LOCAL_VIEW:
			DPRINTK("GET_LOCAL_VIEW command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);

			if( server_ctrl->have_check_passwd[idx] == 0 )
			{
				DPRINTK("user not set right passwd\n");
				return SOCKET_SEND_ERROR;
			}
			
			server_ctrl->net_play_mode = GET_LOCAL_VIEW;
			server_ctrl->client_send_data[idx] = 1;

			if( server_ctrl->super_user_id  == idx )
			{
				server_ctrl->play_cam = cmd.play_fileId;	
				server_ctrl->client_use_cam[idx] = cmd.play_fileId;
				start_stop_net_view(1,cmd.play_fileId);
				open_net_audio(cmd.page);
			}else
			{
				server_ctrl->play_cam = 0xffffffff;	
				server_ctrl->client_use_cam[idx] = cmd.play_fileId;
				DPRINTK("normal user cam = %x\n",server_ctrl->client_use_cam[idx]);
				start_stop_net_view(1,0xffffffff);
				//open_net_audio(cmd.page);
			}			
			
			break;
		case STOP_LOCAL_VIEW:
			DPRINTK("STOP_LOCAL_VIEW command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);
			server_ctrl->net_play_mode = 0;
			server_ctrl->client_send_data[idx] = 0;
			if( server_ctrl->client_num <=  1 )
			{
				start_stop_net_view(0,0xffff);
				open_net_audio(0);
			}
			break;
		case STOP_NET_CONNECT:
			DPRINTK("%d client disconnect\n",idx);			
			clear_client(idx,server_ctrl);
			open_net_audio(0);
			break;
		case SET_NET_PARAMETER:
			DPRINTK("SET_NET_PARAMETER command client_send_data[%d]=%d\n",idx,server_ctrl->client_send_data[idx]);
			server_ctrl->bit_stream = cmd.length;
			server_ctrl->frame_rate = cmd.page;
			server_ctrl->net_trans_mode = cmd.play_fileId;

			//通通使用网络引擎
			server_ctrl->net_trans_mode = INTERNATE_MODE;
			
			if( server_ctrl->net_trans_mode == INTERNATE_MODE )
			{
				ret = change_net_frame_rate(server_ctrl->bit_stream ,server_ctrl->frame_rate);
				if( ret < 0 )
				{		
					DPRINTK( "change_net_frame_rate()  error \n" );
					return SOCKET_SEND_ERROR;
				}	
			}
			
			break;
		case COMMAND_GET_FILELIST:
			DPRINTK("COMMAND_GET_FILELIST %d\n",idx);

			if( server_ctrl->have_check_passwd[idx] == 0 )
			{
				DPRINTK("user not set right passwd\n");
				return SOCKET_SEND_ERROR;
			}
			
			cmd.searchDate = g_pstcommand->GST_FTC_CustomTimeTotime_t(cmd.searchDate / 10000,
				(cmd.searchDate % 10000) / 100, cmd.searchDate % 100, 0 , 0 , 0 );
			
			g_search_time = cmd.searchDate;			
			ret = g_pstcommand->GST_FTC_GetRecordlist(server_ctrl->rec_list_info, 0x0f,
						cmd.searchDate, cmd.searchDate + 3600 * 24, cmd.page);
			if( ret < 0 )
				cmd.page = 0;			
			else			
				cmd.page = ret;
		
			printf("time %ld page = %d\n",g_search_time,cmd.page );
			
			cmd.play_fileId = SEACHER_LIST_MAX_NUM * sizeof(GST_FILESHOWINFO);			

			cmd.length = DISK_FILE_LIST;

			cmd.command = OTHER_INFO;

			g_cmd = cmd;
			
			ret = op_other_info((char*)&cmd,idx, server_ctrl);
			if ( ret <= 0 )
			{			
				DPRINTK( "socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}					
		
			break;
		case COMMAND_OPEN_NET_PLAY_FILE:
			DPRINTK("COMMAND_OPEN_NET_PLAY_FILE %d\n",idx);				
			if ( 0 > socket_read_data( fd, (char*)&server_ctrl->play_rec_record_info, cmd.length) )
			{
				DPRINTK( "process_command() read 2 cmd error \n" );
				return SOCKET_READ_ERROR;
			}

			// 说明有现场用户控制机子回放，不允许网络回放.
			if( g_pstcommand->GST_FTC_get_play_status() == 1 )
			{
				cmd.command = NET_FILE_OPEN_ERROR;
				cmd.length     = 0x11;
			}
			else
			{
							
				ret = g_pstcommand->GST_FTC_ReclistPlay(&server_ctrl->play_rec_record_info);
				if( ret < 0 )
				{
					cmd.command = NET_FILE_OPEN_ERROR;
					cmd.length     = 0x00;
				}
				else
				{
					cmd.command = NET_FILE_OPEN_SUCCESS;

					//g_pstcommand->GST_FTC_AudioPlayCam(0x01);

					//是传送文件, 不进行回放延时操作
					// 文件系统利用GST_FTC_ReclistPlay 来判断是不是网络文件传送
					// 此处不做处理了
					if( cmd.searchDate == 0xff )  
					{
						g_pstcommand->GST_FTC_AudioPlayCam(0x01);
					}
				}
			}
			
			ret = safe_socket_send_data(server_ctrl,idx,(char*)&cmd, sizeof(NET_COMMAND));
			if ( ret <= 0 )
			{			
				DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}	

			if( cmd.command == NET_FILE_OPEN_SUCCESS )
			{
				server_ctrl->net_play_mode = NET_FILE_PLAYING;
				server_ctrl->client_send_data[idx] = 1;

				start_stop_net_file_view(1);
			}			

			
			break;
		case COMMAND_STOP_NET_PLAY_FILE:
			DPRINTK("COMMAND_STOP_NET_PLAY_FILE %d\n",idx);			

			for( count = 0; count < 30; count++)
			{
				if( g_pstcommand->GST_FTC_get_play_status() == 0 )
				{
					printf("checked no file play sleep 200ms, to make sure file play\n");
					usleep(200000); // 200ms
				}else
					break;
			}
			
			
			if( g_pstcommand->GST_FTC_get_play_status() == 1 )
			{
				g_pstcommand->GST_FTC_AudioPlayCam(0);
				g_pstcommand->GST_FTC_StopPlay();
			}

			server_ctrl->net_play_mode = 0;
			server_ctrl->client_send_data[idx] = 0;
			
			start_stop_net_file_view(0);
			break;
		case NET_KEYBOARD:
			//DPRINTK("COMMAND_KEY_BOARD %d\n",idx);	
			ret = op_keyboard_led_info((char*)&cmd);
			if ( ret <= 0 )
			{			
				DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}
			break;
		case OTHER_INFO:
		//	DPRINTK("COMMAND_OTHER_INFO %d\n",idx);
			DPRINTK("OTHER_INFO %d\n",idx);	
			ret = op_other_info((char*)&cmd,idx, server_ctrl);
			if ( ret <= 0 )
			{			
				DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}	
			break;
		case SEND_USER_NAME_PASS:
			DPRINTK("SEND_USER_NAME_PASS %d\n",idx);
			ret = op_user_info((char*)&cmd,idx, server_ctrl);
			if ( ret <= 0 )
			{			
				DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}	
			break;
		case CREATE_NET_KEY_FRAME:
			DPRINTK("create key frame %d\n",idx);
			instant_keyframe_net(0xffff);			
			break;
		case GET_SYSTEM_FILE_FROM_PC:
			DPRINTK("GET_SYSTEM_FILE_FROM_PC %d\n",idx);
			ret = get_update_soft((char*)&cmd,idx, server_ctrl);
			if ( ret <= 0 )
			{			
				DPRINTK( "get_update_soft() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}					
			break;
		case EXECUTE_SHELL_FILE:
			DPRINTK("EXECUTE_SHELL_FILE %d\n",idx);
			ret = execute_shell_file((char*)&cmd,idx, server_ctrl);
			if ( ret < 0 )
			{			
				DPRINTK( "execute_shell_file() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}				
			break;
		case TIME_FILE_PLAY:
			DPRINTK("TIME_FILE_PLAY %d\n",idx);
			ret = net_play_file((char*)&cmd,idx, server_ctrl);
			if ( ret < 0 )
			{			
				DPRINTK( "net_play_file() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}
			break;
		case COMMAND_SEND_MAC_PARA:
			DPRINTK("COMMAND_SEND_MAC_PARA %d\n",idx);

			memset(&dvr_mac_para,0x00,sizeof(dvr_mac_para));
			
			g_pstcommand->GST_MENU_cam_parameter_get(&dvr_mac_para.cam_set);			
			g_pstcommand->GST_MENU_ptz_parameter_get(&dvr_mac_para.ptz_set);			
			g_pstcommand->GST_MENU_rec_parameter_get(&dvr_mac_para.rec_set);
			

			cmd.play_fileId = sizeof(dvr_mac_para);			

			cmd.length = SEND_MAC_PARA;

			cmd.command = OTHER_INFO;

			g_cmd = cmd;
			
			ret = op_other_info((char*)&cmd,idx, server_ctrl);
			if ( ret <= 0 )
			{			
				DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}		
				
			break;
		case COMMAND_SET_MAC_PARA:
			DPRINTK("COMMAND_SET_MAC_PARA %d\n",idx);
			ret = net_set_dvr_mac_para((char*)&cmd,idx, server_ctrl);
			if ( ret < 0 )
			{			
				DPRINTK( "net_play_file() send 3 cmd error \n" );
				return SOCKET_SEND_ERROR;
			}			
			break;
		case COMMAND_SET_MAC_KEY:
			DPRINTK("COMMAND_SET_MAC_KEY %d\n",idx);
			net_press_dvr_key((char*)&cmd,idx, server_ctrl);
			break;
		case COMMAND_SEND_AUDIO_DATA:
		//	DPRINTK("COMMAND_SEND_AUDIO_DATA %d\n",idx);
			net_send_audio_data((char*)&cmd,idx, server_ctrl);
			break;
		case COMMAND_SEND_GUI_MSG:
			DPRINTK("COMMAND_SEND_GUI_MSG add %d\n",idx);			

			cmd.play_fileId = GUI_MSG_MAX_NUM;			

			cmd.length = SEND_GUI_MSG;

			cmd.command = OTHER_INFO;		

			if( g_pstcommand->GST_MENU_gui_msg_get != 0 )
			{
				g_pstcommand->GST_MENU_gui_msg_get(gui_msg,&send_msg_length);			
			
				if( (send_msg_length > 0) &&
					(send_msg_length <= GUI_MSG_MAX_NUM))
				{

					cmd.play_fileId = send_msg_length;
					g_cmd = cmd;				
				
					ret = op_other_info((char*)&cmd,idx, server_ctrl);
					if ( ret <= 0 )
					{			
						DPRINTK( "safe_socket_send_data() send 3 cmd error \n" );
						return SOCKET_SEND_ERROR;
					}		
				}
				else
				{
					DPRINTK("send msg length=%d error!\n",send_msg_length);
					return SOCKET_SEND_ERROR;
				}		
			}else
			{
				DPRINTK("GST_MENU_gui_msg_get  is empty func\n");
			}

			break;
		case COMMAND_SET_GUI_MSG:
			//DPRINTK("COMMAND_SET_GUI_MSG %d\n",idx);			
			net_set_gui_msg((char*)&cmd,idx, server_ctrl);			
			break;
		case COMMAND_SET_AUDIO_SOCKET:
			set_audio_socket_mode(idx, server_ctrl);
			break;
		case COMMAND_AUDIO_SET:
			net_audio_set_fuc((char*)&cmd,idx, server_ctrl);
			break;
		case NET_CAM_NET_MODE:
			DPRINTK("NET_CAM_NET_MODE %d\n", cmd.length);
			if( cmd.length == TD_DRV_VIDEO_SIZE_D1 )
			{
				g_net_mode = TD_DRV_VIDEO_SIZE_D1;
			}else
			{
				g_net_mode = TD_DRV_VIDEO_SIZE_CIF;
			}
			break;	
		default:
			break;			
	}
	
	return 1;
}

int start_mini_httpd(int port)
{
	/* if minihttp alive than kill it */
	char cmd[200];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
//	int i,j;
	int pid;

	sprintf(cmd,"ps | grep mini_httpd.conf|grep -v grep > /bin/2.txt");

	printf("start cmd : %s  port:%d\n",cmd,port);

	rel = command_drv(cmd);
	/*if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return ERROR;
	}*/

	fp = fopen("/bin/2.txt","r");
	if( fp == NULL )
	{
		printf(" open /bin/2.txt error!\n");
		return FILEERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		return ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		return ERROR;
	}

	printf(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( (fileOffset > 5) && (fileOffset <= 200) )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			fclose(fp);
			return ERROR;
		}	

		cmd[5] = '\0';

		pid = atoi(cmd);		

		sprintf(cmd,"kill -9 %d",pid);

		printf("cmd: %s\n",cmd);

		rel = command_drv(cmd);
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			fclose(fp);
			return ERROR;
		}
	}

	fclose(fp);

	
	sprintf(cmd,"/mnt/mtd/mini_httpd -p %d -C /mnt/mtd/mini_httpd.conf&",port);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return ERROR;
	}

	

	socket_ctrl.http_port = port;




	return ALLRIGHT;
}

int set_client_socket(int idx, NETSOCKET * server_ctrl)
{
	struct  linger  linger_set;
//	int nNetTimeout;
	struct timeval stTimeoutVal;
	int keepalive = 1;
	int keepIdle = 30;
	int keepInterval = 3;
	int keepCount = 3;

	
	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;

    if (setsockopt( server_ctrl->clientfd[idx], SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
    {
		printf( "setsockopt SO_LINGER  error\n" );
	}

	
       if (setsockopt( server_ctrl->clientfd[idx], SOL_SOCKET, SO_KEEPALIVE, (char *)&keepalive, sizeof(int) ) < 0 )
       {
		printf( "setsockopt SO_KEEPALIVE error\n" );
	}

	
	if(setsockopt( server_ctrl->clientfd[idx],SOL_TCP,TCP_KEEPIDLE,(void *)&keepIdle,sizeof(keepIdle)) < -1)
	{
		printf( "setsockopt TCP_KEEPIDLE error\n" );
	}

	
	if(setsockopt( server_ctrl->clientfd[idx],SOL_TCP,TCP_KEEPINTVL,(void *)&keepInterval,sizeof(keepInterval)) < -1)
	{
		printf( "setsockopt TCP_KEEPINTVL error\n" );
	}

	
	if(setsockopt( server_ctrl->clientfd[idx],SOL_TCP,TCP_KEEPCNT,(void *)&keepCount,sizeof(keepCount)) < -1)
	{
		printf( "setsockopt TCP_KEEPCNT error\n" );
	}
	

	stTimeoutVal.tv_sec = 30;
	stTimeoutVal.tv_usec = 0;
	if ( setsockopt( server_ctrl->clientfd[idx], SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
       {
		printf( "setsockopt SO_SNDTIMEO error\n" );
	}
		 	
	if ( setsockopt( server_ctrl->clientfd[idx], SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
       {
		printf( "setsockopt SO_RCVTIMEO error\n" );
	}


	return ALLRIGHT;

}

NETSOCKET * net_get_server_ctrl()
{
	return &socket_ctrl;
}

int set_socket_alivekeep(int * set_socket)
{

	//Setting For KeepAlive
	int keepalive = 1;
	int keepalive_time = 30;
	int keepalive_intvl = 3;
	int keepalive_probes= 3;
	int ret;
	
	ret = setsockopt(*set_socket,SOL_SOCKET,SO_KEEPALIVE,(void*)(&keepalive),(socklen_t)sizeof(keepalive));
	if(ret < 0 )
	{
		DPRINTK("SO_KEEPALIVE error\n");
		return ret;
	}

	ret = setsockopt(*set_socket, IPPROTO_TCP, TCP_KEEPIDLE,(void*)(&keepalive_time),(socklen_t)sizeof(keepalive_time));
	if(ret < 0 )
	{
		DPRINTK("TCP_KEEPIDLE error\n");
		return ret;
	}
	
	ret = setsockopt(*set_socket, IPPROTO_TCP, TCP_KEEPINTVL,(void*)(&keepalive_intvl),(socklen_t)sizeof(keepalive_intvl));
	if(ret < 0 )
	{
		DPRINTK("TCP_KEEPINTVL error\n");
		return ret;
	}
	
	ret = setsockopt(*set_socket, IPPROTO_TCP, TCP_KEEPCNT,(void*)(&keepalive_probes),(socklen_t)sizeof(keepalive_probes));
	if(ret < 0 )
	{
		DPRINTK("TCP_KEEPCNT error\n");
		return ret;
	}

	return 1;
}

int net_server_get_client_socket(NETSOCKET * server_ctrl,int client_socket,struct sockaddr_in * dest_addr)
{
	int i = 0;
	NET_COMMAND net_cmd;

	lock_accept_mutex();
	
	for ( i = 0; i < MAXCLIENT; i++ )
	{
		if ( 0 == server_ctrl->client_flag[i] )
			break;
	}

	if ( i >= MAX_CONNECT_NUM )
	{
		memset( &net_cmd, 0, sizeof(NET_COMMAND));
		net_cmd.command =USER_TOO_MUCH;
		socket_send_data(client_socket, (char*)&net_cmd, 
			sizeof(NET_COMMAND));
		usleep(500000);
		close( client_socket );
		DPRINTK("cut %d come\n",client_socket);	
		goto err;
	}

	server_ctrl->clientfd[i] = client_socket;
	server_ctrl->client_addr[i] = *dest_addr;
	set_client_socket(i,server_ctrl);				
	server_ctrl->client_flag[i] = 1;				
	FD_SET( server_ctrl->clientfd [i], &server_ctrl->watch_set );

	//当有人在线时不改变dvr的net_play_mode值。
	if( server_ctrl->client_num == 0 )					
		server_ctrl->net_play_mode = 0;
	
	server_ctrl->client_num++;
	if(server_ctrl->client_num > MAXCLIENT )
			server_ctrl->client_num = MAXCLIENT;
	server_ctrl->maxfd = get_max_fd( server_ctrl );		

	DPRINTK("%s come client_num=%d  supreid=%d fd=%d\n",inet_ntoa(server_ctrl->client_addr[i].sin_addr),
		server_ctrl->client_num,server_ctrl->super_user_id,server_ctrl->clientfd[i]);

	unlock_accept_mutex();
	return 1;

err:
	unlock_accept_mutex();
	return -1;
}

NETSOCKET * net_get_sever_ctrl()
{
	return &socket_ctrl;
}
	

int net_server_process(NETSOCKET * server_ctrl)
{
	struct sockaddr_in adTemp;
	struct  linger  linger_set;
	int i,sin_size;
	struct  timeval timeout;
	int ret;
	int tmp_fd = 0;
	NET_COMMAND net_cmd;
	int send_img_count = 0;
//	struct timeval stTimeoutVal;

//	signal(SIGALRM, net_send_timeout);

	server_ctrl->listenfd = -1;
	server_ctrl->super_user_id = -1;
	
	for(i= 0; i < MAXCLIENT; i++ )
	{
		server_ctrl->clientfd[i] = -1;
		server_ctrl->client_flag[i] = 0;	
		server_ctrl->client_send_data[i] = 0;
		server_ctrl->info_cmd_to_send[i]=0;
		server_ctrl->send_flag[i] = 0;
		server_ctrl->close_socket_flag[i]= 0 ;
	}

/*	server_ctrl->user_name[0][0] = '\0';
	server_ctrl->user_name[1][0] = '\0';
	server_ctrl->user_name[2][0] = '\0';
	server_ctrl->user_name[3][0] = '\0';

	strcpy(server_ctrl->user_name[0],"yue");
	strcpy(server_ctrl->user_password[0],"bin");

	strcpy(server_ctrl->user_name[1],"123");
	strcpy(server_ctrl->user_password[1],"123");
	*/
	
	if( -1 == (server_ctrl->listenfd =  socket(PF_INET,SOCK_STREAM,0)))
	{
		DPRINTK( "DVR_NET_Initialize socket initialize error\n" );
		//return DVR_ERR_SOCKET_INIT;
		return SOCKET_BUILD_ERROR;
	}

	i = 1;
	if (setsockopt(server_ctrl->listenfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) 
	{		
		close( server_ctrl->listenfd );
		server_ctrl->listenfd = -1;
		DPRINTK("setsockopt SO_REUSEADDR error \n");		
		return SOCKET_BUILD_ERROR;	
	}	

	linger_set.l_onoff = 1;
	linger_set.l_linger = 0;
       if (setsockopt( server_ctrl->listenfd, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
       {
		close( server_ctrl->listenfd );
		server_ctrl->listenfd = -1;
		DPRINTK( "pNetSocket->sockSrvfd  SO_LINGER  error\n" );
		return SOCKET_BUILD_ERROR;
	}

	if( server_ctrl->server_port  <= 0 )
		server_ctrl->server_port = 6802;

	if( server_ctrl->server_port  > 9999 )
		server_ctrl->server_port = 6802;

	if(  server_ctrl->server_port == 80 )
	{
		 server_ctrl->server_port = 6802;
	}
			

	
	
	DPRINTK( "22Bind socket port %d\n", server_ctrl->server_port);
	memset(&server_ctrl->server_addr, 0, sizeof(struct sockaddr_in)); 
	server_ctrl->server_addr.sin_family=AF_INET; 
	server_ctrl->server_addr.sin_addr.s_addr=htonl(INADDR_ANY); 
	//inet_aton("192.168.1.122", &server_ctrl->server_addr.sin_addr);
	server_ctrl->server_addr.sin_port=htons(server_ctrl->server_port); 

	if ( -1 == (bind(server_ctrl->listenfd,(struct sockaddr *)(&server_ctrl->server_addr),
		sizeof(struct sockaddr))))
	{
		close( server_ctrl->listenfd );
		server_ctrl->listenfd = -1;
		DPRINTK( "DVR_NET_Initialize socket bind error\n" );
		//return DVR_ERR_SOCKET_BIND;
		return SOCKET_BUILD_ERROR;
	}

	if ( -1 == (listen(server_ctrl->listenfd,MAXCLIENT)))
	{
		close( server_ctrl->listenfd);
		server_ctrl->listenfd = -1;
		DPRINTK( "DVR_NET_Initialize socket listen error\n" );
		//return DVR_ERR_SOCKET_LISTEN;
		return SOCKET_BUILD_ERROR;
	}
	
	sin_size=sizeof(struct sockaddr_in);

	FD_ZERO( &server_ctrl->watch_set);
	FD_SET( server_ctrl->listenfd, &server_ctrl->watch_set ) ;
	server_ctrl->maxfd = server_ctrl->listenfd;
   
	server_ctrl->socket_is_work = 1;

	DPRINTK("net server working! reset_net_server = %d\n",server_ctrl->reset_net_server);

	while(server_ctrl->reset_net_server == 0 )
	{	
		FD_ZERO(&server_ctrl->catch_fd_set); 
 		FD_SET(server_ctrl->listenfd,&server_ctrl->catch_fd_set);
		for(i = 0;i < MAXCLIENT;i++ )
		{
			if ( server_ctrl->clientfd[i] > 0 )
				FD_SET( server_ctrl->clientfd[i],&server_ctrl->catch_fd_set);
		}

		server_ctrl->maxfd = get_max_fd( server_ctrl );
	
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
	//	server_ctrl->catch_fd_set = server_ctrl->watch_set;
		ret  = select( server_ctrl->maxfd+1, 
			&server_ctrl->catch_fd_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "DVR_NET_Initialize max connect user\n" );
			continue;
		}

	
		if ( ret > 0 && FD_ISSET( server_ctrl->listenfd, &server_ctrl->catch_fd_set ) )
		{
			lock_accept_mutex();
			
			for ( i = 0; i < MAXCLIENT; i++ )
			{
				if ( 0 == server_ctrl->client_flag[i] )
					break;
			}

			if ( i >= MAX_CONNECT_NUM )
			{
				if ( -1 == (tmp_fd = accept( server_ctrl->listenfd,
					(struct sockaddr *)&adTemp, &sin_size)))
				{
					DPRINTK( "DVR_NET_Initialize accept error 2\n" );
				}
				else
				{
					memset( &net_cmd, 0, sizeof(NET_COMMAND));
					net_cmd.command =USER_TOO_MUCH;
					socket_send_data(tmp_fd, (char*)&net_cmd, 
						sizeof(NET_COMMAND));
					usleep(500000);
					close( tmp_fd );
					printf("cut %s come\n",inet_ntoa(adTemp.sin_addr));
				}
			}
			else
			{
				if ( -1 == (server_ctrl->clientfd[i] = accept( server_ctrl->listenfd, 
					(struct sockaddr *)(&server_ctrl->client_addr[i]), &sin_size) ))
				{
					printf( "DVR_NET_Initialize socket accept error\n" );
				}
				else
				{
					set_client_socket(i,server_ctrl);				
					server_ctrl->client_flag[i] = 1;				
					FD_SET( server_ctrl->clientfd [i], &server_ctrl->watch_set );

					//当有人在线时不改变dvr的net_play_mode值。
					if( server_ctrl->client_num == 0 )					
						server_ctrl->net_play_mode = 0;
					
					server_ctrl->client_num++;
					if(server_ctrl->client_num > MAXCLIENT )
							server_ctrl->client_num = MAXCLIENT;
					server_ctrl->maxfd = get_max_fd( server_ctrl );
				}

				printf("%s come client_num=%d  supreid=%d fd=%d\n",inet_ntoa(server_ctrl->client_addr[i].sin_addr),
					server_ctrl->client_num,server_ctrl->super_user_id,server_ctrl->clientfd[i]);
			}
			
			unlock_accept_mutex();
		}

		if ( ret > 0 )
		{
			for ( i = 0; i < MAX_CONNECT_NUM; i++ )
			{			
			
				if ( -1 != server_ctrl->clientfd[i] )
				{				
					if ( FD_ISSET( server_ctrl->clientfd[i], &server_ctrl->catch_fd_set ) )
					{
						ret = process_command( i, server_ctrl );
						if ( ret < 0 )
						{
							printf("%s disconnect\n",inet_ntoa(server_ctrl->client_addr[i].sin_addr));
							/*FD_CLR( server_ctrl->clientfd[i], &server_ctrl->watch_set );
							
							close( server_ctrl->clientfd[i] );

							server_ctrl->clientfd[i] = -1;
							server_ctrl->client_flag[i]  = 0;						
							
							server_ctrl->maxfd = get_max_fd( server_ctrl );
							*/
							clear_client(i,server_ctrl);
						}
					}
				}
			}
		}		

		#ifdef NET_USE_NEW_FUN	 

		#else
		send_other_info_data(server_ctrl);
	
		for( send_img_count = 0; send_img_count < 4; send_img_count++)
		{
			send_img_data(server_ctrl);
		}		

		check_play_status(server_ctrl);			

		#endif

		if( socket_ctrl.close_all_socket == 9999 )
		{
			for ( i = 0; i < MAXCLIENT; i++ )
			{			
				clear_client(i,server_ctrl);
			}

			socket_ctrl.close_all_socket = 0;
		}

		//usleep(1000);
	}

	DPRINTK( "net work end\n" );
	for ( i = 0; i < MAXCLIENT; i++ )
	{
	/*
		if ( server_ctrl->clientfd[i] > 0 )
		{
			DPRINTK( "close( server_ctrl->clientfd[%d] )\n", i );
			shutdown( server_ctrl->clientfd[i], SHUT_RDWR );
			close( server_ctrl->clientfd[i] );
		}
		server_ctrl->clientfd[i] = -1;
		server_ctrl->client_flag[i] = 0;
		*/
		clear_client(i,server_ctrl);
	}
	
	
	if ( server_ctrl->listenfd> 0 )
	{
		printf( "close( pNetSocket->sockSrvfd )\n" );
		shutdown( server_ctrl->listenfd, SHUT_RDWR );
		close( server_ctrl->listenfd );
	}
	
	server_ctrl->listenfd = -1;
	return 1;	
}

int stop_net_server()
{
	socket_ctrl.net_flag = 0;
	socket_ctrl.reset_net_server = 1;

	return ALLRIGHT;	
}


void create_net_server_thread( void )
{
	SET_PTHREAD_NAME(NULL);
	int ret;
	static int first = 1;
	int use_kill_func = 0;
	int i = 0;
	
	DPRINTK(" pid = %d\n",getpid());
	
	socket_ctrl.net_flag = 1;
	socket_ctrl.reset_net_server = 0;
	socket_ctrl.client_num = 0;
	socket_ctrl.net_trans_mode = LOCAL_NET_MODE;
	socket_ctrl.super_user_id = -1;
	socket_ctrl.server_upnp.external_port = 18000;
	socket_ctrl.server_upnp.upnp_name[0] = 0;
	socket_ctrl.server_upnp.upnp_flag = 0;
	socket_ctrl.server_upnp.upnp_over = 0;
	socket_ctrl.yun_upnp.external_port = 32000;
	socket_ctrl.yun_upnp.upnp_name[0] = 0;
	socket_ctrl.yun_upnp.upnp_flag = 0;
	socket_ctrl.yun_upnp.upnp_over= 0;
	strcpy(socket_ctrl.net_dev_name,"eth0");

	for(i= 0; i < MAXCLIENT; i++ )
	{
		pthread_mutex_init(&socket_ctrl.send_mutex[i], NULL);
	}


	


	while(socket_ctrl.net_flag)
	{	
		//if( first == 0 )
		//	nvr_allow_ipcam_connect(0);
	
		change_net_parameter(&socket_ctrl);		

		//if( first == 0 )
		//	nvr_allow_ipcam_connect(1);


		if(1)
		{
			char server_ip[512];
			char mac_id[512];
			char upnp_name[512];
			get_svr_mac_id(server_ip,mac_id);
			if( mac_id[0] != 0 )
			{
				
				{ //是否开云功能。
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					pAllInfo->net_st.yun_server_open = 0;
					open_yun_server(pAllInfo->net_st.yun_server_open);
				}

				{
					if( socket_ctrl.server_port  <= 0 )
						socket_ctrl.server_port= 6802;

					if( socket_ctrl.server_port  > 9999 )
						socket_ctrl.server_port = 6802;

					if(  socket_ctrl.server_port == 80 )					
						 socket_ctrl.server_port = 6802;					
				}

				

				if( get_yun_server_flag() == 0 )
				{
					socket_ctrl.server_upnp.internal_port = socket_ctrl.server_port;				
					socket_ctrl.server_upnp.external_port = socket_ctrl.server_port;
					strcpy(socket_ctrl.server_upnp.internal_ip,socket_ctrl.set_ip);
					sprintf(socket_ctrl.server_upnp.upnp_name,"%s_APP",mac_id);					
				}else
				{
					socket_ctrl.server_upnp.internal_port = socket_ctrl.server_port;					
					if( socket_ctrl.server_upnp.external_port < 10000)
						socket_ctrl.server_upnp.external_port = 18000;
					strcpy(socket_ctrl.server_upnp.internal_ip,socket_ctrl.set_ip);
					sprintf(socket_ctrl.server_upnp.upnp_name,"%s_APP",mac_id);	
				}

				socket_ctrl.server_upnp.upnp_over = 0;

				socket_ctrl.yun_upnp.internal_port = ys_get_yun_transmit_port();				
				strcpy(socket_ctrl.yun_upnp.internal_ip,socket_ctrl.set_ip);
				sprintf(socket_ctrl.yun_upnp.upnp_name,"%s_YUN",mac_id);	

				socket_ctrl.yun_upnp.upnp_over = 0;
				
				Lock_Set_UPnP(&socket_ctrl.server_upnp,0);	

				yun_server_restart();
			}
		}		

		//注册到服务器上
		login_to_svr();
	
		ret = net_server_process(&socket_ctrl);
		if( ret < 0 )
		{
			usleep(500000);
		}

		if(socket_ctrl.reset_net_server == 1)
		{
			sleep(1);
			socket_ctrl.reset_net_server = 0;
		}
	}
	
}




typedef struct _addr_array_
{
	int fdsock;
	int iflag;
}ADDR_ARRAY;

static ADDR_ARRAY net_addr[MAXCLIENT];

int audio_socket_inarray(int fdsock)
{
	int i;
	
	for( i = 0; i < MAXCLIENT;i++)
	{
		if( net_addr[i].iflag == 0)
		{
			net_addr[i].fdsock = fdsock;
			net_addr[i].iflag = 1;			
			DPRINTK("socket %d in\n",net_addr[i].fdsock);
			break;
		}
	}

	if( i >= MAXCLIENT )
	{
		DPRINTK("audio socket too much,kick it!\n");
		if( fdsock > 0 )
		{
			close(fdsock);
			fdsock = 0;
		}

		return -1;
	}
	
	return 1;
}

int audio_udp_send_func()
{	
	char buf[AUDIO_MAX_BUF_SIZE];
	struct sockaddr_in serv_addr,addr;
	int bd;
	int ret;
	int len;
	int sockfd;
	int i,j;
	NETSOCKET * server_ctrl;
	
	BUF_FRAMEHEADER * pheader;	
	char * send_buf = NULL;
	int send_num = 0;
	int connect_test_num = 0;
	int frame_num = 0;
	

	server_ctrl = &socket_ctrl;
	
	memset(net_addr,0x00,sizeof(ADDR_ARRAY)*MAXCLIENT);	
	
	pheader = (BUF_FRAMEHEADER *)buf;
	send_buf = buf + sizeof(BUF_FRAMEHEADER);

	//防止send失败造成pipe破裂，导致程序退出
	signal(SIGPIPE,SIG_IGN);

	DPRINTK("bbb in %d\n",server_ctrl->reset_net_udp_server );
	
	while(server_ctrl->reset_net_udp_server == 0)
	{			
		memset(buf,0,AUDIO_MAX_BUF_SIZE);		

		if( Get_udp_audio_data(pheader,send_buf) > 0 )
		{		
			
			for( i = 0; i < MAXCLIENT;i++)
			{
				if( net_addr[i].iflag == 1 )
				{
					//if( send_num % 100 == 0 )
					//	DPRINTK("send data!\n");					
				
					if( pheader->iFrameType !=  AUDIO_TYPE_G711_ULAW)
					{
						char audio_enc_buf[8192];						
						pheader->iFrameType = AUDIO_TYPE_G711_ULAW;
						pheader->iLength = G711_EnCode(audio_enc_buf,send_buf,pheader->iLength,G711_ULAW);
						memcpy(send_buf,audio_enc_buf,pheader->iLength);
					} 
					
					pheader->frame_num = frame_num;
					frame_num++;

					DPRINTK("type=%d  length=%d  num=%d  i=%d fd=%d\n",
						pheader->type,pheader->iLength,pheader->frame_num,i,net_addr[i].fdsock);
					
					ret = socket_send_data(net_addr[i].fdsock,(char*)buf, sizeof(BUF_FRAMEHEADER) + pheader->iLength);
					if ( ret <= 0 )
					{			
						DPRINTK( "socket_send  error i=%d\n",i);
						
						if( net_addr[i].fdsock > 0)
						{
							DPRINTK("close audio socket: (%d)\n",net_addr[i].fdsock);
							close(net_addr[i].fdsock);
							net_addr[i].fdsock = 0;
							net_addr[i].iflag = 0;
						}						
					}	
					
					
				}
			}

			send_num++;
		}else
		{
			usleep(100000);
		}

	}

	server_ctrl->reset_net_udp_server = 0;

	DPRINTK("bbbout\n");

	return 1;
	
}


int audio_udp_recv_func()
{	
	char buf[AUDIO_MAX_BUF_SIZE];	
	int ret;
	int len;	
	int i,j;
	NETSOCKET * server_ctrl;
	
	BUF_FRAMEHEADER * pheader;	
	char * send_buf = NULL;
	int send_num = 0;
	int connect_test_num = 0;
	fd_set watch_set;
	struct  timeval timeout;
	int max_fd = 0;	

	server_ctrl = &socket_ctrl;	
	
	pheader = (BUF_FRAMEHEADER *)buf;
	send_buf = buf + sizeof(BUF_FRAMEHEADER);

	//防止send失败造成pipe破裂，导致程序退出
	signal(SIGPIPE,SIG_IGN);

	DPRINTK("aaa in %d\n",server_ctrl->reset_net_udp_server );
	
	while(server_ctrl->reset_net_udp_server == 0)
	{		
		memset(buf,0,AUDIO_MAX_BUF_SIZE);	
	
		max_fd = 0;
		FD_ZERO(&watch_set); 
		for( i = 0; i < MAXCLIENT;i++)
		{
			if( net_addr[i].fdsock > 0)
			{
				FD_SET(net_addr[i].fdsock,&watch_set);
				if( max_fd < net_addr[i].fdsock )
					max_fd = net_addr[i].fdsock;
			}	
		}		

		if( max_fd <= 0 )
		{
			usleep(1000);
			continue;
		}
		
		timeout.tv_sec = 1;
		timeout.tv_usec = 1000;

		ret  = select( max_fd + 1, &watch_set, NULL, NULL, &timeout );
		if ( ret < 0 )
		{
			DPRINTK( "recv socket select error!\n" );
			for( i = 0; i < MAXCLIENT;i++)
			{
				if( net_addr[i].fdsock > 0)
				{
					DPRINTK("close audio socket: (%d)\n",net_addr[i].fdsock);
					close(net_addr[i].fdsock);
					net_addr[i].fdsock = 0;
					net_addr[i].iflag = 0;
				}	
			}

			continue;
		}

		for( i = 0; i < MAXCLIENT;i++)
			if ( ret > 0 && FD_ISSET( net_addr[i].fdsock, &watch_set ) )
			{
				ret = socket_read_data(net_addr[i].fdsock,(char*)pheader,sizeof(BUF_FRAMEHEADER));
				if( ret < 0 )
				{			
					if( net_addr[i].fdsock > 0)
					{
						DPRINTK("close audio socket: (%d)\n",net_addr[i].fdsock);
						close(net_addr[i].fdsock);
						net_addr[i].fdsock = 0;
						net_addr[i].iflag = 0;
					}	
				}else
				{

					ret = socket_read_data(net_addr[i].fdsock,send_buf,pheader->iLength);
					if( ret < 0 )
					{			
						if( net_addr[i].fdsock > 0)
						{
							DPRINTK("close audio socket: (%d)\n",net_addr[i].fdsock);
							close(net_addr[i].fdsock);
							net_addr[i].fdsock = 0;
							net_addr[i].iflag = 0;
						}	
					}
					else
					{
						DPRINTK("get audio: %d  %d  %d\n",pheader->iLength,net_dvr_get_net_speak_chan() ,ret);

						if( pheader->type == AUDIO_FRAME)
						{
							if( pheader->iFrameType == AUDIO_TYPE_G711_ULAW )
							{
								if( pheader->iLength <  ( 8192/3) )
								{
										char audio_dec_buf[8192];
										pheader->iLength =G711_Decode(audio_dec_buf ,buf + sizeof(BUF_FRAMEHEADER),pheader->iLength,G711_ULAW);
										pheader->iFrameType = AUDIO_TYPE_PCM;
										memcpy(send_buf,audio_dec_buf,pheader->iLength);
								}
							}else
							{
								pheader->iFrameType = AUDIO_TYPE_PCM;
							}
						}

						if( net_dvr_get_net_speak_chan() == NET_DVR_CHAN )
						{
							if( machine_use_audio() == 0 )
							{								
								Hisi_write_audio_buf((unsigned char *)send_buf,pheader->iLength,0);
							}else
							{
								DPRINTK("machine_use_audio == 1\n");
							}
						}else
						{
							int speak_cam;
							speak_cam = net_dvr_get_net_speak_chan();

							for( j = 0;j < MAX_CHANNEL; j++ )
							if( get_bit(speak_cam, j) )
							{									
								//Net_dvr_send_audio_data_to_cam(j,buf,AUDIO_MAX_BUF_SIZE);							
							}
						}
					}
				}
			}	
	}

	for( i = 0; i < MAXCLIENT;i++)
	{
		if( net_addr[i].fdsock > 0)
		{			
			close(net_addr[i].fdsock);
			net_addr[i].fdsock = 0;
			net_addr[i].iflag = 0;
		}	
	}

	server_ctrl->reset_net_udp_server = 0;

	DPRINTK("aaaout\n");

	return 1;
	
}


int get_net_trans_mode()
{
	return socket_ctrl.net_trans_mode;
}


void net_set_write_dns_file(char *dns1,char *dns2)
{
	char cmd[180];	

	sprintf(cmd,"echo \"nameserver %s\" > /etc/resolv.conf;echo \"nameserver %s\" >> /etc/resolv.conf;sync;",
		dns1,dns2);

	command_drv(cmd);	
}


int set_net_parameter(char * ip, int port, char * net_gate,char * netmask,char * dns1,char *dns2)
{
	char self_ip[] = "192.168.1.50";
	char self_net_gate[] = "192.168.1.1";
	char self_netmask[] = "255.255.255.0";
	char dns[] = "0,0,0,0";

	if( ip == NULL || net_gate == NULL )
	{	
		printf(" SetNetParameter parameter error!\n");
		return ERROR;
	}

	if( port < 0 || port > 9999 )
		port = 6802;

	printf("(%s,%d)%s:%s, %s,%s\n",__FILE__, __LINE__, __FUNCTION__,
  	ip, net_gate,  netmask);

	if( strcmp(ip,"0.0.0.0") == 0 )
	{
		printf(" ip is %s,so self data\n",ip);
		
		socket_ctrl.server_port = port;

		strcpy(socket_ctrl.set_ip,self_ip);

		strcpy(socket_ctrl.set_net_gateway,self_net_gate);

		strcpy(socket_ctrl.set_net_mask,self_netmask);
		
	}else
	{
		socket_ctrl.server_port = port;

		strcpy(socket_ctrl.set_ip,ip);

		strcpy(socket_ctrl.set_net_gateway,net_gate);

		strcpy(socket_ctrl.set_net_mask,netmask);
	}

	if( dns1[0] == 0 )
	{
		strcpy(socket_ctrl.dns1,dns);
	}else
	{
		strcpy(socket_ctrl.dns1,dns1);
	}

	if( dns2[0] == 0 )
	{
		strcpy(socket_ctrl.dns2,dns);
	}else
	{
		strcpy(socket_ctrl.dns2,dns2);
	}

	net_set_write_dns_file(socket_ctrl.dns1,socket_ctrl.dns2);

	change_ip_flag = 1;

	socket_ctrl.reset_net_server = 1;
	socket_ctrl.reset_net_udp_server = 1;


	printf(".... change ip %s port=%d\n",ip,port);	

	if(get_appselect_num() == 0)
		change_net_parameter(&socket_ctrl);
	
	return ALLRIGHT;
}




int set_net_parameter_dev(char * dev_name,char * ip, int port, char * net_gate,char * netmask,char * dns1,char *dns2)
{
	char self_ip[] = "192.168.1.50";
	char self_net_gate[] = "192.168.1.1";
	char self_netmask[] = "255.255.255.0";
	char dns[] = "0,0,0,0";
	static int use_ip_set = 0;

	if( ip == NULL || net_gate == NULL )
	{	
		printf(" SetNetParameter parameter error!\n");
		return ERROR;
	}
	

	if( port < 0 || port > 9999 )
		port = 6802;

	printf("(%s,%d)%s:%s, %s,%s\n",__FILE__, __LINE__, __FUNCTION__,
  	ip, net_gate,  netmask);	
	

	if( strcmp(ip,"0.0.0.0") == 0 )
	{
		printf(" ip is %s,so self data\n",ip);
		
		socket_ctrl.server_port = port;

		strcpy(socket_ctrl.set_ip,self_ip);

		strcpy(socket_ctrl.set_net_gateway,self_net_gate);

		strcpy(socket_ctrl.set_net_mask,self_netmask);
		
	}else
	{
		socket_ctrl.server_port = port;

		strcpy(socket_ctrl.set_ip,ip);

		strcpy(socket_ctrl.set_net_gateway,net_gate);

		strcpy(socket_ctrl.set_net_mask,netmask);
	}

	if( dns1[0] == 0 )
	{
		strcpy(socket_ctrl.dns1,dns);
	}else
	{
		strcpy(socket_ctrl.dns1,dns1);
	}

	if( dns2[0] == 0 )
	{
		strcpy(socket_ctrl.dns2,dns);
	}else
	{
		strcpy(socket_ctrl.dns2,dns2);
	}

	strcpy(socket_ctrl.net_dev_name,dev_name);

	//net_set_write_dns_file(socket_ctrl.dns1,socket_ctrl.dns2);

	change_ip_flag = 1;

	socket_ctrl.reset_net_server = 1;

	printf(".... change ip %s port=%d socket_ctrl.set_ip=%s\n",ip,port,socket_ctrl.set_ip);	

	change_net_parameter_dev(&socket_ctrl);	
	
	return ALLRIGHT;
}



void init_random() 
{ 
  unsigned int ticks; 
  struct timeval tv; 
  int fd; 
  gettimeofday (&tv, NULL); 
  ticks = tv.tv_sec + tv.tv_usec; 
  fd = open ("/dev/urandom", O_RDONLY); 
  if (fd >= 0) 
    { 
      unsigned int r; 
      int i; 
      for (i = 0; i < 512; i++) 
        { 
          read (fd, &r, sizeof (r)); 
          ticks += r; 
        } 
      close (fd); 
    } 
  srand (ticks); 
 // DPRINTK("init finished \n"); 
} 

unsigned int new_rand() 
{ 
  int fd; 
  unsigned int n = 0; 
  fd = open ("/dev/urandom", O_RDONLY); 
  if (fd > 0) 
  { 
    read (fd, &n, sizeof (n)); 
  } 
  close (fd); 
  return n; 
}

int get_mac_random()
{
	int n;
	init_random(); 
  	n = rand ();

	return n%100000;
}

int get_mac_num()
{
	int num;
	FILE * fp=NULL;
	FILE * fp2=NULL;

	fp = fopen("/mnt/mtd/mac_num","rb");
	if( fp == NULL )
	{
		num = get_mac_random();
		if( num == 0 )
			num = get_mac_random();

		fp2 = fopen("/mnt/mtd/mac_num","wb");
		if( fp2 == NULL )
		{
			DPRINTK("can't open mac_num\n");
		}else
		{
			fwrite(&num,1,sizeof(num),fp2);
		}

		if( fp2 )
			fclose(fp2);

		Net_cam_mtd_store();
		
	}else
	{
		fread(&num,1,sizeof(num),fp);
	}

	if( fp )
	fclose(fp);

	return num;
}

static int start_net_connect_work = 0;
static char net_set_buf[800] = {0};
static char net_gw_buf[50];


int dvr_test_net_connected(char * net_set_cmd,char * pGateWay)
{
	int net_line_connecting = 0;
	int get_way_is_exist = 0;
	char net_dev[30] = "eth0";

//	strcpy(net_set_buf,net_set_cmd);
//	strcpy(net_gw_buf,pGateWay);

	net_line_connecting = net_line_connect(net_dev,socket_ctrl.set_net_gateway);

	if( net_line_connecting == 0 )
	{
	/*	get_way_is_exist = net_ping(socket_ctrl.set_net_gateway);		

	
		if( get_way_is_exist > 0)
		{
			start_net_connect_work = 1;
		}else
		{
			start_net_connect_work = 0;
		}
	*/
		start_net_connect_work = 1;
	}else
	{
		start_net_connect_work = 0;
	}

	DPRINTK("start_net_connect_work %d %d\n",start_net_connect_work,get_way_is_exist);

	return 1;
}



void net_connect_test_stop()
{
	start_net_connect_work = 0;
	DPRINTK("stop\n");
}

int net_connected_op_func()
{
	int net_connecting = 0;
	int ret;
	int test_time=60*60; //one hour test onece.
	int test_count = 0;
	int i;
	int connect_to_cam = 0;

	return 1;

	sleep(90);

	DPRINTK("wireless test\n");

	ret = command_drv("cat /proc/net/dev_mcast | grep eth0");
	DPRINTK("cat /proc/net/dev_mcast | grep eth0 =%d\n",ret);	
	if( ret == 0)
		start_net_connect_work = 0;
	else
		start_net_connect_work = 1;	

	while(1)
	{
		/*
		if( start_net_connect_work )
		{
			if( (Net_dvr_have_client() == 1) || (is_have_client() > 0 ))
			{				
				test_count = 0;
			}

			if( test_count >= 10 )
			{		
				net_connecting = net_ping(socket_ctrl.set_net_gateway);

				if( net_connecting < 0 )
				{
					DPRINTK("Can't connect gw!test_count=%d test_time=%d\n",test_count,test_time);								
					sleep(10);
					test_count = test_count  + 10;
					
				}else
				{
					DPRINTK("router online\n");
					test_count = 0;
				}
			}

			test_count++;

			if( test_count > test_time )
			{
				DPRINTK("can't connect router!\n");
				command_drv("reboot");
			}
		}*/

		sleep(1);	
		
	}
}








int  change_net_parameter_dev(NETSOCKET * server_ctrl)
{
	char cmd[200];
	char tmp[800];
	int ret;
	int num;
	int ip_last_num = 0;
	static int mac_num = 0;
	int buf_pos = 0;


	memset(cmd,0x00,200);
	memset(tmp,0x00,800);

	if( change_ip_flag == 0 )
		return 0;
	
	change_ip_flag = 0;	


	DPRINTK("net_dev_name=%s\n",server_ctrl->net_dev_name);

	//if( strcmp(server_ctrl->net_dev_name,"eth0") ==0 )
	{
		sprintf(cmd,"ifconfig %s 0.0.0.0;",server_ctrl->net_dev_name);

		strcpy(tmp + buf_pos,cmd);

		buf_pos += strlen(cmd);
	}

	num = strlen(server_ctrl->set_ip);

	ip_last_num =  server_ctrl->set_ip[num-1] -48;

	if( server_ctrl->set_ip[num-2] >= 48 )
		ip_last_num = ip_last_num + (server_ctrl->set_ip[num-2] -48) * 10;

	if( server_ctrl->set_ip[num-3]>= 48 )
		ip_last_num = ip_last_num + (server_ctrl->set_ip[num-3] -48) * 100;	



	//if( get_ipcam_mode_num() == IPCAM_ID_MODE )
	if(1)
	{
		if( mac_num == 0 )
		{
			mac_num = get_mac_num();
			DPRINTK("mac_num = %d\n",mac_num);

			if(1)
			{
				char server_ip[512];
				char mac_id[512];
				int mac_hex_id = 0;
				get_svr_mac_id(server_ip,mac_id);

				if( mac_id[0] != NULL )
				{
					int i = 0,j = 0;

					for( i = 0; i < strlen(mac_id); i++)
					{
						if( mac_id[i] <= '9' && mac_id[i] >= '0' )
						{
							server_ip[j] = mac_id[i];
							j++;
						}
					}

					server_ip[j] = '\0';
					mac_hex_id = atoi(server_ip);		
				}

				DPRINTK("mac_hex_id = %d\n",mac_hex_id);

				if( mac_hex_id != 0 )
				{				
					mac_num = mac_hex_id;
					DPRINTK("mac_num = %d  %x  id=%s\n",mac_num,mac_num,mac_id);
				}
			}
		}

		if( strcmp(server_ctrl->net_dev_name,"eth0") ==0 )
		{
			sprintf(cmd,"ifconfig %s hw ether 00:18:E6:%d%d:%d%d:%d%d;",server_ctrl->net_dev_name,(mac_num%1000000)/100000,(mac_num % 100000) /10000,(mac_num % 10000) /1000,
			(mac_num % 1000) /100,(mac_num % 100) /10,mac_num % 10);		

			server_ctrl->net_dev_mac[0] = 0x00;
			server_ctrl->net_dev_mac[1] = 0x18;
			server_ctrl->net_dev_mac[2] = 0xE6;
			server_ctrl->net_dev_mac[3] = ((mac_num%1000000)/100000)*16 + (mac_num % 100000) /10000;
			server_ctrl->net_dev_mac[4] = ((mac_num % 10000) /1000)*16 + (mac_num % 1000) /100;
			server_ctrl->net_dev_mac[5] = ((mac_num % 100) /10)*16 + mac_num % 10;

			strcpy(tmp + buf_pos,cmd);

			buf_pos += strlen(cmd);
		}
	}else
	{
		sprintf(cmd,"ifconfig %s hw ether 00:18:E6:8%d:B6:%d%d;",server_ctrl->net_dev_name,ip_last_num /100,(ip_last_num % 100) /10, ip_last_num % 10);
		strcpy(tmp + buf_pos,cmd);
		buf_pos += strlen(cmd);
	}

	sprintf(cmd,"ifconfig %s %s netmask %s;",server_ctrl->net_dev_name,server_ctrl->set_ip,server_ctrl->set_net_mask);
	

	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);

	if( strcmp(server_ctrl->net_dev_name,"eth0") ==0 )
	{
		sprintf(cmd,"ifconfig %s up;",server_ctrl->net_dev_name);

		strcpy(tmp + buf_pos,cmd);

		buf_pos += strlen(cmd);	
	}

	{
		unsigned char host_ip[4] = {0};
		unsigned char host_net_mask[4] = {0};
		unsigned char host_net_gw[4] = {0};
		unsigned char host_net_new_gw[4] = {0};
		unsigned int tmp_num[4];
		int i = 0;
		sscanf(server_ctrl->set_ip,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_ip[0] = tmp_num [0];
		host_ip[1] = tmp_num [1];
		host_ip[2] = tmp_num [2];
		host_ip[3] = tmp_num [3];		
		
		sscanf(server_ctrl->set_net_mask,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_net_mask[0] = tmp_num [0];
		host_net_mask[1] = tmp_num [1];
		host_net_mask[2] = tmp_num [2];
		host_net_mask[3] = tmp_num [3];
		sscanf(server_ctrl->set_net_gateway,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_net_gw[0] = tmp_num [0];
		host_net_gw[1] = tmp_num [1];
		host_net_gw[2] = tmp_num [2];
		host_net_gw[3] = tmp_num [3];

		if( host_net_new_gw[0] == 0 )
			host_net_new_gw[0] = 1;
		if( host_net_new_gw[3] == 0 )
			host_net_new_gw[3] = 1;

		for( i = 0; i < 4 ; i++)
		{
			host_net_new_gw[i] = (host_ip[i]&host_net_mask[i]) |( (~host_net_mask[i]) & host_net_gw[i]);

			DPRINTK("gw:%x&%x=%x %x&%x=%x \n",host_ip[i],host_net_mask[i],host_ip[i]&host_net_mask[i],
				~host_net_mask[i],host_net_gw[i],(~host_net_mask[i]) & host_net_gw[i]);
		}		
		
		sprintf(cmd,"route add default gw %d.%d.%d.%d;",host_net_new_gw[0],host_net_new_gw[1],
			host_net_new_gw[2],host_net_new_gw[3]);
		
		DPRINTK("%s\n",cmd);
	}
		
	//sprintf(cmd,"route add default gw %s;",server_ctrl->set_net_gateway);	
	

	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);

	printf("command: %s-------\n",tmp);

	if (access("/ramdisk/mulInterface",F_OK) == 0)
	{
		sprintf(cmd,"ifconfig br0 hw ether 00:18:E6:%d%d:%d%d:%d%d;",(mac_num%1000000)/100000,(mac_num % 100000) /10000,(mac_num % 10000) /1000,
			(mac_num % 1000) /100,(mac_num % 100) /10,mac_num % 10);
		command_process(cmd);
		Ioctl_set_br0_net(server_ctrl->set_ip, server_ctrl->set_net_mask, server_ctrl->set_net_gateway);
	}
	else
	{
		ret = command_process(tmp);
		if( ret < 0 )
			return -1;	
	}

//	sleep(2);

//	process_send_command("echo '' > /ramdisk/camsetDone");

	printf(" change net parameter success!\n");


	strcpy(net_set_buf,tmp);
	

	return ALLRIGHT;

}


void reset_net_ip()
{
	command_process(net_set_buf);
}


int  change_net_parameter(NETSOCKET * server_ctrl)
{
	char cmd[200];
	char tmp[800];
	int ret;
	int num;
	int ip_last_num = 0;
	static int mac_num = 0;
	int buf_pos = 0;


	memset(cmd,0x00,200);
	memset(tmp,0x00,800);

	if( change_ip_flag == 0 )
		return 0;
	
	change_ip_flag = 0;	


	//strcpy(cmd,"ifconfig eth0 0.0.0.0;");


	
	strcpy(cmd,"ifconfig eth0 down;");


	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);

	num = strlen(server_ctrl->set_ip);

	ip_last_num =  server_ctrl->set_ip[num-1] -48;

	if( server_ctrl->set_ip[num-2] >= 48 )
		ip_last_num = ip_last_num + (server_ctrl->set_ip[num-2] -48) * 10;

	if( server_ctrl->set_ip[num-3]>= 48 )
		ip_last_num = ip_last_num + (server_ctrl->set_ip[num-3] -48) * 100;	



	//if( get_ipcam_mode_num() == IPCAM_ID_MODE )
	if( 1 )
	{
		if( mac_num == 0 )
		{
			mac_num = get_mac_num();
			DPRINTK("mac_num = %d\n",mac_num);

			if(1)
			{
				char server_ip[512];
				char mac_id[512];
				int mac_hex_id = 0;
				get_svr_mac_id(server_ip,mac_id);

				if( mac_id[0] != NULL )
				{
					int i = 0,j = 0;

					for( i = 0; i < strlen(mac_id); i++)
					{
						if( mac_id[i] <= '9' && mac_id[i] >= '0' )
						{
							server_ip[j] = mac_id[i];
							j++;
						}
					}

					server_ip[j] = '\0';
					mac_hex_id = atoi(server_ip);		
				}

				DPRINTK("mac_hex_id = %d\n",mac_hex_id);

				if( mac_hex_id != 0 )
				{				
					mac_num = mac_hex_id;
					DPRINTK("mac_num = %d  %x  id=%s\n",mac_num,mac_num,mac_id);
				}
			}
		}

		sprintf(cmd,"ifconfig eth0 hw ether 00:18:E6:%d%d:%d%d:%d%d;",(mac_num%1000000)/100000,(mac_num % 100000) /10000,(mac_num % 10000) /1000,
		(mac_num % 1000) /100,(mac_num % 100) /10,mac_num % 10);		

		server_ctrl->net_dev_mac[0] = 0x00;
		server_ctrl->net_dev_mac[1] = 0x18;
		server_ctrl->net_dev_mac[2] = 0xE6;
		server_ctrl->net_dev_mac[3] = ((mac_num%1000000)/100000)*16 + (mac_num % 100000) /10000;
		server_ctrl->net_dev_mac[4] = ((mac_num % 10000) /1000)*16 + (mac_num % 1000) /100;
		server_ctrl->net_dev_mac[5] = ((mac_num % 100) /10)*16 + mac_num % 10;


		strcpy(tmp + buf_pos,cmd);

		buf_pos += strlen(cmd);
	}else
	{
		sprintf(cmd,"ifconfig eth0 hw ether 00:18:E6:8%d:B6:%d%d;",ip_last_num /100,(ip_last_num % 100) /10, ip_last_num % 10);
		strcpy(tmp + buf_pos,cmd);
		buf_pos += strlen(cmd);
	}

	sprintf(cmd,"ifconfig eth0 %s netmask %s;",server_ctrl->set_ip,server_ctrl->set_net_mask);
	

	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);


	strcpy(cmd,"ifconfig eth0 up;");

	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);		
	
	{
		unsigned char host_ip[4] = {0};
		unsigned char host_net_mask[4] = {0};
		unsigned char host_net_gw[4] = {0};
		unsigned char host_net_new_gw[4] = {0};
		unsigned int tmp_num[4];
		int i = 0;
		sscanf(server_ctrl->set_ip,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_ip[0] = tmp_num [0];
		host_ip[1] = tmp_num [1];
		host_ip[2] = tmp_num [2];
		host_ip[3] = tmp_num [3];		
		sscanf(server_ctrl->set_net_mask,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_net_mask[0] = tmp_num [0];
		host_net_mask[1] = tmp_num [1];
		host_net_mask[2] = tmp_num [2];
		host_net_mask[3] = tmp_num [3];
		sscanf(server_ctrl->set_net_gateway,"%d.%d.%d.%d",&tmp_num[0],&tmp_num[1],&tmp_num[2],&tmp_num[3]);
		host_net_gw[0] = tmp_num [0];
		host_net_gw[1] = tmp_num [1];
		host_net_gw[2] = tmp_num [2];
		host_net_gw[3] = tmp_num [3];

		for( i = 0; i < 4 ; i++)
		{
			host_net_new_gw[i] = (host_ip[i]&host_net_mask[i]) |( (~host_net_mask[i]) & host_net_gw[i]);

			DPRINTK("gw:%x&%x=%x %x&%x=%x \n",host_ip[i],host_net_mask[i],host_ip[i]&host_net_mask[i],
				~host_net_mask[i],host_net_gw[i],(~host_net_mask[i]) & host_net_gw[i]);
		}			
		
		if( host_net_new_gw[0] == 0 )
			host_net_new_gw[0] = 1;
		if( host_net_new_gw[3] == 0 )
			host_net_new_gw[3] = 1;
		
		sprintf(cmd,"route add default gw %d.%d.%d.%d;",host_net_new_gw[0],host_net_new_gw[1],
			host_net_new_gw[2],host_net_new_gw[3]);

		DPRINTK("%s\n",cmd);
	}
		
	//sprintf(cmd,"route add default gw %s;",server_ctrl->set_net_gateway);	

	strcpy(tmp + buf_pos,cmd);

	buf_pos += strlen(cmd);

	//printf("command: %s\n",tmp);

	if (access("/ramdisk/mulInterface",F_OK) == 0)
	{
		sprintf(cmd,"ifconfig br0 hw ether 00:18:E6:%d%d:%d%d:%d%d;",(mac_num%1000000)/100000,(mac_num % 100000) /10000,(mac_num % 10000) /1000,
			(mac_num % 1000) /100,(mac_num % 100) /10,mac_num % 10);
		command_process(cmd);
		Ioctl_set_br0_net(server_ctrl->set_ip, server_ctrl->set_net_mask, server_ctrl->set_net_gateway);
	}
	else
	{
		ret = command_process(tmp);
		if( ret < 0 )
			return -1;	
	}

//	sleep(2);

//	process_send_command("echo '' > /ramdisk/camsetDone");

	printf(" change net parameter success!\n");


	strcpy(net_set_buf,tmp);
	

	return ALLRIGHT;

}


int net_kick_client( NETSOCKET * server_ctrl)
{
	if( close_client )
	{
		 clear_client(socket_ctrl.cur_client_idx, &socket_ctrl);
		close_client = 0;

	}

	return 1;
}

int kick_client()
{
/*	int id;

	id = 0;
	
	if( socket_ctrl.clientfd[0] > 0 )
		close_client = 1;

	printf(" close_client = %d\n",close_client);
	*/	

	return kick_all_client(&socket_ctrl);
}

int kick_normal_client(NETSOCKET * server_ctrl)
{
	int i;

	start_stop_net_file_view(0);
	server_ctrl->net_play_mode = 0;

	for(i = 0; i < MAXCLIENT; i++ )
	{
		if( i != server_ctrl->super_user_id )
		{
			if( server_ctrl->clientfd[i] >= 0 )
				clear_client(i,server_ctrl);
		}
	}

	return 1;
}

int kick_all_client(NETSOCKET * server_ctrl)
{
	int i;

// 踢掉超级用户
printf("super ...\n");
	if( server_ctrl->super_user_id != -1)
	{
		clear_client(server_ctrl->super_user_id,server_ctrl);
		return 1;
	}	

	printf("normal ...\n");

//踢掉普通拥护
	start_stop_net_file_view(0);
	start_stop_net_view(0,0xffff);
	server_ctrl->net_play_mode = 0;

	for(i = 0; i < MAXCLIENT; i++ )
	{
		if( server_ctrl->clientfd[i] > 0 )
			clear_client(i,server_ctrl);		
	}

	printf(" end...\n");

	return 1;
}

int get_client_info(char * client_ip)
{
	if( socket_ctrl.clientfd[0] > 0 )
	{
		strcpy(client_ip,inet_ntoa(socket_ctrl.client_addr[0].sin_addr));
		return ALLRIGHT;
	}
	
	client_ip[0] = '\0';
	return ERROR;	
}

int get_client_info_by_id(char * client_ip, int id)
{
	int i;
	int y = 0;

	for(i = 0; i < MAXCLIENT;i ++ )
	{
		if( socket_ctrl.clientfd[i] >= 0 )
		{
			if( y == id )
			{
				strcpy(client_ip,inet_ntoa(socket_ctrl.client_addr[i].sin_addr));
				return ALLRIGHT;
			}

			y++;
		}
	}

	client_ip[0] = '\0';
	return ERROR;	
}

int get_client_user_num()
{	
	return socket_ctrl.client_num;
}


int is_have_client()
{

	if( socket_ctrl.client_num> 0 )
		return 1;
	else
		return -1;
}

int get_admin_info(char * name1,char * pass1,char * name2, char * pass2,
	char * name3,char * pass3	,char * name4,char * pass4)
{
	
	
	strcpy(socket_ctrl.user_name[0],name1);
	strcpy(socket_ctrl.user_name[1],name2);
	strcpy(socket_ctrl.user_name[2],name3);
	strcpy(socket_ctrl.user_name[3],name4);

	strcpy(socket_ctrl.user_password[0],pass1);
	strcpy(socket_ctrl.user_password[1],pass2);
	strcpy(socket_ctrl.user_password[2],pass3);
	strcpy(socket_ctrl.user_password[3],pass4);

	//printf("net name %s, pass=%s\n",socket_ctrl.user_name[0],socket_ctrl.user_password[0]);

	return ALLRIGHT;
}


int FindSTR( char  * byhtml, char *cfind, int nStartPos)
{
	int i;
	long length;
	char * tmphtml;
	char * temfind ;

	
	if( byhtml == NULL || cfind == NULL)
		return -1;

	tmphtml =(char *) byhtml + nStartPos;

	temfind = cfind;

	if(nStartPos < 0)
		return -1;

	length = strlen(tmphtml); 
	
	for( i = 0 ; i < length; i++)
	{		

		if( *tmphtml != *temfind )
		{
			tmphtml++;

			continue;
		}

		while( *tmphtml == *temfind )
		{
			//printf(" %c.%c|",*tmphtml,*temfind);
			tmphtml++;
			temfind++;

			if( *temfind == (char)NULL ) // 找到了。
			{			
				return nStartPos + i;
			}
		}

		//printf("\n");	
		
		if( *temfind == (char)NULL ) // 找到了。
		{			
			return nStartPos + i;
		}
		else
		{	// 从与temfind首字节相同的那一位的后一位重新开始找，
			temfind = cfind;
			tmphtml = (char *)byhtml + nStartPos + i + 1;
		}
	}

	return -1;
}

// start 是指 '<' 所在的位置，end 指 '>'的位置，此函数取两个位置之间的那部分字符串。
int GetTmpLink(char * byhtml, char * temp, int start, int end,int max_num)
{
	int i;
	
	if(  byhtml == NULL ||  temp == NULL)
		return -1;

	if( end - start > max_num )
	{		
		temp[0] = (char)NULL;
		return -1;
	}	

	for(i = start + 1; i < end ; i++)
	{
		*temp = byhtml[i];

		temp++;
	}

	*temp = (char)NULL; // 结束符。

	return 1;
}


int nvr_drv_get_file_data(char* file_name,char * buf,int max_len)
{
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;


	fp = fopen(file_name,"rb");
	if( fp == NULL )
	{
		DPRINTK(" open %s error!\n",file_name);
		goto get_err;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		DPRINTK("fseek error!!\n");
		goto get_err;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		DPRINTK(" ftell error!\n");
		goto get_err;
	}

	DPRINTK(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 0 )
	{	
		rel = fread(buf,1,max_len,fp);
		if( rel <= 0 )
		{
			DPRINTK(" fread Error!\n");
			goto get_err;
		}	
		
	}

	fclose(fp);

	return rel;

get_err:
	if( fp )
	   fclose(fp);

	return -1;
}


void kill_dhcp()
{
	char cmd[200];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i,j;
	int pid;

	sprintf(cmd,"ps | grep /mnt/mtd/dhclient|grep -v grep > /tmp/2.txt");

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return ;
	}

	fp = fopen("/tmp/2.txt","r");
	if( fp == NULL )
	{
		printf(" open /tmp/2.txt error!\n");
		return ;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		return ;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		return ;
	}

	printf(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 )
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			fclose(fp);
			return ;
		}	

		cmd[5] = '\0';

		pid = atoi(cmd);		

		sprintf(cmd,"kill -9 %d",pid);

		printf("cmd: %s\n",cmd);

		rel = command_drv(cmd);
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			fclose(fp);
			return ;
		}
	}

	fclose(fp);
	
}

int del_dns_file()
{
	char cmd[350];	
	FILE *fp=NULL;
//	long fileOffset = 0;
	int rel;
//	int i,j;
//	int pid;
	

/*	fp = fopen("/mnt/mtd/dns.txt","r");
	if( fp == NULL )
	{
		printf("not have /mnt/mtd/dns.txt !\n");
		return 1;
	}	

	fclose(fp);
*/

	sprintf(cmd,"rm  -rf /mnt/mtd/dns.txt");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);

	//sprintf(cmd,"/mnt/mtd/dns.txt");	

	//rel = remove(cmd);

	if( rel < 0 )
	{
		DPRINTK("rm %s  error!\n",cmd);
	}

	//sprintf(cmd,"rm  -rf /etc/resolv.conf");	
	sprintf(cmd,"echo ""  /etc/resolv.conf");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);

	//sprintf(cmd,"/mnt/mtd/dns.txt");	

	//rel = remove(cmd);

	if( rel < 0 )
	{
		DPRINTK("rm %s  error!\n",cmd);
	}


	return rel;
}


int write_dns_file()
{
	char cmd[350];
	int rel;	
//	FILE *fp=NULL;
//	long fileOffset = 0;
//	int j;

/*	fp = fopen("/etc/resolv.conf","r");
	if( fp == NULL )
	{
		printf(" open /etc/resolv.conf error!\n");
		goto GET_ERROR1;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR1;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR1;
	}

	printf(" fileOffset = %d\n",fileOffset);

	rewind(fp);	

	
	if( fileOffset > 5 && fileOffset < 300)
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!\n");
			goto GET_ERROR1;
		}	

		cmd[fileOffset] = '\0';

		j = FindSTR(cmd,"search",0);		
		if( j >= 0)		
			goto GET_ERROR1;
		
	}else
	{
		printf(" fileOffset = %d error\n",fileOffset);

		goto GET_ERROR1;
	}

	fclose(fp);
*/

	sprintf(cmd,"cat /etc/resolv.conf > /mnt/mtd/dns.txt");	

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);

	return rel;	

/*GET_ERROR1:

	if( fp )
		fclose(fp);

	return ERROR; */
}


static void net_get_dns(char * dns1,char * dns2)
{
	char cmd[350];
	FILE *fp=NULL;
	long fileOffset = 0;
	char * pcmd = NULL;
	int i;
	int not_url = 0;
	int get_num = 0;

	dns1[0] = 0;
	dns2[0] = 0;

	fp = fopen("/etc/resolv.conf","r");
	if( fp == NULL )
	{
		printf(" open /etc/resolv.conf error!\n");
		goto G_ERROR;
	}

	while(1)
	{	
		pcmd = fgets(cmd,100,fp);

		if( pcmd == NULL )
		{
			DPRINTK("can't get lines\n");
			goto G_ERROR;
		}else
		{
			not_url = 0;

			for(i=0;i<100;i++)
			{
				if( (pcmd[i] == '\n') || (pcmd[i] == '\r') )
				{
					pcmd[i] = 0;
					break;
				}
			}
			
			
			if( (pcmd[0] != 'n') || (pcmd[1] != 'a') || (pcmd[2] != 'm')|| (pcmd[3] != 'e')  )
			{
					not_url = 1;
					DPRINTK("%s is not ip\n",pcmd);					
			}
			

			if( not_url == 0 )
			{
				if( get_num == 0 )
				{
					strcpy(dns1,&pcmd[strlen("nameserver")+1]);
					DPRINTK("dns1=%s\n",dns1);
					if( strcmp(dns1,"127.0.0.1" ) != 0 )
						get_num++;
				}else
				{
					strcpy(dns2,&pcmd[strlen("nameserver")+1]);
					DPRINTK("dns2=%s\n",dns2);
					goto G_ERROR;
				}
				
			}
		}
	}

	

	G_ERROR:

	if( fp )
		fclose(fp);

	return ;
}

#define DHCP_TXT "/tmp/dhcp.txt"


int try_dhcp(char * ip,char * netmsk,char * gateway,char * dns1,char * dns2)
{
	char cmd[3000];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i,j;
	char host_ip[16]={0};
	int iIsMulInterface = 0;// 0:eth0 1:ra0 2:br0

	if (access("/ramdisk/mulInterface",F_OK) == 0)
		iIsMulInterface = 2;
	else if(access("/ramdisk/raInterface",F_OK) == 0)
		iIsMulInterface = 1;
		

	ip[0] = 0;
	gateway[0] = 0;
	dns1[0] = 0;
	dns2[0] = 0;

	rel = del_dns_file();
	if( rel < 0 )
	{
		printf("del_dns_file error!\n");
		goto GET_ERROR;
	}


	sprintf(cmd,"rm -rf %s",DHCP_TXT);

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);


	command_drv("killall udhcpc");

	if (iIsMulInterface == 1)
		sprintf(cmd,"udhcpc -b -i ra0 -s /mnt/mtd/simple.script > %s",DHCP_TXT);
	else if (iIsMulInterface == 2)
		sprintf(cmd,"udhcpc -b -i br0 -s /mnt/mtd/simple.script > %s",DHCP_TXT);
	else
		sprintf(cmd,"udhcpc -b -s /mnt/mtd/simple.script > %s",DHCP_TXT);

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		goto GET_ERROR;
	}

	i = 0;

	while(1)
	{
		DPRINTK("wait %d\n",i);
		sleep(1);
		i++;

		fp = fopen(DHCP_TXT,"r");
		if( fp )
		{
			fclose(fp);
			fp = NULL;
			break;
		}

		if( i > 12 )
		{
			DPRINTK("time out %d\n",i);
			break;
		}
	}

	

	fp = fopen(DHCP_TXT,"r");
	if( fp == NULL )
	{
		printf(" open %s error!\n",DHCP_TXT);
		goto GET_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR;
	}

	//printf(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 5 && fileOffset < 2990)
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto GET_ERROR;
		}	

		cmd[fileOffset] = '\0';		

		j = FindSTR(cmd," obtained",0);		
		if( j < 0)		
			goto GET_ERROR;

		i = FindSTR(cmd,"Lease of ",0);		
		if( i < 0)		
			goto GET_ERROR;
		

		rel = GetTmpLink(cmd,ip,i+strlen("Lease of ")-1,j,30);
		if( rel < 0)		
			goto GET_ERROR;			

		
	}else
	{
		
		printf(" fileOffset = %ld error\n",fileOffset);
		goto GET_ERROR;	
	}

	fclose(fp);
	fp = NULL;

	sprintf(cmd,"rm -rf %s",DHCP_TXT);
	printf("start cmd : %s \n",cmd);
	rel = command_drv(cmd);

	net_get_dns(dns1,dns2);

	gateway[0] = 0;

	printf("start search gateway\n");

	rel = get_gateway(gateway);
	if( rel < 0 )
		goto GET_ERROR;

	if( gateway[0] == 0 )
	{

		printf("Not get gateway,so guass\n");
		
		strcpy(gateway,ip);
		
		j = strlen(gateway);

		for( i = j; i > 0;i--)
		{
			if( gateway[i] == '.' )
			{
				gateway[i+1] = '1';
				gateway[i+2] = '\0';
				break;
			}
		}
	}else
	{
		printf("Get gateway %s\n",gateway);
	}	

	if (iIsMulInterface == 1)
		rel = Ioctl_get_ip_netmask(netmsk, "ra0");
	else if (iIsMulInterface == 2)
		rel = Ioctl_get_ip_netmask(netmsk, "br0");
	else
		rel = Ioctl_get_ip_netmask(netmsk, "eth0");
	rel = write_dns_file();
	if( rel < 0)		
		goto GET_ERROR;	

	return ALLRIGHT;

GET_ERROR:

	if( fp )
		fclose(fp);

	command_drv("killall udhcpc");

	sprintf(cmd,"rm -rf %s",DHCP_TXT);
	printf("start cmd : %s \n",cmd);
	rel = command_drv(cmd);

	return ERROR;
	
}


void get_server_port(int * server_port,int * http_port)
{
	*server_port = socket_ctrl.server_port;
	*http_port = socket_ctrl.http_port;
}

void get_server_upnp_port(int * external_port)
{
	*external_port = socket_ctrl.server_upnp.external_port;
	
}



pthread_mutex_t upnp_mutex;
#define MAX_UPNP_TMP_BUF_SIZE  (10000)


void del_random_upnp_port(char * buf,int max_length)
{
	int i = 0,j = 0,h=0;
	char tmp[1000] = {0};
	int pos = 0;
	int ret;
	int len;
	char tmp_str[5][100] = {0};
	int external_port;
	
	for( i = 0; i  < 100; i++)
	{		
		j = FindSTR(buf,"UDP",pos);
		if( j > 0 )
		{
			pos = j + strlen("UDP");
		}else
			goto tcp_start;
			

		h = FindSTR(buf,"\'\'",pos);
		if( h > 0 )
		{
			pos = j + strlen("\'\'");
		}else
			goto tcp_start;

		ret = GetTmpLink(buf,tmp,j-1 ,h-1,1000);
		if( ret < 0)		
		{
			DPRINTK("get list info err\n");
			return ;
		}		

		DPRINTK("get line: %s \n",tmp);
		
		if( (FindSTR(tmp,"_YUN",0) < 0) 
			&& ( FindSTR(tmp,"_APP",0) < 0 ) )
		{
			sscanf(tmp,"%s %s %s",tmp_str[0],tmp_str[1],tmp_str[2]);

			len = strlen(tmp_str[1]);
			for( i = 0;i < len;i++)
			{
				if( tmp_str[1][i] == '-' )
				{
					tmp_str[1][i] = 0;
					break;
				}
			}
			external_port = atoi(tmp_str[1]);

			DPRINTK("get external_port=%d %s\n",external_port,tmp_str[0]);

			sprintf(tmp,"/mnt/mtd/upnpc-static -d %d UDP",external_port);

			DPRINTK("start cmd : %s \n",tmp);

			command_drv(tmp);		

			goto end;
			
		}		
	}

tcp_start:

	pos = 0;

	for( i = 0; i  < 100; i++)
	{		
		j = FindSTR(buf,"TCP",pos);
		if( j > 0 )
		{
			pos = j + strlen("TCP");
		}else
			goto end;
			

		h = FindSTR(buf,"\'\'",pos);
		if( h > 0 )
		{
			pos = j + strlen("\'\'");
		}else
			goto end;

		ret = GetTmpLink(buf,tmp,j -1,h-1,1000);
		if( ret < 0)		
		{
			DPRINTK("get list info err\n");
			return ;
		}		

		DPRINTK("get line: %s \n",tmp);
		
		if( (FindSTR(tmp,"_YUN",0) < 0) 
			&& ( FindSTR(tmp,"_APP",0) < 0 ) )
		{
			sscanf(tmp,"%s %s %s",tmp_str[0],tmp_str[1],tmp_str[2]);

			len = strlen(tmp_str[1]);
			for( i = 0;i < len;i++)
			{
				if( tmp_str[1][i] == '-' )
				{
					tmp_str[1][i] = 0;
					break;
				}
			}
			external_port = atoi(tmp_str[1]);

			DPRINTK("get external_port=%d %s\n",external_port,tmp_str[0]);

			sprintf(tmp,"/mnt/mtd/upnpc-static -d %d TCP",external_port);

			DPRINTK("start cmd : %s \n",tmp);

			command_drv(tmp);		

			goto end;
			
		}		
	}

end:
	return ;
}

int set_UPnP(NET_UPNP_ST_INFO * upnp)
{
	char cmd[MAX_UPNP_TMP_BUF_SIZE];
	char buf_tmp1[MAX_UPNP_TMP_BUF_SIZE];
	char tmp[500];
	int rel;
	int read_num =0;
	int read_num2 = 0;
	static int first = 0;	
	FILE *fp=NULL;
	int j = 0;
	int last_time_out = 0;	
	unsigned short server_port = 0;
	int external_port = 0;
	char ip[50];
	char name[50];
	int port;
	int is_yun_upnp = 0;

	strcpy(ip,upnp->internal_ip);
	strcpy(name,upnp->upnp_name);
	port = upnp->internal_port;

	j = FindSTR(name,"_YUN",0);
	if( j >= 0)	
		is_yun_upnp = 1;	

	if( first == 0 )
	{
		sprintf(cmd,"chmod 777 /mnt/mtd/upnpc-static");

		rel = command_drv("chmod 777 /mnt/mtd/upnpc-static");
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			return -1;
		}		
		
		first = 1;
	}

	command_drv("rm -rf /tmp/upnp_news_list");

	sprintf(cmd,"/mnt/mtd/upnpc-static -l  2>&1 > /tmp/upnp_news_list");

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		DPRINTK("%s error!\n",cmd);
		goto err;
	}

	while(1)
	{
		memset(cmd,0x00,MAX_UPNP_TMP_BUF_SIZE);	
		read_num = nvr_drv_get_file_data("/tmp/upnp_news_list",cmd,MAX_UPNP_TMP_BUF_SIZE);
		if( read_num < 0 )
		{
			//return -1;
		}	
		
		//DPRINTK("read %d\n",read_num);

		if( read_num >  0)
		{			
			//command_drv("cat /tmp/upnp_news_list");
			break;			
		}

		sleep(1);

		last_time_out++;		

		if( last_time_out > 30 )
		{
			DPRINTK("upnp time out!");
			goto err;
		}
	}

	j = FindSTR(cmd,"No IGD UPnP Device",0);		
	if( j >= 0 )
	{
		DPRINTK("no device\n");		
		goto err;
	}

	//此次upnp为 yun 端口映射的化，检测局域网内是否已经有转发设备了
	//如果有，则不再次转发。
	if( is_yun_upnp == 1 )
	{
		//找不到自己的设备号时。
		j = FindSTR(cmd,name,0);
		if( j <0)
		{
			DPRINTK("can't find %s\n",name);
			j = FindSTR(cmd,"_YUN",0);
			if( j >= 0)
			{ 
			//如果找到了其他的设备的yun端口映射，则不在进行映射。
				DPRINTK("find other yun dev\n");
				goto err;
			}
		}
	}
	

	sprintf(tmp,"cat /tmp/upnp_news_list | grep %s|grep -v grep > /tmp/%s",name,name);

	DPRINTK("start cmd : %s \n",tmp);

	rel = command_drv(tmp);
	if( rel < 0 )
	{
		DPRINTK("%s error!\n",tmp);		
	}

	sprintf(tmp,"/tmp/%s",name);

	memset(buf_tmp1,0x00,MAX_UPNP_TMP_BUF_SIZE);
	read_num = nvr_drv_get_file_data(tmp,buf_tmp1,MAX_UPNP_TMP_BUF_SIZE);
	if( read_num <= 0 )
	{		
		del_random_upnp_port(cmd,MAX_UPNP_TMP_BUF_SIZE);
	}else
	{
		char tmp_str[5][100] = {0};
		int i = 0;
		int len = 0;
		

		DPRINTK("port in list. %s\n",buf_tmp1);
		
		sscanf(buf_tmp1,"%s %s %s %s %s",tmp_str[0],tmp_str[1]
			,tmp_str[2],tmp_str[3],tmp_str[4]);

		DPRINTK("get %s\n",tmp_str[2]);

		len = strlen(tmp_str[2]);
		for( i = 0;i < len;i++)
		{
			if( tmp_str[2][i] == '-' )
			{
				tmp_str[2][i] = 0;
				break;
			}
		}

		external_port = atoi(tmp_str[2]);

		DPRINTK("get external_port=%d\n",external_port);	

		if( external_port == upnp->external_port )
			goto set_upnp;

		sprintf(cmd,"/mnt/mtd/upnpc-static -d %d TCP",external_port);

		DPRINTK("start cmd : %s \n",cmd);

		rel = command_drv(cmd);
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			goto err;
		}		
		
	}


try_get_external_port:	
	
	sprintf(tmp,"%d->",upnp->external_port);
	j = FindSTR(cmd,tmp,0);		
	if( j >= 0 )
	{
		DPRINTK("port %d is already in upnp list,change it\n",upnp->external_port);		
		upnp->external_port++;
		goto try_get_external_port;
	}		

set_upnp:		

	sprintf(cmd,"/mnt/mtd/upnpc-static -a %s %d %d TCP %s",ip,port,upnp->external_port,name);

	DPRINTK("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		DPRINTK("%s error!\n",cmd);
		goto err;
	}



	command_drv("rm -rf /tmp/upnp_news_list");

	sprintf(cmd,"/mnt/mtd/upnpc-static -l  2>&1 > /tmp/upnp_news_list");

	DPRINTK("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		DPRINTK("%s error!\n",cmd);
		goto err;
	}

	while(1)
	{

		memset(cmd,0x00,MAX_UPNP_TMP_BUF_SIZE);	
		read_num = nvr_drv_get_file_data("/tmp/upnp_news_list",cmd,MAX_UPNP_TMP_BUF_SIZE);
		if( read_num < 0 )
		{
			//return -1;
		}	
		
		//DPRINTK("read %d\n",read_num);

		if( read_num >  0)
		{			
			//command_drv("cat /tmp/upnp_news_list");
			break;			
		}

		sleep(1);

		last_time_out++;		

		if( last_time_out > 30 )
		{
			DPRINTK("upnp time out!");
			goto err;
		}
	}


	j = FindSTR(cmd,name,0);		
	if( j >= 0 )
	{
		upnp->upnp_flag = 1;		
	}else
	{
		
		goto err;
	}

	upnp->upnp_over = 1;

	DPRINTK("success\n");

	return 1;

err:
	upnp->upnp_flag = 0;
	upnp->upnp_over = 1;

	DPRINTK("err\n");
	return -1;
}

#define UPNP_COUNT_NUM (1800)
static int upnp_count = 0;


int Lock_Set_UPnP(NET_UPNP_ST_INFO * upnp,int right_now)
{
	int ret = 0;
	static int first = 0;	
	if( first == 0 )
	{
		pthread_mutex_init(&upnp_mutex,NULL);			
		first = 1;
	}	
	
	pthread_mutex_lock(&upnp_mutex);	
	
	if( right_now == 1 )
	{
		ret = set_UPnP(upnp);
		upnp_count = 1;
	}
	else
		upnp_count = UPNP_COUNT_NUM -5;
	
	pthread_mutex_unlock(&upnp_mutex);
	

	return ret;
}




int No_Use_Set_UPnP(char * ip,int port)
{
	DPRINTK("in func\n");
	return 1;
}

int Clear_UPnP(int port)
{
	char cmd[350];
	int rel;
	static int first = 0;

	return 1;

	if( first == 0 )
	{
		sprintf(cmd,"chmod 777 /mnt/mtd/upnpc-static");

		rel = command_drv("chmod 777 /mnt/mtd/upnpc-static");
		if( rel < 0 )
		{
			printf("%s error!\n",cmd);
			return -1;
		}
		first = 1;
	}

	sprintf(cmd,"/mnt/mtd/upnpc-static -d %d TCP",port);

	printf("start cmd : %s \n",cmd);

	rel = command_drv(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return -1;
	}

	return 1;
}





int get_sensor_type()
{
	char cmd[350];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i,j;
	//int sensor = SONY_IMX274_LVDS_4K_30FPS;

	//return SONY_IMX274_LVDS_4K_30FPS;
	int sensor = SONY_IMX290_LVDS_2K_30FPS;

	return SONY_IMX290_LVDS_2K_30FPS;

retry:

	fp = fopen("/mnt/mtd/sensor_type.txt","r");
	if( fp == NULL )
	{
		DPRINTK(" /mnt/mtd/sensor_type.txt  err\n\n");
		system("echo \"imx323\" > /mnt/mtd/sensor_type.txt");
		sleep(1);
		goto retry;
		exit(0);
		goto GET_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto GET_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto GET_ERROR;
	}

	printf(" fileOffset = %ld\n",fileOffset);

	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 2 && fileOffset < 340)
	{	
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto GET_ERROR;
		}	

		cmd[fileOffset] = '\0';		

		j = FindSTR(cmd,"ar0130",0);		
		if( j >= 0)	
		{
			sensor = APTINA_AR0130_DC_720P_30FPS;
			goto GET_OK;
		}

		j = FindSTR(cmd,"9m034",0);		
		if( j >= 0)	
		{
			sensor = APTINA_9M034_DC_720P_30FPS;
			goto GET_OK;
		}

		j = FindSTR(cmd,"ov9712",0);		
		if( j >= 0)	
		{
			sensor = OMNI_OV9712_DC_720P_30FPS;
			goto GET_OK;
		}

		j = FindSTR(cmd,"hm1375",0);		
		if( j >= 0)	
		{
			sensor = HIMAX_1375_DC_720P_30FPS;
			goto GET_OK;
		}

		j = FindSTR(cmd,"mt9p006",0);		
		if( j >= 0)	
		{
			sensor = APTINA_MT9P006_DC_1080P_30FPS;
			goto GET_OK;
		}	

		j = FindSTR(cmd,"imx122",0);		
		if( j >= 0)	
		{
			sensor = SONY_IMX122_DC_1080P_30FPS;
			goto GET_OK;
		}	

		j = FindSTR(cmd,"imx138",0);		
		if( j >= 0)	
		{
			sensor = SONY_IMX138_DC_720P_30FPS;
			goto GET_OK;
		}	

		j = FindSTR(cmd,"ov2710",0);		
		if( j >= 0)	
		{
			sensor = OMNI_OV2710_DC_1080P_30FPS;
			goto GET_OK;
		}	

		j = FindSTR(cmd,"sc1035",0);		
		if( j >= 0)	
		{
			sensor = GOKE_SC1035;
			goto GET_OK;
		}	

		j = FindSTR(cmd,"jxh42",0);		
		if( j >= 0)	
		{
			sensor = GOKE_JXH42;
			goto GET_OK;
		}

		j = FindSTR(cmd,"sc2135",0);		
		if( j >= 0)	
		{
			sensor = GOKE_SC2135;
			goto GET_OK;
		}

		j = FindSTR(cmd,"sc1145",0);		
		if( j >= 0)	
		{
			sensor = GOKE_SC1145;
			goto GET_OK;
		}

		j = FindSTR(cmd,"sc1135",0);		
		if( j >= 0)	
		{
			sensor = GOKE_SC1135;
			goto GET_OK;
		}

		j = FindSTR(cmd,"ov9750",0);		
		if( j >= 0)	
		{
			sensor = GOKE_OV9750;
			goto GET_OK;
		}

		j = FindSTR(cmd,"imx323",0);		
		if( j >= 0)	
		{
			sensor = HI3516E_IMX323;
			goto GET_OK;
		}

		
	}else
	{
		
		printf(" fileOffset = %ld error\n",fileOffset);
		goto GET_ERROR;	
	}

GET_OK:

	fclose(fp);
	fp = NULL;


	return sensor;

GET_ERROR:

	if( fp )
		fclose(fp);

	return ERROR;
	
}



int get_chip_type()
{
	char cmd[350];
	FILE *fp=NULL;
	int fileOffset = 0;
	int rel;
	int i,j;
	int chip_type = chip_hi3519v101;

/*
	fileOffset = nvr_drv_get_file_data("/mnt/chip_info.txt",cmd,350);
	if( fileOffset <= 0 )
	{
		DPRINTK("/mnt/chip_info.txt err\n\n");		
		goto GET_ERROR;
	}


	if( fileOffset > 2 && fileOffset < 340)
	{			
		j = FindSTR(cmd,"hi3518e",0);		
		if( j >= 0)	
		{
			chip_type = chip_hi3518e;
			goto GET_OK;
		}	

		
		
	}else
	{	
		goto GET_ERROR;	
	}*/

GET_OK:	
	return chip_type;

GET_ERROR:

	return ERROR;
	
}


void upnp_thread()
{
	SET_PTHREAD_NAME(NULL);
	
	while(1)
	{
		//DPRINTK("enable_upnp=%d count=%d %s:%d\n",enable_upnp,upnp_count,upnp_ip,upnp_port);
		
		upnp_count++;
		
		sleep(1);			

		if( upnp_count % UPNP_COUNT_NUM == 0 )	
		{	
			
			if( socket_ctrl.server_upnp.upnp_name[0] != 0 )
				Lock_Set_UPnP(&socket_ctrl.server_upnp,1);				
			
			
			if( socket_ctrl.yun_upnp.upnp_name[0] != 0 )
				Lock_Set_UPnP(&socket_ctrl.yun_upnp,1);				
		}
	}
}


NET_UPNP_ST_INFO * net_get_server_info()
{
	return &socket_ctrl.server_upnp;
}

NET_UPNP_ST_INFO * net_get_yun_server_info()
{
	return &socket_ctrl.yun_upnp;
}


void net_client_connnet_check_func()
{
	
	static int net_client_send_recv_num[MAXCLIENT] ={ 0};
	static int net_client_same_num[MAXCLIENT] = {0};
	int i;

	//DPRINTK("check\n");
	for( i = 0; i <MAXCLIENT;i++)
	{
		if( socket_ctrl.clientfd[i] > 0 )
		{
			if( net_client_send_recv_num[i] !=
				( socket_ctrl.socket_send_num[i] + socket_ctrl.socket_recv_num[i]) )
			{
				net_client_send_recv_num[i] =  socket_ctrl.socket_send_num[i] + socket_ctrl.socket_recv_num[i];
				net_client_same_num[i] = 0;
			}else
			{				
				net_client_same_num[i]++;

				DPRINTK("num is same: %d %d %d\n",
					net_client_send_recv_num[i],
					socket_ctrl.socket_send_num[i] + socket_ctrl.socket_recv_num[i],
					net_client_same_num[i]);

				if(net_client_same_num[i] >= 2 )
				{
					socket_ctrl.close_all_socket = 9999;
					net_client_same_num[i] = 0;
					net_client_send_recv_num[i]  = 0;
				}
			}
		}
	}

	
}

void Nvr_get_host_ip(char * ip)
{
	strcpy(ip,socket_ctrl.set_ip);
}


static int netstat_value_array[NETSTAT_ARRAY_NUM];
static int netstat_array_pos = 0;

void insert_netstat_array(int value)
{
	netstat_value_array[netstat_array_pos] = value;
	netstat_array_pos++;

	if( netstat_array_pos >= NETSTAT_ARRAY_NUM)
		netstat_array_pos = 0;
}



int get_netstat_result()
{
	int big_value_num =0;
	int small_value_num = 0;
	int i = 0;
	int value = 0;
	static int show_count = 0;
	
	for( i = 0; i < NETSTAT_ARRAY_NUM; i++)
	{
		if( netstat_value_array[i] > 10000)
			big_value_num++;
		else if( netstat_value_array[i] < 6000)
			small_value_num++;
		else
		{
		}		
	}

	show_count++;	
	if( show_count % 5 == 0 )
	{
		printf("\n netstat array: ");
		for( i = 0; i < NETSTAT_ARRAY_NUM; i++)
		{			
			printf("%d ",netstat_value_array[i]);
		}

		printf("\n");
	}	


	if( big_value_num >= 6 )
		value = 1;

	if( small_value_num >= 8 )
		value = -1;

	
	//DPRINTK("big_value_num=%d small_value_num=%d  value=%d\n",\
	//	big_value_num,small_value_num,value);

	return value;
}

int check_Wifi_Status()
{
	char cmd[100];
	int rel = 0, read_num = 0;
	rel = command_drv("iwpriv ra0 connStatus | grep 'Connected' > /tmp/wifi_7601.txt");
	if( rel < 0 )
	{
		DPRINTK("iwpriv ra0 connStatus | grep 'Connected' > /tmp/wifi_7601.txt error!\n");
		return 0;
	}

	return 1;
}

int kill_Wpa_Supplicant()
{
	int rel = 0;
	rel = command_drv("killall wpa_supplicant");
	if( rel < 0 )
	{
		DPRINTK("killall wpa_supplicant error!\n");
		return 0;
	}

	rel = command_drv("ifconfig ra0 up");
	if( rel < 0 )
	{
		DPRINTK("kill_Wpa_Supplicant - ifconfig ra0 up error!\n");
		return 0;
	}

	return 1;
}

int start_Wifi_Connection()
{
	int rel = 0;

	rel = command_drv("ifconfig ra0 up");
	if( rel < 0 )
	{
		DPRINTK("start_Wifi_Connection - ifconfig ra0 up error!\n");
		return 0;
	}

	rel = command_drv("wpa_supplicant -Dwext -ira0 -c /mnt/mtd/pc/wifi_wpa.conf -d &");
	if( rel < 0 )
	{
		DPRINTK("wpa_supplicant -Dwext -ira0 -c /mnt/mtd/pc/wifi_wpa.conf -d & error!\n");
		return 0;
	}

	return 1;
}
