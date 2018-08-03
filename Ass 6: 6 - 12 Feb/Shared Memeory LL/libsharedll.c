#include "sharedll.h"

int V(int semid)
{
	// increment the semaphore value
	// return status of the semop() operation
	struct sembuf buffer;
	buffer.sem_num = 0;
	buffer.sem_op = 1;
	buffer.sem_flg = 0;
	return semop(semid, &buffer, 1);
}

int P(int semid)
{
	// decrement the semaphore value
	// return status of the semop() operation
	struct sembuf buffer;
	buffer.sem_num = 0;
	buffer.sem_op = -1;
	buffer.sem_flg = 0;
	return semop(semid, &buffer, 1);
}

void lockMemory(int shmid)
{	
	// creates the semaphore corresponding to the shared memory shmid, using shmid as a key and performs P()
	int semid = semget(shmid, 1, IPC_CREAT | IPC_EXCL | 0777);
	if(semid >0){
		// TRUE when semaphore successfully created
		// initialize its values and set semaphore value as '1'
		semun setVal;		
		setVal.val = 1;
		semctl(semid, 0, SETVAL, setVal);
	}
	if(semid == -1 && errno == EEXIST){
		// TRUE when semaphore already created for shmid
		// then fetch its semid and use it for P() operation
		semid = semget(shmid, 1,IPC_EXCL);
	}
	P(semid);
	if(supressStatusMsg)
		printf("shmid %d semid %d locked\n", shmid, semid);
	return ;
}

void unlockMemory(int shmid)
{
	// gets the semaphore corresponding to the shared memory shmid, using shmid as a key and performs V()
	int semid = semget(shmid, 1, IPC_EXCL);
	V(semid);
	if(supressStatusMsg)
		printf("shmid %d semid %d unlocked\n", shmid, semid);
	return;
}

void releaseSem(int shmid)
{
	// delete semaphore after using it, semid retrieved using shmid
	int semid = semget(shmid, 1, IPC_EXCL);
	semctl(semid, 0, IPC_RMID);
	if(supressStatusMsg)
		printf("shmid: %d  semid: %d removed\n", shmid, semid);
	return;	
}

int getList_shmid(int list_no)
{
	// returns list descriptors shmid corresponding to <list_no>
	return shmget( ftok("/bin/ls",list_no), sizeof(list), IPC_EXCL);
}
list* getListDescriptor(int list_no)
{
	int list_shmid = getList_shmid(list_no);
	if( list_shmid == -1){
		// TRUE when List <list_no> is not created
		// get shmid of that list descriptor and return handle (listptr) of that list
		// if error occurs then return -1
		if(supressStatusMsg)
			fprintf(stderr, "List no %d not created. Please use createList( %d ) first. ERRNO: %d\n", list_no, list_no, errno);
		return (void*) -1; 
	}
	else{
		// TRUE when list descriptor corresponding to <list_no> already created
		// simply attach pointer to that list descriptor
		lockMemory(list_shmid);
		list *listptr = shmat(list_shmid, NULL, 0);
		if(listptr == (void*) -1){
			// TRUE when attaching failed for current process
			if(supressStatusMsg)
				fprintf(stderr, "Failed to attach list %d. ERRNO: %d\n", list_no, errno);
			unlockMemory(list_shmid);
			return (void *) -1;
		}
		else{
			unlockMemory(list_shmid);
			return listptr;
		}
	}
} 
int createList(int list_no)
{
	// This function is used to initialize a list descriptor corresponding to list <list_no>
	// If the list already exists then first shmget() fails and -1 is returned
	// If list does not exists but list creation fails then also return -1
	// Otherwise initialize its values and return 0
	int key = ftok("/bin/ls", list_no);
	int list_shmid = shmget(key, sizeof(list), IPC_CREAT | IPC_EXCL | 0777);
	if( errno == EEXIST){
		// TRUE when list corresponding to <list_no> already exists
		if(supressStatusMsg)
			fprintf(stderr, "List no %d already exists. ERRNO: %d\n", list_no, errno);
		return -1;
	}
	else if(list_shmid == -1){
		// TRUE when fails to create list descriptor 
		if(supressStatusMsg)
			fprintf(stderr, "Creation of list %d failed, with ERRNO: %d\n", list_no, errno );
		return -1;
	}

	lockMemory(list_shmid);
	list *listptr = shmat(list_shmid, NULL, 0);
	if(listptr == (void*) -1){
		// TRUE when fails to attaching 
		// then remove list descriptor because can not initialize list descriptor
		if(supressStatusMsg)
			fprintf(stderr, "Error in attaching List Descriptor of List %d in Current Process Space, ERRNO: %d\n", list_no, errno );
		unlockMemory(list_shmid);
		shmctl(list_shmid, IPC_RMID, NULL);
		return -1;
	}
	// initializing list descriptor
	listptr->shmid = -1;
	if(supressStatusMsg)
		printf("List No: %d successfully created and initialized\n", list_no);
	unlockMemory(list_shmid);
	return 0;
}

int insertLast(int list_no, void *ptr, size_t size)
{
	// Used to insert node at the end of the list
	// list_no : 	this is the list which user wants to enter node
	// 	   ptr : 	void pointer to the data that user wants to keep in newly created node
	// 	  size :	size of the data that user wants to enter, i.e. size of data pointed by ptr
	
	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;																	

	int newnode_shmid = shmget(IPC_PRIVATE, sizeof(node), IPC_CREAT | IPC_EXCL | 0777);		// get shmid of the list descriptor
	node *nodeptr = shmat(newnode_shmid, NULL, 0);											// current node
	nodeptr->value_shmid = shmget(IPC_PRIVATE, sizeof(node), IPC_CREAT | IPC_EXCL | 0777);	// create value field of that node 
	void *value = shmat(nodeptr->value_shmid, NULL, 0);										// attach value filed
	memcpy(value, ptr, size);																// store data provided by the user in the value field
	nodeptr->next_shmid = -1;																// last node always points to -1
	shmdt(value);
	shmdt(nodeptr);

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	if(listptr->shmid == -1){
		// list is initialized but empty
		listptr->shmid = newnode_shmid;														// list descriptor's shmid points to newly created node 		
		if(supressStatusMsg)
			printf("Value %d inserted at last of list %d\n", *(int*)ptr, list_no);
		unlockMemory(list_shmid);
		shmdt(listptr);
	}
	else{
		// list is not empty
		int node_shmid = listptr->shmid;
		nodeptr = shmat(node_shmid, NULL, 0);
		lockMemory(node_shmid);
		unlockMemory(list_shmid);
		while(nodeptr->next_shmid != -1){													// traverse until last node is reached
			lockMemory(nodeptr->next_shmid);
			unlockMemory(node_shmid);
			node_shmid = nodeptr->next_shmid;
			shmdt(nodeptr);
			nodeptr = shmat(node_shmid, NULL, 0);
		}
		nodeptr->next_shmid = newnode_shmid;												// establish link between last node and newly created node
		shmdt(nodeptr);
		if(supressStatusMsg)
			printf("Value %d inserted at last of list %d\n", *(int*)ptr, list_no);
		unlockMemory(node_shmid);
		shmdt(listptr);
	}
	return 0;																				// return successfully
}

int insertFirst(int list_no, void *ptr, size_t size)
{
	// Used to insert node at the end of the list
	// list_no : 	this is the list which user wants to enter node
	// 	   ptr : 	void pointer to the data that user wants to keep in newly created node
	// 	  size :	size of the data that user wants to enter, i.e. size of data pointed by ptr

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int node_shmid = shmget(IPC_PRIVATE, sizeof(node), IPC_CREAT | IPC_EXCL | 0777 );		// create new node
	node *nodeptr = shmat(node_shmid, NULL, 0);												// attach new node
	nodeptr->value_shmid = shmget(IPC_PRIVATE, sizeof(size), IPC_CREAT | IPC_EXCL | 0777);	// create value filed for new node
	void *value = shmat(nodeptr->value_shmid, NULL, 0);										// attach value field		
	memcpy(value, ptr, size);																// store data provided by the user in value field 
	shmdt(value);
	
	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	nodeptr->next_shmid = listptr->shmid;													// establish link between new node and first node (-1 if there is not any) of list
	shmdt(nodeptr);
	listptr->shmid = node_shmid;															// modify shmid field  list descriptor
	if(supressStatusMsg)
		printf("Value %d inserted at first of list %d\n", *(int*)ptr, list_no);
	unlockMemory(list_shmid);
	shmdt(listptr);
	return 0;																				// return successfully
}


int displayList(int list_no)
{
	// Used to display the contents of the list, only can be used when nodes hold only integer data

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	int node_shmid = listptr->shmid;
	if(node_shmid == -1){
		// TRUE when list is empty
		// return '-1'
		if(supressStatusMsg)
			fprintf(stderr, "List %d is Empty\n", list_no);
		unlockMemory(list_shmid);
		shmdt(listptr);
		return -1;
	}

	lockMemory(node_shmid);
	unlockMemory(list_shmid);
	int count = 0;
	node *nodeptr = shmat(node_shmid, NULL, 0);
	void *value;
	while(node_shmid != -1){																// traverse until last node is reached and display values subsequently 
		value = shmat(nodeptr->value_shmid, NULL, 0);										// attach value
		printf("List %d Index %d SHMID %d Value %d\n", list_no, count++, node_shmid, *(int*)value);// print data
		shmdt(value);
		lockMemory(nodeptr->next_shmid);
		unlockMemory(node_shmid);
		node_shmid = nodeptr->next_shmid;
		shmdt(nodeptr);
		nodeptr = shmat(node_shmid, NULL, 0);
	}
	unlockMemory(node_shmid);
	releaseSem(node_shmid);
	shmdt(listptr);
	return 0;																				// return successfully
}


int deleteNode(int list_no, int index)
{
	// Deletes the node directed by the index
	// list_no : the list no on which user wants the operation to be done
	//   index : the index ('0' indexed) on which user wants the operation to be done

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_no);
	if(listptr->shmid == -1){
		// TRUE when list is empty, simply return -1
		if(supressStatusMsg)
			fprintf(stderr, "List %d is Empty. Can not Delete Index %d\n", list_no, index );
		unlockMemory(list_shmid);
		shmdt(listptr);
		return -1;
	}
	int node_shmid;
	node *nodeptr;
	if(index == 0){
		// TRUE when index is 0, i.e. user wants to delete first node of the list
		// in this after shmid of the list descriptor needs to be modified
		node_shmid = listptr->shmid;
		nodeptr = shmat(node_shmid, NULL, 0);												// attach node
		listptr->shmid = nodeptr->next_shmid;												// update shmid of the list descriptor
		unlockMemory(list_shmid);
		shmctl(nodeptr->value_shmid, IPC_RMID, NULL);										// delete value field first (0-th) node
		shmdt(nodeptr);
		shmctl(node_shmid, IPC_RMID, NULL);													// delete first node
		releaseSem(node_shmid);																// delete semaphore corresponding to the first node
		if(supressStatusMsg)
			printf("List %d Index %d Deleted\n", list_no, index );
		shmdt(listptr);
	}
	else{
		// TRUE when user wants to delete other than first node
		node_shmid = listptr->shmid;
		lockMemory(node_shmid);
		unlockMemory(list_shmid);
		nodeptr = shmat(node_shmid, NULL, 0);
		int count = 0;
		while(nodeptr->next_shmid != -1){													// traverse until targeted node is reached, t.e. (count+1)-th node
			if(count == index-1){
				// TRUE when destination is reached
				int target_shmid = nodeptr->next_shmid;
				node *target_nodeptr = shmat(target_shmid, NULL, 0);						// attach targeted node
				nodeptr->next_shmid = target_nodeptr->next_shmid;							// establish link between previous and next node of the targeted node
				unlockMemory(node_shmid);
				shmctl(target_nodeptr->value_shmid, IPC_RMID, NULL);						// delete value field of the targeted node
				shmdt(target_nodeptr);
				shmctl(target_shmid, IPC_RMID, NULL);										// delete targeted node
				releaseSem(target_shmid);
				shmdt(nodeptr);
				if(supressStatusMsg)
					printf("List %d Index %d Deleted\n", list_no, index);
				return 0;																	// return successfully
			}
			lockMemory(nodeptr->next_shmid);
			unlockMemory(node_shmid);
			node_shmid = nodeptr->next_shmid;
			shmdt(nodeptr);
			nodeptr = shmat(node_shmid, NULL, 0);
			count++;
		}
	}
	// Control comes out of the while only if user wants to delete a node which is greater than the size of the list (index > length of the list) 
	unlockMemory(node_shmid);
	if(supressStatusMsg)
		fprintf(stderr, "Index %d is greater than the length of List %d\n",list_no, index );		
	return -1;																				// returns with -1, unsuccessful operation																
}

int deleteList(int list_no)
{
	// Delete the whole list, i.e. list descriptor and all the nodes and semaphores corresponding to each shared memory
	// list_no : the list no on which user wants the operation to be done
	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	int node_shmid = listptr->shmid;														// get shmid of the first node
	shmdt(listptr);
	shmctl(list_shmid, IPC_RMID, NULL);														// delete list descriptor
	unlockMemory(list_shmid);
	releaseSem(list_shmid);																	// delete semaphore corresponding to list descriptor
	int temp_shmid;
	node *nodeptr;
	while(node_shmid != -1 ){																// traverse until end of the list
		temp_shmid = node_shmid;
		nodeptr = shmat(node_shmid, NULL, 0);
		node_shmid = nodeptr->next_shmid;
		shmctl(nodeptr->value_shmid, IPC_RMID, NULL);										// delete value field of the each node
		shmdt(nodeptr);
		shmctl(temp_shmid, IPC_RMID, NULL);													// delete the node
		releaseSem(temp_shmid);																// delete semaphore corresponding to that node
	}
	if(supressStatusMsg)
		printf("List %d Deleted\n", list_no);
	return 0;																				// returns successfully
}



void* getVal(int list_no, int index)
{
	// Returns void pointer to the value field of the node pointed by index
	// list_no : the list no on which user wants the operation to be done
	//   index : the index ('0' indexed) on which user wants the operation to be done 
	// 	   ptr : 	void pointer to the data that user wants to keep in newly created node
	// 	  size :	size of the data that user wants to enter, i.e. size of data pointed by ptr

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return (void*) -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	if(listptr->shmid == -1){
		// TRUE when list is empty, returns '-1'
		if(supressStatusMsg)
			fprintf(stderr, "List %d is Empty. Can not return value\n", list_no);
		unlockMemory(list_shmid);
		return (void*) 1;
	}
	// Control comes here when list is not empty, then traversal to last node is required
	int node_shmid = listptr->shmid;														// get shmid of first node
	lockMemory(node_shmid);
	shmdt(listptr);
	unlockMemory(list_shmid);
	int count = 0;
	node *nodeptr;
	while(node_shmid != -1){																// traverse until not reached to the targeted 
		nodeptr = shmat(node_shmid, NULL, 0);												// attach node
		if(count == index){
			// TRUE when Reached at Destination 
			void *value = shmat(nodeptr->value_shmid, NULL, 0);								// attach value field of that node
			shmdt(nodeptr);
			unlockMemory(node_shmid);
			return value;																	// return value pointer, remembering not to detach value pointer 
		}
		lockMemory(nodeptr->next_shmid);
		unlockMemory(node_shmid);
		node_shmid = nodeptr->next_shmid;
		shmdt(nodeptr);
		count++;
	}
	// Control comes out of the while only if user wants to delete a node which is greater than the size of the list (index > length of the list) 
	unlockMemory(node_shmid);
	if(supressStatusMsg)
		fprintf(stderr, "Index %d is greater than the length of List %d\n",list_no, index );
	return (void*) -1;																		// returns unsuccessfully
}
int setVal(int list_no, int index, void *ptr, size_t size)
{
	// Updates/modifies the value pointed by the index
	// list_no : the list no on which user wants the operation to be done
	//   index : the index ('0' indexed) on which user wants the operation to be done	
	// 	   ptr : 	void pointer to the data that user wants to keep in newly created node
	// 	  size :	size of the data that user wants to enter, i.e. size of data pointed by ptr

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	if(listptr->shmid == -1){
		// TRUE when list is empty, returns '-1'
		if(supressStatusMsg)
			fprintf(stderr, "List %d is Empty. Can not Set value\n", list_no);
		shmdt(listptr);
		unlockMemory(list_shmid);
		return -1;
	}

	// Control comes here when list is not empty
	int node_shmid = listptr->shmid;														// get first node
	lockMemory(node_shmid);
	shmdt(listptr);
	unlockMemory(list_shmid);
	node *nodeptr;
	int count = 0;
	while(node_shmid != -1){																// traverse until targeted node (index) is not reached
		nodeptr = shmat(node_shmid, NULL, 0);												// attach node
		if(count == index){
			// TRUE when Reached at Destination
			void *value = shmat(nodeptr->value_shmid, NULL, 0);								// attach value field of targeted node
			memset(value, 0, size);															// reset the value field 
			memcpy(value, ptr, size);														// copy the data provided by the user
			shmdt(value);
			shmdt(nodeptr);
			if(supressStatusMsg)
				fprintf(stderr, "Value updated successfully at index %d of list %d\n", count, list_no);
			unlockMemory(node_shmid);
			return 0; 																		// return successfully
		}
		lockMemory(nodeptr->next_shmid);
		unlockMemory(node_shmid);
		node_shmid = nodeptr->next_shmid;
		shmdt(nodeptr);
		count++;
	}
	// Control comes out of the while only if user wants to delete a node which is greater than the size of the list (index > length of the list) 
	unlockMemory(node_shmid);
	if(supressStatusMsg)
		fprintf(stderr, "Index %d is greater than the length of List %d\n",list_no, index );
	return 	-1;
}

int getLength(int list_no)
{
	// Returns the length of the list
	// list_no : the list no on which user wants the operation to be done

	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	int node_shmid = listptr->shmid;
	if(node_shmid == -1){
		// TRUE when list is empty, returns length as '0'
	 	unlockMemory(node_shmid);
		shmdt(listptr);
		return 0;
	}

	// Control comes here if list is not empty
	lockMemory(node_shmid);
	unlockMemory(list_shmid);
	int count = 0;
	node *nodeptr;
	while(node_shmid != -1){																// traverse until not reached to the last node 
		nodeptr = shmat(node_shmid, NULL, 0);
		lockMemory(nodeptr->next_shmid);
		unlockMemory(node_shmid);
		node_shmid = nodeptr->next_shmid;
		shmdt(nodeptr);
		count++;																			// increment count to keep track of the length
	}
	unlockMemory(node_shmid);
	shmdt(listptr);
	return count;																			// return count as length of the list
}

int getIndex(int list_no, void *ptr, size_t size)
{
	// Used to linearly search the value provided by the user
	// list_no : the list no on which user wants the operation to be done
	// 	   ptr : void pointer to the data that user wants to keep in newly created node
	// 	  size : size of the data that user wants to enter, i.e. size of data pointed by ptr
	// Caution : User must provide the correct size of the value that he/she wants to search and also size of all value field must be uniform through out
	
	list *listptr = getListDescriptor(list_no);												// attach list descriptor for list <list_shmid>; if fails then return
    if(listptr == (void*) -1)
	    return -1;

	int list_shmid = getList_shmid(list_no);
	lockMemory(list_shmid);
	if(listptr->shmid == -1){
		// TRUE when list is not empty, returns '-1'
		if(supressStatusMsg)
			fprintf(stderr, "List %d is Empty. Can not get index\n", list_no);
		shmdt(listptr);
		unlockMemory(list_shmid);
		return -1;
	}

	int node_shmid = listptr->shmid;														// get first node
	lockMemory(node_shmid);
	shmdt(listptr);
	unlockMemory(list_shmid);
	node *nodeptr;
	int count = 0;
	void *value;
	char *buffer = malloc(sizeof(size));													// used to store data of the value field in each node
	char *buffer2 = malloc(sizeof(size));													// used to store data of the that user provides through ptr to be searched
	memcpy(buffer2, ptr, size);																// store the data provided by the user through in buffer 2 
	while(node_shmid != -1){																// traverse until targeted value is found
		nodeptr = shmat(node_shmid, NULL, 0);
		value = shmat(nodeptr->value_shmid, NULL, 0);										// attach value field
		memset(buffer, 0, size);															// set buffer a empty
		memcpy(buffer, value, size);														// copy data in the value field of the node 
		if( strcmp(buffer, buffer2) == 0 ){
			// targeted value found
			shmdt(value);
			shmdt(nodeptr);
			unlockMemory(node_shmid);
			return count;																	// return count as the index of the targeted value
		}
		shmdt(value);
		lockMemory(nodeptr->next_shmid);
		unlockMemory(node_shmid);
		node_shmid = nodeptr->next_shmid;
		shmdt(nodeptr);
		count++;																			// increment count to keep track of the index to be returned 
	}
	// Control comes out of the while only if user wants to delete a node which is greater than the size of the list (index > length of the list) 
	unlockMemory(node_shmid);
	if(supressStatusMsg)
		fprintf(stderr, "Value not Found in List %d\n",list_no);
	return 	-1;																				// return unsuccessfully
}