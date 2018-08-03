#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/wait.h>
#include<signal.h>

#define KEY 4
#define MAX 30

struct Reg
{
	int roll;
}arr[MAX];

Reg *arr=NULL;
int count = 0;
int shmid, *roll;
int if_present(int roll)
{
	for(int i=0; i<count; i++)
	{
		if(arr[i].roll == roll)
		{
			return 1;
		}
	}
	return 0;
}
int if_roll(int roll)
{
	if(roll>0 && roll<=30)
		return 1;
	else
		return 0;

}

int main(int argc, char *argv[])
{
	int i;
	shmid = shmget(KEY , sizeof(int) , IPC_CREAT | 0777);
	roll = shmat( shmid , NULL, 0);

	*roll = -1;
 	struct shmid_ds data; 
	
	while(1)
	{
		if( *roll != -1)
		{
			if(if_present(*roll))
			{
				printf("already Registered\n");
				*roll = -1;
				continue;
			}
			if(if_roll(*roll))
			{
				i = count;
				count++;
				arr[i].roll = *roll;
				printf("registered successfully\n");
			}
			else
			{
				printf("Invalid roll\n");
			}
				*roll = -1;
		}
	}
	return 0;
}