#ifndef _BASE_H_
#define _BASE_H_

#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include <time.h>

#include "../include/devfile.h"
#include "../include/filesystem.h"
#include "MacBaseData.h"




typedef struct _ENCODE_ITEM
{
	int encode_fd;
	int ichannel;    //通道号
	int mode;        // 制式 N /P
	int size;		  // cif  d1 hd1
	int width;
	int height;
	unsigned char * mmp_addr; // 映射地址
//	H264RateControl     ratec;
	int iframe_num;
	int enc_quality;
	int favc_quant;
	int bitstream;
	int framerate;
	int create_key_frame;  // 产生关键帧 
	int is_key_frame;
	int cut_pic;
	int cut_x;
	int cut_y;
	int u32MaxQuant;
	int u32MinQuant;
	int u32IPInterval;
	int enc_frame_num;
	int gop;
	int VeGroup; //视频组
	int is_open_encode;
	int is_open_decode;
	int is_open_motiondetect;
	int is_start_motiondetect;
	int is_net_enc; //是不是网络引擎
	unsigned short * motion_buf;
}ENCODE_ITEM;


enum{
	IPCAM_SET_CAM_COMMAND_BASE = 3000,
	IPCAM_SET_CAM_COMMAND_SEND,
	IPCAM_SET_CAM_COMMAND_SET
};

typedef struct _IPCAM_INFO_TO_PC_
{
	unsigned char net_ip[8];
	unsigned char net_mask[8];
	unsigned char net_gw[8];
	unsigned char net_dns1[8];
	unsigned char net_dns2[8];
	char udp_connect_url[60];
	int server_port;
	int ie_port;
	int cmd;
}IPCAM_INFO_TO_PC;

typedef enum sample_vi_mode_e
{
    APTINA_AR0130_DC_720P_30FPS = 0,
    APTINA_9M034_DC_720P_30FPS,
    SONY_ICX692_DC_720P_30FPS,
    SONY_IMX104_DC_720P_30FPS,
    SONY_IMX138_DC_720P_30FPS,
    SONY_IMX122_DC_1080P_30FPS,
    SONY_IMX290_MIPI_1080P_30FPS,
    OMNI_OV9712_DC_720P_30FPS,
    APTINA_MT9P006_DC_1080P_30FPS,
	OMNI_OV2710_DC_1080P_30FPS,
    SOI_H22_DC_720P_30FPS,
	HIMAX_1375_DC_720P_30FPS,
    SAMPLE_VI_MODE_1_D1,
    APTINA_AR0130_DC_960P_30FPS,
    SONY_IMX138_DC_960P_30FPS,        
    GOKE_SC1035,
     GOKE_JXH42,
     GOKE_SC2135,
     GOKE_SC1145,
     GOKE_SC1135,
     GOKE_OV9750,
     HI3516E_IMX323,
     SONY_IMX274_LVDS_4K_30FPS,
    SONY_IMX290_LVDS_2K_30FPS,
}SAMPLE_VI_MODE_E;



#endif //_BASE_H_


