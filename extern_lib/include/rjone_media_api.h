


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
	GPIO_ID_E uGpioId;                             			/** GPIO ºÅ  Èç GPIO_0  GPIO1 **/
	GPIO_WORK_MODE_E uGpioWorkMode;			/** GPIO ÊäÈëÄ£Ê½»òÕßÊä³öÄ£Ê½**/
	unsigned char uGpioMask;						/** Æğ×÷ÓÃ¹Ü½Å ÑÚÂë Èç 0x01->GPIO_0_1    0x80->GPIO_0_7     0x03->(GPIO_0_1 and GPIO_0_2) **/
	unsigned char uGpioValue;						/** GPIO Öµ£¬Èç¹ûGPIO´¦ÓÚÊäÈëÄ£Ê½£¬ uGpioValue  ·µ»Ø GPIO_VALUE & uGpioMask  **/
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
	RJONE_CODE_TYPE encType;               /* Ñ¹Ëõ¸ñÊ½		*/
	RJONE_VENC_RC_MODE_E rcMode;		/* ÂëÁ÷¿ØÖÆ·½Ê½*/	
	int iPicWidth; 		 /*ÊÓÆµ Í¼Ïñ¿í¶È   */
	int iPicHeight;		/* ÊÓÆµÍ¼Ïñ¸ß¶È*/	
	int iFrameRate;		/* Ö¡ÂÊ fps */
	int iBitRate;			/* ÂëÁ÷´óĞ¡*/
	int iGop;				/* ¹Ø¼üÖ¡¼ä¸ô*/
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
	RJONE_CODE_TYPE encType;               /* Ñ¹Ëõ¸ñÊ½		*/	
	RJONE_AUDIO_SAMPLE_RATE_E sampleRate; 		 /*ÒôÆµ²ÉÑùÂÊ  */
	RJONE_AUDIO_BIT_WIDTH_E bitWidth;			/* ÒôÆµÎ»¿í*/	
	RJONE_AUDIO_SOUND_MODE_E soundMode;		/* ÒôÆµÄ£Ê½ mono or stereo*/
	int iPtNumPerFrm;			/* Ã¿Ö¡µÄ²ÉÑùµãÊı (80/160/240/320/480/1024/2048) */
	int iChnCnt; 				/* ÉùµÀÊı*/
}RJONE_AUDIO_ATTR;



typedef struct media_attr_s
{	
	int iMediaId;					/* Óë´Ëmedia_attr ¶ÔÓ¦µÄmedia Id Öµ */
	RJONE_MEDIA_TYPE_E mediaType;   /* media is video or audio type.  */
	union
	{
		RJONE_VIDEO_ATTR videoAttr; 
		RJONE_AUDIO_ATTR audioAttr;
	};
}RJONE_MEDIA_ATTR_S;


typedef struct media_sys_param
{  
    RJONE_CHIP_TYPE_E chipType;        	/*Ê¹ÓÃµÄĞ¾Æ¬ĞÍºÅ*/
    RJONE_SENSOR_TYPE_E sensorType; 	/*  Ê¹ÓÃµÄsensorĞÍºÅ*/ 
    RJONE_VIDEO_NORM_E videoNorm;		/* ÊÓÆµÖÆÊ½*/
    int iMediaNum;					/* ÏµÍ³³õÊ¼»¯Ê±£¬ÆôÓÃmedia ÊıÁ¿*/ 
    RJONE_MEDIA_ATTR_S mediaAttr[RJONE_MAX_MEDIA_NUM];     /* media ÊôĞÔ */   
}RJONE_MEDIA_SYS_PARAM;


typedef struct video_frame_header_s
{
       int iCode;                                           // Ñ¹Ëõ¸ñÊ½
	int iIsKeyFrame;					/* ÊÇ²»ÊÇI Ö¡*/
	int iFrameDataLen;				/* Ö¡Êı¾İ³¤¶È*/	
	UINT8 u8FrameIndex;
	int iFramerate;					// Ö¡ÂÊ
}VIDEO_FRAME_HEADER_S;

typedef struct audio_frame_header_s
{
       INT8 iCode;                                           // Ñ¹Ëõ¸ñÊ½
      	UINT8 u8AudioSamplerate;				//²ÉÑùÂÊ RJONE_AUDIO_SAMPLERATE
	UINT8 u8AudioBits;						//²ÉÑùÎ»¿íRJONE_AUDIO_DATABITS
	UINT8 u8AudioChannel;					//Í¨µÀ		RJONE_AUDIO_CHANNEL		
	int iRawAudioDataLen;			/*  Ô­Ê¼ÒôÆµÊı¾İ³¤¶È(Î´Ñ¹ËõµÄÒôÆµÊı¾İ)*/
	int iEncAudioDataLen;			/*  Ô­Ê¼ÒôÆµÊı¾İÑ¹ËõºóÊı¾İ³¤¶È*/		
	UINT8 u8FrameIndex;
}AUDIO_FRAME_HEADER_S;


typedef struct _media_frame_head_s_
{
	RJONE_MEDIA_TYPE_E mediaType;   /* ÊÓÆµÖ¡»òÕßÒôÆµÖ¡*/
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
 º¯ÊıÃû:RJONE_GetMediaSystemVer
 ¹¦ÄÜ: µÃµ½media system °æ±¾ºÅ
 ÊäÈë:	ÎŞ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»Ø 
 		MedisSystem Ver(UINT)   "Version: %d.%d.%d.%d", (Ver & 0xFF000000)>>24, (Ver & 0x00FF0000)>>16, (Ver & 0x0000FF00)>>8, (Ver & 0x000000FF) >> 0 )
 		
 		´íÎó·µ»Ø 
 		RJONE_FAILED
 ËµÃ÷:
************************************************************************/
unsigned int RJONE_GetMediaSystemVer();  


/************************************************************************
 º¯ÊıÃû:RJONE_InitMediaSystem
 ¹¦ÄÜ:  ³õÊ¼»¯media ÏµÍ³
 ÊäÈë:
 		pstMediaSysParam   ½á¹¹ÌåÖ¸Õë
 		RecvCallBackFunc    »Øµ÷º¯ÊıÖ¸Õë£¬»ñµÃÊÓÆµºÍÒôÆµÊı¾İ¡£
 		int (*RecvCallBackFunc)(RJONE_MEDIA_FRAME_HEAD_S * pFrameHead,char * pFrameBuf,int iMediaId); 
 		ÊäÈë²ÎÊı
 			pFrameHead  Ö¡Í·½á¹¹Ìå
 			pFrameBuf    Ö¡Êı¾İ
 			iMediaId   ²úÉú´ËÖ¡µÄÃ½ÌåIDºÅ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_InitMediaSystem(RJONE_MEDIA_SYS_PARAM * pstMediaSysParam,int (*RecvCallBackFunc)(RJONE_MEDIA_FRAME_HEAD_S * ,char * ,int ));  


/************************************************************************
 º¯ÊıÃû:RJONE_MediaSystemStartWork
 ¹¦ÄÜ: media ÏµÍ³¿ªÊ¼¹¤×÷
 ÊäÈë:
 		ÎŞ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_MediaSystemStartWork();  

/************************************************************************
 º¯ÊıÃû:RJONE_MediaSystemStopWork
 ¹¦ÄÜ: media ÏµÍ³Í£Ö¹¹¤×÷
 ÊäÈë:
 		ÎŞ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_MediaSystemStopWork();  


/************************************************************************
 º¯ÊıÃû:RJONE_DestroyMediaSystem
 ¹¦ÄÜ: ¹Ø±Õmedia ÏµÍ³
 ÊäÈë:
 		ÎŞ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_DestroyMediaSystem();  



/************************************************************************
 º¯ÊıÃû:RJONE_GetMediaSystemParam
 ¹¦ÄÜ:  »ñµÃmediaÏµÍ³²ÎÊı
 ÊäÈë:
 		ÎŞ
 Êä³ö:
 		pstMediaSysParam  ÓÃÀ´´æ·Å»ñµÃµÄ½á¹¹ÌåÊı¾İ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_GetMediaSystemParam(J_OUT RJONE_MEDIA_SYS_PARAM * pstMediaSysParam); 



/************************************************************************
 º¯ÊıÃû:RJONE_SetVideoEncode
 ¹¦ÄÜ:ÉèÖÃÊÓÆµÑ¹ËõÒıÇæ²ÎÊı
 ÊäÈë:
 		iMediaId   ÓëRJONE_MEDIA_ATTR_S ½á¹¹ÌåÖĞ¶ÔÓ¦µÄiMediaId.
 		iFrameRate  	ÉèÖÃµÄÖ¡ÂÊ
 		iBitRate		ÉèÖÃµÄÂëÁ÷
 		iGop			ÉèÖÃµÄIÖ¡¼ä¸ô
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_SetVideoEncode(int iMediaId,int iFrameRate,int iBitRate,int iGop);




/************************************************************************
 º¯ÊıÃû:RJONE_SendDataToAudioDec
 ¹¦ÄÜ:²¥·ÅÒôÆµÊı¾İ
 ÊäÈë: 		
 		pAudioData  	ÒôÆµÊı¾İÖ¸Õë
 		iDataLen		ÒôÆµÊı¾İ³¤¶È 		
 		enEncodeType  ÒôÆµ±àÂë¸ñÊ½
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_SendDataToAudioDec(void * pAudioData,int iDataLen , RJONE_CODE_TYPE enEncodeType);



/************************************************************************
 º¯ÊıÃû:RJONE_EnableAec
 ¹¦ÄÜ:ÆôÓÃ»ØÉùµÖÏû
 ÊäÈë:
 		ÎŞ
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_EnableAec();


/************************************************************************
 º¯ÊıÃû:RJONE_DisableAec
 ¹¦ÄÜ:¹Ø±Õ»ØÉùµÖÏû
 ÊäÈë:
 		ÎŞ	
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_DisableAec();



/************************************************************************
 º¯ÊıÃû:RJONE_GetSnapPictureData
 ¹¦ÄÜ:×¥Í¼
 ÊäÈë:ÎŞ	
 Êä³ö:
 		pStorePicData  	»ñÈ¡	SNAP_JPG_PIC ½á¹¹Êı¾İ¡£
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_GetSnapPictureData(J_OUT SNAP_JPG_PIC * pStorePicData);


/************************************************************************
 º¯ÊıÃû:RJONE_ReleaseSnapPictureData
 ¹¦ÄÜ:×¥Í¼
 ÊäÈë: 		
 		pStorePicData  ÒªÊÍ·ÅÄÚ´æµÄ×¥Í¼½á¹¹Ìå¡£
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_ReleaseSnapPictureData(SNAP_JPG_PIC * pStorePicData);


/************************************************************************
 º¯ÊıÃû:RJONE_GetGammaTable
 ¹¦ÄÜ: »ñµÃµ±Ç°Gamma Type Öµ
 ÊäÈë: ÎŞ
 Êä³ö:
 		peGamaType »ñµÃµ±Ç°GamaType Öµ¡£
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_GetGammaTable(J_OUT RJONE_GAMMA_TYPE * peGamaType);

/************************************************************************
 º¯ÊıÃû:RJONE_SetGammaTable
 ¹¦ÄÜ: ÉèÖÃÍ¼Ïñ´¦ÀíµÄGAMA ±í
 ÊäÈë: 		
 		eGamaType  Ê¹ÓÃÄÇÖÖGAMAÄ£Ê½
 		pSelfGameTable  µ±eGamaType = GAMMA_TYPE_SELF Ê±£¬ÓÃ»§GamaTable Ö¸Õë¡£
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_SetGammaTable(RJONE_GAMMA_TYPE eGamaType,void * pSelfGameTable);


/************************************************************************
 º¯ÊıÃû:RJONE_GetViCSCAttr
 ¹¦ÄÜ:  »ñµÃÍ¼Ïñ²ÎÊı
 ÊäÈë:ÎŞ	 		
 Êä³ö:
 		piLumaVal  	ÁÁ¶ÈÖµ  luminance: [0 ~ 100] 
 		piContrVal		¶Ô±È¶È  contrast : [0 ~ 100] 
 		piHueVal		É«²î	    hue : [0 ~ 100
 		piSatuVal         ±¥ºÍ¶È  satuature: [0 ~ 100]
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_GetViCSCAttr(J_OUT int * piLumaVal,J_OUT int * piContrVal,J_OUT int * piHueVal,J_OUT int * piSatuVal);




/************************************************************************
 º¯ÊıÃû:RJONE_SetViCSCAttr
 ¹¦ÄÜ:  ÉèÖÃÍ¼Ïñ²ÎÊı
 ÊäÈë: 		
 		iLumaVal  	ÁÁ¶ÈÖµ  luminance: [0 ~ 100] 
 		iContrVal		¶Ô±È¶È  contrast : [0 ~ 100] 
 		iHueVal		É«²î	    hue : [0 ~ 100
 		iSatuVal         ±¥ºÍ¶È  satuature: [0 ~ 100]
 Êä³ö:ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_SetViCSCAttr(int iLumaVal,int iContrVal,int iHueVal,int iSatuVal);



/************************************************************************
 º¯ÊıÃû:RJONE_GPIO_OP
 ¹¦ÄÜ:¿ØÖÆGPIO ¹Ü½Å
 ÊäÈë:
 		pGpioData GPIO ¿ØÖÆ½á¹¹ÌåÖ¸Õë
 Êä³ö:
 		pGpioData µ±ÉèÖÃGPIOÎªÊäÈë¹¤×÷Ä£Ê½Ê±£¬Ëü½«»ñµÃGPIO ¹Ü½Å¶ÁÈ¡µÄÖµ¡£ 
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_GPIO_OP(J_IN J_OUT HISI_GPIO_ST  * pGpioData);



/************************************************************************
 º¯ÊıÃû:RJONE_Get_Adc0_Value
 ¹¦ÄÜ:¶ÁÈ¡adc0 ²É¼¯µÄÊı¾İ
 ÊäÈë:
 		ÎŞ
 Êä³ö:
 		pU8Value È¡µÃÊı¾İÖµµÄÖ¸Õë¡£
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_Get_Adc0_Value(J_OUT UINT8  * pU8Value);


/************************************************************************
 º¯ÊıÃû:RJONE_Set_Video_Mirror_Flip
 ¹¦ÄÜ:¶Ô²É¼¯Í¼Ïñ½øĞĞË®Æ½ºÍ´¹Ö±·­×ª
 ÊäÈë:
 		ÎŞ
 Êä³ö:
 		pU8Value È¡µÃÊı¾İÖµµÄÖ¸Õë¡£
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_Set_Video_Mirror_Flip(int iMirror,int iFlip);



/************************************************************************
 º¯ÊıÃû:RJONE_CreateIKeyFrame
 ¹¦ÄÜ: ÊÓÆµ²úÉú¼´Ê±I Ö¡
 ÊäÈë: ÎŞ
 Êä³ö:
 		iMediaId   ÓëRJONE_MEDIA_ATTR_S ½á¹¹ÌåÖĞ¶ÔÓ¦µÄiMediaId.
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_CreateIKeyFrame(int iMediaId);


/************************************************************************
 º¯ÊıÃû:RJONE_ShowOsd
 ¹¦ÄÜ: ÔÚÊÓÆµÍ¼ÏñÉÏÏÔÊ¾OSD
 ÊäÈë: 
 		id  OSD id ·¶Î§ÊÇ[0 - 7]
 		chan  ÊÓÆµÍ¨µÀ [0 - 1] 
 		screen_x  OSD ÏÔÊ¾µÄ×ø±ê×óÉÏ½Ç x Öµ
 		screen_y  OSD ÏÔÊ¾µÄ×ø±ê×óÉÏ½Ç y Ö
 		show_buf  OSDÉÏÒªÏÔÊ¾µÄÍ¼Æ¬Êı¾İµØÖ·£¬Í¼Æ¬¸ñÊ½ÎªARGB1555
 		show_pic_witdh  ÏÔÊ¾µÄÍ¼Æ¬ µÄ¿í¶È
 		show_pic_witdh  ÏÔÊ¾µÄÍ¼Æ¬ µÄ¸ß¶È
 		u32BgAlpha   ±³¾°ÑÕÉ«Í¸Ã÷¶È ·¶Î§ [0-128]    ÖµÔ½Ğ¡Ô½Í¸Ã÷
 		u32FgAlpha    Ç°¾°ÑÕÉ«Í¸Ã÷¶È ·¶Î§[0-128]  ÖµÔ½Ğ¡Ô½Í¸Ã÷
 Êä³ö:
 		ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_ShowOsd(int id,int chan,int screen_x,int screen_y,char * show_buf,int show_pic_witdh, int show_pic_height,unsigned int u32BgAlpha,unsigned int u32FgAlpha);


/************************************************************************
 º¯ÊıÃû:RJONE_GetMotionInfo
 ¹¦ÄÜ:  »ñÈ¡ÒÆ¶¯¼ì²âĞÅÏ¢
 ÊäÈë:
 		ÎŞ
 Êä³ö:
 		pBufSt  			·µ»ØÒÆ¶¯¼ì²â½á¹¹ÌåÖ¸Õë 		
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_GetMotionInfo( J_OUT MOTION_DETECT_BUF_S  * pBufSt);


/************************************************************************
 º¯ÊıÃû:RJONE_SetWorkFrequency
 ¹¦ÄÜ:   ÉèÖÃÉè±¸µÄ¹¤×÷ÆµÂÊ
 ÊäÈë:
 		enVideoNorm  ¹¤×÷ÆµÂÊ
 Êä³ö:
 		ÎŞ	
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS
 ËµÃ÷:
************************************************************************/
int RJONE_SetWorkFrequency( J_IN RJONE_VIDEO_NORM_E  enVideoNorm);


/************************************************************************
 º¯ÊıÃû:RJONE_CloseOsd
 ¹¦ÄÜ: ¹Ø±ÕÖ¸¶¨ OSD,ÈÃÆä²»ÏÔÊ¾
 ÊäÈë: 
 		id  OSD id ·¶Î§ÊÇ[0 - 7]
 Êä³ö:
 		ÎŞ
 ·µ»Ø:³É¹¦·µ»ØRJONE_SUCCESS 	     
 ËµÃ÷:
************************************************************************/
int RJONE_CloseOsd(int id);

#ifdef __cplusplus
}
#endif

#endif 

