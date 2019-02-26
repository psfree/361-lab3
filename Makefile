all: client.c 
	gcc -g -o client client.c
clean: 
	$(RM) myprog
