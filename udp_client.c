#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512 // Tamanho do buffer
#define PORT 9877  // Porto do qual vamos enviar
void erro(char *s) {
    perror(s);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: client IP PORT message");
        return -1;
    }

    struct sockaddr_in si_outra;
    // struct sockaddr_in si_minha;
    char endServer[100];
    strcpy(endServer, argv[1]);
    struct hostent *hostPtr;
    if ((hostPtr = gethostbyname(endServer)) == 0)
        erro("Nao consegui obter endereço");
    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    // char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }
    /*
    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);
*/
    si_outra.sin_family = AF_INET;
    si_outra.sin_port = htons(atoi(argv[2]));
    si_outra.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;

    // Enviar a palavra para o server
    int sent_len = sendto(s, (const char *)argv[3], strlen(argv[3]), 0,
                          (const struct sockaddr *)&si_outra, slen);
    if (sent_len < 0) {
        perror("Sent 0 bytes");
    }

    // Espera recepção de mensagem (a chamada é bloqueante)
    int len = 0;
    if ((recv_len =
             recvfrom(s, &len, sizeof(len), 0, (struct sockaddr *)&si_outra,
                      (socklen_t *)&slen)) == -1) {
        erro("Erro no recvfrom");
    }
    // Para ignorar o restante conteúdo (anterior do buffer)
    // buf[recv_len] = '\0';

    printf("Recebi uma mensagem do sistema com o endereço %s e o porto %d\n",
           inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
    printf("No. caracteres: %d\n", len);

    // Fecha socket e termina programa
    close(s);
    return 0;
}
