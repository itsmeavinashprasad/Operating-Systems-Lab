#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>   // for fork()
#include<sys/shm.h>  // for shmget(), shmat(), shmdt()
#include<sys/ipc.h>  // for IPC_PRIVATE, IPC_CREAT, IPC_RMID
#include<sys/wait.h>	// for waitpid()


int main()
{
	//  declare variables required for input
	int m, n, r;
	printf("Enter m , n , r: \n");
	scanf("%d %d %d", &m, &n, &r);

	int arr1[m][n], arr2[n][r];

	// take arra inputs

	printf("Enter elements of the 1st array\n");
	for(int i=0; i<m; i++)
	{
		for(int j=0; j<n; j++)
		{
			scanf("%d", &arr1[i][j] );
		}
	}

	printf("Enter elements of the 2nd array\n");
	for(int i=0; i<n; i++)
	{
		for(int j=0; j<r; j++)
		{
			scanf("%d", &arr2[i][j] );
		}
	}
	// ================ using shmget() ==================
	int shmid = shmget(IPC_PRIVATE , m*r*sizeof(int), IPC_CREAT | 0777);
	
	pid_t pid, c_pid[m*r] ;

	//  =============== MULTIPLYING =====================
	
	int count = 0;
	
	for (int i = 0; i < m; i++) 
	{
		for (int j = 0; j < r; j++) 
		{
			// craete child
			pid = fork();
			if(pid == 0)
			{
				//child process
				int sum = 0;
				for (int k = 0; k < n; k++) 
		        {
		        	// calculate sum
		        	 sum = sum + arr1[i][k]*arr2[k][j];
		        }
		        // attach shared memory
		        int *ptr = shmat(shmid , NULL, 0);
				*(ptr + count) = sum;
				// detach shared memory
				shmdt(ptr);
				// terminate child process
				exit(0);
			}
			else
			{
				// parent process
				c_pid[count] = pid;		// store child's process id
				count ++;
			}
	    }
	}

	
	int status;
	for (int i = 0; i < m*r; ++i)
	{
		// wait for m*r child to terminate
		waitpid( c_pid[i] , &status , 0);
	}

	
	printf("====================== RESULT ======================\n");
	// attach shared memory to parent because all child has been terminated
	int *ptr = shmat(shmid , NULL, 0 );
	count = 0;
	// ===================== print output ===========================
	for(int i=0; i<m; i++)
	{
		for(int j=0 ; j<r; j++)
		{
			printf("%d ",*(ptr+count) );
			count++;
		}
		printf("\n");
	}

	shmdt(ptr);  //detach shared memory
	shmctl(shmid, IPC_RMID, NULL);	// destroy shared memory
	return 0;
}