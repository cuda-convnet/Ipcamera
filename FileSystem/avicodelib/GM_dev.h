#ifndef _GM_DEV_H_
#define _GM_DEV_H_


#include "gmavi_api.h"
#include "favc_motiondet.h"
#include <pthread.h>

#include "BaseData.h"
#include "bufmanage.h"

typedef struct _GM_ENCODE_ST_
{
	int width;
	int height;
	int video_input_system;
	int bitrate;
	int framerate;
	int gop;
	int enc_fd;
	int enc_buf_size;
	int channel_num;
	int  is_sub_enc;
	int have_sub1_enc;
	int main_enc_fd;
	int sub_buf_offset;
	unsigned char *bs_buf;
	void * sub1_enc;
	int use_vbr;
	int max_bitrate;
	int is_open_motion;
	int is_alarm;
	struct favc_md gfavcmd; 
	struct md_cfg md_param;
	struct md_res active;
	int mb_width;
	int mb_height;
	char motion_area[8100];  // 1920*1080    (1920/16)* (1080/16) = 8100
	pthread_mutex_t motion_lock;
}GM_ENCODE_ST;



int GM_set_encode();
int GM_CVBS_output(int video_in_mode );

#endif

