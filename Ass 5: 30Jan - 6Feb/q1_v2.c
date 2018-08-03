#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>   // for fork()
#include<sys/shm.h>  // for shmget(), shmat(), shmdt()
#include<sys/ipc.h>  // for IPC_PRIVATE, IPC_CREAT, IPC_RMID
#include<sys/wait.h>	// for waitpid()

int main(int argc, char *argv[])
{
	int no_of_matrices;
	printf("Enter no of matrices: ");
	scanf("%d", &no_of_matrices);

	int dimension[no_of_matrices+1];	// array to store length of subsequence matrix orders

	int **matrices[no_of_matrices];		// 3-D matrix to keep input matrix

	//TAKE USER INPUT
	for(int i=0; i<no_of_matrices; i++)
	{
		printf("Enter dimension of matrix: %d\n", i+1);
		int temp1, temp2;
		scanf("%d %d", &temp1, &temp2 );
		if(i != 0)
		{
			// TRUE when user is not entering 1st matrix
			// This block checks whether entered dimension are correct for multiplication or not
			if(temp1 != dimension[i])
			{
				// invalid dimension
				printf("Invalid Combination of Dimension of Matrix Entered. Please Try Again\n");
				i--;
				continue;
			}
		}
		// initialize variables
		dimension[i] = temp1;
		dimension[i+1] = temp2;
		//dynamically alloacate memory for matrix of different orders
		matrices[i] =(int **) malloc(dimension[i] * sizeof(int * ));
		for(int j=0; j<dimension[i]; j++)
		{
			matrices[i][j] = (int *) malloc(dimension[i+1] * sizeof(int));
		}

		printf("Enter matrix %d : ,  Dimension: row = %d ,col = %d: \n", i+1, dimension[i], dimension[i+1]);
		for(int j=0; j<dimension[i]; j++)
		{
			for(int k=0; k<dimension[i+1]; k++)
			{
				printf("Matrix[%d], element [%d][%d]: ", i+1, j+1 ,k+1 );
				scanf("%d", &matrices[i][j][k]);
			}
		}
		printf("====================  OKAY DONE ==========================\n\n");

	}

	// DIAPLAY USER INPUT
	printf("====================  USER INPUT ==========================\n");
	for(int i=0; i<no_of_matrices; i++)
	{
		printf("\nMatrix %d :\n", i+1);
		for(int j=0; j<dimension[i]; j++)
		{
			for(int k=0; k<dimension[i+1]; k++)
			{
				printf("%d ", matrices[i][j][k]);
			}
			printf("\n");
		}
	}
	
	// START MULTIPLICATION ALGORITHM
	// decaring variables
	int shmid , **arr1 , **arr2;
	pid_t pid;
	int **result; 
	result = (int**) malloc(sizeof(int *));

	for(int l=0; l<no_of_matrices -1 ; l++)
	{
		// attach shmid
		shmid = shmget(IPC_PRIVATE , dimension[l] * dimension[l+2] * sizeof(int), IPC_CREAT | 0777);
		if(l == 0)
			arr1 = matrices[0];
		
		arr2 = matrices[l+1];

		int count = 0;
		int m = dimension[0], n = dimension[l+1] , r = dimension[l+2];

		for (int i = 0; i < m; i++) 
		{
			for (int j = 0; j < r; j++) 
			{
				// create child
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
					count ++;
				}
		    }
		}
		waitpid(pid , NULL, 0);		// wait for last child process to calculate result 
									// assuming all child processes prior to last child processes have already completed execution

		
		count = 0;
		int *ptr = shmat(shmid , NULL, 0);		// attach shared memory with this process
		// make result matrix dynamically
		result = (int**) realloc(result , m * sizeof(int*));
		for( int p=0; p<m; p++)
			result[p] = (int*) realloc(result[p] , r * sizeof(int));
		// copy resultant matrix to this process
		for(int p=0; p<m; p++)
		{
			for(int q=0; q<r; q++)
			{
				result[p][q] = *(ptr + count);
				count++;
			}
		}
		// reassign variable for next iteration
		arr1 = result;
		// destroy shared memory
		shmctl(shmid, IPC_RMID, NULL);
	}

	// display final result
	printf("====================  RESULT ==============================\n");
	for (int i = 0; i < dimension[0]; ++i)
	{
		for (int j = 0; j < dimension[no_of_matrices]; ++j)
		{
			printf("%d ",result[i][j] );
		}
		printf("\n");
	}
	return 0;
}