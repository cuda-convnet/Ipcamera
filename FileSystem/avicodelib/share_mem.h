#ifndef _SHARE_MEM_H_
#define _SHARE_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>


/*
此共享内存是单向通行，若想双向通行请建两个通道。
*/

#define COMMEN_KEY 0x668866
#define COMMEN_MSG_KEY 0x768866

#define SHARE_MEM_FLAG  0x567
#define MEM_MAX_FRAME_BUF_SIZE (512*1024)

#define IS_SRV  (1)
#define IS_CLI  (2)

#define NET_DVR_NET_COMMAND (1001)
#define NET_DVR_NET_FRAME (1002)
#define NET_DVR_NET_SELF_DATA (1003)

#define MAX_TEXT 512

struct my_msg_st
{
   long my_msg_type;   //消息类型
   char msg_text[MAX_TEXT];
};

/*
typedef enum 
{
	DATA_NO = 0,
	DATA_OP,
	DATA_WRITE,
	DATA_READ
}MEM_EVENT;
*/

typedef enum MEM_EVENT_
{
    DATA_NO  = 0,              
    DATA_OP  = 1,                
    DATA_WRITE  = 2,            
    DATA_READ
} MEM_EVENT;



typedef struct _MEM_START_ST_
{
	int mem_flag; // SHARE_MEM_FLAG
	int mem_size; // sizeof(MEM_START_ST) + sizeof(MEM_SRV_ST) + sizeof(MEM_CLIENT_ST) + sizeof(buf)
	int total_chan_num; //
}MEM_START_ST;

typedef struct _MEM_SRV_ST_
{
	int srv_alive_count_num;
}MEM_SRV_ST;

typedef struct _MEM_CILENT_ST_
{
	int cilent_alive_count_num;
}MEM_CLIENT_ST;



typedef struct _MEM_OP_ST_
{
	int chan;
	int type; // IS_SRV. server   IS_CLI..client
	int data_size;
	MEM_EVENT mem_event;
}MEM_OP_ST;

#ifndef MN_MAX_BIND_ENC_NUM
#define MN_MAX_BIND_ENC_NUM (3)
#endif

typedef struct _MEM_MODULE_ST_ {
int listen_socket;
int client;
int msg_fd1;
int msg_fd2;
int work_mode;
int (*net_get_data)(void * pst,char * buf,int length,int cmd);
int send_th_flag;
int recv_th_flag;
int is_alread_work;
int build_once;
int bind_enc_num[MN_MAX_BIND_ENC_NUM];
int send_enc_data;
int video_mode;
int video_image_size;
int frame_num[MN_MAX_BIND_ENC_NUM];
int lost_frame[MN_MAX_BIND_ENC_NUM];
int dvr_cam_no;
int send_cam;
int net_send_count;
int live_cam;
int rec_cam;
int net_cam;
int recv_data_num;
char mac[30];
pthread_t netsend_id;
pthread_t netrecv_id;
pthread_t netcheck_id;
int stop_work_by_user;
int enc_info[8];
int cam_chan_recv_num[6];
int have_d1_enginer;
int ipcam_type;
int thread_stop_num;
int decoding;
int connect_mode; // NET_MODE_NORMAL |  NET_MODE_LOCAL
int send_data_mode; // NET_SEND_BUF_MODE | NET_SEND_DIRECT_MODE
int chan_num;
int connect_port;   // 12200  or  16000
int get_first_key_frame; // 0 or 1
int old_dec_chan;
int is_svr;
int buf_max_size;
char * buf_ptr;
char * totol_info_ptr;
char * srv_info_ptr;
char * client_info_ptr;
char * head_ptr;
char * data_ptr;
pthread_mutex_t lock;
int rec_chan_num;
long long  start_frame_time_sec[MN_MAX_BIND_ENC_NUM];
long long  last_frame_time_sec[MN_MAX_BIND_ENC_NUM];
long long  real_time_sec[MN_MAX_BIND_ENC_NUM];
}MEM_MODULE_ST;


MEM_MODULE_ST * MEM_init_share_mem(int chan,int buf_size,int is_svr,int (*func)(void *,char *,int,int));
int MEM_destroy_share_mem(void * value);
int MEM_destroy_work_thread(MEM_MODULE_ST * pst);
int MEM_create_work_thread(MEM_MODULE_ST * pst);
int MEM_read_data_lock(MEM_MODULE_ST * pst,char * buf, int *size,int delay_time);
int MEM_write_data_lock(MEM_MODULE_ST * pst,char * buf, int size,int delay_time);


#ifdef __cplusplus
}
#endif



#endif //_SHARE_MEM_H_
