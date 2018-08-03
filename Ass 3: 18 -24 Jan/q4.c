#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

int main()
{
	FILE *fp;
	fp = fopen("sample.txt", "w");
	
	
	
	pid_t id= fork();
	
	if( id == 0 )
	{
		//child
		for(int i=0; i<10; i++)
		{
			puts("child");
			fputs("child\n",fp);
		}
	}
	else
	{
		//parent
		// for(int i=0; i<10; i++)
		// {
		// 	puts("parent");
		// 	fputs("parent\n",fp);
		// }
		fclose(fp);

	}
	// fclose(fp);
	return 0;

}