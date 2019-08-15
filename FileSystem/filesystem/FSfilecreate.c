#include "saveplayctrl.h"
#include "FTC_common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "devfile.h"


time_t usbStartTime = 0,usbEndTime = 0;
int usbCam = 0;
int g_usbFlag = 0;
int g_writeCdFlag = 0;
int g_cdCurrentFlag = 0;
int g_cdDevice = 0;
GST_FILESHOWINFO tempFileShowInfo;
time_t cdStartTime[3];
time_t cdEndTime[3];
int cd_cam[3];

int FS_WriteUsb(time_t startTime,time_t  endTime,int iCam,int realWrite)
{
	int usbSize = 0;
	long long fileSize = 0;
		

	printf("usbStartTime = %ld usbEndTime = %ld usbCam = %d realWrite=%d\n",
		startTime,endTime,iCam,realWrite);

	if( gUsbWrite == 1  && realWrite != 0 )
	{
		printf(" backuping...., can't backup again!\n");
		return NO;
	}

	if( realWrite == 0 )
	{
		gUsbWrite = 0;
	}

	if( iCam == 0 )
	{
		printf(" iCam error!\n");
		return NO;
	}
	
	// 检测usb 大小和所要写的文件大小
	if( realWrite == 2 )
	{
		usbSize = FS_GetDiskUseSize(NULL);
		if( usbSize < 0 )
			return usbSize;
		
		printf("usbsize = %d \n",usbSize);

		if( usbSize < 5*1024 )
		{
			printf("usbSize = %d is not have enough space\n",usbSize);
			g_usbFlag = USBSPACENOTENOUGH;
			return NO;
		}
		
		return YES;
	}

	// 真正进行usb 写入.
	if( realWrite == 1 )
	{
		usbStartTime = startTime;
		usbEndTime = endTime;
		usbCam = iCam;
		gUsbWrite = 1;		
		g_usbFlag =USBBACKUPING;
		return ALLRIGHT;
	}
	

	usbStartTime = 0;
	usbEndTime = 0;
	usbCam = 0;

	return ALLRIGHT;
}

int FS_ListWriteToUsb(GST_FILESHOWINFO * stPlayFile,int realWrite)
{
	int usbSize = 0;
	long long fileSize = 0;
	RECORDLOGPOINT * pRecPoint;
		

	printf("usbStartTime = %ld usbEndTime = %ld usbCam = %d realWrite=%d\n",
		stPlayFile->fileStartTime,stPlayFile->fileEndTime,stPlayFile->iCam,realWrite);

	if( gUsbWrite == 1  && realWrite != 0 )
	{
		printf(" backuping...., can't backup again!\n");
		return NO;
	}
	
	if( realWrite == 0 )
	{
		gUsbWrite = 0;
	}

	// 检测usb 大小和所要写的文件大小
	if( realWrite == 2 )
	{
		usbSize = FS_GetDiskUseSize(NULL);
		if( usbSize < 0 )
		{
			if( usbSize == NOUSBDEVICE )
			{
				g_usbFlag = NOUSBDEV;
			}
			return usbSize;
		}
		
		printf("usbsize = %d \n",usbSize);

		if( usbSize < 5*1024 )
		{
			printf("usbSize = %d is not have enough space\n",usbSize);
			g_usbFlag = USBSPACENOTENOUGH;
			return NO;
		}	
		
		return YES;
	}

	if( stPlayFile->iCam  == 0 )
	{
		printf(" iCam error!\n");
		return NO;
	}

	tempFileShowInfo.event = stPlayFile->event;
	tempFileShowInfo.fileEndTime = stPlayFile->fileEndTime;
	tempFileShowInfo.fileStartTime = stPlayFile->fileStartTime;
	tempFileShowInfo.iCam = stPlayFile->iCam;
	tempFileShowInfo.iDiskId = stPlayFile->iDiskId;
	tempFileShowInfo.iFilePos = stPlayFile->iFilePos;
	tempFileShowInfo.iLaterTime = stPlayFile->iLaterTime;
	tempFileShowInfo.imageSize = stPlayFile->imageSize;
	tempFileShowInfo.iPartitionId = stPlayFile->iPartitionId;
	tempFileShowInfo.iPreviewTime = stPlayFile->iPreviewTime;
	tempFileShowInfo.iSeleted      = stPlayFile->iSeleted;
	tempFileShowInfo.states	      = stPlayFile->states;
	tempFileShowInfo.videoStandard = stPlayFile->videoStandard;
	
	
	// 真正进行usb 写入.
	if( realWrite == 1 )
	{
		if( stPlayFile->event.loss != 0 || stPlayFile->event.motion != 0  ||
		 	stPlayFile->event.sensor != 0)
		{
			// 报警备份还是用时间备份的方式进行

			pRecPoint = FS_GetPosRecordMessage(stPlayFile->iDiskId,stPlayFile->iPartitionId,stPlayFile->iFilePos);
			if( pRecPoint == NULL )
				return ERROR;
			if(  stPlayFile->fileStartTime - 10 >= pRecPoint->pFilePos->startTime)
			{
				usbStartTime = stPlayFile->fileStartTime - 10;
				
			}
			else
			{
				usbStartTime = pRecPoint->pFilePos->startTime;
			}
			
			usbEndTime = stPlayFile->fileEndTime + 10;
			usbCam = stPlayFile->iCam;
		}
		else
		{
			usbStartTime = 0;
			usbEndTime = 0;
			usbCam = 0;
		}		
		gUsbWrite = 1;		
		g_usbFlag =USBBACKUPING;
		return ALLRIGHT;
	}

	usbStartTime = 0;
	usbEndTime = 0;
	usbCam = 0;

	return ALLRIGHT;
}

void FS_ReleaseUsbThread()
{
	gUsbWrite = -1;
}

int FS_UsbWriteStatus()
{	
	printf("g_usbFlag = %d 2\n",g_usbFlag);
	return g_usbFlag;
}


void thread_for_create_file(void)
{
	int ret;


	while(1)
	{
		if( gStopCreateFile == 1 )
		{
			usleep(50000);		
		}
		else if( gStopCreateFile == -1 )
		{	
			printf(" create file thread stop!\n");		
			return;
		}
		else if(gStopCreateFile == 0 )
		{
			printf("create file start!====\n");
		
			ret = FS_CreateFile();

			if(ret  == ALLRIGHT )
				printf(" CreateFile OK!\n");
			else			
				printf(" CreateFile faild!\n");
			
			if( gStopCreateFile != -1 )
				gStopCreateFile = 1;
			
		}
	//	else
	//		usleep(50000);	
		
	}	
	
}

void thread_for_create_usb_file(void)
{
	SET_PTHREAD_NAME(NULL);
	int ret;

	DPRINTK(" pid = %d\n",getpid());

	ret = FS_UsbBufInit();
	if( ret < 0 )
		return ;
	
	while(1)
	{
		if( gUsbWrite == 0 )
		{
			usleep(50000);		
		}
		else if( gUsbWrite == -1 )
		{	
			printf(" usb  thread stop!\n");
			FS_UsbRelease();
			return;
		}
		else if(gUsbWrite == 1 )
		{
			printf("usb write  file start!====\n");

			if( usbStartTime==0 && usbEndTime==0 && usbCam == 0)
			{
				ret = FS_ListWriteDataToUsb(&tempFileShowInfo);
			}
			else
			{		
				ret = FS_WriteDataToUsb(usbStartTime,usbEndTime,usbCam);
			}

			printf("usb ret = %d\n",ret);

			if(ret  == ALLRIGHT )
			{				
				printf(" usb write  OK!\n");
				g_usbFlag = USBBACKUPSTOP;
				printf(" g_usbFlag = %d\n",g_usbFlag);
			}
			else
			{
				if( ret == USBNOENOUGHSPACE )
				{
					g_usbFlag = USBSPACENOTENOUGH;
				}
				else
				{
					printf(" usb write   faild!\n");
					g_usbFlag = USBWRITEERROR;
				}
			}
			
			gUsbWrite = 0;
		}
		
	}
}

int FS_scan_cdrom()
{	

	char cmd[200];
	int fp = 0;
	long fileOffset = 0;
	int rel;
	int i,j;
	int pid;

/*	sprintf(cmd,"ls /dev/cdroms|grep cdrom0|grep -v grep > /bin/3.txt");

	printf("start cmd : %s \n",cmd);

	rel = command_ftc(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return 0;
	}
*/
	struct stat st;


 //      rel = ( lstat ("/dev/cdroms/cdrom0", &st) == 0 && S_ISLNK (st.st_mode) );

    rel = ( lstat ("/dev/sr0", &st) == 0 && S_ISBLK (st.st_mode) );



	DPRINTK("open cdrom0 lstat = %d\n",rel);

	return rel;

/*	fp = open("/dev/cdroms/cdrom0",O_RDONLY);
	if( fp <= 0 )
	{
		DPRINTK("open cdrom0 error!\n");
		return 0;
	}

	close(fp);

	return 1;
	*/
}


int FS_WriteDataToCdrom(time_t timeStart_1,time_t timeEnd_1,int cam1,
							time_t timeStart_2,time_t timeEnd_2,int cam2,
							time_t timeStart_3,time_t timeEnd_3,int cam3,int iDevice)
{
	cdStartTime[0] = timeStart_1;
	cdStartTime[1] = timeStart_2;
	cdStartTime[2] = timeStart_3;

	cdEndTime[0] = timeEnd_1;
	cdEndTime[1] = timeEnd_2;
	cdEndTime[2] = timeEnd_3;

	cd_cam[0] = cam1;
	cd_cam[1] = cam2;
	cd_cam[2] = cam3;
	

	g_cdCurrentFlag = 1;
	g_writeCdFlag = 1;
	g_cdDevice = iDevice;
	
	return ALLRIGHT;
}

int FS_WriteCdStatus()
{
	//DPRINTK("g_cdCurrentFlag=%d\n",g_cdCurrentFlag);
	return g_cdCurrentFlag;
}

int FS_GetCDROMDevName(char * dev)
{
	struct stat st;
	int rel;
	int count;
	int i;
	int get= 0;
	
	for(i=0;i<16;i++)
	{
		sprintf(dev,"/dev/sg%d",i);

		rel = ( lstat (dev, &st) == 0 && S_ISCHR (st.st_mode) );

		if( rel == 1 )
		{
			DPRINTK("get dev=%s\n",dev);
			get = 1;
		}else
		{
			break;
		}		
	}

	if( get == 1 )
	{
		sprintf(dev,"/dev/sg%d",i-1);
		DPRINTK("cd_dev=%s\n",dev);

		return 1;
	}

	return 0; 
}

int FS_fireToCDROM()
{

	GST_DISKINFO stDiskInfo;

	char cmd[150];

	char dev_name[100];

	int rel;

	stDiskInfo = DISK_GetDiskInfo(0);

	memset(cmd, 0, 50);

	if( g_cdDevice == 1)
	{
		rel = FS_GetCDROMDevName(dev_name);
		if( rel != 1 )
		{
			DPRINTK("Can't get cd dev name\n");
			return ERROR;
		}

		
		{
			
			sprintf(cmd,"mkisofs -J -r -o /tddvr/%s5/cd.iso /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5],&stDiskInfo.devname[5]);

			DPRINTK("%s\n",cmd);
			rel = command_ftc(cmd);

			if( rel == -1 )
			{
				printf(" %s error!\n",cmd);
				return ERROR;
			}

			sprintf(cmd,"cdrecord -v fs=2m  speed=16 dev=%s  /tddvr/%s5/cd.iso", dev_name,&stDiskInfo.devname[5]);

		}
	
	//	sprintf(cmd,"mkisofs -J -r /tddvr/%s5/AnaVideo | cdrecord -v dev=%s  --eject fs=2m  speed=4 -", &stDiskInfo.devname[5],dev_name); // maybe devName is /tddvr/hda5
	
	//	sprintf(cmd,"mkisofs -J -r /tddvr/%s5/AnaVideo | cdrecord -v dev=/dev/sg0  --eject fs=2m  speed=4 -", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5
	}
	else
	{
	//	sprintf(cmd,"mkisofs -R -J -split-output -o /tddvr/%s5/dvd /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5],&stDiskInfo.devname[5]);
		sprintf(cmd,"mkisofs -R -J -o /tddvr/%s5/dvd /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5],&stDiskInfo.devname[5]);

		printf("%s\n",cmd);
		rel = command_ftc(cmd);

		if( rel == -1 )
		{
			printf(" %s error!\n",cmd);
			return ERROR;
		} 
		sprintf(cmd,"/mnt/mtd/growisofs -use-the-force-luke  -Z /dev/sr0=/tddvr/%s5/dvd", &stDiskInfo.devname[5]);	
	

	//sprintf(cmd,"/mnt/mtd/growisofs -use-the-force-luke -Z /dev/sr0  -J -R /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5]);	

	
	
	//	sprintf(cmd,"cdrecord-wrapper.sh --eject  speed=16 -v /tddvr/%s5/dvd*",&stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5
	
	//	sprintf(cmd,"/mnt/mtd/growisofs -use-the-force-luke  -Z /dev/cdroms/cdrom0 -R -J /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5]);	
	/*	sprintf(cmd,"mkisofs -R -J -split-output -o /tddvr/%s5/dvd /tddvr/%s5/AnaVideo", &stDiskInfo.devname[5],&stDiskInfo.devname[5]);

		printf("%s\n",cmd);
		rel = command_ftc(cmd);

		if( rel == -1 )
		{
			printf(" %s error!\n",cmd);
			return ERROR;
		}
	
		sprintf(cmd,"/mnt/mtd/Cdrw write /tddvr/%s5/dvd* dev=/dev/cdroms/cdrom0",&stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5
	*/
	}

	printf("%s\n",cmd);

	rel = command_ftc(cmd);

	if( rel == -1 )
	{
		printf(" %s error!\n",cmd);
		return ERROR;
	}

	return ALLRIGHT;	
}

int FS_StopCdromThread()
{
	g_writeCdFlag = -1;
	return 1;
}


void thread_for_create_cdrom_file(void)
{
	SET_PTHREAD_NAME(NULL);
	int rel;
	int StopCreate;

	DPRINTK(" pid = %d\n",getpid());

	while(1)
	{	
		if( g_writeCdFlag == 0 )
		{
			sleep(1);
		}else if(  g_writeCdFlag == 1 )
		{
			/*StopCreate = 0;
		
			//停止创建文件线程
			if( FS_CreateFileStatus() == 0 )
			{
				StopCreate = 1;
				FS_StopCreateFile();
				printf("Stop create file !\n");
			}*/
		
			rel = FS_WriteDataTo_CDROM();
			if( rel < 0 )
			{
				if( rel != USBNOENOUGHSPACE )
					g_cdCurrentFlag = 2;
				else
				{
					rel = FS_fireToCDROM();
					if( rel > 0 )
						g_cdCurrentFlag = 0;
					else
						g_cdCurrentFlag = 2;	
				}
			}
			else
			{			
				rel = FS_fireToCDROM();
				if( rel > 0 )
					g_cdCurrentFlag = 0;
				else
					g_cdCurrentFlag = 2;		
			}

			g_writeCdFlag = 0;

			/*if( StopCreate ==  1 )
			{
				FS_StartCreateFile();
				printf("Start create file !\n");
			}*/
			
		}else if(  g_writeCdFlag == -1 )
		{
			printf(" thread_for_create_cdrom_file release!\n");
			break;
		}		
	}
}





