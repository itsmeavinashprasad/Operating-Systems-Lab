#include<stdio.h>
#include<unistd.h>
int main()
{
	pid_t id1, id2;
	id1 = fork();
	id2 = fork();
	if(id1 != 0 && id2 != 0)
		printf("%s\n","I am Parent" );
	else
		printf("%s\n","I am Child" );

	return 0;
}