#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<signal.h>

#define KEY 4
int shmid, *roll;


int main(int argc, char *argv[])
{
	shmid = shmget(KEY , sizeof(int) , IPC_CREAT);
	roll = shmat(shmid , NULL, 0);
	int temp;
	scanf("%d", &temp);
	while(1)
	{
		if( *roll == -1)
		{
			*roll = temp;
			break;
		}
	}	
	*roll = temp;

	return 0;
}