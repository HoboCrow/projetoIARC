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

void porxy_tcp()
{
    int fd, client;
    struct sockaddr_in addr, client_addr;
    int client_addr_size;

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PROXY_PORT);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        erro("na funcao socket");
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("na funcao bind");

    char selfAddr[INET_ADDRSTRLEN];
    printf("Proxy listening on %s:%d\n",
           inet_ntop(AF_INET, &addr.sin_addr, selfAddr, INET_ADDRSTRLEN),
           PROXY_PORT);
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
            if (fork() == 0)
            {
                close(fd);
                process_client(client);
                exit(0);
            }
            close(client);
        }
    }
}

void proxy_udp()
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
    si_minha.sin_port = htons(PROXY_PORT);
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
    if (argc != 2)
    {
        perror("Numero errado de parametros. Usar:\n.\\proxy [PORT]");
        return -1;
    }
    PROXY_PORT = atoi(argv[1]);
    if (PROXY_PORT == 0)
    {
        perror("Proxy port is invalid");
        return -1;
    }
    if (fork() != 0)
    {
        porxy_tcp();
    }
    else
    {
        //proxy_udp();
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
    // Buscar o endereço do proxy
    char endServer[100];
    int fd;
    struct sockaddr_in addr;
    struct hostent *hostPtr;
    strcpy(endServer, "localhost");
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");

    bzero((void *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short)SERVER_PORT);

    // Connectar ao proxy
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        erro("socket");
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        erro("Connect");

    while (1)
    {
        printf("Waiting for client op\n");
        nread = read(client_fd, recvBuffer, BUF_SIZE);
        if (nread == 0)
        {
            printf("Client ended connection.\n");
            return;
        }
        recvBuffer[nread] = '\0';
        printf("Client wrote: %s\n", recvBuffer);
        write(fd, recvBuffer, strlen(recvBuffer));
        nread = read(fd, sendBuffer, BUF_SIZE);
        if (nread == 0)
        {
            printf("Server ended connection.");
            return;
        }
        sendBuffer[nread] = '\0';
        printf("Server answer: %s\n", sendBuffer);
        write(client_fd, sendBuffer, strlen(sendBuffer));
    }
    printf("Proxy client process ended\n");
}

void erro(char *msg)
{
    printf("Erro: %s\n", msg);
    exit(-1);
}
