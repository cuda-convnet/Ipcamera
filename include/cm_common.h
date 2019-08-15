
#ifndef _CM_COMMON_H_
#define _CM_COMMON_H_

typedef struct _GST_BXS_PARAM {
	void (*GST_DVR_MAIN_exit)();
}GST_BXS_PARAM;

int tw_init(void);
//int tw_get_function_addr(GST_DEV_FILE_PARAM * gst);
void TurnOnOff();
void dev_ctrl_thread_pro(void);
void wtd_thread_pro(void);
int tw_stop(void);
int get_allfunction_ptr(GST_DEV_FILE_PARAM *filepar);


//void trace(char * error);


#endif //_CM_COMMON_H_


