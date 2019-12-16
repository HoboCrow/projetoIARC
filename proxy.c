/*******************************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surjem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 *******************************************************************************/
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 1024

void process_client(int fd);
void erro(char *msg);
int num_clientes = 20;
int PROXY_PORT = 0;
int SERVER_PORT = 3010;
int NUM_MAX_CLIENTES;
struct sockaddr_in proxy_addr, server_addr;

void porxy_tcp() {
    int fd, client;
    struct sockaddr_in client_addr;
    int client_addr_size;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0)
        erro("na funcao bind");

    char selfAddr[INET_ADDRSTRLEN];
    printf("Proxy listening on %s:%d\n",
           inet_ntop(AF_INET, &proxy_addr.sin_addr, selfAddr, INET_ADDRSTRLEN),
           PROXY_PORT);
    if (listen(fd, NUM_MAX_CLIENTES) < 0) erro("na funcao listen");
    client_addr_size = sizeof(client_addr);
    while (1) {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was working
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        // wait for new connection
        client = accept(fd, (struct sockaddr *)&client_addr,
                        (socklen_t *)&client_addr_size);
        if (client > 0) {
            if (fork() == 0) {
                close(fd);
                process_client(client);
                exit(0);
            }
            close(client);
        }
    }
}

void proxy_udp() {
    struct sockaddr_in si_minha, si_outra;
    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int s, recv_len;
    socklen_t slen = sizeof(si_outra);

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }
    bzero((void *)&si_minha, sizeof(si_minha));
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PROXY_PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    /*
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1) {
        perror("Bind error UDP");
    }*/

    while (1) {
        printf("Waiting for UDP receive\n");
        if ((recv_len = recvfrom(s, recvBuffer, BUF_SIZE, 0,
                                 (struct sockaddr *)&si_outra,
                                 (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        recvBuffer[recv_len] = '\0';
        printf("Recieved: %s\n", recvBuffer);
        // Ver se 'e de um cliente ou do server
        // Guardar o endereço de votla

        memset(sendBuffer, 0, BUF_SIZE);
        strcpy(sendBuffer, "TEST");
        printf("PORT: %d\n", proxy_addr.sin_port);
        int sent_len = sendto(s, sendBuffer, BUF_SIZE, 0,
                              (struct sockaddr *)&si_outra, slen);
        if (sent_len < 0) {
            perror("Sent 0 bytes... humm\n");
        } else {
            printf("Send successfull\n");
        }
    }

    // Fecha socket e termina programa
    close(s);
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        perror("Numero errado de parametros. Usar:\n.\\proxy [PORT]");
        return -1;
    }
    PROXY_PORT = atoi(argv[1]);
    if (PROXY_PORT == 0) {
        perror("Proxy port is invalid");
        return -1;
    }

    bzero((void *)&proxy_addr, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    proxy_addr.sin_port = htons(PROXY_PORT);

    struct hostent *hostPtr;
    char endServer[100];
    strcpy(endServer, "localhost");
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");
    bzero((void *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    server_addr.sin_port = htons((short)SERVER_PORT);

    if (fork() != 0) {
        porxy_tcp();
    } else {
        proxy_udp();
    }
    return 0;
}

int calcNum(char *string, int *valid) {
    int len = strlen(string);
    *valid = 1;
    int num = 0;
    int mult = 1;
    int i = 0;
    if (string[0] == '-') {
        i++;
        mult = -1;
    }
    for (; i < len; i++) {
        if (string[i] < '0' || string[i] > '9') {
            *valid = 0;
            return 0;
        }
        num *= 10;
        num += (int)string[i] - '0';
    }
    return mult * num;
}

void process_client(int client_fd) {
    int nread = 0;
    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int fd;

    // Connectar ao proxy
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) erro("socket");
    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        erro("Connect");

    if (fork() == 0) {
        // Um processo para reencaminhar server -> cliente
        while (1) {
            printf("Waiting for server\n");
            memset(sendBuffer, 0, BUF_SIZE);
            nread = read(fd, sendBuffer, BUF_SIZE);
            if (nread == 0) {
                printf("Server ended connection.");
                return;
            }
            sendBuffer[nread] = '\0';
            printf("Server answer: %s\n", sendBuffer);
            printf("Sending %ld bytes to client\n", strlen(sendBuffer));
            write(client_fd, sendBuffer, BUF_SIZE);
        }
    } else {
        // cliente -> server
        while (1) {
            printf("Waiting for client op\n");
            memset(recvBuffer, 0, BUF_SIZE);
            nread = read(client_fd, recvBuffer, BUF_SIZE);
            if (nread == 0) {
                printf("Client ended connection.\n");
                return;
            }
            recvBuffer[nread] = '\0';
            printf("Client wrote: %s\n", recvBuffer);
            write(fd, recvBuffer, strlen(recvBuffer));
        }
    }
    printf("Proxy client process ended\n");
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}
