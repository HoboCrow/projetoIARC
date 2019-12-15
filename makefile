all: tcp_client.c tcp_server.c udp_server.c
	gcc -Wall tcp_client.c -o client.o
	gcc -Wall tcp_server.c -o server.o
	gcc -Wall udp_server.c -o udp_server.o
	gcc -Wall udp_client.c -o udp_client.o
clean:
	rm *.o