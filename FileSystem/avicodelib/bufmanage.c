// BufManage.c : implementation of the BufManage class.
//
//////////////////////////////////////////////////////////////////////

#include <pthread.h>
#include "bufmanage.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

 


int init_buf_list(BUFLIST * list,int buf_max_size,int  buf_max_count)
{
	int i = 0;
	
	list->top = 0;
	list->bottom = 0;
	list->item_count = 0;
	list->buf_addr = NULL;
	list->inter_critical = 0;

	pthread_mutex_init(&list->control_mutex,NULL);

	if(buf_max_size <= 0)
		buf_max_size = BUF_MAX_SIZE;

	if( buf_max_count <= 0 )
		buf_max_count = MAX_BUF_NUM;

	if( buf_max_count > MAX_BUF_NUM )
	{
		DPRINTK("buf_max_count=%d  MAX_BUF_NUM=%d  Error\n",buf_max_count,MAX_BUF_NUM);
		return -1;
	}

	list->buf_addr = (char*)malloc(buf_max_size * buf_max_count);

	DPRINTK("init_buf_list = %d count =%d size=%d\n",buf_max_size * buf_max_count,buf_max_count,buf_max_size);

	if(list->buf_addr == NULL)
		return -1;

	for( i = 0 ; i < buf_max_count; i++ )
	{
		list->subbuf_addr[i] = list->buf_addr + buf_max_size * i;
	}

	list->max_length = buf_max_size;
	list->max_count  = buf_max_count;

	return 1;
}

int destroy_buf_list(BUFLIST * list)
{
	list->top = 0;
	list->bottom = 0;
	list->item_count = 0;
	list->inter_critical = 0;

	if( list->buf_addr )
	{		
		pthread_mutex_destroy( &list->control_mutex);
		free(list->buf_addr);
		list->buf_addr = NULL;
	}

	return 1;
}

int insert_data(BUF_FRAMEHEADER *pheadr,char * pdata,BUFLIST * list)
{
	char * buf = NULL;

	if( list->item_count >= list->max_count )
		return -1;

	if(  pheadr->iLength > list->max_length || pheadr->iLength <= 0)
	{
		DPRINTK("buf iLength = %ld  max length=%d,larger\n",pheadr->iLength,list->max_length);
		return -1;
	}

	pthread_mutex_lock( &list->control_mutex);

	list->inter_critical = 1;

	buf = list->subbuf_addr[list->top];

	memcpy(buf,(void*)pheadr,sizeof(BUF_FRAMEHEADER));

	buf += sizeof(BUF_FRAMEHEADER);

	memcpy(buf,(void*)pdata, pheadr->iLength);

	list->item_count++;

	list->top++;

	if( list->top >= list->max_count )
		list->top = 0;	


//	printf("insert index =%d length=%d frametype=%d top=%d bot=%d count=%d %d\n", 
//					pheadr->index,pheadr->iLength,pheadr->iFrameType, 
//					list->top,list->bottom,list->item_count,pheadr->type);

	pthread_mutex_unlock( &list->control_mutex);

	list->inter_critical = 0;


	return 1;
}

int get_one_buf(BUF_FRAMEHEADER * pheadr,char * pdata,BUFLIST * list)
{
	char * buf = NULL;

	if( list->item_count <= 0 )
		return -1;

	pthread_mutex_lock( &list->control_mutex);

	list->inter_critical = 1;


	buf = list->subbuf_addr[list->bottom];

	memcpy((void*)pheadr,buf,sizeof(BUF_FRAMEHEADER));

	buf += sizeof(BUF_FRAMEHEADER);

	memcpy((void*)pdata,buf, pheadr->iLength);

	list->item_count--;

	list->bottom++;

	if( list->bottom >= list->max_count )
		list->bottom = 0;

	//printf("put index =%d length=%d frametype=%d top=%d bot=%d count=%d\n",pheadr->index,pheadr->iLength,pheadr->iFrameType,list->top,list->bottom,list->item_count);

	pthread_mutex_unlock( &list->control_mutex);

	list->inter_critical = 0;

	
	return 1;
}


/*
FILE * fp_debug_header = NULL;
FILE * fp_debug_data = NULL;
int data_offset = 0;
*/

int CheckFrameHeader2(BUF_FRAMEHEADER * pHeader)
{
	// 后面的buf 中没有数据时，返头。
	if( (pHeader->type != VIDEO_FRAME) && (pHeader->type != AUDIO_FRAME) )
	{
		DPRINTK(" type = %ld\n",pHeader->type);
		return ERROR;	
	}

	
	if( (pHeader->index>= get_dvr_max_chan(1)) || (pHeader->index  <0) )
	{
		DPRINTK(" iChannel = %ld  type=%ld length=%ld sec=%ld usec=%ld\n",pHeader->index,
			pHeader->type,pHeader->iLength,pHeader->ulTimeSec,pHeader->ulTimeUsec);
		return ERROR;
	}

	
	if( (pHeader->iLength > (MAX_FRAME_BUF_SIZE - 10*1024)) || (pHeader->iLength  <= 0) )
	{
		DPRINTK(" ulDataLen = %ld\n",pHeader->iLength);
		return ERROR;
	}

	return ALLRIGHT;
}


GST_DRV_BUF_INFO * get_enc_buf_info(BUFLIST * list,char * pdata)
{
	BUF_FRAMEHEADER pheadr;

	int channel_mask = 0;

	if( is_enable_get_rec_data() == 0 )
	{		
		return NULL;
	}

	get_enc_buf(&pheadr,pdata);

	if( CheckFrameHeader2(&pheadr) == ERROR )	
		return NULL;	

	list->bufinfo.iFrameType[pheadr.index] = pheadr.iFrameType;
	list->bufinfo.iChannelMask = set_bit(channel_mask, pheadr.index);   // just have one channel
	list->bufinfo.tv.tv_sec = pheadr.ulTimeSec;
	list->bufinfo.tv.tv_usec = pheadr.ulTimeUsec;
	list->bufinfo.iBufLen[pheadr.index] = pheadr.iLength;
	list->bufinfo.iFrameCountNumber[pheadr.index] = pheadr.frame_num;
	list->bufinfo.iBufDataType = pheadr.type;

//	printf("2==  index=%d frametype=%d %d\n",pheadr.index,list->bufinfo.iFrameType[pheadr.index],pheadr.iLength);


	return &list->bufinfo;

}


GST_DRV_BUF_INFO * get_buf_info(BUFLIST * list,char * pdata)
{
	BUF_FRAMEHEADER pheadr;
//	char * buf = NULL;
	int channel_mask = 0;
//	FILEINFO stfile;
//	FRAMEHEADERINFO f_header;

	if( list->item_count <= 0 )
		return NULL;
	
	get_one_buf(&pheadr,pdata,list);	

	list->bufinfo.iFrameType[pheadr.index] = pheadr.iFrameType;
	list->bufinfo.iChannelMask = set_bit(channel_mask, pheadr.index);   // just have one channel
	list->bufinfo.tv.tv_sec = pheadr.ulTimeSec;
	list->bufinfo.tv.tv_usec = pheadr.ulTimeUsec;
	list->bufinfo.iBufLen[pheadr.index] = pheadr.iLength;
	list->bufinfo.iFrameCountNumber[pheadr.index] = pheadr.frame_num;
	list->bufinfo.iBufDataType = pheadr.type;

	return &list->bufinfo;

}

int list_to_list(BUFLIST * dest_list,BUFLIST * source_list)
{
	static unsigned char temp_list[NET_BUF_MAX_SIZE];
	BUF_FRAMEHEADER pheadr;
	int ret;

	ret = get_one_buf(&pheadr,temp_list,source_list);
	if(ret < 0 )
		return ret;

	if( Net_cam_get_support_512K_buf_flag() == 0 )
	{
		if( pheadr.iLength > (OLD_MAX_FRAME_BUF_SIZE - 5*1024))
		{
			DPRINTK("frame size %d too bigger lost it\n",pheadr.iLength );			
			return -1;
		}
	}else
	{
		if(pheadr.iLength  > (MAX_FRAME_BUF_SIZE - 5*1024))
		{
			DPRINTK("frame size %d too bigger lost it\n",pheadr.iLength );			
			return -1;
		}
	}	

	ret = insert_net_buf(&pheadr,temp_list);
	if(ret < 0 )
		return ret;

	return ALLRIGHT;
}

int get_enable_insert_data_num(BUFLIST * list)
{
	if( list->item_count < list->max_count )
		return (list->max_count - list->item_count);
	else
		return -1;
}

int enable_insert_data(BUFLIST * list)
{
	if( list->item_count < list->max_count )
		return 1;
	else
		return -1;
}

int enable_get_data(BUFLIST * list)
{
	if( list->item_count > 0 )
		return 1;
	else
		return -1;
}

int clear_buf(BUFLIST * list)
{
	pthread_mutex_lock( &list->control_mutex);
	list->top = 0;
	list->bottom = 0;
	list->item_count = 0;
	pthread_mutex_unlock( &list->control_mutex);
	list->inter_critical = 0;

	return 1;
}

//将(0-31)位置1
int set_bit(int iCam, int iBitSite)
{
	int value;

	value = 1;

	value = value << iBitSite;

	iCam = iCam |value;

	return iCam;
}

// 返回（0 - 31  ) 位的值，返回值为0 或1
int get_bit(int iCam, int iBitSite)
{
	iCam = iCam >> iBitSite;

	iCam = iCam & 0x01;

	return iCam;
}

