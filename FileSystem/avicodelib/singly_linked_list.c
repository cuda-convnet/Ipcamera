#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "singly_linked_list.h"
//#include "rjone_debug.h"

#include "BaseData.h"


#ifndef DPRINTK
#define DPRINTK(fmt, args...)	printf("(%s,%d)%s: " fmt,__FILE__,__LINE__, __FUNCTION__ , ## args)
#endif




int sll_del_list_tail_item_no_lock(SINGLY_LINKED_LIST_INFO_ST  * list_info);
int sll_init_list(SINGLY_LINKED_LIST_INFO_ST  * list_info,int max_item_num,int data_max_size)
{
	memset(list_info,0x00,sizeof(SINGLY_LINKED_LIST_INFO_ST));
	list_info->max_item_num = max_item_num;
	list_info->max_item_size = data_max_size;
	pthread_mutex_init(&list_info->lock, NULL);

	//list_info->seq_no = 5465486786887LL;
	//list_info->head_id = list_info->seq_no;
	//list_info->tail_id = list_info->seq_no;
	DPRINTK("data_max_size= %d  num=%d\n",data_max_size,max_item_num);
	return 1;
}

int sll_destroy_list(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{
	pthread_mutex_lock(&list_info->lock);	
	if(list_info->list != NULL)
	{
		SINGLY_LINKED_LIST_ST * del = NULL;
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		while( cur->next != NULL )
		{
			del = cur;
			cur = cur->next;

			#ifdef debug_log
			DPRINTK("del item id=%lld len=%d\n",del->id,del->len);
			#endif
			
			if(del->data)		
				free(del->data);				
			free(del);								
		}

		if( cur )
		{
			del = cur;
			#ifdef debug_log
			DPRINTK("del item id=%lld len=%d\n",del->id,del->len);
			#endif
			
			if(del->data)		
				free(del->data);				
			free(del);		
		}
		
	}

	pthread_mutex_unlock(&list_info->lock);	
	pthread_mutex_destroy(&list_info->lock);	
	memset(list_info,0x00,sizeof(SINGLY_LINKED_LIST_INFO_ST));
	return 1;
}

int  sll_add_list_item(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * data,int len,int offset)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;	
	int ret = -1;
	long long  add_id = 0;
	struct timeval sendStartTime;
	struct timeval sendEndTime;

	pthread_mutex_lock( &list_info->lock);

	//gettimeofday( &sendStartTime, NULL );	

	if( list_info->item_num + 1 > list_info->max_item_num )
	{
		sll_del_list_tail_item_no_lock(list_info);
		if( list_info->item_num + 1 > list_info->max_item_num )
		{	
			ret = SLL_ITEM_ENOUGH_ERR;
			DPRINTK("create err\n");
			goto err;
		}
	}	
		
	
	list_st = (SINGLY_LINKED_LIST_ST*)malloc(sizeof(SINGLY_LINKED_LIST_ST));
	if( list_st == NULL )
	{
		DPRINTK("malloc err\n");
		goto err;
	}

	memset(list_st,0x00,sizeof(SINGLY_LINKED_LIST_ST));

	list_st->data = (char*)malloc(len);
	if( list_st->data == NULL )
	{
		DPRINTK("malloc err\n");
		goto err;
	}
	
	memcpy(list_st->data + offset,data,len-offset);
	list_st->len = len;	
	
	list_st->id = list_info->seq_no;
	list_info->seq_no++;	
	list_info->head_id = list_st->id;
	list_info->item_num++;

	if(list_info->list == NULL )
	{
		list_info->list = list_st;		
	}else
	{
		if( list_info->pListHead != NULL )
		{
			list_info->pListHead->next =  list_st;
			list_info->pListHead =  list_st;
		}else
		{
			SINGLY_LINKED_LIST_ST * cur = list_info->list;
			DPRINTK("busy!\n");
			while( cur->next != NULL )
			{
				cur = cur->next;
			}

			cur->next = list_st;
			list_info->pListHead = list_st;
		}
	}	

	list_info->all_data_size += len;

	#ifdef debug_log
	DPRINTK("add item id=%lld len=%d\n",list_st->id,list_st->len);
	#endif

	add_id = list_st->id;

/*
	gettimeofday( &sendEndTime, NULL );
	 {
		 	struct timeval result;
			int time_usec =0;
			 result.tv_sec = sendEndTime.tv_sec - sendStartTime.tv_sec;     
        		 result.tv_usec = sendEndTime.tv_usec - sendStartTime.tv_usec;
			 time_usec = result.tv_sec * 1000000 + result.tv_usec;
			

			 if( time_usec > (1000000/60) )
			 {
			 	DPRINTK("send data timeout4:==============  length=%ld  %d\n",
							len,time_usec);	

			 }
			 
	 }
	 */

	pthread_mutex_unlock( &list_info->lock);	

	return 1;
err:

	if( list_st )
	{
		if(list_st->data)		
			free(list_st->data);
		
		free(list_st);		
	}

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}


int  sll_add_list_item_no_malloc(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * data,int len)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;	
	int ret = -1;
	long long add_id = 0;
	struct timeval sendStartTime;
	struct timeval sendEndTime;

	pthread_mutex_lock( &list_info->lock);

	//gettimeofday( &sendStartTime, NULL );

	if( list_info->item_num + 1 > list_info->max_item_num )
	{
		sll_del_list_tail_item_no_lock(list_info);
		if( list_info->item_num + 1 > list_info->max_item_num )
		{	
			free(data);		
			ret = SLL_ITEM_ENOUGH_ERR;
			DPRINTK("create err, free data\n");
			goto err;
		}
	}	
	
	list_st = (SINGLY_LINKED_LIST_ST*)malloc(sizeof(SINGLY_LINKED_LIST_ST));
	if( list_st == NULL )
	{
		DPRINTK("malloc err\n");
		goto err;
	}

	memset(list_st,0x00,sizeof(SINGLY_LINKED_LIST_ST));

	list_st->data = (char*)data;
	if( list_st->data == NULL )
	{
		DPRINTK("malloc err\n");
		goto err;
	}	
	
	list_st->len = len;	
	
	list_st->id = list_info->seq_no;
	list_info->seq_no++;	
	list_info->head_id = list_st->id;
	list_info->item_num++;	

	if(list_info->list == NULL )
	{
		list_info->list = list_st;		
	}else
	{
		if( list_info->pListHead != NULL )
		{
			list_info->pListHead->next =  list_st;
			list_info->pListHead =  list_st;
		}else
		{
			SINGLY_LINKED_LIST_ST * cur = list_info->list;
			while( cur->next != NULL )
			{
				cur = cur->next;
			}

			cur->next = list_st;
			list_info->pListHead = list_st;
		}
	}

	list_info->all_data_size += len;

	#ifdef debug_log
	DPRINTK("add item id=%lld len=%d\n",list_st->id,list_st->len);
	#endif

	add_id = list_st->id;

	pthread_mutex_unlock( &list_info->lock);	

	return 1;
err:

	if( list_st )
	{
		if(list_st->data)		
			free(list_st->data);
		
		free(list_st);		
	}

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}


int sll_del_list_tail_item(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;	
	int ret = 1;
	int data_len = 0;
	int real_free = 0;

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = 1;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		if( cur )
		{		
			#ifdef debug_log
			DPRINTK("del item id=%lld len=%d\n",cur->id,cur->len);
			#endif
		
			list_info->list = cur->next;

			data_len = cur->len;


			if( cur->user_num <= 0 )
			{				
				if(cur->data)		
					free(cur->data);

				free(cur);	

				real_free = 1;
			}else
			{
				list_info->all_not_free_data_size += data_len;
				DPRINTK("user_num=%d not free  id=%lld all_not_free_data_size=%d\n",cur->user_num,cur->id,list_info->all_not_free_data_size);
			}			
				
		}
	}		
	
	
	list_info->tail_id++;
	list_info->item_num--;

	if( real_free == 1 )
		list_info->all_data_size -= data_len;

	pthread_mutex_unlock( &list_info->lock);	

	return 1;
err:

	if( list_st )
	{
		if(list_st->data)		
			free(list_st->data);
		
		free(list_st);		
	}

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}



int sll_del_list_tail_item_no_lock(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;	
	int ret = 1;
	int data_len = 0;
	int real_free = 0;	

	if( list_info->item_num - 1 < 0 )
	{
		ret = 1;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		if( cur )
		{		
			#ifdef debug_log
			DPRINTK("del item id=%lld len=%d\n",cur->id,cur->len);
			#endif
		
			list_info->list = cur->next;

			data_len = cur->len;


			if( cur->user_num <= 0 )
			{				
				if(cur->data)		
					free(cur->data);

				free(cur);	

				real_free = 1;
			}else
			{
				list_info->all_not_free_data_size += data_len;
				DPRINTK("user_num=%d not free  id=%lld  all_not_free_data_size=%d\n",cur->user_num,cur->id,list_info->all_not_free_data_size);
			}			
				
		}
	}		
	
	
	list_info->tail_id++;
	list_info->item_num--;

	if( real_free == 1 )
		list_info->all_data_size -= data_len;

	return 1;
err:

	if( list_st )
	{
		if(list_st->data)		
			free(list_st->data);
		
		free(list_st);		
	}	
	return ret;
}



int sll_get_list_item(SINGLY_LINKED_LIST_INFO_ST  * list_info,char * dest_buf,int dest_max_len,long long item_id)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;		
	int ret = 0;	

	DPRINTK("11\n");

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		int find_available_item = 0;	
		
		if( cur->id == item_id )
		{
			find_available_item = 1;
		}else
		{
			while( cur->next != NULL )
			{
				cur = cur->next;
				
				if( cur->id == item_id )
				{
					find_available_item = 1;
					break;
				}
			}
		}	

		if( find_available_item == 0 )
		{
			ret = SLL_ITEM_EMPTY_ERR;		
			goto err;
		}

		if( dest_max_len < cur->len )
		{
			ret = SLL_ITEM_BUF_NOT_ENOUGH_ERR;		
			goto err;
		}

		memcpy(dest_buf,cur->data,cur->len);

		ret = cur->len;		

		#ifdef debug_log
		DPRINTK("get item id=%d len=%d\n",cur->id,cur->len);
		#endif
	}		


	pthread_mutex_unlock( &list_info->lock);	

	return ret;
err:

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}



int sll_get_last_key_frame_list_item_no(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;		
	int ret = 0;	

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		int find_available_item = 0;
		char * h264_data_buf = NULL;
		int last_frame_id = 0;
		
		
		while( cur->next != NULL )
		{
			cur = cur->next;
			
			if( cur->data != NULL )
			{
				h264_data_buf = cur->data  + 36;				

				if(h264_data_buf[0] ==0x0 && h264_data_buf[1] ==0x0 && h264_data_buf[2] ==0x0 && h264_data_buf[3] ==0x1 &&  (h264_data_buf[4] & 0x0f)  == (0x67 & 0x0f))
				{
					DPRINTK("get last key frame   id=%d\n",cur->id);

					find_available_item = 1;
					last_frame_id =  cur->id;
				}				
			}
		}			

		if( find_available_item == 0 )
		{
			ret = SLL_ITEM_EMPTY_ERR;		
			goto err;
		}

		ret = last_frame_id;		
	}		


	pthread_mutex_unlock( &list_info->lock);	

	return ret;
err:

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}


#define VIDEO_FRAME 1
#define AUDIO_FRAME 2
#define DATA_FRAME  3



int sll_get_list_item_lock(SINGLY_LINKED_LIST_INFO_ST  * list_info,SINGLY_LINKED_GET_DATA_ST * get_data,long long item_id)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;		
	int ret = 0;		

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}	

	if(  item_id <  list_info->tail_id || item_id > list_info->head_id )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}

	if(list_info->list == NULL )
	{		
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		int find_available_item = 0;

		if(  get_data->id >= list_info->tail_id && get_data->id < list_info->head_id )
		{
			if( (get_data->id + 1 == item_id )  && get_data->list != NULL )
			{
				SINGLY_LINKED_LIST_ST * tmp = get_data->list->next;
				if(tmp)
				{
					if( tmp->id == item_id  )
					{
						cur = tmp;
						//DPRINTK("item_id =%d use\n",item_id);
					}
				}
			}
		}
		
		if( cur->id == item_id )
		{
			find_available_item = 1;
		}else
		{
			DPRINTK("busy id=%lld  %lld %lld-%lld\n",item_id,get_data->id,list_info->head_id,list_info->tail_id );
			while( cur->next != NULL )
			{
				cur = cur->next;
				
				if( cur->id == item_id )
				{
					find_available_item = 1;
					break;
				}
			}
		}	

		if( find_available_item == 0 )
		{
			ret = SLL_ITEM_EMPTY_ERR;		
			goto err;
		}		

		get_data->id = cur->id;
		get_data->len = cur->len;
		get_data->data = cur->data;
		get_data->list = cur;

		cur->user_num++;		
		ret = cur->len;		

		#ifdef debug_log
		DPRINTK("get item id=%d len=%d\n",cur->id,cur->len);
		#endif
	}		


	pthread_mutex_unlock( &list_info->lock);	

	return ret;
err:

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}




int sll_get_list_item_unlock(SINGLY_LINKED_LIST_INFO_ST  * list_info,SINGLY_LINKED_GET_DATA_ST * get_data)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;		
	int ret = 0;	
	long long item_id;

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;
		int find_available_item = 0;	

		item_id = get_data->id;

		if(  get_data->id >= list_info->tail_id && get_data->id <=  list_info->head_id )
		{			
			SINGLY_LINKED_LIST_ST * tmp = get_data->list;
			if(tmp)
			if( tmp->id == item_id )
			{
				cur = tmp;
				//DPRINTK("item_id =%d use\n",item_id);
			}			
		}else
		{
			goto num_not_in_arae;
		}
		
		if( cur->id == item_id )
		{
			find_available_item = 1;
		}else
		{
			DPRINTK("busy!\n");
			while( cur->next != NULL )
			{
				cur = cur->next;
				
				if( cur->id == item_id )
				{
					find_available_item = 1;
					break;
				}
			}
		}	

	num_not_in_arae:

		if( find_available_item == 0 )
		{
			ret = SLL_ITEM_EMPTY_ERR;

			DPRINTK("data not in array, free [%d]!user_num=%d\n",get_data->id,get_data->list->user_num);

			get_data->list->user_num--;

			if( get_data->list->user_num <= 0 )
			{					
				
				if(get_data->data)		
					free(get_data->data);

				free(get_data->list);
				get_data->list = NULL;	

				list_info->all_data_size -= get_data->len;		

				list_info->all_not_free_data_size -= get_data->len;

				DPRINTK("[%d] free data! all_not_free_data_size=%d\n",get_data->id,list_info->all_not_free_data_size);
			}
			
			goto err;
		}		

		cur->user_num--;		

		#ifdef debug_log
		DPRINTK("unlock item id=%d len=%d\n",cur->id,cur->len);
		#endif
	}		


	pthread_mutex_unlock( &list_info->lock);	

	return ret;
err:

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}


int sll_get_tail_list_item_no(SINGLY_LINKED_LIST_INFO_ST  * list_info,unsigned int * iListTailNo)
{
	SINGLY_LINKED_LIST_ST * list_st = NULL;		
	int ret = 0;	

	pthread_mutex_lock( &list_info->lock);	

	if( list_info->item_num - 1 < 0 )
	{
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}	

	if(list_info->list == NULL )
	{		
		ret = SLL_ITEM_EMPTY_ERR;		
		goto err;
	}else
	{
		SINGLY_LINKED_LIST_ST * cur = list_info->list;	
		*iListTailNo = cur->id;
	}	

	pthread_mutex_unlock( &list_info->lock);	

	return ret;
err:

	pthread_mutex_unlock( &list_info->lock);		
	return ret;
}


int sll_list_item_num(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{
	return list_info->item_num;
}

int sll_list_enable_add_item(SINGLY_LINKED_LIST_INFO_ST  * list_info)
{	

	if( list_info->all_data_size >= list_info->max_item_size )
	{
		//DPRINTK("%d >= %d\n",list_info->all_data_size, list_info->max_item_size);
		return 0;
	}

	if( list_info->item_num  + 2 < list_info->max_item_num )
		return 1;
	else
	{
		//DPRINTK("%d >= %d\n",list_info->item_num, list_info->max_item_num);
		return 0;
	}
}

