#include "saveplayctrl.h"
#include "FTC_common.h"
#include <sched.h>
#include "devfile.h"

int g_EnableFileSave;
int g_EnableVideoPlay;
int g_EnableAudioPlay;

int gRecstart;
int gPlaystart;

int g_RecStatusCurrent = 0;
int g_PlayStatusCurrent = 0;

int g_CheckFileSystem = 0;
int g_hour_clear_time = 0;
int g_hour_between_time = 0;


int g_PlaybackImageSize = TD_DRV_VIDEO_SIZE_CIF;

//BUF_LIST_PARAM * g_pstRecFile;
//BUF_LIST_PARAM * g_pstBufListParam;
GST_PLAYBACK_VIDEO_PARAM  g_pstPlaybackModeParam;
GST_PLAYBACK_VIDEO_PARAM g_pstRecModeParam;

GST_DEV_FILE_PARAM * g_pstCommonParam;

GST_FILESHOWINFO g_playFileInfo;

int ftc_get_rec_cur_status()
{
//	printf(" gRecstart =%d g_RecCam=%d\n",gRecstart,g_RecCam);

	if( gRecstart == 1 )
		return g_RecCam;
	else
		return 0;	
}

int ftc_is_disk_record()
{
//	printf(" gRecstart =%d g_RecCam=%d\n",gRecstart,g_RecCam);

	if( gRecstart == 1 )
		return 1;
	else
		return 0;	
}


int ftc_get_play_mode(int * image, int * standard)
{
	*image = g_pstPlaybackModeParam.iPbImageSize;
	*standard = g_pstPlaybackModeParam.iPbVideoStandard;
	printf("play image_size=%d standard=%d\n",*image,*standard);
	return 1; 
}


int FTC_Machine_status_write()
{
	FILE * fp = NULL;
	int rel = 0;		
	int number = 0;
	static int ifirst = 0;

	if( ifirst > 120 )
		return ALLRIGHT;

	if( ifirst == 0 )
	{
		fp = fopen("/cjy/MachineRunStatus","w+b");
		
		if( !fp )
		{
			printf("create file error!\n");
			return ERROR;
		} 

		fclose(fp);

		ifirst = 1;
	}

	ifirst++;

	fp = fopen("/cjy/MachineRunStatus","r+b");

	if( !fp )
	{
		printf("no file error!\n");
		return ERROR;
	} 

	number = 1;
	
	rel = fwrite(&number,1,sizeof(int),fp );
	if( rel != sizeof(int) )
	{
		printf("write number error!\n");
	}

	fflush(fp); 
 
	fclose(fp);
	fp = NULL;
	
	return ALLRIGHT;
}

void FTC_SetIntervalCleanTime(int clean_time,int Interval_time)
{
	g_hour_clear_time = clean_time;
	g_hour_between_time = Interval_time;
	printf("set clean time %d %d\n",clean_time,Interval_time);
}


// 此线程是用来检测由于硬盘操作失误导致机子异常
//的情况
// 1. 由于更新失败导致机子无法起动。
// 2.录相时由于硬盘异常导致录相线程阻塞。
// 3.回放线程阻塞
// 4. 硬盘出错，所有对硬盘的操作都失误。

//解决方法
// 1. 在系统/cjy 目录下建 一个systemwork 的文件，每过5 秒钟
// 向里面写一个1. 在主程序外面还有一个监视程序也将
// 读取这个文件，并判断是否是1 ，并向文件写入0，由此
// 判断主进程是否正常运行
// 2-4 做标志位，查询标志位即可，对各进程都要处理

#define MAX_CHECK_DISK_TIME (7200)

void thread_for_check_filesystem_work(void)
{
	SET_PTHREAD_NAME(NULL);
	int rel = 0;
	int iRecCount = 0;
	int iPlayCount = 0;
	int iCreateFileCount = 0;
	int hour_clear_count = 0;
	FILEINFOPOS stRecPos;
	struct timeval cur_time;
	//检查sd卡，防止在程序运行期间sd卡复位，导致不在进行录像的问题。
	int disk_check_time = MAX_CHECK_DISK_TIME-5;  //5秒查一次又没有硬盘。
	int have_disk = 0;
	int already_check_time_num =0;

	printf(" thread_for_check_filesystem_work pid = %d\n",getpid());

	g_CheckFileSystem = 1;
	
	while(1)
	{
		sleep(1);

		if( g_CheckFileSystem == -1 )
		{
			printf("thread_for_check_filesystem_work out!\n");
			return;
		}

		//录相线程检测
		if(gRecstart == 1 )
		{
			if(g_RecStatusCurrent == 1 )
			{
				iRecCount = 0;
				g_RecStatusCurrent = 0;
			}
			else
			{
				iRecCount++;
				DPRINTK("iRecCount=%d\n",iRecCount);
				if( iRecCount >= 60 )
				{
					// 录相线程阻塞,发信息给系统
					printf(" rec thread block!\n");

					rel = FS_GetRecCurrentPoint(&stRecPos);
					if( rel < 0 )
					{
						printf("FS_GetRecCurrentPoint error!\n");						
					}
					FS_SetErrorMessage(FILEERROR, stRecPos.iDiskId);
					iRecCount=0;
				}
			}
		}else
		{
			iRecCount = 0;
			g_RecStatusCurrent = 0;
		}

		//回放线程检测
		if(gPlaystart == 1 )
		{
			if(g_PlayStatusCurrent == 1 )
			{
				iPlayCount = 0;
				g_PlayStatusCurrent = 0;
			}
			else
			{
				iPlayCount++;
				if( iPlayCount >= 20 )
				{
					// 回放线程阻塞,发信息给系统
					printf(" play thread block!\n");
					//FS_SetErrorMessage(FILEERROR, g_playFileInfo.iDiskId);
					iPlayCount=0;
				}
			}
		}else
		{
			iPlayCount = 0;
			g_PlayStatusCurrent = 0;
		}

		//文件创建线程检测
	/*	if(FS_CreateFileStatus() ==  0 )
		{
			if(FS_GetCreateFileFlag() == 1 )
			{
				iCreateFileCount = 0;
				FS_SetCreateFileFlag(0);
			}
			else
			{
				iCreateFileCount++;
				if( iCreateFileCount >= 10 )
				{
					//文件创建线程阻塞,发信息给系统
					printf(" create thread  thread block!\n");
					FS_SetErrorMessage(FILEERROR, g_createFileDiskNum);
					iCreateFileCount=0;
				}
			}
		}else
		{
			iCreateFileCount = 0;
			FS_SetCreateFileFlag(0);
		}
		*/

	/*	if( FTC_Machine_status_write() != ALLRIGHT )
		{
			printf(" FTC_Machine_status_write  thread block!\n");		
		}
	*/

		if( g_hour_clear_time != 0 && g_hour_clear_time > 0 && g_hour_between_time != 0)
		{
			hour_clear_count++;

			if( hour_clear_count > g_hour_clear_time && FTC_Get_Play_Status() ==0 )
			{

				g_pstCommonParam->GST_DRA_get_sys_time( &cur_time, NULL );
				
				rel =FS_CoverFileClearLog(0,cur_time.tv_sec - g_hour_between_time, -1);
				if( rel < 0 )
				{
					printf("run FS_CoverFileClearLog error!\n");				
				}else
				{
					printf("hour clear ok!%d %d\n",g_hour_clear_time,g_hour_between_time);
				}
				
				hour_clear_count = 0;
			}
		}else
		{
			hour_clear_count = 0;
		}		

		{
			disk_check_time++;

			//由于DISK_GetAllDiskInfo()存在内存泄露的问题。所以检查时间
			//不能太频繁
			if( disk_check_time > MAX_CHECK_DISK_TIME )
			{
				int iDiskNum = 0;
				
				disk_check_time = 0;

				already_check_time_num++;

				DPRINTK("disk is ok check %d  %d\n",already_check_time_num,MAX_CHECK_DISK_TIME);

				DISK_GetAllDiskInfo();     // get all disk information.
				iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

				if( iDiskNum > 0 )
				{
					if( have_disk == 0 )	
					{
						have_disk = 1;	
						DPRINTK("check disk start  set have_disk = 1\n");
					}
				}else
				{
					if( have_disk == 1 )
					{
						DPRINTK("check disk num = %d buf flag have_disk = 1,so disk lost!! need reboot\n");
						FS_SetErrorMessage(FILEERROR, 1);						
					}
				}
			}
		}
	
	}
}

int FTC_StopCheckFileSystem()
{
	g_CheckFileSystem = -1;
	return 1;
}

void FTC_TF_TEST_FUNC()
{
	int disk_num = 0;
	int i;
	int ret;

	DPRINTK("in\n");
	
	DISK_GetAllDiskInfo();
	
	
	disk_num = DISK_GetDiskNum();   // get the number of disk whick PC have.

	for( i = 0; i < disk_num; i++)
	{
		 FS_IO_FILE_CHECK(i);		
	}

	DPRINTK("out\n");

}

int FTC_GetFunctionAddr(GST_DEV_FILE_PARAM * gst)
{
	int i;


	DISK_GetAllDiskInfo();

	i = DISK_GetDiskNum();

	printf("11=========== disk num = %d\n",i);

	FTC_TF_TEST_FUNC();

	i = DISK_GetDiskNum();

	printf("22=========== disk num = %d\n",i);

	gst->GST_DISK_GetAllDiskInfo = DISK_GetAllDiskInfo;
	gst->GST_DISK_GetDiskNum    = DISK_GetDiskNum;
	
	gst->GST_FS_BuildFileSystem    = FS_BuildFileSystem;
	gst->GST_FS_CheckNewDisk     = FS_CheckNewDisk;
	gst->GST_FS_PartitionDisk        = FS_PartitionDisk;
	gst->GST_FS_GetDiskInfoPoint  = DISK_GetDiskInfoPoint;
	gst->GST_FS_WriteInfoToAlarmLog = FS_WriteInfoToAlarmLog;
	gst->GST_FS_WriteDataToUsb        = FS_WriteDataToUsb;
	gst->GST_FS_MountAllPatition	= FS_MountAllPatition;
	gst->GST_FS_UMountAllPatition  	= FS_UMountAllPatition;
	gst->GST_FS_PhysicsFixAllDisk      =  FS_PhysicsFixAllDisk;
	gst->GST_FS_EnableDiskReCover   = FS_EnableDiskReCover;
	gst->GST_FS_ClearDiskLogMessage = FS_ClearDiskLogMessage;
	gst->GST_FS_StartCreateFile	      = FS_StartCreateFile;
	gst->GST_FS_StopCreateFile		= FS_StopCreateFile;
	gst->GST_FS_WriteUsb				= FS_WriteUsb;
	gst->GST_FS_UsbWriteStatus		= FS_UsbWriteStatus;
	gst->GST_FS_get_cover_rate		= FTC_get_cover_rate;
	gst->GST_FS_get_disk_remain_rate   = FTC_get_disk_remain_rate;
	gst->GST_FS_GetBadBlockSize		= FTC_GetBadBlockSize;
	gst->GST_FS_GetErrorMessage		= FS_GetErrorMessage;
	gst->GST_FS_ListWriteToUsb       = FS_ListWriteToUsb;
	gst->GST_FS_GetDataSize   	 = FS_GetDataSize;
	gst->GST_FS_WriteDataToCdrom = FS_WriteDataToCdrom;
	gst->GST_FS_WriteCdStatus	= FS_WriteCdStatus;
	gst->GST_FS_scan_cdrom		= FS_scan_cdrom;
	gst->GST_FS_WriteInfoToEventLog = FS_WriteInfoToEventLog;
	gst->GST_FS_FormatUsb = FS_FormatUsb;
	gst->GST_FS_CheckUsb = FS_CheckUsb;
	
	gst->GST_FTC_CustomTimeTotime_t = FTC_CustomTimeTotime_t;
	gst->GST_FTC_GetRecordlist	= FTC_ON_GetRecordlist;
	gst->GST_FTC_GetWeekDay	= FTC_GetWeekDay;
	gst->GST_FTC_SetPlayFileInfo	= FTC_SetPlayFileInfo;
	gst->GST_FTC_time_tToCustomTime = FTC_time_tToCustomTime;
	gst->GST_FTC_StopPlay		= FTC_StopPlay;
	gst->GST_FTC_StartRec		= FTC_StartRec;
	gst->GST_FTC_StopRec		= FTC_StopRec;
	gst->GST_FTC_GetAlarmEventlist = FTC_ON_GetAlarmEventlist;
	gst->GST_FTC_SetTimeToPlay    = FTC_SetTimeToPlay_lock;
	gst->GST_FTC_PrintMemoryInfo  = FTC_PrintMemoryInfo;
	gst->GST_FTC_AudioRecCam      = FTC_AudioRecCam;
	gst->GST_FTC_AudioPlayCam	 = FTC_AudioPlayCam;	
	gst->GST_FTC_PreviewRecordStop = FTC_PreviewRecordStop;
	gst->GST_FTC_set_count_create_keyframe = FTC_set_count_create_keyframe;
	gst->GST_FTC_CheckPreviewStatus = FTC_CheckPreviewStatus;
	gst->GST_FTC_PreviewRecordStart = FTC_PreviewRecordStart;
	gst->GST_FTC_PlayBack		     = FTC_PlayBack;
	gst->GST_FTC_PlayFast		    = FTC_PlayFast;
	gst->GST_FTC_PlayPause		    = FTC_PlayPause;
	gst->GST_FTC_GetRecMode           = FTC_GetRecMode;
	gst->GST_FTC_CurrentPlayMode    =  FTC_CurrentPlayMode;
	gst->GST_FTC_PlaySingleFrame     = FTC_PlaySingleFrame;
	gst->GST_FTC_PlayUsbWrite          = FTC_PlayUsbWrite;
	gst->GST_FTC_PlayChangFile         = FTC_PlayChangFile;
	gst->GST_FTC_ChangeRecordToPlay = FTC_ChangeRecordToPlay;
	gst->GST_FTC_PlayUsbWriteStatus    = FTC_PlayUsbWriteStatus;
	gst->GST_FTC_CurrentDiskWriteAndReadStatus = FTC_CurrentDiskWriteAndReadStatus;
	gst->GST_FTC_PlayNearRecFile		= FTC_PlayNearRecFile;
	gst->GST_FTC_GetPlayCurrentStauts = FTC_GetPlayCurrentStauts;
	gst->GST_FTC_GetCurrentPlayInfo = FTC_GetCurrentPlayInfo;
	gst->GST_FTC_AlarmlistPlay	   = FTC_AlarmlistPlay;
	gst->GST_FTC_ReclistPlay             = FTC_ReclistPlay;
	gst->GST_FTC_SetRecDuringTime  = FTC_SetRecDuringTime;
	gst->GST_FTC_GetEventlist		    = FTC_GetEventlist;
	gst->GST_FTC_get_play_status 	   = FTC_Get_Play_Status;
	gst->GST_FTC_get_play_time	   = FTC_get_play_time;
	gst->GST_FTC_get_disk_rec_start_end_time = FTC_get_disk_rec_start_end_time_lock;
	gst->GST_FTC_get_cur_play_cam  = FTC_get_cur_play_cam;
	gst->GST_FTC_StopSavePlay         = FTC_StopSavePlay;
	gst->GST_FTC_get_rec_cur_status = ftc_get_rec_cur_status;
	//gst->GST_FTC_preview_buf_write = preview_buf_write;

	g_pstCommonParam = gst;	


	{
		int disk_num = 0;

		 DISK_GetAllDiskInfo();		
		
		disk_num = DISK_GetDiskNum();   // get the number of disk whick PC have.

		for( i = 0; i < disk_num; i++)
		{
			FS_UMountAllPatition(i);			 
		}
	}

	return ALLRIGHT;
}


int  FTC_InitSavePlayControl(GST_DEV_FILE_PARAM * gst)
{

	int ret;
	pthread_t idusbfile;

	ret = FS_InitFileInfoLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecordLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecAddLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecTimeStickLogBuf();
	if( ret < 0 )
		return ERROR;	

	ret = FS_InitAlarmLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitEventLogBuf();
	if( ret < 0 )
		return ERROR;
	
	FS_InitControlLogMutex();

	#ifdef USE_WRITE_BUF_SYSTEM
	
		ret = FS_write_buf_sys_init();
		if( ret < 0 )
		{
			DPRINTK("FS_write_buf_sys_init error!\n");
			exit(0);
			return;
		}
	
	#endif


	#ifdef USE_READ_BUF_SYSTEM
		
		ret = FS_read_buf_sys_init();
		if( ret < 0 )
		{
			DPRINTK("FS_read_buf_sys_init error!\n");
			exit(0);
			return;
		}

	#endif



	//FS_AllDiskEnableDMA(DISEABLE);

	
	g_pstCommonParam = gst;
	

	//g_pstRecFile = &gstSaveToFile;
	//g_pstBufListParam = &gstBufListParam;
	
	g_pstPlaybackModeParam.iPbImageSize = TD_DRV_VIDEO_SIZE_D1;
	g_pstPlaybackModeParam.iPbVideoStandard =TD_DRV_VIDEO_STARDARD_NTSC;

	g_pstRecModeParam.iPbImageSize =TD_DRV_VIDEO_SIZE_D1;
	g_pstRecModeParam.iPbVideoStandard = TD_DRV_VIDEO_STARDARD_NTSC;

	g_EnableFileSave = 1;
 	g_EnableVideoPlay = 1;
	g_EnableAudioPlay = g_EnableVideoPlay;

	gRecstart = 0;
	gPlaystart = 0;	

	g_stLogPos.iCurFilePos = 0;
	g_stLogPos.iDiskId = 0;
	g_stLogPos.iPartitionId = 5;

	printf( "thread_for_check_filesystem_work  thread \n");

	ret = pthread_create(&idusbfile,NULL,(void*)thread_for_check_filesystem_work,NULL);
	if ( 0 != ret )
	{
		printf( "create thread_for_create_cdrom_file  thread error\n");
		return ERROR;
	}

	return ALLRIGHT;
	
}

int FTC_CreateAllThread()
{
	pthread_t idrecfile;
	pthread_t idvideoplayback;
	pthread_t idaudioplayback;
	pthread_t idcreatefile;
	pthread_t idpreview;
	pthread_t idusbfile;
	pthread_t iddisksleep;
	int ret;

	printf(" idrecfile pthread_create\n");

	ret =  pthread_create(&idrecfile, NULL,(void *) thread_for_rec_file,NULL);
	if ( 0 != ret )
	{
		printf( "create thread_for_rec_file thread error\n");
		return ERROR;
	}	
	

	printf(" idvideoplayback pthread_create\n");

	ret =  pthread_create(&idvideoplayback, NULL,(void *) thread_for_play_file,NULL);
	if ( 0 != ret )
	{
		printf( "create main control thread error\n");
		return ERROR;
	}	

	
	printf(" idaudioplayback pthread_create\n");

	ret =  pthread_create(&idaudioplayback, NULL,(void *) thread_for_audio_playback,NULL);
	if ( 0 != ret )
	{
		printf( "create main control thread error\n");
		return ERROR;
	}	

printf( "thread_for_preview_record thread \n");

#ifdef USE_PREVIEW_ENC_THREAD_WRITE

#else
	ret = pthread_create(&idpreview,NULL,(void*)thread_for_preview_record,NULL);
	if ( 0 != ret )
	{
		printf( "create thread_for_preview_record thread error\n");
		return ERROR;
	}
#endif

	printf( "thread_for_create_usb_file  thread \n");

	ret = pthread_create(&idusbfile,NULL,(void*)thread_for_create_usb_file,NULL);
	if ( 0 != ret )
	{
		printf( "create thread_for_create_usb_file  thread error\n");
		return ERROR;
	}

	printf( "thread_for_create_cdrom_file  thread \n");

	ret = pthread_create(&idusbfile,NULL,(void*)thread_for_create_cdrom_file,NULL);
	if ( 0 != ret )
	{
		printf( "create thread_for_create_cdrom_file  thread error\n");
		return ERROR;
	}
	
	printf("  pthread_create all OK 1\n");


	return ALLRIGHT;
		
}

int FTC_StopSavePlay()
{
	int iDiskNum = 0;	
	int i = 0;
	//GST_DISKINFO stDiskInfo;	

//printf("FTC_StopSavePlay 22---11\n");
//	return ALLRIGHT;

	
//	FTC_StopCheckFileSystem();
//	FS_ReleaseUsbThread();
//	FS_OutCreateFileThread();
//	FS_StopCdromThread();

//	gRecstart = -1;
	printf("gRecstart 22---11\n");

	sleep( 2);

//	gPlaystart = -1;	
//	printf("gPlaystart ---22\n");

//	return ALLRIGHT;

	printf(" umount disk \n");	

	iDiskNum = DISK_GetDiskNum();

	printf(" iDiskNum = %d disk \n",iDiskNum);

	for(i = 0; i < iDiskNum; i++ )
	{		
		FS_UMountAllPatition(i);	
	}	
	printf(" umount disk success\n");

	for(i = 0; i < iDiskNum; i++ )
	{			
	//	stDiskInfo = DISK_GetDiskInfo(i);

	//	DISK_DiskSleep(stDiskInfo.devname);
	}	

	printf(" 1\n");
//	FS_ReleaseAlarmLogBuf();
	printf(" 2\n");
//	FS_ReleaseRecordLogBuf();
	printf(" 3\n");
//	FS_ReleaseFileInfoLogBuf();
	printf(" 4\n");
//	FS_ReleaseEventLogBuf();

//	FS_ReleaseRecTimeStickLogBuf();
	printf(" 5\n");
	ReleasePreviewViewBuf();
	printf(" 6\n");

	#ifdef USE_WRITE_BUF_SYSTEM
		FS_write_buf_sys_destroy();	
	#endif

	
	#ifdef USE_READ_BUF_SYSTEM
		FS_read_buf_sys_destroy();	
	#endif

	printf(" FTC_StopSavePlay stop OK!\n");


	return ALLRIGHT;
}


int FTC_FixAllDisk()
{
	int iDiskNum  = 0;
	GST_DISKINFO stDiskInfo;		
	int i,j;
	int ret;
	
	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.
	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}


	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  0; i < iDiskNum ; i++  )
	{	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for( j = 1 ; j < stDiskInfo.partitionNum; j++ )
		{			

			// 复修文件系统
			ret = FS_FixFileInfo(i,stDiskInfo.partitionID[j]);
			if( ret < 0 )
			{
				printf("FS_FixFileInfo error! disk=%d p=%d\n",i,stDiskInfo.partitionID[j]);
				
			}
			else
				printf("FS_FixFileInfo success! disk=%d p=%d\n",i,stDiskInfo.partitionID[j]);
		}
	}

	return ALLRIGHT;
}


int div_safe(float num1,float num2)
{
	if( num2 == 0 )
	{
		DPRINTK("num2=%d\n",num2);
		return -1;
	}

	return (int)(num1/num2);
}




