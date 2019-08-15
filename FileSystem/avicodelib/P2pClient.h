#ifndef _NET_P2P_CLIENT_H_
#define _NET_P2P_CLIENT_H_

#include "P2pBaseDate.h"

#define MAX_URL_DNS_NUM (5)

#define NET_CAM_PORT (12200)
#define BORADCAST_PORT (12201)
#define IP_TEST_PORT (12202)
#define BORADCAST_TO_PC_PORT (13203)
#define DVR_AUTHOR_INFO (9)

#define CONNECT_POS (200)
#define AUTHORIZATION_POS (201)
#define CAM_VIDEO_TYPE (202)
#define NET_CAM_VERSION (204)
#define CAM_SD_HAVE_PEOPLE (203)
#define NET_CAM_ID_INFO (20)
#define NET_CAM_NET_CONFIG (24)
#define NET_REMOTE_SET_IP (205)
#define NET_USE_ID_CONNECT (80)
#define NET_CAM_MAC_POS (190)
#define NET_CAM_HAVE_MAC (189)
#define NET_CAM_HARDWARE_TYPE (188)

#define NET_CAM_HISI_CHIP (0)
#define NET_CAM_GOKE_CHIP (88)
#define NET_CAM_HISI_3516D_CHIP (89)
#define NET_CAM_HISI_3516E_CHIP (90)

typedef struct 
{
	unsigned char IPAlloction;
	unsigned char IpAddress[4];
	unsigned char GateWay[4];
	unsigned char SubNet[4];
	unsigned char DDNS1[4];
	unsigned char DDNS2[4];
	unsigned int Port;
	unsigned int IEPort;
} ValNetStatic2;



int init_p2p_socket();
int p2p_login(char * svr_ip,int svr_port);
int close_p2p_socket();
int p2p_set_send_cmd(char * buf,NETREQUEST * client_request);
int p2p_create_svr_to_dvr_hole(char * svr_ip,int svr_port);
int p2p_create_dvr_to_pc_hole(char * svr_ip,int svr_port);
int  p2p_client_to_svr_thread(void * value);
int recv_command(int client_socket);
NETREQUEST get_client_request(char *cmdBuf);
void close_dvr_svr_socket();
void close_svr_dvr_socket();
void login_to_svr();
int p2p_get_mac_id(char * svr_ip,int svr_port);
void get_host_name(char * pServerIp,char * buf);
int write_dvr_id_log_info(char * dvr_id);
int write_svr_log_info(char * server_ip);
int get_new_mac_id();
void get_svr_mac_id(char * server_ip,char * mac_id );
void get_host_name_by_ping(char * pServerIp,char * buf);
int    is_conneted_svr();
void get_svr_mac_id(char * server_ip,char * mac_id );
void net_ddns_open_close_flag(int flag);

int  start_p2p_svr_to_dvr_thread();

void login_to_svr();
int is_connect_to_server();
int p2p_listen_to_pc(int port);
int p2p_udp_login(char * svr_ip,int svr_port);


#endif // !defined(_NET_P2P_BASE_DATE_H_)

