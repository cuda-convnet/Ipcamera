#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <linux/videodev.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
 #include <pthread.h>

#include "fmpeg4_avcodec.h"
#include "ratecontrol.h"
#include "fic8120ctrl.h"
#include "BaseData.h"
#include "bufmanage.h"
#include "audioctrl.h"
#include "netserver.h"
#include "fic8120osd.h"
#include "../systemcall/systemcall.h"
#include "P2pClient.h"
#include "Hisi_dev.h"
#include "buf_array.h"
#include "NetModule.h"
#include "GM_dev.h"
#include "nvr_ipcam_protocol.h"
#include "ipcam_config.h"
#include "config_file.h"
#include "share_mem.h"
#include "yun_svr.h"
#include "gk_dev.h"
#include "singly_linked_list.h"

#include "rjone_rtspserver.h"
#include "rjone_media_api.h"

#define OUT_FILE  "0_in.avi"
#define IN_FILE	  "0_out.avi"



//#define ENABLE_OUT_FILE
//#define SHOW_STAND_PAL
#define DEBUG

#ifndef  ENABLE_OUT_FILE
	#define ENABLE_IN_FILE
	#define DECODE_YUV
#endif

#define USE_AUDIO
#define NET_USE
#define USE_ONVIF
#define USE_YUN_SERVER
//#define USE_H264_RATE_CTRL
#define USE_MOTION_DETECT
#define USE_RTSP_SERVER

#define USE_DVR_SERVER_LOGIN

#define RJONE_RTSP_CHN0_URL   "0.ch"
#define RJONE_RTSP_CHN1_URL   "1.ch"
#define RJONE_RTSP_CHN2_URL   "2.ch"

#define FIC_VERSION "1.100"

//GST_DEV_FILE_PARAM  * g_pstcommand;

#define INIT_QUANT 25
#define MAX_QUANT  34


struct fcap_ext_buf_info {
	uint32_t   bg_width;
	uint32_t   bg_height;
	uint32_t   phy_addr[100];
	uint32_t   buf_size;
	uint8_t    nbuf;                      //number of buffer
	uint8_t    npending;                  //number of pending buffer
};

#define FCAP100_IOC_MAGIC  'h'

#define FCAP100_IOCSEXTBUF     _IOW(FCAP100_IOC_MAGIC, 2, struct fcap_ext_buf_info)

FILE * g_outfile = NULL;
FILE * g_infile = NULL;

extern NETSOCKET socket_ctrl;
RateControl     ratec;
video_profile   video_setting;
video_profile   video_net_setting;

unsigned char        * video_buf_virt = NULL;
unsigned char        * video_buf_virt_2 = NULL;
unsigned char 	   *undecode_video_buf = NULL;
unsigned char        * encode_mmp_addr = NULL;
unsigned char       *  dec_mmap_addr = NULL;
unsigned char   	* video_encode_buf = NULL;
unsigned char   	* video_encode_buf_2= NULL;
unsigned char 	* lcd_mmp_addr = NULL;
unsigned char 	*lcd_fb_mem = NULL;

int cap_id;
int fb_mode;
int frame_num = 0;
int frame_num_2 = 0;
int lcd_mmp_num = 0;
int g_encode_flag = 0;
int g_decode_flag = 0;
int all_encode_flag = 0;
int g_lcdflag = 0;
BUFLIST g_encodeList;
BUFLIST g_decodeList;
BUFLIST g_audioList;
BUFLIST g_audiodecodeList;
BUFLIST g_slavereclist;
BUFLIST g_slavenetlist;
BUFLIST g_othreinfolist;
int g_enable_rec_flag = 0;
int g_enable_net_local_view_flag = 0;
int g_enable_net_file_view_flag = 0;
int g_net_audio_open = 0;
int net_local_cam = 0x0f;
int g_enable_play_flag = 0;
int g_standard = TD_DRV_VIDEO_STARDARD_NTSC;
int rec_audio_flag = 0;
int play_audio_flag = 0;
static int net_th_flag = 0;
static int net_send_th_flag = 0;
static int buf_check_th_flag = 0;
LCDSHOWARRAY g_show_array;
int video_fd= 0;
int video_fd2=0;
int fmpeg4_enc_fd=0;
int fmpeg4_dec_fd=0;
int lcd_fd = 0;
int32_t RGB_Y_tab[256];
int32_t B_U_tab[256];
int32_t G_U_tab[256];
int32_t G_V_tab[256];
int32_t R_V_tab[256];
DECODEITEM g_decodeItem;
int synchronization = 0;
int show_bigger = 0;
//#define LOG_CAP         //capture for display , not use when encoding.
char * d1_store_buf[3] = {0};

int chan0_cap_num = 0;
int chan1_cap_num = 0;


ENCODE_ITEM enc_d1[MAX_CHANNEL*4];
ENCODE_ITEM enc_cif[MAX_CHANNEL*4];
ENCODE_ITEM enc_hd1[MAX_CHANNEL*4];
DECODE_ITEM dec_d1,dec_cif,dec_hd1;
DECODE_ITEM dd_dec[16];
ENCODE_ITEM enc_net_cif[MAX_CHANNEL*4];
ENCODE_ITEM enc_net_d1[MAX_CHANNEL*4];
ENCODE_ITEM enc_net_hd1[MAX_CHANNEL*4];
FRAME_RATE_CTRL enc_frame_rate_ctrl[MAX_CHANNEL*4];
FRAME_RATE_CTRL enc_frame_rate_ctrl_net[MAX_CHANNEL*4];

GM_ENCODE_ST gm_enc_st[MN_MAX_BIND_ENC_NUM] = {0};
int code_h264_mode[3] = {0};  // 0 = VBR , 1 = CBR.
int code_h264_profile[3] = {0};


int cur_standard = 0;
int cur_size = 0;
int app_select_num = 0;

int net_chan_send_count[MAX_CHANNEL*4];
int playback_first_keyframe[MAX_CHANNEL*4];
int screen_show_mode = 4;
int mult_play_cam[MAX_CHANNEL*4];

int cif_to_middle = 0;
int clear_show_fb = 0;

int isp_init_ok = 0;
int device_allready_init = 0;
int local_audio_play = 0;
int machine_start_net_test = 0;

const int lcd_menu_osd_fb_num = 5;

KEYBOARD_LED_INFO net_key_board;
int rec_type = TYPE_CIF;
int delay_sec_time = 0;
pthread_mutex_t cap_control_mutex;
int g_record_start_create_iframe = 0;
int g_net_start_create_iframe = 0;
int local_audio_play_cam = 0;

static int g_max_channel = 8;
static int g_chip_num = 1;
char sys_version[100];
int slave_net_send_data = 0;
int slave_rec_send_data = 0;
static int enc_send_data = 0;
static int g_video_hd_type = TD_DRV_VIDEO_SIZE_1080P;
static int g_sub_enc_control = 0;
NET_MODULE_ST * client_pst[MAX_CLIENT] = {0};
MEM_MODULE_ST *pMemModule[MAX_CLIENT] = {0};
int open_rstp_server = 0;
CYCLE_BUFFER * cycle_enc_buf = NULL;
CYCLE_BUFFER * cycle_net_buf = NULL;
int ipcam_use_id_mode = IPCAM_ID_MODE;
int user_id_mode_set_ip = 0;
char net_dev_name[50] = "eth0";
int printf_frame_info = 0;
int ipcam_use_common_protocol = 1;
int g_sensor_type = OMNI_OV9712_DC_720P_30FPS;
int g_chip_type = chip_unkown;
int g_is_running_shell = 0;
int g_is_update_now = 0;
int g_set_net_to_251 = 0;
int g_light_check_mode = IPCAM_LIGHT_CONTROL_ADC_RESISTANCE_NORMAL_CHECK_AUTO;
int g_day_night_mode = IPCAM_DAY_NIGHT_MODE_AUTO;
int g_exposure_mode = ISP_EXPOSURE_MODE_NORMAL; 
int g_limit_exposure_time = 220;
int g_switch_exp_time = 80;
int g_switch_avg_lum = 200;
int g_au8VpssTF_NUM = 1;
int g_u32ShowFrameUseDay = 0x01;
int g_u32ShowFrameWorkIndexNum = 0x03;
int g_u32ShowFrameCloseIndexNum = 0x00;
int g_u32DenoiseAutoOpen = 0x0; //自动降噪打开。
int g_u32DenoiseAutoOpen_Night = 0x0;  //夜视自动降噪打开。
int g_u32OpenSelfDynamicBadPixel=0;  //动态坏点校政是否使用自己设置的值。
int g_u32OpenSelfDynamicBadPixelAnalogGain=0;  //动态坏点校正开启时，逻辑增益的值。
int g_u16DynamicBadPixelSlope = 0x0;  //动态坏点校正的强度
int g_u16DynamicBadPixelThresh = 0x0;  //检测动态坏点的门限值
int g_iUseSelfAutoSharpenValue=0x00;//自动锐度时用自己的值。
int g_iSharpenAltD[8] = {0};   //自动锐度的8个值。
int g_iSharpenAltUd[8] = {0};  //自动锐度的边缘锐度8个值。
int g_auto_dhcp_over = 0;

int g_count_down_reboot_times = 0;

int g_is_dealwith_broadcast_ip = 0;
SINGLY_LINKED_LIST_INFO_ST gVideolist[MAX_VIDEO_STREAM_NUM];
SINGLY_LINKED_LIST_INFO_ST gThirdStreamlist;
SINGLY_LINKED_LIST_INFO_ST gAudiolist;
GK_MEDIA_ST g_ipc_media;



static int audio_speak_mode = NET_DVR_AUDIO_MODE_NET; //NET_DVR_AUDIO_MODE_LOCAL
static int audio_speak_cam = 0;
static int audio_speak_work = 0;
static int support_512k_buff = 1;

void Net_cam_send_data_block_check();
void Stop_GM_System();


void set_DynamicBadPixel_value(int u32OpenSelfDynamicBadPixel,int u32OpenSelfDynamicBadPixelAnalogGain,int u16DynamicBadPixelSlope,int u16DynamicBadPixelThresh)
{
	
	g_u16DynamicBadPixelSlope = u16DynamicBadPixelSlope;
	g_u16DynamicBadPixelThresh = u16DynamicBadPixelThresh;
	g_u32OpenSelfDynamicBadPixel = u32OpenSelfDynamicBadPixel;
	g_u32OpenSelfDynamicBadPixelAnalogGain = u32OpenSelfDynamicBadPixelAnalogGain;


	DPRINTK("g_u32OpenSelfDynamicBadPixel=%d g_u32OpenSelfDynamicBadPixelAnalogGain=%d g_u16DynamicBadPixelSlope=%d g_u16DynamicBadPixelThresh=%d\n",
		g_u32OpenSelfDynamicBadPixel,g_u32OpenSelfDynamicBadPixelAnalogGain,g_u16DynamicBadPixelSlope,g_u16DynamicBadPixelThresh);	
}


void get_DynamicBadPixel_value(int * u32OpenSelfDynamicBadPixel,int *u32OpenSelfDynamicBadPixelAnalogGain,int * u16DynamicBadPixelSlope,int * u16DynamicBadPixelThresh)
{
	*u32OpenSelfDynamicBadPixel = g_u32OpenSelfDynamicBadPixel;
	*u32OpenSelfDynamicBadPixelAnalogGain = g_u32OpenSelfDynamicBadPixelAnalogGain;
	*u16DynamicBadPixelSlope = g_u16DynamicBadPixelSlope;
	*u16DynamicBadPixelThresh = g_u16DynamicBadPixelThresh;	
}

void set_self_auto_sharpen_value(int iUseSelfAutoSharpenValue,int * p_iSharpenAltD,int * p_iSharpenAltUd)
{
	
	g_iUseSelfAutoSharpenValue = iUseSelfAutoSharpenValue;

	memcpy(g_iSharpenAltD,p_iSharpenAltD,8*sizeof(int));
	memcpy(g_iSharpenAltUd,p_iSharpenAltUd,8*sizeof(int));

	DPRINTK("g_iUseSelfAutoSharpenValue=%d\n",	g_iUseSelfAutoSharpenValue);	
}

void get_self_auto_sharpen_value(int * iUseSelfAutoSharpenValue,int * p_iSharpenAltD,int * p_iSharpenAltUd)
{
	
	*iUseSelfAutoSharpenValue = g_iUseSelfAutoSharpenValue;

	memcpy(p_iSharpenAltD,g_iSharpenAltD,8*sizeof(int));
	memcpy(p_iSharpenAltUd,g_iSharpenAltUd,8*sizeof(int));	
}


void set_denoise_auto_open(int u32DenoiseAutoOpen,int u32DenoiseAutoOpen_Night)
{
	g_u32DenoiseAutoOpen = u32DenoiseAutoOpen;
	g_u32DenoiseAutoOpen_Night = u32DenoiseAutoOpen_Night;

	DPRINTK("u32DenoiseAutoOpen=%d u32DenoiseAutoOpen_Night=%d \n",
		g_u32DenoiseAutoOpen,g_u32DenoiseAutoOpen_Night);
}


void get_denoise_auto_open(int * u32DenoiseAutoOpen,int * u32DenoiseAutoOpen_Night)
{
	*u32DenoiseAutoOpen = g_u32DenoiseAutoOpen;
	*u32DenoiseAutoOpen_Night = g_u32DenoiseAutoOpen_Night;	
}


void set_show_frame_value(int use_in_day,int start_index_num,int end_index_num)
{
	g_u32ShowFrameUseDay = use_in_day;
	g_u32ShowFrameWorkIndexNum = start_index_num;
	g_u32ShowFrameCloseIndexNum = end_index_num;

	DPRINTK("use_in_day=%d start_index_num=%d end_index_num=%d\n",
		g_u32ShowFrameUseDay,g_u32ShowFrameWorkIndexNum,g_u32ShowFrameCloseIndexNum);
}

int get_show_frame_value(int * use_in_day,int * start_index_num,int * end_index_num)
{
	*use_in_day = g_u32ShowFrameUseDay;
	*start_index_num = g_u32ShowFrameWorkIndexNum;
	*end_index_num = g_u32ShowFrameCloseIndexNum;	
	return 1;
}

void  set_vpsstf_num(int num)
{
	g_au8VpssTF_NUM = num;
	DPRINTK("g_au8VpssTF_NUM=%d\n",num);	
}

int  get_vpsstf_num()
{
	return g_au8VpssTF_NUM;
}

unsigned char get_slow_frame_num()
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	return pAllInfo->light_check_mode.u32SlowFrame;
}

unsigned int get_SystemGainMax()
{
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	return pAllInfo->light_check_mode.u32SystemGainMax;
}

void  set_exposure_mode(int flag)
{
	g_exposure_mode = flag;
	DPRINTK("g_exposure_mode=%d\n",g_exposure_mode);	
}

int  get_exposure_mode()
{
	return g_exposure_mode;
}

void  set_exposure_limit_time(int time)
{
	g_limit_exposure_time = time;
	DPRINTK("g_limit_exposure_time=%d\n",g_limit_exposure_time);	
}

int get_exposure_limit_time()
{
	return g_limit_exposure_time;
}

void  set_switch_exp_time(int time)
{
	g_switch_exp_time = time;
	DPRINTK("g_switch_exp_time=%d\n",g_switch_exp_time);	
}

int get_switch_exp_time()
{
	return g_switch_exp_time;
}

void  set_switch_avg_lum(int time)
{
	g_switch_avg_lum = time;
	DPRINTK("g_switch_avg_lum=%d\n",g_switch_avg_lum);	
}

int get_switch_avg_lum()
{
	return g_switch_avg_lum;
}

void  set_light_check_mode(int flag)
{
	g_light_check_mode = flag;
	DPRINTK("g_light_check_mode=%d\n",g_light_check_mode);	
}

int get_light_check_mode()
{
	return g_light_check_mode;
}

void  set_day_night_mode(int flag)
{
	g_day_night_mode = flag;
	printf("g_day_night_mode=%d\n",g_day_night_mode);	
}

int get_day_night_mode()
{
	return g_day_night_mode;
}

int get_ipcam_id_mode_set_ip()
{
	return user_id_mode_set_ip;
}

int  Hisi_is_set_net_to_251()
{
	return g_set_net_to_251;
}


int Hisi_get_sensor_type()
{


	return g_sensor_type;
}

int Hisi_get_chip_type()
{
	return g_chip_type;
}

int Hisi_get_encode_type(int chn)
{
	if( chn < 0 || chn >=g_ipc_media.stream_num )
	{
		DPRINTK("chn = %d is error!\n",chn);
		return -1;
	}
	return g_ipc_media.stMediaStream[chn].code;
}

int get_ipcam_mode_num()
{
	return ipcam_use_id_mode;
}


int ipcam_set_use_common_protocol()
{
	ipcam_use_common_protocol = 1;
	return 1;
}

int ipcam_get_use_common_protocol_flag()
{
	return ipcam_use_common_protocol;
}

void  Net_cam_set_support_512K_buf(int flag)
{
	if( flag ==  1)
		support_512k_buff = 1;
	else
		support_512k_buff = 0;
}

int Net_cam_get_support_512K_buf_flag()
{
	return support_512k_buff;
}


int Net_cam_is_open_rstp_server()
{
	return open_rstp_server;
}

int Net_cam_get_sub_enc_control()
{
	return g_sub_enc_control;
}

int Net_cam_set_support_max_video(int videotype)
{
	DPRINTK("videotype = %d\n ",videotype);

	if( g_video_hd_type >TD_DRV_VIDEO_SIZE_960P ||
		g_video_hd_type < TD_DRV_VIDEO_SIZE_D1 )
	{
		DPRINTK("wrong ! set default\n");
		g_video_hd_type = TD_DRV_VIDEO_SIZE_D1;
	}else
	{
		DPRINTK("set ok\n");
		g_video_hd_type = videotype;
	}

	return 1;	
}

int Net_cam_get_support_max_video()
{
	return g_video_hd_type;
}

int machine_use_audio()
{
	if(local_audio_play == 1)
		return 1;

	if( g_decodeItem.enable_play == 1 )
		return 1;

	return 0;
}


int get_dhcp_status()
{
	return g_auto_dhcp_over;
}

//----------------------------------save net cfg start---------------
#define NET_CFG_FILENAME "/mnt/mtd/net_cfg.dat"
int net_cfg_save(unsigned char *ipaddress,unsigned char *gateway,unsigned char *netmask,unsigned char *ddns1,unsigned char *ddns2,unsigned char *Wififlag)
{
	int ret;
	FILE *fp;
	
	fp = fopen(NET_CFG_FILENAME, "w + b");
	if(fp == NULL)
	{
		printf("open %s fail \n",NET_CFG_FILENAME);
		return -1;
	}
	ret = fwrite(ipaddress, 1, 4, fp);
	if(ret != 4)
		goto net_save_err;
	ret = fwrite(gateway, 1, 4, fp);
	if(ret != 4)
		goto net_save_err;
	ret = fwrite(netmask, 1, 4, fp);
	if(ret != 4)
		goto net_save_err;
	ret = fwrite(ddns1, 1, 4, fp);
	if(ret != 4)
		goto net_save_err;
	ret = fwrite(ddns2, 1, 4, fp);
	if(ret != 4)
		goto net_save_err;
	ret = fwrite(Wififlag, 1, 1, fp);
	if(ret != 1)
		goto net_save_err;
	
	fclose(fp);
	

	{
			IPCAM_ALL_INFO_ST * pAllInfo = get_ipcam_config_st();
			IPCAM_NET_ST * pNetSt = &pAllInfo->net_st;
			pNetSt->ipv4[0]=ipaddress[0];
			pNetSt->ipv4[1]=ipaddress[1];
			pNetSt->ipv4[2]=ipaddress[2];
			pNetSt->ipv4[3]=ipaddress[3];
			
			pNetSt->gw[0]=gateway[0];
			pNetSt->gw[1]=gateway[1];
			pNetSt->gw[2]=gateway[2];
			pNetSt->gw[3]=gateway[3];
			
			pNetSt->netmask[0]=netmask[0];
			pNetSt->netmask[1]=netmask[1];
			pNetSt->netmask[2]=netmask[2];
			pNetSt->netmask[3]=netmask[3];
			
			pNetSt->dns1[0]=ddns1[0];
			pNetSt->dns1[1]=ddns1[1];
			pNetSt->dns1[2]=ddns1[2];
			pNetSt->dns1[3]=ddns1[3];
			
			pNetSt->dns2[0]=ddns2[0];
			pNetSt->dns2[1]=ddns2[1];
			pNetSt->dns2[2]=ddns2[2];
			pNetSt->dns2[3]=ddns2[3];
						
			save_ipcam_config_st(pAllInfo);						
	}
	return 0;
	
net_save_err:
	printf("net_cfg_save error\n");
	fclose(fp);	
	return -1;
}
int net_cfg_load(unsigned char *ipaddress,unsigned char *gateway,unsigned char *netmask,unsigned char *ddns1,unsigned char *ddns2,unsigned char *Wififlag)
{
	int ret;
	FILE *fp;
	char buf[100];
  int flag=0;
  
retry:
	flag++;
		
	fp = fopen(NET_CFG_FILENAME, "r + b");
	if(fp==NULL)
	{
		fp = fopen(NET_CFG_FILENAME, "w + b");
		if(fp==NULL)
		{
			printf("*************open net cfg file fail*************\n");
			return -1;
		}
		fclose(fp);
		
		ipaddress[0]=192;
		ipaddress[1]=168;
		ipaddress[2]=1;
		ipaddress[3]=60;
		
		gateway[0]=192;
		gateway[1]=168;
		gateway[2]=1;
		gateway[3]=1;
		
		netmask[0]=255;
		netmask[1]=255;
		netmask[2]=255;
		netmask[3]=0;
		
		ddns1[0]=202;
		ddns1[1]=96;
		ddns1[2]=134;
		ddns1[3]=133;
		
		ddns2[0]=202;
		ddns2[1]=96;
		ddns2[2]=128;
		ddns2[3]=166;
		
		Wififlag[0]=1;
		
		net_cfg_save(ipaddress,gateway,netmask,ddns1,ddns2,Wififlag);
	}
	else
	{
		ret = fread(ipaddress, 1, 4, fp);
		if(ret!=4)
			goto net_load_err;
		ret = fread(gateway, 1, 4, fp);
		if(ret!=4)
			goto net_load_err;
		ret = fread(netmask, 1, 4, fp);
		if(ret!=4)
			goto net_load_err;
		ret = fread(ddns1, 1, 4, fp);
		if(ret!=4)
			goto net_load_err;
		ret = fread(ddns2, 1, 4, fp);
		if(ret!=4)
			goto net_load_err;
		ret = fread(Wififlag, 1, 1, fp);
		if(ret!=1)
			goto net_load_err;
		fclose(fp);
	}
	
	return 0;
	
net_load_err:
	if(flag<3)
	{
		fclose(fp);
		memset(buf,0,sizeof(buf));
		sprintf(buf,"rm -f %s",NET_CFG_FILENAME);
		//system(buf);
		command_process(buf);
		goto retry;
	}
	else
		fclose(fp);
	
	return -1;
}

int saveipmenu(char *ipaddress,char *gateway,char *netmask,char *ddns1,char *ddns2)
{
	printf("save ok!\n");

	int i = 0, a[4];
	unsigned char Wififlag = 0;
	unsigned char uIp[16], uMsk[16], uGw[16], uDns1[16], uDns2[16];
	
	net_cfg_load(uIp,uGw,uMsk,uDns1,uDns2,&Wififlag);

	sscanf(ipaddress,"%3d.%3d.%3d.%3d",&a[0],&a[1],&a[2],&a[3]);	
		for (i=0;i<4;i++)
			uIp[i] = a[i];
		sscanf(gateway,"%3d.%3d.%3d.%3d",&a[0],&a[1],&a[2],&a[3]);	
		for (i=0;i<4;i++)
			uGw[i] = a[i];
		sscanf(netmask,"%3d.%3d.%3d.%3d",&a[0],&a[1],&a[2],&a[3]);	
		for (i=0;i<4;i++)
			uMsk[i] = a[i];
		sscanf(ddns1,"%3d.%3d.%3d.%3d",&a[0],&a[1],&a[2],&a[3]);	
		for (i=0;i<4;i++)
			uDns1[i] = a[i];
		sscanf(ddns2,"%3d.%3d.%3d.%3d",&a[0],&a[1],&a[2],&a[3]);	
		for (i=0;i<4;i++)
			uDns2[i] = a[i];
/*
{
	U8 tmp[2],tmp1[2];
	tmp[0]=NetStaticMenu.Port>>8;
	tmp[1]=NetStaticMenu.Port;
	tmp1[0]=NetStaticMenu.IEPort>>8;
	tmp1[1]=NetStaticMenu.IEPort;
	I_write_block_eeprom(ADDNSTC,sizeof(NetStaticMenu),&NetStaticMenu.IPAlloction);


	I_write_block_eeprom(ADDNSTC+PORT,2,tmp);
	I_write_block_eeprom(ADDNSTC+IEPORT,2,tmp1);
	I_write_block_eeprom(ADDNSTC+DHCPIP,4,NetStaticMenu.IpAddress);
	I_write_block_eeprom(ADDNSTC+DHCPGATEWAY,4,NetStaticMenu.GateWay);
	I_write_block_eeprom(ADDNSTC+DHCPDNS1,4,NetStaticMenu.DDNS1);
	I_write_block_eeprom(ADDNSTC+DHCPDNS2,4,NetStaticMenu.DDNS2);
}*/

	if(net_cfg_save(uIp,uGw,uMsk,uDns1,uDns2,&Wififlag)<0)
	{
		printf("******net_cfg_save false ********\n");
	}
}

//----------------------------------save net cfg end---------------

void ip_set_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	char ipcam_area[200];
	char ipcam_area_head[200];
	char ipcam_gw[200];
	int i;
	int find_use_ip = 0;
	unsigned char host_ip[4] = {0};
	int random_num = 0;
	char server_ip[100] = "";
	char dvr_id[100] ={0};
	static int first_time = 1;

	if( ipcam_get_host_ip(net_dev_name,host_ip) < 0 )
	{
		DPRINTK("ipcam_get_host_ip get error!\n");
		user_id_mode_set_ip = 1;
		return ;
	}

	if (g_is_dealwith_broadcast_ip == 1)
	{
		DPRINTK("\nsys is dealwith IP broadcast now! drop this!\n");
		return;
	}

	g_is_dealwith_broadcast_ip = 1;

/*
	if( first_time == 1  )
	{
		get_svr_mac_id(server_ip,dvr_id);

		if( dvr_id[0] == 0 )
		{
		}else
		{
			host_ip[2] = 8;
			DPRINTK("set ip area 8\n");
		}

		first_time = 0;
	}
	*/

	sprintf(ipcam_area_head,"%d.%d.%d.",host_ip[0],host_ip[1],host_ip[2]);
	
	random_num = get_mac_random();

	random_num = 20 + random_num%179;

	DPRINTK("ipcam_area_head = %s  random_num=%d\n",ipcam_area_head,random_num);
	
	for( i = random_num; i > 20; i--)
	{
		if( i == 60 )
			continue;
	
		sprintf(ipcam_area,"%s%d",ipcam_area_head,i);
		
		if( check_ip_attack(net_dev_name,ipcam_area) == 1 )
		{
			find_use_ip = 1;
			break;
		}
	}

	if( find_use_ip == 0 )
	{
		for( i = 219; i > random_num; i--)
		{
			if( i == 60 )
				continue;
		
			sprintf(ipcam_area,"%s%d",ipcam_area_head,i);
			
			if( check_ip_attack(net_dev_name,ipcam_area) == 1 )
			{
				find_use_ip = 1;
				break;
			}
		}
	}


	if( find_use_ip ==  1)
	{
		DPRINTK("find avalibe ip %s\n",ipcam_area);
		sprintf(ipcam_gw,"%s1",ipcam_area_head);
		//set_net_parameter_dev(net_dev_name,ipcam_area,6802,ipcam_gw,
		//"255.255.255.0",ipcam_gw,ipcam_gw);
		saveipmenu(ipcam_area,ipcam_gw,"255.255.255.0",ipcam_gw,ipcam_gw);
		printf("echo '' > /ramdisk/camsetDone");
	}

	user_id_mode_set_ip = 1;

	sleep(5);
	g_is_dealwith_broadcast_ip=0;

	pthread_detach(pthread_self());

	DPRINTK("out \n");
}


int  start_id_mode_ip_set()
{
	pthread_t id_ip_set;
	int ret;
	struct timeval this_time;
	static struct timeval last_set_time;

	get_sys_time( &this_time, NULL );

	DPRINTK("this:%ld - last:%ld = %ld",this_time.tv_sec,last_set_time.tv_sec,
		abs(this_time.tv_sec - last_set_time.tv_sec));
	
	if( abs(this_time.tv_sec - last_set_time.tv_sec) < 5 )
	{
		DPRINTK("time is short 5 sec,not set\n");
		return -1;
	}	

#if 1
	ret = pthread_create(&id_ip_set,NULL,(void*)ip_set_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create ip_set_thread  error\n");		
	}	
#endif

	last_set_time = this_time;

	return 1;
	
}


void set_ipcam_mode(int flag,char * pnet_dev)
{

	if( flag == 999999 )
	{
		user_id_mode_set_ip = 0;
		ipcam_use_id_mode = IPCAM_ID_MODE;
		return ;
	}

	if( flag == IPCAM_IP_MODE)
		ipcam_use_id_mode = IPCAM_IP_MODE;
	else
		ipcam_use_id_mode = IPCAM_ID_MODE;
	
	strcpy(net_dev_name,pnet_dev);
	DPRINTK("ipcam_use_id_mode=%d net_dev_name=%s\n",
		ipcam_use_id_mode,net_dev_name);

	if( ipcam_use_id_mode == IPCAM_ID_MODE  )
	{		
		//start_id_mode_ip_set();	

		user_id_mode_set_ip = 1;
	}

	
}

int get_net_dev_name(char * dev_name)
{
	strcpy(dev_name,net_dev_name);
}

void Net_dvr_adjust_d1_enc(int net_flag)
{
	static int rec_gop = -1;

	if( Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_D1 )
	{
		if( net_flag == 1 )
		{
			if( rec_gop == -1 )
			{			
				rec_gop = enc_d1[0].gop;
				enc_d1[0].gop = enc_d1[0].framerate /2;		

				DPRINTK("set chan 0 gop = %d\n",enc_d1[0].gop);

				
			}
		}else
		{
			if( rec_gop != -1 )
			{
				enc_d1[0].gop = rec_gop;
				rec_gop = -1;
				DPRINTK("restore chan 0 gop = %d\n",enc_d1[0].gop);
				DPRINTK("change (%d,%d,%d) \n",
					enc_d1[0].bitstream,enc_d1[0].framerate,enc_d1[0].gop);
			}
		}
	}else
	{
		if( rec_gop != -1 )
		{
			enc_d1[0].gop = rec_gop;
			rec_gop = -1;
			DPRINTK("restore chan 0 gop = %d\n",enc_d1[0].gop);
			DPRINTK("change (%d,%d,%d) \n",
					enc_d1[0].bitstream,enc_d1[0].framerate,enc_d1[0].gop);
		}
	}
}


int net_send_calculate(int cam,int send_success)
{
	static int send_count[2] = {0};
	static int error_count = 0;
	unsigned int net_cam = 0;
	int value;

	net_cam = get_net_local_cam();

	if( get_dvr_max_chan(1) < 16 )	
		return 1;


	error_count++;
	if( error_count >= 1000 )
	{
		error_count = 0;
		
		send_count[0] = 0;
		send_count[1] = 0;
	}

	if( send_success == 1 )
	{
		send_count[cam]++;
		return 1;
	}

	value =  net_cam & 0x00ff;	
	if( value == 0 )	
		send_count[0] = send_count[1];
	

	value =  net_cam & 0xff00;	
	if( value == 0 )	
		send_count[1] = send_count[0];

	if( (send_count[0] > 1000 ) ||
			(send_count[1] > 1000 ) )
	{
		send_count[0] = 0;
		send_count[1] = 0;
	}
	


	if( cam == 0 )	
	{
		net_cam = net_cam & 0x00ff;	

		if( net_cam !=  0 )
		{
			value = send_count[0] -send_count[1];
			
			if( value >=2 )
			{
				return 0;
			}
			else
			{
				//send_count[0]++;
				return 1;
			}
		}
		
	}else
	{
		net_cam = net_cam & 0xff00;

		if( net_cam !=  0 )
		{
			value = send_count[1] -send_count[0];
			
			if( value >=2 )
			{
				return 0;
			}
			else
			{
				//send_count[1]++;
				return 1;
			}
		}

	}

	return 0;
	
}


int get_dvr_max_chan(int use_chip_flag)
{
	if( use_chip_flag == 0 )
		return g_max_channel;
	else
		return g_max_channel*g_chip_num;
}

int get_dvr_chip_num()
{
	return g_chip_num;
}

int div_safe2(float num1,float num2)
{
	if( num2 == 0 )
	{
		DPRINTK("num2=%f\n",num2);
		return -1;
	}

	return (int)(num1/num2);
}

float div_safe3(float num1,float num2)
{
	if( num2 == 0 )
	{
		DPRINTK("num2=%f\n",num2);
		return -1;
	}

	return (float)(num1/num2);
}


int is_16ch_host_machine()
{
	if( (get_dvr_max_chan(1)==16) && (Hisi_is_slave_chip()== 0) )
		return 1;
	else
		return 0;
}

int is_16ch_slave_machine()
{
	if((get_dvr_max_chan(1)==8) && (Hisi_is_slave_chip()== 1)  )
		return 1;
	else	
		return 0;	
}

void set_dvr_max_chan(int chan)
{
	static int first_set = 0;

	if( first_set != 0 )
	{
		DPRINTK("already set g_max_channel=%d\n",g_max_channel);
		return;
	}

	g_max_channel = chan;

	 //16路dvr上有2个芯片，4，8路上一个芯片

	if( chan > 8 )
	{
		g_chip_num = chan / 8;
		g_max_channel = 8;
	}
	

	first_set = 1;

	DPRINTK("g_max_channel=%d g_chip_num=%d\n",g_max_channel,g_chip_num);
}

char * get_dvr_version()
{

	sprintf(sys_version,"%s %s",__DATE__,__TIME__);
	
	return sys_version;
}

int Net_cam_is_960p_ipcam()
{
	if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_960P )
		return 1;
	else
		return 0;
}



void net_chan_send_count_add(int index)
{
	net_chan_send_count[index]++;
}


void get_cur_img_x_y(int * width,int * height)
{
	   if(  cur_standard == TD_DRV_VIDEO_STARDARD_PAL)
	   {	
		    *width = CAP_WIDTH;
		    *height = CAP_HEIGHT_PAL;
		    
	   }else
	   {
	   	    *width = CAP_WIDTH;
		    *height = CAP_HEIGHT_NTSC;		    
	   }
}

int process_send_command(char * cmd)
{
	FILE * fp = NULL;
	int rel;
	int sleep_time;
	long fileOffset = 0;	
	char cmd_buf[200];
	int  count = 0;

	return SendM2(cmd);


	fileOffset = strlen(cmd);

	if( fileOffset > 199 )
	{
		printf(" command %s is too long!\n",cmd);
	}

LAST_COMMAD_NOT_RUN:	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);
		count++;
		if( count > 10 )
		{
			printf("LAST_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto LAST_COMMAD_NOT_RUN;
	}
	

	fp = fopen(COMMAND_FILE,"wb");
	if( fp == NULL)
		return -1;

	strcpy(cmd_buf,cmd);

	rel = fwrite(cmd_buf,1,fileOffset+1,fp);

	if( rel != fileOffset+1)
	{
		DPRINTK("fwrite error!\n");
		fclose(fp);
		fp = NULL;
		return -1;
	}


	fclose(fp);
	fp = NULL;

	//等待命令执行程序处理
	sleep_time = SLEEP_TIME*2;	
	usleep(sleep_time);

END_COMMAD_NOT_RUN:	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);
		count++;

		if( count > 10 )
		{
			printf("END_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto END_COMMAD_NOT_RUN;
	}


	fp = fopen(COMMAND_FAILD_FILE,"rb");
	if( fp != NULL )
	{
		fclose(fp);
		fp = NULL;
		return -1;
	}

	return 1;
	
}


unsigned short *  get_motion_buf_hisi(int chan)
{
	ENCODE_ITEM * enc_item = NULL;
//	int i = 0;
	int ret;
	static int show_count = 0;
	
	enc_item = &enc_d1[chan];
	
	ret = Hisi_get_motion_buf( enc_item);
	if( ret < 0 )
		return NULL;

if(1)
{
	int y,x;
	unsigned short * motion_buf;
	int  move_item_count = 0;

	motion_buf = enc_item->motion_buf;
	for( y =0; y < 30;y++)
	{						
		for(x=0; x < 40; x++ )
		{
			if(motion_buf[x+y*40] > 40 )
			{
				//printf("(%d,%d)=%d\n",x,y,motion_buf[x+y*40] );
				move_item_count++;

				//printf("%02x ",motion_buf[x+y*40]);
			}
		}

		//printf("\n");
	}

	//if( move_item_count > 0 )
	{
		show_count++;
		if( show_count % 360 == 0 )
		DPRINTK("move block num=%d\n",move_item_count);
	}

}
	return enc_item->motion_buf;
}


int reduce_frame_show(int chan)
{
	static int count[16];
	int a,b;

	g_pstcommand->GST_FTC_GetPlayCurrentStauts(&a,&b);

	if( a == PLAYPAUSE )
		return 1;

	count[chan]++;

	if(  count[chan] > 3000 )
		count[chan] = 0;

	if( count[chan] % 3 == 0 )
		return 1;
	else
		return 0;
}


int fic_get_bit(int iCam, int iBitSite)
{
	iCam = iCam >> iBitSite;

	iCam = iCam & 0x01;

	return iCam;
}

void set_mult_play_cam(int cam)
{
	int i;
	int num = 0;

	for( i = 0; i < 16; i++ ){
		
		mult_play_cam[i] = -1;
	}
	

	for( i = 0; i < 16; i++ )	{
		
		if(  fic_get_bit(cam,i) ){
			
			mult_play_cam[num] = i;
			num++;

			if( num >=16)
				break;
		}
	}

/*	if( num > 1 )	
	{
		set_screen_show_mode(4);
		scaler_d1_to_d1();
	}
	else
	{
		set_screen_show_mode(1);
		scaler_cif_to_d1();
	}
	*/
}


#define FFB_IOC_MAGIC  'c'
#define FFB_INPUT_RES         _IOW(FFB_IOC_MAGIC, 6, unsigned int)
//FLCDS_INPUT_RES
#define TVE_INPUT_RES_CIF 0
#define TVE_INPUT_RES_VGA 1
#define TVE_INPUT_RES_D1  2

int scaler_cif_to_d1()
{
	int i = TVE_INPUT_RES_CIF;

	if(  cur_size == TD_DRV_VIDEO_SIZE_CIF )
	{
		if (ioctl(lcd_fd,FFB_INPUT_RES,&i) < 0)
		{	printf("TVE Error: Switch failed!\n");	
			return -1;
		}
	}

	printf("tve cif to d1\n");

	return 1;
}

int scaler_d1_to_d1()
{
	int i = TVE_INPUT_RES_D1;

	if(  cur_size == TD_DRV_VIDEO_SIZE_CIF )
	{
		if (ioctl(lcd_fd,FFB_INPUT_RES,&i) < 0)
		{	printf("TVE Error: Switch failed!\n");	
			return -1;
		}
	}

	printf("tve d1 to d1\n");

	return 1;
}

char * get_d1_combine_buf(char *  buf_save,char * dest_buf)
{
	int width,height;
//	int mode;

	if( cur_size!= TD_DRV_VIDEO_SIZE_D1 )
	{
		printf(" mode is error!it is not D1 mode1\n");
		return NULL;
	}	

	width = CAP_WIDTH;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_NTSC )
		height = CAP_HEIGHT_NTSC;
	else
		height = CAP_HEIGHT_PAL;

/*	memcpy(	d1_store_buf[2],dest_buf+width*height,width*height);
	memcpy(dest_buf+width*height,buf_save,width*height); 
	memcpy(buf_save,d1_store_buf[2],width*height); 	*/  

	memcpy(	d1_store_buf[2],dest_buf+width*height/2,width*height/2);
	memcpy(	d1_store_buf[2]+width*height/2,dest_buf+width*height + width*height /4,width*height/4);
	memcpy(dest_buf+width*height/2,buf_save,width*height/2); 
	memcpy(dest_buf+width*height + width*height /4,buf_save+width*height/2,width*height/4); 
	memcpy(buf_save,d1_store_buf[2],width*height*3/4); 	

//	memset(dest_buf+width*height/2,0x10,width*height/2); 
//	memset(dest_buf+width*height + width*height /4,0x80,width*height/4); 

	return dest_buf;
}

int get_mult_play_cam(int chan)
{
	int i;

	for( i = 0; i < 16; i++ ){
		
		if( mult_play_cam[i] == chan )
			return i;
	}

	return 0;
}

void set_screen_show_mode(int mode )
{
	switch(mode){
		
		case 1: screen_show_mode=1;break;
		case 4: screen_show_mode=4;break;
		case 9: screen_show_mode=9;break;
		case 16: screen_show_mode=16;break;
		default: screen_show_mode = 1; break;
	}	
	
}

int get_screen_show_mode()
{
	return screen_show_mode;
}

void set_play_back_first_keyframe(int chan,int value)
{
	if( chan < 0 || chan > 16 )
		return;

	if( value == 0 )
		playback_first_keyframe[chan] = 0;
	else
		playback_first_keyframe[chan] = 1;
}

int get_play_back_first_keyframe_status(int chan)
{
	if( chan < 0 || chan > 16 )
		return -1;

	return playback_first_keyframe[chan];
}

void clear_playback_keyframe()
{
	int i;

	for( i=0 ; i < 16; i++ )
	{
		playback_first_keyframe[i] = 0;
	}
}


int reset_tw2835()
{
//	int usbflag;

/*	g_pstcommand->gst_ext_read_fic(0x91500000,&usbflag);
	usbflag |= 0x00100000;
	g_pstcommand->gst_ext_write_fic(0x91500000,usbflag);

	g_pstcommand->gst_ext_read_fic(0x91600000,&usbflag);
	usbflag |= 0x00100000;
	g_pstcommand->gst_ext_write_fic(0x91600000,usbflag);
*/

	return 1;
}

int open_net_audio(int flag)
{
	printf("g_net_audio_open = %d\n",flag);
	g_net_audio_open = flag;

	return 1;
}



int set_delay_sec_time(int sec)
{
	delay_sec_time = sec;

	printf("set delay time =%d\n",delay_sec_time);

	return 1;
}

int get_delay_sec_time()
{
	return delay_sec_time;
}

int get_sys_time(struct timeval *tv,char * p)
{
	gettimeofday( tv, NULL );

	//printf("time = %d delay time %d ",tv->tv_sec, get_delay_sec_time());

	tv->tv_sec += get_delay_sec_time();

	//printf("change time %d\n",tv->tv_sec);

	
	return 1;
}

int set_rec_type(int type)
{
	rec_type = type;

	return rec_type;
}

int show_cif_to_d1_size(int flag)
{
	show_bigger = flag;
	clear_fb();

	return 1;
}



int command_drv2(char * command)
{	
	int	res = 1;

	res = SendM2(command);
	DPRINTK(" command_drv: %s! code=%d\n",command,res);
	return res;
	
	
}

int command_process(char * command)
{	
	int	res = 1;
	
	res = process_send_command(command);
	if( res >= 0 )
	{
		printf(" process_send_command : %s sucess!\n",command);
		return 1;
	}else
	{
		printf(" process_send_command : %s faild!\n",command);
		return -1;
	}
	
	return -1;	
	
}



int command_drv(char * command)
{	

	return SendM2(command);
	
}




///////////////////////////////////////////////////////////////////////////////
// drv use fuction

int read_channel_id( unsigned char* pY1, unsigned char* pY2, int iWidth,
				unsigned char* id1,
				unsigned char* id2,
				unsigned char* id3,
				unsigned char* id4 )
{
	int idx;
	
	*id1 = 0;
	*id2 = 0;
	*id3 = 0;
	*id4 = 0;
	
	for( idx = 0; idx < iWidth-2; idx++ )
	{
		if ( 0x5f == pY1[idx] && 0x1 == pY1[idx+1] &&  0x50 == pY1[idx+2] )
		{
			if ( (0x50 == ( pY1[idx+4] & pY1[idx+84] )) && 
				(0x5f == ( pY1[idx+4] | pY1[idx+84] )) && 
				(0x50 == ( pY1[idx+6] & pY1[idx+86] )) && 
				(0x5f == ( pY1[idx+6] | pY1[idx+86] )) )
			{
				*id1 = pY1[idx+6];
				*id2 = pY1[idx+10];
				*id3 = pY2[idx+6];
				*id4 = pY2[idx+10];
				return 0;
			}
			else
			{
				idx += 126;
			}
		}
	}
	
	return 1;
}


void  init_all_variable()
{
	g_decodeItem.enable_play = 0;
	g_decodeItem.frame_num = 0;
	pthread_mutex_init(&g_decodeItem.mutex,NULL);
	pthread_cond_init(&g_decodeItem.cond,NULL);
	pthread_mutex_init(&cap_control_mutex,NULL);

}

void release_all_variable()
{
	pthread_mutex_destroy(&g_decodeItem.mutex);
	pthread_cond_destroy(&g_decodeItem.cond);
	pthread_mutex_destroy( &cap_control_mutex);
}

int init_all_custom_buf()
{
	video_encode_buf =(unsigned char *)malloc(100);
	if(!video_encode_buf)
	{
		trace("no enough memory!");
		return -1;
	}

	video_encode_buf_2 =(unsigned char *)malloc(100);
	if(!video_encode_buf_2)
	{
		trace("no enough memory!");
		return -1;
	}
	

	
	undecode_video_buf =(unsigned char *)malloc(100);
	if(!undecode_video_buf)
	{
		trace("no enough memory!");
		return -1;
	}


	return 1;
}

void  release_custom_buf()
{
	if( video_encode_buf )
	{
		free(video_encode_buf);
		video_encode_buf = NULL;
	}

	if( video_encode_buf_2 )
	{
		free(video_encode_buf_2);
		video_encode_buf_2 = NULL;
	}


	if( undecode_video_buf )
	{
		free(undecode_video_buf);
		undecode_video_buf = NULL;
	}

}

#define FCAP_IOC_MAGIC  'f'
#define FCAP_IOSDEINTERLACE _IOW(FCAP_IOC_MAGIC, 12, int)
#define FCAP_DEINTERLACE_OFF 0
#define FCAP_DEINTERLACE_ON  1

int init_cap_device(int standard)
{
	DPRINTK("in func\n");    
	return 1;
}

int release_cap_device()
{
	DPRINTK("in func\n");    
	return 1;
}

int frame_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;
//	char cmd[50];
	static int fram_num = 1000;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 1000 )
	  {	  
	  	
		get_sys_time( &tvNew, NULL );
		
		frameRate = div_safe2(count,tvNew.tv_sec - oldTime);
		count = 0;
		DPRINTK("g_encodeList:  %d\n", get_enable_insert_data_num(&g_encodeList));	
	  }

	  if( get_enable_insert_data_num(&g_encodeList) < fram_num)
	  {
	  	fram_num = get_enable_insert_data_num(&g_encodeList);
		DPRINTK("buf remain  %d\n", get_enable_insert_data_num(&g_encodeList));	
	  }

/*
	if( count % 400 == 0 )
	{
		DPRINTK("buf remain  %d\n", get_enable_insert_data_num(&g_encodeList));	
	}
	
*/	  return 1;
	
}


int net_frame_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;
//	char cmd[50];
//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 500 )
	  {	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));
		count = 0;
		DPRINTK("net encode frame rate: %d f/s\n",frameRate);	
	  }

	  return 1;
	
}

int frame_rate_ctrl(int channel)
{
	struct timeval recTime;
	long long temp_time,temp_time1;
	int times;
	int i;
	static int num = 0;

	i = channel;	

	get_sys_time( &recTime, NULL );


	

	if( enc_frame_rate_ctrl[i].last_frame_time.tv_sec - recTime.tv_sec > 2 ||
		enc_frame_rate_ctrl[i].last_frame_time.tv_sec - recTime.tv_sec < -2 )
	{
		enc_frame_rate_ctrl[i].last_frame_time.tv_sec = recTime.tv_sec;
		enc_frame_rate_ctrl[i].last_frame_time.tv_usec = recTime.tv_usec;
	}else
	{
		num++;
	
		temp_time = (enc_frame_rate_ctrl[i].last_frame_time.tv_sec - recTime.tv_sec )* 1000000 +
			enc_frame_rate_ctrl[i].last_frame_time.tv_usec + enc_frame_rate_ctrl[i].frame_interval_time;

		temp_time1 = (recTime.tv_sec - recTime.tv_sec )* 1000000 +	recTime.tv_usec;

		if( 0 )
		{
			printf("11temp_time=%lld temp_time1=%lld\n",temp_time,temp_time1);
			printf("now time=%ld lastframetime=%ld\n",enc_frame_rate_ctrl[i].last_frame_time.tv_sec ,enc_frame_rate_ctrl[i].last_frame_time.tv_usec);
			printf("last time=%ld lastframetime=%ld\n",recTime.tv_sec,recTime.tv_usec);
		}
		
		if( temp_time1 < temp_time )
		{
			if( num > 100 )
			{
				printf("nowtime=%lld lastframetime=%lld\n",temp_time1,temp_time);
			}
			return 0;
		}
		else
		{
			num = 0;
			
			times = div_safe2( (temp_time1 -( temp_time - enc_frame_rate_ctrl[i].frame_interval_time) ) , enc_frame_rate_ctrl[i].frame_interval_time);

			enc_frame_rate_ctrl[i].last_frame_time.tv_sec += (enc_frame_rate_ctrl[i].last_frame_time.tv_usec +
				enc_frame_rate_ctrl[i].frame_interval_time * times) / 1000000;

			enc_frame_rate_ctrl[i].last_frame_time.tv_usec = (enc_frame_rate_ctrl[i].last_frame_time.tv_usec +
				enc_frame_rate_ctrl[i].frame_interval_time * times) % 1000000;
		}
	}	

		return ALLRIGHT;
}

int frame_rate_ctrl_net(int channel)
{
	struct timeval recTime;
	long long temp_time,temp_time1;
	int times;
	int i;
	static int num = 0;

	i = channel;

	get_sys_time( &recTime, NULL );




	if( enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec - recTime.tv_sec > 2 ||
		enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec - recTime.tv_sec < -2 )
	{
		enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = recTime.tv_sec;
		enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = recTime.tv_usec;
	}else
	{
		num++;
	
		temp_time = (enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec - recTime.tv_sec )* 1000000 +
			enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec + enc_frame_rate_ctrl_net[i].frame_interval_time;

		temp_time1 = (recTime.tv_sec - recTime.tv_sec )* 1000000 +	recTime.tv_usec;

		if( 0 )
		{
			printf("11temp_time=%lld temp_time1=%lld\n",temp_time,temp_time1);
			printf("now time=%ld lastframetime=%ld\n",enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec ,enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec);
			printf("last time=%ld lastframetime=%ld\n",recTime.tv_sec,recTime.tv_usec);
		}
		
		if( temp_time1 < temp_time )
		{
			if( num > 100 )
			{
				printf("nowtime=%lld lastframetime=%lld\n",temp_time1,temp_time);
			}
			return 0;
		}
		else
		{
			num = 0;
			
			times = div_safe2((temp_time1 -( temp_time - enc_frame_rate_ctrl_net[i].frame_interval_time) ) , enc_frame_rate_ctrl_net[i].frame_interval_time);

			enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec += (enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec +
				enc_frame_rate_ctrl_net[i].frame_interval_time * times) / 1000000;

			enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = (enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec +
				enc_frame_rate_ctrl_net[i].frame_interval_time * times) % 1000000;
		}
	}	

		return ALLRIGHT;
}



void colorspace_init(void)
{
	int32_t i;
	for (i = 0; i < 16; i++) {
		RGB_Y_tab[i] = 128 * i;
		B_U_tab[i] = 222 * (16 - 128);
		G_U_tab[i] = 44 * (16 - 128);
		G_V_tab[i] = 88 * (16 - 128);
		R_V_tab[i] = 176 * (16 - 128);
	}
	for (i = 16; i < 240; i++) {
		RGB_Y_tab[i] = 128 * i;
		B_U_tab[i] = 222 * (i - 128);
		G_U_tab[i] = 44 * (i - 128);
		G_V_tab[i] = 88 * (i - 128);
		R_V_tab[i] = 176 * (i - 128);
	}
	for (i = 240; i < 256; i++) {
		RGB_Y_tab[i] = 128 * i;
		B_U_tab[i] = 222 * (240 - 128);
		G_U_tab[i] = 44 * (240 - 128);
		G_V_tab[i] = 88 * (240 - 128);
		R_V_tab[i] = 176 * (240 - 128);
	}
}

#define MIN(a,b)  (((a)<(b)) ? (a) : (b))//LIZG 28/10/2002
#define MAX(a,b)  (((a)<(b)) ? (b) : (a))//LIZG 28/10/2002

#define MK_RGB555(R,G,B)	((MAX(0,MIN(255, R)) << 7) & 0x7c00) | \
							((MAX(0,MIN(255, G)) << 2) & 0x03e0) | \
							((MAX(0,MIN(255, B)) >> 3) & 0x001f)

void
yv12_to_rgb555_rgb555_c(uint8_t * dst,
				 int dst_stride,
				 uint8_t * y_src,
				 uint8_t * u_src,
				 uint8_t * v_src,
				 int y_stride,
				 int uv_stride,
				 int width,
				 int height)
{
	const uint32_t dst_dif = 4 * dst_stride - 2 * width;
	int32_t y_dif = 2 * y_stride - width;

	uint8_t *dst2 = dst + 2 * dst_stride;
	uint8_t *y_src2 = y_src + y_stride;
	uint32_t x, y;

	if (height < 0) {
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}

	for (y = height / 2; y; y--) {
		int r, g, b;
		int r2, g2, b2;

		r = g = b = 0;
		r2 = g2 = b2 = 0;

		// process one 2x2 block per iteration
		for (x = 0; x < (uint32_t) width / 2; x++) {
			int u, v;
			int b_u, g_uv, r_v, rgb_y;

			u = u_src[x];
			v = v_src[x];

			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];
//Y0,U,V=>R0
			rgb_y = RGB_Y_tab[*y_src];
			b = ((rgb_y + b_u) >> 7);
			g = ((rgb_y - g_uv) >> 7);
			r = ((rgb_y + r_v) >> 7);
			*(uint16_t *) dst = MK_RGB555(r, g, b);
//Y1,U,V=>R1
			y_src++;
			rgb_y = RGB_Y_tab[*y_src];
			b = ((rgb_y + b_u) >> 7);
			g = ((rgb_y - g_uv) >> 7);
			r = ((rgb_y + r_v) >> 7);
			*(uint16_t *) (dst + 2) = MK_RGB555(r, g, b);
			y_src++;
//Y2,U,V=>R2
			rgb_y = RGB_Y_tab[*y_src2];
			b2 = ((rgb_y + b_u) >> 7);
			g2 = ((rgb_y - g_uv) >> 7);
			r2 = ((rgb_y + r_v) >> 7);
			*(uint16_t *) (dst2) = MK_RGB555(r2, g2, b2);
			y_src2++;
//Y3,U,V=>R3
			rgb_y = RGB_Y_tab[*y_src2];
			b2 = ((rgb_y + b_u) >> 7);
			g2 = ((rgb_y - g_uv) >> 7);
			r2 = ((rgb_y + r_v) >> 7);
			*(uint16_t *) (dst2 + 2) = MK_RGB555(r2, g2, b2);
			y_src2++;

			dst += 4;
			dst2 += 4;
		}

		dst += dst_dif;
		dst2 += dst_dif;

		y_src += y_dif;
		y_src2 += y_dif;

		u_src += uv_stride;
		v_src += uv_stride;
	}
}

void show_lcd(unsigned char *fb_mem, unsigned char *yuv, int cap_width, int cap_height, int lcd_width, int lcd_height)
{
    int             i,j;
    int				width, height;
//	int				lcd_y_size = (((lcd_width*lcd_height)+0xffff)&0xffff0000);
//    unsigned char   r_y,g_u,b_v;
//	unsigned char u_src[1],v_src[1], *k;
	unsigned char * yuv_u,*yuv_v;
	unsigned char * fb_mem_u, *fb_mem_v;
	int tmp =0;
	int show_line = 0;
	int line_size = 0;

	if( g_show_array.change_fb == 0 )
		tmp=3;    		
	else
		tmp=1;	

	fb_mem = g_show_array.avp[tmp].data[0];
     	fb_mem_u = g_show_array.avp[tmp].data[1];
	fb_mem_v = g_show_array.avp[tmp].data[2];	

	yuv = g_show_array.avp[2].data[0];
	yuv_u = g_show_array.avp[2].data[1];
	yuv_v = g_show_array.avp[2].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )				
		height = 576 ;
	else			
		height = 480 ;

	#ifdef CUT_GREEN_LINE
		show_line = 8;
	#endif

	line_size = width * 2;	
	
#ifdef USE_H264_ENC_DEC
	for(i = show_line; i < height; i++ )
	{
		if( i %2 ==  0 )
		{
			/*for( j = 0; j < 352 /2 ; j++)
			{				
				fb_mem[i*width*2 + j*8] = yuv[ (i/2) * 2* 352 + j* 4 ];
				fb_mem[i*width*2 + j*8 + 1] = yuv[ (i/2) * 2* 352 + j* 4 + 1];
				fb_mem[i*width*2 + j*8 + 2] = yuv[ (i/2) * 2* 352 + j* 4 + 2];
				fb_mem[i*width*2 + j*8 + 3] = yuv[ (i/2) * 2* 352 + j* 4 + 1];
				fb_mem[i*width*2 + j*8 + 4] = yuv[ (i/2) * 2* 352 + j* 4 ];
				fb_mem[i*width*2 + j*8 + 5] = yuv[ (i/2) * 2* 352 + j* 4 + 3];
				fb_mem[i*width*2 + j*8 + 6] = yuv[ (i/2) * 2* 352 + j* 4 + 2];
				fb_mem[i*width*2 + j*8 + 7] = yuv[ (i/2) * 2* 352 + j* 4 + 3];
			}*/

			for( j = 0; j < 176 ; j++)
			{				
				fb_mem[i*line_size + (j<<3)] = yuv[ i* 352 + (j<<2)];
				fb_mem[i*line_size + (j<<3) + 1] = yuv[ i* 352 + (j<<2) + 1];
				fb_mem[i*line_size + (j<<3) + 2] = yuv[ i* 352 + (j<<2) + 2];
				fb_mem[i*line_size + (j<<3) + 3] = yuv[ i* 352 + (j<<2) + 1];
				fb_mem[i*line_size + (j<<3) + 4] = yuv[ i* 352 + (j<<2) ];
				fb_mem[i*line_size + (j<<3) + 5] = yuv[ i* 352 + (j<<2) + 3];
				fb_mem[i*line_size + (j<<3)+ 6] = yuv[ i* 352 + (j<<2) + 2];
				fb_mem[i*line_size + (j<<3) + 7] = yuv[ i* 352 + (j<<2) + 3];
			}
		}else
		{
			memcpy(&fb_mem[ i * width*2 ],&fb_mem[ ( i - 1 ) * width*2 ],width*2);
		}
	}
	

#else
//Y
	for( i = 0; i < height ; i++ )
	{
		if( i % 2 == 0 )
		{
			for(j = 0; j < 352; j++)
			{
				fb_mem[ i*width + j*2 ] = yuv[ i / 2* 352 + j ];
				fb_mem[ i*width + j*2 + 1] =  yuv[ i / 2* 352  + j ];
				
			}
		}
		else
		{
			memcpy(&fb_mem[ i * width ],&fb_mem[ ( i - 1 ) * width ],width);
		}
	}


// U

	width = 720 /2;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )				
		height = 576 /2;
	else			
		height = 480 /2;
	

	for( i = 0; i < height ; i++ )
	{
		if( i % 2 == 0 )
		{
			for(j = 0; j < 352 /2; j++)
			{
				fb_mem_u[ i*width + j*2 ] = yuv_u[ i/2*176 + j ];
				fb_mem_u[ i*width + j*2 + 1] =  yuv_u[ i/2*176 + j ];				
			}
		}
		else
		{
			memcpy(&fb_mem_u[ i * width ],&fb_mem_u[ ( i - 1 ) * width ],width);
		}
	}

	
//v 
	width = 720 /2;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )				
		height = 576 /2;
	else			
		height = 480 /2;

	for( i = 0; i < height ; i++ )
	{
		if( i % 2 == 0 )
		{
			for(j = 0; j < 352 /2; j++)
			{
				fb_mem_v[ i*width + j*2 ] = yuv_v[ i/2*176 + j ];
				fb_mem_v[ i*width + j*2 + 1] =  yuv_v[ i/2*176 + j ];				
			}
		}
		else
		{
			memcpy(&fb_mem_v[ i * width ],&fb_mem_v[ ( i - 1 ) * width ],width);
		}
	}	

#endif

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&tmp) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return  ;
	}

	g_show_array.change_fb = !g_show_array.change_fb;


	return;


}
void show_lcd_352x240_l(unsigned char *fb_mem, unsigned char *yuv, int cap_width, int cap_height, int lcd_width, int lcd_height)
{
    int             i;
    int				width, height;
	int				lcd_y_size = (((lcd_width*lcd_height)+0xffff)&0xffff0000);
//unsigned char u_src[1],v_src[1], *k;
     	  
	if (cap_height < lcd_height)
		height = cap_height;
	else
		height = lcd_height;
	if (cap_width < lcd_width)
		width = cap_width;
	else
		width = lcd_width;

		/* Y*/
	for (i = 0; i < height; i ++)
		memcpy ((lcd_width/2) + (fb_mem + i * lcd_width), yuv + i * cap_width, width);
	/* U*/
	fb_mem += lcd_y_size;
	yuv += cap_width * cap_height;
	for (i = 0; i < height / 2; i ++)
		memcpy ((lcd_width/4) + (fb_mem + i * lcd_width / 2), yuv + i * cap_width / 2, width / 2);
	/* V*/
	fb_mem += lcd_y_size/4;
	yuv += cap_width * cap_height/4;
	for (i = 0; i < height / 2; i ++)
		memcpy ((lcd_width/4) + (fb_mem + i * lcd_width / 2), yuv + i * cap_width / 2, width / 2);
}

void show_lcd_rgb(unsigned char *fb_mem, unsigned char *yuv, int cap_width, int cap_height, int lcd_width, int lcd_height)
{
    int				width, height;

  	if (cap_height < lcd_height)
	    height = cap_height;
	else
	    height = lcd_height;
  	if (cap_width < lcd_width)
	    width = cap_width;
	else
	    width = lcd_width;
	    
//printf("fb_mem=%x %x %x %x\n",fb_mem,yuv,yuv + cap_width * cap_height,yuv + cap_width * cap_height * 5 / 4);	    
    yv12_to_rgb555_rgb555_c(fb_mem,                                           // dst
                            lcd_width,                                        // dst stride
                            yuv,                                              // y_src
                            yuv + cap_width * cap_height,                     // u_src
                            yuv + cap_width * cap_height * 5 / 4,             // v_src
                            cap_width,                                        // y_stride
                            cap_width/2,                                      // uv stride
                            width,                                            // width
                            height);                                          // height
}


int decode_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;

//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;	

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 200 )
	  {
	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));

		count = 0;

		printf("2decode frame rate: %d f/s\n",frameRate);
		
	  }

	  return 1;
	
}


int p720_encode_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;

//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;	

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 500 )
	  {
	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));

		count = 0;

		DPRINTK(" %d f/s\n",frameRate);
		
	  }

	  return 1;
	
}



int cif_decode_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;

//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;	

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 500 )
	  {
	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));

		count = 0;

		DPRINTK(" %d f/s\n",frameRate);
		
	  }

	  return 1;
	
}


int enable_audio_rec(int flag)
{
	if( flag == 1 )
		rec_audio_flag = 1;
	else
		rec_audio_flag = 0;

	return 1;
}

int clear_fb()
{
//	int i;

	printf(" clean fb buf!\n");
	
	clear_show_fb = 1;
	return 1;
}

int show_cif_img_to_middle(int flag)
{
	if( flag == 1)
		cif_to_middle = 1;
	else
		cif_to_middle = 0;

	clear_fb();

	return 1;
}

int show_d1_to_lcd()
{
//	int i,j;
//	unsigned char *fb_mem, * fb_men_u, * fb_men_v;
//	unsigned char *yuv_men, * yuv_men_u, * yuv_men_v;
//	int				lcd_y_size = (((720 * 576)+0xffff)&0xffff0000);
	int  tmp;
//	int off_x,off_y;
//	int index,y;
//	int loop;
//	int value1,value2;
	
	if( g_show_array.change_fb == 0 )
		tmp=2;    		
	else
		tmp=1;	
/*
	fb_mem = g_show_array.avp[tmp].data[0];
	fb_men_u = g_show_array.avp[tmp].data[1];
	fb_men_v = g_show_array.avp[tmp].data[2];

	yuv_men = g_show_array.avp[3].data[0];
	yuv_men_u = g_show_array.avp[3].data[1];
	yuv_men_v = g_show_array.avp[3].data[2];

	memcpy(fb_mem,yuv_men, 720*576);
	memcpy(fb_men_u,yuv_men_u, 720*576 / 4);
	memcpy(fb_men_v,yuv_men_v, 720*576 / 4);
	*/


	/*
	// ?????a????¨2???á?¨￠?¨¨???￥??|¨¤¨a
	fb_mem = g_show_array.avp[tmp].data[0];

	loop = 576 * 720 * 2;

	
	for(index =1 ; index < loop ; index++)
	{
		if( index % 2 != 1 )
			continue;
		
		value1 = fb_mem[index- 2];
		value2 = fb_mem[index];
		value2 = value2 + (value2 - value1)/2;

		value1 = value2 > 255 ? 255:value2;
		fb_mem[index] = value1< 0 ? 0:value1;	
		
	}	*/

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&tmp) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}

	g_show_array.change_fb = !g_show_array.change_fb;

	return 1;
}

int show_mult_chan_d1_to_screen(int chan)
{
//	int i,j;
	unsigned char *fb_mem, * fb_men_u, * fb_men_v;
	unsigned char *yuv_men, * yuv_men_u, * yuv_men_v;
	unsigned char *yuv_men2, * yuv_men_u2, * yuv_men_v2;
//	int				lcd_y_size = (((720 * 576)+0xffff)&0xffff0000);
	int  tmp;
	int  combine_screen_num = 3;
	int  combine_screen_num2 = 3;
	int off_x,off_y;
	int width,height;
	int x,y;
	int pos,pos1;
	
	if( g_show_array.change_fb == 0 ){
		tmp=2;    	
		combine_screen_num =4;
		combine_screen_num2 = 3;
	}else{
		tmp=1;
		combine_screen_num =3;
		combine_screen_num2 = 4;
		//combine_screen_num =4;
	}

	fb_mem = g_show_array.avp[tmp].data[0];
	fb_men_u = g_show_array.avp[tmp].data[1];
	fb_men_v = g_show_array.avp[tmp].data[2];
	yuv_men = g_show_array.avp[combine_screen_num].data[0];
	yuv_men_u = g_show_array.avp[combine_screen_num].data[1];
	yuv_men_v = g_show_array.avp[combine_screen_num].data[2];
	
	yuv_men2 = g_show_array.avp[combine_screen_num2].data[0];
	yuv_men_u2 = g_show_array.avp[combine_screen_num2].data[1];
	yuv_men_v2 = g_show_array.avp[combine_screen_num2].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		height = 576;	
		off_x = 0;
		off_y = 0;
	}
	else
	{
		height = 480;
		off_x = 0;
		off_y = 0;
	}

	if( get_screen_show_mode() == 1 )
	{
		show_d1_to_lcd();
		return 1;
	}

	if( get_screen_show_mode() == 4 ){
		//off_x =  (chan % 2) * (352+16);
		off_x =  (chan % 2) * (352);
		off_y =  (chan / 2 ) * height/2;

		for( y = 10 ; y < height ; y++ ){

			if( y % 2 == 1 )
				continue;
			// ¨¨?¨a???¨?¨°?à?2???¨?¨|¨￠
			for( x = 0 ; x < width -20 ; x++){

				//if( x % 4 != 0 || x > 704)
				if( x % 4 != 0 )
					continue;				

				pos = y*width*2 + x*2;

				pos1 =  (y / 2 +off_y) * width * 2 + (x / 2+off_x)*2;

				yuv_men[pos1] = fb_mem[pos];
				yuv_men[pos1+1] = fb_mem[pos+1];
				yuv_men[pos1+2] = fb_mem[pos+2];
				yuv_men[pos1+3] = fb_mem[pos+3];
				
			}
		}
		
		
	}


	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&combine_screen_num) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}	




	for( y = off_y ; y < off_y+height/2 ; y++ ){

			pos = y*width*2 + off_x*2;
			memcpy(&yuv_men2[pos],&yuv_men[pos],352*2);
	}
	
/*	if( g_show_array.change_fb == 0 ){		 

		memcpy(g_show_array.avp[3].data[0],g_show_array.avp[4].data[0],width*height*2);
	}else{		
		memcpy(g_show_array.avp[4].data[0],g_show_array.avp[3].data[0],width*height*2);
	} */

	g_show_array.change_fb = !g_show_array.change_fb;

	return 1;
}

int show_cif_to_lcd()
{
//	int i,j;
	unsigned char *fb_mem, * fb_men_u, * fb_men_v;
	unsigned char *yuv_men, * yuv_men_u, * yuv_men_v;
//	int				lcd_y_size = (((720 * 576)+0xffff)&0xffff0000);
	int  tmp;
	int off_x,off_y;
	int width,height;
	int show_line = 0;
	

#ifdef USE_H264_ENC_DEC
	if( g_show_array.change_fb == 0 )
		tmp=3;    		
	else
		tmp=1;	

	fb_mem = g_show_array.avp[tmp].data[0];
	fb_men_u = g_show_array.avp[tmp].data[1];
	fb_men_v = g_show_array.avp[tmp].data[2];

	yuv_men = g_show_array.avp[2].data[0];
	yuv_men_u = g_show_array.avp[2].data[1];
	yuv_men_v = g_show_array.avp[2].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		height = 576;	
		off_x = 144;
		off_y = 180;
		
	}
	else
	{
		height = 480;
		off_x = 180;
		off_y = 144;
	}

	//if( cif_to_middle == 0 )
	{		
		off_x = 0;
		off_y = 0;
	}

	#ifdef CUT_GREEN_LINE
	show_line = 8;	
	#endif

	
	/*for( i = show_line ; i <  height /2 ; i++ )
	{	
		if( (i >= 20) && ( i<= 30 ) )
		{
			memcpy(&fb_mem[((i-20)*2 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i* 352*2], 352*2);		
			memcpy(&fb_mem[((i-20)*2 + 1 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);		
			continue;
		}

		if( (i >= 30) && ( i<= 40 ) )
			continue;
	
		if( i % 2 == 0 )
			memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);
		else
			memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ (i - 1)* 352*2], 352*2);
		
	}*/	


	/*for( i = 0 ; i <  height /2 ; i++ )
	{			
		if( i % 2 == 0 )
			memcpy(&fb_mem[i* 352*2],&yuv_men[ i * 352*2], 352*2);
		else
			memcpy(&fb_mem[i* 352*2],&yuv_men[ (i - 1)* 352*2], 352*2);
		
	}*/

	memcpy(&fb_mem[0],&yuv_men[ 0], 352*2*height /2);
	

	

	/*for( i = 0 ; i <  10 ; i++ )
	{				
			memcpy(&fb_mem[(i*2 +20 +off_y)* width*2 + off_x*2],&yuv_men[ (i+20) * 352*2], 352*2);		
			memcpy(&fb_mem[(i*2 + 1 +20 +off_y)* width*2 + off_x*2],&yuv_men[ (i+20) * 352*2], 352*2);
		
	}	*/

	//memcpy(&fb_mem[(20 +off_y)* width*2 + off_x*2],&yuv_men[ 21* 352*2], 352*2);
	//memcpy(&fb_mem[(30 +off_y)* width*2 + off_x*2],&yuv_men[ 29* 352*2], 352*2);

	
	/*for( i = 0 ; i < height/4; i++ )
	{
		memcpy(&fb_men_u[( i + off_y /2 ) * (width/2) + off_x /2  ],&yuv_men_u[i * 352/2], 352/2);
	}
	
	for( i = 0 ; i < height/4; i++ )
	{
		memcpy(&fb_men_v[( i + off_y /2 ) * (width/2)  + off_x /2  ],&yuv_men_v[i * 352/2], 352/2);
	}		*/
	
	

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&tmp) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}	

	g_show_array.change_fb = !g_show_array.change_fb;

#else
	
	if( g_show_array.change_fb == 0 )
		tmp=2;    		
	else
		tmp=1;	

	fb_mem = g_show_array.avp[tmp].data[0];
	fb_men_u = g_show_array.avp[tmp].data[1];
	fb_men_v = g_show_array.avp[tmp].data[2];

	yuv_men = g_show_array.avp[2].data[0];
	yuv_men_u = g_show_array.avp[2].data[1];
	yuv_men_v = g_show_array.avp[2].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		height = 576;	
		off_x = 144;
		off_y = 180;
	}
	else
	{
		height = 480;
		off_x = 180;
		off_y = 144;
	}

	if( cif_to_middle == 0 )
	{		
		off_x = 0;
		off_y = 0;
	}
	
	for( i = 0 ; i < height/2; i++ )
	{
		memcpy(&fb_mem[(i + off_y) * width + off_x],&yuv_men[ i * 352], 352);
	}	
	
	for( i = 0 ; i < height/4; i++ )
	{
		memcpy(&fb_men_u[( i + off_y /2 ) * (width/2) + off_x /2  ],&yuv_men_u[i * 352/2], 352/2);
	}
	
	for( i = 0 ; i < height/4; i++ )
	{
		memcpy(&fb_men_v[( i + off_y /2 ) * (width/2)  + off_x /2  ],&yuv_men_v[i * 352/2], 352/2);
	}		
	
	

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&tmp) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}

	g_show_array.change_fb = !g_show_array.change_fb;

#endif

	return 1;
}

int show_mult_chan_cif_to_lcd(int chan)
{
	int i;
	unsigned char *fb_mem, * fb_men_u, * fb_men_v;
	unsigned char *yuv_men, * yuv_men_u, * yuv_men_v;
//	int				lcd_y_size = (((720 * 576)+0xffff)&0xffff0000);
//	int  tmp;
	int off_x,off_y;
	int width,height;
	int show_line = 0;
	int  combine_screen_num = 3;


	if( g_show_array.change_fb == 0 ){	   	
		combine_screen_num =4;
	}else{		
		//combine_screen_num =3;
		combine_screen_num =4;
	}	

	//printf("chan =%d  show_mode=%d\n",chan,get_screen_show_mode());

	if( get_screen_show_mode() == 1 )
	{
		show_cif_to_lcd();
		return 1;
	}

	fb_mem = g_show_array.avp[combine_screen_num].data[0];
	fb_men_u = g_show_array.avp[combine_screen_num].data[1];
	fb_men_v = g_show_array.avp[combine_screen_num].data[2];

	yuv_men = g_show_array.avp[2].data[0];
	yuv_men_u = g_show_array.avp[2].data[1];
	yuv_men_v = g_show_array.avp[2].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		height = 576;		
	}
	else
	{
		height = 480;		
	}

	if( cif_to_middle == 0 )
	{		
		off_x = 0;
		off_y = 0;
	}

	//off_x =  (chan % 2) * (352+8);
	off_x =  (chan % 2) * 352;
	//off_y =  (chan / 2 ) * height/2;
	off_y =  (chan / 2 ) * (height/2 - 8);

	#ifdef CUT_GREEN_LINE
	show_line = 8;	
	#endif

	if( chan / 2 == 0 )
	{
		for( i = show_line ; i <  height /2 ; i++ )
		{	
			if( (i >= 20) && ( i<= 30 ) )
			{
				memcpy(&fb_mem[((i-20)*2 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i* 352*2], 352*2);		
				memcpy(&fb_mem[((i-20)*2 + 1 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);		
				continue;
			}

			if( (i >= 30) && ( i<= 40 ) )
				continue;
		
			if( i % 2 == 0 )
				memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);
			else
				memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ (i - 1)* 352*2], 352*2);
			
		}		
	}else
	{
		for( i = show_line ; i <  height /2 ; i++ )
		{	
			if( (i >= 20) && ( i<= 30 ) )
			{
				memcpy(&fb_mem[((i-20)*2 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i* 352*2], 352*2);		
				memcpy(&fb_mem[((i-20)*2 + 1 +20 +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);		
				continue;
			}

			if( (i >= 30) && ( i<= 40 ) )
				continue;
		
			if( i % 2 == 0 )
				memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ i * 352*2], 352*2);
			else
				memcpy(&fb_mem[(i +off_y)* width*2 + off_x*2],&yuv_men[ (i - 1)* 352*2], 352*2);
			
		}	
	}
	

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&combine_screen_num) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}

/*	if( g_show_array.change_fb == 0 ){		 

		memcpy(g_show_array.avp[3].data[0],g_show_array.avp[4].data[0],width*height*2);
	}else{		
		memcpy(g_show_array.avp[4].data[0],g_show_array.avp[3].data[0],width*height*2);
	}*/

	g_show_array.change_fb = !g_show_array.change_fb;

	return 1;	
}

int show_hd1_to_lcd()
{
	int i;
	unsigned char *fb_mem, * fb_men_u, * fb_men_v;
	unsigned char *yuv_men, * yuv_men_u, * yuv_men_v;
//	int				lcd_y_size = (((720 * 576)+0xffff)&0xffff0000);
	int  tmp;
//	int off_x,off_y;
	int width,height;
	
	if( g_show_array.change_fb == 0 )
		tmp=2;    		
	else
		tmp=1;	

	fb_mem = g_show_array.avp[tmp].data[0];
	fb_men_u = g_show_array.avp[tmp].data[1];
	fb_men_v = g_show_array.avp[tmp].data[2];

	yuv_men = g_show_array.avp[3].data[0];
	yuv_men_u = g_show_array.avp[3].data[1];
	yuv_men_v = g_show_array.avp[3].data[2];

	width = 720;
	if( cur_standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		height = 576;		
	}
	else
	{
		height = 480;		
	}

	for( i = 0 ; i < height; i++ )
	{
		memcpy(&fb_mem[i* width ],&yuv_men[ i/2 * width], width);
	}

	for( i = 0 ; i < height/2; i++ )
	{
		memcpy(&fb_men_u[ i * (width/2)   ],&yuv_men_u[i /2* width/2], width/2);
	}	

	for( i = 0 ; i < height/2; i++ )
	{
		memcpy(&fb_men_v[ i * (width/2)   ],&yuv_men_v[i /2* width/2], width/2);
	}		

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&tmp) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}

	g_show_array.change_fb = !g_show_array.change_fb;

	return 1;
}


int get_frame_from_buf_and_decode(BUF_FRAMEHEADER * header, char * Buf,int cap_width,int cap_height)
{
	int iNumber = 0;
	int getPictrue;
//	static int first = 1;
	char *  ptrBuf = NULL;
//	uint8_t             *ptr;
	int len;
//	int ret = 0;
	int i = 0;
	int j,k;
	int width,height;

	AVFrame yuvframe;
//	struct timeval tvOld;
  //     struct timeval tvNew;	
//	FILE *  fp = NULL;
//	static int cap_count = 0;
	int rel = 0;

	rel = get_one_buf(header,Buf,&g_decodeList);

	if( rel < 0 )
	{
		DPRINTK(" get_one_buf %d\n",rel);
		return ALLRIGHT;
	}

//	printf("DRV %d %d %d %d\n",header->iFrameType,header->iLength,header->type,header->ulTimeSec);

	if( header->type != AUDIO_FRAME && header->type != VIDEO_FRAME )
		return ERROR;

	if( header->type == AUDIO_FRAME)
	{	
		// ?¨¤¨a?§|ì¨¤???¤?¨o?à?ê??¤?¨°??|ì?¨￠3???¨a		
		//	printf("DRV %d %d %d %d\n",header->iFrameType,header->iLength,header->type,header->ulTimeSec);
		if( get_screen_show_mode() == 1)
			insert_data(header,Buf,&g_audiodecodeList);		
		return ALLRIGHT;
	}

	iNumber = header->iLength;
	ptrBuf = Buf;

	if( header->iFrameType == 1 )
	{
		set_play_back_first_keyframe(header->index, 1);
	}

	if( get_play_back_first_keyframe_status(header->index) == 0 )
		return ALLRIGHT;

/*	printf("c=%d %d type=%d frametype=%d sec=%ld %ld\n",
		header->index, get_mult_play_cam(header->index),
		header->type,header->iFrameType,header->ulTimeSec,header->ulTimeUsec);
*/
	
	if( clear_show_fb )
	{
		#ifdef USE_H264_ENC_DEC
			width = 720; 
			height = 576;
			for( i = 1; i < 5; i++ )
			{
				for(k= 0 ; k < height; k++ )
				{
					for( j = 0; j <width/2; j++ )
					{
						g_show_array.avp[i].data[0][(k*width*2)+j*4] = 0xa5 ;
						g_show_array.avp[i].data[0][(k*width*2)+j*4+1] = 0x3c;
						g_show_array.avp[i].data[0][(k*width*2)+j*4+2] = 0x5f;
						g_show_array.avp[i].data[0][(k*width*2)+j*4+3] = 0x3c;
					}
				}
			}
		#else
		for(i = 1; i < 5; i++ )
		{
			memset(g_show_array.avp[i].data[0],0x3c,720*576);
			memset(g_show_array.avp[i].data[1],0xa5,720*576 / 4);
			memset(g_show_array.avp[i].data[2],0x5f,720*576 / 4);
		}
		#endif

		clear_show_fb = 0;
	}


	//printf("dec chan=%d %x %x %x %x\n",header->index,Buf[1000],Buf[1001],Buf[1002],Buf[1003]);
	
	getPictrue = 0;
	while( iNumber > 0 )
	{
		if(  cur_size == TD_DRV_VIDEO_SIZE_CIF )
		{
		

			yuvframe.data[0] = g_show_array.avp[2].data[0];
			yuvframe.data[1] = g_show_array.avp[2].data[1];
			yuvframe.data[2] = g_show_array.avp[2].data[2];

  		//	len =  xvid_decode(&yuvframe,&getPictrue,ptrBuf,iNumber,&dec_cif);	

			len = mac_decode(&yuvframe,&getPictrue,ptrBuf,iNumber,
				get_mult_play_cam(header->index));
		}
		else if( cur_size == TD_DRV_VIDEO_SIZE_D1 )
		{
		
			if( g_show_array.change_fb == 0 )
			{
				yuvframe.data[0] = g_show_array.avp[2].data[0];
				yuvframe.data[1] = g_show_array.avp[2].data[1];
				yuvframe.data[2] = g_show_array.avp[2].data[2];
			}else
			{
				yuvframe.data[0] = g_show_array.avp[1].data[0];
				yuvframe.data[1] = g_show_array.avp[1].data[1];
				yuvframe.data[2] = g_show_array.avp[1].data[2];
			}			
		//	len =  xvid_decode(&yuvframe,&getPictrue,ptrBuf,iNumber,&dec_d1);

			len = mac_decode(&yuvframe,&getPictrue,ptrBuf,iNumber,
					get_mult_play_cam(header->index));

			/*if( cap_count == 0 )
			{
				fp = fopen("/tddvr/sda5/1.p","wb");

				if( fp != NULL )
				{
					fwrite(yuvframe.data[0],1,720*576*2,fp);
				
					fclose(fp);
				}
				
				cap_count = 1;
			}*/		
			
		}
		else if(cur_size == TD_DRV_VIDEO_SIZE_HD1)
		{
			yuvframe.data[0] = g_show_array.avp[3].data[0];
			yuvframe.data[1] = g_show_array.avp[3].data[1];
			yuvframe.data[2] = g_show_array.avp[3].data[2];

  			len =  xvid_decode(&yuvframe,&getPictrue,ptrBuf,iNumber,&dec_hd1);			
		}else
		{
			DPRINTK("cur_size1 =%d error",cur_size);
		}
		
		if(len < 0 )
		{
		  	trace("decode error!\n");
			printf(" decode = %d\n",get_mult_play_cam(header->index));
			//printf(" iChannel = %d type=%d sec=%ld  usec=%ld   %x%x%x%x\n",header->index ,header->type,
			//								header->ulTimeSec,header->ulTimeUsec,ptrBuf[5],ptrBuf[6],ptrBuf[7],ptrBuf[8]);

			//return -1;
			len = iNumber;
		}

		if ((iNumber - len) <= 12) 
		{
			 len = iNumber;
		}

		// ?a3?¨￠?¨a???
	  	if (getPictrue) 
	 	{			 		
			if( cur_size == TD_DRV_VIDEO_SIZE_CIF)
			{
		 	/*	if( show_bigger )
		 			show_lcd(NULL,NULL,0,0,0,0);
				else
					show_cif_to_lcd();*/

				if( show_bigger && (get_screen_show_mode() == 1) )
				{
					if( reduce_frame_show(header->index) )
		 				show_lcd(NULL,NULL,0,0,0,0);
				}
				else
				{
					if( get_screen_show_mode() == 1 )
						show_mult_chan_cif_to_lcd(get_mult_play_cam(header->index));
					else
					{
						if( reduce_frame_show(header->index) )
							show_mult_chan_cif_to_lcd(get_mult_play_cam(header->index));
					}
					
				}
				
			}

			if( cur_size == TD_DRV_VIDEO_SIZE_D1 )
			{
				//show_d1_to_lcd();

				show_mult_chan_d1_to_screen(get_mult_play_cam(header->index));
			}

			if( cur_size == TD_DRV_VIDEO_SIZE_HD1 )
			{
				show_hd1_to_lcd();
			}
			
	 		decode_rate_show();
	  	}
  
		iNumber -= len;
		ptrBuf  += len; 
	 	

	}

	return 1;
	
}


struct timeval  oldframetime[16];

int get_frame_decode_hisi(BUF_FRAMEHEADER * header, char * Buf,int cap_width,int cap_height)
{
	int iNumber = 0;
	char *  ptrBuf = NULL;
	int rel = 0;
	ENCODE_ITEM * enc_item = NULL;
	static int frame_num[16] = {0};
	static int key_frame_num[16] = {0};
	
	long ulDelayTime = 0;
 

	rel = get_one_buf(header,Buf,&g_decodeList);

	if( rel < 0 )
	{
		DPRINTK(" get_one_buf %d\n",rel);
		return ALLRIGHT;
	}

//	printf("DRV %d %d %d %d\n",header->iFrameType,header->iLength,header->type,header->ulTimeSec);

	if( header->type != AUDIO_FRAME && header->type != VIDEO_FRAME )
		return ERROR;

	if( header->type == AUDIO_FRAME)
	{	
		// ?¨¤¨a?§|ì¨¤???¤?¨o?à?ê??¤?¨°??|ì?¨￠3???¨a		
		//	printf("DRV %d %d %d %d\n",header->iFrameType,header->iLength,header->type,header->ulTimeSec);
		//if( get_screen_show_mode() == 1)
			insert_data(header,Buf,&g_audiodecodeList);		
		return ALLRIGHT;
	}

	iNumber = header->iLength;
	ptrBuf = Buf;

	if( header->iFrameType == 1 )
	{
		set_play_back_first_keyframe(header->index, 1);
		frame_num[header->index] = 0;		
	}

	if( get_play_back_first_keyframe_status(header->index) == 0 )
		return ALLRIGHT;

	/*printf("c=%ld %d type=%ld frametype=%ld sec=%ld %ld  num=%d fnum=%ld\n",
		header->index, get_mult_play_cam(header->index),
		header->type,header->iFrameType,header->ulTimeSec,header->ulTimeUsec,
		frame_num[header->index],header->frame_num );*/

	if( (key_frame_num[header->index] >= 2 ) && 
		(header->iFrameType !=1)  )
	{

		
		ulDelayTime = ( header->ulTimeSec - oldframetime[header->index].tv_sec ) * 1000000 
					+ (header->ulTimeUsec - oldframetime[header->index].tv_usec);

		if( ulDelayTime > 1000000 ||  ulDelayTime < -1000000  )
		{
			frame_num[header->index] = 0;
		}

		printf("c=%ld %d type=%ld frametype=%ld sec=%ld %ld  num=%d fnum=%ld (%ld,%ld,%ld) \n",
		header->index, get_mult_play_cam(header->index),
		header->type,header->iFrameType,header->ulTimeSec,header->ulTimeUsec,
		frame_num[header->index],header->frame_num,
		oldframetime[header->index].tv_sec , oldframetime[header->index].tv_usec,ulDelayTime);
	
	}


	if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
		enc_item = &enc_cif[header->index];
	else
		enc_item = &enc_d1[header->index];		


	if( frame_num[header->index]  == header->frame_num )
	{	
		rel = Hisi_send_decode_data(enc_item,ptrBuf,header->iLength,get_mult_play_cam(header->index));
		if( rel < 0 )
		{
			DPRINTK("chan=%ld length=%ld frametype=%ld",header->index,header->iLength,header->iFrameType);
		}

		frame_num[header->index]++;	
		oldframetime[header->index].tv_sec = header->ulTimeSec;
		oldframetime[header->index].tv_usec = header->ulTimeUsec;
	}else
	{
		//if( header->frame_num % 5 == 0 )
	//	DPRINTK("chan=%ld cur frame=%d  frame_num=%ld\n",header->index,frame_num[header->index],header->frame_num );
	}	

 	if( header->iFrameType == 1 )
		key_frame_num[header->index]++;
	else
		key_frame_num[header->index]=0;


	//printf("dec chan=%d %x %x %x %x\n",header->index,Buf[1000],Buf[1001],Buf[1002],Buf[1003]);
	

	return 1;
	
}



#define FFB_BUFFMT_NONINTERLACE 0
#define FFB_BUFFMT_INTERLACE 1
#define FFB_IOSBUFFMT _IOW('c', 14, unsigned int)
#define FFB_IOGBUFFMT _IOR('c', 15, unsigned int)






int64_t av_gettime_from_startup(void)
{
#ifdef CONFIG_WIN32
    struct _timeb tb;
    _ftime(&tb);
    return ((int64_t)tb.time * int64_t_C(1000) + (int64_t)tb.millitm) * 
int64_t_C(1000);
#else
    struct timeval 	tv;
	static int64_t 	time_offset;
	static int		init_time_offset = 0;
	
    gettimeofday(&tv,NULL);
	if(init_time_offset == 0){
		time_offset = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
		init_time_offset = 1;
		return (int64_t) 0;
	}
	return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec)-time_offset;
#endif
}


//////////////////////////////////////////////////////////////////////////
// normol ap fuction

#ifdef USE_H264_ENC_DEC

void record_start_create_iframe(int cam)
{	
	int i;


	for(i = 0; i < get_dvr_max_chan(0); i++ )	
	{
		if( get_bit(cam,i) )
		{
			enc_cif[i].iframe_num = 0;
			enc_d1[i].iframe_num = 0;
			enc_hd1[i].iframe_num = 0;
			//printf("channel %d keyframe\n",i);
		}
	}
}

void net_start_create_iframe(int cam)
{
	int i = 0;

		for(i = 0; i < get_dvr_max_chan(0); i++ )
		{
			if( get_bit(cam,i) )
			{
				enc_net_cif[i].iframe_num = 1;
				enc_net_d1[i].iframe_num = 1;
				enc_net_hd1[i].iframe_num = 1;
				//printf("channel %d keyframe\n",i);
			}		
		}	

}


#endif


int drv_start_record()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_start_enc(enc_item);
		if( ret < 0 )
			goto NOT_START;
	}

	return 1;

NOT_START:

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_stop_enc(enc_item);
	}
	
	return -1;
}


int drv_stop_record()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_stop_enc(enc_item);
		
	}

	return 1;
}
// record function

int drv_start_motiondetect()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < MAX_CAM_GROUP; i++ )
	{
		enc_item = &enc_d1[i];

		ret = Hisi_enable_motiondetect(enc_item);
		if( ret < 0 )
			goto NOT_START;
	}

	return 1;

NOT_START:

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		enc_item = &enc_d1[i];

		ret = Hisi_enable_motiondetect(enc_item);
	}
	
	return -1;
}


int drv_stop_motiondetect()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i <  MAX_CAM_GROUP; i++ )
	{
		enc_item = &enc_d1[i];

		ret = Hisi_disable_motiondetect(enc_item);
		
	}

	return 1;
}

int drv_start_decode()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_start_dec(enc_item);
		if( ret < 0 )
			goto NOT_START;
	}

	return 1;

NOT_START:

	for( i = 0; i < 8; i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_stop_dec(enc_item);
	}
	
	return -1;
}


int drv_stop_decode()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_cif[i];
		else
			enc_item = &enc_d1[i];

		ret = Hisi_stop_dec(enc_item);
		
	}

	return 1;
}

int drv_net_start_record()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_net_cif[i];
		else
			enc_item = &enc_net_d1[i];	

		ret = Hisi_start_enc(enc_item);
		if( ret < 0 )
			goto NOT_START;
	}

	return 1;

NOT_START:

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_net_cif[i];
		else
			enc_item = &enc_net_d1[i];

		ret = Hisi_stop_enc(enc_item);
	}
	
	return -1;
}


int drv_net_stop_record()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int ret;

	for( i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_net_cif[i];
		else
			enc_item = &enc_net_d1[i];

		ret = Hisi_stop_enc(enc_item);
		
	}

	return 1;
}

int  start_stop_record(int iFlag)
{
	g_enable_rec_flag = iFlag;

	//if(g_enable_rec_flag == 1)
	//     Net_dvr_stop_client();
 	
	net_dvr_set_enc(0x3,1);
	

	
	clear_buf(&g_encodeList);
	clear_buf(&g_audioList);

	printf(" start_stop_record = %d %d\n",iFlag,get_enable_insert_data_num(&g_encodeList));

	return 1;
}



int  instant_keyframe(int cam)
{
	int i = 0;	


	for(i = 0; i < get_dvr_max_chan(0); i++ )
	{
		
	
		if( get_bit(cam,i) )
		{
			enc_cif[i].create_key_frame = 1;
			enc_d1[i].create_key_frame = 1;
			enc_hd1[i].create_key_frame = 1;
			//printf("channel %d keyframe\n",i);
		}
	}

	

	return ALLRIGHT;
}

int  instant_keyframe_net(int cam)
{
	int i = 0;
	ENCODE_ITEM * enc_item = NULL;


	for(i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
			enc_item = &enc_net_cif[i];
		else
			enc_item = &enc_net_d1[i];
	
		if( get_bit(cam,i) )
		{
			enc_net_cif[i].create_key_frame = 1;
			enc_net_d1[i].create_key_frame = 1;
			enc_net_hd1[i].create_key_frame = 1;
			//printf("channel %d keyframe\n",i);

			Hisi_create_IDR_2(i);
		}

		net_chan_send_count[i] = 0;
	}	


	

	return ALLRIGHT;
}

int is_chan_net_send(int chan)
{
	int i = 0;

	if( net_chan_send_count[chan] > 200 || net_chan_send_count[chan] < 0)
	{		
		for(i = 0; i < get_dvr_max_chan(0); i++ )		
		{
			net_chan_send_count[i] = 0;
		}
	}

	
	for(i = 0; i < get_dvr_max_chan(0); i++ )	
	{
		if( get_bit(net_local_cam,i) )
		{
			if( net_chan_send_count[chan] - net_chan_send_count[i] >=2 )
				return 0;
		}
	}

	return 1;
}


int get_net_local_cam()
{
	int cam =0;	

	if( get_dvr_max_chan(1) == 16 )	
		cam = net_local_cam&0xffff;

	if( get_dvr_max_chan(1) == 8 )	
		cam = net_local_cam&0x00ff;

	if( get_dvr_max_chan(1) == 4 )	
		cam = net_local_cam&0x000f;


	if( is_16ch_slave_machine() )
	{
		cam = net_local_cam;
	}

	return cam;
}

int is_show_net_channel(int channel)
{
	int i;	
	
	for(i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( get_bit(net_local_cam,channel) )
			return 1;
	}

	return -1;
}

int net_send_cam_total_num()
{
	int i;	
	int num = 0;

	for(i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( get_bit(net_local_cam,i) )
		{
			num++;
		}
	}


	return num;
}

int slave_net_send_cam_total_num()
{
	int i;	
	int num = 0;

	for(i = 8; i < get_dvr_max_chan(1); i++ )
	{
		if( get_bit(net_local_cam,i) )
		{
			num++;
		}
	}

	return num;
}

int get_net_low_chan()
{
	int i;	
	int num = -1;

	for(i = 0; i < get_dvr_max_chan(0); i++ )
	{
		if( get_bit(net_local_cam,i) )
		{
			//DPRINTK("net_local_cam = %d i=%d  get_dvr_max_chan=%d\n",net_local_cam,i,get_dvr_max_chan(0));
			num = i;
			break;
		}
	}

	return num;
}

GST_DRV_BUF_INFO * get_save_buf_info(char * pData)
{
	return get_enc_buf_info(&g_encodeList,pData);
}

GST_DRV_BUF_INFO * get_net_buf_info(char * pData)
{

	static GST_DRV_BUF_INFO bufinfo;
	BUF_FRAMEHEADER pheadr;

	int channel_mask = 0;	
	
	if( get_net_buf(&pheadr,pData) <= 0 )
	{		
		return NULL;
	}
	

	bufinfo.iFrameType[pheadr.index] = pheadr.iFrameType;
	bufinfo.iChannelMask = set_bit(channel_mask, pheadr.index);   // just have one channel
	bufinfo.tv.tv_sec = pheadr.ulTimeSec;
	bufinfo.tv.tv_usec = pheadr.ulTimeUsec;
	bufinfo.iBufLen[pheadr.index] = pheadr.iLength;
	bufinfo.iFrameCountNumber[pheadr.index] = pheadr.frame_num;
	bufinfo.iBufDataType = pheadr.type;

	return &bufinfo;
	
}

void clear_net_buf()
{
}


// play back function

int start_stop_play_back(int iFlag)
{
	g_decodeItem.enable_play = iFlag;

	g_decodeItem.frame_num = 0;


	instant_keyframe(0xffff);


	clear_playback_keyframe(); //make sure first decode frame is key frame

	//show_cif_img_to_middle(1);

	//clear_fb();	

	clear_buf(&g_decodeList);
//	clear_buf(&g_audioList);
	clear_buf(&g_audiodecodeList);


	if( g_decodeItem.enable_play == 1 )
	{
		if( drv_start_decode() <  0 )
		{
			DPRINTK("drv_start_decode err!\n");
		}
	}else
	{
		if( drv_stop_decode() <  0 )
		{
			DPRINTK("drv_start_decode err!\n");
		}
	}
	
	return 1;
}

int enable_insert_play_buf()
{
	return enable_insert_data(&g_decodeList);
}

int insert_play_buf(char *header,char * pData)
{
	BUF_FRAMEHEADER fheader;
	FRAMEHEADERINFO * pHeader;

	pHeader = (FRAMEHEADERINFO *)header;

	fheader.iFrameType = pHeader->ulFrameType;
	fheader.iLength       = pHeader->ulDataLen;
	fheader.index	        = pHeader->iChannel;
	fheader.standard     = pHeader->videoStandard;
	fheader.size		  = pHeader->imageSize;
	fheader.frame_num  = pHeader->ulFrameNumber;

	if(pHeader->type == 'V' )
		fheader.type           = VIDEO_FRAME;
	else
		fheader.type           = AUDIO_FRAME;

	fheader.ulTimeSec 	 = pHeader->ulTimeSec;
	fheader.ulTimeUsec = pHeader->ulTimeUsec;

	//?à?·??·?ê±2?è?ò??μ￡??a???÷3?′í￡??é?üê?ò??μêy?Yoíêó?μêy?Y?ìo?á?
	//μ?2é?′êy?Yê±￡??′2?3?à′
/*	if( get_screen_show_mode() == 4 && 
		fheader.type  == AUDIO_FRAME && g_enable_net_file_view_flag == 0)
		return -1;*/

	//printf("insert %d %d %d %d\n",fheader.iFrameType,fheader.iLength ,fheader.type ,fheader.ulTimeSec);
	

	return insert_data(&fheader,pData,&g_decodeList);
}

int enable_local_audio_play(int flag,int cam)
{
	if( flag == 1 )
	{
		local_audio_play = 1;
	}
	else
	{
		local_audio_play = 0;
	}	

	local_audio_play_cam = cam;

	//show_lcd_num(4);
	printf(" play local audio! flag=%d cam=%d\n",flag,local_audio_play_cam);


	return ALLRIGHT;
}

int insert_audio_data()
{
	unsigned char  audio_buf[MAX_AUDIO_BUF];
	unsigned char  audio_buf2[MAX_AUDIO_BUF];
	int send_length;
	struct timeval recTime;
	BUF_FRAMEHEADER fheader;
	int ret;
//	static int local_audio_block = 0;
	int count;
	int i;
	int cam;
	int max_chan = get_dvr_max_chan(0);
	NET_MODULE_ST * pst = NULL;	
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
	int j;

	pst = client_pst[0];
	
	max_chan = 1;	
	get_sys_time( &recTime, NULL );
	
	if( sll_list_item_num(&gAudiolist)  >  0  )
	{
		have_data = 1;		
	}	

	if( have_data == 0 )
	{
		usleep(10000);
		goto go_out;
	}		
		
	
	{				
		j = 0;
	
		len = sll_get_list_item(&gAudiolist,(char*)audio_buf,MAX_AUDIO_BUF,recv_packet_no[j]);
		if( len > 0 )			
		{				
		  	
			cam = j;	
			recv_packet_no[j] = recv_packet_no[j]+1;		
			
			 fheader.iLength = len;
			 fheader.iFrameType =0; 
			 fheader.index = cam;   
			 fheader.type = AUDIO_FRAME;
			 fheader.ulTimeSec = recTime.tv_sec;
			 fheader.ulTimeUsec = recTime.tv_usec;
		 	 fheader.standard = 0;		

			if( g_enable_rec_flag )
			{
				if( is_enable_insert_rec_data() > 0 && (g_pstcommand->GST_DISK_GetDiskNum() > 0))
				{
				 	insert_enc_buf(&fheader,audio_buf);
				}
			}

			if( g_net_audio_open )
			{	
				if( machine_use_audio() == 0 )
				{				
					if( net_dvr_get_net_listen_chan() == NET_DVR_CHAN)
					{
						if(	get_enable_insert_data_num(&g_audioList)>NET_BUF_REMAIN_NUM )
						{							
							insert_data(&fheader,audio_buf,&g_audioList);								
						}
					}
				}
			}

			if( local_audio_play )
			{
				if( g_enable_rec_flag != 1 )
				{		
					get_one_buf(&fheader,audio_buf,&g_audioList);
				}		
				
				if( get_bit(local_audio_play_cam, fheader.index) )
				{				
					Hisi_write_audio_buf(audio_buf,fheader.iLength,fheader.index);		
				}

			}

			if( pst != NULL && pst->client >  0 )
			{			
				memcpy(audio_buf2,&fheader,sizeof(fheader));
				memcpy(audio_buf2+sizeof(fheader),audio_buf,fheader.iLength);
				send_length = fheader.iLength + sizeof(BUF_FRAMEHEADER);
				
				if( NM_net_send_data(pst,audio_buf2,send_length,NET_DVR_NET_FRAME) < 0 )
				{
					DPRINTK("insert data error: chan=%ld type=%d length=%ld type=%ld num=%ld %d\n",
						fheader.index,fheader.type,fheader.iLength,fheader.iFrameType,
						fheader.frame_num,send_length);									
				}			
			}		
			
		}else
		{
			if( recv_packet_no[j] >= gAudiolist.head_id )
			{
				if( recv_packet_no[j] - gAudiolist.head_id > 2 )
				{
					DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gAudiolist.head_id);
					recv_packet_no[j]  = gAudiolist.head_id;						
				}
				usleep(10000);
			}else
			{
				DPRINTK("lost frame recv_packet_no=%d  head_id=%d\n",recv_packet_no[j],gAudiolist.head_id);
				recv_packet_no[j]  = gAudiolist.head_id;	
				pst->lost_frame[j] = 1;
			}
		}	
	}	
	
	 return 1;
go_out:
	return -1;
}


int decode_rate_show_audio()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;

//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;	

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > 200 )
	  {
	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));

		count = 0;

		DPRINTK("2decode frame rate: %d f/s\n",frameRate);
		
	  }

	  return 1;
	
}



int net_cam_insert_audio_data(NET_MODULE_ST * pst)
{
	unsigned char  audio_buf[MAX_AUDIO_BUF];
	struct timeval recTime;
	BUF_FRAMEHEADER fheader;
	int ret;
//	static int local_audio_block = 0;
	int count;
	int i;
	int cam;
	int max_chan = get_dvr_max_chan(0);
	unsigned char * buf_ptr = NULL;
	int send_length = 0;

	//°??·d1ê±×üóDò??μ?aê§
	max_chan = 1;	

	get_sys_time( &recTime, NULL );

	buf_ptr = audio_buf + sizeof(fheader);

	for( i = 0; i < max_chan;i++)
	{
		cam = i;
		//ret = Hisi_read_audio_buf(buf_ptr,&count,cam);
		ret = Hisi_read_audio_buf(audio_buf,&count,cam);
		if( ret < 0 )
			continue;
		//DPRINTK("read chan[%d] %d\n",cam,count);

		 fheader.iLength = count;
		 fheader.iFrameType =0; 
		 fheader.index = cam;   
		 fheader.type = AUDIO_FRAME;
		 fheader.ulTimeSec = recTime.tv_sec;
		 fheader.ulTimeUsec = recTime.tv_usec;
	 	 fheader.standard = 0;	

		 memcpy(audio_buf,&fheader,sizeof(fheader));
		 send_length = fheader.iLength + sizeof(BUF_FRAMEHEADER);

		if( NM_net_send_data(pst,audio_buf,send_length,NET_DVR_NET_FRAME) < 0 )
		{
			DPRINTK("insert data error: chan=%ld type=%d length=%ld type=%ld num=%ld %d\n",
				fheader.index,fheader.type,fheader.iLength,fheader.iFrameType,
				fheader.frame_num,send_length);									
		}				
	
		decode_rate_show_audio();
	}
	
	 return 1;
}


int audio_to_list()
{
	static unsigned char temp_list[BUF_MAX_SIZE];
	BUF_FRAMEHEADER pheadr;
	int ret;

	ret = get_one_buf(&pheadr,temp_list,&g_audioList);
	if(ret < 0 )
		return ret;

	if( g_enable_rec_flag  )
	{
		ret = insert_data(&pheadr,temp_list,&g_encodeList);
		if(ret < 0 )
			return ret;
	}

	return ALLRIGHT;
}

void solution_first_rec_delay()
{
	static struct timeval oldtime;
	struct timeval newtime;
	static int first_rec = 1;

	if( first_rec )			
	{
		get_sys_time( &newtime, NULL );


		if( oldtime.tv_sec == 0 )
			oldtime.tv_sec = newtime.tv_sec;
		
		
		if( newtime.tv_sec - oldtime.tv_sec > 2 )
		{
			first_rec = 0;
			clear_buf(&g_encodeList);
			instant_keyframe(0xff);		
			printf("==== solution_first_rec_delay !\n");
		}
	}
}


int encode_net_data()
{
	int ret=-1;
//	int count = 0;
	int save_buf_num = 0;
//	static int net_buf_enough = 0;
//	int slave_control_cam = 0;

	/*

	if( get_enable_insert_data_num(&g_netList) >= NET_BUF_REMAIN_NUM )				 
	{
		ret = Hisi_get_net_encode_data(&g_netList,video_encode_buf_2);
		if( ret != 1)
		{
			ret = -1;			
		}else
		{
			ret = 1;
		}
	}	
	*/

	return ret;
}

void net_encode_thread(void * value)
{
//	int path;
//	char * data_buf = NULL;
//	VSTREAM_INFO	*pInfo;
//	int i;

	DPRINTK(" pid = %d\n",getpid());
	
	
	net_th_flag = 1;

	while( net_th_flag )
	{
		
		if( g_enable_net_local_view_flag ==  1 )
		{
			if( g_enable_net_file_view_flag == 1 )
			{
				DPRINTK("live and play together!\n");
				usleep(500000);
				continue;
			}
			
			if( encode_net_data() < 0 )
				usleep(10);			
		}else
		{
			usleep(100);
		}
	}

	DPRINTK("thread end\n");
	
}

void net_send_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	NETSOCKET * server_ctrl;
	int ret;

	DPRINTK(" pid = %d\n",getpid());
	
	signal(SIGPIPE,SIG_IGN);	
	
	net_send_th_flag = 1;

	server_ctrl = (NETSOCKET *)net_get_server_ctrl();

	while( net_send_th_flag )
	{
		
		if( is_have_client()==1 )
		{			
			send_other_info_data(server_ctrl);
			
			ret = send_img_data(server_ctrl);
			if( ret == 0 )			
				usleep(10000);				
				
			check_play_status(server_ctrl);	
		}else
		{
			usleep(10000);
		}
	}

	return ;
}

static void net_send_boardcast_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	upd_boradcast();

	return ;
}

static void net_pc_send_boardcast_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	upd_to_pc_boradcast();	
	return ;
}


void print_sensor_type()
{
	char sensor_type_buf[100] = "";

	switch(g_sensor_type)
	{
		case APTINA_AR0130_DC_720P_30FPS:
			strcpy(sensor_type_buf,"APTINA_AR0130_DC_720P_30FPS");
			break;
		case APTINA_9M034_DC_720P_30FPS:
			strcpy(sensor_type_buf,"APTINA_9M034_DC_720P_30FPS");
			break;
		case SONY_IMX138_DC_720P_30FPS:
			strcpy(sensor_type_buf,"SONY_IMX138_DC_720P_30FPS");
			break;
		case SONY_IMX122_DC_1080P_30FPS:
			strcpy(sensor_type_buf,"SONY_IMX122_DC_1080P_30FPS");
			break;
		case OMNI_OV9712_DC_720P_30FPS:
			strcpy(sensor_type_buf,"OMNI_OV9712_DC_720P_30FPS");
			break;
		case APTINA_MT9P006_DC_1080P_30FPS:
			strcpy(sensor_type_buf,"APTINA_MT9P006_DC_1080P_30FPS");
			break;
		case OMNI_OV2710_DC_1080P_30FPS:
			strcpy(sensor_type_buf,"OMNI_OV2710_DC_1080P_30FPS");
			break;
		case HIMAX_1375_DC_720P_30FPS:
			strcpy(sensor_type_buf,"HIMAX_1375_DC_720P_30FPS");
			break;
		case GOKE_SC1035:
			strcpy(sensor_type_buf,"GOKE_SC1035");
			break;
		case GOKE_JXH42:
			strcpy(sensor_type_buf,"GOKE_JXH42");
			break;
		default:
			strcpy(sensor_type_buf,"NOKNOW SENSOR");
			break;
	}	
	

	//DPRINTK("sensor=%d  %s\n",g_sensor_type,sensor_type_buf);
}



int fab_SendUdp(char * ip,int port,char * buf,int length,int local_port)
{
	char * host_name = ip;	
	char * pVersion = NULL;
	int so_broadcast;
	char server_ip[100];
	char mac_id[100];
	int mac_hex_id = 0;
	int * pmac_hex_id = NULL;
	struct sockaddr_in serv_addr; 
	#ifdef WIN32
	SOCKET sockfd = 0;
	BOOL bReuseaddr=TRUE;	
	#else
	int  sockfd = 0;
	int  bReuseaddr=1;
	#endif
	int ret;
	
	struct sockaddr_in local;
	struct sockaddr_in from;
	int fromlen =sizeof(from);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) 
	{ 
		perror("Error: socket"); 
		goto send_error;
	}

	#ifdef WIN32
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseaddr,sizeof(BOOL)) < 0) 
	{			
		perror("setsockopt SO_REUSEADDR error \n");		
		goto send_error;
	}
	#else
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr,sizeof(int)) < 0) 
	{			
		perror("setsockopt SO_REUSEADDR error \n");		
		goto send_error;
	}
	#endif

	if( strcmp(ip,"255.255.255.255") == 0 )
	{
		#ifdef WIN32
		 BOOL bBroadcast = TRUE;
		 int num = 0; 		
		 int ret_val = setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,( const char* )&bBroadcast,sizeof(bBroadcast));
		 if(ret_val == -1)
		 {
			 perror("Failed in setsockfd \n");
			goto send_error;
		 } 
		 #else
		  int bBroadcast = 1;
		 int num = 0; 		
		 int ret_val = setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&bBroadcast,sizeof(bBroadcast));
		 if(ret_val == -1)
		 {
			 perror("Failed in setsockfd \n");
			goto send_error;
		 } 
		 #endif
	}

	
	local.sin_family=AF_INET;
	local.sin_port=htons(local_port);  ///监听端口
	local.sin_addr.s_addr=INADDR_ANY;       ///本机

	ret = bind(sockfd,(struct sockaddr*)&local,fromlen);
	if( ret == -1 )
	{
		perror("bind to error\n");
		goto send_error;
	}

	
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(port); 
	serv_addr.sin_addr.s_addr = inet_addr(host_name); 	
	

	ret = sendto(sockfd,buf,length,0,(struct sockaddr*)&serv_addr,fromlen);		

	if( ret <= 0 )
	{
		perror("send to error\n");
		goto send_error;
	}
	
	if( sockfd > 0 )
	{
		close(sockfd);
		sockfd = 0;
	}

	return 1;

send_error:
	
	if( sockfd > 0 )
	{
		close(sockfd);
		sockfd = 0;
	}


	return -1;
}


void send_ipcam_ip_to_web(int count)
{
	int ret = 0;
	static int need_send_msg = 0;
	static char config_value[200] ="";
	char buf[256] = "IamIpCam";
	
	//检测是否有机器连上了web
	if( count % 10 == 0 )
	{
		ret= cf_read_value("/tmp/http_ip","ip",config_value);
		if( ret >= 0)
		{
			need_send_msg = 10;	
			SendM2("rm -rf /tmp/http_ip");
		}
	}

	if( need_send_msg > 0 )
	{
		need_send_msg--;
		DPRINTK("send %s web ip %d\n",config_value,need_send_msg);
		fab_SendUdp(config_value,13202,buf,256,12987);		
	}
}


static void net_cam_status_check_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	int net_client_alive_check_num = 0;
	int count = 0;
	int count2 = 0;
	
	while( 1 )
	{		
		usleep(1000000/30);	
		count++;
		
		if( count >= (5*30) )
		{
			count = 0;
			set_watchdog_func();
			Net_cam_send_data_block_check();			

			{
				
				net_client_alive_check_num++;		
				if( net_client_alive_check_num % 2 == 0 )
				{
					net_client_connnet_check_func();
				}
			}			
		}

		send_ipcam_ip_to_web(count);	
		

		//if(count % 30 == 0)
		//	DPRINTK("count = %d\n",count);
		
		//DPRINTK("count = %d g_count_down_reboot_times = %d\n",count,g_count_down_reboot_times);

		if ( g_count_down_reboot_times > 0 )
		{
			g_count_down_reboot_times++;
			g_count_down_reboot_times = 1;
			if ( g_count_down_reboot_times > 61 )
			{
				g_count_down_reboot_times = 0;
				SendM2("reboot");
			}
		}

/*
		if(count == 8*30)
		{
			char save_buf[256];
			if( GetVoiceCodeInfo(save_buf,256) == 1 )
			{
				DPRINTK("GetVoiceCodeInfo get info %s\n",save_buf);
			}
		}
	*/

	}
	return ;
}


#define NVR_BOARDCAST_COMMAND_POS (12)
#define NVR_BOARDCAST_IP_NUM      (NVR_BOARDCAST_COMMAND_POS+4)
#define NVR_BOARDCAST_IP_AREA     (NVR_BOARDCAST_IP_NUM+4)
#define NVR_BOARDCAST_IP_ADDR     (NVR_BOARDCAST_IP_AREA+4)

#define NVR_COMMAND_BASE   (8000)
#define NVR_COMMAND_INFO    (NVR_COMMAND_BASE+1)
#define NVR_COMMAND_SET_IP    (NVR_COMMAND_BASE+2)
#define NVR_COMMAND_REGET_NEW_IP    (NVR_COMMAND_BASE+3)

#define NVR_MAX_CHANNEL (32)

void nvr_boardcast_op_func(char * buf)
{
	int cmd = 0;
	int ip_num = 0;
	char ip[20] = {0};
	int nvr_ip_value[4] = {0};
	int ip_value[4] = {0};
	int ip_array[NVR_MAX_CHANNEL] = {0};
	int i;
	int is_same_ip_area = 0;
	char ipcam_gw[20];
	char ipcam_area[20];

	if( Net_dvr_have_client() == 1 )			
		return ;	

	cmd = *(int*)&buf[NVR_BOARDCAST_COMMAND_POS];
	ip_num = *(int*)&buf[NVR_BOARDCAST_IP_NUM];

	if( ip_num <= 0 || ip_num > NVR_MAX_CHANNEL)
		return;

	memcpy(&ip_array[0],&buf[NVR_BOARDCAST_IP_ADDR],NVR_MAX_CHANNEL*sizeof(int));

	Nvr_get_host_ip(ip);
	if( ip[0] == 0 )
	{
		DPRINTK("Can't get  ip!\n");
		return ;
	}	

	sscanf(ip,"%d.%d.%d.%d",&ip_value[0],&ip_value[1],&ip_value[2],&ip_value[3]);

	nvr_ip_value[0] = (unsigned char)buf[NVR_BOARDCAST_IP_AREA];
	nvr_ip_value[1] = (unsigned char)buf[NVR_BOARDCAST_IP_AREA+1];
	nvr_ip_value[2] = (unsigned char)buf[NVR_BOARDCAST_IP_AREA+2];
	nvr_ip_value[3] = (unsigned char)buf[NVR_BOARDCAST_IP_AREA+3];	

	if( ip_value[0] != nvr_ip_value[0] || ip_value[1] != nvr_ip_value[1]
		|| ip_value[2] != nvr_ip_value[2] )
	{
		is_same_ip_area = 0;
	}else
	{
		is_same_ip_area = 1;
	}
	
	if( cmd ==  NVR_COMMAND_INFO)
	{
		for( i = 0; i < ip_num;i++)
		{			
			if( ip_array[i] == 0 )
				continue;
			
			if( get_ipcam_mode_num() == IPCAM_ID_MODE )
			{
				if( ip_array[i] == ipcam_get_hex_id() )
				{					
					if( is_same_ip_area == 0  && get_ipcam_id_mode_set_ip() == 1 )
					{
						sprintf(ipcam_gw,"%d.%d.%d.1",nvr_ip_value[0],
							nvr_ip_value[1],nvr_ip_value[2]);

						{
							int random_num;
							random_num = get_mac_random();
							nvr_ip_value[3] = random_num % 20 + 230;
						}

						sprintf(ipcam_area,"%d.%d.%d.%d",nvr_ip_value[0],
							nvr_ip_value[1],nvr_ip_value[2],nvr_ip_value[3]);

						g_set_net_to_251 = 1;
						
						set_net_parameter_dev(net_dev_name,ipcam_area,6802,ipcam_gw,
						"255.255.255.0",ipcam_gw,ipcam_gw);

						DPRINTK("ipcam change IP area: %s %s\n",ipcam_area,ipcam_gw);

						start_id_mode_ip_set();	
						sleep(2);
						g_set_net_to_251 = 0;
					}
				}
			}			
		}		
	}

	if( cmd ==  NVR_COMMAND_REGET_NEW_IP)
	{
		 DPRINTK("1111  ipcam change IP  ip_num=%d ip_array[0]=%X id=%X\n",
		 	ip_num,ip_array[0],ipcam_get_hex_id());
		  
		for( i = 0; i < ip_num;i++)
		{
			if( ip_array[i] == 0 )
				continue;
			
			if( get_ipcam_mode_num() == IPCAM_ID_MODE )
			{
				if( ip_array[i] == ipcam_get_hex_id() )
				{	
				       DPRINTK("NVR_COMMAND_REGET_NEW_IP  ipcam change IP \n");
					start_id_mode_ip_set();
				}
			}			
		}		
	}

	if( cmd ==  NVR_COMMAND_SET_IP)
	{
	}
	
}


int net_cam_ip_test()
{	
	char buf[256];
	struct sockaddr_in serv_addr,addr;
	int bd;
	int ret;
	int len;
	int sockfd;
	int i;
	ValNetStatic2 net_config;
	int * pmac_hex_id = NULL;



	sleep(15);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) 
	{ 
		DPRINTK("Error: socket"); 
		return; 
	} 
	memset(&addr, 0, sizeof(addr)); 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(IP_TEST_PORT); 
	addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(host_name);//htonl(INADDR_ANY);// 

	bd = bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));


	{
		struct  linger  linger_set;
		struct timeval stTimeoutVal;		
		linger_set.l_onoff = 1;
		linger_set.l_linger = 0;

	    if (setsockopt( sockfd, SOL_SOCKET, SO_LINGER , (char *)&linger_set, sizeof(struct  linger) ) < 0 )
	    {
			printf( "setsockopt SO_LINGER  error\n" );
		}

		stTimeoutVal.tv_sec = 100;
		stTimeoutVal.tv_usec = 0;
		if ( setsockopt( sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	    {
			printf( "setsockopt SO_SNDTIMEO error\n" );
		}
			 	
		if ( setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&stTimeoutVal, sizeof(struct timeval) ) < 0 )
	    {
			printf( "setsockopt SO_RCVTIMEO error\n" );
		}
	}


	DPRINTK("recv boardcast thread start!!\n");

	while(1)
	{
		len = sizeof(serv_addr); 
		memset(&serv_addr, 0, len);
		memset(buf,0,256);
		
		ret = recvfrom(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serv_addr,(socklen_t*)&len); 
		
		printf("RecvieIP %s buf=%s\n",inet_ntoa(serv_addr.sin_addr),buf);

		if( ret > 0 )
		{			
			if(strcmp(buf,"NvrInfo") == 0 )
			{				
				nvr_boardcast_op_func(buf);
			}

			if( strcmp(buf,"IpTest") == 0 )
			{				
				strcpy(buf,"IamIpCam");

				buf[CONNECT_POS] = (char)Net_dvr_have_client();
				buf[AUTHORIZATION_POS] = (char)get_dvr_authorization();
				buf[CAM_VIDEO_TYPE] = Net_cam_get_support_max_video();
				
				serv_addr.sin_port = htons(BORADCAST_PORT); 

				if( buf[NET_REMOTE_SET_IP] == 11 )
				{
					memcpy(&net_config,&buf[NET_CAM_NET_CONFIG],sizeof(net_config));

					pmac_hex_id = (int *)&buf[NET_CAM_ID_INFO];


					if( ipcam_get_hex_id() == *pmac_hex_id)
					{
						DPRINTK("id  %X  %X\n",ipcam_get_hex_id(),*pmac_hex_id);
						
						if( g_pstcommand->GSt_NET_set_ipcam_net_info!= 0 )
						{	
							int t_Flag = 1;
							DPRINTK("Remote Set Ip %d.%d.%d.%d Net_dvr_have_client=%d netstat_have_rstp_user=%d Net_have_fab_client=%d\n",net_config.IpAddress[0],
								net_config.IpAddress[1],net_config.IpAddress[2],net_config.IpAddress[3],
								Net_dvr_have_client(),netstat_have_rstp_user(),Net_have_fab_client());

							if ( is_wifi_connect_send_data() == 1 )
								t_Flag = 0;
							
							if( Net_have_fab_client() <= t_Flag )
							{
							/*
								g_pstcommand->GSt_NET_set_ipcam_net_info(&net_config);
								Net_cam_mtd_store();
								command_drv("reboot");
							*/
								char new_ip[50] = "";
								char sub_mask[50] = "";
								char gw[50] = "";
								char ddns1[50] = "";
								char ddns2[50] = "";
								int server_port;
								int http_port;	
								
								
								IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
								IPCAM_NET_ST * pNetSt = &pAllInfo->net_st;
								pNetSt->ipv4[0] = net_config.IpAddress[0];
								pNetSt->ipv4[1] = net_config.IpAddress[1];
								pNetSt->ipv4[2] = net_config.IpAddress[2];
								pNetSt->ipv4[3] = net_config.IpAddress[3];

								pNetSt->netmask[0] =  net_config.SubNet[0];
								pNetSt->netmask[1] =  net_config.SubNet[1];
								pNetSt->netmask[2] =  net_config.SubNet[2];
								pNetSt->netmask[3] =  net_config.SubNet[3];

								pNetSt->gw[0] = net_config.GateWay[0];
								pNetSt->gw[1] = net_config.GateWay[1];
								pNetSt->gw[2] = net_config.GateWay[2];
								pNetSt->gw[3] = net_config.GateWay[3];

								pNetSt->dns1[0] = net_config.DDNS1[0];
								pNetSt->dns1[1] = net_config.DDNS1[1];
								pNetSt->dns1[2] = net_config.DDNS1[2];
								pNetSt->dns1[3] = net_config.DDNS1[3];

								pNetSt->dns2[0] = net_config.DDNS2[0];
								pNetSt->dns2[1] = net_config.DDNS2[1];
								pNetSt->dns2[2] = net_config.DDNS2[2];
								pNetSt->dns2[3] = net_config.DDNS2[3];
								save_ipcam_config_st(pAllInfo);	
								get_server_port(&server_port,&http_port);	

								sprintf(new_ip,"%d.%d.%d.%d",pNetSt->ipv4[0],pNetSt->ipv4[1] ,pNetSt->ipv4[2] ,pNetSt->ipv4[3] );
								sprintf(sub_mask,"%d.%d.%d.%d",pNetSt->netmask[0],pNetSt->netmask[1] ,pNetSt->netmask[2] ,pNetSt->netmask[3] );
								sprintf(gw,"%d.%d.%d.%d",pNetSt->gw[0],pNetSt->gw[1] ,pNetSt->gw[2] ,pNetSt->gw[3] );
								sprintf(ddns1,"%d.%d.%d.%d",pNetSt->dns1[0],pNetSt->dns1[1] ,pNetSt->dns1[2] ,pNetSt->dns1[3] );
								sprintf(ddns2,"%d.%d.%d.%d",pNetSt->dns2[0],pNetSt->dns2[1] ,pNetSt->dns2[2] ,pNetSt->dns2[3] );
								set_net_parameter_dev(net_dev_name,new_ip,server_port,gw,sub_mask,ddns1,ddns2);
								printf("echo '' > /ramdisk/camsetDone");
							}
						}
					}

					*pmac_hex_id = 0;

					memset(&buf[NET_CAM_NET_CONFIG],0x00,sizeof(net_config));

				}

				

				if(dvr_is_recording() == 1 )		
					buf[CONNECT_POS] = 1;
			
				//sendto(sockfd,buf,sizeof(buf),0,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in));
			}
		}else
		{
			//DPRINTK("time out!\n");
		}

	}

	if( sockfd > 0)
	{
		close(sockfd);
		sockfd = 0;
	}


	return 1;
	
}

static void net_ip_test_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	net_cam_ip_test();

	return ;
}

// 5 second check once.
void Net_cam_send_data_block_check()
{
	static int net_check_is_send_frame = 0;
	static int net_check_is_send_frame_failed_count = 0;
	static int net_check_not_user_count = 0;	

	
	if( Net_cam_have_client_check_all_socket() == 1 )
	{	
		//¨?DD?¨￠??¨?¨|?¨￠??ê?2??¨￠¨oy?Y?ê?¨???a??o?￥¨oy¨¤???¨?2a??ê
		Net_dvr_check_client(); 		
	
		if( net_check_is_send_frame != Net_dvr_get_send_frame_num() )
		{		
			//DPRINTK("(%d,%d) %d\n",net_check_is_send_frame,Net_dvr_get_send_frame_num(),net_check_is_send_frame_failed_count);
			net_check_is_send_frame = Net_dvr_get_send_frame_num();
			net_check_is_send_frame_failed_count = 0;
		}else
		{	
			
			net_check_is_send_frame_failed_count++;

			DPRINTK("%d\n",net_check_is_send_frame_failed_count);

			if( net_check_is_send_frame_failed_count > 10 )
			{
				DPRINTK("can't send frame data,so reboot\n");
				//Stop_GM_System();
				sleep(3);
				command_drv("reboot");
				sleep(10);
			}
		}	

		net_check_not_user_count = 0;
	}		
	else
	{
		net_check_not_user_count++;

		DPRINTK("net_check_not_user_count = %d\n",net_check_not_user_count);

		if( net_check_not_user_count >  (1800/5) )
		{
			/*
			DPRINTK("no user connect 30 minutes,so reboot\n");
			Stop_GM_System();					
			command_drv("reboot");
			sleep(10);
			*/
		}
	}
}

#define CHECK_NET_STATUS_TIME (3)

void net_status_check_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	int bit[6];
	int frame_rate[6];
	int gop[6];
	int bit2[6];
	int frame_rate2[6];
	int gop2[6];
	unsigned long net_stat = 0;
	int net_stats_result = 0;
	int reset = 0;
	int i =0;
	int adjust_bit = 0;
	int adjuse_gop = 0;
	int check_down_time = 0;
	int check_up_time = 0;
	int check_zero_time = 0;
	int need_reset = 0;
	int net_usr_check = 0;
	int sub_enc_control_compare = 0;
	int sub_enc_same_count = 0;
	int open_for_rstp = 0;	
	int net_value = 10000;
	int have_modify_rtsp_enc_set = 0;

	sleep(10);

	DPRINTK("start\n");

	/*if( nettset_dev() == 1 )
	{
		//use dev eth0:
		enc_d1[0].bitstream = 1024*4;
		enc_d1[3].bitstream = 768;
	}*/
	
	while(1)
	{
				
		for( i = 0; i < MN_MAX_BIND_ENC_NUM ; i++ )
		{
			if( enc_d1[i].bitstream != bit[i] )
				reset = 1;

			if( enc_d1[i].framerate != frame_rate[i] )
				reset = 1;

			if( enc_d1[i].gop != gop[i] )
				reset = 1;

			if( reset == 1 )
			{
				DPRINTK("change (%d,%d,%d) (%d,%d,%d)\n",
					enc_d1[i].bitstream,enc_d1[i].framerate,enc_d1[i].gop,
					bit[i],frame_rate[i],gop[i] );
			}
		}
	

		if( reset == 1 )
		{
			reset = 0;	

			for( i = 0; i < MN_MAX_BIND_ENC_NUM ; i++ )
			{
				bit[i] = enc_d1[i].bitstream;
				frame_rate[i] = enc_d1[i].framerate;
				gop[i] = enc_d1[i].gop;				
			}			

			for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++)
			{
				bit2[i] = bit[i] ;
				frame_rate2[i] = frame_rate[i];
				gop2[i] = gop[i];				
				Hisi_net_cam_enc_set(i,bit[i],frame_rate[i],gop[i]);

				if(get_appselect_num() == 1)
				{
					if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() &&  (i == 1) )
						filo_set_save_num(cycle_enc_buf,frame_rate[i]*4);
					
					if( TD_DRV_VIDEO_SIZE_D1 == get_cur_size() &&  (i == 0) )
						filo_set_save_num(cycle_enc_buf,frame_rate[i]*4);					
				}
			}
		}

	
/* 20170907 bylx no ctrl fps
		net_stat = netstat_main();
		insert_netstat_array(net_stat);
		net_stats_result = get_netstat_result();
		//DPRINTK("adjust  %ld  get_netstat_result=%d\n",net_stat,net_stats_result);
		
		
		if( 0 && Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_1080P )
		{
			if( netstat_have_rstp_user() == 1 ) //当有rtsp用户时，帧率减半
			{
				int adjust_bit,adjuse_gop,adjust_fps;
				int now_fps,now_bitstream,now_gop;
				
			       for( i = 0; i < 2; i++)
			       	{
					if( Hisi_get_stream_fps(i) > 15 )
					{
						DPRINTK("[%d] rtsp limit frame_rate to 15\n",i);							
						now_fps = Hisi_get_stream_fps(i);
						now_bitstream = Hisi_get_stream_bitstream(i);
						now_gop = Hisi_get_stream_gop(i);
						adjust_bit = now_bitstream /now_fps * 15;	
						adjuse_gop = now_gop /now_fps * 15;
						adjust_fps = 15;
						Hisi_net_cam_enc_set(i,adjust_bit,adjust_fps,adjuse_gop);	

						have_modify_rtsp_enc_set = 1;
					}
			       	}

				 sleep(1);
				 continue;
			}else
			{
				if( have_modify_rtsp_enc_set == 1 )
				{
					DPRINTK("Restore enc set,after rtsp used!\n");
					have_modify_rtsp_enc_set = 0;
					reset = 1;
					continue;
				}
			}
		}			

		
		if( Net_dvr_have_client() == 1  )		
		{
			// not set enc for pc  ipcam.
			//if( (net_stat > net_value)  || get_video_send_lost_flag())		
			if( net_stats_result == 1 )
			{	
				//if( strcmp(net_dev_name,"ra0") == 0 )
				//{
				//	net_value = 200000;
				//}
			
				//DPRINTK("adjust -%ld \n",net_stat);
				check_up_time++;
				need_reset = 1;

				if( check_up_time >= 2 )
				{					
				
					for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
					{
						if( frame_rate2[i] > 24) //发现网络卡顿，直接调到帧率的中间值。
						{
							frame_rate2[i] = 13;
						}	
						
						frame_rate2[i] = frame_rate2[i] -2;

						DPRINTK("111 frame_rate2[%d] = %d\n",i,frame_rate2[i]);											

						if( frame_rate2[i] < 6)
						{
							frame_rate2[i] = 6;		
							//continue;
						}

						//DPRINTK("222 frame_rate2[%d] = %d\n",i,frame_rate2[i]);

						if( (frame_rate2[i] >= 6)  &&  
							(frame_rate2[i] <=  frame_rate[i])  )
						{
							adjust_bit = bit[i] /frame_rate[i] * frame_rate2[i];	
							adjuse_gop = gop[i] /frame_rate[i] * frame_rate2[i];	
							Hisi_net_cam_enc_set(i,adjust_bit,frame_rate2[i],gop[i]);
						}
					}

					check_up_time = 0;
				}
			}else
			{
				check_up_time = 0;
			}

			
			if( net_stats_result == -1 )
			{
				//DPRINTK("adjust + %ld \n",net_stat);		

				check_down_time++;
				need_reset = 1;
				if( check_down_time > CHECK_NET_STATUS_TIME*6)
				{
					for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
					{
						frame_rate2[i] = frame_rate2[i] + 1;

						if( frame_rate2[i] >  frame_rate[i] )
							frame_rate2[i] = frame_rate[i] + 1;		


						if( (frame_rate2[i] >= 2)  &&  
							(frame_rate2[i] <=  frame_rate[i])  )
						{				
							adjust_bit = bit[i] /frame_rate[i] * frame_rate2[i];	
							adjuse_gop = gop[i] /frame_rate[i] * frame_rate2[i];
							Hisi_net_cam_enc_set(i,adjust_bit,frame_rate2[i],gop[i]);
						}
					}

					check_down_time = 0;
				}
			}	else
			{
				if( net_stat >= 5000)
				{
					check_down_time = 0;
				}
			}

		}

		if( Net_dvr_have_client() == 0  )
		{
			if( net_stat == 0 )
			{
				check_zero_time++;

				if( check_zero_time > CHECK_NET_STATUS_TIME)
				{
					if( need_reset == 1 )
					{
						reset = 1;
						need_reset = 0;					
					}
				}
			}else
			{
				check_zero_time = 0;
			}

			
		}	
		*/
		usleep(300000);
	}

	return ;
}

int get_rec_audio_cam()
{
	return g_pstcommand->GST_FTC_AudioRecCam(GETRECCAM);
}

int Net_dvr_get_machine_speak_mode()
{
	return audio_speak_mode;
}

int Net_dvr_get_machine_speak_cam()
{
	return audio_speak_cam;
}

int Net_dvr_get_machine_speak_work()
{
	return audio_speak_work;
}

void audio_encode_thread()
{
	int ret;
	int rec_audio_cam = 0;
	int count=0;

	DPRINTK(" pid = %d\n",getpid());

	while(g_encode_flag)
	{
/*		rec_audio_cam = get_rec_audio_cam();		
		
	 	if(( (g_net_audio_open == 1 )  || (  Net_dvr_get_machine_speak_work() == 1)
			||  (( g_enable_rec_flag == 1)&&(rec_audio_cam!=0))  )
						&&  get_enable_insert_data_num(&g_audioList) > 16)
						*/
		if(1)
	 	{	 		
	 		 if( (ret = insert_audio_data()) < 0 )
			{
				trace("insert_audio_data error!\n");
				
			}		

			usleep(10000);
	 	}
		else
		{		
			usleep(10000);
		}
	}
}

int  audio_play_buf()
{
	unsigned char  audio_buf[MAX_AUDIO_BUF];	
	BUF_FRAMEHEADER header;
	int ret;

	get_one_buf(&header,audio_buf,&g_audiodecodeList);

	//ret = GM_write_audio_buf(audio_buf,header.iLength);
	ret = Hisi_write_audio_buf(audio_buf,header.iLength,0);	
	if( ret < 0 )
		return ret;	

	return ret;
}

void audio_decode_thread()
{
	int ret;

	DPRINTK(" pid = %d\n",getpid());


	while(g_decode_flag)
	{
	 	if( (g_decodeItem.enable_play == 1)  && (enable_get_data(&g_audiodecodeList) > 0) )
	 	{
	 		 if( (ret = audio_play_buf()) < 0 )
			{
				trace("insert_audio_data error!");				
			}				
	 	}
		else
			usleep(10000);
	}
}



void decode_thread()
{
	SET_PTHREAD_NAME(NULL);
	 BUF_FRAMEHEADER fheader;
	 int ret;
	 int clear_play_buf = 0;


	DPRINTK(" pid = %d\n",getpid());

	 while(g_decode_flag)
	 {
	 	
		if( ((g_decodeItem.enable_play == 1) || (g_enable_net_file_view_flag == 1)) 
					&& enable_get_data(&g_decodeList) > 0 )
		{		
			
			if( g_enable_net_file_view_flag )
			{
				
				 if( (enable_get_data(&g_decodeList) > 0)  &&
			 				(is_enable_insert_net_data() > 0) )
				 {		
				 		
						if( list_to_list(NULL, &g_decodeList) < 0 )
							trace("list_to_list error!");						
				 }else
				 	usleep(10000);
			}else
			{		
				ret =  get_frame_decode_hisi(&fheader ,undecode_video_buf, video_setting.decode_width, video_setting.decode_height) ;
				if( ret < 0 )
				{
					trace(" get_frame_from_buf_and_decode error!\n");
					return ;	
				}

				/*if( g_pstcommand->GST_FTC_get_play_status() == 1 )
				{
					clear_play_buf = 1;
				}*/
			}

		}
		else
		{
			

		/*	if( g_pstcommand->GST_FTC_get_play_status() == 0)
			{
				if( clear_play_buf == 1 )
				{
					int i;					
					clear_play_buf = 0;

					for (i = 0; i < get_dvr_max_chan(0); i++)
					{
						Hisi_clear_vo_buf(i);
					}					
				}
			}
		  */
			
			usleep(10000);
		}

	 }


}



#define I_FRAME_CREATE_NUM 80

int init_mpeg4_dev(ENCODE_ITEM * enc_item,video_profile *setting)
{
	DPRINTK("in\n");
	return 1;
}


int close_mpeg4_dev(ENCODE_ITEM * enc_item)
{
	DPRINTK("in\n");
	return 1;
}


int mpeg4_encode(ENCODE_ITEM * enc_item, unsigned char *frame, void * data)
{
	DPRINTK("in\n");
	return 1;
}





int xvid_decode(void *data, int *got_picture, uint8_t *buf, int buf_size,DECODE_ITEM * dec_item)
{
	DPRINTK("in\n");
	return 1;
}

int mac_decode(void *data, int *got_picture, uint8_t *buf, int buf_size,int chan)
{
	DPRINTK("in\n");
	return 1;
}


int xvid_decoder_end(DECODE_ITEM * dec_item)
{
	DPRINTK("in\n");
	return 1;
}

int xvid_decoder_init(DECODE_ITEM * dec_item)
{
	DPRINTK("in\n");
	return 1;
}

int test_decode_fuc( uint8_t *buf, int buf_size)
{
	DPRINTK("in\n");
	return 1;
}

int start_stop_net_view(int flag , int play_cam)
{	
	clear_net_buf();

	play_cam = 0x01;

	net_local_cam = play_cam;	

	if( flag == 1 )
	{
		g_enable_net_local_view_flag = 1;

		if(Net_dvr_get_net_mode() == TD_DRV_VIDEO_SIZE_CIF)
		{
			net_dvr_set_enc((0x01 << 1),1);	
		}else
		{
			net_dvr_set_enc((0x01 << 0),1);		
			
		}
			
	}
	else
	{
		g_enable_net_local_view_flag = 0;
			net_dvr_set_enc((0x01 << 1),0);
	}	

	printf(" g_enable_net_local_view_flag = %d  cam=%x\n",g_enable_net_local_view_flag,net_local_cam);

	return 1;
}

int start_stop_net_file_view(int flag)
{
	clear_net_buf();

	if( flag == 1 )
		g_enable_net_file_view_flag = 1;
	else
		g_enable_net_file_view_flag = 0;	

	if( g_enable_net_file_view_flag == 0 )
	{
		//è·±￡ μ￥?· í?????·?oó￡?fb ±????y3￡μ?d1 to d1??ê???2?ê?cif to d1
		//set_mult_play_cam(0x0f);
	}

	DPRINTK(" g_enable_net_file_view_flag = %d\n",g_enable_net_file_view_flag);

	return 1;
}

int get_net_file_view_flag()
{
	printf(" g_enable_net_file_view_flag1 = %d\n",g_enable_net_file_view_flag);
	return g_enable_net_file_view_flag;
}

void Stop_GM_System()
{
	int i = 0;

	open_rstp_server = 0;	
	GM_CVBS_CLOSE();
	Net_dvr_stop_client();

	sleep(3);	
	
	for( i = MN_MAX_BIND_ENC_NUM - 1; i >= 0; i-- )
	{
		GM_net_cam_stop_enc_main_bs(&gm_enc_st[i],1);	
	}

	for( i = 0; i <MN_MAX_BIND_ENC_NUM; i++ )			
	{
		GM_close_main_enc(&gm_enc_st[i]);
	}

	GM_close_system();

	DPRINTK("end!\n");

	sleep(2);
	
}



void close_all_device()
{
	int i = 0;	

	all_encode_flag = 0;
	Stop_GM_System();	
	return;
	
	g_encode_flag = 0;
	g_decode_flag = 0;	
	g_lcdflag = 0;
	net_th_flag = 0;
	net_send_th_flag = 0;
	buf_check_th_flag = 0;

	Net_dvr_stop_client();

	usleep(200000);

	for( i = MN_MAX_BIND_ENC_NUM - 1; i >= 0; i-- )
	{
		GM_net_cam_stop_enc_main_bs(&gm_enc_st[i],1);	
	}

	for( i = 0; i <MN_MAX_BIND_ENC_NUM; i++ )			
	{
		GM_close_main_enc(&gm_enc_st[i]);
	}

	GM_close_system();
	
	
	#ifdef USE_AUDIO
	GM_destroy_audio();
	#endif

	#ifdef NET_USE
	stop_net_server();
	#endif	



	Hisi_stop_vo(1);	
	Hisi_stop_vi();
	Hisi_destroy_dev();
	release_custom_buf();	
	destroy_buf_list(&g_encodeList);	
	destroy_buf_list(&g_decodeList);
	destroy_buf_list(&g_audioList);
	destroy_buf_list(&g_audiodecodeList);
	release_all_variable();

}

int close_dec_enc()
{
	int i;

	if( g_enable_rec_flag == 1 || g_decodeItem.enable_play == 1 )
	{
		DPRINTK("error: rec flag = %d, decode flag = %d\n",g_enable_rec_flag,g_decodeItem.enable_play );
		return ERROR;
	}

	 switch( cur_size )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				close_mpeg4_dev(&enc_cif[i]);					
			}

			xvid_decoder_end(&dec_cif);		
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				close_mpeg4_dev(&enc_d1[i]);					
			}

			xvid_decoder_end(&dec_d1);
			break;
		case TD_DRV_VIDEO_SIZE_HD1:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				close_mpeg4_dev(&enc_hd1[i]);					
			}
			xvid_decoder_end(&dec_hd1);
			break;
		default:
			DPRINTK("size_mode =%d error\n", cur_size);
			return ERROR;
	}

	 return ALLRIGHT;
	
}

int close_net_enc()
{
	int i;

	 switch( cur_size )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )			
			{					
				close_mpeg4_dev(&enc_net_cif[i]);
				printf(" close_mpeg4_dev %d \n",i);
			}	
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				close_mpeg4_dev(&enc_net_d1[i]);	
				printf(" close_mpeg4_dev %d \n",i);
			}	
			break;
		case TD_DRV_VIDEO_SIZE_HD1:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				close_mpeg4_dev(&enc_net_hd1[i]);	
				printf(" close_mpeg4_dev %d \n",i);
			}	
			break;
		default:
			DPRINTK("size_mode =%d error\n", cur_size);
			return ERROR;
	}

	 return ALLRIGHT;
}




#ifdef USE_H264_ENC_DEC

int get_enc_quality(float bitrates,int frame_rate)
{
	int quality = 30;
	float obitrates = 0;


	//printf("old %f %d\n",bitrates,frame_rate);

	obitrates = div_safe2( bitrates , frame_rate) * 25;

	 if( cur_size == TD_DRV_VIDEO_SIZE_CIF)
	 {
	 	if( obitrates > 450)
			quality = 28;
		else if ( (obitrates <= 450)  &&  (obitrates > 300)  )
			quality = 32;
		else
			quality = 33;
	 }
	 
 	if( cur_size == TD_DRV_VIDEO_SIZE_D1)
	 {
	 	if( obitrates > 1800)
			quality = 29;
		else if ( (obitrates <= 1800)  &&  (obitrates > 1300)  )
			quality = 31;
		else
			quality = 33;
	 }

	//printf(" obitrates = %f quality=%d\n",obitrates,quality);
	
	return quality;
}

int net_bit_change(int bit_stream,int frame_rate)
{
	int bitstream = 0;

	bitstream = div_safe2(bit_stream , 25) * frame_rate;

	return bitstream;
}

#endif



int change_net_frame_rate(int bit_stream,int frame_rate)
{
	int ret;
//	int tmp_rate;
//	int tmp_framerate;
//	static int run_num = 0;

	DPRINTK(" bit_stream=%d  framerate=%d\n",bit_stream,frame_rate);
	
	

	if(video_net_setting.bit_rate != bit_stream ||
		video_net_setting.framerate != frame_rate )
	{
		
		video_net_setting.bit_rate = bit_stream;		
		video_net_setting.framerate = frame_rate;


		//tmp_rate = video_net_setting.bit_rate;
	//	video_net_setting.bit_rate = net_bit_change(video_net_setting.bit_rate,frame_rate);

		
		//ret = set_net_frame_rate(&video_net_setting,cur_standard,cur_size);
		DPRINTK("(%d,%d,%d)\n",video_net_setting.bit_rate,video_net_setting.framerate,video_net_setting.gop_size);
		ret = set_net_frame_rate_hisi(&video_net_setting,cur_standard,cur_size);	

		//video_net_setting.bit_rate = tmp_rate;
		
		if( ret < 0 )
		{
			printf(" set_net_encode error!bit_stream=%d frame_rate =%d\n",bit_stream,frame_rate);
			stop_net_server();
			return -1;
		}else
		{
			printf(" set_net_encode success!bit_stream=%d frame_rate =%d\n",bit_stream,frame_rate);
		}
		
	}

	return 1;
}

int set_net_frame_rate(video_profile * seting,int standard,int size_mode)
{	
	int i;
//	int favc_quant = 0;
	switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{			
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate - 4;
				enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000, enc_frame_rate_ctrl_net[i].frame_rate);

				
				
				#ifdef USE_H264_ENC_DEC

				printf("net cif bitrate = %d framerate=%d \n",seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);

				enc_net_cif[i].bitstream = seting->bit_rate * 1000;
				enc_net_cif[i].framerate = enc_frame_rate_ctrl_net[i].frame_rate;
					
				if( enc_frame_rate_ctrl_net[i].frame_rate > 30 )
					enc_frame_rate_ctrl_net[i].frame_rate = 25;				
				
					#ifdef USE_H264_RATE_CTRL					
					memset(&enc_net_cif[i].ratec,0,sizeof(H264RateControl));	
					H264RateControlInit(&enc_net_cif[i].ratec,(float)seting->bit_rate*1000,
					RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, (float)enc_frame_rate_ctrl_net[i].frame_rate,
					MAX_QUANT,1,INIT_QUANT); 
					enc_net_cif[i].favc_quant = INIT_QUANT;
					#else
					enc_net_cif[i].enc_quality = get_enc_quality(seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);
					#endif
					
				#else
				printf("1 bitrate = %d framerate=%d \n",seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);
				if( enc_frame_rate_ctrl_net[i].frame_rate > 30 )
					enc_frame_rate_ctrl_net[i].frame_rate = 25;
				RateControlInit(&enc_net_cif[i].ratec,seting->bit_rate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
	                    ARG_RC_BUFFER, get_framerate(enc_frame_rate_ctrl_net[i].frame_rate),31,1,4);
				#endif


			}		
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:
			if(rec_type == TYPE_QUAD)
			{
			
			}
			else
			{
				for( i = 0; i < get_dvr_max_chan(0); i++ )	
				{				
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate - 4;
					enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl_net[i].frame_rate);
					
					#ifdef USE_H264_ENC_DEC
					printf("net d1 bitrate = %d framerate=%d \n",seting ->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);

					enc_net_d1[i].bitstream = seting->bit_rate * 1000;
					enc_net_d1[i].framerate = enc_frame_rate_ctrl_net[i].frame_rate;

					if( enc_frame_rate_ctrl_net[i].frame_rate > 30 )
						enc_frame_rate_ctrl_net[i].frame_rate = 25;		
					
						#ifdef USE_H264_RATE_CTRL						
						memset(&enc_net_d1[i].ratec,0,sizeof(H264RateControl));	
						H264RateControlInit(&enc_net_d1[i].ratec,(float)seting->bit_rate*1000,
						RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, (float)enc_frame_rate_ctrl_net[i].frame_rate,
						MAX_QUANT,1,INIT_QUANT);  
						enc_net_d1[i].favc_quant = INIT_QUANT;
						#else
						enc_net_d1[i].enc_quality = get_enc_quality(seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);
						#endif
					
					#else
					
					printf("d1 bitrate = %d framerate=%d \n",seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate);
					if( enc_frame_rate_ctrl_net[i].frame_rate > 30 )
						enc_frame_rate_ctrl_net[i].frame_rate = 25;
					RateControlInit(&enc_net_d1[i].ratec,seting->bit_rate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
		                    		ARG_RC_BUFFER, get_framerate(enc_frame_rate_ctrl_net[i].frame_rate),31,1,4);

					#endif
				}	
			}
			
			break;	
		case TD_DRV_VIDEO_SIZE_HD1:
			
			break;
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return ERROR;
	}

	return ALLRIGHT;
}



int set_net_frame_rate_hisi(video_profile * seting,int standard,int size_mode)
{	
	int i;
//	int favc_quant = 0;
	switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )			
			{			
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate ;
				enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl_net[i].frame_rate);
				enc_frame_rate_ctrl_net[i].gop_size = seting->gop_size;
				
				if( enc_frame_rate_ctrl_net[i].frame_rate > 30 )
					enc_frame_rate_ctrl_net[i].frame_rate = 25;	


				enc_net_cif[i].bitstream = seting->bit_rate;
				enc_net_cif[i].framerate = enc_frame_rate_ctrl_net[i].frame_rate;
				enc_net_cif[i].gop = seting->gop_size;

				printf("net cif bitrate = %d framerate=%d gop=%d\n",
					seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate,enc_net_cif[i].gop);

				enc_d1[2].bitstream =  seting->bit_rate;
				enc_d1[2].framerate = enc_frame_rate_ctrl_net[i].frame_rate;
				enc_d1[2].gop =  seting->gop_size;

				
				if( Hisi_set_encode(&enc_net_cif[i]) < 0 )
					DPRINTK("Hisi_set_encode [%d] err\n",i);	

			}				
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:
			
				for( i = 0; i < get_dvr_max_chan(0); i++ )	
				{				
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate ;
					enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl_net[i].frame_rate);
					enc_frame_rate_ctrl_net[i].gop_size = seting->gop_size;														


					enc_net_d1[i].bitstream = seting->bit_rate;
					enc_net_d1[i].framerate = enc_frame_rate_ctrl_net[i].frame_rate;
					enc_net_d1[i].gop = seting->gop_size;

					printf("net d1 bitrate = %d framerate=%d gop=%d\n",
						seting->bit_rate,enc_frame_rate_ctrl_net[i].frame_rate,enc_net_d1[i].gop);

					enc_d1[2].bitstream =  seting->bit_rate;
					enc_d1[2].framerate = enc_frame_rate_ctrl_net[i].frame_rate;
					enc_d1[2].gop =  seting->gop_size;

				
					if( Hisi_set_encode(&enc_net_d1[i]) < 0 )
						DPRINTK("Hisi_set_encode [%d] err\n",i);	
				}				
			
			break;	
	
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return ERROR;
	}

	return ALLRIGHT;
}


int set_net_encode(video_profile * seting,int standard,int size_mode)
{
	int i;
	int video_step = 0;

	i = close_net_enc();
	if( i < 0 )
	{
		printf("close_net_enc error!\n");
		return ERROR;
	}
	
	switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:			
			video_step = get_dvr_max_chan(0);
			for( i = 0; i < get_dvr_max_chan(0); i++ )						
			{
				memset(&enc_net_cif[i],0x00,sizeof( enc_net_cif[i]) );
				
				enc_net_cif[i].ichannel = i + video_step;
				enc_net_cif[i].mmp_addr = NULL;
				enc_net_cif[i].encode_fd = 0;
				enc_net_cif[i].mode = standard;
				enc_net_cif[i].size = size_mode;
				enc_net_cif[i].is_net_enc = 1;
				enc_net_cif[i].bitstream = seting->bit_rate;
				enc_net_cif[i].framerate = seting->framerate;
				enc_net_cif[i].gop = seting->gop_size;
				enc_net_cif[i].VeGroup = i + video_step;
				
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate - 4;
				enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl_net[i].frame_rate);

				if( Hisi_create_encode(&enc_net_cif[i]) < 0 )
					goto RELEASE;			
			}		
			
			break;
			case TD_DRV_VIDEO_SIZE_D1:				
				video_step = get_dvr_max_chan(0);
				for( i = 0; i < get_dvr_max_chan(0); i++ )					
				{
					memset(&enc_net_d1[i],0x00,sizeof( enc_net_d1[i]) );
					
					enc_net_d1[i].ichannel = i + video_step;
					enc_net_d1[i].mmp_addr = NULL;
					enc_net_d1[i].encode_fd = 0;
					enc_net_d1[i].mode = standard;
					enc_net_d1[i].size = size_mode;
					enc_net_d1[i].is_net_enc = 1;
					enc_net_d1[i].bitstream = seting->bit_rate;
					enc_net_d1[i].framerate = seting->framerate;
					enc_net_d1[i].gop = seting->gop_size;
					enc_net_d1[i].VeGroup = i + video_step;
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl_net[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl_net[i].frame_rate = seting->framerate - 4;
					enc_frame_rate_ctrl_net[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl_net[i].frame_rate);
				
					if( Hisi_create_encode(&enc_net_d1[i]) < 0 )				
						goto RELEASE;					
						
				}		
			
			break;			
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return ERROR;
	}

	printf(" net enc crreate ok!\n");

	return ALLRIGHT;

	RELEASE:
		release_net_encode();

	return ERROR;

	
	 
}

int release_net_encode()
{
	int i;

	#ifdef NET_USE
	stop_net_server();
	#endif

	 switch( cur_size )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )			
			{					
				Hisi_destroy_encode(&enc_net_cif[i]);					
			}			
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:			
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{					
				Hisi_destroy_encode(&enc_net_d1[i]);					
			}	
			
			break;		
		default:
			DPRINTK("size_mode =%d error\n", cur_size);
			return  1 ;
	}
	return 1;
}



int change_device_set_2(video_profile * seting,int standard,int size_mode)
{
//	int ret;
//	int key;
	int i;
	unsigned int bit_rate;
//	int favc_quant;
	
	if( cur_size != size_mode )
	{
		printf(" size is not right!\n");
		return -1;
	}	

	cur_standard = standard;
	cur_size = size_mode;

	if( seting != NULL )
	{
	      #ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;	  
		#else
	    video_setting.qmax = 31;	
		#endif
	    video_setting.qmin = 1;
	    video_setting.quant = INIT_QUANT;
	    video_setting.bit_rate = seting->bit_rate;
	    video_setting.width = seting->width;
	    video_setting.height = seting->height;
	    video_setting.framerate = seting->framerate;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;
	    video_setting.decode_width = seting->width;
	    video_setting.decode_height = seting->height;
	}
	else
	{
	     #ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;	  
	   #else
	    video_setting.qmax = 31;	
	    #endif
	    video_setting.qmin = 1;
	    video_setting.quant = 0;
	    video_setting.bit_rate = 512;	  
	    video_setting.framerate = 30;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;

	   if(  standard == TD_DRV_VIDEO_STARDARD_PAL)
	   {	
		    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_PAL;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_PAL;
	   }else
	   {
	   	    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_NTSC;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_NTSC;
	   }
	}

	 switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{				
				enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl[i].frame_rate =(int)( (video_profile *)(seting + i)->framerate);
				enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);

				enc_frame_rate_ctrl[i].gop_size =(int)( (video_profile *)(seting + i)->gop_size);
				

				if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
					enc_frame_rate_ctrl[i].frame_interval_time = 20000;

				bit_rate =(unsigned int )( (video_profile *)(seting + i)->bit_rate);

				

				#ifdef USE_H264_ENC_DEC
				
					
				printf("cif bitrate = %d framerate=%d gop=%d\n",(int)((video_profile *)(seting + i)->bit_rate),
				enc_frame_rate_ctrl[i].frame_rate,enc_frame_rate_ctrl[i].gop_size);
			
				enc_cif[i].bitstream = bit_rate*1000;
				
				if( enc_frame_rate_ctrl[i].frame_rate > 30 )
					enc_frame_rate_ctrl[i].frame_rate = 25;				
				
					#ifdef USE_H264_RATE_CTRL
				
					memset(&enc_cif[i].ratec,0,sizeof(H264RateControl));	
					H264RateControlInit(&enc_cif[i].ratec,(float)bit_rate*1000,
					RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, (float)enc_frame_rate_ctrl[i].frame_rate,
					MAX_QUANT,1,INIT_QUANT);  
					enc_cif[i].favc_quant = INIT_QUANT;
					#else
					enc_cif[i].enc_quality = get_enc_quality(bit_rate,enc_frame_rate_ctrl[i].frame_rate);
					#endif
			
				#else
				printf("cif bitrate = %d framerate=%d rtn_quant=%d\n",(video_profile *)(seting + i)->bit_rate,enc_frame_rate_ctrl[i].frame_rate,enc_cif[i].ratec.rtn_quant);
			
				if( enc_frame_rate_ctrl[i].frame_rate > 30 )
					enc_frame_rate_ctrl[i].frame_rate = 25;
				RateControlInit(&enc_cif[i].ratec,bit_rate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
	                    ARG_RC_BUFFER, get_framerate(enc_frame_rate_ctrl[i].frame_rate),31,1,4);

				#endif
			}	
				
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:

			if( rec_type == TYPE_QUAD )
			{
				for( i = 0; i < 1; i++ )
				{				
					enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
					enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);

					if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
						enc_frame_rate_ctrl[i].frame_interval_time = 20000;

					bit_rate =(unsigned int)( (video_profile *)(seting + i)->bit_rate);

					#ifdef USE_H264_ENC_DEC
					printf("d1 bitrate = %d framerate=%d gop=%d\n",(int)((video_profile *)(seting + i)->bit_rate),
					enc_frame_rate_ctrl[i].frame_rate,enc_frame_rate_ctrl[i].gop_size);
					//enc_d1[i].ratec.bit_rate = bit_rate;
					if( enc_frame_rate_ctrl[i].frame_rate > 30 )
						enc_frame_rate_ctrl[i].frame_rate = 25;	
					
						#ifdef USE_H264_RATE_CTRL
			
						memset(&enc_d1[i].ratec,0,sizeof(H264RateControl));	
						H264RateControlInit(&enc_d1[i].ratec,(float)bit_rate*1000,
						RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, (float)enc_frame_rate_ctrl[i].frame_rate,
						MAX_QUANT,1,INIT_QUANT);  
						enc_d1[i].favc_quant = INIT_QUANT;
						#else
						enc_d1[i].enc_quality = get_enc_quality(bit_rate,enc_frame_rate_ctrl[i].frame_rate);
						#endif
						
					#else
					printf("qudp bitrate = %d framerate=%d rtn_quant=%d\n",(video_profile *)(seting + i)->bit_rate,enc_frame_rate_ctrl[i].frame_rate,enc_cif[i].ratec.rtn_quant);
				
					if( enc_frame_rate_ctrl[i].frame_rate > 30 )
						enc_frame_rate_ctrl[i].frame_rate = 25;
					RateControlInit(&enc_d1[i].ratec,bit_rate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
		                    ARG_RC_BUFFER, get_framerate(enc_frame_rate_ctrl[i].frame_rate),31,1,4);
					#endif
				}	
			}else
			{
				for( i = 0; i < get_dvr_max_chan(0); i++ )	
				{					
					enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
					enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);
					enc_frame_rate_ctrl[i].gop_size = (int)((video_profile *)(seting + i)->gop_size);

					if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
						enc_frame_rate_ctrl[i].frame_interval_time = 20000;
					
					bit_rate = (unsigned int)((video_profile *)(seting + i)->bit_rate);
					printf("d1 bitrate = %d framerate=%d gop=%d\n",(int)((video_profile *)(seting + i)->bit_rate),
							enc_frame_rate_ctrl[i].frame_rate,enc_frame_rate_ctrl[i].gop_size);
					#ifdef USE_H264_ENC_DEC

					enc_d1[i].bitstream = bit_rate*1000;
					enc_d1[i].framerate = enc_frame_rate_ctrl[i].frame_rate;
					
					if( enc_frame_rate_ctrl[i].frame_rate > 30 )
						enc_frame_rate_ctrl[i].frame_rate = 25;	
					
						#ifdef USE_H264_RATE_CTRL
						printf("d1 bitrate = %d framerate=%d \n",(video_profile *)(seting + i)->bit_rate,enc_frame_rate_ctrl[i].frame_rate);
						printf("d1d bitrate = %d framerate=%d \n",enc_d1[i].bitstream,enc_d1[i].framerate);
						
						memset(&enc_d1[i].ratec,0,sizeof(H264RateControl));	
						H264RateControlInit(&enc_d1[i].ratec,(float)bit_rate*1000,
						RC_DELAY_FACTOR,RC_AVERAGING_PERIOD, RC_BUFFER_SIZE_BITRATE, (float)enc_frame_rate_ctrl[i].frame_rate,
						MAX_QUANT,1,INIT_QUANT);  
						enc_d1[i].favc_quant = INIT_QUANT;
						#else
						enc_d1[i].enc_quality = get_enc_quality(bit_rate,enc_frame_rate_ctrl[i].frame_rate);
						#endif
						
					#else
					printf("qudp bitrate = %d framerate=%d rtn_quant=%d\n",(video_profile *)(seting + i)->bit_rate,enc_frame_rate_ctrl[i].frame_rate,enc_d1[i].ratec.rtn_quant);
				
					
					if( enc_frame_rate_ctrl[i].frame_rate > 30 )
						enc_frame_rate_ctrl[i].frame_rate = 25;
					
					RateControlInit(&enc_d1[i].ratec,bit_rate*1000, DELAY_FACTOR, RC_AVERAGE_PERIOD,
		                    ARG_RC_BUFFER, get_framerate(enc_frame_rate_ctrl[i].frame_rate),31,1,4);
					#endif
				
						
				}		
			}
		
			
			break;
		case TD_DRV_VIDEO_SIZE_HD1:
					
			break;
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return 1;
	}


	printf("change ok!\n");
	return 1;
	
/*	RELEASE:
		printf("change faild!\n");
	close_all_device();
	return -1; */

}



int single_chan_set_encode(int chan ,int bitstream,int framerate,int gop)
{
	ENCODE_ITEM * enc_item = NULL;


	//DPRINTK("%d,%d,%d,%d\n",chan,bitstream,framerate,gop);

	if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
		enc_item = &enc_cif[chan];
	else
		enc_item = &enc_d1[chan];	

	enc_item->ichannel = chan;
	enc_item->bitstream = bitstream;
	enc_item->framerate = framerate;
	enc_item->gop = gop;

	if( TD_DRV_VIDEO_SIZE_D1== get_cur_size() )
	{
		if( enc_item->framerate > 6 )
			enc_item->framerate = 6;
	}
					
	if( Hisi_set_encode(enc_item) < 0 )
	{
			DPRINTK("Hisi_set_encode [%d] err\n",chan);	
			return -1;
	}

	return 1;				
}

static int net_play_rec_rate_adujst_set = 0;

int net_play_rec_rate_adujst()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	int control_rate = 12;
	int bit_rate;
		

	if( net_play_rec_rate_adujst_set == 0 )
	{

		for( i = 0; i < get_dvr_max_chan(0); i++ )	
		{				
			if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
				enc_item = &enc_cif[i];
			else
				enc_item = &enc_d1[i];


			if( enc_frame_rate_ctrl[i].frame_rate > control_rate )
			{
				bit_rate = div_safe2(enc_item->bitstream,enc_item->framerate)*control_rate;
				
				enc_item->framerate = control_rate;
				enc_item->bitstream = bit_rate;		
				
				if( Hisi_set_encode(enc_item) < 0 )
					DPRINTK("Hisi_set_encode [%d] err\n",i);			
			}
		}	

		if( is_16ch_host_machine() == 1 )
		{
			for( i = 8; i < get_dvr_max_chan(1); i++ )	
			{				
				if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
					enc_item = &enc_cif[i];
				else
					enc_item = &enc_d1[i];


				if( enc_frame_rate_ctrl[i].frame_rate > control_rate)
				{
					bit_rate = div_safe2(enc_item->bitstream,enc_item->framerate)*control_rate;
				
					enc_item->framerate = control_rate;
					enc_item->bitstream = bit_rate;
					
					
				}			
			
				
			}	
		}
				
		
		net_play_rec_rate_adujst_set = 1;
	}
	return 1;
}

int net_play_rec_rate_restore()
{
	ENCODE_ITEM * enc_item = NULL;
	int i = 0;
	
	if( net_play_rec_rate_adujst_set == 1 )
	{
		for( i = 0; i < get_dvr_max_chan(0); i++ )	
		{				
			if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
				enc_item = &enc_cif[i];
			else
				enc_item = &enc_d1[i];

			enc_item->framerate = enc_frame_rate_ctrl[i].frame_rate;
			enc_item->bitstream = enc_frame_rate_ctrl[i].bitstream;
			if( Hisi_set_encode(enc_item) < 0 )
				DPRINTK("Hisi_set_encode [%d] err\n",i);			
			
		}	

		if( is_16ch_host_machine() == 1 )
		{
			for( i = 8; i < get_dvr_max_chan(1); i++ )	
			{				
				if( TD_DRV_VIDEO_SIZE_CIF == get_cur_size() )
					enc_item = &enc_cif[i];
				else
					enc_item = &enc_d1[i];				
				
				enc_item->framerate = enc_frame_rate_ctrl[i].frame_rate;
				enc_item->bitstream = enc_frame_rate_ctrl[i].bitstream;
				//host μ?(8-15) í¨μàê?slave μ?(0-7) í¨μà?￡
			
				
			}	
		}
				
		net_play_rec_rate_adujst_set = 0;
	}
	return 1;
}


int change_device_set_hisi(video_profile * seting,int standard,int size_mode)
{
//	int ret;
//	int key;
	int i;
	unsigned int bit_rate;
//	int favc_quant;
	
	if( cur_size != size_mode )
	{
		DPRINTK(" size is not right! cur_size=%d size_mode=%d \n",cur_size,size_mode);
		return -1;
	}

	cur_standard = standard;
	cur_size = size_mode;

	if( seting != NULL )
	{
	      #ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;	  
		#else
	    video_setting.qmax = 31;	
		#endif
	    video_setting.qmin = 1;
	    video_setting.quant = INIT_QUANT;
	    video_setting.bit_rate = seting->bit_rate;
	    video_setting.width = seting->width;
	    video_setting.height = seting->height;
	    video_setting.framerate = seting->framerate;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;
	    video_setting.decode_width = seting->width;
	    video_setting.decode_height = seting->height;
	}
	else
	{
	     #ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;	  
	   #else
	    video_setting.qmax = 31;	
	    #endif
	    video_setting.qmin = 1;
	    video_setting.quant = 0;
	    video_setting.bit_rate = 512;	  
	    video_setting.framerate = 30;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;

	   if(  standard == TD_DRV_VIDEO_STARDARD_PAL)
	   {	
		    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_PAL;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_PAL;
	   }else
	   {
	   	    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_NTSC;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_NTSC;
	   }
	}

	 switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:			
			 i = 0;
			{				
				enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl[i].frame_rate =(int)( (video_profile *)(seting + i)->framerate);
				enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);

				enc_frame_rate_ctrl[i].gop_size =(int)( (video_profile *)(seting + i)->gop_size);						

				if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
					enc_frame_rate_ctrl[i].frame_interval_time = 20000;

				bit_rate =(unsigned int )( (video_profile *)(seting + i)->bit_rate);	

				enc_frame_rate_ctrl[i].bitstream = bit_rate;

					
				printf("cif bitrate = %d framerate=%d gop=%d\n",(int)((video_profile *)(seting + i)->bit_rate),
				enc_frame_rate_ctrl[i].frame_rate,enc_frame_rate_ctrl[i].gop_size);
			
			
				if( enc_frame_rate_ctrl[i].frame_rate > 30 )
					enc_frame_rate_ctrl[i].frame_rate = 30;

				enc_d1[1].bitstream = bit_rate;
				enc_d1[1].framerate = enc_frame_rate_ctrl[i].frame_rate;
				enc_d1[1].gop = enc_frame_rate_ctrl[i].gop_size;
				
				if( Hisi_set_encode(&enc_cif[i]) < 0 )
					DPRINTK("Hisi_set_encode [%d] err\n",i);			
			
				
			}	

			
				
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:					
				i = 0;
				{					
					enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
					enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
					enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
					enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);
					enc_frame_rate_ctrl[i].gop_size = (int)((video_profile *)(seting + i)->gop_size);

					if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
						enc_frame_rate_ctrl[i].frame_interval_time = 20000;
					
					bit_rate = (unsigned int)((video_profile *)(seting + i)->bit_rate);
					printf("d1 bitrate = %d framerate=%d gop=%d\n",(int)((video_profile *)(seting + i)->bit_rate),
							enc_frame_rate_ctrl[i].frame_rate,enc_frame_rate_ctrl[i].gop_size);					

					enc_d1[0].bitstream = bit_rate;
					enc_d1[0].framerate = enc_frame_rate_ctrl[i].frame_rate;
					enc_d1[0].gop = enc_frame_rate_ctrl[i].gop_size;
					
					if( enc_frame_rate_ctrl[i].frame_rate > 30 )
						enc_frame_rate_ctrl[i].frame_rate = 30;						
			
					if( Hisi_set_encode(&enc_d1[i]) < 0 )
						DPRINTK("Hisi_set_encode [%d] err\n",i);		
						
					}	
			
			break;
		case TD_DRV_VIDEO_SIZE_HD1:
					
			break;
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return 1;
	}


	net_play_rec_rate_adujst_set = 0;

	printf("change ok!\n");
	return 1;
	
/*	RELEASE:
		printf("change faild!\n");
	close_all_device();
	return -1; */

}


int change_device_set(video_profile * seting,int standard,int size_mode)
{
	int ret;
//	int key;
	int i;
	static int only_once = 1;

//	if( only_once == 0 )
		return change_device_set_hisi(seting,standard,size_mode);

	only_once = 0;

/*	if( is_have_client() )
	{			
		kick_client();				
	}
	*/

	ret = close_dec_enc();
	if( ret < 0 )
		return ret;

	cur_standard = standard;
	cur_size = size_mode;

	if( seting != NULL )
	{
	    video_setting.qmax = 31;
	    video_setting.qmin = 1;
	    video_setting.quant = 0;
	    video_setting.bit_rate = seting->bit_rate;
	    video_setting.width = seting->width;
	    video_setting.height = seting->height;
	    video_setting.framerate = seting->framerate;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;
	    video_setting.decode_width = seting->width;
	    video_setting.decode_height = seting->height;
	}
	else
	{
	    video_setting.qmax = 31;
	    video_setting.qmin = 1;
	    video_setting.quant = 0;
	    video_setting.bit_rate = 512;	  
	    video_setting.framerate = 25;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;

	   if(  standard == TD_DRV_VIDEO_STARDARD_PAL)
	   {	
		    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_PAL;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_PAL;
	   }else
	   {
	   	    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_NTSC;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_NTSC;
	   }
	}

	 switch( size_mode )
	{	
		case TD_DRV_VIDEO_SIZE_CIF:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{
				enc_cif[i].ichannel = i;
				enc_cif[i].mmp_addr = NULL;
				enc_cif[i].encode_fd = 0;
				enc_cif[i].mode = standard;
				enc_cif[i].size = size_mode;	
				enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
				enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);

				if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
					enc_frame_rate_ctrl[i].frame_interval_time = 20000;
			
				if( init_mpeg4_dev(&enc_cif[i],(video_profile *)(seting + i)) < 0 )
					goto RELEASE;			
			}
			
			dec_cif.decode_fd = 0;
			dec_cif.mode = standard;
			dec_cif.size = size_mode;
			if( xvid_decoder_init( &dec_cif) < 0 )
				goto RELEASE;
				
			
			break;
		case TD_DRV_VIDEO_SIZE_D1:					
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{
				enc_d1[i].ichannel = i;
				enc_d1[i].mmp_addr = NULL;
				enc_d1[i].encode_fd = 0;
				enc_d1[i].mode = standard;
				enc_d1[i].size = size_mode;
				enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
				enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);

				if( enc_frame_rate_ctrl[i].frame_rate >= 25 )
					enc_frame_rate_ctrl[i].frame_interval_time = 20000;
			
				if( init_mpeg4_dev(&enc_d1[i],(video_profile *)(seting + i)) < 0 )				
					goto RELEASE;
					
			}			
			
			dec_d1.decode_fd = 0;
			dec_d1.mode = standard;
			dec_d1.size = size_mode;
			if( xvid_decoder_init( &dec_d1) < 0 )
			{
				printf("wront decode\n");
				goto RELEASE;
			}
			
			break;
		case TD_DRV_VIDEO_SIZE_HD1:
			for( i = 0; i < get_dvr_max_chan(0); i++ )	
			{
				enc_hd1[i].ichannel = i;
				enc_hd1[i].mmp_addr = NULL;
				enc_hd1[i].encode_fd = 0;
				enc_hd1[i].mode = standard;
				enc_hd1[i].size = size_mode;	
				enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
				enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
				enc_frame_rate_ctrl[i].frame_rate = (int)((video_profile *)(seting + i)->framerate);
				enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);
				if( init_mpeg4_dev(&enc_hd1[i],(video_profile *)(seting + i)) < 0 )
					goto RELEASE;			
			}
			
			dec_hd1.decode_fd = 0;
			dec_hd1.mode = standard;
			dec_hd1.size = size_mode;
			if( xvid_decoder_init( &dec_hd1) < 0 )
				goto RELEASE;
			
			break;
		default:
			DPRINTK("size_mode =%d error\n", size_mode);
			return 1;
	}


	printf("change ok!\n");
	return 1;
	
	RELEASE:
		printf("change faild!\n");
	close_all_device();
	return -1;
}

int get_cur_standard()
{
	return cur_standard;
}

int get_cur_size()
{
//	int size;

	if( rec_type == TYPE_QUAD )
		return QUAD_MODE_NET;

	return cur_size;
}

int set_lcd_show_buf(int num)
{
	
	set_osd_show_lcd_buf(g_show_array.avp[num].data[0],g_show_array.avp[num].data[1],
			g_show_array.avp[num].data[2]);

	set_osd_backgroud_lcd_buf(g_show_array.avp[6].data[0],g_show_array.avp[6].data[1],
			g_show_array.avp[6].data[2]);

	return 1;
}



int clear_osd(int color)
{
	int ret;
	
	ret = clear_lcd_osd(color);

	if( ret < 0 )
		return ret;

	if (ioctl(lcd_fd,FLCD_SET_SPECIAL_FB,&lcd_menu_osd_fb_num) < 0) 
	 {
		 trace("LCD Error: cannot set frame buffer as 1.\n");
		 return -1;
	}

	return ret;
}

KEYBOARD_LED_INFO * get_keyboard_led_info()
{
	return &net_key_board;
}

int op_keyboard_led_info(char * cmd)
{
	NET_COMMAND * pcmd;
	pcmd = (NET_COMMAND*)cmd;
	net_key_board.key_code = (int)pcmd->length;
	net_key_board.cam_no   = (unsigned char)pcmd->page;
	net_key_board.key_style = (int)pcmd->play_fileId;	

	printf("key %x,%d,%d\n",net_key_board.key_code,net_key_board.cam_no,
		net_key_board.key_style);
	return 1;
}




void encode_buf_op_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);	
	char buf[NET_BUF_MAX_SIZE];
	
	DPRINTK(" pid = %d\n",getpid());

	
	while(1)
	{	
	 
		 #ifdef USE_PREVIEW_ENC_THREAD_WRITE
			 //?D??encodelist |ì?¨oy?Y1?2?1??¨¤?ê??¨¤¨￠?D??¨¨?preview buf;

			if( g_enable_rec_flag ==1 )
			{				
				if( g_pstcommand->GST_FTC_get_rec_cur_status() == 0 )
				{
					if( get_save_buf_info(buf) == NULL )
						usleep(10000);
				}else
					usleep(10000);
				
			}else
			{
				usleep(10000);
			}

				 
			#endif			

	}

	return ;
}


int IsFileExist(char * fileName)
{
	FILE * fp;

	fp = fopen(fileName,"rb");

	if( fp == NULL )
		return -1;

	fclose(fp);

	return 1;
}

int net_cam_create_I_frame()
{
	GM_Net_intra_frame(&gm_enc_st[0]);	
}

int net_dvr_set_enc(int chan,int flag)
{
	int i;
	
	for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
	{			
		if( get_bit(chan,i) == 1)
		{		
			if( flag == 1 )
			{					
				//Net_cam_start_enc(i);
				Hisi_create_IDR_2(i);				

				{
					NET_MODULE_ST * pst2 = NULL;	
					pst2 = client_pst[0];
					pst2->net_lost_frame[i] = 1;
				}
			}else
			{
				DPRINTK("stop chan=%d\n",i);
				//Net_cam_stop_enc(i);
			}			
			
		}		
	}
}


int is_wifi_connect_send_data()
{
	if(  strcmp(net_dev_name,"ra0") == 0)
	{
		return 1;
	}

	return 0;
}

void net_send_encode_info(NVR_COMMAND_ST * pnvrst)
{
	memset(pnvrst,0x00,sizeof(NVR_COMMAND_ST));

	pnvrst->cmd = NET_CAM_GET_ENGINE_INFO;
	pnvrst->value[0] = gm_enc_st[0].width;
	pnvrst->value[1] =  gm_enc_st[0].height;
	pnvrst->value[2] = gm_enc_st[1].width;
	pnvrst->value[3] =  gm_enc_st[1].height;
	pnvrst->value[4] =  gm_enc_st[2].width;
	pnvrst->value[5] =  gm_enc_st[2].height;
	pnvrst->value[6] = 8*1000; // 8K
	pnvrst->value[7] = 16; //16bit
	pnvrst->value[8] = is_wifi_connect_send_data(); //is use wifi connect  1:wifi  0:network cable
}



int net_cam_recv_data(void * point,char * buf,int length,int cmd)
{
	NET_MODULE_ST * pst = (NET_MODULE_ST *)point;

	NET_COMMAND g_cmd;
	char pc_cmd[110*1024];
	int i;
	static FILE * fp = NULL;
	static FILE * fp_read = NULL;
	int ret;

	ret = 1;

	if( cmd == NET_DVR_NET_COMMAND )
	{

		g_cmd = *(NET_COMMAND*)buf;

		switch(g_cmd.command)
		{
			case NET_CAM_GET_ENGINE_INFO:
				{
					NVR_COMMAND_ST cmd_st;				

					net_send_encode_info(&cmd_st);

					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&cmd_st,sizeof(cmd_st));
					Net_dvr_send_cam_data(pc_cmd,1024,pst);
					
				}
				break;
			case NET_CAM_BIND_ENC:
					DPRINTK("fd=%d  NET_CAM_BIND_ENC chn[%d]\n",pst->client,g_cmd.length);				

					pst->bind_enc_num[0] = 0;
					pst->bind_enc_num[1] = 1;
					pst->bind_enc_num[2] = 2;
					pst->bind_enc_num[3] = 3;
					pst->bind_enc_num[4] = 4;
					pst->bind_enc_num[5] = 5;				

					pst->video_mode = get_cur_standard();				
				break;
			case NET_CAM_SEND_DATA:
					DPRINTK("fd=%d  NET_CAM_SEND_DATA  cam=%x\n",pst->client,g_cmd.length);

					
					pst->bind_enc_num[0] = 0;
					pst->bind_enc_num[1] = 1;
					pst->bind_enc_num[2] = 2;
					pst->bind_enc_num[3] = 3;
					pst->bind_enc_num[4] = 4;
					pst->bind_enc_num[5] = 5;	
					pst->video_mode = get_cur_standard();
					
					pst->send_enc_data = 1;
					pst->send_cam = g_cmd.length;
					pst->send_data_flag = 1;

					for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
					{
						// 2011-12-19
						//|ì?à?¨??¨′?a1?sub enc ¨°y??¨o?à?ê??¨￠ 
						//3?????2??á?¨°???§|ì?DT????ê
						//if( get_bit(pst->send_cam | 0x02,i) == 0 )
						if( get_bit(pst->send_cam | 0x06,i) == 0 )
						{						
							DPRINTK("stop chan = %d\n",i);								
							Net_cam_stop_enc(i);
						}
					}							
	
					
					for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
					{
						//?a¨￠?motion |ì¨2¨°???d1?1??¨°y??¨°???§¨°a?1????ê
						if( get_bit(pst->send_cam | 0x02 ,i) == 0 )
							continue;						

						// 2011-12-20
						//|ì?à?¨??¨′?a1?sub enc ¨°y??¨o?à?ê??¨￠ 
						//3?????2??á?¨°???§|ì?DT????ê
						DPRINTK("start chan = %d\n",i);					
						
						if( i == 2)
						{
							if( g_sub_enc_control == 0 )
							{								
								Net_cam_start_enc(i);
								g_sub_enc_control++;
							
							}else
							{
								DPRINTK("sub enc is already open\n");
								g_sub_enc_control++;
								if( g_sub_enc_control > 30 )
									g_sub_enc_control = 1;
							}
							
						}else
						{							
							Net_cam_start_enc(i);							
						}

						if(  is_wifi_connect_send_data() == 1)
						{
							DPRINTK("Use wifi not create IDR\n");
						}else
							Hisi_create_IDR_2(i);						
						
					}					

					enc_send_data = 1;					
					pst->is_onvif_connect = 0;
					

					break;
					
			case NET_CAM_STOP_SEND:
					DPRINTK("fd=%d  NET_CAM_STOP_SEND\n",pst->client);
					
					//for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
					//	Net_cam_stop_enc(i);	
					
					pst->send_enc_data = 0;
					pst->send_cam = 0;
					
					break;
			case NET_CAM_ENC_SET:
				DPRINTK("fd=%d  NET_CAM_ENC_SET\n",pst->client);		

				if( (strcmp(net_dev_name,"ra0") == 0) &&  (g_cmd.page % 100 >= 22))
				{
						DPRINTK("now use ra0 ,can't set framerate >= 22\n");
				}else
				{
					Hisi_net_cam_enc_set(g_cmd.length,g_cmd.play_fileId,g_cmd.page % 100,g_cmd.page/100);
					{
						int chan = 0;							
						{						
							chan = g_cmd.length;					
							enc_d1[chan].bitstream = g_cmd.play_fileId;
							enc_d1[chan].framerate = g_cmd.page % 100;
							enc_d1[chan].gop = g_cmd.page/100;
						}						
					}	
				}

				break;
			case EXECUTE_SHELL_FILE:

				memset(pc_cmd,0x00,1024);

				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  EXECUTE_SHELL_FILE: lenght=%d error\n",pst->client,g_cmd.length);
				}else
				{
					memcpy(pc_cmd,buf + sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  EXECUTE_SHELL_FILE: %s %d\n",pst->client,pc_cmd,sizeof(NET_COMMAND));
					{
						if( strcmp(pc_cmd,"fast_reboot") == 0 )
						{
							ipcam_fast_reboot();						
						}
						
						if( strcmp(pc_cmd,"reset") == 0 )
						{
							Stop_GM_System();
							Net_cam_mtd_store();
							command_drv("reboot");
							sleep(10);
						}

						if( strcmp(pc_cmd,"reboot") == 0 )
						{
							Stop_GM_System();
							Net_cam_mtd_store();
							command_drv("reboot");
							sleep(10);
						}
						
						int rel;						
						rel = SendM2(pc_cmd);
						if( rel < 0 )
							DPRINTK("fd=%d  EXECUTE_SHELL_FILE: %s error\n",pst->client,pc_cmd);

						g_cmd.page = rel;

						NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);			
					}
				}				
				break;	
			case NET_CAM_FILE_OPEN:
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_FILE_OPEN: lenght=%d error\n",pst->client,g_cmd.length);
					ret = -1;
				}else
				{
					memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s \n",pst->client,pc_cmd);

					if( fp )
					{
						fclose(fp);
						fp = NULL;
					}

					fp = fopen(pc_cmd,"wb");

					if( fp == NULL )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s error\n",pst->client,pc_cmd);
						ret = -1;
					}else
					{						
						
					}
					
				}		

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);	
				
				break;	

			case NET_CAM_FILE_WRITE:
				memset(pc_cmd,0x00,1024);				
				memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
				DPRINTK("fd=%d  NET_CAM_FILE_WRITE: %d \n",pst->client,g_cmd.length);			

				if( fp != NULL )
				{
					ret = fwrite(pc_cmd,1,g_cmd.length,fp);

					if( ret != g_cmd.length )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_WRITE: %d error\n",pst->client,g_cmd.length);
						ret = -1;
					}
				}else
				{
					DPRINTK("fd=%d  NET_CAM_FILE_WRITE: FILE NOT OPEN\n",pst->client);
					ret = -1;
				}		

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);	
						
				break;	
			case NET_CAM_FILE_CLOSE:
				DPRINTK("fd=%d  NET_CAM_FILE_CLOSE \n",pst->client);
				if( fp )
				{
					fclose(fp);
					fp = NULL;
					DPRINTK("fd=%d  NET_CAM_FILE_CLOSE OK\n",pst->client);
				}		

				ret = 0;

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);

				break;
			case NET_CAM_SEND_INTERNATIONAL_DATA:
				pst->net_send_count = g_cmd.length;				
				break;
			case NET_CAM_GET_MAC:
				memset(pc_cmd,0x00,1024);
				memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
				{
					char svr[30];
					char mac[30];

					memset(svr,0x0,30);
					memset(mac,0x0,30);

					get_svr_mac_id(svr,mac);

					if( mac[0] == 0 )
					{
						strcpy(pc_cmd + sizeof(g_cmd),"NO MAC");
					}else
					{
						strcpy(pc_cmd + sizeof(g_cmd),mac);
					}

					{
						int disk_num = 0;
						disk_num = g_pstcommand->GST_DISK_GetDiskNum();

						pc_cmd[500] = (char)disk_num;
						DPRINTK("disk_num=%d\n",disk_num);
					}

					Net_dvr_send_cam_data(pc_cmd,1024,pst);
				}				
				break;			
			case NET_CAM_GET_FILE_OPEN:
				memset(pc_cmd,0x00,1024);

				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_GET_FILE_OPEN: lenght=%d error\n",pst->client,g_cmd.length);
					ret = -1;
				}else
				{
					memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_GET_FILE_OPEN: %s \n",pst->client,pc_cmd);

					if( fp_read )
					{
						fclose(fp_read);
						fp_read = NULL;
					}

					fp_read = fopen(pc_cmd,"rb");

					if( fp_read == NULL )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s error\n",pst->client,pc_cmd);
						ret = -1;
					}				
				}		
				
				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
							
								
				break;
			case NET_CAM_GET_FILE_READ:

				memset(pc_cmd,0x00,1024);	
				memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));				
				DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: %d \n",pst->client,g_cmd.length);			

				if( fp_read != NULL )
				{
					ret = fread(pc_cmd + sizeof(g_cmd),1,g_cmd.length,fp_read);

					if( ret >= 0 )
					{
						g_cmd.length = ret;
						memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
						Net_dvr_send_cam_data(pc_cmd,g_cmd.length+sizeof(g_cmd),pst);
						
					}else
					{
						DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: %d error\n",pst->client,ret);
					}					
				}else
				{
					DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: FILE NOT OPEN\n",pst->client);
				}					
								
				break;
			case NET_CAM_GET_FILE_CLOSE:
				DPRINTK("fd=%d  NET_CAM_GET_FILE_CLOSE \n",pst->client);
				if( fp_read )
				{
					fclose(fp_read);
					fp_read = NULL;
					DPRINTK("fd=%d  NET_CAM_GET_FILE_CLOSE OK\n",pst->client);
				}

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);

				break;			
			case NET_CAM_WRITE_DVR_SVR:
			
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_WRITE_DVR_SVR: lenght=%d error\n",pst->client,g_cmd.length);
				}else
				{
					memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_WRITE_DVR_SVR: %s \n",pst->client,pc_cmd);
					write_svr_log_info(pc_cmd);
				}	
				break;
			case NET_CAM_WIFI_ENABLE:
				DPRINTK("NET_CAM_WIFI_ENABLE = 0 \n");
				ret = command_drv2("cat /proc/net/dev | grep ra0");
				DPRINTK("cat /proc/net/dev | grep ra0  =%d\n",ret);	
				if( ret == 0)
					g_cmd.length = 1;
				else
					g_cmd.length = 0;
				
				Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
				break;
			case NET_CAM_GET_CAM_TYPE:
				DPRINTK("NET_CAM_GET_CAM_TYPE = %d \n",Net_cam_get_support_max_video());
				g_cmd.length = Net_cam_get_support_max_video();
				Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
				break;
			case IPCAM_CMD_GET_ALL_INFO:
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					{						
						memcpy(pc_cmd + sizeof(g_cmd),pAllInfo,sizeof(IPCAM_ALL_INFO_ST));	
						Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+sizeof(IPCAM_ALL_INFO_ST),pst);
					}	

					if( pst->send_cam == 0 )
						pst->is_onvif_connect = 1;
				}
				break;
			case IPCAM_CMD_SAVE_ALLINFO:
				DPRINTK("fd=%d  IPCAM_CMD_SAVE_ALLINFO\n",pst->client);
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();						
					memcpy(pAllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));						
					save_ipcam_config_st(pAllInfo);					
				}
				break;
			case NET_CAM_SET_WIFI_PARATERMERS:
				
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_PARATERMERS: length=%d error\n",pst->client,g_cmd.length);
				}else
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					NET_WIFI_SET_PARAMETER net_st;
					NET_WIFI_SET_PARAMETER net_st_old;
					int num = 0;
					
					memcpy(&net_st,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_PARATERMERS: %s %s\n",pst->client,net_st.ssid,net_st.passwd);
					if( pAllInfo->net_st.enable_wifi_auto_set_by_nvr == 1 )
					{
						retry_ssid:
						ret = cf_read_value("/mnt/mtd/RT2870STA.dat","SSID",net_st_old.ssid);
						if( ret < 0 )
						{
							if( num == 0 )
							{
								DPRINTK("Can't get SSID, cp -rf /mnt/mtd/RT2870STA.dat_bak /mnt/mtd/RT2870STA.dat\n");
								SendM2("cp -rf /mnt/mtd/RT2870STA.dat_bak /mnt/mtd/RT2870STA.dat");
								num++;
								goto retry_ssid;
							}
							return 1;
						}
						
						ret = cf_read_value("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st_old.passwd);
						if( ret < 0 )
							return 1;

						DPRINTK("old ssid: %s %s  now ssid: %s %s\n",
							net_st_old.ssid,net_st_old.passwd,net_st.ssid,net_st.passwd);

						if( strcmp(net_st.ssid,net_st_old.ssid) != 0 ||  strcmp(net_st.passwd,net_st_old.passwd) != 0)
						{
							g_count_down_reboot_times = 1;
						/*
							cf_write_value("/mnt/mtd/RT2870STA.dat","SSID",net_st.ssid);
							cf_write_value("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st.passwd);
							cf_write_value("/mnt/mtd/RT2870STA.dat","Key1Str",net_st.passwd);
							cf_write_value("/mnt/mtd/RT2870STA.dat","WirelessMode","9");
							cf_write_value("/mnt/mtd/RT2870STA.dat","Channel","1");
							cf_write_value("/mnt/mtd/RT2870STA.dat","AuthMode","WPA2");
							cf_write_value("/mnt/mtd/RT2870STA.dat","EncrypType","AES");
							cf_write_value("/mnt/mtd/RT2870STA.dat","DefaultKeyID","1");	
						*/
							SendM2("cp /mnt/mtd/RT2870STA.dat /tmp");

							DPRINTK("use co_SetValueToEtcFile2 \n");
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","SSID",net_st.ssid);
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","WPAPSK",net_st.passwd);
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","Key1Str",net_st.passwd);
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","WirelessMode","9");
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","Channel","1");
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","AuthMode","WPA2");
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","EncrypType","AES");
							co_SetValueToEtcFile2("/tmp/RT2870STA.dat","DefaultKeyID","1");
							SendM2("cp /tmp/RT2870STA.dat   /mnt/mtd/");
							SendM2("sync");
							//sleep(2);
							if ( g_count_down_reboot_times == 0 )
								SendM2("reboot");
						}
					}
				}	

				
				break;
			case NET_CAM_SET_IPCAM_SUB_STREAM_RESOLUTION:
				DPRINTK("fd=%d  NET_CAM_SET_IPCAM_SUB_STREAM_RESOLUTION\n",pst->client);
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					IPCAM_ALL_INFO_ST tmp;
					int need_reboot = 0;
					tmp = *pAllInfo;
					if( g_cmd.length == 0 )
					{						
						if(tmp.onvif_st.vcode[1].w != 640)
						{
							tmp.onvif_st.vcode[1].w = 640;
							tmp.onvif_st.vcode[1].h = 480;	
							tmp.onvif_st.vcode[1].BitrateLimit = 1500;
							save_ipcam_config_st(&tmp);	
							need_reboot = 1;
							DPRINTK("sub stream set 640x480\n");
						}						
					}else if( g_cmd.length == 1 )
					{
						if(tmp.onvif_st.vcode[1].w != 320)
						{
							tmp.onvif_st.vcode[1].w = 320;
							tmp.onvif_st.vcode[1].h = 240;
							tmp.onvif_st.vcode[1].BitrateLimit = 512;
							save_ipcam_config_st(&tmp);	
							need_reboot = 1;
							DPRINTK("sub stream set 320x240\n");
						}	
					}else
					{
					}

					pst->is_yb_nvr = 1;

					if( need_reboot )
					{
						if ( g_count_down_reboot_times == 0 )
							ipcam_fast_reboot();
					}
				}
				break;
			case NET_CAM_SET_WIFI_SSID_PASS:				
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_SSID_PASS: length=%d error\n",pst->client,g_cmd.length);
				}else
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					NET_WIFI_SET_PARAMETER net_st;
					NET_WIFI_SET_PARAMETER net_st_old;
					
					memcpy(&net_st,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_SSID_PASS: %s %s\n",pst->client,net_st.ssid,net_st.passwd);
					
					ret = cf_read_value("/mnt/mtd/RT2870STA.dat","SSID",net_st_old.ssid);
					if( ret < 0 )
						return 1;
					ret = cf_read_value("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st_old.passwd);
					if( ret < 0 )
						return 1;

					

					DPRINTK("old ssid: %s %s  now ssid: %s %s\n",
						net_st_old.ssid,net_st_old.passwd,net_st.ssid,net_st.passwd);

					if( strcmp(net_st.ssid,net_st_old.ssid) != 0 ||  strcmp(net_st.passwd,net_st_old.passwd) != 0)
					{						

						DPRINTK("use co_SetValueToEtcFile2 \n");
						co_SetValueToEtcFile2("/mnt/mtd/RT2870STA.dat","SSID",net_st.ssid);
						co_SetValueToEtcFile2("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st.passwd);							
						sync();
						//sleep(1);
						//system("reboot");
					}

					g_cmd.page = 1;
					NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
			
					
				}
				break;

			case NET_CAM_TEST_WIFI_SSID_PASS:				
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_TEST_WIFI_SSID_PASS: length=%d error\n",pst->client,g_cmd.length);
				}else
				{					
					NET_WIFI_SET_PARAMETER net_st;					
					
					memcpy(&net_st,buf+ sizeof(g_cmd),g_cmd.length);

					DPRINTK("NET_CAM_TEST_WIFI_SSID_PASS now ssid: %s %s\n",net_st.ssid,net_st.passwd);

					ret = test_wifi(net_st.ssid,net_st.passwd);
					DPRINTK("ret=%d\n",ret);
					g_cmd.page = ret;
					NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
					
				}
				break;
			case NET_CAM_GET_ALL_STREAM_RESOLUTION:
				DPRINTK("NET_CAM_GET_ALL_STREAM_RESOLUTION \n");				
				{					
					IPCAM_STREAM_SET stream_st;	
					int base_frame_rate = 0;
					int j;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_VCODE_VS_ST * pvcode = NULL;
					H264_STREAM_INFO * pstream_info = NULL;

					
					if( pAllInfo->sys_st.ipcam_stardard == TD_DRV_VIDEO_STARDARD_PAL )
						base_frame_rate = 25;
					else
						base_frame_rate = 30;

					memset(&stream_st,0x00,sizeof(stream_st));

					stream_st.stream_num = 2;

					pvcode = &pAllInfo->onvif_st.vcode[0];
					stream_st.stream_info[0].stream_resolution_num = 1;
					stream_st.stream_info[0].stream_select_resolution_index = 0;
					stream_st.stream_info[0].stream_resolution[0].w = pvcode->w;
					stream_st.stream_info[0].stream_resolution[0].h = pvcode->h;				
					stream_st.stream_info[0].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[0].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[0].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[0].steam_enc.base_frame_rate = base_frame_rate;

					pvcode = &pAllInfo->onvif_st.vcode[1];
					stream_st.stream_info[1].stream_resolution_num = 2;
					stream_st.stream_info[1].stream_select_resolution_index = 0;
					stream_st.stream_info[1].stream_resolution[0].w = 640;
					stream_st.stream_info[1].stream_resolution[0].h = 480;
					stream_st.stream_info[1].stream_resolution[1].w = 320;
					stream_st.stream_info[1].stream_resolution[1].h = 240;
					stream_st.stream_info[1].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[1].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[1].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[1].steam_enc.base_frame_rate = base_frame_rate;			

					if( pAllInfo->onvif_st.vcode[1].w == 640 )					
						stream_st.stream_info[1].stream_select_resolution_index = 0;
					else
						stream_st.stream_info[1].stream_select_resolution_index = 1;


					for( i = 0; i < stream_st.stream_num; i++)
					{
						pstream_info = &stream_st.stream_info[i];
						DPRINTK("[%d] stream_resolution_num=%d\n",i,pstream_info->stream_resolution_num);
						DPRINTK("[%d] stream_select_resolution_index=%d\n",i,pstream_info->stream_select_resolution_index);
						for( j = 0; j < pstream_info->stream_resolution_num; j++)
						{
							DPRINTK("     (%d)  %dx%d\n",j,pstream_info->stream_resolution[j].w,pstream_info->stream_resolution[j].h);
						}
						DPRINTK("[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info->steam_enc.bitstream,
							pstream_info->steam_enc.frame_rate,pstream_info->steam_enc.gop,pstream_info->steam_enc.base_frame_rate);
					}				
				
					g_cmd.length = sizeof(stream_st);
					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&stream_st,sizeof(stream_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_SET_ALL_STREAM_RESOLUTION:
				DPRINTK("NET_CAM_SET_ALL_STREAM_RESOLUTION \n");	
				{					
					IPCAM_STREAM_SET stream_st;	
					IPCAM_STREAM_SET stream_set_st;
					int base_frame_rate = 0;
					int j;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_VCODE_VS_ST * pvcode = NULL;
					H264_STREAM_INFO * pstream_info = NULL;
					H264_STREAM_INFO * pstream_info_set = NULL;
					int set_index;
					int need_reboot = 0;
					int change = 0;

					memset(&stream_st,0x00,sizeof(stream_st));

					if( pAllInfo->sys_st.ipcam_stardard == TD_DRV_VIDEO_STARDARD_PAL )
						base_frame_rate = 25;
					else
						base_frame_rate = 30;

					memset(&stream_st,0x00,sizeof(stream_st));

					stream_st.stream_num = 2;

					pvcode = &pAllInfo->onvif_st.vcode[0];
					stream_st.stream_info[0].stream_resolution_num = 1;
					stream_st.stream_info[0].stream_select_resolution_index = 0;
					stream_st.stream_info[0].stream_resolution[0].w = pvcode->w;
					stream_st.stream_info[0].stream_resolution[0].h = pvcode->h;				
					stream_st.stream_info[0].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[0].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[0].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[0].steam_enc.base_frame_rate = base_frame_rate;

					pvcode = &pAllInfo->onvif_st.vcode[1];
					stream_st.stream_info[1].stream_resolution_num = 2;
					stream_st.stream_info[1].stream_select_resolution_index = 0;
					stream_st.stream_info[1].stream_resolution[0].w = 640;
					stream_st.stream_info[1].stream_resolution[0].h = 480;
					stream_st.stream_info[1].stream_resolution[1].w = 320;
					stream_st.stream_info[1].stream_resolution[1].h = 240;
					stream_st.stream_info[1].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[1].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[1].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[1].steam_enc.base_frame_rate = base_frame_rate;			

					if( pAllInfo->onvif_st.vcode[1].w == 640 )					
						stream_st.stream_info[1].stream_select_resolution_index = 0;
					else
						stream_st.stream_info[1].stream_select_resolution_index = 1;

					memcpy(&stream_set_st,buf+ sizeof(g_cmd),sizeof(IPCAM_STREAM_SET));


					for( i = 0; i < stream_set_st.stream_num; i++)
					{
						pstream_info = &stream_set_st.stream_info[i];
						DPRINTK("[%d] stream_resolution_num=%d\n",i,pstream_info->stream_resolution_num);
						DPRINTK("[%d] stream_select_resolution_index=%d\n",i,pstream_info->stream_select_resolution_index);
						for( j = 0; j < pstream_info->stream_resolution_num; j++)
						{
							DPRINTK("     (%d)  %dx%d\n",j,pstream_info->stream_resolution[j].w,pstream_info->stream_resolution[j].h);
						}
						DPRINTK("[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info->steam_enc.bitstream,
							pstream_info->steam_enc.frame_rate,pstream_info->steam_enc.gop,pstream_info->steam_enc.base_frame_rate);
					}	

					for( i = 0; i < stream_st.stream_num; i++)
					{
						pstream_info = &stream_st.stream_info[i];
						pstream_info_set = &stream_set_st.stream_info[i];
						if( pstream_info->stream_select_resolution_index != pstream_info_set->stream_select_resolution_index )
						{
							if( pstream_info_set->stream_select_resolution_index >=0 && 
								pstream_info_set->stream_select_resolution_index < pstream_info->stream_resolution_num )
							{
								pAllInfo->onvif_st.vcode[i].w = pstream_info->stream_resolution[pstream_info_set->stream_select_resolution_index].w;
								pAllInfo->onvif_st.vcode[i].h = pstream_info->stream_resolution[pstream_info_set->stream_select_resolution_index].h;

								DPRINTK("resolution change , so reboot\n");
								need_reboot = 1;
								change = 1;
							}
						}

						if( pstream_info->steam_enc.bitstream !=  pstream_info_set->steam_enc.bitstream 
							|| pstream_info->steam_enc.frame_rate !=  pstream_info_set->steam_enc.frame_rate 
							|| pstream_info->steam_enc.gop !=  pstream_info_set->steam_enc.gop )
						{
							pvcode = &pAllInfo->onvif_st.vcode[i];

							pvcode->GovLength = pstream_info_set->steam_enc.gop / base_frame_rate;
							pvcode->BitrateLimit = pstream_info_set->steam_enc.bitstream;
							pvcode->FrameRateLimit = pstream_info_set->steam_enc.frame_rate;

							DPRINTK("set chan[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info_set->steam_enc.bitstream,
							pstream_info_set->steam_enc.frame_rate,pstream_info_set->steam_enc.gop,pstream_info_set->steam_enc.base_frame_rate);

							Hisi_set_encode_bnc(i,pstream_info_set->steam_enc.gop,
								pstream_info_set->steam_enc.frame_rate,
								pstream_info_set->steam_enc.bitstream);

							change=1;
						}			
						
						
					}	

					if( change == 1)
						save_ipcam_config_st(pAllInfo);	

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
					
					if( need_reboot )					
						ipcam_fast_reboot();
								
				}
				break;
			case NET_CAM_GET_IPCAM_INFO:
				DPRINTK("NET_CAM_GET_IPCAM_INFO \n");
				{					
					NET_IPCAM_INFO ipcam_st;						
					int j;
					char svr[30];
					char mac[30];

					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					memset(&ipcam_st,0x00,sizeof(ipcam_st));

					
					memset(svr,0x0,30);
					memset(mac,0x0,30);
					get_svr_mac_id(svr,mac);

					strcpy(ipcam_st.soft_ver,pAllInfo->app_version);
					sprintf(ipcam_st.hard_ver,"Hisi+%s",pAllInfo->isp_st.strSensorTypeName);
					sprintf(ipcam_st.ipam_id,"%s-%X",mac,ipcam_get_hex_id());
							
				
					g_cmd.length = sizeof(ipcam_st);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&ipcam_st,sizeof(ipcam_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_GET_IMAGE_PARAMETERS:
				DPRINTK("NET_CAM_GET_IMAGE_PARAMETERS \n");
				{					
					NET_IPCAM_IMAGE_PARA image_st;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_ISP_ST * pIsp = NULL;

					memset(&image_st,0x00,sizeof(image_st));

					pIsp = &pAllInfo->isp_st;
					
					image_st.u32LumaVal = pIsp->u32LumaVal;
					image_st.u32ContrVal = pIsp->u32ContrVal;
					image_st.u32HueVal = pIsp->u32HueVal;
					image_st.u32SatuVal = pIsp->u32SatuVal;

					DPRINTK("lum=%d contr=%d hue=%d sat=%d\n",
						image_st.u32LumaVal,image_st.u32ContrVal,
						image_st.u32HueVal,image_st.u32SatuVal);
						
				
					g_cmd.length = sizeof(image_st);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&image_st,sizeof(image_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_SET_IMAGE_PARAMETERS:
				DPRINTK("NET_CAM_SET_IMAGE_PARAMETERS \n");
				{					
					NET_IPCAM_IMAGE_PARA image_st;	
					NET_IPCAM_IMAGE_PARA image_set_st;	
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_ISP_ST * pIsp = NULL;

					memset(&image_st,0x00,sizeof(image_st));

					pIsp = &pAllInfo->isp_st;
					
					image_st.u32LumaVal = pIsp->u32LumaVal;
					image_st.u32ContrVal = pIsp->u32ContrVal;
					image_st.u32HueVal = pIsp->u32HueVal;
					image_st.u32SatuVal = pIsp->u32SatuVal;		

					DPRINTK("old lum=%d contr=%d hue=%d sat=%d\n",
						image_st.u32LumaVal,image_st.u32ContrVal,
						image_st.u32HueVal,image_st.u32SatuVal);

					memcpy(&image_set_st,buf+ sizeof(g_cmd),sizeof(NET_IPCAM_IMAGE_PARA));

					DPRINTK("set lum=%d contr=%d hue=%d sat=%d\n",
						image_set_st.u32LumaVal,image_set_st.u32ContrVal,
						image_set_st.u32HueVal,image_set_st.u32SatuVal);
					
					if(image_set_st.u32LumaVal != image_st.u32LumaVal  ||
					   image_set_st.u32ContrVal != image_st.u32ContrVal  ||
					   image_set_st.u32HueVal != image_st.u32HueVal  ||
					   image_set_st.u32SatuVal != image_st.u32SatuVal )
					{
						pIsp->u32LumaVal = image_set_st.u32LumaVal;
						pIsp->u32ContrVal = image_set_st.u32ContrVal;
						pIsp->u32HueVal = image_set_st.u32HueVal;
						pIsp->u32SatuVal = image_set_st.u32SatuVal;							
						save_ipcam_config_st(pAllInfo);
						
						Hisi_set_vi_csc_attr(image_set_st.u32LumaVal,image_set_st.u32ContrVal
						, image_set_st.u32HueVal,image_set_st.u32SatuVal);	
					}	

							
				
					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);					
				}
				break;
			case NET_CAM_SET_DEFAULT:
				DPRINTK("NET_CAM_SET_DEFAULT \n");
				{					
					NET_IPCAM_RESTORE_DEFAULT default_st;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					int need_reboot = 0;
					

					memset(&default_st,0x00,sizeof(default_st));					

					memcpy(&default_st,buf+ sizeof(g_cmd),sizeof(default_st));

					DPRINTK("default_st=%d\n",default_st.set_default_para);

					switch(default_st.set_default_para)
					{
						case RESTORET_DEFAULT_PARA_NET:
							 {
								IPCAM_NET_ST * pNetSt = &pAllInfo->net_st;
								pNetSt->net_dev = IPCAM_NET_DEVICE_LAN;
								pNetSt->ipv4[0] = 192;
								pNetSt->ipv4[1] = 168;
								pNetSt->ipv4[2] = 1;
								pNetSt->ipv4[3] = 68;

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

								pNetSt->serv_port = 80;

								pNetSt->net_work_mode = 1004;  //IPCAM_IP_MODE  (1004)
								save_ipcam_config_st(pAllInfo);

								need_reboot = 1;
							}
							break;
						case RESTORET_DEFAULT_PARA_IMAGE:
							 {
								IPCAM_ISP_ST * pIsp = &pAllInfo->isp_st;

								pIsp->u32LumaVal = 50;
								pIsp->u32ContrVal = 50;
								pIsp->u32HueVal = 50;
								pIsp->u32SatuVal = 50;								
								save_ipcam_config_st(pAllInfo);

								Hisi_set_vi_csc_attr(pIsp->u32LumaVal,pIsp->u32ContrVal,
									pIsp->u32HueVal ,pIsp->u32SatuVal);
								
							}
							break;
						case RESTORET_DEFAULT_PARA_ENC:
							{
								IPCAM_ALL_INFO_ST * pIpcam_st = pAllInfo;
								for( i = 0; i < MAX_RTSP_NUM; i++)
								{
									if( i == 0 )
									{									
										//pIpcam_st->onvif_st.vcode[i].w = 1280;
										//pIpcam_st->onvif_st.vcode[i].h = 720;			
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 2048;			
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
										
									}else if( i == 1 )
									{	
										//pIpcam_st->onvif_st.vcode[i].w = 640;
										//pIpcam_st->onvif_st.vcode[i].h = 480;										
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 1024;
										if( pIpcam_st->onvif_st.vcode[i].w == 320)
											pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 512;
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
									}else 
									{	
										//pIpcam_st->onvif_st.vcode[i].w = 640;
										//pIpcam_st->onvif_st.vcode[i].h = 480;										
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 512;
										if( pIpcam_st->onvif_st.vcode[i].w == 320)
											pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 256;
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
									}								
								}

								save_ipcam_config_st(pAllInfo);
							}
							break;
						case RESTORET_DEFAULT_PARA_ALL:
							{
								command_drv("rm -rf /mnt/mtd/ipcam_config.data");
								need_reboot = 1;
							}
							break;
						default:
							ret = -1;
							break;
					}						
				
					g_cmd.page= ret;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);	

					if( need_reboot )
					{						
						command_drv("reboot");
					}
				}
				break;
			case NET_CAM_GET_SYS_PARAMETERS:
				DPRINTK("NET_CAM_GET_SYS_PARAMETERS \n");
				{
					NET_IPCAM_SYS_INFO sys_info;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					memset(&sys_info,0x00,sizeof(sys_info));				

					{
						int year,month,day,hour,minute,second;
						time_t time1;
						long long  ipcam_time;
						struct timeval now_time;
						gettimeofday( &now_time, NULL );
						time1= now_time.tv_sec;
					
						g_pstcommand->GST_FTC_time_tToCustomTime(time1,&year,&month,&day,&hour,&minute,&second);

						ipcam_time = (year+1900)*10000 + (month+1)*100 + day;
						ipcam_time = ipcam_time*1000000;
						ipcam_time = ipcam_time + hour*10000 + minute*100 + second;

						sys_info.day_date = (year+1900)*10000 + (month+1)*100 + day;
						sys_info.day_time = hour*10000 + minute*100 + second;
						sys_info.time_region = pAllInfo->sys_st.time_region;
						sys_info.time_zone = pAllInfo->sys_st.time_zone;	
						sys_info.stardard = pAllInfo->sys_st.ipcam_stardard;
					}	

					g_cmd.length = sizeof(sys_info);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&sys_info,sizeof(sys_info));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);
				}
				break;
			case NET_CAM_SET_SYS_PARAMETERS:
				DPRINTK("NET_CAM_SET_SYS_PARAMETERS \n");
				{					
					NET_IPCAM_SYS_INFO sys_info;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					int need_reboot = 0;
					int need_save = 0;
					memset(&sys_info,0x00,sizeof(sys_info));
					memcpy(&sys_info,buf+ sizeof(g_cmd),sizeof(sys_info));

					DPRINTK("day:%d  time:%d time_region=%d time_zone=%d\n",
						sys_info.day_date,sys_info.day_time,sys_info.time_region,sys_info.time_zone);

					if( sys_info.stardard != pAllInfo->sys_st.ipcam_stardard)
					{
						pAllInfo->sys_st.ipcam_stardard = sys_info.stardard;
						need_reboot = 1;
						need_save = 1;
					}
					
					if( pAllInfo->sys_st.time_region !=  sys_info.time_region  ||
						pAllInfo->sys_st.time_zone !=  sys_info.time_zone )
					{	
						pAllInfo->sys_st.time_region = sys_info.time_region;
						pAllInfo->sys_st.time_zone = sys_info.time_zone;		
						need_save = 1;
					}					

					if(sys_info.day_date != 0 )
					{						
						time_t tmp_time = 0;
						int _Year;
						int _Month;
						int _Day;
						int _Hour;
						int _Minute;
						int _Second;	
						char tmp_buf[200];

						_Year = sys_info.day_date /10000;
						_Month = sys_info.day_date % 10000 /100;
						_Day = sys_info.day_date % 100;

						_Hour = sys_info.day_time /10000;
						_Minute = sys_info.day_time % 10000 /100;
						_Second = sys_info.day_time % 100;	

						DPRINTK("set date %d-%d-%d %d:%d:%d  zone=%d\n",
							_Year,_Month,_Day,_Hour,_Minute,_Second,pAllInfo->sys_st.time_zone);						
						
						
						tmp_time = FTC_CustomTimeTotime_t2(_Year,_Month,_Day,_Hour,_Minute,_Second);
						tmp_time += pAllInfo->sys_st.time_zone*3600;							
								
						FTC_time_tToCustomTime2(tmp_time,&_Year,&_Month,&_Day,&_Hour,&_Minute,&_Second);	

						DPRINTK("set show date %d-%d-%d %d:%d:%d\n",
							_Year,_Month,_Day,_Hour,_Minute,_Second);						
						
						
						sprintf(tmp_buf,"date %0.2d%0.2d%0.2d%0.2d%0.4d.%0.2d",_Month,_Day,_Hour,_Minute,_Year,_Second);
						
						SendM2(tmp_buf);						
					}

				
							
					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);	
				

					if( need_save == 1)
					{
						save_ipcam_config_st(pAllInfo);
					}					

					if( need_reboot == 1 )
					{
						command_drv("reboot");
					}
					
				}
				break;
				
			default:break;
		}

	}

	if( cmd == NET_DVR_NET_FRAME )
	{
		BUF_FRAMEHEADER * pheadr = NULL;
		BUF_FRAMEHEADER fheadr;

		pheadr = &fheadr;

		memcpy(pheadr,buf,sizeof(BUF_FRAMEHEADER));

		if( pheadr->type == AUDIO_FRAME )
		{				
			Hisi_write_audio_buf(buf + sizeof(BUF_FRAMEHEADER),fheadr.iLength,0);	
		}
	}

	if( cmd == NET_DVR_NET_SELF_DATA )
	{
		if( g_pstcommand->GST_DRA_Net_dvr_recv_self_data != 0 )
		{		
			if( buf[0] == 0x9A )
			{
				if( Net_have_fab_client() > 2 )
				{
					DPRINTK("have %d client ,so not net set ip\n",Net_have_fab_client());
				}else
					g_pstcommand->GST_DRA_Net_dvr_recv_self_data(buf,length);
			}else		
				g_pstcommand->GST_DRA_Net_dvr_recv_self_data(buf,length);
		}else
		{
			DPRINTK("GST_DRA_Net_dvr_recv_self_data empty\n");
		}
	}
		
	
	return 1;
}


void ipcam_fast_reboot()
{
	command_process("/mnt/mtd/fast_run.sh");
}



int net_cam_recv_data_mult_client(void * point,char * buf,int length,int cmd)
{
	NET_MODULE_ST * pst = (NET_MODULE_ST *)point;

	NET_COMMAND g_cmd;
	char pc_cmd[110*1024];
	int i;	
	int ret;
	int rel;
	static FILE * fp = NULL;
	static FILE * fp_read = NULL;

	ret = 1;

	if( cmd == NET_DVR_NET_COMMAND )
	{

		g_cmd = *(NET_COMMAND*)buf;

		switch(g_cmd.command)
		{			
			case NET_CAM_GET_ENGINE_INFO:
				{
					NVR_COMMAND_ST cmd_st;				

					net_send_encode_info(&cmd_st);

					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&cmd_st,sizeof(cmd_st));
					Net_dvr_send_cam_data(pc_cmd,1024,pst);					
				}
				break;
			case NET_CAM_BIND_ENC:
					DPRINTK("fd=%d  NET_CAM_BIND_ENC chn[%d]\n",pst->client,g_cmd.length);				
		
				break;
			case NET_CAM_SEND_DATA:
					DPRINTK("fd=%d  NET_CAM_SEND_DATA  cam=%x\n",pst->client,g_cmd.length);

					
					pst->bind_enc_num[0] = 0;
					pst->bind_enc_num[1] = 1;
					pst->bind_enc_num[2] = 2;
					pst->bind_enc_num[3] = 3;
					pst->bind_enc_num[4] = 4;
					pst->bind_enc_num[5] = 5;	
					pst->video_mode = get_cur_standard();
					
					pst->send_enc_data = 1;
					pst->send_cam = g_cmd.length;		

					GM_Net_intra_frame(&gm_enc_st[0]);		

					pst->is_onvif_connect = 0;

					break;
					
			case NET_CAM_STOP_SEND:				
					break;		
			case EXECUTE_SHELL_FILE:
				memset(pc_cmd,0x00,1024);

				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  EXECUTE_SHELL_FILE: lenght=%d error\n",pst->client,g_cmd.length);
				}else
				{
					memcpy(pc_cmd,buf + sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  EXECUTE_SHELL_FILE: %s %d\n",pst->client,pc_cmd,sizeof(NET_COMMAND));
					{						
						if( strcmp(pc_cmd,"fast_reboot") == 0 )
						{
							ipcam_fast_reboot();						
						}

						
						if( strcmp(pc_cmd,"reset") == 0 )
						{
							Stop_GM_System();
							Net_cam_mtd_store();
							command_drv("reboot");
							sleep(10);
						}

						if( strcmp(pc_cmd,"reboot") == 0 )
						{
							Stop_GM_System();
							Net_cam_mtd_store();
							command_drv("reboot");
							sleep(10);
						}
						
						
						rel = SendM2(pc_cmd);
						if( rel < 0 )
							DPRINTK("fd=%d  EXECUTE_SHELL_FILE: %s error\n",pst->client,pc_cmd);

						g_cmd.page = rel;

						NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);		

					}
				}				
				break;				
			case NET_CAM_GET_MAC:
				memset(pc_cmd,0x00,1024);
				memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
				{
					char svr[30];
					char mac[30];

					memset(svr,0x0,30);
					memset(mac,0x0,30);

					get_svr_mac_id(svr,mac);

					if( mac[0] == 0 )
					{
						strcpy(pc_cmd + sizeof(g_cmd),"NO MAC");
					}else
					{
						strcpy(pc_cmd + sizeof(g_cmd),mac);
					}

					Net_dvr_send_cam_data(pc_cmd,1024,pst);
				}				
				break;		
			case NET_CAM_GET_CAM_TYPE:
				DPRINTK("NET_CAM_GET_CAM_TYPE = %d \n",Net_cam_get_support_max_video());
				g_cmd.length = Net_cam_get_support_max_video();
				Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
				break;			
			case IPCAM_CMD_SET_ENCODE:
				DPRINTK("fd=%d  NET_CAM_ENC_SET\n",pst->client);
				Hisi_net_cam_enc_set(g_cmd.length,g_cmd.play_fileId,g_cmd.page % 100,g_cmd.page/100);
				{
					int chan = 0;							
					{
						chan = g_cmd.length;					
						enc_d1[chan].bitstream = g_cmd.play_fileId;
						enc_d1[chan].framerate = g_cmd.page % 100;
						enc_d1[chan].gop = g_cmd.page/100;
					}						
				}
				g_cmd.page = rel;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_CMD_GET_ALL_INFO:
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					{
						//ipcam_get_cur_color_temp(&pAllInfo->isp_st.u32ColorTemp);
						
						pAllInfo->isp_st.u32IsLightDay = Hisi_get_is_light_day();
						pAllInfo->light_check_mode.u32IsLightDay = Hisi_get_is_light_day();
						
						memcpy(pc_cmd + sizeof(g_cmd),pAllInfo,sizeof(IPCAM_ALL_INFO_ST));						

						Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+sizeof(IPCAM_ALL_INFO_ST),pst);
						if( pst->send_cam == 0 )
							pst->is_onvif_connect = 1;
					}			
				}
				break;
			case IPCAM_ISP_CMD_AEAttr:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_AEAttr\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_CMD_AEAttr);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_vi_csc:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_vi_csc\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_CMD_vi_csc);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_au16StaticWB:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_au16StaticWB\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_CMD_au16StaticWB);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_Load_Isp_default:		
				DPRINTK("fd=%d  IPCAM_ISP_CMD_Load_Isp_default\n",pst->client);
				{
					Hisi_set_isp_init_value(0);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_CMD_SAVE_ALLINFO:
				DPRINTK("fd=%d  IPCAM_CMD_SAVE_ALLINFO\n",pst->client);
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();						
					memcpy(pAllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));						
					save_ipcam_config_st(pAllInfo);					
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_SharpenAttr:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_SharpenAttr\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_CMD_SharpenAttr);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_CODE_SET_PARAMETERS:
				DPRINTK("fd=%d  IPCAM_CODE_SET_PARAMETERS\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_CODE_SET_PARAMETERS);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_NET_SET_PARAMETERS:
				DPRINTK("fd=%d  IPCAM_NET_SET_PARAMETERS\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_NET_SET_PARAMETERS);

					{
						IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();					
						memcpy(pAllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));				
						save_ipcam_config_st(pAllInfo);	
					}
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_SYS_SET_PARAMETERS:
				DPRINTK("fd=%d  IPCAM_SYS_SET_PARAMETERS\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_SYS_SET_PARAMETERS);					
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_Load_Net_Default:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_Load_Net_Default\n",pst->client);
				{
					Hisi_get_net_init_value(0);		
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_Load_Sys_Default:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_Load_Sys_Default\n",pst->client);
				{
					Hisi_get_sys_init_value(0);				
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_CMD_DenoiseAttr:
				DPRINTK("fd=%d  IPCAM_ISP_CMD_DenoiseAttr\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_CMD_DenoiseAttr);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;	
			case IPCAM_ISP_GET_COLORTMEP:
				DPRINTK("fd=%d  IPCAM_ISP_GET_COLORTMEP\n",pst->client);
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					ipcam_get_cur_color_temp(&pAllInfo->isp_st.u32ColorTemp);					
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_SET_COLORMATRIX:
				DPRINTK("fd=%d  IPCAM_ISP_SET_COLORMATRIX\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_SET_COLORMATRIX);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_ISP_SET_GAMMA_MODE:
				DPRINTK("fd=%d  IPCAM_ISP_SET_GAMMA_MODE\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_SET_GAMMA_MODE);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;			

			case IPCAM_ISP_SET_LIGHT_CHEKC_MODE:
				DPRINTK("fd=%d  IPCAM_ISP_SET_LIGHT_CHEKC_MODE\n",pst->client);
				{
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					Hisi_set_isp_value(&AllInfo,IPCAM_ISP_SET_LIGHT_CHEKC_MODE);
				}
				g_cmd.page = 1;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;
			case IPCAM_SET_TIME_ZONE:
				DPRINTK("fd=%d  IPCAM_SET_TIME_ZONE\n",pst->client);
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					IPCAM_ALL_INFO_ST AllInfo;
					memcpy(&AllInfo,buf+sizeof(g_cmd),sizeof(IPCAM_ALL_INFO_ST));
					pAllInfo->sys_st.time_zone = AllInfo.sys_st.time_zone;		
					time_zone_change();
					p2p_enable_set_time();
					login_to_svr();
				}
				break;
			case NET_CAM_FILE_OPEN:				
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_FILE_OPEN: lenght=%d error\n",pst->client,g_cmd.length);
					ret = -1;
				}else
				{
					memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s \n",pst->client,pc_cmd);

					if( fp )
					{
						fclose(fp);
						fp = NULL;
					}

					fp = fopen(pc_cmd,"wb");

					if( fp == NULL )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s error\n",pst->client,pc_cmd);
						ret = -1;
					}else
					{						
						
					}
					
				}					
				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);	
				
				break;	

			case NET_CAM_FILE_WRITE:

				memset(pc_cmd,0x00,1024);				
				memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
				DPRINTK("fd=%d  NET_CAM_FILE_WRITE: %d \n",pst->client,g_cmd.length);			

				if( fp != NULL )
				{
					ret = fwrite(pc_cmd,1,g_cmd.length,fp);

					if( ret != g_cmd.length )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_WRITE: %d error\n",pst->client,g_cmd.length);
						ret = -1;
					}
				}else
				{
					DPRINTK("fd=%d  NET_CAM_FILE_WRITE: FILE NOT OPEN\n",pst->client);
					ret = -1;
				}		

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);	
								
				break;	
			case NET_CAM_FILE_CLOSE:
				DPRINTK("fd=%d  NET_CAM_FILE_CLOSE \n",pst->client);
				if( fp )
				{
					fclose(fp);
					fp = NULL;
					DPRINTK("fd=%d  NET_CAM_FILE_CLOSE OK\n",pst->client);
				}		

				ret = 1;			
				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				
				break;			
			
			case NET_CAM_GET_FILE_OPEN:
				memset(pc_cmd,0x00,1024);

				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_GET_FILE_OPEN: lenght=%d error\n",pst->client,g_cmd.length);
					ret = -1;
				}else
				{
					memcpy(pc_cmd,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_GET_FILE_OPEN: %s \n",pst->client,pc_cmd);

					if( fp_read )
					{
						fclose(fp_read);
						fp_read = NULL;
					}

					fp_read = fopen(pc_cmd,"rb");

					if( fp_read == NULL )
					{
						DPRINTK("fd=%d  NET_CAM_FILE_OPEN: %s error\n",pst->client,pc_cmd);
						ret = -1;
					}				
				}		
				
				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
								
				break;
			case NET_CAM_GET_FILE_READ:

				memset(pc_cmd,0x00,1024);	
				memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));				
				DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: %d \n",pst->client,g_cmd.length);			

				if( fp_read != NULL )
				{
					ret = fread(pc_cmd + sizeof(g_cmd),1,g_cmd.length,fp_read);

					if( ret >= 0 )
					{
						g_cmd.length = ret;
						memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
						Net_dvr_send_cam_data(pc_cmd,g_cmd.length+sizeof(g_cmd),pst);
						
					}else
					{
						DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: %d error\n",pst->client,ret);
					}					
				}else
				{
					DPRINTK("fd=%d  NET_CAM_GET_FILE_READ: FILE NOT OPEN\n",pst->client);
				}					
								
				break;
			case NET_CAM_GET_FILE_CLOSE:
				DPRINTK("fd=%d  NET_CAM_GET_FILE_CLOSE \n",pst->client);
				if( fp_read )
				{
					fclose(fp_read);
					fp_read = NULL;
					DPRINTK("fd=%d  NET_CAM_GET_FILE_CLOSE OK\n",pst->client);
				}

				g_cmd.page = ret;
				NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
				break;		
			case NET_CAM_SET_WIFI_SSID_PASS:				
				memset(pc_cmd,0x00,1024);
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_SSID_PASS: length=%d error\n",pst->client,g_cmd.length);
				}else
				{
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					NET_WIFI_SET_PARAMETER net_st;
					NET_WIFI_SET_PARAMETER net_st_old;
					
					memcpy(&net_st,buf+ sizeof(g_cmd),g_cmd.length);
					DPRINTK("fd=%d  NET_CAM_SET_WIFI_SSID_PASS: %s %s\n",pst->client,net_st.ssid,net_st.passwd);
					
					ret = cf_read_value("/mnt/mtd/RT2870STA.dat","SSID",net_st_old.ssid);
					if( ret < 0 )
						return 1;
					ret = cf_read_value("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st_old.passwd);
					if( ret < 0 )
						return 1;

					

					DPRINTK("old ssid: %s %s  now ssid: %s %s\n",
						net_st_old.ssid,net_st_old.passwd,net_st.ssid,net_st.passwd);

					if( strcmp(net_st.ssid,net_st_old.ssid) != 0 ||  strcmp(net_st.passwd,net_st_old.passwd) != 0)
					{						

						DPRINTK("use co_SetValueToEtcFile2 \n");
						co_SetValueToEtcFile2("/mnt/mtd/RT2870STA.dat","SSID",net_st.ssid);
						co_SetValueToEtcFile2("/mnt/mtd/RT2870STA.dat","WPAPSK",net_st.passwd);							
						sync();
						//sleep(1);
						//system("reboot");
					}

					g_cmd.page = 1;
					NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
			
					
				}
				break;

			case NET_CAM_TEST_WIFI_SSID_PASS:				
				if( g_cmd.length > 500  || g_cmd.length <= 0 )
				{
					DPRINTK("fd=%d  NET_CAM_TEST_WIFI_SSID_PASS: length=%d error\n",pst->client,g_cmd.length);
				}else
				{					
					NET_WIFI_SET_PARAMETER net_st;					
					
					memcpy(&net_st,buf+ sizeof(g_cmd),g_cmd.length);

					DPRINTK("NET_CAM_TEST_WIFI_SSID_PASS now ssid: %s %s\n",net_st.ssid,net_st.passwd);

					ret = test_wifi(net_st.ssid,net_st.passwd);
					DPRINTK("ret=%d\n",ret);
					g_cmd.page = ret;
					NM_net_send_data(pst,(char*)&g_cmd,sizeof(g_cmd),NET_DVR_NET_COMMAND);
					
				}
				break;
			case NET_CAM_WIFI_ENABLE:
				DPRINTK("NET_CAM_WIFI_ENABLE = 0 \n");
				ret = command_drv2("cat /proc/net/dev | grep ra0");
				DPRINTK("cat /proc/net/dev | grep ra0  =%d\n",ret);	
				if( ret == 0)
					g_cmd.length = 1;
				else
					g_cmd.length = 0;
				
				Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
				break;
			case NET_CAM_GET_ALL_STREAM_RESOLUTION:
				DPRINTK("NET_CAM_GET_ALL_STREAM_RESOLUTION \n");				
				{					
					IPCAM_STREAM_SET stream_st;	
					int base_frame_rate = 0;
					int j;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_VCODE_VS_ST * pvcode = NULL;
					H264_STREAM_INFO * pstream_info = NULL;

					
					if( pAllInfo->sys_st.ipcam_stardard == TD_DRV_VIDEO_STARDARD_PAL )
						base_frame_rate = 25;
					else
						base_frame_rate = 30;

					memset(&stream_st,0x00,sizeof(stream_st));

					stream_st.stream_num = 2;

					pvcode = &pAllInfo->onvif_st.vcode[0];
					stream_st.stream_info[0].stream_resolution_num = 1;
					stream_st.stream_info[0].stream_select_resolution_index = 0;
					stream_st.stream_info[0].stream_resolution[0].w = pvcode->w;
					stream_st.stream_info[0].stream_resolution[0].h = pvcode->h;				
					stream_st.stream_info[0].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[0].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[0].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[0].steam_enc.base_frame_rate = base_frame_rate;

					pvcode = &pAllInfo->onvif_st.vcode[1];
					stream_st.stream_info[1].stream_resolution_num = 2;
					stream_st.stream_info[1].stream_select_resolution_index = 0;
					stream_st.stream_info[1].stream_resolution[0].w = 640;
					stream_st.stream_info[1].stream_resolution[0].h = 480;
					stream_st.stream_info[1].stream_resolution[1].w = 320;
					stream_st.stream_info[1].stream_resolution[1].h = 240;
					stream_st.stream_info[1].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[1].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[1].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[1].steam_enc.base_frame_rate = base_frame_rate;			

					if( pAllInfo->onvif_st.vcode[1].w == 640 )					
						stream_st.stream_info[1].stream_select_resolution_index = 0;
					else
						stream_st.stream_info[1].stream_select_resolution_index = 1;


					for( i = 0; i < stream_st.stream_num; i++)
					{
						pstream_info = &stream_st.stream_info[i];
						DPRINTK("[%d] stream_resolution_num=%d\n",i,pstream_info->stream_resolution_num);
						DPRINTK("[%d] stream_select_resolution_index=%d\n",i,pstream_info->stream_select_resolution_index);
						for( j = 0; j < pstream_info->stream_resolution_num; j++)
						{
							DPRINTK("     (%d)  %dx%d\n",j,pstream_info->stream_resolution[j].w,pstream_info->stream_resolution[j].h);
						}
						DPRINTK("[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info->steam_enc.bitstream,
							pstream_info->steam_enc.frame_rate,pstream_info->steam_enc.gop,pstream_info->steam_enc.base_frame_rate);
					}				
				
					g_cmd.length = sizeof(stream_st);
					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&stream_st,sizeof(stream_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_SET_ALL_STREAM_RESOLUTION:
				DPRINTK("NET_CAM_SET_ALL_STREAM_RESOLUTION \n");	
				{					
					IPCAM_STREAM_SET stream_st;	
					IPCAM_STREAM_SET stream_set_st;
					int base_frame_rate = 0;
					int j;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_VCODE_VS_ST * pvcode = NULL;
					H264_STREAM_INFO * pstream_info = NULL;
					H264_STREAM_INFO * pstream_info_set = NULL;
					int set_index;
					int need_reboot = 0;
					int change = 0;

					memset(&stream_st,0x00,sizeof(stream_st));

					if( pAllInfo->sys_st.ipcam_stardard == TD_DRV_VIDEO_STARDARD_PAL )
						base_frame_rate = 25;
					else
						base_frame_rate = 30;

					memset(&stream_st,0x00,sizeof(stream_st));

					stream_st.stream_num = 2;

					pvcode = &pAllInfo->onvif_st.vcode[0];
					stream_st.stream_info[0].stream_resolution_num = 1;
					stream_st.stream_info[0].stream_select_resolution_index = 0;
					stream_st.stream_info[0].stream_resolution[0].w = pvcode->w;
					stream_st.stream_info[0].stream_resolution[0].h = pvcode->h;				
					stream_st.stream_info[0].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[0].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[0].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[0].steam_enc.base_frame_rate = base_frame_rate;

					pvcode = &pAllInfo->onvif_st.vcode[1];
					stream_st.stream_info[1].stream_resolution_num = 2;
					stream_st.stream_info[1].stream_select_resolution_index = 0;
					stream_st.stream_info[1].stream_resolution[0].w = 640;
					stream_st.stream_info[1].stream_resolution[0].h = 480;
					stream_st.stream_info[1].stream_resolution[1].w = 320;
					stream_st.stream_info[1].stream_resolution[1].h = 240;
					stream_st.stream_info[1].steam_enc.gop = base_frame_rate *  pvcode->GovLength;
					stream_st.stream_info[1].steam_enc.bitstream = pvcode->BitrateLimit ;
					stream_st.stream_info[1].steam_enc.frame_rate = pvcode->FrameRateLimit;
					stream_st.stream_info[1].steam_enc.base_frame_rate = base_frame_rate;			

					if( pAllInfo->onvif_st.vcode[1].w == 640 )					
						stream_st.stream_info[1].stream_select_resolution_index = 0;
					else
						stream_st.stream_info[1].stream_select_resolution_index = 1;

					memcpy(&stream_set_st,buf+ sizeof(g_cmd),sizeof(IPCAM_STREAM_SET));


					for( i = 0; i < stream_set_st.stream_num; i++)
					{
						pstream_info = &stream_set_st.stream_info[i];
						DPRINTK("[%d] stream_resolution_num=%d\n",i,pstream_info->stream_resolution_num);
						DPRINTK("[%d] stream_select_resolution_index=%d\n",i,pstream_info->stream_select_resolution_index);
						for( j = 0; j < pstream_info->stream_resolution_num; j++)
						{
							DPRINTK("     (%d)  %dx%d\n",j,pstream_info->stream_resolution[j].w,pstream_info->stream_resolution[j].h);
						}
						DPRINTK("[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info->steam_enc.bitstream,
							pstream_info->steam_enc.frame_rate,pstream_info->steam_enc.gop,pstream_info->steam_enc.base_frame_rate);
					}	

					for( i = 0; i < stream_st.stream_num; i++)
					{
						pstream_info = &stream_st.stream_info[i];
						pstream_info_set = &stream_set_st.stream_info[i];
						if( pstream_info->stream_select_resolution_index != pstream_info_set->stream_select_resolution_index )
						{
							if( pstream_info_set->stream_select_resolution_index >=0 && 
								pstream_info_set->stream_select_resolution_index < pstream_info->stream_resolution_num )
							{
								pAllInfo->onvif_st.vcode[i].w = pstream_info->stream_resolution[pstream_info_set->stream_select_resolution_index].w;
								pAllInfo->onvif_st.vcode[i].h = pstream_info->stream_resolution[pstream_info_set->stream_select_resolution_index].h;

								DPRINTK("resolution change , so reboot\n");
								need_reboot = 1;
								change = 1;
							}
						}

						if( pstream_info->steam_enc.bitstream !=  pstream_info_set->steam_enc.bitstream 
							|| pstream_info->steam_enc.frame_rate !=  pstream_info_set->steam_enc.frame_rate 
							|| pstream_info->steam_enc.gop !=  pstream_info_set->steam_enc.gop )
						{
							pvcode = &pAllInfo->onvif_st.vcode[i];

							pvcode->GovLength = pstream_info_set->steam_enc.gop / base_frame_rate;
							pvcode->BitrateLimit = pstream_info_set->steam_enc.bitstream;
							pvcode->FrameRateLimit = pstream_info_set->steam_enc.frame_rate;

							DPRINTK("set chan[%d] bit=%d frame_rate=%d gop=%d base_frame_rate=%d\n",i,pstream_info_set->steam_enc.bitstream,
							pstream_info_set->steam_enc.frame_rate,pstream_info_set->steam_enc.gop,pstream_info_set->steam_enc.base_frame_rate);

							Hisi_set_encode_bnc(i,pstream_info_set->steam_enc.gop,
								pstream_info_set->steam_enc.frame_rate,
								pstream_info_set->steam_enc.bitstream);

							change=1;
						}			
						
						
					}	

					if( change == 1)
						save_ipcam_config_st(pAllInfo);	

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);
					
					if( need_reboot )					
						ipcam_fast_reboot();
								
				}
				break;
			case NET_CAM_GET_IPCAM_INFO:
				DPRINTK("NET_CAM_GET_IPCAM_INFO \n");
				{					
					NET_IPCAM_INFO ipcam_st;						
					int j;
					char svr[30];
					char mac[30];

					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					memset(&ipcam_st,0x00,sizeof(ipcam_st));

					
					memset(svr,0x0,30);
					memset(mac,0x0,30);
					get_svr_mac_id(svr,mac);

					strcpy(ipcam_st.soft_ver,pAllInfo->app_version);
					sprintf(ipcam_st.hard_ver,"Hisi+%s",pAllInfo->isp_st.strSensorTypeName);
					sprintf(ipcam_st.ipam_id,"%s-%X",mac,ipcam_get_hex_id());
							
				
					g_cmd.length = sizeof(ipcam_st);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&ipcam_st,sizeof(ipcam_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_GET_IMAGE_PARAMETERS:
				DPRINTK("NET_CAM_GET_IMAGE_PARAMETERS \n");
				{					
					NET_IPCAM_IMAGE_PARA image_st;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_ISP_ST * pIsp = NULL;

					memset(&image_st,0x00,sizeof(image_st));

					pIsp = &pAllInfo->isp_st;
					
					image_st.u32LumaVal = pIsp->u32LumaVal;
					image_st.u32ContrVal = pIsp->u32ContrVal;
					image_st.u32HueVal = pIsp->u32HueVal;
					image_st.u32SatuVal = pIsp->u32SatuVal;

					DPRINTK("lum=%d contr=%d hue=%d sat=%d\n",
						image_st.u32LumaVal,image_st.u32ContrVal,
						image_st.u32HueVal,image_st.u32SatuVal);
						
				
					g_cmd.length = sizeof(image_st);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&image_st,sizeof(image_st));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);	
					
				}
				break;
			case NET_CAM_SET_IMAGE_PARAMETERS:
				DPRINTK("NET_CAM_SET_IMAGE_PARAMETERS \n");
				{					
					NET_IPCAM_IMAGE_PARA image_st;	
					NET_IPCAM_IMAGE_PARA image_set_st;	
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_ISP_ST * pIsp = NULL;

					memset(&image_st,0x00,sizeof(image_st));

					pIsp = &pAllInfo->isp_st;
					
					image_st.u32LumaVal = pIsp->u32LumaVal;
					image_st.u32ContrVal = pIsp->u32ContrVal;
					image_st.u32HueVal = pIsp->u32HueVal;
					image_st.u32SatuVal = pIsp->u32SatuVal;		

					DPRINTK("old lum=%d contr=%d hue=%d sat=%d\n",
						image_st.u32LumaVal,image_st.u32ContrVal,
						image_st.u32HueVal,image_st.u32SatuVal);

					memcpy(&image_set_st,buf+ sizeof(g_cmd),sizeof(NET_IPCAM_IMAGE_PARA));

					DPRINTK("set lum=%d contr=%d hue=%d sat=%d\n",
						image_set_st.u32LumaVal,image_set_st.u32ContrVal,
						image_set_st.u32HueVal,image_set_st.u32SatuVal);
					
					if(image_set_st.u32LumaVal != image_st.u32LumaVal  ||
					   image_set_st.u32ContrVal != image_st.u32ContrVal  ||
					   image_set_st.u32HueVal != image_st.u32HueVal  ||
					   image_set_st.u32SatuVal != image_st.u32SatuVal )
					{
						pIsp->u32LumaVal = image_set_st.u32LumaVal;
						pIsp->u32ContrVal = image_set_st.u32ContrVal;
						pIsp->u32HueVal = image_set_st.u32HueVal;
						pIsp->u32SatuVal = image_set_st.u32SatuVal;							
						save_ipcam_config_st(pAllInfo);
						
						Hisi_set_vi_csc_attr(image_set_st.u32LumaVal,image_set_st.u32ContrVal
						, image_set_st.u32HueVal,image_set_st.u32SatuVal);	
					}	

							
				
					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);					
				}
				break;
			case NET_CAM_SET_DEFAULT:
				DPRINTK("NET_CAM_SET_DEFAULT \n");
				{					
					NET_IPCAM_RESTORE_DEFAULT default_st;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					int need_reboot = 0;
					

					memset(&default_st,0x00,sizeof(default_st));					

					memcpy(&default_st,buf+ sizeof(g_cmd),sizeof(default_st));

					DPRINTK("default_st=%d\n",default_st.set_default_para);

					switch(default_st.set_default_para)
					{
						case RESTORET_DEFAULT_PARA_NET:
							 {
								IPCAM_NET_ST * pNetSt = &pAllInfo->net_st;
								pNetSt->net_dev = IPCAM_NET_DEVICE_LAN;
								pNetSt->ipv4[0] = 192;
								pNetSt->ipv4[1] = 168;
								pNetSt->ipv4[2] = 1;
								pNetSt->ipv4[3] = 68;

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

								pNetSt->serv_port = 80;

								pNetSt->net_work_mode = 1004;  //IPCAM_IP_MODE  (1004)
								save_ipcam_config_st(pAllInfo);

								need_reboot = 1;
							}
							break;
						case RESTORET_DEFAULT_PARA_IMAGE:
							 {
								IPCAM_ISP_ST * pIsp = &pAllInfo->isp_st;

								pIsp->u32LumaVal = 50;
								pIsp->u32ContrVal = 50;
								pIsp->u32HueVal = 50;
								pIsp->u32SatuVal = 50;								
								save_ipcam_config_st(pAllInfo);

								Hisi_set_vi_csc_attr(pIsp->u32LumaVal,pIsp->u32ContrVal,
									pIsp->u32HueVal ,pIsp->u32SatuVal);
								
							}
							break;
						case RESTORET_DEFAULT_PARA_ENC:
							{
								IPCAM_ALL_INFO_ST * pIpcam_st = pAllInfo;
								for( i = 0; i < MAX_RTSP_NUM; i++)
								{
									if( i == 0 )
									{									
										//pIpcam_st->onvif_st.vcode[i].w = 1280;
										//pIpcam_st->onvif_st.vcode[i].h = 720;			
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 2048;			
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
										
									}else if( i == 1 )
									{	
										//pIpcam_st->onvif_st.vcode[i].w = 640;
										//pIpcam_st->onvif_st.vcode[i].h = 480;										
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 1024;
										if( pIpcam_st->onvif_st.vcode[i].w == 320)
											pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 512;
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
									}else 
									{	
										//pIpcam_st->onvif_st.vcode[i].w = 640;
										//pIpcam_st->onvif_st.vcode[i].h = 480;										
										pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 512;
										if( pIpcam_st->onvif_st.vcode[i].w == 320)
											pIpcam_st->onvif_st.vcode[i].BitrateLimit  = 256;
										pIpcam_st->onvif_st.vcode[i].Encoding = 2;  //JPEG = 0, MPEG4 = 1, H264 = 2;
										pIpcam_st->onvif_st.vcode[i].EncodingInterval = 1;
										pIpcam_st->onvif_st.vcode[i].FrameRateLimit = 25;
										pIpcam_st->onvif_st.vcode[i].GovLength = 2;
										pIpcam_st->onvif_st.vcode[i].Quality = 100;
										pIpcam_st->onvif_st.vcode[i].h264_mode = 0;
										pIpcam_st->onvif_st.vcode[i].H264_profile = 0;  //H264 profile  baseline/main/high  (2-0)
									}								
								}

								save_ipcam_config_st(pAllInfo);
							}
							break;
						case RESTORET_DEFAULT_PARA_ALL:
							{
								command_drv("rm -rf /mnt/mtd/ipcam_config.data");
								need_reboot = 1;
							}
							break;
						default:
							ret = -1;
							break;
					}						
				
					g_cmd.page= ret;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);	

					if( need_reboot )
					{						
						command_drv("reboot");
					}
				}
				break;
			case NET_CAM_GET_SYS_PARAMETERS:
				DPRINTK("NET_CAM_GET_SYS_PARAMETERS \n");
				{
					NET_IPCAM_SYS_INFO sys_info;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	
					memset(&sys_info,0x00,sizeof(sys_info));				

					{
						int year,month,day,hour,minute,second;
						time_t time1;
						long long  ipcam_time;
						struct timeval now_time;
						gettimeofday( &now_time, NULL );
						time1= now_time.tv_sec;
					
						g_pstcommand->GST_FTC_time_tToCustomTime(time1,&year,&month,&day,&hour,&minute,&second);

						ipcam_time = (year+1900)*10000 + (month+1)*100 + day;
						ipcam_time = ipcam_time*1000000;
						ipcam_time = ipcam_time + hour*10000 + minute*100 + second;

						sys_info.day_date = (year+1900)*10000 + (month+1)*100 + day;
						sys_info.day_time = hour*10000 + minute*100 + second;
						sys_info.time_region = pAllInfo->sys_st.time_region;
						sys_info.time_zone = pAllInfo->sys_st.time_zone;	
						sys_info.stardard = pAllInfo->sys_st.ipcam_stardard;
					}	

					g_cmd.length = sizeof(sys_info);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&sys_info,sizeof(sys_info));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);
				}
				break;
			case NET_CAM_SET_SYS_PARAMETERS:
				DPRINTK("NET_CAM_SET_SYS_PARAMETERS \n");
				{					
					NET_IPCAM_SYS_INFO sys_info;					
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					int need_reboot = 0;
					int need_save = 0;
					memset(&sys_info,0x00,sizeof(sys_info));
					memcpy(&sys_info,buf+ sizeof(g_cmd),sizeof(sys_info));

					DPRINTK("day:%d  time:%d time_region=%d time_zone=%d\n",
						sys_info.day_date,sys_info.day_time,sys_info.time_region,sys_info.time_zone);

					if( sys_info.stardard != pAllInfo->sys_st.ipcam_stardard)
					{
						pAllInfo->sys_st.ipcam_stardard = sys_info.stardard;
						need_reboot = 1;
						need_save = 1;
					}
					
					if( pAllInfo->sys_st.time_region !=  sys_info.time_region  ||
						pAllInfo->sys_st.time_zone !=  sys_info.time_zone )
					{	
						pAllInfo->sys_st.time_region = sys_info.time_region;
						pAllInfo->sys_st.time_zone = sys_info.time_zone;		
						need_save = 1;
					}					

					if(sys_info.day_date != 0 )
					{						
						time_t tmp_time = 0;
						int _Year;
						int _Month;
						int _Day;
						int _Hour;
						int _Minute;
						int _Second;	
						char tmp_buf[200];

						_Year = sys_info.day_date /10000;
						_Month = sys_info.day_date % 10000 /100;
						_Day = sys_info.day_date % 100;

						_Hour = sys_info.day_time /10000;
						_Minute = sys_info.day_time % 10000 /100;
						_Second = sys_info.day_time % 100;	

						DPRINTK("set date %d-%d-%d %d:%d:%d  zone=%d\n",
							_Year,_Month,_Day,_Hour,_Minute,_Second,pAllInfo->sys_st.time_zone);						
						
						
						tmp_time = FTC_CustomTimeTotime_t2(_Year,_Month,_Day,_Hour,_Minute,_Second);
						tmp_time += pAllInfo->sys_st.time_zone*3600;							
								
						FTC_time_tToCustomTime2(tmp_time,&_Year,&_Month,&_Day,&_Hour,&_Minute,&_Second);	

						DPRINTK("set show date %d-%d-%d %d:%d:%d\n",
							_Year,_Month,_Day,_Hour,_Minute,_Second);						
						
						
						sprintf(tmp_buf,"date %0.2d%0.2d%0.2d%0.2d%0.4d.%0.2d",_Month,_Day,_Hour,_Minute,_Year,_Second);
						
						SendM2(tmp_buf);						
					}

				
							
					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);	
				

					if( need_save == 1)
					{
						save_ipcam_config_st(pAllInfo);
					}					

					if( need_reboot == 1 )
					{
						command_drv("reboot");
					}
					
				}
				break;			
			case NET_CAM_GET_USER_INFO:
				DPRINTK("NET_CAM_GET_USER_INFO \n");
				{
					NET_USER_INFO user_info;
					int i = 0;

					memset(&user_info,0x00,sizeof(user_info));

					for( i = 0; i < 4; i++)
					{
						strcpy(user_info.name[i],socket_ctrl.user_name[i]);
						strcpy(user_info.pass[i],socket_ctrl.user_password[i]);
					}				

					g_cmd.length = sizeof(user_info);					
					memset(pc_cmd,0x00,1024);
					memcpy(pc_cmd,&g_cmd,sizeof(g_cmd));
					memcpy(pc_cmd+sizeof(g_cmd),&user_info,sizeof(user_info));
					Net_dvr_send_cam_data(pc_cmd,sizeof(g_cmd)+g_cmd.length,pst);				
				}
			 case NET_CAM_SET_USER_INFO:
				DPRINTK("NET_CAM_SET_USER_INFO \n");
				{
					NET_USER_INFO user_info;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_SYS_ST * pSysInfo = &pAllInfo->sys_st;
					int i = 0;

					memset(&user_info,0x00,sizeof(user_info));					
					memcpy(&user_info,buf+ sizeof(g_cmd),sizeof(user_info));


					for( i = 0; i < 4; i++)
					{
						DPRINTK("Set User: %s %s\n",
							user_info.name[i],user_info.pass[i]);					
					
						strcpy(socket_ctrl.user_name[i],user_info.name[i]);
						strcpy(socket_ctrl.user_password[i],user_info.pass[i]);
						strcpy(pSysInfo->ipcam_username[i],user_info.name[i]);
						strcpy(pSysInfo->ipcam_password[i],user_info.pass[i]);
						pSysInfo->ipcam_user_mode[i] = user_info.user_mode[i]; // IPCAM_USER_MODE_ADMIN OR IPCAM_USER_MODE_GUEST
					}	

					save_ipcam_config_st(pAllInfo);

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;
			 case NET_CAM_USER_LOGIN:
			 	DPRINTK("NET_CAM_USER_LOGIN \n");
				{
					NET_USER_INFO user_info;
					IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
					IPCAM_SYS_ST * pSysInfo = &pAllInfo->sys_st;
					int i = 0;
					int is_right_login = 0;

					memset(&user_info,0x00,sizeof(user_info));					
					memcpy(&user_info,buf+ sizeof(g_cmd),sizeof(user_info));
					

					for( i = 0; i < 4; i++)
					{
						DPRINTK("Login User: %s %s  - %s %s\n",
							user_info.name[0],user_info.pass[0],
							pSysInfo->ipcam_username[i],pSysInfo->ipcam_password[i]);
					
						if( strcmp(pSysInfo->ipcam_username[i],user_info.name[0]) != 0 )
							continue;

						if( strcmp(pSysInfo->ipcam_password[i],user_info.pass[0]) != 0 )
							continue;

						is_right_login = 1;

						break;
					}	

					if(is_right_login == 0 )
						g_cmd.page= BAD_USER_PASS;	
					else
						g_cmd.page= i; // i == 0 is stand for super user.		
						
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);		
			 	}
			 	break;
			case NET_CAM_GET_ADC_MODE:
				DPRINTK("NET_CAM_GET_ADC_MODE \n");
				{
					// normal  是普通灯板
					// yatai_adc     是亚泰灯板
					ret = command_drv2("cat /mnt/mtd/adc_cfg.txt | grep 0");
					DPRINTK("cat /mnt/mtd/adc_cfg.txt | grep 0  =%d\n",ret);	
					if( ret == 0 )
						g_cmd.page = YATAI_LIGHT_BOARD;
					else
						g_cmd.page = NORMAL_LIGHT_BOARD;					
																
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;
			 case NET_CAM_SET_ADC_MODE:
				DPRINTK("NET_CAM_SET_ADC_MODE \n");
				{
					if( g_cmd.length == NORMAL_LIGHT_BOARD )
					{
						command_drv2("echo \"1\" >  /mnt/mtd/adc_cfg.txt");
						DPRINTK("echo \"1\" >  /mnt/mtd/adc_cfg.txt\n");
					}else
					{
						command_drv2("echo \"0\" >  /mnt/mtd/adc_cfg.txt");
						DPRINTK("echo \"0\" >  /mnt/mtd/adc_cfg.txt\n");
					}

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;

			case NET_CAM_GET_AUDIO_MODE:
				DPRINTK("NET_CAM_GET_AUDIO_MODE \n");
				{
					// normal  是普通灯板
					// yatai_adc     是亚泰灯板
					ret = command_drv2("cat /mnt/mtd/audio_input_proc.txt | grep speak");
					DPRINTK("cat /mnt/mtd/audio_input_proc.txt | grep speak  =%d\n",ret);	
					if( ret == 0 )
						g_cmd.page = AUDIO_SPEAK_INPUT;
					else
						g_cmd.page = AUDIO_NORMAL_INPUT;					
																
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;
			 case NET_CAM_SET_AUDIO_MODE:
				DPRINTK("NET_CAM_SET_AUDIO_MODE \n");
				{
					if( g_cmd.length == AUDIO_NORMAL_INPUT )
					{
						command_drv2("echo \"normal\" >  /mnt/mtd/audio_input_proc.txt");
						DPRINTK("echo \"normal\" >  /mnt/mtd/audio_input_proc.txt\n");
					}else
					{
						command_drv2("echo \"speak\" >  /mnt/mtd/audio_input_proc.txt");
						DPRINTK("echo \"speak\" >  /mnt/mtd/audio_input_proc.txt\n");
					}

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;
			case NET_CAM_GET_ENCODE_TYPE:
				DPRINTK("NET_CAM_GET_ENCODE_TYPE \n");
				{

					if( g_cmd.length == 0 )
					{
						ret = command_drv2("cat /mnt/mtd/set_config/enc_type_0.txt | grep h265");
						DPRINTK("cat /mnt/mtd/set_config/enc_type_0.txt | grep h265  =%d\n",ret);	
						if( ret == 0 )
							g_cmd.page = CONFIG_ENC_TYPE_H265;
						else
							g_cmd.page = CONFIG_ENC_TYPE_H264;
					}else if( g_cmd.length == 1 )
					{
						ret = command_drv2("cat /mnt/mtd/set_config/enc_type_1.txt | grep h265");
						DPRINTK("cat /mnt/mtd/set_config/enc_type_1.txt | grep h265  =%d\n",ret);	
						if( ret == 0 )
							g_cmd.page = CONFIG_ENC_TYPE_H265;
						else
							g_cmd.page = CONFIG_ENC_TYPE_H264;
					}						
																
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;
			case NET_CAM_SET_ENCODE_TYPE:
				DPRINTK("NET_CAM_SET_ENCODE_TYPE \n");
				{
					
					mkdir("/mnt/mtd/set_config",0777);
					
					if( g_cmd.length == 0 )
					{
						if( g_cmd.page == CONFIG_ENC_TYPE_H265 )
						{
							command_drv2("echo \"h265\" >  /mnt/mtd/set_config/enc_type_0.txt");
							DPRINTK("echo \"h265\" >  /mnt/mtd/set_config/enc_type_0.txt\n");						
						}else
						{
							command_drv2("echo \"h264\" >  /mnt/mtd/set_config/enc_type_0.txt");
							DPRINTK("echo \"h264\" >  /mnt/mtd/set_config/enc_type_0.txt\n");	
						}
					}else if( g_cmd.length == 1 )
					{
						if( g_cmd.page == CONFIG_ENC_TYPE_H265 )
						{
							command_drv2("echo \"h265\" >  /mnt/mtd/set_config/enc_type_1.txt");
							DPRINTK("echo \"h265\" >  /mnt/mtd/set_config/enc_type_1.txt\n");		
						}else
						{
							command_drv2("echo \"h264\" >  /mnt/mtd/set_config/enc_type_1.txt");
							DPRINTK("echo \"h264\" >  /mnt/mtd/set_config/enc_type_1.txt\n");	
						}
					}		
					

					g_cmd.page= 1;					
					Net_dvr_send_cam_data(&g_cmd,sizeof(g_cmd),pst);				
				}
				break;	
			default:break;
		}

	}

	if( cmd == NET_DVR_NET_SELF_DATA )
	{
		if( g_pstcommand->GST_DRA_Net_dvr_recv_self_data != 0 )
		{			
			g_pstcommand->GST_DRA_Net_dvr_recv_self_data(buf,length);
		}else
		{
			DPRINTK("GST_DRA_Net_dvr_recv_self_data empty\n");
		}
	}
	
	return 1;
}



static void net_dvr_test_net_connected(void * value)
{
	
	net_connected_op_func();
	
	return ;
}

void net_cam_audio_encode_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	NET_MODULE_ST * pst=NULL;
	int s32Ret;
	
	pst = (NET_MODULE_ST *)value;

	DPRINTK(" pid = %d\n",getpid());
	


	while(pst->recv_th_flag == 1 )
	{
		if( pst->send_enc_data == 0 )
		{
			usleep(100);
			continue;
		}

		if( fifo_enable_insert(pst->cycle_send_buf) == 0 )
		{
			usleep(100);			
			continue;
		}	

		if( get_dvr_authorization() == 1 )
		{			
			
			net_cam_insert_audio_data(pst);
		}
		else
			usleep(100);		
	}

	pthread_detach(pthread_self());

	DPRINTK("out \n");


	return ;
	
}


int Net_cam_mtd_store_1()
{
	DIR * dir =NULL; 
	struct   dirent   *d = NULL;
	struct stat file_stat_st[50];
	struct stat compare_stat;
	int file_num = 0;
	int ret;
	int i;
	int file_same = 0;
	char filename[250];

	return 1;

	memset(&file_stat_st,0x00,sizeof(struct stat));

	ret = mount("/dev/mtdblock1","/mnt/nfs","jffs2",0,NULL);

	if( ret < 0 )
	{
		DPRINTK("mount /dev/mtdblock1 /mnt/nfs error\n");
		goto store_failed;
	}
	
	dir=opendir("/mnt/nfs"); 	
	while((d=readdir(dir))) 
	 { 			
		if(strcmp(d->d_name,".") == 0 )
			continue;
		if(strcmp(d->d_name,"..") == 0 )
			continue;

		sprintf(filename,"/mnt/nfs/%s",d->d_name);
	 
   		ret = stat(filename, &file_stat_st[file_num]);

		if( ret < 0 )
		{
			DPRINTK("stat %s error\n",d->d_name);
			goto store_failed;
		}		

		// DPRINTK("%s: st_size=%d  st_mtime=%ld\n",filename,
		// 	file_stat_st[file_num].st_size,file_stat_st[file_num].st_mtime);

		   
		file_num++;
		if( file_num >= 50 )
		{
			DPRINTK(" file num bigger than 50\n");
			goto store_failed;
		}
	 } 	 
	closedir(dir);		
	
	dir=opendir("/mnt/mtd"); 	
	while((d=readdir(dir))) 
	 { 		     		
		if(strcmp(d->d_name,".") == 0 )
			continue;
		if(strcmp(d->d_name,"..") == 0 )
			continue;

		sprintf(filename,"/mnt/mtd/%s",d->d_name);
	 
   		ret = stat(filename, &compare_stat);

		if( ret < 0 )
		{
			DPRINTK("stat %s error\n",d->d_name);
			goto store_failed;
		}		

		//DPRINTK("%s: st_size=%d  st_mtime=%ld\n",filename,
		// 	compare_stat.st_size,compare_stat.st_mtime);

		file_same = 0;
		for(i = 0; i <file_num; i++)
		{
			//DPRINTK("compare: %d %d  %ld %ld\n",file_stat_st[i].st_size,compare_stat.st_size
			//	,file_stat_st[i].st_mtime,compare_stat.st_mtime);
			
			if( (file_stat_st[i].st_size == compare_stat.st_size )
				&&  (file_stat_st[i].st_mtime == compare_stat.st_mtime) )
			{
				file_same = 1;
				//DPRINTK("same file!\n");
			}
		}	


		if( file_same == 0 )
		{
			// new file!;

			char cmd[200];

			DPRINTK("%s: st_size=%d  st_mtime=%ld\n",filename,
		 	compare_stat.st_size,compare_stat.st_mtime);

			sprintf(cmd,"cp -rpf %s /mnt/nfs",filename);

			DPRINTK("exec: %s\n",cmd);
			
			SendM2(cmd);		
		
		}
		
	 } 	 

	closedir(dir);

	umount("/mnt/nfs");	

	return 1;
	

store_failed:

	if( dir )
		closedir(dir);


	return -1;
}

int Net_cam_mtd_store()
{
	int ret = 0;

	mtd_store_lock();
	
	ret = Net_cam_mtd_store_1();

	mtd_store_unlock();

	return ret;
}



int dvr_cam_create_audio_enc_thread(NET_MODULE_ST * pst)
{
	static NET_MODULE_ST * s_pst = NULL;
	int ret;
	pthread_t id_encode1;

	s_pst = pst;


	ret = pthread_create(&id_encode1,NULL,(void*)net_cam_audio_encode_thread,(void *)s_pst);
	if ( 0 != ret )
	{
		trace( "create encode_thread thread error\n");
		return -1;
	}


	return 1;
}

void Set_motion_alarm_md_num(int md_num)
{
	return GM_set_motion_alarm_md_num(&gm_enc_st[1],md_num);
}



int Get_motion_alarm_buf(unsigned char * buf,int * mb_width,int *mb_height)
{
	if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_576P )
	{
		return GM_get_motion_alarm_buf(&gm_enc_st[0],buf,mb_width,mb_height);
	}

	return GM_get_motion_alarm_buf(&gm_enc_st[1],buf,mb_width,mb_height);
}


#define PROCESS_NAME_LIST "/ramdisk/process.txt"

void net_check_onvif_rtsp_work()
{
	char cmd[10000];
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
//	int i,j;	int pid;
	


	{
		char onvif_server_cmd[100];
		char rtsp_server1_cmd[100];
		char rtsp_server2_cmd[100];
		char excute_cmd[100];
		IPCAM_ALL_INFO_ST * pAllInfo =  NULL;
		int i = 0;

		pAllInfo =  get_ipcam_config_st();

		sprintf(onvif_server_cmd,"/ramdisk/onvif_server 127.0.0.1 16100");
		sprintf(rtsp_server1_cmd,"/ramdisk/testH264VideoStreamer 127.0.0.1 %d 0",pAllInfo->rtsp_st.url_port[0]);
		sprintf(rtsp_server2_cmd,"/ramdisk/testH264VideoStreamer 127.0.0.1 %d 1",pAllInfo->rtsp_st.url_port[1]);


		
		if( Net_have_onvif_app_connect() <= 0 )	
		{
			sprintf(excute_cmd,"%s &",onvif_server_cmd);
			command_drv(excute_cmd);
			DPRINTK("cmd:%s\n",excute_cmd);
		}
	
	 	#ifdef USE_RTSP_SERVER
		#else
		
		if( pMemModule[0] == NULL )
		{
			sprintf(excute_cmd,"nice -n -15 %s &",rtsp_server1_cmd);
			command_drv(excute_cmd);
			DPRINTK("cmd:%s\n",excute_cmd);
		}

		if( pMemModule[1] == NULL )
		{
			sprintf(excute_cmd,"%s &",rtsp_server2_cmd);
			command_drv(excute_cmd);
			DPRINTK("cmd:%s\n",excute_cmd);
		}
		#endif
		

	}

	return ;

op_error:
	if( fp )
	{
	   fclose(fp);
	   fp = NULL;
	}
	return ;
}


int net_recv_enc_data_mem(void * point,char * buf,int length,int cmd)
{
	MEM_MODULE_ST * pst = (MEM_MODULE_ST *)point;
	{
		
		DPRINTK("no support recv chan=%d\n",pst->dvr_cam_no);		
	}	
	
	return 1;
}

void rtsp_server_connect()
{
	int i = 0;
	for( i = 0 ; i < 2; i++)
	{	
		if( pMemModule[i] != NULL )
			continue;
		
		pMemModule[i] = MEM_init_share_mem(i,512*1024,0,net_recv_enc_data_mem);
	
		if( pMemModule[i]   == NULL )
		{					
			DPRINTK("create pMemModule %d error!\n",i);			
		}else
			pMemModule[i]->dvr_cam_no = i;									
	} 
}



static void net_onvif_rtsp_server_check_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	if( ipcam_is_now_dhcp() == 1 )
	{
		sleep(5);
		ipcam_auto_dhcp();		
	}

	//启动太早，程序会不正常。
	sleep(15);

	while( 1 )
	{			
		sleep(2);		
		net_check_onvif_rtsp_work();		
		sleep(3);	
		#ifdef USE_RTSP_SERVER
		{
			static int count = 0;			
			IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
			count++;
			if( count < 3 )
			{
				Hisi_set_isp_value(pAllInfo,IPCAM_CODE_SET_PARAMETERS);
			}
		}
		#else
		rtsp_server_connect();
		#endif
	}

	
	return ;
}


int preview_frame_rate_show_audio()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;
//	char cmd[50];
//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > get_dvr_max_chan(1)*25*5 )
	  {	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));
		count = 0;
		DPRINTK("rate: %d f/s (%d,%d) (%u,%u)len=%u %u \n",frameRate,cycle_enc_buf->item_num,
			cycle_enc_buf->save_num,cycle_enc_buf->in,cycle_enc_buf->out,
			fifo_len(cycle_enc_buf),cycle_enc_buf->size);	
	  }

	  return 1;
	
}


int preview_frame_rate_show_video()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;
//	char cmd[50];
//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > get_dvr_max_chan(1)*25*10 )
	  {	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));
		count = 0;
		DPRINTK("rate: %d f/s (%d,%d) (%u,%u)len=%u %u \n",frameRate,cycle_enc_buf->item_num,
			cycle_enc_buf->save_num,cycle_enc_buf->in,cycle_enc_buf->out,
			fifo_len(cycle_enc_buf),cycle_enc_buf->size);	
	  }

	  return 1;
	
}

int preview_frame_rate_show_net_video()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;
//	char cmd[50];
//	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if(count > get_dvr_max_chan(1)*25*10 )
	  {	  	
		get_sys_time( &tvNew, NULL );
		frameRate = div_safe2(count , (tvNew.tv_sec - oldTime ));
		count = 0;
		DPRINTK("rate: %d f/s (%d,%d) (%u,%u)len=%u %u \n",frameRate,cycle_net_buf->item_num,
			cycle_net_buf->save_num,cycle_net_buf->in,cycle_net_buf->out,
			fifo_len(cycle_net_buf),cycle_net_buf->size);	
	  }

	  return 1;
	
}







int insert_enc_buf(BUF_FRAMEHEADER * header,char * buf)
{
	int ret;
	int length;

	if( header->iLength >= (MAX_FRAME_BUF_SIZE - 10*1024) )
	{
		DPRINTK(" header->iLength = %d too big!\n",header->iLength);
		return -1;
	}
	

	fifo_lock_buf(cycle_enc_buf);		
	
	ret = fifo_put(cycle_enc_buf,header,sizeof(BUF_FRAMEHEADER));
	if( ret != sizeof(BUF_FRAMEHEADER) )
	{
		DPRINTK("%d insert error!\n",ret);		
		goto err;
	}

	length = header->iLength;

	ret = fifo_put(cycle_enc_buf,buf,length);
	if( ret != length )
	{
		DPRINTK("%d %d insert error!\n",ret,length);		
		goto err;
	}

	
	fifo_unlock_buf(cycle_enc_buf);

	if( header->type == VIDEO_FRAME )
		preview_frame_rate_show_video();

	if( header->type == AUDIO_FRAME )
		preview_frame_rate_show_audio();

	return 1;

err:
	fifo_reset(cycle_enc_buf);
	fifo_unlock_buf(cycle_enc_buf);

	return -1;
}


int get_enc_buf(BUF_FRAMEHEADER * header,char * buf)
{
	int ret;
	int length;
	

	fifo_read_lock_buf(cycle_enc_buf);
	
	ret = fifo_get(cycle_enc_buf,header,sizeof(BUF_FRAMEHEADER));
	if( ret != sizeof(BUF_FRAMEHEADER) )
	{
		DPRINTK("%d get error!\n",ret);		
		goto err2;
	}

	length = header->iLength;	

	//DPRINTK(" ret=%d  length=%d\n",ret,length);

	ret = fifo_get(cycle_enc_buf,buf,length);
	if( ret != length )
	{
		DPRINTK("%d %d get error! %d\n",ret,length,sizeof(BUF_FRAMEHEADER));		
		goto err2;
	}
	fifo_read_unlock_buf(cycle_enc_buf);

	return 1;

err2:
	fifo_read_unlock_buf(cycle_enc_buf);
	
	fifo_lock_buf(cycle_enc_buf);
	fifo_reset(cycle_enc_buf);
	fifo_unlock_buf(cycle_enc_buf);

	return -1;
}

int is_enable_insert_rec_data()
{
	return fifo_enable_insert(cycle_enc_buf);
}

int is_enable_get_rec_data()
{
	return fifo_enable_get(cycle_enc_buf);
}

void set_cycle_save_num(int num)
{
	if( num <= 0 )
	{
		DPRINTK("count%d error\n",num);		
		return;
	}
	
	filo_set_save_num(cycle_enc_buf,num);
}


int insert_net_buf(BUF_FRAMEHEADER * header,char * buf)
{
	int ret;
	int length;

	if( header->iLength >= (MAX_FRAME_BUF_SIZE - 10*1024) )
	{
		DPRINTK(" header->iLength = %d too big!\n",header->iLength);
		return -1;
	}

	if( is_enable_insert_net_data() <= 0 )
		return -1;	


	fifo_lock_buf(cycle_net_buf);		


	ret = fifo_put(cycle_net_buf,header,sizeof(BUF_FRAMEHEADER));
	if( ret != sizeof(BUF_FRAMEHEADER) )
	{
		DPRINTK("%d insert error!\n",ret);		
		goto err;
	}

	length = header->iLength;

	ret = fifo_put(cycle_net_buf,buf,length);
	if( ret != length )
	{
		DPRINTK("%d %d insert error!\n",ret,length);		
		goto err;
	}
	
	
	fifo_unlock_buf(cycle_net_buf);

	if( header->type == VIDEO_FRAME )
		preview_frame_rate_show_net_video();
	

	return 1;

err:
	fifo_reset(cycle_net_buf);
	fifo_unlock_buf(cycle_net_buf);

	return -1;
}



int get_net_buf(BUF_FRAMEHEADER * header,char * buf)
{
	int ret;
	int length;	

	if( is_enable_get_net_data() <=0 )
		return -1;	
	
	fifo_read_lock_buf(cycle_net_buf);
	
	ret = fifo_get(cycle_net_buf,header,sizeof(BUF_FRAMEHEADER));
	if( ret != sizeof(BUF_FRAMEHEADER) )
	{
		DPRINTK("%d get error!\n",ret);		
		goto err2;
	}

	length = header->iLength;		

	ret = fifo_get(cycle_net_buf,buf,length);
	if( ret != length )
	{
		DPRINTK("%d %d get error! %d\n",ret,length,sizeof(BUF_FRAMEHEADER));		
		goto err2;
	}
	fifo_read_unlock_buf(cycle_net_buf);

	return 1;

err2:
	fifo_read_unlock_buf(cycle_net_buf);
	
	fifo_lock_buf(cycle_net_buf);
	fifo_reset(cycle_net_buf);
	fifo_unlock_buf(cycle_net_buf);

	return -1;
}


int reset_net_cycle_buf()
{
	fifo_lock_buf(cycle_net_buf);
	fifo_reset(cycle_net_buf);
	fifo_unlock_buf(cycle_net_buf);
}

int is_enable_insert_net_data()
{	
	int ret = 0;
	
	if( fifo_num(cycle_net_buf) > 30 )
	{
		DPRINTK("num=%d \n",fifo_num(cycle_net_buf));
		return 0;
	}
	

	return fifo_enable_insert(cycle_net_buf);
}

int get_net_data_num()
{
	return fifo_num(cycle_net_buf);
}

int is_enable_get_net_data()
{
	return fifo_enable_get(cycle_net_buf);
}


int get_net_local_view_flag()
{
	return g_enable_net_local_view_flag;
}



void encode_thread(void * value)
{	
	SET_PTHREAD_NAME(NULL);
	int save_buf_num = 0;
	int i;
	int write_count = 0;
	int rel;
	NET_MODULE_ST * pst=NULL;
	NET_MODULE_ST * pst2=NULL;
	int get_video_data = 0;
	int thread_id = 0;

	thread_id = *(int*)value;
	
	DPRINTK("encode_thread pid = %d thread_id=%d\n",getpid(),thread_id);

	while(all_encode_flag)
	{	
		get_video_data = 0;
	       
		pst = client_pst[0];
		if( pst )
		{
			if( pst->recv_th_flag ==  1 )
			{
				if( g_enable_rec_flag == 0 )
	       			{
					if( fifo_enable_insert(pst->cycle_send_buf) == 0 )
					{			
						usleep(10000);			
						continue;
					}else
						get_video_data = 1;
				}else
				{
					get_video_data = 1;
				}
			}
		}	
	       	

		pst2 = client_pst[1];
		if( pst2 )
		{
			if( pst2->is_alread_work ==  1 )
			{				
				get_video_data = 1;
			}
		}

		if( Net_cam_is_open_rstp_server() ==1 )
		{
			get_video_data = 1;
		}

		 if( g_enable_net_local_view_flag == 1 )
		 {
		 	get_video_data = 1;
		 }

		 if( g_enable_rec_flag == 1 )
		{
			if( is_enable_insert_rec_data() == 1 )
				get_video_data = 1;
			else
				get_video_data = 0;
		}

		//printf("get_video_data = %d thread_id=%d\n",get_video_data,thread_id);
		if( get_video_data == 1 )
		{
			//if( get_dvr_authorization() == 1 )
			{
				if( thread_id == 0 )
					Hisi_Net_cam_net_send_video_data(pst,g_enable_rec_flag,g_enable_net_local_view_flag);
				else if( thread_id == 1 )
					Hisi_Net_cam_extern_net_send_video_data(pst,g_enable_rec_flag,g_enable_net_local_view_flag);				
				else if( thread_id == 2 )
					Hisi_Net_cam_rec_send_video_data(pst,g_enable_rec_flag,g_enable_net_local_view_flag);
				else if( thread_id == 3 )						
					Hisi_Net_cam_rtsp_send_video_data(pst,g_enable_rec_flag,g_enable_net_local_view_flag);
				else
					usleep(10000);
			}
		}else		
			usleep(10000);			

		
	}

	pthread_detach(pthread_self());

	DPRINTK("out \n");
}


void net_dvr_audio_udp_th(void * value)
{
	SET_PTHREAD_NAME(NULL);
	while(1)
	{
		audio_udp_send_func();
		sleep(3);
	}
	
	return ;
}

void net_dvr_audio_udp_th2(void * value)
{
	SET_PTHREAD_NAME(NULL);
	while(1)
	{
		audio_udp_recv_func();
		sleep(3);
	}
	
	return ;
}


int dvr_is_recording()
{
	if( g_pstcommand->GST_DISK_GetDiskNum() ==  0 )
		return 0;

	return g_enable_rec_flag;
}

int  app_select_func()
{
	FILE * fp = NULL;
      int fd;
      int sec = 10;
      char wd = 0;
      int check_time = 0;  
	int ret = 0;

	fp = fopen("/mnt/mtd/appselect.txt","r+b");
	if( fp == NULL )
	{		
		return ret;
	}
	
	fseek( fp , 0 , SEEK_SET );	
	
	ret = fread(&wd,1,1,fp);
	if( ret != 1 )
	{			
		DPRINTK("fread appselect error!\n");			
	}		

	fclose(fp);
	fp = NULL;
	
	if( wd == '1' )
		ret = 1;
	else
		ret = 0;

	return ret;
		
}

int get_appselect_num()
{
	return app_select_num;
}

int ipcam_is_now_dhcp()
{
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

		if( pAllInfo->net_st.net_use_dhcp == 1 )
		{	
			if( get_dhcp_status() == 0 )
				return 1;
			
			return 0;
		}else
			return 0;
}


void ipcam_get_sensor_name(char * sensor_name)
{
	

	switch(g_sensor_type)
	{
		case APTINA_AR0130_DC_720P_30FPS:
			strcpy(sensor_name,"AR0130");
			break;
		case APTINA_9M034_DC_720P_30FPS:
			strcpy(sensor_name,"9M034");
			break;
		case SONY_IMX138_DC_720P_30FPS:
			strcpy(sensor_name,"IMX138");
			break;
		case SONY_IMX122_DC_1080P_30FPS:
			strcpy(sensor_name,"IMX122");
			break;
		case OMNI_OV9712_DC_720P_30FPS:
			strcpy(sensor_name,"OV9712");
			break;
		case APTINA_MT9P006_DC_1080P_30FPS:
			strcpy(sensor_name,"MT9P006");
			break;
		case OMNI_OV2710_DC_1080P_30FPS:
			strcpy(sensor_name,"OV2710");
			break;
		case HIMAX_1375_DC_720P_30FPS:
			strcpy(sensor_name,"HM1375");
			break;
		case GOKE_SC1035:
			strcpy(sensor_name,"SC1035	");
			break;
		case GOKE_JXH42:
			strcpy(sensor_name,"JXH42");
			break;
		case GOKE_SC2135:
			strcpy(sensor_name,"SC2135");
			break;
		case GOKE_SC1145:
			strcpy(sensor_name,"SC1145");
			break;
		case GOKE_SC1135:
			strcpy(sensor_name,"SC1135");
			break;
		case GOKE_OV9750:
			strcpy(sensor_name,"OV9750");
			break;
		case HI3516E_IMX323:
			strcpy(sensor_name,"IMX323");
			break;
		case SONY_IMX274_LVDS_4K_30FPS:
			strcpy(sensor_name,"IMX274");
			break;
		case SONY_IMX290_LVDS_2K_30FPS:
			strcpy(sensor_name,"IMX290");
			break;
		default:
			strcpy(sensor_name,"NOKNOW SENSOR");
			break;
	}	
	

	DPRINTK("sensor=%d  %s\n",g_sensor_type,sensor_name);
}

void init_ipcam_all_info()
{
	int i = 0;
	IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

	get_nvr_ver_info(pAllInfo->app_version);
	ipcam_get_sensor_name(pAllInfo->isp_st.strSensorTypeName);

	for( i = 0; i < 2; i++)
	{
		pAllInfo->onvif_st.input[i].w = gm_enc_st[i].width;
		pAllInfo->onvif_st.input[i].h = gm_enc_st[i].height;
		pAllInfo->onvif_st.vcode[i].w  = gm_enc_st[i].width;
		pAllInfo->onvif_st.vcode[i].h  = gm_enc_st[i].height;		
	}	

	if( pAllInfo->load_default_isp == 1)
	{
		DPRINTK("load default isp\n");
		Hisi_get_isp_sharpen_value(pAllInfo);
		Hisi_set_isp_init_value(0);
		Hisi_get_isp_colormatrix(pAllInfo);
		// Isp night set is same to day set.		
		pAllInfo->load_default_isp = 0;
	}else
	{
		
		
	}
	
	Hisi_get_ipcam_config();
	

	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_AEAttr);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_vi_csc);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_au16StaticWB);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_SharpenAttr);
	Hisi_set_isp_value(pAllInfo,IPCAM_CODE_SET_PARAMETERS);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_CMD_DenoiseAttr);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_SET_COLORMATRIX);	
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_SET_GAMMA_MODE);
	Hisi_set_isp_value(pAllInfo,IPCAM_ISP_SET_LIGHT_CHEKC_MODE);

	

	//Hisi_set_vpp_value(0,2,2,2);

	
}

void net_auto_dhcp_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	char ip[30];
	char msk[30];
	char gateway[30];
	char dns1[30];
	char dns2[30];
	static int is_run = 0;
	int ret = 0;


	while( is_run == 1 )
	{
		DPRINTK("wait for dhcp\n");
		sleep(1);
	}

	is_run = 1;		

	g_auto_dhcp_over = 0;
	
	ret = try_dhcp(ip,msk,gateway,dns1,dns2);
	if( ret >= 0 )
	{
		int server_port;
		int http_port;	
		get_server_port(&server_port,&http_port);
		set_net_parameter_dev(net_dev_name,ip,server_port,gateway,
			msk/*"255.255.255.0"*/,dns1,dns1);
	}else
	{
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
		IPCAM_NET_ST * pNetSt = NULL;
		char ip[50] ="";
		char netmask[50] = "";
		char gw[50] ="";
		char dns1[50] ="";
		char dns2[50] = "";	
		int server_port;
		int http_port;	
		get_server_port(&server_port,&http_port);

		pNetSt = &pAllInfo->net_st;	
		sprintf(ip,"%d.%d.%d.%d",pNetSt->ipv4[0],pNetSt->ipv4[1],pNetSt->ipv4[2],pNetSt->ipv4[3]);
		sprintf(netmask,"%d.%d.%d.%d",pNetSt->netmask[0],pNetSt->netmask[1],pNetSt->netmask[2],pNetSt->netmask[3]);
		sprintf(gw,"%d.%d.%d.%d",pNetSt->gw[0],pNetSt->gw[1],pNetSt->gw[2],pNetSt->gw[3]);
		sprintf(dns1,"%d.%d.%d.%d",pNetSt->dns1[0],pNetSt->dns1[1],pNetSt->dns1[2],pNetSt->dns1[3]);
		sprintf(dns2,"%d.%d.%d.%d",pNetSt->dns2[0],pNetSt->dns2[1],pNetSt->dns2[2],pNetSt->dns2[3]);
		set_net_parameter_dev(net_dev_name,ip,server_port,gw,netmask,dns1,dns2);
		net_set_write_dns_file(dns1,dns2);	
	}

	DPRINTK("ret=%d\n",ret);


	g_auto_dhcp_over = 1;
	
	return ;
}


int ipcam_auto_dhcp()
{
	int ret;
	pthread_t id_dhcp;

	DPRINTK("start \n");

	ret = pthread_create(&id_dhcp,NULL,(void*)net_auto_dhcp_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_ip_test_thread  thread error\n");		
	}

	pthread_join(id_dhcp,NULL);

	DPRINTK("end \n");

	return 1;
}

int DC_InsertListData(SINGLY_LINKED_LIST_INFO_ST * plist,char * frame_data_buf,int frame_len)
{	
	int ret;
	
	while( sll_list_enable_add_item(plist) == 0)
	{				
		sll_del_list_tail_item(plist);				
	}
	
	ret = sll_add_list_item_no_malloc(plist,frame_data_buf,frame_len);

	return ret;
}


int DC_ClearListData(SINGLY_LINKED_LIST_INFO_ST * plist)
{
	while( sll_list_item_num(plist) > 0 )
	{
		sll_del_list_tail_item(plist);
	}

	return 1;
}

int goke_audio_data_callback(char * pBuf,int len,int chan)
{
	unsigned char  * audio_buf = (unsigned char*)pBuf;
	unsigned char  audio_buf2[MAX_AUDIO_BUF];
	int send_length;
	struct timeval recTime;
	BUF_FRAMEHEADER fheader;
	int ret;	
	int count;
	int i;
	int cam;
	int max_chan = get_dvr_max_chan(0);
	NET_MODULE_ST * pst = NULL; 
	struct timeval tv;
	char my_buf[MAX_FRAME_BUF_SIZE];
	int maxfd = 0;	
	int send_net_frame = 0; 
	int j;

	//DPRINTK("get [%d] len=%d\n",chan,len);
	
	pst = client_pst[0];
	
	max_chan = 1;	
	get_sys_time( &recTime, NULL );	
						
	j = chan;				
	cam = j;			
				
	fheader.iLength = len;
	fheader.iFrameType =0; 
	fheader.index = cam;	
	fheader.type = AUDIO_FRAME;
	fheader.ulTimeSec = recTime.tv_sec;
	fheader.ulTimeUsec = recTime.tv_usec;
	fheader.standard = 0;		
	
	if( g_enable_rec_flag )
	{
		if( is_enable_insert_rec_data() > 0 && (g_pstcommand->GST_DISK_GetDiskNum() > 0))
		{
			insert_enc_buf(&fheader,audio_buf);
		}
	}
	
	if( g_net_audio_open )
	{	
		if( machine_use_audio() == 0 )
		{				
			if( net_dvr_get_net_listen_chan() == NET_DVR_CHAN)
			{
				if( get_enable_insert_data_num(&g_audioList)>NET_BUF_REMAIN_NUM )
				{							
					insert_data(&fheader,audio_buf,&g_audioList);								
				}
			}
		}
	}
	
	if( local_audio_play )
	{
		if( g_enable_rec_flag != 1 )
		{		
			get_one_buf(&fheader,audio_buf,&g_audioList);
		}		
		
		if( get_bit(local_audio_play_cam, fheader.index) )
		{				
			Hisi_write_audio_buf(audio_buf,fheader.iLength,fheader.index);		
		}
	
	}
	
	if( pst != NULL && pst->client >  0 )
	{			
		memcpy(audio_buf2,&fheader,sizeof(fheader));
		memcpy(audio_buf2+sizeof(fheader),audio_buf,fheader.iLength);
		send_length = fheader.iLength + sizeof(BUF_FRAMEHEADER);
		
		if( NM_net_send_data(pst,audio_buf2,send_length,NET_DVR_NET_FRAME) < 0 )
		{
			DPRINTK("insert data error: chan=%ld type=%d length=%ld type=%ld num=%ld %d\n",
				fheader.index,fheader.type,fheader.iLength,fheader.iFrameType,
				fheader.frame_num,send_length); 								
		}			
	}	
			
		
	 return 1;
go_out:
	return -1;
}

/*
float rj_chn_fps_calculate(int chn)
{
	static int count[64] = {0};
	static long oldTime[64] = {0};
	float frameRate;

	struct timeval tvOld;
       struct timeval tvNew;

	  if( count[chn] == 0 )
	  {
		 gettimeofday( &tvOld, NULL );
		oldTime[chn] = tvOld.tv_sec;
		
	  }

	  count[chn]++;

	 	
	gettimeofday( &tvNew, NULL );

	if( abs(tvNew.tv_sec - oldTime[chn]) >= 5 )
	{
		frameRate = div_safe3(count[chn] , (tvNew.tv_sec - oldTime[chn] ));
		//DPRINTK("[%d] fps=%f  count=%d\n",chn,frameRate,count[chn]);
		count[chn] = 0;
		return frameRate;
	}	

	 return -1;	
}


int RJ_ChnFpsIncreaseCalculate(SINGLY_LINKED_LIST_INFO_ST * plist,int chn)
{
	float fps;
	fps = rj_chn_fps_calculate(chn);
	if( fps > 0 )
	{
		plist->chn_fps = fps;		
	}
	return 1;	
}


int RJ_InsertVideoAudioData(SINGLY_LINKED_LIST_INFO_ST * plist,char * frame_data_buf,int frame_len,RJ_CODE_TYPE video_code)
{	
	int ret;
	long long tmp_time;
	BUF_FRAMEHEADER * pheader = NULL;
	pheader = (BUF_FRAMEHEADER *)frame_data_buf;

	if( pheader->type == VIDEO_FRAME )
	{						
		RJ_ChnFpsIncreaseCalculate(plist,pheader->index);	

		tmp_time = pheader->ulTimeSec;
			tmp_time = tmp_time*1000000 + pheader->ulTimeUsec;
		plist->extern_data.last_video_time = tmp_time;
		if( GK_CODE_265 == video_code)
			plist->extern_data.video_code = (int)2;
		else
			plist->extern_data.video_code = (int)1;
	}

	while( sll_list_enable_add_item(plist) == 0)
	{				
		sll_del_list_tail_item(plist);			
	}	
	
	ret = sll_add_list_item_no_malloc(plist,frame_data_buf,frame_len);		
	return ret;
}
*/


int goke_video_data_callback(char * pBuf,int len,int chan)
{
	NET_MODULE_ST * pst = client_pst[0];
	NET_MODULE_ST * pst2 = NULL;
	char * my_buf = NULL;
	int VeChn = chan;	
	unsigned char * pVideoBuf = NULL;
	unsigned char * ptr_point = NULL;	
	BUF_FRAMEHEADER fheader;
	BUF_FRAMEHEADER * pHeader = NULL;
	char * video_data_ptr = my_buf;
	int ret;	
	static int check_enc_fps[MN_MAX_BIND_ENC_NUM] = {0};
	struct timeval recTime;
	static int count = 0;

	my_buf = (unsigned char*)malloc(len + sizeof(BUF_FRAMEHEADER));
	if( my_buf == NULL )
	{
		DPRINTK("malloc err!\n");
		goto err;
	}

	pVideoBuf = video_data_ptr = my_buf + sizeof(BUF_FRAMEHEADER);
	pHeader = my_buf;

									
	Net_cam_set_send_video_frame();	
		
	ptr_point = pVideoBuf;		
	
	memset(&fheader,0x00,sizeof(fheader));

	fheader.iLength = len;			

	if( fheader.iLength > (MAX_FRAME_BUF_SIZE - 1024) )
	{
		DPRINTK("VeChn = %d length = %ld too big size ,drop!\n",VeChn,fheader.iLength);
		
		pst->lost_frame[VeChn] = 1;				
		goto err;
	}

	//if( VeChn == 1 )
	//	DPRINTK("%x %x %x %x %x\n",pVideoBuf[0], pVideoBuf[1] ,pVideoBuf[2] , pVideoBuf[3] ,pVideoBuf[4]);

	if(pVideoBuf[0] ==0x0 && pVideoBuf[1] ==0x0 && pVideoBuf[2] ==0x0 && pVideoBuf[3] ==0x1 &&  (pVideoBuf[4] & 0x0f)  == (0x67 & 0x0f))
	{
		fheader.iFrameType =1; // 1 is keyframe 0 is not keyframe 	
		pst->lost_frame[VeChn] = 0;

		//DPRINTK("[%d]    len=%d\n",VeChn,len);
	}else
		fheader.iFrameType =0;		


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
		goto err;

	memcpy(video_data_ptr,pBuf,len);
	ptr_point = pVideoBuf;

	if( get_bit(pst->send_cam,VeChn) == 1 )
	{	  
	      ret  =GM_send_encode_data(pst,fheader.iFrameType,(char*)pHeader,fheader.iLength,VeChn);
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
				 if( get_bit(pst2->send_cam,VeChn) == 1 )
	    			 	GM_send_encode_data(pst2,fheader.iFrameType,(char*)pHeader,fheader.iLength,VeChn);
			}
		}
	 }	


	if( get_net_local_view_flag() == 1 )
	{				
	  	//ret = GM_send_dvr_net_data(fheader.iFrameType,(char*)pHeader,fheader.iLength,VeChn); 	
			
	}	

	if( dvr_is_recording() == 1 )
	{
		 ret  =GM_send_rec_data(fheader.iFrameType,(char*)pHeader,fheader.iLength,VeChn);	  
	}

	//DPRINTK("[%d] recv_packet_no=%d  len=%d\n",VeChn,recv_packet_no[VeChn],len);
#ifdef USE_RTSP_SERVER
	{				
		//DC_InsertListData(&gVideolist[VeChn],my_buf,len + sizeof(BUF_FRAMEHEADER));
		//RJ_InsertVideoAudioData(&gVideolist[VeChn],my_buf,len + sizeof(BUF_FRAMEHEADER),g_ipc_media.stMediaStream[VeChn].code);
	}
#else
	if(0)
	{
		MEM_MODULE_ST * pMem =NULL;
		int current_enc_fps = 0;
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
		
		check_enc_fps[VeChn]++;
	
		if( check_enc_fps[VeChn] > 5 )
		{
			check_enc_fps[VeChn] = 0;
			current_enc_fps = pAllInfo->onvif_st.vcode[VeChn].FrameRateLimit;
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
				memcpy(pHeader,&fheader,sizeof(BUF_FRAMEHEADER));					
				if( MEM_write_data_lock(pMem,(char*)pHeader,fheader.iLength + sizeof(BUF_FRAMEHEADER),0) < 0 )
				{
					DPRINTK("insert data error: chan=%ld length=%ld type=%ld num=%ld\n",
						VeChn,fheader.iLength,fheader.iFrameType,fheader.frame_num);	
	
					pMemModule[VeChn] = NULL;
				}
					
			}						
		}
	}


	if(my_buf)
	{
		free(my_buf);
		my_buf = NULL;
	}

#endif	

/*	if( VeChn == 0 )
	  	p720_encode_rate_show();
	else
		cif_decode_rate_show();	*/
		
	return 1;

err:

	if(my_buf)
	{
		free(my_buf);
		my_buf = NULL;
	}
	
	return -1;
}

void set_rtsp_fps(int chan ,int framerate)
{
	if( gVideolist[chan].self_date_fps != framerate)
	{
		DPRINTK("video[%d] %d -> %d\n",chan, gVideolist[chan].self_date_fps,framerate);
		gVideolist[chan].self_date_fps = framerate;	
	}
}

typedef enum {
    GADI2_ISP_ANTIFLICKER_FREQ_50HZ,
    GADI2_ISP_ANTIFLICKER_FREQ_60HZ
} GADI2_ISP_AntiFlickerFreqEnumT;

int init_all_device(video_profile * seting,int standard,int size_mode)
{
	int ret;
	int IsInMtdDir = 0;
	pthread_t id_encode;
//	pthread_t id_encode2;
	pthread_t id_decode;
//	pthread_t id_lcdshow;
	pthread_t id_audio_encode;
	pthread_t id_audio_decode;
	pthread_t id_net;
	pthread_t id_net_pc;
	pthread_t id_net_enc;
	pthread_t id_buf_check;
	pthread_t id_net_send;
	pthread_t id_net_check;
	pthread_t id_net_tes_connected;
	pthread_t id_net_2;
	pthread_t id_onvif_check;
	pthread_t id_net_upnp;
	pthread_t id_net_wifi;
	pthread_t id_onvif_data_send;
	int i;
	int video_in_mode;
	int wait_device_ready_num = 0;

	if ( IsFileExist("/mnt/mtd/video.xml" ) == 1)
			IsInMtdDir = 1;
		else
			IsInMtdDir = 0;

	app_select_num = app_select_func();

	DPRINTK("app_select_num = %d\n",app_select_num);
	
	cur_standard = standard;
	cur_size = size_mode;

	

	net_key_board.key_code = 0;

	if( seting != NULL )
	{
	#ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;
	    video_setting.quant = 20;
	#else
	    video_setting.qmax = 31;
	   video_setting.quant = 0;
	#endif
	    video_setting.qmin = 1;	   
	    video_setting.bit_rate = seting->bit_rate;
	    video_setting.width = seting->width;
	    video_setting.height = seting->height;
	    video_setting.framerate = seting->framerate;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;
	    video_setting.decode_width = seting->width;
	    video_setting.decode_height = seting->height;
	}
	else
	{
	   #ifdef USE_H264_ENC_DEC
	    video_setting.qmax = MAX_QUANT;	  
	#else
	    video_setting.qmax = 31;	
	#endif
	    video_setting.qmin = 1;
	    video_setting.quant = 60;
	    video_setting.bit_rate = 2048;
	    video_setting.framerate = 25;
	    video_setting.frame_rate_base = 1;
	    video_setting.gop_size = 60;

	if( size_mode == TD_DRV_VIDEO_SIZE_CIF )
			  video_setting.bit_rate = 512;				

	   if(  standard == TD_DRV_VIDEO_STARDARD_PAL)
	   {	
		    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_PAL;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_PAL;	
			video_setting.framerate = 25;
	   }else
	   {
	   	    video_setting.width = CAP_WIDTH;
		    video_setting.height = CAP_HEIGHT_NTSC;
		    video_setting.decode_width = CAP_WIDTH;
		    video_setting.decode_height = CAP_HEIGHT_NTSC;
			video_setting.framerate = 30;
	   }
	}

	video_net_setting.qmax = video_setting.qmax;
	video_net_setting.qmin = video_setting.qmin;
	video_net_setting.quant = video_setting.quant;
	//if( size_mode == TD_DRV_VIDEO_SIZE_CIF )
	video_net_setting.bit_rate = 512;
	//else
	//	video_net_setting.bit_rate = 1500;
	video_net_setting.framerate = 16;
	video_net_setting.frame_rate_base = 1;
	video_net_setting.gop_size = 10;
	video_net_setting.width = video_setting.width;
	video_net_setting.height = video_setting.height;
	video_net_setting.decode_width = video_setting.decode_width;
	video_net_setting.decode_height = video_setting.decode_height;

	
	printf(" this is %d channel machine!!!============ %s\n",get_dvr_max_chan(1),__DATE__);


	signal(SIGPIPE,SIG_IGN);

       CaptureAllSignal();

	if( size_mode == TD_DRV_VIDEO_SIZE_CIF )
		set_rec_type(TYPE_CIF);
	else
		set_rec_type(TYPE_D1);

	DPRINTK(" open Hisi system\n");

	Hisi_destroy_dev();

	DPRINTK("standard = %d\n",standard);

	if( standard == TD_DRV_VIDEO_STARDARD_PAL )
		video_in_mode = TD_DRV_VIDEO_STARDARD_PAL;
	else
		video_in_mode = TD_DRV_VIDEO_STARDARD_NTSC;

	ret = get_sensor_type();
	if( ret >= 0 )
	{
		g_sensor_type = ret;
		DPRINTK("Get sensor type = %d\n",g_sensor_type);
	}
	
	ret = get_chip_type();
	if( ret >= 0 )
	{
		g_chip_type = ret;
		DPRINTK("Get g_chip_type = %d\n",g_chip_type);
	}




	{	
		int img_w,img_h;
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();	

		 if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_1080P )
		{
			img_w  =1920;
			img_h = 1080;				
		}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_720P )
		{
			img_w  =1280;
			img_h = 720;				
		}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_480P )
		{
			img_w =640;
			img_h = 480;				
		}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_960P )
		{
			img_w =1280;
			img_h = 960;			
		}
		else
		{
			img_w =320;
			img_h = 240;					
		}

		init_vbr_quality();
		

		
		if( pAllInfo->onvif_st.vcode[0].BitrateLimit == 999999)
		{
			VBR_QUALITY_ST * pEnc = NULL;

			DPRINTK("Ipcam encode use default value\n");			

			for( i = 0; i < MAX_RTSP_NUM  ; i++ )	
			{
				if( i == 0 )
				{
					pAllInfo->onvif_st.vcode[i].w = img_w;
					pAllInfo->onvif_st.vcode[i].h = img_h;
				}

				if( i == 1 )
				{
					if( get_default_sub_enc_resolution() == 1 )
					{
						pAllInfo->onvif_st.vcode[i].w = 640;
						pAllInfo->onvif_st.vcode[i].h = 480;
					}				

					//由于内存没有优化，先都只用cif子码流
					pAllInfo->onvif_st.vcode[i].w = 640;
					pAllInfo->onvif_st.vcode[i].h = 480;
				}

				if( i == 2 )
				{
					
					//由于内存没有优化，先都只用cif子码流
					pAllInfo->onvif_st.vcode[i].w = 320;
					pAllInfo->onvif_st.vcode[i].h = 240;
				}
				pEnc =  get_enc_info(pAllInfo->onvif_st.vcode[i].w,pAllInfo->onvif_st.vcode[i].h);
				if( pEnc != NULL )
				{
					pAllInfo->onvif_st.vcode[i].BitrateLimit = pEnc->default_bitstream;
					pAllInfo->onvif_st.vcode[i].FrameRateLimit = pEnc->default_framerate;
					pAllInfo->onvif_st.vcode[i].GovLength = pEnc->default_gop;
					pAllInfo->onvif_st.vcode[i].H264_profile = pEnc->default_H264_profile;
				}

				DPRINTK("[%d]  (%d,%d) %d,%d,%d,%d\n",i,
					pAllInfo->onvif_st.vcode[i].w,pAllInfo->onvif_st.vcode[i].h,
					pAllInfo->onvif_st.vcode[i].BitrateLimit,pAllInfo->onvif_st.vcode[i].FrameRateLimit,
					pAllInfo->onvif_st.vcode[i].GovLength,pAllInfo->onvif_st.vcode[i].H264_profile);
			}
		}
		
		for( i = 0; i < MAX_RTSP_NUM  ; i++ )	
		{
			code_h264_mode[i] = pAllInfo->onvif_st.vcode[i].h264_mode;
			code_h264_profile[i] = pAllInfo->onvif_st.vcode[i].H264_profile;

			if( code_h264_mode[i] == 1)
			{
				if( i == 0 )
				{
					if ( IsInMtdDir == 1 )
						SendM2("sed -i '20,30s/<h264_bcr>1</<h264_bcr>0</g' /mnt/mtd/video.xml");
					else
						SendM2("sed -i '20,30s/<h264_bcr>1</<h264_bcr>0</g' /usr/local/bin/video.xml");
				}
				else if (i == 1)
				{
					if ( IsInMtdDir == 1 )
						SendM2("sed -i '50,60s/<h264_bcr>1</<h264_bcr>0</g' /mnt/mtd/video.xml");
					else
						SendM2("sed -i '50,60s/<h264_bcr>1</<h264_bcr>0</g' /usr/local/bin/video.xml");
				}
			}
		}		

		//if( g_sensor_type == GOKE_SC1035 )
		{
			if( pAllInfo->onvif_st.vcode[1].w == 320 )
			{
				if ( IsInMtdDir == 1 )
				{
					SendM2("sed -i 's/<width>640</<width>320</g' /mnt/mtd/video.xml");
					SendM2("sed -i 's/<width>720</<width>320</g' /mnt/mtd/video.xml");
					SendM2("sed -i 's/<height>480</<height>240</g' /mnt/mtd/video.xml");
				}else{
					SendM2("sed -i 's/<width>640</<width>320</g' /usr/local/bin/video.xml");
					SendM2("sed -i 's/<width>720</<width>320</g' /usr/local/bin/video.xml");
					SendM2("sed -i 's/<height>480</<height>240</g' /usr/local/bin/video.xml");
				}
			}else
			{
				if ( IsInMtdDir == 1 )
				{
					SendM2("sed -i 's/<width>320</<width>640</g' /mnt/mtd/video.xml");
					SendM2("sed -i 's/<width>720</<width>640</g' /mnt/mtd/video.xml");
					SendM2("sed -i 's/<height>240</<height>480</g' /mnt/mtd/video.xml");
				}else{
					SendM2("sed -i 's/<width>320</<width>640</g' /usr/local/bin/video.xml");
					SendM2("sed -i 's/<width>720</<width>640</g' /usr/local/bin/video.xml");
					SendM2("sed -i 's/<height>240</<height>480</g' /usr/local/bin/video.xml");
				}
			}

			if ( IsInMtdDir == 1 )
				SendM2("sed -i 's/<h264_gop_N>50</<h264_gop_N>25</g' /mnt/mtd/video.xml");
			else
				SendM2("sed -i 's/<h264_gop_N>50</<h264_gop_N>25</g' /usr/local/bin/video.xml");

			if( pAllInfo->sys_st.ipcam_stardard == TD_DRV_VIDEO_STARDARD_NTSC)
			{
				//SendM2("sed -i 's/<vi_framerate>25</<vi_framerate>30</g' /usr/local/bin/video.xml");
			}

			
		}

		{
			GK_MEDIA_ST stMedia;
			int s32Ret;
			memset(&stMedia,0x00,sizeof(GK_MEDIA_ST));			

			stMedia.stream_num = MAX_RTSP_NUM;

			for( i = 0; i < stMedia.stream_num; i++)
			{
				stMedia.stMediaStream[i].code = GK_CODE_264;
				stMedia.stMediaStream[i].ctrl = GK_CODE_CTRL_VBR;
				stMedia.stMediaStream[i].img_width = pAllInfo->onvif_st.vcode[i].w;
				stMedia.stMediaStream[i].img_height= pAllInfo->onvif_st.vcode[i].h;
			}

			command_drv2("mkdir /mnt/mtd/set_config/");


			s32Ret = command_drv2("cat /mnt/mtd/set_config/enc_type_0.txt | grep h265");
			//DPRINTK("cat /mnt/mtd/set_config/enc_type_0.txt | grep h264  =%d\n",s32Ret);	
			if( s32Ret == 0 )
				stMedia.stMediaStream[0].code = GK_CODE_265;
			
			
			s32Ret = command_drv2("cat /mnt/mtd/set_config/enc_type_1.txt | grep h265");
			//DPRINTK("cat /mnt/mtd/set_config/enc_type_1.txt | grep h264  =%d\n",s32Ret);	
			if( s32Ret == 0 )
				stMedia.stMediaStream[1].code = GK_CODE_265;

			s32Ret = command_drv2("cat /mnt/mtd/set_config/enc_type_2.txt | grep h265");
			//DPRINTK("cat /mnt/mtd/set_config/enc_type_1.txt | grep h264  =%d\n",s32Ret);	
			if( s32Ret == 0 )
				stMedia.stMediaStream[2].code = GK_CODE_265;
			g_ipc_media = stMedia;
			
			set_gk_media(stMedia);	
			start_gk_media();	

		}
	}

/*
	if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_1080P )
	{	
		command_drv("echo \"256000\" > /proc/sys/net/core/wmem_default");
		command_drv("echo \"256000\" > /proc/sys/net/core/wmem_max");
	}else
	{
		command_drv("echo \"256000\" > /proc/sys/net/core/wmem_default");
		command_drv("echo \"256000\" > /proc/sys/net/core/wmem_max");
	}
*/
	

	//ret = command_process("/usr/local/bin/ctlserver&");
	//DPRINTK("/usr/local/bin/ctlserver & ret=%d\n",ret);

/*
	if( ret != 0 )
	{
		DPRINTK("not run /usr/local/bin/ctlserver  error!\n");
		exit(0);
	}
*/
	
	//初始化Hisi的硬件
	if( Hisi_init_dev(size_mode,video_in_mode) < 0 )
	{
		printf("Hisi_init_dev error\n");
		goto RELEASE;
	}		

	isp_init_ok = 1;
	
	
	DPRINTK(" init encode!\n");	

	for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )			
	{	

		memset(&enc_d1[i],0x00,sizeof( enc_d1[i]) );
		memset(&gm_enc_st[i],0x00,sizeof( gm_enc_st[i]) );
			
		enc_d1[i].ichannel = i;
		enc_d1[i].mmp_addr = NULL;
		enc_d1[i].encode_fd = 0;
		enc_d1[i].mode = standard;
		enc_d1[i].size = size_mode;
		enc_d1[i].bitstream = video_setting.bit_rate;
		enc_d1[i].framerate = video_setting.framerate;		
		enc_d1[i].gop = video_setting.framerate;
		enc_d1[i].VeGroup = i;
		enc_frame_rate_ctrl[i].last_frame_time.tv_sec = 0;
		enc_frame_rate_ctrl[i].last_frame_time.tv_usec = 0;
		enc_frame_rate_ctrl[i].frame_rate = video_setting.framerate;
		enc_frame_rate_ctrl[i].frame_interval_time = div_safe2(1000000 , enc_frame_rate_ctrl[i].frame_rate);


		if( i == 0 || i == 2)
		{
			enc_d1[i].bitstream = 1024*4;		
		}else
		{
			enc_d1[i].bitstream = 2*1024;		
		}	
	
		gm_enc_st[i].bitrate = enc_d1[i].bitstream ;
		gm_enc_st[i].channel_num = enc_d1[i].ichannel;
		gm_enc_st[i].framerate  = enc_d1[i].framerate;
		gm_enc_st[i].gop = enc_d1[i].gop;
		gm_enc_st[i].video_input_system = video_in_mode;

		if( i == 0 )
		{		

			if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_576P )
			{
				gm_enc_st[i].width =704;
				
				if( video_in_mode == TD_DRV_VIDEO_STARDARD_PAL)
					gm_enc_st[i].height = 576;
				else
					gm_enc_st[i].height = 480;
				
				gm_enc_st[i].max_bitrate = 2*1024;
				
			}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_720P )
			{
				gm_enc_st[i].width =1280;
				gm_enc_st[i].height = 720;
				gm_enc_st[i].max_bitrate = 4*1024;
			}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_480P )
			{
				gm_enc_st[i].width =640;
				gm_enc_st[i].height = 480;
				gm_enc_st[i].max_bitrate = 2*1024;
			}else if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_960P )
			{
				gm_enc_st[i].width =1280;
				gm_enc_st[i].height = 960;
				gm_enc_st[i].max_bitrate = 3*1024;
			}
			else
			{
				gm_enc_st[i].width =1920;
				gm_enc_st[i].height = 1080;	
				gm_enc_st[i].max_bitrate = 6*1024;
			}
			
			gm_enc_st[i].use_vbr = 1;			
		}else
		{
			if( i == 1 )
			{
				gm_enc_st[i].width =640;				
				gm_enc_st[i].height = 480;

				gm_enc_st[i].use_vbr = 1;
				gm_enc_st[i].max_bitrate = 1*1024;

				 {
						IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();
						if( pAllInfo->onvif_st.vcode[1].w == 640 )
						{
							
						}else if( pAllInfo->onvif_st.vcode[1].w == 320 )
						{
							gm_enc_st[i].width =320;				
							gm_enc_st[i].height = 240;
							gm_enc_st[i].use_vbr = 1;
							gm_enc_st[i].max_bitrate = 512;
						}else
						{}
				  }
			}

			if( i == 2 )
			{
				gm_enc_st[i].width =320;
				gm_enc_st[i].height = 240;

				gm_enc_st[i].use_vbr = 1;
				gm_enc_st[i].max_bitrate = 512;
				
			}
		}	

		if( Net_cam_get_support_max_video()  == TD_DRV_VIDEO_SIZE_1080P )
		{
			//gm_enc_st[i].framerate = 24;
		}
		

		enc_d1[i].is_open_encode = 1;		

		if( i == 1 )
		{
			gm_enc_st[i].is_open_motion = 1;			
		}		

		if( i == 2 )
		{
			gm_enc_st[i].is_sub_enc = 1;	
			gm_enc_st[i].use_vbr = 1;			
		}

		if( i == 0 )
		{
			enc_d1[i].size = TD_DRV_VIDEO_SIZE_D1;
		}else
		{
			enc_d1[i].size = TD_DRV_VIDEO_SIZE_CIF;
		}

		if( i == 2 )
			enc_d1[i].size = TD_DRV_VIDEO_SIZE_D1;
		 
		if( Hisi_create_encode(&enc_d1[i]) < 0 )	//创建压缩引擎			
			goto RELEASE;	

	
		
		enc_d1[i].motion_buf = (unsigned short*)malloc(MOTION_BUF_SIZE);
		if(enc_d1[i].motion_buf == NULL )
		{
			DPRINTK("malloc  motion_buf failed!\n");
		       goto RELEASE;
		}
	}		

	{
		open_rstp_server = 1;
		DPRINTK("rstp_server_start ok!\n");
	}

	#ifdef USE_DVR_SERVER_LOGIN
		init_p2p_socket();	//授权& 传统DDNS服务
	#endif


	for( i = 0; i < MN_MAX_BIND_ENC_NUM; i++ )
	{
		Net_cam_start_enc(i);	
		
	}

	sleep(1);

	for( i = MN_MAX_BIND_ENC_NUM - 1; i >= 0; i-- )
	{
		Net_cam_stop_enc(i);
	}	


	
	DPRINTK(" init NM_ini_st!\n");	

	//支持fab协议的网络连接
	for(i = 0; i < MAX_CLIENT; i++)
	{
		if( i == 0 )		
			client_pst[i] = NM_ini_st(BUILD_MODE_ONCE,NET_SEND_DIRECT_MODE,0x100000/8,net_cam_recv_data);			
		else		
			client_pst[i] = NM_ini_st(BUILD_MODE_ONCE,NET_SEND_DIRECT_MODE,0x100000/8,net_cam_recv_data_mult_client);

		if( client_pst[i] == NULL )
		{
			DPRINTK( "create NM_ini_st  error i=%d\n",i);
			goto RELEASE;
		}	
		
	}	
	
	DPRINTK(" init step 1!\n");	
	if( ipcam_get_use_common_protocol_flag() == 1 )
	{
		//fab
		dvr_net_create_nvr_listen(NORMAL_IPCAM_PORT);
	}else
	{
		//老期协议，现在已经不在使用
		dvr_net_create_nvr_listen(NET_CAM_PORT);
	}

	DPRINTK(" init step 2!\n");	

	
	ret = pthread_create(&id_net,NULL,(void*)net_send_boardcast_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_send_boardcast_thread  thread error\n");
		goto RELEASE;
	}

	DPRINTK(" init step 3!\n");	

	//接收NVR或客户端的广播信息，进行IPCAM 网络设置线程。
	ret = pthread_create(&id_net_send,NULL,(void*)net_ip_test_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_ip_test_thread  thread error\n");
		goto RELEASE;
	}	



	DPRINTK(" init step %d!\n",__LINE__);	

	// IPCAM根据网络速度自动调节帧率码流线程
	ret = pthread_create(&id_net_check,NULL,(void*)net_status_check_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_ip_test_thread  thread error\n");
		goto RELEASE;
	}
	

	DPRINTK(" init step %d!\n",__LINE__);

	//程序定时检测杂项
	ret = pthread_create(&id_net_2,NULL,(void*)net_cam_status_check_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_cam_status_check_thread  thread error\n");
		goto RELEASE;
	}	


	all_encode_flag = 1;	


	DPRINTK(" init step %d!\n",__LINE__);


	if(1)
	{
		pthread_t id_buf_op;
		pthread_t id_net_audio_udp;
		pthread_t id_net_audio_udp2;


		g_encode_flag = 1;
		g_decode_flag = 1;


		//录像视频缓存列表，现已不用
		if( init_buf_list(&g_encodeList,1000,1) < 0 )
		{
			trace("init_buf_list error!\n");
			return -1;
		}

		//网络回放缓存列表
		if( init_buf_list(&g_decodeList,MAX_FRAME_BUF_SIZE,1) < 0 )
		{
			trace("init_buf_list error!\n");
			return -1;
		}		

		//录像视频缓存，现用
		if( (cycle_enc_buf = (CYCLE_BUFFER *)init_cycle_buffer(1*1024*1024)) == NULL )
		{
			DPRINTK("init_cycle_buffer error!\n");
			return -1;
		}

		if( init_buf_list(&g_audioList,AUDIO_ENC_DATA_MAX_LENGTH,20) < 0 )
		{
			trace("init_buf_list error!\n");
			return -1;
		}	

		if( init_buf_list(&g_audiodecodeList,AUDIO_ENC_DATA_MAX_LENGTH,5) < 0 )
		{
			trace("init_buf_list error!\n");
			return -1;
		}	

		//cms 网传缓存
		if( (cycle_net_buf = (CYCLE_BUFFER *)init_cycle_buffer(512*1024)) == NULL )
		{
			DPRINTK("init_cycle_buffer error!\n");
			return -1;
		}else
		{
			filo_set_save_num(cycle_net_buf,2);
		}

		//共用视频缓存，
		ret = sll_init_list(&gVideolist[0],60,1024*1024);
		if( ret < 0 )
		{
			DPRINTK("sll_init_list err\n");
			return -1;
		}

		ret = sll_init_list(&gVideolist[1],60,512*1024);
		if( ret < 0 )
		{
			DPRINTK("sll_init_list err\n");
			return -1;
		}

		ret = sll_init_list(&gThirdStreamlist,60,512*1024);
		if( ret < 0 )
		{
			DPRINTK("sll_init_list err\n");
			return -1;
		}
		ret = sll_init_list(&gAudiolist,32,64*1024);
		if( ret < 0 )
		{
			DPRINTK("sll_init_list err\n");
			return -1;
		}

		set_video_buf_array(&gVideolist[0],&gVideolist[1],&gThirdStreamlist);
		set_audio_buf_array(&gAudiolist);

		//set_video_recv_callback(goke_video_data_callback);
		set_audio_recv_callback(goke_audio_data_callback);


		goke_set_md_buf(enc_d1[0].motion_buf);

		if (access("/mnt/mtd/1080p.txt", F_OK) != 0)
		{
			set_rtsp_fps(0,25);
			set_rtsp_fps(1,25);
		}else{//7102+2135
			set_rtsp_fps(0,/*25*/15);
			set_rtsp_fps(1,/*25*/15);
		}
		
		DPRINTK(" init step %d!\n",__LINE__);

		if( standard == TD_DRV_VIDEO_STARDARD_PAL )
			goke_set_antiflicker(1,GADI2_ISP_ANTIFLICKER_FREQ_50HZ);
		else
			goke_set_antiflicker(1,GADI2_ISP_ANTIFLICKER_FREQ_60HZ);
		
	DPRINTK(" init step %d!\n",__LINE__);
		for( i = 0; i < 2; i++)
		{
			//处理和分发视频数据
			ret = pthread_create(&id_encode,NULL,(void*)encode_thread,(void *) &i);
			if ( 0 != ret )
			{
				trace( "create encode_thread thread error\n");
				goto RELEASE;
			}	
			usleep(150000);
		}
		
		
		DPRINTK(" init step %d!\n",__LINE__);

		while( check_goke_is_work() == 0 )
		{
			wait_device_ready_num++;
			DPRINTK("wait goke app run %d\n",wait_device_ready_num);
			usleep(100000);

			if( wait_device_ready_num >= 60 )
			{
				DPRINTK("Device is not ready.Need reboot.\n");
				command_drv("reboot");
				wait_device_ready_num = 0;
			}
			
		}
		/*
		if (access("/mnt/mtd/1080p.txt", F_OK) != 0)
		{
			goke_set_enc(0,25,3000,25);
		}else//7102+2135
			goke_set_enc(0,15,3000,15);*/

		if( init_buf_list(&g_othreinfolist,200,100) < 0 )
		{
			trace("init_buf_list error!\n");
			return -1;
		}			

		 init_all_variable();		
		

	 	 if(	 init_all_custom_buf() < 0 )
	   		goto RELEASE;

		printf(" init audio!\n");	
			printf(" init audio!\n");
		#ifdef USE_AUDIO

		//if( Hisi_get_chip_type() == chip_hi3518e )
		//{
		//}else		
		{
			enable_audio_rec(1);
			
			if( Hisi_init_audio_device() < 0 )
				goto  RELEASE;	
		}
		#endif			


		
		ret = pthread_create(&id_decode,NULL,(void*)decode_thread,NULL);
		if ( 0 != ret )
		{
			trace( "create decode_thread  thread error\n");
			goto RELEASE;
		}

		ret = pthread_create(&id_buf_op,NULL,(void*)encode_buf_op_thread,NULL);
		if ( 0 != ret )
		{
			trace( "create encode_buf_op_thread  thread error\n");
			goto RELEASE;
		}	

		#ifdef USE_AUDIO

		//if( Hisi_get_chip_type() == chip_hi3518e )
		//{
		//}else
		{
			/*
			ret = pthread_create(&id_audio_encode,NULL,(void*)audio_encode_thread,NULL);
			if ( 0 != ret )
			{
				trace( "create decode_thread  thread error\n");
				goto RELEASE;
			}
			

			ret = pthread_create(&id_audio_decode,NULL,(void*)audio_decode_thread,NULL);
			if ( 0 != ret )
			{
				trace( "create decode_thread  thread error\n");
				goto RELEASE;
			}
			printf("create audio thread!\n");
			*/
		}
		#endif

#ifdef NET_USE	
		ret = pthread_create(&id_net,NULL,(void*)create_net_server_thread,NULL);
		if ( 0 != ret )
		{
			trace( "create decode_thread  thread error\n");
			goto RELEASE;
		}

		#ifdef NET_USE_NEW_FUN

		ret = pthread_create(&id_net_send,NULL,(void*)net_send_thread,NULL);
		if ( 0 != ret )
		{
			trace( "create net_send_thread  thread error\n");
			goto RELEASE;
		}
		 printf("create net_send_thread thread!\n");
		 #else
		 DPRINTK("net not create net_send_thread\n");

		 #endif

	/*	 
		 ret = pthread_create(&id_net_tes_connected,NULL,(void*)net_dvr_test_net_connected,NULL);
		if ( 0 != ret )
		{
			trace( "create net_dvr_test_net_connected  thread error\n");
			goto RELEASE;
		}	*/
		

		ret = pthread_create(&id_net_audio_udp,NULL,(void*)net_dvr_audio_udp_th,NULL);
		if ( 0 != ret )
		{
			trace( "create net_dvr_audio_udp_th  thread error\n");
			goto RELEASE;
		}		

		ret = pthread_create(&id_net_audio_udp2,NULL,(void*)net_dvr_audio_udp_th2,NULL);
		if ( 0 != ret )
		{
			trace( "create net_dvr_audio_udp_th2  thread error\n");
			goto RELEASE;
		}		
	
#endif
	}	
	


	ret = pthread_create(&id_net_pc,NULL,(void*)net_pc_send_boardcast_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_send_boardcast_thread  thread error\n");
		goto RELEASE;
	}
/*

	ret = pthread_create(&id_net_upnp,NULL,(void*)upnp_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_send_boardcast_thread  thread error\n");
		goto RELEASE;
	}


	#ifdef USE_YUN_SERVER
	ys_start_server();
	#endif
*/


	start_printf_send_th(28000);
	init_ipcam_all_info();	

	#ifdef USE_ONVIF
	//start_mini_httpd(80);
	
	if( 1 )
	{
		IPCAM_ALL_INFO_ST * pAllInfo =  get_ipcam_config_st();

		pAllInfo->sys_st.onvif_is_open = 1;

		if( pAllInfo->sys_st.onvif_is_open == 1)
		{
			ret = pthread_create(&id_onvif_check,NULL,(void*)net_onvif_rtsp_server_check_thread,NULL);
			if ( 0 != ret )
			{
				trace( "create net_cam_status_check_thread  thread error\n");
				goto RELEASE;
			}	
		}		
	}

	#endif

	


#ifdef USE_RTSP_SERVER

	if(1)
	{

		struct AudioInfo audioInfo1 = {1, 1, 8000, 16, 64};

		RJone_RTSPServer_Init();

		DPRINTK("add audio (%d,%d,%d,%d,%d) to rtsp\n",audioInfo1.audioCodec,audioInfo1.channelNumber,audioInfo1.sampleRate,audioInfo1.bitPerSample,audioInfo1.bitrate);
		ret = RJone_RTSPServer_AddStream(RJONE_RTSP_CHN0_URL,&gVideolist[0],2,&gAudiolist,audioInfo1); //开音频和视频
		//ret = RJone_RTSPServer_AddStream(RJONE_RTSP_CHN0_URL,&gVideolist[0],0,&gAudiolist,audioInfo1);//只开视频
		if( ret < 0 )
		{
			DPRINTK("RJone_RTSPServer_AddStream  err=%d\n",ret);
			
		}
		
		
		ret = RJone_RTSPServer_AddStream(RJONE_RTSP_CHN1_URL,&gVideolist[1],0,&gAudiolist,audioInfo1);
		if( ret < 0 )
		{
			DPRINTK("RJone_RTSPServer_AddStream  err=%d\n",ret);		
		}


		ret = RJone_RTSPServer_AddStream(RJONE_RTSP_CHN2_URL,&gThirdStreamlist,0,&gAudiolist,audioInfo1);
		if( ret < 0 )
		{
			DPRINTK("RJone_RTSPServer_AddStream  err=%d\n",ret);		
		}
		
	printf( "RJone_RTSPServer_Startup!\n");
		RJone_RTSPServer_Startup(554);
		
	printf( "RJone_RTSPServer_Startup over!\n");
	}
	
#endif

	printf( "all  thread OK!\n");

	device_allready_init = 1;	

	return 1;


RELEASE:
	close_all_device();

	return -1;
	
}


void drv_ctrl_lib_function(GST_DEV_FILE_PARAM  * gst)
{
	
	gst->GST_DRA_start_stop_recording_flag = start_stop_record;
	gst->GST_DRA_local_instant_i_frame = instant_keyframe;
	gst->GST_CMB_GetSaveFileEncodeData = get_save_buf_info;
	gst->GST_DRV_CTRL_StartStopPlayBack = start_stop_play_back;
	gst->GST_set_mult_play_cam = set_mult_play_cam;
	gst->GST_DRV_CTRL_EnableInsertPlayBuf = enable_insert_play_buf;
	gst->GST_DRV_CTRL_InsertPlayBuf = insert_play_buf;
	gst->GST_DRV_CTRL_InitAllDevice = init_all_device;
	gst->GST_DRV_CTRL_StopAllDevice = close_all_device;
	gst->GST_DRA_show_cif_to_d1_size = show_cif_to_d1_size;
	gst->GST_DRA_change_device_set  = change_device_set;	
	gst->GST_DRA_show_cif_img_to_middle = show_cif_img_to_middle;
	gst->GST_DRA_enable_local_audio_play = enable_local_audio_play;
	gst->GST_NET_set_net_parameter = set_net_parameter;
	gst->GST_NET_kick_client = kick_client;
	gst->GST_NET_get_client_info = get_client_info;
	gst->GST_NET_is_have_client = is_have_client;
	gst->GST_NET_get_admin_info = get_admin_info;
	gst->GST_OSD_clear_osd = clear_osd;
	gst->GST_OSD_osd_line = osd_line;
	gst->GST_OSD_osd_24bit_photo = osd_24bit_photo;
	gst->GST_OSD_load_yuv_pic = load_yuv_pic;
	gst->GST_OSD_osd_2bit_photo = osd_2bit_photo;
	gst->GST_DRA_get_keyboard_led_info = get_keyboard_led_info;
	gst->GST_NET_get_client_user_num = get_client_user_num;
	gst->GST_NET_get_client_info_by_id = get_client_info_by_id;
	gst->GST_DRA_set_rec_type =  set_rec_type;
	gst->GST_DRA_set_delay_sec_time = set_delay_sec_time;
	gst->GST_DRA_get_delay_sec_time = get_delay_sec_time;
	gst->GST_DRA_get_sys_time = get_sys_time;
	gst->GST_NET_start_mini_httpd = start_mini_httpd;
	gst->GST_OSD_set_background_pic = set_background_pic;
	gst->GST_NET_try_dhcp = try_dhcp;
	gst->GST_NET_is_conneted_svr = is_conneted_svr;
	gst->GST_NET_get_svr_mac_id = get_svr_mac_id;
	gst->GST_NET_net_ddns_open_close_flag = net_ddns_open_close_flag;
	gst->GST_DRA_Hisi_window_vo = Hisi_window_vo;
	gst->GST_DRA_Hisi_playback_window_vo = Hisi_playback_window_vo;
	gst->GST_DRA_drv_start_motiondetect = drv_start_motiondetect;
	gst->GST_DRA_drv_stop_motiondetect = drv_stop_motiondetect;
	gst->GST_DRA_get_motion_buf_hisi = get_motion_buf_hisi;
	gst->GST_DRA_Hisi_set_vo_framerate = Hisi_set_vo_framerate;
	gst->GST_DRA_get_dvr_version = get_dvr_version;
	gst->GST_DRA_get_dvr_max_chan = get_dvr_max_chan;
	gst->GST_DRA_set_dvr_max_chan = set_dvr_max_chan;
	gst->GST_DRA_Hisi_set_chip_slave = Hisi_set_chip_slave;
	gst->GST_DRA_Net_dvr_send_self_data = Net_dvr_send_self_data;
	gst->GST_DRA_Net_dvr_have_client = Net_cam_have_client_check_all_socket;
	gst->GST_DRA_Net_dvr_Hisi_set_venc_vbr = Hisi_set_venc_vbr;
	gst->GST_DRA_Get_motion_alarm_buf = Get_motion_alarm_buf;
	gst->GST_DRA_Net_cam_set_support_max_video = Net_cam_set_support_max_video;
	gst->GST_DRA_Net_cam_get_support_max_video = Net_cam_get_support_max_video;
	gst->GST_DRA_Net_cam_mtd_store = Net_cam_mtd_store;
	gst->GST_DRA_Net_dvr_get_send_frame_num = Net_dvr_get_send_frame_num;
	gst->GST_DRA_Clear_UPnP2 = Clear_UPnP;
	gst->GST_DRA_set_UPnP2 = No_Use_Set_UPnP;
	gst->GST_DRA_Net_dvr_Net_send_self_data = Net_send_self_data;
	gst->GST_DRA_Hisi_set_spi_antiflickerattr = Hisi_set_spi_antiflickerattr;
	gst->GST_DRA_Hisi_get_ipcam_config_st = get_ipcam_config_st;
	gst->GST_DRA_Hisi_save_ipcam_config_st = save_ipcam_config_st;
	gst->GST_DRA_Hisi_set_isp_value = Hisi_set_isp_value;
	gst->GST_DRA_Hisi_ipcam_fast_reboot = ipcam_fast_reboot;
	gst->GSt_NET_set_ipcam_mode = set_ipcam_mode;
	gst->GSt_NET_get_isp_light_dark_value = Hisi_get_isp_light_dark_value;
	gst->GST_NET_open_yun_server = open_yun_server;	
	
	DPRINTK("filesystem version:%s time:%s\n",get_dvr_version(),__DATE__);

	g_pstcommand = gst;
}



