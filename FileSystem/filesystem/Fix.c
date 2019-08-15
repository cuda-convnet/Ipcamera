#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "filesystem.h"


int PhysicsFixDisk(char * devName)
{
/*
	GST_DISKINFO stDiskInfo;

	char cmd[50];

	int rel;

	memset(cmd, 0, 50);

	sprintf(cmd,"dosfsck -Vva %s", devName); // maybe devName is /tddvr/hda5

	printf("%s\n",cmd);

	rel = command_ftc(cmd);

	if( rel == -1 )
	{
		printf(" %s error!\n",cmd);
		return ERROR;
	}
*/
	int   pid;
	pid = vfork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
		return ALLRIGHT;
	} 
	else if (pid ==0) 
	{
		printf("PhysicsFix %s\n",devName);
		execl("/bin/dosfsck", "dosfsck", "-a", devName, (char* )0);		
		exit(1);
	}

	return ALLRIGHT;	
}

int CheckShutdownStatus(int iDiskId)
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

	DISK_GetAllDiskInfo();     // get all disk information.

	stDiskInfo = DISK_GetDiskInfo(iDiskId );
	
	memset(devName, 0, 30);

	sprintf(devName,"/tddvr/%s5", &stDiskInfo.devname[5]); // maybe devName is /tddvr/hda5

	memset(cmd,0x00,50);

//	sprintf(cmd,"mkdir %s",devName);

//	rel = command_ftc(cmd);    // mkdir /tddvr/hda5

	mkdir(devName,0777);

	memset(cmd,0x00,50);

//	sprintf(cmd,"mount -t vfat %s5 %s",stDiskInfo.devname,devName);  // mount /dev/hda5

//	rel = command_ftc(cmd);

	sprintf(cmd,"%s5",stDiskInfo.devname);

	rel = ftc_mount(cmd,devName);

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
		fclose(fp);
		fp = NULL;
		umount( devName );
		return ERROR;
	}	
	

	return ALLRIGHT;
}

int PhysicsFixAllDisk()
{
	GST_DISKINFO stDiskInfo;
	int iDiskNum;	
	int i,j,k;	
	char DevName[50];
	int rel;
	char cmd[50];
	
	DISK_GetAllDiskInfo();  

	iDiskNum = DISK_GetDiskNum();

	for(i = 0 ; i < iDiskNum; i++ )
	{	

		rel = CheckShutdownStatus(i);
		if( rel < 0 )
		{
			// 如果是MOUNTERROR 错误的话，百分之八十是个新硬盘，所以不修复
			if( rel != MOUNTERROR)
			{
				stDiskInfo = DISK_GetDiskInfo(i);
			
				for(j = 1; j < stDiskInfo.partitionNum; j++ )
				{
					sprintf(DevName,"%s%d",stDiskInfo.devname, stDiskInfo.partitionID[j]);
					printf("%s \n",DevName);
					PhysicsFixDisk(DevName);
				}
				
			}
		}
	}

	return ALLRIGHT;
	
}


int  main(void)
{	
	PhysicsFixAllDisk();

	return 1;
}



