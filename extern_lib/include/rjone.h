

#ifndef _RJONE_H_
#define _RJONE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rjone_type.h"


#define RJONE_SYS_VER  (0x01000003)

#define RJONE_SUCCESS   (1)
#define RJONE_FAILED      (-1)

#define RJONE_FRAME_SEND_PACKET_LEN (10*1024)


#define RJONE_LISTEN_PORT (11666)



typedef enum {
	IOCTRL_TYPE_UNKN,
	//现场视频音频请求
	IOCTRL_TYPE_LIVE_START_REQ,
	IOCTRL_TYPE_LIVE_START_RESP,

	IOCTRL_TYPE_LIVE_STOP_REQ,
	IOCTRL_TYPE_LIVE_STOP_RESP,
	

	//音频对讲
	IOCTRL_TYPE_LIVE_AUDIO_SPEAK_START_REQ,
	IOCTRL_TYPE_LIVE_AUDIO_SPEAK_START_RESP,

	IOCTRL_TYPE_LIVE_AUDIO_SPEAK_STOP_REQ,
	IOCTRL_TYPE_LIVE_AUDIO_SPEAK_STOP_RESP,


	//无线部分
	IOCTRL_TYPE_GET_AP_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_AP_PARAMETERS_RESP,

	IOCTRL_TYPE_SET_AP_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_AP_PARAMETERS_RESP,

	IOCTRL_TYPE_GET_STA_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_STA_PARAMETERS_RESP,

	IOCTRL_TYPE_SET_STA_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_STA_PARAMETERS_RESP,

	IOCTRL_TYPE_LIST_WIFI_AP_REQ,
	IOCTRL_TYPE_LIST_WIFI_AP_RESP,


	//云台部分
	IOCTRL_TYPE_PTZ_COMMAND_REQ,
	IOCTRL_TYPE_PTZ_COMMAND_RESP,


	//录像文件回放下载部分
	IOCTRL_TYPE_LIST_RECORDFILES_REQ,
	IOCTRL_TYPE_LIST_RECORDFILES_RESP,

	IOCTRL_TYPE_DOWNLOAD_RECORD_FILE_REQ,
	IOCTRL_TYPE_DOWNLOAD_RECORD_FILE_RESP,

	IOCTRL_TYPE_RECORD_PLAYCONTROL_REQ,
	IOCTRL_TYPE_RECORD_PLAYCONTROL_RESP,
	
	//网络控制设备录像
	IOCTRL_TYPE_NET_TRIGGER_RECORD_START_REQ,
	IOCTRL_TYPE_NET_TRIGGER_RECORD_START_RESP,

	IOCTRL_TYPE_NET_TRIGGER_RECORD_CHECK_REQ,
	IOCTRL_TYPE_NET_TRIGGER_RECORD_CHECK_RESP,

	IOCTRL_TYPE_NET_TRIGGER_RECORD_STOP_REQ,
	IOCTRL_TYPE_NET_TRIGGER_RECORD_STOP_RESP,

	//设备推送
	IOCTRL_TYPE_PUSH_EVENT_REQ,
	IOCTRL_TYPE_PUSH_EVENT_RESP,

	IOCTRL_TYPE_GET_PUSH_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_PUSH_PARAMETERS_RESP,
	
	IOCTRL_TYPE_SET_PUSH_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_PUSH_PARAMETERS_RESP,


	//设备参数设置
	IOCTRL_TYPE_GET_SYSFWVER_REQ,	//App to Device
	IOCTRL_TYPE_GET_SYSFWVER_RESP,	//Device to App

	IOCTRL_TYPE_GET_DEV_TIME_REQ,
	IOCTRL_TYPE_GET_DEV_TIME_RESP,
	
	IOCTRL_TYPE_SET_DEV_TIME_REQ,
	IOCTRL_TYPE_SET_DEV_TIME_RESP,

	IOCTRL_TYPE_CHANGE_DEV_PASSWORD_REQ,
	IOCTRL_TYPE_CHANGE_DEV_PASSWORD_RESP,

	IOCTRL_TYPE_GET_DEV_INTERNET_FLAG_REQ,  
 	IOCTRL_TYPE_GET_DEV_INTERNET_FLAG_RESP, 
 	
 	IOCTRL_TYPE_SET_DEV_INTERNET_FLAG_REQ,  
 	IOCTRL_TYPE_SET_DEV_INTERNET_FLAG_RESP,


	// 图像参数设置
	IOCTRL_TYPE_GET_DEV_IMAGE_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_DEV_IMAGE_PARAMETERS_RESP,
	
	IOCTRL_TYPE_SET_DEV_IMAGE_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_DEV_IMAGE_PARAMETERS_RESP,


	// 视频参数设置
	IOCTRL_TYPE_GET_DEV_VIDEO_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_DEV_VIDEO_PARAMETERS_RESP,
	
	IOCTRL_TYPE_SET_DEV_VIDEO_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_DEV_VIDEO_PARAMETERS_RESP,


	//报警设置
	IOCTRL_TYPE_GET_DEVICE_ALARM_REQ,
	IOCTRL_TYPE_GET_DEVICE_ALARM_RESP,
	IOCTRL_TYPE_SET_DEVICE_ALARM_REQ,
	IOCTRL_TYPE_SET_DEVICE_ALARM_RESP,


	//录像参数设置
	IOCTRL_TYPE_GET_RECORD_PARAMETERS_REQ,
	IOCTRL_TYPE_GET_RECORD_PARAMETERS_RESP,
	
	IOCTRL_TYPE_SET_RECORD_PARAMETERS_REQ,
	IOCTRL_TYPE_SET_RECORD_PARAMETERS_RESP,	
	

	//验证密码 &  授权
	IOCTRL_TYPE_AUTHORIZE_REQ, 
	IOCTRL_TYPE_AUTHORIZE_RESP,
	
	//设备升级	
	IOCTRL_TYPE_UPGRADE_REQ,		       //App to Device, data: upgrade file size, 
	IOCTRL_TYPE_UPGRADE_READY,		//Device to App, Device is ready for receiving upgrade firmware. App begin to send data
	IOCTRL_TYPE_UPGRADE_QUERY,		//App to Device, Query device to upgrade firmware ok ? after sending upgrade data over
	IOCTRL_TYPE_UPGRADE_OK,			//Device to App
	IOCTRL_TYPE_UPGRADE_FAILED,		//Device to App


	//控制门锁
	IOCTRL_TYPE_DOORBELL_OPEN_REQ,             
	IOCTRL_TYPE_DOORBELL_OPEN_RESP,      


	// SD卡 TF卡 操作
	IOCTRL_TYPE_SET_DEVICE_SDFORMAT_REQ,
	IOCTRL_TYPE_SET_DEVICE_SDFORMAT_RESP,
	
	IOCTRL_TYPE_SDFORMAT_QUERY_REQ,
	IOCTRL_TYPE_SDFORMAT_QUERY_RESP,
		
	IOCTRL_TYPE_GET_SD_INFO_REQ,
	IOCTRL_TYPE_GET_SD_INFO_RESP,   
	
	//局域网RTSP 操作
	IOCTRL_TYPE_GET_RTSP_INFO_REQ,
	IOCTRL_TYPE_GET_RTSP_INFO_RESP,


	//SUMMER 方案 a115
	//外设云台& 温度 peripheral equipment生词本 
	IOCTRL_TYPE_GET_PERIPHERAL_EQUIPMENT_INFO_REQ,
	IOCTRL_TYPE_GET_PERIPHERAL_EQUIPMENT_INFO_RESP,	

	//设置设备工作频率 50hz  or 60hz
	IOCTRL_TYPE_GET_FREQUENCY_INFO_REQ,
	IOCTRL_TYPE_GET_FREQUENCY_INFO_RESP,

	IOCTRL_TYPE_SET_FREQUENCY_INFO_REQ,
	IOCTRL_TYPE_SET_FREQUENCY_INFO_RESP,
}RJONE_IOCTRL_TYPE;



/******************************************************************************************
								视频音频部分
*******************************************************************************************/


///////////相关数据结构////////

typedef enum
{
	FRAME_TYPE_BASE = 0,
    	FRAME_TYPE_VIDEO,			//视频数据帧
    	FRAME_TYPE_AUDIO,			//音频数据帧
}RJONE_FRAME_TYPE;


typedef enum 
{
	VIDEO_FRAME_FLAG_I	= 0x00,	// Video I Frame
	VIDEO_FRAME_FLAG_P	= 0x01,	// Video P Frame
	VIDEO_FRAME_FLAG_B	= 0x02,	// Video B Frame
}RJONE_VIDEO_FRAME;

typedef enum
{
	ASAMPLE_RATE_8K	= 0x00,
	ASAMPLE_RATE_11K= 0x01,
	ASAMPLE_RATE_12K= 0x02,
	ASAMPLE_RATE_16K= 0x03,
	ASAMPLE_RATE_22K= 0x04,
	ASAMPLE_RATE_24K= 0x05,
	ASAMPLE_RATE_32K= 0x06,
	ASAMPLE_RATE_44K= 0x07,
	ASAMPLE_RATE_48K= 0x08,
}RJONE_AUDIO_SAMPLERATE;


typedef enum
{
	ADATABITS_8	= 0,
	ADATABITS_16	= 1,
}RJONE_AUDIO_DATABITS;

typedef enum
{
	ACHANNEL_MONO	= 0,
	ACHANNEL_STEREO	= 1,
}RJONE_AUDIO_CHANNEL;

typedef enum
{
	CODECID_UNKN = 1,
	CODECID_V_MJPEG,
	CODECID_V_MPEG4,
	CODECID_V_H264,
	
	CODECID_A_PCM ,
	CODECID_A_ADPCM,
	CODECID_A_SPEEX,	
	CODECID_A_AMR,
	CODECID_A_AAC,
	CODECID_A_G711A,
	CODECID_A_G726,         
	CODECID_A_AC3,
	CODECID_A_MP3,
}RJONE_CODE_TYPE;

typedef enum
{
	USE_TYPE_UNKN,
	USE_TYPE_LIVE,
	USE_TYPE_PLAYBACK,
	USE_TYPE_SPEAK,
}RJONE_FRAME_USE_TYPE;





////////网络应答数据结构////////


//IOCTRL_TYPE_LIVE_START_REQ,
//IOCTRL_TYPE_LIVE_START_RESP,
typedef struct _LIVE_START_REQ_
{
	UINT8 u8EnableVideoSend;  // 1: 发送视频，0 : 不发送视频 
	UINT8 u8EnableAudioSend;  // 1. 发送音频.    0 : 不发送音频
	UINT8 u8VideoChan;		//请求视频通道数据， 0 为主码流720p，1 为子码流VGA。2 为QVGA
	UINT8 u8Reserved[9];
}RJONE_LIVE_START_REQ;


typedef struct _LIVE_START_RESP_
{
	INT32 s32Result;		// RJONE_SUCCESS: 成功  . 不成功则返回错误值	
	UINT8 u8Reserved[8];
}RJONE_LIVE_START_RESP;



//IOCTRL_TYPE_LIVE_STOP_REQ,
//IOCTRL_TYPE_LIVE_STOP_RESP,
typedef struct _LIVE_STOP_REQ_
{	
	UINT8 u8Reserved[8];
}RJONE_LIVE_STOP_REQ;


typedef struct _LIVE_STOP_RESP_
{
	INT32 s32Result;		// RJONE_SUCCESS: 成功  . 不成功则返回错误值	
	UINT8 u8Reserved[8];
}RJONE_LIVE_STOP_RESP;



//IOCTRL_TYPE_LIVE_AUDIO_SPEAK_START_REQ,
//IOCTRL_TYPE_LIVE_AUDIO_SPEAK_START_RESP,
typedef struct _AUDIO_SPEAK_START_REQ
{
	UINT8 u8Reserved[8];
}RJONE_AUDIO_SPEAK_START_REQ;

typedef struct _AUDIO_SPEAK_START_RESP
{
	INT32 s32Result;					// s32Result = RJONE_SUCCESS:      授权成功，开始对讲
									// s32Result != RJONE_SUCCESS: 授权失败，返回错误原因
	UINT8 u8Reserved[8];
}RJONE_AUDIO_SPEAK_START_RESP;



//IOCTRL_TYPE_LIVE_AUDIO_SPEAK_STOP_REQ,
//IOCTRL_TYPE_LIVE_AUDIO_SPEAK_STOP_RESP,
typedef struct _AUDIO_SPEAK_STOP_REQ
{
	UINT8 u8Reserved[8];
}RJONE_AUDIO_SPEAK_STOP_REQ;

typedef struct _AUDIO_SPEAK_STOP_RESP
{
	INT32 s32Result;	
	UINT8 u8Reserved[8];
}RJONE_AUDIO_SPEAK_STOP_RESP;

///////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								无线部分
*******************************************************************************************/

////////相关数据结构////////
typedef enum
{
	IOCTRL_WIFIAPENC_INVALID		= 0x00, 
	IOCTRL_WIFIAPENC_NONE			= 0x01,
	IOCTRL_WIFIAPENC_WEP_NO_PWD		= 0x02,
	IOCTRL_WIFIAPENC_WEP_WITH_PWD	= 0x03,
	IOCTRL_WIFIAPENC_WPA_TKIP		= 0x04, 
	IOCTRL_WIFIAPENC_WPA_AES		= 0x05, 
	IOCTRL_WIFIAPENC_WPA2_TKIP		= 0x06, 
	IOCTRL_WIFIAPENC_WPA2_AES		= 0x07,
	IOCTRL_WIFIAPENC_WPA_PSK_AES    = 0x08,
	IOCTRL_WIFIAPENC_WPA_PSK_TKIP   = 0x09,	
	IOCTRL_WIFIAPENC_WPA2_PSK_AES   = 0x0A,
	IOCTRL_WIFIAPENC_WPA2_PSK_TKIP  = 0x0B,
}RJONE_WIFIAP_ENC;



typedef struct _RJONE_LIST_AP_INFO_
{
	CHAR  strAddress[20];		      // 机器的ip地址
	CHAR  strSsid[64];			      // SSID
	UINT16 u16Channel;			// 信道
	UINT8   u8Enctype;			// 加密模式  参考RJONE_WIFIAP_ENC
	UINT8   u8SignalLevel;		// 信号强度，值的范围[0-100]，值越大信号越强。
	UINT8   u8Reserve[8]; 		//保留
}RJONE_LIST_AP_INFO;

typedef struct
{
	CHAR strSsid[64];				//WiFi ssid 
	CHAR strPassword[64];		//WiFi 密码
	UINT8 u8Enctype;			       // 加密模式 参考 RJONE_WIFIAP_ENC
    	UINT8 u8SignalChannel;		//wifi  信道 [1,14]
    	UINT8 reserved[10];
 }RJONE_STA_INFO,RJONE_AP_INFO;


////////网络应答数据结构////////

//IOCTRL_TYPE_GET_AP_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_AP_PARAMETERS_RESP,
typedef struct _GET_AP_PARAMETERS_REQ
{
	UINT8 u8Reserved[8];
}RJONE_GET_AP_PARAMETERS_REQ;


typedef struct _GET_AP_PARAMETERS_RESP
{
	INT32 s32Result;					//s32Result = RJONE_SUCCESS: 获得参数成功，
									//s32Result != RJONE_SUCCESS: 获取失败,返回错误原因	
	RJONE_AP_INFO stApInfo;	
	UINT8 u8Reserved[8];
}RJONE_GET_AP_PARAMETERS_RESP;



//IOCTRL_TYPE_SET_AP_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_AP_PARAMETERS_RESP,
typedef struct _SET_AP_PARAMETERS_REQ
{
	RJONE_AP_INFO stApInfo;
	UINT8 u8Reserved[8];
}RJONE_SET_AP_PARAMETERS_REQ;


typedef struct _SET_AP_PARAMETERS_RESP
{
	INT32 s32Result;	
	UINT8 u8Reserved[8];
}RJONE_SET_AP_PARAMETERS_RESP;




//IOCTRL_TYPE_GET_STA_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_STA_PARAMETERS_RESP,
typedef struct _GET_STA_PARAMETERS_REQ
{
	UINT8 u8Reserved[8];
}RJONE_GET_STA_PARAMETERS_REQ;


typedef struct _GET_STA_PARAMETERS_RESP
{
	INT32 s32Result;					//s32Result = RJONE_SUCCESS: 获得参数成功，
										//s32Result != RJONE_SUCCESS: 获取失败,返回错误原因
	RJONE_STA_INFO stStaInfo;	
	UINT8 u8Reserved[8];
}RJONE_GET_STA_PARAMETERS_RESP;



//IOCTRL_TYPE_SET_STA_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_STA_PARAMETERS_RESP,
typedef struct _SET_STA_PARAMETERS_REQ
{
	RJONE_STA_INFO stStaInfo;
	UINT8 u8Reserved[8];
}RJONE_SET_STA_PARAMETERS_REQ;


typedef struct _SET_STA_PARAMETERS_RESP
{
	INT32 s32Result;			
	UINT8 u8Reserved[8];
}RJONE_SET_STA_PARAMETERS_RESP;




//IOCTRL_TYPE_LIST_WIFI_AP_REQ,
//IOCTRL_TYPE_LIST_WIFI_AP_RESP,
typedef struct _LIST_WIFI_AP_REQ
{
	UINT8 u8Reserved[8];
}RJONE_LIST_WIFI_AP_REQ;


typedef struct _LIST_WIFI_AP_RESP
{
	INT32 s32Result;															
	UINT32 u32ApCount;			//查找到的 AP 数量
	UINT8 u8Reserved[8];
	RJONE_LIST_AP_INFO  stApInfo[0]; 	//AP INFO 数据地址						
}RJONE_LIST_WIFI_AP_RESP;



///////////////////////////////////////////////////////////////////////////////////////////


/******************************************************************************************
								云台部分
*******************************************************************************************/


////////相关数据结构////////

typedef enum 
{
	IOCTRL_PTZ_STOP,
	IOCTRL_PTZ_UP,
	IOCTRL_PTZ_DOWN,
	IOCTRL_PTZ_LEFT,
	IOCTRL_PTZ_RIGHT, 
	IOCTRL_PTZ_BACK,
	IOCTRL_PTZ_FORWARD, 	
	IOCTRL_LENS_ZOOM_IN, 
	IOCTRL_LENS_ZOOM_OUT,    
}RJONE_PTZ_CMD;

////////网络应答数据结构////////
//IOCTRL_TYPE_PTZ_COMMAND_REQ,
//IOCTRL_TYPE_PTZ_COMMAND_RESP,
typedef struct _PTZ_COMMAND_REQ
{
	RJONE_PTZ_CMD enPtzCmd;	//PTZ 控制命令
	UINT8 u8Speed;				// PTZ 控制速度范围[1-10] ,值越大速度越快
	UINT8 u8Reserved[7];
}RJONE_PTZ_COMMAND_REQ;


typedef struct _PTZ_COMMAND_RESP
{
	INT32 s32Result;		
	UINT8 u8Reserved[8];
}RJONE_PTZ_COMMAND_RESP;


///////////////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								网络录像文件部分
*******************************************************************************************/

////////相关数据结构////////
typedef struct _DAY_TIME_
{
	UINT16 u16Year;
	UINT8  u8Month;	//January=0,...
	UINT8  u8Day;		
	UINT8  u8Hour;
	UINT8  u8Minute;
	UINT8  u8Second;
	UINT8 u8Reserved[1];
}RJONE_DAY_TIME;



typedef struct _RJONE_RECORD_FILE_
{
	CHAR strFileName[40];		              //文件名字
	UINT32 u32FileLen;			       //文件长度
}RJONE_RECORD_FILE;



// IOCTRL Record Playback Command
typedef enum 
{
	IOCTRL_RECORD_PLAY_CMD_START,	//command below is valid when IOCTRL_RECORD_PLAY_CMD_START is successful 
	IOCTRL_RECORD_PLAY_CMD_PAUSE,
	IOCTRL_RECORD_PLAY_CMD_RESUME,
	IOCTRL_RECORD_PLAY_CMD_STOP,
	IOCTRL_RECORD_PLAY_CMD_END,       //这个命令是指，文件播放到结尾点时，设备端向客户端发送命令。
	IOCTRL_RECORD_PLAY_CMD_FAST,   // 1X, 2X, 4X
	IOCTRL_RECORD_PLAY_CMD_SEEKTIME,
}RJONE_RECORD_PLAY_CMD;



////////网络应答数据结构////////
//IOCTRL_TYPE_LIST_RECORDFILES_REQ,
//IOCTRL_TYPE_LIST_RECORDFILES_RESP,
typedef struct _LIST_RECORDFILES_REQ
{
	RJONE_DAY_TIME	  stStartTime; // 录像搜索的开始时间
	RJONE_DAY_TIME	  stEndTime;   // 录像搜索的结束时间
	UINT8  u8Reserved[8];		
}RJONE_LIST_RECORDFILES_REQ;


typedef struct _LIST_RECORDFILES_RESP
{	
	INT32  s32Result;			//RJONE_SUCCESS: Success   RJONE_FAILED: search time don't belong to [stStartTime, stEndTime], within one day.
	UINT32   u32SearchFileTotal;			//符合查询条件的文件的总数量
	UINT8    u8Count;						// 当前包，包含的文件个数
	UINT8    u8EndFlg;					//  u8EndFlg = 1 表示当前包为最后一个发送包。
	UINT8    u8Index;						// 包的序列号	
	UINT8    u8Reserved[5];
	RJONE_RECORD_FILE	  stFile[0];		//文件信息的地址	
}RJONE_LIST_RECORDFILES_RESP;




//IOCTRL_TYPE_DOWNLOAD_RECORD_FILE_REQ,
//IOCTRL_TYPE_DOWNLOAD_RECORD_FILE_RESP,
typedef struct _DOWNLOAD_RECORD_FILE_REQ
{
	CHAR strFileName[40];		//文件名字	
	UINT8  u8Reserved[8];	
}RJONE_DOWNLOAD_RECORD_FILE_REQ;


typedef struct _DOWNLOAD_RECORD_FILE_RESP
{
	INT32  s32Result;			//RJONE_SUCCESS: 成功 :  失败返回错误原因
	UINT8  u8Reserved[8];	
}RJONE_DOWNLOAD_RECORD_FILE_RESP;


//IOCTRL_TYPE_RECORD_PLAYCONTROL_REQ,
//IOCTRL_TYPE_RECORD_PLAYCONTROL_RESP,
typedef struct _RECORD_PLAYCONTROL_REQ
{
	UINT16 u16Command;			//回放录像控制命令，参考 RJONE_RECORD_PLAY_CMD
	UINT16 us16SeekTimeSec;	       // 让设备按指定的时间点，来回放文件， 范围 [0 - 文件结束时间点]
	UINT32 u32Param;			//当   u16Command = IOCTRL_RECORD_PLAY_CMD_FAST 时，
								//  u32Param = 1 : 1倍速  u32Param = 2 : 2倍速u32Param = 4 : 4倍速
	CHAR strFileName[40];		  //文件名字	
	UINT8  u8Reserved[8];
}RJONE_RECORD_PLAYCONTROL_REQ;


typedef struct _RECORD_PLAYCONTROL_RESP
{
	INT32    s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT16  u16Command;		//回放录像控制命令，参考 RJONE_RECORD_PLAY_CMD
	UINT16   s32FilePlaySec;		//返回播放文件播放总时间，单位是秒。
	UINT8    u8Reserved[8];		
}RJONE_RECORD_PLAYCONTROL_RESP;



//IOCTRL_TYPE_NET_TRIGGER_RECORD_START_REQ,
//IOCTRL_TYPE_NET_TRIGGER_RECORD_START_RESP,
typedef struct _NET_TRIGGER_RECORD_START_REQ
{
	UINT8  u8Reserved[8];
}RJONE_NET_TRIGGER_RECORD_START_REQ;


typedef struct _NET_TRIGGER_RECORD_START_RESP
{
	INT32  	s32Result;			      //RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8Reserved[8];		
}RJONE_NET_TRIGGER_RECORD_START_RESP;


//IOCTRL_TYPE_NET_TRIGGER_RECORD_CHECK_REQ,
//IOCTRL_TYPE_NET_TRIGGER_RECORD_CHECK_RESP,
typedef struct _NET_TRIGGER_RECORD_CHECK_REQ
{		
	UINT8  u8Reserved[8];
}RJONE_NET_TRIGGER_RECORD_CHECK_REQ;


typedef struct _NET_TRIGGER_RECORD_CHECK_RESP
{
	INT32 s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8NowRecord;		// 0 没有在录像， 1 表示在录像。	
	UINT8  u8Reserved[7];
}RJONE_NET_TRIGGER_RECORD_CHECK_RESP;


//IOCTRL_TYPE_NET_TRIGGER_RECORD_STOP_REQ,
//IOCTRL_TYPE_NET_TRIGGER_RECORD_STOP_RESP,
typedef struct _NET_TRIGGER_RECORD_STOP_REQ
{		
	UINT8  u8Reserved[8];
}RJONE_NET_TRIGGER_RECORD_STOP_REQ;


typedef struct _NET_TRIGGER_RECORD_STOP_RESP
{
	INT32 s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8Reserve[8];		
}RJONE_NET_TRIGGER_RECORD_STOP_RESP;




//////////////////////////////////////////////////////////////////////////////////////////////////


/******************************************************************************************
								设备推送私有协议部分
*******************************************************************************************/

////////相关数据结构////////

#define RJONE_PUSH_NET_VERSION 0x1000
//消息ID
typedef enum 
{
	PUSH_CMD_ID_DEV_REGISTER             = 1,  			//设备注册消息,设备->服务器
	PUSH_CMD_ID_DEV_REGISTER_REPLY       = 2,  			//设备注册回应,服务器->设备
	PUSH_CMD_ID_DEV_EVENT_NOTIFY         = 3,  			//设备消息通知,设备->服务器
	PUSH_CMD_ID_DEV_EVENT_NOTIFY_REPLY   = 4,  		//设备消息通知回应,服务器->设备
	PUSH_CMD_ID_PHONE_REGISTER           = 5,  			//手机注册消息,手机->服务器
	PUSH_CMD_ID_PHONE_REGISTER_REPLY     = 6,  		//手机注册回应,服务器->手机
	PUSH_CMD_ID_HEARTBEAT                = 7,  				//心跳消息,手机<->服务器
	PUSH_CMD_ID_PUSH_EVENT_NOTIFY        = 9,  			//手机设备消息通知,服务器->手机
	PUSH_CMD_ID_PUSH_EVENT_NOTIFY_REPLY  = 10, 		//手机设备消息通知回应,手机->服务器
	PUSH_CMD_ID_PHONE_DEV_REGISTER       = 11, 		//手机关联设备注册,手机->服务器
	PUSH_CMD_ID_PHONE_DEV_REGISTER_REPLY = 12, 	//手机关联设备注册回应,服务器->手机
	PUSH_CMD_ID_PHONE_DELETE_DEV         = 13, //手机删除关联设备,手机->服务器
}RJONE_PUSH_CMD_ID;


//设备类型
typedef enum 
{
	DEV_TYPE_DOOR_BELL = 1, //门铃
}RJONE_DEV_TYPE;


//设备事件类型
typedef enum 
{
	DEV_EVENT_DOOR_BELL_PUSHED = 1, //门铃被按下
	DEV_EVENT_DOOR_BELL_BROKEN = 2, //门铃被强拆
}RJONE_DEV_EVENT_TYPE;


//========语言类型手机端使用=========
typedef enum 
{
	LANGUAGE_ENGLISH = 1,
	LANGUAGE_CHINESE = 2,
}RJONE_PUSH_LANGUAGE_TYPE;


//附加数据类型
typedef enum
{
	EXTRA_DATA_NONE    = 0, //无附加数据
	EXTRA_DATA_SPEECH  = 1, //语音
	EXTRA_DATA_PICTURE = 2, //图片
}RJONE_PUSH_EXTRA_DATA_TYPE;


//网络消息包头
typedef struct PushNetPackHdr
{
	UINT16 u16Version; 			//当前必须为 PUSH_NET_VERSION
	UINT16 u16Cmd;     			//消息ID，决定了包数据格式，见 CommandId
	UINT32 u32PackLen; 			//包数据长度
} RJONE_PUSH_NET_PACK_HDRS;


//设备注册消息,PUSH_CMD_ID_DEV_REGISTER
typedef struct PushCmdDevRegister
{
	CHAR strDevId[64];   				//设备唯一ID
	INT8  s8DevType;     				//取值见 DevType
	INT8  s8Reserved[7];
}RJONE_PUSH_CMD_DEV_REGISTER;


//设备注册回应,PUSH_CMD_ID_DEV_REGISTER_REPLY
typedef struct PushCmdDevRegisterReply
{
	INT8   s8Redirect;     				//重定向标志。如果不为0，设备需要更新后面的服务器地址和端口，并重新向新服务器地址注册
	INT8   s8Reserved[5];
	UINT16 u16ServerPort;   			//服务器端口
	CHAR   strServerIp[16]; 			//服务器IP
}RJONE_PUSH_DEV_REGISTER_REPLY;


//设备消息通知,PUSH_CMD_ID_DEV_EVENT_NOTIFY
typedef struct PushCmdDevEventNotify
{
	CHAR strDevId[64];           	//设备唯一ID
	INT8 s8DevType;         	//设备类型,取值见 DevType
	INT8 s8EventType;       	//事件类型,取值见 DevEventType
	INT8 s8ExtraDataType;   	//附加数据类型,取值见 ExtraDataType
	INT8 s8Reserved[5];
	UINT32 u32Timestamp;     	//事件的UTC时间
	UINT32 u32ExtraDataLen;  //附加数据长度
	INT8   s8ExtraData[0]; 	//附加数据,长度由 extraDataLen 字段决定
} RJONE_PUSH_CMD_DEV_EVENT_NOTIFY;



//手机注册消息,PUSH_CMD_ID_PHONE_REGISTER
typedef struct PushCmdPhoneRegister
{
	CHAR   strPhoneId[64];     		//手机唯一标识(对于iOS为系统分配的device token)
	INT8   s8PhoneType;       		//手机类型，取值见 PhoneType
	INT8   s8Language;        		//手机语言，取值见 LanguageType
	UINT16 u16EventExpireTime; 	//设备事件保留天数,该手机未接收到的设备事件在服务器保留的天数
	INT8    s8Timezone;        		//手机时区，取值[-24,24]
	INT8    s8AppId;          		//用于标识不同APP的ID
	INT8   s8Reserved[2];
	UINT64 u64LastEventId;    	 	//最后一个从服务器收到的事件ID，没有则填0
}RJONE_PUSH_CMD_PHONE_REGISTER;


//手机注册回应,PUSH_CMD_ID_PHONE_REGISTER_REPLY
typedef struct PushCmdPhoneRegisterReply
{
	INT8   s8Redirect;     			//重定向标志。如果不为0，手机APP需要更新后面的服务器地址和端口，并重新向新服务器地址注册
	INT8   s8Reserved[5];
	UINT16 u16ServerPort;   		//服务器端口
	CHAR   strServerIp[16]; 		//服务器IP
}RJONE_PUSH_CMD_PHONE_REGISTER_REPLY;


//用于手机关联设备注册消息中的设备列表项
typedef struct PhoneDevEntry
{
	CHAR strDevId[64]; 			//设备ID
	CHAR strName[32];  			//该设备在手机上的显示名称
} RJONE_PHONE_DEV_ENTRY;


//手机关联设备注册,PUSH_CMD_ID_PHONE_DEV_REGISTER
typedef struct PushCmdPhoneDevRegister
{
	UINT32 u32DevCount; 						//手机关联的设备个数
	UINT32 u32Reserved;
	RJONE_PHONE_DEV_ENTRY devEntries[0]; 		//手机关联的设备列表，个数由devCount指定
}RJONE_PUSH_CMD_PHONE_DEV_REGISTER;


//手机关联设备注册回应,PUSH_CMD_ID_PHONE_DEV_REGISTER_REPLY
typedef struct PushCmdPhoneDevRegisterReply
{
	UINT32 u32DevCount; //成功注册的设备个数
}RJONE_PUSH_CMD_PHONE_DEV_REGISTER_REPLY;


//手机设备消息通知,PUSH_CMD_ID_PUSH_EVENT_NOTIFY
 typedef struct PushCmdPushEventNotify
{
	CHAR   strDevId[64];       	//设备唯一ID
	INT8   s8DevType;         	//设备类型,取值见 DevType
	INT8   s8EventType;       	//事件类型,取值见 DevEventType
	INT8   s8Reserved;
	INT8   s8ExtraDataType;   //附加数据类型,取值见 ExtraDataType
	UINT32 u32Timestamp;     //事件的UTC时间
	UINT64 u64EventId;     	//服务器给事件分配的唯一ID
	UINT32 u32ExtraDataLen;  //附加数据长度
	INT8   s8ExtraData[0]; 	//附加数据,长度由 extraDataLen 字段决定
}RJONE_PUSH_CMD_PUSH_EVENT_NOTIFY;


//手机设备消息通知回应,PUSH_CMD_ID_PUSH_EVENT_NOTIFY_REPLY
typedef struct PushCmdPushEventNotifyReply
{
	UINT64 u64EventId;     //收到的事件ID
}RJONE_PUSH_CMD_PUSH_EVENT_NOTIFY_REPLY;

//手机删除关联设备,手机->服务器,PUSH_CMD_ID_PHONE_DELETE_DEV
typedef struct PushCmdPhoneDeleteDev
{
	char devId[64]; //设备ID
}RJONE_PUSH_CMD_PHONE_DELETE_DEV;



////////网络应答数据结构////////

//IOCTRL_TYPE_GET_PUSH_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_PUSH_PARAMETERS_RESP,

typedef struct _GET_PUSH_PARAMETERS_REQ
{		
	UINT8  u8Reserved[8];       
}RJONE_GET_PUSH_PARAMETERS_REQ;

typedef struct _GET_PUSH_PARAMETERS_RESP
{		
	INT32 s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8   u8EnablePush;		//PUSH 功能使能:   0 关闭  1开启
	UINT8   u8OpenDoorPush;		// 开门推送消息:    0 关闭  1开启
	UINT8   u8PushExtraDataType;	//推送附带信息: 参考 RJONE_PUSH_EXTRA_DATA_TYPE
	UINT8    u8Reserved[9];	      
}RJONE_GET_PUSH_PARAMETERS_RESP;

	
//IOCTRL_TYPE_SET_PUSH_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_PUSH_PARAMETERS_RESP,

typedef struct _SET_PUSH_PARAMETERS_REQ
{		
	UINT8   u8EnablePush;		//PUSH 功能使能:   0 关闭  1开启
	UINT8   u8OpenDoorPush;		// 开门推送消息:    0 关闭  1开启
	UINT8   u8PushExtraDataType;	//推送附带信息: 参考 RJONE_PUSH_EXTRA_DATA_TYPE
	UINT8    u8Reserved[9];	   
}RJONE_SET_PUSH_PARAMETERS_REQ;

typedef struct _SET_PUSH_PARAMETERS_RESP
{		
	INT32    s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8Reserved[8];	      
}RJONE_SET_PUSH_PARAMETERS_RESP;


//IOCTRL_TYPE_PUSH_EVENT_REQ,
//IOCTRL_TYPE_PUSH_EVENT_RESP,?
typedef struct _PUSH_EVENT_REQ
{		
	 // 触发一次设备推送, 
	UINT8  u8Reserved[8];       
}RJONE_PUSH_EVENT_REQ;


typedef struct _PUSH_EVENT_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8Reserve[8];		
}RJONE_PUSH_EVENT_RESP;

//////////////////////////////////////////////////////////////////////////////////////////////////




/******************************************************************************************
								设备参数设置部分
*******************************************************************************************/

////////相关数据结构////////


typedef struct  _RJONE_SYS_TIME_S_
{	
	RJONE_DAY_TIME  stSysTime;			// 设备时间
	INT8		s8TimeZone;					// 时区
	UINT8	u8Reserve[3];
}RJONE_SYS_TIME_S;



typedef enum
{
	PASS_TYPE_UNKW,	 
	PASS_TYPE_DOOR,		//开锁密码
	PASS_TYPE_NET_VIEW,   //网络观看密码
}RJONG_PASS_TYPE;



////////网络应答数据结构////////


//IOCTRL_TYPE_GET_SYSFWVER_REQ,	
//IOCTRL_TYPE_GET_SYSFWVER_RESP,
typedef struct _GET_SYSFWVER_REQ
{		
	UINT8    u8Reserved[8];	
}RJONE_GET_SYSFWVER_REQ;


typedef struct _GET_SYSFWVER_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT32  u32VerFW;			//硬件版本号 u32VerFW(UINT)   "Version: %d.%d.%d.%d", (u32VerFW & 0xFF000000)>>24, (u32VerFW & 0x00FF0000)>>16, (u32VerFW & 0x0000FF00)>>8, (u32VerFW & 0x000000FF) >> 0 )
	UINT32  u32VerSW;			//软件版本号 u32VerSW(UINT)   "Version: %d.%d.%d.%d", (u32VerFW & 0xFF000000)>>24, (u32VerFW & 0x00FF0000)>>16, (u32VerFW & 0x0000FF00)>>8, (u32VerFW & 0x000000FF) >> 0 )
	UINT8    u8DevType;			// 参考RJONE_DEV_TYPE
	UINT8    u8Reserved[7];	
}RJONE_GET_SYSFWVER_RESP;




//IOCTRL_TYPE_GET_DEV_TIME_REQ,
//IOCTRL_TYPE_GET_DEV_TIME_RESP,
typedef struct _GET_DEV_TIME_REQ
{		
	UINT8    u8Reserved[8];	
}RJONE_GET_DEV_TIME_REQ;


typedef struct _GET_DEV_TIME_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	RJONE_SYS_TIME_S stSysTime;
	UINT8    u8Reserved[8];	
}RJONE_GET_DEV_TIME_RESP;


//IOCTRL_TYPE_SET_DEV_TIME_REQ,
//IOCTRL_TYPE_SET_DEV_TIME_RESP,
typedef struct _SET_DEV_TIME_REQ
{		
	RJONE_SYS_TIME_S stSysTime;
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_TIME_REQ;


typedef struct _SET_DEV_TIME_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因		
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_TIME_RESP;



//IOCTRL_TYPE_CHANGE_DEV_PASSWORD_REQ,
//IOCTRL_TYPE_CHANGE_DEV_PASSWORD_RESP,
typedef struct _CHANGE_DEV_PASSWORD_REQ
{			
	CHAR    strOldPass[48];	// 原密码
	CHAR    strNewPass[48];     // 新密码
	UINT8   u8PassType;     	 //密码类型，参考 RJONG_PASS_TYPE
	UINT8   u8Reserved[7];
}RJONE_CHANGE_DEV_PASSWORD_REQ;


typedef struct _CHANGE_DEV_PASSWORD_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因		
	UINT8   u8Reserved[8];
}RJONE_CHANGE_DEV_PASSWORD_RESP;



//IOCTRL_TYPE_GET_DEV_INTERNET_FLAG_REQ,  
//IOCTRL_TYPE_GET_DEV_INTERNET_FLAG_RESP,  
typedef struct _GET_DEV_INTERNET_FLAG_REQ
{			
	UINT8    u8Reserved[8];
}RJONE_GET_DEV_INTERNET_FLAG_REQ;


typedef struct _GET_DEV_INTERNET_FLAG_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8    u8EnableInernet;		// 0: 不能连internet.   1: 能连internet
	UINT8    u8Reserved[7];
}RJONE_GET_DEV_INTERNET_FLAG_RESP;



//IOCTRL_TYPE_SET_DEV_INTERNET_FLAG_REQ,  
//IOCTRL_TYPE_SET_DEV_INTERNET_FLAG_RESP,  
typedef struct _SET_DEV_INTERNET_FLAG_REQ
{			
	UINT8    u8EnableInernet;		// 0: 不能连internet.   1: 能连internet
	UINT8    u8Reserved[7];
}RJONE_SET_DEV_INTERNET_FLAG_REQ;


typedef struct _SET_DEV_INTERNET_FLAG_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_INTERNET_FLAG_RESP;



/////////////////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								图像参数设置
*******************************************************************************************/

////////相关数据结构////////

typedef enum
{
	GAMMA_TYPE_DEFAULT = 0,
    	GAMMA_TYPE_0,
    	GAMMA_TYPE_1,
    	GAMMA_TYPE_2,   
    	GAMMA_TYPE_AUTO,  
}RJONE_GAMMA_TYPE;



////////网络应答数据结构////////
//IOCTRL_TYPE_GET_DEV_IMAGE_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_DEV_IMAGE_PARAMETERS_RESP,
typedef struct _GET_DEV_IMAGE_PARAMETERS_REQ
{		
	UINT8    u8Reserved[8];
}RJONE_GET_DEV_IMAGE_PARAMETERS_REQ;


typedef struct _GET_DEV_IMAGE_PARAMETERS_RESP
{
	INT32 s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8  u8LumaVal;    			//亮度 [0 ~ 100]  默认值 50.
	UINT8  u8ContrVal;    			//对比度 [0 ~ 100]  默认值 50.
	UINT8  u8HueVal;	      			//色差度 [0 ~ 100]  默认值 50.	
	UINT8  u8SatuVal;      			//饱和度 [0 ~ 100]  默认值 50.
	UINT8  u8Gamma;			// 参考RJONE_GAMMA_TYPE
	UINT8  u8Mirror;			// 图像水平翻转u8Mirror=1表示翻转
	UINT8  u8Flip;			// 图像垂直翻转u8Flip =1 表示翻转
	UINT8    u8Reserved[9];
}RJONE_GET_DEV_IMAGE_PARAMETERS_RESP;


//IOCTRL_TYPE_SET_DEV_IMAGE_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_DEV_IMAGE_PARAMETERS_RESP,
typedef struct _SET_DEV_IMAGE_PARAMETERS_REQ
{		
	UINT8  u8LumaVal;    		//亮度 [0 ~ 100]  默认值 50.
	UINT8  u8ContrVal;    		//对比度 [0 ~ 100]  默认值 50.
	UINT8  u8HueVal;	      		//色差度 [0 ~ 100]  默认值 50.	
	UINT8  u8SatuVal;      		//饱和度 [0 ~ 100]  默认值 50.
	UINT8  u8Gamma;		// 参考RJONE_GAMMA_TYPE
	UINT8  u8Mirror;			// 图像水平翻转u8Mirror=1表示翻转
	UINT8  u8Flip;			// 图像垂直翻转u8Flip =1 表示翻转
	UINT8    u8Reserved[9];
}RJONE_SET_DEV_IMAGE_PARAMETERS_REQ;


typedef struct _SET_DEV_IMAGE_PARAMETERS_RESP
{
	INT32    s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_IMAGE_PARAMETERS_RESP;


///////////////////////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								视频参数设置
*******************************************************************************************/

////////相关数据结构////////




////////网络应答数据结构////////
//IOCTRL_TYPE_GET_DEV_VIDEO_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_DEV_VIDEO_PARAMETERS_RESP,
typedef struct _GET_DEV_VIDEO_PARAMETERS_REQ
{		
	UINT32 u32VideoChn;		//视频的通道号，一般 0 为 主码流， 1为辅码流
}RJONE_GET_DEV_VIDEO_PARAMETERS_REQ;


typedef struct _GET_DEV_VIDEO_PARAMETERS_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT8    u8VideoChn;		 //在设置中的通道号。一般 0 为主通道， 1为辅通道。
	UINT8   u8Code;     		 //压缩格式 参考RJONE_CODE_TYPE
	UINT8   u8FrameRate;    	// 0-20 .   默认值为  20.
	UINT8   u8Gop;    		  	// I 帧间隔 。 默认值为 40	
	UINT32  u32BitRate;	       //压缩码流.  默认值为 2048 Kb/S
	UINT32  u32PicWidth;		// 视频的宽。
	UINT32  u32PicHeight; 	// 视频的高。
}RJONE_GET_DEV_VIDEO_PARAMETERS_RESP;



//IOCTRL_TYPE_SET_DEV_VIDEO_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_DEV_VIDEO_PARAMETERS_RESP,
typedef struct _SET_DEV_VIDEO_PARAMETERS_REQ
{		
	UINT8    u8VideoChn;		 //在设置中的通道号。一般 0 为主通道， 1为辅通道。
	UINT8   u8Code;     		 //压缩格式 参考RJONE_CODE_TYPE
	UINT8   u8FrameRate;    	// 0-20 .   默认值为  20.
	UINT8   u8Gop;    		  	// I 帧间隔 。 默认值为 40	
	UINT32  u32BitRate;	       //压缩码流.  默认值为 2048 Kb/S
	UINT32  u32PicWidth;		// 视频的宽。
	UINT32  u32PicHeight; 	// 视频的高。
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_VIDEO_PARAMETERS_REQ;


typedef struct _SET_DEV_VIDEO_PARAMETERS_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8    u8Reserved[8];
}RJONE_SET_DEV_VIDEO_PARAMETERS_RESP;

/////////////////////////////////////////////////////////////////////////////////////////////




/******************************************************************************************
								报警设置
*******************************************************************************************/

////////相关数据结构////////


////////网络应答数据结构////////
//IOCTRL_TYPE_GET_DEVICE_ALARM_REQ,
//IOCTRL_TYPE_GET_DEVICE_ALARM_RESP,
typedef struct _GET_DEVICE_ALARM_REQ
{	
	UINT8    u8Reserved[8];
}RJONE_GET_DEVICE_ALARM_REQ;


typedef struct _GET_DEVICE_ALARM_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因		
	UINT8 u8Enable;			//  1: 报警启用 0:报警关闭
	UINT8 u8AlarmRecord;	//  报警后是否启用录像。 1:启用  0:不启用
	UINT8 u8Snap;			// 报警后是否启用抓拍。  1:启用  0:不启用
	UINT8 u8Push;			// 报警后是否启用推送。  1:启用  0:不启用
	UINT8 u8Reserved[8];
}RJONE_GET_DEVICE_ALARM_RESP;



//IOCTRL_TYPE_SET_DEVICE_ALARM_REQ,
//IOCTRL_TYPE_SET_DEVICE_ALARM_RESP,
typedef struct _SET_DEVICE_ALARM_REQ
{	
	UINT8 u8Enable;			//  1: 报警启用 0:报警关闭
	UINT8 u8AlarmRecord;	//  报警后是否启用录像。 1:启用  0:不启用
	UINT8 u8Snap;			// 报警后是否启用抓拍。  1:启用  0:不启用
	UINT8 u8Push;			// 报警后是否启用推送。  1:启用  0:不启用
	UINT8 u8Reserved[8];
}RJONE_SET_DEVICE_ALARM_REQ;


typedef struct _SET_DEVICE_ALARM_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因			
	UINT8    u8Reserved[8];
}RJONE_SET_DEVICE_ALARM_RESP;

////////////////////////////////////////////////////////////////////////////////////////////


/******************************************************************************************
								录像参数设置
*******************************************************************************************/

////////相关数据结构////////

typedef enum 
{
	FILE_FORMAT_NONE    = 0, 
	FILE_FORMAT_AVI, 		
	FILE_FORMAT_MP4, 
}RJONG_FILE_FORMAT;




////////网络应答数据结构////////
//IOCTRL_TYPE_GET_RECORD_PARAMETERS_REQ,
//IOCTRL_TYPE_GET_RECORD_PARAMETERS_RESP,
typedef struct _GET_RECORD_PARAMETERS_REQ
{	
	UINT8    u8Reserved[8];
}RJONE_GET_RECORD_PARAMETERS_REQ;


typedef struct _GET_RECORD_PARAMETERS_RESP
{	
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因
	UINT32 u32RecordTimeSec;				//  有触发事件时，录多少秒的录像。	
	UINT8   u8RecordFileFormat;			//  录像文件格式 ，参考 RJONG_FILE_FORMAT
	UINT8    u8Reserved[7];
}RJONE_GET_RECORD_PARAMETERS_RESP;




//IOCTRL_TYPE_SET_RECORD_PARAMETERS_REQ,
//IOCTRL_TYPE_SET_RECORD_PARAMETERS_RESP,
typedef struct _SET_RECORD_PARAMETERS_REQ
{	
	UINT32 u32RecordTimeSec;				//  有触发事件时，录多少秒的录像。	
	UINT8   u8RecordFileFormat;			//  录像文件格式 ，参考 RJONG_FILE_FORMAT
	UINT8    u8Reserved[7];	
}RJONE_SET_RECORD_PARAMETERS_REQ;


typedef struct _SET_RECORD_PARAMETERS_RESP
{
	INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8    u8Reserved[8];
}RJONE_SET_RECORD_PARAMETERS_RESP;


///////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								验证密码 &  授权
*******************************************************************************************/

////////相关数据结构////////



////////网络应答数据结构////////
//IOCTRL_TYPE_AUTHORIZE_REQ, 
//IOCTRL_TYPE_AUTHORIZE_RESP,
typedef struct _AUTHORIZE_REQ
{		
	CHAR  strPassWord[48];	     //填写观看密码
	UINT8 u8Reserved[8];
}RJONE_AUTHORIZE_REQ;

typedef struct _AUTHORIZE_RESP
{
       INT32 	s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因		
	UINT8 u8ValueReq; 			//=IOCTRLSetAuthorizeReq.value	
	UINT8 u8Reserved[7];
}RJONE_AUTHORIZE_RESP;

///////////////////////////////////////////////////////////////////////////////////////////////


/******************************************************************************************
								设备升级
*******************************************************************************************/


////////相关数据结构////////
typedef enum {
	UPGRADE_DATA_FIRST,		//App to Device, begin to transfer upgrade pakcet data
	UPGRADE_DATA_MID,		//App to Device, middle data of upgrade pakcet
	UPGRADE_DATA_END,		//App to Device, complete transfer.
}RJONE_ENUM_UPGRADE_DATA_TYPE;




//IOCTRL_TYPE_UPGRADE_READY,		//Device to App, Device is ready for receiving upgrade firmware. App begin to send data
typedef struct
{
	INT8  result;                  			 //0: ready; 1: wrong device password
	UINT8 reserved[3];
}RJONE_UPGREDE_RESP;


//for Upgrade firmware
typedef struct{
	UINT16 nUpgradeDataType;             //refer to RJONE_ENUM_UPGRADE_DATA_TYPE
	UINT16 nUpgradeDataSize;
	UINT32 nCRC;
   	CHAR  uDevPasswd[24];                  //end user upgrad firmware need input passward.
 	UINT8  bNeedDevPasswd;               //0: no need, 1:need verfy device password   
	UINT8  reserve[7];
}RJONE_UPGRADE_HEAD;


typedef struct
{
	UINT8  nouse[2]; //default: PK
	UINT8  dev_type;  // 1: single sensor; 2:dual sensor //2013-03-09
	UINT8  reserve1;
	UINT8  verThisBin[4]; 	// 1.2.3.4, verThisBin[0,1,2,3]=0x04,0x03,0x02,0x01
	UINT32 sizeThisBin;
	UINT8  flag[4];	      //default: P2PU
	UINT8  reserve2[8];
}RJONE_UPGRADE_FILE_HEAD;


////////网络应答数据结构////////


//IOCTRL_TYPE_UPGRADE_REQ,		//App to Device
// data: upgrade file size

//IOCTRL_TYPE_UPGRADE_READY,		//Device to App, Device is ready for receiving upgrade firmware. App begin to send data
// no data

//IOCTRL_TYPE_UPGRADE_QUERY,	//App to Device, Query device to upgrade firmware ok ? after sending upgrade data over
// no data

//IOCTRL_TYPE_UPGRADE_OK,			//Device to App
// no data

//IOCTRL_TYPE_UPGRADE_FAILED,		//Device to App
// no data

//IOCTRL_TYPE_GET_SN_ETC_REQ	//App to Device
// no data





//////////////////////////////////////////////////////////////////////////////////////////////////


/******************************************************************************************
								控制门铃
*******************************************************************************************/

////////相关数据结构////////



////////网络应答数据结构////////
//IOCTRL_TYPE_DOORBELL_OPEN_REQ,             
//IOCTRL_TYPE_DOORBELL_OPEN_RESP, 
typedef struct _DOORBELL_OPEN_REQ
{		
	UINT8	u8DoorBellOpen;           // 1. Open   0. No work;
	UINT8    u8Reserved[7];
}RJONE_DOORBELL_OPEN_REQ;


typedef struct DOORBELL_OPEN_RESP
{
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8 u8Reserved[8];
}RJONE_DOORBELL_OPEN_RESP;


//////////////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								SD卡 TF卡 操作
*******************************************************************************************/

////////相关数据结构////////



////////网络应答数据结构////////

//IOCTRL_TYPE_SET_DEVICE_SDFORMAT_REQ,
//IOCTRL_TYPE_SET_DEVICE_SDFORMAT_RESP,
typedef struct _SET_DEVICE_SDFORMAT_REQ
{			
	UINT8    u8Reserved[8];
}RJONE_SET_DEVICE_SDFORMAT_REQ;


typedef struct SET_DEVICE_SDFORMAT_RESP
{
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8 u8Reserved[8];
}RJONE_SET_DEVICE_SDFORMAT_RESP;


//IOCTRL_TYPE_SDFORMAT_QUERY_REQ,
//IOCTRL_TYPE_SDFORMAT_QUERY_RESP,
typedef struct _SDFORMAT_QUERY_REQ
{		
	UINT8 u8Reserved[8];
}RJONE_SDFORMAT_QUERY_REQ;


typedef struct _SDFORMAT_QUERY_RESP
{
	INT32    s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	INT8     s8QueryResult;           // 1: 正在格式化，-1 格式化失败，0.格式化完成
	UINT8    u8Reserved[7];
}RJONE_SDFORMAT_QUERY_RESP;


//IOCTRL_TYPE_GET_SD_INFO_REQ,
//IOCTRL_TYPE_GET_SD_INFO_RESP,
typedef struct _GET_SD_INFO_REQ
{		
	UINT8 u8Reserved[8];
}RJONE_GET_SD_INFO_REQ;


typedef struct GET_SD_INFO_RESP
{
	INT32    s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT32  u32SDSize;			//  SD 卡的容量大小，单位 M bytes.
	UINT32  u32UseSize;			// SD卡使用了的空间大小  M bytes.
	UINT32  u32AvailableSize;       // SD卡未使用的空间大小  M bytes.
	UINT8 u8Reserved[8];
}RJONE_GET_SD_INFO_RESP;


//	IOCTRL_TYPE_GET_RTSP_INFO_REQ,
//	IOCTRL_TYPE_GET_RTSP_INFO_RESP,
typedef struct _GET_RTSP_INFO_REQ
{		
	UINT8 u8Reserved[8];
}RJONE_GET_RTSP_INFO_REQ;


typedef struct GET_RTSP_INFO_RESP
{
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	CHAR  stUrl[2][50];			// 两组 rtsp 的地址。
	UINT8 u8VideoCode;		//视频压缩格式 RJONE_CODE_TYPE
	UINT8 u8AudioCode;		//音频压缩格式 RJONE_CODE_TYPE
	UINT8 u8AudioSamplerate;				//采样率 RJONE_AUDIO_SAMPLERATE
	UINT8 u8AudioBits;						//采样位宽RJONE_AUDIO_DATABITS
	UINT8 u8AudioChannel;					//通道		RJONE_AUDIO_CHANNEL					
	UINT8 u8Reserved[78];
}RJONE_GET_RTSP_INFO_RESP;


//IOCTRL_TYPE_GET_PERIPHERAL_EQUIPMENT_INFO_REQ,
//IOCTRL_TYPE_GET_PERIPHERAL_EQUIPMENT_INFO_RESP,
typedef struct _GET_PERIPHERAL_EQUIPMENT_INFO_REQ
{		
	UINT8 u8Reserved[8];
}RJONE_GET_PERIPHERAL_EQUIPMENT_INFO_REQ;

typedef struct _GET_PERIPHERAL_EQUIPMENT_INFO_RESP
{		
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8 u8TemperatureF;   //华氏温度
	UINT8 u8TemperatureC;   //摄氏温度
	UINT8 u8PtzHorizontal;	//云台水平位移角度 (范围0~120)，现未起作用
	UINT8 u8PtzVertical;		//云台垂直位移角度 (范围0~120).    现未起作用
	UINT8 u8Reserved[8];
}RJONE_GET_PERIPHERAL_EQUIPMENT_INFO_RESP;



//IOCTRL_TYPE_GET_FREQUENCY_INFO_REQ,
//IOCTRL_TYPE_GET_FREQUENCY_INFO_RESP,

//IOCTRL_TYPE_SET_FREQUENCY_INFO_REQ,
//IOCTRL_TYPE_SET_FREQUENCY_INFO_RESP,

typedef enum 
{
    RJONE_VIDEO_ENCODING_MODE_PAL_50HZ=0,
    RJONE_VIDEO_ENCODING_MODE_NTSC_60HZ,
    RJONE_VIDEO_ENCODING_MODE_AUTO,
    RJONE_VIDEO_ENCODING_MODE_BUTT
}RJONE_VIDEO_NORM_E;


typedef struct _GET_FREQUENCY_INFO_REQ
{		
	UINT8 u8Reserved[8];
}RJONE_GET_FREQUENCY_INFO_REQ;

typedef struct _GET_FREQUENCY_INFO_RESP
{		
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8 u8Frequency;  		//参考 RJONE_VIDEO_NORM_E
	UINT8 u8Reserved[7];
}RJONE_GET_FREQUENCY_INFO_RESP;


typedef struct _SET_FREQUENCY_INFO_REQ
{		
	UINT8 u8Frequency;  		//参考 RJONE_VIDEO_NORM_E
	UINT8 u8Reserved[7];
}RJONE_SET_FREQUENCY_INFO_REQ;

typedef struct _SET_FREQUENCY_INFO_RESP
{		
	INT32  s32Result;			//RJONE_SUCCESS 成功:   失败返回错误原因	
	UINT8 u8Reserved[8];
}RJONE_SET_FREQUENCY_INFO_RESP;


//////////////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
							局域网内设备广播检测
*******************************************************************************************/

////////相关数据结构////////

#define RJONE_REQUEST_DEV_INFO   5

#define RJONE_BROADCAST_RECV_PORT (22356)


typedef struct _DEV_NET_INFO
{
	    UINT32 u32ListenPort;			// 设备监听端口
           CHAR u8IP[16];				//设备IP 
           CHAR u8MAC[18];	   			// 设备 MAC 
	     UINT8 u8Reserved[2];		   
}RJONE_DEV_NET_INFO;

typedef struct  _DEV_BROADCAST_
{
        CHAR strDevId[64];         //设备唯一ID   ,                                           
        RJONE_DEV_NET_INFO   apInfo;   //无线AP 模式，网络信息
	 RJONE_DEV_NET_INFO   staInfo;  // 无线STA 模式.  网络信息
	 RJONE_DEV_NET_INFO   lineInfo;  //有线模式，网络信息
	 UINT32 u32SoftwareV;  //软件版本
	 UINT32 u32FirewareV;  //硬件版本
	 UINT8 u8DevType;          // 设备类型
	 UINT8 u8Cmd;                 // 设备端发送查询信息时，填RJONE_REQUEST_DEV_INFO 值。
        UINT8 u8Reserved[6];
}RJONE_DEV_SEND_BROADCAST,RJONE_DEV_RECV_BROADCAST;


///////////////////////////////////////////////////////////////////////////////////////////



/******************************************************************************************
								网络封包
*******************************************************************************************/

////////相关数据结构////////


typedef enum {
	SIO_TYPE_UNKN,
	SIO_TYPE_VIDEO_AUDIO_FRAME,	
	SIO_TYPE_IOCTRL,
	SIO_TYPE_DOWNLOAD_DATA,
	SIO_TYPE_HEART_ALIVE_PACKET,
	SIO_TYPE_UPGRADE_PACKET,
}RJONE_STREAM_IO_TYPE;

//NOTE: struct below is all Little Endian
typedef struct
{
        UINT8 u8StreamIOType; 			//填写 RJONE_STREAM_IO_TYPE  中相关项
        UINT8  u8Reserved[3];
        UINT32 u32DataSize;	                    //整个数据包长度
}RJONE_STREAM_IO_HEAD;



//for SIO_TYPE_IOCTRL
typedef struct{
	UINT16 u16IOCtrlType;					//参考 RJONE_IOCTRL_TYPE
	UINT16 u16IOCtrlDataSize;
	UINT8   u8Reserve[4];
}RJONE_IO_CTRL_HEAD;

typedef struct _VIDEO_FRAME_HEADER_
{	
	UINT32 nTimeStampMSec;				// 时间戳 毫秒	
	UINT32 u32FrameDataLen;			       // 帧数据长度
	UINT8   u8Code;						//压缩格式 RJONE_CODE_TYPE
	UINT8   u8VideoFrameType;			// I帧 or  p帧or b帧  RJONE_VIDEO_FRAME
	UINT8   u8FrameIndex;			//帧计数，当是I帧时，u8rameIndex = 0，p帧时u8rameIndex=u8rameIndex+1. 这个变量主要用来判断丢帧情况。
	UINT8   u8Reserved[9];
}RJONE_VIDEO_FRAME_HEADER;

typedef struct _AUDIO_FRAME_HEADER_
{
	UINT8 u8Code;							//压缩格式RJONE_CODE_TYPE
	UINT8 u8AudioSamplerate;				//采样率 RJONE_AUDIO_SAMPLERATE
	UINT8 u8AudioBits;						//采样位宽RJONE_AUDIO_DATABITS
	UINT8 u8AudioChannel;					//通道		RJONE_AUDIO_CHANNEL					
	UINT32 nTimeStampMSec;				// 时间戳 毫秒	
	UINT32 u32FrameDataLen;			// 帧数据长度	
	UINT8   u8FrameIndex;			//帧计数，每来一帧 u8rameIndex=u8rameIndex+1. 这个变量主要用来判断丢帧情况。
	UINT8   u8Reserved[7];
}RJONE_AUDIO_FRAME_HEADER;


// for SIO_TYPE_VIDEO_AUDIO_FRAME
typedef struct _FRAME_HEAD_
{	
	UINT8 u8FrameType;  				       // 参考 RJONE_FRAME_TYPE
	UINT8 u8FrameUseType;				// 参考 FRAME_USE_TYPE	
	UINT16 u16FrameSplitPackTotalNum;	       // 单帧数据大小超过发送缓冲时，进行分包，u8FramePackNum表示分包的总个数。小于等于1  时，表示未分包。
	UINT16 u16SplitPackNo;					// 当前分包的序号		
	UINT8   u8Reserved[2];
	union
	{
		RJONE_VIDEO_FRAME_HEADER stVideoFrameHead; 
		RJONE_AUDIO_FRAME_HEADER stAudioFrameHead;
	};		
}RJONE_FRAME_HEAD;



// for SIO_TYPE_SEND_FILE_DATA
typedef struct _FILE_DOWNLOAD_PACK_HEAD_
{	
	UINT32  u32FileTotalSize;			//发送文件的总长度
	UINT32  u32TotalSendPackNum;	//要发送的总包数
	UINT32  u32CurPackNo;			//当前包的序号
	UINT32  u32CurPackSize;			//当前包中录像文件长度
	UINT32  uCRC;					// uCRC=0 时不效验
	UINT8    u8Endflag;				//u8Endflag=1: 表示当前包是结束包	
	UINT8    u8FileType;				// 这个参数未定义作用，在下个修正版本中，可能修改。
	UINT8    u8Reserve[2];	
	UINT8    u8Data[0];				//数据的起始地址	
}RJONE_FILE_DOWNLOAD_PACK_HEAD;




// 网络包发送格式

/*   控制数据包

	RJONE_STREAM_IO_HEAD + RJONE_IO_CTRL_HEAD + 控制数据

*/


/*  音视频数据包

	RJONE_STREAM_IO_HEAD + RJONE_FRAME_HEAD + 音视频数据

*/

/* 下载文件数据包

	RJONE_STREAM_IO_HEAD + RJONE_FILE_DOWNLOAD_PACK_HEAD + 视频文件数据

*/


/* 升级文件数据包

	RJONE_STREAM_IO_HEAD + RJONE_UPGRADE_HEAD + 升级文件数据

*/

/*  心跳包发送
	RJONE_STREAM_IO_HEAD     u8StreamIOType=SIO_TYPE_HEART_ALIVE_PACKET
*/

////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif 
