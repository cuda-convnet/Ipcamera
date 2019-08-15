#ifndef _RTSP_LIB_HEADER_
#define _RTSP_LIB_HEADER_

#ifdef __cplusplus
extern "C" {
#endif


int rtsp_build_stream(int rtsp_chan,char * rtsp_name,int port);
int rtsp_set_enc_list_ptr(void * list);
void rtsp_set_frame_rate(int chan ,float rate);
void  rtsp_set_frame_durationInMicroseconds(int chan ,unsigned int iDurationInMicroseconds);
int rtsp_insert_enc_data(int chan ,char * buf,int length,int cmd);
int rtsp_set_h264_sps_pps(int chan,char * buf, int len);





#ifdef __cplusplus
}
#endif

#endif

