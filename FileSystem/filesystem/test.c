
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "filesystem.h"
#include "saveplayctrl.h"


#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sched.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/mman.h>

#include <linux/unistd.h>


//格式化硬盘，建立文件系统。
int function1()
{
	FS_UMountAllPatition(0);

	FS_PartitionDisk(0);
	
	FS_BuildFileSystem(0);
}

// 查看fileinfo.log
int function2()
{
	FILEINFOLOGPOINT * pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int rel;
	int i;

	rel = FS_InitFileInfoLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitFileInfoLogBuf error!\n");
		return ERROR;
	}

	for(i= 0; i < 360; i++)
	{
	pFileInfoPoint = FS_GetPosFileInfo(0 , 5, i,&fileInfoPoint);
	if(pFileInfoPoint == NULL)
			return ERROR;
	
	printf(" %s  state %d  starttime %ld offset %ld pos %d endtime %ld effect=%d\n",pFileInfoPoint->pFilePos->fileName,
		pFileInfoPoint->pFilePos->states, pFileInfoPoint->pFilePos->StartTime,
		pFileInfoPoint->pFilePos->ulOffSet, pFileInfoPoint->pUseinfo->iCurFilePos,
		pFileInfoPoint->pFilePos->EndTime,pFileInfoPoint->pFilePos->iDataEffect);
	}
	FS_ReleaseFileInfoLogBuf();
}

//查看alarm.log
int function3()
{
	ALARMLOGPOINT * pstFileInfo;
	int rel,i;

	FS_InitAlarmLogBuf();

	for(i=0;i<10;i++)
	{

	pstFileInfo = FS_GetPosAlarmMessage(0,5,i);


	if( pstFileInfo == NULL )
		{
		printf(" 2 wrong!\n");
		return 0;
		}
	printf("1 %d total = %d\n", pstFileInfo->pFilePos->id,pstFileInfo->pUseinfo->totalnumber);

	
	printf("event %d%d%d starttime = %ld, icam=%d,endtime=%ld,stats = %d\n",
		pstFileInfo->pFilePos->event.motion,pstFileInfo->pFilePos->event.loss,pstFileInfo->pFilePos->event.sensor,
		pstFileInfo->pFilePos->alarmStartTime ,
		pstFileInfo->pFilePos->iCam ,pstFileInfo->pFilePos->alarmEndTime,pstFileInfo->pFilePos->states );
	}
	FS_ReleaseAlarmLogBuf();

}

// 查看record.log
int function4()
{
	
}

// 查看diskinfo.log
int function5()
{
	DISKCURRENTSTATE stDiskState;
	int rel;

	rel = FS_GetDiskCurrentState(0, &stDiskState);

	printf("iAllfileCreate = %d,iCapacity = %d,iCoverCapacity = %d,iEnableCover = %d,
		iPartitionId = %d,	IsCover =%d,	IsRecording = %d,	iUseCapacity = %d\n",
		stDiskState.iAllfileCreate,stDiskState.iCapacity,stDiskState.iCoverCapacity,
		stDiskState.iEnableCover,stDiskState.iPartitionId,stDiskState.IsCover,
		stDiskState.IsRecording,stDiskState.iUseCapacity);
//	rel = FS_WriteDiskCurrentState(0, &stDiskState);
}

int function6()
{
	int rel;

	rel = FS_InitFileInfoLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitFileInfoLogBuf error!\n");
		return ERROR;
	}

	rel = FS_FixFileInfo(0,5);

	if( rel < 0 )
	{
		printf("FS_FixFileInfo error!\n");
	}

	FS_ReleaseFileInfoLogBuf();
	
	return ALLRIGHT;
}

int function7()
{
	FILE * fp;
	int rel;
	int i = 0;
	unsigned long offset;

	FRAMEHEADERINFO stframe;
	
	offset = 117440512;

	fp = fopen("/tddvr/hda5/video_189","rb");

	if( !fp )
	{
		printf(" openf sdf s\n");
		exit(0);
	}

	rel = fseek(fp, offset, SEEK_SET);
	if( rel !=  0 )
	{
		printf(" fseek sdf s\n");
		exit(0);
	}

	for(  i = 0 ; i < 60000;i++ )
	{
	rel = fread(&stframe,1, sizeof( FRAMEHEADERINFO ), fp );

	if( rel != sizeof( FRAMEHEADERINFO ) )
	{
		printf(" read  sdf s\n");
		exit(0);
	}
	offset += sizeof(FRAMEHEADERINFO);

	printf("chan%d  type %c time %ld Dataoffset %ld  frametype=%d headoffset %ld dlength=%d mode=%d image=%d frameNum=%ld\n",
					stframe.iChannel, stframe.type, stframe.ulTimeSec,
					stframe.ulFrameDataOffset,stframe.ulFrameType,offset,stframe.ulDataLen,stframe.videoStandard,stframe.imageSize,stframe.ulFrameNumber);

	}
	
	fclose(fp);
	
}

int function8()
{
	int rel;

	FS_UMountAllPatition(0);

	rel = FS_CheckNewDisk(0);

	printf(" rel = %d",rel);
}

int function9()
{
	int rel;

	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();
	
	stDiskInfo = DISK_GetDiskInfo(0);

	printf(" p %d  %d  %d",stDiskInfo.partitionID[stDiskInfo.partitionNum -1],
		stDiskInfo.partitionID[stDiskInfo.partitionNum],stDiskInfo.partitionNum);
	
	

}

int function10()
{
/*
	GST_FILESHOWINFO fileinfo[10];
	int rel,i;

//	FS_UMountAllPatition(0);
//	FS_MountAllPatition(0);

	rel = FS_InitFileInfoLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitFileInfoLogBuf error!\n");
		return ERROR;
	}

	fileinfo[0].iDiskId = 0;
	fileinfo[0].iPartitionId = 5;
	fileinfo[0].iFilePos = 3;

	FS_GetFilelist(fileinfo,0, 37154,38529);

	for( i = 0; i < 10 ; i++ )
	{
		printf("dd  iDiskid = %d, paid = %d, fileid = %d\n",fileinfo[i].iDiskId,fileinfo[i].iPartitionId,fileinfo[i].iFilePos);
	}
	FS_ReleaseFileInfoLogBuf();
	*/
}

void function11()
{

	int a,b,c,d,e,f,g;
	time_t time;

	a = 70;
	b= 0;
	c = 1;
	d = 0;
	e = 0;
	f =  0;

	time = FTC_CustomTimeTotime_t(a,b,c,d,e,f);

	printf(" time %s %ld \n",ctime(&time),time);

	time = 1231;

	FTC_time_tToCustomTime(time, &a,&b,&c,&d,&e,&f);

	printf(" %d,%d,%d   %d,%d,%d\n",a,b,c,d,e,f);
	
/*
	struct tm t;
time_t t_of_day;
t.tm_year=1997-1900;
t.tm_mon=6;
t.tm_mday=1;
t.tm_hour=0;
t.tm_min=0;
t.tm_sec=1;
t.tm_isdst=0;
t_of_day=mktime(&t);
printf(ctime(&t_of_day));
*/
}

int  function12()
{
	RECORDLOGPOINT * pRecPoint;
	int rel;
	int i;

	rel = FS_InitRecordLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitRecordLogBuf error!\n");
		return ERROR;
	}

	for(i= 0; i < 1000; i++)
	{
	pRecPoint = FS_GetPosRecordMessage(0 , 5, i);
	if(pRecPoint == NULL)
			return ERROR;

	printf("id %d   starttime=%ld  endtime=%ld filepos=%d offset=%ld mode=%d image=%d effect=%d\n",
		pRecPoint->pFilePos->id,pRecPoint->pFilePos->startTime,pRecPoint->pFilePos->endTime,
		pRecPoint->pFilePos->filePos,pRecPoint->pFilePos->ulOffSet,pRecPoint->pFilePos->videoStandard,
		pRecPoint->pFilePos->imageSize,pRecPoint->pFilePos->iDataEffect);
	
	}
	
	FS_ReleaseRecordLogBuf();
}

int  function13()
{	
	int i = 0;

	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();
	

	for(i = 0; i < DISK_GetDiskNum(); i++ )
	{
		stDiskInfo = DISK_GetDiskInfo(i);
		printf(" dev=%s size=%lld\n",stDiskInfo.devname,stDiskInfo.size);
	}

	printf("usb num = %d \n", DISK_HaveUsb());

	if( DISK_HaveUsb() )
	{
		stDiskInfo = DISK_GetUsbInfo();
		printf(" dev=%s size=%lld\n",stDiskInfo.devname,stDiskInfo.size);
	
	}
	

	return ALLRIGHT;
}
	
int function14()
{
	int iCam;

	int rel;

	iCam = 0x09;

	rel = FS_InitFileInfoLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitFileInfoLogBuf error!\n");
		return ERROR;
	}

	FS_UMountAllPatition(0);

	FS_MountAllPatition(0);

	rel = FS_WriteDataToUsb(100,2000,iCam);

	if( rel < 0 )
	{
		printf("FS_WriteDataToUsb error!\n");
	}

	FS_ReleaseFileInfoLogBuf();

	return 1;

}

int function15()
{
	time_t a,b,c1;

	int a1,b1,c,d,e,f;

	a = 1147630981;

	b = 1147631018;


	FTC_time_tToCustomTime(a, &a1,&b1,&c,&d,&e,&f);

	printf("a %d,%d,%d,   %d:%d:%d\n",a1,b1,c,d,e,f);

	b1=12;
	d =24;

	f=59;

	c1 = FTC_CustomTimeTotime_t(a1,b1,c,d,e,f);

	a1 = 0;
	b1=0;
	c=0;
	d=0;
	e=0;
	f=0;

	FTC_time_tToCustomTime(c1, &a1,&b1,&c,&d,&e,&f);

	printf("c %d,%d,%d,   %d:%d:%d\n",a1,b1,c,d,e,f);

	
	FTC_time_tToCustomTime(b, &a1,&b1,&c,&d,&e,&f);

	printf("3 %d,%d,%d,   %d:%d:%d\n",a1,b1,c,d,e,f);

	FTC_PrintMemoryInfo();
}

int function16()
{
	int iDiskNum = 0;

	DISK_GetAllDiskInfo();

	iDiskNum = DISK_GetDiskNum();

	printf(" disk num = %d\n",iDiskNum);

	return ALLRIGHT;
}


int function17()
{
GST_RECORDEVENT event;
time_t eventStartTime;
time_t eventEndTime;
int iCam;
int   iPreviewTime;
int   iLaterTime;
int  states;
int  iMessageId;
int rel;
 GST_FILESHOWINFO stFileShowInfo;
 time_t searchStartTime;
 time_t searchEndTime;

RECORDLOGPOINT * po;

iMessageId = -1;


	return ALLRIGHT;
}

int function18()
{
	long long lSize = 0;
	int rel;

	rel = FS_InitFileInfoLogBuf();
	if( rel < 0 )
	{
		printf(" FS_InitFileInfoLogBuf error!\n");
		return ERROR;
	}


	DISK_GetAllDiskInfo();

	

	//lSize = FS_GetDataSize(1150198399,1150198465,15);

	printf("lSize = %lld\n",lSize);
	
	FS_ReleaseFileInfoLogBuf();

	return ALLRIGHT;
}


// 清理日志文件
int function19()
{
	int ret;

	ret = FS_InitFileInfoLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecordLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitAlarmLogBuf();
	if( ret < 0 )
		return ERROR;

	FS_ClearDiskLogMessage(0);
/*	ret = FS_DelRecLosMessage(1150198399,1150285379,-1);
	if( ret < 0 )
	{
		printf("FS_DelRecLosMessage error!\n");
		return ret;
	}
*/

	FS_ReleaseAlarmLogBuf();
	FS_ReleaseRecordLogBuf();
	FS_ReleaseFileInfoLogBuf();
	return 1;
}

int  function20()
{
	 unsigned long ulFrameOffset = 0;	
	 FILE * fp;
	int rel;
	int i = 0;
	unsigned long offset;

	FRAMEHEADERINFO stframe;

	char fileName[50];
	 struct timeval nowTime;	

	 unsigned long ulEndOffset = 0;
	 FRAMEHEADERINFO  stFrameHeaderInfo;
	 char * databuf = NULL;
	 int iFirst = 0;

	databuf= (char *)malloc(20*AUDIOBUF);
	if( !databuf)
	{
		printf("databuf malloc error!\n");
		return NOENOUGHMEMORY;
	}

	fp = fopen("/tddvr/hda5/video_3","rb");

	if( !fp )
	{
		printf(" openf sdf s\n");
		exit(0);
	}

	 ulFrameOffset = FRAMEHEADOFFSET;

		rel = fseek(fp, ulFrameOffset, SEEK_SET);
		if ( rel < 0 )
		{
			free(databuf);
			fclose(fp);
			printf("  fseek error!\n");
			return FILEERROR;
		}
	
	
	while(1)
	{
	
	rel = fread(&stframe,1, sizeof( FRAMEHEADERINFO ), fp );

	if( rel != sizeof( FRAMEHEADERINFO ) )
	{
		printf(" read  sdf s\n");
		exit(0);
	}
	offset += sizeof(FRAMEHEADERINFO);

	printf("chan%d  type %c time %ld Dataoffset %ld  frametype=%d headoffset %ld dlength=%d mode=%d image=%d frameNum=%ld\n",
					stframe.iChannel, stframe.type, stframe.ulTimeSec,
					stframe.ulFrameDataOffset,stframe.ulFrameType,offset,stframe.ulDataLen,stframe.videoStandard,stframe.imageSize,stframe.ulFrameNumber);

	}
	
	fclose(fp);
}


int function21()
{
	int rel ;

	rel = FS_CheckShutdownStatus(0);

	printf("rel = %d\n",rel);
}

int function22()
{
	int ret;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	int iInfoNumber = 0;

	ret = FS_InitFileInfoLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecordLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitAlarmLogBuf();
	if( ret < 0 )
		return ERROR;
	

	iDiskNum = DISK_GetDiskNum();

	for( i = 0; i < iDiskNum; i++ )
	{
		FS_MountAllPatition(i);

		stDiskInfo = DISK_GetDiskInfo(i);
		

		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			pFileInfoPoint = FS_GetPosFileInfo(i,stDiskInfo.partitionID[j] , 0,&fileInfoPoint );
			if( pFileInfoPoint == NULL )
					return ERROR;

			pFileInfoPoint->pUseinfo->iTotalFileNumber =3;

			ret = UseInfoWriteToFile(pFileInfoPoint);
			if( ret  < 0 )
			{
				printf(" UseInfoWriteToFile error!");
				return ret;
			}
		}

		FS_UMountAllPatition(i);
	}

	

	FS_ReleaseAlarmLogBuf();
	FS_ReleaseRecordLogBuf();
	FS_ReleaseFileInfoLogBuf();
}

int function23()
{
	int i,iDiskNum;
	int rel;
	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();

	iDiskNum = DISK_GetDiskNum();

	for(i = 0 ; i <  iDiskNum; i++ )
	{
		stDiskInfo = DISK_GetDiskInfo(i);

		rel = DISK_EnableDMA(stDiskInfo.devname,ENABLE);

		if( rel < 0 )
			printf("%s error!\n",stDiskInfo.devname);
		else
			printf("%s suceess!\n",stDiskInfo.devname);
	}


}

int function24()
{
	int rel ;
/*
	rel = FS_PhysicsFixDisk("/dev/hdb5");

	printf(" rel = %d\n",rel);

	rel = FS_PhysicsFixDisk("/dev/hdb6");

	printf(" rel = %d\n",rel);
*/

FS_PhysicsFixAllDisk();
}

int function25()
{
	char cmd[50];
	int rel;
	
				sprintf(cmd,"rm -f /flash/gflag ");

					printf("command_ftc: %s\n",cmd);
					
					rel = command_ftc(cmd);

					if( rel == -1 )
					{
						printf(" %s faild!\n",cmd);
						exit(1);
					}				
					
					sprintf(cmd,"umount /flash");

					printf("command_ftc: %s\n",cmd);

					rel = command_ftc(cmd);

					if( rel == -1 )
					{
						printf(" %s faild!\n",cmd);
						exit(1);
					}				

					sprintf(cmd,"eraseall -j /dev/mtd2");

					printf("command_ftc: %s\n",cmd);

					rel = command_ftc(cmd);

					if( rel == -1 )
					{
						printf(" %s faild!\n",cmd);
							exit(1);
					}

					sprintf(cmd,"mount -t jffs2 /dev/mtdblock2 /flash");

					printf("command_ftc: %s\n",cmd);

					rel = command_ftc(cmd);

					if( rel == -1 )
					{
						printf(" %s faild!\n",cmd);
							exit(1);
					}

					return 1;
}

int shutdowninfo()
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
	DISKFIXINFO stFixInfo;

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(0 );
	
	memset(devName, 0, 30);

	sprintf(devName,"/tddvr/%s5", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	memset(cmd,0x00,50);

	sprintf(cmd,"mkdir %s",devName);

	rel = command_ftc(cmd);    // mkdir /tddvr/hda5

	memset(cmd,0x00,50);

	sprintf(cmd,"mount -t vfat %s5 %s",stDiskInfo.devname,devName);  // mount /dev/hda5

	rel = command_ftc(cmd);

	if( rel == -1 )
	{
			printf(" can't mount %s", stDiskInfo.devname);
			return MOUNTERROR;
	}

	sprintf(cmd,"%s/shutdown",devName);

	printf(" open %s\n",cmd);

	fp = fopen(cmd,"rb");
	if( fp == NULL )
	{
		umount( devName );
		return ALLRIGHT;
	}
	else
	{
		printf(" have this file %s need fix disk\n",cmd);

		rel = fread(&stFixInfo,1,sizeof(DISKFIXINFO),fp);
		if( rel != sizeof(DISKFIXINFO) )
		{
			printf(" 2read stfixinfo error! rel = %d\n",rel);
			fclose(fp);
			return FILEERROR;
		}
		fclose(fp);
		fp = NULL;

		printf("stFixInfo.create=%d,rec=%d\n",stFixInfo.iCreatePartiton,stFixInfo.iRecPartiton);
		
		umount( devName );
		return ERROR;
	}	
}

int Write1txt()
{
	FILE * fp;

	fp = fopen("1.txt","r+b");

	fseek(fp,500*1024 - 20,SEEK_SET);

	fwrite("yueyue binbin",1,14,fp);

	fclose(fp);

	return 1;
}

int GetDiskMbr()
{	
	int fd;
	int fd1;
	char * buf = NULL;
	int iNumber = 0;

	fd = open("/dev/hda",O_RDONLY);

	if( fd < 0 )
	{	
		printf("open Error!\n");
		return -1;
	}

	fd1 = open("disk.mbr",O_CREAT|O_WRONLY|O_TRUNC);

	if( fd1 < 0 )
	{
		printf("open disk.mbr error!\n");
		return -1;
	}

	buf = (char *)malloc(512);
	if( !buf )
	{
		printf(" malloc err!\n");
		close(fd);
		close(fd1);
	}

	iNumber = read(fd,buf,512);

	if( iNumber != 512 )
	{
		printf("read error number=%d\n",iNumber);
	}

	iNumber = write(fd1,buf,512);

	if( iNumber != 512 )
	{
		printf("write error number=%d\n",iNumber);
	}

	close(fd);
	close(fd1);

	return ALLRIGHT;	
}

int changerecuseinfo()
{
	int ret;
	int i,j,k;
	int iDiskNum;
	GST_DISKINFO stDiskInfo;
	RECORDLOGPOINT *  pFileInfoPoint;
	int iInfoNumber = 0;

	ret = FS_InitFileInfoLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitRecordLogBuf();
	if( ret < 0 )
		return ERROR;

	ret = FS_InitAlarmLogBuf();
	if( ret < 0 )
		return ERROR;
	

	iDiskNum = DISK_GetDiskNum();

	for( i = 0; i < iDiskNum; i++ )
	{
		FS_MountAllPatition(i);

		stDiskInfo = DISK_GetDiskInfo(i);
		

		for(j = 1; j < stDiskInfo.partitionNum; j++ )
		{
			pFileInfoPoint = FS_GetPosRecordMessage(i,stDiskInfo.partitionID[j] , 0 );
			if( pFileInfoPoint == NULL )
					return ERROR;

			printf("1 j=%d totalnumber=%d\n",j,pFileInfoPoint->pUseinfo->totalnumber);

		/*
			pFileInfoPoint->pUseinfo->totalnumber = 120;

			ret = RecordMessageUseInfoWriteToFile();
			if( ret  < 0 )
			{
				printf(" RecordMessageUseInfoWriteToFile error!");
				return ret;
			}
		*/
		}

		FS_UMountAllPatition(i);
	}

	

	FS_ReleaseAlarmLogBuf();
	FS_ReleaseRecordLogBuf();
	FS_ReleaseFileInfoLogBuf();

	return 1;

}

int  main(void)
{

	ALARMLOGPOINT * pstFileInfo;

	FILEINFOLOGPOINT * pFileInfoPoint;

	DISKCURRENTSTATE stDiskState;

	FRAMEHEADERINFO stFrameHeaderInfo;

	int i = 0;

	char cmd[50];

	long long lSize = 0;
	
	static int fd;

	int iFsize = 0;

	int rel;

	DISK_GetAllDiskInfo();


	function12();




}




