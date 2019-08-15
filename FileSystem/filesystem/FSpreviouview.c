#include "filesystem.h"
#include "FTC_common.h"
#include "saveplayctrl.h"

#ifdef hi3515
	#define PREVIEW_BUF_SIZE  (512*1024) //3515大于这个数 umount 函数将失灵
#else
	#define PREVIEW_BUF_SIZE  (512*1024)
#endif

PREVIEWBUF gPreviewBuf;
int g_PreviewSecond = 0;
char * dateBuf = NULL;
time_t g_previous_headtime = 0;
int  iFrameNum = 0;
unsigned long ulDateLength = 0;
unsigned long ulFrameOffset = 0;
unsigned long ulFileDataOffset = 0;
int iThreadStop = 0;
int g_preview_keyframe_count = 100;
int g_old_preview_keyframe_count = 100;
int g_have_keyframe_num = 800;

pthread_mutex_t PreviewMutex;

void  set_count_create_keyframe(int count)
{
	printf(" set_count_create_keyframe %d\n",count);
	g_preview_keyframe_count = count;
	g_old_preview_keyframe_count = count;
}

int get_save_frame_count()
{
	return g_preview_keyframe_count;
}


int InitPreviouView(int iSecond)
{
	static int iFirst = 1;

	if( iFirst != 1 )
		return ERROR;



	gPreviewBuf.PreviewBufStart = NULL;

	gPreviewBuf.PreviewBufStart = (char *)malloc(PREVIEW_BUF_SIZE);
	

	printf(" buf length %d\n",PREVIEW_BUF_SIZE);

	if( gPreviewBuf.PreviewBufStart == NULL )
		return NOENOUGHMEMORY;

	memset(gPreviewBuf.PreviewBufStart ,0,PREVIEW_BUF_SIZE);

//	printf("Set preview buf zero!\n");

	gPreviewBuf.lBufLength = PREVIEW_BUF_SIZE;

	gPreviewBuf.pOffSetHead = gPreviewBuf.PreviewBufStart;

	gPreviewBuf.pOffSetTail   = gPreviewBuf.PreviewBufStart;

	printf("Start Addr: %d\n",gPreviewBuf.PreviewBufStart);
	printf("tail   Addr: %d\n",gPreviewBuf.pOffSetTail);

	printf("preview time %d\n",iSecond);

	g_PreviewSecond = iSecond;

	dateBuf = (char *)malloc(1024*512);
	if( !dateBuf )
	{
		printf("dateBuf malloc error!\n");
		return ;
	}
	pthread_mutex_init(&PreviewMutex,NULL);

	iFirst = 0;

	return ALLRIGHT;
	
}

void FS_SetPreviewTime(int iSecond)
{
	g_PreviewSecond = iSecond;
}

int ReleasePreviewViewBuf()
{
	iThreadStop = 1;

	if( gPreviewBuf.PreviewBufStart )
	{
		free(gPreviewBuf.PreviewBufStart);
		gPreviewBuf.PreviewBufStart = NULL;
		printf("gPreviewBuf.PreviewBufStart  release OK!\n");
	}

	if( dateBuf )
	{
		free(dateBuf);
		dateBuf  = NULL;
		printf("PreviewBuf   dateBuf  release OK!\n");
	}
	pthread_mutex_destroy( &PreviewMutex);

	printf(" PreviewViewBuf release!\n");

	return ALLRIGHT;
}

int check_file_have_space_write()
{
	FILEINFOLOGPOINT *  pFileInfoPoint;
	FILEINFOLOGPOINT fileInfoPoint;
	GST_DISKINFO stDiskInfo;
	char fileName[60];
	char whole_file_name[50];
	FRAMEHEADERINFO temp_frameinfo;
	char buf[1024];
	int ret = 0;
	FILE *  head_fp = NULL;
	FILE *  data_fp = NULL;
	int fileLength = 0;

	
	ret = FS_GetOpreateDiskPartition();
	if( ret < 0 )
	{
		printf(" FS_GetOpreateDiskPartition error !\n");
		return ERROR;
	}

	pFileInfoPoint  = FS_GetPosFileInfo( g_stRecFilePos.iDiskId, 
			g_stRecFilePos.iPartitionId, g_stRecFilePos.iCurFilePos,&fileInfoPoint);
	if(pFileInfoPoint == NULL)
	{
		printf(" FS_GetPosFileInfo error!\n");
		return ERROR;
	}

	stDiskInfo = DISK_GetDiskInfo(pFileInfoPoint->iDiskId);

	sprintf(fileName,"/tddvr/%s%d/%s",&stDiskInfo.devname[5],
					pFileInfoPoint->iPatitionId, pFileInfoPoint->pFilePos->fileName);

	

	sprintf(whole_file_name,"%s.avh",fileName);
		
	head_fp = fopen( whole_file_name, "r+b" );

	if ( head_fp == NULL )
	{
		head_fp = fopen(whole_file_name,"w+b");
		if( head_fp == NULL )
		{
			printf(" create %s error!\n",whole_file_name);
			goto OPENERROR;
		}					
	}

	sprintf(whole_file_name,"%s.avd",fileName);

	data_fp = fopen( whole_file_name, "r+b" );

	if ( data_fp == NULL )
	{
		data_fp = fopen(whole_file_name,"w+b");
		if( data_fp == NULL )
		{
			printf(" create %s error!\n",whole_file_name);
			goto OPENERROR;
		}						
	}

	ret = fseek(head_fp, 0, SEEK_END);
	if ( ret < 0 )
	{
		printf("  fseek error!\n");
		goto OPENERROR;
	}					

	fileLength = ftell(head_fp);

	if( fileLength < pFileInfoPoint->pFilePos->ulOffSet + 1500 * sizeof(FRAMEHEADERINFO) )
	{
		printf("preview rec %s is truncation! change long size\n",fileName);		
	}
	else
	{
		printf("previes rec : file is ok!\n");
		fclose(head_fp);
		fclose(data_fp);
		return ALLRIGHT;		
	}

	ret = fwrite(&temp_frameinfo,1500,sizeof(FRAMEHEADERINFO),head_fp);
	if( ret != (1500*sizeof(FRAMEHEADERINFO)) )
	{
		printf(" can't write data in head file!\n");
		goto OPENERROR;	
	}

	ret = fwrite(buf,2560,1024,data_fp);
	if( ret != (2560 * 1024) )
	{
		printf(" can't write data in data file!\n");
		goto OPENERROR;	
	}

	fclose(head_fp);
	fclose(data_fp);	

	return ALLRIGHT;

	OPENERROR:
	if( head_fp )
	{
		fclose(head_fp);
		head_fp = NULL;
	}

	if( data_fp )
	{
		fclose(data_fp);
		data_fp = NULL;
	}
	

	return ERROR;
}


// 存入一帧数据。（包括音视频）
int WriteToPreviewViewBuf(FRAMEHEADERINFO * headinfo, char * Buf)
{
	FRAMEHEADERINFO * phead;
	int addr_number = 0;

	int rel;
	

	if(gPreviewBuf.PreviewBufStart == NULL )
	{
		return ERROR;
	}


	if(gPreviewBuf.pOffSetTail != gPreviewBuf.PreviewBufStart)
	{
		// 要录的这一帧数据与头帧的时间差大于指定秒数。
	/*	phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;
		
	//	printf(" time 1 = %ld   time 2 = %ld\n",headinfo->ulTimeSec ,phead->ulTimeSec );

	//	printf("head  Addr: %d  [%d] type = %c typeframe = %d time=%ld\n",gPreviewBuf.pOffSetHead,
	//			phead->iChannel,phead->type,phead->ulFrameType,phead->ulTimeSec);

		g_previous_headtime = phead->ulTimeSec;

		if( headinfo->ulTimeSec - phead->ulTimeSec > g_PreviewSecond  )
		{
			printf(" come here+++++++++++++++\n");
			printf(" time 1 = %ld   time 2 = %ld\n",headinfo->ulTimeSec ,phead->ulTimeSec );
				rel = SetHeadToNextKeyFrame();

				if( rel < 0 )
					return ERROR;
		}
		*/
		
		phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;

		g_previous_headtime = phead->ulTimeSec;

		if( ftc_get_rec_cur_status() == 0 )
		{
			//if( iFrameNum >= (get_save_frame_count() * 2) )
			if( iFrameNum >= get_save_frame_count() )
			{
				//DPRINTK("iFrameNum = %d  write over\n",iFrameNum); 
				rel = SetHeadToNextKeyFrame();
				if( rel < 0 )
					return ERROR;
				
				rel = SetHeadToNextKeyFrame();
				if( rel < 0 )
					return ERROR;
				
				rel = SetHeadToNextKeyFrame();
				if( rel < 0 )
					return ERROR;
				
			}	
		}
		
		
	}

	// 如果的空间不足存下一帧完整数据，则pOffSetTail 返头,并清空剩余内存。
	if( (gPreviewBuf.pOffSetTail + sizeof( FRAMEHEADERINFO ) + 
		headinfo->ulDataLen) > (gPreviewBuf.PreviewBufStart + gPreviewBuf.lBufLength - 200) )
	{
		
	//	printf(" wo here\n");
		//printf(" tail %ld, length %ld\n",gPreviewBuf.pOffSetTail, 
		//	(gPreviewBuf.PreviewBufStart + gPreviewBuf.lBufLength ) - gPreviewBuf.pOffSetTail );
				
		memset(gPreviewBuf.pOffSetTail, 0 , 
			(gPreviewBuf.PreviewBufStart + gPreviewBuf.lBufLength) - gPreviewBuf.pOffSetTail );
	
		gPreviewBuf.pOffSetTail = gPreviewBuf.PreviewBufStart;
	}

	memcpy(gPreviewBuf.pOffSetTail, headinfo, sizeof( FRAMEHEADERINFO ) );
	iFrameNum++;

//	printf("1   Addr: %d\n",gPreviewBuf.pOffSetTail);

	phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetTail;	

//	printf("tail  Addr: %d  [%d] type = %c typeframe = %d time=%ld\n",gPreviewBuf.pOffSetTail,
//				phead->iChannel,phead->type,phead->ulFrameType,phead->ulTimeSec);

	gPreviewBuf.pOffSetTail += sizeof( FRAMEHEADERINFO );

	/*if( phead->type == 'V' &&  phead->iChannel == 0  && phead->ulFrameType == 1)
	{
		g_have_keyframe_num++;
	}*/


	memcpy(gPreviewBuf.pOffSetTail, Buf , headinfo->ulDataLen);

	ulDateLength += headinfo->ulDataLen;

//	printf("2  Addr: %d\n",gPreviewBuf.pOffSetTail);

	gPreviewBuf.pOffSetTail += headinfo->ulDataLen;

	addr_number = (int )gPreviewBuf.pOffSetTail % 4;
	if( addr_number != 0 )
		gPreviewBuf.pOffSetTail += (4 -addr_number);
//	printf(" iFrameNum = %d,ulDateLength = %ld\n",iFrameNum,ulDateLength);

	
	return 1;
	
}

int preview_check_write_buf_is_ok()
{
	int ret = 1;	


	 //pOffSetTail 调头已经追赶pOffSetHead
	if( ulDateLength > (gPreviewBuf.lBufLength / 2)) 
	{
		//小于500K 就不能继续往buf中写数据，等待buf 清出空间。
		if(  gPreviewBuf.lBufLength - ulDateLength < (800*1024) )
		{

			//DPRINTK("1Addr: pOffSetHead=%d  pOffSetTail=%d %ld %d (%d,%d)\n",gPreviewBuf.pOffSetHead,gPreviewBuf.pOffSetTail,ulDateLength,
			//		gPreviewBuf.pOffSetHead - gPreviewBuf.pOffSetTail,iFrameNum,g_preview_keyframe_count);

			if( iFrameNum < g_preview_keyframe_count )
			{
				//printf("buf rewrite ,but iFrameNum=%d < g_preview_keyframe_count=%d,so change g_preview_keyframe_count\n",
				//	iFrameNum, g_preview_keyframe_count);

				g_preview_keyframe_count = iFrameNum;

				if( g_preview_keyframe_count < 100 )
					g_preview_keyframe_count = 100;
			}

			if( ftc_get_rec_cur_status() == 0 )
				return 1;		
				
			
			ret = 0;
		}else
		{
			//DPRINTK("222Addr: pOffSetHead=%d  pOffSetTail=%d %ld %d %d\n",gPreviewBuf.pOffSetHead,gPreviewBuf.pOffSetTail,ulDateLength,
			//		gPreviewBuf.pOffSetHead - gPreviewBuf.pOffSetTail,iFrameNum);

			/*if( g_old_preview_keyframe_count != g_preview_keyframe_count )
			{
				g_preview_keyframe_count = g_old_preview_keyframe_count;
				//printf("g_preview_keyframe_count reset\n");
			}*/

			if( g_old_preview_keyframe_count > g_preview_keyframe_count )
			{
				g_preview_keyframe_count++;
			}
			
			ret = 1;
		}
	}	else
	{
		if( g_old_preview_keyframe_count != g_preview_keyframe_count )
		{
				g_preview_keyframe_count = g_old_preview_keyframe_count;
				DPRINTK("g_preview_keyframe_count reset\n");
		}
			
		ret = 1;
	}


	return ret;
}

int get_preview_buf_frame_num()
{
	//printf(" == %d\n",iFrameNum);
	return iFrameNum;
}

int CheckFrameHeader(FRAMEHEADERINFO * pHeader)
{
	// 后面的buf 中没有数据时，返头。
	if( pHeader->type != 'V' && pHeader->type != 'A' )
	{
		//printf(" type = %c\n",pHeader->type);
		return ERROR;	
	}

	
	if( pHeader->iChannel >= g_pstCommonParam->GST_DRA_get_dvr_max_chan(1) || pHeader->iChannel  <0 )
	{
	//	DPRINTK(" iChannel = %d  type=%c length=%ld sec=%ld usec=%ld\n",pHeader->iChannel,
	//		pHeader->type,pHeader->ulDataLen,pHeader->ulTimeSec,pHeader->ulTimeUsec);
		return ERROR;
	}

	// 4* 128 * 1024 = 524288
	if( pHeader->ulDataLen > 524288 ||pHeader->ulDataLen  <= 0 )
	{
		DPRINTK(" ulDataLen = %d\n",pHeader->ulDataLen);
		return ERROR;
	}

	return ALLRIGHT;
}

int SetHeadToNextKeyFrame()
{
	FRAMEHEADERINFO * phead;
	int  addr_number = 0;

	int iRound = 0;

	int iChannel = 0;

	int skip = 0;

	phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;

	//iChannel = phead->iChannel;

	iChannel = 0;

//	printf(" SetHeadToNextKeyFrame iChannel=%d type=%c frametype=%d len=%d time=%ld\n",phead->iChannel,phead->type,
//				phead->ulFrameType,phead->ulDataLen,phead->ulTimeSec);

	while(1)
	{
		phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;

		if( skip == 0 )
		{	
			gPreviewBuf.pOffSetHead += sizeof(FRAMEHEADERINFO);

			iFrameNum--;

			gPreviewBuf.pOffSetHead  += phead->ulDataLen;
			addr_number = (int )gPreviewBuf.pOffSetHead % 4;
			if( addr_number != 0 )
				gPreviewBuf.pOffSetHead += (4 -addr_number);			

			ulDateLength -= phead->ulDataLen;		

		}else
		{
			skip = 0;
		}

		phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;
	

	/*	if( phead->type == 'V' &&  phead->ulFrameType == 1 && phead->iChannel == iChannel)
		{
			printf("OK==  iChannel=%d type=%c frametype=%d len=%d time=%ld\n",phead->iChannel,phead->type,
				phead->ulFrameType,phead->ulDataLen,phead->ulTimeSec);
			g_have_keyframe_num--;
			if( g_have_keyframe_num <= 0 )
				g_have_keyframe_num = 0;
			break;
		}		*/


		if( phead->type == 'V' && phead->ulDataLen > 0)
		{
		//	printf("OK==  iChannel=%d type=%c frametype=%d len=%d time=%ld\n",phead->iChannel,phead->type,
		//		phead->ulFrameType,phead->ulDataLen,phead->ulTimeSec);			
			break;
		}	
	

		// 后面的buf 中没有数据时，返头。
		if( CheckFrameHeader(phead) == ERROR)
		{
		
			/*printf("BAD==  iChannel=%d type=%c frametype=%d len=%d time=%ld  chan=%d\n",phead->iChannel,phead->type,
				phead->ulFrameType,phead->ulDataLen,phead->ulTimeSec,iChannel);
			printf(" go head!!!\n");			
			*/
		
			gPreviewBuf.pOffSetHead  = gPreviewBuf.PreviewBufStart;
			iRound++;

			//printf("SetHead  Addr: %d Tail Addr: %d \n",gPreviewBuf.pOffSetHead,gPreviewBuf.pOffSetTail);

			if( iRound >= 2 )
			{
				printf(" search twice! error\n");
				return ERROR;
			}

			// 这是为了保证能读到PreviewBufStart  那个位置上的那一帧数据
			skip = 1;
		}

	}
	return  1;
}


// 得到一帧数据，用此函数时预录相应该已经停止。
int GetOneKey(FRAMEHEADERINFO * framehead, char * Buf)
{
	FRAMEHEADERINFO * phead;
	int addr_number = 0;
	int ret;

	pthread_mutex_lock( &PreviewMutex);
	// 是否读完缓存。
	if( gPreviewBuf.pOffSetHead == gPreviewBuf.pOffSetTail )
	{
		ret = PREVIEWWRITEOVER;
		goto J_FAIL;
	}

	phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;
	if( phead->type != 'V' && phead->type != 'A' )
	{
		printf("3Addr: pOffSetHead=%d  pOffSetTail=%d %d\n",gPreviewBuf.pOffSetHead,gPreviewBuf.pOffSetTail,iFrameNum );
		ret = ERROR;
		goto J_FAIL;	
	}

	/*if( phead->type == 'V' &&  phead->ulFrameType == 1 && phead->iChannel == 0 )
	{		
		g_have_keyframe_num--;
		if( g_have_keyframe_num <= 0 )
			g_have_keyframe_num = 0;		
	}	*/

//	printf(" %c channel %d length %d frametype %d\n",phead->type, phead->iChannel,
//		phead->ulDataLen,phead->ulFrameType);

	memcpy(framehead, gPreviewBuf.pOffSetHead ,sizeof(FRAMEHEADERINFO) );

//	printf("2  Addr: %d\n",gPreviewBuf.pOffSetHead);

	gPreviewBuf.pOffSetHead += sizeof(FRAMEHEADERINFO);	

	memcpy(Buf, gPreviewBuf.pOffSetHead, framehead->ulDataLen);

	gPreviewBuf.pOffSetHead += phead->ulDataLen;

	addr_number = (int )gPreviewBuf.pOffSetHead % 4;
	if( addr_number != 0 )
		gPreviewBuf.pOffSetHead += (4 -addr_number);	
	//printf("22  Addr: %d   \n",gPreviewBuf.pOffSetHead);
	ulDateLength -= phead->ulDataLen;
	iFrameNum--;
	phead = (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;

	// 是否读完缓存。
	if( gPreviewBuf.pOffSetHead == gPreviewBuf.pOffSetTail )
	{
		ret = PREVIEWWRITEOVER;
		printf( " PREVIEWWRITEOVER addr\n");
		goto J_FAIL;	
	}

	// 后面的buf 中没有数据时，返头。
	if( phead->type != 'V' && phead->type != 'A' )
	{
		gPreviewBuf.pOffSetHead  = gPreviewBuf.PreviewBufStart;

	//	printf( " change addr\n");
		
	}
	pthread_mutex_unlock( &PreviewMutex);
	return 1;
J_FAIL:
	pthread_mutex_unlock( &PreviewMutex);
	return ret;
	
}

int ResetPreviouViewBuf()
{
	gPreviewBuf.pOffSetHead = gPreviewBuf.PreviewBufStart;

	gPreviewBuf.pOffSetTail   = gPreviewBuf.PreviewBufStart;

	g_PreviewSecond = 0;
	g_previous_headtime = 0;
	iFrameNum = 0;
	ulDateLength = 0;
	ulFrameOffset = 0;
	ulFileDataOffset = 0;
	g_have_keyframe_num = 0;

	return ALLRIGHT;
}

int FS_SetFileOffset()
{
	if( g_ulFrameHeaderOffset == 0 || g_ulFileDataOffset == 0 )
		return ERROR;

// 保留录相文件中的当前位移，用于将内存数据写入正确的位置
	ulFrameOffset = g_ulFrameHeaderOffset;
	ulFileDataOffset = g_ulFileDataOffset;

	printf(" ulFrameOffset = %ld,ulFileDataOffset = %ld\n",ulFrameOffset,ulFileDataOffset);

// 除去内存数据后，正确的录相数据。
	g_ulFrameHeaderOffset +=  iFrameNum * sizeof(FRAMEHEADERINFO);

	g_ulFileDataOffset += ulDateLength;

	return ALLRIGHT;
	
}

void previe_write_data_delay(FRAMEHEADERINFO * headinfo)
{
	FRAMEHEADERINFO  * pheader = NULL;
	while( g_preview_sys_flag ==  1 )
	{
		pheader =  (FRAMEHEADERINFO *)gPreviewBuf.pOffSetHead;
		if( (((headinfo->ulTimeSec - pheader->ulTimeSec) >= g_PreviewSecond )
			|| ((headinfo->ulTimeSec - pheader->ulTimeSec) <  0 ))
		&& (ftc_get_rec_cur_status() != 0 ))
		{	
			printf("2  Addr: %d  Addr: %d \n",gPreviewBuf.pOffSetHead,gPreviewBuf.pOffSetTail);
			printf("buf delay rec time = %ld  %ld\n",headinfo->ulTimeSec,pheader->ulTimeSec);
			usleep(10);
		}else
			break;
	}
}

static  int preview_frame_rate_show()
{
	static int count = 0;
	static long oldTime = 0;
	int frameRate;	
//	char cmd[50];
	static int filenum = 0;
	
	struct timeval tvOld;
       struct timeval tvNew;

	  if( count == 0 )
	  {
		 get_sys_time( &tvOld, NULL );
		oldTime = tvOld.tv_sec;
		
	  }

	  count++;

	  if( iFrameNum > 4000  &&  filenum==0)
	  {
		  DPRINTK("encode frame rate:    %d %d %d %ld %ld\n",
			iFrameNum,get_save_frame_count(),g_old_preview_keyframe_count,
			ulDateLength,gPreviewBuf.lBufLength);	

			filenum = 1;
	  }

	  if(  iFrameNum < 4000 )
	  {
	  	filenum =0;
	  }



	  if(count > 4000 )
	  {	 	  	
		get_sys_time( &tvNew, NULL );
	
		frameRate = div_safe(count , tvNew.tv_sec - oldTime);
		count = 0;
		DPRINTK("encode frame rate: %d f/s   %d %d %d %ld %ld\n",
			frameRate,iFrameNum,get_save_frame_count(),g_old_preview_keyframe_count,
			ulDateLength,gPreviewBuf.lBufLength);	
	  }

	  return 1;
	
}

int preview_buf_write(int flag)
{
	GST_DRV_BUF_INFO * pDrvBufInfo = NULL;
	int enable_write = 0;
	struct timeval audiotv;
	int index = 0;
	FRAMEHEADERINFO stFrameHeaderInfo;
	int rel;


	if( g_preview_sys_flag != 1 )
	{
		g_PreviewIsStart = 0;

		DPRINTK("preview not start!\n");
		return -2;
	}

	pthread_mutex_lock( &PreviewMutex);
	enable_write = preview_check_write_buf_is_ok();
	pthread_mutex_unlock( &PreviewMutex);
	if( enable_write == 0 )
	{
		//DPRINTK("buf can't write\n");
		return -1;
	}

	pDrvBufInfo = g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(dateBuf);
		if ( pDrvBufInfo  &&
			((TD_DRV_DATA_TYPE_VIDEO == pDrvBufInfo->iBufDataType) ||
			(TD_DRV_DATA_TYPE_AUDIO == pDrvBufInfo->iBufDataType) ))
		{
			g_PreviewIsStart = 1;
		
			g_pstCommonParam->GST_DRA_get_sys_time( &audiotv, NULL );
			if( TD_DRV_DATA_TYPE_AUDIO == pDrvBufInfo->iBufDataType )
			{			
				for ( index = 0; index < 16; index++ )			
				{
					if (pDrvBufInfo->iChannelMask & (1<<index) )
					{										
						stFrameHeaderInfo.iChannel = index;
						stFrameHeaderInfo.type       = 'A';
						stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[index];
						stFrameHeaderInfo.ulFrameType = 0;
						stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
						stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;

						//previe_write_data_delay(&stFrameHeaderInfo);						
						pthread_mutex_lock( &PreviewMutex);
						rel = WriteToPreviewViewBuf(&stFrameHeaderInfo,dateBuf);
						pthread_mutex_unlock( &PreviewMutex);
						if( rel < 0 )
						{						
							DPRINTK(" WriteToPreviewViewBuf error!\n");
						}								
						//printf("A=  %d    length: %d\n", index, pDrvBufInfo->iBufLen[index]);									
					}
				}
			}else
			{			
				for ( index = 0; index < 16; index++ )					
				{
					if (!( pDrvBufInfo->iChannelMask & ( 1 << index )))
						continue;				

					stFrameHeaderInfo.iChannel = index;
					stFrameHeaderInfo.type       = 'V';
					stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[index];
					stFrameHeaderInfo.ulFrameType = pDrvBufInfo->iFrameType[index];
					stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
					stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;
					stFrameHeaderInfo.videoStandard = g_pstRecModeParam.iPbVideoStandard;
					stFrameHeaderInfo.imageSize	= g_pstRecModeParam.iPbImageSize;
					stFrameHeaderInfo.ulFrameNumber = pDrvBufInfo->iFrameCountNumber[index];				     
		
					if( pDrvBufInfo->tv.tv_sec == 0 )
					{
						stFrameHeaderInfo.ulTimeSec = audiotv.tv_sec;
						printf(" is 0, videotv = %ld\n",audiotv.tv_sec);
					}

					//printf(" channel[%d]   length: %d\n", stFrameHeaderInfo.iChannel , pDrvBufInfo->iBufLen[index]);

					//previe_write_data_delay(&stFrameHeaderInfo);	
					pthread_mutex_lock( &PreviewMutex);
					rel = WriteToPreviewViewBuf(&stFrameHeaderInfo,dateBuf);
					pthread_mutex_unlock( &PreviewMutex);
					if( rel < 0 )
					{							
						DPRINTK(" WriteToPreviewViewBuf error!\n");
					
					}		

					preview_frame_rate_show();

					
				}
			}
		}else
		{
			return 0;
		}


		return 1;
}




void thread_for_preview_record()
{
	SET_PTHREAD_NAME(NULL);
	int rel;
	FILEINFOLOGPOINT *  pFileInfoPoint;
	GST_DISKINFO stDiskInfo;
	char filename[50];
	FRAMEHEADERINFO stFrameHeaderInfo;
	int iFirst = 0;
	int stat = 1;
	FILE *  head_fp = NULL;
	FILE *  data_fp = NULL;
	char whole_file_name[60];
	 struct timeval audiotv;
	GST_DRV_BUF_INFO * pDrvBufInfo = NULL;
	int index = 0;
	int frame_count = 0;
	int enable_write = 0;

		printf(" thread_for_preview_record pid = %d\n",getpid());
	
	while(1)
	{
		if( iThreadStop )
			break;
	
		if( g_preview_sys_flag != 1 )
		{
			g_PreviewIsStart = 0;
			usleep(10000);
			continue;
		}

		enable_write = preview_check_write_buf_is_ok();
		if( enable_write == 0 )
		{
			usleep(10);

			#ifdef TEST_BUF_EFFICIENCY
			DPRINTK("preview_check_write_buf_is_ok %d\n",enable_write);			
		    #endif
			
			continue;
		}

		pDrvBufInfo = g_pstCommonParam->GST_CMB_GetSaveFileEncodeData(dateBuf);
		if ( pDrvBufInfo  &&
			((TD_DRV_DATA_TYPE_VIDEO == pDrvBufInfo->iBufDataType) ||
			(TD_DRV_DATA_TYPE_AUDIO == pDrvBufInfo->iBufDataType) ))
		{
			g_PreviewIsStart = 1;
		
			g_pstCommonParam->GST_DRA_get_sys_time( &audiotv, NULL );
			if( TD_DRV_DATA_TYPE_AUDIO == pDrvBufInfo->iBufDataType )
			{			
				for ( index = 0; index < 16; index++ )			
				{
					if (pDrvBufInfo->iChannelMask & (1<<index) )
					{										
						stFrameHeaderInfo.iChannel = index;
						stFrameHeaderInfo.type       = 'A';
						stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[index];
						stFrameHeaderInfo.ulFrameType = 0;
						stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
						stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;

						//previe_write_data_delay(&stFrameHeaderInfo);						
						pthread_mutex_lock( &PreviewMutex);
						rel = WriteToPreviewViewBuf(&stFrameHeaderInfo,dateBuf);
						pthread_mutex_unlock( &PreviewMutex);
						if( rel < 0 )
						{						
							printf(" WriteToPreviewViewBuf error!\n");
						}								
						//printf("A=  %d    length: %d\n", index, pDrvBufInfo->iBufLen[index]);									
					}
				}
			}else
			{			
				for ( index = 0; index < 16; index++ )					
				{
					if (!( pDrvBufInfo->iChannelMask & ( 1 << index )))
						continue;				

					stFrameHeaderInfo.iChannel = index;
					stFrameHeaderInfo.type       = 'V';
					stFrameHeaderInfo.ulDataLen = pDrvBufInfo->iBufLen[index];
					stFrameHeaderInfo.ulFrameType = pDrvBufInfo->iFrameType[index];
					stFrameHeaderInfo.ulTimeSec = pDrvBufInfo->tv.tv_sec;
					stFrameHeaderInfo.ulTimeUsec = pDrvBufInfo->tv.tv_usec;
					stFrameHeaderInfo.videoStandard = g_pstRecModeParam.iPbVideoStandard;
					stFrameHeaderInfo.imageSize	= g_pstRecModeParam.iPbImageSize;
					stFrameHeaderInfo.ulFrameNumber = pDrvBufInfo->iFrameCountNumber[index];				     
		
					if( pDrvBufInfo->tv.tv_sec == 0 )
					{
						stFrameHeaderInfo.ulTimeSec = audiotv.tv_sec;
						printf(" is 0, videotv = %ld\n",audiotv.tv_sec);
					}

					//printf(" channel[%d]   length: %d\n", stFrameHeaderInfo.iChannel , pDrvBufInfo->iBufLen[index]);

					//previe_write_data_delay(&stFrameHeaderInfo);	
					pthread_mutex_lock( &PreviewMutex);
					rel = WriteToPreviewViewBuf(&stFrameHeaderInfo,dateBuf);
					pthread_mutex_unlock( &PreviewMutex);
					if( rel < 0 )
					{							
						printf(" WriteToPreviewViewBuf error!\n");
					//	FS_SetErrorMessage(rel, 0);
					}		

					preview_frame_rate_show();

					/*if( stFrameHeaderInfo.iChannel == 0  && stFrameHeaderInfo.ulFrameNumber > g_preview_keyframe_count )
					{
						g_pstCommonParam->GST_DRA_local_instant_i_frame(0xffff);						
						//printf("11 GST_DRA_local_instant_i_frame\n");
					}*/
				}
			}
		}else
		{
			#ifdef TEST_BUF_EFFICIENCY
			//DPRINTK("GST_CMB_GetSaveFileEncodeData = NULL\n");				
		    #endif
			usleep(50);
		}
				
	
	//	ResetPreviouViewBuf();
	}
	printf(" preview thread stop!\n");
	return ;
	
}

int FS_CheckPreviewStatus()
{
	if(g_PreviewIsStart != 1)
		return 0;

	return 1;
}


