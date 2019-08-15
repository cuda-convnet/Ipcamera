
#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

//#define SAVE_ISP_DEFAULT_VALUE 1

#define CONFIG_FILE_ISP  "/mnt/mtd/isp_config.txt"
#define CONFIG_FILE_ISP_DEFAULT  "/mnt/mtd/isp_config_default.txt"

int cf_read_value(char * file_name,char * value_descriptors,char * value);
int cf_write_value(char * file_name,char * value_descriptors,char * value);


#endif // !defined(_BUF_ARRAY_H_)

