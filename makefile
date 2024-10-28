all: prog


prog: Shell.o
	gcc Shell.o -o prog

Shell.o: Shell.c
	gcc -g -Wall -c Shell.c

clean:
	/bin/rm -f prog Shell.o
