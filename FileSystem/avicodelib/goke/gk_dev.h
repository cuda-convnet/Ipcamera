
#ifndef _GOKE_DEV_H_
#define _GOKE_DEV_H_

#define GK_AUDIO_ON 1

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STREAM_NUM (3)


typedef enum
{
	GK_CODE_264 = 0,
	GK_CODE_265,
	GK_CODE_NUM,
}GK_CODE;

typedef enum
{
	GK_CODE_CTRL_CBR = 0,
	GK_CODE_CTRL_VBR,
	GK_CODE_CTRL_NUM,
}GK_CODE_CTRL;


typedef enum
{
	GK_STANDARD_PAL = 0,
	GK_STANDARD_NTSC,
	GK_STANDARD_NUM,
}GK_STANDARD;

typedef struct _GK_MEDIA_CHN_ST_
{
	int img_width;
	int img_height;
	GK_CODE  code;    
	GK_CODE_CTRL ctrl;
}GK_MEDIA_CHN_ST;

typedef struct _GK_MEDIA_ST_
{
	int stream_num;
	GK_STANDARD vi_standard; // PAL = 0 NTSC = 1
	GK_MEDIA_CHN_ST stMediaStream[MAX_STREAM_NUM];
}GK_MEDIA_ST;


typedef int (*ENC_DATA_GET_CALLBACK)(char * pBuf,int len,int chan);


int set_gk_media(GK_MEDIA_ST media);

int start_gk_media();

int stop_gk_media();

 int set_video_buf_array(void * pStreamBuf1,void * pStreamBuf2,void * pStreamBuf3);

  int set_audio_buf_array(void * pStreamBuf1);

 int set_video_recv_callback(ENC_DATA_GET_CALLBACK recv);

 int set_audio_recv_callback(ENC_DATA_GET_CALLBACK recv);

 int goke_set_enc(int stream_id,int framerate,int bitrate,int gop);

 int goke_set_enc2(int stream_id,int framerate,int bitrate,int gop);

 int goke_set_img(int brightness,int saturation,int contrast);

 int goke_create_idr(int stream_id);

 int goke_set_backlight(int enable);

 int goke_set_antiflicker(int enable,int freq);

void GK_Set_Day_Night_Params(int value, int curMainFps, int curSubFps);

 int goke_init_pda();

 int goke_set_md_buf(unsigned short * pBuf);

 int audio_ao_send(unsigned char *buf, unsigned int size);

 int goke_set_mirror(int mode);
 
 int GetVoiceCodeInfo(char * save_detect_info,int iBufLen);


#define GK_VIDEO_DATA_OFFSET  (96)  //(36)

extern int goke_auido_dev_set_flag;


#ifdef __cplusplus
};
#endif


#endif // _RJONE_RTSPSERVER_H_
