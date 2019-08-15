#ifndef _G711_LIB_H_
#define _G711_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif


#define G711_ULAW  (0)
#define G711_ALAW  (1)


int G711_EnCode(unsigned char* pCodecBits, const char* pBuffer, int nBufferSize,int g711_type);
int G711_Decode(char* pRawData, const unsigned char* pBuffer, int nBufferSize,int g711_type);

#ifdef __cplusplus
}
#endif

#endif
