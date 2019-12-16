/**********************************************************************
 * CLIENTE liga ao servidor (definido em argv[1]) no porto especificado
 * (em argv[2]), escrevendo a palavra predefinida (em argv[3]).
 * USO: >cliente <enderecoServidor>  <porto>  <Palavra>
 **********************************************************************/
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define UDP 0
#define TCP 1;

void erro(char *msg);

int main(int argc, char const *argv[]) {
    if (argc != 5)
        erro(
            "Numero de parametros de entrada errado.\nUsar: client.out [IP "
            "Proxy] [IP server] [PORT] [UDP/TCP]");

    const char *ipProxy = argv[1];
    // const char *ipServer = argv[2];
    // const char *port = argv[3];
    // const char *protocol = argv[4];
    int PORT;
    // int PROTOCOL;

    if ((PORT = atoi(argv[3])) == 0) erro("PORT tem valor invalido\n");
    /*
    if (strcmp(protocol, "UDP") == 0) {
        PROTOCOL = UDP;
    } else if (strcmp(protocol, "TCP") == 0) {
        PROTOCOL = TCP;
    } else
        erro("Protocolo desconhecido.\n");
    */
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

    // Connectar ao proxy TCP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect failed");
        erro("connect");
    }
    // Connectar ao prixy UDP
    int s;
    socklen_t slen = sizeof(addr);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int n;
    printf(
        "Commands: \nLIST - listar documetnos para download\nDOWNLOAD TCP/UDP "
        "ENC/NOR NOME\n");
    do {
        // ler comando do user e enviar
        printf("Insira um comando: ");
        fgets(sendBuffer, BUF_SIZE, stdin);
        sendBuffer[strlen(sendBuffer) - 1] = '\0';

        char *delim = " ";
        char copySendBuffer[BUF_SIZE];
        strcpy(copySendBuffer, sendBuffer);
        char *commando = strtok(copySendBuffer, delim);
        if (strcmp(commando, "LIST") == 0) {
            write(fd, sendBuffer, BUF_SIZE);
            n = read(fd, recvBuffer, BUF_SIZE);
            if (n == 0) erro("Conecao morreu\n");
            recvBuffer[n] = '\0';
            printf("%s\n", recvBuffer);
        } else if (strcmp(commando, "DOWNLOAD") == 0) {
            char *down_protocol = strtok(NULL, delim);
            char *enc_mode = strtok(NULL, delim);
            char *name = strtok(NULL, delim);
            if (strcmp(down_protocol, "TCP") == 0) {
                snprintf(sendBuffer, BUF_SIZE, "%s %s %s", commando, enc_mode,
                         name);
                write(fd, sendBuffer, BUF_SIZE);
                n = read(fd, recvBuffer, BUF_SIZE);
                if (n == 0) erro("Conexao morreu\n");
                recvBuffer[n] = '\0';
                printf("%s\n", recvBuffer);
                n = read(fd, recvBuffer, BUF_SIZE);
                recvBuffer[n] = '\0';
                long int file_size = atol(recvBuffer);
                printf("Received file size : %ld\n", file_size);
                char full_path[100];
                snprintf(full_path, 100, "./downloads/%s", name);
                FILE *new_file = fopen(full_path, "w");
                if (new_file == NULL) {
                    perror("File ;");
                }
                int remain_data = file_size;
                int rcv_len = 0;
                while ((remain_data > 0) &&
                       ((rcv_len = recv(fd, recvBuffer, BUF_SIZE, 0)) > 0)) {
                    printf("buffer: %s\n", recvBuffer);
                    fwrite(recvBuffer, sizeof(char), rcv_len, new_file);
                    remain_data -= rcv_len;
                    memset(recvBuffer, 0, BUF_SIZE);
                    printf("remain_data : %d\n", remain_data);
                }
                fclose(new_file);
                truncate(full_path, file_size);
                printf("File downloaded!\n");
            } else {
                // Mandar por UDP
                // Enviar a palavra para o server
                int sent_len = sendto(s, sendBuffer, BUF_SIZE, 0,
                                      (const struct sockaddr *)&addr, slen);
                if (sent_len < 0) {
                    perror("Sent 0 bytes");
                }

                // Espera recepção de mensagem (a chamada é bloqueante)
                int recv_len;
                if ((recv_len = recvfrom(s, recvBuffer, BUF_SIZE, 0,
                                         (struct sockaddr *)&addr,
                                         (socklen_t *)&slen)) == -1) {
                    erro("Erro no recvfrom");
                }
                printf("Recebi, por UDP: %s\n", recvBuffer);
            }
        }
    } while (strcmp(sendBuffer, "QUIT") != 0);

    close(fd);
    exit(0);
}

void erro(char *msg) {
    printf("Erro: %s\n", msg);
    exit(-1);
}
