test: main.c
	gcc -msse2 -Wall -O0 -march=native main.c -o test
