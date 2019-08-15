#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "string_op_func.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif



static FILE * fp_config = NULL;
static char  * config_buf = NULL;
static int config_file_size = 0;



int SOF_FindSTR( char  * byhtml, char *cfind, int nStartPos)
{
	int i;
	long length;
	char * tmphtml;
	char * temfind ;

	
	if( byhtml == NULL || cfind == NULL)
		return -1;

	tmphtml =(char *) byhtml + nStartPos;

	temfind = cfind;

	if(nStartPos < 0)
		return -1;

	length = strlen(tmphtml); 
	
	for( i = 0 ; i < length; i++)
	{		

		if( *tmphtml != *temfind )
		{
			tmphtml++;

			continue;
		}

		while( *tmphtml == *temfind )
		{
			//printf(" %c.%c|",*tmphtml,*temfind);
			tmphtml++;
			temfind++;

			if( *temfind == (char)NULL ) // 找到了。
			{			
				return nStartPos + i;
			}
		}

		//printf("\n");	
		
		if( *temfind == (char)NULL ) // 找到了。
		{			
			return nStartPos + i;
		}
		else
		{	// 从与temfind首字节相同的那一位的后一位重新开始找，
			temfind = cfind;
			tmphtml = (char *)byhtml + nStartPos + i + 1;
		}
	}

	return -1;
}



// start 是指 '<' 所在的位置，end 指 '>'的位置，此函数取两个位置之间的那部分字符串。
int SOF_GetBetweenWords(char * byhtml, char * temp, int start, int end,int max_num)
{
	int i;	
	
	if(  byhtml == NULL ||  temp == NULL)
		return -1;

	if( end - start > max_num )
	{		
		temp[0] = (char)NULL;
		return -1;
	}	

	for(i = start + 1; i < end ; i++)
	{
		*temp = byhtml[i];

		temp++;
	}

	*temp = (char)NULL; // 结束符。

	return 1;
}

int SOF_GetNumWord(char * source_buf,char * num_buf,int start,int max_num)
{
	int i;
	int length = 0;
	char * start_pos_ptr = NULL;
	char * hex_ptr = NULL;
	char tmp_word = 0;
	int get_between_num = 0;
	int is_16_num = 0;
	int skip = 0;
	
	if(  source_buf == NULL ||  num_buf == NULL)
		return -1;	

	start_pos_ptr = source_buf + start;
	length = strlen(start_pos_ptr);

	for( i = 0 ; i < length-1; i++)
	{		
		tmp_word = *start_pos_ptr;
		if( tmp_word < '0'  ||  tmp_word >'9')
		{	
			//这种方法可以筛选出 16进制数据如0x234.
			
			if( tmp_word == '-' )
			{ //识别出带负号的数据。
				hex_ptr = start_pos_ptr + 1;

				if( *hex_ptr < '0'  ||  *hex_ptr >'9')
				{	
					start_pos_ptr++;
					continue;
				}
			}else
			{			
				start_pos_ptr++;
				continue;
			}
		}		

		while( 1 )
		{			
			*num_buf = *start_pos_ptr;
			
			start_pos_ptr++;
			num_buf++;
			get_between_num++;

			if( *start_pos_ptr == (char)NULL ) // 找到了。						
				goto get_ok;			

			tmp_word = *start_pos_ptr;
			if( (tmp_word < '0'  ||  tmp_word >'9') )
			{
				if( tmp_word == 'x' )
				{					
					hex_ptr = start_pos_ptr - 1;

					if( *hex_ptr != '0' )
					{
						goto get_ok;	
					}else
					{
						is_16_num = 1;
					}
				}else if( (tmp_word >= 'a' && tmp_word <= 'f')  || (tmp_word >= 'A' && tmp_word <= 'F') )
				{
					if( is_16_num != 1 )
						goto get_ok;	
				}else							
					goto get_ok;	
				
			}

			if( get_between_num >= max_num )
				return -2;
		}
		
	}

	return -1;

get_ok:

	*num_buf =  (char)NULL;
	return start + i;	
}

char * SOF_GetFileToBuf(char * file_name,int * file_length)
{
	char * file_buf = NULL;	
	FILE *fp=NULL;
	long fileOffset = 0;
	int rel;
	int i;

	fp = fopen(file_name,"rb");
	if( fp == NULL )
	{
		printf(" open test.txt error!\n");
		goto OP_ERROR;
	}

	rel = fseek(fp,0L,SEEK_END);
	if( rel != 0 )
	{
		printf("fseek error!!\n");
		goto OP_ERROR;
	}

	fileOffset = ftell(fp);
	if( fileOffset == -1 )
	{
		printf(" ftell error!\n");
		goto OP_ERROR;
	}
	
	rewind(fp);	

	/* if minihttp alive than kill it */
	if( fileOffset > 1 )
	{	
		file_buf = (char*)malloc(fileOffset + 1);
		if( file_buf == NULL )
		{
			printf("memory allow error\n");
			goto OP_ERROR;
		}
	
		rel = fread(file_buf,1,fileOffset,fp);
		if( rel != fileOffset )
		{
			printf(" fread Error!=n");
			goto OP_ERROR;
		}	

		file_buf[fileOffset] = '\0';	
		
	}else
	{
		
		printf(" fileOffset = %ld error\n",fileOffset);
		goto OP_ERROR;	
	}

	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}

	*file_length = fileOffset;

	return file_buf;

OP_ERROR:
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}

	if(file_buf)
	{
		free(file_buf);
		file_buf = NULL;
	}

	return NULL;
}

char *  SOF_FreeFileBuf(char * buf)
{
	if( buf)	
		free(buf);	

	return NULL;
}



int SOF_cf_open_file(char * file_name)
{	
	int rel = 0;
	int fileOffset = 0;	
	char word_buf[50] = "CONFIG_FILE HEADER\r\n"; 

	if( fp_config )
	{
		fclose(fp_config);
		fp_config = NULL;
	}

	fp_config = fopen(file_name, "rb");
	if (fp_config == NULL) 
	{
		fp_config = fopen(file_name, "wb");	
		if( fp_config == NULL )		
			goto ERROR_OP;
	}

	rel = fseek(fp_config,0L,SEEK_END);
	if( rel != 0 )
	{
		DPRINTK("fseek error!!\n");
		goto ERROR_OP;
	}

	fileOffset = ftell(fp_config);
	if( fileOffset == -1 )
	{
		DPRINTK(" ftell error!\n");
		goto ERROR_OP;
	}	

	if( fileOffset < 0 )
	{
		DPRINTK(" file len= %d err\n",fileOffset);
		goto ERROR_OP;
	}

	//DPRINTK(" file len= %d \n",fileOffset);

	config_file_size = fileOffset;

	if( config_buf )
	{
		free(config_buf);
		config_buf = NULL;
	}

	if( config_buf == NULL &&  fileOffset > 0  )
	{
		config_buf = (char*)malloc(fileOffset);
		if( config_buf == NULL )
		{
			DPRINTK(" config_buf == NULL  err\n");
			goto ERROR_OP;
		}

		rewind(fp_config);	

		rel = fread(config_buf,1,fileOffset,fp_config);
		if( rel != fileOffset )
		{
			DPRINTK(" Fread error!\n");		
			goto ERROR_OP;
		}	
	}


	if( fp_config )
	{
		fclose(fp_config);
		fp_config = NULL;
	}


	return 0;

ERROR_OP:

	if( fp_config )
	{
		fclose(fp_config);
		fp_config = NULL;
	}	

	config_file_size = 0;

	return -1;
}

int SOF_cf_close_file(char * file_name)
{
	if( fp_config )
	{
		fclose(fp_config);
		fp_config = NULL;
	}

	if( config_buf )
	{
		free(config_buf);
		config_buf = NULL;
	}

	return 1;
}

int SOF_cf_read_value(char * file_name,char * value_descriptors,char * value)
{
	int ret = -1;
	char find_buf[100] = "";
	char tmp[150] ="";
	int find1 = 0;
	int find2 = 0;

	//DPRINTK("1\n");

	ret = SOF_cf_open_file(file_name);
	if(ret < 0 )
		goto OP_END;

	//DPRINTK("2\n");

	sprintf(find_buf,"%s=",value_descriptors);

	find1 = SOF_FindSTR(config_buf,find_buf,0);
	if( find1 < 0   )	
	{
		ret = -1;
		goto OP_END;
	}

	//DPRINTK("3\n");

	find2 = SOF_FindSTR(config_buf,"\x0A",find1 +1);
	if( find2 < 0   )	
	{
		ret = -1;
		goto OP_END;
	}

	//DPRINTK("4\n");


	ret = SOF_GetBetweenWords(config_buf,tmp,find1+strlen(find_buf) -1,find2,100);
	if( ret > 0)	
	{
		strcpy(value,tmp);
	}

	//DPRINTK("5\n");

	//DPRINTK("get: %s %s\n",value_descriptors,value);

	ret = 1;

OP_END:

	SOF_cf_close_file(NULL);
	
	return ret;
}

int SOF_cf_write_value(char * file_name,char * value_descriptors,char * value)
{
	int ret = 0;
	char find_buf[100] = "";
	char tmp[256] ="";
	int find1 = 0;
	int find2 = 0;
	int write_num = 0;
	FILE * fp = NULL;

	ret = SOF_cf_open_file(file_name);
	if(ret < 0 )
		goto OP_END;

	//DPRINTK("1\n");
	//printf("config_buf:\n %s\n",config_buf);

	sprintf(find_buf,"%s=",value_descriptors);

	find1 = SOF_FindSTR(config_buf,find_buf,0);
	if( find1 >= 0 )
	{
		find2 = SOF_FindSTR(config_buf,"\x0A",find1 +1);
	}

	if( find1 < 0)
	{
		//printf("config_buf:\n %s\n",config_buf);
		//printf("find_buf:\n %s\n",find_buf);
	}

	//DPRINTK("2 %d %d\n",find1,find2);

	fp= fopen(file_name, "wb");
	if( fp == NULL )
	{
		ret = -1;
		DPRINTK(" create file %s error!\n",file_name);		
		goto OP_END;
	}

	if( find1 >=0 && find2 >=0 )
	{
		write_num = find1;

		//DPRINTK("3  %d\n",write_num);

		if( write_num > 0 )
		{
			ret= fwrite(config_buf,1,write_num,fp);
			if( ret != write_num )
			{
				DPRINTK(" fwrite Error!\n");		
				goto OP_END;
			}
		}

		//DPRINTK("5\n");

		sprintf(find_buf,"%s=%s\r\n",value_descriptors,value);

		ret= fwrite(find_buf,1,strlen(find_buf),fp);
		if( ret != strlen(find_buf) )
		{
			DPRINTK(" fwrite Error!\n");		
			goto OP_END;
		}

		//DPRINTK("6\n");

		write_num = config_file_size - (find2+1);

		if( write_num > 0 )
		{
			ret= fwrite(&config_buf[find2+1],1,write_num,fp);
			if( ret != write_num )
			{
				DPRINTK(" fwrite Error!\n");		
				goto OP_END;
			}
		}

		//DPRINTK("7\n");
	}else
	{
		if( config_file_size > 0 )
		{
			ret= fwrite(config_buf,1,config_file_size,fp);
			if( ret != config_file_size )
			{
				DPRINTK(" fwrite Error!\n");		
				goto OP_END;
			}
		}


		sprintf(find_buf,"%s=%s\r\n",value_descriptors,value);
		ret= fwrite(find_buf,1,strlen(find_buf),fp);
		if( ret != strlen(find_buf) )
		{
			DPRINTK(" fwrite Error!\n");		
			goto OP_END;
		}
	}

	//DPRINTK("4\n");

	ret = 1;

	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	SOF_cf_close_file(NULL);

	return 1;

OP_END:

	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	SOF_cf_close_file(NULL);
	
	return -1;
}


int SOF_Is_file_exist(char * file_name)
{
	FILE * fp;

	fp = fopen(file_name,"rb");

	if( fp == NULL )
		return -1;

	fclose(fp);

	return 1;
}

