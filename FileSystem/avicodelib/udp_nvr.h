#ifndef _UDP_NVR_HEADER_
#define _UDP_NVR_HEADER_

#ifdef __cplusplus
extern "C" {
#endif

#include "time.h"

#ifdef WIN32
#include "winsock2.h"
#endif

#define MAX_NUM 2
#define MAX_MSG_LEN 600
#define MAX_TIME_NOT_ALIVE_NUM 17280 // 120 * 5 ten minute
#define CHECK_TIME_PEER 5000
#define SEND_ALIVE_TIME 30  //second
#define LISTEN_TIMEOUT 15
#define UNKNOW_PC_KICK_TIME 5
#define MAX_LISTEN_PORT (5)

#define UDP_CONNECT_ALIVE_TIME (10)

#define VERIFY_FLAG (0x47345656)

#define NVR_SEARCH_MSG "Search ipcam device"
#define IPCAM_ANSWER_MSG "This is ipcam.wait connect"


#define SVR_LISTEN_UDP_PORT		21688
#define UDP_DATA_LENGTH     (512)

#define NET_BASE	100
#define NET_LOGIN	NET_BASE+1
#define NET_ALIVE	NET_BASE+2
#define NET_CLIENT_CONNECT_REQUSET	NET_BASE+3
#define NET_CONNECT_SERVER_NEW_PORT	NET_BASE+4
#define NET_CONNECTED_SERVER_NEW_PORT	NET_BASE+5
#define NET_SEND_DATA_NVR_OR_CLIENT	NET_BASE+6
#define NET_NVR_CONNECT_SERVER_NEW_PORT	NET_BASE+7
#define NET_NVR_CONNECTED_SERVER_NEW_PORT	NET_BASE+8
#define NET_NVR_SEND_DATA_TO_CLIENT 	NET_BASE+9
#define NET_CLIENT_SEND_DATA_TO_NVR 	NET_BASE+10
#define NET_NVR_START_UDP_LISTEN 	NET_BASE+11
#define NET_CLIENT_START_UDP_CONNECT 	NET_BASE+12

#define FAB_SERVER_IP "219.132.130.181"
//#define FAB_SERVER_IP "192.168.1.57"
//#define FAB_SERVER_IP "192.168.0.57"

typedef struct _FAB_UDP_SOCKET_
{
#ifdef WIN32
	SOCKET socket_fd;
	HANDLE m_NetUdpRecvHandle;
	HANDLE m_hExit;
#else
	int socket_fd;
	pthread_t netrecv_id;
	int m_hExit;
#endif
	int port;
	int open_socket;
	void * self_data;
	int (*recv_call_back)(char * ,int,void *,void *);
	int (*send_func)(char * ,int,void *);
}FAB_UDP_SOCKET;

typedef struct _FAB_UDP_CMD_ST_
{
	int verify_flag;
	int cmd;
	char source_ip[20];
	char dst_ip[20];
	int source_port;
	int dst_port;
	char nvr_mac_id[10];
	int array_id;
	int server_port;	
	char extern_data[200];
	char lan_ip[30];
	int lan_port;	
}FAB_UDP_CMD_ST;


int fab_udp_start_recv_th(FAB_UDP_SOCKET * pSocket);
int fab_udp_stop_recv_th(FAB_UDP_SOCKET * pSocket);
int fab_SendUdp2(char * ip,int port,char * buf,int length,FAB_UDP_SOCKET * pSocket);

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

#ifdef __cplusplus
}
#endif

#endif