#include "thread_linked_list.h"

void *runner()
{

	printf("Thread %ld entereed \n", pthread_self());
	displayList(0);
	insertLast(0,1);
	insertLast(0,2);
	deleteAt(0,1);
	insertLast(0,3);
	insertLast(0,4);
	insertLast(0,5);
	insertLast(0,6);
	setValue(0,1,5);
	displayList(0);
	deleteList(0);
	displayList(0);
	printf("Thread %ld exiting 	 \n", pthread_self());


}
int main(int argc, char const *argv[])
{
	printf("IN MAIN\n");
	pthread_t tid[3];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	init();
	for (int i = 0; i < 3; ++i)
	{	
		int status = pthread_create(tid + i, &attr, runner, NULL);
	}
	for (int i = 0; i < 3; ++i)
	{
		pthread_join(tid[i], NULL);	
	}
	return 0;

}