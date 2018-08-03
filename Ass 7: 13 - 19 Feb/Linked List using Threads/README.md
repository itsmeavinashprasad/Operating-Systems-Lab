Usage:  1. Run script "run.sh" by ./run.sh
		2. Then use complied output file "a.out" as ./a.out
		3. Modify runner() in "main.c" to use other functions
		4. init() must be used before implementing functions provided by the library in "thread_linked_list.h"
		5. init() must be called only once 
		6. All the function provided in the library are synchronized, so they will be giving error free output ( except init() function )
		7. Print Messages can be suppressed in the function. But then return value can be used for checking if the functions can are executed successfully or not.
Working:
	There are MAX_LIST_COUNT no of mutex present
	For each list there is one mutex
	At a time only one operation can be implemented on single list
	MAX_LISTS_COUNT can be changed as user wants to
 
