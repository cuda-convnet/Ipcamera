#include "stdio.h"
#include "NetModule.h"
#include "nvr_ipcam_protocol.h"
#include "netserver.h"


int NIP_nvr_send_command_convert(char * dest_buf,int dest_length,char * source_buf,int source_length)
{

	NET_DATA_ST st_data;
	NET_COMMAND g_cmd;
	NVR_COMMAND_ST cmd_st;
	int change_data = 0;
	int send_len = 0;
	int live_cam = 0;

	if( dest_length < MAX_FRAME_BUF_SIZE )
	{
		DPRINTK("dest_length=%d < %d\n",dest_length,MAX_FRAME_BUF_SIZE);		
			goto NIP_error;
	}

	if( dest_length < sizeof(NET_DATA_ST) || source_length <  sizeof(NET_DATA_ST))
	{
		DPRINTK("dest_length=%d source_length=%d sizeof(NET_DATA_ST)=%d\n",
			dest_length,source_length,sizeof(NET_DATA_ST));
			goto NIP_error;
	}

	if( dest_buf == NULL || source_buf == NULL )
	{
		DPRINTK("dest_buf =%x source_buf=%x\n",dest_buf,source_buf);			
			goto NIP_error;
	}

	memcpy(&st_data,source_buf,sizeof(NET_DATA_ST));

	if( st_data.cmd == NET_DVR_NET_COMMAND )
	{
		memcpy(&g_cmd,source_buf + sizeof(NET_DATA_ST),sizeof(NET_COMMAND));

		switch(g_cmd.command)
		{
			case NET_CAM_ENC_SET:
				cmd_st.cmd = NET_CAM_ENC_SET;
				cmd_st.value[0] = g_cmd.length; //通道   0:是指 720p或1080p的压缩引擎. 1:是指 cif 压缩引擎 2:是指 cif 网络压缩引擎。
				cmd_st.value[1] = g_cmd.page % 100;  //帧率
				cmd_st.value[2] = g_cmd.play_fileId;  //码流
				cmd_st.value[3] = g_cmd.page/100;  //gop	

				//st_data.cmd = NET_DVR_NET_COMMAND;
				st_data.data_length = sizeof(NVR_COMMAND_ST);

				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;

				change_data = 1;
				break;
			case NET_CAM_SEND_DATA:
				cmd_st.cmd = NET_CAM_SEND_DATA;
				cmd_st.value[0] = g_cmd.length; 				

				//st_data.cmd = NET_DVR_NET_COMMAND;
				st_data.data_length = sizeof(NVR_COMMAND_ST);

				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;

				change_data = 1;
				break;
			case NET_CAM_GET_ENGINE_INFO:
				memcpy(&cmd_st,source_buf + sizeof(NET_DATA_ST) + sizeof(NET_COMMAND),
					sizeof(NVR_COMMAND_ST));

				st_data.data_length = sizeof(NVR_COMMAND_ST);				
				
				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;
			
				change_data = 1;
				break;
			default:break;
		}	
				
	}

	if( st_data.cmd == NET_DVR_NET_FRAME )
	{
		BUF_FRAMEHEADER old_header;
		NVR_FRAME_HEADER nvr_header;		
		memcpy(&old_header,source_buf+ sizeof(NET_DATA_ST),sizeof(BUF_FRAMEHEADER));
		
		nvr_header.channel  = old_header.index;
		nvr_header.iFrameType = old_header.iFrameType;
		nvr_header.iLength = old_header.iLength;
		nvr_header.frame_num =old_header.frame_num;
		nvr_header.type = old_header.type;
		nvr_header.ulTimeSec = old_header.ulTimeSec ;
		nvr_header.ulTimeUsec = old_header.ulTimeUsec;		


		if( old_header.type == VIDEO_FRAME )
		{
			if( NIP_frame_convert_to_w_h(old_header.standard,old_header.size,&nvr_header.pic_width,&nvr_header.pic_height) <0 )
			{
				DPRINTK("NIP_frame_convert_to_w_h   (%d,%d ) error\n",old_header.standard,old_header.size);
				goto NIP_error;
			}
		}

		st_data.data_length = sizeof(NVR_FRAME_HEADER) + nvr_header.iLength;
		memcpy(dest_buf,&st_data,sizeof(st_data));
		memcpy(&dest_buf[sizeof(st_data)],&nvr_header,sizeof(NVR_FRAME_HEADER));
		memcpy(dest_buf + sizeof(st_data) + sizeof(NVR_FRAME_HEADER),
			source_buf+ sizeof(NET_DATA_ST) + sizeof(BUF_FRAMEHEADER),nvr_header.iLength);
		send_len = sizeof(st_data) + st_data.data_length;			
		
		change_data = 1;
	}

	if( change_data == 1 )
		return send_len;
	
	return 0;

NIP_error:


	return -1;
}

int NIP_frame_convert(unsigned long * standard,unsigned long* image_size,unsigned long pic_width,unsigned long pic_height)
{
	int change_data = 0;
	
	if( pic_width == CAP_WIDTH_CIF && pic_height == CAP_HEIGHT_CIF_PAL)
	{
		*standard = TD_DRV_VIDEO_STARDARD_PAL;
		*image_size = TD_DRV_VIDEO_SIZE_CIF;
		change_data = 1;
	}

	if( pic_width == CAP_WIDTH_CIF && pic_height == CAP_HEIGHT_CIF_NTSC)
	{
		*standard = TD_DRV_VIDEO_STARDARD_NTSC;
		*image_size = TD_DRV_VIDEO_SIZE_CIF;
		change_data = 1;
	}

	if( pic_width == IMG_720P_WIDTH  && pic_height == IMG_720P_HEIGHT)
	{
		*standard = get_cur_standard();
		*image_size = TD_DRV_VIDEO_SIZE_720P;
		change_data = 1;
	}

	if( pic_width == IMG_480P_WIDTH  && pic_height == IMG_480P_HEIGHT)
	{
		*standard = get_cur_standard();
		*image_size = TD_DRV_VIDEO_SIZE_480P;
		change_data = 1;
	}

	if( pic_width == IMG_1080P_WIDTH  && pic_height == IMG_1080P_HEIGHT)
	{
		*standard = get_cur_standard();
		*image_size = TD_DRV_VIDEO_SIZE_1080P;
		change_data = 1;
	}

	if( pic_width == IMG_1080P_WIDTH  && pic_height == 1080)
	{
		*standard = get_cur_standard();
		*image_size = TD_DRV_VIDEO_SIZE_1080P;
		change_data = 1;
	}


	if( change_data == 1 )
		return 1;

	return -1;
NIP_error:
	return -1;
}


int NIP_nvr_recv_command_convert(char * dest_buf,int dest_length,char * source_buf,int source_length,int cmd)
{

	NET_DATA_ST st_data;
	NET_COMMAND g_cmd;
	NVR_COMMAND_ST cmd_st;
	int change_data = 0;
	int send_len = 0;
	int live_cam = 0;

	if( cmd == NET_DVR_NET_SELF_DATA )
		return 0;

	if( dest_length < MAX_FRAME_BUF_SIZE )
	{
		DPRINTK("dest_length=%d < %d\n",dest_length,MAX_FRAME_BUF_SIZE);		
			goto NIP_error;
	}

	if( dest_length < sizeof(NET_DATA_ST) || source_length <  sizeof(NET_DATA_ST))
	{
		DPRINTK("dest_length=%d source_length=%d sizeof(NET_DATA_ST)=%d\n",
			dest_length,source_length,sizeof(NET_DATA_ST));
			goto NIP_error;
	}

	if( dest_buf == NULL || source_buf == NULL )
	{
		DPRINTK("dest_buf =%x source_buf=%x\n",dest_buf,source_buf);			
			goto NIP_error;
	}	

	if( cmd == NET_DVR_NET_COMMAND )
	{
		memcpy(&cmd_st,source_buf,sizeof(NVR_COMMAND_ST));

		switch(cmd_st.cmd)
		{		
			case NET_CAM_GET_ENGINE_INFO:
				DPRINTK("NET_CAM_GET_ENGINE_INFO\n");
				g_cmd.command= NET_CAM_GET_ENGINE_INFO;
				g_cmd.length = 0;	
			
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));	
				send_len = sizeof(NET_COMMAND);			
				change_data = 1;
				break;
			case NET_CAM_ENC_SET:				
				g_cmd.command= NET_CAM_ENC_SET;
				g_cmd.length = cmd_st.value[0];
				g_cmd.page = cmd_st.value[3]*100 + cmd_st.value[1]%100;
				if( cmd_st.value[2] < 20*1024 )
					g_cmd.play_fileId = cmd_st.value[2];	
				else					
					g_cmd.play_fileId = cmd_st.value[2] / 1024;	

				DPRINTK("NET_CAM_ENC_SET (%d,%d,%d,%d)\n",
					cmd_st.value[0],cmd_st.value[1],cmd_st.value[2],cmd_st.value[3]);
				
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));				
				send_len = sizeof(g_cmd);			
				change_data = 1;
				break;
			case NET_CAM_SEND_DATA:
				DPRINTK("NET_CAM_SEND_DATA (%x)\n",cmd_st.value[0]);
				g_cmd.command= NET_CAM_SEND_DATA;
				g_cmd.length = cmd_st.value[0];	
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));				
				send_len = sizeof(g_cmd);			
				change_data = 1;
				break;		
			case NET_CAM_NET_ALIVE:
				DPRINTK("NET_CAM_NET_ALIVE \n");
			default:break;
		}	
				
	}

	if( cmd == NET_DVR_NET_FRAME )
	{
		BUF_FRAMEHEADER old_header;
		NVR_FRAME_HEADER nvr_header;		

		memcpy(&nvr_header,source_buf,sizeof(NVR_FRAME_HEADER));


/*
//		switch(Net_dvr_get_support_max_video())
		switch(TD_DRV_VIDEO_SIZE_720P)
		{			
			case TD_DRV_VIDEO_SIZE_720P:	
				if( nvr_header.channel == 0 )
				{
					if( nvr_header.pic_width != IMG_720P_WIDTH || nvr_header.pic_height != IMG_720P_HEIGHT)
					{
						DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
						goto NIP_error;
					}
				}

				if( nvr_header.channel == 1 || nvr_header.channel == 2 )
				{
					if( nvr_header.pic_width != CAP_WIDTH_CIF)
					{
						DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
						goto NIP_error;
					}
				}
				
				break;
			case TD_DRV_VIDEO_SIZE_1080P:
				if( nvr_header.channel == 0 )
				{
					if( nvr_header.pic_width != IMG_1080P_WIDTH || nvr_header.pic_height != IMG_1080P_HEIGHT)
					{
						if( nvr_header.pic_width != IMG_1080P_WIDTH || nvr_header.pic_height != 1080)
						{
							DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
							goto NIP_error;
						}
					}
				}

				if( nvr_header.channel == 1 || nvr_header.channel == 2 )
				{
					if( nvr_header.pic_width != CAP_WIDTH_CIF)
					{
						DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
						goto NIP_error;
					}
				}
				
				break;
			case TD_DRV_VIDEO_SIZE_480P:
				if( nvr_header.channel == 0 )
				{
					if( nvr_header.pic_width != IMG_480P_WIDTH || nvr_header.pic_height != IMG_480P_HEIGHT)
					{						
						DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
						goto NIP_error;						
					}
				}

				if( nvr_header.channel == 1 || nvr_header.channel == 2 )
				{
					if( nvr_header.pic_width != CAP_WIDTH_CIF)
					{
						DPRINTK("pic_size no surpport  chan=%d  (%d,%d ) error\n",nvr_header.channel ,nvr_header.pic_width,nvr_header.pic_height);
						goto NIP_error;
					}
				}
				
				break;
			default:
				DPRINTK("nvr Net_dvr_get_support_max_video = %d error\n",
					Net_dvr_get_support_max_video());
				break;
		}
*/
		old_header.index = nvr_header.channel ;
		old_header.iFrameType = nvr_header.iFrameType;
		old_header.iLength = nvr_header.iLength;
		old_header.frame_num = nvr_header.frame_num;
		old_header.type = nvr_header.type;
		old_header.ulTimeSec = nvr_header.ulTimeSec;
		old_header.ulTimeUsec = nvr_header.ulTimeUsec;

		if( nvr_header.type == VIDEO_FRAME )
		{
			if( NIP_frame_convert(&old_header.standard,&old_header.size,nvr_header.pic_width,nvr_header.pic_height) <0 )
			{
				DPRINTK("NIP_frame_convert   (%d,%d ) error\n",nvr_header.pic_width,nvr_header.pic_height);
				goto NIP_error;
			}
		}
		
		memcpy(dest_buf,&old_header,sizeof(old_header));
		memcpy(&dest_buf[sizeof(old_header)],source_buf+sizeof(NVR_FRAME_HEADER),old_header.iLength);
		send_len = sizeof(old_header) + old_header.iLength;			
		change_data = 1;			
		
	}

	if( change_data == 1 )
		return send_len;
	
	return 0;

NIP_error:


	return -1;
}


int NIP_ipcma_recv_command_convert(char * dest_buf,int dest_length,char * source_buf,int source_length,int cmd)
{

	NET_DATA_ST st_data;
	NET_COMMAND g_cmd;
	NVR_COMMAND_ST cmd_st;
	int change_data = 0;
	int send_len = 0;
	int live_cam = 0;

	if( cmd == NET_DVR_NET_SELF_DATA )
		return 0;

	if( dest_length < MAX_FRAME_BUF_SIZE )
	{
		DPRINTK("dest_length=%d < %d\n",dest_length,MAX_FRAME_BUF_SIZE);		
			goto NIP_error;
	}

	if( dest_length < sizeof(NET_DATA_ST) || source_length <  sizeof(NET_DATA_ST))
	{
		DPRINTK("dest_length=%d source_length=%d sizeof(NET_DATA_ST)=%d cmd=%d\n",
			dest_length,source_length,sizeof(NET_DATA_ST),cmd);
			goto NIP_error;
	}

	if( dest_buf == NULL || source_buf == NULL )
	{
		DPRINTK("dest_buf =%x source_buf=%x\n",dest_buf,source_buf);			
			goto NIP_error;
	}	

	if( cmd == NET_DVR_NET_COMMAND )
	{
		memcpy(&cmd_st,source_buf,sizeof(NVR_COMMAND_ST));

		switch(cmd_st.cmd)
		{		
			case NET_CAM_GET_ENGINE_INFO:
				DPRINTK("NET_CAM_GET_ENGINE_INFO\n");
				g_cmd.command= NET_CAM_GET_ENGINE_INFO;
				g_cmd.length = 0;	
			
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));	
				send_len = sizeof(NET_COMMAND);			
				change_data = 1;
				break;
			case NET_CAM_ENC_SET:				
				g_cmd.command= NET_CAM_ENC_SET;
				g_cmd.length = cmd_st.value[0];
				g_cmd.page = cmd_st.value[3]*100 + cmd_st.value[1]%100;
				if( cmd_st.value[2] < 20*1024 )
					g_cmd.play_fileId = cmd_st.value[2];	
				else					
					g_cmd.play_fileId = cmd_st.value[2] / 1024;	

				DPRINTK("NET_CAM_ENC_SET (%d,%d,%d,%d)\n",
					cmd_st.value[0],cmd_st.value[1],cmd_st.value[2],cmd_st.value[3]);
				
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));				
				send_len = sizeof(g_cmd);			
				change_data = 1;
				break;
			case NET_CAM_SEND_DATA:
				DPRINTK("NET_CAM_SEND_DATA (%x)\n",cmd_st.value[0]);
				g_cmd.command= NET_CAM_SEND_DATA;
				g_cmd.length = cmd_st.value[0];	
				memcpy(dest_buf,&g_cmd,sizeof(g_cmd));				
				send_len = sizeof(g_cmd);			
				change_data = 1;
				break;		
			case NET_CAM_NET_ALIVE:
				DPRINTK("NET_CAM_NET_ALIVE \n");
			default:break;
		}	
				
	}

	

	if( change_data == 1 )
		return send_len;
	
	return 0;

NIP_error:


	return -1;
}


int NIP_frame_convert_to_w_h(unsigned long  standard,unsigned long image_size,unsigned long *pic_width,unsigned long *pic_height)
{
	int change_data = 0;

	if( standard == TD_DRV_VIDEO_STARDARD_PAL && image_size == TD_DRV_VIDEO_SIZE_CIF)
	{
		*pic_width = CAP_WIDTH_CIF;
		*pic_height = CAP_HEIGHT_CIF_PAL;
		change_data = 1;
	}

	if( standard == TD_DRV_VIDEO_STARDARD_NTSC && image_size == TD_DRV_VIDEO_SIZE_CIF)
	{
		*pic_width = CAP_WIDTH_CIF;
		*pic_height = CAP_HEIGHT_CIF_NTSC;
		change_data = 1;
	}

	if( image_size == TD_DRV_VIDEO_SIZE_480P)
	{
		*pic_width = IMG_480P_WIDTH;
		*pic_height = IMG_480P_HEIGHT;
		change_data = 1;
	}

	if( image_size == TD_DRV_VIDEO_SIZE_720P)
	{
		*pic_width = IMG_720P_WIDTH;
		*pic_height = IMG_720P_HEIGHT;
		change_data = 1;
	}

	if( image_size == TD_DRV_VIDEO_SIZE_1080P)
	{
		*pic_width = IMG_1080P_WIDTH;
		*pic_height = IMG_1080P_HEIGHT;
		change_data = 1;
	}

	if(change_data == 0 )
	{
		*pic_width = CAP_WIDTH_CIF;
		*pic_height = CAP_HEIGHT_CIF_NTSC;
		change_data = 1;
	}
	

	if( change_data == 1 )
		return 1;

	return -1;
NIP_error:
	return -1;
}


int NIP_ipcam_send_command_convert(char * dest_buf,int dest_length,char * source_buf,int source_length)
{

	NET_DATA_ST st_data;
	NET_COMMAND g_cmd;
	NVR_COMMAND_ST cmd_st;
	int change_data = 0;
	int send_len = 0;
	int live_cam = 0;	

	if( dest_length < MAX_FRAME_BUF_SIZE )
	{
		DPRINTK("dest_length=%d < %d\n",dest_length,MAX_FRAME_BUF_SIZE);		
			goto NIP_error;
	}

	if( dest_length < sizeof(NET_DATA_ST) || source_length <  sizeof(NET_DATA_ST))
	{
		DPRINTK("dest_length=%d source_length=%d sizeof(NET_DATA_ST)=%d\n",
			dest_length,source_length,sizeof(NET_DATA_ST));
			goto NIP_error;
	}

	if( dest_buf == NULL || source_buf == NULL )
	{
		DPRINTK("dest_buf =%x source_buf=%x\n",dest_buf,source_buf);			
			goto NIP_error;
	}

	memcpy(&st_data,source_buf,sizeof(NET_DATA_ST));

	if( st_data.cmd == NET_DVR_NET_COMMAND )
	{
		memcpy(&g_cmd,source_buf + sizeof(NET_DATA_ST),sizeof(NET_COMMAND));

		switch(g_cmd.command)
		{
			case NET_CAM_ENC_SET:
				cmd_st.cmd = NET_CAM_ENC_SET;
				cmd_st.value[0] = g_cmd.length; //通道   0:是指 720p或1080p的压缩引擎. 1:是指 cif 压缩引擎 2:是指 cif 网络压缩引擎。
				cmd_st.value[1] = g_cmd.page % 100;  //帧率
				cmd_st.value[2] = g_cmd.play_fileId;  //码流
				cmd_st.value[3] = g_cmd.page/100;  //gop	

				//st_data.cmd = NET_DVR_NET_COMMAND;
				st_data.data_length = sizeof(NVR_COMMAND_ST);

				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;

				change_data = 1;
				break;
			case NET_CAM_SEND_DATA:
				cmd_st.cmd = NET_CAM_SEND_DATA;
				cmd_st.value[0] = g_cmd.length; 				

				//st_data.cmd = NET_DVR_NET_COMMAND;
				st_data.data_length = sizeof(NVR_COMMAND_ST);

				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;

				change_data = 1;
				break;
			case NET_CAM_GET_ENGINE_INFO:
				memcpy(&cmd_st,source_buf + sizeof(NET_DATA_ST) + sizeof(NET_COMMAND),
					sizeof(NVR_COMMAND_ST));

				st_data.data_length = sizeof(NVR_COMMAND_ST);				
				
				memcpy(dest_buf,&st_data,sizeof(st_data));
				memcpy(&dest_buf[sizeof(st_data)],&cmd_st,st_data.data_length);
				send_len = sizeof(st_data) + st_data.data_length;
			
				change_data = 1;
				break;
			default:break;
		}	
				
	}

	if( st_data.cmd == NET_DVR_NET_FRAME )
	{
		BUF_FRAMEHEADER old_header;
		NVR_FRAME_HEADER nvr_header;		
		memcpy(&old_header,source_buf+ sizeof(NET_DATA_ST),sizeof(BUF_FRAMEHEADER));
		
		nvr_header.channel  = old_header.index;
		nvr_header.iFrameType = old_header.iFrameType;
		nvr_header.iLength = old_header.iLength;
		nvr_header.frame_num =old_header.frame_num;
		nvr_header.type = old_header.type;
		nvr_header.ulTimeSec = old_header.ulTimeSec ;
		nvr_header.ulTimeUsec = old_header.ulTimeUsec;


		if( old_header.type == VIDEO_FRAME )
		{
			if( NIP_frame_convert_to_w_h(old_header.standard,old_header.size,&nvr_header.pic_width,&nvr_header.pic_height) <0 )
			{
				DPRINTK("NIP_frame_convert_to_w_h   (%d,%d ) error\n",old_header.standard,old_header.size);
				goto NIP_error;
			}
		}

		st_data.data_length = sizeof(NVR_FRAME_HEADER) + nvr_header.iLength;
		memcpy(dest_buf,&st_data,sizeof(st_data));
		memcpy(&dest_buf[sizeof(st_data)],&nvr_header,sizeof(NVR_FRAME_HEADER));
		memcpy(dest_buf + sizeof(st_data) + sizeof(NVR_FRAME_HEADER),
			source_buf+ sizeof(NET_DATA_ST) + sizeof(BUF_FRAMEHEADER),nvr_header.iLength);
		send_len = sizeof(st_data) + st_data.data_length;
		
		change_data = 1;
	}

	if( change_data == 1 )
		return send_len;
	
	return 0;

NIP_error:


	return -1;
}

