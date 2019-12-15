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

#define SERVER_PORT 9000
#define BUF_SIZE 1024

void process_client(int fd);
void erro(char *msg);
int num_clientes = 0;

int main() {
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVER_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");

    char selfAddr[INET_ADDRSTRLEN];
    printf("Server listening on %s:%d\n",
           inet_ntop(AF_INET, &addr.sin_addr, selfAddr, INET_ADDRSTRLEN),
           SERVER_PORT);
    if (listen(fd, 5) < 0)
        erro("na funcao listen");
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
            num_clientes++;
            char clientAddrSt[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, clientAddrSt,
                      INET_ADDRSTRLEN);

            char welcomeBuff[BUFSIZ];
            sprintf(welcomeBuff, "IP: %s:%d clients: %d", clientAddrSt,
                    ntohs(client_addr.sin_port), num_clientes);
            if (fork() == 0) {
                close(fd);
                write(client, welcomeBuff,
                      strlen(welcomeBuff)); // write ip to client
                process_client(client);
                exit(0);
            }
            close(client);
        }
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
    int dados[10] = {0};

    while (1) {
        printf("Waiting for client...\n");
        nread = read(client_fd, recvBuffer, BUF_SIZE);
        if (nread <= 0) {
            printf("Client disconnected\n");
            return;
        }
        recvBuffer[nread] = '\0';
        printf("Client wrote: %s\n", recvBuffer);
        if (strcmp(recvBuffer, "DADOS") == 0) {
            printf("O Cliente quere inserir os dados:\n");
            // TODO ler os dados do cliente
            int valid = 0;
            for (int i = 0; i < 10; i++) {
                nread = read(client_fd, recvBuffer, BUF_SIZE);
                if (nread <= 0) {
                    printf("Client disconnected\n");
                    return;
                }
                recvBuffer[nread] = '\0';
                char *pBuff = &recvBuffer[0];
                int num = calcNum(pBuff, &valid);
                if (valid) {
                    dados[i] = num;
                    printf("Read: %d\n", num);
                    strcpy(sendBuffer, "OK");
                    write(client_fd, sendBuffer, strlen(sendBuffer));
                } else {
                    printf("Invalid num: %s\n", recvBuffer);
                    strcpy(sendBuffer, "ERROR");
                    write(client_fd, sendBuffer, strlen(sendBuffer));
                    break;
                }
            }
        } else if (strcmp(recvBuffer, "SOMA") == 0) {
            printf("O Cliente quer fazer a soma\n");
            int sum = 0;
            for (int i = 0; i < 10; i++) {
                sum += dados[i];
            }
            write(client_fd, &sum, sizeof(sum));
        } else if (strcmp(recvBuffer, "MEDIA") == 0) {
            printf("O Cliente quer fazer a media\n");
            int sum = 0;
            for (int i = 0; i < 10; i++) {
                sum += dados[i];
            }
            double med = sum / 10.0;
            write(client_fd, &med, sizeof(med));
        } else if (strcmp(recvBuffer, "EXIT") == 0) {
            printf("Client exited.\n");
            fflush(stdout);
            close(client_fd);
            return;
        } else {
            printf("Unkown command.\n");
            char *ans = "UNKOWN COMMAND";
            write(client_fd, ans, strlen(ans));
        }
    }
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}
