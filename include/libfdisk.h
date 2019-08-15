#ifndef _LIBFDISK_H_
#define _LIBFDISK_H_

#include "../include/devfile.h"
#include <time.h>

#define TRUE   		1			/* Boolean constants */
#define FALSE 		0
#define ENABLE  		1
#define DISEABLE 	0


#define MAXCHANNEL  4

#define WIN95_FAT32   	 	0x0b
#define Win95_FAT32_LBA 	0x0c
#define EXTENDED          		0x05
#define WIN98_EXTENDED  	0x0f


#define MAXDEVNUM     	8

#define MAX_USB_SIZE (30)  // G bytes

#define USE_FILE_IO_SYS 




int DISK_SetDisk(int disk_id, int size);
void DISK_GetAllDiskInfo();
int  DISK_GetDiskNum();
GST_DISKINFO DISK_GetDiskInfo(int disk_id);
GST_DISKINFO * DISK_GetDiskInfoPoint(int disk_id);
int DISK_Mount(char * dev, char * targetDir);
int DISK_UnMount(char * dev);
int DISK_Formatdisk(char * devname, int IsCheck);
int DISK_DiskSleep(char * device);
int DISK_EnableDMA(char * device, int flag);// flag = ENABLE is enable DMA, flag = DISEABLE is disable DMA.
GST_DISKINFO DISK_GetUsbInfo();
int DISK_HaveUsb();
int DiskSetUMDMA66(char * device);
int DISK_FormatUsb( int size);
int DISK_Set_No_DISK_FLAG(int flag);


#endif
