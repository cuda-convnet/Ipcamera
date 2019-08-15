/*!
 * \file RtspServerDef.h
 *
 * \author Liulz
 * \date 十一月 2014
 *
 * RTSP服务器库内部定义
 */
#ifndef _RTSP_SERVER_DEF_H_
#define _RTSP_SERVER_DEF_H_
#include "singly_linked_list.h"


#ifdef __cplusplus
extern "C" {
#endif


struct AudioInfo
{
	int audioCodec;    // 0 pcm,1 g.711,2 g.726
	int channelNumber;
	int sampleRate; 
	int bitPerSample;
	int bitrate;       //kbps
};



#ifdef __cplusplus
};
#endif


#endif // _RTSP_SERVER_DEF_H_
