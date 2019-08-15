#ifndef _NET_P2P_BASE_DATE_H_
#define _NET_P2P_BASE_DATE_H_

#define MAX_CLIENT (5)


#define MAX_NUM 10
#define MAX_MSG_LEN 600
#define MAX_TIME_NOT_ALIVE_NUM (6*3)
#define CHECK_TIME_PEER 5000
#define SEND_ALIVE_TIME  (60*5)  //second
#define LISTEN_TIMEOUT 15


#define ERR_NET   -1000
#define ERR_NET_INIT1  ERR_NET-1
#define ERR_NET_ALREADY_WORK	ERR_NET-2
#define ERR_NET_NOT_INIT		ERR_NET-3
#define ERR_NET_BUILD			ERR_NET-4
#define ERR_NET_LISTEN			ERR_NET-5
#define ERR_NET_SET_ACCEPT_SOCKET	ERR_NET-6
#define ERR_NET_DATE			ERR_NET-7
#define ERR_MAX_USER			ERR_NET-8
#define ERR_ALREADY_MOST_USER ERR_NET-9
#define ERR_DVR_IS_NOT_ALIVE  ERR_NET-10
#define ERR_CREATE_HOLE  ERR_NET-11
#define ERR_CONNECT_DVR_HOLE  ERR_NET-12

#define SVR_LISTEN_UDP_PORT		10688
#define UDP_DATA_LENGTH     (512)
#define SVR_LISTEN_PORT		10088
#define SVR_CONNECT_PORT	10116
#define SVR_TO_DVR_PORT	10126
#define DVR_TO_PC_PORT  10138
#define BASE_PORT		10200
#define MAX_P2P_CLIENT 2

#define DVR_USER 1001
#define PC_USER  1002

#define GET_DVR_ID "getname"


// net command define
#define NET_BASE	100
#define NET_LOGIN	NET_BASE+1
#define NET_ALIVE	NET_BASE+2
#define NET_CREATE_HOLE	NET_BASE+3
#define NET_CREATE_HOLE_SUCCESS	NET_BASE+4
#define NET_ALREADY_MOST_USER	NET_BASE+5
#define NET_DVR_STILL_ALIVE		NET_BASE+6
#define NET_LOGIN_OK			NET_BASE+7
#define NET_CONNECT_TO_SVR_HOLE	NET_BASE+8
#define NET_CONNECT_TO_DVR		NET_BASE+9
#define NET_DVR_NOT_ALIVE		NET_BASE+10
#define NET_CREATE_HOLE_FAILED	NET_BASE+11
#define NET_DVR_LISTEN_PC		NET_BASE+12
#define NET_P2P_CONNECT_FAILED  NET_BASE+13
#define NET_DVR_LISTEN_PC_ERROR		NET_BASE+14
#define NET_DVR_GET_MAC_ID		NET_BASE+15
#define NET_LOGIN_SHELL			NET_BASE+16
#define NET_LOGIN_NOT_AUTHORIZATION		NET_BASE+17


typedef struct NET_REQUEST
{
	int      cmd;
	char     dvrName[200];
	char     SuperPassWord[10];
	char     NormalPassWord[10];
	int		 port1;
	int		 port2;
}NETREQUEST;

typedef struct NET_CLIENT_INFO_
{	
	char     dvrName[200];
	char     SuperPassWord[10];
	char     NormalPassWord[10];
	int		 port1;
	int		 port2;
	int		 isAlive;
}NET_CLIENT_INFO;


#define AUDIO_TYPE_PCM   99
#define AUDIO_TYPE_G711_ULAW  (AUDIO_TYPE_PCM+1)
#define AUDIO_TYPE_G711_ALAW  (AUDIO_TYPE_PCM+1)

#define MAX_VBR_QUALITY_ITEM_NUM (5)
#define MAX_VBR_QUALITY_LEVER (5)

typedef struct _VBR_QUALITY_ST_
{
	int w;
	int h;
	int bit[MAX_VBR_QUALITY_LEVER];
	int quality[MAX_VBR_QUALITY_LEVER];
	int default_bitstream;
	int default_framerate;
	int default_gop;
	int default_H264_profile;
}VBR_QUALITY_ST;

#endif // !defined(_NET_P2P_BASE_DATE_H_)
