/******************************************************************************
  A simple program of Hisilicon mpp audio input/output/encoder/decoder implementation.
  Copyright (C), 2010-2021, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2013-7 Created
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "sample_comm.h"
#include "acodec.h"
//#include "tlv320aic31.h"

#include "singly_linked_list.h"
#include "MacBaseData.h"

#include "gk_dev.h"
#include "G711_lib.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif


static PAYLOAD_TYPE_E gs_enPayloadType = PT_LPCM;

//static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAioReSample  = HI_FALSE;
static HI_BOOL gs_bUserGetMode  = HI_FALSE;
static HI_BOOL gs_bAoVolumeCtrl = HI_FALSE;
static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: talk*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 0;  


#define SAMPLE_DBG(s32Ret)\
do{\
    printf("s32Ret=%#x,fuc:%s,line:%d\n", s32Ret, __FUNCTION__, __LINE__);\
}while(0)

/******************************************************************************
* function : PT Number to String
******************************************************************************/
static char* SAMPLE_AUDIO_Pt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  
    {
        return "g711a";
    }
    else if (PT_G711U == enType)  
    {
        return "g711u";
    }
    else if (PT_ADPCMA == enType)  
    {
        return "adpcm";
    }
    else if (PT_G726 == enType) 
    {
        return "g726";
    }
    else if (PT_LPCM == enType)  
    {
        return "pcm";
    }
    else 
    {
        return "data";
    }
}

/******************************************************************************
* function : Open Aenc File
******************************************************************************/
static FILE * SAMPLE_AUDIO_OpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN, "audio_chn%d.%s", AeChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

/******************************************************************************
* function : Open Adec File
******************************************************************************/
static FILE *SAMPLE_AUDIO_OpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE* pfd;
    HI_CHAR aszFileName[FILE_NAME_LEN];

    /* create file for save stream*/
    snprintf(aszFileName, FILE_NAME_LEN ,"audio_chn%d.%s", AdChn, SAMPLE_AUDIO_Pt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("%s: open file %s failed\n", __FUNCTION__, aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}


/******************************************************************************
* function : file -> Adec -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AdecAo(HI_VOID)
{
    HI_S32      s32Ret;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AoChnCnt;
    FILE*        pfd = NULL;
    AIO_ATTR_S stAioAttr;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
#endif

	gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR3;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR3;
    }

	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR2;
    }

    s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR1;
    }

    pfd = SAMPLE_AUDIO_OpenAdecFile(AdChn, gs_enPayloadType);
    if (!pfd)
    {
        SAMPLE_DBG(HI_FAILURE);
        goto ADECAO_ERR0;
    }
    s32Ret = SAMPLE_COMM_AUDIO_CreatTrdFileAdec(AdChn, pfd);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto ADECAO_ERR0;
    }
    
    printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdFileAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        return HI_FAILURE;
    }

ADECAO_ERR0:
	s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
    
ADECAO_ERR1:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
ADECAO_ERR2:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
	
ADECAO_ERR3:
    return s32Ret;
}

/******************************************************************************
* function : Ai -> Aenc -> file
*                       -> Adec -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiAenc(HI_VOID)
{
    HI_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
	HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    HI_BOOL     bSendAdec = HI_TRUE;
    FILE        *pfd = NULL;
    AIO_ATTR_S stAioAttr;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
#endif
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;
		
    /********************************************
      step 1: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        goto AIAENC_ERR6;
    }

    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto AIAENC_ERR6;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = 1;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto AIAENC_ERR5;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
				for (j=0; j<i; j++)
				{
					SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, j);
				}
                goto AIAENC_ERR4;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
				for (j=0; j<i; j++)
				{
					SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, j, j);
				}
                goto AIAENC_ERR4;
            }
        }
        printf("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
    }

    /********************************************
      step 5: start Adec & Ao. ( if you want )
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAENC_ERR3;
        }

		s32AoChnCnt = stAioAttr.u32ChnCnt;
        s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL , 0);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAENC_ERR2;
        }

        pfd = SAMPLE_AUDIO_OpenAencFile(AdChn, gs_enPayloadType);
        if (!pfd)
        {
            SAMPLE_DBG(HI_FAILURE);
            goto AIAENC_ERR1;
        }
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencAdec(AeChn, AdChn, pfd);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAENC_ERR1;
        }

        s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAENC_ERR0;
        }

        printf("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);
     }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /********************************************
      step 6: exit the process
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
    	s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
		
AIAENC_ERR0:
		s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }        

AIAENC_ERR1:
		s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }        

AIAENC_ERR2:
        s32Ret |= SAMPLE_COMM_AUDIO_StopAdec(AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }
        
    }

AIAENC_ERR3:
    for (i = 0; i < s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret |= SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
            }
        }
        else
        {
            s32Ret |= SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_DBG(s32Ret);
            }
        }
    }

AIAENC_ERR4:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }

AIAENC_ERR5:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }

AIAENC_ERR6:

    return s32Ret;
}

/******************************************************************************
* function : Ai -> Ao(with fade in/out and volume adjust)
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiAo(HI_VOID)
{
    HI_S32 s32Ret;
	HI_S32 s32AiChnCnt;
	HI_S32 s32AoChnCnt;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn = 0;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    AIO_ATTR_S stAioAttr;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 1;
#else //  inner acodec
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
#endif
	gs_bAioReSample = HI_TRUE;

    /* config ao resample attr if needed */
    if (HI_TRUE == gs_bAioReSample)
    {
        stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_32000;
        stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM * 4;

        /* ai 32k -> 8k */
        enOutSampleRate = AUDIO_SAMPLE_RATE_8000;

        /* ao 8k -> 32k */
        enInSampleRate  = AUDIO_SAMPLE_RATE_8000;
    }
    else
    {
        enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    	enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
    }
	
    /* resample and anr should be user get mode */
    gs_bUserGetMode = (HI_TRUE == gs_bAioReSample) ? HI_TRUE : HI_FALSE; 
    
    /* config audio codec */
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        goto AIAO_ERR3;
    }
    
    /* enable AI channle */    
    s32AiChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto AIAO_ERR3;
    }

    /* enable AO channle */   
	s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
         goto AIAO_ERR2;
    }

    /* bind AI to AO channle */
    if (HI_TRUE == gs_bUserGetMode)
    {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
             goto AIAO_ERR1;
        }
    }
    else
    {   
        s32Ret = SAMPLE_COMM_AUDIO_AoBindAi(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAO_ERR1;
        }
    }
    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);

    if (HI_TRUE == gs_bAoVolumeCtrl)
    {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAoVolCtrl(AoDev);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            goto AIAO_ERR0;
        }
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();
    
	if(HI_TRUE == gs_bAoVolumeCtrl)
	{
		s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAoVolCtrl(AoDev);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
            return HI_FAILURE;
        }
	}
	
AIAO_ERR0:

    if (HI_TRUE == gs_bUserGetMode)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }
    }
    else
    {
        s32Ret = SAMPLE_COMM_AUDIO_AoUnbindAi(AiDev, AiChn, AoDev, AoChn);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }
    }
	
AIAO_ERR1:
    
    s32Ret |= SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
	
AIAO_ERR2:
	s32Ret |= SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
	
AIAO_ERR3:

    return s32Ret;
}


/******************************************************************************
* function : Ai -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiVqeProcessAo(HI_VOID)
{
	return 0;
	/*
    HI_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    HI_S32      s32AiChnCnt;
    HI_S32      s32AoChnCnt;
    AIO_ATTR_S  stAioAttr;
    AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
	HI_VOID     *pAiVqeAttr = NULL;
	AO_VQE_CONFIG_S stAoVqeAttr;
	HI_VOID     *pAoVqeAttr = NULL;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
#endif
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;

	if (1 == u32AiVqeType)
    {
        memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
	    stAiVqeTalkAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
	    stAiVqeTalkAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
	    stAiVqeTalkAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
	    stAiVqeTalkAttr.stAecCfg.bUsrMode    = HI_FALSE;
	    stAiVqeTalkAttr.stAecCfg.s8CngMode   = 0;
	    stAiVqeTalkAttr.stAgcCfg.bUsrMode    = HI_FALSE;
	    stAiVqeTalkAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAiVqeTalkAttr.stHpfCfg.bUsrMode    = HI_TRUE;
	    stAiVqeTalkAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;
		stAiVqeTalkAttr.stHdrCfg.bUsrMode    = HI_FALSE;

		stAiVqeTalkAttr.u32OpenMask = AI_TALKVQE_MASK_AEC | AI_TALKVQE_MASK_AGC | AI_TALKVQE_MASK_ANR | AI_TALKVQE_MASK_HPF;

		pAiVqeAttr = (HI_VOID *)&stAiVqeTalkAttr;
    }	
	else
	{
		pAiVqeAttr = HI_NULL;
	}

	if (1 == u32AoVqeType)
    {
        memset(&stAoVqeAttr, 0, sizeof(AO_VQE_CONFIG_S));
	    stAoVqeAttr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000;
	    stAoVqeAttr.s32FrameSample       = SAMPLE_AUDIO_PTNUMPERFRM;
	    stAoVqeAttr.enWorkstate          = VQE_WORKSTATE_COMMON;
	    stAoVqeAttr.stAgcCfg.bUsrMode    = HI_FALSE;
	    stAoVqeAttr.stAnrCfg.bUsrMode    = HI_FALSE;
		stAoVqeAttr.stHpfCfg.bUsrMode    = HI_TRUE;
	    stAoVqeAttr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150;

		stAoVqeAttr.u32OpenMask = AO_VQE_MASK_AGC | AO_VQE_MASK_ANR | AO_VQE_MASK_EQ | AO_VQE_MASK_HPF;

		pAoVqeAttr = (HI_VOID *)&stAoVqeAttr;
    }
    

    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_DBG(s32Ret);
        goto VQE_ERR2;
    }

    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, pAiVqeAttr, u32AiVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto VQE_ERR2;
    }
    s32AoChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample, pAoVqeAttr, u32AoVqeType);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
        goto VQE_ERR1;
    }

    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAo(AiDev, i, AoDev, i);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
			for (j=0; j<i; j++)
			{
				SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, j);
			}
            goto VQE_ERR0;
        }
		
		printf("bind ai(%d,%d) to ao(%d,%d) ok \n", AiDev, i, AoDev, i);
    }

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    for (i=0; i<s32AiChnCnt; i++)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, i);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_DBG(s32Ret);
        }
    }
	
VQE_ERR0:
	s32Ret = SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }
    
VQE_ERR1:
    s32Ret |= SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_TRUE);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_DBG(s32Ret);
    }

VQE_ERR2:

    return s32Ret;*/
}


HI_VOID SAMPLE_AUDIO_Usage(HI_CHAR* sPrgNm)
{
    printf("\n\n/Usage:%s <index>/\n", sPrgNm);
    printf("\tindex and its function list below\n");
    printf("\t0:  start AI to AO loop\n");
    printf("\t1:  send audio frame to AENC channel from AI, save them\n");
    printf("\t2:  read audio stream from file, decode and send AO\n");
    printf("\t3:  start AI(AEC/ANR/ALC process), then send to AO\n");
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
#ifndef __HuaweiLite__
void SAMPLE_AUDIO_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
    
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_AUDIO_DestoryAllTrd();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}
#endif

/******************************************************************************
* function : main
******************************************************************************/

#ifdef __HuaweiLite__
HI_S32 app_main(int argc, char* argv[])
#else
HI_S32 main_audio(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret = HI_SUCCESS;
    VB_CONF_S stVbConf;
	HI_U32 u32Index = 0;

    if (2 != argc)
    {
        SAMPLE_AUDIO_Usage(argv[0]);
        return HI_FAILURE;
    }

    if (!strncmp(argv[1], "-h", 2))
	{
		SAMPLE_AUDIO_Usage(argv[0]);
    	return HI_SUCCESS;
	}

    u32Index = atoi(argv[1]);

    if (u32Index > 3)
    {
        SAMPLE_AUDIO_Usage(argv[0]);
        return HI_FAILURE;
    }

#ifndef __HuaweiLite__    
    signal(SIGINT, SAMPLE_AUDIO_HandleSig);
    signal(SIGTERM, SAMPLE_AUDIO_HandleSig);
#endif

    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: system init failed with %d!\n", __FUNCTION__, s32Ret);
        return HI_FAILURE;
    }
    
    switch (u32Index)
    {
        case 0:
        {
            s32Ret = SAMPLE_AUDIO_AiAo();
            break;
        }
        case 1:
        {
            s32Ret = SAMPLE_AUDIO_AiAenc();
            break;
        }
        case 2:
        {
            s32Ret = SAMPLE_AUDIO_AdecAo();
            break;
        }
        case 3:
        {
            s32Ret = SAMPLE_AUDIO_AiVqeProcessAo();
            break;
        }
        default:
        {
            break;
        }
    }
    
    SAMPLE_COMM_SYS_Exit();

	if (HI_SUCCESS == s32Ret)
    {
        SAMPLE_PRT("program exit normally!\n");
    }
    else
    {
        SAMPLE_PRT("program exit abnormally!\n");
    }

    return s32Ret;
}

//extern SINGLY_LINKED_LIST_INFO_ST * p_gVideolist[2];
extern SINGLY_LINKED_LIST_INFO_ST * p_gAudiolist;

int Hisi_ReadAudioBuf(unsigned char *buf,int * count,int chan);

void hisi_get_audio_encode_thread(void * value)
{	
	 //prctl(PR_SET_NAME, "hisi_get_audio_encode_thread");	
	int s32Ret;
	unsigned char  audio_buf[4096];
	int data_len = 0;
	int chn = 0;	


	DPRINTK(" pid = %d\n",getpid());
	


	while(1 )
	{
		s32Ret = Hisi_ReadAudioBuf(audio_buf,&data_len,chn);
		if( s32Ret <  0 )
		{			
			DPRINTK("Hisi_ReadAudioBuf [%d] error!\n",chn);
			sleep(1);
			continue;
		}

		if( data_len > 0 )
		{
			
		      unsigned char * new_buf_ptr = NULL;
                    unsigned char * ptr_point = NULL;		
			BUF_FRAMEHEADER * pheader = NULL;
			int i = 0;
			struct timeval get_time;
			gettimeofday( &get_time, NULL );
			//rtsp 传送g711a 的音频
			char buffAudioFrame[10000];
			int  audio_enc_len = 0;
			audio_enc_len = G711_EnCode(buffAudioFrame,audio_buf,data_len,G711_ALAW);
			
			new_buf_ptr = (unsigned char *)malloc(audio_enc_len + GK_VIDEO_DATA_OFFSET);
			if( new_buf_ptr )
			{				
				ptr_point = new_buf_ptr + GK_VIDEO_DATA_OFFSET;
				pheader = new_buf_ptr;

				pheader->index = i;
				pheader->type = AUDIO_FRAME; //#define VIDEO_FRAME 1
				pheader->iLength = data_len;
				pheader->ulTimeSec = get_time.tv_sec;
				pheader->ulTimeUsec = get_time.tv_usec;				
				pheader->iFrameType = 0;

				memcpy(ptr_point,buffAudioFrame,audio_enc_len);

				 if( p_gAudiolist != NULL)
				  {						
				  	//DPRINTK("%x %x %x %x %x %x\n",audio_buf[0],audio_buf[1],audio_buf[2],audio_buf[3],audio_buf[4],audio_buf[5]);

				
					HisiInsertVideoData(p_gAudiolist,(char*)new_buf_ptr,data_len + GK_VIDEO_DATA_OFFSET,i);
				  }else
				  {
				  	free(new_buf_ptr);
				  }
				
			}else
			{
				DPRINTK("[%d] malloc %d errer 112\n",i,data_len);
			}
		}

		if( data_len > 0 )
			CallAudioRecvBack(audio_buf,data_len);
	}

	pthread_detach(pthread_self());

	DPRINTK("out \n");


	return ;
	
}



int Hisi_InitAudio()
{
	AIO_ATTR_S stAioAttr;
  	HI_S32 s32Ret, s32AiChnCnt;
	AUDIO_DEV   AiDev = SAMPLE_AUDIO_AO_DEV;
	AI_CHN      AiChn = 0;
	AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
	AO_CHN      AoChn = 0;
	 HI_S32      s32AencChnCnt = 1;
	 int AeChn = 0;
	 ADEC_CHN    AdChn = 0;
	 int gs_bMicIn = HI_FALSE;

	
	stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	//stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
	stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag = 1;
	stAioAttr.u32FrmNum = 30;
	stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
	stAioAttr.u32ChnCnt = 2;
	stAioAttr.u32ClkSel = 0;

	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;



	
	/********************************************
	   step 1: config audio codec
	 ********************************************/
	 s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
	 if (HI_SUCCESS != s32Ret)
	 {
		 DPRINTK("ret=%x\n",s32Ret);
		 return HI_FAILURE;
	 }


	/********************************************
       step 2: start Ai
	********************************************/
	s32AiChnCnt = stAioAttr.u32ChnCnt; 	
	s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr,AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, NULL,0);
	if (s32Ret != HI_SUCCESS)
	{
	     DPRINTK("ret=%x\n",s32Ret);
	    return HI_FAILURE;
	}



	/********************************************
      	step 3: start Aenc
    	********************************************/
	s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, stAioAttr.u32PtNumPerFrm,gs_enPayloadType);
	if (s32Ret != HI_SUCCESS)
	{
	    DPRINTK("ret=%x\n",s32Ret);
	    return HI_FAILURE;
	}	

	#ifdef hi3520D
	for( AeChn = 0 ; AeChn < s32AencChnCnt; AeChn++)
	{
		AiChn = AeChn;
		 s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
	       if (s32Ret != HI_SUCCESS)
	       {
	          DPRINTK("ret=%x\n",s32Ret);
	           return HI_FAILURE;
	       }
	}
	
	#else
	 s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
       if (s32Ret != HI_SUCCESS)
       {
          DPRINTK("ret=%x\n",s32Ret);
           return HI_FAILURE;
       }
	 #endif

	  s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
        if (s32Ret != HI_SUCCESS)
        {
            DPRINTK("ret=%x\n",s32Ret);
            return HI_FAILURE;
        }

        s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, AoChn,  &stAioAttr,AUDIO_SAMPLE_RATE_BUTT, HI_FALSE, NULL , 0);
        if (s32Ret != HI_SUCCESS)
        {
            DPRINTK("ret=%x\n",s32Ret);
            return HI_FAILURE;
        }


	 s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            DPRINTK("ret=%x\n",s32Ret);
            return HI_FAILURE;
        }


	//创建音频读取线程
	{
		pthread_t id_audio_encode;
		s32Ret = pthread_create(&id_audio_encode,NULL,(void*)hisi_get_audio_encode_thread,(void *)NULL);
		if ( 0 != s32Ret )
		{
			DPRINTK( "create hisi_get_audio_encode_thread thread error\n");
			return -1;
		}
	}
	return 1;

}


int Hisi_ReadAudioBuf(unsigned char *buf,int * count,int chan)
{
	
    HI_S32 s32ret, i, max, u32ChnCnt;
    HI_S32 AencFd[32];
    AENC_CHN AeChn;
    AUDIO_STREAM_S stStream; 
    fd_set read_fds;
    struct timeval TimeoutVal;
	struct timeval recTime;
	int rec_audio_cam = 0;	


    u32ChnCnt = 1;  
    
    FD_ZERO(&read_fds);
    for (i=0; i<u32ChnCnt; i++)
    {
        AencFd[i] = HI_MPI_AENC_GetFd(i);
		FD_SET(AencFd[i],&read_fds);
		if(max <= AencFd[i])    max = AencFd[i];
    }
    
       
    TimeoutVal.tv_sec = 1;
    TimeoutVal.tv_usec = 0;    
   
     
    s32ret = select(max+1, &read_fds, NULL, NULL, &TimeoutVal);
	if (s32ret < 0) 
	{
       DPRINTK("get aenc stream select error\n");
	}
	else if (0 == s32ret) 
	{
		//DPRINTK("get aenc stream select time out\n");
        
	}else
	{
     
     for (i=0; i<u32ChnCnt; i++)
     {
         AeChn = i;	
		 
         if (FD_ISSET(AencFd[AeChn], &read_fds))
         {
             /* get stream from aenc chn */
             s32ret = HI_MPI_AENC_GetStream(AeChn, &stStream, HI_FALSE);
          	//s32ret = HI_MPI_AENC_GetStream(AeChn, &stStream, HI_IO_BLOCK);
             if (HI_SUCCESS != s32ret )
             {
                 printf("HI_MPI_AENC_GetStream chn:%d, fail\n",AeChn);
                 return -1;
             }

		//DPRINTK("[%d] %d\n",AeChn,stStream.u32Len);
			 
		 memcpy(buf,stStream.pStream,stStream.u32Len);

		*count = stStream.u32Len;

		//audio_frame_rate_show();            
            
             s32ret = HI_MPI_AENC_ReleaseStream(AeChn, &stStream);
			 if (HI_SUCCESS != s32ret )
             {
                 printf("HI_MPI_AENC_ReleaseStream chn:%d, fail\n",AeChn);
                 return -1;
             }	
		
			 
         }            
     	}
	}
  
	
	return 1;
}


int Hisi_WriteAudioBuf(unsigned char *buf,int count,int chan)
{
	AUDIO_STREAM_S stAudioStream;
	HI_S32 s32AdecChn =0;
	HI_S32 s32ret;
	static char tmp_buf[640];
	static int tmp_buf_length = 0;

	DPRINTK("%d\n",count);

	if( count < 1024 )
	{
		stAudioStream.pStream = buf;
		stAudioStream.u32Len = count;	
		
		s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
		if (s32ret)
		{
		   DPRINTK("HI_MPI_ADEC_SendStream chn:%d err:0x%x\n",s32AdecChn,s32ret);        
		}
	}else
	{
		int start_pos = 0;
		int i = 0;	
		
		while( i < count )
		{
			if( i == 0  && tmp_buf_length != 0 )
			{
				memcpy(tmp_buf+tmp_buf_length,buf + start_pos,640 -tmp_buf_length);
				stAudioStream.pStream = tmp_buf;
				stAudioStream.u32Len = 640;

				s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
				if (s32ret)
				{
				   DPRINTK("HI_MPI_ADEC_SendStream chn:%d err:0x%x\n",s32AdecChn,s32ret);        
				}

				start_pos += 640 -tmp_buf_length;
				i += 640 -tmp_buf_length;				
				tmp_buf_length = 0;

				DPRINTK("head start_pos = %d i=%d\n",start_pos,i);
			}else
			{
		
				if( i + 640  < count )
				{
					stAudioStream.pStream = buf + start_pos;
					stAudioStream.u32Len = 640;
				}else
				{					
					memcpy(tmp_buf,buf + start_pos,count - i);
					tmp_buf_length = count - i;

					DPRINTK("tail tmp_buf_length = %d \n",tmp_buf_length);
					break;
				}
				
				s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_TRUE);
				if (s32ret)
				{
				   DPRINTK("HI_MPI_ADEC_SendStream chn:%d err:0x%x\n",s32AdecChn,s32ret);        
				}

				start_pos += 640;
				i += 640;

				DPRINTK("start_pos = %d i=%d\n",start_pos,i);
			}

			usleep(10);			
		}
		
	}
	
	
	return count;
}


