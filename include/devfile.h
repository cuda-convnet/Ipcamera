
#ifndef _DEV_FILE_H_
#define _DEV_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ipcam_config.h"
#include <linux/prctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CAM_GROUP (3)


#define  ERROR                		-1
#define  NODISKERROR    		-2
#define  DISKTYPEERROR 		-3
#define  DISABLEMAKEDIR 	-4
#define  MOUNTERROR    		-5
#define  NOENOUGHMEMORY 	-6
#define  FILEERROR         		-7          // 文件出错，通常是不能读写文件
#define  FILESEARCHERROR      -8
#define  FIXERROR			-9
#define  BEYONDTHEMARK		-10
#define  FILESTATEERROR        -11
#define  ALLDISKUSEOVER        -12
#define  FILEFULL			-13
#define  PLAYOVER			-14      
#define   PREVIEWWRITEOVER  -15
#define  LOGERROR			-16
#define  ALLDISKNOTENABLETOWRITE -17  //没有硬盘可以用来录相
#define  CANTFINDTIMEPOINT   -18
#define  DISKNOENOUGHCAPACITY -19
#define  GETRECORDFILEUNKNOWERROR -20 // 获得录相文件时异常出错
#define  USBNOENOUGHSPACE    -21         // U盘没有足够的空间
#define  CANTFINDFILE             -23       //回放时找不到文件
#define  READBADFRAME           -24  
#define  NOUSBDEVICE		-26        // 没有USB 设备
#define  RECTESTERROR            -27       // 检查录相时数据是否写硬盘
#define  READNOTHING               3
#define  AUDIO_ERROR		-28
#define  FILE_SIZE_ZERO_ERROR		-29
#define  CREATE_FILE_ERROR   -30
#define  ALLRIGHT          		100



#define TYPE_QUAD 0
#define TYPE_D1 1
#define TYPE_CIF 2 
#define YTPE_HD1 3

#define HD_MODE_VGA_1440x900_60 0
#define HD_MODE_VGA_1280x1024_60 1
#define HD_MODE_HDMI_720P_60 2
#define HD_MODE_HDMI_1080I_50 3
#define HD_MODE_HDMI_1080I_60 4
#define HD_MODE_HDMI_1080P_25 5
#define HD_MODE_HDMI_1080P_30 6
#define HD_MODE_YPBPR_720P_60 7
#define HD_MODE_YPBPR_1080I_50 8
#define HD_MODE_YPBPR_1080I_60 9
#define HD_MODE_YPBPR_1080P_25 (10)
#define HD_MODE_YPBPR_1080P_30 (11)
#define HD_MODE_VGA_800x600_60 (12)
#define HD_MODE_HDMI_1080P_50  (13)
#define HD_MODE_HDMI_1080P_60  (14)
#define HD_MODE_YPBPR_1080P_50 (15)
#define HD_MODE_YPBPR_1080P_60 (16)
#define HD_MODE_HDMI_720P_50 (17)
#define HD_MODE_YPBPR_720P_50 (18)
#define HD_MODE_VGA_1024x768_60 (19)
#define HD_MODE_3840x2160_30  (20)
#define HD_MODE_3840x2160_60  (21)


// 一些操作的返回数值
#define PLAYNORMAL       0
#define PLAYFASTLOWSPEED 1
#define PLAYFASTHIGHSPEED 2
#define PLAYBACKFASTLOWSPEED 3
#define PLAYBACKFASTHIGHSPEED 4
#define PLAYPAUSE 			5
#define PLAYLATERFRAME       6
#define PLAYLASTFRAME         7
#define PLAYFASTHIGHSPEED_2 8
#define PLAYFASTHIGHSPEED_4 9
#define PLAYFASTLOW2SPEED  10
#define PLAYFASTHIGHSPEED_16 11
#define PLAYFASTHIGHSPEED_32 12
#define PLAYBACKFASTHIGHSPEED_2 13
#define PLAYBACKFASTHIGHSPEED_4 14
#define PLAYBACKFASTHIGHSPEED_8 15
#define PLAYBACKFASTHIGHSPEED_16 16
#define PLAYBACKFASTHIGHSPEED_32 17
#define PLAYFASTHIGHSPEED_8 18

#define NET_DVR_NET_COMMAND (1001)
#define NET_DVR_NET_FRAME (1002)
#define NET_DVR_NET_SELF_DATA (1003)

#define NET_DVR_AUDIO_MODE_NET (1004)
#define NET_DVR_AUDIO_MODE_LOCAL (1005)
#define NET_DVR_AUDIO_MODE_PLAYBACK (1006)

#define NET_DVR_BITRATE_MODE_DOUBLE (1007)
#define NET_DVR_BITRATE_MODE_SINGLE (1008)

#define IPCAM_IP_MODE  (1004)
#define IPCAM_ID_MODE  (1005)


#define NET_DVR_CHAN (0)
#define PLAYFILEOVER              -25   // 回放文件结束

// 暂时用不上
#define PLAYNEXTFILE            48
#define PLAYLASTFILE            49


//播放模式
#define RECORDPLAY               0
#define TIMEPOINTPLAY         1
#define  ALARMRECORDPLAY   2

//USB 状态
#define USBBACKUPSTOP        0
#define USBBACKUPING          1
#define USBSPACENOTENOUGH 2
#define USBWRITEERROR       3
#define NOUSBDEV		    4


#define GDE_DRV_MAX_BUF_COUNT 16
#define GDE_MAXPARTITION	30
#define GDE_NET_USER_NAME_LEN 12

#define MENU_GUI_MSG_MAX_NUM (1024*100)


#define OLD_MAX_FRAME_BUF_SIZE (256*1024)
#define MAX_FRAME_BUF_SIZE (512*1024)

#define MAX_VIDEO_STREAM_NUM 2



#define ISP_EXPOSURE_MODE_NORMAL  	(0)
#define ISP_EXPOSURE_MODE_LIMIT_TIME  	(1)

#define NEW_NET_EDIT

#define NET_KEY_FUN
#ifdef NET_KEY_FUN
	enum NET_KEY_VAL
	{
		NET_KEY_VAL_ESC=1,
		NET_KEY_VAL_REC,
	};
#endif 

#define NETCONFIGFUN
#ifdef NETCONFIGFUN
typedef struct
{
	int Brightness[16];
	int Hue[16];
	int Contrast[16];
	int Satutrat[16];
	int PicHdel[16];
	int PicVdel[16];
	int SpotOutputTime[16];
}CAM_PARA;
typedef struct
{
	int BaudRate[16];
	int DeviceID[16];
	int Protocol[16];
}PTZ_PARA;
typedef struct
{
	int PicQty[16];
	int FrmRate[16];
	int Resolution;
}REC_PARA;

typedef struct
{
	CAM_PARA cam_set;
	PTZ_PARA  ptz_set;
	REC_PARA rec_set;
}NET_MAC_SET_PARA;
#endif


enum chip_type
{
	chip_unkown = 0,
	chip_hi3518e,
	chip_hi3519v101,
};




typedef struct _GST_DRV_BUF_INFO 
{
	int iImageCount;
	
	int iDrvIndex;
	int iBufIdx;
	int iBufDataType;
	int iChannelMask;
	
	int iImageWidth;
	int iImageHeight;
	int iImageStandard;
	int iImageSize;

	int iBufLen[GDE_DRV_MAX_BUF_COUNT];
	int iFrameType[GDE_DRV_MAX_BUF_COUNT];
	char* pBuf[GDE_DRV_MAX_BUF_COUNT];
	struct timeval tv;
	int iCurFrameIsOutput;

	int iFrameCountNumber[GDE_DRV_MAX_BUF_COUNT];
}GST_DRV_BUF_INFO;


typedef struct _GST_DISKINFO
{	
	char devname[40];                   /*  HardDisk name ( /dev/hda )  */	
	int partitionNum;				/* the number of partitions */	
	int partitionID[GDE_MAXPARTITION]; /* the id of partition ( /dev/hda2  2 is id ) */	
	int disktype[GDE_MAXPARTITION]; /* FAT32 FAT16 NTFS  */	
	long long partitionSize[GDE_MAXPARTITION];  /* Partition Size ( K bytes) */	
	long long size;				/* the size of disk ( bytes )*/	
	unsigned int  cylinder;	                    /* the cylinder of the disk*//*now use for disk video format   mpeg4=MPEG4_CODE h264=H264_CODE*/
}GST_DISKINFO;

typedef struct _GST_PLAYBACK_VIDEO_PARAM
{
	int iPbVideoStandard;
	int iPbImageSize;
}GST_PLAYBACK_VIDEO_PARAM;

typedef struct AVFrame 
{
    unsigned char *data[4];
} AVFrame;

typedef struct video_profile
{
    unsigned int bit_rate;
    unsigned int width;   //length per dma buffer
    unsigned int height;
    unsigned int framerate;
    unsigned int frame_rate_base;
    unsigned int gop_size;
    unsigned int qmax;
    unsigned int qmin;   
    unsigned int quant;
    unsigned int intra;
    unsigned int decode_width;
    unsigned int decode_height;
    AVFrame *coded_frame;
    char *priv;
} video_profile;



typedef struct _GST_EVENT
{
	char motion;
	char loss;
	char sensor;	
}GST_RECORDEVENT;

typedef enum _EVENT_STATE
{
	POWER_ON = 0,
	POWER_DOWN1,
	POWER_DOWN2,
	POWER_DOWN3,
	POWER_DOWN4,
	POWER_DOWN5,
	POWER_DOWN6,
	ALARM,
	REC_START_BYHAND,
	REC_STOP_BYHAND,
	REC_START_ALARM,
	REC_STOP_ALARM,
	REC_START_TIMESET,
	REC_STOP_TIMESET,
	NET_CONNECT,
	NET_DISCONNECT,
	UNEXPECTED_SHUTDONW,
	RESTORE_POWER_ON,
	NORMAL,
	REC_START_BYHAND_ERROR,
	REC_START_ALARM_ERROR,
	REC_START_TIMESET_ERROR
}EVENTSTATE;


typedef struct _GST_FILESHOWINFO
{
	int iDiskId;  
	int iPartitionId;
	int iFilePos;
	int iCam;  
	int iSeleted;     // 1 表示选定
	GST_RECORDEVENT event;           // 录相原因
	time_t fileStartTime;
	time_t fileEndTime;
	int   iPreviewTime;            //警前时间 
	int   iLaterTime;			//警后录相时间
	int  states;			   //  录像状态, RECORDING 表示丰在录像，NORECORD 表示没有录像
	int	videoStandard;     // PAL,NTSC
	int	imageSize;		// CIF , HD1 , D1
	int  iPlayCam;             // 播放的通道
	EVENTSTATE  CurrentEvent;	   //现在记录的事件,类型为(EVENTSTATE);
	char reason[30];           // 事件记录日志附加信息
}GST_FILESHOWINFO;


#define MAX_PLAY_FILE_ARRAY_ITEM_NUM (5)

typedef struct _GST_PLAY_FILE_ARRAY_
{
	int iPlayFileNum;
	GST_FILESHOWINFO  file[MAX_PLAY_FILE_ARRAY_ITEM_NUM];
}GST_PLAY_FILE_ARRAY;


typedef struct _GST_TIME_TM
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
}GST_TIME_TM;

typedef struct _KEYBOARD_LED_INFO_
{
	char led1;
	char led2;
	int key_code;
	unsigned char cam_no;
	int key_style; 
}KEYBOARD_LED_INFO;

typedef struct _GST_DEV_FILE_PARAM {
   

	//filesystem
	int (*GST_FS_PartitionDisk)(int iDiskId);
	int (*GST_FS_BuildFileSystem)(int iDiskId);
	int (*GST_FS_CheckNewDisk)(int iDiskId);
	GST_DISKINFO* (*GST_FS_GetDiskInfoPoint)(int disk_id);
	int (*GST_FS_WriteDataToUsb)(time_t startTime,time_t endTime, int iCam);
	int (*GST_FS_WriteInfoToAlarmLog)(GST_RECORDEVENT event,time_t eventStartTime,time_t eventEndTime,int iCam,
									int   iPreviewTime,int   iLaterTime,int  states,int  iMessageId);
	int (*GST_FS_MountAllPatition)(int iDiskId);
	int (*GST_FS_UMountAllPatition)(int iDiskId);
	int (*GST_FS_PhysicsFixAllDisk)();
	int (*GST_FS_EnableDiskReCover)(int iFlag);
	int (*GST_FS_ClearDiskLogMessage)(int iDiskId);
	int (*GST_FS_StartCreateFile)();
	int (*GST_FS_StopCreateFile)();
	int (*GST_FS_WriteUsb)(time_t startTime,time_t  endTime,int iCam,int realWrite);
	int (*GST_FS_UsbWriteStatus)();
	int (*GST_FS_get_disk_remain_rate)(int disk_id);
	int (*GST_FS_get_cover_rate)(int disk_id);
	int (*GST_FS_GetBadBlockSize)(int iDiskId);
	int (*GST_FS_GetErrorMessage)(int * iError, time_t * sTime);
	int (*GST_FS_UpdateSoft)();	
	int (*GST_FS_ListWriteToUsb)(GST_FILESHOWINFO * stPlayFile,int realWrite);

	long long  (*GST_FS_GetDataSize)(time_t *TimeStart,time_t *TimeEnd,int backup_cam,int cd_mode);
	int (*GST_FS_WriteDataToCdrom)(time_t timeStart_1,time_t timeEnd_1,int cam1,
								time_t timeStart_2,time_t timeEnd_2,int cam2,
								time_t timeStart_3,time_t timeEnd_3,int cam3,int iDevice);
	int (*GST_FS_WriteCdStatus)();
	int (*GST_FS_scan_cdrom)();
	int (*GST_FS_WriteInfoToEventLog)(int * iCam,time_t * eventStartTime,GST_RECORDEVENT * pEvent,
									EVENTSTATE EventStates,char * Reason );

	int (*GST_FS_FormatUsb)();
	int (*GST_FS_CheckUsb)(int * have_usb,int * format_is_right);
	
	//filethreadctrl
	time_t (*GST_FTC_CustomTimeTotime_t)(int year,int month, int day, int hour, int minute,int second);
	int  (*GST_FTC_GetWeekDay)(time_t mTime);
	int  (*GST_FTC_GetRecordlist)( GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
	int (*GST_FTC_SetPlayFileInfo)(GST_FILESHOWINFO * stPlayFileInfo);
	void (*GST_FTC_time_tToCustomTime)(time_t time,int * year, int * month,int* day,int* hour,int* minute,int* second );
	int (*GST_FTC_StopPlay)();
	int (*GST_FTC_StartRec)(int iCam,int iPreSecond);
	int (*GST_FTC_StopRec)();
	int (*GST_FTC_PreviewRecordStart)(int iSecond);
	int  (*GST_FTC_GetAlarmEventlist)( GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
	int (*GST_FTC_SetTimeToPlay)(time_t startPlayTime,time_t endPlayTime, int iCam,int playStyle);
	int (*GST_FTC_PrintMemoryInfo)();
	int (*GST_FTC_AudioRecCam)(int iCam);
	int (*GST_FTC_AudioPlayCam)(int iCam);
	void (*GST_FTC_set_count_create_keyframe)(int count);
	int (*GST_FTC_CheckPreviewStatus)();
	int (*GST_FTC_PreviewRecordStop)(int iIsRecord);
	int (*GST_FTC_PlayFast)(int iSpeed);
	int (*GST_FTC_PlayBack)(int iSpeed);
	int (*GST_FTC_PlayPause)(int iStop);
	int(*GST_FTC_GetRecMode)(int iVideoStandard,int iVideoSize);
	int (*GST_FTC_CurrentPlayMode)(int * iVideoStandard,int * iVideoSize);
	int (*GST_FTC_PlaySingleFrame)(int iFlag);
	int (*GST_FTC_PlayChangFile)(int iFlag);
	int (*GST_FTC_PlayUsbWrite)(int iFlag);
	int (*GST_FTC_ChangeRecordToPlay)(int iNextOrLast);
	int (*GST_FTC_PlayUsbWriteStatus)();
	int (*GST_FTC_CurrentDiskWriteAndReadStatus)();
	int (*GST_FTC_PlayNearRecFile)(time_t * CurrentPlayTime);
	int (*GST_FTC_GetPlayCurrentStauts)(int * status, int *iPlayStopFlag);
	int (*GST_FTC_GetCurrentPlayInfo)(time_t * startTime,int * iCam);
	int (*GST_FTC_AlarmlistPlay)(GST_FILESHOWINFO * stPlayFile);
	int (*GST_FTC_ReclistPlay)(GST_FILESHOWINFO * stPlayFile);
	int (*GST_FTC_SetRecDuringTime)(int iSecond);
	int (*GST_FTC_GetEventlist)(GST_FILESHOWINFO * stFileShowInfo, int iCam, 
						time_t searchStartTime, time_t searchEndTime,int iPage);
	int (*GST_FTC_get_play_status)();
	int (*GST_FTC_get_play_time)(time_t * play_time);
	int (*GST_FTC_get_disk_rec_start_end_time)(time_t * start_time,time_t * end_time,int disk_id);
	int (*GST_FTC_get_cur_play_cam)();
	int (*GST_FTC_StopSavePlay)();
	int (*GST_FTC_get_rec_cur_status)();
	

	//disk format
	void (*GST_DISK_GetAllDiskInfo)();
	int (*GST_DISK_GetDiskNum)();

	// drv control
	int (*GST_DRV_CTRL_InitAllDevice)(video_profile * seting,int standard,int size_mode);
	void  (*GST_DRV_CTRL_StopAllDevice)();
	int (*GST_DRV_CTRL_StartStopPlayBack)(int iFlag);
	int (*GST_DRV_CTRL_EnableInsertPlayBuf)();	
	int (*GST_DRV_CTRL_InsertPlayBuf)(char *header,char * pData);
	GST_DRV_BUF_INFO* (*GST_CMB_GetSaveFileEncodeData)(char * pData);
	int (*GST_DRA_start_stop_recording_flag)(int iStartFlag );
	int (*GST_DRA_local_instant_i_frame)(int ichlmask );
	int (*GST_DRA_show_cif_to_d1_size)(int flag);
	int (*GST_DRA_change_device_set)(video_profile * seting,int standard,int size_mode);
	int (*GST_DRA_show_cif_img_to_middle)(int flag);
	int (*GST_DRA_enable_local_audio_play)(int flag,int cam);
	KEYBOARD_LED_INFO * (* GST_DRA_get_keyboard_led_info)();
	int (*GST_DRA_set_rec_type)(int type);
	int (*GST_DRA_Hisi_window_vo)(int window,int start_cam,int x,int y,int width1,int height1);
	int (*GST_DRA_Hisi_playback_window_vo)(int window,int start_cam);	
	int (*GST_DRA_drv_start_motiondetect)();
	int (*GST_DRA_drv_stop_motiondetect)();	
	unsigned short *  (*GST_DRA_get_motion_buf_hisi)(int chan);
	int (*GST_DRA_Hisi_set_vo_framerate)(float speed);
	char * (*GST_DRA_get_dvr_version)();
	int (*GST_DRA_get_dvr_max_chan)(int use_chip_flag);
	void (*GST_DRA_set_dvr_max_chan)(int chan);
	void (*GST_DRA_Hisi_set_chip_slave)();
	int (*GST_DRA_Net_dvr_send_self_data)(char * buf,int length);
	int (*GST_DRA_Net_dvr_recv_self_data)(char * buf,int length);
	int (*GST_DRA_Net_dvr_have_client)();
	void (*GST_DRA_Net_dvr_Hisi_set_venc_vbr)();
	int (*GST_DRA_Get_motion_alarm_buf)(unsigned char * buf,int * mb_width,int *mb_height);
	int (*GST_DRA_Net_cam_set_support_max_video)(int videotype);
	int (*GST_DRA_Net_cam_get_support_max_video)();
	int (*GST_DRA_Net_cam_mtd_store)();
	int (*GST_DRA_Net_dvr_get_send_frame_num)();
	int (*GST_DRA_Clear_UPnP2)(int port);
	int (*GST_DRA_set_UPnP2)(char * ip,int port);
	int (*GST_DRA_Net_dvr_Net_send_self_data)(char * buf,int buf_length);
	int  (*GST_DRA_Hisi_set_spi_antiflickerattr)(int gs_enNorm,int antiflicker_mode);
	IPCAM_ALL_INFO_ST *  (*GST_DRA_Hisi_get_ipcam_config_st)();
	int  (*GST_DRA_Hisi_save_ipcam_config_st)(IPCAM_ALL_INFO_ST * pIpcam_st);
	int  (*GST_DRA_Hisi_set_isp_value)(IPCAM_ALL_INFO_ST * pIpcamInfo,int cmd);
	void (*GST_DRA_Hisi_ipcam_fast_reboot)();

	
	//Buffer Manage
	GST_DRV_BUF_INFO* (*GST_CMB_GetEmptyBuffer)();
	void (*GST_CMB_RecycleBuffer)(GST_DRV_BUF_INFO* pBuf);

	int (*GST_CMB_InsertNetBuffer)(GST_DRV_BUF_INFO* pBuf);
	GST_DRV_BUF_INFO* (*GST_CMB_GetNetEncodeData)();

	int (*GST_CMB_InsertSaveFileBuffer)(GST_DRV_BUF_INFO* pBuf);
	

	GST_DRV_BUF_INFO* (*GST_CMB_get_audio_to_encode_buf)();
	int (*GST_CMB_release_audio_encode_buf)(GST_DRV_BUF_INFO* pAudioBuf);	

	int (*GST_DRA_net_set_time_per_frame)(int iCount);
	
	int (*GST_DRA_ReleaseNetBufToDm642Dev)(GST_DRV_BUF_INFO* pBufInfo);
	int (*GST_DRA_release_encode_device_buffer)(GST_DRV_BUF_INFO* pBufInfo);
	
	int (*GST_DRA_start_playback)(GST_PLAYBACK_VIDEO_PARAM* pPBMode);
	int (*GST_DRA_stop_playback)();
	void (*GST_set_mult_play_cam)(int cam);

	int (*GST_DRA_get_buffer_of_playback)(GST_DRV_BUF_INFO* pBufInfo);
	int (*GST_DRA_send_buffer_of_playback)(GST_DRV_BUF_INFO* pBufInfo);
	int (*GST_DRA_release_buffer_of_playback)(GST_DRV_BUF_INFO* pBufInfo);	
	
	void  (*GST_DRA_convert_time_to_date)( unsigned long ulSec, GST_TIME_TM *pParam );
	int (*GST_DRA_set_encode_time_osd_order)(int iOrder );
	int (*GST_DRA_set_delay_sec_time)(int sec);
	int (*GST_DRA_get_delay_sec_time)();
	int (*GST_DRA_get_sys_time)(struct timeval *tv,char *p);

	//net
	int (*GST_NET_set_net_parameter)(char * ip, int port, char * net_gate,char * netmask,char * dns1,char *dns2);
	int (*GST_NET_kick_client)();
	int (*GST_NET_get_client_info)(char * client_ip);
	int (*GST_NET_is_have_client)();
	int (*GST_NET_get_admin_info)(char * name1,char * pass1,char * name2, char * pass2,
					char * name3,char * pass3,char * name4,char * pass4);
	int (*GST_NET_get_client_user_num)();
	int (*GST_NET_get_client_info_by_id)(char * client_ip, int id);
	int (*GST_NET_start_mini_httpd)(int port);
	int (*GST_NET_try_dhcp)(char * ip,char * netmsk,char * gateway,char * dns1,char * dns2);
	int  (*GST_NET_is_conneted_svr)();
	void  (*GST_NET_get_svr_mac_id)(char * server_ip,char * mac_id );
#ifdef	NEW_NET_EDIT
	void (*GST_NET_net_ddns_open_close_flag)(int flag);
#endif
	int (*GSt_NET_get_ipcam_net_info)(void * buf);
	int (*GSt_NET_set_ipcam_net_info)(void * buf);
	void (*GSt_NET_set_ipcam_mode)(int flag,char * pnet_dev);
	int (*GSt_NET_get_isp_light_dark_value)();
	int (*GST_NET_open_yun_server)(int flag);


	//osd
	int (*GST_OSD_clear_osd)(int color);
	unsigned char *  (*GST_OSD_load_yuv_pic)(char * file_name);
	int (*GST_OSD_osd_24bit_photo)(unsigned char * yuv_buf,int point_x,int point_y,int standard);
	int (*GST_OSD_osd_line)(int start_x, int start_y, int end_x, int end_y,
			char * screen_buf,int screen_width, int screen_height, int color);
	int (*GST_OSD_osd_2bit_photo)(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int word_color,int back_color);
	int (*GST_OSD_set_background_pic)(char * path);

	//menu lib
	int (*GST_MENU_set_current_play_mode)(int iPBStandard, int iPBSize);



	void (*GST_MENU_cam_parameter_get)(CAM_PARA *para);
	void (*GST_MENU_cam_parameter_set)(CAM_PARA *para);
	void (*GST_MENU_ptz_parameter_get)(PTZ_PARA *para);
	void (*GST_MENU_ptz_parameter_set)(PTZ_PARA *para);
	void (*GST_MENU_rec_parameter_get)(REC_PARA *para);
	void (*GST_MENU_rec_parameter_set)(REC_PARA *para);
	void (*GST_MENU_net_key)(int iKeyval);
	void (*GST_MENU_gui_msg_get)(char * msg,int *length);
	void (*GST_MENU_gui_msg_set)(char * msg);

	


}GST_DEV_FILE_PARAM;





void drv_ctrl_lib_function(GST_DEV_FILE_PARAM * gst);


//void trace(char * error);

#define SET_PTHREAD_NAME(name) do{\
    char nameBuf[100];\
    memset(nameBuf, 0, sizeof(nameBuf));\
    if((char *)name == NULL)\
    {\
        sprintf(nameBuf, "%s", __FUNCTION__);\
    }\
    else\
    {\
        memcpy(nameBuf, (char *)name, sizeof(nameBuf));\
    }\
    \
    prctl(PR_SET_NAME, nameBuf);\
}while(0)


#define DEBUG

#ifdef DEBUG
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#define trace DPRINTK  
#else
#define DPRINTK(fmt, args...)
#endif


int net_cfg_load(unsigned char *ipaddress,unsigned char *gateway,unsigned char *netmask,unsigned char *ddns1,unsigned char *ddns2,unsigned char *Wififlag);
int net_cfg_save(unsigned char *ipaddress,unsigned char *gateway,unsigned char *netmask,unsigned char *ddns1,unsigned char *ddns2,unsigned char *Wififlag);



#ifdef __cplusplus
  }
#endif


#endif //_DEV_FILE_H_


