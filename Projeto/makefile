all: client.c server.c proxy.c
	gcc -Wall client.c -o client -lsodium
	gcc -Wall server.c -o server -lsodium
	gcc -Wall proxy.c -o proxy -lsodium
clean:
	rm *.out