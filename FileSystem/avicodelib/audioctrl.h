#ifndef _AUDIO_CTRL_H_
#define _AUDIO_CTRL_H_

#define SAMPLE_RATE         8000
#define MAX_AUDIO_BUF     (4096*2)
#define CHANNEL_NUMBER  1
#define AUDIO_DEVIDE       "/dev/dsp2"
#define AUDIO_DEVIDE_2       "/dev/dsp2"



int open_audio_dev(char * dev_name);
int close_audio_dev();
int read_audio_buf(unsigned char *buf,int count);
int write_audio_buf(unsigned char *buf,int count);

#endif 
