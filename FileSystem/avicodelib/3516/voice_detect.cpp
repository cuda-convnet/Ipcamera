//#include "stdafx.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "math.h"
#include <stdlib.h>
#include <string.h>
#include "voice_detect.h"

#ifdef __cplusplus
}
#endif

/*
#define NO_DEBUG_INFO 1

#ifndef _WIN32
	#ifndef NO_DEBUG_INFO
	#define TRACE printf
	#define AfxMessageBox printf
	#else
	#define TRACE(fmt, args...)   
	#define AfxMessageBox(fmt, args...)   
	#endif
#endif
*/

#define TRACE(fmt, args...)   
#define AfxMessageBox(fmt, args...) 


#define ONE_PCM_LEN  VD_WAVE_CHECKK_POINT
#define ONE_PCM_BITWIDTH (2)

//#define ONE_WORD_WAVE_POINT_NUM (44100/20)
#define ONE_WORD_WAVE_POINT_NUM (32000/20)

#define VD_WAVE_CHECKK_POINT (320)

#define MAX_SAVE_CODE_NUM (600)

typedef struct _CODE_SAVE_ST_
{
	int code_value[10];
	int code_num[10];
	int code_confirm_code_value;
	int code_confirm;
	int code_already_save_num;
}CODE_SAVE_ST;



static int code_result_array[10000];
static int insert_code_num = 0;
static int detect_start_audio_data = 0;
static int detect_end_audio_data = 0;
static char verfy_code_result_string[256] = "";


int VD_insert_code_array(int code)
{

	if( insert_code_num >= 9001)
		goto end;

	code_result_array[insert_code_num] = code;
	insert_code_num++;

end:
	return insert_code_num;
}

int VD_reset_insert_code_array()
{
	insert_code_num = 0;
	return 1;
}

float VD_CheckDataTime(short* pBuf,int iLen)
{
	int i = 0;
	int j = 0;
	int max = 0;
	int min = 0;
	float code = 0;
	int audio_data_count = 0;
	int total_value = 0;
	unsigned char between_mountain_array[ONE_PCM_LEN*5];
	int availabe_data_count = 0;
	int calculate_count = 0;
	int last_mount_count = 0;
	short * pData = NULL;
	int iDataLen = iLen /ONE_PCM_BITWIDTH;
	static int allow_show = 0;
	static int check_time = 0;
	int big_range_voice = 0;
	float code_value[11] = { 13.400, 12.300, 11.500, 10.857, 10.333, 9.733, 9.312, 8.937, 8.529, 8.222, 15.500};  // 0 - 10  10表示分割附

	//TRACE("=================== [%d] \n",check_time);	

	check_time++;
/*
	for( i = 0; i < iDataLen /WAVE_CHECKK_POINT; i++)
	{
		pData = &pBuf[WAVE_CHECKK_POINT*i];

		for( j = 0; j < WAVE_CHECKK_POINT; j++ )
		{
			//if(j % 10 ==  0)
			//	TRACE("\n");

			//TRACE("%0.5d ",pData[j]);			

			if( audio_data_count > 30 )
			{
				audio_data_count = 0;
				max = 0;
				min = 0;
			}

			if( pData[j] > max )
				max = pData[j];

			if( pData[j] < min )
				min = pData[j];

			if( audio_data_count >= 30 && abs(max - min) < 3000 )
			{
				//if(allow_show == 1)
				{
					//TRACE("max=%d min=%d max-min=%d < 5000  i=%d\n",max,min,max - min,i);
					allow_show = 0;
				}
				
				return -1;
			}

			audio_data_count++;
		}
	}
*/
	//TRACE("\n");

	for( i = 5; i < iDataLen - 5; i++) //掐掉头尾两个数据点。
	{

		if( pBuf[i] > 25000 )
			big_range_voice = 1;


		if( big_range_voice == 1)
		{
			if( (pBuf[i] >=0 ) &&  ( pBuf[i + 1] < 0 ) )
			{
				 if( last_mount_count == 0 )
				 {
					 last_mount_count = i;
				 }else
				 {
					 if( i - last_mount_count < 3 )
						continue;

					/* if( i - last_mount_count > 20 )
						continue;*/
					
					 between_mountain_array[availabe_data_count] = i - last_mount_count;
					 availabe_data_count++;

					 last_mount_count = i;
				 }
			}

		}else
		{
			if( (pBuf[i] >= pBuf[i - 1]) &&  (pBuf[i] >= pBuf[i - 2])  &&  (pBuf[i] >= pBuf[i - 3]) &&  (pBuf[i] >= pBuf[i - 4])
			 && (pBuf[i] >= pBuf[i + 1]) &&  (pBuf[i] >= pBuf[i + 2]) &&  (pBuf[i] >= pBuf[i + 3])  &&  (pBuf[i] >= pBuf[i + 4]) )		
			 {
				 //TRACE("i = %d  %d  last_mount_count=%d\n",i,pBuf[i],last_mount_count);

				 if( last_mount_count == 0 )
				 {
					 last_mount_count = i;
				 }else
				 {
					 if( i - last_mount_count < 3 )
						continue;

					/* if( i - last_mount_count > 20 )
						continue;*/
					
					 between_mountain_array[availabe_data_count] = i - last_mount_count;
					 availabe_data_count++;

					 last_mount_count = i;
				 }
			 }
		}

		 
	}

	

	for( i = 0; i < availabe_data_count; i++ )
	{
		total_value += between_mountain_array[i];
		calculate_count++;	
	}

	

	
	{
		code = (float)total_value / availabe_data_count;		

		min = 1000;
		max = 0;

		for( i = 0; i < availabe_data_count; i++ )
		{
			if( between_mountain_array[i] > max )
				max = between_mountain_array[i];

			if( between_mountain_array[i] < min )
				min = between_mountain_array[i];			
		}

		if( availabe_data_count < 2 )
			return -1;

		if( abs(max - min) > 4 )
			return -1;

		allow_show = 1;

	/*	TRACE("\nGet available value num %d  code=%f\n",availabe_data_count,code);

		for( i = 0; i < availabe_data_count; i++ )
		{
			TRACE("%d ",between_mountain_array[i]);
		}
		TRACE("\n");*/

	}


	for( i = 0; i < 11; i++)
	{
		if( code < 10)
		{
			if( (code <= code_value[i] + 0.12) 
				&&  (code >=  code_value[i] - 0.12)  )
			{
				code = i;
				return code;
			}
		}else
		{
			if( (code <= code_value[i] + 0.40) 
				&&  (code >=  code_value[i] - 0.40)  )
			{
				code = i;

			/*	if( i  == 10 )
				{
					TRACE("\nGet available value num %d  code=%f\n",availabe_data_count,code);

					for( i = 0; i < availabe_data_count; i++ )
					{
						TRACE("%d ",between_mountain_array[i]);
					}
				}*/
				return code;
			}
		}
		
	}


	return -1;
}

int VD_DetectSoundInfo(char * pBuf,int iLen)
{
	int iCode = -1;

	if( iLen !=  VD_WAVE_CHECKK_POINT*2 )
	{
		TRACE("iLen =%d != VD_WAVE_CHECKK_POINT*2  error!\n");
		goto err;
	}	
	
	iCode = VD_CheckDataTime((short*)pBuf,iLen);

	/*if(iCode >= 0 )
	TRACE("code = %d \n",iCode);	*/

	return iCode;
err:
	return -2;
}


int VD_verify_code_array_value(int * code_value_array,int total_num)
{
	char recv_code[255] = {0};
	int recv_code_num = 0;
	int a,b,c;
	int i,j;
	int code_verify_value;
	char combine_str[200] = "";
	char verify_code[100] = "";
	int len = 0;
	int verify_value = 0; 

	for( i = 4; i < total_num; i++)
	{
		TRACE("%d",code_value_array[i-4]);

		if( (i-4) % 2 == 1)
		{
			TRACE("(%c) ",code_value_array[i-4 - 1]*10 + code_value_array[i-4] + 32);
		}
	}

	TRACE("\n");

	for( i = 0; i < total_num - 8; i++ )
	{				

		if( i % 2 == 0)						
			a = code_value_array[i];						

		if( i % 2 == 1 )
		{
			b = code_value_array[i];

			c = a*10 + b;

			recv_code[recv_code_num] = c + 32;
			recv_code_num++;
		}			
	}

	recv_code[recv_code_num] = 0; 

	code_verify_value = code_value_array[total_num - 8]*1000 + code_value_array[total_num - 7]*100 + \
		code_value_array[total_num - 6]*10 + code_value_array[total_num - 5];


	memset(combine_str,0x00,200);
	memcpy(combine_str,recv_code,recv_code_num);		

	len = strlen(combine_str);
	for( i = 0; i < len; i++)
	{
		verify_value += combine_str[i];		
	}

	verify_value = verify_value % 10000;	

	TRACE("combine_str=%s\n",combine_str);

	if(verify_value == code_verify_value)
	{

		TRACE("verify code  %d - %d\n",verify_value,code_verify_value);		
		//AfxMessageBox(combine_str);
		strcpy(verfy_code_result_string,combine_str);
		return 0;
	}		

	return -1;
}


int VD_verify_code_value(int * code_value_array,CODE_SAVE_ST * pSaveArray,int start_offset,int total_num)
{
	int i = 0,j = 0,h,k;
	CODE_SAVE_ST * pcode_save = NULL;
	int have_no_confirm_code = 0;
	int code_verify_value = 0;	

	for( i = start_offset; i < total_num; i++)
	{
		pcode_save = &pSaveArray[i];
		if( pcode_save->code_confirm == 0 )
		{

			for( j = 0; j < pcode_save->code_already_save_num; j++)
			{
				code_value_array[i-4] = pcode_save->code_value[j];

				have_no_confirm_code = 1;

				//TRACE("0000 i=%d  j=%d \n",i,j);

				//TRACE("start_offset = %d  [%d]=%d\n",start_offset,i,pcode_save->code_value[j]);

				if( VD_verify_code_value(code_value_array,pSaveArray,i+1,total_num) == 0 )
					return 0;
				else
				{
					TRACE("start_offset = %d  [%d]=%d\n",start_offset,i,pcode_save->code_value[j]);
				}
			}
		}
	}

	//TRACE("1111 i=%d  j=%d \n",i,j);

	if( have_no_confirm_code == 0)
	{	
		if( VD_verify_code_array_value(code_value_array,total_num) == 0 )
			return 0;
	}
	return -1;
}



int VD_StartAnalysisCode()
{
	int i,j,x,z;
	int change = 0;
	int a,b,c;	
	int code;
	char recv_code[255] = {0};
	int recv_code_num = 0;
	int code_start = -1;
	int code_end = -1;
	int have_whole_code = 0;
	int start_code_offset = -1;
	int end_code_offset = -1;
	int word_in_data = 0;
	float every_word_num = ((float)ONE_WORD_WAVE_POINT_NUM*2)/VD_WAVE_CHECKK_POINT; //因为播放时是按照22050播放的，而录音确是按44100采集的。所以 ONE_WORD_WAVE_POINT_NUM*2 才录音采样后的一个字符的长度。
	float i_every_word_num = 0;
	CODE_SAVE_ST code_save[MAX_SAVE_CODE_NUM];
	CODE_SAVE_ST * pcode_save = NULL;
	int code_save_num = 0;
	int confirm_word = 0;
	int index = 0;
	int not_find_code_value_count = 0;
	int not_confirm_code_index[MAX_SAVE_CODE_NUM];
	int not_confirm_code_num = 0;
	int confirm_code_value_array[MAX_SAVE_CODE_NUM];

	memset(code_save,0x00,sizeof(CODE_SAVE_ST)*MAX_SAVE_CODE_NUM);

	i_every_word_num = every_word_num *100;

	TRACE("====== insert_code_num = %d\n",insert_code_num);

	if( insert_code_num <= 0 )
		goto err;

	for( i = 0; i < insert_code_num; i++ )
	{
		if( i % 10 == 0 )
			TRACE("\n");

		TRACE("[%d] %d ",i,code_result_array[i]);		
	}

	TRACE("\n");

	//查找开头指示
	for( i = 0; i < insert_code_num; i++ )
	{
		if( code_result_array[i] == 10 )
		{
			TRACE("Find start code [%d] %d ",i,code_result_array[i]);
			start_code_offset = i + 1; 
			break;
		}			
	}

	if( start_code_offset >= 0)
	{	
		i_every_word_num = every_word_num * (word_in_data + 1);
		for( i = start_code_offset; i < insert_code_num; i++ )
		{
			if( word_in_data >= MAX_SAVE_CODE_NUM - 1)
				break;

			if( i - start_code_offset >= i_every_word_num   )
			{
				TRACE("\nWord Num = %d   (%d)-(%f)\n",word_in_data,i - start_code_offset,i_every_word_num);
				word_in_data++;
				i_every_word_num = every_word_num * (word_in_data + 1);
			}


			if( i % 10 == 0 )
				TRACE("\n");

			TRACE("[%d] %d ",i,code_result_array[i]);	

			if(code_result_array[i] == -1 )
				continue;

			pcode_save = &code_save[word_in_data];

			if(pcode_save->code_already_save_num == 0 )
			{
				pcode_save->code_value[pcode_save->code_already_save_num] = code_result_array[i];
				pcode_save->code_num[pcode_save->code_already_save_num]++; 
				pcode_save->code_already_save_num++;
			}else
			{
				int have_code = 0;

				for(j = 0; j < pcode_save->code_already_save_num; j++)
				{
					if( pcode_save->code_value[j] == code_result_array[i])
					{
						have_code = 1;
						break;
					}
				}

				if( have_code )
				{
					pcode_save->code_num[j]++; 
				}else
				{
					pcode_save->code_value[pcode_save->code_already_save_num] = code_result_array[i];
					pcode_save->code_num[pcode_save->code_already_save_num]++; 
					pcode_save->code_already_save_num++;
				}
			}
		}
	}

	//查找结束符
	for( i = 4; i < word_in_data; i++ )
	{
		pcode_save = &code_save[i];

		for( j = 0; j < pcode_save->code_already_save_num;j++ )
		{
			if(pcode_save->code_value[j] == 10 && pcode_save->code_num[j] > 8 )
			{
				end_code_offset = i - 2; //结束符前的静音。
				goto search_end;
			}
		}		
	}

search_end:

	word_in_data = end_code_offset;


	TRACE("\n word_in_data =%d\n",word_in_data);

	if( word_in_data < 4)
	{
		goto err;
	}

	for( i = 0; i < word_in_data; i++ )
	{
		pcode_save = &code_save[i];
		if(pcode_save->code_already_save_num == 0)
			continue;

		TRACE("[%d] ",i);

		for( j = 0; j < pcode_save->code_already_save_num;j++ )
		{
			TRACE("%d(%d) ",pcode_save->code_value[j],pcode_save->code_num[j]);
		}

		TRACE("\n");
	}

	TRACE("decode code value:\n");

	for( i = 4; i < word_in_data; i++ )
	{		

		pcode_save = &code_save[i];
		if(pcode_save->code_already_save_num == 0)
			continue;

		TRACE(" [%d]",i);

		for( j = 0; j < pcode_save->code_already_save_num;j++ )
		{
			if( pcode_save->code_num[j] >= 15 )
			{
				pcode_save->code_confirm_code_value = pcode_save->code_value[j];
				pcode_save->code_confirm = 1;


				break;
			}else
			{
				if(  pcode_save->code_already_save_num == 1)
				{
					pcode_save->code_confirm_code_value = pcode_save->code_value[j];
					pcode_save->code_confirm = 1;
					break;
				}
			}
		}

		if( pcode_save->code_confirm )
			TRACE("%d",pcode_save->code_confirm_code_value);
		else
			TRACE("*",pcode_save->code_value[j]);

	}

	TRACE("\n");

	//确定是否有word没有给检测出来

	for(i = 4; i < word_in_data; i++)
	{	
		pcode_save = &code_save[i];
		if( pcode_save->code_already_save_num == 0)
		{
			//未检测到的code_value,对它进行多样性赋值，方便后面的整体验证求值
			for(j = 0; j < 10; j++)
			{
				pcode_save->code_value[j] = j;
				pcode_save->code_num[j] = 1;
				pcode_save->code_already_save_num++;
			}
			TRACE("[%d] value guess!\n",i);	

			not_confirm_code_index[not_find_code_value_count] = i;
			not_find_code_value_count++;
		}
	}

	if( not_find_code_value_count >= 2 ) //如果有2个以上的code_value没有检测出来，就无法解码
	{
		TRACE("not_find_code_value_count=%d >= 2,can't decode!\n",not_find_code_value_count);
		goto err;
	}

	for(i = 4; i < word_in_data; i++)
	{	
		pcode_save = &code_save[i];
		if(pcode_save->code_confirm == 0 )
		{
			not_confirm_code_num++;
		}
	}



	//if( code_save[word_in_data - 1].code_confirm == 0 || code_save[word_in_data - 2].code_confirm == 0 )
	//{
	//	TRACE("Verify code error,can't decode!\n",not_confirm_code_num);
	//	return 0;
	//}


	for(i = 4; i < word_in_data; i++)
	{
		pcode_save = &code_save[i];
		if(pcode_save->code_confirm == 1 )
			confirm_code_value_array[i-4] = pcode_save->code_confirm_code_value;	
		else
		{
			int max = 0;
			int code_value  = 0;
			for(j = 0; j < pcode_save->code_already_save_num; j++ )
			{
				if( max < pcode_save->code_num[j] )
				{
					max = pcode_save->code_num[j];
					code_value =  pcode_save->code_value[j];
				}
			}

			confirm_code_value_array[i-4] = code_value;
		}
	}

	if( VD_verify_code_array_value(confirm_code_value_array,word_in_data) == 0 )
		return 0;


	if(not_confirm_code_num > 2 )
	{
		TRACE("not_confirm_code_num=%d >= 2,can't decode!\n",not_confirm_code_num);
		goto err;
	}

	VD_verify_code_value(confirm_code_value_array,code_save,4,word_in_data);

	return 0;

err:
	return -1;
}

int VD_GetDataFromAudioPcm(char * pAudioBuf,int iAudioLen,char * pSaveDetectData,int iSaveDataLen)
{
	int code = -1;
	int save_num = 0;
	int get_need_data = 0;
	int clear_array = 0;
	static int end_code_num = 0;
	static int end_code_miss_num = 0;
	static int not_detect_code_num = 0;

	if( end_code_miss_num > 0 )
	{
		end_code_miss_num--;
		goto err;
	}

	code = VD_DetectSoundInfo(pAudioBuf,iAudioLen);		
	if( code == -2)
		goto err;

	if( code == 10 ) //检查是否收到开始数据。
	{
		if( detect_start_audio_data == 0)
		{
			detect_start_audio_data = 1;
			detect_end_audio_data = 0;
			end_code_num = 0;			
			VD_reset_insert_code_array(); //清空之前的数据

			TRACE("Find start code! detect_start_audio_data= %d \n",detect_start_audio_data);
		}
	}	

	save_num = VD_insert_code_array(code);

	/*if( save_num % 10 == 0)
		TRACE("save_num == %d\n",save_num);*/

	if(save_num > 100 )  //避开音频开始标志。
	{
		if( code == 10 )
		{
			detect_end_audio_data = 1;			
		}

		if( detect_end_audio_data == 1)
		{
			end_code_num++;
			if( end_code_num > 60 )
			{
				TRACE("Find end code!\n");

				if( VD_StartAnalysisCode() < 0 )
				{

					TRACE("VD_StartAnalysisCode err!\n");
				}else
				{
					if( iSaveDataLen > strlen(verfy_code_result_string)+1 )
					{
						strcpy(pSaveDetectData,verfy_code_result_string);
						get_need_data = 1;
					}				
				}

				VD_reset_insert_code_array(); //清空之前的数据	
				detect_start_audio_data = 0; //即使有开始检测标志，也要重置
				detect_end_audio_data = 0;
				end_code_miss_num = 80;
			}
		}
	}

	if( save_num >= 200 )
	{

		if( code == -1 )  
			not_detect_code_num++;
		else
			not_detect_code_num = 0;

		if( not_detect_code_num > 200 )  //一定时间不能检测到 code 数据，就复位。
		{
			clear_array = 1;
			not_detect_code_num = 0;
		}

		if( clear_array == 1)
		{
			//TRACE("save_num=%d  start_detect_audio_data = %d not detect audio_data,clear code array!\n",save_num ,detect_start_audio_data);
			VD_reset_insert_code_array(); //清空之前的数据	
			detect_start_audio_data = 0; //即使有开始检测标志，也要重置
			detect_end_audio_data = 0;
		}		
	}

	if(get_need_data == 1)
		return 0;
err:
	return -1;
}