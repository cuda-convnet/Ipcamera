#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "devfile.h"
#include "sample_comm.h"
#include "gk_dev.h"
#include "singly_linked_list.h"

#include "MacBaseData.h"






GK_MEDIA_ST g_MediaSt;

SINGLY_LINKED_LIST_INFO_ST * p_gVideolist[3] = {NULL,NULL,NULL};
ENC_DATA_GET_CALLBACK video_data_get_callback = NULL;
static pthread_mutex_t goke_enc_set_control_mutex;

int set_video_buf_array(void * pStreamBuf1,void * pStreamBuf2,void * pStreamBuf3)
{
	p_gVideolist[0] = (SINGLY_LINKED_LIST_INFO_ST*)pStreamBuf1;
	p_gVideolist[1] = (SINGLY_LINKED_LIST_INFO_ST*)pStreamBuf2;
	p_gVideolist[2] = (SINGLY_LINKED_LIST_INFO_ST*)pStreamBuf3;
	return 1;
}

int set_video_recv_callback(ENC_DATA_GET_CALLBACK recv)
{
	video_data_get_callback = recv;
	return 1;
}


SINGLY_LINKED_LIST_INFO_ST * p_gAudiolist = NULL;
ENC_DATA_GET_CALLBACK audio_data_get_callback = NULL;

int set_audio_buf_array(void * pStreamBuf1)
{
	p_gAudiolist  = (SINGLY_LINKED_LIST_INFO_ST*)pStreamBuf1;	
	return 1;
}


int set_audio_recv_callback(ENC_DATA_GET_CALLBACK recv)
{
	audio_data_get_callback = recv;
	return 1;
}

int CallAudioRecvBack(char * pBuf,int len)
{
	if( audio_data_get_callback == 0 )
		return -1;
	
	return audio_data_get_callback(pBuf,len,0);
}


int set_gk_media(GK_MEDIA_ST media)
{
	int i;
	GK_MEDIA_CHN_ST * pMedia = NULL;
	
	if( media.stream_num < 0 || media.stream_num > MAX_STREAM_NUM)
	{
		DPRINTK("stream_num = %d is error!\n");
		goto err;
	}	
	
	for( i = 0; i < media.stream_num; i++)
	{
		pMedia = &media.stMediaStream[i];
		DPRINTK("[%d] %dx%d code=%d\n",i,pMedia->img_width,pMedia->img_height,pMedia->code);
	}


	g_MediaSt = media;

	return 0;
err:
	return -1;
}

static void StartVideoStream_thread(void * value)
{
	SET_PTHREAD_NAME(NULL);
	
	printf("StartVideoStream_thread .....\n");
	Hisi_StartVideoStream(g_MediaSt);

	return ;
}

int start_gk_media()
{
	int ret;
	pthread_t id_net_send;
	printf("start_gk_media .....\n");
	ret = pthread_create(&id_net_send,NULL,(void*)StartVideoStream_thread,NULL);
	if ( 0 != ret )
	{
		trace( "create net_ip_test_thread  thread error\n");
		goto err;
	}

	sleep(1);

	return 0;
err:
	return -1;
}



int stop_gk_media()
{
	return 0;
err:
	return -1;
}

static float div_safe3(float num1,float num2)
{
	if( num2 == 0 )
	{
		DPRINTK("num2=%f\n",num2);
		return -1;
	}

	return (float)(num1/num2);
}



static float rj_chn_fps_calculate2(int chn)
{
	static int count[64] = {0};
	static long oldTime[64] = {0};
	float frameRate;

	struct timeval tvOld;
       struct timeval tvNew;

	  if( count[chn] == 0 )
	  {
		 gettimeofday( &tvOld, NULL );
		oldTime[chn] = tvOld.tv_sec;
		
	  }

	  count[chn]++;

	 	
	gettimeofday( &tvNew, NULL );

	if( abs(tvNew.tv_sec - oldTime[chn]) >= 5 )
	{
		frameRate = div_safe3(count[chn] , (tvNew.tv_sec - oldTime[chn] ));
		//DPRINTK("[%d] fps=%f  count=%d\n",chn,frameRate,count[chn]);
		count[chn] = 0;
		return frameRate;
	}	

	 return -1;	
}

static int RJ_ChnFpsIncreaseCalculate2(SINGLY_LINKED_LIST_INFO_ST * plist,int chn)
{
	float fps;
	fps = rj_chn_fps_calculate2(chn);
	if( fps > 0 )
	{
		plist->chn_fps  = fps;		
	}
	return 1;	
}


int HisiInsertVideoData(SINGLY_LINKED_LIST_INFO_ST * plist,char * frame_data_buf,int frame_len,int chn)
{	
		
	int ret;
	long long tmp_time;
	BUF_FRAMEHEADER * pheader = NULL;
	pheader = (BUF_FRAMEHEADER *)frame_data_buf;

	if( pheader->type == VIDEO_FRAME )
	{						
		RJ_ChnFpsIncreaseCalculate2(plist,pheader->index);	

		tmp_time = pheader->ulTimeSec;
			tmp_time = tmp_time*1000000 + pheader->ulTimeUsec;
		plist->extern_data.last_video_time = tmp_time;

		if( GK_CODE_265 == g_MediaSt.stMediaStream[chn].code)
			plist->extern_data.video_code = (int)2;
		else
			plist->extern_data.video_code = (int)1;
	}
	
	while( sll_list_enable_add_item(plist) == 0)
	{				
		sll_del_list_tail_item(plist);				
	}		
	ret = sll_add_list_item_no_malloc(plist,frame_data_buf,frame_len);	

	
	return ret;
}




int 	hi3516e_Hisi_set_encode_bnc(int chan,int gop,int frame_rate,int bitstream)
{
	 VENC_CHN VencChn;
	 VENC_CHN_ATTR_S stVencChnAttr;
	 HI_S32 s32Ret;
	 int sub_stream_is_640x480 = 0;
	 int vbr_qulity = 0;
	int quality  = 0;		
	int new_bitstream = 0;
	 
	 s32Ret = HI_MPI_VENC_GetChnAttr(chan, &stVencChnAttr);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n", \
                   chan, s32Ret);
            return -1;
        }	

	if( stVencChnAttr.stVeAttr.enType == PT_H265 )
	{
		if( stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265CBR )
		{
			 stVencChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = gop;
		 	 stVencChnAttr.stRcAttr.stAttrH265Cbr.fr32DstFrmRate = frame_rate;
		 	 stVencChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = bitstream;
		}else if( stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265VBR )
		{
			stVencChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = gop;
		  	stVencChnAttr.stRcAttr.stAttrH265Vbr.fr32DstFrmRate = frame_rate;
		  	stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitstream;
		 
			if( g_MediaSt.vi_standard == GK_STANDARD_PAL )
				new_bitstream = (float)25* (float)bitstream/(float)frame_rate;	
			else
				new_bitstream = (float)30* (float)bitstream/(float)frame_rate;			
			

			DPRINTK("chan=%d new_bitstream=%d  %d,%d\n",chan,new_bitstream,stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp,
				stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp);
		}
	
	}else if( stVencChnAttr.stVeAttr.enType == PT_H264 )
	{
		if( stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR )
		{
			 stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = gop;
		 	 stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = frame_rate;
		 	 stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = bitstream;
		}else if( stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264VBR )
		{
		
			//gop = frame_rate *2;
			stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = gop;
		  	stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = frame_rate;
		  	stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = bitstream;
		 
			if( g_MediaSt.vi_standard == GK_STANDARD_PAL )
				new_bitstream = (float)25* (float)bitstream/(float)frame_rate;	
			else
				new_bitstream = (float)30* (float)bitstream/(float)frame_rate;		

			quality = get_vbr_quality(g_MediaSt.stMediaStream[chan].img_width,g_MediaSt.stMediaStream[chan].img_height,new_bitstream);

			if( quality != 0 )
			{
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = quality;
			} 	
			

			DPRINTK("chan=%d new_bitstream=%d  %d,%d\n",chan,new_bitstream,stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp,
				stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp);
		}
	}else
	{
		DPRINTK("Unknow code! %d\n",stVencChnAttr.stVeAttr.enType);
		return -1;
	}



	  s32Ret = HI_MPI_VENC_SetChnAttr(chan, &stVencChnAttr);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VENC_SetChnAttr chn[%d] failed with %#x!\n", \
                   chan, s32Ret);
            return -1;
        }

	 DPRINTK("chan=%d gop=%d frame_rate=%d bitstream=%d\n",chan,gop,frame_rate, bitstream);

	  return 1;
}



int goke_set_enc(int stream_id,int framerate,int bitrate,int gop)
 {
 	
 	return hi3516e_Hisi_set_encode_bnc(stream_id,gop,framerate,bitrate);
 }


int  hi3516e_set_vi_csc_attr(int u32LumaVal,int u32ContrVal,int u32HueVal,int u32SatuVal)
{
	VI_CSC_ATTR_S  stMWB_attr;
	HI_S32 s32Ret = HI_SUCCESS;

	DPRINTK("luma=%x cont=%x hue=%x sat=%x\n",u32LumaVal,u32ContrVal,u32HueVal,u32SatuVal);

	memset(&stMWB_attr,0x00,sizeof(VI_CSC_ATTR_S));

	s32Ret = HI_MPI_VI_GetCSCAttr(0,&stMWB_attr);
	if( s32Ret != HI_SUCCESS )
	{
		DPRINTK("HI_MPI_ISP_GetMWBAttr err:%x\n",s32Ret);
		return -1;
	}

	DPRINTK("HI_MPI_VI_GetCSCAttr:  luma=%x cont=%x hue=%x sat=%x\n",stMWB_attr.u32LumaVal,
		stMWB_attr.u32ContrVal,
		stMWB_attr.u32HueVal,
		stMWB_attr.u32SatuVal);



	//stMWB_attr.stAWBCalibration.au16StaticWB[0] = 0x147;

	if( u32LumaVal != 0xffff)
		stMWB_attr.u32LumaVal = u32LumaVal;
	if( u32ContrVal != 0xffff)
		stMWB_attr.u32ContrVal = u32ContrVal;
	if( u32HueVal != 0xffff)	
		stMWB_attr.u32HueVal = u32HueVal;	
	
	if( u32SatuVal != 0xffff)
	{	 
		/*
		if( IsLightDay == 1 )			
			stMWB_attr.u32SatuVal = u32SatuVal;
		else
			stMWB_attr.u32SatuVal = 0;
		
		DPRINTK("u32IsLightDay = %d\n",IsLightDay);	
		*/

		stMWB_attr.u32SatuVal = u32SatuVal;
	}
	
	DPRINTK("HI_MPI_VI_SetCSCAttr:  luma=%x cont=%x hue=%x sat=%x\n",stMWB_attr.u32LumaVal,
		stMWB_attr.u32ContrVal,
		stMWB_attr.u32HueVal,
		stMWB_attr.u32SatuVal);

	s32Ret = HI_MPI_VI_SetCSCAttr(0,&stMWB_attr);
	if( s32Ret != HI_SUCCESS )
	{
		DPRINTK("HI_MPI_VI_SetCSCAttr err:%x\n",s32Ret);
		return -1;
	}	
	
	return 1;
}



 int goke_set_img(int brightness,int saturation,int contrast)
{
 	
 	return hi3516e_set_vi_csc_attr(brightness,contrast,0xffff,saturation);
 }


 int goke_create_idr(int stream_id)
{
	int ret;
	
 	ret = HI_MPI_VENC_RequestIDR(stream_id,HI_TRUE);

	if( ret != 0 )
	{
		DPRINTK("ret = %x",ret);
		ret = -1;
	}

	return ret;
 }


 int goke_set_backlight(int enable)
{
 	DPRINTK("Not support!\n");
 	return 0;
 }


 int goke_set_antiflicker(int enable,int freq)
{
 	DPRINTK("Not support!\n");
 	return 0;
 }


void GK_Set_Day_Night_Params(int value, int curMainFps, int curSubFps)
 {
 	DPRINTK("Not support!\n");
 	return 0;
 }


 int goke_init_pda()
{
 	DPRINTK("Not support!\n");
 	return 0;
 }


 int goke_set_md_buf(unsigned short * pBuf)
{
 	DPRINTK("Not support!\n");
 	return 0;
 }


 int audio_ao_send(unsigned char *buf, unsigned int size)
{
 	
 	return Hisi_WriteAudioBuf(buf,size,0);
 }


 int goke_set_mirror(int mode)
{
 	DPRINTK("Not support!\n");
 	return 0;
 }

 
 int GetVoiceCodeInfo(char * save_detect_info,int iBufLen)
{
 	DPRINTK("Not support!\n");
 	return 0;
 }



