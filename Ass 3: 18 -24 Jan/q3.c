#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
void fact(int n)
{
	int x, result=1;
	x = n;
	while(1)
	{
		if(x==0)
		{
			break;
		}
		else
		{
			result *= x;
			x--;
		}
	}
	printf("Factorial of %d is : %d\n",n,result );
}
int main(int argc, char const *argv[])
{
	int n = argc-1;
	int arr[n];

	//input n
	printf("Entered %d inputs :", n);
	for(int i=0; i<n; i++)
	{
		arr[i] = (int) *(argv[i+1]) - 48;
		printf("%d ",arr[i]);
	}
	printf("\n");
	
	
	
	if(n == 1)
	{
		fact(arr[0]);
		exit(1);
	}
	else
	{
		pid_t id1, id2;
		
		for(int i = 0; i < n-1 ; i++)
		{
			if(i==0)
			{
				fact(arr[0]);
				id1 = fork();

			}
			if(id1 == 0 )
			{
				// child
				fact(arr[i+1]);
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