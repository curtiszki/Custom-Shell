all: cshell.c
	gcc -g -Wall -o cshell cshell.c
	
clean:
	rm -f *.o cshell log.txt
