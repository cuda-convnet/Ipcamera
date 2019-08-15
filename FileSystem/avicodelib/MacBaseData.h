#ifndef _MAC_BASE_H_
#define _MAC_BASE_H_


#define CAP_WIDTH 720
#define CAP_HEIGHT_PAL 576
#define CAP_HEIGHT_NTSC 480

#define CAP_WIDTH_CIF 352
#define CAP_HEIGHT_CIF_PAL 288
#define CAP_HEIGHT_CIF_NTSC 240

#define CAP_WIDTH_HD1 720
#define CAP_HEIGHT_HD1_PAL 288
#define CAP_HEIGHT_HD1_NTSC 240


#define IMG_720P_WIDTH		(1280)
#define IMG_720P_HEIGHT	(720)

#define IMG_1080P_WIDTH	(1920)
#define IMG_1080P_HEIGHT	(1072)

#define IMG_960_WIDTH		(960)
#define IMG_NTSC_960_HEIGHT	(480)
#define IMG_PAL_960_HEIGHT	(576)

#define IMG_480P_WIDTH	(640)
#define IMG_480P_HEIGHT	(480)

#define MOTION_BUF_SIZE (20000)


#define VIDEO_FRAME 1
#define AUDIO_FRAME 2
#define DATA_FRAME  3

#define AUDIO_MAX_BUF_SIZE (8000)
#define AUDIO_MAX_SEND_NUM (16)


typedef struct _BUF_FRAME_HEADER_ BUF_FRAMEHEADER;



typedef struct _BUF_FRAME_HEADER_
{
	unsigned long type;
	unsigned long iLength;
	unsigned long iFrameType;
	unsigned long ulTimeSec;
	unsigned long ulTimeUsec;
	unsigned long standard;
	unsigned long size;
	unsigned long index;
	unsigned long frame_num;
}BUF_FRAMEHEADER;





#endif //_MAC_BASE_H_


