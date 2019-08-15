// BufManage.h: interface for the BufManage class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _BUF_MANAGE_H_
#define _BUF_MANAGE_H_


#include "BaseData.h"


#define MAX_BUF_NUM 1000
#define BUF_MAX_SIZE (128*1024+sizeof(FRAMEHEADERINFO))
#define CIF_BUF_SIZE  (40*1024+sizeof(FRAMEHEADERINFO))
#define NET_BUF_MAX_SIZE (128*4*1024+sizeof(FRAMEHEADERINFO))
#define AUDIO_ENC_DATA_MAX_LENGTH (1024*5)

typedef struct _BUF_LIST_
{
	pthread_mutex_t control_mutex;
	int top;
	int bottom;
	int item_count;
	int inter_critical;
	int max_length;
	int max_count;
	char * buf_addr;
	char * subbuf_addr[MAX_BUF_NUM];	
	GST_DRV_BUF_INFO bufinfo;
}BUFLIST;



int clear_buf(BUFLIST * list);
int enable_get_data(BUFLIST * list);
int enable_insert_data(BUFLIST * list);
int get_one_buf(BUF_FRAMEHEADER * pheadr,char * data,BUFLIST * list);
int insert_data(BUF_FRAMEHEADER *pheadr,char * pdata,BUFLIST * list);
int destroy_buf_list(BUFLIST * list);
//int init_buf_list(BUFLIST * list);
int init_buf_list(BUFLIST * list,int buf_max_size,int  buf_max_count);
GST_DRV_BUF_INFO * get_buf_info(BUFLIST * list,char * pdata);
int list_to_list(BUFLIST * dest_list,BUFLIST * source_list);
int get_enable_insert_data_num(BUFLIST * list);

int set_bit(int iCam, int iBitSite);
int get_bit(int iCam, int iBitSite);

#endif // !defined(_BUF_MANAGE_H_)
