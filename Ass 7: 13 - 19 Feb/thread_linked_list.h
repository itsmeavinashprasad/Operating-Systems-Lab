#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<errno.h>

#define MAX_LIST_COUNT 5		// this mny concurrent list can be managed

// node structure
struct node
{
	int value;					// stores integer value
	struct node* nextnode;		// stores address of next node
};
typedef struct node node;

// structure of head
struct head
{
	node **arr_node_heads;		// stores starting node's address of all linked lists
	pthread_mutex_t *arr_mutex;	// stores mutex corresponding to each linked list
};

typedef struct head head;
head *head_struct ;

int init();
int insertFirst(int list_no, int value);
int insertLast(int list_no, int value);
int displayList(int list_no);
int deleteAt(int list_no, int index);
int getIndex(int list_no, int value);
int deleteList(int list_no);
int setValue(int list_no, int index, int value);
int getValue(int list_no, int index);