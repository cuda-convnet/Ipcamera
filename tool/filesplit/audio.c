#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/soundcard.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_FILE_SIZE 20*1024*1024
#define IS_DIRECTORY 1
#define IS_FILE  0


char * file_buf = NULL;


enum mode { PLAY, REC };

typedef struct _FILE_INFO_
{
	char file_name[200];
	int    is_directory;
	int    size;
}FILE_INFO;

void usage(void) {
	fprintf(stderr,
	"usage: demo<options>\n"
	"options:\n"
	"	input <directory>\n"
	"	output  <directory>\n");
	exit(1);
}

int write_file(char * file_name, FILE * input_file,int is_dir)
{
	FILE_INFO backup_file;
	int length = 0;
	FILE  * fp = NULL;
	int rel;	

	printf(" write %s in file\n",file_name);

	if( strlen(file_name) > 59 )
	{	
		printf(" file name is too long");
		return -1;
	}
	strcpy(backup_file.file_name,file_name);
	backup_file.is_directory = is_dir;

	if( is_dir == IS_DIRECTORY )
	{
		rel = fwrite(&backup_file,1,sizeof(FILE_INFO),input_file);
		if( rel != sizeof(FILE_INFO) )
		{			
			printf("fwrite %s error!\n",file_name);
			return -1;
		}

		return 1;
	}

	fp = fopen(file_name,"rb");
	if( !fp )
	{
		printf("open %s error!\n",file_name);
		return -1;
	}

	rel = fseek(fp, 0, SEEK_END);
	if ( rel < 0 )
	{					
		fclose(fp);		
		printf("fseek %s error!\n",file_name);
		return -1;
	}					

	length = ftell(fp);

	if( length >= MAX_FILE_SIZE )
	{
		fclose(fp);		
		printf(" %s is too long error!\n",file_name);
		return -1;
	}

	backup_file.size = length;

	rel = fwrite(&backup_file,1,sizeof(FILE_INFO),input_file);
	if( rel != sizeof(FILE_INFO) )
	{
		fclose(fp);
		printf("fwrite %s error!\n",file_name);
		return -1;
	}

	rewind(fp);	

	rel = fread(file_buf,1,length,fp);
	if( rel != length )
	{		
		printf("fread %s error!\n",file_name);
		fclose(fp);
		return -1;
	}
	
	rel = fwrite(file_buf,1,length,input_file);
	if( rel != length )
	{
		printf("fwrite %s error!\n",file_name);		
		fclose(fp);
		return -1;
	}

	fclose(fp);
	
	return 1;
}

int check_file(char * dir,FILE * input_file)
{
	struct dirent **namelist;
	int i,total;	
	int ret;
	char indir[1024];
	struct   stat   filestat;
	
	total = scandir(dir,&namelist ,0,alphasort);
	if( total < 0 )
	{
		return 0;
	}
	else
	{
		for(i=0;i<total;i++)
		{
			if( namelist[i]->d_name[0] == '.' )
				continue;

			sprintf(indir,"%s/%s",dir,namelist[i]->d_name);

			stat(indir,&filestat);
			
			if(S_ISREG(filestat.st_mode))   
			{   
   				 printf("%s\n",indir);   
				 ret = write_file(indir, input_file,IS_FILE);
				 if( ret < 0 )
				 {
				 	printf("write_file error!");
					exit(0);
				 }
    			}   
                     else
			{   
				if(S_ISDIR(filestat.st_mode))   
				{   
					 ret = write_file(indir, input_file,IS_DIRECTORY);
					 if( ret < 0 )
					 {
					 	printf("write_file error!");
						exit(0);
					 }
					check_file(indir, input_file);                                            
                            }
                     }

			
		}
		
		for(i=0;i<total;i++)
		{	
			free(namelist[i]);
		}
		
		free(namelist);
	}

	return 1;
	
}

void changestr(char * source,char old_char,char new_char)
{
	int length = 0;
	int i;

	length = strlen(source);

	for( i = 0; i < length ; i++)
	{
		if( source[i] == old_char )
			source[i] = new_char;
	}
}

int splite_file(char * file_name,char * outpath)
{
	FILE * output_file = NULL;
	FILE * new_file = NULL;
	FILE_INFO backup_file;
	int rel;
	int length = 0;
	int write_length= 0;
	char new_path[256];

	output_file = fopen(file_name,"rb");
	if( !output_file )
	{
		printf("can't open %s\n",output_file);
		return -1;
	}

	rel = fseek(output_file, 0, SEEK_END);
	if ( rel < 0 )
	{					
		fclose(output_file);		
		printf("fseek %s error!\n",file_name);
		return -1;
	}					

	length = ftell(output_file);

	if( length >= MAX_FILE_SIZE )
	{
		fclose(output_file);		
		printf(" %s is too long error!\n",output_file);
		return -1;
	}
	
	rewind(output_file);

	rel =  fread(file_buf,1,length,output_file);
	if( rel != length )
	{		
		printf("fread %s error!\n",file_name);
		fclose(output_file);
		return -1;
	}

	fclose(output_file);
	output_file = NULL;

	 remove(file_name);
	

	while( length - write_length > 0 )
	{	
		memcpy(&backup_file,file_buf+write_length,sizeof(FILE_INFO));		

		printf("11file %s \n",backup_file.file_name);

		sprintf(new_path,"%s/%s",outpath,backup_file.file_name);	
		changestr(new_path,'\\','/');
		//printf("2new_path=%s\n",new_path);

		if( backup_file.is_directory == IS_DIRECTORY )
		{
			write_length = write_length + sizeof(FILE_INFO);
			mkdir(new_path,0777);			
		}

		if( backup_file.is_directory == IS_FILE)
		{			
			write_length = write_length + sizeof(FILE_INFO) + backup_file.size;			
		}
	
	}

	write_length = 0;	

	while( length - write_length > 0 )
	{	
		memcpy(&backup_file,file_buf+write_length,sizeof(FILE_INFO));	

		printf("file %s \n",backup_file.file_name);

		sprintf(new_path,"%s/%s",outpath,backup_file.file_name);
		changestr(new_path,'\\','/');
		//printf("2new_path=%s\n",new_path);

		if( backup_file.is_directory == IS_DIRECTORY )
		{
			write_length = write_length + sizeof(FILE_INFO);			
		}

		if( backup_file.is_directory == IS_FILE)
		{		
			new_file = fopen(new_path,"wb");
			if( !new_file )
			{
				printf(" can't create file %s \n",backup_file.file_name);
				exit(0);
			}			

			rel = fwrite(file_buf + write_length + sizeof(FILE_INFO),1,backup_file.size,new_file);
			if( rel != backup_file.size )
			{
				printf("fwrite %s error!\n",backup_file.file_name);		
				fclose(new_file);
				exit(0);
			}

			fclose(new_file);
			
			write_length = write_length + sizeof(FILE_INFO) + backup_file.size;
		}
	
	}

	if(output_file)
	fclose(output_file);
	
	return 1;	
}

int main(int argc, char *argv[])
{

	int audio_fd;
	int omode;
	enum mode mode;
	char para;
  	char filename[30] ;
	int music_fd;	
	signed short applicbuf[4096];
	int count;
	int format;
	int channels = 2;
	int speed =  8000;//8000;//11025; //22050; //44100;  //44.1 KHz
	int totalbyte;
	int totalword;
	int total = 0;
	int totalaudiobyte = 0;
	int temp_tatol = 0;
	struct timeval tvOld;
       struct timeval tvNew;	
	FILE_INFO backup_file;
	struct dirent **namelist;
	int i;
	FILE * input_file = NULL;


	file_buf = (char  * ) malloc(MAX_FILE_SIZE);

	if( !file_buf)
	{
		printf("memory is not enough!\n");
		return 1;
	}


	if (argc < 3)
		usage();

	if (strcmp(argv[1], "input") == 0) 
	{		
		input_file = fopen("update_8120.bin","wb");
		if( !input_file )
		{
			printf(" can't open file fic8120.bin \n");
			exit(0);
		}

		check_file(argv[2],input_file);

		fclose(input_file);	
		
	} else if (strcmp(argv[1], "output") == 0)
	{
		
		splite_file(argv[2],argv[3]);
	} else 
	{
		usage();
	}            

	if( file_buf )
		free(file_buf);

	return 0;
}
