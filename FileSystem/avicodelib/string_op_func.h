#ifndef _STRING_OP_FUNC_HEADER
#define _STRING_OP_FUNC_HEADER

#ifdef __cplusplus
extern "C" {
#endif

int SOF_FindSTR( char  * byhtml, char *cfind, int nStartPos);
int SOF_GetBetweenWords(char * byhtml, char * temp, int start, int end,int max_num);
int SOF_GetNumWord(char * source_buf,char * num_buf,int start,int max_num);
char * SOF_GetFileToBuf(char * file_name,int * file_length);
char * SOF_FreeFileBuf(char * buf);
int SOF_Is_file_exist(char * file_name);

#ifdef __cplusplus
}
#endif

#endif