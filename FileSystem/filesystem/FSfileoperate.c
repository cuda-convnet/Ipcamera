#include <sys/mount.h>
#include <errno.h>
#include "filesystem.h"
#include "FTC_common.h"
#include "DFile.h"
#include "IOFile.h"

DISK_IO_FILE rec_data_filest;
DISK_IO_FILE rec_header_filest;
FILE * rec_head_fp = NULL;
#ifdef USE_DISK_WIRET_DIRECT
int rec_data_fp = 0;
#else
FILE * rec_data_fp = NULL;
#endif

DISK_IO_FILE play_data_filest;
DISK_IO_FILE play_header_filest;
FILE * play_header_fp= NULL;
#ifdef USE_DISK_READ_DIRECT
int  play_data_fp = 0;
#else
FILE * play_data_fp = NULL;
#endif



FILE * fpDataToUsb = NULL;
FILE * fpHeaderToUsb = NULL;
FILE * fpAudioPlay = NULL;

unsigned long g_ulAudioPlayFileOffset = 0;

int g_CreateFileStatusCurrent = 0;

char * g_UsbBuf = NULL;
char * g_CreateFileBuf = NULL;

int g_CurrentError = 0;
time_t g_ErrorTime = 0;

time_t UsbWriteTime = 0;

long long HeaderToUsbOffset = 0;
long long DataToUsbFileOffset = 0;
long long DataToUsbFileOffsetSave = 0;
long long g_UsbAvailableSize = 0;
int cdrom_mode = 0;

time_t curtime,lasttime;

int g_iDiskId = 0,g_iPartitionId = 0;

unsigned long g_ulFileDataOffset = 0; 
unsigned long g_ulFrameHeaderOffset = 0;

unsigned long rec_file_size = 0;

unsigned long ulPlayFrameHeaderOffset = 0;
unsigned long ulPlayFrameHeaderEndOffset = 0;

GST_PLAYBACK_VIDEO_PARAM g_RecMode;
GST_PLAYBACK_VIDEO_PARAM  g_PlayMode;

FILEINFOPOS g_stRecFilePos;
FILEINFOPOS g_stPlayFilePos;

FILEINFOPOS g_stLogPos;

int g_createFileDiskNum = 0;

int g_iDiskIsRightFormat[8]={0,0,0,0,0,0,0,0};

pthread_mutex_t ControlLogMutex;

pthread_mutex_t ControlMutex2;

int gStopCreateFile = 1;
int gUsbWrite = 0;

#define WRITE_BUF_SYS_MAX_NUM (10)
#define WRITE_BUF_SYS_MAX_SAVE_NUM (256*1024)
#define WRITE_BUF_SYS_MAX_DATA_NUM (WRITE_BUF_SYS_MAX_SAVE_NUM+(256*1024))
#define WRITE_BUF_SYS_MAX_HEAD_NUM (50*1024)
#define WRITE_BUF_SYS_BUF_MUST_WRITE (99999)
#define READ_BUF_SYS_MAX_DATA_NUM (512*1024)
#define READ_BUF_SYS_MAX_HEAD_NUM (50*1024)
#define READ_BUF_SYS_MAX_READ_NUM (20)
WRTIE_BUF g_rec_write_buf,g_play_read_buf;

static int g_usb_write_date_size = 0;

int fs_get_usb_write_size()
{
	return g_usb_write_date_size;	
}

void fs_set_play_mode(int image_size,int standard)
{
	g_PlayMode.iPbImageSize = image_size;
	g_PlayMode.iPbVideoStandard = standard;
		
}


int FS_UsbBufInit()
{		
	static int iFirst = 1;
	if( iFirst == 1 )
	{
		g_UsbBuf= (char *)malloc(1024);
		if( !g_UsbBuf)
		{
			printf("g_UsbBuf malloc error!\n");
			return NOENOUGHMEMORY;
		}
		iFirst = 0;
	}

	return ALLRIGHT;
}

int FS_UsbRelease()
{
	if( g_UsbBuf )
	{
		free(g_UsbBuf);
		g_UsbBuf = NULL;
	}
	return ALLRIGHT;
}

int  FS_InitControlLogMutex()
{
	static int iFirst = 1;
	if( iFirst == 1 )
	{
		pthread_mutex_init(&ControlLogMutex,NULL);

		pthread_mutex_init(&ControlMutex2,NULL);
		
		iFirst = 0;
	}
	return ALLRIGHT;
}

int FS_LogMutexLock()
{
	pthread_mutex_lock( &ControlLogMutex);
	return ALLRIGHT;
}

int FS_LogMutexUnLock()
{
	pthread_mutex_unlock( &ControlLogMutex);
	return ALLRIGHT;
}

int FS_PlayMutexLock()
{
	pthread_mutex_lock( &ControlMutex2);
	return ALLRIGHT;
}

int FS_PlayMutexUnLock()
{
	pthread_mutex_unlock( &ControlMutex2);
	return ALLRIGHT;
}

int FS_LogMutexDestroy()
{
	pthread_mutex_destroy( &ControlLogMutex);
	return ALLRIGHT;
}

int  FS_BuildFileSystem(int iDiskId)
{
	int i,j,h,k,z,num;
	int rel = 0;
	char devpatitions[30];
	int iDiskNum;
	char devName[30];
	char cmd[50];
	int iFileNumber = 0;
	FILEINFO stFileInfo;
	GST_DISKINFO stDiskInfo;

	printf("FS_BuildFileSystem mount all\n");
	
	rel = FS_MountAllPatition(iDiskId);

	if( rel != ALLRIGHT )
	{
		printf("MountAllPatition error!\n");
		return iDiskId;
	}

	printf("FS_BuildFileSystem mount over\n");
		
	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	for( j = 1 ; j < stDiskInfo.partitionNum; j++ )
	{
	
		rel = FS_CreateFileInfoLog( iDiskId, j, &iFileNumber);
		if( rel != ALLRIGHT )
		{
			printf(" CreateFileInfoLog error!\n");
			return ERROR;
		}

		
		rel = FS_CreateRecordLog( iDiskId, stDiskInfo.partitionID[j]);
		if( rel != ALLRIGHT )
		{
			printf(" FS_CreateRecordLog error!\n");
			return ERROR;
		}
	
		rel = FS_CreateRecAddLog( iDiskId, stDiskInfo.partitionID[j]);
		if( rel != ALLRIGHT )
		{
			printf(" FS_CreateRecAddLog error!\n");
			return ERROR;
		}	


		rel = FS_CreateRecTimeStickLog( iDiskId, stDiskInfo.partitionID[j]);
		if( rel != ALLRIGHT )
		{
			printf(" FS_CreateRecTimeStickLog error!\n");
			return ERROR;
		}			

		rel = FS_CreateAlarmLog( iDiskId, stDiskInfo.partitionID[j]);	
		if( rel != ALLRIGHT )
		{
			printf(" FS_CreateRecordLog error!\n");
			return ERROR;
		}


		if( stDiskInfo.partitionID[j] == 5 )
		{
			rel = FS_CreateEventLog(iDiskId,stDiskInfo.partitionID[j]);
			if( rel != ALLRIGHT )
			{
				printf(" FS_CreateEventLog error!\n");
				return ERROR;
			}
		}

	}

	rel = FS_CreateDiskInfoLog(iDiskId);

	if( rel != ALLRIGHT )
	{
		printf(" FS_CreateDiskInfoLog error!\n");
		return ERROR;
	}

	
	rel = FS_UMountAllPatition(iDiskId);

	if( rel != ALLRIGHT )
	{
		printf(" UMountAllPatition error!\n");
		return ERROR;
	}

	end_format();

	return ALLRIGHT;
	
}

int FS_CreateFileInfoLog(int iDiskId, int id, int * iFileNumber )
{
	int i,j,h,k,z,num;
	int rel = 0;
	char devpatitions[30];
	int iDiskNum;
	char devName[30];
	char cmd[50];
	FILE * fp = NULL;
	FILE * fp1 = NULL;
	char fileNameBuf[50];
	int iFileGroup = 0;
	FILEINFO stFileInfo;
	FILEUSEINFO stDiskUse;
	GST_DISKINFO stDiskInfo;
	int fd;
	


	stDiskInfo = DISK_GetDiskInfo(iDiskId);


	#ifdef USE_EXT3

		#ifdef USE_FILE_IO_SYS			
		if( id == 1 )	
		{
			long long available_size = 0;			

			sprintf(devName,"/dev/%s%d",&stDiskInfo.devname[5], stDiskInfo.partitionID[id+1]);

			available_size = disk_io_check_disk_size(devName);

			//这种检测方法大概有1G的误差。所以加1
			available_size = available_size + 1;  

			available_size = available_size*1024*1024;	

			DPRINTK(" %lld  %lld  compare\n",available_size,stDiskInfo.partitionSize[id+1]);

			if( available_size <  stDiskInfo.partitionSize[id+1])
			{			
				set_disk_is_invalid(devName);				
				check_io_file_version(devName);
				SendM2("reboot");
				sleep(10);
			
			}else
			{			
				iFileGroup =(stDiskInfo.partitionSize[id+1] - (stDiskInfo.partitionSize[id+1]/100) )  / MAXFILESIZE;
				DPRINTK("PSize=%lld  %lld\n",stDiskInfo.partitionSize[id+1],
					(stDiskInfo.partitionSize[id+1] - (stDiskInfo.partitionSize[id+1]/100) ));		
			}
		}
		#else

			if( id == 1 )
			iFileGroup =((stDiskInfo.partitionSize[id] - (stDiskInfo.partitionSize[id]/100) ) - 4*1024 * 1024) / MAXFILESIZE;
			else
			iFileGroup =(stDiskInfo.partitionSize[id] - (stDiskInfo.partitionSize[id]/100) )  / MAXFILESIZE;

			DPRINTK("PSize=%lld  %lld\n",stDiskInfo.partitionSize[id],
				(stDiskInfo.partitionSize[id] - (stDiskInfo.partitionSize[id]/100) ));			
		#endif	

	#else
	
	if( id == 1 )
		iFileGroup =(stDiskInfo.partitionSize[id] - 4*1024 * 1024) / MAXFILESIZE;
	else
		iFileGroup =stDiskInfo.partitionSize[id]  / MAXFILESIZE;

		DPRINTK("PSize=%lld  \n",stDiskInfo.partitionSize[id]);
		
	#endif

	iFileGroup = iFileGroup -5;

	printf(" id = %d iFileGroup = %d\n",id,iFileGroup);
	
	sprintf(fileNameBuf,"/tddvr/%s%d/fileinfo.log", 
			&stDiskInfo.devname[5], stDiskInfo.partitionID[id]); 

	

	fp1 = fopen(fileNameBuf,"w+b");
	if( !fp1 )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	stDiskUse.iCurFilePos = 0;
	stDiskUse.iTotalFileNumber = iFileGroup;

	rel = fwrite(&stDiskUse, 1, sizeof(FILEUSEINFO), fp1);
	if( rel != sizeof(FILEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp1)
				fclose(fp1);

			return FILEERROR;
	}
		

	for( h = 0; h < iFileGroup; h++ )
	{
			
		sprintf(fileNameBuf,"/tddvr/%s%d/video_%d", &stDiskInfo.devname[5]
					, stDiskInfo.partitionID[id], *iFileNumber);
		
				memset(&stFileInfo, 0 ,sizeof(FILEINFO));

				sprintf(fileNameBuf,"video_%d", *iFileNumber);

				strcpy( stFileInfo.fileName, fileNameBuf);		
			
				stFileInfo.states       = NOWRITE;
				stFileInfo.StartTime = 0;
				stFileInfo.EndTime   = 0;
				stFileInfo.iDataEffect = NOEFFECT;
				stFileInfo.iEnableReWrite = YES;
				stFileInfo.ulOffSet   = 0;
				stFileInfo.isCover   = NO;
	
				num = fwrite(&stFileInfo, 1, sizeof(FILEINFO), fp1);

				if( num != sizeof(FILEINFO) )
				{
					printf(" write %s error!\n", fileNameBuf);			
					fclose(fp1);
					return FILEERROR;
				}

				fflush(fp1);				
				
			     *iFileNumber = *iFileNumber + 1;
			
		}
		
		
		fclose(fp1);


		sprintf(fileNameBuf,"/dev/%s6",&stDiskInfo.devname[5]); 


		iFileGroup =  iFileGroup * 128;

		if( format_disk_to_io_sys(fileNameBuf,iFileGroup) < 0 )
			return FILEERROR;
		

		return ALLRIGHT;
	
	
}


int FS_CreateRecordLog(int iDiskId, int iPatitionId)
{
	char fileNameBuf[50];
	FILE * fp = NULL;
	GST_DISKINFO stDiskInfo;
	MESSAGEUSEINFO  useInfo;
	RECORDMESSAGE recordMsg;

	int rel,i;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	sprintf(fileNameBuf,"/tddvr/%s%d/record.log", 
			&stDiskInfo.devname[5], iPatitionId); 

	fp = fopen(fileNameBuf,"w+b");
	if( !fp )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	useInfo.id = 0;
	useInfo.totalnumber = MAXMESSAGENUMBER;

	rel = fwrite(&useInfo, 1, sizeof(MESSAGEUSEINFO), fp);

	if( rel != sizeof(MESSAGEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			
			return iDiskId;
	}

	for( i = 0; i <MAXMESSAGENUMBER;i++)
	{
		recordMsg.endTime = 0;
		recordMsg.id		 = i;
		recordMsg.iDataEffect = NOEFFECT;
		recordMsg.startTime = 0;
		recordMsg.ulOffSet   = 0;
		recordMsg.iCam	= 0;
		recordMsg.filePos   = -1;
		recordMsg.imageSize = 0;
		recordMsg.videoStandard = 0;
		recordMsg.rec_size = 0;
		recordMsg.have_add_info = 0;
		recordMsg.rec_mode = -1;

		rel = fwrite(&recordMsg, 1, sizeof(RECORDMESSAGE), fp);

		if( rel != sizeof(RECORDMESSAGE) )
		{
			printf("2 write %s error!\n", fileNameBuf);		
			fclose(fp);			
			return iDiskId;
		}
		fflush(fp);
	}

	fclose(fp);

	return ALLRIGHT;	
}


int FS_CreateRecAddLog(int iDiskId, int iPatitionId)
{
	char fileNameBuf[50];
	FILE * fp = NULL;
	GST_DISKINFO stDiskInfo;
	MESSAGEUSEINFO  useInfo;
	RECADDMESSAGE recordMsg;

	int rel,i;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	sprintf(fileNameBuf,"/tddvr/%s%d/recordadd.log", 
			&stDiskInfo.devname[5], iPatitionId); 

	fp = fopen(fileNameBuf,"w+b");
	if( !fp )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	useInfo.id = 0;
	useInfo.totalnumber = MAXADDMESSAGENUMBER;

	rel = fwrite(&useInfo, 1, sizeof(MESSAGEUSEINFO), fp);

	if( rel != sizeof(MESSAGEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			
			return iDiskId;
	}

	for( i = 0; i <MAXADDMESSAGENUMBER;i++)
	{
		recordMsg.rec_log_id = -1;
		recordMsg.iCam = 0;
		recordMsg.rec_size = 0;
		recordMsg.startTime = 0;
		recordMsg.filePos = -1;
		recordMsg.rec_log_time =0;
		

		rel = fwrite(&recordMsg, 1, sizeof(RECADDMESSAGE), fp);

		if( rel != sizeof(RECADDMESSAGE) )
		{
			printf("2 write %s error!\n", fileNameBuf);		
			fclose(fp);			
			return iDiskId;
		}
		fflush(fp);
	}

	fclose(fp);

	return ALLRIGHT;	
}


int FS_CreateRecTimeStickLog(int iDiskId, int iPatitionId)
{
	char fileNameBuf[50];
	FILE * fp = NULL;
	GST_DISKINFO stDiskInfo;
	MESSAGEUSEINFO  useInfo;
	RECTIMESTICKMESSAGE recordMsg;

	int rel,i;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	sprintf(fileNameBuf,"/tddvr/%s%d/recordtime.log", 
			&stDiskInfo.devname[5], iPatitionId); 

	fp = fopen(fileNameBuf,"w+b");
	if( !fp )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	useInfo.id = 0;
	useInfo.totalnumber = MAXTIMESTICKMESSAGENUMBER;

	rel = fwrite(&useInfo, 1, sizeof(MESSAGEUSEINFO), fp);

	if( rel != sizeof(MESSAGEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			
			return iDiskId;
	}

	for( i = 0; i <MAXTIMESTICKMESSAGENUMBER;i++)
	{
		
		recordMsg.iCam = 0;		
		recordMsg.startTime = 0;		

		rel = fwrite(&recordMsg, 1, sizeof(RECTIMESTICKMESSAGE), fp);

		if( rel != sizeof(RECTIMESTICKMESSAGE) )
		{
			printf("2 write %s error!\n", fileNameBuf);		
			fclose(fp);			
			return iDiskId;
		}
		fflush(fp);
	}

	fclose(fp);

	return ALLRIGHT;	
}


int FS_CreateAlarmLog(int iDiskId, int iPatitionId)
{
	char fileNameBuf[50];
	FILE * fp = NULL;
	GST_DISKINFO stDiskInfo;
	MESSAGEUSEINFO  useInfo;
	ALARMMESSAGE     alarmMsg;

	int rel,i;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	sprintf(fileNameBuf,"/tddvr/%s%d/alarm.log", 
			&stDiskInfo.devname[5], iPatitionId); 

	fp = fopen(fileNameBuf,"w+b");
	if( !fp )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	useInfo.id = 0;
	useInfo.totalnumber = MAXMESSAGENUMBER;

	rel = fwrite(&useInfo, 1, sizeof(MESSAGEUSEINFO), fp);

	if( rel != sizeof(MESSAGEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			
			return iDiskId;
	}

	for( i = 0; i <MAXMESSAGENUMBER;i++)
	{		
		alarmMsg.alarmEndTime = 0;
		alarmMsg.iCam         = 0;
		alarmMsg.id		   = i;
		alarmMsg.iDataEffect = NOEFFECT;
		alarmMsg.alarmStartTime = 0;
		alarmMsg.event.sensor= 0;
		alarmMsg.event.loss = 0;
		alarmMsg.event.motion = 0;
		alarmMsg.iRecMessageId = -1;
		alarmMsg.states             =  NORECORD;
		alarmMsg.imageSize      = 0;
		alarmMsg.videoStandard = 0;


		rel = fwrite(&alarmMsg, 1, sizeof(ALARMMESSAGE), fp);

		if( rel != sizeof(ALARMMESSAGE) )
		{
			printf("2 write %s error!\n", fileNameBuf);		
			fclose(fp);			
			return iDiskId;
		}
		fflush(fp);
	}

	fclose(fp);

	return ALLRIGHT;	
}

int FS_CreateEventLog(int iDiskId, int iPatitionId)
{
	char fileNameBuf[50];
	FILE * fp = NULL;
	GST_DISKINFO stDiskInfo;
	MESSAGEUSEINFO  useInfo;
	EVENTMESSAGE     eventMsg;

	int rel,i;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	sprintf(fileNameBuf,"/tddvr/%s%d/event.log", 
			&stDiskInfo.devname[5], iPatitionId); 

	fp = fopen(fileNameBuf,"w+b");
	if( !fp )
	{
			printf(" can't create %s\n", fileNameBuf);
			return FILEERROR;
	}

	useInfo.id = 0;
	useInfo.totalnumber = EVENTMAXMESSAGE;

	rel = fwrite(&useInfo, 1, sizeof(MESSAGEUSEINFO), fp);

	if( rel != sizeof(MESSAGEUSEINFO ) )
	{
			printf(" write %s error!\n",fileNameBuf);

			if( fp)
			fclose(fp);
			
			return iDiskId;
	}

	for( i = 0; i <EVENTMAXMESSAGE;i++)
	{		
		eventMsg.CurrentEvent = NORMAL;
		eventMsg.EvenEndTime = 0;
		eventMsg.event.loss = 0;
		eventMsg.event.motion = 0;
		eventMsg.event.sensor = 0;
		eventMsg.EventStartTime = 0;
		eventMsg.iCam = 0;
		eventMsg.id = i;
		eventMsg.iDataEffect = NOEFFECT;
		eventMsg.iLaterTime = 0;
		eventMsg.iPreviewTime = 0;
		eventMsg.iRecMessageId = -1;

		rel = fwrite(&eventMsg, 1, sizeof(EVENTMESSAGE), fp);

		if( rel != sizeof(EVENTMESSAGE) )
		{
			printf("2 write %s error!\n", fileNameBuf);		
			fclose(fp);			
			return iDiskId;
		}
		fflush(fp);
	}

	fclose(fp);

	return ALLRIGHT;	
}

int FS_CreateDiskInfoLog(int iDiskId)
{
	int i;
	int rel = 0;
	int iDiskNum;
	FILE * fp = NULL;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	DISKCURRENTSTATE  stDiskState;
	

	DISK_GetAllDiskInfo();     // get all disk information.

	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/diskinfo.log",&stDiskInfo.devname[5]);

	fp = fopen(fileNameBuf, "w+b");

	if( fp == NULL )
	{
			printf(" open %s error!\n", fileNameBuf);
			return iDiskId;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	stDiskInfo.cylinder = g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video(); 
	
	rel = fwrite(&stDiskInfo, 1, sizeof( GST_DISKINFO), fp);

	if( rel != sizeof( GST_DISKINFO ) )
	{
			printf(" write %s error!!!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			return iDiskId;
	}


	stDiskState.iCapacity = stDiskInfo.size /( 1024*1024 );
	stDiskState.iCoverCapacity = 0;
	stDiskState.iEnableCover    = ENABLE;
	stDiskState.IsCover            = NO;
	stDiskState.IsRecording      =  NO;
	stDiskState.iUseCapacity    =  0;
	stDiskState.iPartitionId	     = 5;
	stDiskState.iAllfileCreate    =  NO;

	rel = fwrite(&stDiskState, 1, sizeof( DISKCURRENTSTATE), fp);

	if( rel != sizeof(DISKCURRENTSTATE ) )
	{
			printf("2 write %s error!!!\n",fileNameBuf);

			if( fp)
				fclose(fp);
			return iDiskId;
	}
	fflush(fp);
	
	fclose(fp);

	return ALLRIGHT;
}

int fs_open_record_file(char * file_name,int reopen)
{
	#ifdef USE_FILE_IO_SYS
		char whole_file_name[50];
		char fileName[50];
		char dev[50];	

		memset(fileName, 0 , 50 );
		memset(dev, 0 , 50 );
		strncpy(fileName,&file_name[6],4);
		sprintf(dev,"/dev%s6",fileName);
		strcpy(whole_file_name,&file_name[12]);

		if( reopen == 1 )
		{
			DPRINTK("cover open new file\n");
		
			sprintf(fileName,"%s.avh",whole_file_name);

			if( file_io_open(dev,fileName,"w",&rec_header_filest,0) < 0 )
			{
				printf("can't open %s %s\n",dev,fileName);
				goto  OPENERROR;;
			}
			
			sprintf(fileName,"%s.avd",whole_file_name);	

			if( file_io_open(dev,fileName,"w",&rec_data_filest,0) < 0 )
			{
				printf("can't open %s %s\n",dev,fileName);
				goto  OPENERROR;;
			}		

			return ALLRIGHT;		
		}
	

		sprintf(fileName,"%s.avh",whole_file_name);

		if( file_io_open(dev,fileName,"+",&rec_header_filest,0) < 0 )
		{
			printf("can't open %s %s\n",dev,fileName);
			goto  OPENERROR;;
		}
		
		sprintf(fileName,"%s.avd",whole_file_name);	

		if( file_io_open(dev,fileName,"+",&rec_data_filest,0) < 0 )
		{
			printf("can't open %s %s\n",dev,fileName);
			goto  OPENERROR;;
		}	

		return ALLRIGHT;

		OPENERROR:
			
		file_io_close(&rec_header_filest);
		file_io_close(&rec_data_filest);

		return FILEERROR;

	#else
		char whole_file_name[50];

		if( reopen == 1 )
		{
			DPRINTK("cover open new file\n");

			sprintf(whole_file_name,"%s.avh",file_name);		
			
			rec_head_fp = fopen(whole_file_name,"w+b");
			if( rec_head_fp == NULL )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}	
			
			sprintf(whole_file_name,"%s.avd",file_name);		

			#ifdef USE_DISK_WIRET_DIRECT
			rec_data_fp = DF_Open(whole_file_name,"w+b");
			if( rec_data_fp == -1 )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}		

			#else
			rec_data_fp = fopen(whole_file_name,"w+b");
			if( rec_data_fp == NULL )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}		
			#endif

			return ALLRIGHT;		
		}

		sprintf(whole_file_name,"%s.avh",file_name);
			
		rec_head_fp = fopen( whole_file_name, "r+b" );

		if ( rec_head_fp == NULL )
		{
			rec_head_fp = fopen(whole_file_name,"w+b");
			if( rec_head_fp == NULL )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}					
		}

		sprintf(whole_file_name,"%s.avd",file_name);

		#ifdef USE_DISK_WIRET_DIRECT
		rec_data_fp = DF_Open( whole_file_name, "r+b" );

		if ( rec_data_fp == -1 )
		{
			rec_data_fp = DF_Open(whole_file_name,"w+b");
			if( rec_data_fp == -1 )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}						
		}

		#else

		rec_data_fp = fopen( whole_file_name, "r+b" );

		if ( rec_data_fp == NULL )
		{
			rec_data_fp = fopen(whole_file_name,"w+b");
			if( rec_data_fp == NULL )
			{
				printf(" create %s error!\n",whole_file_name);
				goto OPENERROR;
			}						
		}

		#endif

		return ALLRIGHT;

		OPENERROR:
		if( rec_head_fp )
		{
			fclose(rec_head_fp);
			rec_head_fp = NULL;
		}

		#ifdef USE_DISK_WIRET_DIRECT
		
		if( rec_data_fp > 0 )
		{
			DF_Close(rec_data_fp);
			rec_data_fp = 0;
		}

		#else

		if( rec_data_fp )
		{
			fclose(rec_data_fp);
			rec_data_fp = NULL;
		}

		#endif	

		return FILEERROR;

	#endif
}

// 打开录相文件并修改log 文件。
int FS_OpenRecFile(GST_PLAYBACK_VIDEO_PARAM  recModeParam,  int iCam,int iNoChangeLog)
{
	int iDiskNum = 0;
	int i,j,rel;
	char fileTempName[50];
	int opennewfile = 0;
	

	FRAMEHEADERINFO   stFrameHeaderInfo;
	DISKCURRENTSTATE   stDiskState;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT  fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	struct timeval stTimeVal;
	time_t  recTime;	
	int my_data1 = 0;

	g_RecMode = recModeParam;

	if( g_RecMode.iPbImageSize>TD_DRV_VIDEO_SIZE_CIF||
		g_RecMode.iPbImageSize <  TD_DRV_VIDEO_SIZE_D1 )
	{
		printf(" rec mode wront,ImageSize =%d\n",g_RecMode.iPbImageSize);
		return ERROR;
	}

	if( g_RecMode.iPbVideoStandard>TD_DRV_VIDEO_STARDARD_NTSC   ||
		g_RecMode.iPbVideoStandard <TD_DRV_VIDEO_STARDARD_PAL  )
	{
		printf(" rec mode wront,ImageSize =%d\n",g_RecMode.iPbImageSize);
		return ERROR;
	}


// 查找可用文件。

RE_TRY:

	memset(fileTempName,0,50);

	rel = FS_GetRecondFileName(fileTempName,&opennewfile);

	if( rel < 0 )
	{
		// 各种异常情况.

		DPRINTK(" rel = %d\n",rel);
		while(1 )
		{
			// 当前磁盘用完了，并且可以进行覆盖操作
			if( rel == ALLDISKUSEOVER )
			{
				rel = FS_GetRecondFileName(fileTempName,&opennewfile);
				if( rel > 0 )
					break;
			}

			//当前磁盘用完了，但没有磁盘可以进行覆盖操作
			if( rel == ALLDISKNOTENABLETOWRITE)
			{
				FS_SetErrorMessage(ALLDISKNOTENABLETOWRITE, 0);

				DPRINTK("Send ALLDISKNOTENABLETOWRITE info,need reboot nvr! wait 300 sec!\n");
				sleep(300);
				exit(0);
				return ALLDISKNOTENABLETOWRITE;
			}

			//返回的是异常错误，有待处理
			if( rel != ALLDISKUSEOVER &&  rel != ALLDISKNOTENABLETOWRITE
				&& rel != FILESTATEERROR )
			{				
	//			FS_SetErrorMessage(GETRECORDFILEUNKNOWERROR, 0);
				return rel;
			}

			rel = FS_GetRecondFileName(fileTempName,&opennewfile);
			if( rel ==ALLRIGHT )
			{
				break;
			}
			
		}				
		
	}

/*
	rel = FS_RecFileCurrentPartitionWriteToShutDownLog(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId);
	if( rel < 0)
	{
		printf(" FS_CreateFileCurrentPartitionWriteToShutDownLog error!\n");					
		return FILEERROR;
	}		*/			

	printf(" fileTempName %s\n",fileTempName);

	rel = fs_open_record_file(fileTempName,opennewfile);
	if( rel < 0 )
		return rel;	

	pFileInfoPoint = FS_GetPosFileInfo(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId,g_stRecFilePos.iCurFilePos,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	g_pstCommonParam->GST_DRA_get_sys_time( &stTimeVal, NULL );	

	printf(" rec start rec is %ld\n",stTimeVal.tv_sec);

	// 录相文件正常打开，将所有标志位修改填好。
	// 文件状态为未使用的新文件，打开文件后马上修改fileinfo 信息。
	if( pFileInfoPoint->pFilePos->states == NOWRITE )
	{
		// 修改fileinfo 信息，并写入fileinfo.log
		// 开始时间 为0 表示此为新文件。
		
		pFileInfoPoint->pFilePos->iDataEffect = NO;
		pFileInfoPoint->pFilePos->states 		= WRITING;
		pFileInfoPoint->pFilePos->ulOffSet	= sizeof(FILEINFO);
		pFileInfoPoint->pFilePos->StartTime   = stTimeVal.tv_sec;
		pFileInfoPoint->pFilePos->EndTime     = 0; 
		pFileInfoPoint->pFilePos->ulVideoSize = g_RecMode.iPbImageSize;
		pFileInfoPoint->pFilePos->mode = g_RecMode.iPbVideoStandard;

		//如果g_previous_headtime不为0 表示，这是预录相 
		if( g_previous_headtime != 0 )
		{
			printf(" g_previous_headtime = %ld\n",g_previous_headtime);
			pFileInfoPoint->pFilePos->StartTime   = g_previous_headtime;
		}

		printf("start time %ld\n",pFileInfoPoint->pFilePos->StartTime);
		rel = FS_FileInfoWriteToFile(pFileInfoPoint);
		if( rel < 0 )
		{
			printf("FS_FileInfoWriteToFile error!\n");
			return FILEERROR;
		}			


		{
				int tmp;

				tmp = pFileInfoPoint->pFilePos->ulVideoSize;

				pFileInfoPoint->pFilePos->ulVideoSize = 
					g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video();
				
				rel = file_io_write(&rec_header_filest,pFileInfoPoint->pFilePos,sizeof(FILEINFO));

				pFileInfoPoint->pFilePos->ulVideoSize = tmp;

				if( rel != sizeof(FILEINFO) )
				{			
					DPRINTK("write file error!\n");
					return FILEERROR;
				}	

				
				file_io_sync(&rec_header_filest);

				DPRINTK("pos=%d  filelenght=%d\n",file_io_file_get_pos(&rec_header_filest),
					file_io_file_get_length(&rec_header_filest));
	

		}
		

		g_ulFrameHeaderOffset = sizeof(FILEINFO);
		g_ulFileDataOffset          = 0;
		
							
	}

	// 这里的文件状态是不未写完的录相文件，那将它结束时间
	// 摸去，状态改好。
	if( pFileInfoPoint->pFilePos->states == WRITEOVER )
	{
	//	pFileInfoPoint->pFilePos->iDataEffect = NO;
	//	pFileInfoPoint->pFilePos->EndTime     = 0; 
		pFileInfoPoint->pFilePos->states 		= WRITING;
		
		rel = FS_FileInfoWriteToFile(pFileInfoPoint);
		if( rel < 0 )
		{
			printf("FS_FileInfoWriteToFile error!\n");
			return FILEERROR;
		}	

		g_ulFrameHeaderOffset = pFileInfoPoint->pFilePos->ulOffSet;

		printf(" g_ulFrameHeaderOffset = %ld\n",g_ulFrameHeaderOffset);	

		
		my_data1 = (g_ulFrameHeaderOffset - 2 * sizeof(FRAMEHEADERINFO))-sizeof(FILEINFO);

		DPRINTK(" g_ulFrameHeaderOffset - 2 * sizeof(FRAMEHEADERINFO1) = %d  %d  %d\n",
			g_ulFrameHeaderOffset - 2 * sizeof(FRAMEHEADERINFO),sizeof(FILEINFO),my_data1);		 


		if( my_data1 < 0 )
		{			
			DPRINTK("file is bad,write new\n");
			pFileInfoPoint->pFilePos->states 		= NOWRITE;
			
			rel = FS_FileInfoWriteToFile(pFileInfoPoint);
			if( rel < 0 )
			{
				printf("FS_FileInfoWriteToFile error!\n");
				return FILEERROR;
			}	

			goto RE_TRY;

			return FILEERROR;
		}

		if( file_io_lseek(&rec_header_filest,  g_ulFrameHeaderOffset -1 * sizeof(FRAMEHEADERINFO) , SEEK_SET ) == (off_t)-1 )
		{
			printf("  fseek error!\n");
			file_io_close(&rec_header_filest);
			file_io_close(&rec_data_filest);
			return FILEERROR;
		}		

		printf(" seek %ld\n",g_ulFrameHeaderOffset -1 * sizeof(FRAMEHEADERINFO));

		rel = file_io_read(&rec_header_filest,&stFrameHeaderInfo,sizeof(FRAMEHEADERINFO));
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			printf("fread error! rel =%d readsize=%d\n",rel,sizeof(FRAMEHEADERINFO));
			file_io_close(&rec_header_filest);
			file_io_close(&rec_data_filest);
			return FILEERROR;
		}
		
		g_ulFileDataOffset = stFrameHeaderInfo.ulFrameDataOffset + stFrameHeaderInfo.ulDataLen;

		#ifdef USE_DISK_WIRET_DIRECT
		DPRINTK("g_ulFileDataOffset %ld is not right pos change\n",g_ulFileDataOffset);
		g_ulFileDataOffset =  (g_ulFileDataOffset + DF_PAGESIZE - 1)/DF_PAGESIZE*DF_PAGESIZE;

		#endif

		printf(" g_ulFrameHeaderOffset %ld g_ulFileDataOffset %ld \n",
			g_ulFrameHeaderOffset, g_ulFileDataOffset);

		
		
		
	}
	
	if( iNoChangeLog == 1 )
	{
		if( g_previous_headtime != 0)
			recTime = g_previous_headtime;
		else
			recTime = stTimeVal.tv_sec;
	
		// 还要修改record.log 和alarm.log.
		rel = FS_WriteStartInfoToRecLog(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId,
				recTime,g_stRecFilePos.iCurFilePos,iCam, g_ulFrameHeaderOffset);

		if( rel < 0 )	
		{
			printf(" FS_WriteStartInfoToRecLog error!\n");
			return rel;
		}

		printf(" Rec log start write %ld\n",recTime);

		g_stLogPos = g_stRecFilePos;		
	}	


	return ALLRIGHT;


	
}

int FS_GetRecondFileName(char * fileName,int * openNewFile)
{
	int iDiskNum = 0;
	int i,j,rel,k;
	
	DISKCURRENTSTATE   stDiskState;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT	fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	struct timeval stTimeVal;
	FILEUSEINFO stFileinfo;
	struct timeval timetv;
	static int start_first_file_skip = 0;
	

	// 查找可用文件。
	iDiskNum = DISK_GetDiskNum();	
	for( i = 0; i< iDiskNum; i++ )
	{		
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Sleep(i) == 0 )
		{
			DPRINTK("disk=%d is sleep!\n",i);
			continue;
		}
		#endif
	
		rel = FS_GetDiskCurrentState(i, &stDiskState);
		if( rel < 0 )
			return rel;		

		printf(" isrecording  %d, iUseCpapcity  %d  i=%d\n",stDiskState.IsRecording, stDiskState.iUseCapacity,i);

		// 判断当前要操作的硬盘是不是这个硬盘。
		// isRecording == NO iUseCpapacity == 0 时，表示此为未使用过的硬盘，让它往下操作。
		if( stDiskState.IsRecording == NO && stDiskState.iUseCapacity == 0)
		{
			// 修改标志位，写入diskinfo.log.
			stDiskState.IsRecording = YES;
			stDiskState.iUseCapacity = 1;
			rel = FS_WriteDiskCurrentState(i, &stDiskState);

			//printf("2  isrecording  %d, iUseCpapcity  %d\n",stDiskState.IsRecording, stDiskState.iUseCapacity);
			
			if( rel < 0 )
				return rel;			
		}
		
		//标志位为NO说明不用此硬盘(或此硬盘已用完)
		if( stDiskState.IsRecording == NO)
			continue;		
		
		rel = FS_GetFileUseInfo(i, stDiskState.iPartitionId, &stFileinfo);
		if( rel < 0 )
				return rel;
		
		// 得到文件信息，判断要操作的文件是否在这个盘。
		pFileInfoPoint  = FS_GetPosFileInfo( i , stDiskState.iPartitionId, stFileinfo.iCurFilePos,&fileInfoPoint);
		if(pFileInfoPoint == NULL)
		{
			printf(" FS_GetPosFileInfo error!\n");
			return ERROR;
		}

		DPRINTK(" curfilepos %d stats = %d\n",pFileInfoPoint->pUseinfo->iCurFilePos,pFileInfoPoint->pFilePos->states );

			// 在这里会遇到几种情况。
			// 正常情况，下一个文件可用。		
			// 此硬盘的文件还没有全用完，可以接着用。
		if( pFileInfoPoint->pUseinfo->iCurFilePos < pFileInfoPoint->pUseinfo->iTotalFileNumber )
		{
			// 判断这个录相文件是否写完，写完就要将文件指针指向下一个。
			if( pFileInfoPoint->pFilePos->states != NOWRITE && 
						pFileInfoPoint->pFilePos->states != WRITEOVER)
			{
				//将iCurFilePos 加1 指向下一个文件。

			//	printf(" exp stats = %d\n",pFileInfoPoint->pFilePos->states);
FILESTATEWRONG:

				// 这里表示录相覆盖到了一个正在回放的文件
				if( stDiskState.IsCover == YES && pFileInfoPoint->pFilePos->states == PLAYING )
				{
					pFileInfoPoint->pFilePos->iDataEffect = NOEFFECT;
					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" UseInfoWriteToFile error!");
						return rel;
					}	

					rel =FS_CoverFileClearLog(pFileInfoPoint->pFilePos->StartTime,
							pFileInfoPoint->pFilePos->EndTime, -1);
					if( rel < 0 )
					{
						printf(" FS_CoverFileClearLog error!");
						return rel;
					}						
				}
		

				pFileInfoPoint->pUseinfo->iCurFilePos += 1;
				rel = UseInfoWriteToFile(pFileInfoPoint);
				if( rel < 0 )
				{
					printf(" UseInfoWriteToFile error!");
					return rel;
				}
		//		printf(" ok exp! pos = %d\n",pFileInfoPoint->pUseinfo->iCurFilePos );

				// 换分区情况.
				if( pFileInfoPoint->pUseinfo->iCurFilePos >= pFileInfoPoint->pUseinfo->iTotalFileNumber)
				{
					pFileInfoPoint->pUseinfo->iCurFilePos = 0;
					// 写入fileinfo.log.
					rel = UseInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" UseInfoWriteToFile error!");
						return rel;
					}

					// 指向下一个分区			
					stDiskState.iPartitionId += 1;
					rel = FS_WriteDiskCurrentState(i, &stDiskState);
					if( rel < 0 )
						return rel;
							
					// 换硬盘情况。
					stDiskInfo = DISK_GetDiskInfo(i);

					// 已经是最后一个分区了，要换硬盘了。
					if( stDiskState.iPartitionId > stDiskInfo.partitionID[stDiskInfo.partitionNum - 1] )
					{
						stDiskState.iPartitionId = 5;
						stDiskState.IsRecording = NO;

						// 写入diskinfo.log
						rel = FS_WriteDiskCurrentState(i, &stDiskState);
						if( rel < 0 )
							return rel;

						// 还要将下一个硬盘的状态调对。

						// 换硬盘情况。
								
						// 所有硬盘都用完了，要进行覆盖操作。

					}
							
				}
																						
					
				return FILESTATEERROR;
			}			

			// 覆盖处理。
			if(stDiskState.IsCover == YES && pFileInfoPoint->pFilePos->states == WRITEOVER) 
			{

				if( stDiskState.iEnableCover != ENABLE )
				{						
					DPRINTK("iEnableCover=%d disk=%d\n",stDiskState.iEnableCover,i);
					return ALLDISKNOTENABLETOWRITE;
				}
			
				if( pFileInfoPoint->pFilePos->isCover == NO)
				{
					rel =FS_CoverFileClearLog(pFileInfoPoint->pFilePos->StartTime,
							pFileInfoPoint->pFilePos->EndTime, -1);
					if( rel < 0 )
					{
						printf(" FS_CoverFileClearLog error!");
						//return rel;
					}						
				
					pFileInfoPoint->pFilePos->states  = NOWRITE;
					pFileInfoPoint->pFilePos->isCover = YES;
					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" UseInfoWriteToFile error!");
						return rel;
					}	

					
				}
			}

			// 用户将时间调前了，就打开一个新的文件
			// 再录相
			if( pFileInfoPoint->pFilePos->states == WRITEOVER )
			{				
				g_pstCommonParam->GST_DRA_get_sys_time( &timetv, NULL );

				if( (timetv.tv_sec  < pFileInfoPoint->pFilePos->StartTime) || 
					 (timetv.tv_sec  < pFileInfoPoint->pFilePos->EndTime) ||
					 ( (timetv.tv_sec-3600)  > pFileInfoPoint->pFilePos->EndTime )||
					 (start_first_file_skip == 0) )
				{
					if( stDiskState.IsCover == YES &&  pFileInfoPoint->pFilePos->isCover == YES)
					{
						pFileInfoPoint->pFilePos->isCover = NO;
					
						rel = FS_FileInfoWriteToFile(pFileInfoPoint);
						if( rel < 0 )
						{
							printf(" UseInfoWriteToFile error! 918");
							return rel;
						}	
						printf(" file cover is to NO!\n"); 
					}


					DPRINTK("now time=%ld  filetime:(%ld,%ld,%ld) \n",
					timetv.tv_sec,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime,(timetv.tv_sec-60));

					if( (timetv.tv_sec  < pFileInfoPoint->pFilePos->StartTime) || 
					 (timetv.tv_sec  < pFileInfoPoint->pFilePos->EndTime) )
					{
						DPRINTK("rec time < file time\n");
					}

					if( (timetv.tv_sec-3600)  > pFileInfoPoint->pFilePos->EndTime )
					{
						DPRINTK("rec time > file time + 3600\n");
					}

					if( start_first_file_skip == 0 )
					{						
						start_first_file_skip = 1;
						DPRINTK("start_first_file_skip ok!\n");
					}
					//printf("time error change next file to rec!\n");
					goto FILESTATEWRONG;
				}

				if( (g_RecMode.iPbImageSize != pFileInfoPoint->pFilePos->ulVideoSize) ||
					(g_RecMode.iPbVideoStandard != pFileInfoPoint->pFilePos->mode) )
				{

					DPRINTK("iPbImageSize=%d iPbVideoStandard=%d, ulVideoSize=%d mode=%d\n",
						g_RecMode.iPbImageSize,g_RecMode.iPbVideoStandard,
						pFileInfoPoint->pFilePos->ulVideoSize,pFileInfoPoint->pFilePos->mode);
				
					if( stDiskState.IsCover == YES &&  pFileInfoPoint->pFilePos->isCover == YES)
					{
						pFileInfoPoint->pFilePos->isCover = NO;
					
						rel = FS_FileInfoWriteToFile(pFileInfoPoint);
						if( rel < 0 )
						{
							printf(" UseInfoWriteToFile error! 918");
							return rel;
						}	
						printf(" file cover is to NO!\n"); 
					}
				
					printf("mode dif change next file to rec!\n");
					goto FILESTATEWRONG;
				}
			}
			
			g_stRecFilePos.iDiskId = i;
			g_stRecFilePos.iPartitionId = pFileInfoPoint->iPatitionId;
			g_stRecFilePos.iCurFilePos = pFileInfoPoint->pUseinfo->iCurFilePos;

			if( pFileInfoPoint->pFilePos->states  == NOWRITE)
			{
				DPRINTK("(%d,%d,%d)cover file open by w+b\n",
					g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId,g_stRecFilePos.iCurFilePos);
				*openNewFile = 1;
			}

			stDiskInfo = DISK_GetDiskInfo(pFileInfoPoint->iDiskId);

			sprintf(fileName,"/tddvr/%s%d/%s",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);
				
			return ALLRIGHT;
		}						

	}

	// 程序走到这里说明，每个硬盘IsRecording 都为NO
	//要进行覆盖操作。
	k = 0;			
	for(j = 0; j <iDiskNum; j++)
	{		
		rel = FS_GetDiskCurrentState(j, &stDiskState);
		if( rel < 0 )
			return rel;	

		if( stDiskState.iEnableCover == ENABLE )
		{
			//查一下这个硬盘中的文件有没有可以覆盖的文件。
			if( FS_HaveFileToRecover(j) == ALLRIGHT )
			{			
				// 标识硬盘正在覆盖
				stDiskState.IsCover = YES;
				stDiskState.IsRecording = YES;
										
				rel = FS_WriteDiskCurrentState(j, &stDiskState);
				if( rel < 0 )
				{
					printf(" FS_WriteDiskCurrentState error!11\n");
					return rel;	
				}
				
				DPRINTK(" [%d] enable cover!\n",j);

				k++;
			}else
			{
				DPRINTK("FS_HaveFileToRecover return error!\n");
			}
		}else
		{
			DPRINTK("iEnableCover=%d disk=%d\n",stDiskState.iEnableCover,j);
		}
		
	}
							
	// k == 0 表示没有硬盘可以覆盖。
	if( k == 0 )
	{
		DPRINTK("k=0 iDiskNum=%d\n",iDiskNum);
		return ALLDISKNOTENABLETOWRITE;
	}
	
	// 当前硬盘已用完要进行覆盖操作 
	return ALLDISKUSEOVER;
}



int RecWriteOver(int iReason)
{
	int rel;
	int iDiskNum = 0;
	DISKCURRENTSTATE   stDiskState;
	FILEINFOLOGPOINT *  pFileInfoPoint = NULL;
	FILEINFOLOGPOINT 	fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	struct timeval stTimeVal;

	#ifdef USE_WRITE_BUF_SYSTEM
	FS_write_buf_write_to_file();
	#endif

	if( rec_header_filest.fd > 0 )
	{		
		pFileInfoPoint  = FS_GetPosFileInfo(g_stRecFilePos.iDiskId, g_stRecFilePos.iPartitionId, g_stRecFilePos.iCurFilePos,&fileInfoPoint);

		if( pFileInfoPoint != NULL )
		{
			g_pstCommonParam->GST_DRA_get_sys_time( &stTimeVal, NULL );

			if( g_preview_sys_flag == 1 )
				pFileInfoPoint->pFilePos->EndTime = g_previous_headtime;
			else
				pFileInfoPoint->pFilePos->EndTime = stTimeVal.tv_sec;
			pFileInfoPoint->pFilePos->iDataEffect = EFFECT;
			pFileInfoPoint->pFilePos->states      = WRITEOVER;
			pFileInfoPoint->pFilePos->ulOffSet   = g_ulFrameHeaderOffset;

			
			DPRINTK("start:%ld end:%ld abs(%d) %ld\n",
				pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime,
				labs(pFileInfoPoint->pFilePos->EndTime - pFileInfoPoint->pFilePos->StartTime),
				g_previous_headtime);


			// 2011-02-14
			// 这里不只知道什么原因加了以下代码。这个影响到了移动录象，造成移动录象，FILEINFO EndTime纪录出错，导致
			// 回放不出录象。现在修改。	
			// 知道原因了是因为 g_previous_headtime 有特殊时刻，数值不对。
			// 现做如下修改。
			/*
			if( labs(pFileInfoPoint->pFilePos->EndTime - pFileInfoPoint->pFilePos->StartTime) > (60*2*60) )
			{
				DPRINTK("rec time error,need fix\n");

				pFileInfoPoint->pFilePos->EndTime = pFileInfoPoint->pFilePos->StartTime + 60;
			}		
			*/

			if( labs(pFileInfoPoint->pFilePos->EndTime - pFileInfoPoint->pFilePos->StartTime) > (60*2*60) )
			{
				DPRINTK("rec time error,need fix\n");

				if( stTimeVal.tv_sec - pFileInfoPoint->pFilePos->StartTime >  0 )
				{
					
					if( (stTimeVal.tv_sec - 2) > pFileInfoPoint->pFilePos->EndTime )
						pFileInfoPoint->pFilePos->EndTime = stTimeVal.tv_sec - 2;
					else
						pFileInfoPoint->pFilePos->EndTime = stTimeVal.tv_sec;				
				}else
				{
					pFileInfoPoint->pFilePos->EndTime = pFileInfoPoint->pFilePos->StartTime + 60;
				}

				DPRINTK("stTimeVal.tv_sec = %ld StartTime=%ld EndTime = %ld g_previous_headtime=%ld\n",stTimeVal.tv_sec,
						pFileInfoPoint->pFilePos->StartTime,
						pFileInfoPoint->pFilePos->EndTime,
						g_previous_headtime);
			}	

			

			// 将文件标志位置为BADFILE,此文件将不会再被使用
			if( iReason == FILEWRITEERROR )
			{
				pFileInfoPoint->pFilePos->states      = BADFILE;				
				printf("2disk: %d,%d,%d \n",g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId,g_stRecFilePos.iCurFilePos);
			}

			if( pFileInfoPoint->pFilePos->isCover == YES && iReason != STOPRECORD)
				pFileInfoPoint->pFilePos->isCover = NO;

			printf(" pFileInfoPoint->pUseinfo->iCurFilePos = %d  stats=%d isCover=%d\n",
				pFileInfoPoint->pUseinfo->iCurFilePos,pFileInfoPoint->pFilePos->states,
				pFileInfoPoint->pFilePos->isCover );
		
			rel = FS_FileInfoWriteToFile(pFileInfoPoint);
			if( rel < 0 )
				return rel;

			if( file_io_lseek(&rec_header_filest,  0 , SEEK_SET ) == (off_t)-1 )
			{
				DPRINTK("  fseek error!\n");
				return FILEERROR;
			}	
			

			{
				int tmp;

				tmp = pFileInfoPoint->pFilePos->ulVideoSize;

				pFileInfoPoint->pFilePos->ulVideoSize = 
					g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video();
				
				rel = file_io_write(&rec_header_filest,pFileInfoPoint->pFilePos, sizeof(FILEINFO));

				pFileInfoPoint->pFilePos->ulVideoSize = tmp;
				
				if( rel != sizeof(FILEINFO) )
				{
					return FILEERROR;
				}

			}

			file_io_close(&rec_header_filest);

			#ifdef USE_DISK_WIRET_DIRECT
			DF_Close(rec_data_fp);
			rec_data_fp = 0;
			#else
			file_io_close(&rec_data_filest);
			#endif


			// 将停止录相的信息写入record.log.
			if(iReason != FILEWRITEOVER )
			{			
				if( iReason == FILEWRITEERROR )
				{
					rel = FS_WriteEndInfotoRecLog(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);
					if(rel < 0 )
					{
						printf("FS_WriteEndInfotoRecLog error!\n");
						return rel;
					}
					printf(" FS_WriteEndInfotoRecLog over  iReason=%d\n",iReason);
				}else
				{		
				
					rel = FS_WriteEndInfotoRecLog(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,stTimeVal.tv_sec);
					if(rel < 0 )
					{
						printf("FS_WriteEndInfotoRecLog error!\n");
						return rel;
					}
					printf(" FS_WriteEndInfotoRecLog over %ld  iReason=%d\n",stTimeVal.tv_sec,iReason);
				}
			}		
			
			rel = FS_GetDiskCurrentState(g_stRecFilePos.iDiskId, &stDiskState);						
			if( rel < 0 )
				return rel;			

			// 如果录相文件写完了，那就修改useinfo的值。
			if( iReason == FILEWRITEOVER || iReason == FILEWRITEERROR || iReason == RECTIMEOVER)
			{
				printf(" over iReason =%d\n",iReason);
			
				pFileInfoPoint->pUseinfo->iCurFilePos += 1;			
			
				// 换分区情况.
				if( pFileInfoPoint->pUseinfo->iCurFilePos >= pFileInfoPoint->pUseinfo->iTotalFileNumber)
				{
					pFileInfoPoint->pUseinfo->iCurFilePos = 0;
					rel = UseInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" UseInfoWriteToFile error!");
						return rel;
					}
					
					// 写入fileinfo.log.								
					stDiskState.iPartitionId += 1;
							
							// 换硬盘情况。
					stDiskInfo = DISK_GetDiskInfo(g_stRecFilePos.iDiskId);

					// 已经是最后一个分区了，要换硬盘了。
					if( stDiskState.iPartitionId > stDiskInfo.partitionID[stDiskInfo.partitionNum - 1] )
					{
						stDiskState.iPartitionId = 5;
						stDiskState.IsRecording = NO;
						// 写入diskinfo.log
						rel = FS_WriteDiskCurrentState(g_stRecFilePos.iDiskId, &stDiskState);
						if( rel < 0 )
							return rel;

						DPRINTK("stDiskState.IsRecording = %d  ,diskid=%d ,stDiskState.iUseCapacity=%d\n",
							stDiskState.IsRecording,g_stRecFilePos.iDiskId,stDiskState.iUseCapacity);
										
					}
					else
					{
						// stDiskState.IsRecording = NO;
						// 写入diskinfo.log
						rel = FS_WriteDiskCurrentState(g_stRecFilePos.iDiskId, &stDiskState);
						if( rel < 0 )
							return rel;
					}
							
				}			

				rel = UseInfoWriteToFile(pFileInfoPoint);
				if( rel < 0 )
				{
						printf(" UseInfoWriteToFile error!");
						return rel;
				}
			
				
			}

			return ALLRIGHT;			
		}

		
	}

	return FILEERROR;
}

int fs_get_rec_size()
{
	return rec_file_size;
}

int fs_set_rec_size(unsigned long size)
{
	rec_file_size = size;
	return ALLRIGHT;
}

int fs_write_rec_size_to_log()
{
	RECORDLOGPOINT *  pRecPoint;
	int ret = 0;
	int iCurrentId,rel;
	
	if( rec_file_size == 0 )
		return 0;	
		
	pRecPoint = FS_GetPosRecordMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

	// 得到当前message 
	iCurrentId = pRecPoint->pUseinfo->id ;

	pRecPoint = FS_GetPosRecordMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecordMessage error!\n");
		return FILEERROR;
	}
	
	pRecPoint->pFilePos->rec_size = rec_file_size;

	rel = RecordMessageWriteToFile();
	if( rel < 0 )
	{
		printf("RecordMessageWriteToFile error!\n");
		return rel;
	}

	return ALLRIGHT;
}

int FS_read_buf_sys_init()
{
	memset(&g_play_read_buf,0x00,sizeof(WRTIE_BUF));

	g_play_read_buf.data_buf = malloc(READ_BUF_SYS_MAX_DATA_NUM);
	if( g_play_read_buf.data_buf == NULL )
	{
		DPRINTK("malloc data_buf error!\n");
		goto init_error;
	}

	g_play_read_buf.head_buf = malloc(READ_BUF_SYS_MAX_HEAD_NUM);
	if( g_play_read_buf.head_buf == NULL )
	{
		DPRINTK("malloc head_buf error!\n");
		goto init_error;
	}

	#ifdef USE_DISK_READ_DIRECT
	{
		int pagesize;
		pagesize = getpagesize();

		g_play_read_buf.data_buf=(char*)((((int) g_play_read_buf.data_buf+pagesize-1)/pagesize)*pagesize);
		g_play_read_buf.head_buf=(char*)((((int) g_play_read_buf.head_buf+pagesize-1)/pagesize)*pagesize);
	}
	#endif

	g_play_read_buf.conut = 0;
	g_play_read_buf.data_size = 0;
	g_play_read_buf.head_size = 0;
	g_play_read_buf.create_flag = 1;
	g_play_read_buf.offset = 0;

	return 1;

	init_error:

	if( g_play_read_buf.data_buf )
	{
		free(g_play_read_buf.data_buf);
		g_play_read_buf.data_buf = NULL;
	}

	if( g_play_read_buf.head_buf )
	{
		free(g_play_read_buf.head_buf);
		g_play_read_buf.head_buf = NULL;
	}


	return -1;
}

int FS_read_buf_sys_destroy()
{
	if( g_play_read_buf.data_buf )
	{
		free(g_play_read_buf.data_buf);
		g_play_read_buf.data_buf = NULL;
	}

	if( g_play_read_buf.head_buf )
	{
		free(g_play_read_buf.head_buf);
		g_play_read_buf.head_buf = NULL;
	}

	g_play_read_buf.conut = 0;
	g_play_read_buf.data_size = 0;
	g_play_read_buf.head_size = 0;
	g_play_read_buf.create_flag = 0;
	g_play_read_buf.offset = 0;

}

void FS_read_buf_sys_clear()
{
	if( g_play_read_buf.create_flag != 1 )
	{
		DPRINTK("read buf sys not create\n");
		return ;
	}
	
	g_play_read_buf.conut = 0;
	g_play_read_buf.data_size = 0;
	g_play_read_buf.head_size = 0;
	g_play_read_buf.offset = 0;
}

int FS_read_data_to_buf(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData, char * AudioData,int playCam)
{	
	int rel;
	int read_count = 0;
	int offset = 0;
	FRAMEHEADERINFO * pheader_start = NULL;
	FRAMEHEADERINFO * pheader_end = NULL;
	char * pdata_offset = NULL;
	int i;
	int data_offset = 0;

	if( g_play_read_buf.create_flag != 1 )
	{
		DPRINTK("read buf sys not create\n");
		return -1;
	}

	if( g_play_read_buf.conut > 0 )
	{
		//判断是不是内存已经读到结尾了。
		if( g_play_read_buf.offset >= g_play_read_buf.head_size)
		{
			FS_read_buf_sys_clear();
		}
	}

	if( g_play_read_buf.conut > 0 )
	{
		if( ulPlayFrameHeaderOffset >=
			(g_play_read_buf.head_offset_start+g_play_read_buf.head_size) )
		{
			FS_read_buf_sys_clear();
		}else if(ulPlayFrameHeaderOffset < g_play_read_buf.head_offset_start)
		{
			FS_read_buf_sys_clear();
		}
		else
		{
		}
	}

	// count 等于 0 时，读取数据到内存块。
	if( g_play_read_buf.conut == 0 )
	{

		if( file_io_lseek(&play_header_filest,  ulPlayFrameHeaderOffset , SEEK_SET )== (off_t)-1 )
			return FILEERROR;	

		offset = ulPlayFrameHeaderEndOffset - ulPlayFrameHeaderOffset;

		if( offset < sizeof(FRAMEHEADERINFO) )
			return FILEERROR;

		if( offset >= (READ_BUF_SYS_MAX_READ_NUM*sizeof(FRAMEHEADERINFO)) )
		{
			offset = READ_BUF_SYS_MAX_READ_NUM*sizeof(FRAMEHEADERINFO);
		}else
		{
			offset = (offset / sizeof(FRAMEHEADERINFO)) * sizeof(FRAMEHEADERINFO);
		}

		read_count = offset /  sizeof(FRAMEHEADERINFO);

		rel =file_io_read(&play_header_filest,g_play_read_buf.head_buf,offset);
				
		if( rel != offset )
		{
			DPRINTK("offset=%d %d read error!\n",offset,rel);
			return FILEERROR;
		}

		data_offset = 0;

		for(i=0;i< read_count;i++)
		{
			pheader_start = (FRAMEHEADERINFO *)(g_play_read_buf.head_buf + i*sizeof(FRAMEHEADERINFO) );			
			data_offset = data_offset + pheader_start->ulDataLen;

			if( data_offset >  READ_BUF_SYS_MAX_DATA_NUM - 10*4096 ) 
			{
			//	DPRINTK("2offset = %d too large! READ_BUF_SYS_MAX_DATA_NUM=%d\n",
			//	data_offset,READ_BUF_SYS_MAX_DATA_NUM);
				
				read_count = i;
				offset =  read_count*sizeof(FRAMEHEADERINFO);

			//	DPRINTK("read_count=%d offset=%d\n",read_count,offset);
				break;
			}	
			
		}	
		
		g_play_read_buf.head_size = offset;

		pheader_start = (FRAMEHEADERINFO *)g_play_read_buf.head_buf;

		if( read_count > 1 )
		{
			pheader_end = (FRAMEHEADERINFO *)(g_play_read_buf.head_buf + offset - sizeof(FRAMEHEADERINFO) );

			//这里有个问题，如果dvr正在进行覆盖，pheader_start 和 pheader_end指向不同的录象段，就会有问题。
			offset = (pheader_end->ulFrameDataOffset+pheader_end->ulDataLen)- pheader_start->ulFrameDataOffset;
		}else
		{
			offset = pheader_start->ulDataLen;
		}
		if( (offset >  READ_BUF_SYS_MAX_DATA_NUM) || (offset <= 0 )  )
		{
			DPRINTK("offset = %d too large! READ_BUF_SYS_MAX_DATA_NUM=%d\n",
				offset,READ_BUF_SYS_MAX_DATA_NUM);

			DPRINTK("pheader_end->ulFrameDataOffset=%ld %ld pheader_start->ulFrameDataOffset=%ld\n",
				pheader_end->ulFrameDataOffset,pheader_end->ulDataLen,pheader_start->ulFrameDataOffset);
			return FILEERROR;
		}		

		#ifdef USE_DISK_READ_DIRECT
		{
			int off_pos = 0;
			int data_length = 0;

			off_pos = pheader_start->ulFrameDataOffset /DF_PAGESIZE*DF_PAGESIZE;
			data_length = (offset + DF_PAGESIZE  + DF_PAGESIZE - 1)/DF_PAGESIZE*DF_PAGESIZE;
			
			rel = DF_Lseek(play_data_fp, off_pos, SEEK_SET);		

			if( rel < 0 )
			{
				printf("2=== fseek error %ld\n",off_pos);
				return FILEERROR;
			}

			rel = DF_Read(play_data_fp, g_play_read_buf.data_buf , data_length);		

			if( rel <= 0 )
			{
				DPRINTK("offset=%d %d read error!\n",offset,rel);
				return FILEERROR;
			}

			g_play_read_buf.data_size = offset;

			g_play_read_buf.conut = read_count;

			g_play_read_buf.offset = 0;

			g_play_read_buf.data_offset_start = off_pos;
			g_play_read_buf.head_offset_start = ulPlayFrameHeaderOffset;
			

		}

		#else

		if( file_io_lseek(&play_data_filest,  pheader_start->ulFrameDataOffset , SEEK_SET )== (off_t)-1 )
			return FILEERROR;

		rel =file_io_read(&play_data_filest,g_play_read_buf.data_buf ,offset);
				
		if( rel != offset  )
		{
			DPRINTK("offset=%d %d read error!\n",offset,rel);
			return FILEERROR;
		}
		
		g_play_read_buf.data_size = offset;

		g_play_read_buf.conut = read_count;

		g_play_read_buf.offset = 0;

		g_play_read_buf.data_offset_start = pheader_start->ulFrameDataOffset;
		g_play_read_buf.head_offset_start = ulPlayFrameHeaderOffset;

		#endif
		
		
	}

	// 下面的是从内存中读取一帧数据。

	pheader_start = (FRAMEHEADERINFO *)(g_play_read_buf.head_buf + g_play_read_buf.offset);

	memcpy(stFrameHeaderInfo,pheader_start,sizeof(FRAMEHEADERINFO) );	

	ulPlayFrameHeaderOffset += sizeof( FRAMEHEADERINFO );
	g_play_read_buf.offset += sizeof( FRAMEHEADERINFO );

	if( CheckFrameHeader(stFrameHeaderInfo) == ERROR )
	{
		//要跳过这一帧		
		//printf(" CheckFrameHeader BAD\n");
		return READBADFRAME;
	}

	if( (FTC_GetBit(playCam, stFrameHeaderInfo->iChannel ) != 1) && stFrameHeaderInfo->type == 'V')
		return READNOTHING;


	offset = stFrameHeaderInfo->ulFrameDataOffset - g_play_read_buf.data_offset_start;

	if( (offset >  READ_BUF_SYS_MAX_DATA_NUM) || (offset < 0 )  )
	{
		DPRINTK("offset = %d too large! READ_BUF_SYS_MAX_DATA_NUM=%d\n",
			offset,READ_BUF_SYS_MAX_DATA_NUM);
		return FILEERROR;
	}	

	pdata_offset = g_play_read_buf.data_buf + offset;

	if( stFrameHeaderInfo->type == 'V' )
	{		
		memcpy(FrameData, pdata_offset,stFrameHeaderInfo->ulDataLen);

		curtime = stFrameHeaderInfo->ulTimeSec;
	}
	else if( stFrameHeaderInfo->type == 'A'  )
	{				
		memcpy(AudioData, pdata_offset,stFrameHeaderInfo->ulDataLen);
	}
	else
	{
		return FILEERROR;
	}

	return ALLRIGHT;
	
}

int FS_write_buf_sys_init()
{
	memset(&g_rec_write_buf,0x00,sizeof(WRTIE_BUF));

	g_rec_write_buf.data_buf = malloc(WRITE_BUF_SYS_MAX_DATA_NUM);
	if( g_rec_write_buf.data_buf == NULL )
	{
		DPRINTK("malloc data_buf error!\n");
		goto init_error;
	}

	g_rec_write_buf.head_buf = malloc(WRITE_BUF_SYS_MAX_HEAD_NUM);
	if( g_rec_write_buf.head_buf == NULL )
	{
		DPRINTK("malloc head_buf error!\n");
		goto init_error;
	}

	#ifdef USE_DISK_WIRET_DIRECT
	{
		int pagesize;
		pagesize = getpagesize();

		g_rec_write_buf.data_buf=(char*)((((int) g_rec_write_buf.data_buf+pagesize-1)/pagesize)*pagesize);
		g_rec_write_buf.head_buf=(char*)((((int) g_rec_write_buf.head_buf+pagesize-1)/pagesize)*pagesize);
	}
	#endif

	g_rec_write_buf.conut = 0;
	g_rec_write_buf.data_size = 0;
	g_rec_write_buf.head_size = 0;
	g_rec_write_buf.create_flag = 1;
	g_rec_write_buf.offset = 0;

	return 1;

	init_error:

	if( g_rec_write_buf.data_buf )
	{
		free(g_rec_write_buf.data_buf);
		g_rec_write_buf.data_buf = NULL;
	}

	if( g_rec_write_buf.head_buf )
	{
		free(g_rec_write_buf.head_buf);
		g_rec_write_buf.head_buf = NULL;
	}


	return -1;
}

int FS_write_buf_sys_destroy()
{
	if( g_rec_write_buf.data_buf )
	{
		free(g_rec_write_buf.data_buf);
		g_rec_write_buf.data_buf = NULL;
	}

	if( g_rec_write_buf.head_buf )
	{
		free(g_rec_write_buf.head_buf);
		g_rec_write_buf.head_buf = NULL;
	}

	g_rec_write_buf.conut = 0;
	g_rec_write_buf.data_size = 0;
	g_rec_write_buf.head_size = 0;
	g_rec_write_buf.create_flag = 0;

}

int FS_write_buf_sys_data(char * buf1,int size1,char * buf2,int size2)
{

	if( g_rec_write_buf.create_flag != 1 )
	{
		DPRINTK("write buf sys not create\n");
		return -1;
	}


	if( (g_rec_write_buf.data_size + size2)  >= WRITE_BUF_SYS_MAX_DATA_NUM )
	{
		DPRINTK("g_rec_write_buf.data_size = %d  must write,WRITE_BUF_SYS_MAX_DATA_NUM=%d lost frame\n",
			g_rec_write_buf.data_size + size2,WRITE_BUF_SYS_MAX_DATA_NUM);

		return WRITE_BUF_SYS_BUF_MUST_WRITE;
	}

	memcpy(g_rec_write_buf.head_buf + g_rec_write_buf.head_size,buf1,size1);
	g_rec_write_buf.head_size += size1;

	memcpy(g_rec_write_buf.data_buf + g_rec_write_buf.data_size,buf2,size2);
	g_rec_write_buf.data_size += size2;

	g_rec_write_buf.conut++;

	if( g_rec_write_buf.head_size >= (WRITE_BUF_SYS_MAX_HEAD_NUM - 30*1024) )
	{
		DPRINTK("g_rec_write_buf.head_size = %d must write\n",g_rec_write_buf.head_size);
		return WRITE_BUF_SYS_BUF_MUST_WRITE;
	}

	if( g_rec_write_buf.data_size >= (WRITE_BUF_SYS_MAX_DATA_NUM - WRITE_BUF_SYS_MAX_SAVE_NUM) )
	{
		if(  g_rec_write_buf.data_size >= WRITE_BUF_SYS_MAX_DATA_NUM )
		{
			DPRINTK("g_rec_write_buf.data_size = %d  must write,WRITE_BUF_SYS_MAX_DATA_NUM=%d\n",
			g_rec_write_buf.data_size,WRITE_BUF_SYS_MAX_DATA_NUM);
		
		}
		
		return WRITE_BUF_SYS_BUF_MUST_WRITE;
	}


	return g_rec_write_buf.conut;
}

int FS_write_buf_write_to_file()
{
	int rel;
	int frame_num = 0;
	FRAMEHEADERINFO * stFrameHeaderInfo=NULL;
    int data_offset = 0;
	int frame_head_offset = 0;
	static int iCount = 0;


	if( g_rec_write_buf.create_flag != 1 )
	{
		DPRINTK("write buf sys not create\n");
		return -1;
	}
	
	if( g_rec_write_buf.conut == 0 )
	{
		DPRINTK("g_rec_write_buf.conut = 0,no data write\n");
		return 0;
	}

	//数据修正。
	frame_head_offset = g_ulFrameHeaderOffset;
	data_offset = g_ulFileDataOffset;
	for( frame_num = 0;frame_num < g_rec_write_buf.conut;frame_num++)
	{
		stFrameHeaderInfo = (FRAMEHEADERINFO *)(g_rec_write_buf.head_buf + frame_num*sizeof(FRAMEHEADERINFO));
		stFrameHeaderInfo->ulFrameDataOffset = data_offset;
		data_offset += stFrameHeaderInfo->ulDataLen;
	}

	if( file_io_lseek(&rec_header_filest,  g_ulFrameHeaderOffset , SEEK_SET )== (off_t)-1 )
	{		
		DPRINTK("===fseek error %ld\n",g_ulFrameHeaderOffset);
		DPRINTK(" error %s %d\n", strerror(errno),errno);
		return FILEERROR;
	}	
		
	

	rel = file_io_write(&rec_header_filest,g_rec_write_buf.head_buf,g_rec_write_buf.head_size);
	if( rel != g_rec_write_buf.head_size )
	{
		DPRINTK("====disk fwrite error %d %d\n",rel,g_rec_write_buf.head_size);
		return FILEERROR;
	}
	
	g_ulFrameHeaderOffset += g_rec_write_buf.head_size;

	#ifdef USE_DISK_WIRET_DIRECT
	rel = DF_Lseek(rec_data_fp, g_ulFileDataOffset, SEEK_SET);
	if( rel < 0 )
	{
		printf("2=== fseek error %ld\n",g_ulFileDataOffset);
		return FILEERROR;
	}
	#else

	if( file_io_lseek(&rec_data_filest,  g_ulFileDataOffset , SEEK_SET )== (off_t)-1 )
	{		
		DPRINTK("2=== fseek error %ld\n",g_ulFileDataOffset);
		return FILEERROR;
	}		

	#endif

	#ifdef USE_DISK_WIRET_DIRECT
	if( g_rec_write_buf.data_size % DF_PAGESIZE != 0 )
	{
		g_rec_write_buf.data_size = (g_rec_write_buf.data_size + DF_PAGESIZE - 1)/DF_PAGESIZE*DF_PAGESIZE;
	}	

	rel = DF_Write(rec_data_fp, g_rec_write_buf.data_buf , g_rec_write_buf.data_size );

	#else
	rel = file_io_write(&rec_data_filest,g_rec_write_buf.data_buf,g_rec_write_buf.data_size);	
	if( iCount == 66)
	{
		DPRINTK("file_io_write %d once\n",g_rec_write_buf.data_size);
	}
		
	#endif
	
	if( rel != g_rec_write_buf.data_size )
	{
		DPRINTK("FrameData %d %d\n",rel,g_rec_write_buf.data_size);
		DPRINTK(" error %s %d\n", strerror(errno),errno);

		g_ulFrameHeaderOffset -= g_rec_write_buf.head_size;
		
		return FILEERROR;
	}

	//printf("channel = %d data fwrite  %d type=%c num=%d\n",stFrameHeaderInfo->iChannel,
	//	stFrameHeaderInfo->ulDataLen,stFrameHeaderInfo->type,stFrameHeaderInfo->ulFrameNumber);
	//	printf("FrameData %x\n",FrameData);

	g_ulFileDataOffset += g_rec_write_buf.data_size;
	rec_file_size = rec_file_size + g_rec_write_buf.data_size + g_rec_write_buf.head_size;

	//DPRINTK("data=%d head=%d\n",g_rec_write_buf.data_size,g_rec_write_buf.head_size);

	g_rec_write_buf.conut = 0;
	g_rec_write_buf.data_size = 0;
	g_rec_write_buf.head_size = 0;	

	iCount++;

	if( iCount > 30 )
	{
		iCount = 0;

		file_io_sync(&rec_data_filest);
		file_io_sync(&rec_header_filest);		

		rel = fs_write_rec_size_to_log();
		if( rel < 0 )
			return FILEERROR;		
		//DPRINTK("sync\n");
		//SendM2("sync");
	}

	return ALLRIGHT;
}


int FS_WriteOneFrame(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData)
{
	int rel;

	FRAMEHEADERINFO tempHeaderInfo;

	static int iCount = 0;
	static int iWriteCount = 0;

	#ifdef USE_WRITE_BUF_SYSTEM	
	
	rel = FS_write_buf_sys_data((char*)stFrameHeaderInfo,sizeof(FRAMEHEADERINFO),
			FrameData,stFrameHeaderInfo->ulDataLen);

	if( rel < 0 )
		return 0;

	if( rel == WRITE_BUF_SYS_BUF_MUST_WRITE )
	{
		return FS_write_buf_write_to_file();
	}


	if( g_RecMode.iPbImageSize == TD_DRV_VIDEO_SIZE_D1 )
	{
		if( rel > (WRITE_BUF_SYS_MAX_NUM) )
		{
			return FS_write_buf_write_to_file();
		}
	}else
	{
		if( rel > WRITE_BUF_SYS_MAX_NUM )
		{
			return FS_write_buf_write_to_file();
		}
	}

	return 1;
	
	#endif

	
	rel = fseek(rec_head_fp, g_ulFrameHeaderOffset, SEEK_SET);

	if( rel < 0 )
	{
		printf("===fseek error %ld\n",g_ulFrameHeaderOffset);
		return FILEERROR;
	}

	stFrameHeaderInfo->ulFrameDataOffset = g_ulFileDataOffset;

	rel = fwrite(stFrameHeaderInfo,1, sizeof( FRAMEHEADERINFO ) ,rec_head_fp );

	if( rel != sizeof( FRAMEHEADERINFO ) )
	{
		printf("====disk fwrite error %d\n",rel);
		return FILEERROR;
	}

	fflush(rec_head_fp);

	iWriteCount++;

	if( iWriteCount > 200 )
	{
		#ifdef USE_EXT3
			sync();
		#else
			sync();
		#endif
		iWriteCount = 0;
		//printf("sync 1\n");
	}

	iCount++;

	g_ulFrameHeaderOffset += sizeof( FRAMEHEADERINFO );

	#ifdef USE_DISK_WIRET_DIRECT
	rel = DF_Lseek(rec_data_fp, g_ulFileDataOffset, SEEK_SET);
	#else
	rel = fseek(rec_data_fp, g_ulFileDataOffset, SEEK_SET);
	#endif	

	if( rel < 0 )
	{
		printf("2=== fseek error %ld\n",g_ulFileDataOffset);
		return FILEERROR;
	}

	#ifdef USE_DISK_WIRET_DIRECT
	rel = DF_Write(rec_data_fp,FrameData ,stFrameHeaderInfo->ulDataLen );
	#else
	rel = fwrite(FrameData , 1, stFrameHeaderInfo->ulDataLen , rec_data_fp );
	#endif
	
	if( rel != stFrameHeaderInfo->ulDataLen  )
	{
		printf("channel = %d data fwrite error %d type=%c  writenuml=%d\n",stFrameHeaderInfo->iChannel,stFrameHeaderInfo->ulDataLen,stFrameHeaderInfo->type,rel);
		printf("FrameData %x\n",FrameData);
		g_ulFrameHeaderOffset -= sizeof( FRAMEHEADERINFO );
		
		return FILEERROR;
	}

	//printf("channel = %d data fwrite  %d type=%c num=%d\n",stFrameHeaderInfo->iChannel,
	//	stFrameHeaderInfo->ulDataLen,stFrameHeaderInfo->type,stFrameHeaderInfo->ulFrameNumber);
	//	printf("FrameData %x\n",FrameData);

	g_ulFileDataOffset += stFrameHeaderInfo->ulDataLen;

	rec_file_size = rec_file_size + sizeof( FRAMEHEADERINFO ) + stFrameHeaderInfo->ulDataLen;

	if( iCount > 6000 )
	{
		iCount = 0;

		printf("=2=rec_file_size = %ld\n",rec_file_size);

		rel = fs_write_rec_size_to_log();
		if( rel < 0 )
			return FILEERROR;

		printf("=1=rec_file_size = %ld\n",rec_file_size);
	}

	return ALLRIGHT;
		
}


int FS_CheckRecFile()
{
	if( g_ulFileDataOffset > MAXFRAMEDATAOFFSET )
	{
		//printf(" g_ulFileDataOffset = %ld\n",g_ulFileDataOffset);
		return FILEFULL;
	}
	
	if( g_ulFrameHeaderOffset > MAXFRAMEHEADOFFSET )
	{
		//printf(" g_ulFrameHeaderOffset = %ld\n",g_ulFrameHeaderOffset);
		return FILEFULL;
	}

	return ALLRIGHT;
}

int fs_open_play_file(char * file_name)
{
	char whole_file_name[50];
	char fileName[50];
	char dev[50];	

	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	strncpy(fileName,&file_name[6],4);
	sprintf(dev,"/dev%s6",fileName);
	strcpy(whole_file_name,&file_name[12]);
	
	sprintf(fileName,"%s.avh",whole_file_name);

	if( file_io_open(dev,fileName,"r",&play_header_filest,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto  OPENERROR;;
	}
	
	sprintf(fileName,"%s.avd",whole_file_name);	

	if( file_io_open(dev,fileName,"r",&play_data_filest,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto  OPENERROR;;
	}			

	
	return ALLRIGHT;

	OPENERROR:
		file_io_close(&play_header_filest);
		file_io_close(&play_data_filest);


	return FILEERROR;
}

int OpenPlayFile( int iDiskId, int iPartitionId, int iFilepos )
{
	int rel;

	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT  fileInfoPoint;

	GST_DISKINFO stDiskInfo;

	char filename[50];

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId, iPartitionId, iFilepos,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	// 判断文件是否可放。
	if( pFileInfoPoint->pFilePos->states != WRITEOVER &&  pFileInfoPoint->pFilePos->states !=  WRITING)
	{
		printf(" this file is used by other. state = %d\n", pFileInfoPoint->pFilePos->states );
		return FILEERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(pFileInfoPoint->iDiskId);

	memset(filename,0,50);
			
	sprintf(filename,"/tddvr/%s%d/%s",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);

	DPRINTK(":%s  %ld %ld\n",filename,pFileInfoPoint->pFilePos->StartTime,
		pFileInfoPoint->pFilePos->EndTime);

	rel = fs_open_play_file(filename);
	if( rel < 0 )
		return rel;

	if( pFileInfoPoint->pFilePos->states == WRITEOVER )
	{
		// 修改文件状态
		pFileInfoPoint->pFilePos->states = PLAYING;

		rel = FS_FileInfoWriteToFile(pFileInfoPoint);
		if( rel < 0 )
		{
			printf(" UseInfoWriteToFile error!");
			return rel;
		}
	} 

	g_stPlayFilePos.iCurFilePos = iFilepos;
	g_stPlayFilePos.iDiskId       = iDiskId;
	g_stPlayFilePos.iPartitionId  = iPartitionId;

	// 设置数据和帧头位移 点。
	ulPlayFrameHeaderOffset = sizeof(FILEINFO);
	ulPlayFrameHeaderEndOffset = pFileInfoPoint->pFilePos->ulOffSet;

	if( pFileInfoPoint->pFilePos->states ==  WRITING )
	{
		ulPlayFrameHeaderEndOffset = g_ulFrameHeaderOffset;

		DPRINTK("play file is writting!\n");
	}

	curtime = 0;
	lasttime = 0;	

	return ALLRIGHT;
	
}

int ReadOneFrame(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData, char * AudioData,int index,int playCam)
{
	int rel;

	#ifdef USE_READ_BUF_SYSTEM

	rel  = FS_read_data_to_buf(stFrameHeaderInfo,FrameData,AudioData,playCam);
	if( rel < 0 )
	{
		FS_read_buf_sys_clear();
	}

	return rel;	

	#endif


	return ALLRIGHT;
}

int FS_GoBackOneFrame()
{
	if( ulPlayFrameHeaderOffset > sizeof(FILEINFO))
		ulPlayFrameHeaderOffset -= sizeof( FRAMEHEADERINFO );

	return ALLRIGHT;
}

int  ClosePlayFile()
{
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT     fileInfoPoint;
	int rel;

	pFileInfoPoint = FS_GetPosFileInfo(g_stPlayFilePos.iDiskId ,g_stPlayFilePos.iPartitionId, g_stPlayFilePos.iCurFilePos,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	if( pFileInfoPoint->pFilePos->states == PLAYING )
	{
		pFileInfoPoint->pFilePos->states = WRITEOVER;

		rel = FS_FileInfoWriteToFile(pFileInfoPoint);
		if( rel < 0 )
		{
			printf(" UseInfoWriteToFile error!");
			return rel;
		}
	} 


	file_io_close(&play_header_filest);
	file_io_close(&play_data_filest);


	return ALLRIGHT;
	
}

int  CheckPlayFile()
{
	if(  ulPlayFrameHeaderOffset >= ulPlayFrameHeaderEndOffset )
		return PLAYOVER;

/*
	if( lasttime != 0 )
	{
	// 如果两帧之间的时差大于1 秒说明这两帧不是同一次录的。
		if( curtime - lasttime > 1 || curtime - lasttime <0)
			return PLAYOVER;
	}
*/
	lasttime = curtime;

	return ALLRIGHT;
}

int FS_OutCreateFileThread()
{
	gStopCreateFile = -1;
	return ALLRIGHT;
}

int FS_CreateFileStatus()
{
	return gStopCreateFile;
}

int FS_StopCreateFile()
{
	//表示正在创建 文件
	if( gStopCreateFile == 0 )
	{
		gStopCreateFile = 2;
		while(1)
		{
			usleep(20000);
			if( gStopCreateFile ==  1)
			{
				printf(" stop create file succes!\n");
				return ALLRIGHT;
			}
		}
	}
	return ALLRIGHT;
}

int FS_StartCreateFile()
{

	gStopCreateFile = 0;

	return ALLRIGHT;
}

int FS_GetCreateFileFlag()
{
	return g_CreateFileStatusCurrent;
}

int FS_SetCreateFileFlag(int iFlag)
{
	g_CreateFileStatusCurrent = iFlag;
	return ALLRIGHT;
}

int FS_CreateFile()
{
	int iDiskNum = 0;
	int iPartionId,iFileNum;
	int rel,i,j,k,z;
	int num;
	char fileName[50];
	FILE * fp=NULL;
	int  iCreateFileNum = 0;
	int  fd = 0;
	int iFileOk = 0;

	DISKCURRENTSTATE   stDiskState;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT  fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stFileinfo;
	char * buf;
	FILESTATE tempfilestate;
	long fileLength = 0;

	//gStopCreateFile = 0;	

	iDiskNum = DISK_GetDiskNum();	
	//硬盘搜索
	for( i = 0; i< iDiskNum; i++ )
	{
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Sleep(i) == 0 )
		{
			DPRINTK("disk=%d is sleep!\n");
			continue;
		}
		#endif
	
	
		iCreateFileNum = 0;
	
		rel = FS_GetDiskCurrentState(i, &stDiskState);
		if( rel < 0 )
			return rel;

		if( stDiskState.iAllfileCreate == YES )
			continue;

		stDiskInfo = DISK_GetDiskInfo(i);

		// 分区搜索
		for( j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			rel = FS_GetFileUseInfo(i, stDiskInfo.partitionID[j], &stFileinfo);
			if( rel < 0 )
				return ERROR;

			// fileinfo.log 中单个文件搜索
			for( k = 0 ; k < stFileinfo.iTotalFileNumber; k++ )
			{
				FS_LogMutexLock();
				
				pFileInfoPoint = FS_GetPosFileInfo(i, stDiskInfo.partitionID[j], k,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
				{
					FS_LogMutexUnLock();
					return ERROR;
				}
				tempfilestate =  pFileInfoPoint->pFilePos->states;

				iPartionId = stDiskInfo.partitionID[j];

				if( tempfilestate == NOCREATE )
				{
					memset(fileName, 0 , 50 );

					sprintf(fileName,"/tddvr/%s%d/%s", &stDiskInfo.devname[5]
						, iPartionId , pFileInfoPoint->pFilePos->fileName );
				}

				FS_LogMutexUnLock();

				//创建录相文件
				if( tempfilestate == NOCREATE )
				{
					rel = FS_CreateFileCurrentPartitionWriteToShutDownLog(i,iPartionId );
					if( rel < 0)
					{
						printf(" FS_CreateFileCurrentPartitionWriteToShutDownLog %s error!\n", fileName);					
						return FILEERROR;
					}
					
					fileLength = 0;

					g_createFileDiskNum = i;
				
					fp = fopen( fileName, "a+b");
					if( !fp )
					{
						printf(" create %s error!", fileName);					
						return FILEERROR;
					}

					rel = fseek(fp, 0, SEEK_END);
					if ( rel < 0 )
					{					
						fclose(fp);
						printf("  fseek error!\n");
						return FILEERROR;
					}					

					fileLength = ftell(fp);

					printf(" normal writing %s fileLength = %ld\n", fileName,fileLength);

					fclose(fp);
					fp = NULL;

					fd = open(fileName, O_WRONLY ,0644);
					if(fd<0)
					{
					        printf("create %s error\n",fileName);
						return FILEERROR;
					}

					if (lseek (fd, fileLength, SEEK_CUR) < 0)
	            					  return FILEERROR;

					iFileOk = 1;

					fileLength = fileLength / (1024*1024);

					for( z = (int)fileLength * 4; z < FILESIZE * 4; z++)
					{
					//	printf("z=%d\n",z);			

						g_CreateFileStatusCurrent = 1;

						if (lseek (fd, MAXFILEBUF /4, SEEK_CUR) < 0)
						{
							printf(" fd error 5\n");
							 iFileOk = 0;
							 break;
						}

						 if (write (fd, "0", 1) != 1)
        	      				{
        	      					printf(" fd error 4\n");
							 iFileOk = 0;
							 break;
						}

						  if( fsync(fd) != 0 )
						 {
						 	printf(" fd error 3\n");
						 	 iFileOk = 0;
							 break;
						 }	

						 usleep(800000);												

						if( gStopCreateFile != 0)
						{							
							close(fd);					
							printf("1 create file thread go out! gStopCreateFile=%d\n",gStopCreateFile);
							return FILEERROR;
						}
						
						
					}

					if( iFileOk == 1 )
						printf(" write %s over\n", fileName);
					else
						printf(" write %s error\n", fileName);
					
					close(fd);	
					
					// 修改fileinfo 信息。
					FS_LogMutexLock();
					
					pFileInfoPoint = FS_GetPosFileInfo(i , iPartionId, k,&fileInfoPoint);
					if( pFileInfoPoint == NULL )
					{
						FS_LogMutexUnLock();
						return ERROR;
					}

					if( iFileOk == 1 )
						pFileInfoPoint->pFilePos->states = NOWRITE;
					else
					{
						pFileInfoPoint->pFilePos->states = BADFILE;
						printf("===disk: %d,%d,%d \n",i,iPartionId,k);
					}
					
					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						FS_LogMutexUnLock();
						return rel;
					}
					FS_LogMutexUnLock();

					iCreateFileNum++;
					
				}
				

			}
		}

		// iCreateFileNum == 0 说明没有创建文件,则将此硬盘的iAllfileCreate赋为YES
		if( iCreateFileNum == 0 && stDiskState.iAllfileCreate == NO )
		{
			FS_LogMutexLock();
			
			rel = FS_GetDiskCurrentState(i, &stDiskState);
			if( rel < 0 )
			{
				FS_LogMutexUnLock();
				return rel;
			}
			
			stDiskState.iAllfileCreate = YES;

			rel = FS_WriteDiskCurrentState(i, &stDiskState);
			if( rel < 0 )
			{
				FS_LogMutexUnLock();
				return rel;
			}
			
			FS_LogMutexUnLock();
		}
		
	}		

	gStopCreateFile = 1;

	return ALLRIGHT;
}




int FS_GetOpreateDiskPartition()
{
	int iDiskNum = 0;
	int i,j,rel;
	char fileTempName[50];
	int opennewfile;


// 查找可用文件。
	memset(fileTempName,0,50);

	rel = FS_GetRecondFileName(fileTempName,&opennewfile);

	if( rel < 0 )
	{
		// 各种异常情况.

		printf(" rel = %d\n",rel);
		while(1 )
		{
			// 当前磁盘用完了，并且可以进行覆盖操作
			if( rel == ALLDISKUSEOVER )
			{
				rel = FS_GetRecondFileName(fileTempName,&opennewfile);
				if( rel > 0 )
					break;
			}

			//当前磁盘用完了，但没有磁盘可以进行覆盖操作
			if( rel == ALLDISKNOTENABLETOWRITE)
			{
				DPRINTK("File error: ALLDISKNOTENABLETOWRITE\n");
				return ALLDISKNOTENABLETOWRITE;
			}

			//返回的是异常错误，有待处理
			if( rel != ALLDISKUSEOVER &&  rel != ALLDISKNOTENABLETOWRITE
				&& rel != FILESTATEERROR )
			{
				return rel;
			}

			rel = FS_GetRecondFileName(fileTempName,&opennewfile);
			if( rel ==ALLRIGHT )
			{
				break;
			}
			
		}				
		
	}

	g_stLogPos = g_stRecFilePos;

	printf(" current oprate file  %s\n",fileTempName);

	return ALLRIGHT;
}


int FS_ListWriteDataToUsb(GST_FILESHOWINFO * stPlayFile)
{
	int i,j,k;
	int iDiskNum;
	int startTime,endTime,iCam;
	GST_DISKINFO stDiskInfo;
	GST_DISKINFO stUsbInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT    fileInfoPoint;
	FILEINFO tmp_fileinfo;
	int iInfoNumber = 0;
	int rel,ret;
	char fileName[500];
	 struct timeval nowTime;	 
	 long UsbSize = 0;
	 GST_TIME_TM mytime;
	 char timebuf[500];
	RECORDLOGPOINT * pRecPoint;
	int iDisk,iPartition,iFilePos;

	UsbWriteTime = 0;

	 if( stPlayFile->event.loss != 0 || stPlayFile->event.motion != 0  ||
	 	stPlayFile->event.sensor != 0)
	 {
	 // 这里的alarm 备份不能用.
	 //备份alarmlist中的文件
	 	pRecPoint = FS_GetPosRecordMessage(stPlayFile->iDiskId,stPlayFile->iPartitionId,stPlayFile->iFilePos);
		if( pRecPoint == NULL )
			return ERROR;
		
	//	startTime =pRecPoint->pFilePos->startTime;
	//	endTime  = pRecPoint->pFilePos->endTime;	
	
		startTime = stPlayFile->fileStartTime -10;
		endTime  = stPlayFile->fileEndTime + 10;
		iDisk  = pRecPoint->iDiskId;
		iPartition = pRecPoint->iPatitionId;
		iFilePos   = pRecPoint->pFilePos->filePos;
		iCam      = stPlayFile->iCam;

	
	 }
	 else
	 {
	 //备份reclist 中的文件
	 	startTime = stPlayFile->fileStartTime;
		endTime  = stPlayFile->fileEndTime;
		iDisk  = stPlayFile->iDiskId;
		iPartition = stPlayFile->iPartitionId;
		iFilePos   = stPlayFile->iFilePos;
		iCam      = stPlayFile->iCam;
	 }

	 g_UsbAvailableSize = 0;

	DISK_GetAllDiskInfo();

	rel = FS_MountUsb();
              
	if( rel < 0 )
	{
		printf("FS_MountUsb error!\n");
		return rel;
	}

	stUsbInfo = DISK_GetUsbInfo();
			
	iDiskNum = DISK_GetDiskNum();


	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDisk; i < iDiskNum; i++ )
	{
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;


		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			if(stDiskInfo.partitionID[j] < iPartition )
				continue;
		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
					return ERROR;
			
			
			for(k = iFilePos; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;
				
				if( FTC_CompareTime(startTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(endTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,startTime,endTime )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,startTime,endTime ))
				{
					if( pFileInfoPoint->pFilePos->states != WRITEOVER )
						continue;

				  	printf("k=%d starttime=%ld endtime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime);

						if( fpDataToUsb == NULL && fpHeaderToUsb == NULL)
						{

							g_UsbAvailableSize = FS_GetDiskUseSize(2);

							g_UsbAvailableSize = g_UsbAvailableSize * 1024 - 5 * 1024 * 1024;

							
							//sprintf(fileName,"mkdir /tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
							//		stUsbInfo.partitionID[0]); 		

							//command_ftc(fileName);    // mkdir /tddvr/hda5

							sprintf(fileName,"/tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0]);

							mkdir(fileName,0777);

							printf("command_ftc: %s\n",fileName);

							memset(fileName,0x00,50);

							printf("0\n");
						
							g_pstCommonParam->GST_DRA_get_sys_time( &nowTime, NULL );

							// 做usb 备份时，将生成两个文件，一个是帧头存储文件( .ANH )
							// 另一个的视频数据文件( .ANV )

							memset(timebuf,0,50);

							if( UsbWriteTime == 0 )
								UsbWriteTime = startTime;

							printf("1 UsbWriteTime = %ld\n",UsbWriteTime);

							FTC_ConvertTime_to_date(UsbWriteTime,timebuf);							

							sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avd",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0],timebuf);
							
							fpDataToUsb = fopen(fileName,"w+b");
							if( fpDataToUsb == NULL )
							{
								printf("open %s error!\n",fileName);
								return FILEERROR;
							}

							memset(fileName,0x00,50);
							sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avh",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0],timebuf);

							fpHeaderToUsb  = fopen(fileName,"w+b");
							if( fpHeaderToUsb == NULL )
							{
								printf("open %s error!\n",fileName);

								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}

							rel = fwrite(&tmp_fileinfo,1,sizeof(FILEINFO),fpHeaderToUsb);
							if( rel != sizeof(FILEINFO) )
							{
								fclose(fpHeaderToUsb);
								fpHeaderToUsb = NULL;
								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}

							HeaderToUsbOffset = sizeof(FILEINFO);							
							DataToUsbFileOffset = 0;

						}

						if(fpDataToUsb)
						{							
							printf("time %d,%d\n",startTime,endTime);
							rel = FS_GetDataFromFile(pFileInfoPoint->iDiskId,pFileInfoPoint->iPatitionId,
								pFileInfoPoint->iCurFilePos,iCam,startTime,endTime);

							if( rel < 0 )
							{
								if( fpDataToUsb )
								{
									fclose(fpDataToUsb);
									fpDataToUsb = NULL;										
								}

								if( fpHeaderToUsb )
								{
									fclose(fpHeaderToUsb);
									fpHeaderToUsb = NULL;										
								}
							
								if( rel == FILEFULL)
								{								
								}
								else		
								{
									printf(" usb umount  Sleep!\n");
									sleep(3);
								
									ret  = FS_UMountUsb();
									if( ret  < 0 )
									{
										printf("FS_UMountUsb error!\n");										
									}								

									printf(" usb umount sucess!\n");
									return rel;
								}
							}
							else
							{
								if( rel == USBBACKUPSTOP )
									goto BACKUPEND;
							}
						}
						
				}		
				else
				{
					if( iFilePos == k )
					{
						printf(" Backup error!\n");
						ret  = FS_UMountUsb();
						if( ret  < 0 )
						{
							printf("FS_UMountUsb error!\n");										
						}	
						return FILEERROR;
					}
					else
					{
						printf(" recfile is not in start end time! Backup End!\n");
						goto BACKUPEND;
					}
				}
				
				
			}
		}
	}

BACKUPEND:
	if( fpDataToUsb )
	{
		fclose(fpDataToUsb);
		fpDataToUsb = NULL;			
	}

	if( fpHeaderToUsb )
	{
		fclose(fpHeaderToUsb);
		fpHeaderToUsb = NULL;										
	}

	rel  = FS_UMountUsb();

	if( rel  < 0 )
	{
		printf("FS_UMountUsb error!\n");
		return rel;
	}

	return ALLRIGHT;
}


int FS_WriteDataToUsb(time_t startTime,time_t endTime, int iCam)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	GST_DISKINFO stUsbInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT     fileInfoPoint;
	int iInfoNumber = 0;
	int rel,ret;
	char fileName[500];
	 struct timeval nowTime;	 
	 long UsbSize = 0;
	 GST_TIME_TM mytime;
	 char timebuf[500];
	 FILEINFO tmp_fileinfo;

	 g_UsbAvailableSize = 0;
	 
	UsbWriteTime = 0;

	g_usb_write_date_size = 0;

	DISK_GetAllDiskInfo();

	rel = FS_MountUsb();
              
	if( rel < 0 )
	{
		printf("FS_MountUsb error!\n");
		return rel;
	}

	stUsbInfo = DISK_GetUsbInfo();
			
	iDiskNum = DISK_GetDiskNum();

	

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = 0; i < iDiskNum; i++ )
	{
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
					return ERROR;
			
			
			for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;
				
				if( FTC_CompareTime(startTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(endTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,startTime,endTime )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,startTime,endTime ))
				{
					if( pFileInfoPoint->pFilePos->states != WRITEOVER )
						continue;

				  	DPRINTK("k=%d starttime=%ld endtime=%ld\n",k, 
						pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime);

						if( fpDataToUsb == NULL && fpHeaderToUsb == NULL)
						{

							g_UsbAvailableSize = FS_GetDiskUseSize(2);

							g_UsbAvailableSize = g_UsbAvailableSize * 1024 - 5 * 1024 * 1024;

							
							//sprintf(fileName,"mkdir /tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
							//		stUsbInfo.partitionID[0]); 		

							//command_ftc(fileName);    // mkdir /tddvr/hda5

							sprintf(fileName,"/tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0]); 	

							mkdir(fileName,0777);


							//sprintf(fileName,"/tddvr/%s%d/AnaVideo/playback",&stUsbInfo.devname[5],
							//		stUsbInfo.partitionID[0]); 	 		

							//mkdir(fileName,0777);    // mkdir /tddvr/hda5/AnaVideo/playback
							
							sprintf(fileName,"cp -rf /mnt/mtd/playback.zip /tddvr/%s%d/AnaVideo/playback",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0]); 	 	

							command_ftc(fileName);    // mkdir /tddvr/hda5	

							if( rel < 0 )
							{
								DPRINTK("%s error\n",fileName);
							}else
								DPRINTK("%s ok\n",fileName);	

							printf("command_ftc: %s\n",fileName);
						

							memset(fileName,0x00,50);
						
							g_pstCommonParam->GST_DRA_get_sys_time( &nowTime, NULL );

							// 做usb 备份时，将生成两个文件，一个是帧头存储文件( .ANH )
							// 另一个的视频数据文件( .ANV )

							memset(timebuf,0,50);

							
							if( UsbWriteTime == 0)
							{
								if( FTC_CompareTime(startTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime) == 1)
										UsbWriteTime = startTime;
								else
										UsbWriteTime = pFileInfoPoint->pFilePos->StartTime;
							}

							FTC_ConvertTime_to_date(UsbWriteTime,timebuf);
							

							sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avd",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0],timebuf);
							
							fpDataToUsb = fopen(fileName,"w+b");
							if( fpDataToUsb == NULL )
							{
								printf("open %s error!\n",fileName);
								return FILEERROR;
							}							

							memset(fileName,0x00,50);
							sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avh",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0],timebuf);

							fpHeaderToUsb  = fopen(fileName,"w+b");
							if( fpHeaderToUsb == NULL )
							{
								printf("open %s error!\n",fileName);

								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}		

							tmp_fileinfo.ulVideoSize = g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video();;

							rel = fwrite(&tmp_fileinfo,1,sizeof(FILEINFO),fpHeaderToUsb);
							if( rel != sizeof(FILEINFO) )
							{
								fclose(fpHeaderToUsb);
								fpHeaderToUsb = NULL;
								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}
							
							HeaderToUsbOffset = sizeof(FILEINFO);
							DataToUsbFileOffset = 0;

						}

						

						if(fpDataToUsb)
						{								
						
							DPRINTK("time %ld,%ld\n",startTime,endTime);
							rel = FS_GetDataFromFile(pFileInfoPoint->iDiskId,pFileInfoPoint->iPatitionId,
								pFileInfoPoint->iCurFilePos,iCam,startTime,endTime);

							if( rel < 0 )
							{
								if( fpDataToUsb )
								{
									fclose(fpDataToUsb);
									fpDataToUsb = NULL;										
								}

								if( fpHeaderToUsb )
								{
									fclose(fpHeaderToUsb);
									fpHeaderToUsb = NULL;										
								}
							
								if( rel == FILEFULL)
								{								
								}
								else		
								{
									printf(" usb umount  Sleep!\n");
									sleep(3);
								
									ret  = FS_UMountUsb();
									if( ret  < 0 )
									{
										printf("FS_UMountUsb error!\n");										
									}		

									printf(" usb umount sucess!\n");
									
									return rel;
								}
							}
							else
							{
								if( rel == USBBACKUPSTOP )
									goto BACKUPEND;
							}
						}
						
				}		
				
				
			}
		}
	}

BACKUPEND:
	if( fpDataToUsb )
	{
		fclose(fpDataToUsb);
		fpDataToUsb = NULL;			
	}

	if( fpHeaderToUsb )
	{
		fclose(fpHeaderToUsb);
		fpHeaderToUsb = NULL;										
	}

	rel  = FS_UMountUsb();

	if( rel  < 0 )
	{
		printf("FS_UMountUsb error!\n");
		return rel;
	}

	return ALLRIGHT;
}

int FS_GetDataFromFile(int iDiskId, int iPartitionId,int iFilePos,int iCam,time_t startTime,time_t endTime)
{
	FILE * fp_head = NULL;
	FILE * fp_data = NULL;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT 	fileInfoPoint;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	 struct timeval nowTime;	
	 unsigned long ulFrameOffset = 0;
	 unsigned long ulEndOffset = 0;
	 FRAMEHEADERINFO  stFrameHeaderInfo;
	 char * databuf = NULL;
	 int iFirst = 0;
	 static int write_frame_num = 0;
	 DISK_IO_FILE fileheadst;
	 DISK_IO_FILE filedatast;
	 char dev[50];	

	 if( g_UsbBuf == NULL )
	 {
	 	return NOENOUGHMEMORY;
	 }

	databuf= g_UsbBuf;	

	 stDiskInfo = DISK_GetDiskInfo(iDiskId);

	FS_LogMutexLock();

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId,iPartitionId , iFilePos,&fileInfoPoint );
	if( pFileInfoPoint == NULL )
	{		
		FS_LogMutexUnLock();
		return ERROR;
	}

	sprintf(fileName,"/tddvr/%s%d/%s.avh",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);

	ulEndOffset = pFileInfoPoint->pFilePos->ulOffSet;

	DPRINTK("open %s file\n",fileName);

	FS_LogMutexUnLock();


	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avh",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );	
	if( file_io_open(dev,fileName,"r",&fileheadst,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto file_op_error;
	}


	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avd",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );	
	if( file_io_open(dev,fileName,"r",&filedatast,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto file_op_error;
	}
	

	ulFrameOffset = sizeof(FILEINFO);

	while(1)
	{		
		if( file_io_lseek(&fileheadst,  ulFrameOffset , SEEK_SET )== (off_t)-1 )
		{
			DPRINTK("seek error \n");
			rel = FILEERROR;
			goto file_op_error;
		}
		
		rel = file_io_read(&fileheadst,&stFrameHeaderInfo,sizeof(FRAMEHEADERINFO));
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			DPRINTK(" fread error\n");
			rel = FILESEARCHERROR;
			goto file_op_error;
		}			
		
		ulFrameOffset += sizeof(FRAMEHEADERINFO);
		if( ulFrameOffset >= ulEndOffset )
		{			 
			file_io_close(&fileheadst);
			file_io_close(&filedatast);	

			// 当写的USB文件大于128M 时要开新文件.
			if( DataToUsbFileOffset > 128 * 1024 * 1024 )
				return FILEFULL;
			
			return ALLRIGHT;
		}

		if( gUsbWrite != 1)
		{			 
			file_io_close(&fileheadst);
			file_io_close(&filedatast);
			printf("stop usb write\n");
			return USBBACKUPSTOP;
		}
				
		
		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ) == 0 )
				continue;

		// 确保第一帧为关键帧
		if( iFirst == 0 && stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V'
				&&  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) )
		{
			iFirst = 1;
		}

		if(  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) && iFirst == 1)
		{
		/*
			printf(" time %ld type=%c ichannel=%d ulFrametype=%d cp %d bit=%d\n",stFrameHeaderInfo.ulTimeSec,
			stFrameHeaderInfo.type,stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulFrameType ,
			FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ),FTC_GetBit(iCam,stFrameHeaderInfo.iChannel));
		*/
			if( file_io_lseek(&filedatast,  stFrameHeaderInfo.ulFrameDataOffset , SEEK_SET )== (off_t)-1 )
			{
				DPRINTK("seek error \n");
				rel = FILEERROR;
				goto file_op_error;
			}
		
			rel = file_io_read(&filedatast,databuf,stFrameHeaderInfo.ulDataLen);
			if( rel != stFrameHeaderInfo.ulDataLen )
			{
				DPRINTK(" fread error\n");
				rel = FILESEARCHERROR;
				goto file_op_error;
			}		

		
			stFrameHeaderInfo.ulFrameDataOffset = DataToUsbFileOffset ;		
				
			rel = fwrite(&stFrameHeaderInfo,1,sizeof(FRAMEHEADERINFO),fpHeaderToUsb);
			if( rel != sizeof(FRAMEHEADERINFO) )
			{				 
				file_io_close(&fileheadst);
				file_io_close(&filedatast);
				DPRINTK(" 3 fwrite  error!\n");				
				return FILEERROR;
			}

			fflush(fpHeaderToUsb);
			
			UsbWriteTime = stFrameHeaderInfo.ulTimeSec;
				
			HeaderToUsbOffset += sizeof(FRAMEHEADERINFO);

			rel = fwrite(databuf,1,stFrameHeaderInfo.ulDataLen,fpDataToUsb);
			if( rel != stFrameHeaderInfo.ulDataLen )
			{				 
				file_io_close(&fileheadst);
				file_io_close(&filedatast);
				DPRINTK(" 2 fwrite  error!\n");			
				return FILEERROR;
			}			

			fflush(fpDataToUsb);

			DataToUsbFileOffset += stFrameHeaderInfo.ulDataLen;			

			g_usb_write_date_size = g_usb_write_date_size +
				sizeof(FRAMEHEADERINFO) + stFrameHeaderInfo.ulDataLen;

			if( g_UsbAvailableSize < DataToUsbFileOffset + HeaderToUsbOffset  )
			{
				printf("usb no space!\n");				
				file_io_close(&fileheadst);
				file_io_close(&filedatast);
					
				return USBNOENOUGHSPACE;
			}

			write_frame_num++;

			if( write_frame_num > 3000)
			{
				sync();
				
				write_frame_num = 0;
				
				//usleep(10);
			}

			
		}
		
	}
	
	
	return ALLRIGHT;
	
file_op_error:
	file_io_close(&fileheadst);
	file_io_close(&filedatast);
	return FILEERROR;	
}


int FS_PlayFileGoToTimePoint(time_t startTimePoint,int iCam)
{
	int rel;
	int a,b,c,d,e,f;
	int i= 0,channel=0;
	FRAMEHEADERINFO  stFrameHeaderInfo;
	FRAMEHEADERINFO * pframeheader = NULL;
	char buf[10000];
	int max_read_num = 0;
	int frame_size = 0;
	int ready_read_num = 0;

	max_read_num = 10000 / sizeof(FRAMEHEADERINFO);	
	frame_size = sizeof(FRAMEHEADERINFO);

	if( file_io_lseek(&play_header_filest,  ulPlayFrameHeaderOffset , SEEK_SET )== (off_t)-1 )
		return FILEERROR;

	DPRINTK(" play iCam = %d  %ld\n",iCam,ulPlayFrameHeaderOffset);

	while(1)
	{
		//sd卡读写都比较慢所以循环多的地方要注意查看状态。
		if( gPlaystart != 1 )
		{
			DPRINTK(" gPlaystart =  %d!\n",gPlaystart);
			return FILEERROR;
		}		

		g_PlayStatusCurrent = 1;

		if(( ulPlayFrameHeaderOffset + max_read_num*frame_size) < ulPlayFrameHeaderEndOffset )
		{
			ready_read_num = max_read_num;
		}else
		{
			ready_read_num = (ulPlayFrameHeaderEndOffset - ulPlayFrameHeaderOffset)/frame_size;
		}

		DPRINTK("ready_read_num = %d\n",ready_read_num);

		if( ready_read_num <= 0 )
		{
			DPRINTK(" read over!\n");
			return FILEERROR;
		}

		memset(buf,0x00,10000);	
		
		rel =file_io_read(&play_header_filest,buf,ready_read_num*frame_size );

		if( rel != ready_read_num*frame_size )
		{
			DPRINTK(" read over!\n");
			return FILEERROR;
		}	

		for( i = 0; i < ready_read_num; i++ )
		{
			pframeheader = (FRAMEHEADERINFO *)(buf + i*frame_size);

			memcpy(&stFrameHeaderInfo,pframeheader,frame_size);

			if( i == 0 )
			{
				DPRINTK("chan=%d num=%d frametype=%d type=%c \n",stFrameHeaderInfo.iChannel,
					stFrameHeaderInfo.ulFrameNumber,stFrameHeaderInfo.ulFrameType,stFrameHeaderInfo.type);
			}

			if( stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V' 
			&&  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel)  )
			{
				if( stFrameHeaderInfo.ulTimeSec > startTimePoint - 2)
				{
						DPRINTK(" find play point %ld chan=%d\n", stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.iChannel );
						return ALLRIGHT;
				}
			}

			ulPlayFrameHeaderOffset += sizeof( FRAMEHEADERINFO );

			if( ulPlayFrameHeaderOffset >= ulPlayFrameHeaderEndOffset - 100)
			{
				DPRINTK(" read over! 2\n");
				return CANTFINDTIMEPOINT;
			}
		}

		usleep(10);	
	}

	return ALLRIGHT;
}


int FS_DiskFormatIsOk(int iDiskId)
{
	return g_iDiskIsRightFormat[iDiskId];		
}

int FS_PhysicsFixDisk(char * devName)
{

	GST_DISKINFO stDiskInfo;

	char cmd[50];

	int rel;

	memset(cmd, 0, 50);


	#ifdef USE_EXT3
		sprintf(cmd,"fsck.ext3 -a %s", devName); // maybe devName is /tddvr/hda5
	#else
		sprintf(cmd,"/mnt/mtd/dosfsck -a -m %s", devName); // maybe devName is /tddvr/hda5
	#endif
	

	rel = command_ftc(cmd);

	if( rel == -1 )
	{
		printf(" %s error!\n",cmd);
		return ERROR;
	}
/*
	int   pid;
	pid = vfork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
		return ALLRIGHT;
	} 
	else if (pid ==0) 
	{
		printf("PhysicsFix %s\n",devName);
		execl("/bin/dosfsck", "dosfsck", "-a", devName, (char* )0);		
		exit(1);
	}
*/
	return ALLRIGHT;	
}

int FS_PhysicsFixDisk_LowMemory(char * devName)
{
/*
	GST_DISKINFO stDiskInfo;

	char cmd[50];

	int rel;

	memset(cmd, 0, 50);

	sprintf(cmd,"dosfsck -Vva %s", devName); // maybe devName is /tddvr/hda5

	printf("%s\n",cmd);

	rel = command_ftc(cmd);

	if( rel == -1 )
	{
		printf(" %s error!\n",cmd);
		return ERROR;
	}
*/
	int   pid;
	pid = vfork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
		return ALLRIGHT;
	} 
	else if (pid ==0) 
	{
		printf("PhysicsFix  low memory %s\n",devName);
		execl("/bin/dosfsck_low", "dosfsck_low", "-a","-m", devName, (char* )0);		
		exit(1);
	}

	return ALLRIGHT;	
}

int FS_CheckShutdownStatus(int iDiskId,DISKFIXINFO* stFixInfo)
{
	int iDiskNum = 0;
	int j;	
	FILE * fp = NULL;
	GST_DISKINFO * buf = NULL;
	char devName[30];
	int   rel=0;
	char cmd[50];
	char fileName[30];
	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(iDiskId );
	
	memset(devName, 0, 30);

	sprintf(devName,"/tddvr/%s5", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	memset(cmd,0x00,50);

	//sprintf(cmd,"mkdir %s",devName);

	//rel = command_ftc(cmd);    // mkdir /tddvr/hda5
	mkdir(devName,0777);

	memset(cmd,0x00,50);

	//sprintf(cmd,"mount -t vfat %s5 %s",stDiskInfo.devname,devName);  // mount /dev/hda5

	//rel = command_ftc(cmd);

	sprintf(cmd,"%s5",stDiskInfo.devname); 

	rel = ftc_mount(cmd,devName);

	if( rel == -1 )
	{
			printf(" can't mount %s", stDiskInfo.devname);
			return MOUNTERROR;
	}

	sprintf(cmd,"%s/shutdown",devName);

	DPRINTK(" open %s\n",cmd);

	fp = fopen(cmd,"rb");
	if( fp == NULL )
	{
		umount( devName );
		return ALLRIGHT;
	}
	else
	{
		DPRINTK(" have this file %s need fix disk\n",cmd);

		rel = fread(stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" 2read stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			umount( devName );
			return FILEERROR;
		}
		fclose(fp);
		fp = NULL;

		DPRINTK("stFixInfo.create=%d,rec=%d\n",stFixInfo->iCreatePartiton,stFixInfo->iRecPartiton);
		
		umount( devName );
		return ALLRIGHT;
	}	
	

	return ALLRIGHT;
}


int FS_ClearDiskFixFile(int iDiskId)
{
	int iDiskNum = 0;
	int j;	
	FILE * fp = NULL;
	GST_DISKINFO * buf = NULL;
	char devName[30];
	int   rel=0;
	char cmd[50];
	char fileName[30];
	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(iDiskId );
	
	memset(devName, 0, 30);

	sprintf(devName,"/tddvr/%s5", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	memset(cmd,0x00,50);

	//sprintf(cmd,"mkdir %s",devName);

	//rel = command_ftc(cmd);    // mkdir /tddvr/hda5

	mkdir(devName,0777);

	memset(cmd,0x00,50);

	//sprintf(cmd,"mount -t vfat %s5 %s",stDiskInfo.devname,devName);  // mount /dev/hda5

	//rel = command_ftc(cmd);

	sprintf(cmd,"%s5",stDiskInfo.devname); 

	rel = ftc_mount(cmd,devName);

	if( rel == -1 )
	{
			printf(" can't mount %s", stDiskInfo.devname);
			return MOUNTERROR;
	}

	sprintf(cmd,"rm -rf %s/shutdown",devName);	

	rel = command_ftc(cmd);

//	rel = remove(cmd);
	if( rel < 0 )
	{
		DPRINTK("remove %s error\n",cmd);
	}else
	{
		DPRINTK("remove %s ok\n",cmd);
	}

	umount( devName );	

	return ALLRIGHT;
}


int FS_UseLogToFixDisk()
{
	GST_DISKINFO stDiskInfo;
	DISKCURRENTSTATE   stDiskState;
	int iDiskNum;	
	int i,j,k;	
	char DevName[50];
	int rel,rel0;
	int error = 100;
	DISKFIXINFO stFixInfo;
	int rec_disk_partition = 0;
	
	DISK_GetAllDiskInfo();  

	iDiskNum = DISK_GetDiskNum();

	for(i = 0 ; i < iDiskNum; i++ )
	{	
		stFixInfo.iCreatePartiton = 0;
		stFixInfo.iRecPartiton = 0;
		stFixInfo.iCreateCDPartition = 0;

		rec_disk_partition = 0;

		rel = FS_CheckNewDisk(i);
		DPRINTK("check is new disk %d\n",rel);
		if(  rel == YES )
			continue;

		rel = FS_CheckShutdownStatus(i,&stFixInfo);

/*
		if( rel == FILEERROR )
		{
		//	printf(" FS_CheckShutdownStatus read error!\n");			

			//DPRINTK("use disk info to check disk!\n");

			FS_MountAllPatition(i);

			rel0 = FS_GetDiskCurrentState(i, &stDiskState);

			FS_UMountAllPatition(i);
			
			if( (rel0 < 0) &&  (rel == FILEERROR) )
			{
				DPRINTK("fix info not find\n");
				continue;
			}

			if( stDiskState.IsRecording == YES )
				rec_disk_partition  = stDiskState.iPartitionId;
		
		}
			
	*/	
		if( rel >= 0 )
		{

			DPRINTK("DiskID=%d iCreatePartiton =%d, iRecPartiton =%d iCreateCDPartition =%d rec_disk_partition=%d\n",
					i, stFixInfo.iCreatePartiton,stFixInfo.iRecPartiton,stFixInfo.iCreateCDPartition,rec_disk_partition);
		
			stDiskInfo = DISK_GetDiskInfo(i);
				
			for(j = 1; j < stDiskInfo.partitionNum; j++ )
			{
				sprintf(DevName,"%s%d",stDiskInfo.devname, stDiskInfo.partitionID[j]);
				DPRINTK("%s \n",DevName);
				if( stFixInfo.iCreatePartiton == stDiskInfo.partitionID[j] || stFixInfo.iRecPartiton ==  stDiskInfo.partitionID[j] || 
					stFixInfo.iCreateCDPartition == stDiskInfo.partitionID[j] || rec_disk_partition ==  stDiskInfo.partitionID[j]  )
				{			
						FS_PhysicsFixDisk(DevName);
				}
			}			
		}

		//if( rel < 0 )
		{
			//清除硬盘修复文件，避免启动不成功第二次修复
			FS_ClearDiskFixFile(i);
			//error = -1;
		}
		
	}


	return error;
}

int FS_UseWriteFileToFixDisk()
{
	GST_DISKINFO stDiskInfo;
	DISKCURRENTSTATE   stDiskState;
	int iDiskNum;	
	int i,j,k;	
	char DevName[50];
	char cmd[50];
	int rel,rel0;
	int error = 100;
	DISKFIXINFO stFixInfo;
	int rec_disk_partition = 0;
	FILE * fp = NULL;
	
	DISK_GetAllDiskInfo();  

	iDiskNum = DISK_GetDiskNum();

	for(i = 0 ; i < iDiskNum; i++ )
	{	
		stFixInfo.iCreatePartiton = 0;
		stFixInfo.iRecPartiton = 0;
		stFixInfo.iCreateCDPartition = 0;

		rec_disk_partition = 0;

		rel = FS_CheckNewDisk(i);
		DPRINTK("check is new disk %d\n",rel);
		if(  rel == YES )
			continue;		

	

		rel = FS_MountAllPatition(i);
		if( rel < 0 )
		{
			DPRINTK("mount disk=%d error!",i);
			return -1;
		} 
	
		stDiskInfo = DISK_GetDiskInfo(i);
			
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			sprintf(DevName,"/tddvr/%s%d/checkdisk",&stDiskInfo.devname[5], stDiskInfo.partitionID[j]);
			DPRINTK("%s \n",DevName);


			fp = fopen(DevName,"wb");
			if( fp == NULL )
			{	
				DPRINTK("create %s error,disk need fix \n",DevName);			

				{	
					sprintf(DevName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5

					rel = umount(DevName);

					if( rel == -1 )
					{
						printf(" can't umount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
						return ERROR;
					}
				}

				sprintf(DevName,"%s%d",stDiskInfo.devname, stDiskInfo.partitionID[j]);
				
				rel =FS_PhysicsFixDisk(DevName);
				if( rel < 0 )
				{
					DPRINTK("mount disk=%d error!",i);
					return -1;
				}

				{							
					memset(DevName, 0, 30);

					sprintf(DevName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5

					memset(cmd,0x00,50);					

					mkdir(DevName,0777);

					memset(cmd,0x00,50);					
					
					sprintf(cmd,"%s%d",stDiskInfo.devname,stDiskInfo.partitionID[j]); 

					rel = ftc_mount(cmd,DevName);		
					
					if( rel == -1 )
					{
						printf(" can't mount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
						return ERROR;
					}		
				}
			}else
			{
				fclose(fp);
				fp = NULL;
			}
		
		}

		rel = FS_UMountAllPatition(i);
		if( rel < 0 )
		{
			DPRINTK("umount disk=%d error!",i);
			return -1;
		}	
		
	}
	
	return 1;
}


int FS_PhysicsFixAllDisk()
{
	#ifdef USE_LOG_TO_FIX_DISK
		return FS_UseLogToFixDisk();
	#else
		return FS_UseWriteFileToFixDisk();
	#endif
}

int FS_DelRecLogByFilePos(int iDiskId,int iPartition,int iFilePos)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{	
		if( iDiskId != i )
			continue;		
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );

			if( stDiskInfo.partitionID[j]  != iPartition )
				continue;
						
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
		
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

				if( pRecPoint->pFilePos->filePos == iFilePos )
				{
					pRecPoint->pFilePos->endTime = 0;
					pRecPoint->pFilePos->filePos   = -1;
					pRecPoint->pFilePos->iCam    = 0;
					pRecPoint->pFilePos->iDataEffect = NOEFFECT;
					pRecPoint->pFilePos->startTime = 0;
					pRecPoint->pFilePos->ulOffSet   = 0;
					pRecPoint->pFilePos->videoStandard = 0;
					pRecPoint->pFilePos->imageSize = 0;

					rel = RecordMessageWriteToFile();
					if( rel < 0 )
					{
						printf("RecordMessageWriteToFile error!\n");
						return rel;
					}
				}						
				
			}
		}
	}
	
	return ALLRIGHT;
}

int FS_DelRecLosMessage(time_t startTime,time_t endTime,int iDiskId)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{
	// 这步是处理硬盘log 时，防止其它硬盘中的log被清理
		if( iDiskId != -1 )
		{
			if( iDiskId != i )
				continue;
		}

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
						
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;

			if( iDiskId == i )
			{
				pRecPoint->pUseinfo->id = 0;
				rel = RecordMessageUseInfoWriteToFile();
				if( rel < 0 )
				{
					printf("RecordMessageUseInfoWriteToFile error!\n");
					return rel;
				}
			}
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,startTime,endTime )
				     || FTC_CompareTime(pRecPoint->pFilePos->endTime,startTime,endTime )
				     || FTC_CompareTime(startTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime)
				     || FTC_CompareTime(endTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime)
				     ||  ( iDiskId == i ))
				{			
					//DPRINTK("clear log k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);
				
					pRecPoint->pFilePos->endTime = 0;
					pRecPoint->pFilePos->filePos   = -1;
					pRecPoint->pFilePos->iCam    = 0;
					pRecPoint->pFilePos->iDataEffect = NOEFFECT;
					pRecPoint->pFilePos->startTime = 0;
					pRecPoint->pFilePos->ulOffSet   = 0;
					pRecPoint->pFilePos->videoStandard = 0;
					pRecPoint->pFilePos->imageSize = 0;
					pRecPoint->pFilePos->have_add_info = 0;
					pRecPoint->pFilePos->rec_mode = 0;

					rel = RecordMessageWriteToFile();
					if( rel < 0 )
					{
						printf("RecordMessageWriteToFile error!\n");
						return rel;
					}										
				}				
				
			}
		}
	}
	
	return ALLRIGHT;
}

int FS_DelRecAddLosMessage(time_t startTime,time_t endTime,int iDiskId)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECADDLOGPOINT * pRecPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{
	// 这步是处理硬盘log 时，防止其它硬盘中的log被清理
		if( iDiskId != -1 )
		{
			if( iDiskId != i )
				continue;
		}

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
						
			pRecPoint = FS_GetPosRecAddMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;

			if( iDiskId == i )
			{
				pRecPoint->pUseinfo->id = 0;
				rel = RecAddMessageUseInfoWriteToFile();
				if( rel < 0 )
				{
					printf("RecAddMessageUseInfoWriteToFile error!\n");
					return rel;
				}
			}
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosRecAddMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,startTime,endTime )
				     ||   ( iDiskId == i ))
				{			
					//printf("clear log k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);
				
					pRecPoint->pFilePos->rec_log_id = -1;
					pRecPoint->pFilePos->iCam = 0;
					pRecPoint->pFilePos->rec_size = 0;
					pRecPoint->pFilePos->startTime = 0;
					rel = RecAddMessageWriteToFile();
					if( rel < 0 )
					{
						printf("RecAddMessageWriteToFile error!\n");
						return rel;
					}										
				}				
				
			}
		}
	}
	
	return ALLRIGHT;
}


int FS_DelRecTimeStickMessage(time_t startTime,time_t endTime,int iDiskId)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECTIMESTICKLOGPOINT * pRecPoint;
	RECTIMESTICKLOGPOINT   RecPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{
	// 这步是处理硬盘log 时，防止其它硬盘中的log被清理
		if( iDiskId != -1 )
		{
			if( iDiskId != i )
				continue;
		}

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
						
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0,&RecPoint );
			if( pRecPoint == NULL )
					return ERROR;

			if( iDiskId == i )
			{
				pRecPoint->pUseinfo->id = 0;
				rel = RecTimeStickMessageUseInfoWriteToFile(pRecPoint);
				if( rel < 0 )
				{
					printf("RecTimeStickMessageUseInfoWriteToFile error!\n");
					return rel;
				}
			}
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k ,&RecPoint );

				if( pRecPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,startTime,endTime )||
					FTC_CompareTime(pRecPoint->pFilePos->startTime+300,startTime,endTime ) || 
					FTC_CompareTime(startTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->startTime+300 )||
					FTC_CompareTime(endTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->startTime+300  ) || 
					( iDiskId == i ))
				{			
					
					if( pRecPoint->pFilePos->startTime > 300 )
					{				
						DPRINTK("clear log k=%d starttime=%ld\n",k, pRecPoint->pFilePos->startTime);

						pRecPoint->pFilePos->iCam = 0;			
						pRecPoint->pFilePos->startTime = 0;
						rel = RecTimeStickMessageWriteToFile(pRecPoint);
						if( rel < 0 )
						{
							printf("RecTimeStickMessageWriteToFile error!\n");
							return rel;
						}	

						#ifdef USE_PLAYBACK_QUICK_SYSTEM
						FTC_rec_write_log_change_disk_rec_time(0,i,stDiskInfo.partitionID[j]);	
						#endif
					
					}else
					{
						if( pRecPoint->pFilePos->startTime > 0 )
						{
							DPRINTK("clear log k=%d starttime=%ld\n",k, pRecPoint->pFilePos->startTime);

							pRecPoint->pFilePos->iCam = 0;			
							pRecPoint->pFilePos->startTime = 0;
							rel = RecTimeStickMessageWriteToFile(pRecPoint);
							if( rel < 0 )
							{
								printf("RecTimeStickMessageWriteToFile error!\n");
								return rel;
							}	
							#ifdef USE_PLAYBACK_QUICK_SYSTEM
							FTC_rec_write_log_change_disk_rec_time(0,i,stDiskInfo.partitionID[j]);	
							#endif
						}
						//DPRINTK("pRecPoint->pFilePos->startTime <  300 continue!\n ");
					}
				}				
				
			}
		}
	}
	
	return ALLRIGHT;
}


int FS_DelAlarmLosMessage(time_t startTime,time_t endTime,int iDiskId)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	ALARMLOGPOINT * pRecPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		// 这步是处理硬盘log 时，防止其它硬盘中的log被清理
		if( iDiskId != -1 )
		{
			if( iDiskId != i )
				continue;
		}

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif

		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
						
			pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;

			if( iDiskId == i )
			{
				pRecPoint->pUseinfo->id = 0;
				rel = AlarmMessageUseInfoWriteToFile();
				if( rel < 0 )
				{
					printf("AlarmMessageUseInfoWriteToFile error!\n");
							return rel;
				}
			}
			
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->alarmStartTime,startTime,endTime )
				     	|| FTC_CompareTime(pRecPoint->pFilePos->alarmEndTime,startTime,endTime )
					|| FTC_CompareTime(startTime,pRecPoint->pFilePos->alarmStartTime,pRecPoint->pFilePos->alarmEndTime )
					|| FTC_CompareTime(endTime,pRecPoint->pFilePos->alarmStartTime,pRecPoint->pFilePos->alarmEndTime )
					|| ( iDiskId == i ) )
				{		
				
					pRecPoint->pFilePos->alarmEndTime = 0;
					pRecPoint->pFilePos->alarmStartTime = 0;
					pRecPoint->pFilePos->event.loss = 0;
					pRecPoint->pFilePos->event.motion = 0;
					pRecPoint->pFilePos->event.sensor = 0;
					pRecPoint->pFilePos->iCam   = 0;
					pRecPoint->pFilePos->iDataEffect = NOEFFECT;
					pRecPoint->pFilePos->iLaterTime = 0;
					pRecPoint->pFilePos->iPreviewTime = 0;
					pRecPoint->pFilePos->states       = NORECORD;
					pRecPoint->pFilePos->iRecMessageId = -1;
					
					rel = AlarmMessageWriteToFile();
					if( rel < 0 )
					{
						printf("AlarmMessageWriteToFile error!\n");
						return rel;
					}										
				}				
				
			}
		}
	}
	
	return ALLRIGHT;
}


int FS_DelFileLosMessage(int iDiskId)
{
	int i,j,k,rel;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT * pFileInfoPoint;
	FILEINFOLOGPOINT  fileInfoPoint;

	iDiskNum = DISK_GetDiskNum();

	// iDiskId 为-1 时，表示按startTime和endTime来清理log.
	// iDiskId 为硬盘号时,表示按硬盘来清理log
	if( iDiskId >= iDiskNum || iDiskId < -1 )
	{
		printf(" wrong iDiskId!\n");
		return ERROR;
	}

	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		// 这步是处理硬盘log 时，防止其它硬盘中的log被清理
		if( iDiskId != -1 )
		{
			if( iDiskId != i )
				continue;
		}

		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
						
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0,&fileInfoPoint );
			if( pFileInfoPoint == NULL )
					return ERROR;

			if( iDiskId == i )
			{
				pFileInfoPoint->pUseinfo->iCurFilePos = 0;
				rel = UseInfoWriteToFile(pFileInfoPoint);
				if( rel < 0 )
				{
						printf(" UseInfoWriteToFile error!");
						return rel;
				}
			}
				
			for(k = pFileInfoPoint->pUseinfo->iTotalFileNumber - 1; k >= 0 ; k--)
			{	
				
				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				pFileInfoPoint->pFilePos->EndTime = 0;
				pFileInfoPoint->pFilePos->iDataEffect = NOEFFECT;
				pFileInfoPoint->pFilePos->isCover = NO;
				pFileInfoPoint->pFilePos->StartTime = 0;			
				pFileInfoPoint->pFilePos->ulOffSet   = 0;
			//	printf("pos = %d  pFilePos->states = %d\n",pFileInfoPoint->iCurFilePos,	pFileInfoPoint->pFilePos->states);

				// 没有创建 的文件和已经损坏的文件将不会改状态。
				if( pFileInfoPoint->pFilePos->states != NOCREATE &&
						pFileInfoPoint->pFilePos->states != BADFILE)
				{					
					pFileInfoPoint->pFilePos->states = NOWRITE;					
				}	

				rel = FS_FileInfoWriteToFile(pFileInfoPoint);
				if( rel < 0 )
				{
					printf(" FS_FileInfoWriteToFile error! 349\n");
					return FILEERROR;
				}								
				
			}
		}
	}
	
	return ALLRIGHT;
}

int FS_CoverFileClearLog(time_t startTime,time_t endTime,int iDiskId)
{
	int rel;
	long value;

	if( startTime == 0)
	{
		DPRINTK("startTime = 0  set 301\n");
		startTime = 301;
	}

	if( endTime == 0)
	{
		DPRINTK("endTime = 0  set 301\n");
		endTime = 301;
	}

	if( endTime < startTime )
	{
		DPRINTK("endTime=%d startTime=%d so endTime=startTime+1\n",endTime,startTime);
		endTime = startTime + 1;
	}

	value = labs(endTime - startTime );

	printf("disk id=%d  %ld-%ld abs(%ld)\n",iDiskId,startTime,endTime,value);

	if( value > (24*3600) )
	{
		printf("time abs > one day\n");

		rel = FS_CoverFileClearLog(startTime,startTime+1800,iDiskId);
		if( rel < 0 )
			return rel;

		rel = FS_CoverFileClearLog(endTime,endTime+1800,iDiskId);
		if( rel < 0 )
			return rel;

		return ALLRIGHT;
	}
	

	rel = FS_DelRecLosMessage(startTime,endTime,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelRecLosMessage error!\n");
		return rel;
	}

/*	rel = FS_DelRecAddLosMessage(startTime,endTime,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelRecAddLosMessage error!\n");
		return rel;
	} */

	rel = FS_DelRecTimeStickMessage(startTime,endTime,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelRecTimeStickMessage error!\n");
		return rel;
	}
	
	rel = FS_DelAlarmLosMessage(startTime,endTime,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelAlarmLosMessage error!\n");
		return rel;
	}

	return ALLRIGHT;
}


int FS_ClearDiskLogMessage(int iDiskId)
{
	//各分区*.log的清理
	//硬盘diskinfo.log 的清理

	DISKCURRENTSTATE  stDiskState;
	int rel;

	DPRINTK("iDiskId=%d\n",iDiskId);

	rel = FS_DelRecLosMessage(0,0,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelRecLosMessage error!\n");
		return rel;
	}
	/*rel = FS_DelRecAddLosMessage(0,0,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelRecLosMessage error!\n");
		return rel;
	}*/

	rel = FS_DelRecTimeStickMessage(0,0,iDiskId);	
	if( rel < 0 )
	{
		printf("FS_DelRecTimeStickMessage error!\n");
		return rel;
	}

	rel = FS_DelAlarmLosMessage(0,0,iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelAlarmLosMessage error!\n");
		return rel;
	}

	rel = FS_DelFileLosMessage(iDiskId);
	if( rel < 0 )
	{
		printf("FS_DelFileLosMessage error!\n");
		return rel;
	}

	rel =  FS_GetDiskCurrentState(iDiskId, &stDiskState);
	if( rel < 0 )
	{
		printf("FS_GetDiskCurrentState error!\n");
		return rel;
	}

	stDiskState.iCoverCapacity = 0;
//	stDiskState.iEnableCover    = ENABLE;
	stDiskState.IsCover            = NO;
	stDiskState.IsRecording      =  NO;
	stDiskState.iUseCapacity    =  0;
	stDiskState.iPartitionId	     = 5;

	rel = FS_WriteDiskCurrentState(iDiskId, &stDiskState);
	if( rel < 0 )
	{
		printf(" FS_WriteDiskCurrentState error!\n");
		return rel;
	}
	
	return ALLRIGHT;
}

int FS_SearchIFrame(int iSpeed)
{
	if( iSpeed < 1 || iSpeed > 2 )
	{
		printf(" iSpeed = %d error!\n",iSpeed);
		return ERROR;
	}

	return ALLRIGHT;
}

int FS_SearchNextIFrame(int iCam)
{
	int rel;
	int a,b,c,d,e,f;	
	int i= 0,channel=15;
	
	

	FRAMEHEADERINFO  stFrameHeaderInfo;

	if( play_header_filest.fd <= 0 )
		return FILEERROR;

	if( ulPlayFrameHeaderOffset + 12*sizeof(FRAMEHEADERINFO) >= ulPlayFrameHeaderEndOffset - 100)
	{
		printf(" search to end of file\n");
		return FILESEARCHERROR;
	}

	//ulPlayFrameHeaderOffset = ulPlayFrameHeaderOffset + 12*sizeof(FRAMEHEADERINFO);

	if( file_io_lseek(&play_header_filest,  ulPlayFrameHeaderOffset , SEEK_SET )== (off_t)-1 )
		return FILEERROR;

	
/*
	for( i = 0 ; i < 16; i++ )
	{
		if(  FTC_GetBit(iCam,i) == 1 )
		{
			channel = i ;
			printf("the channel = %d\n",channel);
			break;
		}
	}

	if( i >= 16 )
	return CANTFINDTIMEPOINT;	
*/
	while(1)
	{
		rel =file_io_read(&play_header_filest,&stFrameHeaderInfo,sizeof( FRAMEHEADERINFO ) );
				
		if( rel != sizeof( FRAMEHEADERINFO ) )
			return FILEERROR;
		

		if( (stFrameHeaderInfo.imageSize != g_PlayMode.iPbImageSize) 
			|| (g_PlayMode.iPbVideoStandard != stFrameHeaderInfo.videoStandard) )
			return -101;


	//	printf("the channel = %d type=%c key=%d\n",stFrameHeaderInfo.iChannel,
	//		stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType);

	/*	if( channel > stFrameHeaderInfo.iChannel && FTC_GetBit(iCam,stFrameHeaderInfo.iChannel ) == 1 )
		{
			channel = stFrameHeaderInfo.iChannel;
			printf("the channel = %d\n",channel);
		}
	
		if( stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V' 
						&& channel == stFrameHeaderInfo.iChannel )
		{			
			printf("fast channel=%d type=%c frameType=%d\n",stFrameHeaderInfo.iChannel,
				stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType);
					return ALLRIGHT;			
		} */

		if( stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V' 
						&& FTC_GetBit(iCam,stFrameHeaderInfo.iChannel ) )
		{			
			//printf("fast channel=%d type=%c frameType=%ld %x\n",stFrameHeaderInfo.iChannel,
			//	stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,iCam);
					return ALLRIGHT;			
		} 
	
		ulPlayFrameHeaderOffset += sizeof( FRAMEHEADERINFO );

		if( ulPlayFrameHeaderOffset >= ulPlayFrameHeaderEndOffset - 100)
		{
			printf(" search to end change file\n");
			return FILESEARCHERROR;			
		}
		
	}

	return ALLRIGHT;
}

int FS_SearchLastIFrame(int iCam)
{
	int rel;
	int a,b,c,d,e,f;
	int i= 0,channel=3;
	int image_size,standard;

	FRAMEHEADERINFO  stFrameHeaderInfo;

	if( play_header_filest.fd <= 0 )
		return FILEERROR;

	if( ulPlayFrameHeaderOffset - 12*sizeof(FRAMEHEADERINFO) <=  FRAMEHEADOFFSET )
	{
		printf(" search to head of file\n");
		return FILESEARCHERROR;
	}

	ulPlayFrameHeaderOffset = ulPlayFrameHeaderOffset - 2*sizeof(FRAMEHEADERINFO);



	channel = g_pstCommonParam->GST_DRA_get_dvr_max_chan(1) -1;
		

	while(1)
	{
		if( ulPlayFrameHeaderOffset  <=  150)
		{
			DPRINTK(" search to file start  change file ulPlayFrameHeaderOffset=%ld \n",ulPlayFrameHeaderOffset);
			return FILESEARCHERROR;			
		}

		if( file_io_lseek(&play_header_filest,  ulPlayFrameHeaderOffset , SEEK_SET )== (off_t)-1 )
			return FILEERROR;		


		rel =file_io_read(&play_header_filest,&stFrameHeaderInfo,sizeof( FRAMEHEADERINFO ) );
				
		if( rel != sizeof( FRAMEHEADERINFO ) )
			return FILEERROR;	
		

		if( (stFrameHeaderInfo.imageSize != g_PlayMode.iPbImageSize) 
			|| (g_PlayMode.iPbVideoStandard != stFrameHeaderInfo.videoStandard) )
		{
			DPRINTK("ulPlayFrameHeaderOffset=%ld  (%d,%d) (%d,%d) %d %ld %c\n",ulPlayFrameHeaderOffset,
			stFrameHeaderInfo.imageSize,stFrameHeaderInfo.videoStandard,g_PlayMode.iPbImageSize,g_PlayMode.iPbVideoStandard,
			stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulDataLen,stFrameHeaderInfo.type);
			return -101;
		}

		/*if( channel > stFrameHeaderInfo.iChannel && FTC_GetBit(iCam,stFrameHeaderInfo.iChannel ) == 1 )
			{
				channel = stFrameHeaderInfo.iChannel;
				printf("the channel = %d\n",channel);
			}
		
	
		if( stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V' 
			&& channel == stFrameHeaderInfo.iChannel )
		{				
			printf("last channel=%d type=%c frameType=%d\n",stFrameHeaderInfo.iChannel,
				stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType);
					return ALLRIGHT;			
		}*/

		if( stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V' 
			&& FTC_GetBit(iCam,stFrameHeaderInfo.iChannel ) )
		{				
			//printf("fast channel=%d type=%c frameType=%d %x num=%d\n",stFrameHeaderInfo.iChannel,
			//	stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,iCam,stFrameHeaderInfo.ulFrameNumber);
					return ALLRIGHT;				
		}
	
		ulPlayFrameHeaderOffset -= sizeof( FRAMEHEADERINFO );

		if( ulPlayFrameHeaderOffset  <=  150)
		{
			DPRINTK(" search to file start  change file ulPlayFrameHeaderOffset=%ld \n",ulPlayFrameHeaderOffset);
			return FILESEARCHERROR;			
		}
		
	}

	return ALLRIGHT;
}

int FS_PlayMoveToEnd()
{
	if( play_header_filest.fd <= 0 )
		return FILEERROR;

	ulPlayFrameHeaderOffset = ulPlayFrameHeaderEndOffset ;

	printf(" ulPlayFrameHeaderOffset = %ld\n",ulPlayFrameHeaderOffset);

	return ALLRIGHT;
}


long long  FS_GetDataSize(time_t *TimeStart,time_t *TimeEnd,int backup_cam,int cd_mode)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	GST_DISKINFO stUsbInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	time_t startTime,endTime;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	int iCam = 0x0f;
	 struct timeval nowTime;	 
	 long long lDataLength = 0;
	DISK_GetAllDiskInfo();

	startTime = *TimeStart;
	endTime = *TimeEnd;
	iCam = backup_cam;

	printf("startTime = %ld  endTime = %ld cam = %d cd_mode=%d\n",startTime,endTime,backup_cam,cd_mode);

	cdrom_mode = cd_mode;
				
	iDiskNum = DISK_GetDiskNum();

	DataToUsbFileOffset = 0;	

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = 0; i < iDiskNum; i++ )
	{
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(startTime,endTime,i) == 0 )			
			continue;		
		#endif
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			printf("j = %d\n",j);
		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
					return ERROR;
			
			
			for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;
				
				if( FTC_CompareTime(startTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(endTime,pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,startTime,endTime )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,startTime,endTime ))
				{
					if( pFileInfoPoint->pFilePos->states != WRITEOVER )
						continue;

				 	printf("k=%d starttime=%ld endtime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime);

													
					printf("time %d,%d icam=%d\n",startTime,endTime,iCam);
					rel = FS_GetDataSizeFromFile(pFileInfoPoint->iDiskId,pFileInfoPoint->iPatitionId,
								pFileInfoPoint->iCurFilePos,iCam,&startTime,&endTime,cd_mode);

					if( rel < 0 )
					{	
						if( rel == FILEFULL )
						{
							printf("2 Data size is %lld\n",DataToUsbFileOffset);
							*TimeEnd = endTime;
							return DataToUsbFileOffset;
						}
							
						return FILEERROR;								
					}						
						
				}				
				
			}
		}
	}

	printf("1 Data size is %lld\n",DataToUsbFileOffset);

	DataToUsbFileOffsetSave = DataToUsbFileOffset;

	return DataToUsbFileOffset;
}

int FS_GetDataSizeFromFile(int iDiskId, int iPartitionId,int iFilePos,int iCam,time_t *TimeStart,time_t *TimeEnd,int cd_mode)
{
	FILE * fp = NULL;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	 struct timeval nowTime;	
	 unsigned long ulFrameOffset = 0;
	 unsigned long ulEndOffset = 0;
	 FRAMEHEADERINFO  stFrameHeaderInfo;
	 time_t startTime,endTime;
	 char * databuf = NULL;
	 int iFirst = 0;
	 long long cd_size = 0;

	#ifdef USE_FILE_IO_SYS

	DISK_IO_FILE filest;
	DISK_IO_FILE filest2;
	char dev[50];	
	
	#endif

	 switch(cd_mode){
	 	case 0: cd_size = DVD_SIZE; break;
		case 1: cd_size = CD_SIZE; break;
		case 2: cd_size = MD_SIZE; break;
		default: cd_size = CD_SIZE; break;
	 	
	 }

	 startTime = *TimeStart;
	 endTime  = *TimeEnd;
	
	 if( g_UsbBuf == NULL )
	 {
	 	return NOENOUGHMEMORY;
	 }

	databuf= g_UsbBuf;	

	#ifdef USE_FILE_IO_SYS

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	FS_LogMutexLock();

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId,iPartitionId , iFilePos ,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
	{
		 FS_LogMutexUnLock();
		return ERROR;
	}

	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avh",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );	
	ulEndOffset = pFileInfoPoint->pFilePos->ulOffSet;

	DPRINTK("open %s file\n",fileName);

	FS_LogMutexUnLock();
	

	if( file_io_open(dev,fileName,"+",&filest,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}

	ulFrameOffset = sizeof(FILEINFO);

	while(1)
	{		

		if( file_io_lseek(&filest,  ulFrameOffset , SEEK_SET ) == (off_t)-1  )
		{
			DPRINTK("seek error \n");			
			goto file_op_error;
		}

		rel = file_io_read(&filest,&stFrameHeaderInfo,sizeof(FRAMEHEADERINFO));
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			DPRINTK(" fread error \n");			
			goto file_op_error;
		}
	
		
		ulFrameOffset += sizeof(FRAMEHEADERINFO);
		if( ulFrameOffset >= ulEndOffset )
		{
			 
			file_io_close(&filest);	
			
			return ALLRIGHT;
		}
				
		
		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ) == 0 )
				continue;

		// 确保第一帧为关键帧
		if( iFirst == 0 && stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V'
				&&  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) )
		{
			iFirst = 1;
		}

		if(  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) && iFirst == 1)
		{
		/*
			printf(" time %ld type=%c ichannel=%d ulFrametype=%d cp %d bit=%d\n",stFrameHeaderInfo.ulTimeSec,
			stFrameHeaderInfo.type,stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulFrameType ,
			FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ),FTC_GetBit(iCam,stFrameHeaderInfo.iChannel));
*/
			DataToUsbFileOffset = DataToUsbFileOffset + sizeof(FRAMEHEADERINFO) + stFrameHeaderInfo.ulDataLen;

			if( DataToUsbFileOffset > cd_size )
			{
				*TimeEnd = stFrameHeaderInfo.ulTimeSec;
				
				file_io_close(&filest);
				return FILEFULL;
			}
		//	printf("DataToUsbFileOffset %lld\n",DataToUsbFileOffset);
		}
		
	}	

	file_io_close(&filest);	

	return ALLRIGHT;

file_op_error:

	file_io_close(&filest);	

	return ERROR;

	#else
	
	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	FS_LogMutexLock();

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId,iPartitionId , iFilePos ,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
	{		
		FS_LogMutexUnLock();
		return ERROR;
	}

	sprintf(fileName,"/tddvr/%s%d/%s.avh",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);

	ulEndOffset = pFileInfoPoint->pFilePos->ulOffSet;

	printf("open %s file\n",fileName);

	FS_LogMutexUnLock();

	fp = fopen(fileName,"rb");
	if( fp == NULL )
	{
		 
		printf("open %s error!\n");
		return FILEERROR;
	}

	ulFrameOffset = sizeof(FILEINFO);

	while(1)
	{
		
		rel = fseek(fp, ulFrameOffset, SEEK_SET);
		if ( rel < 0 )
		{
			 
			fclose(fp);
			printf("  fseek error!\n");
			return FILEERROR;
		}

		rel = fread(&stFrameHeaderInfo,1,sizeof(FRAMEHEADERINFO),fp);
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			 
			fclose(fp);
			printf("  fread  error!\n");
			return FILEERROR;
		}
		
		ulFrameOffset += sizeof(FRAMEHEADERINFO);
		if( ulFrameOffset >= ulEndOffset )
		{
			 
			fclose(fp);		
			
			return ALLRIGHT;
		}
				
		
		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ) == 0 )
				continue;

		// 确保第一帧为关键帧
		if( iFirst == 0 && stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V'
				&&  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) )
		{
			iFirst = 1;
		}

		if(  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) && iFirst == 1)
		{
		/*
			printf(" time %ld type=%c ichannel=%d ulFrametype=%d cp %d bit=%d\n",stFrameHeaderInfo.ulTimeSec,
			stFrameHeaderInfo.type,stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulFrameType ,
			FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ),FTC_GetBit(iCam,stFrameHeaderInfo.iChannel));
*/
			DataToUsbFileOffset = DataToUsbFileOffset + sizeof(FRAMEHEADERINFO) + stFrameHeaderInfo.ulDataLen;

			if( DataToUsbFileOffset > cd_size )
			{
				*TimeEnd = stFrameHeaderInfo.ulTimeSec;
				fclose(fp);	
				return FILEFULL;
			}
		//	printf("DataToUsbFileOffset %lld\n",DataToUsbFileOffset);
		}
		
	}	

	if( fp)
		fclose(fp);	

	return ALLRIGHT;

	#endif
	
	
}



int FS_GetErrorMessage(int * iError, time_t * sTime)
{
	int error = 0;
	
	if( g_CurrentError != 0 )
	{
		printf("1__GetErrorMessage Error = %d Time = %ld\n",g_CurrentError,g_ErrorTime);
		*iError = g_CurrentError;
		*sTime = g_ErrorTime;
		g_CurrentError = 0;
		g_ErrorTime = 0;
		return ALLRIGHT;
	}	

	*iError = 0;
	*sTime = 0;

	return ALLRIGHT;
}

int FS_SetErrorMessage(int iError,time_t sTime)
{
	// g_CurrentError == 0  是为了保证在上一条出错信息
	// 未被取走时，不再存入新的出错信息

	printf("1___ g_CurrentError = %d g_ErrorTime = %ld\n",g_CurrentError,g_ErrorTime);
	if( g_CurrentError == 0 )
	{
		g_CurrentError = iError;
		g_ErrorTime = sTime;
		printf("2___ g_CurrentError = %d g_ErrorTime = %ld\n",g_CurrentError,g_ErrorTime);
	}

	return ALLRIGHT;
}


int FS_CreateFileCurrentPartitionWriteToShutDownLog(int iDiskId, int iPartitionId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	FILE * fp = NULL;
	DISKFIXINFO stFixInfo;


	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/shutdown", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	fp = fopen(fileNameBuf,"r+b");
	if( fp == NULL )
	{
		fp = fopen(fileNameBuf,"w+b");
		if( fp == NULL )
		{
			printf(" create %s error!\n",fileNameBuf);
			return FILEERROR;
		}

		stFixInfo.iCreatePartiton = iPartitionId;
		stFixInfo.iRecPartiton 	= 0;
		stFixInfo.iCreateCDPartition = 0;

		printf("0 stFixInfo.iCreatePartiton = %d\n",iPartitionId);;

		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
		fclose(fp);
		return ALLRIGHT;
	}

	rel = fread(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
	if( rel != sizeof(DISKFIXINFO) )
	{
		printf(" read stfixinfo error! rel = %d\n",rel);
		fclose(fp);
		return FILEERROR;
	}

	printf("1 stFixInfo.iCreatePartiton = %d,recPartion =%d, iPartitionId = %d\n",stFixInfo.iCreatePartiton,stFixInfo.iRecPartiton,iPartitionId);;

	if( stFixInfo.iCreatePartiton != iPartitionId )
	{
		rewind(fp);

		printf("2 stFixInfo.iCreatePartiton = %d\n",iPartitionId);;

		stFixInfo.iCreatePartiton = iPartitionId;
		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
	}
	fclose(fp);
	return ALLRIGHT;	

}

int FS_RecFileCurrentPartitionWriteToShutDownLog(int iDiskId, int iPartitionId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	FILE * fp = NULL;
	DISKFIXINFO stFixInfo;


	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/shutdown", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	fp = fopen(fileNameBuf,"r+b");
	if( fp == NULL )
	{
		fp = fopen(fileNameBuf,"w+b");
		if( fp == NULL )
		{
			printf(" create %s error!\n",fileNameBuf);
			return FILEERROR;
		}

		printf("0 stFixInfo.iRecPartiton = %d\n",iPartitionId);;

		stFixInfo.iCreatePartiton = 0;
		stFixInfo.iRecPartiton 	= iPartitionId;
		stFixInfo.iCreateCDPartition = 0;

		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" 2write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
		fclose(fp);
		return ALLRIGHT;
	}

	rel = fread(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
	if( rel != sizeof(DISKFIXINFO) )
	{
		printf(" 2read stfixinfo error! rel = %d\n",rel);
		//fclose(fp);
		//return FILEERROR;
		stFixInfo.iRecPartiton = 0;
		printf(" read stfixinfo error! self set value!\n");
	}

	printf("1 stFixInfo.iRecPartiton = %d,createPart=%d,iPar=%d\n",
		stFixInfo.iRecPartiton,stFixInfo.iCreatePartiton,iPartitionId);

	if( stFixInfo.iRecPartiton != iPartitionId )
	{
		rewind(fp);

		printf("2 stFixInfo.iRecPartiton = %d\n",iPartitionId);;

		stFixInfo.iRecPartiton = iPartitionId;
		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
	}
	fclose(fp);
	
	return ALLRIGHT;	
}


int FS_CreateCDPartitionWriteToShutDownLog(int iDiskId, int iPartitionId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	FILE * fp = NULL;
	DISKFIXINFO stFixInfo;


	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/shutdown", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	fp = fopen(fileNameBuf,"r+b");
	if( fp == NULL )
	{
		fp = fopen(fileNameBuf,"w+b");
		if( fp == NULL )
		{
			printf(" create %s error!\n",fileNameBuf);
			return FILEERROR;
		}

		stFixInfo.iCreatePartiton = 0;
		stFixInfo.iRecPartiton 	= 0;
		stFixInfo.iCreateCDPartition = iPartitionId;

		printf("0 stFixInfo.iCreateCDPartition = %d\n",iPartitionId);;

		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
		fclose(fp);
		return ALLRIGHT;
	}

	rel = fread(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
	if( rel != sizeof(DISKFIXINFO) )
	{
		printf(" 2read stfixinfo error! rel = %d\n",rel);
		//fclose(fp);
		//return FILEERROR;
		stFixInfo.iCreateCDPartition = 0;
		printf(" read stfixinfo error! self set value 0!\n");
	}

	printf("1 stFixInfo.iCreateCDPartition = %d,recPartion =%d, iPartitionId = %d\n",stFixInfo.iCreateCDPartition,stFixInfo.iRecPartiton,iPartitionId);;

	if( stFixInfo.iCreateCDPartition != iPartitionId )
	{
		rewind(fp);

		printf("2 stFixInfo.iCreateCDPartition = %d\n",iPartitionId);;

		stFixInfo.iCreateCDPartition = iPartitionId;
		rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" write stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fflush(fp);
	}
	fclose(fp);
	return ALLRIGHT;	

}


int FS_MachineRightShutdown(int iDiskId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	FILE * fp = NULL;
	DISKFIXINFO stFixInfo;


	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/shutdown", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	fp = fopen(fileNameBuf,"r+b");
	if( fp == NULL )
	{
		return ALLRIGHT;
	}

	stFixInfo.iRecPartiton = 0;
	stFixInfo.iCreatePartiton = 0;
	stFixInfo.iCreateCDPartition = 0;
	
	rel = fwrite(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
	if( rel != sizeof(DISKFIXINFO) )
	{
		printf(" 3write stfixinfo error! rel = %d\n",rel);
		fclose(fp);
		return FILEERROR;
	}
	fflush(fp);
	fclose(fp);	
	
	return ALLRIGHT;
}

int FS_RecFileChangePartiton()
{	
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;

	pFileInfoPoint = FS_GetPosFileInfo(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId , g_stRecFilePos.iCurFilePos ,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
	{		 
		return ERROR;
	}

	if( g_stRecFilePos.iCurFilePos ==  pFileInfoPoint->pUseinfo->iTotalFileNumber - 1)
		return YES;
	else
		return NO;
	
}

int FS_OpenAudioFile()
{
	int rel;

	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;

	GST_DISKINFO stDiskInfo;

	char filename[50];	

	if( fpAudioPlay== NULL )
	{
		pFileInfoPoint = FS_GetPosFileInfo(g_stPlayFilePos.iDiskId,g_stPlayFilePos.iPartitionId,g_stPlayFilePos.iCurFilePos,&fileInfoPoint);

		if( pFileInfoPoint == NULL )
			return ERROR;	

		stDiskInfo = DISK_GetDiskInfo(pFileInfoPoint->iDiskId);

		memset(filename,0,50);
				
		sprintf(filename,"/tddvr/%s%d/%s",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);
	
		fpAudioPlay = fopen( filename, "r" );
		if ( !fpAudioPlay )
		{
			printf(" open %s error!!!\n", filename);
			return FILEERROR;						
		}

		g_ulAudioPlayFileOffset = ulPlayFrameHeaderOffset;
	
	}

	return ALLRIGHT;
}

int FS_CloseAudioFile()
{
	if( fpAudioPlay )
	{
		fclose(fpAudioPlay);
		fpAudioPlay = NULL ;
	}

	return ALLRIGHT;
}

void FS_SetAudioPos()
{	
	g_ulAudioPlayFileOffset = ulPlayFrameHeaderOffset;
}

int FS_GetPlayFilePos()
{
	return g_stPlayFilePos.iCurFilePos;
}

int FS_GetAudioFrame(char *audioBuf,int * iLength,int iChannel)
{
	int rel = 0;

	FRAMEHEADERINFO stFrameHeaderInfo;

	rel = fseek(fpAudioPlay,g_ulAudioPlayFileOffset,SEEK_SET);
	if( rel == -1 )
	{
		FS_CloseAudioFile();
		return FILEERROR;
	}

	while(1)
	{
		if(  g_ulAudioPlayFileOffset >= ulPlayFrameHeaderEndOffset )
		{
			return PLAYOVER;
		}
	
		rel = fread(&stFrameHeaderInfo,1, sizeof( FRAMEHEADERINFO ) ,fpAudioPlay );

		if( rel != sizeof( FRAMEHEADERINFO ) )
			return FILEERROR;

		g_ulAudioPlayFileOffset +=  sizeof( FRAMEHEADERINFO );

		if( CheckFrameHeader(&stFrameHeaderInfo) == ERROR )
		{
		//要跳过这一帧		
			printf(" CheckFrameHeader BAD\n");
			continue;
		}

		if( stFrameHeaderInfo.type == 'A' && stFrameHeaderInfo.iChannel == iChannel)
		{
			rel = fseek(fpAudioPlay,  stFrameHeaderInfo.ulFrameDataOffset, SEEK_SET);

			if( rel < 0 )
				return FILEERROR;

			rel = fread(audioBuf , 1, stFrameHeaderInfo.ulDataLen , fpAudioPlay );
		
			if( rel != stFrameHeaderInfo.ulDataLen  )
				return FILEERROR;	

		//	printf(" type=%c iChannel = %d sec=%ld usec=%ld\n",stFrameHeaderInfo.type ,
		//		stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec);

			*iLength = stFrameHeaderInfo.ulDataLen;

			return ALLRIGHT;
		}
		
	}
}


int FS_WriteDataTo_CDROM()
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	GST_DISKINFO stUsbInfo;
	GST_DISKINFO stTempDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int iInfoNumber = 0;
	int rel,ret;
	char fileName[50];
	 struct timeval nowTime;	 
	 long UsbSize = 0;
	 GST_TIME_TM mytime;
	 char timebuf[50];
	 FILEINFO tmp_fileinfo;

	// cdrom备份时不分通道
	 int iCam = 0x0f;

/*	rel =  FS_CreateCDPartitionWriteToShutDownLog(0,5);
	if( rel < 0 )
		return rel;
*/
	 g_UsbAvailableSize = 550 * 1024* 1024;


	  switch(cdrom_mode){
	 	case 0: g_UsbAvailableSize = DVD_SIZE; break;
		case 1: g_UsbAvailableSize = CD_SIZE; break;
		case 2: g_UsbAvailableSize = MD_SIZE; break;
		default: g_UsbAvailableSize = MD_SIZE; break;
	 	
	 }

	 DataToUsbFileOffset = 0;
	 HeaderToUsbOffset = 0;

	 g_usb_write_date_size = 0;

			
	iDiskNum = DISK_GetDiskNum();
	stTempDiskInfo = DISK_GetDiskInfo(0);

//	sprintf(fileName,"mkdir /tddvr/%s5/AnaVideo",&stTempDiskInfo.devname[5]); 		

//	command_ftc(fileName);    // mkdir /tddvr/hda5

	sprintf(fileName,"/tddvr/%s5/AnaVideo",&stTempDiskInfo.devname[5]); 		

	mkdir(fileName,0777);    // mkdir /tddvr/hda5

//	printf("command_ftc: %s\n",fileName);	
	
	sprintf(fileName,"rm -rf /tddvr/%s5/AnaVideo/*",&stTempDiskInfo.devname[5]); 		

	command_ftc(fileName);    // mkdir /tddvr/hda5	

	if( rel < 0 )
	{
		DPRINTK("rm %s error\n",fileName);
	}else
		DPRINTK("rm %s ok\n",fileName);

	//sprintf(fileName,"/tddvr/%s5/AnaVideo/playback",&stTempDiskInfo.devname[5]); 		

	//mkdir(fileName,0777);    // mkdir /tddvr/hda5/AnaVideo/playback
	
	sprintf(fileName,"cp -rf /mnt/mtd/playback.zip /tddvr/%s5/AnaVideo/playback",&stTempDiskInfo.devname[5]); 		

	command_ftc(fileName);    // mkdir /tddvr/hda5	

	if( rel < 0 )
	{
		DPRINTK("%s error\n",fileName);
	}else
		DPRINTK("%s ok\n",fileName);	


	printf("command_ftc: %s\n",fileName);

	memset(fileName,0x00,50);
	

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = 0; i < iDiskNum; i++ )
	{
		if( g_iDiskIsRightFormat[i] == 0 )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
					return ERROR;
			
			
			for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;
				
				if( FTC_CompareTime(cdStartTime[0],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(cdEndTime[0],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,cdStartTime[0],cdEndTime[0] )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,cdStartTime[0],cdEndTime[0] )||	
				      
				      FTC_CompareTime(cdStartTime[1],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(cdEndTime[1],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,cdStartTime[1],cdEndTime[1] )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,cdStartTime[1],cdEndTime[1] )||
				      
				      FTC_CompareTime(cdStartTime[2],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(cdEndTime[2],pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,cdStartTime[2],cdEndTime[2] )
				      || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,cdStartTime[2],cdEndTime[2] ))
				{
					if( pFileInfoPoint->pFilePos->states != WRITEOVER )
						continue;

				  	printf("k=%d starttime=%ld endtime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime);

						if( fpDataToUsb == NULL && fpHeaderToUsb == NULL)
						{						

							g_UsbAvailableSize -= (DataToUsbFileOffset + HeaderToUsbOffset);						
						
							g_pstCommonParam->GST_DRA_get_sys_time( &nowTime, NULL );

							// 做usb 备份时，将生成两个文件，一个是帧头存储文件( .ANH )
							// 另一个的视频数据文件( .ANV )

							memset(timebuf,0,50);

							FTC_CD_ConvertTime_to_date(nowTime.tv_sec,timebuf);							

							sprintf(fileName,"/tddvr/%s5/AnaVideo/%s.avd",&stTempDiskInfo.devname[5],timebuf);
														
							fpDataToUsb = fopen(fileName,"w+b");
							if( fpDataToUsb == NULL )
							{
								printf("open %s error!\n",fileName);
								return FILEERROR;
							}

							printf("create %s ok!\n",fileName);

							memset(fileName,0x00,50);
							sprintf(fileName,"/tddvr/%s5/AnaVideo/%s.avh",&stTempDiskInfo.devname[5],	timebuf);

							fpHeaderToUsb  = fopen(fileName,"w+b");
							if( fpHeaderToUsb == NULL )
							{
								printf("open %s error!\n",fileName);

								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}

							printf("create %s ok!\n",fileName);

							tmp_fileinfo.ulVideoSize = 
								g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video();;

							rel = fwrite(&tmp_fileinfo,1,sizeof(FILEINFO),fpHeaderToUsb);
							if( rel != sizeof(FILEINFO) )
							{
								fclose(fpHeaderToUsb);
								fpHeaderToUsb = NULL;
								fclose(fpDataToUsb);
								fpDataToUsb = NULL;
								return FILEERROR;
							}

							HeaderToUsbOffset = sizeof(FILEINFO);													
							DataToUsbFileOffset = 0;

						}

						if(fpDataToUsb)
						{							
						
							rel = FS_GetDataFromFile_CDROM(pFileInfoPoint->iDiskId,pFileInfoPoint->iPatitionId,
								pFileInfoPoint->iCurFilePos,iCam);

							if( rel < 0 )
							{
								if( fpDataToUsb )
								{
									fclose(fpDataToUsb);
									fpDataToUsb = NULL;										
								}

								if( fpHeaderToUsb )
								{
									fclose(fpHeaderToUsb);
									fpHeaderToUsb = NULL;										
								}
							
								if( rel == FILEFULL)
								{								
								}
								else		
								{									
									return rel;
								}
							}
							else
							{
								if( rel == USBBACKUPSTOP )
									goto BACKUPEND;
							}
						}
						
				}		
				
				
			}
		}
	}

BACKUPEND:
	if( fpDataToUsb )
	{
		fclose(fpDataToUsb);
		fpDataToUsb = NULL;			
	}

	if( fpHeaderToUsb )
	{
		fclose(fpHeaderToUsb);
		fpHeaderToUsb = NULL;										
	}

	return ALLRIGHT;
}

int FS_GetDataFromFile_CDROM(int iDiskId, int iPartitionId,int iFilePos,int iCam)
{
	FILE * fp = NULL;
	FILE * fp_data = NULL;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT  fileInfoPoint;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	 struct timeval nowTime;	
	 unsigned long ulFrameOffset = 0;
	 unsigned long ulEndOffset = 0;
	 FRAMEHEADERINFO  stFrameHeaderInfo;
	 char * databuf = NULL;
	 int iFirst = 0;
	  DISK_IO_FILE fileheadst;
	 DISK_IO_FILE filedatast;
	 char dev[50];	

	 if( g_UsbBuf == NULL )
	 {
	 	return NOENOUGHMEMORY;
	 }

	databuf= g_UsbBuf;	

	 stDiskInfo = DISK_GetDiskInfo(iDiskId);

	FS_LogMutexLock();

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId,iPartitionId , iFilePos ,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
	{		 
			return ERROR;
	}

	sprintf(fileName,"/tddvr/%s%d/%s.avh",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);

	ulEndOffset = pFileInfoPoint->pFilePos->ulOffSet;

	printf("open %s file\n",fileName);

	FS_LogMutexUnLock();

	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avh",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );	
	if( file_io_open(dev,fileName,"r",&fileheadst,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto file_op_error;
	}	

	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avd",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );	
	if( file_io_open(dev,fileName,"r",&filedatast,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		goto file_op_error;
	}

	ulFrameOffset = sizeof( FILEINFO );


	while(1)
	{		
		if( file_io_lseek(&fileheadst,  ulFrameOffset , SEEK_SET )== (off_t)-1 )
		{
			DPRINTK("seek error \n");
			rel = FILEERROR;
			goto file_op_error;
		}
		
		rel = file_io_read(&fileheadst,&stFrameHeaderInfo,sizeof(FRAMEHEADERINFO));
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			DPRINTK(" fread error\n");
			rel = FILESEARCHERROR;
			goto file_op_error;
		}	

		
		
		ulFrameOffset += sizeof(FRAMEHEADERINFO);
		if( ulFrameOffset >= ulEndOffset )
		{			 
			file_io_close(&fileheadst);
			file_io_close(&filedatast);

			if( DataToUsbFileOffset > 128 * 1024 * 1024 )
				return FILEFULL;
			
			return ALLRIGHT;
		}

		if( g_writeCdFlag != 1)
		{			 
			file_io_close(&fileheadst);
			file_io_close(&filedatast);
			printf("stop cdrom write\n");
			return USBBACKUPSTOP;
		}
				
		
		if( (FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[0],cdEndTime[0] ) == 0)
			&&  (FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[1],cdEndTime[1] ) == 0)
			&&  (FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[2],cdEndTime[2] ) == 0))
				continue;

		iCam = 0;

		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[0],cdEndTime[0] ) == 1 )
		{
			iCam = iCam |cd_cam[0];
		}

		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[1],cdEndTime[1] ) == 1 )
		{
			iCam = iCam |cd_cam[1];
		}

		if( FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,cdStartTime[2],cdEndTime[2] ) == 1 )
		{
			iCam = iCam |cd_cam[2];
		}

		// 确保第一帧为关键帧
		if( iFirst == 0 && stFrameHeaderInfo.ulFrameType == 1 && stFrameHeaderInfo.type == 'V'
				&&  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) )
		{
			iFirst = 1;
		}

		if(  FTC_GetBit(iCam,stFrameHeaderInfo.iChannel) && iFirst == 1)
		{
		
			//printf(" time %ld type=%c ichannel=%d ulFrametype=%d  bit=%ld\n",stFrameHeaderInfo.ulTimeSec,
			//stFrameHeaderInfo.type,stFrameHeaderInfo.iChannel,stFrameHeaderInfo.ulFrameType ,stFrameHeaderInfo.ulDataLen);
			//FTC_CompareTime(stFrameHeaderInfo.ulTimeSec ,startTime,endTime ),FTC_GetBit(iCam,stFrameHeaderInfo.iChannel));

			if( file_io_lseek(&filedatast,  stFrameHeaderInfo.ulFrameDataOffset , SEEK_SET )== (off_t)-1 )
			{
				DPRINTK("seek error \n");
				rel = FILEERROR;
				goto file_op_error;
			}
			
			rel = file_io_read(&filedatast,databuf,stFrameHeaderInfo.ulDataLen);
			if( rel != stFrameHeaderInfo.ulDataLen )
			{
				DPRINTK(" fread error\n");
				rel = FILESEARCHERROR;
				goto file_op_error;
			}		

	
			stFrameHeaderInfo.ulFrameDataOffset = DataToUsbFileOffset ;		
				
			rel = fwrite(&stFrameHeaderInfo,1,sizeof(FRAMEHEADERINFO),fpHeaderToUsb);
			if( rel != sizeof(FRAMEHEADERINFO) )
			{
				 DPRINTK("write erro!\n");
				goto file_op_error;
			}

			fflush(fpHeaderToUsb);

			HeaderToUsbOffset += sizeof(FRAMEHEADERINFO);

			rel = fwrite(databuf,1,stFrameHeaderInfo.ulDataLen,fpDataToUsb);
			if( rel != stFrameHeaderInfo.ulDataLen )
			{				 
				DPRINTK("write erro!\n");
				goto file_op_error;
			}

			fflush(fpDataToUsb);

			DataToUsbFileOffset += stFrameHeaderInfo.ulDataLen;			

			if( g_UsbAvailableSize < DataToUsbFileOffset + HeaderToUsbOffset  )
			{
				printf("cdrom no space!\n");				
				file_io_close(&fileheadst);
				file_io_close(&filedatast);
				return USBNOENOUGHSPACE;
			}

			g_usb_write_date_size = g_usb_write_date_size 
				+ sizeof(FRAMEHEADERINFO) + stFrameHeaderInfo.ulDataLen;
		}
		
	}	
	
	return ALLRIGHT;
	
file_op_error:
	file_io_close(&fileheadst);
	file_io_close(&filedatast);
	return FILEERROR;	
	
}

int get_cdrom_backup_perecnt()
{
	float a,b,c;

	a = (float)g_usb_write_date_size;
	b = (float)DataToUsbFileOffsetSave;	

	c = a*100 / b;

	if( FS_WriteCdStatus() == 1 )
	{
		if( c > 90 )
			c = 90;
	}

	return (int)c;
}



