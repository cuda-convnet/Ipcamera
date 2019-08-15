#include "saveplayctrl.h"
#include "FTC_common.h"
#include "devfile.h"


#define DF_PLAYING 1
#define DF_PLAYSTOP 0

char * audioBuf[2] = {NULL, NULL};
int 	   iHaveAudioBuf = 0;
int      g_AudioChannel = -1;
time_t g_TimePlayPoint = 0;
time_t g_TimePlayStartTime = 0;
time_t g_TimePlayEndTime = 0;
int      g_AudioLength = 0;
int 	  g_PlayCam = 0;
int     g_PlayStatus = PLAYNORMAL;
int     g_SearchLastSingleFrameChangeFile = 0;
int    g_HandStopPlay = 0;

int iMustOpenFileFlag = 0;
int iHD1OpenFileFlag[4] = {0,0,0,0};
int iOpenFileFlag = 0;

time_t PlayUsbWriteTime = 0;
time_t play_file_time = 0;

int iAudioNowPlay = 0;

struct timeval pcTime,oldPcTime;
struct timeval PlayTime,oldPlaytime,lastPlayTime;

char * audioPlayBuf[5] = {NULL,NULL,NULL,NULL,NULL};


int gUsbFlag = 0;
long UsbWriteDateLength = 0;
long UsbWriteHeaderLength = 0;
FILE *  pUsbFile = NULL;
FILE *  pUsbHeaderFile = NULL;
int UsbAvailableSize = 0;  // 单位是K

int play_start_stop_status = 0;
int net_file_send_flag = 0;
int normal_play_fast_search = 0;

void set_normal_play_fast_search(int flag)
{	
	normal_play_fast_search = flag;
	printf("set normal_play_fast_search = %d\n",normal_play_fast_search);
}

int get_normal_play_fast_search()
{
	return normal_play_fast_search;
}

int net_file_send(int flag)
{
	if( flag == 1 )
		net_file_send_flag = 1;
	else
		net_file_send_flag = 0;

	DPRINTK("net_file_send_flag = %d\n",net_file_send_flag);
	
	return 1;
}


int FTC_Get_Play_Status()
{
	return play_start_stop_status;
}

int OpenUsbFile()
{
	int rel;
	struct timeval nowTime;
	GST_DISKINFO stUsbInfo;
	char fileName[50];
	char timebuf[50];
	//GST_TIME_TM mytime;
	FILEINFO file_tmp;
	int ret;

	if( pUsbFile )
		return ERROR;

	if( gPlaystart != 1)
		return ERROR;

	UsbAvailableSize = 0;
	UsbWriteDateLength = 0;
	UsbWriteHeaderLength = 0;
	
	DISK_GetAllDiskInfo();
	stUsbInfo = DISK_GetUsbInfo();
	rel = FS_MountUsb();
	if( rel < 0 )
	{		
		printf("FS_MountUsb error! error = %d\n",rel);
		return rel;
	}

	UsbAvailableSize = FS_GetDiskUseSize(2);

	if( UsbAvailableSize <  0 )
	{	
		printf(" FS_GetDiskUseSize error!\n");
		return UsbAvailableSize;
	}

	//sprintf(fileName,"mkdir /tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
	//								stUsbInfo.partitionID[0]); 		

	//command_ftc(fileName);    // mkdir /tddvr/hda5

	sprintf(fileName,"/tddvr/%s%d/AnaVideo",&stUsbInfo.devname[5],
									stUsbInfo.partitionID[0]); 	

	mkdir(fileName,0777);

	printf("command_ftc: %s\n",fileName);

	g_pstCommonParam->GST_DRA_get_sys_time( &nowTime, NULL );

	//g_pstCommonParam->GST_DRA_convert_time_to_date(PlayUsbWriteTime,&mytime);
	
	memset(timebuf,0,50);

	FTC_ConvertTime_to_date(PlayUsbWriteTime,timebuf);

	//sprintf(timebuf,"%04d%02d%02d_%02d%02d%02d",mytime.tm_year,mytime.tm_mon+1,mytime.tm_mday,
	//	mytime.tm_hour,mytime.tm_min,mytime.tm_sec);

	sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avd",&stUsbInfo.devname[5],
				stUsbInfo.partitionID[0],timebuf);

	printf("open use file %s\n",fileName);

	pUsbFile = fopen(fileName,"w+b");
	if( pUsbFile == NULL )
	{
		printf("open %s error!\n",fileName);
		goto USEFILEERROR;
	}

	sprintf(fileName,"/tddvr/%s%d/AnaVideo/%s.avh",&stUsbInfo.devname[5],
				stUsbInfo.partitionID[0],timebuf);

	printf("open use file %s\n",fileName);

	pUsbHeaderFile = fopen(fileName,"w+b");
	if( pUsbHeaderFile == NULL )
	{
		printf("open %s error!\n",fileName);
		goto USEFILEERROR;
	}

	ret = fwrite(&file_tmp,1,sizeof(FILEINFO),pUsbHeaderFile);

	if( ret != sizeof(FILEINFO) )
	{
		goto USEFILEERROR;
	}

	UsbWriteHeaderLength = sizeof(FILEINFO);
							
	return ALLRIGHT;

USEFILEERROR:

	if( pUsbFile )
	{
		fclose(pUsbFile);
		pUsbFile = NULL;
	}

	if( pUsbHeaderFile )
	{
		fclose(pUsbHeaderFile);
		pUsbHeaderFile = NULL;
	}

	return FILEERROR;
}

int CloseUsbFile()
{
	int rel;

	if( pUsbFile )
	{
		fclose(pUsbFile);
		pUsbFile = NULL;	
	}
	
	if( pUsbHeaderFile )
	{
		fclose(pUsbHeaderFile);
		pUsbHeaderFile = NULL;	
	}

	rel  = FS_UMountUsb();
	if( rel  < 0 )
	{
		printf("FS_UMountUsb error!\n");
		return rel;
	}
	
	return ALLRIGHT;
}

int WriteToUsb(FRAMEHEADERINFO * stFrameHeaderInfo, char * FrameData)
{
	int rel;

	if( pUsbFile == NULL || pUsbHeaderFile == NULL)
	{
		DPRINTK("pUsbFile=%x  pUsbHeaderFile=%x\n",pUsbFile,pUsbHeaderFile);
		return ERROR;
	}

	if( (UsbWriteDateLength + UsbWriteHeaderLength ) / 1024 > UsbAvailableSize - 4096 )
	{
		printf(" usb no enough capacity!\n");
		return DISKNOENOUGHCAPACITY;
	}

	stFrameHeaderInfo->ulFrameDataOffset = UsbWriteDateLength;

	rel = fwrite(stFrameHeaderInfo,1,sizeof( FRAMEHEADERINFO ),pUsbHeaderFile);
	if( rel != sizeof(FRAMEHEADERINFO) )
	{
		printf(" write usb error!\n");
		return FILEERROR;
	}

	fflush(pUsbHeaderFile);

	UsbWriteHeaderLength += sizeof( FRAMEHEADERINFO );

	rel = fwrite(FrameData,1,stFrameHeaderInfo->ulDataLen,pUsbFile);
	if( rel != stFrameHeaderInfo->ulDataLen )
	{
		printf(" write usb error!\n");
		return FILEERROR;
	}

	fflush(pUsbFile);

	UsbWriteDateLength += stFrameHeaderInfo->ulDataLen;

	if( UsbWriteDateLength > 134217728 )  //134217728 = 1024*1024*128
	{
		rel = UsbChangeFile();
		if( rel < 0 )
		{
			printf(" UsbChangeFile error = %d\n",rel);
			return rel;
		}
	}

	return ALLRIGHT;
}

int UsbChangeFile()
{
	int rel;
	
	if(pUsbFile == NULL )
		return ERROR;

	rel = FTC_PlayUsbWrite(0);
	if( rel < 0 )
	{
		printf("UsbWrite rel = %d\n",rel);
		return ERROR;
	}

	rel = FTC_PlayUsbWrite(1);
	if( rel < 0 )
	{
		printf("UsbWrite rel = %d\n",rel);
		return ERROR;
	}
	
}

int FTC_get_play_time(time_t * play_time)
{
	*play_time = play_file_time;
	return ALLRIGHT;
}

int FTC_PlayUsbWriteStatus()
{
	printf("FTC_PlayUsbWriteStatus gUsbFlag=%d\n",gUsbFlag);
	return gUsbFlag;
}

int FTC_PlayUsbWrite(int iFlag)
{
	int rel;

	printf(" usbWrtine iFlag = %d\n",iFlag);
	if( iFlag == 1)
	{
		gUsbFlag = USBBACKUPING;
	//	printf("1 gUsbFlag=%d\n",gUsbFlag);
	}

	switch(iFlag)
	{
		case 0:				
			rel = CloseUsbFile();
			if( rel < 0 )
			{
				gUsbFlag = USBWRITEERROR;
				return rel;	
			}	
			gUsbFlag = USBBACKUPSTOP;
			break;
		case 1:			
			rel = OpenUsbFile();
			//	printf("2 gUsbFlag=%d\n",gUsbFlag);
			if( rel < 0 )			
			{
				if(rel == NOUSBDEVICE )
					gUsbFlag = NOUSBDEV;			
				else
					gUsbFlag = USBWRITEERROR;
				return rel;	
			}	
			printf("3 gUsbFlag=%d\n",gUsbFlag);
			gUsbFlag = USBBACKUPING;		
			break;
		default:
			break;
	}
	return ALLRIGHT;
}

void  PlayFast()
{
	int rel;
	int rel_status;
	static int check_count = 0;
	
	rel = FS_SearchNextIFrame(g_playFileInfo.iCam);
	if( rel < 0 )
	{
		rel_status = rel;
		rel = ClosePlayFile();
		if( rel == ALLRIGHT)
		{
			printf("PlayBackOver()\n");			
			if( rel_status == -101 )
			{
				printf("frame mode error\n");
				// 停止回放
				iOpenFileFlag = 0;	
				gPlaystart = -2;	

				check_count = 0;
			}else
			{
				//寻找下一个回放录相					
				
				rel = FTC_ToNextFile();
				if( rel < 0 )
				{													
					iOpenFileFlag = 0;	
					gPlaystart = -2;	

					check_count = 0;
				}		

				check_count++;

				if( check_count > 50 )
				{
					iOpenFileFlag = 0;	
					gPlaystart = -2;	

					check_count = 0;

					DPRINTK("change file > 50,not find video data\n");
				}
			}
		}
		else
		{
			printf("ClosePlayFile error\n");
			// 停止回放
			iOpenFileFlag = 0;	
			gPlaystart = -2;	

			check_count = 0;
		}
		iMustOpenFileFlag = 0;		
	}else
	{
		check_count = 0;
	}
}

void PlayBackFast()
{
	int rel;
	int rel_status;

	static int check_count = 0;
	
	rel = FS_SearchLastIFrame(g_playFileInfo.iCam);
	if( rel < 0 )
	{
		rel_status = rel;
		rel = ClosePlayFile();
		if( rel == ALLRIGHT)
		{
			printf("PlayBackOver()\n");

			if( rel_status == -101 )
			{
				printf("frame mode error\n");
				// 停止回放
				iOpenFileFlag = 0;	
				gPlaystart = -2;	
				check_count = 0;
			}else
			{
				//寻找下一个回放录相									
				rel = FTC_ToLastFile();
				if( rel < 0 )
				{													
					iOpenFileFlag = 0;	
					gPlaystart = -2;	
					check_count = 0;
				}	
				g_SearchLastSingleFrameChangeFile = 1;

				check_count++;

				if( check_count > 50 )
				{
					iOpenFileFlag = 0;	
					gPlaystart = -2;
					check_count = 0;

					DPRINTK("change file > 50,not find video data\n");
				}
			}
		}
		else
		{
			printf("ClosePlayFile error\n");
			// 停止回放
			iOpenFileFlag = 0;	
			gPlaystart = -2;	
			check_count = 0;
		}
		
		iMustOpenFileFlag = 0;		
	}else
	{
		check_count = 0;
	}
}

int  ChangeToNextFile()
{
	int rel;
	
	rel = ClosePlayFile();
	if( rel == ALLRIGHT)
	{
		printf("PlayBackOver()\n");
		//寻找下一个回放录相									
		rel = FTC_ToNextFile();
		if( rel < 0 )
		{													
			iOpenFileFlag = 0;	
			gPlaystart = -2;	
		}									
	}
	else
	{
		printf("ClosePlayFile error\n");
		// 停止回放
		iOpenFileFlag = 0;	
		gPlaystart = -2;	
	}
	iMustOpenFileFlag = 0;	

	return ALLRIGHT;	
}

int ChangeToLastFile()
{
	int rel;
	rel = ClosePlayFile();
	if( rel == ALLRIGHT)
	{
		printf("PlayBackOver()\n");
		//寻找下一个回放录相									
		rel = FTC_ToLastFile();
		if( rel < 0 )
		{													
			iOpenFileFlag = 0;	
			gPlaystart = -2;	
		}									
	}
	else
	{
		printf("ClosePlayFile error\n");
		// 停止回放
		iOpenFileFlag = 0;	
		gPlaystart = -2;	
	}
	iMustOpenFileFlag = 0;	
	
	return ALLRIGHT;	
}

int ChangePlayStatus(int iStatus)
{

	//printf(" iStatus = %d\n",iStatus);
	switch(iStatus)
	{
		case PLAYNORMAL:
			break;					
		case PLAYFASTLOWSPEED:
		//	PlayFast();
		//	usleep(350000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED:
			PlayFast();		
			usleep(120000);
			ResetShowTime();
			break;							
		case PLAYBACKFASTLOWSPEED:
			PlayBackFast();			
			usleep(120000);
			ResetShowTime();
			break;
		case PLAYBACKFASTHIGHSPEED:
			PlayBackFast();				
			usleep(80000);
			ResetShowTime();
			break;
		case PLAYPAUSE:
				usleep(100000);
				return PLAYPAUSE;
			break;
		case PLAYLATERFRAME:
			break;
		case PLAYLASTFRAME:
			PlayBackFast();
			break;
		case PLAYNEXTFILE:
			ChangeToNextFile();
			break;
		case PLAYLASTFILE:
			ChangeToLastFile();
			break;			
		case PLAYFASTSEARCH:
			PlayFast();		
			usleep(125000);
			ResetShowTime();
			break;
		case PLAYFASTLOW2SPEED:
			PlayFast();		
			usleep(150000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED_2:
			PlayFast();			
			usleep(80000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED_4:
			PlayFast();			
			usleep(40000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED_8:			
			PlayFast();	
			usleep(20000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED_16:			
			PlayFast();	
			usleep(10000);
			ResetShowTime();
			break;
		case PLAYFASTHIGHSPEED_32:			
			PlayFast();
			usleep(5000);
			ResetShowTime();
			break;		
		case PLAYBACKFASTHIGHSPEED_2:					
			PlayBackFast();	
			usleep(80000);
			ResetShowTime();
			break;
		case PLAYBACKFASTHIGHSPEED_4:			
			PlayBackFast();	
			usleep(40000);
			ResetShowTime();
			break;
		case PLAYBACKFASTHIGHSPEED_8:			
			PlayBackFast();	
			usleep(20000);
			ResetShowTime();
			break;
		case PLAYBACKFASTHIGHSPEED_16:			
			PlayBackFast();	
			usleep(10000);
			ResetShowTime();
			break;

		case PLAYBACKFASTHIGHSPEED_32:			
			PlayBackFast();	
			usleep(1000);
			ResetShowTime();
			break;
			
		default:
			break;
	}

	return ALLRIGHT;
}

int ResetShowTime()
{
	pcTime.tv_sec = 0;
	pcTime.tv_usec = 0;
	PlayTime.tv_sec = 0;
	PlayTime.tv_usec = 0;
	oldPcTime.tv_sec = 0;
	oldPcTime.tv_usec = 0;
	oldPlaytime.tv_sec = 0;
	oldPlaytime.tv_usec = 0;
	lastPlayTime.tv_sec = 0;
	lastPlayTime.tv_usec = 0;
	return 1;
}

int AdjustShowTime(time_t Second,time_t uSecond)
{
	long ulPcDelayTime = 0;
	long ulPlayDelayTime = 0;
	long DelayTime = 0;
	
	
	g_pstCommonParam->GST_DRA_get_sys_time( &pcTime, NULL );
	PlayTime.tv_sec = Second;
	PlayTime.tv_usec = uSecond;


//	printf("Second = %ld uSecond=%ld\n",Second,uSecond);
//	printf("%ld,%ld, %ld,%ld\n",oldPcTime.tv_sec,oldPcTime.tv_usec ,
//		oldPlaytime.tv_sec,oldPlaytime.tv_usec);
	if( oldPcTime.tv_sec == 0 && oldPcTime.tv_usec == 0 &&
		oldPlaytime.tv_sec == 0 && oldPlaytime.tv_usec == 0 )
	{
		oldPcTime.tv_sec = pcTime.tv_sec;
		oldPcTime.tv_usec = pcTime.tv_usec;
		oldPlaytime.tv_sec = Second;
		oldPlaytime.tv_usec = uSecond;
		lastPlayTime.tv_sec = Second;
		lastPlayTime.tv_usec = uSecond;
		
	}
	else
	{
		ulPcDelayTime = (pcTime.tv_sec - oldPcTime.tv_sec) * 1000000 
					+ (pcTime.tv_usec -  oldPcTime.tv_usec);
		ulPlayDelayTime = (PlayTime.tv_sec - oldPlaytime.tv_sec) * 1000000 
					+ (PlayTime.tv_usec -  oldPlaytime.tv_usec);

		if( ulPcDelayTime - ulPlayDelayTime > 2000000 )
		{
			ResetShowTime();
			return ALLRIGHT;
		}

		//防止长时间的阻塞
		if( PlayTime.tv_sec - lastPlayTime.tv_sec > 2 && lastPlayTime.tv_sec != 0)
		{
			ResetShowTime();
			return ALLRIGHT;
		}

		//防止长时间的阻塞
		if( lastPlayTime.tv_sec - PlayTime.tv_sec > 2 && lastPlayTime.tv_sec != 0)
		{
			ResetShowTime();
			return ALLRIGHT;
		}

		if( ( pcTime.tv_sec - oldPcTime.tv_sec > 32)  && (PlayTime.tv_sec - oldPlaytime.tv_sec > 32 ))
		{
		//	printf("44 pcTime.tv_sec=%ld oldPcTime.tv_sec=%ld delaytime=%ld PlayTime.tv_sec=%ld oldPlaytime.tv_sec=%ld dl=%ld\n",pcTime.tv_sec ,oldPcTime.tv_sec ,
		//		pcTime.tv_sec - oldPcTime.tv_sec,PlayTime.tv_sec,oldPlaytime.tv_sec,PlayTime.tv_sec - oldPlaytime.tv_sec);
			oldPcTime.tv_sec += 10;
			oldPlaytime.tv_sec += 10;
		}

//printf("===ulPcDelayTime = %ld ulPlayDelayTime=%ld\n",ulPcDelayTime,ulPlayDelayTime);
		DelayTime = ulPlayDelayTime - ulPcDelayTime;		

		if( DelayTime > 0  )
		{
		//	printf("DelayTime = %ld\n",DelayTime);
			usleep(DelayTime);				
		}
	
		lastPlayTime.tv_sec = PlayTime.tv_sec;
		lastPlayTime.tv_usec = PlayTime.tv_usec;

	}
	return ALLRIGHT;
}

int FTC_ClearAllChannel(int ImageSize,int Standard)
{
	GST_DRV_BUF_INFO stBufInfo;
	int ret = 0;
	int iCount = 0;
	int iGetCount = 0;	

	while(1)
	{
		stBufInfo.iBufDataType = TD_DRV_DATA_TYPE_PLAYBACK_VIDEO;
		ret =g_pstCommonParam->GST_DRA_get_buffer_of_playback( &stBufInfo );
		if ( 0 == ret )
		{
			stBufInfo.iChannelMask = 1;			
			stBufInfo.iBufLen[0] = 512;
			stBufInfo.iCurFrameIsOutput = 4;
			stBufInfo.iImageStandard = Standard;
			stBufInfo.iImageSize = ImageSize;

			while(1)
			{
				ret = g_pstCommonParam->GST_DRA_send_buffer_of_playback(&stBufInfo);
				if ( 0 == ret )
				{	
					g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stBufInfo);
					return ALLRIGHT;
				}
				else
				{				
					iCount++;					
					if( iCount > 200)
					{
						printf("clear channel  decode  out error!\n");
						g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stBufInfo);		
						return ERROR;
					}

					usleep(5000);
				}
			}
				
		}
		else
		{
			iGetCount++;
			if( iGetCount > 200)
			{
				printf(" get play buff error!\n");
				return ERROR;
			}
			usleep(5000);
		}
	}
}

int OpenAudio()
{
	iHaveAudioBuf = 1;
	iAudioNowPlay = 1;

	return ALLRIGHT;
}

int CloseAudio()
{
	iAudioNowPlay = 0;
	sleep(1);
	
	return ALLRIGHT;
}

int get_play_start_flag()
{
	return g_TimePlayPoint;
}

void thread_for_play_file(void)
{
	SET_PTHREAD_NAME(NULL);
	GST_DRV_BUF_INFO stBufInfo;
	int ret = 0;
	//FILE *f[4] = {NULL, NULL, NULL, NULL};
	//char filen[4][60];
	int indexcountt = 0;
	//unsigned long ulLen;
	//FILE_HEADER_INFO stFileHeaderInfo;
	//int iFileIndex[4] = {0,0,0,0};//{15,15,15,15};//
	//int iReadSize = 0;
	//int iDelFileCount = 0;
	//int iDelFileIndex = 0;
	int idx = 0;
	//int iSwapIndexVal = 0;
	int iShowTotalExecCount = 0;
	//char* pDataTmpBuf = NULL;
        struct timeval tvOld;
        struct timeval tvNew;       
        int iAvidFrameDataFlag = 0;
       // int iPrintWillVideoFileOpenFlag = 0;
	FRAMEHEADERINFO stFrameHeaderInfo;
	int iFirstPlay = 0;
	int rel;
	int iSkip = 0;
	int iDecodeCount = 0;
	int iContinueRead = 0;
	int audioChannel = -1;
	//int audioNumber = 0;
	static int skim = 0;   // useing for distinguish D1 and quad

	unsigned long ulSecond = 0;
	unsigned long ulUSecond = 0;

	//int iDiskId,iPartitionId,iFilePos;
	//int iPlayBufId = 0;

	char * pFramePlayBuf = NULL;
	int read_count  = 0;

	DPRINTK(" pid = %d\n",getpid());

	pFramePlayBuf = (char *) malloc(512*1024);
	if( !pFramePlayBuf )
	{
		printf("malloc pFramePlayBuf error!\n");
		return;
	}

	audioBuf[0] = (char *)malloc(AUDIOBUF);
	if( !audioBuf[0] )
	{
		printf("audio[0] malloc error!\n");
		return ;
	}

	audioBuf[1] = (char *)malloc(AUDIOBUF);
	if( !audioBuf[1] )
	{
		printf("audio[1] malloc error!\n");
		return ;
	}
                          
	while( g_EnableVideoPlay )
	{
		
		
		while(gPlaystart==0) 
		{			
			//iFirstPlay = 0;
			usleep( 10000 );
		}
		
		// -1 表示退出线程，-2表示停止回放
		if( gPlaystart == -1 || gPlaystart ==  -2)
		{
			iHaveAudioBuf =0;

			printf("end file 1\n");			

			if( iOpenFileFlag > 0 )
			{
				printf("end file 2\n");
				FS_LogMutexLock();						
				rel = ClosePlayFile();
				FS_LogMutexUnLock();								
				if( rel == ALLRIGHT)
					printf("play file close suceess!\n");				
				else				
					printf("play file close faild\n");			

				iMustOpenFileFlag = 0;
			}

			if( gPlaystart == -1 )
			{
				if( audioBuf[0] )
					free(audioBuf[0]);
	
				if( audioBuf[1] )
					free(audioBuf[1]);	
				printf(" play thread  go out!\n");
				return;
			}	

			if( gPlaystart == -2 )
			{
				printf("end file 3\n");				

			//	if( g_HandStopPlay == 0 )
				play_start_stop_status = DF_PLAYSTOP;
					//FS_SetErrorMessage(PLAYFILEOVER, 0);

				iMustOpenFileFlag = 0;

				iOpenFileFlag = 0;

				g_pstCommonParam->GST_DRV_CTRL_StartStopPlayBack(0);

				set_normal_play_fast_search(0);

				gPlaystart = 0;				
				DPRINTK("play stop!!!!!!!!!\n");
				continue;
			}
						
		
		}


		stBufInfo.iBufDataType = TD_DRV_DATA_TYPE_PLAYBACK_VIDEO;
	//	ret =g_pstCommonParam->GST_DRA_get_buffer_of_playback( &stBufInfo );

		ret =g_pstCommonParam->GST_DRV_CTRL_EnableInsertPlayBuf();
		if ( 0 < ret )
		{
			g_PlayStatusCurrent = 1;
		
			iAvidFrameDataFlag = 0;	
			
			//printf(" iMustOpenFileFlag = %d gPlaystart=%d\n",iMustOpenFileFlag,gPlaystart);
			if ( 0 == iMustOpenFileFlag && gPlaystart == 1)
			{
					audioChannel = -1;

					
			
					FS_LogMutexLock();
					
					iOpenFileFlag = OpenPlayFile(g_playFileInfo.iDiskId,
						g_playFileInfo.iPartitionId, g_playFileInfo.iFilePos);					
					
					FS_LogMutexUnLock();
					
					if ( iOpenFileFlag  < 0  ) 
					{	
						printf(" OpenPlayFile error!\n");							
						gPlaystart = -2;
						continue;
					}
					else
					{
						iMustOpenFileFlag = 1;
					}
					
					printf(" Play back start! %d,%d,%d \n",g_playFileInfo.iDiskId,
						g_playFileInfo.iPartitionId, g_playFileInfo.iFilePos);

					// 让文件跑到时间点
					if( g_TimePlayPoint != 0)
					{
						rel = FS_PlayFileGoToTimePoint(g_TimePlayPoint,g_playFileInfo.iCam);
						if(rel < 0 )
						{
							printf(" file read error! or No %ld time point, rel = %d\n",g_TimePlayPoint,rel);
							gPlaystart = -2;
							continue;
						}	
						else
							printf(" FS_PlayFileGoToTimePoint OK!\n");
						
						g_TimePlayPoint = 0;

					}

					printf("2 g_PlayStatus = %d,g_TimePlayStartTime=%ld,g_TimePlayEndTime=%ld\n",
						g_PlayStatus,g_TimePlayStartTime,g_TimePlayEndTime);

					// 倒退播放换文件时，把播放位置指向文件结尾处
					if( g_PlayStatus == PLAYBACKFASTLOWSPEED ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED_2 ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED_4 ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED_8 ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED_16 ||
						g_PlayStatus == PLAYBACKFASTHIGHSPEED_32 ||
						g_SearchLastSingleFrameChangeFile == 1)
					{
						if( g_SearchLastSingleFrameChangeFile == 1)
							g_SearchLastSingleFrameChangeFile = 0;
						printf(" g_PlayStatus = %d\n",g_PlayStatus);
						rel = FS_PlayMoveToEnd();
						if( rel < 0 )
						{
							printf(" FS_PlayMoveToEnd error \n");								
							gPlaystart = -2;
							continue;
						}
					}

					ResetShowTime();

					iFirstPlay = 0;

					play_start_stop_status = DF_PLAYING;
					
				}
					

				// 在这里做快进后退暂停处理
					if ( 1 == iMustOpenFileFlag )
					{
						rel = ChangePlayStatus(g_PlayStatus);	
						if( rel == PLAYPAUSE )
						{							
							ResetShowTime();
							audioChannel = -1;
							continue;
						}

					/*	// 单帧播放
						if( g_PlayStatus == PLAYLATERFRAME || g_PlayStatus == PLAYLASTFRAME)
						{
							g_PlayStatus = PLAYPAUSE;						
						}
						*/

						if( g_PlayStatus != PLAYNORMAL )
						{
							ResetShowTime();
							audioChannel = -1;						
						}
					}
					

					if ( 1 == iMustOpenFileFlag )
					{		
						stBufInfo.iChannelMask = 0;		
						iContinueRead = 1;
						
						for ( read_count=0; read_count<2; read_count++ )											
						{
							if ( iOpenFileFlag > 0 && iContinueRead == 1)
							{							
								while(1)
								{	
								    //判断文件是否读完
									if( iMustOpenFileFlag == 1 )
									{
										rel =  CheckPlayFile();	
										if( rel < 0 )
										{
											rel = ClosePlayFile();
											if( rel == ALLRIGHT)
											{
												printf("PlayBackOver()\n");
												//寻找下一个回放录相									
												rel = FTC_ToNextFile();
												if( rel < 0 )
												{													
													iOpenFileFlag = 0;	
													gPlaystart = -2;														
												}									
											}
											else
											{
												printf("ClosePlayFile error\n");
												// 停止回放
												iOpenFileFlag = 0;	
												gPlaystart = -2;												
											}
											iMustOpenFileFlag = 0;		
										 }
									}
									

									if(iMustOpenFileFlag == 0 )									
											break;									

									
								//	printf(" g_pstPlaybackModeParam.iPbImageSize = %d\n",g_pstPlaybackModeParam.iPbImageSize);

									//printf(" iPlayBufId = %d g_playFileInfo.iCam=%d\n",iPlayBufId,idx);							
									rel = ReadOneFrame(&stFrameHeaderInfo, pFramePlayBuf, audioBuf[0],idx, g_playFileInfo.iCam);
									
									if( rel < 0 )
									{		
											if( rel == READBADFRAME )
											{
												// 读到了一个坏帧
												// 将此帧改为不播放的音频帧
												//这样将自动放弃这一帧
											//	printf("read a bad frame!\n");
												stFrameHeaderInfo.type = 'A';
												stFrameHeaderInfo.iChannel = -10;
												continue;
												
											}
											else
											{
												printf(" ReadOneFrame error!\n");
												//FS_SetErrorMessage(rel, stFrameHeaderInfo.ulTimeSec);									
												iMustOpenFileFlag = 0;										
												gPlaystart = -2;												
												break;
											}
									}									

									//printf("size %d %d    length: %d   type=%c frametype=%d framenum=%d\n",  g_pstPlaybackModeParam.iPbImageSize,idx, stFrameHeaderInfo.ulDataLen,
									//				stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,stFrameHeaderInfo.ulFrameNumber);
									
									
									if( g_TimePlayEndTime != 0 &&  stFrameHeaderInfo.type == 'V')
									{
									//	printf(" g_TimePlayStartTime=%ld,g_TimePlayEndTime=%ld,stFrameHeaderInfo.ulTimeSec=%ld\n",
									//		g_TimePlayStartTime,g_TimePlayEndTime,stFrameHeaderInfo.ulTimeSec);
																	
										if( (stFrameHeaderInfo.ulTimeSec >= g_TimePlayEndTime) || 
											 (stFrameHeaderInfo.ulTimeSec < ( g_TimePlayStartTime-20) ) )
										{
											printf("ulTimeSec %ld  larger than g_TimePlayEndTime %ld\n",stFrameHeaderInfo.ulTimeSec ,g_TimePlayEndTime);
											//gPlaystart = -2;
											//iMustOpenFileFlag = 0;	
											//FS_GoBackOneFrame();
											//FS_GoBackOneFrame();
											//FS_GoBackOneFrame();
											//FS_GoBackOneFrame();
											//g_PlayStatus = PLAYPAUSE;
											//iContinueRead = 0;
											//FS_SetErrorMessage(PLAYFILEOVER, 0);
											
											gPlaystart = -2;	
											iMustOpenFileFlag = 0;	
											play_start_stop_status = DF_PLAYSTOP;
											break;
										}								
										
									}

									//printf(" imagesize=%d playsize=%d\n",stFrameHeaderInfo.imageSize ,g_pstPlaybackModeParam.iPbImageSize);

															
										

									iSkip = 0;		

									
									if( stFrameHeaderInfo.type == 'V' )
											break;									
								

									// 文件一定是以视频帧结束。
									if( stFrameHeaderInfo.type == 'A' )
									{			
										if( g_PlayStatus == PLAYNORMAL )
										{

										//printf(" iChannel = %d type=%c sec=%ld  usec=%ld g_AudioChannel=%d  %x%x%x%x\n",stFrameHeaderInfo.iChannel ,stFrameHeaderInfo.type,
										//	stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec,g_AudioChannel,audioBuf[0][5],audioBuf[0][6],audioBuf[0][7],audioBuf[0][8]);

										
										if( stFrameHeaderInfo.iChannel == g_AudioChannel )
										{
											if( g_pstPlaybackModeParam.iPbImageSize == TD_DRV_VIDEO_SIZE_D1 )
											{
												if( (iFirstPlay == 1) && (net_file_send_flag == 0))
													AdjustShowTime(stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec);
											}
											{
												int send_count = 0;
												int send_ret = 0;

												while(1)
												{
													if( g_pstCommonParam->GST_DRV_CTRL_EnableInsertPlayBuf() < 0 )
													{
														usleep(10);	

														send_count++;
														if( send_count > 100)
															break;
													}else
														break;												
												}

												 g_pstCommonParam->GST_DRV_CTRL_InsertPlayBuf((char*)&stFrameHeaderInfo,audioBuf[0]); 
											}
										}

										     if( gUsbFlag == 1 && stFrameHeaderInfo.iChannel == g_AudioChannel)
										    {
										    	//	printf("sec=%ld usec=%ld type=%c channel=%d\n",stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec,
											//			stFrameHeaderInfo.type,stFrameHeaderInfo.iChannel);
												rel = WriteToUsb(&stFrameHeaderInfo,audioBuf[0]);
												if( rel < 0 )
												{
													printf(" WriteToUsb error = %d\n",rel);												
													FTC_PlayUsbWrite(0);
													gUsbFlag = 2;
												}
										    }	
										}
									}
								}

								// 文件读完或出错时跳出
								if( iMustOpenFileFlag == 0 )
										break;

								// 确保回放的第一帧为关键帧							
								iFirstPlay = 1;
								

								//区分QUAD 和D1

							//	printf(" skim =%d idx=%d g_playFileInfo.iCam=%d  type=%c size =%d\n",skim,stFrameHeaderInfo.iChannel,g_playFileInfo.iCam,
							//	stFrameHeaderInfo.type,stFrameHeaderInfo.imageSize);

								
								if(FTC_GetBit( g_playFileInfo.iCam,stFrameHeaderInfo.iChannel) != 1 && (stFrameHeaderInfo.type != 'A'))
								{
									skim++;

									if( skim > 2000000 )
									{
										gPlaystart = -2;
										iMustOpenFileFlag = 0;	
										//FS_SetErrorMessage(PLAYFILEOVER, 0);
										play_start_stop_status = DF_PLAYSTOP;
										printf(" more than 2000000 pic is not show!\n");
										skim = 0;
											break;
									}

									if( skim > 10000 )
									{
										if( g_PlayStatus == PLAYNORMAL )
										{
											set_normal_play_fast_search(1);
											FTC_play_fast_search(1);
										}

										if(g_PlayStatus == PLAYFASTLOWSPEED)
										{
											set_normal_play_fast_search(2);
											FTC_play_fast_search(1);
										}
										
									}
								}else if( stFrameHeaderInfo.type == 'V')
								{
									skim = 0;
									if( get_normal_play_fast_search() == 1 )
									{
										FTC_PlayFast(0);
										set_normal_play_fast_search(0);
									}

									if( get_normal_play_fast_search() == 2 )
									{
										FTC_PlayFast(1);
										set_normal_play_fast_search(0);
									}
								}

								idx = stFrameHeaderInfo.iChannel;

								
								if( FTC_GetBit( g_playFileInfo.iCam,idx) == 1 && (iSkip == 0) && (iFirstPlay == 1))
								{
									stBufInfo.iChannelMask |= (1<<idx);
									stBufInfo.iFrameType[idx] = stFrameHeaderInfo.ulFrameType;
									stBufInfo.iBufLen[idx] = stFrameHeaderInfo.ulDataLen;
									stBufInfo.iFrameCountNumber[idx] = stFrameHeaderInfo.ulFrameNumber;
									stBufInfo.iCurFrameIsOutput = 1;

																											
									iAvidFrameDataFlag = 1;	

									/*if( idx == 0 )
									printf("1size= %d %d    length: %d   type=%c frametype=%d  framenum=%d  sec=%ld usec=%ld\n",  g_pstPlaybackModeParam.iPbImageSize,idx, stFrameHeaderInfo.ulDataLen,
													stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,stFrameHeaderInfo.ulFrameNumber,
													stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec);

									printf("%x %x %x %x\n",pFramePlayBuf[5],pFramePlayBuf[6],pFramePlayBuf[7],pFramePlayBuf[8]);
									*/				
									ulSecond  = stFrameHeaderInfo.ulTimeSec;
									ulUSecond = stFrameHeaderInfo.ulTimeUsec;

									PlayUsbWriteTime = stFrameHeaderInfo.ulTimeSec;

									//回放时备份
									if( (gUsbFlag == 1) && (pUsbFile != NULL) && (pUsbHeaderFile!=NULL) )
									{
										rel = WriteToUsb(&stFrameHeaderInfo,pFramePlayBuf);
										if( rel < 0 )
										{
											printf(" WriteToUsb error = %d\n",rel);
										//	FS_SetErrorMessage(USBWRITEERROR, stFrameHeaderInfo.ulTimeSec);
											FTC_PlayUsbWrite(0);

											if(rel == DISKNOENOUGHCAPACITY)
												gUsbFlag = USBSPACENOTENOUGH;										
											else
												gUsbFlag = USBWRITEERROR;
										}
									}			

									// 单帧播放
									if( g_PlayStatus == PLAYLATERFRAME || g_PlayStatus == PLAYLASTFRAME)
									{
										g_PlayStatus = PLAYPAUSE;						
									}									 
								
									break;
								
								
								}
								
								
								}
															
							}

						if( (iFirstPlay == 1) && (net_file_send_flag == 0))
								AdjustShowTime(ulSecond,ulUSecond);
						}
														
						

			iDecodeCount = 0;
			while(1==iAvidFrameDataFlag)
			{
				if( gPlaystart == -1 )
					break;
				
				if( g_pstCommonParam->GST_DRV_CTRL_EnableInsertPlayBuf() < 0 )
				{
					
					if( gPlaystart ==  1 )
					{
					//	DPRINTK("stop play by hand, out!\n");
						usleep(10);
						continue;
					}
				}


				play_file_time = stFrameHeaderInfo.ulTimeSec;

				ret = g_pstCommonParam->GST_DRV_CTRL_InsertPlayBuf((char*)&stFrameHeaderInfo,pFramePlayBuf);
				
				if ( ret >= 0 )
				{
					//printf("222 %x %x %x %x\n",pFramePlayBuf[1000],pFramePlayBuf[1001],pFramePlayBuf[1002],pFramePlayBuf[1003]);
				
					if ( 0 == indexcountt )
					{
						g_pstCommonParam->GST_DRA_get_sys_time( &tvOld, NULL );
						indexcountt++;
					}
					else if ( indexcountt > 2000 )
					{
						g_pstCommonParam->GST_DRA_get_sys_time( &tvNew, NULL );

						#ifdef USE_DISK_READ_DIRECT
						printf( "READ_DIRECT %d This decode Frame Avg ************** %d\n", 
							(int)iShowTotalExecCount++,
							(int)div_safe(indexcountt,tvNew.tv_sec - tvOld.tv_sec) );

						#else
						printf( "5555 -- %d This decode Frame Avg ************** %d\n", 
							(int)iShowTotalExecCount++,
							(int)div_safe(indexcountt,tvNew.tv_sec - tvOld.tv_sec) );

						#endif
						indexcountt = 0;
					}
					else
					{
						indexcountt++;						
					}

								
					iAvidFrameDataFlag = 0;			
					break;

					
				}
				else
				{
					iDecodeCount++;

					if( iDecodeCount > 50 )
					{
						DPRINTK("decode error iDecodeCount=%d\n",iDecodeCount);
						break;
					}

					usleep(10);
				}
			}

		}
		else
		{		
			//DPRINTK("can't insert play data\n");
			usleep( 10 );
		}
		
	}


	if( pFramePlayBuf)
		free(pFramePlayBuf);

	if( audioBuf[0] )
		free(audioBuf[0]);
	
	if( audioBuf[1] )
		free(audioBuf[1]);

	g_EnableVideoPlay = 100;

}


void thread_for_audio_playback(void)
{
	SET_PTHREAD_NAME(NULL);
       // FILE* fAudio = NULL;
        int ret = 0;
	GST_DRV_BUF_INFO stAudioBufInfo;
  	//FILE_HEADER_INFO stAudioFileHeaderInfo;
        //char auidoname[60];
        //int iAudioFileIdx = 0;
       // int iCountAudioFrames = 0;
        //int iAudioFrameCount = 0;
        //int iReadSize = 0;
        //int iTestNotOpenAudioFileCount = 0;
	 int iDecode = 0;	
	 int iAudioFileOpen = 0;
	 int iLength = 0;
	 int AudioFilePos = 0;

	 DPRINTK(" pid = %d\n",getpid());
	printf("play audio out!\n");
	 return;
        
	while( g_EnableAudioPlay )
	{
		while(gPlaystart==0) usleep( 1000 );

		if( gPlaystart == -1 )
		{
			printf("thread_for_audio_playback out!\n");
			FS_CloseAudioFile();
				return;
		}	

		if( iAudioNowPlay == 1 )
		{
			iDecode = 0;
			stAudioBufInfo.iBufDataType = TD_DRV_DATA_TYPE_PLAYBACK_AUDIO;
			ret = g_pstCommonParam->GST_DRA_get_buffer_of_playback(&stAudioBufInfo);
			if ( 0 == ret )
			{				
					stAudioBufInfo.iChannelMask = 1;		
					
					if( iHaveAudioBuf == 1 && g_PlayStatus == PLAYNORMAL)
					{					
					
						if( iAudioFileOpen == 0 )
						{							
						
							iAudioFileOpen = FS_OpenAudioFile();						

							AudioFilePos = FS_GetPlayFilePos();

							//printf(" iAudioFileOpen = %d \n",iAudioFileOpen);
							
						}

						if( iAudioNowPlay == 0 )
						{	
							FS_CloseAudioFile();
							iHaveAudioBuf = 0;
							iAudioFileOpen = 0;
						}
					
						if( iAudioFileOpen > 0 )
						{	
							ret  = FS_GetAudioFrame(stAudioBufInfo.pBuf[0],&iLength,g_AudioChannel);

							if( ret <= 0 )
							{							
								FS_CloseAudioFile();
								iAudioFileOpen = 0;						
								iAudioNowPlay = 0;
								iHaveAudioBuf = 0;
								printf("audio play over!\n");
							}
							
							stAudioBufInfo.iBufLen[0] = iLength;					
							
							while(iHaveAudioBuf == 1)
							{
								ret = g_pstCommonParam->GST_DRA_send_buffer_of_playback(&stAudioBufInfo);
								if ( 0 == ret )
								{
								
									g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stAudioBufInfo);
									break;
								}
								else
								{
								
									iDecode++;
								
									if( iDecode > 5000 )
									{							
									//	g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stAudioBufInfo);
									//	break;
									}
									usleep(200);
								}
									
							}

							if( iHaveAudioBuf == 0 )
							{						
								g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stAudioBufInfo);
							}	
							
						}
						else
						{
							g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stAudioBufInfo);
						}
					}	
					else
					{
						g_pstCommonParam->GST_DRA_release_buffer_of_playback(&stAudioBufInfo);

						FS_CloseAudioFile();
						iAudioFileOpen = 0;						
						iAudioNowPlay = 0;
						iHaveAudioBuf = 0;

						printf(" play status is not normal over!\n");
					}
			}
			else
			{
				printf( "TDDRV_GetPlaybackBuf error\n" );
			}
		}
		
		usleep( 20 );		
		
	}	

	g_EnableAudioPlay = 100;
}

