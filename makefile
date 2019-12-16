all: client.c server.c proxy.c
	gcc -Wall client.c -o client
	gcc -Wall server.c -o server
	gcc -Wall proxy.c -o proxy
clean:
	rm *.out