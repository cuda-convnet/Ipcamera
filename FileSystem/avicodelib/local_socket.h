#ifndef _LOCAL_SOCKET_HEADER
#define _LOCAL_SOCKET_HEADER


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include  <string.h>
//#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>


#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    //#include <gdk/gdkwin32.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <net/if.h>
    #include <net/if_arp.h>
    #include <sys/un.h>
#endif

#include "rudp.h"

#ifdef WIN32
#define close closesocket
#endif



#define LS_BASE_ERR (-100)
#define LS_MOMERY_ALLOW_ERR (LS_BASE_ERR-1)
#define LS_CREATE_MUTEX_ERR (LS_BASE_ERR-2)
#define LS_SOCKET_ALREADY_WORK_ERR (LS_BASE_ERR-3)
#define LS_SOCKET_INIT_ERR (LS_BASE_ERR-4)
#define LS_SOCKET_REUSEADDR_ERR (LS_BASE_ERR-5)
#define LS_SOCKET_LINGER_ERR (LS_BASE_ERR-6)
#define LS_SOCKET_BIND_ERR (LS_BASE_ERR-7)
#define LS_SOCKET_CREATE_LISTEN_THREAD_ERR (LS_BASE_ERR-8)
#define LS_SOCKET_LISTEN_THREAD_ERR (LS_BASE_ERR-9)
#define LS_SOCKET_READ_ERR (LS_BASE_ERR-10)
#define LS_SOCKET_WRITE_ERR (LS_BASE_ERR-11)
#define LS_SOCKET_NULL_ERR (LS_BASE_ERR-12)
#define LS_SOCKET_SET_ERR (LS_BASE_ERR-13)
#define LS_SOCKET_TIME_OUT_ERR (LS_BASE_ERR-14)
#define LS_SOCKET_NOT_INIT_ERR (LS_BASE_ERR-15)




#define LS_CONNECT_OBJECT_REMOTE  (1)
#define LS_CONNECT_OBJECT_LOCAL    (2)

#define LS_MAX_BUF_SIZE (100*1024)
#define LS_RECV_MAX_SIZE (1024*1024)

#define LS_DATA_SEND_MODE_NET  (0)
#define LS_DATA_SEND_MEM_NET  (1)

#define LS_ALIVE_PACKET_MAC  (0x32452abc)
#define LS_GET_MEM_DATA_OVER (LS_ALIVE_PACKET_MAC+1)


#define LS_COMMEN_KEY 0xba8866
#define LS_MAX_SEND_MEMORY_SIZE (100*1024)



#define LS_MEM_SEND_STATUS_SENDING  (1)
#define LS_MEM_SEND_STATUS_ARRIVE (2)

typedef struct _LS_NET_DATA_ST{
int split_length;
int data_length;
int is_mult_data;
int send_mode;  //  LS_DATA_SEND_MODE_NET is net mode , LS_DATA_SEND_MEM_NET is mem mode .
int mem_key;
}LS_NET_DATA_ST;


typedef struct _LS_UDP_ST_
{
	u_int32_t id_key;
	u_int32_t start_recv_packet_no;
	u_int32_t start_send_packet_no;
	int is_init;
	char * big_recv_buf;
	int big_recv_len;	
	int self_data_type;
	char * self_data;
	char * self_data_no_free;
	char * response;
	void * parent_socket;
	struct sockaddr_in address;	
	struct rudp_packet send_data[RUDP_WINDOW];	
	struct rudp_packet now_recv_data[RUDP_WINDOW];	
	pthread_mutex_t mem_send_mutex;
	pthread_cond_t mem_send_block_cond;
	pthread_mutex_t socket_lock;
	pthread_mutex_t need_response_lock;	
	int mem_send_status;
	int send_err_flag;
	int (*recv_data)(void * plsocket,void * udp_st,char * buf,int length);
	struct _LS_UDP_ST_ * next;
}LS_UDP_ST;


typedef struct _LOCAL_SOCKET_
{
	int init_flag;
	int socket;
	int socket_type;  // SOCK_STREAM or SOCK_DGRAM
	int connect_object; // LS_CONNECT_OBJECT_REMOTE or LS_CONNECT_OBJECT_LOCAL
	int is_listen_work;
	int socket_port;
	int listen_port;
	int connect_port;
	char connect_addr[60];
	struct sockaddr_in address;
	int socket_id;
	pthread_mutex_t socket_lock;
	pthread_t netrecv_id;
	pthread_t netcheck_id;
	pthread_t netlisten_id;
	pthread_mutex_t mem_send_mutex;
	pthread_cond_t mem_send_block_cond;
	pthread_mutex_t need_response_lock;
	pthread_mutex_t socket_recv_lock;
	char * self_data;
	char * self_data_no_release;
	char * response;
	int recv_th_flag;
	int check_th_flag;
	int send_err_flag;
	int all_thread_flag;
	int mem_fd;
	int mem_key;
	int mem_max_size;
	char * mem_buf_ptr;
	int remote_mem_fd;
	int remote_mem_key;
	int remote_mem_max_size;
	char * remote_mem_buf_ptr;	
	int mem_send_status; //LS_MEM_SEND_STATUS_SENDING  LS_MEM_SEND_STATUS_ARRIVE
	int (*recv_data)(void * plsocket,char * buf,int length);
	int (*recv_udp_data)(void * plsocket,void * remote,char * buf,int length);
	int (*accpet_socket)(void * plsocket,int accept_socket,char * connect_ip,int port);
	LS_UDP_ST * udp_list;	
}LOCAL_SOCKET;




int ls_set_init_key_id(int num);
int ls_init_socket(LOCAL_SOCKET * pLsocket,int socket_type,int connect_object,int mem_max_size);
int ls_destroy_socket(LOCAL_SOCKET * pLsocket);
int ls_send_data(LOCAL_SOCKET * pLsocket,char * buf,int length);
int ls_listen(LOCAL_SOCKET * pLsocket,int listen_port,int (*accpet_socket)(void *,int ,char *,int),int (*recv_udp_data)(void *,void *,char *,int));
int ls_connect(LOCAL_SOCKET * pLsocket,char * ip,int port,int (*recv_data)(void *,char *,int));
int ls_connect_by_accept(LOCAL_SOCKET * pLsocket,int socket_fd,char * connect_ip,int port,int (*recv_data)(void * ,char * ,int ));
int ls_check_socket_iswork(LOCAL_SOCKET * pLsocket);
int ls_stop_socket_work(LOCAL_SOCKET * pLsocket);
int ls_reset_socket(LOCAL_SOCKET * pLsocket);
int ls_ini_udp_st(LS_UDP_ST * udp_st,char * ip,int port,int (*recv_data)(void *,void *,char *,int));
int ls_destroy_udp_st(LS_UDP_ST * udp_st);
int ls_add_udp_task_to_socket(LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st);
int ls_del_udp_task_to_socket(LOCAL_SOCKET * pLsocket,LS_UDP_ST * udp_st);
int ls_del_udp_task(LS_UDP_ST * udp_st);
int ls_send_udp_data(LS_UDP_ST * udp_st,char * buf,int length);
void ls_usleep(int million_sec);

#ifdef __cplusplus
}
#endif

#endif
