#include <stdio.h>
#include <stdlib.h>

#include "config_file.h"

#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif

static FILE * fp_config = NULL;
static char  * config_buf = NULL;
static int config_file_size = 0;
pthread_mutex_t config_control_mutex;
static int config_control_mutex_init = 0;

static int FindSTR( char  * byhtml, char *cfind, int nStartPos)
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
static  int GetTmpLink(char * byhtml, char * temp, int start, int end,int max_num)
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

int cf_open_file(char * file_name)
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

int cf_close_file(char * file_name)
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
}

int cf_read_value(char * file_name,char * value_descriptors,char * value)
{
	int ret = -1;
	char find_buf[100] = "";
	char tmp[150] ="";
	int find1 = 0;
	int find2 = 0;
	int i = 0;
	int len = 0;

	if( config_control_mutex_init == 0 )
	{
		pthread_mutex_init(&config_control_mutex,NULL);
		config_control_mutex_init = 1;
	}

	pthread_mutex_lock( &config_control_mutex);
	
	ret = cf_open_file(file_name);
	if(ret < 0 )
		goto OP_END;

	sprintf(find_buf,"%s=",value_descriptors);	

	find1 = FindSTR(config_buf,find_buf,0);
	//DPRINTK("find %s %d\n",find_buf,find1);
	if( find1 < 0   )	
	{
		ret = -1;
		goto OP_END;
	}	
	
	//find2 = FindSTR(config_buf,"\r",find1 +1);  //回车符
	//if( find2 < 0   )	
	{
		find2 = FindSTR(config_buf,"\n",find1 +1);  //换行符
		if( find2 < 0   )	
		{
			ret = -1;
			goto OP_END;
		}		
	}		

	
	
	ret = GetTmpLink(config_buf,tmp,find1+strlen(find_buf) -1,find2,100);
	if( ret > 0)	
	{
		strcpy(value,tmp);
	}

	//DPRINTK("find: %d %d %s  %s\n",find1+strlen(find_buf) -1,find2,tmp,value);

	len = strlen(value);

	//DPRINTK("len=%d\n",len);

	for( i = 0; i < len; i++)
	{
		if( value[i] == 0x0D || value[i] == 0x0A )
		{
			value[i] = 0x00;
			DPRINTK("[i]=0x00\n",i);
		}
	}

	DPRINTK("get: %s %s\n",value_descriptors,value);

	ret = 1;
	
OP_END:
	pthread_mutex_unlock(&config_control_mutex);
	
	return ret;
}

int cf_write_value(char * file_name,char * value_descriptors,char * value)
{
	int ret = 0;
	char find_buf[100] = "";
	char tmp[256] ="";
	int find1 = 0;
	int find2 = 0;
	int write_num = 0;
	FILE * fp = NULL;

	if( config_control_mutex_init == 0 )
	{
		pthread_mutex_init(&config_control_mutex,NULL);
		config_control_mutex_init = 1;
	}

	pthread_mutex_lock( &config_control_mutex);
	
	ret = cf_open_file(file_name);
	if(ret < 0 )
		goto OP_END;

	DPRINTK("write: %s=%s\n",value_descriptors,value);
	//printf("config_buf:\n %s\n",config_buf);

	sprintf(find_buf,"%s=",value_descriptors);

	find1 = FindSTR(config_buf,find_buf,0);
	if( find1 >= 0 )
	{
		find2 = FindSTR(config_buf,"\x0A",find1 +1);
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

		sprintf(find_buf,"%s=%s\n",value_descriptors,value);

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


		sprintf(find_buf,"%s=%s\n",value_descriptors,value);
		ret= fwrite(find_buf,1,strlen(find_buf),fp);
		if( ret != strlen(find_buf) )
		{
			DPRINTK(" fwrite Error!\n");		
			goto OP_END;
		}
	}

	//DPRINTK("4\n");

	DPRINTK("set: %s %s\n",value_descriptors,value);
	 
	ret = 1;

	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	pthread_mutex_unlock(&config_control_mutex);

	return 1;
	
OP_END:

	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	pthread_mutex_unlock(&config_control_mutex);
	return -1;
}
 

