#ifndef _SAVEPLAYCTROL_H_
#define _SAVEPLAYCTROL_H_



#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "devfile.h"
#include "filesystem.h"


#ifdef __cplusplus
  }
#endif




#define TD_DRV_DATA_TYPE_PLAYBACK_VIDEO 4


#define TD_DRV_DATA_TYPE_VIDEO 1
#define TD_DRV_DATA_TYPE_AUDIO 2
#define TD_DRV_DATA_TYPE_NET 3
#define TD_DRV_DATA_TYPE_PLAYBACK_VIDEO 4
#define TD_DRV_DATA_TYPE_PLAYBACK_AUDIO 5



int FTC_GetFunctionAddr(GST_DEV_FILE_PARAM * gst);
int  FTC_InitSavePlayControl(GST_DEV_FILE_PARAM * gst);
int FTC_StopSavePlay();
int FTC_CreateAllThread();


#endif


