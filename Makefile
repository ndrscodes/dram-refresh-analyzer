test: main.c
	gcc -msse2 -Wall -O1 -march=native main.c -o test
