/**************************************************

  File Name     : Hisi_dev.c
  Version       : 1.0
  Author        : yb
  Created       : 2009/08/13
  Description   : 
  History       :

***************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>


#include "devfile.h"
#include "filesystem.h"
#include "Hisi_dev.h"
#include "func_common.h"
#include "NetModule.h"
#include "GM_dev.h"
#include "P2pBaseDate.h"
#include "config_file.h"
#include "ipcam_config.h"
#include "share_mem.h"
#include "string_op_func.h"
#include "singly_linked_list.h"
#include "fic8120ctrl.h"

#include "gk_dev.h"

#define SIO0_WORK_MODE   AIO_MODE_I2S_MASTER
#define SIO1_WORK_MODE   AIO_MODE_I2S_SLAVE


unsigned char		* net_multframe_in_one_buf = NULL;


unsigned char isday = 0;


#define DEBUGPOS printf("error file:%s,line:%d\r\n",__FILE__,__LINE__);

#define CHECK(express,name)\
    do{\
        if (1 != express)\
        {\
            printf("%s failed at %s: LINE: %d !\n", name, __FUNCTION__, __LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

int g_backlight_mode = 0;

static int IsLightDay = 1;
static int GammaMode = 0;


/* for vi config  (tw2815)                                                       */
static int fd2815a = -1;
static int fd2815b = -1;
static int is_open_vo = 0;
static int cam_mode = TD_DRV_VIDEO_STARDARD_PAL;
static int is_slave_chip = 0;
extern BUFLIST g_slavereclist;
extern BUFLIST g_slavenetlist;
extern BUFLIST g_encodeList;
extern BUFLIST g_audioList;
extern int slave_net_send_data;
extern int slave_rec_send_data;
static int net_use_mode = NET_SMALL_STREAM_MODE;

pthread_mutex_t mtd_store_mutex;
pthread_mutex_t hisi_control_mutex;
pthread_mutex_t hisi_window_vo_mutex;
static int lock_count=0;
static int encode_vbr = 0;
extern int code_h264_mode[3];  // 0 = VBR , 1 = CBR.
extern int code_h264_profile[3];
extern SINGLY_LINKED_LIST_INFO_ST gVideolist[MAX_VIDEO_STREAM_NUM];
extern SINGLY_LINKED_LIST_INFO_ST gAudiolist;


extern NET_MODULE_ST * client_pst[MAX_CLIENT];
extern MEM_MODULE_ST *pMemModule[MAX_CLIENT];
extern GM_ENCODE_ST gm_enc_st[MN_MAX_BIND_ENC_NUM];
extern ENCODE_ITEM enc_d1[16];
extern GST_DEV_FILE_PARAM * g_pstCommonParam;
extern int isp_init_ok;

static int g_isp_light_dark_value = 0;
static VBR_QUALITY_ST vbr_quality[MAX_VBR_QUALITY_ITEM_NUM] ={0};
static int default_sub_enc_resolution = 0; // 0 is  640x480 ,  1 is 320x240 .


int init_vbr_quality()
{
	int i;
	int j;
	int ret;
	char config_value[150] ="";
	char config_filename[100] = "/mnt/mtd/ipcam_enc_config.txt";

	ret= cf_read_value(config_filename,"sub_enc_resolution",config_value);
	if( ret >= 0)	
	{
		default_sub_enc_resolution = atoi(config_value);
	}	

	for( i = 0; i < MAX_VBR_QUALITY_ITEM_NUM; i++)
	{
				
		char parameter_name[50] = "";
		memset(&vbr_quality[i],0x00,sizeof(VBR_QUALITY_ST));

		sprintf(parameter_name,"u32Resolution_w_%d",i);		
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].w = atoi(config_value);
		}

		sprintf(parameter_name,"u32Resolution_h_%d",i);
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].h = atoi(config_value);
		}

		sprintf(parameter_name,"default_bitstream_%d",i);
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].default_bitstream = atoi(config_value);
		}

		sprintf(parameter_name,"default_framerate_%d",i);
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].default_framerate = atoi(config_value);
		}

		sprintf(parameter_name,"default_gop_%d",i);
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].default_gop = atoi(config_value);
		}

		sprintf(parameter_name,"default_H264_profile_%d",i);
		ret= cf_read_value(config_filename,parameter_name,config_value);
		if( ret >= 0)	
		{
			vbr_quality[i].default_H264_profile = atoi(config_value);
		}

		for( j = 0; j < MAX_VBR_QUALITY_LEVER; j++)
		{
			sprintf(parameter_name,"u32Bit_%d_%d",j,i);
			ret= cf_read_value(config_filename,parameter_name,config_value);
			if( ret >= 0)	
			{
				vbr_quality[i].bit[j]= atoi(config_value);
			}

			sprintf(parameter_name,"u32Quality_%d_%d",j,i);
			ret= cf_read_value(config_filename,parameter_name,config_value);
			if( ret >= 0)	
			{
				vbr_quality[i].quality[j] = atoi(config_value);
			}
		}		
	}

	return 1;
}

int get_default_sub_enc_resolution()
{
	return default_sub_enc_resolution;
}

int get_vbr_quality(int w,int h,int bitstream)
{
	int quality = 0;
	int i,j;

	for( i = 0; i < MAX_VBR_QUALITY_ITEM_NUM; i++)
	{
		if( w == vbr_quality[i].w  &&  h == vbr_quality[i].h )
		{
			for( j = MAX_VBR_QUALITY_LEVER - 1; j > 0; j--)
			{
				if( vbr_quality[i].bit[j] != 0 )
				{
					if( bitstream <= vbr_quality[i].bit[j] )
					{
						quality = vbr_quality[i].quality[j];

						DPRINTK("Get Quality %d,%d %d quality=%d\n",w,h,bitstream,quality);
						goto find_quality;
					}
				}
			}	
		}			
	}

find_quality:
	
	return quality;
}

VBR_QUALITY_ST * get_enc_info(int w,int h)
{	
	int i,j;

	for( i = 0; i < MAX_VBR_QUALITY_ITEM_NUM; i++)
	{
		if( w == vbr_quality[i].w  &&  h == vbr_quality[i].h )
		{
			return &vbr_quality[i];
		}			
	}

	return NULL;
}



void Hisi_set_venc_vbr()
{
	DPRINTK("set enc vbr!\n");
	encode_vbr = 1;
}

int Hisi_get_venc_vbr()
{
	return encode_vbr;
}

void Hisi_control_lock()
{
	int ret = 0;	
	ret = pthread_mutex_lock( &hisi_control_mutex);
	DPRINTK("lock %d ret=%d\n",lock_count,ret);
	lock_count++;
	 if( lock_count > 1000 )
	 	lock_count = 0;
}

void Hisi_control_unlock()
{
	 int ret = 0;	
	 ret = pthread_mutex_unlock( &hisi_control_mutex);
	 DPRINTK("2unlock %d ret=%d\n",lock_count,ret);
	 	 
}

void mtd_store_lock()
{
	int ret = 0;	
	ret = pthread_mutex_lock( &mtd_store_mutex);
	DPRINTK("lock  ret=%d\n",ret);
}

void mtd_store_unlock()
{
	int ret = 0;	
	ret = pthread_mutex_unlock( &mtd_store_mutex);
	DPRINTK("unlock ret=%d\n",ret);
}

void Hisi_set_chip_slave()
{
	is_slave_chip = 1;
}

int Hisi_is_slave_chip()
{
	return is_slave_chip;
}


int Hisi_set_spi_antiflickerattr(int gs_enNorm,int antiflicker_mode)
{
	if( 	antiflicker_mode== 0 )
		goke_set_backlight(1);
	else
		goke_set_backlight(0);
	return 1;
}




int Hisi_set_isp_DefectPixel(int use_self_data)
{
	DPRINTK("in func\n");    
	return 1;
}




int Hisi_init_dev(int image_size,int gs_enNorm)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_destroy_dev()
{
	DPRINTK("in func\n");    
	return 1;
}




int Hisi_stop_vi()
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_init_vo()
{
	DPRINTK("in func\n");    
	return 1;
}




int Hisi_window_vo_new(int window,int start_cam,int x,int y,int width1,int height1)
{
	DPRINTK("in func\n");
	return 1;
}

int Hisi_window_vo(int window,int start_cam,int x,int y,int width1,int height1)
{
	DPRINTK("in func\n");
	return 1;
}

int Hisi_playback_window_vo(int window,int start_cam)
{
	DPRINTK("in func\n");
	return 1;
}


int Hisi_set_vo_framerate(float speed)
{
	DPRINTK("in func\n");
	return 1;
}

int Hisi_stop_vo(int real_close)
{
	DPRINTK("in func\n");
	return 1;
}


int Hisi_clear_vo_buf(int chan)
{
	DPRINTK("in func\n");
	return 1;
}


int Hisi_create_encode(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_set_encode(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Get_udp_audio_data(BUF_FRAMEHEADER * pheadr,char * pdata)
{	
	return get_one_buf(pheadr,pdata,&g_audioList);
}



int Hisi_destroy_encode(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_create_decode(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_destroy_decode(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


//#define USE_COMBINE_FRAME_MODE 



int Net_cam_get_encode_data(NET_MODULE_ST * pst)
{
	DPRINTK("in func\n");    
	return 1;
}



int Net_send_self_data(char * buf,int buf_length)
{
	static int number = 0;
	BUF_FRAMEHEADER fheader;
	fheader.index = 0;
	fheader.type = DATA_FRAME;
	fheader.iLength = buf_length;
	fheader.frame_num = number;

	number++;

	if( number > 100)
		number = 0;
	
	if( is_have_client() <=0 )
		return -1;	
	
	return insert_data(&fheader,buf,&g_audioList);	
}



//#define ENCODE_SAVE_FILE_TEST

int Hisi_get_encode_data(BUFLIST * list,unsigned char * encode_buf)
{
	DPRINTK("in func\n");    
	return 1;
}


char * net_mult_frame_in_one_buf(BUF_FRAMEHEADER *pheadr,char * pdata,int reset,
			int  * buf_length,int * buf_count,int get_buf)
{
	static int write_buf_count = 0;
	static int write_buf_length = 0;
	FRAMEHEADERINFO stframeheader;
	char * tmp_buf = (char *)net_multframe_in_one_buf;

	if( reset == 1 )
	{
		write_buf_count = 0;
		write_buf_length = 0;

		return NULL;
	}

	if( get_buf == 1 )
	{
		*buf_length = write_buf_length;
		*buf_count = write_buf_count;
		write_buf_count = 0;
		write_buf_length = 0;
		return tmp_buf;
	}

	stframeheader.iChannel = pheadr->index;
	stframeheader.imageSize = pheadr->size;
	stframeheader.ulDataLen = pheadr->iLength;
	stframeheader.ulFrameNumber =pheadr->frame_num;
	stframeheader.ulFrameType = pheadr->iFrameType;
	stframeheader.ulTimeSec = pheadr->ulTimeSec;
	stframeheader.ulTimeUsec = pheadr->ulTimeUsec;
	stframeheader.videoStandard = pheadr->standard;
	

	if( pheadr->type == VIDEO_FRAME )
		stframeheader.type = 'V';
	else
		stframeheader.type = 'A';

	

	memcpy( tmp_buf + write_buf_length,(char *)&stframeheader,sizeof(FRAMEHEADERINFO));
	write_buf_length += sizeof(FRAMEHEADERINFO);

	memcpy( tmp_buf + write_buf_length,pdata,pheadr->iLength);
	write_buf_length += pheadr->iLength;

	// 数据对齐。
	write_buf_length = (write_buf_length + 3) /4 * 4;

	write_buf_count++;	

	if( write_buf_length > 100*1024 )
	{		
		if( buf_length )
		{
			*buf_length = write_buf_length;
		}
	}else
	{
		if( buf_length )
		{
			*buf_length = 0;
		}
	}
	
	return NULL;
	
}

// 这种是多帧合为一帧传送
int Hisi_get_net_encode_data(BUFLIST * list,unsigned char * encode_buf)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_start_enc(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}

int Net_cam_start_enc(int chan)
{
	DPRINTK("in func\n");    
	return 1;
}

int Net_cam_stop_enc(int chan)
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_stop_enc(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_create_IDR(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n"); 
	return 1;
}


int Hisi_create_IDR_2(int ichannel)
{

	goke_create_idr(ichannel);
	return 1;
}


int Hisi_start_dec(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_stop_dec(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_send_decode_data(ENCODE_ITEM * enc_item,unsigned char * pdata,int length,int display_chan)
{
	DPRINTK("in func\n");    
	return 1;
}
int Hisi_create_motiondetect(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_enable_motiondetect(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_disable_motiondetect(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_destroy_motiondetect(ENCODE_ITEM * enc_item)
{
	DPRINTK("in func\n");    
	return 1;
}



int Hisi_get_motion_buf(ENCODE_ITEM * enc_item)
{	
	if( enc_item->motion_buf != NULL )
		return 1;
	else
		return -1;
}

#define SAMPLE_AUDIO_PTNUMPERFRM   320
#define SAMPLE_AUDIO_AO_DEV 0
#define SAMPLE_AUDIO_AI_SPEAK_DEV 1




int Hisi_init_audio_device()
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_destroy_audio_device()
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_read_audio_buf(unsigned char *buf,int * count,int chan)
{
	DPRINTK("in func\n");
	return 1;
}


int Hisi_write_audio_buf(unsigned char *buf,int count,int chan)
{
	//DPRINTK("in func\n");
	int ret = audio_ao_send(buf, count);
	return ret;
	//return 1;
}

static int goke_chan_fps[6] = {0};
static int goke_chan_gop[6] = {0};
static int goke_chan_bitstream[6] = {0};

int Hisi_set_encode_bnc(int chan,int gop,int frame_rate,int bitstream)
{
	goke_set_enc(chan,frame_rate,bitstream,gop);
	set_rtsp_fps(chan,frame_rate);
	goke_chan_fps[chan] = frame_rate;
	goke_chan_gop[chan] = gop;
	goke_chan_bitstream[chan] = bitstream;
	//Hisi_create_IDR_2(chan);
	return 1;
}

int Hisi_get_stream_fps(int chan)
{
	return goke_chan_fps[chan];
}

int Hisi_get_stream_gop(int chan)
{
	return goke_chan_gop[chan];
}

int Hisi_get_stream_bitstream(int chan)
{
	return goke_chan_bitstream[chan];
}




int Hisi_chan_set_encode(int chan,int bit,int rate,int gop)
{
	return Hisi_set_encode_bnc(chan,gop,rate,bit);
}


int Hisi_get_chan_current_fps(int chan)
{
	//DPRINTK("in func\n");    
	return 0;
}

static int video_lost_flag = 0;

int get_video_send_lost_flag()
{
	return video_lost_flag;
}


void set_video_send_lost_flag(int flag)
{
	if( flag == 0 )
		video_lost_flag = 0;
	else 
		video_lost_flag = 1;
}


int Hisi_Net_cam_net_send_video_data(NET_MODULE_ST * pst,int rec_flag,int net_send_flag)
{
      NET_MODULE_ST * pst2 = NULL;	

	int i,j;		
	int ret;
	int  s32Ret;	
	int  VeChn;
	struct timeval recTime;
	struct timeval sendStartTime;
	struct timeval sendEndTime;
	BUF_FRAMEHEADER fheader;
	BUF_FRAMEHEADER * pHeader = NULL;
	unsigned char * ptr_point = NULL;	
	unsigned char * pVideoBuf = NULL;
	struct timeval tv;
	char * my_buf = NULL;
	int maxfd = 0;	
	int send_net_frame = 0;	
	static int not_get_data_num = 0;
	static int check_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int cur_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int show_count = 0;
	static long long recv_packet_no[MAX_VIDEO_STREAM_NUM] = {0};
	static int get_first_frame[MAX_VIDEO_STREAM_NUM] = {0};;
	int have_data = 0;
	int len;
	int last_key_frame_no = 0;
	static SINGLY_LINKED_GET_DATA_ST video_data[MAX_VIDEO_STREAM_NUM];
	int retry_num = 0;

	

	send_net_frame = 1;	

	for( i = 0; i < MAX_VIDEO_STREAM_NUM; i++)
	{
		if( sll_list_item_num(&gVideolist[i])  >  0  )
		{
			have_data = 1;
			break;
		}
	}

	if( have_data == 0 )
	{
		usleep(500000);
		printf("have no data!\n");
		goto go_out;
	}		
		
	for (j=0;j<MAX_VIDEO_STREAM_NUM;j++)
	{		
		//pVideoBuf = my_buf + sizeof(BUF_FRAMEHEADER);
		//len = sll_get_list_item(&gVideolist[j],(char*)pVideoBuf,MAX_FRAME_BUF_SIZE,recv_packet_no[j]);
		
		len = sll_get_list_item_lock(&gVideolist[j],&video_data[j],recv_packet_no[j]);
		if( len > 0 )			
		{								
		  	my_buf = video_data[j].data;
			//pVideoBuf = my_buf + sizeof(BUF_FRAMEHEADER);
			pVideoBuf = my_buf + GK_VIDEO_DATA_OFFSET;
			
			recv_packet_no[j] = recv_packet_no[j]+1;

			//DPRINTK("recv_packet_no[%d] = %lld\n",j,recv_packet_no[j] );

			pHeader = (BUF_FRAMEHEADER *)my_buf;

			if( pHeader->type != VIDEO_FRAME )
				goto release_buf;


			Net_cam_set_send_video_frame();
			
			VeChn = j;
			

			if( VeChn == 2 || VeChn == 5 )
			{
				pst->net_send_count--;
			}

			ptr_point = pVideoBuf;			

			
			
			memset(&fheader,0x00,sizeof(fheader));

			//fheader.iLength = len -sizeof(BUF_FRAMEHEADER);
			fheader.iLength = len -GK_VIDEO_DATA_OFFSET;
			

			if( fheader.iLength > (MAX_FRAME_BUF_SIZE - 1024) )
			{
				DPRINTK("VeChn = %d length = %ld too big size ,drop!\n",VeChn,fheader.iLength);
				
				pst->lost_frame[VeChn] = 1;
				goto release_buf;
			}

			

			//DPRINTK("[%d] %x %x %x %x %x type=%d\n",j,pVideoBuf[0], pVideoBuf[1] ,pVideoBuf[2] , pVideoBuf[3] ,pVideoBuf[4],Hisi_get_encode_type(VeChn));

			
			
			if( Hisi_get_encode_type(VeChn) == GK_CODE_265 )
			{

				 if( pVideoBuf[4]== 0x40  &&  pVideoBuf[3] ==0x01 &&  pVideoBuf[2] ==0x00 &&  pVideoBuf[1] ==0x00 &&  pVideoBuf[0] ==0x00)
	 			{
					fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
					pst->lost_frame[VeChn] = 0;

					//DPRINTK("[%d] keyframe recv_packet_no=%d  len=%d\n",VeChn,recv_packet_no[VeChn],len);
				}else
					fheader.iFrameType =0;	
			}else if( Hisi_get_encode_type(VeChn) == GK_CODE_264 )
			{
				//DPRINTK("%x %x %x %x %x\n",pVideoBuf[0], pVideoBuf[1] ,pVideoBuf[2] , pVideoBuf[3] ,pVideoBuf[4]);
				if(pVideoBuf[0] ==0x0 && pVideoBuf[1] ==0x0 && pVideoBuf[2] ==0x0 && pVideoBuf[3] ==0x1 &&  (pVideoBuf[4] & 0x0f)  == (0x67 & 0x0f))
				{
					fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
					pst->lost_frame[VeChn] = 0;

					//DPRINTK("[%d] keyframe recv_packet_no=%d  len=%d\n",VeChn,recv_packet_no[VeChn],len);
				}else
					fheader.iFrameType =0;	
			}
			
			
			//if( VeChn == 1 ) //主子码流有时长时间没有I 帧
			{
				static int no_key_frame_num[6] = {0};	
				
				no_key_frame_num[VeChn]++;

				if( fheader.iFrameType == 1 )
					no_key_frame_num[VeChn] = 0;

				if( no_key_frame_num[VeChn] > (Hisi_get_stream_gop(VeChn)  + 2 ) )
				{			
					Hisi_create_IDR_2(VeChn);
					no_key_frame_num[VeChn] = 0;			
				}
				
			}

		
			if( pst->lost_frame[VeChn] == 1 )						
				goto release_buf;	
			
			
			if( get_bit(pst->send_cam,VeChn) == 1 )
			{
			      ptr_point = my_buf + GK_VIDEO_DATA_OFFSET - sizeof(BUF_FRAMEHEADER);
				  
			      ret  =GM_send_encode_data(pst,fheader.iFrameType,ptr_point,fheader.iLength,VeChn);
				  if( ret < 0 )
				  {					  	
				  	pst->lost_frame[VeChn] = 1;
				  }
			}

			{
				int j;
				for( j = 1; j < MAX_CLIENT;j++)
				{
					pst2 = client_pst[j];				
					if( pst2->is_alread_work == 1)
					{
						//DPRINTK("GM_send_encode_data > 0\n");
						 if( get_bit(pst2->send_cam,VeChn) == 1 )
						 {
						 	ptr_point = my_buf + GK_VIDEO_DATA_OFFSET - sizeof(BUF_FRAMEHEADER);
			    			 	GM_send_encode_data(pst2,fheader.iFrameType,ptr_point,fheader.iLength,VeChn);
						 }
					}
				}
			 }	
			

			if( rec_flag == 1 )
			{
				//DPRINTK("GM_send_rec_data\n");
				ptr_point = my_buf + GK_VIDEO_DATA_OFFSET - sizeof(BUF_FRAMEHEADER);
				 ret  =GM_send_rec_data(fheader.iFrameType,ptr_point,fheader.iLength,VeChn);
			   	// if( ret < 0 )			  
			  	//	Hisi_create_IDR_2(VeChn);
			  
			}		
		

			if( VeChn == 0 )
	  			p720_encode_rate_show();
			else
				cif_decode_rate_show();			

			release_buf:

			sll_get_list_item_unlock(&gVideolist[j],&video_data[j]);			
			
		}else
		{
			if( recv_packet_no[j] >= gVideolist[j].head_id )
			{
				if( recv_packet_no[j] - gVideolist[j].head_id > 2 )
				{
					DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
					recv_packet_no[j]  = gVideolist[j].head_id;						
				}
				usleep(10000);
			}else
			{

				if( abs(recv_packet_no[j] - gVideolist[j].head_id) <  gVideolist[j].item_num *2 )
				{
					DPRINTK("set net send video lost flag!\n");
					set_video_send_lost_flag(1);
				}
			
				last_key_frame_no = sll_get_last_key_frame_list_item_no(&gVideolist[j]);				
				DPRINTK("[%d] lost frame recv_packet_no=%d  head_id=%d last_key_frame_no=%d\n",\
					j,recv_packet_no[j],gVideolist[j].head_id,last_key_frame_no);					

				if( last_key_frame_no > 0 )
				{
					recv_packet_no[j]  = last_key_frame_no;					
				}
				else
					recv_packet_no[j]  = gVideolist[j].head_id;
				
				pst->lost_frame[j] = 1;			
				
			}
		}
		
	
	}
		     
    
	return 1;

go_out:
	return 0;
}




int Hisi_Net_cam_extern_net_send_video_data(NET_MODULE_ST * pst,int rec_flag,int net_send_flag)
{

	
      NET_MODULE_ST * pst2 = NULL;	

	int i,j;		
	int ret;
	int  s32Ret;	
	int  VeChn;
	struct timeval recTime;
	struct timeval sendStartTime;
	struct timeval sendEndTime;
	BUF_FRAMEHEADER fheader;
	unsigned char * ptr_point = NULL;	
	unsigned char * pVideoBuf = NULL;
	struct timeval tv;
	char * my_buf = NULL;
	int maxfd = 0;	
	int send_net_frame = 0;	
	static int not_get_data_num = 0;
	static int check_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int cur_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int show_count = 0;
	static long long  recv_packet_no[MAX_VIDEO_STREAM_NUM] = {0};
	static int get_first_frame[MAX_VIDEO_STREAM_NUM] = {0};;
	int have_data = 0;
	int len;
	int last_key_frame_no = 0;
	static SINGLY_LINKED_GET_DATA_ST video_data;
	int retry_num = 0;
	int net_chan = 0;
	static int lost_frame[2];
	BUF_FRAMEHEADER * pHeader = NULL;

	send_net_frame = 1;	

	if( Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_D1 )	
		net_chan = 0;
	else
		net_chan = 1;


	if( sll_list_item_num(&gVideolist[net_chan])  >  0  )
	{
		have_data = 1;			
	}	

	if( have_data == 0 )
	{
		usleep(10000);
		goto go_out;
	}	

	if( net_send_flag == 0 )
	{
		usleep(10000);
		goto go_out;
	}
		
	if( g_pstcommand->GST_FTC_get_play_status() == 0  )
	{		
		j = net_chan;
		len = sll_get_list_item_lock(&gVideolist[j],&video_data,recv_packet_no[j]);
		if( len > 0 )			
		{		
			my_buf = video_data.data;
			//pVideoBuf = my_buf + sizeof(BUF_FRAMEHEADER);
			pVideoBuf = my_buf + GK_VIDEO_DATA_OFFSET;
			
			recv_packet_no[j] = recv_packet_no[j]+1;
			
			VeChn = j;			

			ptr_point = pVideoBuf;			


			pHeader = (BUF_FRAMEHEADER *)my_buf;

			if( pHeader->type != VIDEO_FRAME )
				goto release_buf;


			
			memset(&fheader,0x00,sizeof(fheader));

			//fheader.iLength = len -sizeof(BUF_FRAMEHEADER);
			fheader.iLength = len -GK_VIDEO_DATA_OFFSET;

			

			if( fheader.iLength > (MAX_FRAME_BUF_SIZE - 1024) )
			{
				DPRINTK("VeChn = %d length = %ld too big size ,drop!\n",VeChn,fheader.iLength);
				
				lost_frame[VeChn] = 1;
				goto release_buf;
			}

			//DPRINTK("[%d] %x %x %x %x %x %d\n",VeChn,pVideoBuf[0],pVideoBuf[1],pVideoBuf[2],pVideoBuf[3],pVideoBuf[4],Hisi_get_encode_type(VeChn) );
			if( Hisi_get_encode_type(VeChn) == GK_CODE_265 )
			{

				 if( pVideoBuf[4]== 0x40  &&  pVideoBuf[3] ==0x01 &&  pVideoBuf[2] ==0x00 &&  pVideoBuf[1] ==0x00 &&  pVideoBuf[0] ==0x00)
	 			{
					fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
					pst->lost_frame[VeChn] = 0;

					//DPRINTK("[%d] keyframe recv_packet_no=%d  len=%d\n",VeChn,recv_packet_no[VeChn],len);
				}else
					fheader.iFrameType =0;	
			}else if( Hisi_get_encode_type(VeChn) == GK_CODE_264 )
			{
				//DPRINTK("%x %x %x %x %x\n",pVideoBuf[0], pVideoBuf[1] ,pVideoBuf[2] , pVideoBuf[3] ,pVideoBuf[4]);
				if(pVideoBuf[0] ==0x0 && pVideoBuf[1] ==0x0 && pVideoBuf[2] ==0x0 && pVideoBuf[3] ==0x1 &&  (pVideoBuf[4] & 0x0f)  == (0x67 & 0x0f))
				{
					fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
					pst->lost_frame[VeChn] = 0;

					//DPRINTK("[%d] keyframe recv_packet_no=%d  len=%d\n",VeChn,recv_packet_no[VeChn],len);
				}else
					fheader.iFrameType =0;	
			}
			

			//DPRINTK("send [%d] len=%d lost_frame[VeChn] =%d iFrameType=%d\n",VeChn,fheader.iLength,pst->lost_frame[VeChn]  ,fheader.iFrameType);
		
			if( pst->lost_frame[VeChn] == 1 )						
				goto release_buf;	


			if( net_send_flag == 1 )
			{					
				ret = GM_send_dvr_net_data(fheader.iFrameType,my_buf,fheader.iLength,VeChn); 			
			}			
		

			if( VeChn == 0 )
	  			p720_encode_rate_show();
			else
				cif_decode_rate_show();

			release_buf:

			sll_get_list_item_unlock(&gVideolist[j],&video_data);			
			
		}else
		{
			if( recv_packet_no[j] >= gVideolist[j].head_id )
			{
				if( recv_packet_no[j] - gVideolist[j].head_id > 2 )
				{
					DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
					recv_packet_no[j]  = gVideolist[j].head_id;						
				}
				usleep(10000);
			}else
			{			
			
				last_key_frame_no = sll_get_last_key_frame_list_item_no(&gVideolist[j]);				
				DPRINTK("[%d] lost frame recv_packet_no=%d  head_id=%d last_key_frame_no=%d\n",\
					j,recv_packet_no[j],gVideolist[j].head_id,last_key_frame_no);					

				if( last_key_frame_no > 0 )
				{
					recv_packet_no[j]  = last_key_frame_no;					
				}
				else
					recv_packet_no[j]  = gVideolist[j].head_id;
				
				lost_frame[j] = 1;			
				
			}
		}
		
	
	}
		     
    
	return 1;

go_out:
	return 0;
}





int Hisi_Net_cam_rec_send_video_data(NET_MODULE_ST * pst,int rec_flag,int net_send_flag)
{
      NET_MODULE_ST * pst2 = NULL;	

	int i,j;		
	int ret;
	int  s32Ret;	
	int  VeChn;
	struct timeval recTime;
	struct timeval sendStartTime;
	struct timeval sendEndTime;
	BUF_FRAMEHEADER fheader;
	unsigned char * ptr_point = NULL;	
	unsigned char * pVideoBuf = NULL;
	struct timeval tv;
	char my_buf[MAX_FRAME_BUF_SIZE];
	int maxfd = 0;	
	int send_net_frame = 0;	
	static int not_get_data_num = 0;
	static int check_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int cur_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int show_count = 0;
	static unsigned int  recv_packet_no[MAX_VIDEO_STREAM_NUM] = {0};
	static int get_first_frame[MAX_VIDEO_STREAM_NUM] = {0};;
	int have_data = 0;
	int len;

	send_net_frame = 1;	

	for( i = 0; i < MAX_VIDEO_STREAM_NUM; i++)
	{
		if( sll_list_item_num(&gVideolist[i])  >  0  )
		{
			have_data = 1;
			break;
		}
	}

	if( have_data == 0 )
	{
		usleep(10000);
		goto go_out;
	}		
		
	for (j=0;j<MAX_VIDEO_STREAM_NUM;j++)
	{		
		pVideoBuf = my_buf + sizeof(BUF_FRAMEHEADER);
		len = sll_get_list_item(&gVideolist[j],(char*)pVideoBuf,MAX_FRAME_BUF_SIZE,recv_packet_no[j]);
		if( len > 0 )			
		{								
		  	Net_cam_set_send_video_frame();

			recv_packet_no[j] = recv_packet_no[j]+1;
			
			VeChn = j;

			if( VeChn == 2 || VeChn == 5 )
			{
				pst->net_send_count--;
			}

			ptr_point = pVideoBuf;					

			
			memset(&fheader,0x00,sizeof(fheader));

			fheader.iLength = len;			

			if( fheader.iLength > (MAX_FRAME_BUF_SIZE - 1024) )
			{
				DPRINTK("VeChn = %d length = %ld too big size ,drop!\n",VeChn,fheader.iLength);
				
				pst->lost_frame[j] = 1;
				continue;
			}

			if(pVideoBuf[0] ==0x0 && pVideoBuf[1] ==0x0 && pVideoBuf[2] ==0x0 && pVideoBuf[3] ==0x1 &&  (pVideoBuf[4] & 0x0f)  == (0x65 & 0x0f))
			{
				fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
				pst->lost_frame[j] = 0;
			}
			else
				fheader.iFrameType =0;				

		
			if( pst->lost_frame[j] == 1 )
				continue;

			
			if( rec_flag == 1 )
			{
				 ret  =GM_send_rec_data(fheader.iFrameType,ptr_point,fheader.iLength,VeChn);
			   	// if( ret < 0 )			  
			  	//	Hisi_create_IDR_2(VeChn);
			  
			}


			if( VeChn == 0 )
	  			p720_encode_rate_show();
			else
				cif_decode_rate_show();				
			
		}else
		{
			if( recv_packet_no[j] >= gVideolist[j].head_id )
			{
				if( recv_packet_no[j] - gVideolist[j].head_id > 2 )
				{
					DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
					recv_packet_no[j]  = gVideolist[j].head_id;						
				}
				usleep(10000);
			}else
			{
				DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
				recv_packet_no[j]  = gVideolist[j].head_id;	
				pst->lost_frame[j] = 1;
			}
		}
		
	
	}
		     
    
	return 1;

go_out:
	return 0;
}




int Hisi_Net_cam_rtsp_send_video_data(NET_MODULE_ST * pst,int rec_flag,int net_send_flag)
{
      NET_MODULE_ST * pst2 = NULL;	

	int i,j;		
	int ret;
	int  s32Ret;	
	int  VeChn;
	struct timeval recTime;
	struct timeval sendStartTime;
	struct timeval sendEndTime;
	BUF_FRAMEHEADER fheader;
	unsigned char * ptr_point = NULL;	
	unsigned char * pVideoBuf = NULL;
	struct timeval tv;
	char my_buf[MAX_FRAME_BUF_SIZE];
	int maxfd = 0;	
	int send_net_frame = 0;	
	static int not_get_data_num = 0;
	static int check_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int cur_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	static int show_count = 0;
	static unsigned int  recv_packet_no[MAX_VIDEO_STREAM_NUM] = {0};
	static int get_first_frame[MAX_VIDEO_STREAM_NUM] = {0};;
	int have_data = 0;
	int len;

	send_net_frame = 1;	

	for( i = 0; i < MAX_VIDEO_STREAM_NUM; i++)
	{
		if( sll_list_item_num(&gVideolist[i])  >  0  )
		{
			have_data = 1;
			break;
		}
	}

	if( have_data == 0 )
	{
		usleep(10000);
		goto go_out;
	}		
		
	for (j=0;j<MAX_VIDEO_STREAM_NUM;j++)
	{		
		pVideoBuf = my_buf + sizeof(BUF_FRAMEHEADER);
		len = sll_get_list_item(&gVideolist[j],(char*)pVideoBuf,MAX_FRAME_BUF_SIZE,recv_packet_no[j]);
		if( len > 0 )			
		{								
		  	Net_cam_set_send_video_frame();

			recv_packet_no[j] = recv_packet_no[j]+1;
			
			VeChn = j;

			if( VeChn == 2 || VeChn == 5 )
			{
				pst->net_send_count--;
			}

			ptr_point = pVideoBuf;					

			
			memset(&fheader,0x00,sizeof(fheader));

			fheader.iLength = len;			

			if( fheader.iLength > (MAX_FRAME_BUF_SIZE - 1024) )
			{
				DPRINTK("VeChn = %d length = %ld too big size ,drop!\n",VeChn,fheader.iLength);
				
				pst->lost_frame[j] = 1;
				continue;
			}

			if(pVideoBuf[0] ==0x0 && pVideoBuf[1] ==0x0 && pVideoBuf[2] ==0x0 && pVideoBuf[3] ==0x1 &&  (pVideoBuf[4] & 0x0f)  == (0x65 & 0x0f))
			{
				fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
				pst->lost_frame[j] = 0;
			}
			else
				fheader.iFrameType =0;				

		
			if( pst->lost_frame[j] == 1 )
				continue;		

			
			{
				   	MEM_MODULE_ST * pMem =NULL;
					int current_enc_fps = 0;
					
					check_enc_fps[VeChn]++;

					if( check_enc_fps[VeChn] > 5 )
					{
						check_enc_fps[VeChn] = 0;
						current_enc_fps = Hisi_get_chan_current_fps(VeChn);
						if( current_enc_fps > 0 )
						{							
							fheader.size = current_enc_fps;							
						}
					}else
					{
						fheader.size = 0 ;
					}

					{
						MEM_MODULE_ST * pMem =NULL;
						pMem = pMemModule[VeChn];
						if( pMem )
						{		
							fheader.type = VIDEO_FRAME;
							memcpy(my_buf,&fheader,sizeof(BUF_FRAMEHEADER));					
						   	if( MEM_write_data_lock(pMem,my_buf,fheader.iLength + sizeof(BUF_FRAMEHEADER),0) < 0 )
							{
								DPRINTK("insert data error: chan=%ld length=%ld type=%ld num=%ld\n",
									VeChn,fheader.iLength,fheader.iFrameType,fheader.frame_num);	

								pMemModule[VeChn] = NULL;
							}
								
						}				

						show_count++;
					}
				   }


			if( VeChn == 0 )
	  			p720_encode_rate_show();
			else
				cif_decode_rate_show();					
			
		}else
		{
			if( recv_packet_no[j] >= gVideolist[j].head_id )
			{
				if( recv_packet_no[j] - gVideolist[j].head_id > 2 )
				{
					DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
					recv_packet_no[j]  = gVideolist[j].head_id;						
				}
				usleep(10000);
			}else
			{
				DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gVideolist[j].head_id);
				recv_packet_no[j]  = gVideolist[j].head_id;	
				pst->lost_frame[j] = 1;
			}
		}
		
	
	}
		     
    
	return 1;

go_out:
	return 0;
}


int  Hisi_set_isp_SetAWBAttr(int au16StaticWB_0,int au16StaticWB_1,int au16StaticWB_2,int au16StaticWB_3)
{
	DPRINTK("in func\n");    
	return 1;
}


int  Hisi_set_isp_SetDenoiseAttr(int mode,int  u8ThreshTarget)
{
	DPRINTK("in func\n");    
	return 1;
}


int  Hisi_set_isp_SetAEAttr(int u16ExpTimeMax,int u16ExpTimeMin,int u16AGainMax,
	int u16AGainMin,int u16DGainMax,int u16DGainMin,int u8ExpStep,int u8ExpCompensation,int u32SystemGainMax)
{
	DPRINTK("in func\n");    
	return 1;
}



int  Hisi_set_isp_limit_expouse(int u16ExpTimeMax,int u16ExpTimeMin)
{
	DPRINTK("in func\n");    
	return 1;
}





int  Hisi_set_isp_max_gain(int u32SystemGainMax)
{
	DPRINTK("in func\n");    
	return 1;
}


int  Hisi_SetSlowFrameRate(unsigned char uSlowRate)
{	
	
	return 1;
}



int Hisi_set_hm1375_AEAttr()
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_set_imx138_AEAttr()
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_set_AE_weighting_table()
{
	DPRINTK("in func\n");    
	return 1;
}

int Hisi_enable_DRC(int flag)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_enable_antifog(int flag)
{
	DPRINTK("in func\n");    
	return 1;
}


int Hisi_enable_ae_ajdust(int flag)
{
	DPRINTK("in func\n");    
	return 1;
}


int  Hisi_set_vi_csc_attr(int u32LumaVal,int u32ContrVal,int u32HueVal,int u32SatuVal)
{
	int brightness;       /*Brightness                               0 - 255           */
	int colorSaturation;  /*Color saturation                        0 - 255          */
	int contrast;         /*Contrast                                  0 - 128           */

	DPRINTK("brightness=%d  colorSaturation=%d  contrast=%d\n",u32LumaVal,u32SatuVal,u32ContrVal);

	brightness = u32LumaVal * 100 / 100;
	colorSaturation = u32SatuVal * 100 / 100;
	contrast = u32ContrVal * 100 / 100;
	
	
	if( IsLightDay == 0 )	
	{
		DPRINTK("IsLightDay = %d  set saturation = 0\n",IsLightDay);	
		colorSaturation = 0;
	}	
	
	return goke_set_img(brightness,colorSaturation,contrast);
}



int  Hisi_get_isp_sharpen_value(IPCAM_ALL_INFO_ST * pAllInfo)
{
	DPRINTK("in func\n");    
	return 1;
}

int  Hisi_set_isp_colormatrix(IPCAM_ALL_INFO_ST * pAllInfo)
{
	DPRINTK("in func\n");    
	return 1;
}


int  Hisi_get_isp_colormatrix(IPCAM_ALL_INFO_ST * pAllInfo)
{
	DPRINTK("in func\n");    
	return 1;
}



int  Hisi_set_isp_sharpen_value(int is_auto,int u8StrengthUdTarget,int u8StrengthMin,int u8StrengthTarget)
{
	DPRINTK("in func\n");    
	return 1;
}



int  Hisi_set_isp_saturation_value(int value)
{
	DPRINTK("in func\n");    
	return 1;
}


int ipcam_get_cur_color_temp(int * pcolor_temp)
{
	DPRINTK("in func\n");    
	return 1;
}

int His_set_isp_sat_by_gamma_mode(int gamma)
{
	DPRINTK("in func\n");    
	return 1;
}

int  Hisi_set_isp_auto_image_quality_adjustment()
{
	DPRINTK("in func\n");    
	return 1;
}


int atox(char * pValue)
{
	int value;

	sscanf(pValue,"%x",&value);

	//DPRINTK("value=%x  %s\n",value,pValue);

	return value;
}

int Hisi_set_isp_init_value(int load_default)
{
	char config_value[150] ="";
	char log_file_name[100] ="";
	int ret;
	unsigned char saturation = 0xff;  //ar0130 0x90;
	unsigned short au16StaticWB_0 = 0xffff; //ar0130 0x177;
	unsigned short au16StaticWB_1 = 0xffff;
	unsigned short au16StaticWB_2 = 0xffff;
	unsigned short au16StaticWB_3 = 0xffff;
	int u16ExpTimeMax= 0xffff;
	int u16ExpTimeMin= 0xffff;
	int u16AGainMax= 0xffff;
	int u16AGainMin= 0xffff;
	int u16DGainMax= 0xffff;
	int u16DGainMin= 0xffff;
	int u8ExpStep= 0xffff;	
	int u32LumaVal= 0xffff;    
	int u32ContrVal= 0xffff;              
	int u32HueVal= 0xffff;               
	int u32SatuVal= 0xffff;   
	int u16NightExpTimeMax= 0xffff;
	int u16NightExpTimeMin= 0xffff;
	int u16NightAGainMax= 0xffff;
	int u16NightAGainMin= 0xffff;
	int u16NightDGainMax= 0xffff;
	int u16NightDGainMin= 0xffff;
	int u8StrengthUdTarget = 0xff;
	int u8StrengthTarget = 0xff;
	int u8StrengthMin = 0xff;
	int u8NightStrengthMin = 0xff;
	int u32GameMode = 0x00;
	int u32DenoiseManualValue[3] = {0};
	int u32DenoiseMode = 0x00;
	int u32DenoiseMode_Night = 0x00;
	int u32NightDenoiseManualValue = 0x30;
	int u8ExpCompensation = 0x80;
	int u32SharpenAuto =0x1;
	int u32SystemGainMax = 0x0;
	int u32cds[2] = {149,165};
	int u32LightCheckMode = IPCAM_LIGHT_CONTROL_ADC_RESISTANCE_NORMAL_CHECK_AUTO;
	int u32day_night_mode = IPCAM_DAY_NIGHT_MODE_AUTO;
	int u32SlowFrame = 0;
	strcpy(log_file_name,CONFIG_FILE_ISP);
	
	ret= cf_read_value(log_file_name,"saturation",config_value);
	if( ret >= 0)	
		saturation = (unsigned char)atox(config_value);
	
	Hisi_set_isp_saturation_value(saturation);


	ret= cf_read_value(log_file_name,"au16StaticWB_0",config_value);
	if( ret >= 0)	
		au16StaticWB_0 = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"au16StaticWB_1",config_value);
	if( ret >= 0)	
		au16StaticWB_1 = (unsigned short)atox(config_value);

	ret= cf_read_value(log_file_name,"au16StaticWB_2",config_value);
	if( ret >= 0)	
		au16StaticWB_2 = (unsigned short)atox(config_value);

	ret= cf_read_value(log_file_name,"au16StaticWB_3",config_value);
	if( ret >= 0)	
		au16StaticWB_3 = (unsigned short)atox(config_value);
	
	
	Hisi_set_isp_SetAWBAttr(au16StaticWB_0,au16StaticWB_1,au16StaticWB_2,au16StaticWB_3);

	

	ret= cf_read_value(log_file_name,"u16NightExpTimeMax",config_value);
	if( ret >= 0)	
		u16NightExpTimeMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16NightExpTimeMin",config_value);
	if( ret >= 0)	
		u16NightExpTimeMin = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16NightAGainMax",config_value);
	if( ret >= 0)	
		u16NightAGainMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16NightAGainMin",config_value);
	if( ret >= 0)	
		u16NightAGainMin = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16NightDGainMax",config_value);
	if( ret >= 0)	
		u16NightDGainMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16NightDGainMin",config_value);
	if( ret >= 0)	
		u16NightDGainMin = (unsigned short)atox(config_value);	
	

	ret= cf_read_value(log_file_name,"u16ExpTimeMax",config_value);
	if( ret >= 0)	
		u16ExpTimeMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16ExpTimeMin",config_value);
	if( ret >= 0)	
		u16ExpTimeMin = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16AGainMax",config_value);
	if( ret >= 0)	
		u16AGainMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16AGainMin",config_value);
	if( ret >= 0)	
		u16AGainMin = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16DGainMax",config_value);
	if( ret >= 0)	
		u16DGainMax = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u16DGainMin",config_value);
	if( ret >= 0)	
		u16DGainMin = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u8ExpStep",config_value);
	if( ret >= 0)	
		u8ExpStep = (unsigned short)atox(config_value);

	ret= cf_read_value(log_file_name,"u8ExpCompensation",config_value);
	if( ret >= 0)	
		u8ExpCompensation = (unsigned char)atox(config_value);	

	Hisi_set_isp_SetAEAttr(u16ExpTimeMax,u16ExpTimeMin,u16AGainMax,u16AGainMin,u16DGainMax,u16DGainMin,u8ExpStep,u8ExpCompensation,0);
     
	ret= cf_read_value(log_file_name,"u32LumaVal",config_value);
	if( ret >= 0)	
		u32LumaVal = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u32ContrVal",config_value);
	if( ret >= 0)	
		u32ContrVal = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u32HueVal",config_value);
	if( ret >= 0)	
		u32HueVal = (unsigned short)atox(config_value);	

	ret= cf_read_value(log_file_name,"u32SatuVal",config_value);
	if( ret >= 0)	
		u32SatuVal = (unsigned short)atox(config_value);	

	Hisi_set_vi_csc_attr(u32LumaVal,u32ContrVal,u32HueVal,u32SatuVal);


	ret= cf_read_value(log_file_name,"u8StrengthTarget",config_value);
	if( ret >= 0)	
		u8StrengthTarget = (unsigned char)atox(config_value);	

	ret= cf_read_value(log_file_name,"u8StrengthMin",config_value);
	if( ret >= 0)	
		u8StrengthMin = (unsigned char)atox(config_value);	

	ret= cf_read_value(log_file_name,"u8NightStrengthMin",config_value);
	if( ret >= 0)	
		u8NightStrengthMin = (unsigned short)atox(config_value);	
	
	ret= cf_read_value(log_file_name,"u8StrengthUdTarget",config_value);
	if( ret >= 0)	
		u8StrengthUdTarget = (unsigned short)atox(config_value);	


	ret= cf_read_value(log_file_name,"u32cds_0",config_value);
	if( ret >= 0)	
		u32cds[0] = (unsigned char)atox(config_value);	

	ret= cf_read_value(log_file_name,"u32cds_1",config_value);
	if( ret >= 0)	
		u32cds[1] = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32GameMode",config_value);
	if( ret >= 0)	
		u32GameMode = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32DenoiseMode",config_value);
	if( ret >= 0)	
		u32DenoiseMode = (unsigned char)atox(config_value);
	

	ret= cf_read_value(log_file_name,"u32DenoiseManualValue_0",config_value);
	if( ret >= 0)	
		u32DenoiseManualValue[0] = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32DenoiseManualValue_1",config_value);
	if( ret >= 0)	
		u32DenoiseManualValue[1] = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32DenoiseManualValue_2",config_value);
	if( ret >= 0)	
		u32DenoiseManualValue[2] = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32NightDenoiseManualValue",config_value);
	if( ret >= 0)	
		u32NightDenoiseManualValue = (unsigned char)atox(config_value);

	ret= cf_read_value(log_file_name,"u32SharpenAuto",config_value);
	if( ret >= 0)	
		u32SharpenAuto = atox(config_value);

	ret= cf_read_value(log_file_name,"u32LightCheckMode",config_value);
	if( ret >= 0)
	{
		u32LightCheckMode = atox(config_value);		
	}

	ret= cf_read_value(log_file_name,"u32SystemGainMax",config_value);
	if( ret >= 0)	
	{
		u32SystemGainMax = (unsigned int)atox(config_value);		
	}

	ret= cf_read_value(log_file_name,"u32SlowFrame",config_value);
	if( ret >= 0)	
	{
		u32SlowFrame = (unsigned int)atox(config_value);		
	}
	

	

	{
		int value = 0;
		if( u32DenoiseMode > 1 && u32DenoiseMode < 5 )
		{
			value = u32DenoiseManualValue[u32DenoiseMode - 2];
		}
		Hisi_set_isp_SetDenoiseAttr(u32DenoiseMode,value);	
	}


	{
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
		IPCAM_ISP_ST * pIsp = &pAllInfo->isp_st;
		pIsp->saturation = saturation;
		pIsp->au16StaticWB_0 = au16StaticWB_0;
		pIsp->au16StaticWB_1 = au16StaticWB_1;
		pIsp->au16StaticWB_2 = au16StaticWB_2;
		pIsp->au16StaticWB_3 = au16StaticWB_3;
		pIsp->u16ExpTimeMax = u16ExpTimeMax;
		pIsp->u16ExpTimeMin = u16ExpTimeMin;
		pIsp->u16AGainMax = u16AGainMax;
		pIsp->u16AGainMin = u16AGainMin;
		pIsp->u16DGainMax = u16DGainMax;
		pIsp->u16DGainMin = u16DGainMin;
		pIsp->u8ExpCompensation = u8ExpCompensation;
		pIsp->u16NightExpTimeMax = u16NightExpTimeMax;
		pIsp->u16NightExpTimeMin = u16NightExpTimeMin;
		pIsp->u16NightAGainMax = u16NightAGainMax;
		pIsp->u16NightAGainMin = u16NightAGainMin;
		pIsp->u16NightDGainMax = u16NightDGainMax;
		pIsp->u16NightDGainMin = u16NightDGainMin;
		pIsp->u8ExpStep = u8ExpStep;	
		pIsp->u32LumaVal = u32LumaVal;
		pIsp->u32ContrVal = u32ContrVal;
		pIsp->u32HueVal = u32HueVal;
		pIsp->u32SatuVal = u32SatuVal;
		pIsp->u8StrengthTarget = u8StrengthTarget;
		pIsp->u8StrengthMin = u8StrengthMin;
		pIsp->u8StrengthUdTarget = u8StrengthUdTarget;
		pIsp->u8NightStrengthMin = u8NightStrengthMin;
		pIsp->u32cds[0] = u32cds[0];
		pIsp->u32cds[1] = u32cds[1];
		pIsp->u32GameMode =u32GameMode;
		pIsp->u32DenoiseMode =u32DenoiseMode;
		pIsp->u32DenoiseManualValue[0] =u32DenoiseManualValue[0];
		pIsp->u32DenoiseManualValue[1] =u32DenoiseManualValue[1];
		pIsp->u32DenoiseManualValue[2] =u32DenoiseManualValue[2];
		pIsp->u32NightDenoiseManualValue= u32NightDenoiseManualValue;
		pIsp->u32SharpenAuto = u32SharpenAuto;

		pAllInfo->light_check_mode.change_day_night_mode = u32LightCheckMode;
		pAllInfo->light_check_mode.day_night_mode = u32day_night_mode;
		pAllInfo->light_check_mode.u32SystemGainMax = u32SystemGainMax;
		pAllInfo->light_check_mode.u32SlowFrame = u32SlowFrame;

		
		Hisi_set_ipcam_light_check_mode(&pAllInfo->light_check_mode);
		
		pAllInfo->isp_night_st = pAllInfo->isp_st;

		ret= cf_read_value(log_file_name,"u32SharpenAuto_Night",config_value);
		if( ret >= 0)	
			u32SharpenAuto = atox(config_value);

		pAllInfo->isp_night_st.u32SharpenAuto = u32SharpenAuto;

		ret= cf_read_value(log_file_name,"u32DenoiseMode_Night",config_value);
		if( ret >= 0)	
			u32DenoiseMode_Night = (unsigned char)atox(config_value);
		
		pAllInfo->isp_night_st.u32DenoiseMode = u32DenoiseMode_Night;

		ret= cf_read_value(log_file_name,"u32GameMode_Night",config_value);
		if( ret >= 0)	
			u32GameMode = (unsigned char)atox(config_value);
		pAllInfo->isp_night_st.u32GameMode = u32GameMode;

		ret= cf_read_value(log_file_name,"u8StrengthUdTarget_Night",config_value);
		if( ret >= 0)	
		{
			u8StrengthUdTarget = (unsigned char)atox(config_value);
			pAllInfo->isp_night_st.u8StrengthUdTarget = u8StrengthUdTarget;
		}

		ret= cf_read_value(log_file_name,"u8StrengthTarget_Night",config_value);
		if( ret >= 0)	
		{
			u8StrengthTarget = (unsigned char)atox(config_value);
			pAllInfo->isp_night_st.u8StrengthTarget = u8StrengthTarget;
		}	
		
		ret= cf_read_value(log_file_name,"u32LumaVal_Night",config_value);
		if( ret >= 0)
		{
			u32LumaVal = (unsigned short)atox(config_value);
			pAllInfo->isp_night_st.u32LumaVal = u32LumaVal;
		}


		ret= cf_read_value(log_file_name,"u32ContrVal_Night",config_value);
		if( ret >= 0)	
		{
			u32ContrVal = (unsigned short)atox(config_value);
			pAllInfo->isp_night_st.u32ContrVal = u32ContrVal;
		}

	

		ret= cf_read_value(log_file_name,"u32HueVal_Night",config_value);
		if( ret >= 0)	
		{
			u32HueVal = (unsigned short)atox(config_value);	
			pAllInfo->isp_night_st.u32HueVal = u32HueVal;
		}


		ret= cf_read_value(log_file_name,"u32SatuVal_Night",config_value);
		if( ret >= 0)	
		{
			u32SatuVal = (unsigned short)atox(config_value);	
			pAllInfo->isp_night_st.u32SatuVal = u32SatuVal;
		}
		

		if( pIsp->u32SharpenAuto )
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTargetOrigen,pIsp->u8StrengthMinOrigen,
			pIsp->u8StrengthTargetOrigen);
		else
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTarget,pIsp->u8StrengthMin,
			pIsp->u8StrengthTarget);
	}
	

	Hisi_set_isp_auto_image_quality_adjustment();
	
}


int Hisi_get_ipcam_config()
{
	char config_value[150] ="";
	char log_file_name[100] ="";
	int ret;
	int u32LightCheckMode = IPCAM_LIGHT_CONTROL_ADC_RESISTANCE_NORMAL_CHECK_AUTO;
	int u32ExposureMode = ISP_EXPOSURE_MODE_NORMAL;
	int u32SwitchExpTime = 100;
	int u32SwitchAvgLum = 200;
	int u32LimitExpTime = 220;
	int u32SystemGainMax = 0x0;
	int u8VpssTF_NUM = 0x01;
	int u32ShowFrameUseDay = 0x01;
	int u32ShowFrameWorkIndexNum = 0x03;
	int u32ShowFrameCloseIndexNum = 0x00;	
	int u32DenoiseAutoOpen=0x0; //自动降噪打开
	int u32DenoiseAutoOpen_Night=0x0; //夜视自动降噪打开
	int iUseSelfAutoSharpenValue=0x00;//自动锐度时用自己的值。
	int iSharpenAltD[8] = {0};  //自动锐度的8个值。
        int iSharpenAltUd[8] = {0}; //自动锐度的边缘锐度8个值。
	int u32OpenSelfDynamicBadPixel=0;  //动态坏点校政是否使用自己设置的值。
	int u32OpenSelfDynamicBadPixelAnalogGain=0; //动态坏点校正开启时，逻辑增益的值。
	int u16DynamicBadPixelSlope = 0x0; //动态坏点校正的强度
	int u16DynamicBadPixelThresh = 0x0; //检测动态坏点的门限值
	
	
	strcpy(log_file_name,CONFIG_FILE_ISP);

	ret= cf_read_value(log_file_name,"u32OpenSelfDynamicBadPixel",config_value);
	if( ret >= 0)
	{
		u32OpenSelfDynamicBadPixel = atox(config_value);		
	}

	ret= cf_read_value(log_file_name,"u32OpenSelfDynamicBadPixelAnalogGain",config_value);
	if( ret >= 0)
	{
		u32OpenSelfDynamicBadPixelAnalogGain = atoi(config_value);		
	}

	ret= cf_read_value(log_file_name,"u16DynamicBadPixelSlope",config_value);
	if( ret >= 0)
	{
		u16DynamicBadPixelSlope = atox(config_value);		
	}

	ret= cf_read_value(log_file_name,"u16DynamicBadPixelThresh",config_value);
	if( ret >= 0)
	{
		u16DynamicBadPixelThresh = atox(config_value);	

		set_DynamicBadPixel_value(u32OpenSelfDynamicBadPixel,
			u32OpenSelfDynamicBadPixelAnalogGain,u16DynamicBadPixelSlope,u16DynamicBadPixelThresh);
	}

	
	ret= cf_read_value(log_file_name,"u32DenoiseAutoOpen",config_value);
	if( ret >= 0)
	{
		u32DenoiseAutoOpen = atox(config_value);			
	}
	
	ret= cf_read_value(log_file_name,"u32DenoiseAutoOpen_Night",config_value);
	if( ret >= 0)
	{
		u32DenoiseAutoOpen_Night = atox(config_value);		
		set_denoise_auto_open(u32DenoiseAutoOpen,u32DenoiseAutoOpen_Night);
	}


		ret= cf_read_value(log_file_name,"iUseSelfAutoSharpenValue",config_value);
	if( ret >= 0)
	{
		iUseSelfAutoSharpenValue = atox(config_value);			
	}

	if( iUseSelfAutoSharpenValue == 1 )
	{
		ret= cf_read_value(log_file_name,"u8SharpenAltD",config_value);
		if( ret >= 0)
		{
			sscanf(config_value,"%d,%d,%d,%d,%d,%d,%d,%d",
				&iSharpenAltD[0],&iSharpenAltD[1],&iSharpenAltD[2],&iSharpenAltD[3],
				&iSharpenAltD[4],&iSharpenAltD[5],&iSharpenAltD[6],&iSharpenAltD[7]);
		}

		ret= cf_read_value(log_file_name,"u8SharpenAltUd",config_value);
		if( ret >= 0)
		{
			sscanf(config_value,"%d,%d,%d,%d,%d,%d,%d,%d",
				&iSharpenAltUd[0],&iSharpenAltUd[1],&iSharpenAltUd[2],&iSharpenAltUd[3],
				&iSharpenAltUd[4],&iSharpenAltUd[5],&iSharpenAltUd[6],&iSharpenAltUd[7]);
		}

		set_self_auto_sharpen_value(iUseSelfAutoSharpenValue,iSharpenAltD,iSharpenAltUd);
	}


	ret= cf_read_value(log_file_name,"u32ExposureMode",config_value);
	if( ret >= 0)
	{
		u32ExposureMode = atox(config_value);
		set_exposure_mode(u32ExposureMode);
	}

	ret= cf_read_value(log_file_name,"u32SwitchAvgLum",config_value);
	if( ret >= 0)
	{
		u32SwitchAvgLum = atox(config_value);
		set_switch_avg_lum(u32SwitchAvgLum);
	}

	ret= cf_read_value(log_file_name,"u32SwitchExpTime",config_value);
	if( ret >= 0)
	{
		u32SwitchExpTime = atox(config_value);
		set_switch_exp_time(u32SwitchExpTime);
	}

	ret= cf_read_value(log_file_name,"u32LimitExpTime",config_value);
	if( ret >= 0)
	{
		u32LimitExpTime = atox(config_value);
		set_exposure_limit_time(u32LimitExpTime);
	}

	ret= cf_read_value(log_file_name,"u32ShowFrameUseDay",config_value);
	if( ret >= 0)
	{
		u32ShowFrameUseDay = atox(config_value);		
	}

	ret= cf_read_value(log_file_name,"u32ShowFrameWorkIndexNum",config_value);
	if( ret >= 0)
	{
		u32ShowFrameWorkIndexNum = atox(config_value);	
	}
	
	ret= cf_read_value(log_file_name,"u32ShowFrameCloseIndexNum",config_value);
	if( ret >= 0)
	{
		u32ShowFrameCloseIndexNum = atox(config_value);		
	}

	set_show_frame_value(u32ShowFrameUseDay,u32ShowFrameWorkIndexNum,u32ShowFrameCloseIndexNum);

	return 1;
}



int Hisi_get_is_light_day()
{
	//DPRINTK("get_day_night_mode=%d\n",get_day_night_mode());

	if( get_day_night_mode() == IPCAM_DAY_NIGHT_MODE_AUTO )
		return IsLightDay;
	else if( get_day_night_mode() == IPCAM_LIGHT_CONTROL_DAY_ENFORCE )
		return 1;
	else if( get_day_night_mode() == IPCAM_LIGHT_CONTROL_NIGHT_ENFORCE )
		return 0;
	else
		return IsLightDay;
}


int  Hisi_set_isp_is_light_day(int value)
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	DPRINTK("value=%d\n",value);

	IsLightDay = value;
	pAllInfo->isp_st.u32IsLightDay = value;
	
	if( value == 1 )
	{		
		IPCAM_ISP_ST * pIsp = &pAllInfo->isp_st;
		Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_AEAttr);
		Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_vi_csc);
		
		if( pIsp->u32SharpenAuto )
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTargetOrigen,pIsp->u8StrengthMinOrigen,
			pIsp->u8StrengthTargetOrigen);
		else
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTarget,pIsp->u8StrengthMin,
			pIsp->u8StrengthTarget);

		{
			int value = 0;
			if(pIsp->u32DenoiseMode > 1 && pIsp->u32DenoiseMode < 5 )
			{
				value = pIsp->u32DenoiseManualValue[pIsp->u32DenoiseMode - 2];
			}
			Hisi_set_isp_SetDenoiseAttr(pIsp->u32DenoiseMode,value);			
		}

		Hisi_set_isp_is_gamma_mode(pIsp->u32GameMode);
		GK_Set_Day_Night_Params(value, pAllInfo->onvif_st.vcode[0].FrameRateLimit, pAllInfo->onvif_st.vcode[1].FrameRateLimit);
	}else if( value == 0 )
	{
		IPCAM_ALL_INFO_ST ipcam_st;
		IPCAM_ISP_ST * pIsp = &pAllInfo->isp_night_st;
		ipcam_st = *pAllInfo;
		ipcam_st.isp_st = ipcam_st.isp_night_st;
		Hisi_set_isp_value(&ipcam_st,IPCAM_ISP_CMD_AEAttr);
		Hisi_set_isp_value(&ipcam_st,IPCAM_ISP_CMD_vi_csc);

		if( pIsp->u32SharpenAuto )
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTargetOrigen,pIsp->u8StrengthMinOrigen,
			pIsp->u8StrengthTargetOrigen);
		else
			Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
			pIsp->u8StrengthUdTarget,pIsp->u8StrengthMin,
			pIsp->u8StrengthTarget);

		{
			int value = 0;
			if(pIsp->u32DenoiseMode > 1 && pIsp->u32DenoiseMode < 5 )
			{
				value = pIsp->u32DenoiseManualValue[pIsp->u32DenoiseMode - 2];
			}
			Hisi_set_isp_SetDenoiseAttr(pIsp->u32DenoiseMode,value);			
		}

		Hisi_set_isp_is_gamma_mode(pIsp->u32GameMode);
		GK_Set_Day_Night_Params(value, pAllInfo->onvif_st.vcode[0].FrameRateLimit, pAllInfo->onvif_st.vcode[1].FrameRateLimit);
	}else
	{
	}
	
	return 1;
}



int  Hisi_set_isp_is_gamma_mode(int mode)
{
	if( isp_init_ok == 0 )
		return 1;

	GammaMode = mode;

	DPRINTK("GammaMode=%d\n",GammaMode);

	Hisi_set_isp_auto_image_quality_adjustment();
	
	return 1;
}

int Hisi_set_ipcam_light_check_mode(IPCAM_DAY_NIGHT_MODE *  pMode)
{
	printf("Hisi_set_ipcam_light_check_mode\n");
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
	IPCAM_DAY_NIGHT_MODE * pLightMode =  &pAllInfo->light_check_mode;
	
	set_light_check_mode(pMode->change_day_night_mode);		
	pLightMode->change_day_night_mode = pMode->change_day_night_mode;
		
	set_day_night_mode(pMode->day_night_mode);
	pLightMode->day_night_mode = pMode->day_night_mode;

	Hisi_set_isp_max_gain(pMode->u32SystemGainMax);
	pLightMode->u32SystemGainMax = pMode->u32SystemGainMax;

	//Hisi_SetSlowFrameRate(pMode->u32SlowFrame);
	pLightMode->u32SlowFrame = pMode->u32SlowFrame;

	return 1;
}



int sys_set_date(int year, int month, int day)
{
	struct tm *tm;
	time_t now;
	unsigned char v2;

	now = time(NULL);
	tm = localtime(&now);

	year = (year>1900) ? year-1900 : 0;
	tm->tm_year = year;
	month = (month>0) ? month-1 : 0;
	tm->tm_mon = month;
	tm->tm_mday = day;

	if ((now = mktime(tm)) < 0)
		return -1;
	
	stime(&now);
	//system("hwclock -uw");

	return 0;
}

int sys_set_time(int hour, int min, int sec)
{
	struct tm *tm;
	time_t now;
	unsigned char v2;

	now = time(NULL);
	tm = localtime(&now);

	tm->tm_hour = hour;
	tm->tm_min = min;
	tm->tm_sec = sec + 3;

	if ((now = mktime(tm)) < 0)
		return -1;
	
	stime(&now);
	//system("hwclock -uw");

	return 0;
}


int Hisi_set_vpp_value(int chan,int value1,int value2,int value3)
{
	DPRINTK("in func\n");    
	return 1;
}




int Hisi_set_isp_value(IPCAM_ALL_INFO_ST * pIpcamInfo,int cmd)
{
	int ret = 1;
	int need_reboot = 0;
	int i = 0;
	IPCAM_ISP_ST * pIsp = &pIpcamInfo->isp_st;

	if( check_goke_is_work() == 0 )
	{
		DPRINTK("goke not run\n");
		return 0;
	}
	
	switch(cmd)
	{
		case IPCAM_ISP_CMD_DenoiseAttr:
			{
				int value = 0;
				if( pIsp->u32DenoiseMode > 1 && pIsp->u32DenoiseMode < 5 )
				{
					value = pIsp->u32DenoiseManualValue[pIsp->u32DenoiseMode - 2];
				}
				Hisi_set_isp_SetDenoiseAttr(pIsp->u32DenoiseMode,value);			
			}
			break;
		case IPCAM_ISP_CMD_saturation:			
			ret =Hisi_set_isp_saturation_value(pIsp->saturation);			
			break;
		case IPCAM_ISP_CMD_au16StaticWB:
			ret =Hisi_set_isp_SetAWBAttr(pIsp->au16StaticWB_0,pIsp->au16StaticWB_1,pIsp->au16StaticWB_2,pIsp->au16StaticWB_3);
			break;
		case IPCAM_ISP_CMD_AEAttr:
			ret =Hisi_set_isp_SetAEAttr(pIsp->u16ExpTimeMax,pIsp->u16ExpTimeMin,pIsp->u16AGainMax,
				pIsp->u16AGainMin,pIsp->u16DGainMax,pIsp->u16DGainMin,pIsp->u8ExpStep,pIsp->u8ExpCompensation,0);				

			if( g_backlight_mode !=  pIsp->u32BackLight )
			{
				if( pIsp->u32BackLight == 0)				
					Hisi_set_spi_antiflickerattr(get_cur_standard(),1);				
				else
					Hisi_set_spi_antiflickerattr(get_cur_standard(),0);

				g_backlight_mode = pIsp->u32BackLight;
			}			
			break;
		case IPCAM_ISP_CMD_vi_csc:
			ret =Hisi_set_vi_csc_attr(pIsp->u32LumaVal,pIsp->u32ContrVal,pIsp->u32HueVal,pIsp->u32SatuVal);			
			break;	
		case IPCAM_ISP_CMD_IS_LIGHT_DAY:			
			Hisi_set_isp_is_light_day(pIsp->u32IsLightDay);
			break;
		case IPCAM_ISP_CMD_SharpenAttr:					
			if( pIsp->u32SharpenAuto )
				Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
				pIsp->u8StrengthUdTargetOrigen,pIsp->u8StrengthMinOrigen,
				pIsp->u8StrengthTargetOrigen);
			else
				Hisi_set_isp_sharpen_value(pIsp->u32SharpenAuto, 
				pIsp->u8StrengthUdTarget,pIsp->u8StrengthMin,
				pIsp->u8StrengthTarget);
			break;
		case IPCAM_CODE_SET_PARAMETERS:
			{
				IPCAM_VCODE_VS_ST * pVCode = NULL;
				for( i = 0;i < 2; i++)
				{
					pVCode =&pIpcamInfo->onvif_st.vcode[i];
					
					if( get_cur_standard() ==TD_DRV_VIDEO_STARDARD_PAL )
						enc_d1[i].bitstream = pVCode->BitrateLimit*pVCode->FrameRateLimit/25;
					else
						enc_d1[i].bitstream = pVCode->BitrateLimit*pVCode->FrameRateLimit/30;					
					enc_d1[i].framerate = pVCode->FrameRateLimit;
					enc_d1[i].gop = pVCode->GovLength*pVCode->FrameRateLimit;

					Hisi_net_cam_enc_set(i,enc_d1[i].bitstream,enc_d1[i].framerate,enc_d1[i].gop);
				}
			}
			break;
		case IPCAM_NET_SET_PARAMETERS:
			{
				IPCAM_NET_ST * pNetSt = NULL;
				char ip[50] ="";
				char netmask[50] = "";
				char gw[50] ="";
				char dns1[50] ="";
				char dns2[50] = "";
				
				pNetSt = &pIpcamInfo->net_st;				

				sprintf(ip,"%d.%d.%d.%d",pNetSt->ipv4[0],pNetSt->ipv4[1],pNetSt->ipv4[2],pNetSt->ipv4[3]);
				sprintf(netmask,"%d.%d.%d.%d",pNetSt->netmask[0],pNetSt->netmask[1],pNetSt->netmask[2],pNetSt->netmask[3]);
				sprintf(gw,"%d.%d.%d.%d",pNetSt->gw[0],pNetSt->gw[1],pNetSt->gw[2],pNetSt->gw[3]);
				sprintf(dns1,"%d.%d.%d.%d",pNetSt->dns1[0],pNetSt->dns1[1],pNetSt->dns1[2],pNetSt->dns1[3]);
				sprintf(dns2,"%d.%d.%d.%d",pNetSt->dns2[0],pNetSt->dns2[1],pNetSt->dns2[2],pNetSt->dns2[3]);
				set_net_parameter(ip,pNetSt->serv_port,gw,netmask,dns1,dns2);
				net_set_write_dns_file(dns1,dns2);
				//printf("echo '' > /ramdisk/camsetDone");
			}
			break;
		case IPCAM_SYS_SET_PARAMETERS:
			{
				IPCAM_SYS_ST * pSysSt = NULL;				
				IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
				
				pSysSt = &pIpcamInfo->sys_st;				

				if( pSysSt->op_mode == IPCAM_SYS_GET_TIME )
				{
					int year,month,day,hour,minute,second;
					time_t time1;
					long long  ipcam_time;
					struct timeval now_time;
					gettimeofday( &now_time, NULL );
					time1= now_time.tv_sec;

				
					g_pstCommonParam->GST_FTC_time_tToCustomTime(time1,&year,&month,&day,&hour,&minute,&second);

					ipcam_time = (year+1900)*10000 + (month+1)*100 + day;
					ipcam_time = ipcam_time*1000000;
					ipcam_time = ipcam_time + hour*10000 + minute*100 + second;					
					pAllInfo->sys_st.ipcam_time = ipcam_time;					 
				}

				if( pSysSt->op_mode == IPCAM_SYS_SET_TIME )
				{
					time_t now;
					char cmd[100];
					int year,month,day,hour,minute,second;
					long long  ipcam_time;

					DPRINTK("IPCAM_SYS_SET_TIME set time %lld\n",pSysSt->ipcam_time);
					
					ipcam_time = pSysSt->ipcam_time / 1000000;
					year = ipcam_time / 10000;
					month = (ipcam_time%10000)/100;
					day = ipcam_time % 100;

					
					
					ipcam_time = pSysSt->ipcam_time % 1000000;
					hour = ipcam_time / 10000;
					minute = (ipcam_time%10000)/100;
					second = ipcam_time % 100;

					DPRINTK("%d_%d_%d %d:%d:%d\n",year,month,day,hour,minute,second);

					sprintf(cmd,"date %0.2d%0.2d%0.2d%0.2d%0.4d.%0.2d",month,day,hour,minute,year,second);
					SendM2(cmd);						
					
				}

				//if( pSysSt->op_mode == IPCAM_SYS_SET_stardard )
				{
					if(pAllInfo->sys_st.ipcam_stardard !=  pSysSt->ipcam_stardard )
					{
						pAllInfo->sys_st.ipcam_stardard = pSysSt->ipcam_stardard;
						save_ipcam_config_st(pAllInfo);	
						need_reboot = 1;
					}
				}

				if( pSysSt->op_mode == IPCAM_SYS_SET_USER )
				{					
					int change = 0;
					int i;

					for( i = 0 ; i < IPCAM_USER_NUM_MAX ; i++)
					{
						if( strcmp(pAllInfo->sys_st.ipcam_username[i],pSysSt->ipcam_username[i]) != 0 )
							change = 1;

						if( strcmp(pAllInfo->sys_st.ipcam_password[i],pSysSt->ipcam_password[i]) != 0 )
							change = 1;

						if( pAllInfo->sys_st.ipcam_user_mode[i] != pSysSt->ipcam_user_mode[i] )
							change = 1;

						if( change == 1 )
							break;
					}


					if( change == 1 )
					{
						pAllInfo->sys_st = *pSysSt;		
						save_ipcam_config_st(pAllInfo);	
					}
				}				

				pAllInfo->sys_st.op_mode = 0;				
			}
			break;		
		case IPCAM_ISP_SET_COLORMATRIX:
			Hisi_set_isp_colormatrix(pIpcamInfo);
			break;
		case IPCAM_ISP_SET_GAMMA_MODE:
			{					
				Hisi_set_isp_is_gamma_mode(pIsp->u32GameMode);				
			}
			break;		
		case IPCAM_ISP_SET_LIGHT_CHEKC_MODE:
			{					
				Hisi_set_ipcam_light_check_mode(&pIpcamInfo->light_check_mode);		
			}
			break;			
		default:
			DPRINTK("wrong cmd: %d\n",cmd);
			ret = -1;
			break;
	}

	if( need_reboot == 1 )
	{
		DPRINTK("need_reboot\n");
		SendM2("reboot");
	}

	return ret;
		
}



int Hisi_get_net_init_value(int load_default)
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
	IPCAM_NET_ST * pNetSt = &pAllInfo->net_st;
	pNetSt->net_dev = IPCAM_NET_DEVICE_LAN;
	pNetSt->ipv4[0] = 192;
	pNetSt->ipv4[1] = 168;
	pNetSt->ipv4[2] = 1;
	pNetSt->ipv4[3] = 60;

	pNetSt->netmask[0] = 255;
	pNetSt->netmask[1] = 255;
	pNetSt->netmask[2] = 255;
	pNetSt->netmask[3] = 0;

	pNetSt->gw[0] = 192;
	pNetSt->gw[1] = 168;
	pNetSt->gw[2] = 1;
	pNetSt->gw[3] = 1;

	pNetSt->dns1[0] = 192;
	pNetSt->dns1[1] = 168;
	pNetSt->dns1[2] = 1;
	pNetSt->dns1[3] = 1;

	pNetSt->dns2[0] = 9;
	pNetSt->dns2[1] = 9;
	pNetSt->dns2[2] = 9;
	pNetSt->dns2[3] = 9;	

	pNetSt->net_work_mode = IPCAM_IP_MODE;
	
}


int Hisi_get_sys_init_value(int load_default)
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
	IPCAM_SYS_ST * pSysSt = &pAllInfo->sys_st;
	int i = 0;

	pSysSt->op_mode = 0;
	pSysSt->ipcam_time = 0;

	for( i = 0; i < IPCAM_USER_NUM_MAX; i++)
	{
		pSysSt->ipcam_username[i][0] = 0;
		pSysSt->ipcam_password[i][0] = 0;
		pSysSt->ipcam_user_mode[i] = 0;
	}

	strcpy(pSysSt->ipcam_username[0],"admin");
	strcpy(pSysSt->ipcam_password[0],"admin");
	pSysSt->ipcam_user_mode[0] = IPCAM_USER_MODE_ADMIN;
		
}


void  Hisi_set_isp_light_dark_value(int value)
{
	int add_value = 0;
	
	if( value > 255 )
		value = 255;

	if( value < 0 )
		value = 0;

	add_value = get_light_check_mode()*1000;

	g_isp_light_dark_value = value + add_value;	
}

int Hisi_get_isp_light_dark_value()
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
	IPCAM_DAY_NIGHT_MODE * pLightMode =  &pAllInfo->light_check_mode;

	
	//printf("g_isp_light_dark_value = %d  %d\n",g_isp_light_dark_value,get_day_night_mode());

	
	if( get_day_night_mode() == IPCAM_LIGHT_CONTROL_DAY_ENFORCE )
	{
		return 1000*IPCAM_LIGHT_CONTROL_MANUAL_DAY_ENFORCE + 1;
	}else if( get_day_night_mode() == IPCAM_LIGHT_CONTROL_NIGHT_ENFORCE )
	{
		return 1000*IPCAM_LIGHT_CONTROL_MANUAL_NIGHT_ENFORCE + 254;
	}	

	return g_isp_light_dark_value;
}


