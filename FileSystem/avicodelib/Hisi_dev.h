#ifndef _HISI_DEV_H_
#define _HISI_DEV_H_


#include <sys/prctl.h>
#include "BaseData.h"
#include "bufmanage.h"
#include "ipcam_config.h"


//windew define
#define window_16 (16)
#define window_9 (9)
#define window_8 (8)
#define window_4 (4)
#define window_1 (1)

#define NET_BIG_STREAM_MODE 0
#define NET_SMALL_STREAM_MODE 1


int Hisi_init_dev(int image_size,int gs_enNorm);
int Hisi_destroy_dev();
 int Hisi_8_window_vi(int input_video_mode);
 int Hisi_stop_vi();
int Hisi_window_vo(int window,int start_cam,int x,int y,int width1,int height1);
int Hisi_stop_vo(int real_close);
 int Hisi_create_encode(ENCODE_ITEM * enc_item);
 int Hisi_destroy_encode(ENCODE_ITEM * enc_item);
int Hisi_get_encode_data(BUFLIST * list,unsigned char * encode_buf);
int Hisi_start_enc(ENCODE_ITEM * enc_item);
int Hisi_stop_enc(ENCODE_ITEM * enc_item);
int Hisi_get_net_encode_data(BUFLIST * list,unsigned char * encode_buf);
int Hisi_create_IDR(ENCODE_ITEM * enc_item);
int Hisi_create_decode(ENCODE_ITEM * enc_item);
int Hisi_destroy_decode(ENCODE_ITEM * enc_item);
int Hisi_start_dec(ENCODE_ITEM * enc_item);
int Hisi_stop_dec(ENCODE_ITEM * enc_item);
int Hisi_send_decode_data(ENCODE_ITEM * enc_item,unsigned char * pdata,int length,int display_chan);
int Hisi_playback_window_vo(int window,int start_cam);
int Hisi_set_encode(ENCODE_ITEM * enc_item);
int Hisi_create_motiondetect(ENCODE_ITEM * enc_item);
int Hisi_enable_motiondetect(ENCODE_ITEM * enc_item);
int Hisi_disable_motiondetect(ENCODE_ITEM * enc_item);
int Hisi_destroy_motiondetect(ENCODE_ITEM * enc_item);
int Hisi_get_motion_buf(ENCODE_ITEM * enc_item);
int Hisi_init_audio_device();
int Hisi_destroy_audio_device();
int Hisi_read_audio_buf(unsigned char *buf,int * count,int chan);
int Hisi_write_audio_buf(unsigned char *buf,int count,int chan);
int Hisi_set_vo_framerate(float speed);
void Hisi_set_chip_slave();
void Hisi_control_lock();
void Hisi_control_unlock();
int Hisi_pciv_host_set_slave_encode_start(int chan);
int  Hisi_pciv_host_set_slave_encode_stop(int chan);
int Hisi_pciv_host_set_slave_net_IDR(int chan);
int Hisi_pciv_host_to_slave_send_cam_data(int chan,int buf_number);
int Hisi_pciv_host_set_slave_net_start(int chan);
int Hisi_pciv_host_set_slave_net_stop(int chan);
int Hisi_pciv_host_encode_create_tune();
int Hisi_pciv_host_encode_destroy_tune();
int Hisi_pciv_host_set_slave_encode(int chan,int bitstream,int framerate,int gop);
int Hisi_init_vo();
int Hisi_pciv_host_remote_vi_open(int window );
int Hisi_pciv_host_set_vi_interlace(int chan);
int Hisi_pciv_host_remote_view(int window,int vi_chan,int vo_chan,int id,int chan);
int Hisi_pciv_host_remote_view_stop(int id);
int slave_net_send_cam_total_num();
int Hisi_pciv_slave_init();
int Hisi_clear_vo_buf(int chan);
void Hisi_set_venc_vbr();
int Net_send_self_data(char * buf,int buf_length);
int Hisi_set_spi_antiflickerattr(int gs_enNorm,int antiflicker_mode);
IPCAM_ALL_INFO_ST * get_ipcam_config_st();
int save_ipcam_config_st(IPCAM_ALL_INFO_ST * pIpcam_st);
int Hisi_set_isp_value(IPCAM_ALL_INFO_ST * pIpcamInfo,int cmd);

#endif

