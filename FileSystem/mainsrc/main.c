
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h> 
#include <time.h>


#include "devfile.h"
#include "saveplayctrl.h"
#include "fic8120ctrl.h"

#include <dirent.h>

#define IO_BASE1	                             0xf0000000
#define IO_ADDRESS1(x)                    (((x>>4)&0xffff0000)+(x&0xffff)+IO_BASE1) 
#define PHY_ADDRESS1(x)                  (((x<<4)&0xfff00000)+(x&0xffff))

#define FCAP_CONTROL  (IO_ADDRESS1(0x96800000))


//#define TEST_MOTION

GST_DEV_FILE_PARAM  gstCommonParam;

int gAppExitFlag = 0;

int command(char * command)
{	
	int	res = 1;
	
	res = SendM2(command);
	if(res == 0x00)
	{				
		return res;
	}
	else
	{				
		return -1;
	}	
	
	return -1;
}


void DVR_Catch(int sig)
{ 
	printf( "catch sig\n" );
	gAppExitFlag = 1;
} 

void DVR_MAIN_exit()
{
	gAppExitFlag = 1;
}

/* read a key without blocking */
static int read_key(void)
{
    int n = 1;
    unsigned char ch;
    struct timeval tv;
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    n = select(1, &rfds, NULL, NULL, &tv);
    if (n > 0) {
        n = read(0, &ch, 1);
        if (n == 1)
            return ch;

        return n;
    }
    return -1;
}

void help_func(char * func)
{
	printf(" %s pal/ntsc cif/d1/720p/1080p/480p/576p\n",func);	
	printf(" for example: %s pal 1080p\n",func);	
}

int main(int argc,char **argv )
{


	SAMPLE_VENC_NORMALP_CLASSIC2();
	return 0;

	int ret;
	int idx;
	int iDiskNumber;
	int key;
	struct timeval nowTime;
	int show_size = 1;
	int chow_mid = 0;
	int a,b;
	time_t play_time;
	GST_FILESHOWINFO fileshow;
	char filename[50];
	unsigned char * yuv_buf= NULL;
	unsigned char * yuv_buf2= NULL;
	video_profile   video_setting[16];
	int fcap_register = 0;
	unsigned short *  motion_buf = NULL;
	int x,y,h;
	int enable_motion = 0;
	char gateway[200],client_ip[200],dns1[200],dns2[200];
	int video_hd_type = 0;
	int disk_num = 0;
	int i = 0;

	a = TD_DRV_VIDEO_STARDARD_PAL;
	b = TD_DRV_VIDEO_SIZE_D1;

	if( argc != 3 )
	{
		help_func(argv[0]);
		exit(0);
	}else
	{		
	
		if( strcmp(argv[1],"pal") == 0 )
		{
			a = TD_DRV_VIDEO_STARDARD_PAL;
		}else
		{
			a = TD_DRV_VIDEO_STARDARD_NTSC;
		}

		if( strcmp(argv[2],"cif") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_CIF;
		}else if(strcmp(argv[2],"d1") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			
		}else if(strcmp(argv[2],"720p") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			video_hd_type = TD_DRV_VIDEO_SIZE_720P;
		}
		else if(strcmp(argv[2],"1080p") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			video_hd_type = TD_DRV_VIDEO_SIZE_1080P;
		}else  if(strcmp(argv[2],"480p") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			video_hd_type = TD_DRV_VIDEO_SIZE_480P;
		}else  if(strcmp(argv[2],"576p") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			video_hd_type = TD_DRV_VIDEO_SIZE_576P;
		}else  if(strcmp(argv[2],"960p") == 0 )
		{
			b = TD_DRV_VIDEO_SIZE_D1;
			video_hd_type = TD_DRV_VIDEO_SIZE_960P;
		}else
		{
			
		}

		printf("func   %s %s video_hd_type=%d\n",argv[1],argv[2],video_hd_type);
	}


	for( ret = 0 ; ret < 16 ; ret++ )
	{
	    video_setting[ret].qmax = 31;
	    video_setting[ret].qmin = 1;
	    video_setting[ret].quant = 0;
	    	
	    if( a  == TD_DRV_VIDEO_STARDARD_PAL)
	  	  	video_setting[ret].framerate = 25;
	    else
			video_setting[ret].framerate = 30;
	   video_setting[ret].bit_rate = 4000/25*video_setting[ret].framerate;
	   if( ret == 1 )
	   	video_setting[ret].bit_rate = 1200;
	    video_setting[ret].frame_rate_base = 1;
	    video_setting[ret].gop_size = 30;	 
	    video_setting[ret].width = 720;
           video_setting[ret].height = 572;
	    video_setting[ret].decode_width = 720;
	    video_setting[ret].decode_height = 572;
	}
	 

	printf( "Test Program Is Begin!11\n");
	
	command("mkdir /tddvr");
	//command("insmod fcap_drv.o");
	//command("insmod fmpeg4_drv.o");
	
	
//	signal(SIGINT, DVR_Catch );
//	signal(SIGPIPE,SIG_IGN);
	
	memset( &gstCommonParam, 0, sizeof(GST_DEV_FILE_PARAM ) );


/////////////////////////////////////////////////////////
///////////////////main init///////////////////////////////
/////////////////////////////////////////////////////////

//	gstCommonParam.GST_DVR_MAIN_exit = DVR_MAIN_exit;
	
	printf("DRV_Control_function\n");
	
	drv_ctrl_lib_function( &gstCommonParam );

	printf( "FTC_GetFunctionAddr\n" );
	FTC_GetFunctionAddr( &gstCommonParam );

	if( video_hd_type != 0 )
	{
		gstCommonParam.GST_DRA_Net_cam_set_support_max_video(video_hd_type);
	}


	
	gstCommonParam.GST_FS_PhysicsFixAllDisk();

	sync();
	printf(" sync ...\n");

	SendM2("cat /proc/meminfo");

	//return 0;	



	gstCommonParam.GST_DISK_GetAllDiskInfo();
	
	
	disk_num = gstCommonParam.GST_DISK_GetDiskNum();   // get the number of disk whick PC have.

	for( i = 0; i < disk_num; i++)
	{
		gstCommonParam.GST_FS_UMountAllPatition(i);

		ret = gstCommonParam.GST_FS_CheckNewDisk(i);

		printf(" GST_FS_CheckNewDisk ret = %d\n",ret );


		if( ret == 1 )
		{
				gstCommonParam.GST_FS_PartitionDisk(i);
				gstCommonParam.GST_FS_BuildFileSystem(i);
				gstCommonParam.GST_FS_MountAllPatition(i);
		}else
			gstCommonParam.GST_FS_MountAllPatition(i);

	}


	if(1)
	{
		#ifdef hi3515
		gstCommonParam.GST_DRA_set_dvr_max_chan(1);
		gstCommonParam.GST_DRA_Hisi_set_net_mode(1);
		#else
		gstCommonParam.GST_DRA_set_dvr_max_chan(1);
		#endif
	}else
	{
		gstCommonParam.GST_DRA_set_dvr_max_chan(8);
		gstCommonParam.GST_DRA_Hisi_set_chip_slave();
	}

//	gstCommonParam.GST_DRA_Hisi_set_hd_mode(HD_MODE_VGA_1280x1024_60);

	printf( "DTC_create_thread\n" );
	ret = FTC_InitSavePlayControl( &gstCommonParam );
	if ( ret < 0 )
	{
		printf( "main() FTC_InitSavePlayControl error\n" );
		exit( 1);
	}

//	gstCommonParam.GST_NET_set_net_parameter("192.168.1.50",6802,"192.168.1.1");

	printf( "init_all_device\n" );
	ret = init_all_device(NULL,a,b);
	if ( ret < 0 )
	{
		printf( "main() InitAllDevice error\n" );
		exit( 1);
	}

//	printf("init init_uart_dev!\n");

//	init_uart_dev();

//////////////////////////////////////////////////
////////////////create thread///////////////////////
//////////////////////////////////////////////////
	
	

	printf( "FTC_CreateAllThread\n" );
	ret = FTC_CreateAllThread(  );
	if ( ret < 0 )
	{
		printf( "main() threadmenu_module_init error\n" );
		exit(1);
	}

	gstCommonParam.GST_FTC_GetRecMode(a,b);
	gstCommonParam.GST_FTC_CurrentPlayMode(&a,&b);
	gstCommonParam.GST_FTC_AudioRecCam(0x00);
	gstCommonParam.GST_FS_EnableDiskReCover(1);

	printf(" rec audio !1!!\n");

	gstCommonParam.GST_NET_get_admin_info("ADMIN0","000000","USER01","111111","","","","");

//	if( b == TD_DRV_VIDEO_SIZE_CIF)
//	gstCommonParam.GST_DRA_change_device_set(&video_setting[0],a,b);

	

	printf( "main loop begin\n" );
	gstCommonParam.GST_NET_set_net_parameter("192.168.17.107",6802,"192.168.17.1","255.255.255.0","202.96.134.133","202.96.128.166");
	while( !gAppExitFlag )
	{
		  key = read_key();
		  switch(key)
		  {
		  	case 'q':
				gAppExitFlag = 1;
				printf("quit!\n");
				break;
			case 'a':
				printf("rec start!\n");
				ret = gstCommonParam.GST_FTC_StartRec(0xffff,1);
				if( ret > 0 )
					printf(" rec start success!\n");
				break;
			case 's':
				printf("rec stop!\n");
				ret = gstCommonParam.GST_FTC_StopRec();
				if( ret > 0 )
					printf(" rec stop success!\n");
				break;
			case 'd':
				printf("play near rec file start!\n");
				 gettimeofday( &nowTime, NULL );
				//gstCommonParam.GST_FTC_GetRecordlist(stf,0x01,500,600,page);			
				gstCommonParam.GST_FTC_SetTimeToPlay(nowTime.tv_sec - 10,0,0x1,1);
				 if( ret > 0 )
					printf("play near rec file  success!\n");
				 play_time = nowTime.tv_sec;
				 gstCommonParam.GST_DRA_Hisi_playback_window_vo(1,0);
				break;
			case 'f':
				printf("play stop!\n");
				gstCommonParam.GST_FTC_StopPlay();
				 if( ret > 0 )
					printf("play stop success!\n");
				break;
			case 'g':
				printf("format disk!\n");
				
				gstCommonParam.GST_FS_UMountAllPatition(0);
				gstCommonParam.GST_FS_PartitionDisk(0);
				gstCommonParam.GST_FS_BuildFileSystem(0);
				gstCommonParam.GST_FS_MountAllPatition(0);
				
				break;	
			case 'h':
				printf(" clearing log!\n");
				ret=gstCommonParam.GST_FS_ClearDiskLogMessage(0);
				if( ret > 0 )
					printf(" clear log! success!\n");
				break;
			case 'j':
				printf(" send uart!\n");
			//	UART1_send_thread_pro1();
			gstCommonParam.GST_FTC_PreviewRecordStart(10);
				break;
			case 'k':
				printf(" show cif to d1 size!\n");
				//show_cif_to_d1_size(show_size);
			//	show_size = !show_size;
			gstCommonParam.GST_FTC_PreviewRecordStop(0);
				break;
			case 'w':
				printf("change dec enc\n");
				ret = gstCommonParam.GST_DRA_change_device_set(&video_setting[0],a,b);
				printf(" ret = %d\n",ret);
				break;
			case 'e':
				printf("channel 5\n");
				//fileshow.iCam = 15;
			//	ret = gstCommonParam.GST_FS_ListWriteToUsb(&fileshow,2);	
			//	printf(" ret = %d\n",ret);
			//	ret = gstCommonParam.GST_FTC_SetTimeToPlay(	play_time,0,0x10,1);
			//	printf(" ret = %d\n",ret);

				gstCommonParam.GST_FTC_StartRec(0xf0,100);
				break;
			case 'r':
				printf("GST_DRA_enable_local_audio_play 1\n");
				gstCommonParam.GST_DRA_enable_local_audio_play(1,0x01);
				break;	
			case 't':
				printf("GST_DRA_enable_local_audio_play 0\n");		
				gstCommonParam.GST_DRA_enable_local_audio_play(0,0x00);
				break;	
			case 'y':
				//system("cat /proc/meminfo");	
				gstCommonParam.GST_NET_set_net_parameter("192.168.17.68",6802,"192.168.17.1","255.255.255.0","202.96.134.133","202.96.128.166");
				break;	
			case 'u':				
				//gstCommonParam.GST_NET_kick_client();

				//ret = gstCommonParam.GST_NET_try_dhcp(client_ip,gateway,dns1,dns2);

				printf("GST_NET_try_dhcp ret = %d %s %s %s %s\n",ret,client_ip,gateway,dns1,dns2);
				break;
			case 'i':		
				//gstCommonParam.GST_DRA_Clear_UPnP2(6802);
				break;	
			case 'z':
				//gstCommonParam.GST_DRA_Hisi_set_spot_window(0);
				/*ret = gstCommonParam.GST_NET_dvr_send_mail("smtp.163.com",25,"yuebin88888","22first",
										"yuebin88888@163.com","binyuebin@163.com",
										"dvr mail test","dvr machine send",NULL,2);
				*/
				printf(" ret = %d\n",ret);
				
				break;
			case 'x':
				if( yuv_buf )
				{					
					gstCommonParam.GST_OSD_osd_24bit_photo(yuv_buf2,230,100,TD_DRV_VIDEO_STARDARD_PAL);
					gstCommonParam.GST_OSD_osd_2bit_photo(yuv_buf,60,100,TD_DRV_VIDEO_STARDARD_PAL,
								51, 75, 0,20,0x8c9bb6,0x5cd273);
						//51, 75, 30,20,0x8ab776,0x515aef);
				}
				
				break;
			case 'v':
			//	gstCommonParam.gst_ext_read_fic(0x96800000,(unsigned int *)&fcap_register);
			//	printf(" rg = %x",fcap_register);
				gstCommonParam.GST_FTC_PlayFast(1);
				gstCommonParam.GST_DRA_Hisi_set_vo_framerate(0.2);
				break;

			case '1':
			//	gstCommonParam.GST_DRA_Hisi_window_vo(1,0,0,0,0,0,0,0,0,0);
				//printf(" rg = %x",fcap_register);
				break;
			case '2':
				//ret = gstCommonParam.GST_FS_scan_cdrom();
				//printf("GST_FS_scan_cdrom  ret  = %d\n",ret);
				//gstCommonParam.GST_DRA_Hisi_window_vo(1,1,20,20,300,300,50,50,400,400);
			//	break;
			case '3':
				//ret = gstCommonParam.GST_FS_scan_cdrom();
				//printf("GST_FS_scan_cdrom  ret  = %d\n",ret);
			//	gstCommonParam.GST_DRA_Hisi_window_vo(1,0,0,0,0,0,0,0,0,0);
				break;
			case '4':
			//	gstCommonParam.GST_DRA_Hisi_window_vo(4,0,0,0,0,0,0,0,0,0);			
				break;

			case '5':
				gstCommonParam.GST_DRA_drv_start_motiondetect();		
				break;
			case '6':
				gstCommonParam.GST_DRA_drv_stop_motiondetect();		
				break;

			case '8':
			//	gstCommonParam.GST_DRA_Hisi_window_vo(9,0,0,0,0,0,0,0,0,0);
				//printf(" rg = %x",fcap_register);
				break;			
			case '9':
				//gstCommonParam.GST_DRA_drv_start_motiondetect();
			//	gstCommonParam.GST_DRA_Hisi_window_vo(9,0,0,0,0,0,0,0,0,0);
				/*{
					ret= gstCommonParam.GST_FS_FormatUsb();
					printf("format_usb ret = %d\n",ret);
				}*/
				break;
			case '0':
				//gstCommonParam.GST_DRA_drv_stop_motiondetect();
			//	gstCommonParam.GST_DRA_Hisi_window_vo(16,0,0,0,0,0,0,0,0,0);
				/*{
					int have_usb=0,format_is_right=0;
					gstCommonParam.GST_FS_CheckUsb(&have_usb,&format_is_right);

					printf("have_usb=%d format_is_right=%d\n",have_usb,format_is_right);

					
				}*/
				break;
			default:
				break;
				
		  }

			
		#ifdef TEST_MOTION

		if( enable_motion == 1)
		for( h = 0; h < 3; h++ )
		{
			int find = 0;
			int find2 = 0;
			int mov_count = 0;
			motion_buf = gstCommonParam.GST_DRA_get_motion_buf_hisi(h);

			if( motion_buf != NULL )
			{
				if( h == 0 )
				{
					find = 0;
					find2 = 100;
					mov_count = 0;					
					for( y =0; y < 36;y++)
					{						
						for(x=0; x < 44; x++ )
						{						
							//printf("%0.2d,",motion_buf[y*22+x]);
							if( motion_buf[y*44+x] > 10 )
							{
								//printf("(%d,%d)=%d\n",x,y,motion_buf[y*22+x]);
								//find = 1;
								if( find < motion_buf[y*44+x] )
									find = motion_buf[y*44+x];

								if( find2 > motion_buf[y*44+x] )
									find2 = motion_buf[y*44+x];

								mov_count++;
							}

							
						}

						//printf("\n");
						
					}

					if( find != 0 )
					{
						printf("(%d,%d)%d\n",find,find2,mov_count);
					}

					//printf("===================\n");
				}
			}
		}

		#endif
		

		usleep(1000);
	}
	
	printf( "Test Program Is end!\n");

	FTC_StopSavePlay();	

	close_all_device();

	//close_uart_dev();

//	signal(SIGINT,SIG_DFL );
//	signal(SIGPIPE,SIG_DFL);
	


	printf( "Test Program Is Over!\n");
	return 0;
	
	
	
}

