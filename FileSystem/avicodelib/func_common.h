#ifndef _FUNC_COMMON_H_
#define _FUNC_COMMON_H_

#define REC_BUF_REMAIN_NUM (300)
#define NET_BUF_REMAIN_NUM (1)

void get_cur_img_x_y(int * width,int * height);
int get_sys_time(struct timeval *tv,char *p);
int get_cur_standard();
int get_cur_size();
int is_show_net_channel(int channel);
void net_chan_send_count_add(int index);
int is_chan_net_send(int chan);
int net_send_cam_total_num();
int get_net_low_chan();
int get_dvr_max_chan(int use_chip_flag);
void set_dvr_max_chan(int chan);
int div_safe2(float num1,float num2);
int net_send_calculate(int cam,int send_success);
int get_net_local_cam();
int Hisi_is_slave_chip();
int net_play_rec_rate_adujst();
int net_play_rec_rate_restore();
int is_16ch_host_machine();
int is_16ch_slave_machine();
int machine_use_audio();
int net_cam_recv_data(void * point,char * buf,int length,int cmd);
int Net_dvr_send_self_data(char * buf,int length);
int Net_dvr_have_client();
int Net_dvr_get_send_frame_num();
int rstp_server_start();
int rstp_write_rtp_frame(int length, void *data);
void rstp_server_stop();
int Net_cam_have_client_check_all_socket();
int Hisi_get_sensor_type();
int Hisi_get_isp_light_dark_value();
void  Hisi_set_isp_light_dark_value(int value);

#endif


