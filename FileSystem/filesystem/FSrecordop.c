#include "filesystem.h"
#include "FTC_common.h"

RECORDLOGPOINT pRecordLogPoint;
RECADDLOGPOINT pRecAddLogPoint;
RECTIMESTICKLOGPOINT pRecTimeStickLogPoint;
pthread_mutex_t rec_time_stick_mutex;

int FS_InitRecordLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pRecordLogPoint .fileBuf  = NULL;

	
	pRecordLogPoint .fileBuf = (char *)malloc(sizeof(MESSAGEUSEINFO) + 
				    MAXMESSAGENUMBER * sizeof(RECORDMESSAGE));

	if(pRecordLogPoint.fileBuf  == NULL )
		return NOENOUGHMEMORY;
	
	iFirst = 0;
		
	return ALLRIGHT;


}

int FS_InitRecAddLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pRecAddLogPoint .fileBuf  = NULL;

	
	pRecAddLogPoint .fileBuf = (char *)malloc(sizeof(MESSAGEUSEINFO) + 
				    MAXADDMESSAGENUMBER* sizeof(RECADDMESSAGE));

	if(pRecAddLogPoint.fileBuf  == NULL )
		return NOENOUGHMEMORY;
	
	iFirst = 0;
		
	return ALLRIGHT;


}


int FS_InitRecTimeStickLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pRecTimeStickLogPoint .fileBuf  = NULL;

	
	pRecTimeStickLogPoint .fileBuf = (char *)malloc(sizeof(MESSAGEUSEINFO) + 
				    MAXTIMESTICKMESSAGENUMBER* sizeof(RECTIMESTICKMESSAGE));

	if(pRecTimeStickLogPoint.fileBuf  == NULL )
		return NOENOUGHMEMORY;
	
	iFirst = 0;

	pthread_mutex_init(&rec_time_stick_mutex,NULL);
		
	return ALLRIGHT;


}

int FS_ReleaseRecordLogBuf()
{
	if( pRecordLogPoint.fileBuf )
	{
		free(pRecordLogPoint.fileBuf );
		pRecordLogPoint.fileBuf  = NULL;
	}
	
	return ALLRIGHT;
}

int FS_ReleaseRecAddLogBuf()
{
	if( pRecAddLogPoint.fileBuf )
	{
		free(pRecAddLogPoint.fileBuf );
		pRecAddLogPoint.fileBuf  = NULL;
	}
	
	return ALLRIGHT;
}

int FS_ReleaseRecTimeStickLogBuf()
{
	if( pRecTimeStickLogPoint.fileBuf )
	{
		free(pRecTimeStickLogPoint.fileBuf );
		pRecTimeStickLogPoint.fileBuf  = NULL;

		pthread_mutex_destroy( &rec_time_stick_mutex);
	}
	
	return ALLRIGHT;
}




int FS_LoadRecordLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pRecordLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/record.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open record.log \n");
			return FILEERROR;
	}

	rel =  fread( pRecordLogPoint.fileBuf, 1, sizeof(MESSAGEUSEINFO) + 
				    MAXMESSAGENUMBER * sizeof(RECORDMESSAGE), fp);

	if( rel != sizeof(MESSAGEUSEINFO) +  MAXMESSAGENUMBER * sizeof(RECORDMESSAGE))
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	pRecordLogPoint.pUseinfo = (MESSAGEUSEINFO * )pRecordLogPoint.fileBuf;
	pRecordLogPoint.id =  pRecordLogPoint.pUseinfo->id;
	pRecordLogPoint.pFilePos  = (RECORDMESSAGE  *)(pRecordLogPoint.fileBuf + sizeof(MESSAGEUSEINFO) +
						pRecordLogPoint.id * sizeof(RECORDMESSAGE));

	pRecordLogPoint.iDiskId = iDiskId;
	pRecordLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}

int FS_LoadRecAddLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pRecAddLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordadd.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open recordadd.log \n");
			return FILEERROR;
	}

	rel =  fread( pRecAddLogPoint.fileBuf, 1, sizeof(MESSAGEUSEINFO) + 
				    MAXADDMESSAGENUMBER* sizeof(RECADDMESSAGE), fp);

	if( rel != sizeof(MESSAGEUSEINFO) +  MAXADDMESSAGENUMBER * sizeof(RECADDMESSAGE))
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	pRecAddLogPoint.pUseinfo = (MESSAGEUSEINFO * )pRecAddLogPoint.fileBuf;
	pRecAddLogPoint.id =  pRecAddLogPoint.pUseinfo->id;
	pRecAddLogPoint.pFilePos  = (RECADDMESSAGE  *)(pRecAddLogPoint.fileBuf + sizeof(MESSAGEUSEINFO) +
						pRecAddLogPoint.id * sizeof(RECADDMESSAGE));

	pRecAddLogPoint.iDiskId = iDiskId;
	pRecAddLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}

int FS_LoadRecTimeStickLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pRecTimeStickLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordtime.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open recordtime.log path=%s\n",FileName);
			return FILEERROR;
	}

	rel =  fread( pRecTimeStickLogPoint.fileBuf, 1, sizeof(MESSAGEUSEINFO) + 
				    MAXTIMESTICKMESSAGENUMBER* sizeof(RECTIMESTICKMESSAGE), fp);

	if( rel != sizeof(MESSAGEUSEINFO) +  MAXTIMESTICKMESSAGENUMBER * sizeof(RECTIMESTICKMESSAGE))
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	pRecTimeStickLogPoint.pUseinfo = (MESSAGEUSEINFO * )pRecTimeStickLogPoint.fileBuf;
	pRecTimeStickLogPoint.id =  pRecTimeStickLogPoint.pUseinfo->id;
	pRecTimeStickLogPoint.pFilePos  = (RECTIMESTICKMESSAGE  *)(pRecTimeStickLogPoint.fileBuf + sizeof(MESSAGEUSEINFO) +
						pRecTimeStickLogPoint.id * sizeof(RECTIMESTICKMESSAGE));

	pRecTimeStickLogPoint.iDiskId = iDiskId;
	pRecTimeStickLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}




RECORDLOGPOINT * FS_GetRecordLogPoint()
{
	if( pRecordLogPoint.fileBuf  != NULL )
		return &pRecordLogPoint;

	return NULL;
}

RECADDLOGPOINT * FS_GetRecAddLogPoint()
{
	if( pRecAddLogPoint.fileBuf  != NULL )
		return &pRecAddLogPoint;

	return NULL;
}

RECTIMESTICKLOGPOINT * FS_GetRecTimeStickLogPoint()
{
	if( pRecTimeStickLogPoint.fileBuf  != NULL )
		return &pRecTimeStickLogPoint;

	return NULL;
}


RECORDLOGPOINT *  FS_GetPosRecordMessage(int iDiskId, int iPatitionId, int iFilePos)
{
	int rel;

	//printf("1  %x  %d %d", pRecordLogPoint .fileBuf,iDiskId,iPatitionId);

	if( pRecordLogPoint .fileBuf  == NULL )
		return NULL;
	
	if( iDiskId < 0 || iPatitionId < 5 )
		return NULL;	

	if( iDiskId != pRecordLogPoint.iDiskId || iPatitionId != pRecordLogPoint.iPatitionId  || g_CurrentFormatReclistReload == 1)
	{
		rel = FS_LoadRecordLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecordLog error!\n");
			return NULL;
		}

		if( g_CurrentFormatReclistReload == 1 )
			printf("format later reclist reload!\n");

		g_CurrentFormatReclistReload = 0;

	}

	if( iFilePos >=  pRecordLogPoint.pUseinfo->totalnumber||
		iFilePos < 0 )
	{
		printf(" e %d %d %d %d\n",iFilePos,pRecordLogPoint.pUseinfo->totalnumber,
			pRecordLogPoint.iDiskId,pRecordLogPoint.iPatitionId);
		return NULL;
	}	

	pRecordLogPoint.pFilePos =  (RECORDMESSAGE  *)(pRecordLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + iFilePos * sizeof(RECORDMESSAGE));

	pRecordLogPoint.id = iFilePos;

	return &pRecordLogPoint;
	
}

RECADDLOGPOINT *  FS_GetPosRecAddMessage(int iDiskId, int iPatitionId, int iFilePos)
{
	int rel;

	if( pRecAddLogPoint .fileBuf  == NULL )
		return NULL;
	
	if( iDiskId < 0 || iPatitionId < 5 )
		return NULL;

	if( iDiskId != pRecAddLogPoint.iDiskId || iPatitionId != pRecAddLogPoint.iPatitionId  || g_CurrentFormatRecAddlistReload == 1)
	{
		rel = FS_LoadRecAddLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecordLog error!\n");
			return NULL;
		}

		if( g_CurrentFormatRecAddlistReload == 1 )
			printf("format later rec add list reload!\n");

		g_CurrentFormatRecAddlistReload = 0;

	}

	if( iFilePos >=  pRecAddLogPoint.pUseinfo->totalnumber||
		iFilePos < 0 )
		return NULL;

	pRecAddLogPoint.pFilePos =  (RECADDMESSAGE  *)(pRecAddLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + iFilePos * sizeof(RECADDMESSAGE));

	pRecAddLogPoint.id = iFilePos;

	return &pRecAddLogPoint;
	
}


RECTIMESTICKLOGPOINT *  FS_GetPosRecTimeStickMessage_lock(int iDiskId, int iPatitionId, int iFilePos,RECTIMESTICKLOGPOINT * input_point)
{
	int rel;

	if( pRecTimeStickLogPoint.fileBuf  == NULL )
	{
		DPRINTK("error 1");
		return NULL;
	}
	
	if( iDiskId < 0 || iPatitionId < 5 )
	{
		DPRINTK("error 2 (%d,%d)\n",iDiskId,iPatitionId);
		return NULL;
	}

	if( iDiskId != pRecTimeStickLogPoint.iDiskId || iPatitionId != pRecTimeStickLogPoint.iPatitionId  || g_CurrentFormatRecTimeSticklistReload == 1)
	{
		rel = FS_LoadRecTimeStickLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecTimeStickLog error!\n");
			return NULL;
		}

		if( g_CurrentFormatRecTimeSticklistReload == 1 )
			printf("format later rec add list reload!\n");

		g_CurrentFormatRecTimeSticklistReload = 0;

	}

	if( iFilePos >=  pRecTimeStickLogPoint.pUseinfo->totalnumber||
		iFilePos < 0 )
	{
		DPRINTK("error 3");
		return NULL;
	}
	
	pRecTimeStickLogPoint.pFilePos =  (RECTIMESTICKMESSAGE  *)(pRecTimeStickLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + iFilePos * sizeof(RECTIMESTICKMESSAGE));

	input_point->file_info = *(pRecTimeStickLogPoint.pFilePos);
	input_point->use_info = *(pRecTimeStickLogPoint.pUseinfo);

	pRecTimeStickLogPoint.id = iFilePos;


	input_point->fileBuf = pRecTimeStickLogPoint.fileBuf;
	input_point->id = pRecTimeStickLogPoint.id;
	input_point->iDiskId = pRecTimeStickLogPoint.iDiskId;
	input_point->iPatitionId = pRecTimeStickLogPoint.iPatitionId;
	input_point->pFilePos = &input_point->file_info;
	input_point->pUseinfo = &input_point->use_info;	

	return input_point;
	
}


RECTIMESTICKLOGPOINT *  FS_GetPosRecTimeStickMessage(int iDiskId, int iPatitionId, int iFilePos,RECTIMESTICKLOGPOINT * input_point)
{
	RECTIMESTICKLOGPOINT * point = NULL;

	pthread_mutex_lock( &rec_time_stick_mutex);

	point = FS_GetPosRecTimeStickMessage_lock(iDiskId,iPatitionId,iFilePos,input_point);

	pthread_mutex_unlock( &rec_time_stick_mutex);

	
	return point;	
}

int RecordMessageUseInfoWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(pRecordLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/record.log", &stDiskInfo.devname[5],pRecordLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open record.log \n");
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(pRecordLogPoint.pUseinfo,1, sizeof( MESSAGEUSEINFO ),fp);

	if( rel != sizeof( MESSAGEUSEINFO ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}

int RecAddMessageUseInfoWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(pRecAddLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordadd.log", &stDiskInfo.devname[5],pRecAddLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open record.log \n");
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(pRecAddLogPoint.pUseinfo,1, sizeof( MESSAGEUSEINFO ),fp);

	if( rel != sizeof( MESSAGEUSEINFO ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}

int RecTimeStickMessageUseInfoWriteToFile_lock(RECTIMESTICKLOGPOINT * input_point)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(input_point->iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordtime.log", &stDiskInfo.devname[5],input_point->iPatitionId); // maybe devName is /tddvr/hda5

	if( input_point->iDiskId != pRecTimeStickLogPoint.iDiskId || input_point->iPatitionId != pRecTimeStickLogPoint.iPatitionId )
	{
		rel = FS_LoadRecTimeStickLog( input_point->iDiskId , input_point->iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecTimeStickLog error!iDiskId =%d,iPatitionId=%d\n",input_point->iDiskId,input_point->iPatitionId);			
			return ERROR;
		}		

	}	

	*(pRecTimeStickLogPoint.pUseinfo ) =input_point->use_info;

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			DPRINTK(" Can't Open recordtime.log path=%s\n",FileName);
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(input_point->pUseinfo,1, sizeof( MESSAGEUSEINFO ),fp);

	if( rel != sizeof( MESSAGEUSEINFO ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}


int RecTimeStickMessageUseInfoWriteToFile(RECTIMESTICKLOGPOINT * input_point)
{
	int rel;

	pthread_mutex_lock( &rec_time_stick_mutex);

	rel = RecTimeStickMessageUseInfoWriteToFile_lock(input_point);

	pthread_mutex_unlock( &rec_time_stick_mutex);

	return rel;
}



int RecordMessageWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(pRecordLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/record.log", &stDiskInfo.devname[5],pRecordLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open record.log \n");
			return FILEERROR;
	}

	offset = sizeof( MESSAGEUSEINFO ) + pRecordLogPoint.id * sizeof(RECORDMESSAGE);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(pRecordLogPoint.pFilePos , 1, sizeof( RECORDMESSAGE ),fp);

	if( rel != sizeof( RECORDMESSAGE ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}


int RecAddMessageWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(pRecAddLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordadd.log", &stDiskInfo.devname[5],pRecAddLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open recordadd.log \n");
			return FILEERROR;
	}

	offset = sizeof( MESSAGEUSEINFO ) + pRecAddLogPoint.id * sizeof(RECADDMESSAGE);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(pRecAddLogPoint.pFilePos , 1, sizeof( RECADDMESSAGE ),fp);

	if( rel != sizeof( RECADDMESSAGE ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;

}

int RecTimeStickMessageWriteToFile_lock(RECTIMESTICKLOGPOINT * input_point)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(input_point->iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/recordtime.log", &stDiskInfo.devname[5],input_point->iPatitionId); // maybe devName is /tddvr/hda5

	if( input_point->iDiskId != pRecTimeStickLogPoint.iDiskId || input_point->iPatitionId != pRecTimeStickLogPoint.iPatitionId )
	{
		rel = FS_LoadRecTimeStickLog( input_point->iDiskId , input_point->iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecTimeStickLog error!iDiskId =%d,iPatitionId=%d\n",input_point->iDiskId,input_point->iPatitionId);			
			return ERROR;
		}		

	}	

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open recordtime.log path=%s\n",FileName);
			return FILEERROR;
	}

	pRecTimeStickLogPoint.pFilePos =  (RECTIMESTICKMESSAGE  *)(pRecTimeStickLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + input_point->id* sizeof(RECTIMESTICKMESSAGE));

	*(pRecTimeStickLogPoint.pFilePos) = input_point->file_info;

	offset = sizeof( MESSAGEUSEINFO ) + input_point->id * sizeof(RECTIMESTICKMESSAGE);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(input_point->pFilePos , 1, sizeof( RECTIMESTICKMESSAGE ),fp);

	if( rel != sizeof( RECTIMESTICKMESSAGE ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;

}

int RecTimeStickMessageWriteToFile(RECTIMESTICKLOGPOINT * input_point)
{
	int rel;

	pthread_mutex_lock( &rec_time_stick_mutex);

	rel = RecTimeStickMessageWriteToFile_lock(input_point);

	pthread_mutex_unlock( &rec_time_stick_mutex);

	return rel;

}

int FS_WriteStartInfoToRecLog(int iDiskId,int iPartitionId,
			time_t recStartTime,int iFilePos,int iCam,unsigned long ulOffSet)
{
	RECORDLOGPOINT * pRecPoint;

	int iCurrentId,rel;
	
	pRecPoint = FS_GetPosRecordMessage(iDiskId,iPartitionId,0);

	// 得到当前message 
	iCurrentId = pRecPoint->pUseinfo->id ;

	pRecPoint = FS_GetPosRecordMessage(iDiskId,iPartitionId,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecordMessage error!\n");
		return FILEERROR;
	}

	pRecPoint->pFilePos->filePos = iFilePos;
	pRecPoint->pFilePos->iCam   = iCam;
	pRecPoint->pFilePos->startTime = recStartTime;
	pRecPoint->pFilePos->ulOffSet = ulOffSet;
	pRecPoint->pFilePos->endTime = 0;
	pRecPoint->pFilePos->videoStandard = g_RecMode.iPbVideoStandard;
	pRecPoint->pFilePos->imageSize   =g_RecMode.iPbImageSize;
	pRecPoint->pFilePos->iDataEffect = NOEFFECT;
	pRecPoint->pFilePos->rec_size = 0;

	rel = RecordMessageWriteToFile();
	if( rel < 0 )
	{
		printf("RecordMessageWriteToFile error!\n");
		return rel;
	}

	fs_set_rec_size(0);

	printf("rec_file_size = %ld\n",fs_get_rec_size());
	printf("recLog id %d   starttime=%ld  endtime=%ld filepos=%d offset=%ld mode=%d image=%d effect=%d\n",
		pRecPoint->pFilePos->id,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,
		pRecPoint->pFilePos->filePos,pRecPoint->pFilePos->ulOffSet,pRecPoint->pFilePos->videoStandard,
		pRecPoint->pFilePos->imageSize,pRecPoint->pFilePos->iDataEffect);

	return ALLRIGHT;
		
}

int FS_ERR_WriteEndInfotoRecLog()
{
	struct timeval stTimeVal;

	int rel;

	g_pstCommonParam->GST_DRA_get_sys_time( &stTimeVal, NULL );

	rel = FS_WriteEndInfotoRecLog(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,stTimeVal.tv_sec);
	if(rel < 0 )
	{
		printf("FS_ERR_WriteEndInfotoRecLog error!\n");
		return rel;
	}
	printf(" FS_ERR_WriteEndInfotoRecLog over %ld  \n",stTimeVal.tv_sec);

	return ALLRIGHT;
}

int FS_WriteEndInfotoRecLog(int iDiskId,int iPartitionId,time_t recEndTime)
{
	RECORDLOGPOINT * pRecPoint;
	int iCurrentId,rel;
		
	pRecPoint = FS_GetPosRecordMessage(iDiskId,iPartitionId,0);

	// 得到当前message 
	iCurrentId = pRecPoint->pUseinfo->id ;

	pRecPoint = FS_GetPosRecordMessage(iDiskId,iPartitionId,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecordMessage error!\n");
		return FILEERROR;
	}

	pRecPoint->pFilePos->endTime = recEndTime;
	pRecPoint->pFilePos->iDataEffect = EFFECT;
	
	if( recEndTime == 0 )
		pRecPoint->pFilePos->iDataEffect = NOEFFECT;

	if( fs_get_rec_size() != 0 )
	pRecPoint->pFilePos->rec_size = fs_get_rec_size();

	rel = RecordMessageWriteToFile();
	if( rel < 0 )
	{
		printf("RecordMessageWriteToFile error!\n");
		return rel;
	}

	DPRINTK("recLog id %d   starttime=%ld  endtime=%ld filepos=%d offset=%ld mode=%d image=%d effect=%d\n",
		pRecPoint->pUseinfo->id,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,
		pRecPoint->pFilePos->filePos,pRecPoint->pFilePos->ulOffSet,pRecPoint->pFilePos->videoStandard,
		pRecPoint->pFilePos->imageSize,pRecPoint->pFilePos->iDataEffect);

	if( ( pRecPoint->pUseinfo->id + 1)  <  pRecPoint->pUseinfo->totalnumber )
		pRecPoint->pUseinfo->id = pRecPoint->pUseinfo->id + 1;
	else
		pRecPoint->pUseinfo->id = 0;

	rel = RecordMessageUseInfoWriteToFile();
	if( rel < 0 )
	{
		printf("RecordMessageUseInfoWriteToFile error!\n");
		return rel;
	}	
	printf("recLog id change  %d   \n",pRecPoint->pUseinfo->id);
	

	return ALLRIGHT;
}


int FS_WriteInfotoRecAddLog(time_t recStartTime,int iCam)
{
	RECADDLOGPOINT * pRecPoint;
	RECORDLOGPOINT * pRecPoint1;
	int iCurrentId,rel;
	int iDiskId, iPartitionId;

	iDiskId = g_stLogPos.iDiskId;
	iPartitionId = g_stLogPos.iPartitionId;
		
	pRecPoint = FS_GetPosRecAddMessage(iDiskId,iPartitionId,0);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecAddMessage error!\n");
		return FILEERROR;
	}


	// 得到当前message 
	iCurrentId = pRecPoint->pUseinfo->id ;

	pRecPoint = FS_GetPosRecAddMessage(iDiskId,iPartitionId,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecAddMessage error!\n");
		return FILEERROR;
	}	


	pRecPoint->pFilePos->startTime = recStartTime;	
	pRecPoint->pFilePos->iCam = iCam;
	pRecPoint->pFilePos->rec_log_id = FS_GetRecMsgCurrentId();	
	pRecPoint->pFilePos->filePos = g_stRecFilePos.iCurFilePos;

	pRecPoint1 = FS_GetPosRecordMessage(iDiskId,iPartitionId,pRecPoint->pFilePos->rec_log_id);

	if( pRecPoint1 != NULL )
	{
		pRecPoint->pFilePos->rec_log_time = pRecPoint1->pFilePos->startTime;
	}


	if( fs_get_rec_size() != 0 )
	pRecPoint->pFilePos->rec_size = fs_get_rec_size();

	rel = RecAddMessageWriteToFile();
	if( rel < 0 )
	{
		printf("RecAddMessageWriteToFile error!\n");
		return rel;
	}	

	printf("rec add Log id %d   starttime=%ld  rec_log_id=%d iCam=%d rec_size=%ld filepos=%d\n",
		pRecPoint->pUseinfo->id,pRecPoint->pFilePos->startTime,
		pRecPoint->pFilePos->rec_log_id,
		pRecPoint->pFilePos->iCam,pRecPoint->pFilePos->rec_size,pRecPoint->pFilePos->filePos );

	if( ( pRecPoint->pUseinfo->id + 1)  <  pRecPoint->pUseinfo->totalnumber )
		pRecPoint->pUseinfo->id = pRecPoint->pUseinfo->id + 1;
	else
		pRecPoint->pUseinfo->id = 0;

	rel = RecAddMessageUseInfoWriteToFile();
	if( rel < 0 )
	{
		printf("RecAddMessageUseInfoWriteToFile error!\n");
		return rel;
	}	
	printf("rec add Log id change  %d   \n",pRecPoint->pUseinfo->id);	

	pRecPoint1 = FS_GetPosRecordMessage(iDiskId,iPartitionId,pRecPoint->pFilePos->rec_log_id);

	if(pRecPoint1->pFilePos->have_add_info != 1 )
	{
		pRecPoint1->pFilePos->have_add_info = 1;
		RecordMessageWriteToFile();
	}

	return ALLRIGHT;
}


int FS_WriteInfotoRecTimeStickLog(time_t recStartTime,int iCam)
{
	RECTIMESTICKLOGPOINT * pRecPoint;
	RECTIMESTICKLOGPOINT RecPoint;
	int iCurrentId,rel;
	int iDiskId, iPartitionId;
	int old_cam = 0;

	iDiskId = g_stLogPos.iDiskId;
	iPartitionId = g_stLogPos.iPartitionId;

		
	pRecPoint = FS_GetPosRecTimeStickMessage(iDiskId,iPartitionId,0,&RecPoint);

	if( pRecPoint == NULL )
	{
		DPRINTK(" FS_GetPosRecTimeStickMessage error!\n");
		return FILEERROR;
	}


	// 得到当前message 
	iCurrentId = pRecPoint->pUseinfo->id ;

	//printf("iCurrentId = %d\n",iCurrentId);

	// 比较上个纪录点看看时间点是不是一样
	if( iCurrentId - 1 >= 0 )
	{
		pRecPoint =  FS_GetPosRecTimeStickMessage(iDiskId,iPartitionId,iCurrentId-1,&RecPoint);

		if( pRecPoint == NULL )
		{
			DPRINTK(" FS_GetPosRecTimeStickMessage error!\n");
			return FILEERROR;
		}	

		if( pRecPoint->pFilePos->startTime == recStartTime )
		{
			if( pRecPoint->pFilePos->iCam != iCam )
			{
				old_cam = pRecPoint->pFilePos->iCam;
				//DPRINTK(" pFilePos->iCam =%x icam=%x\n",pRecPoint->pFilePos->iCam,iCam);
				pRecPoint->pFilePos->iCam = iCam | pRecPoint->pFilePos->iCam;
				//DPRINTK(" rewrite TimeStick cam=%x\n",pRecPoint->pFilePos->iCam);

				if( old_cam != pRecPoint->pFilePos->iCam )
				{
					DPRINTK("real write old_cam=%x  new_cam=%x\n",old_cam,pRecPoint->pFilePos->iCam);
					rel = RecTimeStickMessageWriteToFile(pRecPoint);
					if( rel < 0 )
					{
						printf("RecTimeStickMessageWriteToFile error!\n");
						return rel;
					}	
				}
			}

			if( (pRecPoint->pFilePos->videoStandard != g_RecMode.iPbVideoStandard)||
				(pRecPoint->pFilePos->imageSize   != g_RecMode.iPbImageSize) )
			{
				pRecPoint->pFilePos->videoStandard = g_RecMode.iPbVideoStandard;
				pRecPoint->pFilePos->imageSize   =g_RecMode.iPbImageSize;

				DPRINTK("mode dif real write\n");
				rel = RecTimeStickMessageWriteToFile(pRecPoint);
				if( rel < 0 )
				{
					DPRINTK("RecTimeStickMessageWriteToFile error!\n");
					return rel;
				}
			}

			return ALLRIGHT;
		}
		
	}
	

	pRecPoint = FS_GetPosRecTimeStickMessage(iDiskId,iPartitionId,iCurrentId,&RecPoint);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosRecTimeStickMessage error!\n");
		return FILEERROR;
	}	


	pRecPoint->pFilePos->startTime = recStartTime;	
	pRecPoint->pFilePos->iCam = iCam;
	pRecPoint->pFilePos->videoStandard = g_RecMode.iPbVideoStandard;
	pRecPoint->pFilePos->imageSize   =g_RecMode.iPbImageSize;


	rel = RecTimeStickMessageWriteToFile(pRecPoint);
	if( rel < 0 )
	{
		printf("RecTimeStickMessageWriteToFile error!\n");
		return rel;
	}	

	printf("rec time stick Log id %d   starttime=%ld  iCam=%x \n",
		pRecPoint->pUseinfo->id,pRecPoint->pFilePos->startTime,		
		pRecPoint->pFilePos->iCam );

	if( ( pRecPoint->pUseinfo->id + 1)  <  pRecPoint->pUseinfo->totalnumber )
		pRecPoint->pUseinfo->id = pRecPoint->pUseinfo->id + 1;
	else
		pRecPoint->pUseinfo->id = 0;

	rel = RecTimeStickMessageUseInfoWriteToFile(pRecPoint);
	if( rel < 0 )
	{
		printf("RecTimeStickMessageUseInfoWriteToFile error!\n");
		return rel;
	}	
	printf("rec time stick Log id change  %d   \n",pRecPoint->pUseinfo->id);	

	#ifdef USE_PLAYBACK_QUICK_SYSTEM
	FTC_rec_write_log_change_disk_rec_time(0,iDiskId,iPartitionId);	
	#endif

	return ALLRIGHT;
}

int FS_GetRecMsgCurrentId()
{
	RECORDLOGPOINT * pRecPoint;
		
	pRecPoint = FS_GetPosRecordMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

	// 得到当前message 
	return  pRecPoint->pUseinfo->id ;
}

int FS_GetRecAddMsgCurrentId()
{
	RECADDLOGPOINT * pRecPoint;
		
	pRecPoint = FS_GetPosRecAddMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

	// 得到当前message 
	return  pRecPoint->pUseinfo->id ;
}

int FS_GetRecTimeStickMsgCurrentId()
{
	RECTIMESTICKLOGPOINT * pRecPoint;
	RECTIMESTICKLOGPOINT RecPoint;
		
	pRecPoint = FS_GetPosRecTimeStickMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0,&RecPoint);

	// 得到当前message 
	return  pRecPoint->pUseinfo->id ;
}

int FS_GetRecCurrentPoint(FILEINFOPOS * filePos)
{
	RECORDLOGPOINT * pRecPoint;

	printf("log  iDiskId = %d iPartitionId=%d\n",g_stLogPos.iDiskId,g_stLogPos.iPartitionId);
		
	pRecPoint = FS_GetPosRecordMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

	if( pRecPoint == NULL )
		return ERROR;

	if( pRecPoint->pUseinfo->id > 0 )
		filePos->iCurFilePos =  pRecPoint->pUseinfo->id -1 ;
	else
		filePos->iCurFilePos = pRecPoint->pUseinfo->totalnumber -1;
	
	filePos->iDiskId = g_stLogPos.iDiskId;
	filePos->iPartitionId = g_stLogPos.iPartitionId;

	return ALLRIGHT;
}
 

