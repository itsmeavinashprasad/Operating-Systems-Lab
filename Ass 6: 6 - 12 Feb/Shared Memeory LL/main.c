#include <stdio.h>
#include<unistd.h>
#include<sys/wait.h>
#include "sharedll.h"

int main(int argc, char *argv[])
{
	int n = 1;
	int pid, c_pid[n];
	for (int i = 0; i < n; ++i){
		pid = fork();
		if(pid == 0){
			// child process
			// printf("INSIDE CHILD\n");
			createList(0);
			int x ; 
			for(int k=0; k<5; k++){
				x = 11 * (k+1);
				insertFirst(0, (void*) &x, sizeof(x));
				// insertFirst(0, (void*) &x, sizeof(x));
			}
			exit(0);
		}
		else{
			// parent process
			c_pid[i] = pid;
		}
	}

	for (int i = 0; i < n; ++i){
		waitpid( c_pid[i] , NULL , 0);
		printf("child %d ended\n", i);
	}
	// exit(0);
	printf("===================== LIST 0 =====================\n");
	displayList(0);
	// int x = 20;
	// printf("INDEX of x: %d is %d\n",x, getIndex(0, (void*)&x, sizeof(x) ));
	// printf("===================== LIST 0 =====================\n");
	// displayList(0);
	deleteList(0);


	/*
	printf("================== WELCOME TO SHARED LINKED LIST ========================\n");
	int option = 1, value, status, index, list_no;
	while(option != 0)
	{
		printf("0. Quit\n");
		printf("1. Create List\n");
		printf("2. Insert Node at the end\n");
		printf("3. Insert Node at the begining\n");
		printf("4. Display Contents\n");
		printf("5. Delete Node\n");
		printf("6. Search Index\n");
		printf("7. Get Value\n");
		printf("8. Update Value\n");
		printf("9. Delete List\n");
		printf("Enter your option: ");
		scanf("%d", &option);
		switch(option)
		{
			case 0: 
				printf("Quiting ...... \n");
				// clear_all();
				return 0;
			case 1:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				status = createList(list_no);
				printf("Return Value: %d\n", status);
				break;
			case 2:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Value: ");
				scanf("%d", &value);
				status = insertLast(list_no, value);
				printf("Return Value: %d\n", status);
				break;
			case 3:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Value: ");
				scanf("%d", &value);
				status = insertFirst(list_no, value);
				printf("Return Value: %d\n", status);
				break;
			case 4:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				status = displayList(list_no);
				printf("Return Value: %d\n", status);
				break;
			case 5:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Index: ");
				scanf("%d", &index);
				status = deleteNode(list_no, index);
				printf("Return Value: %d\n", status);
				break;
			case 6:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Value: ");
				scanf("%d", &value);
				status = getIndex(list_no, value);
				printf("Return Value: %d\n", status);
				break;
			case 7:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Index: ");
				scanf("%d", &index);
				status = getVal(list_no, index);
				printf("Return Value: %d\n", status);
				break;
			case 8:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				printf("Enter Index: ");
				scanf("%d", &index);
				printf("Enter Value: ");
				scanf("%d", &value);
				status = updateVal(list_no, index, value);
				printf("Return Value: %d\n", status);
				break;
			case 9:
				printf("Enter list_no: ");
				scanf("%d", &list_no);
				status = deleteList(list_no);
				printf("Return Value: %d\n", status);
				break;
			default:
				printf("ERROR: Wrong input. Please Try Again\n");
				break;
		}
		getchar();
		printf("==================================================================\n");
	}
	*/
	return 0;
}