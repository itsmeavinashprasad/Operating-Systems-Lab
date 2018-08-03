#include "thread_linked_list.h"
int init()
{
	//  must be called once to initialize head structure
	if(head_struct == NULL)
	{
		// TRUE when initializing first linked lis
		// this if cond also ensures head_struct and all variables corresponding to head_struct also initialized
		printf("init() for first time\n");
		head_struct = (head*) malloc(sizeof(head));
		head_struct->arr_node_heads = (node**) malloc(MAX_LIST_COUNT * sizeof(node*));
		head_struct->arr_mutex = (pthread_mutex_t*) malloc(MAX_LIST_COUNT * sizeof(pthread_mutex_t));
		head_struct->reading = (int*) malloc( MAX_LIST_COUNT * sizeof(int));
		// initialize all starting node pointers by NULL
		for(int i=0; i<MAX_LIST_COUNT; i++)
		{
			head_struct->arr_node_heads[i] = (node *) NULL;
			pthread_mutex_init( head_struct->arr_mutex + i, NULL);
			head_struct->reading[i] = 0;
		}
	}
	return 0;
}

int checkInput(int list_no, int level)
{
	//  this func checks and  return errors
	//  level is used for different levels of error cheking
	if(list_no > MAX_LIST_COUNT && level>0)
	{
		fprintf(stderr, "List No exceeded MAX_LIST_COUNT\n" );
		return -3;
	}
	if(head_struct == NULL && level>1 )
	{
		fprintf(stderr, "Please use init() first.\n");
		return -4;
	}
	if(head_struct->arr_node_heads[list_no] == NULL && level>2)
	{
		fprintf(stderr, "Linked Thread %ld List %d is empty\n", pthread_self(), list_no);
		return -5;
	}
	return 0;
}
int insertFirst(int list_no, int value)
{
	// used to insret  node at first of the list
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}	

	node *newnode = (node*) malloc(sizeof(node));
	newnode->value = value;
	newnode->nextnode = head_struct->arr_node_heads[list_no];
	head_struct->arr_node_heads[list_no] = newnode;
	printf("Thread %ld List %d Value %d Inserted at first\n", pthread_self(), list_no, value );
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return 0;
}

int insertLast(int list_no, int value)
{
	// used to insert node at last of the list
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}

	node *newnode = malloc(sizeof(node));
	newnode->value = value;
	newnode->nextnode = (node*) NULL;
	if(head_struct->arr_node_heads[list_no] == NULL)
	{
		// list is empty now
		head_struct->arr_node_heads[list_no] = newnode;
	}
	else
	{
		// list is not empty
		node *current_node = head_struct->arr_node_heads[list_no];
		while(current_node->nextnode != NULL)
			current_node = current_node->nextnode;
		current_node->nextnode = newnode;		
	}
	printf("Thread %ld List %d Value %d Inserted at last.\n", pthread_self(), list_no, value);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return 0;
}

int displayList(int list_no)
{
	// display the contents of the list by tarversing linearly
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 3);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}

	int count = 0;
	printf("=============== Thread: %ld  DISPLAY LIST ================================\n", pthread_self());
	node *current_node = head_struct->arr_node_heads[list_no];
	do
	{
		printf("Index: %d \t Value: %d \t\n", count++, current_node->value );
		current_node = current_node->nextnode;
	}while(current_node != NULL);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return 0;
}

int deleteAt(int list_no, int index)
{
	//  used to delete node at index <index>
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}
	
	node *current_node = head_struct->arr_node_heads[list_no];
	if(index == 0)
	{
		// TRUE when deleteing first node
		head_struct->arr_node_heads[list_no] = current_node->nextnode;
		printf("Thread %ld List %d Index %d deleted.\n", pthread_self(), list_no, index);
		free(current_node);
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return 0;
	}
	int count = 0;
	node *prev_node = current_node;
	do
	{
		count++;
		current_node = prev_node->nextnode;
		if(count == index)
		{
			prev_node->nextnode = current_node->nextnode;
			printf("Thread %ld List %d Index %d deleted.\n", pthread_self(), list_no, index);
			free(current_node);
			pthread_mutex_unlock(head_struct->arr_mutex + list_no);
			return 0;
		}

		prev_node = current_node;
	} while (current_node != NULL);
	fprintf(stderr, "INDEX IS GRATER THAN THE LIST LENGTH\n");
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return -1;
}

int getIndex(int list_no, int value)
{
	// search values lineraly and return he first occurance of <value>
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}

	node *current_node = head_struct->arr_node_heads[list_no];
	int count = 0;
	while(current_node != NULL)
	{
		if(current_node->value == value)
		{
			printf("Thread %ld List %d Index %d Value %d\n", pthread_self(), list_no, count, value);
			pthread_mutex_unlock(head_struct->arr_mutex + list_no);
			return count;
		}	
		current_node = current_node->nextnode;
		count++; 
	}

	fprintf(stderr, "Value %d not found in Thread %ld List %d .\n", value, pthread_self(), list_no);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return -1;
}

int deleteList(int list_no)
{
	// used to delet the all nodes corresponding to the list and the head is set to NULL
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 3);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}
	node *current_node = head_struct->arr_node_heads[list_no];
	head_struct->arr_node_heads[list_no] = (node *) NULL;
	node *next_node = current_node->nextnode;
	while(next_node != NULL)
	{
		current_node = next_node;
		next_node = current_node->nextnode;
		free(current_node);
	}
	printf("Thread %ld List %d deleted.\n", pthread_self(), list_no);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return 0;
}

int setValue(int list_no, int index, int value)
{
	// set a user defiend value at alreday inserted node given by index <index>
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}

	node *current_node = head_struct->arr_node_heads[list_no];
	int count = 0;
	while(current_node != NULL)
	{
		if(count == index)
		{
			current_node->value = value;
			printf("Thread %ld List %d Index %d Value %d \n", pthread_self(), list_no, index, current_node-> value);
			pthread_mutex_unlock(head_struct->arr_mutex + list_no);
			return 0;
		}
		count++;
		current_node = current_node->nextnode;
	}
	fprintf(stderr, "Index %d is graeter than the length of the Thread %ld List %d\n", index, pthread_self(), list_no);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return -1;
}

int getValue(int list_no, int index)
{
	// returns the value of the index 
	pthread_mutex_lock(head_struct->arr_mutex + list_no);
	int stat = checkInput(list_no, 2);
	if(stat != 0)
	{
		pthread_mutex_unlock(head_struct->arr_mutex + list_no);
		return stat;
	}

	node *current_node = head_struct->arr_node_heads[list_no];
	int count = 0;
	while(current_node != NULL)
	{
		if(count == index)
		{
			pthread_mutex_unlock(head_struct->arr_mutex + list_no);
			printf("Thread %ld List %d Index %d Value %d\n", pthread_self(), list_no, index, current_node->value);
			return current_node->value;
		}	
		count++;
		current_node = current_node->nextnode;
	}
	fprintf(stderr, "Index %d is graeter than the length of the Thread %ld List %d\n", index, pthread_self(), list_no);
	pthread_mutex_unlock(head_struct->arr_mutex + list_no);
	return -1;
}
