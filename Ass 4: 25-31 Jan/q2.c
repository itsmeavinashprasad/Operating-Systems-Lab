#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#define BOLD "\033[1m"
#define BOLD_RED "\033[1;31m"
#define RED "\033[0;31m"
#define RESET_TEXT "\033[0m"

void print_usage(char *str)
{
	fprintf(stderr, "%sUsage%s: %s <filename 1> <followed by swiches of filename 1> ... <filename 2> <...>\n" ,BOLD, RESET_TEXT, str);
	fprintf(stderr, "%sNote%s: \n",BOLD, RESET_TEXT );
	fprintf(stderr, "      1. %cfilename%c must be given with proper path (if in the same path as %s, then can be ignored)\n" , '"', '"', str );
	fprintf(stderr, "      2. file pointed by %cfilename%c must be either a binary executable, or a script starting with a line of the form:\n", '"', '"' );

    fprintf(stderr, "         #! interpreter [optional-arg]\n" );
	return ;
}
void do_some_childish_task(char *foo, char *arr[], int n)
{
	// 1. On successful execution of child process i.e. the execve(), process does not return to this source code
	// 2. On an unsucessful execution of child process i.e. the execve(), process return with RETURN_STATUS -1 
	// and error message along with the Usage message gets printed

	arr[n] = NULL;
	if( execve(arr[0] , arr, NULL) == -1)
	{
		fprintf(stderr, "%s can not be excecuted by process: %d\n", arr[0], getpid());
		fprintf(stderr, "%sERROR%s: In %s%s%s : ",BOLD_RED, RESET_TEXT, RED, arr[0], RESET_TEXT);
		perror(NULL);
		print_usage(foo);
	}

	// on unsuccessful execution of execve() child process terminates itself by exit(1)
	exit(1);	

}


int main(int argc, char *argv[])
{
	// if no filename is provided
	if(argc == 1)
	{
		fprintf(stderr, "Too few arguments.\n");
		print_usage(argv[0]);
		return 10;
	}

	
	char *arr[argc-1];
	pid_t pid;
	int i = 1, count = 0;
	
	
	if(argc == 2)
	{
		// if only one filename without any switch is provided
		// then insert that element into arr and execute child process's task
		pid = fork();
		if(pid ==0 )
		{
			arr[0] = *(argv + 1);
			count++;
			do_some_childish_task( argv[0] , arr, count);
		}
		
	}

	// when one file with switch(es) or more than one filenames with/without switch(es) are provided
	
	while( i < argc )	// untill last argument is traversed
	{
		if(i == 1) 
		{
			// traversing 1st argument
			// increase count value and insert argument in arr
			count++;
			arr[count-1] = *(argv+i);
		}
		else if(i == argc-1)
		{
			// traversing last argument
			if(argv[i][0] == '-')
			{
				// if last argument is switch
				// then insert that argument in arr and execute child process
				count++;
				arr[count -1] = *(argv+i);
				pid = fork();
				if(pid == 0)
				{
					do_some_childish_task(argv[0] , arr, count);
				}
			}
			else
			{
				// when last argument is itself a filename
				// then excute child process's task upto previous element kep inside arr
				// again execute child process's task by inserting last argumnrt in arr 
				pid = fork();
				if(pid == 0)
				{
					do_some_childish_task(argv[0] , arr, count);
				}
				else
				{
					count = 0;
					count++;
					arr[count-1] = *(argv+i);
					pid = fork();
					if(pid == 0)
					{
						do_some_childish_task(argv[0],arr, count);
					}
				}
			}
		}
		else if(argv[i][0] != '-')
		{
			// if current argument is itself a filename 
			// then execute child process's task upto previous element and 
			// freshly insert current argumnet into arr
			pid = fork();
			if(pid == 0)
			{
				do_some_childish_task(argv[0] , arr, count);
			}
			else
			{
				count = 1;
				arr[count-1] = *(argv+i);
			}
		}
		else
		{
			// when current argument is switch then simply insert that switch into arr
			count++;
			arr[count-1] = *(argv+i);
		}
		
		i++;
	}

	return 0;
}