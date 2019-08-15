#include "saveplayctrl.h"
#include "FTC_common.h"
#include "devfile.h"

int g_RecCam = 0x0f;
int g_RecAudioCam = 0x0f;
GST_RECORDEVENT g_RecEvent;
time_t g_RecStartTime;
int g_States;
int g_iNoChangeLog = 0;
int g_iAlreadyWriteDisk = 0;
int g_PreRecordFlag = 0;
int iLowestChannelId = 0;
int g_PreviewIsStart = 0;
int g_RecDuringTime = 0;
int g_preview_sys_flag = 0;
int g_chan_already_have_key_frame[16];


void set_rec_chan_have_key_frame(int chan,int flag)
{
	g_chan_already_have_key_frame[chan] = flag;	
}

int get_rec_is_have_key_frame(int chan )
{
	return g_chan_already_have_key_frame[chan];
}

void thread_for_rec_file()
{
	SET_PTHREAD_NAME(NULL);
	GST_DRV_BUF_INFO * pDrvBufInfo = NULL;
	int iOpenFlag = 1;
	int iCountFrame[8];
	FILE_HEADER_INFO stFileHeaderInfo;
	FRAMEHEADERINFO stFrameHeaderInfo;
	unsigned long ulTotalFrameCount = 0;
	int index = 0;
	int iFrameRateGet = 0;
	int iShowTotalExecCount = 0;
       struct timeval tvOld;
       struct timeval tvNew;
	 struct timeval audiotv;
	 struct timeval videotv;
	 struct timeval recTime;
	 struct timeval recEndTime;
	 struct timeval recstickTime;
	int  iFileOpenFlag = 0;
	int iFilePreRecFlag = 0;
	int rel;
	int idx;
	char cmd[60];
	int iTestFrame[8] = {0,0,0,0,0,0,0,0};
	int iBufId = 0;	
	char * pFrameDataBuf = NULL;
	int change_file_count = 0;
	int disk_write_size = 0;

	DPRINTK(" pid = %d\n",getpid());
	

	pFrameDataBuf = (char *) malloc(MAX_FRAME_BUF_SIZE);
	if( !pFrameDataBuf )
	{
		printf("malloc save buf error!\n");
		return;
	}


	g_RecDuringTime = 1200;
  	
	while( g_EnableFileSave )
	{
	
		while(gRecstart==0) {usleep( 1000 );change_file_count = 0;};
	
		
		// 退出 录相 和停止录相 
		//if( gRecstart == -1 ||gRecstart == -2 )
		if( (gRecstart == -1 ||gRecstart == -2) && change_file_count == 0 )
		{	
						
				if( iFileOpenFlag > 0 && g_PreRecordFlag == 0 )
				{				
				
					FS_LogMutexLock();
			
					rel = RecWriteOver(STOPRECORD);

					FS_LogMutexUnLock();

					if( rel < 0 )
						DPRINTK(" RecWriteOver error!\n");
					else
						DPRINTK(" RecWriteOver OK!\n");				

					iFileOpenFlag = 0;
				}				

				if( gRecstart == -1 )
				{
					printf("rec thread out\n");
					return;
				}

				if( gRecstart == -2 )
				{
					printf(" stop rec \n");
					gRecstart = 0;
					if(g_preview_sys_flag == 0)
						g_pstCommonParam->GST_DRA_start_stop_recording_flag(0);
					continue;
				}
				
				
		}	
	
	

		pDrvBufInfo = g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(pFrameDataBuf);
		if ( pDrvBufInfo  && (TD_DRV_DATA_TYPE_VIDEO == pDrvBufInfo->iBufDataType)  )
		{		

			g_previous_headtime = pDrvBufInfo->tv.tv_sec;
				
			for ( index = 0; index <g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); index++ )					
			{
				if (!( pDrvBufInfo->iChannelMask & ( 1 << index )))
						continue;	

				iLowestChannelId = index;
				break;				
			}
	

			g_RecStatusCurrent = 1;
					
			if ( 0 == iFrameRateGet )
			{
				g_pstCommonParam->GST_DRA_get_sys_time( &tvOld, NULL );
			}

			//if( iFileOpenFlag  > 0 &&  gRecstart == 1 &&  g_PreRecordFlag == 0 )
			if( iFileOpenFlag  > 0  &&  g_PreRecordFlag == 0 )
			{
				rel  = FS_CheckRecFile();
				if( rel == FILEFULL)
				{
					if( change_file_count == 0)
					{
						g_pstCommonParam->GST_DRA_local_instant_i_frame(0xffff);
						printf(" create key frame for change file, iLowestChannelId=%d\n",iLowestChannelId);
						change_file_count = 1;
					}
				}
			}

			

			// 每当出现第一个关键帧的时间 ，都 要检查文件
			// 是否写完了.这样就可保证换文件时不会丢掉几帧数据
			//if( iFileOpenFlag  > 0 && pDrvBufInfo->iFrameType[iLowestChannelId] == 1  &&  gRecstart == 1 &&  g_PreRecordFlag == 0 )
			if( iFileOpenFlag  > 0 && pDrvBufInfo->iFrameType[iLowestChannelId] == 1 )
			{
				rel  = FS_CheckRecFile();
				if( rel == FILEFULL )
				{
					g_pstCommonParam->GST_DRA_get_sys_time( &recEndTime, NULL );

					// 保证每多少秒将产生一条录相记录				
					if( recEndTime.tv_sec  -recTime.tv_sec  > g_RecDuringTime || FS_RecFileChangePartiton()  )
					{
						printf(" recEndTime=%ld recTime=%ld -- %ld \n",
							recEndTime.tv_sec,recTime.tv_sec,recEndTime.tv_sec - recTime.tv_sec);
						if( recEndTime.tv_sec  -recTime.tv_sec > 5 )
						{
							g_iNoChangeLog = 1;						

							FS_LogMutexLock();
			
							rel = RecWriteOver(RECTIMEOVER);

							FS_LogMutexUnLock();

							if( rel < 0 )
								DPRINTK(" RecWriteOver error!\n");
							else
								DPRINTK(" RecWriteOver OK!\n");	

							iFileOpenFlag = 0;
						}else
						{
							printf("2  rec time is not have 5 second!\n");
						}
							
					}else
					{	
						// 确保在开始录相的文件中留下每个通道
						//至少一帧关键帧
						if( recEndTime.tv_sec  -recTime.tv_sec > 5 )
						{							
							FS_LogMutexLock();
							
							rel = RecWriteOver(FILEWRITEOVER);
							
							FS_LogMutexUnLock();
							
							if( rel < 0 )
							{
								DPRINTK(" RecWriteOver error!\n");
								//FS_SetErrorMessage(rel, 0);
								gRecstart = 0;						
							}

							iFileOpenFlag = 0;
						}
						else
						{
							printf("rec time is not have 5 second!\n");
						}
					}				

					DPRINTK(" change file!\n");
				
				}
			}

	//		printf("0=%d 1=%d 2=%d 3=%d\n",pDrvBufInfo->iFrameType[0],pDrvBufInfo->iFrameType[1],pDrvBufInfo->iFrameType[2],pDrvBufInfo->iFrameType[3]);
		
			//if ( pDrvBufInfo->iFrameType[iLowestChannelId] == 1 && iFileOpenFlag <= 0  && gRecstart == 1)
			if ( pDrvBufInfo->iFrameType[iLowestChannelId] == 1 && iFileOpenFlag <= 0  )
			{
				g_pstCommonParam->GST_DRA_get_sys_time( &videotv, NULL );
				
		//		if( ((videotv.tv_sec  - pDrvBufInfo->tv.tv_sec <= 2) && (videotv.tv_sec  - pDrvBufInfo->tv.tv_sec >= -1) )
		//			|| (g_preview_sys_flag == 1 && g_PreviewIsStart == 1))

				DPRINTK("time now=%ld  rec time=%ld %d  g_preview_sys_flag=%d g_PreviewIsStart%d\n",videotv.tv_sec,
					pDrvBufInfo->tv.tv_sec,videotv.tv_sec  - pDrvBufInfo->tv.tv_sec,g_preview_sys_flag,g_PreviewIsStart);
				
				if( ((videotv.tv_sec  - pDrvBufInfo->tv.tv_sec <= 120) && (videotv.tv_sec  - pDrvBufInfo->tv.tv_sec >= -120) )
					&& (g_preview_sys_flag == 1 && g_PreviewIsStart == 1))
				{			
					if(   g_PreRecordFlag == 0  && iFilePreRecFlag ==  0)
					{
						change_file_count = 0;

						FS_PlayMutexLock();	
						DPRINTK("g_play_lock lock!\n");
					
						FS_LogMutexLock();						
						
						iFileOpenFlag = FS_OpenRecFile(g_pstRecModeParam , g_RecCam,g_iNoChangeLog);						
						
						FS_LogMutexUnLock();

						DPRINTK("g_play_lock unlock!\n");	
						FS_PlayMutexUnLock();
						
						if ( iFileOpenFlag < 0 )
						{
							//录相 过程中，换文件时发现没有文件可用的处理
							if(  g_iNoChangeLog==0)
							{
								FS_ERR_WriteEndInfotoRecLog();
							}
						
							printf("FS_OpenRecFile error! %d\n",iFileOpenFlag);
						//	FS_SetErrorMessage(iFileOpenFlag, 0);
							gRecstart = -2;
						}else
						{
							if( g_iNoChangeLog == 1 )
							{
								g_pstCommonParam->GST_DRA_get_sys_time( &recTime, NULL );
								printf(" recTime = %ld\n",recTime.tv_sec);
							}
							
							g_iNoChangeLog = 0;
						}
					}
					
				}
				else
				{
					printf(" REC start type=%d   time=%ld curtime=%ld\n",
					pDrvBufInfo->iFrameType[iLowestChannelId],pDrvBufInfo->tv.tv_sec,videotv.tv_sec );
					printf(" wrong time!\n");
				}
				
			}				

			g_pstCommonParam->GST_DRA_get_sys_time( &videotv, NULL );

			if( (videotv.tv_sec % 300 <= 3) && (iFileOpenFlag > 0) )
			{
				recstickTime.tv_sec = videotv.tv_sec / 300;
				recstickTime.tv_sec = recstickTime.tv_sec * 300;
				FS_WriteInfotoRecTimeStickLog(recstickTime.tv_sec,g_RecCam);
			}

			// 手动录相
			if (iFileOpenFlag > 0 &&  g_PreRecordFlag == 0 )
			{			
				for ( index = 0; index <g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); index++ )						
				{
					if (!( pDrvBufInfo->iChannelMask & ( 1 << index )))
						continue;		
					
					// 判断是否录这个通道
					if( FTC_GetBit(g_RecCam, index) == 0 )
						continue;

					//确保预录象的时间和现场时间差距不大，否则会影响回放。
					if( (videotv.tv_sec  - pDrvBufInfo->tv.tv_sec >= 120) ||
						(videotv.tv_sec  - pDrvBufInfo->tv.tv_sec <= -120) )
					{
						DPRINTK(" REC chan=%d num=%d time=%ld curtime=%ld\n",index,
						pDrvBufInfo->iFrameCountNumber[index],
						pDrvBufInfo->tv.tv_sec,videotv.tv_sec );

						continue;						
					}

					

					stFrameHeaderInfo.iChannel = index;
					stFrameHeaderInfo.type       = 'V';
					stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[index];
					stFrameHeaderInfo.ulFrameType = pDrvBufInfo->iFrameType[index];
					stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
					stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;
					stFrameHeaderInfo.videoStandard = g_pstRecModeParam.iPbVideoStandard;
					stFrameHeaderInfo.imageSize	= g_pstRecModeParam.iPbImageSize;
					stFrameHeaderInfo.ulFrameNumber = pDrvBufInfo->iFrameCountNumber[index];

				//	printf("%d %d length=%d \n",index,stFrameHeaderInfo.imageSize,stFrameHeaderInfo.ulDataLen);

					//make sure first frame is key frame.
					if( get_rec_is_have_key_frame(stFrameHeaderInfo.iChannel) == 0 )
					{
						if( stFrameHeaderInfo.type == 'V' && stFrameHeaderInfo.ulFrameType == 1 )
						{
							set_rec_chan_have_key_frame(stFrameHeaderInfo.iChannel,1);
						}else
							continue;
					}
					
					if( pDrvBufInfo->tv.tv_sec == 0 )
						pDrvBufInfo->tv.tv_sec = videotv.tv_sec;
				
					//printf("size= %d %d    length: %d   type=%c  frametype=%d iBufId = %d ulFrameNumber=%ld\n",  g_pstRecModeParam.iPbImageSize,index, pDrvBufInfo->iBufLen[index],
					//								stFrameHeaderInfo.type,stFrameHeaderInfo.ulFrameType,iBufId,stFrameHeaderInfo.ulFrameNumber );

				//	printf(" time sec = %ld, time use=%ld\n",stFrameHeaderInfo.ulTimeSec,stFrameHeaderInfo.ulTimeUsec);

					
					iCountFrame[index]++;
					{
						g_iAlreadyWriteDisk = 1;
					
						rel = FS_WriteOneFrame(&stFrameHeaderInfo,pFrameDataBuf);							
						if( rel < 0 )
						{						
							printf(" FS_WriteOneFrame error!\n");
							// 写出错的处理方法是
							// 将这个正在录相的文件标识为不可用
							// 跳到一下在文件继续录,并且结束上一次的录相记录，
							//开始新一段录相 

							FS_LogMutexLock();							
							rel = RecWriteOver(FILEWRITEERROR);							
							FS_LogMutexUnLock();
							
							if( rel < 0 )
							{
								printf(" FILEERROR change file  error!\n");	
								//FS_SetErrorMessage(rel, 0);
								gRecstart = 0;
								break;
							}
							printf(" FILEERROR change file!\n");	
							
							iFileOpenFlag = 0;
							g_iNoChangeLog = 1;

							if( rel  == RECTESTERROR)
							{
								gRecstart = -2;
							}
							
							break;							
						}								
					//	printf(" %d    length: %d\n", index, pDrvBufInfo->iBufLen[index]);
						disk_write_size += stFrameHeaderInfo.ulDataLen;
					}
				}
			}	
		
			iFrameRateGet++;
			
			if ( iFrameRateGet > 2000 )
			{
				g_pstCommonParam->GST_DRA_get_sys_time( &tvNew, NULL );

				#ifdef USE_DISK_WIRET_DIRECT
				printf( "WIRET_DIRECT %d This Encode Frame Avg ************** %d  speed=%d byte/s \n", 
					(int)iShowTotalExecCount++,
					(int)div_safe(iFrameRateGet,tvNew.tv_sec - tvOld.tv_sec),(int)div_safe(disk_write_size,tvNew.tv_sec - tvOld.tv_sec) );
				
				#else
				printf( "3333 -- %d This Encode Frame Avg ************** %d  speed=%d byte/s \n", 
					(int)iShowTotalExecCount++,
					(int)div_safe(iFrameRateGet,tvNew.tv_sec - tvOld.tv_sec),(int)div_safe(disk_write_size,tvNew.tv_sec - tvOld.tv_sec) );
				#endif
				iFrameRateGet = 0;
				disk_write_size = 0;
			}

			

		//	g_pstCommonParam->GST_DRA_release_encode_device_buffer(pDrvBufInfo);
		//	g_pstCommonParam->GST_CMB_RecycleBuffer(pDrvBufInfo);

		}
		else if ( pDrvBufInfo  && (TD_DRV_DATA_TYPE_AUDIO == pDrvBufInfo->iBufDataType) )
		{
			g_pstCommonParam->GST_DRA_get_sys_time( &audiotv, NULL );
		
			if (iFileOpenFlag > 0  &&  g_PreRecordFlag == 0)
			{
				
				for ( idx = 0; idx <g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); idx++ )								
				{
					if (pDrvBufInfo->iChannelMask & (1<<idx) )
					{					
						if( FTC_GetBit(g_RecCam, idx) == 0 )
							continue;
						
						if( FTC_GetBit(g_RecAudioCam, idx) == 0 )
							continue;
					
						stFrameHeaderInfo.iChannel = idx;
						stFrameHeaderInfo.type       = 'A';
						stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[idx];
						stFrameHeaderInfo.ulFrameType = 0;
						stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
						stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;

						
						rel = FS_WriteOneFrame(&stFrameHeaderInfo,pFrameDataBuf);

						if( rel < 0 )
						{						
							printf(" FS_WriteOneFrame error!\n");
							//FS_SetErrorMessage(rel, 0);								
							
							FS_LogMutexLock();							
							rel = RecWriteOver(FILEWRITEERROR);							
							FS_LogMutexUnLock();
							
							if( rel < 0 )
							{
								printf(" FILEERROR change file  error!\n");					
								gRecstart = 0;
								break;
							}
							printf(" FILEERROR change file!\n");								
							iFileOpenFlag = 0;
							g_iNoChangeLog = 1;
							break;
						}

					//	printf("audio  %d    length: %d\n", idx, pDrvBufInfo->iBufLen[idx]);
					}
				}
					
				//printf("audio  %d    length: %d\n", idx, pDrvBufInfo->iBufLen[idx]);
			}
		
			//g_pstCommonParam->GST_DRA_release_encode_device_buffer(pDrvBufInfo);
			//g_pstCommonParam->GST_CMB_RecycleBuffer(pDrvBufInfo);
	
		}
		else 
		{

			#ifdef hi3515
			usleep( 100 );
			#else
			usleep( 10 );
			#endif
			
		}

	}

	printf(" rec thread end.......\n");


	if( pFrameDataBuf )
		free(pFrameDataBuf);
	
	g_EnableFileSave = 100;
}


