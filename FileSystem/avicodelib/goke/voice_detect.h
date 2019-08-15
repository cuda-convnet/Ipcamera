#ifndef _VOICE_DETECT_HEADER
#define _VOICE_DETECT_HEADER

#ifdef __cplusplus
extern "C" {
#endif



int VD_GetDataFromAudioPcm(char * pAudioBuf,int iAudioLen,char * pSaveDetectData,int iSaveDataLen);

#ifdef __cplusplus
}
#endif

#endif