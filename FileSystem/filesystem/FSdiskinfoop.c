#include "filesystem.h"
#include "FTC_common.h"
int g_LogChanged = 1;


int  FS_GetDiskCurrentState(int iDiskId, DISKCURRENTSTATE * stDiskState)
{
	int i;
	int rel = 0;
	int iDiskNum;
	int iReadNum;
	FILE * fp = NULL;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/diskinfo.log",&stDiskInfo.devname[5]);

	fp = fopen(fileNameBuf, "rb");

	if( fp == NULL )
	{
			printf(" open %s error!\n", fileNameBuf);
			return iDiskId;
	}

	fseek(fp, sizeof( GST_DISKINFO), SEEK_SET);

	iReadNum = fread(stDiskState, 1 , sizeof( DISKCURRENTSTATE ), fp );

	if( iReadNum != sizeof( DISKCURRENTSTATE ) )
	{
		printf(" read error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	return ALLRIGHT;
	
}

int FS_WriteDiskCurrentState(int iDiskId,   DISKCURRENTSTATE * stDiskState)
{
	int i;
	int rel = 0;
	int iDiskNum;
	int iWriteNum;
	FILE * fp = NULL;
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;


	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	memset(fileNameBuf, 0, 50);

	sprintf(fileNameBuf,"/tddvr/%s5/diskinfo.log",&stDiskInfo.devname[5]);

	fp = fopen(fileNameBuf, "r+b");

	if( fp == NULL )
	{
			printf(" open %s error!\n", fileNameBuf);
			return iDiskId;
	}

	rel = fseek(fp, sizeof( GST_DISKINFO), SEEK_SET);
	if( rel == -1 )
	{
		printf(" fseek error! 80\n");
		fclose(fp);
		return FILEERROR;
	}

	iWriteNum = fwrite(stDiskState, 1 , sizeof( DISKCURRENTSTATE ), fp );

	if( iWriteNum != sizeof( DISKCURRENTSTATE ) )
	{
		printf(" write error!\n");
		fclose(fp);
		return FILEERROR;
	}

	fclose(fp);

	return ALLRIGHT;
}

int FS_get_disk_remain_rate(int disk_id)
{	
	int rel,remainCapacity,rate;
	int iUsedFile = 0;
	int iAllFile = 0;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT     fileInfoPoint;
	DISKCURRENTSTATE  stDiskState;

//	printf("FS_get_disk_remain_rate diskid=%d\n",disk_id);

	if( g_iDiskIsRightFormat[disk_id] == 0 )
		return FILEERROR;

	rel =  FS_GetDiskCurrentState(disk_id, &stDiskState);

	if( rel < 0 )
		return rel;

	if( stDiskState.iCapacity <= 0 )
		return FILEERROR;

//	printf("1  IsCover = %d\n",stDiskState.IsCover);

	if( stDiskState.IsCover == YES )
			return 0;
	else
	{
		stDiskInfo = DISK_GetDiskInfo(disk_id);

//		printf("FS_get_disk_remain_rate diskid=%d\n",disk_id);

		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
		//	printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
					
			pFileInfoPoint = FS_GetPosFileInfo(disk_id,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
				return ERROR;

			for(i = 0; i < pFileInfoPoint->pUseinfo->iTotalFileNumber; i++ )
			{
				pFileInfoPoint = FS_GetPosFileInfo(disk_id,stDiskInfo.partitionID[j] , i ,&fileInfoPoint);
				if( pFileInfoPoint == NULL )
					return ERROR;
			
			//	printf(" iUsedFile = %d\n",iUsedFile);
				if( pFileInfoPoint->pFilePos->iDataEffect == EFFECT && 
					pFileInfoPoint->pFilePos->states == WRITEOVER )
				{
					iUsedFile += 1;					
				}

				iAllFile += 1;
			}
//			printf(" pFileInfoPoint->pUseinfo->iCurFilePos = %d\n",pFileInfoPoint->pUseinfo->iCurFilePos);
			
		}
	

		remainCapacity = iAllFile - iUsedFile;

		rate = div_safe(remainCapacity * 100, iAllFile);

	//	printf(" remainCapacity = %d,rate = %d\n ",remainCapacity,rate);

		return rate;
	}

	return FILEERROR;
	
}

int FS_get_cover_rate(int disk_id)
{
	int rel,rate;	
	int iUsedFile = 0;
	int iAllFile = 0;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT     fileInfoPoint;
	DISKCURRENTSTATE  stDiskState;
	int cover_pos = 0;
	
//	printf("FS_get_cover_rate diskid=%d\n",disk_id);

	if( g_iDiskIsRightFormat[disk_id] == 0 )
		return FILEERROR;

	rel =  FS_GetDiskCurrentState(disk_id, &stDiskState);

	if( rel < 0 )
		return rel;

	if( stDiskState.iCapacity <= 0 )
		return FILEERROR;

	
//	printf("2  IsCover = %d\n",stDiskState.IsCover);

	if( stDiskState.IsCover == YES )
	{
		
		stDiskInfo = DISK_GetDiskInfo(disk_id);

//		printf("FS_get_cover_rate diskid=%d\n",disk_id);

		cover_pos = 0;

		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
		//	printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
					
			pFileInfoPoint = FS_GetPosFileInfo(disk_id,stDiskState.iPartitionId, 0 ,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
				return ERROR;

			iAllFile += pFileInfoPoint->pUseinfo->iTotalFileNumber;


			if( stDiskInfo.partitionID[j] <= stDiskState.iPartitionId )
			{
				if( stDiskInfo.partitionID[j] < stDiskState.iPartitionId )
				{
					iUsedFile += pFileInfoPoint->pUseinfo->iTotalFileNumber;
				}else
				{
					iUsedFile += pFileInfoPoint->pUseinfo->iCurFilePos + 1;	
					cover_pos = 1;
				}
			}
				
		}			
		
	
		rate = (iUsedFile*100)/ iAllFile;

		if( rate == 0 && iUsedFile != 0 )
			rate = 1;

		printf("rate = %d iUsedFile=%d  iAllFile=%d cover_pos=%d  (%d,%d)\n",
		rate,iUsedFile,iAllFile,cover_pos,g_stRecFilePos.iDiskId,disk_id);
		return rate;
	}
	else
		return 0;
}

int FS_GetBadBlockSize(int iDiskId)
{
	int rel,BadSize;	
	int iBadFile = 0;	
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT     fileInfoPoint;
	DISKCURRENTSTATE  stDiskState;

	if( g_iDiskIsRightFormat[iDiskId] == 0 )
		return FILEERROR;

	stDiskInfo = DISK_GetDiskInfo(iDiskId);

//	printf("FS_GetBadBlockSize diskid=%d\n",iDiskId);

	iBadFile = 0;

	for(j = 1; j < stDiskInfo.partitionNum; j++ )
	{
//		printf(" j = %d partitionID=%d\n",j,stDiskInfo.partitionID[j] );
					
		pFileInfoPoint = FS_GetPosFileInfo(iDiskId,stDiskInfo.partitionID[j] , 0 ,&fileInfoPoint);
		if( pFileInfoPoint == NULL )
			return ERROR;

		for(k = 0; k < pFileInfoPoint->pUseinfo->iTotalFileNumber; k++)
		{

			pFileInfoPoint = FS_GetPosFileInfo(iDiskId,stDiskInfo.partitionID[j] , k,&fileInfoPoint);
			if( pFileInfoPoint == NULL )
				return ERROR;

			if( pFileInfoPoint->pFilePos->states == BADFILE )
			{
				iBadFile++;
			}			
			
		}	
			
	}

	BadSize = iBadFile * FILESIZE;

	if( BadSize >= 1024 )
	{
		printf(" bad size = %d\n", (BadSize /1024 + 1) );
		return (BadSize /1024 + 1);
	}
	else
	{
		if( BadSize == 0 )
		{
		//	printf(" bad size = 0\n");
			return 0;
		}
		else
		{
		//	printf(" bad size = 1\n");
			return 1;
		}
	}	

}




