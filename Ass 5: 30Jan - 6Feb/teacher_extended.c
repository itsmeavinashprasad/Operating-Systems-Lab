#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>		// for shmget(), shmat(), shmdt(), shmctl()
#include<sys/ipc.h>     // for IPC_RMID, IPC_CREAT, IPC_STAT
#include<signal.h>      // for kill(), signal()


// text formatting macros
#define BOLD "\033[1m"
#define BOLD_RED "\033[1;31m"
#define RED "\033[0;31m"
#define RESET_TEXT "\033[0m"

// macro for checking valid roll no i.e. in range [1-100]
#define IF_ROLL(x) ( (x >0 && x<101)? 1 : 0 )

// structure for registering student details 
struct Register
{
	int roll;		// roll no of a  student, responded by the student process
	pid_t student_pid;		// process id through which student tried to give attendance
};
typedef struct Register Register;

// Globalvariables
Register *arr=NULL;
int no_of_students = 0;
int shmid, *roll;

// function for checking if a student of roll no "roll" is alreday present or not
int if_present(int roll)
{
	for(int i=0; i<no_of_students; i++)
	{
		if(arr[i].roll == roll)
		{
			// student is already present
			return 1;
		}
	}
	// student is not present
	return 0;
}

void signal_handler(int sig)
{
	//  signal handler install for interrupt signal
	//  when Ctrl+C pressend in keyboard, i.e. an interrupt input passed through keyboard 
	// teacher process stops its execution and displays the Register details
	signal(sig, SIG_IGN);		// ignore default functionalities of the signal
	
	// print data
	printf("\n");
	printf("================================================================\n");
	printf("                   DISPLAYING REGISTER STATUS                   \n");
	printf("================================================================\n");
	printf("              |Roll NO|                    |Process ids|        \n");
	printf("----------------------------------------------------------------\n");
	
	for(int i=0; i<no_of_students; i++)
	{
		printf("                 %d                           %d              \n", arr[i].roll, arr[i].student_pid);

	}
	printf("================================================================\n");
	shmdt(roll);  //detach shared memory
	shmctl(shmid, IPC_RMID, NULL);	// destroy shared memory
	exit(sig);	// exit teacher process
}


int main(int argc, char *argv[])
{
	if(argc!= 1)
	{
		fprintf(stderr, "%sUsage%s: %s \n" ,BOLD, RESET_TEXT, argv[0]);
		exit(2);
	}
	int key = ftok("./teacher.c", 5);		// generating key for shared memory
	
	if(signal(SIGINT , signal_handler) == SIG_ERR)		// installing signal handler
	{
		printf("Failed to install signal handler. Exiting\n");
		exit(2);
	}

	int index;

	// int shmget(key_t key, size_t size, int shmflg);
	shmid = shmget(key , sizeof(int) , IPC_CREAT | 0777);		// geting shared memory id using generated key
	printf("Shared Memory acquired. SHMID: %d \n", shmid);
	roll = shmat( shmid , NULL, 0);			// attaching shared memory space to current process's local space
	
	if(roll== (void *)-1)
	{
		printf("Error in attaching shared memory\n");
	}

	arr = (Register *)malloc(sizeof(Register));		// making a dynamic array of structure Register of size f one element
	*roll = -1;		// initializing shared variable
 	struct shmid_ds data;		// structure to pass to shmctl()
	
	printf("Teacher is Ready for attendance\n");
	printf("Press CTRL+C anytime to terminate TEACHER PROCESS and print register status\n");
	while(1)
	{
		// run infinitely untill an interrupt signal comes
		if( *roll != -1)
		{	// TRUE when student is asking for attendance
			
			// int shmctl(int shmid, int cmd, struct shmid_ds *buf);
			shmctl(shmid , IPC_STAT,(struct shmid_ds*) &data);		// copeis shared memory status to "data"
			if(if_present(*roll))		// whenever student process changes the shared variable
			{
				//  TRUE when student is alreday present 
				printf("-> %sStudent %s%d%s is already present %s\n",RED, BOLD_RED, *roll, RED, RESET_TEXT);
				kill(data.shm_lpid , SIGUSR1);		// send user defined signa SIGUSR1 to student process
				*roll = -1;
				continue;
			}
			if(IF_ROLL(*roll))
			{
				// TRUE when student tries to enter a valid roll no which was not entered previously
				index = no_of_students;
				no_of_students++;

				// reallocating "arr" by increasing its size by one element and storing values
				arr = (Register*) realloc( arr , no_of_students*sizeof(Register));	
				arr[index].roll = *roll;
				arr[index].student_pid = data.shm_lpid;
				printf("-> %sREGISTERED:%s    Roll no: %d \t PID: %d\n",BOLD, RESET_TEXT, arr[index].roll , arr[index].student_pid );
				kill(data.shm_lpid , SIGINT);	// send SIGINT to student process
			}
			else
			{
				//  TRUE when student tries to enter an invalid roll no 
				kill(data.shm_lpid , SIGUSR2);		// send SIGUSR2 to student process
				printf("-> %sRoll %s%d%s is a Invalid roll no%s\n",RED, BOLD_RED, *roll, RED, RESET_TEXT);
			}
				*roll = -1;	// resets the roll no
		}
	}// end of while, the program execution never executes instruction afterwards this point


	return 0;
}