/**************************************************

  File Name     : GM_dev.c
  Version       : 1.0
  Author        : yb
  Created       : 2009/08/13
  Description   : 
  History       :

***************************************************/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "GM_dev.h"
#include "NetModule.h"
#include "P2pBaseDate.h"
#include "func_common.h"
#include "netserver.h"

#include "gk_dev.h"

int dvr_fd = 0;
int enc_fd = 0;

//test record
unsigned char *bs_buf;
HANDLE  rec_file;
int enc_buf_size;
int sub1_bs_buf_offset;
int sub2_bs_buf_offset;
int sub3_bs_buf_offset;
int sub4_bs_buf_offset;
int sub5_bs_buf_offset;
int bs_buf_snap_offset;
struct pollfd rec_fds[MN_MAX_BIND_ENC_NUM];
char file_name[128];

//sub record

int multi_bitstream = 0;

extern GM_ENCODE_ST gm_enc_st[MN_MAX_BIND_ENC_NUM];
extern NET_MODULE_ST * client_pst[MAX_CLIENT];
static int net_cam_send_video_frame = 0;
extern int printf_frame_info;

void Net_cam_set_send_video_frame()
{
	net_cam_send_video_frame++;

	if( net_cam_send_video_frame > 10000)
		net_cam_send_video_frame = 1;
}

int Net_cam_get_send_video_frame()
{
	return net_cam_send_video_frame;
}


int check_goke_is_work()
{
	//    return 1;
	if(net_cam_send_video_frame > 0)
		return 1;
	else 
		return 0;
}


void GM_set_video_mode(int mode)
{
	if(mode == TD_DRV_VIDEO_STARDARD_PAL)
	{
		SendM2("echo 1 > /proc/isp0/ae/pwr_freq");
		SendM2("echo w denoise 20 > /proc/isp0/command");
	}	
}

void GM_open_system()
{
	  dvr_fd = open("/dev/dvr_common", O_RDWR);   //open_dvr_common
}

void GM_close_system()
{
	close(dvr_fd);      //close_dvr_common
}

void GM_close_main_enc(GM_ENCODE_ST * enc_st)
{
	DPRINTK("in func\n");    
	return 1;
}




int GM_open_main_enc(GM_ENCODE_ST * enc_st)
{
	DPRINTK("in func\n");    
	return 1;
}


int GM_open_sub_enc(GM_ENCODE_ST * main_enc_st,GM_ENCODE_ST * sub_enc_st)
{
	DPRINTK("in func\n");    
	return 1;
}

void GM_net_encode_close()
{
	DPRINTK("in func\n");    
	return 1;
}



int GM_net_cam_start_enc_main_bs(GM_ENCODE_ST * enc_st,int real_start)
{
	DPRINTK("in func\n");    
	return 1;
}


int GM_net_cam_stop_enc_main_bs(GM_ENCODE_ST * enc_st,int real_stop)
{
	DPRINTK("in func\n");    
	return 1;
}




int GM_send_rec_data(int is_keyframe , char * video_stream,int video_size,int VeChn)
{
	BUF_FRAMEHEADER fheader;
	struct timeval recTime;
	char * my_buf = NULL;
	unsigned char * ptr_point = NULL;
	static int frame_num[16] = {0};
	static int lost_frame[16] = {0};

	my_buf = video_stream;

	if( get_cur_size() == TD_DRV_VIDEO_SIZE_D1 )
	{
		if( VeChn != 0 )
			return 1;
	}

	if( get_cur_size() == TD_DRV_VIDEO_SIZE_CIF)
	{
		if( VeChn != 1 )
			return 1;
	}


		if( video_size > (MAX_FRAME_BUF_SIZE - 5*1024))
		{
			DPRINTK("VeChn = %d frame size %d too bigger lost it\n",VeChn,video_size);		
			return -1;
		}

	
	    get_sys_time( &recTime, NULL );	
		
		ptr_point = my_buf + sizeof(BUF_FRAMEHEADER);
       
		fheader.iLength = video_size; 	
		fheader.iFrameType =is_keyframe;				
		fheader.index = VeChn;   
		fheader.type = VIDEO_FRAME;
		fheader.ulTimeSec = recTime.tv_sec;
		fheader.ulTimeUsec = recTime.tv_usec;
		fheader.standard = get_cur_standard();
		
		if( VeChn == 0 )				
			fheader.size = TD_DRV_VIDEO_SIZE_D1;		
		else
			fheader.size = TD_DRV_VIDEO_SIZE_CIF;				


		if( fheader.iFrameType == 1 )			
		{
			frame_num[VeChn] = 0;	
			lost_frame[VeChn] = 0;
		}
				
		fheader.frame_num = frame_num[VeChn];
		frame_num[VeChn]++;	

		// dvr 只有一个通道。
		fheader.index = 0;

		if( lost_frame[VeChn]  == 0 )
		{			

			if(  insert_enc_buf(&fheader,ptr_point) < 0 )		
			{
				DPRINTK("insert data error: chan=%ld length=%ld type=%ld num=%ld\n",
					fheader.index,fheader.iLength,fheader.iFrameType,fheader.frame_num);

				lost_frame[VeChn] = 1;

				return -1;
			} 
		}else
		{
			DPRINTK("chan=%d lost = %d framenum=%d\n",
				fheader.index,lost_frame[VeChn],fheader.frame_num );
		}

		return 1;
}




int GM_send_dvr_net_data(int is_keyframe , char * video_stream,int video_size,int VeChn)
{
	BUF_FRAMEHEADER fheader;
	struct timeval recTime;
	char * my_buf = NULL;
	unsigned char * ptr_point = NULL;
	static int frame_num[16] = {0};
	static int lost_frame[16] = {0};
	static int buf_enough[16] = {0};
	int net_chan = 0;

	my_buf = video_stream;
	
	//printf("GM_send_dvr_net_data\n");

	if( Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_D1 )	
		net_chan = 0;
	else
		net_chan = 1;
	
	if( VeChn != net_chan )
		return 1;

	if( video_size > (OLD_MAX_FRAME_BUF_SIZE - 5*1024))
	{
		DPRINTK("VeChn = %d frame size %d too bigger support512k=%d\n",
			VeChn,video_size,Net_cam_get_support_512K_buf_flag());
	}

	if(GK_VIDEO_DATA_OFFSET < sizeof(FRAMEHEADERINFO) +  sizeof(NET_COMMAND) )
	{
		DPRINTK("GK_VIDEO_DATA_OFFSET < sizeof(FRAMEHEADERINFO) +  sizeof(NET_COMMAND) error!\n");
		return 1;
	}


	get_sys_time( &recTime, NULL );	

	//ptr_point = my_buf + sizeof(BUF_FRAMEHEADER);
	ptr_point = my_buf + GK_VIDEO_DATA_OFFSET - sizeof(FRAMEHEADERINFO) - sizeof(NET_COMMAND);
   
	fheader.iLength = video_size; 	
	fheader.iFrameType =is_keyframe;				
	fheader.index = VeChn;   
	fheader.type = VIDEO_FRAME;
	fheader.ulTimeSec = recTime.tv_sec;
	fheader.ulTimeUsec = recTime.tv_usec;
	fheader.standard = get_cur_standard();
	
	if( VeChn == 0 )				
		fheader.size = TD_DRV_VIDEO_SIZE_D1;		
	else
		fheader.size = TD_DRV_VIDEO_SIZE_CIF;	

	if( fheader.iFrameType == 1 )			
	{
		frame_num[VeChn] = 0;	
		lost_frame[VeChn] = 0;
	}

	//DPRINTK("VeChn = %d length = %ld lost_frame =%d \n",VeChn,fheader.iLength,lost_frame[VeChn]);
				
	fheader.frame_num = frame_num[VeChn];
	frame_num[VeChn]++;	

	// dvr 只有一个通道。
	fheader.index = 0;

	if( lost_frame[VeChn]  == 0 )
	{	
		FRAMEHEADERINFO stFrameHeaderInfo;		
		int idx = 0;
		NETSOCKET * server_ctrl;	
		NET_COMMAND cmd;
		int buf_pos = 0;
		int ret;
		
		server_ctrl = (NETSOCKET *)net_get_server_ctrl();

		VeChn = 0;
				
		stFrameHeaderInfo.type       = 'V';				
		stFrameHeaderInfo.iChannel = VeChn;			
		stFrameHeaderInfo.ulDataLen = fheader.iLength;
		stFrameHeaderInfo.ulFrameType = fheader.iFrameType;
		stFrameHeaderInfo.ulTimeSec = recTime.tv_sec;
		stFrameHeaderInfo.ulTimeUsec = recTime.tv_usec;
		stFrameHeaderInfo.videoStandard = get_cur_standard();	
		stFrameHeaderInfo.imageSize	= get_cur_size();
		stFrameHeaderInfo.ulFrameNumber =	fheader.frame_num ;	




		//printf("type = %d len=%d number = %d\n",stFrameHeaderInfo.type,stFrameHeaderInfo.ulDataLen,stFrameHeaderInfo.ulFrameNumber);



		if( stFrameHeaderInfo.ulDataLen > NET_BUF_MAX_SIZE )
			DPRINTK("data larger is %ld\n",stFrameHeaderInfo.ulDataLen);				

		if( g_pstcommand->GST_FTC_get_play_status() == 0  )
		{
			if( Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_D1)
					stFrameHeaderInfo.imageSize	= TD_DRV_VIDEO_SIZE_D1;
			else
					stFrameHeaderInfo.imageSize	= TD_DRV_VIDEO_SIZE_CIF;
		}
		
			//printf("safe_socket_send_data\n");
		for( idx = 0; idx < MAXCLIENT; idx++ )
		{		 	
			if( server_ctrl->client_send_data[idx] == 0 )
				continue;	
			
			//printf("safe_socket_send_data2\n");
			server_ctrl->socket_send_num[idx]++;

			cmd.command = COMMAND_GET_FRAME;	
		
			server_ctrl->cur_client_idx = idx;

			buf_pos = 0;

			memcpy(ptr_point + buf_pos,(char*)&cmd, sizeof(NET_COMMAND));
			buf_pos += sizeof(NET_COMMAND);
			memcpy(ptr_point + buf_pos,(char*)&stFrameHeaderInfo, sizeof(FRAMEHEADERINFO));
			buf_pos += sizeof(FRAMEHEADERINFO);
		
			buf_pos += stFrameHeaderInfo.ulDataLen;


			
			ret = safe_socket_send_data(server_ctrl,idx, ptr_point, buf_pos);
			if ( ret <= 0 )
			{
				DPRINTK("1error %s %d\n", strerror(errno),errno);
				clear_client(idx,server_ctrl);				
			}				

			//DPRINTK("333send chan=%d lost = %d framenum=%d\n",
			//fheader.index,lost_frame[VeChn],fheader.frame_num );
		}
	
	}else
	{
		DPRINTK("lost frame chan=%d lost = %d framenum=%d\n",
			fheader.index,lost_frame[VeChn],fheader.frame_num );
	}

	return 1;
}


int GM_send_encode_data(NET_MODULE_ST * pst,int is_keyframe , char * video_stream,int video_size,int VeChn)
{
	BUF_FRAMEHEADER fheader;
	BUF_FRAMEHEADER * pHeader = NULL;
	//struct timeval recTime;
	char * my_buf = NULL;
	unsigned char * ptr_point = NULL;	

	/*
	if( video_size > (OLD_MAX_FRAME_BUF_SIZE - 5*1024))
	{
		DPRINTK("VeChn = %d frame size %d too bigger support512k=%d\n",
			VeChn,video_size,Net_cam_get_support_512K_buf_flag());
	}
	*/

	if( video_size > (MAX_FRAME_BUF_SIZE - 5*1024))
	{
		DPRINTK("VeChn = %d frame size %d too bigger lost it\n",VeChn,video_size);		
		pst->lost_frame[VeChn] = 1;
		return -1;
	}
	
	

	    	//get_sys_time( &recTime, NULL );	

		my_buf = video_stream;
		ptr_point = my_buf + sizeof(BUF_FRAMEHEADER);   

		

		fheader.iLength = video_size; 	
		fheader.iFrameType =is_keyframe;				
		fheader.index = VeChn;   
		fheader.type = VIDEO_FRAME;		
		fheader.standard = pst->video_mode;	

		pHeader = (BUF_FRAMEHEADER*)my_buf;
		fheader.ulTimeSec = pHeader->ulTimeSec;
		fheader.ulTimeUsec = pHeader->ulTimeUsec;

		//DPRINTK("[%d] %ld %ld\n",fheader.index,fheader.ulTimeSec,fheader.ulTimeUsec);
		
		if( VeChn == 0 )
		{
			if(Net_cam_get_support_max_video() == TD_DRV_VIDEO_SIZE_720P)
			{
				fheader.size = TD_DRV_VIDEO_SIZE_720P;
			}else if(Net_cam_get_support_max_video() == TD_DRV_VIDEO_SIZE_1080P)
			{
				fheader.size = TD_DRV_VIDEO_SIZE_1080P;
			}else if(Net_cam_get_support_max_video() == TD_DRV_VIDEO_SIZE_480P)
			{
				fheader.size = TD_DRV_VIDEO_SIZE_480P;
			}else
			{
				fheader.size = TD_DRV_VIDEO_SIZE_1080P;
			}
		}
		else
			fheader.size = TD_DRV_VIDEO_SIZE_CIF;				


		if( fheader.iFrameType == 1 )			
		{
			pst->frame_num[VeChn] = 0;	
			pst->lost_frame[VeChn] = 0;
		}
				
		fheader.frame_num = pst->frame_num[VeChn];
		pst->frame_num[VeChn]++;

		//处理16路视频时，切16分割，cif画面有时等得太久的问题。
		{
			if( (VeChn == 1 ) && (printf_frame_info > 0) )
			{
				DPRINTK("[%d]  is_keyframe=%d  %d\n",VeChn,fheader.iFrameType,fheader.frame_num);
				printf_frame_info--;

				//判断第一通道
				if( printf_frame_info % 5 == 0 )
				{
					GM_Net_intra_frame(&gm_enc_st[1]);
				}
			}
		}

		if( pst->lost_frame[VeChn]  == 0 )
		{		
			memcpy(my_buf,&fheader,sizeof(fheader));

			//if( fheader.index ==  0 )
			//DPRINTK("insert data : chan=%ld length=%ld type=%ld num=%ld\n",
			//		fheader.index,fheader.iLength,fheader.iFrameType,fheader.frame_num);

							
			if( NM_net_send_data(pst,my_buf,fheader.iLength + sizeof(BUF_FRAMEHEADER),NET_DVR_NET_FRAME) < 0 )
			{
				
				DPRINTK("insert data error: chan=%ld length=%ld type=%ld num=%ld key_frame_lost_num=%d\n",
					fheader.index,fheader.iLength,fheader.iFrameType,fheader.frame_num,pst->key_frame_lost_num);

				pst->lost_frame[VeChn] = 1;
				pst->key_frame_lost_num++;

				if( pst->key_frame_lost_num > 30 )
				{
					DPRINTK("key_frame_lost_num > 30,need reboot\n");
					command_drv("reboot");
				}
				return -1;
			} else
			{
				pst->key_frame_lost_num = 0;
			}
		}else
		{
			DPRINTK("chan=%d lost = %d framenum=%d\n",
				fheader.index,pst->lost_frame[VeChn],fheader.frame_num );
		}

		return 1;
}

//#define DVR_ENC_SWAP_INTRA                          _IOR(DVR_ENC_IOC_MAGIC, 18, int)
// DVR_ENC_RESET_INTRA
int GM_Net_intra_frame(GM_ENCODE_ST * enc_st)
{
	//DPRINTK("in func\n");    

	Hisi_create_IDR_2(0);	
	Hisi_create_IDR_2(1);
	
	return 1;
}




int  GM_Net_enc_set_main_bs(GM_ENCODE_ST * enc_st,int enable,int gop,int bit_rate,int frame_rate,int enc_width,int enc_height)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_net_cam_enc_set(int chan,int bit,int rate,int gop)
{
	int edit_rate = 0;
	int edit_bit = 0;
	
	if( chan < 0 || chan >= 3 )
	{	
		DPRINTK("chan %d error!\n");
		return -1;
	}
	
	return Hisi_chan_set_encode(chan,bit,rate,gop);
}

void GM_set_motion_alarm_md_num(GM_ENCODE_ST * enc_st,int md_num)
{
	DPRINTK("in func\n");    
	return 1;
}

int GM_get_motion_alarm(GM_ENCODE_ST * enc_st)
{
	DPRINTK("in func\n");    
	return 1;
}

int GM_get_motion_alarm_buf(GM_ENCODE_ST * enc_st,unsigned char * buf,int * mb_width,int *mb_height)
{
	DPRINTK("in func\n");    
	return 1;
}


int GM_init_audio()
{
	DPRINTK("in func\n");    
	return 1;
}

int GM_destroy_audio()
{
	DPRINTK("in func\n");    
	return 1;
}

int GM_read_audio_buf(unsigned char *buf,int dst_count)
{
	DPRINTK("in func\n");    
	return 1;
}


int GM_write_audio_buf(unsigned char *buf,int dst_count)
{
	DPRINTK("in func\n");    
	return 1;
}

////////////////////////////////////////////////
int disp_fd = 0;
int main_disp_no = 0;
int main_plane_id = 0;
int is_NTSC = 1;

int do_disp_startup()
{
	DPRINTK("in func\n");    
	return 1;
}




int run_lv_command(void)
{
	DPRINTK("in func\n");    
	return 1;
}



int do_liveview_t1()
{
	DPRINTK("in func\n");    
	return 1;
}


int do_liveview_close1ch(void)
{
	DPRINTK("in func\n");    
	return 1;
}


int do_disp_endup()
{
	DPRINTK("in func\n");    
	return 1;
}



int GM_CVBS_output(int video_in_mode )
{
	DPRINTK("in func\n");    
	return 1;
}

int GM_CVBS_CLOSE()
{
	DPRINTK("in func\n");    
	return 1;
}



