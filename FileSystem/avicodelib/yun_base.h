#ifndef _YUN_BASE_HEADER
#define _YUN_BASE_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include "rudp_type.h"
#include "local_socket.h"


#ifdef WIN32
#define close closesocket
#endif

#define YUN_VERSION  (1001)

#define YUN_SERVER_PORT (26685)

#define MSG_FLAG  (0xadc1bcdf)
#define MSG_YUN_HEAD_FLAG  (0xacc1b1df)
#define MAX_MAC_ID_NUM  (20)

#define UDP_REQUEST_BASE  (10000)
#define UDP_REQUEST_CONNECT_NVR  					UDP_REQUEST_BASE+1
#define UDP_REQUEST_LOGIN_SVR  					UDP_REQUEST_BASE+2
#define UDP_REQUEST_CONNECT_DEST 					UDP_REQUEST_BASE+3
#define UDP_REQUEST_CONNECT_BIND					UDP_REQUEST_BASE+4
#define TCP_REQUEST_CONNECT_DEST 					UDP_REQUEST_BASE+5
#define TCP_REQUEST_FAILED							UDP_REQUEST_BASE+6
#define TCP_REQUEST_SUCCESS						UDP_REQUEST_BASE+7
#define TCP_REQUEST_CONNECT_SVR					UDP_REQUEST_BASE+8
#define TCP_REQUEST_WAIT_NVR_CONNECT				UDP_REQUEST_BASE+9
#define TCP_REQUEST_CONNECT_PC  					UDP_REQUEST_BASE+10
#define UDP_REQUEST_CONNECT_SVR  					UDP_REQUEST_BASE+11
#define TCP_REQUEST_CONNECT_YUN_TRANSMIT_SVR	UDP_REQUEST_BASE+12
#define TCP_REQUEST_CONNECT_NO_YUN_SVR			UDP_REQUEST_BASE+13


#define UDP_REQUEST_RESPONSE (20000)
#define UDP_REQUEST_RESPONSE_OK 					UDP_REQUEST_RESPONSE+1
#define UDP_REQUEST_RESPONSE_NVR_NOT_ONLINE 	UDP_REQUEST_RESPONSE+2
#define UDP_REQUEST_RESPONSE_TIME_OUT 			UDP_REQUEST_RESPONSE+3
#define UDP_REQUEST_RESPONSE_YUN_VERSION_ERR	UDP_REQUEST_RESPONSE+4


#define YS_RETURN_BASE  (-100000)
#define YS_RETURN_NVR_NOT_ONLINE    (YS_RETURN_BASE+1)
#define YS_RETURN_YUN_NO_SERVER     (YS_RETURN_BASE+2)
#define YS_RETURN_YUN_VERSION_ERR  (YS_RETURN_BASE+3)



#define UDP_TYPE_NVR   (2)
#define UDP_TYPE_PC     (3)
#define UDP_TYPE_SVR   (6)

#define MAX_CLIENT_NUM (1000)
#define CLIENT_START_PORT (23000)

#define YS_CONNECT_MODE_AUTO (1)
#define YS_CONNECT_MODE_PC_TCP_TO_NVR (2)
#define YS_CONNECT_MODE_NVR_TCP_TO_PC (3)
#define YS_CONNECT_MODE_PC_UDP_TO_NVR (4)
#define YS_CONNECT_MODE_PC_TCP_SVR_TO_NVR (5)

#define YS_MAX_TCP_SOCKET_NUM (2)

#define TCP_CONVERT_UDP_FLAG (0xabbc1123)



typedef struct _TCP_CONVERT_UDP_HEAD_ST_
{
	int flag;
	int socket_id;
	int packet_len;
}TCP_CONVERT_UDP_HEAD_ST;

typedef struct _YUN_SERVER_HEAD_ST_
{
	int flag;
	int socket_id;
	char mac_id[MAX_MAC_ID_NUM];
}YUN_SERVER_HEAD_ST;


typedef struct _UDP_MSG_
{
	u_int32_t msg_flag;  // modify packet flag  MSG_FLAG	
	u_int32_t client_port;  // NVR  client connect port. maybe upnp port.
	u_int32_t yun_port; // yun server port. maybe upnp port.
	u_int32_t yun_session_num; // yun server already have session num.		
	u_int32_t request_cmd; // NVR or PC client request.
	u_int32_t request_response; // request  response.
	struct sockaddr_in nvr_address; // NVR addr
	char mac_id[MAX_MAC_ID_NUM];    // Unique NVR  ID  for example  34K0032
	int type;  // NVR send msg or Pc send msg.  UDP_TYPE_NVR or UDP_TYPE_PC
	int connect_mode;  // Which connect mode for client connect to NVR.  YS_CONNECT_MODE_AUTO,YS_CONNECT_MODE_PC_TCP_TO_NVR
	int yun_version;
}UDP_MSG;


typedef struct _UDP_RESPONSE_ST_
{
	char * wait_response_buf;
	int wait_response_flag;
}UDP_RESPONSE_ST;


typedef struct _THREAD_PASS_ST_
{
	int pass_data_over;
	void * pass_data;
}THREAD_PASS_ST;

typedef struct YS_TCP_CONNECT_ST_
{
	int listen_port;
	int is_work;
	int need_tcp_connect_socket_num;
	int tcp_already_connect_num;
	int tcp_socket[YS_MAX_TCP_SOCKET_NUM];
	int udp_tcp_convert_flag;
	char mac_id[MAX_MAC_ID_NUM];
	LOCAL_SOCKET * udp_listen_st;
	LS_UDP_ST  * udp_st;	
}YS_TCP_CONNECT_ST;

typedef struct _CONNECT_MISSION_ST_
{
	LS_UDP_ST * nvr_udp_addr;
	LS_UDP_ST * client_udp_addr;
	LOCAL_SOCKET * pLsock;
	YS_TCP_CONNECT_ST * tcp_st_addr;
	UDP_MSG send_msg;
	char mac_id[MAX_MAC_ID_NUM];
	struct sockaddr_in nvr_address;
}CONNECT_MISSION_ST;

#define  YS_MAX_UDP_CLIENT_NUM (10000)

typedef struct YS_UDP_LIST_INFO_
{
	void * udp_addr;
	UDP_MSG udp_msg;
	unsigned long resently_active_time;	
	int upnp_client_port_enable_connect;
	int upnp_server_port_enable_connect;
	int upnp_server_used_cout;
	int have_check_upnp_port;
}YS_UDP_LIST_INFO;






void win_gcc_thread_start();
void win_gcc_thread_end();
int ys_start_wait_tcp_connect_thread(YS_TCP_CONNECT_ST * tcp_st);
int pc_connect_nvr_use_svr_bridge(char * svr_ip,int svr_port,int local_port,char * nvr_id,YS_TCP_CONNECT_ST * tcp_st,int connect_mode);
int ys_check_port_availble(int local_port);


#ifdef __cplusplus
}
#endif

#endif
