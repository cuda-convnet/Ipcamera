#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <string.h>

#include "audioctrl.h"




int g_audiofd = 0;
int g_audioplayfd = 0;
unsigned char   g_audiobuf[MAX_AUDIO_BUF];

int open_audio_dev(char * dev_name)
{
	int format;
	int channels;
	int sample;
	int samplesize = 16;
	int abuf_size;
	int frag = 0x7fff000A; /* 2^10 = 1024 */

	if( dev_name != NULL)
	{		

		if ((g_audiofd = open(dev_name,O_RDONLY ,0)) == -1)
		{
			 printf("audio open device error!");
			 return ERROR;
		}	

		if ((g_audioplayfd = open(dev_name,O_WRONLY ,0)) == -1)
		{
			 printf("audio open device error!");
			 return ERROR;
		}	
		
	}else
	{
		if ((g_audiofd = open(AUDIO_DEVIDE,O_RDONLY ,0)) == -1)
		{
			 printf("audio open device error!");
			 return ERROR;
		}	

		if ((g_audioplayfd = open(AUDIO_DEVIDE_2,O_WRONLY ,0)) == -1)
		{
			 printf("audio open device error!");
			 return ERROR;
		}	
	}

	//format = AFMT_U8;
	//format = AFMT_S16_LE;
	channels = CHANNEL_NUMBER;
	sample = SAMPLE_RATE;

        

	if (ioctl(g_audiofd, SNDCTL_DSP_SAMPLESIZE, &samplesize) == -1) 
	{
		printf("SNDCTL_DSP_SAMPLESIZE");	
		return ERROR;
	}	

	if (ioctl(g_audiofd, SNDCTL_DSP_CHANNELS, &channels) == -1) 
	{
		printf("SNDCTL_DSP_CHANNELS");	
		return ERROR;
	}		

	if (ioctl(g_audiofd, SNDCTL_DSP_SPEED, &sample) == -1) 
	{
		printf("SNDCTL_DSP_SPEED");		
		return ERROR;
	} 

	 if (ioctl( g_audiofd, SNDCTL_DSP_SETFRAGMENT, &frag ) == -1) {
	    perror("SNDCTL_DSP_SETFRAGMENT");
	    return ERROR;
	  }

	if (ioctl(g_audiofd, SNDCTL_DSP_GETBLKSIZE, &abuf_size) == -1) 
	{
		printf("SNDCTL_DSP_SPEED");		
		return ERROR;
	} 

	if (abuf_size != 1024) {
    			fprintf( stderr, "Device does not support fragment size of 1024\n" );    
	}

	printf("%d %d %d abuf_size = %d\n",samplesize,channels,sample,abuf_size);


	//format = AFMT_S16_LE;
	channels = CHANNEL_NUMBER;
	sample = SAMPLE_RATE;

	if (ioctl(g_audioplayfd, SNDCTL_DSP_SAMPLESIZE, &samplesize) == -1) 
	{
		printf("SNDCTL_DSP_SAMPLESIZE");	
		return ERROR;
	}	

	if (ioctl(g_audioplayfd, SNDCTL_DSP_CHANNELS, &channels) == -1) 
	{
		printf("SNDCTL_DSP_CHANNELS");	
		return ERROR;
	}		

	if (ioctl(g_audioplayfd, SNDCTL_DSP_SPEED, &sample) == -1) 
	{
		printf("SNDCTL_DSP_SPEED");		
		return ERROR;
	} 

	if (ioctl( g_audioplayfd, SNDCTL_DSP_SETFRAGMENT, &frag ) == -1) {
	    perror("SNDCTL_DSP_SETFRAGMENT");
	    return ERROR;
	  }


	return ALLRIGHT;
	
}


int close_audio_dev()
{
	if( g_audiofd <= 0 )
		return ERROR;

	close(g_audiofd);
	g_audiofd = 0;

	close(g_audioplayfd);
	g_audioplayfd = 0;

	return ALLRIGHT;
}


int read_audio_buf(unsigned char *buf,int count)
{
	int read_bytes = 0;
	int total_bytes = 0;
	int get_bytes = 0;
	unsigned char * pbuf;

	if( g_audiofd <= 0 )
		return ERROR;

	pbuf = buf;

	while( total_bytes < count )
	{
		get_bytes = count - total_bytes;		
	
		read_bytes = read(g_audiofd, (char *)pbuf, get_bytes);	

		if( read_bytes <= 0 )
			return ERROR;

		total_bytes += read_bytes;
		pbuf += read_bytes;
	}

	return count;
}

int write_audio_buf(unsigned char *buf,int count)
{
	int write_bytes = 0;
	int total_bytes = 0;
	unsigned char * pbuf;

	if( g_audioplayfd <= 0 )
		return ERROR;

	pbuf = buf;

	while( total_bytes < count )
	{
		write_bytes = count - total_bytes;
		write_bytes = write(g_audioplayfd, pbuf, write_bytes);	

		if( write_bytes <= 0 )
			return ERROR;

		total_bytes += write_bytes;
		pbuf += write_bytes;
	}

	return count;

}


