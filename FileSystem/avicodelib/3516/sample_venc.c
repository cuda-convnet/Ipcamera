/******************************************************************************
  A simple program of Hisilicon Hi35xx video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_comm_vpss.h"
#include "mpi_vpss.h"
#include "mpi_vgs.h"
#include "get_yuv.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

#include "sample_comm.h"

#include "gk_dev.h"

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char* sPrgNm)
{
#ifndef hi3516ev100
    printf("Usage : %s <index>  eg: %s 0 \n", sPrgNm, sPrgNm);
    printf("index:\n");
    printf("\t 0) NormalP:H.265@1080p@30fps+H.264@1080p@30fps\n");
    printf("\t 1) SmartP: H.265@1080p@30fps+H.264@1080p@30fps\n");
    printf("\t 2) DualP:  H.265@1080p@30fps@BigSmallP+H.264@1080p@30fps@BigSmallP\n");
	printf("\t 3) DualP2: H.265@1080p@30fps@SmallP+H.264@1080p@30fps@BigSmallP\n");
	printf("\t 4) IntraRefresh: H.265@1080p@30fps@IntraRefresh+H.264@1080p@30fps@IntraRefresh\n");
    printf("\t 5) Jpege:1*1080p mjpeg encode + 1*1080p jpeg\n");
	printf("\t 6) OnlineLowDelay:H.264@1080p@30fps@OnlineLowDelay + H.265@1080p@30fps\n");
	printf("\t 7) RoiBgFrameRate:H.264@1080p@30fps@RoiBgFrameRate + H.265@1080p@30fps@RoiBgFrameRate\n");
	printf("\t 8) svc-t :H.264@1080p@30fps@svc-t\n");
	printf("\t 9) EPTZ:H.264@1080p@30fps + H.265@1080p@30fps@EPTZ\n");
#else
    printf("Usage : %s <index>  eg: %s 0 \n", sPrgNm, sPrgNm);
    printf("index:\n");
    printf("\t 0) NormalP:H.265@1080p@30fps\n");
    printf("\t 1) SmartP: H.265@1080p@30fps\n");
    printf("\t 2) DualP:  H.265@1080p@30fps@BigSmallP\n");
	printf("\t 3) DualP2: H.265@1080p@30fps@SmallP\n");
	printf("\t 4) IntraRefresh: H.265@1080p@30fps@IntraRefresh\n");
    printf("\t 5) Jpege:1*1080p mjpeg encode + 1*1080p jpeg\n");
	printf("\t 6) OnlineLowDelay:H.264@1080p@30fps@OnlineLowDelay\n");
	printf("\t 7) RoiBgFrameRate:H.264@1080p@30fps@RoiBgFrameRate\n");
	printf("\t 8) svc-t :H.264@1080p@30fps@svc-t\n");
#endif



    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
#ifndef __HuaweiLite__
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VENC_StopGetStream();
		////SAMPLE_COMM_VPSS_StopChnCropProc();
        SAMPLE_COMM_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}
#endif

/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

/******************************************************************************
* function :  H.264@1080p@30fps+H.264@VGA@30fps


******************************************************************************/
HI_S32 SAMPLE_VENC_NORMALP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;

#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;
	//printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        #ifndef hi3516ev100
	    stVbConf.astCommPool[0].u32BlkCnt = 10;
        #else
        stVbConf.astCommPool[0].u32BlkCnt = 5;
        #endif
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[2].u32BlkCnt = 10;
	}

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
	   // stVpssGrpAttr.bSharpenEn = HI_FALSE;

	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }

	    VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 2)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 3)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

	    VpssChn = 2;
	    stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble		= HI_FALSE;
	    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width		= 720;
	    stVpssChnMode.u32Height 	= (VIDEO_ENCODING_MODE_PAL==gs_enNorm)?576:480;;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;

	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
	                                   gs_enNorm, enSize[2], enRcMode,u32Profile,ROTATE_NONE);

	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}



/******************************************************************************
* function :  Gop_Mode_SmartP for H.265@1080P@normalP +H.265@1080P@smartP


******************************************************************************/
HI_S32 SAMPLE_VENC_SMARTP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_SMARTP,VENC_GOPMODE_SMARTP};
	HI_U32 u32Profile = 0;

	VB_CONF_S stVbConf;
	SAMPLE_VI_CONFIG_S stViConfig = {0};

	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
	VPSS_CHN_MODE_S stVpssChnMode;

	VENC_CHN VencChn;
	SAMPLE_RC_E enRcMode= SAMPLE_RC_VBR;

#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32BlkSize;
	SIZE_S stSize;
	char c;


	/******************************************
	 step  1: init sys variable
	******************************************/
	memset(&stVbConf,0,sizeof(VB_CONF_S));

	SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

	stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/
	if(s32ChnNum >= 1)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    #ifndef hi3516ev100
        stVbConf.astCommPool[0].u32BlkCnt = 10;
    #else
        stVbConf.astCommPool[0].u32BlkCnt = 4;
    #endif
	}
	if(s32ChnNum >= 2)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[2].u32BlkCnt = 10;
	}

	/******************************************
	 step 2: mpp system init.
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		goto END_VENC_1080P_CLASSIC_0;
	}

	/******************************************
	 step 3: start vi dev & chn to capture
	******************************************/
	stViConfig.enViMode   = SENSOR_TYPE;
	stViConfig.enRotate   = ROTATE_NONE;
	stViConfig.enNorm	  = VIDEO_ENCODING_MODE_AUTO;
	stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
	stViConfig.enWDRMode  = WDR_MODE_NONE;
	s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}

	/******************************************
	 step 4: start vpss and vi bind vpss
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}

	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
		stVpssGrpAttr.u32MaxW = stSize.u32Width;
		stVpssGrpAttr.u32MaxH = stSize.u32Height;
		stVpssGrpAttr.bIeEn = HI_FALSE;
		stVpssGrpAttr.bNrEn = HI_TRUE;
		stVpssGrpAttr.bHistEn = HI_FALSE;
		stVpssGrpAttr.bDciEn = HI_FALSE;
		stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
        //stVpssGrpAttr.bSharpenEn = HI_FALSE;

		s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_2;
		}

		s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Vi bind Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_3;
		}

		VpssChn = 0;
		stVpssChnMode.enChnMode 	 = VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		 = HI_FALSE;
		stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		 = stSize.u32Width;
		stVpssChnMode.u32Height 	 = stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
		memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	if(s32ChnNum >= 2)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		VpssChn = 1;
		stVpssChnMode.enChnMode 	  = VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		  = HI_FALSE;
		stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		  = stSize.u32Width;
		stVpssChnMode.u32Height 	  = stSize.u32Height;
		stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	if(s32ChnNum >= 3)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		VpssChn = 2;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		= stSize.u32Width;   //720;
		stVpssChnMode.u32Height 	= stSize.u32Height;  //(VIDEO_ENCODING_MODE_PAL==gs_enNorm)?576:480;;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;

		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	/******************************************
	 step 5: start stream venc
	******************************************/
	/*** HD1080P **/
	/*printf("\t c) cbr.\n");
	printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
	printf("\t f) fixQp\n");
	printf("please input choose rc mode!\n");
	c = (char)getchar();
	switch(c)
	{
		case 'c':
			enRcMode = SAMPLE_RC_CBR;
			break;
		case 'v':
			enRcMode = SAMPLE_RC_VBR;
			break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
		case 'f':
			enRcMode = SAMPLE_RC_FIXQP;
			break;
		default:
			printf("rc mode! is invaild!\n");
			goto END_VENC_1080P_CLASSIC_4;
	}*/

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
		VpssChn = 0;
	    VencChn = 0;

		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;

		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;

		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[2],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
	                                   gs_enNorm, enSize[2], enRcMode,u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}

/******************************************
 step 6: stream venc process -- get stream, then save it to file.
  ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
		if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		goto END_VENC_1080P_CLASSIC_5;
	}

	printf("please press twice ENTER to exit this sample\n");
		getchar();
	getchar();

	/******************************************
	 step 7: exit process
	******************************************/

	SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:
	VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			VencChn = 2;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
			VencChn = 1;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
			VencChn = 0;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
			break;
	}
			SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


/******************************************************************************
* function :   function DUALP :  H.265@1080@30fps+H.264@1080@30fps

******************************************************************************/
HI_S32 SAMPLE_VENC_DUALP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265,PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_DUALP,VENC_GOPMODE_DUALP};
	HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;

#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum = %d\n",s32ChnNum);
    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    #ifndef hi3516ev100
        stVbConf.astCommPool[0].u32BlkCnt = 10;
    #else
        stVbConf.astCommPool[0].u32BlkCnt = 4;
    #endif
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[2].u32BlkCnt = 10;
	}

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
        //stVpssGrpAttr.bSharpenEn = HI_FALSE;

	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }

	    VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 2)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 3)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

	    VpssChn = 2;
	    stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble		= HI_FALSE;
	    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width		= stSize.u32Width;  //720;
	    stVpssChnMode.u32Height 	= stSize.u32Height; //(VIDEO_ENCODING_MODE_PAL==gs_enNorm)?576:480;;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;

	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
		
		SAMPLE_PRT("Get GopAttr failedSAMPLE_COMM_VENC_StartSAMPLE_COMM_VENC_StartSAMPLE_COMM_VENC_StartSAMPLE_COMM_VENC_Start!\n");

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[2],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
	                                   gs_enNorm, enSize[2], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


/******************************************************************************
* function :   function DUALP :  H.265@1080@30fps+H.264@1080@30fps

******************************************************************************/
HI_S32 SAMPLE_VENC_DUALPP_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265,PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_DUALP,VENC_GOPMODE_DUALP};
	HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;

#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;
//	printf("s32ChnNum = %d\n",s32ChnNum);
    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    #ifndef hi3516ev100
        stVbConf.astCommPool[0].u32BlkCnt = 10;
    #else
        stVbConf.astCommPool[0].u32BlkCnt = 4;
    #endif
    }
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[2].u32BlkCnt = 10;
	}

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
        //stVpssGrpAttr.bSharpenEn = HI_FALSE;

	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }

	    VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 2)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 3)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

	    VpssChn = 2;
	    stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble		= HI_FALSE;
	    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width		= stSize.u32Width;  //720;
	    stVpssChnMode.u32Height 	= stSize.u32Height; //(VIDEO_ENCODING_MODE_PAL==gs_enNorm)?576:480;;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;

	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
		stGopAttr.stDualP.u32SPInterval = 0;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
		stGopAttr.stDualP.u32SPInterval = 0;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[2],&stGopAttr,gs_enNorm);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Get GopAttr failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
	                                   gs_enNorm, enSize[2], enRcMode,u32Profile,&stGopAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}




/******************************************************************************
* function :  H264 P refurbish I macro block/I slice,normal@1080+I macro block@1080+Islice @1080
******************************************************************************/
HI_S32 SAMPLE_VENC_IntraRefresh_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	VENC_PARAM_INTRA_REFRESH_S stIntraRefresh;
#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    stVbConf.u32MaxPoolCnt = 128;
    //printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    #ifndef hi3516ev100
        stVbConf.astCommPool[0].u32BlkCnt = 10;
    #else
        stVbConf.astCommPool[0].u32BlkCnt = 4;
    #endif
    }
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =10;
	}


    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
        //stVpssGrpAttr.bSharpenEn = HI_FALSE;

	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }

	    VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
    if(s32ChnNum >= 2)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

		s32Ret = HI_MPI_VENC_GetIntraRefresh(VencChn,&stIntraRefresh);
		stIntraRefresh.bISliceEnable  = HI_TRUE;
		stIntraRefresh.bRefreshEnable = HI_TRUE;
		stIntraRefresh.u32RefreshLineNum = (stSize.u32Height + 63)>>8;
		stIntraRefresh.u32ReqIQp = 30;

		s32Ret = HI_MPI_VENC_SetIntraRefresh(VencChn,&stIntraRefresh);
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
	                                   gs_enNorm, enSize[1], enRcMode,u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = HI_MPI_VENC_GetIntraRefresh(VencChn,&stIntraRefresh);
		stIntraRefresh.bISliceEnable = HI_TRUE;
		stIntraRefresh.bRefreshEnable = HI_TRUE;
		stIntraRefresh.u32RefreshLineNum = (stSize.u32Height + 15)>>6;
		stIntraRefresh.u32ReqIQp = 30;
		s32Ret = HI_MPI_VENC_SetIntraRefresh(VencChn,&stIntraRefresh);

	}

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  1 chn 1080p MJPEG encode and 1 chn 1080p JPEG snap
******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_MJPEG_JPEG(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_MJPEG;
    PIC_SIZE_E enSize[2] = {PIC_HD1080,PIC_HD1080};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    HI_S32 i = 0;
    char ch;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[1]);

    /*video buffer*/
    {
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    #ifndef hi3516ev100
        stVbConf.astCommPool[0].u32BlkCnt = 10;
    #else
        stVbConf.astCommPool[0].u32BlkCnt = 5;
    #endif
    }

#ifndef hi3516ev100
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =5;
	}
#endif


    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_MJPEG_JPEG_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    //stVpssGrpAttr.bSharpenEn = HI_FALSE;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_3;
    }


    VpssChn = 0;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

#ifndef hi3516ev100
    VpssChn = 1;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
#endif

    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad, \
                                    gs_enNorm, enSize[0], enRcMode, u32Profile,ROTATE_NONE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }


	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_MJPEG_JPEG_5;
	}

    VpssGrp = 0;
#ifndef hi3516ev100
    VpssChn = 1;
#else
    VpssChn = 0;
#endif
    VencChn = 1;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize,1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }


    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    printf("press 'q' to exit sample!\npress ENTER to capture one picture to file\n");
    i = 0;
    while ((ch = (char)getchar()) != 'q')
    {
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn,1,1);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: sanp process failed!\n", __FUNCTION__);
            break;
        }
        printf("snap %d success!\n", i);
        i++;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_MJPEG_JPEG_5:
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

#ifndef hi3516ev100
    VpssChn = 1;
#else
    VpssChn = 0;
#endif
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
END_VENC_MJPEG_JPEG_4:    //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_MJPEG_JPEG_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_MJPEG_JPEG_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_MJPEG_JPEG_1:    //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_MJPEG_JPEG_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  SAMPLE_VENC_LOW_DELAY
H.264@NormalP@1080p@30fps@OnlineLowDelay + H.265@NormalP@1080p@30fps@OnlineLowDelay

******************************************************************************/
HI_S32 SAMPLE_VENC_LOW_DELAY(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2] = {PT_H264, PT_H265};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};
    //HI_U32 u32Priority;

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;
    VPSS_LOW_DELAY_INFO_S stLowDelayInfo;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    printf("u32BlkSize: %d\n", u32BlkSize);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
#ifndef hi3516ev100
    stVbConf.astCommPool[0].u32BlkCnt = 10;
#else
    stVbConf.astCommPool[0].u32BlkCnt = 4;
#endif

#ifndef hi3516ev100
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 10;
#endif
    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_LOW_DELAY_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_LOW_DELAY_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_LOW_DELAY_1;
    }


    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_FALSE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
        //stVpssGrpAttr.bSharpenEn = HI_FALSE;
        s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Vpss failed!\n");
            goto END_VENC_LOW_DELAY_2;
        }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_LOW_DELAY_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_LOW_DELAY_4;
    }

#ifndef hi3516ev100
    VpssChn = 1;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_LOW_DELAY_4;
    }
#endif
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_LOW_DELAY_4;
    }

        VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;


    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0], \
                                gs_enNorm, enSize[0], enRcMode, u32Profile,ROTATE_NONE);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

#if 0
    /*set chnl Priority*/
    s32Ret = HI_MPI_VENC_GetChnlPriority(VencChn, &u32Priority);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Get Chnl Priority failed!\n");
        goto END_VENC_LOW_DELAY_5;
    }

    u32Priority = 1;

    s32Ret = HI_MPI_VENC_SetChnlPriority(VencChn, u32Priority);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Set Chnl Priority failed!\n");
        goto END_VENC_LOW_DELAY_5;
    }
#endif

        /*set low delay,only one which low delay channel was enabled*/
#if 1
        s32Ret = HI_MPI_VPSS_GetLowDelayAttr(VpssGrp, VpssChn, &stLowDelayInfo);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VPSS_GetLowDelayAttr failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }
        stLowDelayInfo.bEnable = HI_TRUE;
        stLowDelayInfo.u32LineCnt = stVpssChnMode.u32Height / 2;
        s32Ret = HI_MPI_VPSS_SetLowDelayAttr(VpssGrp, VpssChn, &stLowDelayInfo);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VPSS_SetLowDelayAttr failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }
#endif
        /*** 1080p **/

#ifndef hi3516ev100
            VpssChn = 1;
    VencChn = 1;

    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
                                    gs_enNorm, enSize[1], enRcMode, u32Profile,ROTATE_NONE);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
    }
#endif
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_LOW_DELAY_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_LOW_DELAY_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

#ifndef hi3516ev100
    VpssChn = 1;
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
#endif

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_LOW_DELAY_4:   //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
#ifndef hi3516ev100
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
#endif
END_VENC_LOW_DELAY_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_LOW_DELAY_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_LOW_DELAY_1:   //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_LOW_DELAY_0:   //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}



HI_S32 SAMPLE_VENC_ROIBG_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2] = {PT_H264,PT_H265};
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD1080, PIC_D1};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;
    VENC_ROI_CFG_S  stVencRoiCfg;
    VENC_ROIBG_FRAME_RATE_S stRoiBgFrameRate;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
#ifndef hi3516ev100
    stVbConf.astCommPool[0].u32BlkCnt = 10;
#else
    stVbConf.astCommPool[0].u32BlkCnt = 5;
#endif

#ifndef hi3516ev100
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 10;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt = 0;
#endif

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
	if(s32ChnNum >= 1)
	{
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
	    //stVpssGrpAttr.bSharpenEn = HI_FALSE;
	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Vi bind Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_3;
	    }

	    VpssChn = 0;
	    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble       = HI_FALSE;
	    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width      = stSize.u32Width;
	    stVpssChnMode.u32Height     = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	if(s32ChnNum >= 2)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		VpssChn = 1;
	    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble       = HI_FALSE;
	    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width      = stSize.u32Width;
	    stVpssChnMode.u32Height     = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }
	if(s32ChnNum >= 1)
	{
	    VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0], \
	                                    gs_enNorm, enSize[0], enRcMode, u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    stVencRoiCfg.bAbsQp   = HI_TRUE;
	    stVencRoiCfg.bEnable  = HI_TRUE;
	    stVencRoiCfg.s32Qp    = 30;
	    stVencRoiCfg.u32Index = 0;
	    stVencRoiCfg.stRect.s32X = 64;
	    stVencRoiCfg.stRect.s32Y = 64;
	    stVencRoiCfg.stRect.u32Height = 256;
	    stVencRoiCfg.stRect.u32Width = 256;
	    s32Ret = HI_MPI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = HI_MPI_VENC_GetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_GetRoiBgFrameRate failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    stRoiBgFrameRate.s32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;
	    stRoiBgFrameRate.s32DstFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 5 : 15;

	    s32Ret = HI_MPI_VENC_SetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_SetRoiBgFrameRate!\n");
	        goto END_VENC_1080P_CLASSIC_5;
		}
	}
	if(s32ChnNum >= 2)
	{
		VpssGrp = 0;
	    VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                    gs_enNorm, enSize[1], enRcMode, u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    stVencRoiCfg.bAbsQp   = HI_TRUE;
	    stVencRoiCfg.bEnable  = HI_TRUE;
	    stVencRoiCfg.s32Qp    = 30;
	    stVencRoiCfg.u32Index = 0;
	    stVencRoiCfg.stRect.s32X = 64;
	    stVencRoiCfg.stRect.s32Y = 64;
	    stVencRoiCfg.stRect.u32Height = 256;
	    stVencRoiCfg.stRect.u32Width = 256;
	    s32Ret = HI_MPI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    s32Ret = HI_MPI_VENC_GetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_GetRoiBgFrameRate failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	    stRoiBgFrameRate.s32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;
	    stRoiBgFrameRate.s32DstFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 5 : 15;
	    s32Ret = HI_MPI_VENC_SetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("HI_MPI_VENC_SetRoiBgFrameRate!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);



    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

HI_S32 SAMPLE_VENC_SVC_H264(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD720, PIC_D1};
    HI_U32 u32Profile = 3;/* Svc-t */

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;


    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
#ifndef hi3516ev100
    stVbConf.astCommPool[0].u32BlkCnt = 10;
#else
    stVbConf.astCommPool[0].u32BlkCnt = 4;
#endif

#ifndef hi3516ev100
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 6;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt = 6;
#endif


    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm	  = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    //stVpssGrpAttr.bSharpenEn = HI_FALSE;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble		= HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width		= stSize.u32Width;
    stVpssChnMode.u32Height 	= stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_1080P_CLASSIC_4;
    }
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;

    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad, \
                                    gs_enNorm, enSize[0], enRcMode, u32Profile,ROTATE_NONE);

    printf("SAMPLE_COMM_VENC_Start is ok\n");

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);

    printf("SAMPLE_COMM_VENC_BindVpss is ok\n");


    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream_Svc_t(s32ChnNum);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

    printf("SAMPLE_COMM_VENC_StopGetStream is ok\n");
END_VENC_1080P_CLASSIC_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);



    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_1080P_CLASSIC_3:	 //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:	 //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}


/******************************************************************************
* function :  SAMPLE_VENC_EPTZ
H.264@NormalP@1080p@30fps + H.265@NormalP@1080p@30fps@EPTZ

******************************************************************************/
HI_S32 SAMPLE_VENC_EPTZ(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2] = {PT_H264, PT_H265};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD720};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};
    //HI_U32 u32Priority;

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
	HI_U32 u32SrcWidth,u32SrcHeigth,u32DstWidth,u32DstHeigth;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 2;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    printf("u32BlkSize: %d\n", u32BlkSize);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 10;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, \
                 enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 10;

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_EPTZ_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_EPTZ_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_EPTZ_1;
    }


    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_FALSE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    //stVpssGrpAttr.bSharpenEn = HI_FALSE;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_EPTZ_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_EPTZ_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_EPTZ_4;
    }

    VpssChn = 1;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = stSize.u32Width;
    stVpssChnMode.u32Height     = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_EPTZ_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
	printf("\t a) avbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
			break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        default:
            printf("rc mode! is invaild!\n");
            goto END_VENC_EPTZ_4;
    }

        VpssGrp = 0;
        VpssChn = 0;
        VencChn = 0;

        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0], \
                                        gs_enNorm, enSize[0], enRcMode, u32Profile,ROTATE_NONE);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_EPTZ_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_EPTZ_5;
        }


        /*** 1080p **/
        VpssChn = 1;
        VencChn = 1;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
                                        gs_enNorm, enSize[1], enRcMode, u32Profile,ROTATE_NONE);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_EPTZ_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_EPTZ_5;
        }


   /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_EPTZ_5;
    }

	/*set vpss chnl crop proc*/
	/*VpssGrp = 0;
	VpssChn = 1;
	u32SrcWidth = stVpssChnMode.u32Width;
	u32SrcHeigth = stVpssChnMode.u32Height;
	u32DstWidth = 1280;
	u32DstHeigth = 720;
	s32Ret = SAMPLE_COMM_VPSS_SetChnCropProc(VpssGrp,VpssChn,u32SrcWidth,u32SrcHeigth,u32DstWidth,u32DstHeigth);
	if (HI_SUCCESS != s32Ret)
	{
	 SAMPLE_PRT("Start Venc failed!\n");
	 goto END_VENC_EPTZ_5;
	}*/

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
	//////SAMPLE_COMM_VPSS_StopChnCropProc();

END_VENC_EPTZ_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    VpssChn = 1;
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_EPTZ_4:   //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_EPTZ_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_EPTZ_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_EPTZ_1:   //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_EPTZ_0:   //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/

int app_main(int argc, char *argv[])
{
    HI_S32 s32Ret;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);
#endif

    switch (*argv[1])
    {
        case '0':/* H.264@NormalP@1080p@30fps+H.265NormalP@1080p@30fps*/
            s32Ret = SAMPLE_VENC_NORMALP_CLASSIC();
            break;

		case '1':/*H.264@DualP@1080p@30fps + H.265@SmartP@1080p@30fps*/
    		s32Ret = SAMPLE_VENC_SMARTP_CLASSIC();
            break;

		case '2':/*H.264@SmartP@1080p@30fps + H.265@DualP@1080p@30fps*/
			s32Ret = SAMPLE_VENC_DUALP_CLASSIC();
			break;

		case '3':/* H.265@1080@30fps+H.265@1080@30fps */
            s32Ret = SAMPLE_VENC_DUALPP_CLASSIC();
            break;

        case '4':/* H.264@NormalP@1080p@30fps@P_Islice + H.265@NormalP@1080p@30fps@P_Islice*/
            s32Ret = SAMPLE_VENC_IntraRefresh_CLASSIC();
            break;

		case '5':/* 1*1080p mjpeg encode + 1*1080p jpeg  */
            s32Ret = SAMPLE_VENC_1080P_MJPEG_JPEG();
            break;

        case '6':/* H.264@NormalP@1080p@30fps@OnlineLowDelay + H.265@NormalP@1080p@30fps@OnlineLowDelay*/
            s32Ret = SAMPLE_VENC_LOW_DELAY();
            break;

        case '7':/* H.264@NormalP@1080p@30fps@RoiBgFrameRate+ H.265@NormalP@1080p@30fps@RoiBgFrameRate*/
            s32Ret = SAMPLE_VENC_ROIBG_CLASSIC();
            break;
		case '8':/* H.264 Svc-t */
            s32Ret = SAMPLE_VENC_SVC_H264();
            break;
#ifndef hi3516ev100
		case '9':/* H.264@NormalP@1080p@30fps+ H.265@NormalP@1080p@30fps@EPTZ */
            s32Ret = SAMPLE_VENC_EPTZ();
            break;
#endif
        default:
            SAMPLE_VENC_Usage(argv[0]);
            return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
    {
        printf("program exit normally!\n");
    }
    else
    {
        printf("program exit abnormally!\n");
    }

    exit(s32Ret);

    return s32Ret;
}


HI_S32 SAMPLE_VENC_NORMALP_CLASSIC2(HI_VOID)
	{
		PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
		PIC_SIZE_E enSize[2] = {PIC_UHD4K, PIC_HD1080}; 
		VENC_GOP_ATTR_S stGopAttr;
		VENC_GOP_MODE_E enGopMode[2] = {VENC_GOPMODE_NORMALP,VENC_GOPMODE_NORMALP};
		HI_U32 u32Profile = 0;
		
		VB_CONF_S stVbConf;
		SAMPLE_VI_CONFIG_S stViConfig = {0};
		
		VPSS_GRP VpssGrp;
		VPSS_CHN VpssChn;
		VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
		VPSS_CHN_ATTR_S stVpssChnAttr = {0};
		VPSS_CHN_MODE_S stVpssChnMode;
		
		VENC_CHN VencChn;
		SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
		
		HI_S32 s32ChnNum=2;
		
		HI_S32 s32Ret = HI_SUCCESS;
		HI_U32 u32BlkSize;
		SIZE_S stSize;
		char c;
	
	
		printf("\t step  1: \n");
	
		/******************************************
		 step  1: init sys variable 
		******************************************/
		memset(&stVbConf,0,sizeof(VB_CONF_S));
		
		SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
	  
		stVbConf.u32MaxPoolCnt = 128;
		printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
		/*video buffer*/ 
		if(s32ChnNum >= 1)
		{
			u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
						enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
			stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
			stVbConf.astCommPool[0].u32BlkCnt = 10;
		}
		if(s32ChnNum >= 2)
		{
			u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
						enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
			stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
			stVbConf.astCommPool[1].u32BlkCnt =10;
		}
	
		printf("\t step  2:\n");
	
		/******************************************
		 step 2: mpp system init. 
		******************************************/
		s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("system init failed with %d!\n", s32Ret);
			goto END_VENC_1080P_CLASSIC_0;
		}
	
		printf("\t step  3:  \n");
	
		/******************************************
		 step 3: start vi dev & chn to capture
		******************************************/
		stViConfig.enViMode   = SENSOR_TYPE;
		stViConfig.enRotate   = ROTATE_NONE;
		stViConfig.enNorm	  = VIDEO_ENCODING_MODE_AUTO;
		stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
		stViConfig.enWDRMode  = WDR_MODE_NONE;
		s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("start vi failed!\n");
			goto END_VENC_1080P_CLASSIC_1;
		}
		
		printf("\t step  4:  \n");
		/******************************************
		 step 4: start vpss and vi bind vpss
		******************************************/
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_1;
		}
	
		printf("\t step  5: \n");
	
		if(s32ChnNum >= 1)
		{
			printf("\t step  6: \n");
			VpssGrp = 0;
			stVpssGrpAttr.u32MaxW = stSize.u32Width;
			stVpssGrpAttr.u32MaxH = stSize.u32Height;
			stVpssGrpAttr.bIeEn = HI_FALSE;
			stVpssGrpAttr.bNrEn = HI_TRUE;
			stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_VIDEO;
			stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrRefSource = VPSS_NR_REF_FROM_RFR;
			stVpssGrpAttr.stNrAttr.u32RefFrameNum = 2;
			stVpssGrpAttr.bHistEn = HI_FALSE;
			stVpssGrpAttr.bDciEn = HI_FALSE;
			stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
			stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
			  stVpssGrpAttr.bStitchBlendEn	 = HI_FALSE;
	
			s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Start Vpss failed!\n");
				goto END_VENC_1080P_CLASSIC_2;
			}
	
			s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Vi bind Vpss failed!\n");
				goto END_VENC_1080P_CLASSIC_3;
			}
	
			VpssChn = 0;
			stVpssChnMode.enChnMode 	 = VPSS_CHN_MODE_USER;
			stVpssChnMode.bDouble		 = HI_FALSE;
			stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
			stVpssChnMode.u32Width		 = stSize.u32Width;
			stVpssChnMode.u32Height 	 = stSize.u32Height;
			stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
			memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
			stVpssChnAttr.s32SrcFrameRate = -1;
			stVpssChnAttr.s32DstFrameRate = -1;
			s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Enable vpss chn failed!\n");
				goto END_VENC_1080P_CLASSIC_4;
			}
		}
		if(s32ChnNum >= 2)
		{
			
			printf("\t step  7: \n");
			s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
				goto END_VENC_1080P_CLASSIC_4;
			}
			VpssChn = 1;
			stVpssChnMode.enChnMode 	  = VPSS_CHN_MODE_USER;
			stVpssChnMode.bDouble		  = HI_FALSE;
			stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
			stVpssChnMode.u32Width		  = stSize.u32Width;
			stVpssChnMode.u32Height 	  = stSize.u32Height;
			stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
			stVpssChnAttr.s32SrcFrameRate = -1;
			stVpssChnAttr.s32DstFrameRate = -1;
			s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Enable vpss chn failed!\n");
				goto END_VENC_1080P_CLASSIC_4;
			}
		}
	
		/******************************************
		 step 5: start stream venc
		******************************************/
		/*** HD1080P **/
		printf("\t c) cbr.\n");
		printf("\t v) vbr.\n");
		printf("\t a) avbr.\n");
		printf("\t f) fixQp\n");
		printf("please input choose rc mode!\n");
		c = 'c';//(char)getchar();
		switch(c)
		{
			case 'c':
				enRcMode = SAMPLE_RC_CBR;
				break;
			case 'v':
				enRcMode = SAMPLE_RC_VBR;
				break;
			case 'a':
				enRcMode = SAMPLE_RC_AVBR;
			break;
			case 'f':
				enRcMode = SAMPLE_RC_FIXQP;
				break;
			default:
				printf("rc mode! is invaild!\n");
				goto END_VENC_1080P_CLASSIC_4;
		}
	
		/*** enSize[0] **/
		if(s32ChnNum >= 1)
		{
			VpssGrp = 0;
			VpssChn = 0;
			VencChn = 0;
			
		printf("\t step  8: \n");
			s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Get GopAttr failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}		
			
			s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
										   gs_enNorm, enSize[0], enRcMode,u32Profile,&stGopAttr);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Start Venc failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}
	
			s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Start Venc failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}
		}
	
		/*** enSize[1] **/
		if(s32ChnNum >= 2)
		{
			VpssChn = 1;
			VencChn = 1;
			
		printf("\t step  9: \n");
			s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Get GopAttr failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}		
			
			s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
										   gs_enNorm, enSize[1], enRcMode,u32Profile,&stGopAttr);
	
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Start Venc failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}
	
			s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
			if (HI_SUCCESS != s32Ret)
			{
				SAMPLE_PRT("Start Venc failed!\n");
				goto END_VENC_1080P_CLASSIC_5;
			}
		}
		
		printf("\t step  10: \n");
		/******************************************
		 step 6: stream venc process -- get stream, then save it to file. 
		******************************************/
		s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	
		printf("please press twice ENTER to exit this sample\n");
		getchar();
		getchar();
		//sleep(20000);
	
		/******************************************
		 step 7: exit process
		******************************************/
		SAMPLE_COMM_VENC_StopGetStream();
		
	END_VENC_1080P_CLASSIC_5:
		
		VpssGrp = 0;
		switch(s32ChnNum)
		{
			case 2:
				VpssChn = 1;   
				VencChn = 1;
				SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
				SAMPLE_COMM_VENC_Stop(VencChn);
			case 1:
				VpssChn = 0;  
				VencChn = 0;
				SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
				SAMPLE_COMM_VENC_Stop(VencChn);
				break;
				
		}
		SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
		
	END_VENC_1080P_CLASSIC_4:	//vpss stop
	
		VpssGrp = 0;
		switch(s32ChnNum)
		{
			case 2:
				VpssChn = 1;
				SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
			case 1:
				VpssChn = 0;
				SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
			break;
		
		}
	
	END_VENC_1080P_CLASSIC_3:	 //vpss stop	   
		SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	END_VENC_1080P_CLASSIC_2:	 //vpss stop   
		SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
	END_VENC_1080P_CLASSIC_1:	//vi stop
		SAMPLE_COMM_VI_StopVi(&stViConfig);
	END_VENC_1080P_CLASSIC_0:	//system exit
		SAMPLE_COMM_SYS_Exit();
		
		return s32Ret;	  
	}



#define MAX_FRM_WIDTH   8192

#define VALUE_BETWEEN(x,min,max) (((x)>=(min)) && ((x) <= (max)))

typedef struct hiDUMP_MEMBUF_S
{
    VB_BLK  hBlock;
    VB_POOL hPool;
    HI_U32  u32PoolId;

    HI_U32  u32PhyAddr;
    HI_U8*   pVirAddr;
    HI_S32  s32Mdev;
} DUMP_MEMBUF_S;


typedef struct vpss_video_
{

	pthread_t gs_GetYuv;

	HI_U32 u32VpssDepthFlag;
	HI_U32 u32SignalFlag;

	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn ;
	HI_U32 u32OrigDepth;

	VB_POOL hPool  ;
	DUMP_MEMBUF_S stMem ;
	VGS_HANDLE hHandle;
	HI_U32  u32BlkSize;

	VIDEO_FRAME_INFO_S stFrame;

	HI_CHAR* pUserPageAddr[2];
	HI_U32 u32Size ;
} *pVpss_video,vpss_video;



vpss_video vpss_main;
vpss_video vpss_sub;


	/*
static HI_U32 u32VpssDepthFlag = 0;
static HI_U32 u32SignalFlag = 0;

static VPSS_GRP VpssGrp = 0;
static VPSS_CHN VpssChn = 0;
static HI_U32 u32OrigDepth = 0;

static VB_POOL hPool  = VB_INVALID_POOLID;
static DUMP_MEMBUF_S stMem = {0};
static VGS_HANDLE hHandle = -1;
static HI_U32  u32BlkSize = 0;

static HI_CHAR* pUserPageAddr[2] = {HI_NULL,HI_NULL};
static HI_U32 u32Size = 0;
*/


static HI_S32 VPSS_Restore(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,vpss_video * my_vpss_video)
{
    HI_S32 s32Ret= HI_FAILURE;

    if(VB_INVALID_POOLID != my_vpss_video->stFrame.u32PoolId)
    {
        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &my_vpss_video->stFrame);
        if(HI_SUCCESS != s32Ret)
        {
             printf("Release Chn Frame error!!!\n");
        }
        my_vpss_video->stFrame.u32PoolId = VB_INVALID_POOLID;
    }
    if(-1 != my_vpss_video->hHandle)
    {
        HI_MPI_VGS_CancelJob(my_vpss_video->hHandle);
        my_vpss_video->hHandle = -1;
    }   
    if(HI_NULL != my_vpss_video->stMem.pVirAddr)
    {
        HI_MPI_SYS_Munmap((HI_VOID*)my_vpss_video->stMem.pVirAddr, my_vpss_video->u32BlkSize );
       my_vpss_video-> stMem.u32PhyAddr = HI_NULL;
    }
    if(VB_INVALID_POOLID != my_vpss_video->stMem.hPool)
    {
        HI_MPI_VB_ReleaseBlock(my_vpss_video->stMem.hBlock);
        my_vpss_video->stMem.hPool = VB_INVALID_POOLID;
    }

    if (VB_INVALID_POOLID != my_vpss_video->hPool)
    {
        HI_MPI_VB_DestroyPool( my_vpss_video->hPool );
        my_vpss_video->hPool = VB_INVALID_POOLID;
    }

    if(HI_NULL != my_vpss_video->pUserPageAddr[0])
    {
        HI_MPI_SYS_Munmap(my_vpss_video->pUserPageAddr[0], my_vpss_video->u32Size);
        my_vpss_video->pUserPageAddr[0] = HI_NULL;
    }
    


    if(my_vpss_video->u32VpssDepthFlag)
    {
        if (HI_MPI_VPSS_SetDepth(VpssGrp, VpssChn, my_vpss_video->u32OrigDepth) != HI_SUCCESS)
        {
            printf("set depth error!!!\n");
        }
        my_vpss_video->u32VpssDepthFlag = 0;
    }

    return HI_SUCCESS;
}





void SaveYuvData(VIDEO_FRAME_S* pVBuf,int channel,unsigned long long time,vpss_video * my_vpss_video)
{
	static int mytestcount = 0;
    unsigned int w, h;
    char* pVBufVirt_Y;
    char* pVBufVirt_C;
    char* pMemContent;
    char* pMemContentDes;
    //static unsigned char TmpBuff[MAX_FRM_WIDTH];                //If this value is too small and the image is big, this memory may not be enough
    													
    HI_U32 phy_addr;
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/*When the storage format is a planar format, this variable is used to keep the height of the UV component */

	//HI_U32 u32Size ;


    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        my_vpss_video->u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 3 / 2;
        u32UvHeight = pVBuf->u32Height / 2;
    }
    else if(PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat)
    {
        my_vpss_video->u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height) * 2;
        u32UvHeight = pVBuf->u32Height;
    }
    else if(PIXEL_FORMAT_YUV_400 == enPixelFormat)
    {
        my_vpss_video->u32Size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);
        u32UvHeight = pVBuf->u32Height;
    }

    phy_addr = pVBuf->u32PhyAddr[0];

    //printf("SaveYuvData 2 . %d  %d.. \n",pVBuf->u32Stride[0],pVBuf->u32Stride[1] );
	//return;
    my_vpss_video->pUserPageAddr[0] = (HI_CHAR*) HI_MPI_SYS_Mmap(phy_addr, my_vpss_video->u32Size);
    if (HI_NULL == my_vpss_video->pUserPageAddr[0])
    {
        return;
    }
    //printf("stride: %d,%d\n",pVBuf->u32Stride[0],pVBuf->u32Stride[1] );

    pVBufVirt_Y = my_vpss_video->pUserPageAddr[0];
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0]) * (pVBuf->u32Height);

   // fprintf(stderr, "saving......Y2......\n");
	//mytestcount++;
	//if(mytestcount%3 == 1)
   // for (h = 0; h < pVBuf->u32Height/2; h++)

   // memset(myTmpBuff,0,pVBuf->u32Width*pVBuf->u32Height*3/2);
    int dataoffset = 0;
    /*for (h = 0; h < pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h * pVBuf->u32Stride[0];
        //pMemContentDes = myTmpBuff + h * pVBuf->u32Stride[0];

		
       // fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
       memcpy(myTmpBuff+dataoffset, pMemContent, pVBuf->u32Width);
		dataoffset+=pVBuf->u32Width;
    }
	*/
   // memcpy(myTmpBuff+dataoffset, pVBufVirt_Y, pVBuf->u32Width* pVBuf->u32Height);
	dataoffset+=pVBuf->u32Width* pVBuf->u32Height;
   // memcpy(myTmpBuff+dataoffset, pVBufVirt_C, pVBuf->u32Width* pVBuf->u32Height/2);

    if(PIXEL_FORMAT_YUV_400 != enPixelFormat&&0)
    {
       // fprintf(stderr, "U......\n");
    
        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1];
			
      // memcpy(myTmpBuff+dataoffset, pMemContent, pVBuf->u32Width/2);
		dataoffset+=pVBuf->u32Width/2;

        }

       // fprintf(stderr, "V......\n");
        for (h = 0; h < u32UvHeight; h++)
        {
            pMemContent = pVBufVirt_C + h * pVBuf->u32Stride[1]+pVBuf->u32Width *pVBuf->u32Height/4;
			
			//memcpy(myTmpBuff+dataoffset, pMemContent, pVBuf->u32Width/2);
			 dataoffset+=pVBuf->u32Width/2;

        }
    }
	
						
#if  (defined (AI_TYPE_ALI_DMY))

	video_data_callback(pVBufVirt_Y, pVBuf->u32Width,pVBuf->u32Height, channel,enPixelFormat,time);
#endif
   // fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);

    HI_MPI_SYS_Munmap(my_vpss_video->pUserPageAddr[0], my_vpss_video->u32Size);
    my_vpss_video->pUserPageAddr[0] = HI_NULL;
}



HI_VOID* SetDepth(VPSS_GRP Grp, VPSS_CHN Chn,vpss_video * my_vpss_video)
{
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    HI_U32 u32Depth = 6;
    HI_S32 s32MilliSec = -1;
    HI_S32 s32Times = 10;
    HI_BOOL bSendToVgs = HI_FALSE;
    VIDEO_FRAME_INFO_S stFrmInfo;
    VGS_TASK_ATTR_S stTask;
    HI_U32 u32LumaSize = 0;
    HI_U32 u32ChrmSize = 0;
    HI_U32 u32PicLStride = 0;
    HI_U32 u32PicCStride = 0;
    HI_U32 u32Width = 0;
    HI_U32 u32Height = 0;
    HI_S32 i = 0;
    HI_S32 s32Ret;


	
    //printf("GetYuvDataFormVPSS ......\n");

    
    if (HI_MPI_VPSS_GetDepth(Grp, Chn, &my_vpss_video->u32OrigDepth) != HI_SUCCESS)
    {
        printf("get depth error!!!\n");
        return (HI_VOID*) - 1;
    }

    if (HI_MPI_VPSS_SetDepth(Grp, Chn, u32Depth) != HI_SUCCESS)
    {
        printf("set depth error!!!\n");
        //VPSS_Restore(Grp, Chn);
        return (HI_VOID*) - 1;
    }


	
	return (HI_VOID*) - 1;


}


HI_VOID* InitGetYuvDataFormVPSS(VPSS_GRP Grp, VPSS_CHN Chn, HI_U32 u32FrameCnt,vpss_video * my_vpss_video)
{
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    HI_U32 u32Cnt = u32FrameCnt;
    HI_U32 u32Depth = 2;
    HI_S32 s32MilliSec = -1;
    HI_S32 s32Times = 10;
    HI_BOOL bSendToVgs = HI_FALSE;
    VIDEO_FRAME_INFO_S stFrmInfo;
    VGS_TASK_ATTR_S stTask;
    HI_U32 u32LumaSize = 0;
    HI_U32 u32ChrmSize = 0;
    HI_U32 u32PicLStride = 0;
    HI_U32 u32PicCStride = 0;
    HI_U32 u32Width = 0;
    HI_U32 u32Height = 0;
    HI_S32 i = 0;
    HI_S32 s32Ret;

	
	//VIDEO_FRAME_INFO_S stFrame;

	
    //printf("GetYuvDataFormVPSS ......\n");

    
    if (HI_MPI_VPSS_GetDepth(Grp, Chn, &my_vpss_video->u32OrigDepth) != HI_SUCCESS)
    {
        printf("get depth error!!!\n");
        return (HI_VOID*) - 1;
    }

    if (HI_MPI_VPSS_SetDepth(Grp, Chn, u32Depth) != HI_SUCCESS)
    {
        printf("set depth error!!!\n");
        VPSS_Restore(Grp, Chn,my_vpss_video);
        return (HI_VOID*) - 1;
    }
    my_vpss_video->u32VpssDepthFlag = 1;

    memset(&my_vpss_video->stFrame, 0, sizeof(my_vpss_video->stFrame));
	
    my_vpss_video->stFrame.u32PoolId = VB_INVALID_POOLID;
	
    while (HI_MPI_VPSS_GetChnFrame(Grp, Chn, &my_vpss_video->stFrame, s32MilliSec) != HI_SUCCESS)
    {
        s32Times--;
        if(0 >= s32Times)
        {
            printf("get frame error for 10 times,now exit !!!\n");
            VPSS_Restore(Grp, Chn,my_vpss_video);
            return (HI_VOID*) - 1;
        }
        usleep(40000);
    }
 	
	switch (my_vpss_video->stFrame.stVFrame.enPixelFormat)
	{
		case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
			snprintf(szPixFrm, 10, "P420");
			break;
		case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
			snprintf(szPixFrm, 10, "P422");
			break;
		case PIXEL_FORMAT_YUV_400:
			snprintf(szPixFrm, 10, "P400");
			break;
		default:
			snprintf(szPixFrm, 10, "--");
			break;
	}
	
	//SaveYuvData(&stFrame.stVFrame);
	
	//s32Ret = HI_MPI_VO_SendFrame(0,0,&stFrame,1000);//


	//SaveYuvData_(&stFrame.stVFrame);

    s32Ret = HI_MPI_VPSS_ReleaseChnFrame(Grp, Chn, &my_vpss_video->stFrame);
    if(HI_SUCCESS != s32Ret)
    {
        printf("Release frame error ,now exit !!!\n");
        VPSS_Restore(Grp, Chn,my_vpss_video);
        return (HI_VOID*) - 1;    
    }
    my_vpss_video->stFrame.u32PoolId = VB_INVALID_POOLID;
	
	return (HI_VOID*) - 1; 

}


HI_VOID* GetYuvDataFormVPSS(VPSS_GRP Grp, VPSS_CHN Chn, HI_U32 u32FrameCnt,vpss_video * my_vpss_video)
{
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    HI_U32 u32Cnt = u32FrameCnt;
    HI_U32 u32Depth = 2;
    HI_S32 s32MilliSec = -1;
    HI_S32 s32Times = 10;
    HI_BOOL bSendToVgs = HI_FALSE;
    VIDEO_FRAME_INFO_S stFrmInfo;
    VGS_TASK_ATTR_S stTask;
    HI_U32 u32LumaSize = 0;
    HI_U32 u32ChrmSize = 0;
    HI_U32 u32PicLStride = 0;
    HI_U32 u32PicCStride = 0;
    HI_U32 u32Width = 0;
    HI_U32 u32Height = 0;
    HI_S32 i = 0;
    HI_S32 s32Ret;

	//VIDEO_FRAME_INFO_S stFrame;

    my_vpss_video->u32VpssDepthFlag = 1;

    memset(&my_vpss_video->stFrame, 0, sizeof(my_vpss_video->stFrame));
    my_vpss_video->stFrame.u32PoolId = VB_INVALID_POOLID;

    /* get frame  */
    while (u32Cnt--)
    {
        if (HI_MPI_VPSS_GetChnFrame(Grp, Chn, &my_vpss_video->stFrame, s32MilliSec) != HI_SUCCESS)
        {
            printf("Get frame fail \n");
            usleep(30000);
            continue;
        }
        if(my_vpss_video->VpssChn == 0)
			s32Ret = HI_MPI_VO_SendFrame(0,0,&my_vpss_video->stFrame,s32MilliSec);

		
        bSendToVgs = ((my_vpss_video->stFrame.stVFrame.enCompressMode > 0) || (my_vpss_video->stFrame.stVFrame.enVideoFormat > 0));
        
        if (bSendToVgs)
        {
            u32PicLStride = my_vpss_video->stFrame.stVFrame.u32Stride[0];
            u32PicCStride = my_vpss_video->stFrame.stVFrame.u32Stride[0];
            u32LumaSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] *my_vpss_video-> stFrame.stVFrame.u32Height;
            u32Width    = my_vpss_video->stFrame.stVFrame.u32Width;
            u32Height   = my_vpss_video->stFrame.stVFrame.u32Height;

            if(PIXEL_FORMAT_YUV_SEMIPLANAR_420 == my_vpss_video->stFrame.stVFrame.enPixelFormat)
            {
                my_vpss_video->u32BlkSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] * my_vpss_video->stFrame.stVFrame.u32Height * 3 >> 1;
                u32ChrmSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] *my_vpss_video->stFrame.stVFrame.u32Height >> 2;
            }
            else if(PIXEL_FORMAT_YUV_SEMIPLANAR_422 == my_vpss_video->stFrame.stVFrame.enPixelFormat)
            {
                my_vpss_video->u32BlkSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] * my_vpss_video->stFrame.stVFrame.u32Height * 2; 
                u32ChrmSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] * my_vpss_video->stFrame.stVFrame.u32Height >> 1;
            }
            else if(PIXEL_FORMAT_YUV_400 == my_vpss_video->stFrame.stVFrame.enPixelFormat)
            {
                my_vpss_video->u32BlkSize = my_vpss_video->stFrame.stVFrame.u32Stride[0] * my_vpss_video->stFrame.stVFrame.u32Height;
                u32ChrmSize = 0;
            }
            else
            {
                printf("Unsupported pixelformat %d\n",my_vpss_video->stFrame.stVFrame.enPixelFormat);
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }
            
            my_vpss_video->hPool   = HI_MPI_VB_CreatePool( my_vpss_video->u32BlkSize, 1, HI_NULL);
            if (my_vpss_video->hPool == VB_INVALID_POOLID)
            {
                printf("HI_MPI_VB_CreatePool failed! \n");
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }
            my_vpss_video->stMem.hPool = my_vpss_video->hPool;

            while ((my_vpss_video->stMem.hBlock = HI_MPI_VB_GetBlock(my_vpss_video->stMem.hPool, my_vpss_video->u32BlkSize, HI_NULL)) == VB_INVALID_HANDLE)
            {
                ;
            }
            my_vpss_video->stMem.u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(my_vpss_video->stMem.hBlock);
            
            my_vpss_video->stMem.pVirAddr = (HI_U8*) HI_MPI_SYS_Mmap(my_vpss_video-> stMem.u32PhyAddr,my_vpss_video->u32BlkSize );
            if (my_vpss_video->stMem.pVirAddr == HI_NULL)
            {
                printf("Mem dev may not open\n");
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }

            memset(&stFrmInfo.stVFrame, 0, sizeof(VIDEO_FRAME_S));
            stFrmInfo.stVFrame.u32PhyAddr[0] = my_vpss_video->stMem.u32PhyAddr;
            stFrmInfo.stVFrame.u32PhyAddr[1] = stFrmInfo.stVFrame.u32PhyAddr[0] + u32LumaSize;
            stFrmInfo.stVFrame.u32PhyAddr[2] = stFrmInfo.stVFrame.u32PhyAddr[1] + u32ChrmSize;

            stFrmInfo.stVFrame.pVirAddr[0] = my_vpss_video->stMem.pVirAddr;
            stFrmInfo.stVFrame.pVirAddr[1] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[0] + u32LumaSize;
            stFrmInfo.stVFrame.pVirAddr[2] = (HI_U8*) stFrmInfo.stVFrame.pVirAddr[1] + u32ChrmSize;

            stFrmInfo.stVFrame.u32Width     = u32Width;
            stFrmInfo.stVFrame.u32Height    = u32Height;
            stFrmInfo.stVFrame.u32Stride[0] = u32PicLStride;
            stFrmInfo.stVFrame.u32Stride[1] = u32PicCStride;
            stFrmInfo.stVFrame.u32Stride[2] = u32PicCStride;

            stFrmInfo.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
            stFrmInfo.stVFrame.enPixelFormat  = my_vpss_video->stFrame.stVFrame.enPixelFormat;
            stFrmInfo.stVFrame.enVideoFormat  = VIDEO_FORMAT_LINEAR;

            stFrmInfo.stVFrame.u64pts     = (i * 40);
            stFrmInfo.stVFrame.u32TimeRef = (i * 2);

            stFrmInfo.u32PoolId = my_vpss_video->hPool;

            s32Ret = HI_MPI_VGS_BeginJob(&my_vpss_video->hHandle);
            if (s32Ret != HI_SUCCESS)
            {
                printf("HI_MPI_VGS_BeginJob failed\n");
                my_vpss_video->hHandle = -1;
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }
            memcpy(&stTask.stImgIn, &my_vpss_video->stFrame, sizeof(VIDEO_FRAME_INFO_S));
            memcpy(&stTask.stImgOut , &stFrmInfo, sizeof(VIDEO_FRAME_INFO_S));
            s32Ret = HI_MPI_VGS_AddScaleTask(my_vpss_video->hHandle, &stTask);
            if (s32Ret != HI_SUCCESS)
            {
                printf("HI_MPI_VGS_AddScaleTask failed\n");
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }

            s32Ret = HI_MPI_VGS_EndJob(my_vpss_video->hHandle);
            if (s32Ret != HI_SUCCESS)
            {
                printf("HI_MPI_VGS_EndJob failed\n");
                VPSS_Restore(Grp, Chn,my_vpss_video);
                return (HI_VOID*) - 1;
            }
            my_vpss_video->hHandle = -1;

            SaveYuvData(&stFrmInfo.stVFrame,Chn,my_vpss_video->stFrame.stVFrame.u64pts,my_vpss_video);
			

            HI_MPI_VB_ReleaseBlock(my_vpss_video->stMem.hBlock);

           // SaveYuvData2(&stFrmInfo.stVFrame);
            if (s32Ret != HI_SUCCESS)
            { 
            	printf("HI_MPI_VO_SendFrame failed\n");
              //  VPSS_Restore(Grp, Chn);
               // return (HI_VOID*) - 1;
            }
            //printf("SaveYuvData\n");

			

            my_vpss_video->stMem.hPool =  VB_INVALID_POOLID;
            my_vpss_video->hHandle = -1;
            if(HI_NULL != my_vpss_video->stMem.pVirAddr)
            {
                HI_MPI_SYS_Munmap((HI_VOID*)my_vpss_video->stMem.pVirAddr, my_vpss_video->u32BlkSize );
                my_vpss_video->stMem.u32PhyAddr = HI_NULL;
            }
            if (my_vpss_video->hPool != VB_INVALID_POOLID)
            {
                HI_MPI_VB_DestroyPool(my_vpss_video->hPool );
               my_vpss_video->hPool = VB_INVALID_POOLID;
            }
            
        }
        else
        {
            //SaveYuvData(&stFrame.stVFrame);
            printf("save yuv data2\n");
        }

        //printf("Get VpssGrp %d frame %d!!\n", Grp, u32Cnt);
        /* release frame after using */
        s32Ret = HI_MPI_VPSS_ReleaseChnFrame(Grp, Chn, &my_vpss_video->stFrame);
        if(HI_SUCCESS != s32Ret)
        {
            printf("Release frame error ,now exit !!!\n");
            VPSS_Restore(Grp, Chn,my_vpss_video);
            return (HI_VOID*) - 1;    
        }
        
        my_vpss_video->stFrame.u32PoolId = VB_INVALID_POOLID;
    }
   // VPSS_Restore(Grp, Chn);
    return (HI_VOID*)0;
}



SAMPLE_VO_CONFIG_S g_stVoConfig =
{
    .u32DisBufLen = 0,
};


//VO_INTF_TYPE_E g_enVoIntfType = VO_INTF_CVBS;
VO_INTF_TYPE_E g_enVoIntfType = VO_INTF_BT1120;


//static SAMPLE_BIND_E g_enVioBind = SAMPLE_BIND_VI_VPSS_VO;


static HI_S32 SAMPLE_VIO_StartVO(SAMPLE_VO_CONFIG_S *pstVoConfig)
{
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;
    //VO_CHN VoChn = 0;
    VO_LAYER VoLayer = 0;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    HI_S32 s32Ret = HI_SUCCESS;

    stVoPubAttr.enIntfType = g_enVoIntfType;
    if (VO_INTF_BT1120 == g_enVoIntfType)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_1080P25;
        //gs_u32ViFrmRate = 50;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
    }
    stVoPubAttr.u32BgColor = 0x000000ff;

    /* In HD, this item should be set to HI_FALSE */
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartDev failed!\n");
        return s32Ret;
    }

    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
	

    if (SAMPLE_SENSOR_DOUBLE == SAMPLE_SENSOR_SINGLE)
    {
        if (VO_INTF_BT1120 == g_enVoIntfType)
        {
            stLayerAttr.stDispRect.u32Width = 1920;
            stLayerAttr.stDispRect.u32Height = 1080;
            stLayerAttr.u32DispFrmRt = 30;
        }
        else
        {
            stLayerAttr.stDispRect.u32Width = 720;
            stLayerAttr.stDispRect.u32Height = 288;
            stLayerAttr.u32DispFrmRt = 25;
        }
    }
    else
    {
        s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync,
                                      &stLayerAttr.stDispRect.u32Width, &stLayerAttr.stDispRect.u32Height,
                                      &stLayerAttr.u32DispFrmRt);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_VO_GetWH failed!\n");
            SAMPLE_COMM_VO_StopDev(VoDev);
            return s32Ret;
        }
    }

    stLayerAttr.stImageSize.u32Width  = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    if (pstVoConfig->u32DisBufLen)
    {
        s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr, HI_FALSE);
    }
    else
    {
        s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr, HI_TRUE);
    }
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        SAMPLE_COMM_VO_StopLayer(VoLayer);
        SAMPLE_COMM_VO_StopDev(VoDev);
        return s32Ret;
    }

    return HI_SUCCESS;
}


static SAMPLE_VPSS_ATTR_S g_stVpssAttr =
{
    .VpssGrp = 0,
    .VpssChn = 0,
    .stVpssGrpAttr =
    {
        .bDciEn    = HI_FALSE,
        .bHistEn   = HI_FALSE,
        .bIeEn     = HI_FALSE,
        .bNrEn     = HI_TRUE,
        .bStitchBlendEn = HI_FALSE,
        .stNrAttr  =
        {
            .enNrType       = VPSS_NR_TYPE_VIDEO,
            .u32RefFrameNum = 2,
            .stNrVideoAttr =
            {
                .enNrRefSource = VPSS_NR_REF_FROM_RFR,
                .enNrOutputMode = VPSS_NR_OUTPUT_NORMAL
            }
        },
        .enDieMode = VPSS_DIE_MODE_NODIE,
        .enPixFmt  = PIXEL_FORMAT_YUV_SEMIPLANAR_420,
        .u32MaxW   = 1920,
        .u32MaxH   = 1080
    },
    
    .stVpssChnAttr =
    {
        .bBorderEn       = HI_FALSE,
        .bFlip           = HI_FALSE,
        .bMirror         = HI_FALSE,
        .bSpEn           = HI_FALSE,
        .s32DstFrameRate = -1,
        .s32SrcFrameRate = -1,
    },

    .stVpssChnMode =
    {
        .bDouble         = HI_FALSE,
        .enChnMode       = VPSS_CHN_MODE_USER,
        .enCompressMode  = COMPRESS_MODE_NONE,
        .enPixelFormat   = PIXEL_FORMAT_YUV_SEMIPLANAR_420,
        .u32Width        = 1920,
        .u32Height       = 1080
    },

    .stVpssExtChnAttr =
    {
    }
};


/******************************************************************************
* function :  H.264@1080p@30fps+H.264@VGA@30fps


******************************************************************************/


static HI_S32 SAMPLE_VIO_StartVPSS(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, 
                            SAMPLE_VPSS_ATTR_S* pstVpssAttr, ROTATE_E enRotate)
{
    VPSS_GRP_ATTR_S *pstVpssGrpAttr = NULL;
    VPSS_CHN_ATTR_S *pstVpssChnAttr = NULL;
    VPSS_CHN_MODE_S *pstVpssChnMode = NULL;
    VPSS_EXT_CHN_ATTR_S *pstVpssExtChnAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;

    if (NULL != pstVpssAttr)
    {
        pstVpssGrpAttr = &pstVpssAttr->stVpssGrpAttr;
        pstVpssChnAttr = &pstVpssAttr->stVpssChnAttr;
        pstVpssChnMode = &pstVpssAttr->stVpssChnMode;
        pstVpssExtChnAttr = &pstVpssAttr->stVpssExtChnAttr;
    }
    else
    {
        pstVpssGrpAttr = &g_stVpssAttr.stVpssGrpAttr;
        pstVpssChnAttr = &g_stVpssAttr.stVpssChnAttr;
        pstVpssChnMode = &g_stVpssAttr.stVpssChnMode;
        pstVpssExtChnAttr = &g_stVpssAttr.stVpssExtChnAttr;
    }
    
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, pstVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start VPSS GROUP failed!\n");
        return s32Ret;
    }

    if (enRotate != ROTATE_NONE && SAMPLE_COMM_IsViVpssOnline())
    {
        s32Ret =  HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn, enRotate);
        if (HI_SUCCESS != s32Ret)
        {
           SAMPLE_PRT("set VPSS rotate failed\n");
           SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
           return s32Ret;
        }
    }

    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, pstVpssChnAttr, pstVpssChnMode, pstVpssExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start VPSS CHN failed!\n");
        SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
        return s32Ret;
    }

    return s32Ret;
}




					
#if  (defined (AI_TYPE_ALI_DMY))		
int info_data_callback_ex(face_info * info,void *user)
{

	if(info->rect_num > MAX_RECT_NUM ||info->rect_num < 0)
	{
	
		printf("info_data_callback_ex rect_num error %d\n",info->rect_num);
		return -1;
	}
	int i = 0;
	//printf("info_data_callback_ex %d\n",info->rect_num);
	
	showRectOnVenc(info->rects, info->rect_num);
	//printf("info_data_callback_ex ok\n");
	for(i = 0;i < info->rect_num; i ++ )
	{
			
		/*printf(" rect[%d] top %d\n",i,info->rects[i].top);
		printf(" rect[%d] left %d\n",i,info->rects[i].left);
		printf(" rect[%d] right %d\n",i,info->rects[i].right);
		printf(" rect[%d] bottom %d\n",i,info->rects[i].bottom);
		printf("\n");*/
	}

	
	return 0;
}
#endif                 	
void * Hisi_GetVideoYuvStreamThread(void * ctrl)
{
	//init_data_callback();
	vpss_video * my_vpss_video = ctrl;
	InitGetYuvDataFormVPSS(my_vpss_video->VpssGrp, my_vpss_video->VpssChn, 1, my_vpss_video);
	while(1)
		{
			GetYuvDataFormVPSS(my_vpss_video->VpssGrp, my_vpss_video->VpssChn, 1,my_vpss_video);
			//usleep(1000*1000);
		}
	return NULL;
}	

PIC_SIZE_E g_enPicSize = PIC_HD1080;

HI_U32 gs_u32ViFrmRate = 0;
static SAMPLE_BIND_E g_enVioBind = SAMPLE_BIND_VI_VPSS_VO;





HI_S32 Hisi_StartVideoStream(GK_MEDIA_ST media)
{

	printf("Hisi_StartVideoStream .....\n");
	
   // SAMPLE_VIO_PreView(&g_stViChnConfig);
	//return;

	PAYLOAD_TYPE_E enPayLoad[3]= {PT_H264, PT_H264, PT_H264};
	//PIC_SIZE_E enSize2[2] = {PIC_HD720, PIC_VGA}; 
	PIC_SIZE_E enSize[3] = {PIC_HD720, PIC_VGA,PIC_QVGA}; 
	PIC_SIZE_E initEnSize;
	VENC_GOP_ATTR_S stGopAttr;
	VENC_GOP_MODE_E enGopMode[3] = {VENC_GOPMODE_NORMALP,VENC_GOPMODE_NORMALP,VENC_GOPMODE_NORMALP};
	HI_U32 u32Profile = 0;
	
	VB_CONF_S stVbConf;
	SAMPLE_VI_CONFIG_S stViConfig = {0};
	
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
	VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
	VPSS_CHN_ATTR_S stVpssChnAttr = {0};
	VPSS_CHN_MODE_S stVpssChnMode;
	
	VENC_CHN VencChn;
	SAMPLE_RC_E enRcMode[3]= {SAMPLE_RC_CBR,SAMPLE_RC_CBR,SAMPLE_RC_CBR};
	
	HI_S32 s32ChnNum=3;
	
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 u32BlkSize;
	SIZE_S stSize;
	char c;

    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    VO_LAYER VoLayer = 0;
    VI_CHN ViChn = 0;

    VO_DEV VoDev = 0;;
    VO_CHN VoChn = 0;

    HI_BOOL  bViVpssOnline = HI_FALSE;


    MPP_CHN_S stSrcChn, stDestChn;




	GK_MEDIA_CHN_ST * pMedia = NULL;
	
	 s32ChnNum = media.stream_num;
	
	printf("\t step  1: %d \n",media.vi_standard);
	 


	 if( media.vi_standard == GK_STANDARD_PAL )
		 gs_enNorm = VIDEO_ENCODING_MODE_PAL;
	 else
		 gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
	
	  /*
	int i = 0;
	for( i = 0; i < media.stream_num; i++)
	{
		 pMedia = &media.stMediaStream[i];
	
		 if( pMedia->code == GK_CODE_264 )
			 enPayLoad[i] = PT_H264;
		 else if( pMedia->code == GK_CODE_265 )
			 enPayLoad[i] = PT_H265;
		 else
			 enPayLoad[i] = PT_H264;
	
		 if( pMedia->img_width == 1920 && pMedia->img_height == 1080 )
			 enSize[i] = PIC_HD1080;	 0
		 else if( pMedia->img_width == 1280 && pMedia->img_height == 720 )
			 enSize[i] = PIC_HD720;
		 else if( pMedia->img_width == 640 && pMedia->img_height == 480 )
			 enSize[i] = PIC_VGA;
		 else if( pMedia->img_width == 320 && pMedia->img_height == 240 )
			 enSize[i] = PIC_QVGA;
		 else
			 enSize[i] = PIC_HD1080;
	
		   if( pMedia->ctrl == GK_CODE_CTRL_CBR )
			 enRcMode[i] = SAMPLE_RC_CBR;
		 else if( pMedia->ctrl == GK_CODE_CTRL_VBR )
			 enRcMode[i] = SAMPLE_RC_VBR;
		 else
			 enRcMode[i] = SAMPLE_RC_CBR;
	
		 DPRINTK("[%d] code=%d %d,%d %d\n",i,enPayLoad[i],pMedia->img_width,pMedia->img_height,\
			 enRcMode[i]);
		 
	}*/



	

	/******************************************
	 step  1: init sys variable 
	******************************************/
	memset(&stVbConf,0,sizeof(VB_CONF_S));
	
	SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
  
	stVbConf.u32MaxPoolCnt = 128;
	printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
	/*video buffer*/ 
	if(s32ChnNum >= 1)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[0].u32BlkCnt = 10;
	}
	if(s32ChnNum >= 2)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
	{
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
					enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[2].u32BlkCnt =10;
	}
	//enSize[0] = PIC_HD720;

	printf("\t step  2:\n");

	/******************************************
	 step 2: mpp system init. 
	******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		goto END_VENC_1080P_CLASSIC_0;
	}

	printf("\t step  3: %d \n",SENSOR_TYPE);

	/******************************************
	 step 3: start vi dev & chn to capture
	******************************************/
	memset(&stViConfig,0,sizeof(stViConfig));
	
	stViConfig.enViMode   = SENSOR_TYPE;
	stViConfig.enRotate   = ROTATE_NONE;
	stViConfig.enNorm	  = VIDEO_ENCODING_MODE_AUTO;
	stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
	stViConfig.enWDRMode  = WDR_MODE_NONE;//WDR_MODE_2To1_LINE
	stViConfig.enFrmRate  = 0;//SAMPLE_FRAMERATE_30FPS;

	SAMPLE_VI_CONFIG_S *pstViConfig = &stViConfig;
	s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vi failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}
	
	printf("\t step  4:  \n");
	/******************************************
	 step 4: start vpss and vi bind vpss
	******************************************/
		
	s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		goto END_VENC_1080P_CLASSIC_1;
	}

	/******************************************
		step 2: start VO SD0
		******************************************/
#if (defined (AI_TYPE_SUNRISE))
		s32Ret = SAMPLE_VIO_StartVO(&g_stVoConfig);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("SAsMPLE_VIO start VO failed with %#x!\n", s32Ret);
			goto END_VENC_1080P_CLASSIC_1;
		}
#endif
	
    /******************************************
    step 3: start VPSS
    ******************************************/
	SAMPLE_VPSS_ATTR_S stVpssAttr;
	memcpy(&stVpssAttr, &g_stVpssAttr, sizeof(SAMPLE_VPSS_ATTR_S));

		
    stVpssAttr.stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssAttr.stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssAttr.stVpssChnMode.u32Width  = stSize.u32Width;
    stVpssAttr.stVpssChnMode.u32Height = stSize.u32Height;


		

	printf("\t step  5: \n");

	if(s32ChnNum >= 1)
	{
		printf("\t step  6: \n");
		VpssGrp = 0;
		stVpssGrpAttr.u32MaxW = stSize.u32Width;
		stVpssGrpAttr.u32MaxH = stSize.u32Height;
		stVpssGrpAttr.bIeEn = HI_FALSE;
		stVpssGrpAttr.bNrEn = HI_TRUE;
		stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_VIDEO;
		stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrRefSource = VPSS_NR_REF_FROM_RFR;
		stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrOutputMode = VPSS_NR_OUTPUT_NORMAL;
		stVpssGrpAttr.stNrAttr.u32RefFrameNum = 2;
		stVpssGrpAttr.bHistEn = HI_FALSE;
		stVpssGrpAttr.bDciEn = HI_FALSE;
		stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
		stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
		  stVpssGrpAttr.bStitchBlendEn	 = HI_FALSE;

		s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_2;
		}

		s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Vi bind Vpss failed!\n");
			goto END_VENC_1080P_CLASSIC_3;
		}
		

		printf("\t step  77: \n");
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}

		VpssChn = 0;
		stVpssChnMode.enChnMode 	 = VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		 = HI_FALSE;
		stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		 = stSize.u32Width;
		stVpssChnMode.u32Height 	 = stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
		memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bBorderEn       = HI_FALSE;
        stVpssChnAttr.bFlip           = HI_FALSE;
        stVpssChnAttr.bMirror         = HI_FALSE;
        stVpssChnAttr.bSpEn           = HI_FALSE;

		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable 1 vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		
		
	}
	if(s32ChnNum >= 2)
	{
		
		printf("\t step  7: \n");
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		VpssChn = 1;
		stVpssChnMode.enChnMode 	  = VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		  = HI_FALSE;
		stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		  = stSize.u32Width;
		stVpssChnMode.u32Height 	  = stSize.u32Height;
		stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bBorderEn       = HI_FALSE;
        stVpssChnAttr.bFlip           = HI_FALSE;
        stVpssChnAttr.bMirror         = HI_FALSE;
        stVpssChnAttr.bSpEn           = HI_FALSE;
		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	if(s32ChnNum >= 3)
	{
		
		printf("\t step  7: \n");
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT( "SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
		VpssChn = 2;
		stVpssChnMode.enChnMode 	  = VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		  = HI_FALSE;
		stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
		stVpssChnMode.u32Width		  = stSize.u32Width;
		stVpssChnMode.u32Height 	  = stSize.u32Height;
		stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		stVpssChnAttr.bBorderEn       = HI_FALSE;
        stVpssChnAttr.bFlip           = HI_FALSE;
        stVpssChnAttr.bMirror         = HI_FALSE;
        stVpssChnAttr.bSpEn           = HI_FALSE;
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
	
	enSize[0] = PIC_HD1080;

	//printf("....................................SAMPLE_COMM_VO_BindVpss start \n");
	//SAMPLE_COMM_VO_BindVpss(0, 0, 0, 0);
	//printf("....................................SAMPLE_COMM_VO_BindVpss over\n ");

	/******************************************
	 step 5: start stream venc
	******************************************/
	/*** HD1080P **/
	/*c = 'c';//(char)getchar();
	switch(c)
	{
		case 'c':
			enRcMode = SAMPLE_RC_CBR;
			break;
		case 'v':
			enRcMode = SAMPLE_RC_VBR;
			break;
		case 'a':
			enRcMode = SAMPLE_RC_AVBR;
		break;
		case 'f':
			enRcMode = SAMPLE_RC_FIXQP;
			break;
		default:
			printf("rc mode! is invaild!\n");
			goto END_VENC_1080P_CLASSIC_4;
	}*/

	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
		VpssChn = 0;
		VencChn = 0;
		
	printf("\t step  8: \n");
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[0],&stGopAttr,gs_enNorm);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Get GopAttr failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}		
		
		s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
									   gs_enNorm, enSize[0], enRcMode[0],u32Profile,&stGopAttr);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
		VencChn = 1;
		
	printf("\t step  9: \n");
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[1],&stGopAttr,gs_enNorm);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Get GopAttr failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}		
		
		s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],\
									   gs_enNorm, enSize[1], enRcMode[1],u32Profile,&stGopAttr);

		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}


	if(s32ChnNum >= 3)
	{
		VpssChn = 2;
		VencChn = 2;
		
	printf("\t step  9: \n");
		s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode[2],&stGopAttr,gs_enNorm);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Get GopAttr failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}		
		
		s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
									   gs_enNorm, enSize[2], enRcMode[2],u32Profile,&stGopAttr);

		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}

		s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Start Venc failed!\n");
			goto END_VENC_1080P_CLASSIC_5;
		}
	}

	


		


	
	printf("\t step  10: \n");
	/******************************************
	 step 6: stream venc process -- get stream, then save it to file. 
	******************************************/
	s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Venc failed!\n");
		goto END_VENC_1080P_CLASSIC_5;
	}


	//开启移动侦测
	 ////SAMPLE_IVE_Md();

	//音频开启
	 Hisi_InitAudio();
	
	 
#if  (defined (AI_TYPE_ALI_DMY))
	init_data_callback(info_data_callback_ex,NULL);

#endif

	vpss_main.u32VpssDepthFlag = 0;
	vpss_main.u32SignalFlag = 0;
	
	vpss_main.VpssGrp = 0;
	vpss_main.VpssChn = 0;
	vpss_main.u32OrigDepth = 0;
	
	vpss_main.hPool  = VB_INVALID_POOLID;
	
	memset(&vpss_main.stMem,0,sizeof(vpss_main.stMem));
	vpss_main.hHandle = -1;
	vpss_main.u32BlkSize = 0;
	
	vpss_main.pUserPageAddr[0] = HI_NULL;
	vpss_main.pUserPageAddr[1] = HI_NULL;
	vpss_main.u32Size = 0;



	vpss_sub.u32VpssDepthFlag = 0;
	vpss_sub.u32SignalFlag = 0;
	
	vpss_sub.VpssGrp = 0;
	vpss_sub.VpssChn = 0;
	vpss_sub.u32OrigDepth = 0;
	
	vpss_sub.hPool  = VB_INVALID_POOLID;
	
	memset(&vpss_sub.stMem,0,sizeof(vpss_sub.stMem));
	vpss_sub.hHandle = -1;;
	vpss_sub.u32BlkSize = 0;
	
	vpss_sub.pUserPageAddr[0] = HI_NULL;
	vpss_sub.pUserPageAddr[1] = HI_NULL;
	vpss_sub.u32Size = 0;

	
#if  (defined (AI_TYPE_SUNRISE))
	SetDepth(0,0,&vpss_main);
	//SetDepth(0,1);
#endif

#if  (defined (AI_TYPE_ALI_DMY))
	vpss_main.VpssChn = 0; 
	vpss_main.VpssGrp = 0;
	pthread_create(&vpss_main.gs_GetYuv, 0, Hisi_GetVideoYuvStreamThread, (void*)&vpss_main);
	vpss_sub.VpssChn = 1;
	vpss_sub.VpssGrp = 0;
	pthread_create(&vpss_sub.gs_GetYuv,  0, Hisi_GetVideoYuvStreamThread, (void*)&vpss_sub);
	
#endif

	while(1)
	{
		printf("enter scene auto run........................\n");
		scene_auto_run(NULL);
		sleep(1);
	}

	/******************************************
	 step 7: exit process
	******************************************/
	SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

	VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			VencChn = 2;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
			VencChn = 1;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
			VencChn = 0;
			SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
			SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
	SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

	VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:	 //vpss stop
	SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:	 //vpss stop
	SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
	SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
	SAMPLE_COMM_SYS_Exit();

	return s32Ret;
}

HI_S32 Hisi_StartVideoStream2(GK_MEDIA_ST media)
{

	SAMPLE_VENC_NORMALP_CLASSIC2();
	return -1;

    PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_HD1080};
	
    //PAYLOAD_TYPE_E enPayLoad[2]= {PT_H265, PT_H264};
    //PIC_SIZE_E enSize[2] = {PIC_UHD4K, PIC_HD1080};	

	
	
	HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode[2]= {SAMPLE_RC_CBR,SAMPLE_RC_CBR};

#ifndef hi3516ev100
    HI_S32 s32ChnNum=2;
#else
    HI_S32 s32ChnNum=1;
#endif

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;
    int i;
   GK_MEDIA_CHN_ST * pMedia = NULL;
   
    s32ChnNum = media.stream_num;

	//if( media.vi_standard == GK_STANDARD_PAL )
	//	gs_enNorm = VIDEO_ENCODING_MODE_PAL;
	//else
		gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
	
	

   for( i = 0; i < media.stream_num; i++)
   {
   		pMedia = &media.stMediaStream[i];

		if( pMedia->code == GK_CODE_264 )
			enPayLoad[i] = PT_H264;
		else if( pMedia->code == GK_CODE_265 )
			enPayLoad[i] = PT_H265;
		else
			enPayLoad[i] = PT_H264;

		if( pMedia->img_width == 1920 && pMedia->img_height == 1080 )
			enSize[i] = PIC_HD1080;		
		else if( pMedia->img_width == 1280 && pMedia->img_height == 720 )
			enSize[i] = PIC_HD720;
		else if( pMedia->img_width == 640 && pMedia->img_height == 480 )
			enSize[i] = PIC_VGA;
		else if( pMedia->img_width == 320 && pMedia->img_height == 240 )
			enSize[i] = PIC_QVGA;
		else
			enSize[i] = PIC_HD1080;

	      if( pMedia->ctrl == GK_CODE_CTRL_CBR )
			enRcMode[i] = SAMPLE_RC_CBR;
		else if( pMedia->ctrl == GK_CODE_CTRL_VBR )
			enRcMode[i] = SAMPLE_RC_VBR;
		else
			enRcMode[i] = SAMPLE_RC_CBR;

		DPRINTK("[%d] code=%d %d,%d %d\n",i,enPayLoad[i],pMedia->img_width,pMedia->img_height,\
			enRcMode[i]);
		
   }


    /******************************************
     step  1: init sys variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);

    stVbConf.u32MaxPoolCnt = 128;
	printf("s32ChnNum:%d,Sensor Size:%d\n",s32ChnNum,enSize[0]);
  
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        #ifndef hi3516ev100
	    stVbConf.astCommPool[0].u32BlkCnt = 10;
        #else
        stVbConf.astCommPool[0].u32BlkCnt = 4;

	// 640x480 子码流使用		
	stVbConf.astCommPool[1].u32BlkSize = 472320;
	stVbConf.astCommPool[1].u32BlkCnt = 2;

	// motion detect 使用
	stVbConf.astCommPool[2].u32BlkSize = 460800;
	stVbConf.astCommPool[2].u32BlkCnt = 1;
	
	
        #endif
	}

	#ifndef hi3516ev100
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =10;
	}
	if(s32ChnNum >= 3)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[2].u32BlkCnt = 10;
	}
	#endif

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
	memset(&stViConfig,0,sizeof(stViConfig));
	//printf("step SENSOR_TYPE %d %d\n  ",SENSOR_TYPE,SONY_IMX274_LVDS_4K_30FPS);
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }


	printf("step v thread .....%d.\n",__LINE__);
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

	printf("step v thread .....%d..%d..%d.\n",__LINE__,stSize.u32Width,stSize.u32Height);

    if(s32ChnNum >= 1)
    {
	    VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_VIDEO;
	    stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrRefSource = VPSS_NR_REF_FROM_RFR;
	    stVpssGrpAttr.stNrAttr.u32RefFrameNum = 2;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
          stVpssGrpAttr.bStitchBlendEn   = HI_FALSE;
		
		printf("step v thread .....%d.\n",__LINE__);

	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Start Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_2;
	    }
		
		printf("step v thread .....%d.\n",__LINE__);

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Vi bind Vpss failed!\n");
		    goto END_VENC_1080P_CLASSIC_3;
	    }
		
		printf("step v thread .....%d.\n",__LINE__);

	    VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	printf("step v thread .....%d.\n",__LINE__);
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
	printf("step v thread .....%d.\n",__LINE__);
    if(s32ChnNum >= 2)
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	printf("step v thread .....%d.\n",__LINE__);
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
    }
	
  //  if(s32ChnNum >= 3)
    if(1)
    {
    	// motion detect vpss set
    	HI_U32 u32Depth = 3;
	printf("step v thread .....%d.\n",__LINE__);
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, PIC_VGA, &stSize);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT(	"SAMPLE_COMM_SYS_GetPicSize failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	    VpssChn = 2;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_NONE;
	    stVpssChnAttr.s32SrcFrameRate = 6;
	    stVpssChnAttr.s32DstFrameRate = 1;
	printf("step v thread .....%d.\n",__LINE__);
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("Enable vpss chn failed!\n");
		    goto END_VENC_1080P_CLASSIC_4;
	    }
	printf("step v thread .....%d.\n",__LINE__);

	   s32Ret = HI_MPI_VPSS_SetDepth(VpssGrp, VpssChn, u32Depth);
	    if (HI_SUCCESS != s32Ret)
	    {
		    SAMPLE_PRT("HI_MPI_VPSS_SetDepth vpss chn failed!(%d,%d) ret=%x\n",\
				VpssGrp, VpssChn,s32Ret);
		    goto END_VENC_1080P_CLASSIC_4;
	    }
		
    }
   
	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
		
		printf("step v thread .....%d.\n",__LINE__);

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode[0],u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
		
		printf("step v thread .....%d.\n",__LINE__);

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                   gs_enNorm, enSize[1], enRcMode[1],u32Profile,ROTATE_NONE);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
		

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	/*
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;

	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],\
	                                   gs_enNorm, enSize[2], enRcMode[0],u32Profile,ROTATE_NONE);

	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	*/
    /******************************************
     step 6: stream venc process -- get stream, then save it to file.
    ******************************************/
    
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }


	//开启移动侦测
     SAMPLE_IVE_Md();

    //音频开启
     Hisi_InitAudio();


   

    while(1)
    {
    	   //调用auto_scene 接口

	 /* if( SENSOR_TYPE == SONY_IMX290_MIPI_1080P_30FPS)
	  {
	  	scene_auto_run("/mnt/mtd/sceneauto_imx290.ini");
	  }
	  else
    	   	scene_auto_run(NULL);*/
    	  sleep(1);
    }

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;

	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;

	}

END_VENC_1080P_CLASSIC_3:    //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
