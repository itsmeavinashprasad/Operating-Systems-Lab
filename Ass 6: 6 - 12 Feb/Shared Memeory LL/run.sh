#!/bin/bash
gcc -c -fPIC -Wall -o libsharedll.so libsharedll.c
gcc main.c ./libsharedll.so
