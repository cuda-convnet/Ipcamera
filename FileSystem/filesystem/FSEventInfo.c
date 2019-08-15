#include "filesystem.h"
#include "FTC_common.h"

EVENTLOGPOINT pEventLogPoint;

int FS_InitEventLogBuf()
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;

	pEventLogPoint .fileBuf  = NULL;

	
	pEventLogPoint .fileBuf = (char *)malloc(sizeof(MESSAGEUSEINFO) + 
				    EVENTMAXMESSAGE * sizeof(EVENTMESSAGE));

	if(pEventLogPoint.fileBuf  == NULL )
		return NOENOUGHMEMORY;
	
	iFirst = 0;
		
	return ALLRIGHT;


}

int FS_ReleaseEventLogBuf()
{
	if( pEventLogPoint.fileBuf )
	{
		free(pEventLogPoint.fileBuf );
		pEventLogPoint.fileBuf  = NULL;
	}
	
	return ALLRIGHT;
}


int FS_LoadEventLog( int iDiskId , int iPatitionId)
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	FILEUSEINFO stDiskUseInfo;
	int rel = 0;
		

	if( pEventLogPoint.fileBuf == NULL )
		return NOENOUGHMEMORY;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/event.log", &stDiskInfo.devname[5],iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "rb" );
	if ( !fp)
	{
			printf(" Can't Open event.log \n");
			return FILEERROR;
	}

	rel =  fread( pEventLogPoint.fileBuf, 1, sizeof(MESSAGEUSEINFO) + 
				    EVENTMAXMESSAGE * sizeof(EVENTMESSAGE), fp);

	if( rel != sizeof(MESSAGEUSEINFO) +  EVENTMAXMESSAGE * sizeof(EVENTMESSAGE))
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	pEventLogPoint.pUseinfo = (MESSAGEUSEINFO * )pEventLogPoint.fileBuf;
	pEventLogPoint.id =  pEventLogPoint.pUseinfo->id;
	pEventLogPoint.pFilePos  = (EVENTMESSAGE  *)(pEventLogPoint.fileBuf + sizeof(MESSAGEUSEINFO) +
						pEventLogPoint.id * sizeof(EVENTMESSAGE));

	pEventLogPoint.iDiskId = iDiskId;
	pEventLogPoint.iPatitionId = iPatitionId;
	
	return ALLRIGHT;
	
}

EVENTLOGPOINT * FS_GetEventLogPoint()
{
	if( pEventLogPoint.fileBuf  != NULL )
		return &pEventLogPoint;

	return NULL;
}

EVENTLOGPOINT *  FS_GetPosEventMessage(int iDiskId, int iPatitionId, int iFilePos)
{
	int rel;

	if( pEventLogPoint .fileBuf  == NULL )
		return NULL;
	
	if( iDiskId < 0 || iPatitionId < 5 )
		return NULL;

	if( iDiskId != pEventLogPoint.iDiskId || iPatitionId != pEventLogPoint.iPatitionId  || g_CurrentFormatEventlistReload == 1)
	{
		rel = FS_LoadEventLog( iDiskId , iPatitionId);

		if( rel != ALLRIGHT )
		{
			printf(" FS_LoadEventLog error!\n");
			return NULL;
		}

		if( g_CurrentFormatEventlistReload == 1 )
			printf("format later eventlist reload!\n");

		g_CurrentFormatEventlistReload = 0;

	}

	if( iFilePos >=  pEventLogPoint.pUseinfo->totalnumber||
		iFilePos < 0 )
		return NULL;

	pEventLogPoint.pFilePos =  (EVENTMESSAGE  *)(pEventLogPoint.fileBuf +
					sizeof(MESSAGEUSEINFO) + iFilePos * sizeof(EVENTMESSAGE));

	pEventLogPoint.id = iFilePos;

	return &pEventLogPoint;
	
}



int EventMessageUseInfoWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(pEventLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/event.log", &stDiskInfo.devname[5],pEventLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open event.log \n");
			return FILEERROR;
	}

	fseek( fp , 0 , SEEK_SET );

	rel = fwrite(pEventLogPoint.pUseinfo,1, sizeof( MESSAGEUSEINFO ),fp);

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

int EventMessageWriteToFile()
{
	FILE *fp = NULL;
	char FileName[50];
	GST_DISKINFO stDiskInfo;
	int rel;
	long offset = 0;

	stDiskInfo = DISK_GetDiskInfo(pEventLogPoint.iDiskId);

	memset(FileName, 0, 50);

	sprintf(FileName,"/tddvr/%s%d/event.log", &stDiskInfo.devname[5],pEventLogPoint.iPatitionId); // maybe devName is /tddvr/hda5

	fp = fopen( FileName, "r+b" );
	if ( !fp)
	{
			printf(" Can't Open event.log \n");
			return FILEERROR;
	}

	offset = sizeof( MESSAGEUSEINFO ) + pEventLogPoint.id * sizeof(EVENTMESSAGE);

	fseek( fp , offset , SEEK_SET );

	rel = fwrite(pEventLogPoint.pFilePos , 1, sizeof( EVENTMESSAGE ),fp);

	if( rel != sizeof( EVENTMESSAGE ) )
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
int FS_WriteInfoToEventLog(int * iCam,time_t * eventStartTime,GST_RECORDEVENT * pEvent,
									EVENTSTATE EventStates,char * Reason )
{
	EVENTLOGPOINT * pRecPoint;

	int iCurrentId,rel;

	struct timeval videotv;

	g_pstCommonParam->GST_DRA_get_sys_time( &videotv, NULL );

	printf(" curtime = %ld\n",videotv.tv_sec);

	if( pEvent != NULL )
	{
		printf("event %d%d%d starttime = %ld, icam=%d\n",
			pEvent->motion,pEvent->loss,pEvent->sensor,
			*eventStartTime ,*iCam  );
	}

	printf(" iDiskId = %d, iPartitionId = %d \n",0,5);

	pRecPoint = FS_GetPosEventMessage(0,5,0);

	if( pRecPoint == NULL )
	{
		printf("FS_GetPosEventMessage error!\n");
		return FILEERROR;
	}

	iCurrentId = pRecPoint->pUseinfo->id; // 得到当前message 

	printf("event iCurrentId = %d\n",iCurrentId);

	pRecPoint = FS_GetPosEventMessage(0,5,iCurrentId);

	if( pRecPoint == NULL )
	{
		printf(" FS_GetPosEventMessage error!\n");
		return FILEERROR;
	}

	if( pEvent != NULL )
		pRecPoint->pFilePos->event = *pEvent;

	if( eventStartTime == NULL )
		pRecPoint->pFilePos->EventStartTime  = videotv.tv_sec;
	else
		pRecPoint->pFilePos->EventStartTime = *eventStartTime;	

	if( Reason != NULL )
	{
		if(strlen(Reason) < 30 )
			strcpy(pRecPoint->pFilePos->EventInfo,Reason);
	}

	if( iCam != NULL )
		pRecPoint->pFilePos->iCam    = *iCam;
	
	pRecPoint->pFilePos->CurrentEvent = EventStates;
	pRecPoint->pFilePos->iDataEffect = EFFECT;	

	printf(" time=%ld  event=%d\n",pRecPoint->pFilePos->EventStartTime,pRecPoint->pFilePos->CurrentEvent);
	
	rel = EventMessageWriteToFile();
	if( rel < 0 )
	{
		printf("EventMessageWriteToFile error!\n");
		return rel;
	}

	
	if( ( pRecPoint->pUseinfo->id + 1)  <  pRecPoint->pUseinfo->totalnumber )
		pRecPoint->pUseinfo->id = pRecPoint->pUseinfo->id + 1;
	else
		pRecPoint->pUseinfo->id = 0;

	rel = EventMessageUseInfoWriteToFile();
	if( rel < 0 )
	{
		printf("EventMessageUseInfoWriteToFile error!\n");
		return rel;
	}		

	printf(" change id = %d\n",pRecPoint->pUseinfo->id);

	return iCurrentId;
	
}

int FS_GetEventCurrentPoint()
{
	EVENTLOGPOINT * pRecPoint;
	int lastId = 0;
		
	pRecPoint = FS_GetPosEventMessage(0, 5, 0);

	if( pRecPoint == NULL )
		return ERROR;

	if( pRecPoint->pUseinfo->id > 0 )
		lastId =  pRecPoint->pUseinfo->id -1 ;
	else
		lastId = pRecPoint->pUseinfo->totalnumber -1;
	
	return lastId;
}



