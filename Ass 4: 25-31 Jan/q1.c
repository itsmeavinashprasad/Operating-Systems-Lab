#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#define BOLD "\033[1m"
#define BOLD_RED "\033[1;31m"
#define RED "\033[0;31m"
#define RESET_TEXT "\033[0m"


void print_usage(char *str)
{
	fprintf(stderr, "%sUsage%s: %s <filename> ... <filename>\n" ,BOLD, RESET_TEXT, str);
	fprintf(stderr, "%sNote%s: \n",BOLD, RESET_TEXT );
	fprintf(stderr, "      1. %cfilename%c must be given with proper path (if in the same path as %s, then can be ignored)\n" , '"', '"', str );
	fprintf(stderr, "      2. file pointed by %cfilename%c must be either a binary executable, or a script starting with a line of the form:\n", '"', '"' );

    fprintf(stderr, "         #! interpreter [optional-arg]\n" );

	return ;
}

void do_some_childish_task(char *str)
{
	// 1. On successful execution of child process i.e. the execve(), process does not return to this source code
	// 2. On an unsucessful execution of child process i.e. the execve(), process return with RETURN_STATUS -1 
	// and error message along with the Usage message gets printed

	char *arr[] = { str , NULL };
	if( execve(arr[0] , arr, NULL) == -1)
	{
		fprintf(stderr, "%s can not be executed by process: %d\n", arr[0], getpid());
		fprintf(stderr, "%sERROR%s: In %s%s%s : ",BOLD_RED, RESET_TEXT, RED, arr[0], RESET_TEXT);
		perror(NULL);
	}

	// on unsuccessful execution of execve() child process terminates itself by exit(1)
	exit(1);	

}
int main(int argc, char *argv[])
{
	// if no filename is provided then only prints the message and Usage
	if(argc == 1)
	{
		fprintf(stderr, "Too few arguments.\n");
		print_usage(argv[0]);
		return 10;
	}

	// else for n filenames, n child processes are spawn and each i-th child executes i-th file provided by the user
	int n = argc - 1;
	pid_t pid;
	for(int i = 0; i < n ; i++)
	{
		// in parent process for() loop runs n-times and in each iteration new child process gets created
		pid = fork();
		if(pid == 0)
		{
			// child process's task
			do_some_childish_task(argv[i+1]);
		}
	}
	return 0;
}