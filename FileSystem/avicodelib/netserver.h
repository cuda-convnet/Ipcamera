#ifndef _FIC8120_NET_HEADER
#define _FIC8120_NET_HEADER

#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include  <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/socket.h>


#include "devfile.h"
#include "fic8120ctrl.h"
#include "filesystem.h"
#include "bufmanage.h"

#define USER  -100


#define COMMAND_REV_ERROR (USER-1)
#define SOCKET_CONNECT_ERROR (USER-2)
#define SOCKET_SEND_ERROR (USER-3)
#define NET_FILE_PLAY_OVER (USER-4)
#define SOCKET_BUILD_ERROR (USER-5)
#define SOCKET_ERROR (USER-6)
#define SOCKET_READ_ERROR (USER-7)

#define MAXDATASIZE 512
#define SERVPORT 5150 
#define BACKLOG 3
#define STDIN 0

#define MAXCLIENT 17
#define MAX_CONNECT_NUM (MAXCLIENT+1)
#define MAXLISTITEMNUM       10

#define INTERNATE_MODE 0
#define LOCAL_NET_MODE 1
#define SUPER_USER 2
#define NORMAL_USER 3

#define RIGHT_USER_PASS 1
#define BAD_USER_PASS -2
#define TOO_MUCH_UERS -4


#define NETSTAT_ARRAY_NUM (10)

#define GUI_MSG_MAX_NUM MENU_GUI_MSG_MAX_NUM  

enum
{
	COMMAND_GET_FILELIST,
	COMMAND_OPEN_NET_PLAY_FILE,
	COMMAND_GET_FRAME,
	COMMAND_SET_TIMEPOS,
	COMMAND_STOP_NET_PLAY_FILE,
	PLAY_STOP,
	PLAY_PAUSE,
	NET_FILE_PLAYING,
	NET_FILE_OPEN_ERROR,
	NET_FILE_OPEN_SUCCESS,
	USER_TOO_MUCH,
	GET_LOCAL_VIEW,
	STOP_LOCAL_VIEW,
	GET_NET_VIEW,
	STOP_NET_CONNECT,
	SET_NET_PARAMETER,
	NET_KEYBOARD,
	OTHER_INFO,
	SEND_USER_NAME_PASS,
	CREATE_NET_KEY_FRAME,
	GET_SYSTEM_FILE_FROM_PC,
	EXECUTE_SHELL_FILE,
	TIME_FILE_PLAY,
	COMMAND_SEND_MAC_PARA,
	COMMAND_SET_MAC_PARA,
	COMMAND_SET_MAC_KEY,
	COMMAND_SEND_AUDIO_DATA,
	COMMAND_SEND_GUI_MSG,
	COMMAND_SET_GUI_MSG,
	NET_CAM_BIND_ENC,
	NET_CAM_SEND_DATA,
	NET_CAM_STOP_SEND,
	NET_CAM_ENC_SET,
	NET_CAM_FILE_OPEN,
	NET_CAM_FILE_WRITE,
	NET_CAM_FILE_CLOSE,
	NET_CAM_SEND_INTERNATIONAL_DATA,
	NET_CAM_GET_MAC,
	COMMAND_SET_AUDIO_SOCKET,
	COMMAND_AUDIO_SET,
	NET_CAM_GET_FILE_OPEN,
	NET_CAM_GET_FILE_READ,
	NET_CAM_GET_FILE_CLOSE,
	NET_CAM_WRITE_DVR_SVR,
	NET_CAM_NET_MODE,
	NET_CAM_WIFI_ENABLE,
	NET_CAM_GET_CAM_TYPE,
	NET_CAM_SUPPORT_512k_BUFF,
	NET_CAM_SET_WIFI_PARATERMERS,
	NET_CAM_SET_IPCAM_SUB_STREAM_RESOLUTION,
	NET_CAM_IS_SUPPORT_YUN_SERVER,
	NET_CAM_SET_WIFI_SSID_PASS,   
	NET_CAM_TEST_WIFI_SSID_PASS,
	NET_CAM_GET_ALL_STREAM_RESOLUTION,
	NET_CAM_SET_ALL_STREAM_RESOLUTION,
	NET_CAM_GET_IPCAM_INFO,
	NET_CAM_GET_IMAGE_PARAMETERS,
	NET_CAM_SET_IMAGE_PARAMETERS,
	NET_CAM_SET_DEFAULT,
	NET_CAM_GET_SYS_PARAMETERS,
	NET_CAM_SET_SYS_PARAMETERS,
	NET_CAM_GET_USER_INFO,
	NET_CAM_SET_USER_INFO,
	NET_CAM_USER_LOGIN,
	NET_CAM_GET_ADC_MODE,
	NET_CAM_SET_ADC_MODE,
	NET_CAM_GET_AUDIO_MODE,
	NET_CAM_SET_AUDIO_MODE,
	NET_CAM_GET_ENCODE_TYPE,
	NET_CAM_SET_ENCODE_TYPE
};

enum
{
	NORMAL_LIGHT_BOARD = 0,
	YATAI_LIGHT_BOARD
};

enum
{
	AUDIO_NORMAL_INPUT = 0,
	AUDIO_SPEAK_INPUT
};

enum
{
	CONFIG_ENC_TYPE_H264 = 0,
	CONFIG_ENC_TYPE_H265
};

enum
{
	NONE,
	MACHINE_INFO,
	MAC_DISK_INFO,
	LED_INFO,
	DISK_FILE_LIST,
	PLAY_FILE_RESULT,
	SEND_MAC_PARA,
	PRESS_REC_KEY,
	PRESS_ESC_KEY,
	SEND_GUI_MSG,
	SEND_MAX_VIDEO_SIZE
};



typedef struct DVR_TIME
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
}DVRTIME;


typedef struct _NET_COMMAND_
{
	int command;
	int length;
	int page;
	int play_fileId;
	time_t searchDate;	
}NET_COMMAND;

typedef struct _NET_SEND_INFO_
{
	int disk_size; // (G)
	int disk_remain; //剩于容量百分比
	int disk_cover; //覆盖率
	int disk_bad; //坏区(G)
	time_t start_rec_time; //开始录象时间
	time_t end_rec_time; //结束录象时间
}NET_SEND_INFO;

typedef struct _NET_WIFI_SET_PARAMETER_
{
	char  ssid[20];
	char  passwd[20];
}NET_WIFI_SET_PARAMETER;

#define MAX_STREAM_RESOLUTION_NUM (5)
#define MAX_STREAM_INFO_NUM (3)

typedef struct _STREAM_RESOLUTION
{
	int w;
	int h;
}STREAM_RESOLUTION;

typedef struct _H264_STREAM_SET_
{
	int bitstream;   //码流
	int frame_rate; //帧率
	int gop;           //帧间隔
	int base_frame_rate;  //当前基础帧率一般为25 或30
}H264_ENC;

typedef struct _H264_STREAM_INFO_
{
	int stream_select_resolution_index;
	int stream_resolution_num;
	STREAM_RESOLUTION stream_resolution[MAX_STREAM_RESOLUTION_NUM];
	H264_ENC steam_enc; 
}H264_STREAM_INFO;

typedef struct _IPCAM_STREAM_SET_
{
	int stream_num;
	H264_STREAM_INFO stream_info[MAX_STREAM_INFO_NUM];
}IPCAM_STREAM_SET;

typedef struct _NET_IPCAM_INFO_
{
	char soft_ver[20];
	char hard_ver[20];
	char ipam_id[40];
}NET_IPCAM_INFO;


typedef struct _NET_IPCAM_IMAGE_PARA_
{
	int u32LumaVal;                  /* Luminance: [0 ~ 100] */
	int u32ContrVal;                 /* Contrast: [0 ~ 100] */
	int u32HueVal;                   /* Hue: [0 ~ 100] */
	int u32SatuVal;                  /* Satuature: [0 ~ 100] */
	int reserve[12];                 
}NET_IPCAM_IMAGE_PARA;


#define RESTORET_DEFAULT_PARA (3000)
#define RESTORET_DEFAULT_PARA_NET (RESTORET_DEFAULT_PARA+1)
#define RESTORET_DEFAULT_PARA_IMAGE (RESTORET_DEFAULT_PARA+2)
#define RESTORET_DEFAULT_PARA_ENC (RESTORET_DEFAULT_PARA+3)
#define RESTORET_DEFAULT_PARA_ALL (RESTORET_DEFAULT_PARA+4)

typedef struct _NET_IPCAM_RESTORE_DEFAULT_
{
	int set_default_para;  // RESTORET_DEFAULT_PARA - RESTORET_DEFAULT_PARA_ALL
}NET_IPCAM_RESTORE_DEFAULT;

#define TIME_REGION_ASIA 0
#define TIME_REGION_USA  1
#define TIME_REGION_EURO 2

#define VIDEO_STARDARD_50HZ 0  
#define VIDEO_STARDARD_60HZ 1

typedef struct _NET_IPCAM_SYS_INFO_
{
	int day_date;   //日期
	int day_time;   //时间
	int time_region; //  TIME_REGION_ASIA  TIME_REGION_USA TIME_REGION_EURO
	int time_zone;   //时区 GMT+08 北京 time_zone=+8;
	int stardard; // VIDEO_STARDARD_50HZ VIDEO_STARDARD_60HZ
	int reservered[20]; //保留
}NET_IPCAM_SYS_INFO;

#define IPC_MAX_USER_NUM (5)
#define IPC_USER_WORD_NUM (20)

typedef struct _NET_USER_INFO_
{
	char name[IPC_MAX_USER_NUM][IPC_USER_WORD_NUM];
	char pass[IPC_MAX_USER_NUM][IPC_USER_WORD_NUM];
	int user_mode[IPC_MAX_USER_NUM];
}NET_USER_INFO;



typedef struct _NET_UPNP_ST_INFO_
{	
	unsigned short internal_port;
	unsigned short external_port;
	char internal_ip[40];
	char upnp_name[50];
	int upnp_flag;
	int upnp_over;
}NET_UPNP_ST_INFO;


typedef struct _NET_SOCKET
{
	int listenfd;
	int maxfd;	
	int clientfd[MAXCLIENT];
	int client_flag[MAXCLIENT];
	int client_send_data[MAXCLIENT];
	int send_flag[MAXCLIENT];
	int close_socket_flag[MAXCLIENT];
	int client_num;
	int server_port;
	NET_UPNP_ST_INFO server_upnp;
	NET_UPNP_ST_INFO yun_upnp;
	int http_port;
	fd_set rfd_set;
	fd_set wfd_set;
	fd_set efd_set;
	fd_set catch_fd_set;
	fd_set watch_set;
	int net_flag;
	int reset_net_server;
	int reset_net_udp_server;
	int socket_is_work;
	int cur_client_idx;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr[MAXCLIENT];
	int net_play_mode;
	int net_trans_mode;
	int bit_stream;
	int frame_rate;
	GST_FILESHOWINFO rec_list_info[SEACHER_LIST_MAX_NUM];
	GST_FILESHOWINFO play_rec_record_info;
	int play_cam;
	char set_ip[20];
	char set_net_gateway[20];
	char set_net_mask[20];
	char dns1[20];
	char dns2[20];
	char user_name[4][10];
	char user_password[4][10];
	int  info_cmd_to_send[MAXCLIENT];
	int  super_user_id;
	int  client_use_cam[MAXCLIENT];
	int  socket_send_num[MAXCLIENT];
	int  socket_recv_num[MAXCLIENT];
	int  close_all_socket;
	int  have_check_passwd[MAXCLIENT];
	char net_dev_name[40];
	int  is_super_user[MAXCLIENT];
	pthread_mutex_t send_mutex[MAXCLIENT];
	unsigned char net_dev_mac[10];
}NETSOCKET;

void create_net_server_thread( void );
int stop_net_server();
int set_net_parameter(char * ip, int port, char * net_gate,char * netmask,char * dns1,char *dns2);
int kick_all_client(NETSOCKET * server_ctrl);
int kick_client();
int get_client_info(char * client_ip);
int is_have_client();
int get_admin_info(char * name1,char * pass1,char * name2, char * pass2,char * name3,char * pass3,char * name4,char * pass4);
int kick_normal_client(NETSOCKET * server_ctrl);
int get_client_info_by_id(char * client_ip, int id);
int get_client_user_num();
int try_dhcp(char * ip,char * netmsk,char * gateway,char * dns1,char * dns2);
void get_server_port(int * server_port,int * http_port);

int socket_read_data( int idSock, char* pBuf, int ulLen );
int socket_send_data( int idSock, char* pBuf, int ulLen );
int clear_client(int idx, NETSOCKET * server_ctrl);
int get_net_file_view_flag();
int change_net_frame_rate(int bit_stream,int frame_rate);
int net_kick_client( NETSOCKET * server_ctrl);
int  change_net_parameter(NETSOCKET * server_ctrl);
int get_gateway(char *gateway);
void kill_dhcp();
int FindSTR( char  * byhtml, char *cfind, int nStartPos);
int GetTmpLink(char * byhtml, char * temp, int start, int end,int max_num);
int set_UPnP(NET_UPNP_ST_INFO * upnp);
int Lock_Set_UPnP(NET_UPNP_ST_INFO * upnp,int right_now);
int No_Use_Set_UPnP(char * ip,int port);
int Clear_UPnP(int port);
void upnp_thread();



#endif


