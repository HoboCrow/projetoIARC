/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 **********************************************************************/
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SIZE 1024

void erro(char *msg);

int main(int argc, char *argv[]) {
    char endServer[100];
    int fd;
    struct sockaddr_in addr;
    struct hostent *hostPtr;

    if (argc != 4) {
        printf("cliente <host> <port> <string>\n");
        exit(-1);
    }

    strcpy(endServer, argv[1]);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)atoi(argv[2]));

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");

    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    // ler proprio ip do server
    int n = read(fd, recvBuffer, BUF_SIZE);
    if (n <= 0) {
        printf("Server did not respond\n");
        return 0;
    }
    recvBuffer[n] = '\0';
    printf("IP Address from server: %s\n", recvBuffer);
    printf("Commands: DADOS, SOMA, MEDIA, EXIT\n");
    do {
        // ler comando do user e enviar
        printf("Insira um comando: ");
        fgets(sendBuffer, BUF_SIZE, stdin);
        sendBuffer[strlen(sendBuffer) - 1] = '\0';
        write(fd, sendBuffer, strlen(sendBuffer));

        if (strcmp(sendBuffer, "DADOS") == 0) {
            int error = 0;
            for (int i = 0; i < 10; i++) {
                fgets(sendBuffer, BUF_SIZE, stdin);
                sendBuffer[strlen(sendBuffer) - 1] = '\0';
                write(fd, sendBuffer, strlen(sendBuffer));
                n = read(fd, recvBuffer, BUF_SIZE);
                if (n <= 0) {
                    printf("Server did not respond\n");
                    return 0;
                }
                recvBuffer[n] = '\0';
                if (strcmp(recvBuffer, "ERROR") == 0) {
                    printf("NO no no, not a number...\n");
                    error = 1;
                    break;
                }
            }
            if (error)
                continue;
        } else if (strcmp(sendBuffer, "SOMA") == 0) {
            int soma = 0;
            n = read(fd, &soma, sizeof(soma));
            if (n <= 0) {
                printf("Server did not respond\n");
                return 0;
            }
            printf("Soma: %d\n", soma);
        } else if (strcmp(sendBuffer, "MEDIA") == 0) {
            double med = 0;
            n = read(fd, &med, sizeof(med));
            if (n <= 0) {
                printf("Server did not respond\n");
                return 0;
            }
            printf("Media: %0.1f\n", med);
        } else {
            n = read(fd, recvBuffer, BUF_SIZE);
            if (n <= 0) {
                printf("Server did not respond\n");
                return 0;
            }
            recvBuffer[n] = '\0';
            printf("Error\nServer message: %s\n", recvBuffer);
        }

    } while (strcmp(sendBuffer, "EXIT") != 0);

    close(fd);
    exit(0);
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}
