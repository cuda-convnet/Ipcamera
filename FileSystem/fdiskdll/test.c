#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     /*Unix标准函数定义*/

#include "libfdisk.h"

GST_DISKINFO diskinfo;

void test1()
{
	int num,i,j;
	char devpatitions[30];
	int disksize;
	int diskid =0 ;
	int stat = 1;
	int patitionid = 0;
	int iSelect;
	int rel;
	
	DISK_GetAllDiskInfo();     // get all disk information.
	
	num = DISK_GetDiskNum();   // get the number of disk whick PC have.
	
	if( num == 0 )
	{
		printf( " no disk ! ");
		return 1;
	}


	// show disk information.
	for( i = 0 ; i < num; i++ )
	{

		diskinfo = DISK_GetDiskInfo( i );
	
		printf("\nDiskID:%d   dev : %s          size :%lld(bytes) \n", i,  diskinfo.devname, diskinfo.size);

		DISK_DiskSleep(diskinfo.devname);
	}

	sleep(5);

	DISK_GetAllDiskInfo();     // get all disk information.
	
	num = DISK_GetDiskNum();   // get the number of disk whick PC have.
	
	if( num == 0 )
	{
		printf( " no disk ! ");
		return 1;
	}


	// show disk information.
	for( i = 0 ; i < num; i++ )
	{

		diskinfo = DISK_GetDiskInfo( i );
	
		printf("\nDiskID:%d   dev : %s          size :%lld(bytes) \n", i,  diskinfo.devname, diskinfo.size);

		//DISK_DiskSleep(diskinfo.devname);
	}
}

int main(void)
{

	int num,i,j;
	char devpatitions[30];
	int disksize;
	int diskid =0 ;
	int stat = 1;
	int patitionid = 0;
	int iSelect;
	int rel;

	test1();

	return 1;
	
	DISK_GetAllDiskInfo();     // get all disk information.
	
	num = DISK_GetDiskNum();   // get the number of disk whick PC have.

	if( num == 0 )
	{
		printf( " no disk ! ");
		return 1;
	}


	// show disk information.
	for( i = 0 ; i < num; i++ )
	{

		diskinfo = DISK_GetDiskInfo( i );
	
		printf("\nDiskID:%d   dev : %s          size :%lld(bytes) \n", i,  diskinfo.devname, diskinfo.size);

		for( j = 0 ; j < diskinfo.partitionNum; j++ )
		{
			memset(devpatitions,0,30 );
			sprintf(devpatitions,"%s%d", diskinfo.devname,diskinfo.partitionID[j]);

			if( diskinfo.disktype[j] == WIN95_FAT32 || diskinfo.disktype[j] == Win95_FAT32_LBA )
				printf("   %d   %s    %lld(K bytes)    WIN_FAT32\n",j,	devpatitions,diskinfo.partitionSize[j]);

			else if( diskinfo.disktype[j] == EXTENDED || diskinfo.disktype[j] == WIN98_EXTENDED )
				printf("   %d   %s    %lld(K bytes)    EXTENDED\n",j,	devpatitions,diskinfo.partitionSize[j]);

			else
				printf("   %d   %s    %lld(K bytes)    OTHER\n",j,	devpatitions,diskinfo.partitionSize[j]);
		}
	}


	printf("iuput 9 to quit");
	printf(" select  DiskID (0 - %d) ", num -1 );
	
	scanf("%d",&diskid);
	

	printf(" input split patitions size ( >10 GB):");
	scanf("%d",&disksize);

	rel = DISK_SetDisk(diskid, disksize);  // segmentation disk.

	if(  rel == -1 )
	{
			printf("error!");
			exit(0);
	}


	// show disk information.
	DISK_GetAllDiskInfo();

	for( i = 0 ; i < num; i++ )
	{

		diskinfo = DISK_GetDiskInfo( i );
	
		printf("\nDiskID:%d   dev : %s          size :%lld(bytes) \n", i,  diskinfo.devname, diskinfo.size);

		for( j = 0 ; j < diskinfo.partitionNum; j++ )
		{
			sprintf(devpatitions,"%s%d", diskinfo.devname,diskinfo.partitionID[j]);

			if( diskinfo.disktype[j] == WIN95_FAT32 || diskinfo.disktype[j] == Win95_FAT32_LBA )
				printf("   %d   %s    %lld(K bytes)    WIN_FAT32\n",j,	devpatitions,diskinfo.partitionSize[j]);

			else if( diskinfo.disktype[j] == EXTENDED || diskinfo.disktype[j] == WIN98_EXTENDED )
				printf("   %d   %s    %lld(K bytes)    EXTENDED\n",j,	devpatitions,diskinfo.partitionSize[j]);

			else
				printf("   %d   %s    %lld(K bytes)    OTHER\n",j,	devpatitions,diskinfo.partitionSize[j]);
		}
	}


	diskinfo = DISK_GetDiskInfo(diskid);

	printf("\n select patition id to format (1 -%d)",diskinfo.partitionNum - 1);

	scanf("%d",&patitionid);

	if( patitionid >=1 && patitionid < diskinfo.partitionNum )
	{
		sprintf(devpatitions,"%s%d", diskinfo.devname,diskinfo.partitionID[patitionid]);
		
		rel =  DISK_Formatdisk(devpatitions, FALSE );  // format disk. FALSE is stand for not check bak blocks
											// use TRUE to check bak blocks.

		if( rel == 1)
		{	
			printf(" format success!");
		}
		else
		{	
			printf("format error!");
			exit(0);
		}
			
	}
	else
	{
		printf("Wrong number!");
		return -1;
	}
/*
	printf("\n Do you want %s sleep:(y/n)",diskinfo.devname);

	getchar();
	iSelect = getchar();

	if( iSelect == 'y')
	{
		printf("\n set %s sleep.....",diskinfo.devname);
		
		rel = DISK_DiskSleep(diskinfo.devname);   // let disk sleeping.

		if( rel == 1 )
			printf("%s sleep sucess!",diskinfo.devname);
		else
			printf("%s sleep failed!!",diskinfo.devname);
		
	}
*/

	printf("\n Do you want %s Enable DMA:(y/n)",diskinfo.devname);

	getchar();
	iSelect = getchar();

	if( iSelect == 'y')
	{
		printf("\n set %s DMA.....",diskinfo.devname);
		
		rel = DISK_EnableDMA(diskinfo.devname, ENABLE);   // let disk sleeping.

		if( rel == 1 )
			printf("%s Enable DMA sucess!",diskinfo.devname);
		else
			printf("%s EnableDMA failed!!",diskinfo.devname);
		
	}

	printf("\n Do you want mount %s:(y/n)",devpatitions);

	getchar();
	iSelect = getchar();

	if( iSelect == 'y')
	{
		rel = DISK_Mount(devpatitions,"/mnt");    // mount disk.

		if( rel == -1 )
		{
			printf("\n Mount failed!");
			exit(0);
		}

		printf("\n Mount success!");
		
		
		printf("\n Do you want unmount %s:(y/n)",devpatitions);
		
		getchar();
		iSelect = getchar();
		

		if( iSelect == 'y')
		{
			rel = DISK_UnMount(devpatitions);   // umont disk.

			if( rel == -1 )
			{
				printf("\n Umount failed!");
				exit(0);
			}

			printf("\n Umount success!");
		}
		
	}
	else
	{
		printf(" end!");
	}
	
	
	return 1;
}
