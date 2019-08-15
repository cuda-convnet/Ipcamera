#ifndef _FIC8120_CTRL_HEADER
#define _FIC8120_CTRL_HEADER

#include "devfile.h"
#include "ratecontrol.h"
#include "filesystem.h"
#include "ratecontrol264.h"
#include "BaseData.h"
#include "func_common.h"



#define MAX_LCD_FP 10
#define MAX_CHANNEL 4




#define FLCD_GET_DATA_SEP   0x46db
//#define FLCD_GET_DATA       0x46dc
#define FLCD_SET_FB_NUM     0x46dd
#define FLCD_SWITCH_MODE    0x46de
#define FLCD_SET_SPECIAL_FB    0x46df
#define FLCD_MODE_RGB       0
#define FLCD_MODE_YCBCR     1



#define MAXBUF  (513*1024)



#define FMPEG4_DECODER_DEV  "/dev/fdec"
#define FMPEG4_ENCODER_DEV  "/dev/fenc"
#define FAVC_ENCODER_DEV  "/dev/f264enc"
#define FAVC_DECODER_DEV  "/dev/f264dec"

#define LCD_DEV 				  "/dev/fb0"


#define CUT_GREEN_LINE


#define QUAD_MODE_NET 10

struct flcd_data
{
    unsigned int buf_len;
    unsigned int uv_offset;
    unsigned int frame_no;
    unsigned int mp4_map_dma[10];
};

typedef struct AV_PICTURE
{
	unsigned char *data[4];
	unsigned char *data_phy[4];
	int			  linesizes[4];
	unsigned long show_sec;
	unsigned long show_usec;
}AVPICTURE;

typedef struct LCD_SHOW_ARRAY
{
  	int total_fp;
	int top_fp;
	int bottom_fp;
	int fp_store_count;
	AVPICTURE avp[MAX_LCD_FP];	
	int pic_width;
	int pic_height;
	int change_fb;
	int buf_length;
}LCDSHOWARRAY;

typedef struct _DECODE_ALL_ITEM
{
	int enable_play;
	int frame_num;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}DECODEITEM;





typedef struct _DECODE_ITEM
{
	int decode_fd;
	int ichannel;    //通道号
	int mode;        // 制式 N /P
	int size;		  // cif  d1 hd1
	int width;
	int height;	
}DECODE_ITEM;

typedef struct _FRAME_RATE_CONTROL_
{
	long long frame_interval_time;
	int frame_rate;
	struct timeval last_frame_time;
	int gop_size;
	int bitstream;
}FRAME_RATE_CTRL;



int init_all_custom_buf();
void  release_custom_buf();
int init_cap_device(int standard);
int release_cap_device();
int enable_audio_rec(int flag);
int  encode_video(video_profile *video_setting);
int  instant_keyframe(int cam);
int  start_stop_record(int iFlag);
void drv_ctrl_lib_function(GST_DEV_FILE_PARAM  * gst);
GST_DRV_BUF_INFO * get_save_buf_info(char * pData);
int show_cif_to_d1_size(int flag);

int xvid_decoder_init(DECODE_ITEM * dec_item);
int xvid_decoder_end(DECODE_ITEM * dec_item);
int xvid_decode(void *data, int *got_picture, uint8_t *buf, int buf_size,DECODE_ITEM * dec_item);

int init_mpeg4_dev(ENCODE_ITEM * enc_item,video_profile *setting);
int close_mpeg4_dev(ENCODE_ITEM * enc_item);
int mpeg4_encode(ENCODE_ITEM * enc_item, unsigned char *frame, void * data);

void close_all_device();
int init_all_device(video_profile * seting,int standard,int size_mode);
int change_device_set(video_profile * seting,int standard,int size_mode);
int start_stop_net_view(int flag,int play_cam);
int start_stop_net_file_view(int flag);
void clear_net_buf();

int get_video_channel(unsigned char * y);

int show_cif_img_to_middle(int flag);

KEYBOARD_LED_INFO * get_keyboard_led_info();
int op_keyboard_led_info(char * cmd);
int set_rec_type(int type);
int set_delay_sec_time(int sec);
int get_delay_sec_time();
int test_decode_fuc( uint8_t *buf, int buf_size);
int open_net_audio(int flag);
int start_mini_httpd(int port);

void set_screen_show_mode(int mode );
void set_mult_play_cam(int cam);


int scaler_d1_to_d1();
int scaler_cif_to_d1();
int clear_fb();

int get_net_trans_mode();
int  instant_keyframe_net(int cam);
int mac_decode(void *data, int *got_picture, uint8_t *buf, int buf_size,int chan);
int set_net_encode(video_profile * seting,int standard,int size_mode);
int release_net_encode();
int set_net_frame_rate(video_profile * seting,int standard,int size_mode);
int clear_lcd_osd(int color);
int get_cur_standard();
int get_cur_size();
int command_drv(char * command);
int set_net_frame_rate_hisi(video_profile * seting,int standard,int size_mode);


GST_DRV_BUF_INFO * get_net_buf_info(char * pData);



GST_DEV_FILE_PARAM  * g_pstcommand;

#endif

