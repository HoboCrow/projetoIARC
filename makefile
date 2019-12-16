all: tcp_client.c server.c udp_server.c
	gcc -Wall tcp_client.c -o client.out
	gcc -Wall server.c -o server.out
	gcc -Wall udp_server.c -o udp_server.out
	gcc -Wall udp_client.c -o udp_client.out
clean:
	rm *.out