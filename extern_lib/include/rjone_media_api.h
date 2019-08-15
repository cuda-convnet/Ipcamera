


#ifndef _RJONE_MEDIA_API_
#define _RJONE_MEDIA_API_

#ifdef __cplusplus
extern "C" {
#endif

#include "rjone_type.h"
#include "rjone_err.h"
#include "rjone.h"


#define RJONE_MEDIA_SYS_VER  (0x01000001)
#define RJONE_MAX_MEDIA_NUM (5)





typedef enum 
{
    HISI_GPIO_0=0,
    HISI_GPIO_1,
    HISI_GPIO_2,
    HISI_GPIO_3,
    HISI_GPIO_4,
    HISI_GPIO_5,
    HISI_GPIO_6,
    HISI_GPIO_7,
    HISI_GPIO_8,
    HISI_GPIO_9,
    HISI_GPIO_10,
    HISI_GPIO_11,
}GPIO_ID_E;

typedef enum 
{
    HISI_GPIO_WORK_MODE_INPUT=0,
    HISI_GPIO_WORK_MODE_OUTPUT,
}GPIO_WORK_MODE_E;

typedef struct _hisi_gpio_st_
{
	GPIO_ID_E uGpioId;                             			/** GPIO ��  �� GPIO_0  GPIO1 **/
	GPIO_WORK_MODE_E uGpioWorkMode;			/** GPIO ����ģʽ�������ģʽ**/
	unsigned char uGpioMask;						/** �����ùܽ� ���� �� 0x01->GPIO_0_1    0x80->GPIO_0_7     0x03->(GPIO_0_1 and GPIO_0_2) **/
	unsigned char uGpioValue;						/** GPIO ֵ�����GPIO��������ģʽ�� uGpioValue  ���� GPIO_VALUE & uGpioMask  **/
}HISI_GPIO_ST;



#define RJONE_MAX_VIDEO_BUF_SIZE				(524288) /* (512*1024)*/
#define RJONE_MAX_AUDIO_BUF_SIZE				(8192) /* (512*1024)*/

typedef enum
{
	CHIP_TYPE_BASE=0,
    	CHIP_TYPE_HI3518E,
    	CHIP_TYPE_GM8126,
}RJONE_CHIP_TYPE_E;


typedef enum
{
	SENSOR_TYPE_BT656 = 0,
	SENSOR_TYPE_AR0130,
	SENSOR_TYPE_OV9712,
	SENSOR_TYPE_GC_1004,
	SENSOR_TYPE_SOI_H22,
	SENSOR_TYPE_OV2710,
}RJONE_SENSOR_TYPE_E;




typedef enum
{
	MEDIA_TYPE_BASE = 98,
    	MEDIA_TYPE_VIDEO,
    	MEDIA_TYPE_AUDIO,
}RJONE_MEDIA_TYPE_E;


typedef enum
{

    RJONE_VENC_RC_MODE_H264CBR = 1,    
    RJONE_VENC_RC_MODE_H264VBR,        
    RJONE_VENC_RC_MODE_H264ABR,        
    RJONE_VENC_RC_MODE_H264FIXQP,      
    RJONE_VENC_RC_MODE_MJPEGCBR,    
    RJONE_VENC_RC_MODE_MJPEGVBR,     
    RJONE_VENC_RC_MODE_MJPEGABR,     
    RJONE_VENC_RC_MODE_MJPEGFIXQP,  
    RJONE_VENC_RC_MODE_MPEG4CBR,    
    RJONE_VENC_RC_MODE_MPEG4VBR,       
    RJONE_VENC_RC_MODE_MPEG4ABR,       
    RJONE_VENC_RC_MODE_MPEG4FIXQP,  
    RJONE_VENC_RC_MODE_H264CBRv2,
    RJONE_VENC_RC_MODE_H264VBRv2,
    RJONE_VENC_RC_MODE_BUTT,
}RJONE_VENC_RC_MODE_E;








typedef struct _media_video_attr_
{
	RJONE_CODE_TYPE encType;               /* ѹ����ʽ		*/
	RJONE_VENC_RC_MODE_E rcMode;		/* �������Ʒ�ʽ*/	
	int iPicWidth; 		 /*��Ƶ ͼ����   */
	int iPicHeight;		/* ��Ƶͼ��߶�*/	
	int iFrameRate;		/* ֡�� fps */
	int iBitRate;			/* ������С*/
	int iGop;				/* �ؼ�֡���*/
}RJONE_VIDEO_ATTR;




typedef struct _snap_jpg_pic_
{
	char * pPicPtr;
	int iPicLen;
}SNAP_JPG_PIC;


typedef enum _audio_sampel_rate_ 
{ 
    RJONE_AUDIO_SAMPLE_RATE_8000   = 8000,    /* 8K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_12000  = 12000,   /* 12K samplerate*/    
    RJONE_AUDIO_SAMPLE_RATE_11025  = 11025,   /* 11.025K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_16000  = 16000,   /* 16K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_22050  = 22050,   /* 22.050K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_24000  = 24000,   /* 24K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_32000  = 32000,   /* 32K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_44100  = 44100,   /* 44.1K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_48000  = 48000,   /* 48K samplerate*/
    RJONE_AUDIO_SAMPLE_RATE_BUTT,
}RJONE_AUDIO_SAMPLE_RATE_E; 



typedef enum _audio_bit_width_
{
    RJONE_AUDIO_BIT_WIDTH_8   = 0,   /* 8bit width */
    RJONE_AUDIO_BIT_WIDTH_16  = 1,   /* 16bit width*/
    RJONE_AUDIO_BIT_WIDTH_32  = 2,   /* 32bit width*/
    RJONE_AUDIO_BIT_WIDTH_BUTT,
} RJONE_AUDIO_BIT_WIDTH_E;


typedef enum  _aio_sound_mode_
{
     RJONE_AUDIO_SOUND_MODE_MONO   =0,/*mono*/
     RJONE_AUDIO_SOUND_MODE_STEREO =1,/*stereo*/
     RJONE_AUDIO_SOUND_MODE_BUTT    
} RJONE_AUDIO_SOUND_MODE_E;




typedef struct _media_audio_attr_
{
	RJONE_CODE_TYPE encType;               /* ѹ����ʽ		*/	
	RJONE_AUDIO_SAMPLE_RATE_E sampleRate; 		 /*��Ƶ������  */
	RJONE_AUDIO_BIT_WIDTH_E bitWidth;			/* ��Ƶλ��*/	
	RJONE_AUDIO_SOUND_MODE_E soundMode;		/* ��Ƶģʽ mono or stereo*/
	int iPtNumPerFrm;			/* ÿ֡�Ĳ������� (80/160/240/320/480/1024/2048) */
	int iChnCnt; 				/* ������*/
}RJONE_AUDIO_ATTR;



typedef struct media_attr_s
{	
	int iMediaId;					/* ���media_attr ��Ӧ��media Id ֵ */
	RJONE_MEDIA_TYPE_E mediaType;   /* media is video or audio type.  */
	union
	{
		RJONE_VIDEO_ATTR videoAttr; 
		RJONE_AUDIO_ATTR audioAttr;
	};
}RJONE_MEDIA_ATTR_S;


typedef struct media_sys_param
{  
    RJONE_CHIP_TYPE_E chipType;        	/*ʹ�õ�оƬ�ͺ�*/
    RJONE_SENSOR_TYPE_E sensorType; 	/*  ʹ�õ�sensor�ͺ�*/ 
    RJONE_VIDEO_NORM_E videoNorm;		/* ��Ƶ��ʽ*/
    int iMediaNum;					/* ϵͳ��ʼ��ʱ������media ����*/ 
    RJONE_MEDIA_ATTR_S mediaAttr[RJONE_MAX_MEDIA_NUM];     /* media ���� */   
}RJONE_MEDIA_SYS_PARAM;


typedef struct video_frame_header_s
{
       int iCode;                                           // ѹ����ʽ
	int iIsKeyFrame;					/* �ǲ���I ֡*/
	int iFrameDataLen;				/* ֡���ݳ���*/	
	UINT8 u8FrameIndex;
	int iFramerate;					// ֡��
}VIDEO_FRAME_HEADER_S;

typedef struct audio_frame_header_s
{
       INT8 iCode;                                           // ѹ����ʽ
      	UINT8 u8AudioSamplerate;				//������ RJONE_AUDIO_SAMPLERATE
	UINT8 u8AudioBits;						//����λ��RJONE_AUDIO_DATABITS
	UINT8 u8AudioChannel;					//ͨ��		RJONE_AUDIO_CHANNEL		
	int iRawAudioDataLen;			/*  ԭʼ��Ƶ���ݳ���(δѹ������Ƶ����)*/
	int iEncAudioDataLen;			/*  ԭʼ��Ƶ����ѹ�������ݳ���*/		
	UINT8 u8FrameIndex;
}AUDIO_FRAME_HEADER_S;


typedef struct _media_frame_head_s_
{
	RJONE_MEDIA_TYPE_E mediaType;   /* ��Ƶ֡������Ƶ֡*/
	struct timeval  timeStamp;	
	union
	{
		VIDEO_FRAME_HEADER_S videoFrameHead; 
		AUDIO_FRAME_HEADER_S audioFrameHead;
	};		
	char reserved[4];
}RJONE_MEDIA_FRAME_HEAD_S;


typedef struct _motion_detect_buf_s_
{
	unsigned short * buf;
	int mdWidth;
	int mdHeigth;
	int isTriggerMotion;
}MOTION_DETECT_BUF_S;


/************************************************************************
 ������:RJONE_GetMediaSystemVer
 ����: �õ�media system �汾��
 ����:	��
 ���:��
 ����:�ɹ����� 
 		MedisSystem Ver(UINT)   "Version: %d.%d.%d.%d", (Ver & 0xFF000000)>>24, (Ver & 0x00FF0000)>>16, (Ver & 0x0000FF00)>>8, (Ver & 0x000000FF) >> 0 )
 		
 		���󷵻� 
 		RJONE_FAILED
 ˵��:
************************************************************************/
unsigned int RJONE_GetMediaSystemVer();  


/************************************************************************
 ������:RJONE_InitMediaSystem
 ����:  ��ʼ��media ϵͳ
 ����:
 		pstMediaSysParam   �ṹ��ָ��
 		RecvCallBackFunc    �ص�����ָ�룬�����Ƶ����Ƶ���ݡ�
 		int (*RecvCallBackFunc)(RJONE_MEDIA_FRAME_HEAD_S * pFrameHead,char * pFrameBuf,int iMediaId); 
 		�������
 			pFrameHead  ֡ͷ�ṹ��
 			pFrameBuf    ֡����
 			iMediaId   ������֡��ý��ID��
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_InitMediaSystem(RJONE_MEDIA_SYS_PARAM * pstMediaSysParam,int (*RecvCallBackFunc)(RJONE_MEDIA_FRAME_HEAD_S * ,char * ,int ));  


/************************************************************************
 ������:RJONE_MediaSystemStartWork
 ����: media ϵͳ��ʼ����
 ����:
 		��
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_MediaSystemStartWork();  

/************************************************************************
 ������:RJONE_MediaSystemStopWork
 ����: media ϵͳֹͣ����
 ����:
 		��
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_MediaSystemStopWork();  


/************************************************************************
 ������:RJONE_DestroyMediaSystem
 ����: �ر�media ϵͳ
 ����:
 		��
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_DestroyMediaSystem();  



/************************************************************************
 ������:RJONE_GetMediaSystemParam
 ����:  ���mediaϵͳ����
 ����:
 		��
 ���:
 		pstMediaSysParam  ������Ż�õĽṹ������
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_GetMediaSystemParam(J_OUT RJONE_MEDIA_SYS_PARAM * pstMediaSysParam); 



/************************************************************************
 ������:RJONE_SetVideoEncode
 ����:������Ƶѹ���������
 ����:
 		iMediaId   ��RJONE_MEDIA_ATTR_S �ṹ���ж�Ӧ��iMediaId.
 		iFrameRate  	���õ�֡��
 		iBitRate		���õ�����
 		iGop			���õ�I֡���
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_SetVideoEncode(int iMediaId,int iFrameRate,int iBitRate,int iGop);




/************************************************************************
 ������:RJONE_SendDataToAudioDec
 ����:������Ƶ����
 ����: 		
 		pAudioData  	��Ƶ����ָ��
 		iDataLen		��Ƶ���ݳ��� 		
 		enEncodeType  ��Ƶ�����ʽ
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_SendDataToAudioDec(void * pAudioData,int iDataLen , RJONE_CODE_TYPE enEncodeType);



/************************************************************************
 ������:RJONE_EnableAec
 ����:���û�������
 ����:
 		��
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_EnableAec();


/************************************************************************
 ������:RJONE_DisableAec
 ����:�رջ�������
 ����:
 		��	
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_DisableAec();



/************************************************************************
 ������:RJONE_GetSnapPictureData
 ����:ץͼ
 ����:��	
 ���:
 		pStorePicData  	��ȡ	SNAP_JPG_PIC �ṹ���ݡ�
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_GetSnapPictureData(J_OUT SNAP_JPG_PIC * pStorePicData);


/************************************************************************
 ������:RJONE_ReleaseSnapPictureData
 ����:ץͼ
 ����: 		
 		pStorePicData  Ҫ�ͷ��ڴ��ץͼ�ṹ�塣
 ���:��
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_ReleaseSnapPictureData(SNAP_JPG_PIC * pStorePicData);


/************************************************************************
 ������:RJONE_GetGammaTable
 ����: ��õ�ǰGamma Type ֵ
 ����: ��
 ���:
 		peGamaType ��õ�ǰGamaType ֵ��
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_GetGammaTable(J_OUT RJONE_GAMMA_TYPE * peGamaType);

/************************************************************************
 ������:RJONE_SetGammaTable
 ����: ����ͼ�����GAMA ��
 ����: 		
 		eGamaType  ʹ������GAMAģʽ
 		pSelfGameTable  ��eGamaType = GAMMA_TYPE_SELF ʱ���û�GamaTable ָ�롣
 ���:��
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_SetGammaTable(RJONE_GAMMA_TYPE eGamaType,void * pSelfGameTable);


/************************************************************************
 ������:RJONE_GetViCSCAttr
 ����:  ���ͼ�����
 ����:��	 		
 ���:
 		piLumaVal  	����ֵ  luminance: [0 ~ 100] 
 		piContrVal		�Աȶ�  contrast : [0 ~ 100] 
 		piHueVal		ɫ��	    hue : [0 ~ 100
 		piSatuVal         ���Ͷ�  satuature: [0 ~ 100]
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_GetViCSCAttr(J_OUT int * piLumaVal,J_OUT int * piContrVal,J_OUT int * piHueVal,J_OUT int * piSatuVal);




/************************************************************************
 ������:RJONE_SetViCSCAttr
 ����:  ����ͼ�����
 ����: 		
 		iLumaVal  	����ֵ  luminance: [0 ~ 100] 
 		iContrVal		�Աȶ�  contrast : [0 ~ 100] 
 		iHueVal		ɫ��	    hue : [0 ~ 100
 		iSatuVal         ���Ͷ�  satuature: [0 ~ 100]
 ���:��
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_SetViCSCAttr(int iLumaVal,int iContrVal,int iHueVal,int iSatuVal);



/************************************************************************
 ������:RJONE_GPIO_OP
 ����:����GPIO �ܽ�
 ����:
 		pGpioData GPIO ���ƽṹ��ָ��
 ���:
 		pGpioData ������GPIOΪ���빤��ģʽʱ���������GPIO �ܽŶ�ȡ��ֵ�� 
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_GPIO_OP(J_IN J_OUT HISI_GPIO_ST  * pGpioData);



/************************************************************************
 ������:RJONE_Get_Adc0_Value
 ����:��ȡadc0 �ɼ�������
 ����:
 		��
 ���:
 		pU8Value ȡ������ֵ��ָ�롣
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_Get_Adc0_Value(J_OUT UINT8  * pU8Value);


/************************************************************************
 ������:RJONE_Set_Video_Mirror_Flip
 ����:�Բɼ�ͼ�����ˮƽ�ʹ�ֱ��ת
 ����:
 		��
 ���:
 		pU8Value ȡ������ֵ��ָ�롣
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_Set_Video_Mirror_Flip(int iMirror,int iFlip);



/************************************************************************
 ������:RJONE_CreateIKeyFrame
 ����: ��Ƶ������ʱI ֡
 ����: ��
 ���:
 		iMediaId   ��RJONE_MEDIA_ATTR_S �ṹ���ж�Ӧ��iMediaId.
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_CreateIKeyFrame(int iMediaId);


/************************************************************************
 ������:RJONE_ShowOsd
 ����: ����Ƶͼ������ʾOSD
 ����: 
 		id  OSD id ��Χ��[0 - 7]
 		chan  ��Ƶͨ�� [0 - 1] 
 		screen_x  OSD ��ʾ���������Ͻ� x ֵ
 		screen_y  OSD ��ʾ���������Ͻ� y �
 		show_buf  OSD��Ҫ��ʾ��ͼƬ���ݵ�ַ��ͼƬ��ʽΪARGB1555
 		show_pic_witdh  ��ʾ��ͼƬ �Ŀ��
 		show_pic_witdh  ��ʾ��ͼƬ �ĸ߶�
 		u32BgAlpha   ������ɫ͸���� ��Χ [0-128]    ֵԽСԽ͸��
 		u32FgAlpha    ǰ����ɫ͸���� ��Χ[0-128]  ֵԽСԽ͸��
 ���:
 		��
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_ShowOsd(int id,int chan,int screen_x,int screen_y,char * show_buf,int show_pic_witdh, int show_pic_height,unsigned int u32BgAlpha,unsigned int u32FgAlpha);


/************************************************************************
 ������:RJONE_GetMotionInfo
 ����:  ��ȡ�ƶ������Ϣ
 ����:
 		��
 ���:
 		pBufSt  			�����ƶ����ṹ��ָ�� 		
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_GetMotionInfo( J_OUT MOTION_DETECT_BUF_S  * pBufSt);


/************************************************************************
 ������:RJONE_SetWorkFrequency
 ����:   �����豸�Ĺ���Ƶ��
 ����:
 		enVideoNorm  ����Ƶ��
 ���:
 		��	
 ����:�ɹ�����RJONE_SUCCESS
 ˵��:
************************************************************************/
int RJONE_SetWorkFrequency( J_IN RJONE_VIDEO_NORM_E  enVideoNorm);


/************************************************************************
 ������:RJONE_CloseOsd
 ����: �ر�ָ�� OSD,���䲻��ʾ
 ����: 
 		id  OSD id ��Χ��[0 - 7]
 ���:
 		��
 ����:�ɹ�����RJONE_SUCCESS 	     
 ˵��:
************************************************************************/
int RJONE_CloseOsd(int id);

#ifdef __cplusplus
}
#endif

#endif 

