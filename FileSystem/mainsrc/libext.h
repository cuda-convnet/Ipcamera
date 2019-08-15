#ifndef _LIB_EXT_H_  //pg add
#define _LIB_EXT_H_  //pg add


typedef struct
{
	int (*ext_read_eeprom)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_write_eeprom)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_read_VGA)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_write_VGA)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_read_RTC)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_write_RTC)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);

	int (*ext_write_tw2835)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	int (*ext_read_tw2835)(unsigned char page_num,unsigned int sub_addr,unsigned char data_num,unsigned char *ptr);
	
	int (*ext_GPIOOUTEN)(unsigned char gpiono);
	int (*ext_GPIOOUT)(unsigned char gpiono,unsigned char level);
	int (*ext_GPIOIN)(unsigned char gpiono, unsigned char *gpioinlevel);
	int (*ext_GPIOINEN)(unsigned char gpiono);
	int (*ext_MDATAWR)(unsigned char val);
	int (*ext_MDATAHR)(void);
	int (*ext_MDATARD)(unsigned char *mdata);

	int (*ext_SMAUTHEN)(unsigned char GC_select, unsigned char* raac);
	int (*ext_VERWRPAS)(unsigned char pw_select, unsigned char* rpac);
	int (*ext_VERRDPAS)(unsigned char pw_select, unsigned char* rpac);
	int (*ext_SETUSZON)(unsigned char zone);
	int (*ext_RDUSZON)(unsigned char rd_high_addr,unsigned char rd_low_addr,unsigned char rd_number);
	int (*ext_WRUSZON)(unsigned char wr_high_addr,unsigned char wr_low_addr,unsigned char wr_number);
	int (*ext_SMSETTAB)(unsigned char firstaddr,unsigned char data_num,unsigned char *ptr );
	int (*ext_SMGETTAB)(unsigned char firstaddr,unsigned char data_num,unsigned char *ptr);
	int (*ext_SMWPAN)(void);
	int (*ext_SMRPAN)(void);

	int (*UART_create_thread)(void);
}EXT_LIB_PRO;

typedef struct _UART_INFO 
{
	char RecvBuf[20];
	char SendBuf[20];
	unsigned char RecvEnable; //接收到数据后，清0，禁止继续接收数据，直到接收到的数据处理完毕，由处理模块置1
	unsigned char RecvFlag;   //接收到数据后置1，由处理模块清0
	unsigned char SendFlag;   //发送线程发现该标志为1后，把数据发送出去，发送完毕后清0
}UART_INFO;


int extlib_module_init(EXT_LIB_PRO *arg);
int extlib_module_exit(EXT_LIB_PRO *arg);
void testhw(void);
//int UART_create_thread(void);
extern UART_INFO stUart_info ;
#endif  //_LIB_EXT_H_  pg add

