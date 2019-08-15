#ifndef _FILESYSTEM_H_
#define _FILESYSTEM_H_
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "libfdisk.h"
#include "devfile.h"
#include <time.h>



#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

//����makefile �ж���
//����16 ·��8·�ĺ궨��
/*
#define CHANNEL_8_MAC 

#ifndef CHANNEL_8_MAC
	#define CHANNEL_16_MAC
	//����ʵʱ�ͷ�ʵʱSLOW_16_MAC
	//#define SLOW_16_MAC
#endif
//#define TAILAND_PLAYBACK
*/
#define USE_TWO_CAPTURE
#define MAC_MODEL (3100)


#define DISK2000_SATA3_TEST
#define USE_EXT3
#define USE_WRITE_BUF_SYSTEM
#define USE_READ_BUF_SYSTEM
#define USE_PREVIEW_ENC_THREAD_WRITE
//#define USE_PLAYBACK_QUICK_SYSTEM

#define NET_USE_NEW_FUN
//#define NET_SET_MAC


//����h264 ��mpeg4
#define USE_H264_ENC_DEC
#define NOT_USE_RATE_CONTROL
#define SEACHER_LIST_MAX_NUM 300

#define H264_CODE 0x10001000
#define MPEG4_CODE 0x10002000

#define PREVIEW_SAVE_BUF 800

#define CD_SIZE (550*1024*1024)
#define DVD_SIZE (3*550*1024*1024)
#define MD_SIZE (200*1024*1024)

#define HI_VIDEO_CIF 	0
#define HI_VIDEO_HD1 	1
#define HI_VIDEO_D1   	2

#define MAXDISKNUM  	4
#define MAXFILEBUF 		1024*1024
#define FILESIZE    		128
#define MAXFILEPOS      	20115328 // 134012928               //128*1024*1024-200*1024 
#define MAXFILESIZE        (128 * 1024)
#define FIRSTBUILDFILENUMBER  2
#define CHANNELNUMBER		4

#define GETRECCAM (-99)



#define  PATITIONSIZE		60   // 40GB
#define LARGE_DISK_PATITION_SIZE (200)

#define FRAMEHEADOFFSET   50  // ֡ͷ��ŵ�ƫ�Ƶ�sizeof( FILEINFO )
									  //��Ϊ�µ��ļ�ϵͳ��֡ͷ�ļ������size = 3M,
									  //��������ĳ���ҪС�� 3 M,��������д�ļ�����
#define MAXFRAMEHEADOFFSET 3000000 //3145728  //1024*1024 *3//���֡ͷƫ�Ƶ�
#define MAXFRAMEDATAOFFSET  128246208 //125*1024*1024 //116440512//117424512  // 117440512 - 8 * 3000, 

#define MAXMESSAGENUMBER  1000
#define MAXTIMESTICKMESSAGENUMBER 10000
#define MAXADDMESSAGENUMBER  10000

#define EVENTMAXMESSAGE 3000

#define AUDIOBUF			8*1024


#define ALARMRECORD   1
#define TIMERECORD      2
#define  MANUALRECORD 3

#define NORECORD  0
#define RECORDING 1

#define EFFECT  1
#define NOEFFECT 0

#define ENABLE  1
#define DISNABLE 0

#define YES 1
#define NO  0

#define  FILEWRITEOVER  1
#define  STOPRECORD       0
#define  FILEWRITEERROR           2
#define  RECTIMEOVER     3

#define TD_DRV_VIDEO_STARDARD_NTSC 1
#define TD_DRV_VIDEO_STARDARD_PAL 0

#define TD_DRV_VIDEO_SIZE_960P  9
#define TD_DRV_VIDEO_SIZE_576P  8
#define TD_DRV_VIDEO_SIZE_480P 7
#define TD_DRV_VIDEO_SIZE_1080P 6
#define TD_DRV_VIDEO_SIZE_720P 5
#define TD_DRV_VIDEO_SIZE_960 4
#define TD_DRV_VIDEO_SIZE_CIF 2
#define TD_DRV_VIDEO_SIZE_HD1 1
#define TD_DRV_VIDEO_SIZE_D1 0



#define PLAYFASTSEARCH      99


typedef enum _FILE_STATE
{
	WRITING = 0,
	WRITEOVER,
	NOWRITE,
	NOCREATE,
	PLAYING,
	BADFILE
}FILESTATE;




typedef struct _FILE_INFO
{
	char 		fileName[20];   		//�ļ���
	time_t		StartTime;			//��ʼʱ��
	time_t 		EndTime;				//����ʱ��
	FILESTATE	states;               //�ļ�״̬
	unsigned long ulOffSet;      		// ��Ч�ļ�ƫ��λ�ã�ָ��������Ƶ֡ͷ������
	int			iDataEffect;			// ���ļ������Ƿ���Ч��
	int   	  		iEnableReWrite;		// ���ļ��Ƿ��ܸ��ǡ�
	int 			isCover;				// ���ļ��Ƿ����ڸ���.
	int 			ulVideoSize;             // CIF, HD1 ,D1      
	int 			mode;     // NTSC,PAL
}FILEINFO;



typedef struct _FILE_USE_INFO
{
	int iCurFilePos;		//��¼�ļ�ָ�뵱ǰ��λ�á�
	int iTotalFileNumber;	//�˴��̷����е��ļ�����
}FILEUSEINFO;



typedef struct _FILE_HEADER_INFO
{
	unsigned long ulFrameCount;
	unsigned long ulDataLen;
	unsigned long ulFrameType;
	unsigned long ulStandard;
	unsigned long ulVideoSize;
	unsigned long ulWidth;
	unsigned long ulHeight;
	unsigned long ulTimeSec;
	unsigned long ulTimeUsec;
	unsigned long ulFrameDataOffset;
			char type;						// "V" is video,  "A" is audio
			int    iChannel;					// ͨ��
}FILE_HEADER_INFO;



typedef struct  _FRAME_HEADER_INFO
{
	unsigned long ulDataLen;
	unsigned long ulFrameType;
	unsigned long ulTimeSec;
	unsigned long ulTimeUsec;
	unsigned long ulFrameDataOffset;
	char 		type;						// "V" is video,  "A" is audio
	int    		iChannel;					// ͨ��	
	char 		videoStandard;     // PAL,NTSC
	char 		imageSize;		// CIF , HD1 , D1
	unsigned long ulFrameNumber;
}FRAMEHEADERINFO;




typedef struct _MESSAGE_USE_INFO
{
	int  id;
	int  totalnumber;
}MESSAGEUSEINFO;



typedef struct _ALARM_MESSAGE
{
	int id;					// ���
	int iCam;				// ����ͨ��
	GST_RECORDEVENT event;           // ����ԭ��
	time_t alarmStartTime;		      // ������ʼʱ��
	time_t alarmEndTime;		     // ��������ʱ��
	int   iPreviewTime;            //��ǰʱ�� 
	int   iLaterTime;			//����¼��ʱ��
	int  states;			   //  ¼��״̬, RECORDING ��ʾ����¼��NORECORD ��ʾû��¼��
	int  iDataEffect;            // �Ƿ���Ч
	int  iRecMessageId;     // ָ��RECORDMESSAGE �е�id �ţ�����������¼����Ϣ
	char videoStandard;     // PAL,NTSC
	char imageSize;		// CIF , HD1 , D1
}ALARMMESSAGE;
 


typedef struct _EVENT_MESSAGE
{
	int id;					// ���
	int iCam;				// ����ͨ��
	GST_RECORDEVENT event;           // ����ԭ��
	time_t EventStartTime;		      // ������ʼʱ��
	time_t EvenEndTime;		     // ��������ʱ��
	int   iPreviewTime;            //��ǰʱ�� 
	int   iLaterTime;			//����¼��ʱ��
	EVENTSTATE  CurrentEvent;	   //���ڼ�¼���¼�
	int  iDataEffect;            // �Ƿ���Ч
	int  iRecMessageId;     // ָ��RECORDMESSAGE �е�id �ţ�����������¼����Ϣ
	char EventInfo[30];    // ��ϸ��Ϣ
}EVENTMESSAGE;
 



typedef struct _RECORD_MESSAGE
{
	int id;					// ���	
	time_t startTime;		      // ¼�࿪ʼʱ��
	time_t endTime;		     // ¼�����ʱ��
	int  iDataEffect;            // �Ƿ���Ч
	int  filePos;				// �ļ�ָ�롣
	int  iCam;                        // ��λ��ǵ�¼��ͨ��
	unsigned long ulOffSet;      		// ��Ч�ļ�ƫ��λ�ã�ָ��������Ƶ֡ͷ������
	char videoStandard;     // PAL,NTSC
	char imageSize;		// CIF , HD1 , D1	
	char rec_mode;          //�ֶ�¼�󣬱���¼��
	char have_add_info;  //��û�и�����Ϣ
	unsigned long rec_size; //�˶�¼��Ĵ�С
}RECORDMESSAGE;
typedef struct _REC_ADD_MESSAGE
{
	int rec_log_id;           //ָ��¼���¼�ĵ�
	time_t startTime;  //��ʼʱ��
	int iCam;			     //¼���ͨ��
	unsigned long rec_size;//��һ��¼����ļ���С
	int  filePos;				// �ļ�ָ�롣
	time_t rec_log_time;  
}RECADDMESSAGE;

typedef struct _REC_TIME_STICK_MESSAGE
{
	time_t startTime;  //��ʼʱ��
	int iCam;			     //¼���ͨ��		
	char videoStandard;     // PAL,NTSC
	char imageSize;		// CIF , HD1 , D1	
}RECTIMESTICKMESSAGE;
 


typedef struct _FILE_POS
{
	int iPatitionId;           //  ����ID
	int iCurFilePos;		//  �ļ�ָ�� 
	int iEffect;                 // ���ṹ�Ƿ���Ч,Ϊ��ʱ��ʾ��Ӳ���ϵ�һ���ļ�����δ���
					   // ��Ҫ����޸���
}FILEPOS;



typedef struct _DISK_CURRENT_STATE
{
	int iEnableCover;         // �ܷ񸲸�
	int IsCover;			// �Ƿ����ڸ���
	int iCapacity;              // Ӳ������( M Ϊ��λ)
	int iCoverCapacity;       // ���ڸ�����������̬�õ�����λ����
	int iUseCapacity;        // ������������̬�õ�����λ����ʵ���ǲ���δ�ù���Ӳ��
	int IsRecording;        // �Ƿ�����¼��,�������ж��Ƿ��ô�Ӳ����¼�ࡣ
	int iPartitionId;         //��ǰ����ʹ�õķ���ID.
	int iAllfileCreate;         // �Ƿ��Ӳ���е��ļ���������ϡ�
}DISKCURRENTSTATE;




typedef struct _FILEINFOLOG_POINT
{	
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    iCurFilePos;              			// �ļ�ָ��
	FILEUSEINFO *  pUseinfo;         				// ָ��fileinfo.log �ļ��е�UseInfo
	FILEINFO       *  pFilePos;					// ָ��fileinfo.log �ļ��е�FILEINFO�ṹ
	char 	      *  fileBuf;					// ָ��װ��fileinfo.log�Ļ�������
	FILEINFO  file_info;
	FILEUSEINFO  use_info;
}FILEINFOLOGPOINT;



typedef struct _RECORDLOG_POINT
{	
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    id;              					// ��Ϣ��id��
	MESSAGEUSEINFO *  pUseinfo;         		// ָ��record.log �ļ��е�MESSAGEUSEINFO
	RECORDMESSAGE       *  pFilePos;			// ָ��record.log �ļ��е�RECORDMESSAGE�ṹ
	char 	      *  fileBuf;					// ָ��װ��record.log�Ļ�������
}RECORDLOGPOINT;

typedef struct _REC_ADD_LOG_POINT
{
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    id;              					// ��Ϣ��id��
	MESSAGEUSEINFO *  pUseinfo;         		// ָ��recordadd.log �ļ��е�MESSAGEUSEINFO
	RECADDMESSAGE       *  pFilePos;			// ָ��recordadd.log �ļ��е�RECORDMESSAGE�ṹ
	char 	      *  fileBuf;					// ָ��װ��record.log�Ļ�������
}RECADDLOGPOINT;

typedef struct _REC_TIME_STICK_LOG_POINT
{
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    id;              					// ��Ϣ��id��
	MESSAGEUSEINFO *  pUseinfo;         		// ָ��recordadd.log �ļ��е�MESSAGEUSEINFO
	RECTIMESTICKMESSAGE       *  pFilePos;			// ָ��recordadd.log �ļ��е�RECORDMESSAGE�ṹ
	char 	      *  fileBuf;					// ָ��װ��record.log�Ļ�������	
	RECTIMESTICKMESSAGE file_info;
	MESSAGEUSEINFO use_info;
}RECTIMESTICKLOGPOINT;


typedef struct _ALARMLOG_POINT
{	
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    id;              					// ��Ϣ��id��
	MESSAGEUSEINFO *  pUseinfo;         		// ָ��alarm.log �ļ��е�MESSAGEUSEINFO
	ALARMMESSAGE       *  pFilePos;			// ָ��alarm.log �ļ��е�ALARMLOGPOINT�ṹ
	char 	      *  fileBuf;					// ָ��װ��alarm.log�Ļ�������
}ALARMLOGPOINT;



typedef struct _EVENTLOG_POINT
{	
	int 		    iDiskId;						// Ӳ ��ID
	int 		    iPatitionId;					// ����ID
	int  		    id;              					// ��Ϣ��id��
	MESSAGEUSEINFO *  pUseinfo;         		// ָ��event.log �ļ��е�MESSAGEUSEINFO
	EVENTMESSAGE       *  pFilePos;			// ָ��event.log �ļ��е�ALARMLOGPOINT�ṹ
	char 	      *  fileBuf;					// ָ��װ��event.log�Ļ�������
}EVENTLOGPOINT;



typedef struct _PREVIEWBUF
{
	char * PreviewBufStart;
	long    lBufLength;
	char * pOffSetHead;
	char * pOffSetTail;
}PREVIEWBUF;

// �˽ṹ��Ҫ������¼��ǰ��ָ���ļ������ڲ���������Ϣ��
typedef struct _FILEINFOPOS			
{
	int iDiskId;											
	int iPartitionId;
	int iCurFilePos;	
}FILEINFOPOS;


typedef struct _DISK_FIX_INFO
{
	int   iCreatePartiton;    	//�����ļ��ķ�����
	int   iRecPartiton;		   	 // ¼�� �ļ����ڵķ�����
	int   iCreateCDPartition;     // ��¼cd �ķ�����     
}DISKFIXINFO;

typedef struct _DISK_REC_TIME_
{	
	unsigned long ulRecStartTime;
	unsigned long ulRecEndTime;	
}DISK_REC_TIME;


int FS_TypeCheck(int iDiskId);
int FS_LogCheck(int iDiskId) ;
int FS_PartitionDisk(int iDiskId);
int FS_MountAllPatition(int iDiskId);
int FS_UMountAllPatition(int iDiskId);
int FS_CheckNewDisk(int iDiskId);
int FS_UMountUsb();
int FS_MountUsb();

int FS_WriteUsb(time_t startTime,time_t  endTime,int iCam,int realWrite);
int FS_UsbWriteStatus();

int FS_BuildFileSystem(int iDiskId);
int FS_CreateFileInfoLog(int iDiskId, int id, int * iFileNumber );
int FS_CreateRecordLog(int iDiskId, int iPatitionId);
int FS_CreateAlarmLog(int iDiskId, int iPatitionId);
int FS_CreateDiskInfoLog(int iDiskId);

int FS_InitFileInfoLogBuf();
int FS_LoadFileInfoLog( int iDiskId , int iPatitionId);
FILEINFOLOGPOINT * FS_GetPosFileInfo(int iDiskId, int iPatitionId, int iFilePos,FILEINFOLOGPOINT * input_point);
FILEINFOLOGPOINT * FS_GetFileInfoLogPoint();
int FS_ReleaseFileInfoLogBuf();
int FS_FixFileInfo(int iDiskId, int iPartitionId);
int FS_SearchDate(time_t curTime);

int FS_InitRecordLogBuf();
int FS_ReleaseRecordLogBuf();
int FS_LoadRecordLog( int iDiskId , int iPatitionId);
RECORDLOGPOINT * FS_GetPosRecordMessage(int iDiskId, int iPatitionId, int iFilePos);
RECORDLOGPOINT * FS_GetRecordLogPoint();

int FS_InitRecAddLogBuf();
int FS_ReleaseRecAddLogBuf();
int FS_LoadRecAddLog( int iDiskId , int iPatitionId);
RECADDLOGPOINT * FS_GetPosRecAddMessage(int iDiskId, int iPatitionId, int iFilePos);
RECADDLOGPOINT * FS_GetRecAddLogPoint();

int FS_InitRecTimeStickLogBuf();
int FS_ReleaseRecTimeStickLogBuf();
int FS_LoadRecTimeStickLog( int iDiskId , int iPatitionId);
RECTIMESTICKLOGPOINT * FS_GetPosRecTimeStickMessage(int iDiskId, int iPatitionId, int iFilePos,RECTIMESTICKLOGPOINT * input_point);
RECTIMESTICKLOGPOINT * FS_GetRecTimeStickLogPoint();


int FS_InitAlarmLogBuf();
int FS_ReleaseAlarmLogBuf();
int FS_LoadAlarmLog( int iDiskId , int iPatitionId);
ALARMLOGPOINT *  FS_GetPosAlarmMessage(int iDiskId, int iPatitionId, int iFilePos);
ALARMLOGPOINT * FS_GetAlarmLogPoint();

EVENTLOGPOINT *  FS_GetPosEventMessage(int iDiskId, int iPatitionId, int iFilePos);

int FS_GetDiskCurrentState(int iDiskId, DISKCURRENTSTATE * stDiskState);
int FS_WriteDiskCurrentState(int iDiskId,   DISKCURRENTSTATE * stDiskState);

int FS_CheckRecFile();
int FS_WriteOneFrame(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData);
int FS_GetRecondFileName(char * fileName,int * openNewFile);
int FS_OpenRecFile(GST_PLAYBACK_VIDEO_PARAM  recModeParam,  int iCam,int iNoChangeLog);
int FS_get_disk_remain_rate(int disk_id);
int FS_get_cover_rate(int disk_id);
int FS_GetBadBlockSize(int iDiskId);


int FS_CreateFile();
int FS_StopCreateFile();

int FS_GetOpreateDiskPartition();
int FS_WriteInfoToAlarmLog(GST_RECORDEVENT event,time_t eventStartTime,time_t eventEndTime,int iCam,
									int   iPreviewTime,int   iLaterTime,int  states,int  iRecMessageId);

int FS_WriteDataToUsb(time_t startTime,time_t endTime, int iCam);
int FS_EnableDiskReCover(int iFlag);
int FS_PhysicsFixAllDisk();
int FS_ClearDiskLogMessage(int iDiskId);
int FS_StartCreateFile();
int FS_StopCreateFile();
int FS_OutCreateFileThread();

int FS_UsbBufInit();
int FS_UsbRelease();

int FS_GetErrorMessage(int * iError, time_t * sTime);
int FS_UpdateSoft();
int FS_ListWriteDataToUsb(GST_FILESHOWINFO * stPlayFile);
int FS_ListWriteToUsb(GST_FILESHOWINFO * stPlayFile,int realWrite);

int FS_WriteDataToCdrom(time_t timeStart_1,time_t timeEnd_1,int cam1,
							time_t timeStart_2,time_t timeEnd_2,int cam2,
							time_t timeStart_3,time_t timeEnd_3,int cam3,int iDevice);

long long  FS_GetDataSize(time_t *TimeStart,time_t *TimeEnd,int backup_cam,int cd_mode);
int FS_scan_cdrom();
int FS_WriteCdStatus();
int FS_WriteInfoToEventLog(int * iCam,time_t * eventStartTime,GST_RECORDEVENT * pEvent,
									EVENTSTATE EventStates,char * Reason );

void fs_set_play_mode(int image_size,int standard);
int FS_FormatUsb();
int FS_CheckUsb(int * have_usb,int * format_is_right);

void Net_dvr_stop_client();

extern FILE * rec_head_fp;
extern FILE * rec_data_fp;
extern int g_AlarmMessageId;
extern FILEINFOPOS g_stLogPos;
extern int g_iDiskIsRightFormat[8];
extern time_t g_previous_headtime;
extern unsigned long g_ulFrameHeaderOffset;
extern unsigned long g_ulFileDataOffset;
extern FILEINFOPOS g_stRecFilePos;
extern int gStopCreateFile;
extern GST_PLAYBACK_VIDEO_PARAM g_RecMode;
extern int gUsbWrite;
extern int g_LogChanged;
extern char * g_CreateFileBuf;
extern int g_CurrentFormatReclistReload;
extern int g_CurrentFormatRecAddlistReload;
extern int g_CurrentFormatAlarmlistReload;
extern int g_CurrentFormatFilelistReload;
extern int g_CurrentFormatRecTimeSticklistReload;
extern int g_CurrentFormatDiskRecordTimeReload;
extern int g_CreateFileStatusCurrent;
extern int g_CurrentFormatEventlistReload;
extern int g_writeCdFlag;
extern time_t cdStartTime[3];
extern time_t cdEndTime[3];
extern int cd_cam[3];
extern int g_createFileDiskNum;



#endif
