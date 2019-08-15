#include "stdio.h"
#include "stdlib.h"

void main()
{
	FILE * fp = NULL;
	int rel = 0;

	int iCount = 0;
		
	int number = 0;

	fp = fopen("1.txt","w+b");
	
	fclose(fp);

		fp = fopen("1.txt","r+b");

		if( !fp )
		{
			printf("no file error!\n");
		}

	while(1)
	{
		sleep(5);	

		number = 1;

		rewind(fp);

		rel = fwrite(&number,1,sizeof(int),fp );
		if( rel != sizeof(int) )
		{
			printf("writenumber error!\n");
		}

		fflush(fp);

		printf("write!number = %d\n",number);	

		iCount++;

		if( iCount > 10 )
		    {
				printf(" write exit(1)");
				fclose(fp);

				fp = NULL;
				exit(1);
		    } 
	}

	return;
}
