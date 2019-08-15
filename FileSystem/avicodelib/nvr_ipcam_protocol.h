#ifndef _NVR_IPCAM_PROTOCOL_HEADER_
#define _NVR_IPCAM_PROTOCOL_HEADER_


#define NORMAL_IPCAM_PORT (16000)

#define NVR_COMMON_MAX_ARGC_NUM (20)

typedef struct _NVR_COMMAND_ST{
int cmd;
int value[NVR_COMMON_MAX_ARGC_NUM];
}NVR_COMMAND_ST;

typedef struct _NVR_FRAME_HEADER_
{
	unsigned long type;
	unsigned long iLength;
	unsigned long iFrameType;
	unsigned long ulTimeSec;
	unsigned long ulTimeUsec;
	unsigned long pic_width;
	unsigned long pic_height;
	unsigned long channel;
	unsigned long frame_num;
	unsigned long Hz;
}NVR_FRAME_HEADER;


enum
{
	NVR_CAM_BASE = 10000,
	NET_CAM_GET_ENGINE_INFO,
	NET_CAM_NET_ALIVE
};




#endif


