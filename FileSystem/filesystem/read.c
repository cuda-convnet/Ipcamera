#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>

void FormatFlash()
{
	char cmd[50];
	int res;

	printf(" clear flash!\n");
	strcpy(cmd,"eraseall /dev/mtd2");
	res = system(cmd);
	if( res == -1 )
		printf("%s error\n",cmd);
}

int  main()
{
	FILE * fp = NULL;
	int rel = 0;
	int iCount = 0;
	int number = 0;

	if( fork() == 0 )
	{		
		sleep(20);

		fp = fopen("/cjy/MachineRunStatus","r+b");

		if( !fp )
		{
			printf("no file error!\n");
			FormatFlash();
			exit(1);
		}

		while(1)
		{
			sleep(20);

			rewind(fp);

			rel = fread(&number,1,sizeof(int),fp );
			if( rel != sizeof(int) )
			{
				printf("read number error!\n");
				FormatFlash();
				exit(1);
			}

			printf("read! number = %d\n",number);

			if( number != 1 ) 
			{
				FormatFlash();
				exit(1);
			}

			if( iCount >= 2 )
			{
				fclose(fp);
				printf(" startcheck out!\n");
				return 1;
			}

			iCount++;

			rewind(fp);

			number = 0;

			rel = fwrite(&number,1,sizeof(int),fp );
			printf("write! number = %d\n",number);

			fflush(fp);
		
		}

		return 1;
	}
	else
	{
		system("./ah8004");
	}
}
