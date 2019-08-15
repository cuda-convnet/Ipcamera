#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include <time.h>
#include "ipcam_wifi.h"
#include "devfile.h"

#define WIFI_LIST_FILE "/tmp/wifi_list"
#define WIFI_CONNECT_FILE "/tmp/wifi_connect"

char wifi_ssid[50] = "";
char wifi_passwd[50] = "";
char g_AuthMode[50] = "";


#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("wifi (%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

int is_have_wifi_dev()
{
	int ret = 0;
	ret = command_drv2("cat /proc/net/dev | grep ra0");			
	if( ret == 0)
		return 1;
	else
		return 0;
}

int set_wifi_disconnect()
{
	command_drv2("killall wpa_supplicant");
	command_drv2("iwpriv ra0 set SSID=\"adddd\"");
	command_drv2("ifconfig ra0 up");
	return 1;
}

int is_wifi_connect()
{
	FILE *procpt;
	char line[256];
	int is_connect = 1;
	
	command_drv2("iwpriv ra0 connStatus > "WIFI_CONNECT_FILE);

	procpt = fopen(WIFI_CONNECT_FILE, "r");
	if (procpt == NULL) {
		fprintf(stderr, ("cannot open %s\n"), WIFI_CONNECT_FILE);
		return -1;
	}	

	while (fgets(line, sizeof(line), procpt)) 
	{
		if( FindSTR(line,"Disconnected",0) >= 0 )
		{
			is_connect = 0;
			break;
		}
	}

	fclose(procpt);
	
	return is_connect;
}

int wifi_get_info(char * ssid,char *AuthMode_EncrypType)
{
	FILE *procpt;
	char line[256];
	char num[10][50];
	int find_ssid_wifi = 0;

	AuthMode_EncrypType[0] = 0;	
	
	procpt = fopen(WIFI_LIST_FILE, "r");
	if (procpt == NULL) {
		fprintf(stderr, ("cannot open %s\n"), WIFI_LIST_FILE);
		return -1;
	}		

	while(fgets(line, sizeof(line), procpt)) 
	{
		
		if (sscanf (line, "%s %s %s %s %s %s %s %s",
			    &num[0], &num[1], &num[2], &num[3],&num[4], &num[5], &num[6], &num[7]) != 8)
		{			
			continue;
		}		

		if( strcmp(ssid,num[1]) == 0 )
		{
			DPRINTK("find ssid:%s\n",line);
			DPRINTK("Security: %s\n",num[3]);			
			strcpy(AuthMode_EncrypType,num[3]);			
			find_ssid_wifi = 1;
			break;		
		}	
	}

	fclose(procpt);

	return find_ssid_wifi;
}


int connect_wifi(char * ssid,char * passwd)
{
	int ret_no = -1;
	char AuthMode_EncrypType[256] = "";
	int ret;
	char cmd[256] = "";
	
	command_drv2("ifconfig ra0 up"); //有时候停掉WIFI时ra0会自动down掉，所以up一次。

	
	command_drv2("iwlist ra0 scanning > /tmp/wifi_tmp");
	
	command_drv2("iwpriv ra0 get_site_survey > "WIFI_LIST_FILE);

	ret = wifi_get_info(ssid,AuthMode_EncrypType);
	if( ret <= 0 )
	{		
		ret_no = IPCAM_WIFI_NOT_FIND_SSID;
		goto err;
	}
	
	if( FindSTR(AuthMode_EncrypType,"WPA",0) >= 0 )
	{
		DPRINTK("WIFI USE WPA SECURITY  %s\n",AuthMode_EncrypType);
		sprintf(cmd,"/mnt/mtd/wpa_passphrase %s %s > /tmp/wifi_wpa.conf",ssid,passwd);
		command_drv2(cmd);
		sprintf(cmd,"/mnt/mtd/wpa_supplicant -Dwext -ira0 -c /tmp/wifi_wpa.conf -d &");
		command_drv2(cmd);
		strcpy(g_AuthMode,"WPA");
		
	}else if( FindSTR(AuthMode_EncrypType,"NONE",0) >= 0 ) //"OPEN/NONE"
	{
		command_drv2("iwpriv ra0 set NetworkType=Infra");
		command_drv2("iwpriv ra0 set AuthMode=OPEN");
		command_drv2("iwpriv ra0 set EncrypType=NONE");
		sprintf(cmd,"iwpriv ra0 set SSID=\"%s\"",ssid);
		command_drv2(cmd);
		
	}else if( FindSTR(AuthMode_EncrypType,"SHARED",0) >= 0 ) //"SHARED/WEP"
	{
		command_drv2("iwpriv ra0 set NetworkType=Infra");
		command_drv2("iwpriv ra0 set AuthMode=SHARED");
		command_drv2("iwpriv ra0 set EncrypType=WEP");
		command_drv2("iwpriv ra0 set DefaultKeyID=1");
		sprintf(cmd,"iwpriv ra0 set Key1=\"%s\"",passwd);
		command_drv2(cmd);
		sprintf(cmd,"iwpriv ra0 set SSID=\"%s\"",ssid);
		command_drv2(cmd);
	}else
	{
		DPRINTK("Not support wifi mode\n");
		ret_no = IPCAM_WIFI_NOT_SUPPORT_MODE;
		goto err;
	}	

	return 1;
err:
	return ret_no;
}

int test_wifi(char * ssid,char * passwd)
{
	char dev_name[20] = "";
	int ret = 0;
	int i = 0;
	
	if( ssid == NULL )
		return -1;
	
	get_net_dev_name(dev_name);
	if(strcmp(dev_name,"ra0") == 0 )
		return IPCAM_WIFI_NOT_ALLOW_TEST;

	set_wifi_disconnect();

	ret = connect_wifi(ssid,passwd);	
	if( ret > 0 )
	{
		for(i =0; i < 6; i++)
		{
			sleep(1);
			ret = is_wifi_connect();
			if( ret == 1 )
			{
				goto ok;
			}
		}
		
		ret = IPCAM_WIFI_CONNECT_FAILED;
	}

	set_wifi_disconnect();
	return ret;
ok:
	set_wifi_disconnect();
	return 1;
}

static void net_wifi_control_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	char dev_name[20] = "";
	char wifi_cur_ssid[50] = "";
	char wifi_cur_passwd[50] = "";
	int have_connect_wifi = 0;
	int ret;
	int connected_num = 0;
	int disconnect_num = 0;
	int i;
	int check_time_count = 2;
	
	DPRINTK("start wifi control thread\n");
	//get_net_dev_name(dev_name);

	cf_read_value("/mnt/mtd/RT2870STA.dat","SSID",wifi_cur_ssid);
	cf_read_value("/mnt/mtd/RT2870STA.dat","WPAPSK",wifi_cur_passwd);	
	
	while(1)
	{		
		check_time_count--;
		sleep(5);

		if( check_time_count <= 0 )
		{
			DPRINTK("No wifi dev, so out thread\n");
			return ;
		}
		
		if( is_have_wifi_dev() == 1 )			//这个函数不能长期调用，很影响系统效率。		
			break;		

	}

	command_drv2("iwlist ra0 scanning > /tmp/wifi_tmp");

	while(1)
	{
		sleep(3);		

		get_net_dev_name(dev_name);
		if(strcmp(dev_name,"ra0") == 0 )
		{//当处于WIFI连接时，不能测试WIFI.

			if( have_connect_wifi == 1 )
			{
				ret = is_wifi_connect();
				if( ret == 0 )				
				{
					disconnect_num++;					
					if( disconnect_num > 5 )
					{
						disconnect_num = 0;						
						set_wifi_disconnect();
						DPRINTK("Wifi disconnect, reconnect\n");
						connect_wifi(wifi_cur_ssid,wifi_cur_passwd);						
					}
					DPRINTK("is_wifi_connect = %d disconnect_num=%d\n",ret,disconnect_num);
				}

				
			}else
			{
			
				if( wifi_cur_ssid == NULL )
				{
					DPRINTK("WIFI no ssid\n");
					continue;
				}
								
				ret = connect_wifi(wifi_cur_ssid,wifi_cur_passwd);
				if( ret > 0 )
				{
					have_connect_wifi = 1;
				}else
				{
					DPRINTK("connect_wifi err=%d  %s %s\n",ret,wifi_cur_ssid,wifi_cur_passwd);
				}
			}
		}
		
	}	
	return ;
}


int  start_wifi_control_th()
{
	int ret;
	pthread_t id_net_wifi_catch;

	
	ret = pthread_create(&id_net_wifi_catch,NULL,(void*)net_wifi_control_thread,NULL);
	if ( 0 != ret )
	{
		DPRINTK( "create net_wifi_control_thread  thread error\n");
		return -1;
	}	

	return 1;
}


