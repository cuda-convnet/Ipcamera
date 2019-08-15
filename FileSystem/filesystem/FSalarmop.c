#include "filesystem.h"
#include "FTC_common.h"


ALARMLOGPOINT pAlarmLogPoint;

int g_AlarmMessageId = -1;

int FS_InitAlarmLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pAlarmLogPoint .fileBuf  = NULL;

	
	pAlarmLogPoint .fileBuf = (char *)malloc(sizeof(MESSAGEUSEINFO) + 
				    MAXMESSAGENUMBER * sizeof(ALARMMESSAGE));

	if(pAlarmLogPoint.fileBuf  == NULL )
		return NOENOUGHMEMORY;
	
	iFirst = 0;
		
	return ALLRIGHT;


}

int FS_ReleaseAlarmLogBuf()
{
	if( pAlarmLogPoint.fileBuf )
	{
		free(pAlarmLogPoint.fileBuf );
		pAlarmLogPoint.fileBuf  = NULL;
	}
	
	return ALLRIGHT;
}



int FS_LoadAlarmLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pAlarmLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/alarm.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open alarm.log \n");
			return FILEERROR;
	}

	rel =  fread( pAlarmLogPoint.fileBuf, 1, sizeof(MESSAGEUSEINFO) + 
				    MAXMESSAGENUMBER * sizeof(ALARMMESSAGE), fp);

	if( rel != sizeof(MESSAGEUSEINFO) +  MAXMESSAGENUMBER * sizeof(ALARMMESSAGE))
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	pAlarmLogPoint.pUseinfo = (MESSAGEUSEINFO * )pAlarmLogPoint.fileBuf;
	pAlarmLogPoint.id =  pAlarmLogPoint.pUseinfo->id;
	pAlarmLogPoint.pFilePos  = (ALARMMESSAGE  *)(pAlarmLogPoint.fileBuf + sizeof(MESSAGEUSEINFO) +
						pAlarmLogPoint.id * sizeof(ALARMMESSAGE));

	pAlarmLogPoint.iDiskId = iDiskId;
	pAlarmLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}

ALARMLOGPOINT * FS_GetAlarmLogPoint()
{
	if( pAlarmLogPoint.fileBuf  != NULL )
		return &pAlarmLogPoint;

	return NULL;
}


ALARMLOGPOINT *  FS_GetPosAlarmMessage(int iDiskId, int iPatitionId, int iFilePos)
{
	int rel;

	if( pAlarmLogPoint .fileBuf  == NULL )
		return NULL;

	if( iDiskId < 0 || iPatitionId < 5 )
		return NULL;

	if( iDiskId != pAlarmLogPoint.iDiskId || iPatitionId != pAlarmLogPoint.iPatitionId 
		|| g_CurrentFormatAlarmlistReload == 1)
	{		
		rel = FS_LoadAlarmLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadRecordLog error!\n");
			return NULL;
		}

		if( g_CurrentFormatAlarmlistReload == 1 )
			printf(" format later Alarmlist  reload!\n");

		g_CurrentFormatAlarmlistReload = 0;
	}

	if( iFilePos >=  pAlarmLogPoint.pUseinfo->totalnumber||
		iFilePos < 0 )
		return NULL;

	pAlarmLogPoint.pFilePos =  (ALARMMESSAGE  *)(pAlarmLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + iFilePos * sizeof(ALARMMESSAGE));

	pAlarmLogPoint.id = iFilePos;

	return &pAlarmLogPoint;
	
}

int AlarmMessageUseInfoWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(pAlarmLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/alarm.log", &stDiskInfo.devname[5],pAlarmLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open alarm.log \n");
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(pAlarmLogPoint.pUseinfo,1, sizeof( MESSAGEUSEINFO ),fp);

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

int AlarmMessageWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(pAlarmLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/alarm.log", &stDiskInfo.devname[5],pAlarmLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open alarm.log \n");
			return FILEERROR;
	}

	offset = sizeof( MESSAGEUSEINFO ) + pAlarmLogPoint.id * sizeof(ALARMMESSAGE);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(pAlarmLogPoint.pFilePos , 1, sizeof( ALARMMESSAGE ),fp);

	if( rel != sizeof( ALARMMESSAGE ) )
	{
				printf(" write error!\n");
				fclose(fp);
				return ERROR;
	}

	fflush(fp);

	fclose(fp);

	return ALLRIGHT;
	

}


// 返回当前操作的message id
int FS_WriteInfoToAlarmLog(GST_RECORDEVENT event,time_t eventStartTime,time_t eventEndTime,int iCam,
									int   iPreviewTime,int   iLaterTime,int  states,int  iMessageId)
{
	ALARMLOGPOINT * pRecPoint;

	int iCurrentId,rel;

	struct timeval videotv;

	g_pstCommonParam->GST_DRA_get_sys_time( &videotv, NULL );

	printf(" curtime = %ld\n",videotv.tv_sec);

	printf("event %d%d%d starttime = %ld, icam=%d,endtime=%ld,stats = %d iMessageId=%d\n",
		event.motion,event.loss,event.sensor,
		eventStartTime ,
		iCam ,eventEndTime,states,iMessageId );

	printf(" iDiskId = %d, iPartitionId = %d\n",g_stLogPos.iDiskId,g_stLogPos.iPartitionId);

	pRecPoint = FS_GetPosAlarmMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

	if( pRecPoint == NULL )
	{
		printf("FS_GetPosAlarmMessage error!\n");
		return FILEERROR;
	}

	if(  iMessageId < -1 || iMessageId > pRecPoint->pUseinfo->totalnumber )
	{
		printf("error iMessageId = %d\n",iMessageId);
		return ERROR;
	}

	if( iMessageId == -1)	
		iCurrentId = pRecPoint->pUseinfo->id; // 得到当前message 
	else
		iCurrentId = iMessageId;  //指定的message

	printf("event iCurrentId = %d\n",iCurrentId);

	pRecPoint = FS_GetPosAlarmMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosAlarmMessage error!\n");
		return FILEERROR;
	}

	if( iMessageId != -1)
	{
		pRecPoint->pFilePos->alarmEndTime = videotv.tv_sec;
	}
	else
	{
		pRecPoint->pFilePos->event = event;
		pRecPoint->pFilePos->alarmStartTime = videotv.tv_sec;	
		pRecPoint->pFilePos->iCam    = iCam;
		pRecPoint->pFilePos->alarmEndTime = eventEndTime;
		pRecPoint->pFilePos->states = states;
		pRecPoint->pFilePos->iPreviewTime = iPreviewTime;
		pRecPoint->pFilePos->iLaterTime	     = iLaterTime;
		pRecPoint->pFilePos->iDataEffect	= EFFECT;
		pRecPoint->pFilePos->imageSize      = FTC_GetRecImageSize();
		pRecPoint->pFilePos->videoStandard = FTC_GetRecStandard();
	}	
	

	if( pRecPoint->pFilePos->states == NORECORD)
	{
		pRecPoint->pFilePos->iRecMessageId = -1;
	}
	else
	{
		pRecPoint->pFilePos->iRecMessageId = FS_GetRecMsgCurrentId();
	}

	rel = AlarmMessageWriteToFile();
	if( rel < 0 )
	{
		printf("AlarmMessageWriteToFile error!\n");
		return rel;
	}

// iMessageID 不为-1 表示重复写某条信息，id  不需加1
	if(  iMessageId != -1 )
		return iCurrentId;
	
	if( ( pRecPoint->pUseinfo->id + 1)  <  pRecPoint->pUseinfo->totalnumber )
		pRecPoint->pUseinfo->id = pRecPoint->pUseinfo->id + 1;
	else
		pRecPoint->pUseinfo->id = 0;

	rel = AlarmMessageUseInfoWriteToFile();
	if( rel < 0 )
	{
		printf("AlarmMessageUseInfoWriteToFile error!\n");
		return rel;
	}		

//	printf(" change id = %d\n",pRecPoint->pUseinfo->id);

	return iCurrentId;
	
}

int FS_WriteEndInfotoAlarmLog(int iMessageId,time_t alarmEndTime)
{
	ALARMLOGPOINT * pRecPoint;
	int iCurrentId,rel;
		
	pRecPoint = FS_GetPosAlarmMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,iMessageId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosAlarmMessage error!\n");
		return FILEERROR;
	}

	pRecPoint->pFilePos->alarmEndTime = alarmEndTime;

	rel = AlarmMessageWriteToFile();
	if( rel < 0 )
	{
		printf("AlarmMessageWriteToFile error!\n");
		return rel;
	}

	g_AlarmMessageId = -1;

	return ALLRIGHT;
}

int FS_GetAlarmCurrentPoint(FILEINFOPOS * filePos)
{
	ALARMLOGPOINT * pRecPoint;
		
	pRecPoint = FS_GetPosAlarmMessage(g_stLogPos.iDiskId,g_stLogPos.iPartitionId,0);

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



