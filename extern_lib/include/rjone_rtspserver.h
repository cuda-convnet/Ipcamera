/*!
 * \file rjone_rtspserver.h
 *
 * \author Liulz
 * \date 十一月 2014
 *
 * RTSP服务器库API定义
 */
#ifndef _RJONE_RTSPSERVER_H_
#define _RJONE_RTSPSERVER_H_
#include "singly_linked_list.h"
#include "RtspServerDef.h"

#ifdef __cplusplus
extern "C" {
#endif



enum 
{
	RJONE_RTSPSVR_SUCCESS = 0,

	RJONE_RTSPSVR_ERR_FAILED          = -1,
	RJONE_RTSPSVR_ERR_ALREADY_DONE    = -2,
	RJONE_RTSPSVR_ERR_NOT_INITIALIZED = -3,
	RJONE_RTSPSVR_ERR_NOT_STARTED     = -4,
	RJONE_RTSPSVR_ERR_ALREADY_EXIST   = -5,
};


struct AudioStreamInfo
{
	int audioCodec;    // 0 pcm,1 g.711,2 g.726
	int channel;
	int sampleRate; 
	int bitPerSample;
	int bitrate;       //kbps
};


#define DEFAULT_RTSP_PORT 554
#ifndef MAX_FRAME_SIZE //定义可能的最大帧大小
#define MAX_FRAME_SIZE (256 * 1024)
#endif



//-----------------------RJone RTSP Server API----------------------------------

int RJone_RTSPServer_Init();
void RJone_RTSPServer_Uninit();

 int RJone_RTSPServer_AddStream(const char *name, 
	SINGLY_LINKED_LIST_INFO_ST *  videoBuffer, int type,SINGLY_LINKED_LIST_INFO_ST * audioBuffer,struct AudioInfo audioInfo);

int RJone_RTSPServer_Startup(unsigned short rtspPort);
int RJone_RTSPServer_Shutdown();


#ifdef __cplusplus
};
#endif


//-----------------------调用流程------------------------------------------------
//  RJone_RTSPServer_Init()
//
//		多次调用RJone_RTSPServer_AddStream()将所有实时流添加进来
//
//		RJone_RTSPServer_Startup()
//			程序正常运行直到准备退出
//		RJone_RTSPServer_Shutdown()
//
//  RJone_RTSPServer_Uninit()
//------------------------------------------------------------------------------

#endif // _RJONE_RTSPSERVER_H_
