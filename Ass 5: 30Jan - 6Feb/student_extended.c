#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>		// for shmget(), shmat(), shmdt(), shmctl()
#include<sys/ipc.h>     // for IPC_STAT
#include<signal.h>      // for signal()
#include<unistd.h>		// for sleep()

// text formattong macros
#define BOLD "\033[1m"
#define RED "\033[0;31m"
#define RESET_TEXT "\033[0m"

// Global variables
int shmid, *roll;

void signal_handler1(int sig)	// signal hander for SIGUSR1
{
    signal(sig, SIG_IGN);	// ignore default functionalities of signal 
	printf("%sRoll no already registered.%s\n", RED, RESET_TEXT);
	shmdt(roll);
	exit(sig);
}
void signal_handler2(int sig)	// signal handler for SIGINT
{
    signal(sig, SIG_IGN);
	printf("Attendance Registered Successfully.\n");
	shmdt(roll);
	exit(sig);
}
void signal_handler3(int sig)	// signal handler for SIGUSR2
{
    signal(sig, SIG_IGN);
	printf("%sTried to enter an Invalid Roll No.%s\n", RED, RESET_TEXT);
	shmdt(roll);
	exit(sig);
}

int main(int argc, char *argv[])
{
	if(argc!= 2)
	{
		fprintf(stderr, "%sUsage%s: %s <Roll_No>\n" ,BOLD, RESET_TEXT, argv[0]);
		exit(2);
	}
	if (signal(SIGUSR1, signal_handler1) == SIG_ERR)	// installing signal handler for SIGUSR1
	{
		printf("Failed to install signal handler 1. Exiting...\n");
		exit(1);

	}
	if (signal(SIGINT, signal_handler2) == SIG_ERR)		// installing signal handler for SIGINT
	{
		printf("Failed to install signal handler 2. Exiting...\n");
		exit(2);
	}
	if (signal(SIGUSR2, signal_handler3) == SIG_ERR)	// installing signal handler for SIGUSR2
	{
		printf("Failed to install signal handler 3. Exiting...\n");
		exit(2);
	}

	int key = ftok("./teacher.c", 5);	// generating key 

	// int shmget(key_t key, size_t size, int shmflg);
	shmid = shmget(key , sizeof(int) , IPC_CREAT);		// getting shared memory id using key
	roll = shmat(shmid , NULL, 0);		// attaching shared memory with this process
	int temp, var = 0;
	sscanf(argv[1] , "%d", &temp);		// gettng roll no from console inputs
	
	while(1)
	{
		// checks continuously whether shared variable's value is -1 or not
		// whenever multiple students tries to enter their roll no (may be simultaneously) 
		// and theteacher yet not registered previously entered roll no by the another student
		// or the teacher has been not started yet 
		if( *roll == -1)
		{
			// TRUE when it finds shared variable has been reset
			*roll = temp;
			break;
		}
		else if( var == 0)
		{
			var = 1;
			printf("Teacher is busy in Registering other student's roll no\n");
		}
	}	
	sleep(5);		// sleep() is required so that student waits for an signal from the teacher process
					// at least for a limited time 

	// instructions afterwards this point never get to be executed 
	return 0;
}