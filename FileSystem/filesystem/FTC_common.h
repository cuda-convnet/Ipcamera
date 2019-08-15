#ifndef _FTC_COMMON_H_
#define _FTC_COMMON_H_
 #include <linux/prctl.h>  


extern int g_EnableFileSave;
extern int g_EnableVideoPlay;
extern int g_EnableAudioPlay;

extern int gRecstart;
extern int gPlaystart;

extern int g_PlaybackImageSize;

extern GST_PLAYBACK_VIDEO_PARAM  g_pstPlaybackModeParam;
extern GST_PLAYBACK_VIDEO_PARAM g_pstRecModeParam;
extern GST_DEV_FILE_PARAM * g_pstCommonParam;

extern GST_FILESHOWINFO g_playFileInfo;

extern int g_RecCam;
extern int g_RecAudioCam;
extern int	  g_PlayCam;
extern GST_RECORDEVENT g_RecEvent;
extern int iLowestChannelId;

extern int    g_PlayStatus;


extern int g_States;
extern int g_iAlreadyWriteDisk;
extern int g_PreRecordFlag;
extern int g_PreviewIsStart;
extern int g_HandStopPlay;
extern int g_preview_sys_flag;

extern int g_AudioChannel;
extern time_t g_TimePlayPoint;
extern time_t g_TimePlayEndTime;
extern time_t g_TimePlayStartTime;

extern int g_RecStatusCurrent;
extern int g_PlayStatusCurrent;
extern int g_SearchLastSingleFrameChangeFile ;

extern int g_CheckFileSystem;

typedef struct _AUDIO_PLAY_BUF
{
	char * audioPlayBuf[5];
	int 	  iHead;
	int     iTail;
}AUDIOPLAYBUF;

typedef struct _WRITE_BUF
{
	char * head_buf;
	char * data_buf;
	int head_size;
	int data_size;
	int create_flag;
	int conut;
	int offset;
	int data_offset_start;
	int head_offset_start;
}WRTIE_BUF;

extern int g_RecDuringTime;


void thread_for_rec_file();
void thread_for_play_file(void);
void thread_for_audio_playback(void);
void thread_for_create_file(void);
void thread_for_preview_record();
void thread_for_create_usb_file(void);
void thread_for_create_cdrom_file(void);
void thread_for_create_disk_sleep(void);

time_t  FTC_CustomTimeTotime_t(int year,int month, int day, int hour, int minute,int second);
void FTC_time_tToCustomTime(time_t time,int * year, int * month,int* day,int* hour,int* minute,int* second );
int  FTC_GetWeekDay(time_t mTime);
int  FTC_GetFilelist( GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime);
int FTC_SetPlayFileInfo(GST_FILESHOWINFO * stPlayFileInfo);
int FTC_StopPlay();
int FTC_StartRec(int iCam,int iPreSecond);
int FTC_StopRec();
int FTC_GetRecordlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);

int FTC_PreviewRecordStart(int iSecond);
int  FTC_GetAlarmEventlist( GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
int FTC_SetTimeToPlay(time_t startPlayTime,time_t endPlayTime, int iCam,int playStyle);
int FTC_PrintMemoryInfo();
int FTC_AudioRecCam(int iCam);
int FTC_AudioPlayCam(int iCam);
int FTC_PreviewRecordStop(int IsRecord);
void FTC_set_count_create_keyframe(int count);
int  FTC_CheckPreviewStatus();
int FTC_PlayFast(int iSpeed);
int FTC_PlayBack(int iSpeed);
int FTC_PlayPause(int iStop);
int FTC_GetRecMode(int iVideoStandard,int iVideoSize);
int FTC_CurrentPlayMode(int * iVideoStandard,int * iVideoSize);
int FTC_PlaySingleFrame(int iFlag);
int FTC_PlayUsbWrite(int iFlag);
int FTC_PlayChangFile(int iFlag);
int FTC_ChangeRecordToPlay(int iNextOrLast);
int FTC_PlayUsbWriteStatus();
int FTC_CurrentDiskWriteAndReadStatus();
int FTC_PlayNearRecFile(time_t * CurrentPlayTime);
int FTC_get_cover_rate(int disk_id);
int FTC_GetBadBlockSize(int iDiskId);
int FTC_get_disk_remain_rate(int disk_id);
int FTC_ON_GetRecordlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
int FTC_ON_GetAlarmEventlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
int FTC_GetPlayCurrentStauts(int * status, int *iPlayStopFlag);
int FTC_GetCurrentPlayInfo(time_t * startTime,int * iCam);
int FTC_AlarmlistPlay(GST_FILESHOWINFO * stPlayFile);
int FTC_ReclistPlay(GST_FILESHOWINFO * stPlayFile);
int FTC_ClearAllChannel(int ImageSize,int Standard);
int FTC_SetRecDuringTime(int iSecond);
int FTC_GetEventlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage);
int FTC_Get_Play_Status();
int FTC_get_play_time(time_t * play_time);
int FTC_get_disk_rec_start_end_time(time_t * start_time,time_t * end_time,int disk_id);
int FTC_get_cur_play_cam();
int ftc_get_play_mode(int * image, int * standard);
GST_DRV_BUF_INFO* FTC_get_video_audio_buf(char * pData);
int get_preview_buf_frame_num();
int div_safe(float num1,float num2);
int FS_write_buf_sys_init();
int FS_write_buf_sys_destroy();
int FS_write_buf_sys_data(char * buf1,int size1,char * buf2,int size2);
int FS_write_buf_write_to_file();
int FS_read_buf_sys_init();
int FS_read_buf_sys_destroy();
void FS_read_buf_sys_clear();
int FS_read_data_to_buf(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData, char * AudioData,int playCam);
int preview_buf_write(int flag);
int ftc_is_disk_record();
void  FTC_FileGoToTimePoint(time_t startPlayTime);
int FTC_SetTimeToPlay_lock(time_t startPlayTime,time_t endPlayTime, int iCam,int playStyle);
int FTC_get_disk_rec_start_end_time_lock(time_t * start_time,time_t * end_time,int disk_id);
int ftc_get_disk_format_process_percent();
int fs_get_usb_write_size();
int get_cdrom_backup_perecnt();

#endif

