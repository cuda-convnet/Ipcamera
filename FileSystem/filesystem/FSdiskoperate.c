#include "filesystem.h"
#include "FTC_common.h"
#include "../systemcall/systemcall.h"
#include <sys/mount.h>
#include "IOFile.h"
#include "devfile.h"

int g_CurrentFormatReclistReload = 0;
int g_CurrentFormatRecAddlistReload = 0;
int g_CurrentFormatRecTimeSticklistReload = 0;
int g_CurrentFormatAlarmlistReload = 0;
int g_CurrentFormatFilelistReload = 0;
int g_CurrentFormatEventlistReload = 0;
int g_CurrentFormatDiskRecordTimeReload = 0;

static int g_format_process_percent = 0;

int ftc_get_disk_format_process_percent()
{
	return g_format_process_percent;
}


int ftc_process_send_command(char * cmd)
{
	FILE * fp = NULL;
	int rel;
	int sleep_time;
	long fileOffset = 0;	
	char cmd_buf[200];
	int  count = 0;


	fileOffset = strlen(cmd);

	if( fileOffset > 199 )
	{	
		printf(" command %s is too long!\n",cmd);
	}


LAST_COMMAD_NOT_RUN:
	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);
		count++;
		if( count > 10 )
		{
			printf("LAST_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto LAST_COMMAD_NOT_RUN;
	}
	

	fp = fopen(COMMAND_FILE,"wb");
	if( fp == NULL)
		return -1;

	strcpy(cmd_buf,cmd);

	rel = fwrite(cmd_buf,1,fileOffset+1,fp);

	if( rel != fileOffset+1)
	{
		DPRINTK("fwrite error!\n");
		fclose(fp);
		fp = NULL;
		return -1;
	}


	fclose(fp);
	fp = NULL;

	//等待命令执行程序处理
	sleep_time = SLEEP_TIME*3;	
	usleep(sleep_time);


END_COMMAD_NOT_RUN:
	
	fp = fopen(COMMAND_FILE,"r");
	if( fp != NULL)
	{
		fclose(fp);
		fp = NULL;
		usleep(SLEEP_TIME);		
		count++;

		if( count > 10 )
		{
			//printf("END_COMMAD_NOT_RUN\n");
			count = 0;
		}
		goto END_COMMAD_NOT_RUN;
	}

	fp = fopen(COMMAND_FAILD_FILE,"rb");
	if( fp != NULL )
	{
		fclose(fp);
		fp = NULL;

		printf(" ftc_process_send_command : %s faild!\n",cmd);
		
		return -1;
	}

	printf(" ftc_process_send_command : %s sucess!\n",cmd);

	return 1;
	
}




// excute shell command.
int command_ftc(char * cmd)
{	
	int	res = 1;
	
	res = SendM2(cmd);	
	if(res == 0x00)
	{		
		printf(" command_ftc : %s ok\n",cmd);
		return 1;
	}
	else
	{		
		//printf("command_ftc:%s %d\n",cmd,res);
		if( res < 0  )
		{
			res = ftc_process_send_command(cmd);
			if( res > 0 )
			{				
				return 1;
			}else
			{				
				return -1;
			}
		}

		DPRINTK(" pass : %s res =%d\n",cmd,res);
		return -1;
	}	
	
	return -1;
}


//  check disk type (FAT32, NTFS,LINUX SWP3)
//  if check disk wrong type than return  the disk ID( ID >0 ).
int FS_TypeCheck(int iDiskId) // make sure disk is FAT32 format.
{
	int iDiskNum = 0;

	int j;

	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();     // get all disk information.

	// judge all partitions is FAT32 format.
	
	stDiskInfo = DISK_GetDiskInfo(iDiskId );		

	for( j = 0 ; j < stDiskInfo.partitionNum; j++ )
	{
	/*
		if( j == 0 )
		{
			if( stDiskInfo.disktype[j] != WIN98_EXTENDED )
			{
				printf(" Wrong disk type!1\n");
				return DISKTYPEERROR;    // return the id of the disk which not have right type  format.
			}
		} */

		if( j > 0 )
		{
			if( stDiskInfo.disktype[j] != WIN95_FAT32 )
			{
				printf(" Wrong disk type!2\n");
				return DISKTYPEERROR;
			}
		}	
			
	}	
	
	return ALLRIGHT;
}


void end_format()
{	
	g_format_process_percent = 100;
}

//Partition and format disk. iDiskId is the disk you want operate.
int FS_PartitionDisk(int iDiskId)
{	
	int i,j;
	int rel = 0;
	char devpatitions[30];
	int iDiskNum;
	int patitions_size = 0;
	long long  max_size;
	char cmd[100];
	int step_add = 0;
	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();     // get all disk information.
	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	g_format_process_percent = 0;

	DISK_GetAllDiskInfo();     // get all disk information.
	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	max_size = 644245094400;//600G

	if( stDiskInfo.size > max_size )
	{
		#ifdef USE_EXT3
			patitions_size = LARGE_DISK_PATITION_SIZE;
		#else
			patitions_size = 100;
		#endif
	}
	else
	{
		#ifdef USE_EXT3
			patitions_size = LARGE_DISK_PATITION_SIZE;
		#else
		patitions_size = PATITIONSIZE;
		#endif
	}

	printf("disk size=%lld max=%lld patitions_size=%d\n",stDiskInfo.size,max_size,patitions_size); 

	rel = DISK_SetDisk(iDiskId, patitions_size);  // segmentation disk.
	if(  rel == -1 )
	{
			printf("segmentation disk error!");
			exit(0);
	}

	g_format_process_percent = 10;

	SendM2("mdev -s");
	sleep(1);

	DISK_GetAllDiskInfo();     // get all disk information.
	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	step_add = (100 - g_format_process_percent -1) / (stDiskInfo.partitionNum - 1);

	for( j = 1 ; j < stDiskInfo.partitionNum; j++ )
	{

		memset(devpatitions, 0 , 30);		
		sprintf(devpatitions,"%s%d", stDiskInfo.devname,stDiskInfo.partitionID[j]);		


		#ifdef USE_EXT3
			sprintf(cmd,"mke2fs -j -m 1 -T largefile %s",devpatitions);
			rel =  command_ftc(cmd);
		#else
			rel =  DISK_Formatdisk(devpatitions, FALSE );  // format disk. FALSE is stand for not check bak blocks
											// use TRUE to check bak blocks.
		#endif
	
		if( rel == 1)
		{	
			printf(" format %s success!\n",devpatitions);
		}
		else
		{	
			printf("format %s error!\n", devpatitions);
			exit(0);
		}

		g_format_process_percent += step_add;

		#ifdef USE_FILE_IO_SYS	
		break;
		#endif
				
	}

	g_CurrentFormatReclistReload = 1;
	g_CurrentFormatRecAddlistReload = 1;
	g_CurrentFormatRecTimeSticklistReload = 1;
	g_CurrentFormatDiskRecordTimeReload = 1;
       g_CurrentFormatAlarmlistReload = 1;
	g_CurrentFormatFilelistReload = 1;
	g_CurrentFormatEventlistReload = 1;
	
	return ALLRIGHT; 
}

int FS_MountUsb()
{
	int i,j;
	int rel = 0;
	int iUsbNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	

	DISK_GetAllDiskInfo();     // get all disk information.

	iUsbNum = DISK_HaveUsb();   // get the number of disk whick PC have.

	if( iUsbNum == 0 )
	{
		printf( " no usb ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetUsbInfo();

	// mount all patitions.
	for( j = 0 ; j < 1; j++ )
	{
		memset(devName, 0, 30);

		sprintf(devName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5


	//	memset(cmd,0x00,50);

	//	sprintf(cmd,"mkdir %s",devName);

	//	command_ftc(cmd);    // mkdir /tddvr/hda5

		mkdir(devName,0777);

		memset(cmd,0x00,50);

//		sprintf(cmd,"mount -t vfat %s%d %s",stDiskInfo.devname,stDiskInfo.partitionID[j],devName);  // mount /dev/hda5
		
	//	rel = command_ftc(cmd);

		sprintf(cmd,"%s%d",stDiskInfo.devname,stDiskInfo.partitionID[j]);

		rel = mount(cmd,devName,"vfat",0,NULL);
		
		if( rel == -1 )
		{
			printf(" can't mount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
			return ERROR;
		}		

	}

	printf("FS_MountUsb ok!\n");

	return ALLRIGHT;
}

int FS_UMountUsb()
{	
	int i,j;
	int rel = 0;
	int iUsbNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	

	DISK_GetAllDiskInfo();     // get all disk information.
	
	iUsbNum = DISK_HaveUsb();   // get the number of disk whick PC have.

	if( iUsbNum == 0 )
	{
		printf( " no usb ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetUsbInfo();

	// umount all patitions.
	for( j = 0 ; j < 1; j++ )
	{
		memset(devName, 0, 30);

		sprintf(devName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5

		//memset(cmd,0x00,50);

		//sprintf(cmd,"umount %s",devName);

		//command_ftc(cmd);    // mkdir /tddvr/hda5

		rel = umount(devName);

		if( rel == -1 )
		{
			printf(" can't umount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
			return ERROR;
		}
		
		
	}

	printf("umount usb ok!\n");

	return ALLRIGHT;
}

int FS_FormatUsb()
{
	int i,j;
	int rel = 0;
	int iUsbNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	
	FS_UMountUsb();
	
	DISK_FormatUsb(30);

	DISK_GetAllDiskInfo(); 
	
	stDiskInfo = DISK_GetUsbInfo();

	printf("partitionNum = %d\n",stDiskInfo.partitionNum);

	for( j = 0 ; j < stDiskInfo.partitionNum; j++ )
	{

		memset(devName, 0 , 30);		
		sprintf(devName,"%s%d", stDiskInfo.devname,stDiskInfo.partitionID[j]);
		
		rel =  DISK_Formatdisk(devName, FALSE );  // format disk. FALSE is stand for not check bak blocks
											// use TRUE to check bak blocks.
		if( rel == 1)
		{	
			printf(" format %s success!\n",devName);
		}
		else
		{	
			printf("format %s error!\n", devName);
			return -1;
		}
			
	}

	return 1;
}

int FS_CheckUsb(int * have_usb,int * format_is_right)
{
	int i,j;
	int rel = 0;
	int iUsbNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	

	DISK_GetAllDiskInfo();     // get all disk information.

	iUsbNum = DISK_HaveUsb();   // get the number of disk whick PC have.

	if( iUsbNum == 0 )
	{
		printf( " no usb ! ");
		*have_usb = 0;
	}else
	{
		*have_usb = 1;
	}

	rel = FS_MountUsb();
	if( rel < 0 )
	{
		*format_is_right = 0;
	}else
	{
		*format_is_right = 1;

		rel  = FS_UMountUsb();

		if( rel  < 0 )		
			printf("FS_UMountUsb error!\n");

	}

	return 1;
}

// mount all patition of one disk.
int FS_MountAllPatition(int iDiskId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	FILE * fp = NULL;
	

	DISK_GetAllDiskInfo();     // get all disk information.

	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	// mount all patitions.
	for( j = 1 ; j < stDiskInfo.partitionNum; j++ )
	{
		memset(devName, 0, 30);

		sprintf(devName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5


		memset(cmd,0x00,50);

		//sprintf(cmd,"mkdir %s",devName);

		//command_ftc(cmd);    // mkdir /tddvr/hda5

		mkdir(devName,0777);

		memset(cmd,0x00,50);

		//sprintf(cmd,"mount -t vfat %s%d %s",stDiskInfo.devname,stDiskInfo.partitionID[j],devName);  // mount /dev/hda5

		//DPRINTK(" cmd:%s\n",cmd);

		//rel = command_ftc(cmd);
		
		sprintf(cmd,"%s%d",stDiskInfo.devname,stDiskInfo.partitionID[j]); 

		rel = ftc_mount(cmd,devName);		
		
		if( rel == -1 )
		{
			printf(" can't mount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
			return ERROR;
		}		

	}

	g_iDiskIsRightFormat[iDiskId] = 1;	

	return ALLRIGHT;
}


// umount all patition of one disk.
int FS_UMountAllPatition(int iDiskId)
{
	int i,j;
	int rel = 0;
	int iDiskNum;
	char devName[30];
	char cmd[50];
	char fileNameBuf[50];
	GST_DISKINFO stDiskInfo;
	

	DISK_GetAllDiskInfo();     // get all disk information.

	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}
	

	g_iDiskIsRightFormat[iDiskId] = 0;	

	stDiskInfo = DISK_GetDiskInfo(iDiskId );

	rel = FS_MachineRightShutdown(iDiskId);
	if( rel < 0 )
	{
		printf("FS_MachineRightShutdown error rel = %d\n",rel);
	}

	// umount all patitions.
	for( j = 1 ; j < stDiskInfo.partitionNum; j++ )
	{	
	
		memset(devName, 0, 30);

		sprintf(devName,"/tddvr/%s%d", &stDiskInfo.devname[5],stDiskInfo.partitionID[j]); // maybe devName is /tddvr/hda5

		memset(cmd,0x00,50);

		//sprintf(cmd,"umount %s",devName);

		//DPRINTK(" cmd:%s\n",cmd);

		//command_ftc(cmd);    // mkdir /tddvr/hda5

		rel = umount(devName);

		if( rel == -1 )
		{
			printf(" can't umount %s%d ", stDiskInfo.devname,stDiskInfo.partitionID[j]);
			return ERROR;
		}
		
		
	}	

	return ALLRIGHT;
}

int FS_LogCheck(int iDiskId)    // make sure diskinfo.log is right.
{
	int iDiskNum = 0;
	int j;	
	FILE * fp = NULL;
	GST_DISKINFO * buf = NULL;
	GST_DISKINFO diskinfo_1;
	char devName[30];
	int   rel=0;
	char cmd[50];
	char fileName[30];
	GST_DISKINFO stDiskInfo;

	buf = &diskinfo_1;

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(iDiskId );
	
	memset(devName, 0, 30);

	sprintf(devName,"/tddvr/%s5", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	memset(cmd,0x00,50);

	//sprintf(cmd,"mkdir %s",devName);

	//rel = command_ftc(cmd);    // mkdir /tddvr/hda5

	mkdir(devName,0777);

	memset(cmd,0x00,50);

	//sprintf(cmd,"mount -t vfat %s5 %s",stDiskInfo.devname,devName);  // mount /dev/hda5

	//rel = command_ftc(cmd);

	sprintf(cmd,"%s5",stDiskInfo.devname);

	rel = ftc_mount(cmd,devName);

	if( rel == -1 )
	{
			DPRINTK(" can't mount %s %s\n", cmd,devName);
			return LOGERROR;
	}

	// read diskinfo.log		
/*	buf = (GST_DISKINFO *)malloc(sizeof( GST_DISKINFO));

	if( !buf )
	{
			printf("no enough memory!\n");
			return LOGERROR;
	}
	*/

	sprintf(fileName,"/tddvr/%s5/diskinfo.log",&stDiskInfo.devname[5]);

	fp = fopen(fileName, "rb");

	if( fp == NULL )
	{
			printf(" open %s error!\n", fileName);			
			umount( devName );
			return LOGERROR;
	}
	
	// get GST_DISKINFO from diskinfo.log.
	rel = fread(buf, 1, sizeof( GST_DISKINFO ), fp);
	if( rel != sizeof(  GST_DISKINFO ) )
	{
			printf(" read error! rel = %d GST_DISKINFO =%d\n",rel,sizeof(  GST_DISKINFO ) );			
			fclose(fp);
			umount( devName );

			return LOGERROR;
	}

	if( g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video() >= TD_DRV_VIDEO_SIZE_960 )
	{
		if( buf->cylinder != g_pstCommonParam->GST_DRA_Net_cam_get_support_max_video() )
		{
			printf(" disk video format error\n");			
			fclose(fp);
			umount( devName );
			return LOGERROR;
		}
	}

	

	if( buf->size != stDiskInfo.size )
	{
			printf(" disk info error\n");			
			fclose(fp);
			umount( devName );
			return LOGERROR;
	}


	for( j = 0 ; j < stDiskInfo.partitionNum; j++ )
	{
			if( buf->partitionSize[j] != stDiskInfo.partitionSize[j] )
			{
				printf(" disk info error\n");			
				fclose(fp);
				umount( devName );
				return LOGERROR;
			}					
	}		
	//free( buf);
	fclose(fp);

	sprintf(fileName,"/tddvr/%s5/record.log",&stDiskInfo.devname[5]);

	fp = fopen(fileName, "rb");
	if( fp == NULL )
	{
		printf(" record.log error\n");		
		umount( devName );
		return LOGERROR;
	}
	fclose(fp);

	sprintf(fileName,"/tddvr/%s5/fileinfo.log",&stDiskInfo.devname[5]);
	fp = fopen(fileName, "rb");
	if( fp == NULL )
	{
		printf(" fileinfo.log error\n");	
		umount( devName );
		return LOGERROR;
	}
	fclose(fp);

	sprintf(fileName,"/tddvr/%s5/recordadd.log",&stDiskInfo.devname[5]);
	fp = fopen(fileName, "rb");
	if( fp == NULL )
	{
		printf(" recordadd.log error\n");	
		umount( devName );
		return LOGERROR;
	}
	fclose(fp);

	// read diskinfo.log over. 	
	rel = umount( devName );
	if( rel == -1 )
	{
			DPRINTK(" can't umount %s \n", devName);
			return LOGERROR;
	}	

	sprintf(devName,"/dev/%s6",&stDiskInfo.devname[5]);

	rel = check_io_file_version(devName);
	if(rel  < 0 )
	{		
		DPRINTK("check_io_file_version error!\n");
		return LOGERROR;
	}else
	{
		if( rel == DF_INVALID_DISK )
		{
			DISK_Set_No_DISK_FLAG(1);
			DPRINTK("DISK_Set_No_DISK_FLAG 1\n");
		}
	}
	
	return ALLRIGHT;
	
}

int FS_IO_FILE_CHECK(int iDiskId)
{
	int iDiskNum = 0;
	int j;	
	char devName[30];
	int   rel=0;
	GST_DISKINFO stDiskInfo;

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(iDiskId );
	
	memset(devName, 0, 30);

	sprintf(devName,"/dev/%s6",&stDiskInfo.devname[5]);

	rel = check_io_file_version(devName);
	if(rel  < 0 )
	{		
		DPRINTK("check_io_file_version error!\n");
		return LOGERROR;
	}else
	{
		if( rel == DF_INVALID_DISK )
		{
			DISK_Set_No_DISK_FLAG(1);
			DPRINTK("DISK_Set_No_DISK_FLAG 1\n");
			return DF_INVALID_DISK;
		}
	}
	
	return ALLRIGHT;

}

int FS_CheckNewDisk(int iDiskId)
{
	int rel;	

	if( g_iDiskIsRightFormat[iDiskId] == 1 )
	{
		printf(" g_iDiskIsRightFormat[%d] = %d", iDiskId,1);
		return NO;
	}

	rel = FS_TypeCheck(iDiskId);
	if( rel == DISKTYPEERROR )
	{
		printf("FS_TypeCheck error!\n");	
		return YES;
	}

	rel =  FS_LogCheck(iDiskId);
	if( rel == LOGERROR )
	{
		printf("FS_LogCheck error!\n");		
		return YES;
	}	

	return NO;	
}


// 返回的单位是(K)
int FS_GetDiskUseSize(char * devName)
{
	int rel,iUsbNum;		
	GST_DISKINFO stDiskInfo;
	char cmd[200];
	FILE *fp=NULL;
	long fileOffset = 0;
	char percent[4];
	char UsbUseSize[11];
	int iPercent=0;
	int j;
	int iUseSize;
		

	DISK_GetAllDiskInfo();     // get all disk information.

	iUsbNum = DISK_HaveUsb();   // get the number of disk whick PC have.

	if( iUsbNum == 0 )
	{
		printf( " no usb ! ");		
		return NOUSBDEVICE;
	}

	stDiskInfo = DISK_GetUsbInfo();

	if( devName == NULL )
	{
		rel = FS_MountUsb();
		if( rel < 0 )
		{
			printf("FS_MountUsb error %d\n",rel);
			return ERROR;
		}
	}

	sprintf(cmd,"df %s%d > /bin/1.txt",stDiskInfo.devname,stDiskInfo.partitionID[0]);

	rel = command_ftc(cmd);
	if( rel < 0 )
	{
		printf("%s error!\n",cmd);
		return ERROR;
	}

	if( devName == NULL )
	{
		rel = FS_UMountUsb();
		if( rel < 0 )
		{
			printf("FS_UMountUsb error %d\n",rel);
			return ERROR;
		}
	}

	fp = fopen("/bin/1.txt","r");
	if( fp == NULL )
	{
		printf(" open /bin/1.txt error!\n");
		return FILEERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		return ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		return ERROR;
	}

	printf(" fileOffset = %d\n",fileOffset);

	rewind(fp);

	if( fileOffset < 200 )
	{
		rel = fread(cmd,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			fclose(fp);
			return ERROR;
		}

		// 得到df 命令中磁盘的使用百分比.
		for( rel = fileOffset; rel > 1; rel--)
		{
			if(cmd[rel] == '%')
			{			

				for(j = 9 ; j >= 0; j--)
				{
					if( cmd[rel - 5] != ' ')
					{
						//printf("%d %c \n",rel,cmd[rel - 5]);
						UsbUseSize[j] = cmd[rel - 5];
						rel--;
					}
					else
						break;
				}

			//	printf("j=%d\n",j);

				for(rel = 0; rel <= j; rel++ )
					UsbUseSize[rel] = '0';
				

				iUseSize = atoi(UsbUseSize);

				printf("iUseSize = %d\n",iUseSize);

				fclose(fp);
				fp = NULL;				

				// 返回未使用的磁盘大小(K bytes)
				return iUseSize;
				
			}
		}
		
	}	
	
	return ERROR;
}

int FS_EnableDiskReCover(int iFlag)
{
	int i,j,rel;
	int iDiskNum;
	DISKCURRENTSTATE   stDiskState;

	printf("come to FS_EnableDiskReCover iFlag = %d\n",iFlag);

	iDiskNum = DISK_GetDiskNum();   // get the number of disk whick PC have.
	if( iDiskNum == 0 )
	{
		printf( " no disk ! ");
		return NODISKERROR;
	}

	for(j = 0; j <iDiskNum; j++)
	{
	//	printf("iDiskNum=%d j=%d\n",iDiskNum,j);
		
		if( FS_DiskFormatIsOk(j) == 0 )
			continue;
	
		rel = FS_GetDiskCurrentState(j, &stDiskState);
		if( rel < 0 )
			return rel;	

		printf(" stDiskState.iEnableCover = %d\n",stDiskState.iEnableCover);
		
		if( iFlag == 0 )
		{
			stDiskState.iEnableCover  = DISNABLE;
		}

		if( iFlag == 1 )
		{
			stDiskState.iEnableCover  = ENABLE;
		}

		printf("Set Disk iEnableCover=%d\n",stDiskState.iEnableCover );
								
		rel = FS_WriteDiskCurrentState(j, &stDiskState);
		if( rel < 0 )
			return rel;
		
	}
	

//	printf(" out FS_EnableDiskReCover\n");

	return ALLRIGHT;
}


int FS_AllDiskEnableDMA(int iFlag)
{
	int i,iDiskNum;
	int flag;
	int rel;
	GST_DISKINFO stDiskInfo;

	flag = iFlag;

	printf(" DMA flag=%d\n",flag);

	DISK_GetAllDiskInfo();

	iDiskNum = DISK_GetDiskNum();

	for(i = 0 ; i <  iDiskNum; i++ )
	{
		stDiskInfo = DISK_GetDiskInfo(i);

		rel = DiskSetUMDMA66(stDiskInfo.devname);

		if( rel < 0 )
			printf("%s 66 open error!\n",stDiskInfo.devname);
		else
			printf("%s 66 open suceess!\n",stDiskInfo.devname);
	}

	return ALLRIGHT;
}

////////////////////////////////////////////////////////////////////////
//硬盘休眠
#define SLEEP_CHECK_TIME (1*60*2)
#define SLEEP_TIME2 (1*60*20)

static int disk_sleep[20] = {1}; 
static time_t disk_use_time[20] = {0};
static int use_sleep_sys = 1;

void FS_Disk_Use(int iDisk)
{
	struct timeval stTimeVal;	

	g_pstCommonParam->GST_DRA_get_sys_time( &stTimeVal, NULL );
	
	disk_use_time[iDisk] = stTimeVal.tv_sec;
	
}

int FS_Disk_Is_Sleep(int iDisk)
{	

	return disk_sleep[iDisk];
}

int FS_Disk_Is_Use(time_t searchStartTime,time_t searchEndTime,int idisk)
{

	if( use_sleep_sys == 0 )
		return 1;

	#ifdef USE_PLAYBACK_QUICK_SYSTEM

	GST_DISKINFO stDiskInfo;
	int j;

	stDiskInfo = DISK_GetDiskInfo(idisk);

	for(j = 1; j < stDiskInfo.partitionNum; j++ )
	{

		if( FTC_Check_disk_time(searchStartTime,searchEndTime,idisk,stDiskInfo.partitionID[j])== 1 )		
			return 1;

	}

	return 0;

	#endif
}

void FS_WakeUp_Disk(int iDisk)
{
	DISKCURRENTSTATE   stDiskState;	
	
	FS_GetDiskCurrentState(iDisk, &stDiskState);

	disk_sleep[iDisk] = 1;
	
}

void FS_Disk_Sleep()
{
	int i,iDiskNum;
	int disk1,disk2;
	GST_DISKINFO diskinfo;

	struct timeval stTimeVal;	


	if( use_sleep_sys == 0 )
		return 1;

	g_pstCommonParam->GST_DRA_get_sys_time( &stTimeVal, NULL );

	DISK_GetAllDiskInfo();

	iDiskNum = DISK_GetDiskNum();


	if( iDiskNum <= 2 )
	{
		DPRINTK("iDiskNum=%d not use disk sleep\n",iDiskNum);
		return ;
	}

	disk1 = g_stLogPos.iDiskId;
	disk2 = disk1 + 1;

	if(disk2 >= iDiskNum )
		disk2 = 0;


	for(i=0; i<iDiskNum; i++)
	{
		if( (i == disk1) || (i == disk2) )
		{
			FS_WakeUp_Disk(i);
			DPRINTK("FS_WakeUp_Disk:  %d\n ",i);
			continue;
		}
	
		if( (stTimeVal.tv_sec - disk_use_time[i]) > SLEEP_TIME2 )
		{
			diskinfo = DISK_GetDiskInfo( i );

			DISK_DiskSleep(diskinfo.devname);

			disk_sleep[i] = 0;

			DPRINTK("DISK_DiskSleep:  %s use_time=%ld cur_time=%ld\n",diskinfo.devname,disk_use_time[i],stTimeVal.tv_sec);
		}


		if( stTimeVal.tv_sec < disk_use_time[i] )
		{
			disk_use_time[i] = 0;
		}
	}

	
}


void Disk_sleep_sys_ini()
{
	int iDiskNum,i;


	for(i = 0; i < 20 ; i++ )
	{
		disk_sleep[i] = 1;
		disk_use_time[i] = 0;
	}
	
	DISK_GetAllDiskInfo();

	iDiskNum = DISK_GetDiskNum();


	if( iDiskNum <= 2 )
	{
		use_sleep_sys = 0;
		DPRINTK("disknum=%d, not use disk sleep system\n",iDiskNum);

		return ;
	}else
	{
		use_sleep_sys = 1;
		DPRINTK("disknum=%d, use disk sleep system: check time = %d sec\n",iDiskNum,SLEEP_CHECK_TIME);
	}
	

	#ifdef USE_PLAYBACK_QUICK_SYSTEM

	FTC_GetEveryDiskRecTime();

	#endif
}



void thread_for_create_disk_sleep(void)
{
	SET_PTHREAD_NAME(NULL);
	while(1)
	{
		sleep(SLEEP_CHECK_TIME);

		FS_Disk_Sleep();
		
	}
}




