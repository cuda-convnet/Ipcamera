#include "filesystem.h"
#include "FTC_common.h"
#include "saveplayctrl.h"

#define MAX_DISK_PARTION  (16*8)  // 8个硬盘,每个硬盘16个分区

int PlayStopStatus = 0;
int PlayStartTime = 0;
int PlayCamInfo = 0;
DISK_REC_TIME disk_record_time[MAX_DISK_PARTION];
pthread_mutex_t disk_record_time_mutex;



int ftc_mount(const char * src,const char * dst)
{
	int rel = 0;

	#ifdef USE_EXT3
		rel = mount(src,dst,"ext3",0,NULL);
		//rel = mount(src,dst,"ext3",0,"data=writeback");
	#else
		rel = mount(src,dst,"vfat",0,NULL);
	#endif

	return rel;
}

int ftc_get_dvr_cam_num()
{
	return g_pstCommonParam->GST_DRA_get_dvr_max_chan(1);
}



// 将用户时间转为time_t 时间 
time_t FTC_CustomTimeTotime_t(int year,int month, int day, int hour, int minute,int second)
{
	struct tm tp;
	time_t mtime;	

	tp.tm_year = year;
	tp.tm_mon = month;
	tp.tm_mday = day;
	tp.tm_hour  = hour;
	tp.tm_min   = minute;
	tp.tm_sec   = second;
	tp.tm_isdst = 0;

	mtime = mktime(&tp);

	//printf(" mtime = %ld\n",mtime);

	//printf("time is %d %d %d %d %d %d  %ld\n",year,month,day,hour,minute,second,mtime);

	return mtime;	
}

// 将time_t  转为用户时间
void  FTC_time_tToCustomTime(time_t time,int * year, int * month,int* day,int* hour,int* minute,int* second )
{	
	 struct tm tp;
	
	 gmtime_r(&time,&tp);

	 *year = tp.tm_year;
	 *month = tp.tm_mon;
	 *day = tp.tm_mday;
	 *hour = tp.tm_hour;
	 *minute = tp.tm_min;
	 *second = tp.tm_sec;		
}


// 传入time_t 格式，返回WeekDay (0-6) 星期日为0
int  FTC_GetWeekDay(time_t mTime)
{
	struct tm *tp;

	tp = gmtime(&mTime);

	return tp->tm_wday;
}

int FTC_ConvertTime_to_date(time_t sec,char * buf)
{	
		 struct tm tp;
	
		 gmtime_r(&sec,&tp);		

		sprintf(buf,"%04d%02d%02d_%02d%02d%02d",tp.tm_year + 1900,tp.tm_mon+1,tp.tm_mday,
			tp.tm_hour,tp.tm_min,tp.tm_sec);
		
		return ALLRIGHT;
}

int FTC_CD_ConvertTime_to_date(time_t sec,char * buf)
{
		GST_TIME_TM mytime;		

		struct tm tp;
	
		 gmtime_r(&sec,&tp);		

		//sprintf(buf,"%02d%02d%02d",tp.tm_hour,tp.tm_min,tp.tm_sec);

		sprintf(buf,"%04d%02d%02d_%02d%02d%02d",tp.tm_year + 1900,tp.tm_mon+1,tp.tm_mday,
			tp.tm_hour,tp.tm_min,tp.tm_sec);
		printf(" time buf is %s\n",buf);
		
		return ALLRIGHT;
}

//

int  FTC_GetFilelist( GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int iInfoNumber = 0;

	printf(" iDiskId =%d  iPartitionId=%d  iFilePos=%d\n",stFileShowInfo->iDiskId ,
						stFileShowInfo->iPartitionId ,stFileShowInfo->iFilePos);
	
	iDiskNum = DISK_GetDiskNum();

//	printf(" iDiskNum = %d\n",iDiskNum);

	if( stFileShowInfo->iDiskId < 0 || stFileShowInfo->iDiskId >= iDiskNum )
	{
		return ERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(stFileShowInfo->iDiskId);

	if( stFileShowInfo->iPartitionId < 5 ||
		stFileShowInfo->iPartitionId > stDiskInfo.partitionID[stDiskInfo.partitionNum-1] )
	{
		return ERROR;
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = stFileShowInfo->iDiskId; i < iDiskNum; i++ )
	{	
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	//	printf(" i = %d\n");
		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
		//	printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
			if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
					continue;

			
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
					return ERROR;
			
			for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
			{
			
				if( k < stFileShowInfo->iFilePos && 
					stDiskInfo.partitionID[j] <= stFileShowInfo->iPartitionId  
					&&   i == stFileShowInfo->iDiskId )
					continue;

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
					return ERROR;

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,pFileInfoPoint->pFilePos->EndTime);

				if( FTC_CompareTime(pFileInfoPoint->pFilePos->StartTime,searchStartTime,searchEndTime )
				     || FTC_CompareTime(pFileInfoPoint->pFilePos->EndTime,searchStartTime,searchEndTime )	)
				{
					if( pFileInfoPoint->pFilePos->states != WRITEOVER )
						continue;
				
					(stFileShowInfo + iInfoNumber)->iDiskId = i;
					(stFileShowInfo + iInfoNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
					(stFileShowInfo + iInfoNumber)->iFilePos = k;
					(stFileShowInfo + iInfoNumber)->fileStartTime = pFileInfoPoint->pFilePos->StartTime;

					printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d starttime = %ld\n",(stFileShowInfo + iInfoNumber)->iDiskId ,
						(stFileShowInfo + iInfoNumber)->iPartitionId ,(stFileShowInfo + iInfoNumber)->iFilePos,(stFileShowInfo + iInfoNumber)->fileStartTime);
					
					iInfoNumber++;
					
					if( iInfoNumber >= 100 )
						return ALLRIGHT;
				}		
				
				
			}
		}
	}

	for( i = iInfoNumber; i < 100 ; i++ )
	{		
		(stFileShowInfo + i)->iPartitionId = 0;	
		(stFileShowInfo + i)->iDiskId	   = 0;
		(stFileShowInfo + i)->iFilePos      = 0;
	}	

	return ALLRIGHT;
	
}


int FTC_GetRecordlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int i,j,k,h;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;
	RECADDLOGPOINT * pRecAddPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;
	long rec_size = 0;
	time_t rec_start_time = 0;
	int rec_add_cam = 0;
	
		
	printf(" iDiskId =%d  iPartitionId=%d  iFilePos=%d start=%ld end=%ld page=%d\n",stFileShowInfo->iDiskId ,
						stFileShowInfo->iPartitionId ,stFileShowInfo->iFilePos,searchStartTime,searchEndTime,iPage);

	if( iPage < 0 )
		return ERROR;
	
	iDiskNum = DISK_GetDiskNum();

//	printf(" iDiskNum = %d\n",iDiskNum);

	for( i = 0; i < SEACHER_LIST_MAX_NUM ; i++ )
	{		
		(stFileShowInfo + i)->iPartitionId = 0;	
		(stFileShowInfo + i)->iDiskId	   = 0;
		(stFileShowInfo + i)->iFilePos      = 0;
		(stFileShowInfo + i)->fileStartTime = 0; 
		(stFileShowInfo + i)->iCam = 0;		
		(stFileShowInfo + i)->event.motion = 0;
		(stFileShowInfo + i)->event.loss = 0;
		(stFileShowInfo + i)->event.sensor = 0;
		(stFileShowInfo + i)->iSeleted = 0;	
		
	}

	rel = FS_GetRecCurrentPoint(&stRecPos);
	if( rel < 0 )
	{
		printf("FS_GetRecCurrentPoint error!\n");
		return ALLRIGHT;
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{
		if(i > stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

			if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j] > stRecPos.iPartitionId )
				continue;
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	

				if(k > stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

				//printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

				//printf("k=%d starttime=%ld endtime=%ld effect=%d\n",k, pRecPoint->pFilePos->startTime,
				//	pRecPoint->pFilePos->endTime,pRecPoint->pFilePos->iDataEffect );

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,searchStartTime,searchEndTime )
				     || FTC_CompareTime(pRecPoint->pFilePos->endTime,searchStartTime,searchEndTime )	)
				{		
					if( pRecPoint->pFilePos->iDataEffect != YES )
						continue;
					
				/*	if(pRecPoint->pFilePos->videoStandard != g_pstPlaybackModeParam.iPbVideoStandard ||
						g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize)
						continue;	 */

					//printf("k=%d starttime=%ld endtime=%ld effect=%d add_info=%d\n",k, pRecPoint->pFilePos->startTime,
					//pRecPoint->pFilePos->endTime,pRecPoint->pFilePos->iDataEffect ,pRecPoint->pFilePos->have_add_info);

					if( pRecPoint->pFilePos->have_add_info == 1 )
					{
						pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],0);					

						rec_start_time = pRecPoint->pFilePos->startTime;
						rec_size = 0;
						rec_add_cam = pRecPoint->pFilePos->iCam;
						
						for(h=pRecAddPoint->pUseinfo->id; h<pRecAddPoint->pUseinfo->totalnumber; h++)
						{							
							
							pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],h);

						/*	if( pRecAddPoint->pFilePos->startTime != 0)
							printf("add_info recid=%d start=%ld cam=%x filepos=%d rec_log_time=%ld\n",
								pRecAddPoint->pFilePos->rec_log_id,pRecAddPoint->pFilePos->startTime,
								pRecAddPoint->pFilePos->iCam,pRecAddPoint->pFilePos->filePos,
								pRecAddPoint->pFilePos->rec_log_time);
							*/
							if( pRecAddPoint->pFilePos->rec_log_id != pRecPoint->pFilePos->id )
								continue;

							if( pRecAddPoint->pFilePos->rec_log_time != pRecPoint->pFilePos->startTime )
								continue;

							iInfoNumber++;
							
							if( iInfoNumber  <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
								continue;

						//	if(  iInfoNumber+1  >  (iPage +1)  * 10 )
						//		break;
							

							tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM -1;
				
							(stFileShowInfo + tempNumber)->iDiskId = i;
							(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
							(stFileShowInfo + tempNumber)->iFilePos = pRecAddPoint->pFilePos->filePos;
							(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;	
							(stFileShowInfo + tempNumber)->fileEndTime  = pRecAddPoint->pFilePos->startTime;
							(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
							(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
							(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;
							(stFileShowInfo + tempNumber)->iSeleted = pRecAddPoint->pFilePos->rec_size - rec_size;

							rec_size = pRecAddPoint->pFilePos->rec_size;
							rec_add_cam = pRecAddPoint->pFilePos->iCam;
							rec_start_time = pRecAddPoint->pFilePos->startTime;

						/*	printf(" 2Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
							*/
						}

						for(h=0; h<pRecAddPoint->pUseinfo->id; h++)
						{
							pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],h);							
							

							if( pRecAddPoint->pFilePos->rec_log_id != pRecPoint->pFilePos->id )
								continue;

							if( pRecAddPoint->pFilePos->rec_log_time != pRecPoint->pFilePos->startTime )
								continue;

						/*	if( pRecAddPoint->pFilePos->startTime != 0)
							printf("add_info recid=%d start=%ld cam=%x filepos=%d rec_log_time=%ld\n",
								pRecAddPoint->pFilePos->rec_log_id,pRecAddPoint->pFilePos->startTime,
								pRecAddPoint->pFilePos->iCam,pRecAddPoint->pFilePos->filePos,
								pRecAddPoint->pFilePos->rec_log_time);
						*/
							iInfoNumber++;

							if( iInfoNumber  <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
								continue;

							//if(  iInfoNumber+1  >  (iPage +1)  * 10 )
							//	break;						
							
							tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
							(stFileShowInfo + tempNumber)->iDiskId = i;
							(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
							(stFileShowInfo + tempNumber)->iFilePos = pRecAddPoint->pFilePos->filePos;
							(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;	
							(stFileShowInfo + tempNumber)->fileEndTime  = pRecAddPoint->pFilePos->startTime;
							(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
							(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
							(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;
							(stFileShowInfo + tempNumber)->iSeleted = pRecAddPoint->pFilePos->rec_size - rec_size;

							rec_size = pRecAddPoint->pFilePos->rec_size;
							rec_add_cam = pRecAddPoint->pFilePos->iCam;
							rec_start_time = pRecAddPoint->pFilePos->startTime;

						/*	printf(" 2Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
						*/
						}
					}

					iInfoNumber++;

					if( iInfoNumber <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = i;
					(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
					(stFileShowInfo + tempNumber)->iFilePos = pRecPoint->pFilePos->filePos;
					if(pRecPoint->pFilePos->have_add_info ==  1 )
					{
						(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;
						(stFileShowInfo + tempNumber)->iSeleted = pRecPoint->pFilePos->rec_size - rec_size;
						(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
					}else
					{
						(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->startTime;	
						(stFileShowInfo + tempNumber)->iSeleted = pRecPoint->pFilePos->rec_size;
						(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;
					}
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->endTime;					
					(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
					(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;					


				/*	printf(" 1Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
					*/
				}		
				
				
			}
		}
	}
		

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		if(i <  stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

		if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  < stRecPos.iPartitionId )
				continue;
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				if(k <= stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

			//	printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,searchStartTime,searchEndTime )
				     || FTC_CompareTime(pRecPoint->pFilePos->endTime,searchStartTime,searchEndTime )	)
				{		
					if( pRecPoint->pFilePos->iDataEffect != YES )
						continue;

					/*
					if(pRecPoint->pFilePos->videoStandard != g_pstPlaybackModeParam.iPbVideoStandard ||
						g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize)
						continue;*/

					if( pRecPoint->pFilePos->have_add_info == 1 )
					{
						pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],0);

						rec_start_time = pRecPoint->pFilePos->startTime;
						rec_size = 0;
						rec_add_cam = pRecPoint->pFilePos->iCam;
						
						for(h=pRecAddPoint->pUseinfo->id; h<pRecAddPoint->pUseinfo->totalnumber; h++)
						{
							pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],h);

							if( pRecAddPoint->pFilePos->rec_log_id != pRecPoint->pFilePos->id )
								continue;	

							if( pRecAddPoint->pFilePos->rec_log_time != pRecPoint->pFilePos->startTime )
								continue;

							iInfoNumber++;

							if( iInfoNumber  <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
								continue;

						//	if(  iInfoNumber+1  >  (iPage +1)  * 10 )
						//		break;

							
							tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
							(stFileShowInfo + tempNumber)->iDiskId = i;
							(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
							(stFileShowInfo + tempNumber)->iFilePos = pRecAddPoint->pFilePos->filePos;
							(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;	
							(stFileShowInfo + tempNumber)->fileEndTime  = pRecAddPoint->pFilePos->startTime;
							(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
							(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
							(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;
							(stFileShowInfo + tempNumber)->iSeleted = pRecAddPoint->pFilePos->rec_size - rec_size;

							rec_size = pRecAddPoint->pFilePos->rec_size;
							rec_add_cam = pRecAddPoint->pFilePos->iCam;
							rec_start_time = pRecAddPoint->pFilePos->startTime;

						/*			printf(" 2Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
						*/
						}

						for(h=0; h<pRecAddPoint->pUseinfo->id; h++)
						{
							pRecAddPoint = FS_GetPosRecAddMessage(i,
								stDiskInfo.partitionID[j],h);

							if( pRecAddPoint->pFilePos->rec_log_id != pRecPoint->pFilePos->id )
								continue;	

							if( pRecAddPoint->pFilePos->rec_log_time != pRecPoint->pFilePos->startTime )
								continue;

							iInfoNumber++;

							if( iInfoNumber <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
								continue;;
							
							tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
							(stFileShowInfo + tempNumber)->iDiskId = i;
							(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
							(stFileShowInfo + tempNumber)->iFilePos = pRecAddPoint->pFilePos->filePos;
							(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;	
							(stFileShowInfo + tempNumber)->fileEndTime  = pRecAddPoint->pFilePos->startTime;
							(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
							(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
							(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;
							(stFileShowInfo + tempNumber)->iSeleted = pRecAddPoint->pFilePos->rec_size - rec_size;

							rec_size = pRecAddPoint->pFilePos->rec_size;
							rec_add_cam = pRecAddPoint->pFilePos->iCam;
							rec_start_time = pRecAddPoint->pFilePos->startTime;

						/*	printf(" 2Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
						*/
						}
					}

					iInfoNumber++;

					if( iInfoNumber <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = i;
					(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;
					(stFileShowInfo + tempNumber)->iFilePos = pRecPoint->pFilePos->filePos;
					if(pRecPoint->pFilePos->have_add_info ==  1 )
					{
						(stFileShowInfo + tempNumber)->fileStartTime = rec_start_time;
						(stFileShowInfo + tempNumber)->iSeleted = pRecPoint->pFilePos->rec_size - rec_size;
						(stFileShowInfo + tempNumber)->iCam  = rec_add_cam;
					}else
					{
						(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->startTime;	
						(stFileShowInfo + tempNumber)->iSeleted = pRecPoint->pFilePos->rec_size;
						(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;
					}
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->endTime;					
					(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
					(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;					

				/*	printf(" 1Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
				*/
				}		
				
				
			}
		}
	}

	iPage = iInfoNumber / SEACHER_LIST_MAX_NUM;

	printf("iPage =%d, iInfoNumber=%d\n",iPage,iInfoNumber);

	if( iInfoNumber % SEACHER_LIST_MAX_NUM == 0 )
		return iPage;
	else 
		return iPage + 1;

}


void FTC_GetEveryDiskRecTime()
{
	int iDiskNum;
	int i,j,k,h;
	GST_DISKINFO stDiskInfo;
	RECTIMESTICKLOGPOINT * pRecPoint;	
	RECTIMESTICKLOGPOINT  RecPoint;
	static int RunOnce = 1;
	static int init = 0;


	if( init == 0 )
	{
		init = 1;
		pthread_mutex_init(&disk_record_time_mutex,NULL);
	}



	if( g_CurrentFormatDiskRecordTimeReload == 1 )
	{
		DPRINTK("g_CurrentFormatDiskRecordTimeReload = 1\n");
		g_CurrentFormatDiskRecordTimeReload = 0;
		RunOnce = 1;
	}

	if( RunOnce != 1 )
		return ;


	pthread_mutex_lock( &disk_record_time_mutex);


	iDiskNum = DISK_GetDiskNum();

	for(i=0;i < MAX_DISK_PARTION;i++)
	{
		disk_record_time[i].ulRecStartTime = 0;
		disk_record_time[i].ulRecEndTime = 0;
	}

	for(i=0; i < iDiskNum;i++)
	{
		stDiskInfo = DISK_GetDiskInfo(i);

		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0 ,&RecPoint );
			if( pRecPoint == NULL )
					goto FILE_ERROR;

			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k , &RecPoint );
				if( pRecPoint == NULL )
					goto FILE_ERROR;


				 if( pRecPoint->pFilePos->startTime <=0 )
				 	continue;

				 
				if( disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecStartTime == 0 )
				{
					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecStartTime = 
						pRecPoint->pFilePos->startTime;

					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecEndTime =
						pRecPoint->pFilePos->startTime + 299;					
				}


				if( pRecPoint->pFilePos->startTime < 
					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecStartTime )
				{
					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecStartTime 
						= pRecPoint->pFilePos->startTime;
				}


				if( (pRecPoint->pFilePos->startTime + 299) > 
					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecEndTime )
				{
					disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecEndTime
						= pRecPoint->pFilePos->startTime + 299;
				}
				
			}

			DPRINTK("disk=%d partition=%d rectime(%ld,%ld)\n",i,stDiskInfo.partitionID[j],
				disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecStartTime ,
				disk_record_time[(i*16)+stDiskInfo.partitionID[j]].ulRecEndTime );
		}
	}

	RunOnce = 0;

FILE_ERROR:

	pthread_mutex_unlock( &disk_record_time_mutex);

	return;
}

int FTC_Check_disk_time(time_t searchStartTime, time_t searchEndTime,int iDisk,int partitionID)
{
	int rel = 0;
	DISK_REC_TIME * pRecTime = NULL;
	
	pRecTime = &disk_record_time[(iDisk*16)+ partitionID];

	
	pthread_mutex_lock( &disk_record_time_mutex);


	if( FTC_CompareTime(searchStartTime,pRecTime->ulRecStartTime,pRecTime->ulRecEndTime) )
	{
		if( searchStartTime != 0 )
			rel = 1;
	}

	if( FTC_CompareTime(searchEndTime,pRecTime->ulRecStartTime,pRecTime->ulRecEndTime) )
	{
		if( searchEndTime != 0 )
			rel = 1;
	}

	if( FTC_CompareTime(pRecTime->ulRecStartTime,searchStartTime,searchEndTime) )
	{
		if( (searchStartTime != 0) && ( searchEndTime != 0 ) )
			rel = 1;
	}

	if( FTC_CompareTime(pRecTime->ulRecEndTime,searchStartTime,searchEndTime) )
	{
		if( (searchStartTime != 0) && ( searchEndTime != 0 ) )
		 	rel = 1;
	}

	//确保一天中的时间可以播放。
	if( FTC_CompareTime(searchStartTime,pRecTime->ulRecStartTime - (24*60*60),pRecTime->ulRecEndTime + (24*60*60)) )
	{
		if( searchStartTime != 0 )
		{
			//DPRINTK("one day play\n");
			rel = 1;
		}
	}

	pthread_mutex_unlock( &disk_record_time_mutex);

	if( rel == 1 )
	{
		DPRINTK("disk=%d partition=%d rectime(%ld,%ld) searchtime(%ld,%ld)\n",
			iDisk,partitionID,pRecTime->ulRecStartTime,pRecTime->ulRecEndTime,
			searchStartTime,searchEndTime);

		#ifdef USE_DISK_SLEEP_SYSTEM
		FS_Disk_Use(iDisk);
		#endif
	}

	return rel;
}


void FTC_rec_write_log_change_disk_rec_time(time_t rec_time,int iDisk,int iPartitionID)
{
	int k;
	RECTIMESTICKLOGPOINT * pRecPoint;	
	RECTIMESTICKLOGPOINT  RecPoint;
	DISK_REC_TIME * pRecTime = NULL;
	
	pRecTime = &disk_record_time[(iDisk*16)+ iPartitionID];

	pthread_mutex_lock( &disk_record_time_mutex);

	pRecTime->ulRecStartTime = 0;
	pRecTime->ulRecEndTime = 0;

	pRecPoint = FS_GetPosRecTimeStickMessage(iDisk,iPartitionID , 0 ,&RecPoint );
	if( pRecPoint == NULL )
			goto FUNC_END;

	for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
	{	
		pRecPoint = FS_GetPosRecTimeStickMessage(iDisk,iPartitionID , k , &RecPoint );
		if( pRecPoint == NULL )
			goto FUNC_END;


		 if( pRecPoint->pFilePos->startTime ==0 )
		 	continue;

		 
		if( pRecTime->ulRecStartTime == 0 )
		{
			pRecTime->ulRecStartTime = 
				pRecPoint->pFilePos->startTime;

			pRecTime->ulRecEndTime =
				pRecPoint->pFilePos->startTime + 299;					
		}


		if( pRecPoint->pFilePos->startTime < pRecTime->ulRecStartTime )
		{
			pRecTime->ulRecStartTime = pRecPoint->pFilePos->startTime;
		}


		if( (pRecPoint->pFilePos->startTime + 299) > pRecTime->ulRecEndTime )
		{
			pRecTime->ulRecEndTime = pRecPoint->pFilePos->startTime + 299;
		}
		
	}

FUNC_END:
	pthread_mutex_unlock( &disk_record_time_mutex);

	DPRINTK("disk=%d partition=%d rectime(%ld,%ld)  (%ld,%ld)\n",iDisk,iPartitionID,
		disk_record_time[(iDisk*16)+iPartitionID].ulRecStartTime ,
		disk_record_time[(iDisk*16)+iPartitionID].ulRecEndTime ,
		pRecTime->ulRecStartTime,pRecTime->ulRecEndTime);
}

int FTC_disk_rec_time(time_t * start_time,time_t * end_time,int disk_id)
{
	DISK_REC_TIME * pRecTime = NULL;	
	int i = 0;
	time_t time1,time2;

	pthread_mutex_lock( &disk_record_time_mutex);
	
	time1 = 0;
	time2 = 0;

	for( i= 0;i < 16;i++)
	{
		pRecTime = &disk_record_time[(disk_id*16)+ i];


		if( pRecTime->ulRecStartTime <=0 )
				 	continue;

		if( time1 == 0 )
		{			
			time1 = pRecTime->ulRecStartTime;
			time2 = pRecTime->ulRecEndTime;			
		}

		if( pRecTime->ulRecStartTime < time1 )
		{
			if( pRecTime->ulRecStartTime != 0 )
				time1 = pRecTime->ulRecStartTime;
		}


		if( pRecTime->ulRecEndTime > time2 )
		{
			if( pRecTime->ulRecEndTime != 0 )
				time2 = pRecTime->ulRecEndTime;
		}

		//DPRINTK("(%d,%d)(%ld,%ld) (%ld,%ld)\n",disk_id,i,
		//	pRecTime->ulRecStartTime,pRecTime->ulRecEndTime,
		//	time1,time2);
		
	}
	
	*start_time = time1;
	*end_time = time2;

	pthread_mutex_unlock( &disk_record_time_mutex);

	return 1;
}


int FTC_GetRecordTimeSticklist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int i,j,k,h;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECTIMESTICKLOGPOINT * pRecPoint;	
	RECTIMESTICKLOGPOINT  RecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;
	long rec_size = 0;
	time_t rec_start_time = 0;
	int rec_add_cam = 0;
	
		
	printf(" iDiskId =%d  iPartitionId=%d  iFilePos=%d start=%ld end=%ld page=%d\n",stFileShowInfo->iDiskId ,
						stFileShowInfo->iPartitionId ,stFileShowInfo->iFilePos,searchStartTime,searchEndTime,iPage);

	if( iPage < 0 )
		return ERROR;

	
	iDiskNum = DISK_GetDiskNum();

//	printf(" iDiskNum = %d\n",iDiskNum);

	for( i = 0; i < SEACHER_LIST_MAX_NUM ; i++ )
	{		
		(stFileShowInfo + i)->iPartitionId = 0;	
		(stFileShowInfo + i)->iDiskId	   = 0;
		(stFileShowInfo + i)->iFilePos      = 0;
		(stFileShowInfo + i)->fileStartTime = 0; 
		(stFileShowInfo + i)->iCam = 0;		
		(stFileShowInfo + i)->event.motion = 0;
		(stFileShowInfo + i)->event.loss = 0;
		(stFileShowInfo + i)->event.sensor = 0;
		(stFileShowInfo + i)->iSeleted = 0;	
		
	}

	rel = FS_GetRecCurrentPoint(&stRecPos);
	if( rel < 0 )
	{
		printf("FS_GetRecCurrentPoint error!\n");
		return ALLRIGHT;
	}

	#ifdef USE_PLAYBACK_QUICK_SYSTEM

	FTC_GetEveryDiskRecTime();

	#endif
	

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{
		if(i > stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	
			if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j] > stRecPos.iPartitionId )
				continue;


			#ifdef USE_PLAYBACK_QUICK_SYSTEM

			if( FTC_Check_disk_time(searchStartTime,searchEndTime,i,stDiskInfo.partitionID[j])== 0 )
				continue;

			#endif
			
			
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0 ,&RecPoint );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	

				if(k > stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k , &RecPoint );

				if( pRecPoint == NULL )
					return ERROR;

				if( pRecPoint->pFilePos->startTime == 0 )
					continue;

				//printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

				//printf("k=%d starttime=%ld endtime=%ld effect=%d\n",k, pRecPoint->pFilePos->startTime,
				//	pRecPoint->pFilePos->endTime,pRecPoint->pFilePos->iDataEffect );

				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,searchStartTime,searchEndTime )	)
				{										

					/*
					printf("k=%d starttime=%ld  stand=%d image=%d  play_back(%d,%d)\n",k, pRecPoint->pFilePos->startTime,					
					pRecPoint->pFilePos->videoStandard,pRecPoint->pFilePos->imageSize,
					g_pstPlaybackModeParam.iPbVideoStandard,g_pstPlaybackModeParam.iPbImageSize);
					*/

				/*	if(pRecPoint->pFilePos->videoStandard != g_pstPlaybackModeParam.iPbVideoStandard ||
						g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize)
						continue;*/

					iInfoNumber++;

					if( iInfoNumber <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = i;
					(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;						
					(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->startTime;	
					(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;					
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->startTime + 299; //300秒	 				
					

				/*	printf(" 1Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
					*/
				}		
				
				
			}
		}
	}
		

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		if(i <  stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

		if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  < stRecPos.iPartitionId )
				continue;


			#ifdef USE_PLAYBACK_QUICK_SYSTEM

			if( FTC_Check_disk_time(searchStartTime,searchEndTime,i,stDiskInfo.partitionID[j])== 0 )
				continue;

			#endif
			
			
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0 ,&RecPoint);
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				if(k <= stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k , &RecPoint );

				if( pRecPoint == NULL )
					return ERROR;

				if( pRecPoint->pFilePos->startTime == 0 )
					continue;

				//printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

				//printf("k=%d starttime=%ld \n",k, pRecPoint->pFilePos->startTime );
				
				if( FTC_CompareTime(pRecPoint->pFilePos->startTime,searchStartTime,searchEndTime )	)
				{	
				
					/*if(pRecPoint->pFilePos->videoStandard != g_pstPlaybackModeParam.iPbVideoStandard ||
						g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize)
						continue;*/

					iInfoNumber++;

					if( iInfoNumber <=  iPage * SEACHER_LIST_MAX_NUM  || iInfoNumber  >  (iPage +1)  * SEACHER_LIST_MAX_NUM  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * SEACHER_LIST_MAX_NUM  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = i;
					(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j] ;						
					(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->startTime;	
					(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;					
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->startTime + 299; //300秒	 				
					
				/*	printf(" 1Id=%d iPId =%d  iFilePos=%d start=%ld %ld %d Size=%d Stand=%d len=%ld\n",(stFileShowInfo + tempNumber)->iDiskId ,
						(stFileShowInfo + tempNumber)->iPartitionId ,(stFileShowInfo + tempNumber)->iFilePos,
						(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iCam,(stFileShowInfo + tempNumber)->imageSize,
						(stFileShowInfo + tempNumber)->videoStandard,(stFileShowInfo + tempNumber)->iSeleted);
				*/
				}		
				
				
			}
		}
	}

	iPage = iInfoNumber / SEACHER_LIST_MAX_NUM;

	printf("iPage =%d, iInfoNumber=%d\n",iPage,iInfoNumber);

	if( iInfoNumber % SEACHER_LIST_MAX_NUM == 0 )
		return iPage;
	else 
		return iPage + 1;

}




int FTC_GetAlarmEventlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	ALARMLOGPOINT * pRecPoint;
	GST_RECORDEVENT search_event;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;
	int get_info_num = 0;

	printf("alarm  iDiskId =%d  iPartitionId=%d  iFilePos=%d start=%ld end=%ld page=%d cam=%d\n",stFileShowInfo->iDiskId ,
						stFileShowInfo->iPartitionId ,stFileShowInfo->iFilePos,searchStartTime,searchEndTime,iPage,iCam);

	get_info_num = stFileShowInfo->iDiskId;

	if( get_info_num <= 0 || get_info_num > 10 )
		get_info_num = 10;

	search_event = stFileShowInfo->event;

	if( iPage < 0 )
		return ERROR;
	
	iDiskNum = DISK_GetDiskNum();

//	printf(" iDiskNum = %d\n",iDiskNum);

	for( i = 0; i < 10 ; i++ )
	{		
		(stFileShowInfo + i)->iPartitionId = 0;	
		(stFileShowInfo + i)->iDiskId	   = 0;
		(stFileShowInfo + i)->iFilePos      = 0;
		(stFileShowInfo + i)->fileStartTime = 0; 
		(stFileShowInfo + i)->iCam = 0;	
		(stFileShowInfo + i)->event.motion = 0;
		(stFileShowInfo + i)->event.loss = 0;
		(stFileShowInfo + i)->event.sensor = 0;
	}

	rel = FS_GetAlarmCurrentPoint(&stRecPos);
	if( rel < 0 )
	{
		printf("FS_GetAlarmCurrentPoint error!\n");
		return ALLRIGHT;
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		stDiskInfo = DISK_GetDiskInfo(i);

		if(i > stRecPos.iDiskId )
			continue;
		
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;

		#ifdef USE_DISK_SLEEP_SYSTEM
		if( FS_Disk_Is_Use(searchStartTime,searchEndTime,i) == 0 )			
			continue;		
		#endif
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

			if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j] > stRecPos.iPartitionId )
				continue;

			
			pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	

				if(k > stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

		//		printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->alarmStartTime,pRecPoint->pFilePos->alarmEndTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->alarmStartTime,searchStartTime,searchEndTime )
				     || FTC_CompareTime(pRecPoint->pFilePos->alarmEndTime,searchStartTime,searchEndTime )	)
				{		
					if( pRecPoint->pFilePos->iDataEffect != YES )
						continue;

					if( pRecPoint->pFilePos->iCam != iCam)
						continue;

					if( pRecPoint->pFilePos->event.motion != search_event.motion
						||  pRecPoint->pFilePos->event.loss != search_event.loss
						|| pRecPoint->pFilePos->event.sensor!= search_event.sensor)
						continue;

					iInfoNumber++;

					if( iInfoNumber <=  iPage * get_info_num  || iInfoNumber  >  (iPage +1)  * get_info_num  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * get_info_num  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = i;
					(stFileShowInfo + tempNumber)->iPartitionId = stDiskInfo.partitionID[j]  ;
					(stFileShowInfo + tempNumber)->iFilePos =pRecPoint->pFilePos->iRecMessageId;
					(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->alarmStartTime;	
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->alarmEndTime;
					(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;
					(stFileShowInfo + tempNumber)->iPreviewTime = pRecPoint->pFilePos->iPreviewTime;
					(stFileShowInfo + tempNumber)->iLaterTime  =  pRecPoint->pFilePos->iLaterTime;
					(stFileShowInfo + tempNumber)->states = pRecPoint->pFilePos->states;
					(stFileShowInfo + tempNumber)->event = pRecPoint->pFilePos->event;
					(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
					(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;
				
					printf("  starttime = %ld  endtime=%ld preview=%d iLaterTime =%d cam=%d\n",(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iPreviewTime,(stFileShowInfo + tempNumber)->iLaterTime,(stFileShowInfo + tempNumber)->iCam);

					printf(" event %d,%d,%d\n",(stFileShowInfo + tempNumber)->event.motion,(stFileShowInfo + tempNumber)->event.loss,(stFileShowInfo + tempNumber)->event.sensor);
					
				}		
				
				
			}
		}
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		stDiskInfo = DISK_GetDiskInfo(i);

		if(i <  stRecPos.iDiskId )
			continue;
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

			if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  < stRecPos.iPartitionId )
				continue;
			
			pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				if(k <= stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosAlarmMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

	//			printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->alarmStartTime,pRecPoint->pFilePos->alarmEndTime);

				if( FTC_CompareTime(pRecPoint->pFilePos->alarmStartTime,searchStartTime,searchEndTime )
				     || FTC_CompareTime(pRecPoint->pFilePos->alarmEndTime,searchStartTime,searchEndTime )	)
				{		
					if( pRecPoint->pFilePos->iDataEffect != YES )
						continue;

					iInfoNumber++;

					if( iInfoNumber <=  iPage * get_info_num  || iInfoNumber  >  (iPage +1)  * get_info_num  )
					{
						continue;
					}					

					tempNumber = iInfoNumber - iPage * get_info_num  -1;
				
					(stFileShowInfo + tempNumber)->iDiskId = 0;
					(stFileShowInfo + tempNumber)->iPartitionId = 0 ;
					(stFileShowInfo + tempNumber)->iFilePos =0;
					(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->alarmStartTime;	
					(stFileShowInfo + tempNumber)->fileEndTime  = pRecPoint->pFilePos->alarmEndTime;
					(stFileShowInfo + tempNumber)->iCam  = pRecPoint->pFilePos->iCam;
					(stFileShowInfo + tempNumber)->iPreviewTime = pRecPoint->pFilePos->iPreviewTime;
					(stFileShowInfo + tempNumber)->iLaterTime  =  pRecPoint->pFilePos->iLaterTime;
					(stFileShowInfo + tempNumber)->states = pRecPoint->pFilePos->states;
					(stFileShowInfo + tempNumber)->event = pRecPoint->pFilePos->event;
					(stFileShowInfo + tempNumber)->videoStandard = pRecPoint->pFilePos->videoStandard;
					(stFileShowInfo + tempNumber)->imageSize = pRecPoint->pFilePos->imageSize;

					printf("  starttime = %ld  endtime=%ld preview=%d iLaterTime =%d cam=%d\n",(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->fileEndTime,
						(stFileShowInfo + tempNumber)->iPreviewTime,(stFileShowInfo + tempNumber)->iLaterTime,(stFileShowInfo + tempNumber)->iCam);

					printf(" event %d,%d,%d\n",(stFileShowInfo + tempNumber)->event.motion,(stFileShowInfo + tempNumber)->event.loss,(stFileShowInfo + tempNumber)->event.sensor);
					
				}		
				
				
			}
		}
	}

	iPage = div_safe(iInfoNumber , get_info_num);

	printf("iPage =%d, iInfoNumber=%d\n",iPage,iInfoNumber);

	if( iInfoNumber % get_info_num == 0 )
		return iPage;
	else 
		return iPage + 1;

}


int FTC_GetEventlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;	
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;
	EVENTLOGPOINT * pRecPoint;
	int get_info_num = 0;;

	printf("num=%d event  start=%ld end=%ld page=%d\n",stFileShowInfo->iDiskId,searchStartTime,searchEndTime,iPage);

	get_info_num = stFileShowInfo->iDiskId;

	if( get_info_num <= 0 || get_info_num > 10)
		get_info_num = 10;

	if( iPage < 0 )
		return ERROR;
	
	iDiskNum = DISK_GetDiskNum();


	for( i = 0; i < 10 ; i++ )
	{		
		(stFileShowInfo + i)->iPartitionId = 0;	
		(stFileShowInfo + i)->iDiskId	   = 0;
		(stFileShowInfo + i)->iFilePos      = 0;
		(stFileShowInfo + i)->fileStartTime = 0; 
		(stFileShowInfo + i)->iCam = 0;	
		(stFileShowInfo + i)->event.motion = 0;
		(stFileShowInfo + i)->event.loss = 0;
		(stFileShowInfo + i)->event.sensor = 0;
		(stFileShowInfo + i)->CurrentEvent = NORMAL;
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	
	pRecPoint = FS_GetPosEventMessage(0, 5, 0);

	if( pRecPoint == NULL )
	{	
		return FILEERROR;
	}

	for( i = pRecPoint->pUseinfo->id; i >= 0; i-- )
	{
		pRecPoint = FS_GetPosEventMessage(0, 5, i);

		if( pRecPoint == NULL )
			return FILEERROR;

		if( FTC_CompareTime(pRecPoint->pFilePos->EventStartTime,searchStartTime,searchEndTime ))
		{
			if( pRecPoint->pFilePos->iDataEffect == NOEFFECT )
				continue;

			iInfoNumber++;

			if( iInfoNumber <=  iPage * get_info_num  || iInfoNumber  >  (iPage +1)  * get_info_num  )
			{
				continue;
			}					

			tempNumber = iInfoNumber - iPage * get_info_num  -1;

			(stFileShowInfo + tempNumber)->iCam = pRecPoint->pFilePos->iCam;
			(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->EventStartTime;	
			(stFileShowInfo + tempNumber)->CurrentEvent  = pRecPoint->pFilePos->CurrentEvent;

		//	printf(" 111 %ld %d\n",(stFileShowInfo + tempNumber)->fileStartTime,(stFileShowInfo + tempNumber)->CurrentEvent);
						
		}

	}

	for( i = pRecPoint->pUseinfo->totalnumber - 1; i > pRecPoint->pUseinfo->id ; i-- )
	{
		pRecPoint = FS_GetPosEventMessage(0, 5, i);

		if( pRecPoint == NULL )
			return FILEERROR;

		if( FTC_CompareTime(pRecPoint->pFilePos->EventStartTime,searchStartTime,searchEndTime ))
		{
			if( pRecPoint->pFilePos->iDataEffect == NOEFFECT )
				continue;

			iInfoNumber++;

			if( iInfoNumber <=  iPage * get_info_num  || iInfoNumber  >  (iPage +1)  * get_info_num  )
			{
				continue;
			}					

			tempNumber = iInfoNumber - iPage * get_info_num  -1;

			(stFileShowInfo + tempNumber)->iCam = pRecPoint->pFilePos->iCam;
			(stFileShowInfo + tempNumber)->fileStartTime = pRecPoint->pFilePos->EventStartTime;	
			(stFileShowInfo + tempNumber)->CurrentEvent  = pRecPoint->pFilePos->CurrentEvent;
						
		}

	}
	

	iPage = div_safe(iInfoNumber , get_info_num);

	printf("iPage =%d, iInfoNumber=%d\n",iPage,iInfoNumber);

	if( iInfoNumber % get_info_num == 0 )
		return iPage;
	else 
		return iPage + 1;

}



int FTC_CompareTime(time_t yourtime, time_t startTime, time_t endTime )
{
	if( yourtime >= startTime && yourtime <= endTime )
		return 1;
	else
		return 0;
}


//将(0-31)位置1
int FTC_SetBit(int iCam, int iBitSite)
{
	int value;

	value = 1;

	value = value << iBitSite;

	iCam = iCam |value;

	return iCam;
}

// 返回（0 - 31  ) 位的值，返回值为0 或1
int FTC_GetBit(int iCam, int iBitSite)
{
	iCam = iCam >> iBitSite;

	iCam = iCam & 0x01;

	return iCam;
}


//顺序回放函数
int FTC_OrderPlayFile()
{
	static int iDiskId = 0,iPartitionId = 5,iFilePos = 0;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int rel;

	stDiskInfo = DISK_GetDiskInfo(0);

	pFileInfoPoint = FS_GetPosFileInfo(iDiskId,iPartitionId , iFilePos,&fileInfoPoint );

	if( pFileInfoPoint == NULL )
		return ERROR;

	if( iFilePos >= pFileInfoPoint->pUseinfo->iTotalFileNumber )
	{
		iFilePos = 0;

		iPartitionId++;

		if( iPartitionId > stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ] )
				iPartitionId = 5;
	}

	rel = OpenPlayFile(iDiskId,iPartitionId,iFilePos);

	if( rel > 0 )
	{
		iFilePos++;
	}

	return rel;

	
}

int FTC_ToNextFile()
{
	GST_DISKINFO stDiskInfo;

	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;

	stDiskInfo = DISK_GetDiskInfo(g_playFileInfo.iDiskId);

	pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,g_playFileInfo.iPartitionId , g_playFileInfo.iFilePos ,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	g_playFileInfo.iFilePos += 1;
	
	if( g_playFileInfo.iFilePos  >= pFileInfoPoint->pUseinfo->iTotalFileNumber )
	{
		g_playFileInfo.iFilePos  = 0;

		g_playFileInfo.iPartitionId++;

		if( g_playFileInfo.iPartitionId > stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ] )
		{
			g_playFileInfo.iPartitionId = 5;

			g_playFileInfo.iDiskId++;

			if(g_playFileInfo.iDiskId >= DISK_GetDiskNum())
				g_playFileInfo.iDiskId = 0;			
		}		
	}

	pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,g_playFileInfo.iPartitionId , g_playFileInfo.iFilePos ,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	if( pFileInfoPoint->pFilePos->states != WRITEOVER )
		return FILESTATEERROR;

	return ALLRIGHT;

}

int FTC_ToLastFile()
{
	GST_DISKINFO stDiskInfo;

	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;

	stDiskInfo = DISK_GetDiskInfo(g_playFileInfo.iDiskId);

	pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,g_playFileInfo.iPartitionId , g_playFileInfo.iFilePos ,&fileInfoPoint);

	if( pFileInfoPoint == NULL )
		return ERROR;

	g_playFileInfo.iFilePos -= 1;
	
	if( g_playFileInfo.iFilePos  < 0)
	{
		
		g_playFileInfo.iPartitionId--;

		if( g_playFileInfo.iPartitionId < 5 )
		{
			g_playFileInfo.iDiskId--;

			if(g_playFileInfo.iDiskId < 0 )
			{
			//小于零说明这是id 号低的硬盘	
				g_playFileInfo.iDiskId  = DISK_GetDiskNum() - 1;
				stDiskInfo = DISK_GetDiskInfo(g_playFileInfo.iDiskId);

				pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ] , 0 ,&fileInfoPoint);
				if( pFileInfoPoint == NULL )
					return ERROR;

				g_playFileInfo.iPartitionId = stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ];
				g_playFileInfo.iFilePos = pFileInfoPoint->pUseinfo->iTotalFileNumber-1;
			}
			else
			{
		
				stDiskInfo = DISK_GetDiskInfo(g_playFileInfo.iDiskId);

				pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ] , 0 ,&fileInfoPoint);
				if( pFileInfoPoint == NULL )
					return ERROR;
				
				g_playFileInfo.iPartitionId = stDiskInfo.partitionID[stDiskInfo.partitionNum -1 ];
				g_playFileInfo.iFilePos = pFileInfoPoint->pUseinfo->iTotalFileNumber-1;
			}
			
		}		
		else
		{
			pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,g_playFileInfo.iPartitionId, 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
				return ERROR;
			
			g_playFileInfo.iFilePos = pFileInfoPoint->pUseinfo->iTotalFileNumber-1;
		}		
		
	}

	pFileInfoPoint = FS_GetPosFileInfo(g_playFileInfo.iDiskId ,g_playFileInfo.iPartitionId , g_playFileInfo.iFilePos,&fileInfoPoint );

	if( pFileInfoPoint == NULL )
		return ERROR;

	if( pFileInfoPoint->pFilePos->states != WRITEOVER )
		return FILESTATEERROR;

	return ALLRIGHT;

}




int FTC_SetPlayFileInfo(GST_FILESHOWINFO * stPlayFileInfo)
{
	int count = 0;
	int seacher_file_num = 0;
	int ret;
	time_t play_time = 0;
	
	g_playFileInfo.iDiskId = stPlayFileInfo->iDiskId;
	g_playFileInfo.iPartitionId = stPlayFileInfo->iPartitionId;
	g_playFileInfo.iFilePos      =  stPlayFileInfo->iFilePos;
	g_playFileInfo.iCam 	= stPlayFileInfo->iCam;
//	g_playFileInfo.iCam 	= 0x0f;

	play_time = get_play_start_flag();

SEACHR_PLAY_START:
	printf(" FTC_SetPlayFileInfo %d,%d,%d iCam=%d\n",g_playFileInfo.iDiskId,
		g_playFileInfo.iPartitionId ,g_playFileInfo.iFilePos  ,g_playFileInfo.iCam);

	  g_PlayStatus = PLAYNORMAL;
	  g_SearchLastSingleFrameChangeFile = 0;

	  FTC_FileGoToTimePoint(play_time);	

	  printf("333gPlaystart = %d\n",gPlaystart);

	if( gPlaystart == 1 )
	{
		// 停止回放
		
		gPlaystart = -2;
		
		while(1)
		{			
			usleep(10000);
			
			if( gPlaystart == 0 )
			{
				// 开始回放
				gPlaystart = 1;				
				
				return ALLRIGHT;
			}
		}
	}

	printf(" GST_DRA_start_playback \n");
//	g_pstCommonParam->GST_DRA_start_playback(NULL);

	g_pstCommonParam->GST_set_mult_play_cam(g_playFileInfo.iCam);
	g_pstCommonParam->GST_DRV_CTRL_StartStopPlayBack(1);
	

	gPlaystart = 1;
	

	seacher_file_num++;

	if( seacher_file_num > 5 )
	{
		printf("near 5 file not have data\n");
		return ERROR;
	}

	for( count = 0; count < 50; count++ )
	{
		usleep(100000);

		if( gPlaystart != 1 )
		{

			//10-11-01 修改 gPlaystart 的使用与play thread 冲突。确保回放停止才换下个文件
			while(gPlaystart != 0 )
			{
				usleep(10);
			}
			////////
		
			ret =  FTC_ToNextFile();

			if( ret < 0 )
				return ret;

			printf("play next file\n");

			goto SEACHR_PLAY_START;
		}

		if( get_play_start_flag() == 0 )
		{
			printf(" get_play_start_flag ok!\n");
			break;
		}
	}
	
	DPRINTK("gPlaystart=%d  play file start\n",gPlaystart);
	
	return ALLRIGHT;
}

int FTC_StopPlay()
{
	int iCount = 0;

	if( gPlaystart == 1 )
	{
		// 停止回放
		g_HandStopPlay = 1;
		gPlaystart = -2;
		
		while(1)
		{			
			usleep(100000);

			iCount++;
			
			if( gPlaystart == 0 )
			{	
				g_HandStopPlay = 0;
				return ALLRIGHT;
			}
			else
			{
				if( iCount > 50 )
				{
					g_HandStopPlay = 0;
					printf("stop play file error!\n");
					return ERROR;
				}
			}
		}
	}else
	{
		printf("222gPlaystart = %d\n",gPlaystart);
	}

	return ALLRIGHT;
}


int FTC_StartRec(int iCam,int iPreSecond)
{
	int i = 0;
	int iCount = 0;
	int tmp_cam = 0;
	struct timeval recTime;
	struct timeval recTimeStick;
	int k;


	DPRINTK(" FTC_StartRec  gRecstart=%d\n",gRecstart);
	DPRINTK(" rec g_PreRecordFlag =%d gRecstart=%d iCam=%x\n",g_PreRecordFlag,gRecstart,iCam);


	if( iCam == 0 )
	{
		FTC_StopRec();
		return ALLRIGHT;
	}

	g_pstCommonParam->GST_DRA_get_sys_time( &recTimeStick, NULL );

	k = recTimeStick.tv_sec / 300;

	recTimeStick.tv_sec = k * 300;
	
	if( iPreSecond == 100 && gRecstart == 1)
	{	
		tmp_cam = g_RecCam ^ iCam;;
	
		g_RecCam   = iCam;	

		for(i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++)
		{
			if( FTC_GetBit(tmp_cam, i ) == 1 )
			{
				// make sure first frame is key frame.
				set_rec_chan_have_key_frame(i,0);				
			}
		}

		
		for(i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++)
		{
			if( FTC_GetBit(iCam, i ) == 1 )
			{
				iLowestChannelId = i;	
				break;
			}
		}	

		printf(" new cam = %x  iLowestChannelId=%d\n",g_RecCam,iLowestChannelId);
		
		g_pstCommonParam->GST_DRA_local_instant_i_frame(0xffff);

		g_pstCommonParam->GST_DRA_get_sys_time( &recTime, NULL );
		
		/*if( g_preview_sys_flag == 1 )
			FS_WriteInfotoRecAddLog(g_previous_headtime,iCam);
		else				
			FS_WriteInfotoRecAddLog(recTime.tv_sec,iCam);
			*/

		FS_WriteInfotoRecTimeStickLog(recTimeStick.tv_sec,iCam);
		
		return ALLRIGHT;
	}	

	
	if( gRecstart == 0 && g_PreRecordFlag == 0)
	{
		gRecstart = 1;
	}
	else
	{
		DPRINTK(" gRecstart current =%d  ,Can't REC\n",gRecstart);
		if(1)
		{

			void * tmp_addr;
			
			tmp_addr = g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(NULL);

			if( tmp_addr != NULL )
			{
				DPRINTK("have encode data ,but not rec\n");
				//FS_RecFileCurrentPartitionWriteToShutDownLog(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId);
			}
		}
		return ERROR;
	}

	for(i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++)
	{
		// make sure first frame is key frame.
		set_rec_chan_have_key_frame(i,0);
	}

	for(i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++)
	{		
		if( FTC_GetBit(iCam, i ) == 1 )
		{
			iLowestChannelId = i;	
			break;
		}
	}	

	g_RecCam   = iCam;
	g_iAlreadyWriteDisk = 0;

	printf("start rec !grecstart = %d  iCam=%x lowstChannelId=%d g_RecAudioCam=%d\n",gRecstart,g_RecCam,iLowestChannelId,g_RecAudioCam);
	g_pstCommonParam->GST_DRA_start_stop_recording_flag(1);
	g_pstCommonParam->GST_DRA_local_instant_i_frame(0xffffffff);
	printf("GST_DRA_local_instant_i_frame\n");

	while(1)
	{
		iCount++;
		
		usleep(100000);

		printf(" rec icount = %d\n",iCount);

		if( g_iAlreadyWriteDisk == 0 )
		{		
		
			if( iCount > 100 )	
			{
				printf(" Rec start failed\n");
				g_iAlreadyWriteDisk = 0;
				gRecstart = 0;

				sleep(2);

				if(1)
				{

					void * tmp_addr;
					
					tmp_addr = g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(NULL);

					if( tmp_addr != NULL )
					{
						DPRINTK("have encode data ,but not rec\n");
						//FS_RecFileCurrentPartitionWriteToShutDownLog(g_stRecFilePos.iDiskId,g_stRecFilePos.iPartitionId);
					}
				}
				return ERROR;
			}
		}

		if( g_iAlreadyWriteDisk == 1 )
		{
			printf("  Rec start success\n");
			break;
		}
	}

	//printf("come here!\n");

	FS_WriteInfotoRecTimeStickLog(recTimeStick.tv_sec,iCam);

	return ALLRIGHT;
}

int FTC_StopRec()
{
	int iCount = 0;

	DPRINTK(" FTC_StopRec  gRecstart=%d\n",gRecstart);	
	
	if( gRecstart == 0 )
	{
		printf(" rec already stop!\n");
		return ALLRIGHT;
	}

	if( gRecstart == 1 )
	{
		// 停止录相 
		gRecstart = -2;
		
		while(1)
		{			
			usleep(100000);

			iCount++;
			
			if( gRecstart == 0 )
			{				
				printf(" Record stop right!\n");
				return ALLRIGHT;
			}
			else
			{
				if( iCount > 50 )
				{	
					printf(" Record stop error11!\n");
					return ERROR;
				}
			}
		}
	}
	else
	{
		while(1)
		{			
			usleep(100000);

			iCount++;
			
			if( gRecstart == 0 )
			{				
				printf(" Record stop right!\n");
				return ALLRIGHT;
			}
			else
			{
				if( iCount > 50 )
				{	
					printf(" Record stop error22!\n");
					return ERROR;
				}
			}
		}

	
		printf(" Can't Stop Rec! there are other rec mode\n");
		return ERROR;
	}

	return ALLRIGHT;
}

GST_DRV_BUF_INFO* FTC_get_video_audio_buf(char * pData)
{
	return g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(pData);
}


int FTC_PreviewRecordStart(int iSecond)
{
	int ret;

	int iCount = 0;
	
	printf(" rec g_PreRecordFlag =%d gRecstart=%d\n",g_PreRecordFlag,gRecstart);

	if( iSecond < 3|| iSecond > 10 )
	{
		printf("iSecond = %d  Second must bigger than 5\n",iSecond);
		return ERROR;
	}

	if( ftc_get_rec_cur_status() != 0 )
	{
		printf(" allready record!\n");
		return  ERROR;
	}


	ResetPreviouViewBuf();	

	FS_SetPreviewTime(iSecond);

	printf(" start PreviouView! %d second\n",iSecond);

	g_preview_sys_flag = 1;
	g_PreviewIsStart = 0;

	g_pstCommonParam->GST_DRA_start_stop_recording_flag(1);
	g_pstCommonParam->GST_DRA_local_instant_i_frame(0xffff);
	printf("GST_DRA_local_instant_i_frame\n");	

/*	while(1)
	{	
		usleep(100000);

		iCount++;

		printf(" count=%d\n",iCount);
			
		if( g_PreviewIsStart == 1 )
		{				
			printf(" previewRocord right!\n");
			return ALLRIGHT;
		}
		else
		{
			if( iCount > 30 )
			{
				printf(" previewRocord error!\n");
				g_preview_sys_flag = 0;
				return ERROR;
			}
		}
	}
*/
	g_PreviewIsStart = 1;

	printf("previewRocord ok!\n");
	return ERROR;
}

void FTC_set_count_create_keyframe(int count)
{

	DPRINTK("empty func\n");
	return ;
	
	printf(" set_count_create_keyframe %d\n",count);
	set_count_create_keyframe(count);
}

int  FTC_CheckPreviewStatus()
{	
	return FS_CheckPreviewStatus();
}


int FTC_PreviewRecordStop(int IsRecord)
{
	int iCount = 0;
	int rel;

	if( ftc_get_rec_cur_status() != 0 )
	{
		printf(" allready recording can't stop preview!\n");
		return  ERROR;
	}

	g_preview_sys_flag = 0;

	g_pstCommonParam->GST_DRA_start_stop_recording_flag(0);

	printf(" FTC_PreviewRecordStop %d\n",g_preview_sys_flag);
	
	return ALLRIGHT;
}



void  FTC_FileGoToTimePoint(time_t startPlayTime)
{
	g_TimePlayPoint = startPlayTime;
	DPRINTK("g_TimePlayPoint = %ld\n",g_TimePlayPoint);
}


/*
int FTC_GetPlayMode(time_t * startPlayTime,time_t * endPlayTime,time_t * laterTime)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;		
	long betweenTime = 0;;
	
	iDiskNum = DISK_GetDiskNum();


	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
			printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
			
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;
			
				if( g_pstPlaybackModeParam.iPbVideoStandard != pRecPoint->pFilePos->videoStandard )
						continue;
				if( g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize )
						continue;

				if( FTC_CompareTime(*startPlayTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime ))
				{		
					if( pRecPoint->pFilePos->iDataEffect != YES )
						return ERROR;	

				//	printf("standard %d size %d \n",pRecPoint->pFilePos->videoStandard,pRecPoint->pFilePos->imageSize );
				
					//	g_pstPlaybackModeParam.iPbVideoStandard =pRecPoint->pFilePos->videoStandard;
					//	g_pstPlaybackModeParam.iPbImageSize = pRecPoint->pFilePos->imageSize ;
					
					*startPlayTime = pRecPoint->pFilePos->startTime;
					*endPlayTime   = pRecPoint->pFilePos->endTime;	

					PlayCamInfo = pRecPoint->pFilePos->iCam;

					return ALLRIGHT;					
				}		

			    if( pRecPoint->pFilePos->iDataEffect == YES && 
					pRecPoint->pFilePos->startTime > *startPlayTime )
			    	{			    		
			    		
			    		if( betweenTime == 0 )
			    		{
						betweenTime = pRecPoint->pFilePos->startTime - *startPlayTime;
						* laterTime =  pRecPoint->pFilePos->startTime;
					//	g_pstPlaybackModeParam.iPbVideoStandard =pRecPoint->pFilePos->videoStandard;
					//	g_pstPlaybackModeParam.iPbImageSize = pRecPoint->pFilePos->imageSize ;
						PlayCamInfo = pRecPoint->pFilePos->iCam;

			    		}
					else
					{
						if(  pRecPoint->pFilePos->startTime - *startPlayTime < betweenTime )
						{
							betweenTime = pRecPoint->pFilePos->startTime - *startPlayTime;
							* laterTime =  pRecPoint->pFilePos->startTime;
					//		g_pstPlaybackModeParam.iPbVideoStandard =pRecPoint->pFilePos->videoStandard;
					//		g_pstPlaybackModeParam.iPbImageSize = pRecPoint->pFilePos->imageSize ;
							PlayCamInfo = pRecPoint->pFilePos->iCam;

						}
					}
			}
				
				
				
			}
		}
	}
		
	return ERROR;

}
*/


int cp_cam(int cam,int cam2)
{
	int i;	

	for( i = 0 ; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++ )
	{
		if( FTC_GetBit(cam, i) && FTC_GetBit(cam2, i) )
			return 1;
	}

	return 0;
}

// 改用timestick 搜索
int FTC_GetPlayMode(time_t * startPlayTime,time_t * endPlayTime,time_t * laterTime)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECTIMESTICKLOGPOINT * pRecPoint;
	RECTIMESTICKLOGPOINT RecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;		
	long betweenTime = 0;
	int cam;

	cam = *laterTime;
	*laterTime = 0;

	printf("FTC_GetPlayMode cam = %d\n",cam);
	
	iDiskNum = DISK_GetDiskNum();


	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{	
		stDiskInfo = DISK_GetDiskInfo(i);
	
		//printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
			//printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );


			#ifdef USE_PLAYBACK_QUICK_SYSTEM

				if( FTC_Check_disk_time(*startPlayTime,*endPlayTime,i,stDiskInfo.partitionID[j])== 0 )
					continue;
			#endif			

			//DPRINTK(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	
			
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0 ,&RecPoint);
			if( pRecPoint == NULL )
					return ERROR;			
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k ,&RecPoint);

				if( pRecPoint == NULL )
					return ERROR;


				if( pRecPoint->pFilePos->startTime != 0 )
				{
					//	DPRINTK("%d %ld\n",k,pRecPoint->pFilePos->startTime);
				}
			
				//if( g_pstPlaybackModeParam.iPbVideoStandard != pRecPoint->pFilePos->videoStandard )
				//		continue;
				//if( g_pstPlaybackModeParam.iPbImageSize != pRecPoint->pFilePos->imageSize )
				//		continue;

				if( cp_cam(cam,pRecPoint->pFilePos->iCam) == 0 )
					continue;

				if( FTC_CompareTime(*startPlayTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->startTime+300 ) )
				{								

				//	printf("standard %d size %d \n",pRecPoint->pFilePos->videoStandard,pRecPoint->pFilePos->imageSize );
				
					//	g_pstPlaybackModeParam.iPbVideoStandard =pRecPoint->pFilePos->videoStandard;
					//	g_pstPlaybackModeParam.iPbImageSize = pRecPoint->pFilePos->imageSize ;
					
					*startPlayTime = pRecPoint->pFilePos->startTime;
					*endPlayTime   = pRecPoint->pFilePos->startTime+300 ;

					DPRINTK(" starttime = %ld endtime = %ld cam=%d\n",pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->startTime+300,pRecPoint->pFilePos->iCam);

					PlayCamInfo = pRecPoint->pFilePos->iCam;

					return ALLRIGHT;					
				}		

			    if( pRecPoint->pFilePos->startTime > *startPlayTime  )
			    	{			    		
			    		//printf("cp_cam: %x %x\n",cam,pRecPoint->pFilePos->iCam);
			    		if( betweenTime == 0 )
			    		{
			    			
							betweenTime = pRecPoint->pFilePos->startTime - *startPlayTime;
							* laterTime =  pRecPoint->pFilePos->startTime;					
							PlayCamInfo = pRecPoint->pFilePos->iCam;
						//	DPRINTK("1 cp_cam: %x %x  time=%ld\n",cam,pRecPoint->pFilePos->iCam,pRecPoint->pFilePos->startTime);

						

			    		}
					else
					{
						if(  pRecPoint->pFilePos->startTime - *startPlayTime < betweenTime )
						{							
							betweenTime = pRecPoint->pFilePos->startTime - *startPlayTime;
							* laterTime =  pRecPoint->pFilePos->startTime;					
							PlayCamInfo = pRecPoint->pFilePos->iCam;
						//	DPRINTK("2 cp_cam: %x %x  time=%ld\n",cam,pRecPoint->pFilePos->iCam,pRecPoint->pFilePos->startTime);

						}
					}
			}
								
			}
		}
	}
		
	return ERROR;

}


int FTC_ChangeRecordToPlay(int iNextOrLast)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;		
	time_t startPlayTime;

	printf(" iNextOrLast = %d gPlaystart=%d \n",iNextOrLast,gPlaystart);

//	if( gPlaystart != 1 )
//		return ERROR;

	if( iNextOrLast != 0 && iNextOrLast != 1 )
		return ERROR;

	startPlayTime = g_TimePlayStartTime;

	printf(" startPlayTime = %ld\n",startPlayTime);
	
	iDiskNum = DISK_GetDiskNum();


	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
			
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;
	

				if( FTC_CompareTime(startPlayTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime ))
				{		
					printf("startPlayTime = %ld, pFilePos->start =%ld,pFilePos->endTime = %ld\n",
						startPlayTime,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);
					
					if( pRecPoint->pFilePos->iDataEffect != YES )
						return ERROR;	

					if( iNextOrLast == 1 )
					{
						if( k + 1 >= pRecPoint->pUseinfo->id)
						{
							k = 0;
							if( j + 1 >= stDiskInfo.partitionNum)
							{
								j = 1;
								if( i + 1 >= iDiskNum)								
									i = 0;								
								else
									i += 1;									
								
							}
							else
							     j += 1;
						}
						else
						      k += 1;					
						
					}
					else
					{
						if( k - 1 < 0)
						{							
							if( j - 1 < 1 )
							{								
								if( i -1 < 0)								
									i = iDiskNum;								
								else
									i -= 1;	

								stDiskInfo = DISK_GetDiskInfo(i);
								j = stDiskInfo.partitionNum - 1;								
							}
							else
							{
							     j -= 1;								 
							}

								 
							pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
							if( pRecPoint == NULL )
								return ERROR;	

							k = pRecPoint->pUseinfo->id - 1;

							if( k < 0 )
								k = 0;
							
						}
						else
						      k -= 1;
					
					}

					printf("disk = %d,patition = %d, id = %d\n",i,stDiskInfo.partitionID[j] ,k);
					pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );
					if( pRecPoint == NULL )
						return ERROR;

					printf(" pRecPoint->pFilePos->iDataEffect = %d\n",pRecPoint->pFilePos->iDataEffect);

					if( pRecPoint->pFilePos->iDataEffect != YES )
						return ERROR;					
							
				 	FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,g_playFileInfo.iCam,RECORDPLAY);

					return ALLRIGHT;					
				}		
				
				
			}
		}
	}

	printf(" changeRecordToplay error!\n");
	return ERROR;

}

int FTC_PlayModeChange()
{
	if( g_pstPlaybackModeParam.iPbVideoStandard == 0 )
		g_pstPlaybackModeParam.iPbVideoStandard = 1;
	   else
	   	g_pstPlaybackModeParam.iPbVideoStandard = 0;

	   if(g_pstPlaybackModeParam.iPbImageSize == 0)
	   	g_pstPlaybackModeParam.iPbImageSize = 2;
	   else if( g_pstPlaybackModeParam.iPbImageSize == 2 )
	   	g_pstPlaybackModeParam.iPbImageSize =0;

	   return ALLRIGHT;
}

int FTC_AlarmlistPlay(GST_FILESHOWINFO * stPlayFile)
{
	GST_FILESHOWINFO  stPlayFileInfo;
	int iCam;
	int i = 0,j = 0;
	RECORDLOGPOINT * pRecPoint;
	int rel;
	GST_PLAYBACK_VIDEO_PARAM tempParam;
	int iHaveCam = 0;

	printf(" FTC_StopPlay Start\n");
	FTC_StopPlay();
	printf(" FTC_StopPlay End\n");

	printf(" reclog %d,%d,%d\n",stPlayFile->iDiskId,stPlayFile->iPartitionId,stPlayFile->iFilePos);

	pRecPoint = FS_GetPosRecordMessage(stPlayFile->iDiskId,stPlayFile->iPartitionId,stPlayFile->iFilePos);
	if( pRecPoint == NULL )
		return ERROR;

	printf("alarmStart=%ld start=%ld\n",stPlayFile->fileStartTime,pRecPoint->pFilePos->startTime);

	stPlayFileInfo.iDiskId = pRecPoint->iDiskId;
	stPlayFileInfo.iPartitionId = pRecPoint->iPatitionId;
	stPlayFileInfo.iFilePos = pRecPoint->pFilePos->filePos;
	stPlayFileInfo.fileStartTime = stPlayFile->fileStartTime - 10;
	iHaveCam     = pRecPoint->pFilePos->iCam;

	if( stPlayFile->iPlayCam == 0x0f)
		iCam		= stPlayFile->iCam;
	else
		iCam		= stPlayFile->iPlayCam;

	if( stPlayFileInfo.fileStartTime >= pRecPoint->pFilePos->startTime)
	{
		rel = FTC_SetTimeToPlay(stPlayFileInfo.fileStartTime,stPlayFile->fileEndTime, iCam,RECORDPLAY);
	}else
	{
		rel = FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,stPlayFile->fileEndTime, iCam,RECORDPLAY);
	}
	return rel;

/*
	g_pstPlaybackModeParam.iPbImageSize  = pRecPoint->pFilePos->imageSize;
	g_pstPlaybackModeParam.iPbVideoStandard = pRecPoint->pFilePos->videoStandard;


	// 转成dsp 那边识别的制式
	tempParam.iPbVideoStandard  = g_pstPlaybackModeParam.iPbVideoStandard;
	tempParam.iPbImageSize = g_pstPlaybackModeParam.iPbImageSize ;

	switch(tempParam.iPbImageSize )
	{
		case TD_DRV_VIDEO_SIZE_CIF:
			tempParam.iPbImageSize  = 3;
				break;
		case TD_DRV_VIDEO_SIZE_HD1:
			tempParam.iPbImageSize = 2;
				break;
		case TD_DRV_VIDEO_SIZE_D1:
			tempParam.iPbImageSize  = 1;
				break;	
		default:
			printf(" stFrameHeaderInfo.imageSize = %d error!\n",tempParam.iPbImageSize );
				return ERROR;
				break;
	}	

	if(tempParam.iPbVideoStandard == TD_DRV_VIDEO_STARDARD_NTSC )
		tempParam.iPbVideoStandard = 2;
	else
		tempParam.iPbVideoStandard = 1;

	printf(" FTC_ClearAllChannel start !\n");
	rel = FTC_ClearAllChannel(tempParam.iPbImageSize,tempParam.iPbVideoStandard);
	if( rel < 0 )
	{
		printf(" FTC_ClearAllChannel error!\n");
		return ERROR;
	}


	// 保正HD1, D1 播放时，只有一个通道 可播
	if( g_pstPlaybackModeParam.iPbImageSize != TD_DRV_VIDEO_SIZE_CIF)
	{
		j = 0;
	
		for(i = 0; i < 16; i++)
		{
			if( FTC_GetBit(iCam, i ) == 1 && FTC_GetBit(iHaveCam,i) == 1)
			{
				j = FTC_SetBit(j,i);
				printf("Set play channel = %d   j=%d\n",i,j);				
				break;
			}
		}

		iCam = j;
	
	}

	if( iCam == 0 )
	{
		printf("iCam = %d  no play channel\n",iCam);
		return ERROR;
	}

	stPlayFileInfo.iCam = iCam;

	g_TimePlayEndTime = 0;
	g_TimePlayStartTime = stPlayFileInfo.fileStartTime;

	FTC_FileGoToTimePoint(g_TimePlayStartTime);
					
	// 让CIF  的回放可以画面放大
	g_pstCommonParam->GST_MENU_set_current_play_mode(g_pstPlaybackModeParam.iPbVideoStandard,g_pstPlaybackModeParam.iPbImageSize);
				
	FTC_SetPlayFileInfo(&stPlayFileInfo);					
		printf("FTC_SetPlayFileInfo!\n");	

	return ALLRIGHT;
	*/
	
}

int FTC_ReclistPlay(GST_FILESHOWINFO * stPlayFile)
{
	GST_FILESHOWINFO  stPlayFileInfo;
	int iCam;
	int i = 0,j = 0;
	int rel;
	int iHaveCam = 0;
	GST_PLAYBACK_VIDEO_PARAM tempParam;

	printf("event %d,%d,%d playcam=%d \n",stPlayFile->event.loss,stPlayFile->event.motion,
		stPlayFile->event.sensor,stPlayFile->iPlayCam);

	printf(" reclog %d,%d,%d  starttime=%ld\n",stPlayFile->iDiskId,stPlayFile->iPartitionId,
		stPlayFile->iFilePos, stPlayFile->fileStartTime);

	// 表示是报警列表回放
	if( stPlayFile->event.loss != 0 || stPlayFile->event.motion != 0  ||
	 	stPlayFile->event.sensor != 0)
	{
		printf("here 1\n");
		rel = FTC_AlarmlistPlay(stPlayFile);
		return rel;
	}

	// 往下走是录相列表回放
	printf(" FTC_StopPlay Start\n");
	FTC_StopPlay();
	printf(" FTC_StopPlay End\n");

	
	stPlayFileInfo.iDiskId = stPlayFile->iDiskId;
	stPlayFileInfo.iPartitionId = stPlayFile->iPartitionId;
	stPlayFileInfo.iFilePos  = stPlayFile->iFilePos;
	stPlayFileInfo.fileStartTime = stPlayFile->fileStartTime;
	iHaveCam = stPlayFile->iCam;
	iCam     = stPlayFile->iPlayCam;	
	
//	g_pstPlaybackModeParam.iPbImageSize  = stPlayFile->imageSize;
//	g_pstPlaybackModeParam.iPbVideoStandard = stPlayFile->videoStandard;

	// 转成dsp 那边识别的制式

	/*tempParam.iPbVideoStandard  = g_pstPlaybackModeParam.iPbVideoStandard;
	tempParam.iPbImageSize = g_pstPlaybackModeParam.iPbImageSize ;

	switch(tempParam.iPbImageSize )
	{
		case TD_DRV_VIDEO_SIZE_CIF:
			tempParam.iPbImageSize  = 3;
				break;
		case TD_DRV_VIDEO_SIZE_HD1:
			tempParam.iPbImageSize = 2;
				break;
		case TD_DRV_VIDEO_SIZE_D1:
			tempParam.iPbImageSize  = 1;
				break;	
		default:
			printf(" stFrameHeaderInfo.imageSize = %d error!\n",tempParam.iPbImageSize );
				return ERROR;
				break;
	}	

	if(tempParam.iPbVideoStandard == TD_DRV_VIDEO_STARDARD_NTSC )
		tempParam.iPbVideoStandard = 2;
	else
		tempParam.iPbVideoStandard = 1;
		*/

/*
	printf(" FTC_ClearAllChannel start !\n");
	rel = FTC_ClearAllChannel(tempParam.iPbImageSize,tempParam.iPbVideoStandard);
	if( rel < 0 )
	{
		printf(" FTC_ClearAllChannel error!\n");
		return ERROR;
	}
	*/


	// 保正HD1, D1 播放时，只有一个通道 可播
/*	if( g_pstPlaybackModeParam.iPbImageSize != TD_DRV_VIDEO_SIZE_CIF)
	{
		j = 0;
	
		for(i = 0; i < 16; i++)
		{
			if( FTC_GetBit(iCam, i ) == 1&& FTC_GetBit(iHaveCam,i) == 1 )
			{
				j = FTC_SetBit(j,i);
				printf("Set play channel = %d   j=%d\n",i,j);				
				break;
			}
		}

		iCam = j;
	
	}
	*/
	
	if( iCam == 0 )
	{
		printf("iCam = %d  no play channel\n",iCam);
		return ERROR;
	}

	stPlayFileInfo.iCam = iCam;

	net_file_send(0);
	if(stPlayFile->iLaterTime == 0xff )  //网络文件备份时用
	{
		net_file_send(1);
		g_TimePlayEndTime = stPlayFile->fileEndTime;
	}
	else		
	{
		g_TimePlayEndTime = 0;
	}
	
	g_TimePlayStartTime = stPlayFileInfo.fileStartTime;

	FTC_FileGoToTimePoint(g_TimePlayStartTime);
					
	// 让CIF  的回放可以画面放大
	//g_pstCommonParam->GST_MENU_set_current_play_mode(g_pstPlaybackModeParam.iPbVideoStandard,g_pstPlaybackModeParam.iPbImageSize);
				
	printf("file %d,%d,%d\n",stPlayFileInfo.iDiskId,stPlayFileInfo.iPartitionId,stPlayFileInfo.iFilePos);


	FTC_SetPlayFileInfo(&stPlayFileInfo);					
		printf("FTC_SetPlayFileInfo!\n");	

	return ALLRIGHT;
}

time_t g_LastStartPlayTime = 0;
int g_LastStartPlayCam = 0;
int      g_LastPlayFileIdInArray = 0;

int FTC_SetTimeToPlay(time_t startPlayTime,time_t endPlayTime, int iCam,int playStyle)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	GST_FILESHOWINFO  stPlayFileInfo;
	GST_PLAYBACK_VIDEO_PARAM tempParam;
	time_t startTime,endTime,laterTime;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	 struct timeval nowTime;	 
	 int iGetPlayModError = 0;
	 int net_backup_file = 0;
	GST_PLAY_FILE_ARRAY play_file_array;
	int playTimeOffset = 0;
	 
	 net_backup_file = playStyle /10;
	 playStyle = playStyle %10;

	// iCam = 0x02;

	//playStyle = TIMEPOINTPLAY;

	printf("startPlayTime = %ld,endPlayTime=%ld,iCam = %d playStyle=%d\n",
			startPlayTime,endPlayTime,iCam,playStyle);

	//printf(" FTC_StopPlay Start\n");
	rel = FTC_StopPlay();
	printf(" FTC_StopPlay End,rel = %d\n",rel);

	// 让回放速度正常
	FTC_PlayFast(0);

	startTime = startPlayTime;	
	laterTime = iCam;

	rel = FTC_GetPlayMode(&startTime,&endTime,&laterTime);
	if( rel < 0 )
	{
		if(playStyle == TIMEPOINTPLAY || playStyle == RECORDPLAY)
		{
			if( laterTime != 0 )
			{
				startPlayTime = laterTime;
				printf(" change time point! laterTime=%ld\n",laterTime);
			}
			else
			{
				printf("No record!\n");
				return ERROR;
			}
		}
		else
		{		
			printf("FTC_GetPlayMode error!\n");			
			iGetPlayModError = 1;
		}
	}	


	// 转成dsp 那边识别的制式
	tempParam.iPbVideoStandard  = g_pstPlaybackModeParam.iPbVideoStandard;
	tempParam.iPbImageSize = g_pstPlaybackModeParam.iPbImageSize ;

	switch(tempParam.iPbImageSize )
	{
		case TD_DRV_VIDEO_SIZE_CIF:
			tempParam.iPbImageSize  = 3;
				break;
		case TD_DRV_VIDEO_SIZE_HD1:
			tempParam.iPbImageSize = 2;
				break;
		case TD_DRV_VIDEO_SIZE_D1:
			tempParam.iPbImageSize  = 1;
				break;	
		default:
			printf(" stFrameHeaderInfo.imageSize = %d error!\n",tempParam.iPbImageSize );
				return ERROR;
				break;
	}	

	if(tempParam.iPbVideoStandard == TD_DRV_VIDEO_STARDARD_NTSC )
		tempParam.iPbVideoStandard = 2;
	else
		tempParam.iPbVideoStandard = 1;


	if(iGetPlayModError == 1 )
	{			
		printf("FTC_GetPlayMode error!\n");			
		return ERROR;
	}	


	if( iCam == 0 )
	{
		printf("iCam = %d  no play channel\n",iCam);
		return ERROR;
	}

//	printf(" PlayBack VideoStandard=%d ImageSize=%d\n",g_pstPlaybackModeParam.iPbVideoStandard,
//							g_pstPlaybackModeParam.iPbImageSize);

	 printf("FTC_SetTimeToPlay starttime = %ld,endtime = %ld, iCam=%d\n",startPlayTime,endPlayTime,iCam);

	DISK_GetAllDiskInfo();	
			
	iDiskNum = DISK_GetDiskNum();
	memset(&play_file_array,0x00,sizeof(GST_PLAY_FILE_ARRAY));
	
time_add_offset_search:
	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = 0; i < iDiskNum; i++ )
	{
		if( FS_DiskFormatIsOk(i) == 0 )
		{
			printf(" FS_DiskFormatIsOk = %d\n",FS_DiskFormatIsOk(i) );
			continue;
		}
		stDiskInfo = DISK_GetDiskInfo(i);

		
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j-- )
		{

			#ifdef USE_PLAYBACK_QUICK_SYSTEM

				if( FTC_Check_disk_time(startPlayTime,endPlayTime,i,stDiskInfo.partitionID[j])== 0 )
					continue;
			#endif			

			

		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
			{
					printf(" FS_GetPosFileInfo eror!\n");
					return ERROR;
			}
			
			for(k = pFileInfoPoint->pUseinfo->iTotalFileNumber - 1; k >=0 ; k--)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
				{
					printf("11 FS_GetPosFileInfo eror!\n");
					return ERROR;
				}

				//if(pFileInfoPoint->pFilePos->StartTime != 0)
				//printf("k=%d starttime=%ld endtime=%ld startPlayTime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,
				//		pFileInfoPoint->pFilePos->EndTime,startPlayTime);

				if( FTC_CompareTime(startPlayTime,pFileInfoPoint->pFilePos->StartTime-playTimeOffset,pFileInfoPoint->pFilePos->EndTime) )
				{
					
					if( (pFileInfoPoint->pFilePos->iDataEffect == NOEFFECT)  && 
						(pFileInfoPoint->pFilePos->states != WRITING) )
					{
						DPRINTK(" this is NoEffect!\n");
			//			FS_SetErrorMessage(CANTFINDFILE, 0);
						return ERROR;
					}
				
					if( pFileInfoPoint->pFilePos->states != WRITEOVER && pFileInfoPoint->pFilePos->states != WRITING 
							&& pFileInfoPoint->pFilePos->states != PLAYING)
					{
						DPRINTK("11 This file is used by other! stats =%d\n",pFileInfoPoint->pFilePos->states);
				//		FS_SetErrorMessage(CANTFINDFILE, 0);
						return ERROR;
					}		
					
				  	printf("k=%d starttime=%ld endtime=%ld startPlayTime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,
						pFileInfoPoint->pFilePos->EndTime,startPlayTime);

					stPlayFileInfo.iDiskId = pFileInfoPoint->iDiskId;
					stPlayFileInfo.iPartitionId = pFileInfoPoint->iPatitionId;
					stPlayFileInfo.iFilePos  = pFileInfoPoint->iCurFilePos;
					stPlayFileInfo.iCam     = iCam;

					if( play_file_array.iPlayFileNum >= MAX_PLAY_FILE_ARRAY_ITEM_NUM )
						continue;

					DPRINTK("[%d]  %d,%d,%d \n",play_file_array.iPlayFileNum,stPlayFileInfo.iDiskId,stPlayFileInfo.iPartitionId,stPlayFileInfo.iFilePos);

					play_file_array.file[play_file_array.iPlayFileNum] = stPlayFileInfo;
					play_file_array.iPlayFileNum++;
					
					
				}

				//保证正在录象的文件能被播放
				if( pFileInfoPoint->pFilePos->StartTime != 0 )
				{
					
			//	if( (pFileInfoPoint->pFilePos->EndTime == 0) && ( k == g_stLogPos.iCurFilePos ) )
				if( pFileInfoPoint->pFilePos->states == WRITING )
				{
					struct timeval mytime;

					g_pstCommonParam->GST_DRA_get_sys_time( &mytime, NULL );

					if( FTC_CompareTime(startPlayTime,pFileInfoPoint->pFilePos->StartTime,mytime.tv_sec) )
					{

						DPRINTK("this file is writting:(%ld,%ld) %ld  \n",
							pFileInfoPoint->pFilePos->StartTime,mytime.tv_sec,startPlayTime);
						
						if( (pFileInfoPoint->pFilePos->iDataEffect == NOEFFECT)  && 
							(pFileInfoPoint->pFilePos->states != WRITING) )
						{
							DPRINTK(" this is NoEffect!\n");
				//			FS_SetErrorMessage(CANTFINDFILE, 0);
							return ERROR;
						}
					
						if( pFileInfoPoint->pFilePos->states != WRITEOVER && pFileInfoPoint->pFilePos->states != WRITING 
								&& pFileInfoPoint->pFilePos->states != PLAYING)
						{
							DPRINTK("11 This file is used by other! stats =%d\n",pFileInfoPoint->pFilePos->states);
					//		FS_SetErrorMessage(CANTFINDFILE, 0);
							return ERROR;
						}		
						
					  	printf("k=%d starttime=%ld endtime=%ld startPlayTime=%ld\n",k, pFileInfoPoint->pFilePos->StartTime,
							pFileInfoPoint->pFilePos->EndTime,startPlayTime);

						stPlayFileInfo.iDiskId = pFileInfoPoint->iDiskId;
						stPlayFileInfo.iPartitionId = pFileInfoPoint->iPatitionId;
						stPlayFileInfo.iFilePos  = pFileInfoPoint->iCurFilePos;
						stPlayFileInfo.iCam     = iCam;					
						

						if( playStyle == ALARMRECORDPLAY )
						{
							//g_TimePlayEndTime = endTime;
							g_TimePlayEndTime = 0;
							g_TimePlayStartTime = startTime;
							 printf("alarm starttime = %ld,endtime = %ld, iCam=%d",startTime,endTime,iCam);
						}
						else if( playStyle == TIMEPOINTPLAY )
						{
							g_TimePlayEndTime = 0;
							g_TimePlayStartTime = startPlayTime;						
						}
						else if( playStyle == RECORDPLAY )
						{
							g_TimePlayEndTime = endPlayTime;
							//g_TimePlayEndTime = 0;
							g_TimePlayStartTime = startPlayTime;
						}
						else
						{
							printf(" playStyle = %d error!\n",playStyle);
							return ERROR;
						}

						FTC_FileGoToTimePoint(g_TimePlayStartTime);				
						
						// 让CIF  的回放可以画面放大
					//	g_pstCommonParam->GST_MENU_set_current_play_mode(g_pstPlaybackModeParam.iPbVideoStandard,g_pstPlaybackModeParam.iPbImageSize);
						if( net_backup_file == 1 )
							net_file_send(1);
						else
							net_file_send(0);
					
						 rel = FTC_SetPlayFileInfo(&stPlayFileInfo);					
						printf("FTC_SetPlayFileInfo! old search %d\n",rel);					

						return rel;																	
					}	

				}
				}

				
			}
		}
	}

	//当有一段录像只有一分钟并假设录像开始时间为 01:12   ,
	//设置播放时间为01:11 时，就播放不出录像，做下面的处理解决这个问题。
	if( play_file_array.iPlayFileNum == 0 && playTimeOffset == 0)
	{
		playTimeOffset = 300;
		DPRINTK("Not search rec file,so set playTimeOffset = 300\n");
		goto time_add_offset_search;
	}


	if( play_file_array.iPlayFileNum > 0 )
	{
		if( playStyle == ALARMRECORDPLAY )
		{			
			g_TimePlayEndTime = 0;
			g_TimePlayStartTime = startTime;
			 printf("alarm starttime = %ld,endtime = %ld, iCam=%d",startTime,endTime,iCam);
		}
		else if( playStyle == TIMEPOINTPLAY )
		{
			g_TimePlayEndTime = 0;
			g_TimePlayStartTime = startPlayTime;						
		}
		else if( playStyle == RECORDPLAY )
		{
			g_TimePlayEndTime = endPlayTime;			
			g_TimePlayStartTime = startPlayTime;
		}
		else
		{
			printf(" playStyle = %d error!\n",playStyle);
			return ERROR;
		}

		if( g_LastStartPlayTime == startPlayTime  &&  g_LastStartPlayCam == iCam)
		{
			g_LastPlayFileIdInArray++;
			if( g_LastPlayFileIdInArray >=  play_file_array.iPlayFileNum )
				g_LastPlayFileIdInArray = 0;
		}else
		{
			g_LastStartPlayTime = startPlayTime;
			g_LastStartPlayCam = iCam;
			g_LastPlayFileIdInArray = 0;
		}

		FTC_FileGoToTimePoint(g_TimePlayStartTime);						
		
		if( net_backup_file == 1 )
			net_file_send(1);
		else
			net_file_send(0);

		stPlayFileInfo = play_file_array.file[g_LastPlayFileIdInArray];
		
		 rel = FTC_SetPlayFileInfo(&stPlayFileInfo);					
		printf("FTC_SetPlayFileInfo! old search %d\n",rel);					

		return rel;			
	}


	printf(" Can't Find Play file!\n");
	return ERROR;
}


int FTC_SetTimeToPlay_lock(time_t startPlayTime,time_t endPlayTime, int iCam,int playStyle)
{
	int rel;

	FS_PlayMutexLock();	
	DPRINTK("g_play_lock lock!\n");

	rel = FTC_SetTimeToPlay(startPlayTime,endPlayTime, iCam,playStyle);

	DPRINTK("g_play_lock unlock!\n");	
	FS_PlayMutexUnLock();

	return rel;
}

int FTC_PrintMemoryInfo()
{
	char cmd[]= "cat /proc/meminfo";

	command_ftc(cmd);    // mkdir /tddvr/hda5

	return ALLRIGHT;
}

int FTC_AudioRecCam(int iCam)
{
	if( GETRECCAM == iCam )
	{
		return g_RecAudioCam;
	}

	g_RecAudioCam = iCam;

	printf("audio channel  rec = %d\n",g_RecAudioCam);
	return ALLRIGHT;
}

int FTC_AudioPlayCam(int iCam)
{
	int i = 0;

	for(i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1); i++)
	{
		if( FTC_GetBit(iCam, i ) == 1 )
		{
			g_AudioChannel = i;
			printf("g_AudioChannel =%d iCam = %d\n",g_AudioChannel,iCam);
			return ALLRIGHT;
		}
	}

	g_AudioChannel = -1;
	
	printf("g_AudioChannel =%d iCam = %d\n",g_AudioChannel,iCam);
	return ALLRIGHT;
}

int FTC_PlayFast(int iSpeed)
{
	static int old_speed = 0;
	time_t playtime;
	int cam;
	int ret;
	int i;
	int cam_num=0;
	printf("FTC_PlayFast iSpeed=%d\n",iSpeed);


	if( (iSpeed == 1) &&(old_speed != 8 ))
	{
		ret = FTC_GetCurrentPlayInfo(&playtime,&cam);
		if( ret < 0 )
		{
			DPRINTK("FTC_GetCurrentPlayInfo error\n");
			return ERROR;
		}

		for( i = 0; i < g_pstCommonParam->GST_DRA_get_dvr_max_chan(1);i++)
		{
			if( FTC_GetBit(cam,i) )
				cam_num++;
		}

		DPRINTK("cam_num = %d\n",cam_num);

		if( cam_num >  1 )
		{
			iSpeed = 3;
			DPRINTK("change iSpeed = %d\n",iSpeed);
		}

	}
	
	switch(iSpeed)
	{
		case 0:
			g_PlayStatus = PLAYNORMAL;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 1:
			g_PlayStatus = PLAYFASTLOWSPEED;
			if( old_speed != 8 )
			{			
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(2);
				old_speed = 8;
			}
			break;
		case 2:
			g_PlayStatus = PLAYFASTHIGHSPEED;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 3:
			g_PlayStatus = PLAYFASTLOW2SPEED;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 4:
			g_PlayStatus = PLAYFASTHIGHSPEED_2;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 5:
			g_PlayStatus = PLAYFASTHIGHSPEED_4;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 6:
			g_PlayStatus = PLAYFASTHIGHSPEED_8;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 7:
			g_PlayStatus = PLAYFASTHIGHSPEED_16;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		case 8:
			g_PlayStatus = PLAYFASTHIGHSPEED_32;
			if( old_speed != 1 )
			{
				g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);
				old_speed = 1;
			}
			break;
		default:
			break;
	}
	
	return ALLRIGHT;
}

void FTC_play_fast_search(int flag)
{
	printf("FTC_play_fast_search flag=%d\n",flag);
	if( flag == 1 )
		g_PlayStatus = PLAYFASTSEARCH;
	else
		g_PlayStatus = PLAYNORMAL;
}

int FTC_PlayBack(int iSpeed)
{
	printf("FTC_PlayBack iSpeed=%d\n",iSpeed);

	g_pstCommonParam->GST_DRA_Hisi_set_vo_framerate(1);

	switch(iSpeed)
	{
		case 0:
			g_PlayStatus = PLAYNORMAL;		
			break;
		case 1:
			g_PlayStatus = PLAYBACKFASTLOWSPEED;			
			break;
		case 2:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED;		
			break;
		case 4:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED_2;		
			break;
		case 5:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED_4;		
			break;
		case 6:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED_8;		
			break;
		case 7:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED_16;		
			break;
		case 8:
			g_PlayStatus = PLAYBACKFASTHIGHSPEED_32;		
			break;
		default:
			break;
	}		

	return ALLRIGHT;
}

int FTC_PlayPause(int iStop)
{
	printf("FTC_PlayPause iStop=%d\n",iStop);
	switch(iStop)
	{
		case 0:
			g_PlayStatus = PLAYNORMAL;
			break;
		case 1:
			g_PlayStatus = PLAYPAUSE;
			break;		
		default:
			break;
	}
	return ALLRIGHT;
}

int FTC_PlaySingleFrame(int iFlag)
{
	printf("FTC_PlaySingleFrame iFlag=%d\n",iFlag);
	switch(iFlag)
	{
		case 0:
			g_PlayStatus = PLAYNORMAL;
			break;
		case 1:
			g_PlayStatus = PLAYLATERFRAME;
			break;		
		case 2:
			g_PlayStatus = PLAYLASTFRAME;
			break;
		default:
			break;
	}

	return 1;
}

int FTC_PlayChangFile(int iFlag)
{
	printf("FTC_PlayChangFile iFlag=%d\n",iFlag);
	switch(iFlag)
	{
		case 0:
			g_PlayStatus = PLAYLASTFILE;
			break;
		case 1:
			g_PlayStatus = PLAYNEXTFILE;
			break;
		default:
			break;
	}
}

int FTC_GetRecMode(int iVideoStandard,int iVideoSize)
{
	g_pstRecModeParam.iPbImageSize = iVideoSize;
	g_pstRecModeParam.iPbVideoStandard = iVideoStandard;

	DPRINTK("not change rec mode=%d size=%d\n",iVideoStandard,iVideoSize);
	return ALLRIGHT;
}

int FTC_CurrentPlayMode(int * iVideoStandard,int * iVideoSize)
{
	g_pstPlaybackModeParam.iPbVideoStandard = *iVideoStandard ;
	 g_pstPlaybackModeParam.iPbImageSize = *iVideoSize;

	 DPRINTK("mode=%d size=%d\n",*iVideoStandard,*iVideoSize);

	 fs_set_play_mode(g_pstPlaybackModeParam.iPbImageSize,g_pstPlaybackModeParam.iPbVideoStandard); 

	// 复修文件系统
	FTC_FixAllDisk();
	printf(" have FTC_FixAllDisk\n");

	FTC_check_fileinfo_timestick_lock();
	printf("FTC_check_fileinfo_timestick_lock \n");

	g_RecMode = g_pstRecModeParam;

	#ifdef USE_DISK_SLEEP_SYSTEM	
	Disk_sleep_sys_ini();
	#endif
	
	//得到当前硬盘的操作分区
	FS_GetOpreateDiskPartition();	
	
	return ALLRIGHT;
}

int FTC_CurrentDiskWriteAndReadStatus()
{
	int iStatus = 0;

	int iWrite = 0;
	int iRead = 0;

	// usb的状态
	if( FS_UsbWriteStatus() == 1 )
		iWrite = 1;

	// 生成文件线程的状态
	if( FS_CreateFileStatus() == 0)
		iWrite = 1;
	
	if(  gRecstart == 1 )
		iWrite = 1;

	if( gPlaystart == 1 )
		iRead = 2;

	return iRead+iWrite;
	
}

int FTC_GetRecImageSize()
{
	return g_pstRecModeParam.iPbImageSize;
}

int FTC_GetRecStandard()
{
	return g_pstRecModeParam.iPbVideoStandard;
}


int FTC_PlayNearRecFile(time_t * CurrentPlayTime)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;
	#ifdef TAILAND_PLAYBACK
	int playcam = 0x0f;
	#else
	int playcam = 0x01;
	#endif

	FTC_PrintMemoryInfo();

	// 纠正网络文件传送后播放时间控制无效
	net_file_send(0);
	
	iDiskNum = DISK_GetDiskNum();

//	printf(" iDiskNum = %d\n",iDiskNum);

	rel = FS_GetRecCurrentPoint(&stRecPos);
	if( rel < 0 )
	{
		printf("FS_GetRecCurrentPoint error!\n");
		return ALLRIGHT;
	}

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{
		if(i > stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

			if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j] > stRecPos.iPartitionId )
				continue;
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	

				if(k > stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

				// if( FTC_GetBit(pRecPoint->pFilePos->iCam, 0 ) == 0  )
				// 	continue;
				

		//		printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);
					
				if( pRecPoint->pFilePos->iDataEffect == YES )
				{				
					// 找到了最近的一条记录，开始回放
					//rel = FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,pRecPoint->pFilePos->iCam,RECORDPLAY);
					rel = FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,playcam,TIMEPOINTPLAY);
					if( rel < 0 )
					{
						printf(" FTC_SetTimeToPlay error rel =%d\n",rel);
						return ERROR;
					}		
					
					*CurrentPlayTime = pRecPoint->pFilePos->startTime;

					return ALLRIGHT;
				}				
				
			}
		}
	}
		

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum - 1; i >=  0; i-- )
	{
		if(i <  stRecPos.iDiskId )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
	//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
	//		if( stDiskInfo.partitionID[j] <stFileShowInfo->iPartitionId  &&  i == stFileShowInfo->iDiskId)
	//				continue;

		if( i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  < stRecPos.iPartitionId )
				continue;
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{	
				if(k <= stRecPos.iCurFilePos && i == stRecPos.iDiskId  && stDiskInfo.partitionID[j]  == stRecPos.iPartitionId )
					continue;
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

				 if( FTC_GetBit(pRecPoint->pFilePos->iCam, 0 ) == 0  )
				 	continue;

			//	printf(" iDiskId=%d   iPartitionId =%d  iFilePos=%d\n",i,stDiskInfo.partitionID[j] , k );

			//	printf("k=%d starttime=%ld endtime=%ld\n",k, pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime);
				if( pRecPoint->pFilePos->iDataEffect == YES )
				{				
					// 找到了最近的一条记录，开始回放
					//rel = FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,pRecPoint->pFilePos->iCam,RECORDPLAY);
					rel = FTC_SetTimeToPlay(pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,playcam,RECORDPLAY);
					if( rel < 0 )
					{
						printf(" FTC_SetTimeToPlay error rel =%d\n",rel);
						return ERROR;
					}

					*CurrentPlayTime = pRecPoint->pFilePos->startTime;

					return ALLRIGHT;
				}	
			}
		}
	}

	

	printf("NO REC File!\n");
	return ERROR;
}

int FTC_get_cover_rate(int disk_id)
{
	int rel;

	printf(" FTC_get_cover_rate in\n");

	FS_LogMutexLock();
	
	rel =FS_get_cover_rate(disk_id);
		
	FS_LogMutexUnLock();

	printf(" FTC_get_cover_rate out  rel =%d\n",rel);	
	
	return rel;
	
}

int FTC_GetBadBlockSize(int iDiskId)
{
	int rel;

	printf(" FTC_GetBadBlockSize in\n");

	FS_LogMutexLock();
	
	rel =FS_GetBadBlockSize(iDiskId);
		
	FS_LogMutexUnLock();

	printf(" FTC_GetBadBlockSize out   rel =%d\n",rel);
	
	return rel;
}

int FTC_get_disk_remain_rate(int disk_id)
{
	int rel;
	
	printf(" FTC_get_disk_remain_rate in\n");

	FS_LogMutexLock();
	
	rel =FS_get_disk_remain_rate(disk_id);
		
	FS_LogMutexUnLock();

	printf(" FTC_get_disk_remain_rate out   rel =%d\n",rel);
	
	return rel;
}

int FTC_ON_GetRecordlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int rel;

	FS_LogMutexLock();
	
	//rel =FTC_GetRecordlist( stFileShowInfo,  iCam,  searchStartTime,  searchEndTime,iPage);

	rel =FTC_GetRecordTimeSticklist( stFileShowInfo,  iCam,  searchStartTime,  searchEndTime,iPage);
		
	FS_LogMutexUnLock();
	
	return rel;
}

int FTC_ON_GetAlarmEventlist(GST_FILESHOWINFO * stFileShowInfo, int iCam, time_t searchStartTime, time_t searchEndTime,int iPage)
{
	int rel;

	FS_LogMutexLock();
	
	rel =FTC_GetAlarmEventlist( stFileShowInfo,  iCam,  searchStartTime,  searchEndTime,iPage);
		
	FS_LogMutexUnLock();
	
	return rel;
}

int FTC_GetPlayCurrentStauts(int * status, int *iPlayStopFlag)
{
	*status = g_PlayStatus;
	*iPlayStopFlag   = 0;

	//多路回放一倍快进的处理
	if( g_PlayStatus == PLAYFASTLOW2SPEED )
	{
		*status = PLAYFASTLOWSPEED;
	}

	if( PlayStopStatus )
	{
		printf("FTC_GetPlayCurrentStauts status=1 \n");
		*iPlayStopFlag   = 1;
		PlayStopStatus = 0;
	}
	
	return ALLRIGHT;
}

int FTC_SetPlayStopStauts()
{
	printf("FTC_SetPlayStopStauts set 1 \n");
	PlayStopStatus = 1;
	return ALLRIGHT;
}

int FTC_GetCurrentPlayInfo(time_t * startTime,int * iCam)
{
	*startTime = 0;
	*iCam = 0;

	if( gPlaystart != 1 )
		return ERROR;

	*startTime = PlayStartTime;
	*iCam = g_playFileInfo.iCam;

	printf(" current Play start time = %d,iCam = %d\n",PlayStartTime,g_playFileInfo.iCam);

	return ALLRIGHT;	
}

int FTC_SetRecDuringTime(int iSecond)
{
	if( iSecond < 60*15 || iSecond > 60*120 )
		return ERROR;

	g_RecDuringTime = iSecond;
	return ALLRIGHT;
}

/*
int FTC_get_disk_rec_start_end_time(time_t * start_time,time_t * end_time,int disk_id)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT * pRecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;

	*start_time = 0;
	*end_time = 0;
	
	iDiskNum = DISK_GetDiskNum();

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{
		if(i != disk_id )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{
			
			pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{				
				
				pRecPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , k );

				if( pRecPoint == NULL )
					return ERROR;

				if( pRecPoint->pFilePos->iDataEffect != YES )
					continue;
	
				if( *start_time == 0 && pRecPoint->pFilePos->startTime > 0 )
				{
					*start_time = pRecPoint->pFilePos->startTime;
				}else
				{
					*start_time =  *start_time > pRecPoint->pFilePos->startTime ? pRecPoint->pFilePos->startTime : *start_time;
				}

				*end_time =  *end_time < pRecPoint->pFilePos->endTime ? pRecPoint->pFilePos->endTime : *end_time;			
				
				
			}
		}
	}
		
	return ALLRIGHT;
		
}
*/

//改用timestick 查询
int FTC_get_disk_rec_start_end_time(time_t * start_time,time_t * end_time,int disk_id)
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECTIMESTICKLOGPOINT * pRecPoint;
	RECTIMESTICKLOGPOINT  RecPoint;
	int iInfoNumber = 0;
	int tempNumber = 0;
	FILEINFOPOS stRecPos;
	int rel;

	*start_time = 0;
	*end_time = 0;
	
	iDiskNum = DISK_GetDiskNum();


	#ifdef USE_PLAYBACK_QUICK_SYSTEM

	FTC_GetEveryDiskRecTime();

	FTC_disk_rec_time(start_time, end_time,disk_id);

	DPRINTK("start: %d  %d\n",*start_time,*end_time);
		
	return ALLRIGHT;

	#endif

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i =  iDiskNum - 1; i >=  0; i-- )
	{
		if(i != disk_id )
			continue;
	
		stDiskInfo = DISK_GetDiskInfo(i);
	
//		printf(" i = %d FS_DiskFormatIsOk(i) =%d partitionNum =%d\n",i,FS_DiskFormatIsOk(i),stDiskInfo.partitionNum );
		if( FS_DiskFormatIsOk(i) == 0 )
			continue;
	
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
		{		
			
			pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0 ,&RecPoint);
			if( pRecPoint == NULL )
					return ERROR;
			
			for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
			{				
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k ,&RecPoint);

				if( pRecPoint == NULL )
					return ERROR;
			
				if( pRecPoint->pFilePos->startTime <= 600 )
					continue;			

				if( (*start_time == 0) && (pRecPoint->pFilePos->startTime > 0) )
				{
					*start_time = pRecPoint->pFilePos->startTime;
				}

				if( *start_time > pRecPoint->pFilePos->startTime )
				{
					*start_time = pRecPoint->pFilePos->startTime;
				}

				if( *end_time < (pRecPoint->pFilePos->startTime + 300) )
				{
					*end_time = pRecPoint->pFilePos->startTime + 300;
				}				
				
			}
		}
	}


	printf("start: %d  %d\n",*start_time,*end_time);

		
	return ALLRIGHT;
		
}


int FTC_get_disk_rec_start_end_time_lock(time_t * start_time,time_t * end_time,int disk_id)
{	
	int rel;

	FS_LogMutexLock();

	rel = FTC_get_disk_rec_start_end_time(start_time,end_time,disk_id);

	FS_LogMutexUnLock();

	return rel;
}

int FTC_get_cur_play_cam()
{
	return g_playFileInfo.iCam;
}


int FTC_check_fileinfo_timestick()
{
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	GST_FILESHOWINFO  stPlayFileInfo;
	GST_PLAYBACK_VIDEO_PARAM tempParam;
	time_t startTime,endTime,laterTime;
	int iInfoNumber = 0;
	int rel;
	char fileName[50];
	 struct timeval nowTime;	 
	 int iGetPlayModError = 0;
	 int net_backup_file = 0;
	 time_t rec_start = 0,rec_enc = 0;;


	DISK_GetAllDiskInfo();	
			
	iDiskNum = DISK_GetDiskNum();
	

	// 做文件顺序查找，若指定了文件位置 ，则从那个
	//文件位置继续向下查找。
	for( i = iDiskNum -1; i >= 0; i-- )
	{
		if( FS_DiskFormatIsOk(i) == 0 )
		{
			printf(" FS_DiskFormatIsOk = %d\n",FS_DiskFormatIsOk(i) );
			continue;
		}
		stDiskInfo = DISK_GetDiskInfo(i);

		
		for(j = stDiskInfo.partitionNum - 1; j >= 1; j-- )
		{			

		
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
			{
					printf(" FS_GetPosFileInfo eror!\n");
					return ERROR;
			}
			
			for(k = pFileInfoPoint->pUseinfo->iTotalFileNumber - 1; k >=0 ; k--)
			{		

				pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , k ,&fileInfoPoint);

				if( pFileInfoPoint == NULL )
				{
					printf("11 FS_GetPosFileInfo eror!\n");
					return ERROR;
				}

				if( pFileInfoPoint->pFilePos->StartTime == 0 )
					continue;

				if( pFileInfoPoint->pFilePos->states != WRITEOVER )
					continue;

				if( rec_start == 0 )
					rec_start = pFileInfoPoint->pFilePos->StartTime;

				if( rec_enc == 0 )
					rec_enc = pFileInfoPoint->pFilePos->EndTime;

				if( rec_start > pFileInfoPoint->pFilePos->StartTime )
					rec_start = pFileInfoPoint->pFilePos->StartTime;

				if( rec_enc < pFileInfoPoint->pFilePos->EndTime )
					rec_enc = pFileInfoPoint->pFilePos->EndTime;
				
			}
		}
	}

	if( rec_start == 0 || rec_enc == 0 )
	{
		DPRINTK(" %ld %ld error\n",rec_start,rec_enc);
		return ERROR;
	}

	{
		int i,j,k,rel;
		int iDiskNum;
		GST_DISKINFO stDiskInfo;
		RECTIMESTICKLOGPOINT * pRecPoint;
		RECTIMESTICKLOGPOINT   RecPoint;

		iDiskNum = DISK_GetDiskNum();

		startTime = rec_start;
		endTime = rec_enc;

		DPRINTK("time %ld %ld \n",rec_start,rec_enc);

		//录像的准确时间与time stick的时间有可能有300秒的误差。
		if( startTime >  300 )
			startTime = startTime - 300;
		endTime = endTime + 300;

		DPRINTK("change time %ld %ld \n",startTime,endTime);

		for( i = iDiskNum - 1; i >=  0; i-- )
		{		
		
			stDiskInfo = DISK_GetDiskInfo(i);
		
			if( FS_DiskFormatIsOk(i) == 0 )
				continue;
		
			for(j = stDiskInfo.partitionNum - 1; j >= 1; j--)
			{
				
				pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , 0,&RecPoint );
				if( pRecPoint == NULL )
				{
						DPRINTK("FS_GetPosRecTimeStickMessage error!\n");
						return ERROR;	
				}				
			
				
				for(k = pRecPoint->pUseinfo->totalnumber - 1; k >= 0 ; k--)
				{	
					
					pRecPoint = FS_GetPosRecTimeStickMessage(i,stDiskInfo.partitionID[j] , k ,&RecPoint );

					if( pRecPoint == NULL )
					{
						DPRINTK("FS_GetPosRecTimeStickMessage error!\n");
						return ERROR;	
					}				

					if( pRecPoint->pFilePos->startTime  == 0 )
						continue;					
				
					if( FTC_CompareTime(pRecPoint->pFilePos->startTime,startTime,endTime ) )
					{		
						//if( pRecPoint->pFilePos->startTime > 0 )
						//	DPRINTK("log k=%d starttime=%ld   %ld %ld\n",k, pRecPoint->pFilePos->startTime,rec_start,rec_enc);
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
								DPRINTK("RecTimeStickMessageWriteToFile error!\n");
								return rel;
							}	
							
						}
					}
					
				}
			}
		}
		
		return ALLRIGHT;
	}


	DPRINTK(" Can't Find  file!\n");
	return ERROR;
}

int FTC_check_fileinfo_timestick_lock()
{
	int rel;

	FS_LogMutexLock();

	rel = FTC_check_fileinfo_timestick();

	FS_LogMutexUnLock();

	return rel;
}


