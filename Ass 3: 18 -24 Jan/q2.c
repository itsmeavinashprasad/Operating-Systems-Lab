#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char const *argv[])
{
	//wrong input
	if(argc != 2)
	{
		printf("%s\n","Wring Input. Exiting..." );
		exit(255);
	}
	
	//input n
	int temp  = (int) *argv[1];
	int n  = temp - 48;
	printf("Entered, n: %d\n", n);
	
	if(n == 0)
		printf("%s\n","Not Possible" );
	else if(n == 1)
	{
		printf("%s\n","Yup, Parent Process is up and Running" );
		exit(1);
	}
	else
	{
		pid_t id1, id2;
		printf("%s\n","I am Parent" );
		
		for(int i = 0; i < n-1 ; i++)
		{
			if(i==0)
			{
				id1 = fork();

			}
			if(id1 == 0 )
			{
				// child
				printf("%s\n","I am child" );
			}
			else
			{
				// parent
				fork();
			}
		}
		
	}
		
	return 0;
}