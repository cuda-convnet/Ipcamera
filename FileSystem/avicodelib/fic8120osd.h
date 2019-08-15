#ifndef _FIC8120_OSD_
#define _FIC8120_OSD_

// word color
#define OSD_WRITE 0x888888
#define OSD_BLACK 0x447733

#define OSD_GET_Y(value) ((value>>16) & 0x00ff)
#define OSD_GET_U(value) ((value>>8) & 0x00ff)
#define OSD_GET_V(value) (value & 0x00ff)

#define YUV_8185 1
#define RGB_8185 2
#define BIT1_2COLOR 2
#define BIT4_16COLOR 16
#define BIT24_COLOR 24



typedef struct BXS_PIC_
{
	unsigned short pic_mode; // 1 is YUV , 2 is RGB
	unsigned short bit_color; // 2 is 2color 1bit, 16 is 16 color 4bit,24 is true color is 24 bit
	unsigned short pic_width;
	unsigned short pic_height;
}BXS_PIC;

typedef struct BXS_PIXELS_
{
	unsigned char color1;    //  R or  y
	unsigned char color2;   //  G or u
	unsigned char color3;   // B or v
	unsigned char color4;   //reserved
}BXS_PIXELS;





int set_osd_show_lcd_buf(unsigned char * buf_y,unsigned char * buf_u,unsigned char * buf_v);
int set_osd_backgroud_lcd_buf(unsigned char * buf_y,unsigned char * buf_u,unsigned char * buf_v);
int osd_line(int start_x, int start_y, int end_x, int end_y,
	char * screen_buf,int screen_width, int screen_height, int color);
int osd_24bit_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard);
unsigned char *  load_yuv_pic(char * file_name);
int osd_2bit_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int word_color,int back_color);
int set_background_pic(char * path);
int osd_bmp_photo(unsigned char * yuv_buf,int point_x,int point_y,int standard,int start_x,int start_y,
					int pic_width,int pic_height,int transparence_color,int transparence_degree);


#endif
