#include "filesystem.h"
#include "FTC_common.h"
#include "IOFile.h"

FILEINFOLOGPOINT pLogPoint;
pthread_mutex_t file_info_mutex;

int FS_InitFileInfoLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pLogPoint.fileBuf  = NULL;



// 支持1TG硬盘时，PATITIONSIZE 应该为100 而不是60，上面的
//代码会造成分配的内存不够用，改为100
	pLogPoint .fileBuf = (char *)malloc(sizeof(FILEUSEINFO) + 
				    (LARGE_DISK_PATITION_SIZE * 1024 / 128 ) * sizeof(FILEINFO));


	if(pLogPoint .fileBuf  == NULL )
			return NOENOUGHMEMORY;

	pthread_mutex_init(&file_info_mutex,NULL);
	
	iFirst = 0;
		
	return ALLRIGHT;
	
}


int FS_LoadFileInfoLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;

	rel = FS_GetFileUseInfo(iDiskId,iPatitionId,&stDiskUseInfo);
	if( rel < 0 )
		return FILEERROR;

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/fileinfo.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open %s \n",FileName);
			return FILEERROR;
	}

	if( (sizeof(FILEUSEINFO) + stDiskUseInfo.iTotalFileNumber * sizeof(FILEINFO))
		>    (LARGE_DISK_PATITION_SIZE * 1024 / 128 ) * sizeof(FILEINFO))
	{
		DPRINTK("fileinfo.log too large,no enough memory\n");
		return FILEERROR;
	}

	rel =  fread( pLogPoint.fileBuf, 1, sizeof(FILEUSEINFO) + 
				stDiskUseInfo.iTotalFileNumber * sizeof(FILEINFO), fp);

	if( rel != sizeof(FILEUSEINFO) + stDiskUseInfo.iTotalFileNumber * sizeof(FILEINFO) )
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);
	
	pLogPoint.iCurFilePos = stDiskUseInfo.iCurFilePos;
	pLogPoint.pUseinfo = (FILEUSEINFO * )pLogPoint.fileBuf;
	pLogPoint.pFilePos  = (FILEINFO  *)(pLogPoint.fileBuf + sizeof(FILEUSEINFO) +
						stDiskUseInfo.iCurFilePos * sizeof(FILEINFO));

	pLogPoint.iDiskId = iDiskId;
	pLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}


int  FS_GetFileUseInfo(int iDiskId,int iPatitionId, FILEUSEINFO * pstDiskUseInfo)
{
	FILE *fp = NULL;
	int iSize = 0;
	int iDiskNum = 0;
	char FileName[50];
	int j,i;
	int rel;

	GST_DISKINFO stDiskInfo;

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/fileinfo.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open %s \n",FileName);
			return FILEERROR;
	}

	rel = fread(pstDiskUseInfo,1, sizeof( FILEUSEINFO ),fp);

	if( rel != sizeof( FILEUSEINFO ) )
	{
				printf(" read error!\n");
				fclose(fp);
				return FILEERROR;
	}

	fclose(fp);

	return ALLRIGHT;
}


int FS_ReleaseFileInfoLogBuf()
{
	if( pLogPoint .fileBuf )
	{
		free(pLogPoint .fileBuf );
		pLogPoint .fileBuf  = NULL;
		pthread_mutex_destroy( &file_info_mutex);
	}


	return ALLRIGHT;
}


FILEINFOLOGPOINT * FS_GetPosFileInfoLock(int iDiskId, int iPatitionId, int iFilePos,FILEINFOLOGPOINT * input_point)
{
	int rel;

	if( pLogPoint .fileBuf  == NULL )
	{
		printf(" pLogPoint .fileBuf  is NULL!\n");
		return NULL;
	}

	if( iDiskId < 0 || iPatitionId < 5 )
	{
		printf("iDiskId = %d iPatitionId = %d\n",iDiskId,iPatitionId);
		return NULL;
	}

	if( iDiskId != pLogPoint.iDiskId || iPatitionId != pLogPoint.iPatitionId
		|| g_CurrentFormatFilelistReload == 1)
	{
		rel = FS_LoadFileInfoLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadFileInfoLog error!iDiskId =%d,iPatitionId=%d\n",iDiskId,iPatitionId);			
			return NULL;
		}

		if( g_CurrentFormatFilelistReload == 1 )
			printf(" g_CurrentFormatFilelistReload = %d\n",g_CurrentFormatFilelistReload);

		g_CurrentFormatFilelistReload = 0;

	}

	if( iFilePos >=  pLogPoint.pUseinfo->iTotalFileNumber ||
		iFilePos < 0 )
	{
		printf(" iFilePos = %d iTotalFileNumber = %d\n",iFilePos,pLogPoint.pUseinfo->iTotalFileNumber);
		return NULL;
	}

	pLogPoint.pFilePos =  (FILEINFO  *)(pLogPoint.fileBuf +
					sizeof(FILEUSEINFO) + iFilePos * sizeof(FILEINFO));

	input_point->file_info = *(pLogPoint.pFilePos);
	input_point->use_info = *(pLogPoint.pUseinfo);

	pLogPoint.iCurFilePos = iFilePos;

	input_point->fileBuf = pLogPoint.fileBuf;
	input_point->iCurFilePos = pLogPoint.iCurFilePos;
	input_point->iDiskId = pLogPoint.iDiskId;
	input_point->iPatitionId = pLogPoint.iPatitionId;
	input_point->pFilePos = &input_point->file_info;
	input_point->pUseinfo = &input_point->use_info;	

	if(  input_point->file_info.fileName[0] != 'v' )
	{
		DPRINTK("file_info.fileName is %s  error\n",input_point->file_info.fileName);
		return NULL;
	}
	
	return input_point;	
}




FILEINFOLOGPOINT * FS_GetPosFileInfo(int iDiskId, int iPatitionId, int iFilePos,FILEINFOLOGPOINT * input_point)
{
	FILEINFOLOGPOINT * point = NULL;

	pthread_mutex_lock( &file_info_mutex);

	point = FS_GetPosFileInfoLock(iDiskId,iPatitionId,iFilePos,input_point);

	pthread_mutex_unlock( &file_info_mutex);

	
	return point;	
}



int UseInfoWriteToFileLock(FILEINFOLOGPOINT * input_point)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(input_point->iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/fileinfo.log", &stDiskInfo.devname[5],input_point->iPatitionId); // maybe devName is /tddvr/hda5


	if( input_point->iDiskId != pLogPoint.iDiskId || input_point->iPatitionId != pLogPoint.iPatitionId )
	{
		rel = FS_LoadFileInfoLog( input_point->iDiskId , input_point->iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadFileInfoLog error!iDiskId =%d,iPatitionId=%d\n",input_point->iDiskId,input_point->iPatitionId);			
			return ERROR;
		}		

	}	

	*(pLogPoint.pUseinfo ) =input_point->use_info;


	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open fileinfo.log \n");
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(input_point->pUseinfo,1, sizeof( FILEUSEINFO ),fp);

	if( rel != sizeof( FILEUSEINFO ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);	

	DPRINTK("iCurFilePos=%d iTotalFileNumber=%d\n",
		input_point->pUseinfo->iCurFilePos,
		input_point->pUseinfo->iTotalFileNumber);

	sync();

	return ALLRIGHT;
	

}


int UseInfoWriteToFile(FILEINFOLOGPOINT * input_point)
{
	int rel;

	pthread_mutex_lock( &file_info_mutex);

	rel = UseInfoWriteToFileLock(input_point);

	pthread_mutex_unlock( &file_info_mutex);

	return rel;
	

}

int FS_FileInfoWriteToFileLock(FILEINFOLOGPOINT * input_point)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(input_point->iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/fileinfo.log", &stDiskInfo.devname[5],input_point->iPatitionId); // maybe devName is /tddvr/hda5

	if( input_point->iDiskId != pLogPoint.iDiskId || input_point->iPatitionId != pLogPoint.iPatitionId )
	{
		rel = FS_LoadFileInfoLog( input_point->iDiskId , input_point->iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadFileInfoLog error!iDiskId =%d,iPatitionId=%d\n",input_point->iDiskId,input_point->iPatitionId);			
			return ERROR;
		}		

	}

	pLogPoint.pFilePos =  (FILEINFO  *)(pLogPoint.fileBuf +
					sizeof(FILEUSEINFO) + input_point->iCurFilePos* sizeof(FILEINFO));

	*(pLogPoint.pFilePos) = input_point->file_info;

	if(  input_point->file_info.fileName[0] != 'v' )
	{
		DPRINTK("file_info.fileName is %s  error\n",input_point->file_info.fileName);
		return ERROR;
	}

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open fileinfo.log \n");
			return FILEERROR;
	}

	offset = sizeof( FILEUSEINFO ) + input_point->iCurFilePos * sizeof(FILEINFO);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(input_point->pFilePos , 1, sizeof( FILEINFO ),fp);

	if( rel != sizeof( FILEINFO ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}



int FS_FileInfoWriteToFile(FILEINFOLOGPOINT * input_point)
{
	int rel;

	pthread_mutex_lock( &file_info_mutex);

	rel = FS_FileInfoWriteToFileLock(input_point);

	pthread_mutex_unlock( &file_info_mutex);

	return rel;

}


int FS_FixFileInfo(int iDiskId, int iPartitionId)
{
	FILEINFOLOGPOINT * pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	FILEUSEINFO stFileinfo;
	time_t time;
	unsigned long ulFrameOffset;
	
	int rel;
	int k;
	
	rel = FS_GetFileUseInfo(iDiskId, iPartitionId, &stFileinfo);
	if( rel < 0 )
		return ERROR;

	// fileinfo.log 中单个文件搜索
	for( k = 0 ; k < stFileinfo.iTotalFileNumber; k++ )
	{
		pFileInfoPoint = FS_GetPosFileInfo(iDiskId, iPartitionId, k,&fileInfoPoint);
		if( pFileInfoPoint == NULL )
				return ERROR;

		if(pFileInfoPoint->pFilePos->states == WRITING )
		DPRINTK("file(%d,%d,%d) states=%d\n",iDiskId, iPartitionId, k,pFileInfoPoint->pFilePos->states);

		// 判断这个文件是不是录相时异常中断
		if( pFileInfoPoint->pFilePos->states == WRITING  )
		{
			// 修复录相文件
			rel = FS_FixFile(iDiskId, iPartitionId , k, &time, & ulFrameOffset);
			//system("cat /proc/meminfo");
			if( rel == ALLRIGHT )
			{
				//修复fileinfo.log
				pFileInfoPoint->pFilePos->states = WRITEOVER;
				pFileInfoPoint->pFilePos->EndTime = time;
				pFileInfoPoint->pFilePos->ulOffSet = ulFrameOffset;
				pFileInfoPoint->pFilePos->iDataEffect = YES;

				rel = FS_FileInfoWriteToFile(pFileInfoPoint);
				if( rel < 0 )
				{
					printf(" FS_FileInfoWriteToFile error! 349\n");
					return FILEERROR;
				}

				rel = FS_WriteEndInfotoRecLog(iDiskId,iPartitionId,time);
				if(rel < 0 )
				{
					printf("FS_WriteEndInfotoRecLog error! 349\n");
					return rel;
				}
			}
			else 
			{
				printf(" fix bad file %d,%d,%d error=%d\n",iDiskId,iPartitionId,k,rel);

				if( rel == FILESEARCHERROR )
				{
					time = pFileInfoPoint->pFilePos->StartTime + 5;
					// 文件异常截断和修复失败的处理
					pFileInfoPoint->pFilePos->states = WRITEOVER;
					pFileInfoPoint->pFilePos->iDataEffect = YES;

					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" FS_FileInfoWriteToFile error! 349\n");
						return FILEERROR;
					}					

					rel = FS_WriteEndInfotoRecLog(iDiskId,iPartitionId,time);
					if(rel < 0 )
					{
						printf("FS_WriteEndInfotoRecLog error! 349\n");
						return rel;
					}
					

					
				}else if( rel ==  FILE_SIZE_ZERO_ERROR)
				{
					
					// 文件异常截断和修复失败的处理
					pFileInfoPoint->pFilePos->states = NOWRITE;
					pFileInfoPoint->pFilePos->iDataEffect = NOEFFECT;
					pFileInfoPoint->pFilePos->StartTime = 0;
					pFileInfoPoint->pFilePos->EndTime = 0;				
			

					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" FS_FileInfoWriteToFile error! 349\n");
						return FILEERROR;
					}					

					
				}else
				{					
					
					time = pFileInfoPoint->pFilePos->StartTime + 5;
					// 文件异常截断和修复失败的处理
					pFileInfoPoint->pFilePos->states = BADFILE;
					pFileInfoPoint->pFilePos->EndTime = time;
					pFileInfoPoint->pFilePos->ulOffSet = ulFrameOffset;
					pFileInfoPoint->pFilePos->iDataEffect = NO;

					rel = FS_FileInfoWriteToFile(pFileInfoPoint);
					if( rel < 0 )
					{
						printf(" FS_FileInfoWriteToFile error! 349\n");
						return FILEERROR;
					}

					rel = FS_WriteEndInfotoRecLog(iDiskId,iPartitionId,time);
					if(rel < 0 )
					{
						printf("FS_WriteEndInfotoRecLog error! 349\n");
						return rel;
					}
					
					rel =FS_CoverFileClearLog(pFileInfoPoint->pFilePos->StartTime,
								pFileInfoPoint->pFilePos->EndTime, -1);
					if( rel < 0 )
					{
						printf(" FS_CoverFileClearLog error!");					
					}		

					rel = FS_DelRecLogByFilePos(iDiskId,iPartitionId,k);
					if( rel < 0 )
					{
						printf(" FS_DelRecLogByFilePos error!");	
					}

				}
				
			}
		}

		//判断这个文件是不是回放时异常中断
		if( pFileInfoPoint->pFilePos->states == PLAYING )
		{
			// 修复 fileinfo.log
			pFileInfoPoint->pFilePos->states = WRITEOVER;

			rel = FS_FileInfoWriteToFile(pFileInfoPoint);
			if( rel < 0 )
			{
				printf(" FS_FileInfoWriteToFile error!\n");
				return FILEERROR;
			}
			
		}
		
	}

	return ALLRIGHT;
}

int FS_HaveFileToRecover(int iDiskId)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT   fileInfoPoint;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

	printf("FS_HaveFileToRecover 1 diskid=%d\n",iDiskId);

	for(j = 1; j < stDiskInfo.partitionNum; j++ )
	{
		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
				
		pFileInfoPoint = FS_GetPosFileInfo(iDiskId,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint );
		if( pFileInfoPoint == NULL )
		{
			printf(" pFileInfoPoint = FS_GetPosFileInfo error!11\n");
			return ERROR;
		}
			
		for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
		{				
			pFileInfoPoint = FS_GetPosFileInfo(iDiskId,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

			if( pFileInfoPoint == NULL )
			{
				printf(" pFileInfoPoint = FS_GetPosFileInfo error!22\n");
				return ERROR;
			}

			if( pFileInfoPoint->pFilePos->states == WRITEOVER )
				return ALLRIGHT;
		}
	}

	printf("no file to cover!11\n");

	return ERROR;
}

#define MAX_LOST_FRAME 50

int FS_FixFile(int iDiskId,int iPartitionId,int iFilePos, time_t * time, unsigned long * ulOffset)
{
	FILEINFOLOGPOINT * pFileInfoPoint;
	FILEINFOLOGPOINT   fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	FRAMEHEADERINFO  stFrameInfo;
	FILEINFO   stFileInfo;
	time_t  lasttime;
	time_t  last_time_array[MAX_LOST_FRAME];
	char fileName[50];
	int rel;
	unsigned long ulframeoffset = 0;
	unsigned long ullastoffst = 0;
	unsigned long last_offset_array[MAX_LOST_FRAME];
	int Channel = 0;
	int fileLength = 0;
	FILE * fp_header = NULL;
	FILE * fp_data = NULL;
	int lost_four_group_data = 0;	
	int save_frame_num = 0;
	long datafileOffset = 0;
	int value1;

	#ifdef USE_FILE_IO_SYS

	DISK_IO_FILE filest;
	DISK_IO_FILE filest2;
	char dev[50];	

	stDiskInfo = DISK_GetDiskInfo( iDiskId );

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId, iPartitionId, iFilePos,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
		return ERROR;

	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avh",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );
	if( file_io_open(dev,fileName,"+",&filest,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}


	memset(fileName, 0 , 50 );
	memset(dev, 0 , 50 );
	sprintf(fileName,"%s.avd",  pFileInfoPoint->pFilePos->fileName );
	sprintf(dev,"/dev/%s%d", &stDiskInfo.devname[5], iPartitionId+1  );
	if( file_io_open(dev,fileName,"+",&filest2,0) < 0 )
	{
		printf("can't open %s %s\n",dev,fileName);
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}
					

	datafileOffset = file_io_file_get_length(&filest2);

	if( datafileOffset < 2000 )
	{
		DPRINTK("11 fileName %s size = %ld error!\n",fileName,datafileOffset);
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}

	DPRINTK("%s len=%d\n",filest2.filename,datafileOffset);

	if( file_io_lseek(&filest,  0 , SEEK_SET ) == (off_t)-1  )
	{
		DPRINTK("seek error \n");
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}

	rel = file_io_read(&filest,&stFileInfo,sizeof(FILEINFO));
	if( rel != sizeof(FILEINFO) )
	{
		printf(" fread error 395\n");
		rel = FILE_SIZE_ZERO_ERROR;
		goto file_op_error;
	}
	

	// 得到帧头的偏移位置
	ulframeoffset = stFileInfo.ulOffSet;

	if( file_io_lseek(&filest,  ulframeoffset , SEEK_SET ) == (off_t)-1  )
	{
		DPRINTK("seek error \n");
		rel = FILEERROR;
		goto file_op_error;
	}

	// 检测出现异常录相的第一帧
	rel = file_io_read(&filest,&stFrameInfo,sizeof(FRAMEHEADERINFO));
	if( rel != sizeof(FRAMEHEADERINFO) )
	{
		printf(" fread error 397\n");
		rel = FILESEARCHERROR;
		goto file_op_error;
	}	


	if( stFrameInfo.type != 'V'  && stFrameInfo.ulTimeSec != 0 && stFrameInfo.ulFrameType != 1 )
	{
		printf("error 442\n");
		rel = FILESEARCHERROR;
		goto file_op_error;
	}

	DPRINTK("fix start channel =%d  time=%ld usec=%ld\n",stFrameInfo.iChannel,stFrameInfo.ulTimeSec,stFrameInfo.ulTimeUsec);

	Channel = stFrameInfo.iChannel;

	lasttime =  stFrameInfo.ulTimeSec;
	
	ullastoffst = ulframeoffset;	
	
	ulframeoffset += sizeof(FRAMEHEADERINFO);

	for( rel = 0; rel < MAX_LOST_FRAME ; rel++)
	{
		last_time_array[rel] = stFrameInfo.ulTimeSec;
		last_offset_array[rel] = ulframeoffset;
	}

	printf("======FIX FILE=======2\n");
	

	// 顺序检测每一帧，若两帧的时差大于一秒，则表示文件结束。
	while(1)
	{
		// 文件读完了，（一般不会有这种情况）
		rel = file_io_read(&filest,&stFrameInfo,sizeof(FRAMEHEADERINFO));
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			printf("file read over!\n");
			goto FIXOVER;
		}	


		// 两帧的时差大于一秒
		value1 = stFrameInfo.ulTimeSec - lasttime;
		
		if(value1 > 3  || value1 < (-3)  )
		{
			printf("Fix Last frame time = %ld,BadFrame time = %ld ullastoffst=%ld  value1=%d\n",
				lasttime,stFrameInfo.ulTimeSec,ullastoffst,value1);	
		
			goto FIXOVER;
		}

		if( stFrameInfo.ulFrameDataOffset > datafileOffset )
		{
			printf("ulFrameDataOffset=%ld datafileOffset=%ld\n",stFrameInfo.ulFrameDataOffset ,datafileOffset );
			goto FIXOVER;
		}

		save_frame_num++;		
	
		// 将第一个通道的上一个帧的位置保存
		// 查到出错地方时，这个位置就将成为结束位置。
		if( stFrameInfo.iChannel == Channel )
		{
		//	lost_four_group_data++;

			//if( lost_four_group_data >= MAX_LOST_FRAME )
			{
				ullastoffst = ulframeoffset;
				lasttime = stFrameInfo.ulTimeSec ;
				//lost_four_group_data = 0;
			}

			for( rel = 0; rel < (MAX_LOST_FRAME -1); rel++)
			{
				last_time_array[rel] = last_time_array[rel+1];
				last_offset_array[rel] = last_offset_array[rel+1];
			}

			last_time_array[MAX_LOST_FRAME-1] = lasttime;
			last_offset_array[MAX_LOST_FRAME-1] = ullastoffst;			
		}		
		
		ulframeoffset += sizeof(FRAMEHEADERINFO);



	}
	
FIXOVER:	
	lasttime = last_time_array[0];
	ullastoffst = last_offset_array[0];

	DPRINTK("save_frame_num = %d sec=%ld usec=%ld chan=%d ullastoffst=%ld\n",save_frame_num,
		stFrameInfo.ulTimeSec,stFrameInfo.ulTimeUsec,stFrameInfo.iChannel,ullastoffst);
	
	*time = lasttime;
	*ulOffset = ullastoffst;
	// 修改录相文件中的fileinfo结构
	stFileInfo.EndTime = lasttime;
	stFileInfo.states	= WRITEOVER;
	stFileInfo.ulOffSet  = ullastoffst;

	if( file_io_lseek(&filest,  0 , SEEK_SET ) == (off_t)-1  )
	{
		DPRINTK("seek error \n");
		rel = FILEERROR;
		goto file_op_error;
	}	

	rel = file_io_write(&filest,&stFileInfo,sizeof(FILEINFO));
	if( rel != sizeof(FILEINFO) )
	{
		printf(" fread error 397\n");
		rel = FILEERROR;
		goto file_op_error;
	}	

	file_io_close(&filest);
	file_io_close(&filest2);
	
	return ALLRIGHT;

file_op_error:
	file_io_close(&filest);
	file_io_close(&filest2);
	return rel;

#else

	stDiskInfo = DISK_GetDiskInfo( iDiskId );

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId, iPartitionId, iFilePos,&fileInfoPoint);
	if( pFileInfoPoint == NULL )
		return ERROR;

	memset(fileName, 0 , 50 );

	sprintf(fileName,"/tddvr/%s%d/%s.avh", &stDiskInfo.devname[5]
				, iPartitionId , pFileInfoPoint->pFilePos->fileName );

	fp_header = fopen(fileName,"r+b");	

	if( !fp_header )
	{
		printf("can't open %s\n",fileName);
		//fclose(fp_header);
		return FILE_SIZE_ZERO_ERROR;
	}

	sprintf(fileName,"/tddvr/%s%d/%s.avd", &stDiskInfo.devname[5]
				, iPartitionId , pFileInfoPoint->pFilePos->fileName );

	fp_data = fopen(fileName,"r+b");

	if( !fp_data )
	{
		printf("can't open %s\n",fileName);
		fclose(fp_header);
		//fclose(fp_data);
		return FILE_SIZE_ZERO_ERROR;
	}

	//  有时候物理修复硬盘时，会将avd文件截断，但不截断avh文件。
	rel = fseek(fp_data, 0, SEEK_END);
	if ( rel < 0 )
	{					
		fclose(fp_header);
		fclose(fp_data);
		printf("  fseek error!\n");
		return FILEERROR;
	}					

	datafileOffset = ftell(fp_data);

//录SD卡时，有可能avd,avh的大小为0.
// add 2011/08/24
	if( datafileOffset < 2000 )
	{
		DPRINTK("11 fileName %s size = %ld error!\n",fileName,datafileOffset);

		fclose(fp_header);
		fclose(fp_data);
		return FILE_SIZE_ZERO_ERROR;
	}

	rel = fseek(fp_header, 0, SEEK_END);
	if ( rel < 0 )
	{					
		fclose(fp_header);
		fclose(fp_data);
		printf("  fseek error!\n");
		return FILEERROR;
	}					

	fileLength = ftell(fp_header);

	if( fileLength < 2000 )
	{
		DPRINTK("22 fileName %s size = %d error!\n",fileName,fileLength);

		fclose(fp_header);
		fclose(fp_data);
		return FILE_SIZE_ZERO_ERROR;
	}
	
	
	rel = fseek(fp_header,  0 , SEEK_SET );
	if( rel < 0 )
	{
		printf("fseek error\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILEERROR;
	}

	rel = fread(&stFileInfo,1, sizeof(FILEINFO), fp_header );
	if( rel != sizeof(FILEINFO) )
	{
		printf(" fread error 395\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILEERROR;
	}

	// 得到帧头的偏移位置
	ulframeoffset = stFileInfo.ulOffSet;

	rel = fseek(fp_header,  ulframeoffset , SEEK_SET );
	if( rel < 0 )
	{
		printf("fseek error\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILEERROR;
	}

	// 检测出现异常录相的第一帧
	rel = fread(&stFrameInfo,1, sizeof(FRAMEHEADERINFO), fp_header );
	if( rel != sizeof(FRAMEHEADERINFO) )
	{
		printf(" fread error 397\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILESEARCHERROR;
	}

	if( stFrameInfo.type != 'V'  && stFrameInfo.ulTimeSec != 0 && stFrameInfo.ulFrameType != 1 )
	{
		printf("error 442\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILESEARCHERROR;
	}

	printf("fix start channel =%d  time=%ld usec=%ld\n",stFrameInfo.iChannel,stFrameInfo.ulTimeSec,stFrameInfo.ulTimeUsec);

	Channel = stFrameInfo.iChannel;

	lasttime =  stFrameInfo.ulTimeSec;
	
	ullastoffst = ulframeoffset;	
	
	ulframeoffset += sizeof(FRAMEHEADERINFO);

	for( rel = 0; rel < MAX_LOST_FRAME ; rel++)
	{
		last_time_array[rel] = stFrameInfo.ulTimeSec;
		last_offset_array[rel] = ulframeoffset;
	}

	printf("======FIX FILE=======2\n");
	

	// 顺序检测每一帧，若两帧的时差大于一秒，则表示文件结束。
	while(1)
	{
		// 文件读完了，（一般不会有这种情况）
		rel = fread(&stFrameInfo,1, sizeof(FRAMEHEADERINFO), fp_header );
		if( rel != sizeof(FRAMEHEADERINFO) )
		{
			printf("file read over!\n");
			goto FIXOVER;
		}		

	/*	rel = fseek(fp_data,  stFrameInfo.ulFrameDataOffset, SEEK_SET );
		if( rel < 0 )
		{
			printf("==============data fseek error===========\n");			
			goto FIXOVER;
		} */

		// 两帧的时差大于一秒
		value1 = stFrameInfo.ulTimeSec - lasttime;
		
		if(value1 > 3  || value1 < (-3)  )
		{
			printf("Fix Last frame time = %ld,BadFrame time = %ld ullastoffst=%ld  value1=%d\n",
				lasttime,stFrameInfo.ulTimeSec,ullastoffst,value1);	
		
			goto FIXOVER;
		}

		if( stFrameInfo.ulFrameDataOffset > datafileOffset )
		{
			printf("ulFrameDataOffset=%ld datafileOffset=%ld\n",stFrameInfo.ulFrameDataOffset ,datafileOffset );
			goto FIXOVER;
		}

		save_frame_num++;		
	
		// 将第一个通道的上一个帧的位置保存
		// 查到出错地方时，这个位置就将成为结束位置。
		if( stFrameInfo.iChannel == Channel )
		{
		//	lost_four_group_data++;

			//if( lost_four_group_data >= MAX_LOST_FRAME )
			{
				ullastoffst = ulframeoffset;
				lasttime = stFrameInfo.ulTimeSec ;
				//lost_four_group_data = 0;
			}

			for( rel = 0; rel < (MAX_LOST_FRAME -1); rel++)
			{
				last_time_array[rel] = last_time_array[rel+1];
				last_offset_array[rel] = last_offset_array[rel+1];
			}

			last_time_array[MAX_LOST_FRAME-1] = lasttime;
			last_offset_array[MAX_LOST_FRAME-1] = ullastoffst;			
		}		
		
		ulframeoffset += sizeof(FRAMEHEADERINFO);



	}
	
FIXOVER:	
	lasttime = last_time_array[0];
	ullastoffst = last_offset_array[0];

	printf("save_frame_num = %d sec=%ld usec=%ld chan=%d ullastoffst=%ld\n",save_frame_num,
		stFrameInfo.ulTimeSec,stFrameInfo.ulTimeUsec,stFrameInfo.iChannel,ullastoffst);
	
	*time = lasttime;
	*ulOffset = ullastoffst;
	// 修改录相文件中的fileinfo结构
	stFileInfo.EndTime = lasttime;
	stFileInfo.states	= WRITEOVER;
	stFileInfo.ulOffSet  = ullastoffst;
	
	rel = fseek(fp_header,  0 , SEEK_SET );
	if( rel < 0 )
	{
		printf("fseek error\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILEERROR;
	}

	rel = fwrite( &stFileInfo, 1, sizeof( FILEINFO ), fp_header);
	if( rel != sizeof( FILEINFO ) )
	{
		printf(" fwrite error 556!\n");
		fclose(fp_header);
		fclose(fp_data);
		return FILEERROR;
	}
	
	fclose(fp_header);
	fclose(fp_data);
	return ALLRIGHT;

	#endif
}



