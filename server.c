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
int num_clientes = 0;
int SERVER_PORT = 0;
int NUM_MAX_CLIENTES;

void server_tcp()
{
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
    if (listen(fd, NUM_MAX_CLIENTES) < 0)
        erro("na funcao listen");
    client_addr_size = sizeof(client_addr);
    while (1)
    {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was working
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
        // wait for new connection
        client = accept(fd, (struct sockaddr *)&client_addr,
                        (socklen_t *)&client_addr_size);
        if (client > 0)
        {
            num_clientes++;
            char clientAddrSt[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, clientAddrSt,
                      INET_ADDRSTRLEN);

            char welcomeBuff[BUFSIZ];
            sprintf(welcomeBuff, "IP: %s:%d clients: %d", clientAddrSt,
                    ntohs(client_addr.sin_port), num_clientes);
            if (fork() == 0)
            {
                close(fd);
                write(client, welcomeBuff,
                      strlen(welcomeBuff)); // write ip to client
                process_client(client);
                exit(0);
            }
            close(client);
        }
    }
}

void server_udp()
{
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char recvBuffer[BUF_SIZE];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        erro("Erro na criação do socket");
    }

    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(SERVER_PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1)
    {
        erro("Erro no bind");
    }
    while (1)
    {

        // Espera recepção de mensagem (a chamada é bloqueante)
        if ((recv_len =
                 recvfrom(s, recvBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                          (socklen_t *)&slen)) == -1)
        {
            erro("Erro no recvfrom");
        }
        // Para ignorar o restante conteúdo (anterior do buffer)
        recvBuffer[recv_len] = '\0';

        printf(
            "Recebi uma mensagem do sistema com o endereço %s e o porto %d\n",
            inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
        printf("Conteúdo da mensagem: %s\n", recvBuffer);

        // Enviar o tamanho do buffer lido ao cliente
        int len = strlen(recvBuffer);
        int sent_len =
            sendto(s, &len, sizeof(len), 0, (struct sockaddr *)&si_outra, slen);
        if (sent_len < 0)
        {
            perror("Sent 0 bytes... humm\n");
        }
    }
    // Fecha socket e termina programa
    close(s);
}

int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        perror("Numero errado de parametros. Usar:\n.\\server [PORT] [NUM_MAX_CLIENTES]");
        return -1;
    }
    SERVER_PORT = atoi(argv[1]);
    if (SERVER_PORT == 0)
    {
        perror("Server port is invalid");
        return -1;
    }
    NUM_MAX_CLIENTES = atoi(argv[2]);
    if (NUM_MAX_CLIENTES < 1)
    {
        perror("Numero maximo de clitntes invalido");
        return -1;
    }
    if (fork() == 0)
    {
        server_tcp();
    }
    else
    {
        server_udp();
    }
    return 0;
}

int calcNum(char *string, int *valid)
{
    int len = strlen(string);
    *valid = 1;
    int num = 0;
    int mult = 1;
    int i = 0;
    if (string[0] == '-')
    {
        i++;
        mult = -1;
    }
    for (; i < len; i++)
    {
        if (string[i] < '0' || string[i] > '9')
        {
            *valid = 0;
            return 0;
        }
        num *= 10;
        num += (int)string[i] - '0';
    }
    return mult * num;
}

void process_client(int client_fd)
{
    int nread = 0;
    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int dados[10] = {0};

    while (1)
    {
        printf("Waiting for client...\n");
        nread = read(client_fd, recvBuffer, BUF_SIZE);
        if (nread <= 0)
        {
            printf("Client disconnected\n");
            return;
        }
        recvBuffer[nread] = '\0';
        printf("Client wrote: %s\n", recvBuffer);
        char *saveptr;
        char *delim = " ";
        char *comando = strtok_r(recvBuffer, delim, &saveptr);
        if (strcmp(comando, "LIST") == 0)
        {
            printf("O Cliente quer uma listagem:\n");
            // TODO ler os dados do cliente
            strcpy(sendBuffer, "OK");
            write(client_fd, sendBuffer, strlen(sendBuffer));
        }
        else if (strcmp(comando, "DOWNLOAD") == 0)
        {
            printf("O cliente quer um download\n");
            int enctypt = 0;
            if (strcmp(strtok_r(NULL, delim, &saveptr), "ENC") == 0)
                enctypt = 1;
            char *name = strtok_r(NULL, delim, &saveptr);
            printf("ENC: %d Name: %s\n", enctypt, name);
            strcpy(sendBuffer, "OK");
            write(client_fd, sendBuffer, strlen(sendBuffer));
        }
        else if (strcmp(recvBuffer, "EXIT") == 0)
        {
            printf("Client exited.\n");
            fflush(stdout);
            close(client_fd);
            return;
        }
        else
        {
            printf("Unkown command.\n");
            char *ans = "UNKOWN COMMAND";
            write(client_fd, ans, strlen(ans));
        }
    }
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
