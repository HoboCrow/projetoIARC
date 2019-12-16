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
#define UDP 0
#define TCP 1;

void erro(char *msg);

int main(int argc, char const *argv[])
{

    if (argc != 5)
        erro("Numero de parametros de entrada errado.\nUsar: client.out [IP Proxy] [IP server] [PORT] [UDP/TCP]");

    const char *ipProxy = argv[1];
    const char *ipServer = argv[2];
    const char *port = argv[3];
    const char *protocol = argv[4];
    int PORT;
    int PROTOCOL;

    if ((PORT = atoi(argv[3])) == 0)
        erro("PORT tem valor invalido\n");
    if (strcmp(protocol, "UDP") == 0)
    {
        PROTOCOL = UDP;
    }
    else if (strcmp(protocol, "TCP") == 0)
    {
        PROTOCOL = TCP;
    }
    else
        erro("Protocolo desconhecido.\n");

    // Buscar o endereço do proxy
    char endServer[100];
    int fd;
    struct sockaddr_in addr;
    struct hostent *hostPtr;
    strcpy(endServer, ipProxy);
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)PORT);

    // Connectar ao proxy
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect failed");
        erro("connect");
    }

    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int n;
    printf("Commands: \nLIST - listar documetnos para download\nDOWNLOAD TCP/UDP ENC/NOR NOME\n");
    do
    {
        // ler comando do user e enviar
        printf("Insira um comando: ");
        fgets(sendBuffer, BUF_SIZE, stdin);
        sendBuffer[strlen(sendBuffer) - 1] = '\0';

        char *delim = " ";
        char copySendBuffer[BUF_SIZE];
        strcpy(copySendBuffer, sendBuffer);
        char *commando = strtok(copySendBuffer, delim);
        if (strcmp(commando, "LIST") == 0)
        {
            write(fd, sendBuffer, strlen(sendBuffer));
            n = read(fd, recvBuffer, BUF_SIZE);
            if (n == 0)
                erro("Conecao morreu\n");
            recvBuffer[n] = '\0';
            printf("%s\n", recvBuffer);
        }
        else if (strcmp(commando, "DOWNLOAD") == 0)
        {
            write(fd, sendBuffer, strlen(sendBuffer));
            n = read(fd, recvBuffer, BUF_SIZE);
            if (n == 0)
                erro("Conecao morreu\n");
            recvBuffer[n] = '\0';
            printf("%s\n", recvBuffer);
        }

    } while (strcmp(sendBuffer, "QUIT") != 0);

    close(fd);
    exit(0);
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
