#1/bin/bash

gcc -c -fPIC -Wall -o thread_linked_list.so thread_linked_list.c

gcc main.c ./thread_linked_list.so -pthread