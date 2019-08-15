#ifndef _SINGLY_LINKED_LIST_HEADER
#define _SINGLY_LINKED_LIST_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include  <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>




#define VIDEO_DATA_HEAD_OFFSET     (96)


#define SLL_ERR_BASE  (-100)
#define SLL_ITEM_ENOUGH_ERR (SLL_ERR_BASE-1)
#define SLL_ITEM_EMPTY_ERR (SLL_ERR_BASE-2)
#define SLL_ITEM_BUF_NOT_ENOUGH_ERR (SLL_ERR_BASE-3)

typedef struct _SINGLY_LINKED_EXTERN_DATA_
{
	long long last_video_time;      // 视频数据最新帧的时间戳
	int video_code;
}SINGLY_LINKED_EXTERN_DATA;


typedef struct _SINGLY_LINKED_LIST_ST_
{
	long long id;
	char * data;
	int len;
	int user_num;
	struct _SINGLY_LINKED_LIST_ST_ * next;
}SINGLY_LINKED_LIST_ST;

typedef struct _SINGLY_LINKED_LIST_INFO_ST_
{
	unsigned int max_item_num;
	unsigned int item_num;
	long long head_id;
	long long tail_id;
	long long seq_no;
	unsigned int max_item_size;
	unsigned int all_data_size;
	unsigned int self_date_fps;
	pthread_mutex_t lock;
	SINGLY_LINKED_LIST_ST * list;
	SINGLY_LINKED_LIST_ST * pListHead;
	float chn_fps;  
	unsigned int all_not_free_data_size;
	SINGLY_LINKED_EXTERN_DATA extern_data;
}SINGLY_LINKED_LIST_INFO_ST;


typedef struct _SINGLY_LINKED_GET_DATA_ST_
{
	long long id;
	char * data;
	int len;	
	SINGLY_LINKED_LIST_ST * list;
}SINGLY_LINKED_GET_DATA_ST;


int sll_init_list(SINGLY_LINKED_LIST_INFO_ST  * list_info,int max_item_num,int data_max_size);
int sll_destroy_list(SINGLY_LINKED_LIST_INFO_ST  * list_info);
int  sll_add_list_item(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * data,int len,int offset);
int  sll_add_list_item_no_malloc(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * data,int len);
int sll_del_list_tail_item(SINGLY_LINKED_LIST_INFO_ST  * list_info);
int sll_get_list_item(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * dest_buf,int dest_max_len,long long item_id);
int sll_list_item_num(SINGLY_LINKED_LIST_INFO_ST  * list_info);
int sll_list_enable_add_item(SINGLY_LINKED_LIST_INFO_ST  * list_info);
int sll_get_list_item_lock(SINGLY_LINKED_LIST_INFO_ST  * list_info,SINGLY_LINKED_GET_DATA_ST * get_data,long long item_id);
int sll_get_list_item_unlock(SINGLY_LINKED_LIST_INFO_ST  * list_info,SINGLY_LINKED_GET_DATA_ST * get_data);
int sll_get_last_key_frame_list_item_no(SINGLY_LINKED_LIST_INFO_ST  * list_info);

int set_h264_array_point(void * ptrH264);
void clear_h264_array_point(int id);

#ifdef __cplusplus
}
#endif

#endif
