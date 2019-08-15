/*
	this file is used to print osd or bitmap to screen
*/

#include "fic8120osd.h"
#include <stdio.h>
#include "filesystem.h"

static unsigned char * lcd_buf_y = NULL;
static unsigned char * lcd_buf_u = NULL;
static unsigned char * lcd_buf_v = NULL;
static unsigned char * lcd_buf_y2 = NULL;
static unsigned char * lcd_buf_u2 = NULL;
static unsigned char * lcd_buf_v2 = NULL;
static unsigned char * backgroud_bmp = NULL;
static unsigned int is_use_bitmap = 0;

int osd_24bit_photo2(unsigned char * yuv_buf,int point_x,int point_y,int start_x,int start_y,int p_width,int p_height,int standard);

int set_osd_show_lcd_buf(unsigned char * buf_y,unsigned char * buf_u,unsigned char * buf_v)
{
	lcd_buf_y = buf_y;
	lcd_buf_u = buf_u;
	lcd_buf_v = buf_v;
	
	return 1;
}

int set_osd_backgroud_lcd_buf(unsigned char * buf_y,unsigned char * buf_u,unsigned char * buf_v)
{
	lcd_buf_y2 = buf_y;
	lcd_buf_u2 = buf_u;
	lcd_buf_v2 = buf_v;
	
	return 1;
}

int set_background_pic(char * path)
{
	if( backgroud_bmp != NULL )
	{
		free(backgroud_bmp);
		backgroud_bmp = NULL;
	}

	backgroud_bmp = load_yuv_pic(path);		
	if( backgroud_bmp == NULL )
	{
		printf(" load_yuv_pic %s error!\n",path);
		return -1;
	}
	
	is_use_bitmap = 1;

	return 1;
}

int clear_lcd_osd(int color)
{
	unsigned char color_y,color_u,color_v;
	int width,height,i,j;

	width = 720;
	height = 576;

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}

	color_y =  OSD_GET_Y(color);
	color_u =  OSD_GET_U(color);
	color_v =  OSD_GET_V(color);

	#ifdef USE_H264_ENC_DEC


	/*if( is_use_bitmap == 0 )
	{
		if( backgroud_bmp == NULL )
		{
			backgroud_bmp = load_yuv_pic("/mnt/mtd/picyuv/backgroud1.yuv");		
			if( backgroud_bmp == NULL )
			{
				printf(" load_yuv_pic load backgroud1.yuv error!\n");
			}

			is_use_bitmap = 1;
		}
	}*/

	if( backgroud_bmp != NULL )
	{
		for( i = 0; i < 576 / 160; i++ )
		{
			for(j = 0; j < 720 / 220; j++)
			{
				 osd_24bit_photo(backgroud_bmp,i*220,j*160,TD_DRV_VIDEO_STARDARD_PAL);
			}
		}

		osd_24bit_photo2(backgroud_bmp,3*220,0*160,0,0,60,160,TD_DRV_VIDEO_STARDARD_PAL);	
		osd_24bit_photo2(backgroud_bmp,3*220,1*160,0,0,60,160,TD_DRV_VIDEO_STARDARD_PAL);	
		osd_24bit_photo2(backgroud_bmp,3*220,2*160,0,0,60,160,TD_DRV_VIDEO_STARDARD_PAL);	

		osd_24bit_photo2(backgroud_bmp,0*220,3*160,0,0,220,96,TD_DRV_VIDEO_STARDARD_PAL);	
		osd_24bit_photo2(backgroud_bmp,1*220,3*160,0,0,220,96,TD_DRV_VIDEO_STARDARD_PAL);	
		osd_24bit_photo2(backgroud_bmp,2*220,3*160,0,0,220,96,TD_DRV_VIDEO_STARDARD_PAL);	

		osd_24bit_photo2(backgroud_bmp,3*220,3*160,0,0,60,96,TD_DRV_VIDEO_STARDARD_PAL);	
				
		memcpy(lcd_buf_y2,lcd_buf_y,720*576*2);
	}
	else
	{		
		for(i = 0 ; i < height; i++ )
		{
			for( j = 0; j <width/2; j++ )
			{			
				lcd_buf_y[(i*width*2)+j*4] = color_u;
				lcd_buf_y[(i*width*2)+j*4+1] = color_y;
				lcd_buf_y[(i*width*2)+j*4+2] = color_v;
				lcd_buf_y[(i*width*2)+j*4+3] = color_y;
			}
		}
	}
	
	
	#else
	memset(lcd_buf_y,color_y,720*576);
	memset(lcd_buf_u,color_u,720*576 / 4);
	memset(lcd_buf_v,color_v,720*576 / 4);
	#endif

	return 1;
}

int osd_24bit_photo2(unsigned char * yuv_buf,int point_x,int point_y,int start_x,int start_y,int p_width,int p_height,int standard)
{
	int width,height;
	int photo_width,photo_height;
	int i,j;
	unsigned char * img_y = NULL;
	unsigned char * img_u = NULL;
	unsigned char * img_v = NULL;

	if( yuv_buf == NULL )
	{
		printf(" yuv_buf is NULL!\n");
		return -1;
	}

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}

	if( yuv_buf[4] != 24 )
	{
		printf(" pic is not 24bit\n");
		return -1;
	}

	if(standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		width = 720;
		height = 576;
	}else
	{
		width = 720;
		height = 480;
	}

	photo_width = yuv_buf[0] * 100 + yuv_buf[1];
	photo_height =  yuv_buf[2] * 100 + yuv_buf[3];

	//printf(" photo_width = %d photo_height = %d\n",photo_width,photo_height);

	img_y = &yuv_buf[5];
	img_v = &yuv_buf[5] + photo_width * photo_height;
	img_u = &yuv_buf[5] + photo_width * photo_height * 5 /4;

/*	if( photo_width + point_x >= width )
	{
		printf(" point_x =%d over range!\n",point_x);
		return -1;
	}

	if( photo_height + point_y >= height )
	{
		printf(" point_y =%d over range!\n",point_y);
		return -1;
	}
	*/

	if( p_width == 0 && p_height == 0 )
	{
		p_width = photo_width;
		p_height = photo_height;
	}	
	

#ifdef USE_H264_ENC_DEC
	for( j = 0 ; j <  p_height ; j++)
	{		
		
		for( i = 0 ; i <  p_width; i++)
		{
			if( i % 2 == 0 )
				lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2] = img_u[((j+start_y)/2)*(photo_width/2)+ (i+start_x)/2];
			else
				lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2 ] = img_v[((j+start_y)/2)*(photo_width/2)+ (i+start_x)/2];
			
			lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2 + 1] = img_y[(j+start_y)*photo_width + (i+start_x)];
			
		}
		
	}
		

#else
	
	
	for( i = 0 ; i < p_height; i++ )
	{
		memcpy(&lcd_buf_y[(i + point_y) * width + point_x],&img_y[ i * photo_width], p_width);
	}	
	
	for( i = 0 ; i < p_height/2; i++ )
	{
		memcpy(&lcd_buf_u[( i + point_y /2 ) * (width/2) + point_x /2  ],&img_u[i * photo_width/2], p_width/2);
	}
	
	for( i = 0 ; i < p_height/2; i++ )
	{
		memcpy(&lcd_buf_v[( i + point_y /2 ) * (width/2)  + point_x /2  ],&img_v[i * photo_width/2], p_width/2);
	}		

#endif
	
	return 1;
	
}

// screen_buf is point to yuv420 picture memory, only draw beeline
int osd_line(int start_x, int start_y, int end_x, int end_y,
	char * screen_buf,int screen_width, int screen_height, int color)
{
	unsigned char * img_y = NULL;
	unsigned char * img_u = NULL;
	unsigned char * img_v = NULL;
	int i;
	unsigned char color_y,color_u,color_v;

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}

	if( screen_buf == NULL )
	{
		printf(" fate error! screen_buf is NULL!\n");
		return -1;
	}

	color_y =  OSD_GET_Y(color);
	color_u =  OSD_GET_U(color);
	color_v =  OSD_GET_V(color);
	

	img_y = screen_buf;
	img_u = screen_buf + screen_width * screen_height;
	img_v = screen_buf + screen_width * screen_height * 5 /4;
	

	if( start_x == end_x ) // vertical line
	{
		for( i = start_y ; i < end_y; i++ )
		{
			img_y[start_x + screen_width*i] = color_y ;
		}

		for( i = start_y /2 ; i < end_y / 2; i++ )
		{
			//img_u[start_x / 2+ screen_width / 2*i] = color_u ;
		}

		for( i = start_y /2 ; i < end_y /2 ; i++ )
		{
			//img_v[start_x /2 + screen_width / 2*i] = color_v ;
		}
	}
	else if( start_y == end_y ) //horizontal line
	{
		start_x = 10;
		for( i = start_x ; i < start_x +  720; i++ )
		{
			img_y[i + 720*0] = color_y ;
			//printf("i = %d i + screen_width*start_y=%d\n",i,(i+screen_width*start_y));
		}

		for( i = start_y / 2 ; i < end_y /2; i++ )
		{
			//img_u[start_x /2  + screen_width /2*i] = color_u ;
		}

		for( i = start_y /2; i < end_y /2 ; i++ )
		{
		//	img_v[start_x /2 + screen_width /2*i] = color_v;
		}
		
	}else
	{
		printf(" wrong point value!\n");
		return -1;
	}
	
	

	return 1;
}

int osd_24bit_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard)
{
	int width,height;
	int photo_width,photo_height;
	int i,j;
	unsigned char * img_y = NULL;
	unsigned char * img_u = NULL;
	unsigned char * img_v = NULL;

	if( yuv_buf == NULL )
	{
		printf(" yuv_buf is NULL!\n");
		return -1;
	}

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}

	if( yuv_buf[4] != 24 )
	{
		printf(" pic is not 24bit\n");
		return -1;
	}

	if(standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		width = 720;
		height = 576;
	}else
	{
		width = 720;
		height = 480;
	}

	photo_width = yuv_buf[0] * 100 + yuv_buf[1];
	photo_height =  yuv_buf[2] * 100 + yuv_buf[3];

	printf(" photo_width = %d photo_height = %d\n",photo_width,photo_height);

	img_y = &yuv_buf[5];
	img_v = &yuv_buf[5] + photo_width * photo_height;
	img_u = &yuv_buf[5] + photo_width * photo_height * 5 /4;

	if( photo_width + point_x >= width )
	{
		printf(" point_x =%d over range!\n",point_x);
		return -1;
	}

	if( photo_height + point_y >= height )
	{
		printf(" point_y =%d over range!\n",point_y);
		return -1;
	}

#ifdef USE_H264_ENC_DEC
	for( j = 0 ; j <  photo_height ; j++)
	{		
		
		for( i = 0 ; i <  photo_width; i++)
		{
			if( i % 2 == 0 )
				lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2] = img_u[(j/2)*(photo_width/2)+ i/2];
			else
				lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2 ] = img_v[(j/2)*(photo_width/2) + i/2];						
							
			lcd_buf_y[ width* 2 * (j + point_y )  + (point_x  + i )*2 + 1] = img_y[j*photo_width + i];
			
		}
	}
		

#else
	for( i = 0 ; i < photo_height; i++ )
	{
		memcpy(&lcd_buf_y[(i + point_y) * width + point_x],&img_y[ i * photo_width], photo_width);
	}	
	
	for( i = 0 ; i < photo_height/2; i++ )
	{
		memcpy(&lcd_buf_u[( i + point_y /2 ) * (width/2) + point_x /2  ],&img_u[i * photo_width/2], photo_width/2);
	}
	
	for( i = 0 ; i < photo_height/2; i++ )
	{
		memcpy(&lcd_buf_v[( i + point_y /2 ) * (width/2)  + point_x /2  ],&img_v[i * photo_width/2], photo_width/2);
	}

#endif
	
	return 1;
	
}

inline int get_u(int width,int height,int pos_x,int pos_y)
{
	int u;

	u = ((pos_y /2)  * (width /2) )  + pos_x /2;
	
	return u; 
}

inline int get_v(int width,int height,int pos_x,int pos_y)
{
	int v;

	v =  ((pos_y /2)  * (width /2) )  + pos_x /2;
	
	return v; 
}


int osd_2bit_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int word_color,int back_color)
{
	int width,height;
	int photo_width,photo_height;
	int i,j;
	int bit = 0,k = 0;
	int buf_width,buf_height;
	int point_value;
	unsigned char * img_y = NULL;
	unsigned char * img_u = NULL;
	unsigned char * img_v = NULL;
	unsigned char color_y,color_u,color_v;
	unsigned char color_y2,color_u2,color_v2;
	int ret = -1;

	BXS_PIC * bxs_pic = NULL;

	bxs_pic = ( BXS_PIC * )yuv_buf;	

	if( yuv_buf == NULL )
	{
		printf(" yuv_buf is NULL!\n");
		return -1;
	}

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}


//如果是8185 yuv图片的话
	if(  bxs_pic->pic_mode == YUV_8185 )
	{
		switch(bxs_pic->bit_color)
		{
			case BIT4_16COLOR: 						
			case BIT24_COLOR:
				ret = osd_bmp_photo(yuv_buf, point_x, point_y, standard, start_x, start_y,
					 pic_width, pic_height, 0x000000, 0);	
				return ret;				
		}
	}

	if( word_color == 0 && back_color == 0 )
	{
		osd_24bit_photo2(yuv_buf,point_x,point_y,start_x,start_y,pic_width,pic_height,standard);
		return 1;
	}

	if( yuv_buf[4] != 2 )
	{
		printf(" pic is not 2bit\n");
		return -1;
	}

	/*printf("1 %d %d %d %d %d %d %d \n",point_x,point_y,standard,
		start_x,start_y,
		pic_width,pic_height);*/



	color_y =  OSD_GET_Y(word_color);
	color_u =  OSD_GET_U(word_color);
	color_v =  OSD_GET_V(word_color);
	color_y2 =  OSD_GET_Y(back_color);
	color_u2 =  OSD_GET_U(back_color);
	color_v2 =  OSD_GET_V(back_color);	

	if(standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		width = 720;
		height = 576;
	}else
	{
		width = 720;
		height = 480;
	}

	photo_width = yuv_buf[0] * 100 + yuv_buf[1];
	photo_height =  yuv_buf[2] * 100 + yuv_buf[3];

	//printf("11 photo_width = %d photo_height = %d\n",photo_width,photo_height);

	img_y = &yuv_buf[5];
	img_v = &yuv_buf[5] + photo_width * photo_height;
	img_u = &yuv_buf[5] + photo_width * photo_height * 5 /4;

/*
	if( photo_width + point_x -start_x >= width )
	{
		printf(" point_x =%d over range!\n",point_x);
		return -1;
	}

	if( photo_height + point_y - start_y >= height )
	{
		printf(" point_y =%d over range!\n",point_y);
		return -1;
	}

	if( (start_x + pic_width ) > photo_width )
	{
		printf(" start_x + pic_width =%d over range!\n",start_x + pic_width);
		return -1;
	}

	if( (start_y + pic_height) > photo_height )
	{
		printf(" start_y + pic_height =%d over range!\n",start_y + pic_height);
		return -1;
	}
*/

	if( photo_width % 8 == 0 )
		buf_width = photo_width / 8;
	else
		buf_width = photo_width / 8 + 1;

	buf_height = photo_height;

	
	if( pic_width == 0 || pic_height == 0 )
	{
		start_x = 0;
		start_y = 0;
		pic_width = photo_width;
		pic_height = photo_height;
	}

	if( pic_width == -1 )  // show line photo
	{
		pic_width = photo_width;
		start_x = 0;
	}

	//printf("2 pic_width = %d pic_height=%d start_x=%d start_y=%d\n",
	//	pic_width,pic_height,start_x,start_y);	

	/*if( start_y > 700 || start_x > 700 ||pic_height> 700 || pic_width>700)
	{
		printf("error start_y = %d start_x = %d pic_height=%d pic_width = %d\n",
			start_y,start_x,pic_height,pic_width);

		return 1;
	}

	if( point_y>700 || point_x>700 )
	{
		printf("error point_y = %d point_x = %d \n",point_y,point_x);

		return 1;
	}*/

	//printf("2 %d %d %d %d %d %d %d \n",point_x,point_y,standard,
	//	start_x,start_y,
	//	pic_width,pic_height);

	//printf("read date %x%x%x%x\n",img_y[1],img_y[2],img_y[3],img_y[4]);

	//usleep(300000);


#ifdef USE_H264_ENC_DEC
	for( j = start_y ; j < start_y + pic_height ; j++)
	{
		bit = 0;
		k = 0;
		
		if( k == 0 )
		{
			bit = start_x % 8;
			k   = start_x / 8;
		}
		
		for( i = start_x ; i < start_x + pic_width; i++)
		{
			point_value = (img_y[j*buf_width + k] & (1<< (7-bit)));		

			if( backgroud_bmp != NULL )
			{
				if( point_value > 0 )
				{
					if( i % 2 == 0 )
						lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2] = point_value > 0 ? color_u : color_u2;
					else
						lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 ] = point_value > 0 ? color_v : color_v2;
					
					lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 + 1] = point_value > 0 ? color_y : color_y2;
					
				}else
				{
					if( i % 2 == 0 )
						lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2] = 
								lcd_buf_y2[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2];
					else
						lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 ] = 
								lcd_buf_y2[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 ];
					
					lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 + 1] = 
							lcd_buf_y2[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 + 1];
					
				}
			}else
			{
				if( i % 2 == 0 )
					lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2] = point_value > 0 ? color_u : color_u2;
				else
					lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 ] = point_value > 0 ? color_v : color_v2;
				
				lcd_buf_y[ width* 2 * (j + point_y - start_y)  + (point_x  + i - start_x)*2 + 1] = point_value > 0 ? color_y : color_y2;
				
			}
			bit++;

			if( bit >= 8 )
			{
				bit = 0;
				k++;
			}
		}
	}
#else

	for( j = start_y ; j < start_y + pic_height ; j++)
	{
		bit = 0;
		k = 0;
		
		if( k == 0 )
		{
			bit = start_x % 8;
			k   = start_x / 8;
		}
		
		for( i = start_x ; i < start_x + pic_width; i++)
		{
			point_value = (img_y[j*buf_width + k] & (1<< (7-bit)));		
			
			lcd_buf_y[ width * (j + point_y - start_y)  + point_x  + i - start_x] = point_value > 0 ? color_y : color_y2;
			
			lcd_buf_u[get_u(width,height,i  - start_x + point_x,j + point_y - start_y)] =  point_value > 0 ? color_u : color_u2;
			
			lcd_buf_v[get_v(width,height,i  - start_x + point_x,j + point_y- start_y)] =  point_value > 0 ? color_v : color_v2;

			bit++;

			if( bit >= 8 )
			{
				bit = 0;
				k++;
			}
		}
	}

#endif

	return 1;
	
}

unsigned char * load_bxs_bmp_pic(char * pic_name)
{
	unsigned char * pic_buf = NULL;
	FILE * fp = NULL;
//	char bitmap_header[14];	
	int ret ;
//	int imageSize ;
//	BXS_PIC  bxs_pic;
	int offset;

	fp = fopen(pic_name,"rb");

	if( fp  == NULL )
	{
		DPRINTK("open file %s error!\n",pic_name);
		goto open_faild;
	}

	//由于linux 结构体对齐的原因BITMAPFILEHEADER 无法正常读取。
	
	if( fseek(fp ,0L,SEEK_END) != 0 )
	{
		DPRINTK("fseek file is wrong!\n");
		goto open_faild;
	}

	offset = ftell(fp);
	if( offset == -1 )
	{
		DPRINTK(" ftell error!\n");
		goto open_faild;
	}

	//printf(" offset = %d\n",offset);

	rewind(fp);	

	pic_buf = (unsigned char *)malloc(offset);

	if( pic_buf == NULL )
	{
		DPRINTK("pic_buf malloc error!\n");
		goto open_faild;
	}

	ret = fread(pic_buf,1,offset,fp);
	if( ret != offset )
	{
		DPRINTK("read bmp data error!\n");
		goto open_faild;
	}

	
	fclose(fp);
	fp = NULL;	

	return pic_buf;

open_faild:

	if( fp )
	{
		fclose(fp);
		fp = NULL;
	}

	if( pic_buf )
	{
		free(pic_buf);
		pic_buf = NULL;
	}
	
	return NULL;
}


unsigned char *  load_yuv_pic(char * file_name)
{
	FILE * input_file = NULL;
	unsigned char a,b,a1,b1;
	int width,height;
	char * yuv_array = NULL;
	int ret;
	int total_bytes;
	int buf_width,buf_height;

	return load_bxs_bmp_pic(file_name);

	if( file_name == NULL )
	{
		printf(" file_name is NULL!\n");
		return NULL;
	}

	input_file = fopen(file_name,"rb");

	if( input_file == NULL )
	{
		printf("open %s error!\n",file_name);
		return NULL;
	}

	fread(&a,1,sizeof(unsigned char),input_file);
	fread(&b,1,sizeof(unsigned char),input_file);
	fread(&a1,1,sizeof(unsigned char),input_file);
	fread(&b1,1,sizeof(unsigned char),input_file);

	width = a * 100 + b;
	height = a1 * 100 + b1;

	fread(&a,1,sizeof(unsigned char),input_file);

	if( a == 2 )
	{
		if( width % 8 == 0 )
			buf_width = width / 8;
		else
			buf_width = width / 8 + 1;

		buf_height = height;

		printf(" buf_width = %d buf_height = %d width=%d\n",buf_width, buf_height,width);

		total_bytes = buf_width * buf_height  + 5;

	}else
	{
		total_bytes = width * height * 3 /2 + 5;
	}
	
	yuv_array = (unsigned char *)malloc(total_bytes);

	if( !yuv_array )
	{
		printf(" memory is not enough!\n");
		fclose(input_file);
		return NULL;
	}

	rewind(input_file);

	ret = fread(yuv_array,1,total_bytes,input_file);
	if( ret != total_bytes )
	{
		printf(" ret = %d total_bytes=%d\n",ret,total_bytes);
		free(yuv_array);
		yuv_array = NULL;
		fclose(input_file);
		return NULL;		
	}

	fclose(input_file);
	
	return yuv_array;
}

int osd_yuv16_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int transparence_color,int transparence_degree)
{
	int width,height;
	int photo_width,photo_height;
	int pitch = 0;
	int i,j;
	unsigned char * img_y = NULL;
//	unsigned char * img_u = NULL;
//	unsigned char * img_v = NULL;
	BXS_PIC * bxs_pic = NULL;
	BXS_PIXELS * pic_pixels = NULL;
	int color_bit = 16;
//	int r,g,b;
	unsigned char color_num = 0;
	int color_r,color_g,color_b;

	if( yuv_buf == NULL )
	{
		printf(" yuv_buf is NULL!\n");
		return -1;
	}

	if( lcd_buf_y == NULL )
	{
		printf(" lcd_buf is NULL\n");
		return -1;
	}
	

	if(standard == TD_DRV_VIDEO_STARDARD_PAL )
	{
		width = 720;
		height = 576;
	}else
	{
		width = 720;
		height = 480;
	}	

	bxs_pic = ( BXS_PIC * )yuv_buf;
	pic_pixels = (BXS_PIXELS *)(yuv_buf + sizeof(BXS_PIC));

	img_y = yuv_buf + sizeof(BXS_PIC) + color_bit*sizeof(BXS_PIXELS);

	pitch  = bxs_pic->pic_width /2;

	
	color_r =  OSD_GET_Y(transparence_color);
	color_g =  OSD_GET_U(transparence_color);
	color_b =  OSD_GET_V(transparence_color);
	
	
	photo_width = bxs_pic->pic_width ;
	photo_height = bxs_pic->pic_height;
	


	if( pic_width == 0 )
	{
		pic_width = photo_width;
		pic_height = photo_height;
	}

	for( j = 0 ; j <  pic_height ; j++)
	{		

		for( i = 0 ; i <  pic_width; i++)
		{	
			if( i % 2 == 0 )
			{
				color_num = (img_y[ (start_y+j)*pitch + (start_x+i)/2 ]>> 4 ) & 0x0f;
			}else
			{
				color_num = img_y[ (start_y+j)*pitch + (start_x+i)/2 ] & 0x0f;
			}

			if( color_num >= 16 )
			{
				printf("color_num =%d error!\n",color_num);				
			}

			if( (color_r == pic_pixels[color_num].color1) &&
				 (color_g == pic_pixels[color_num].color2) &&
				  (color_b == pic_pixels[color_num].color3) )
			{
				//screen_buf_v1[(point_y+j)*width*4 + (point_x+i)*4 + 3] = 0;
					continue;
			}
				 
			if( i % 2 == 0 )
			{
				lcd_buf_y[(point_y+j)*width*2 + (point_x+i)*2] = pic_pixels[color_num].color3; //blue				
					
			}else
			{
				lcd_buf_y[(point_y+j)*width*2 + (point_x+i)*2] = pic_pixels[color_num].color2; //blue		
			}

			lcd_buf_y[(point_y+j)*width*2 + (point_x+i)*2 + 1] = pic_pixels[color_num].color1;  //green
				
			
			/*printf("%d,%d  color_num =%d %d,%d,%d\n",i,j,color_num,pic_pixels[color_num].color1,
				pic_pixels[color_num].color2,pic_pixels[color_num].color3);
			sleep(1);*/
		}		
		
	}

	
	return 1;
	
}



int osd_bmp_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int transparence_color,int transparence_degree)
{
	int ret = 0;
	BXS_PIC * bxs_pic = NULL;

	bxs_pic = ( BXS_PIC * )yuv_buf;	

	if( bxs_pic->pic_mode == RGB_8185 )
	{
		switch(bxs_pic->bit_color)
		{
			case BIT4_16COLOR: 
				break;
			case BIT24_COLOR: 
				break;
			default : return ret;
		}
	}

	if(  bxs_pic->pic_mode == YUV_8185 )
	{
		switch(bxs_pic->bit_color)
		{
			case BIT4_16COLOR: 
				ret = osd_yuv16_photo(yuv_buf, point_x, point_y, standard, start_x, start_y,
					 pic_width, pic_height, transparence_color, transparence_degree);
				break;
			case BIT24_COLOR: break;
			default : return ret;
		}
	}
	
	return ret;

}


