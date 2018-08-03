#include<stdio.h>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/ipc.h>
#include<signal.h>
#include<errno.h>
#include<string.h>

#define supressStatusMsg 0
typedef struct node
{
 	int value_shmid;										// holds shmid of value stored in that node
 	int next_shmid;											// shmid of the next node, if last node then -1
}node; 

typedef struct list_descriptor
{	
	int shmid;												// shmid of the head node
}list;

typedef union semun 
{
	int              val;    								/* Value for SETVAL */
	struct semid_ds *buf;    								/* Buffer for IPC_STAT, IPC_SET */
	unsigned short  *array;  								/* Array for GETALL, SETALL */
	struct seminfo  *__buf;  								/* Buffer for IPC_INFO (Linux-specific) */
}semun;



int createList(int list_no);								// used to create a list descriptor and initialize it

int insertFirst(int list_no, void *ptr, size_t size);		// insert at the starting position of a list

int insertLast(int list_no, void *ptr, size_t size);		// insert at the end position of a list

int displayList(int list_no);								// displays the contains of the list
															// works only with integer data type
															// created for testing purpose
															// use getVal() instead for accessing value in a node in your programs 

int deleteNode(int list_no, int index);						// delete node at index <index>

int deleteList(int list_no);								// delete whole list including its list descriptor
															// after using it you have to use createList() for operations on that list

void* getVal(int list_no, int index);						// used to getting the value of <list_no> and <index> in your program

int setVal(int list_no, int index, void *ptr, size_t size);	// set value of any index of a list

int getIndex(int list_no, void *ptr, size_t size);			// search linearly and returns the first occurrence of the value

