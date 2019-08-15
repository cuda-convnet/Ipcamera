#ifndef _YUN_SERVER_HEADER
#define _YUN_SERVER_HEADER

#ifdef __cplusplus
extern "C" {
#endif

#define MAC_ID  (0xabbb1234)

enum
{

	YS_BASE = 1000,
	YS_GET_NVR_ALL_INFO,
	YS_NVR_CONNECT_TO_PC	,
	YS_NVR_CONNECT_TO_YUN_SERVER
};

typedef struct _YUN_CMD_ST_
{
	int flag;
	int cmd;	
	int buf_len;	
}YUN_CMD_ST;


int get_yun_server_flag();
int open_yun_server(int flag);
int yun_server_restart();


#ifdef __cplusplus
}
#endif

#endif

