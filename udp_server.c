#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 512  // Tamanho do buffer
#define PORT 9876   // Porto para recepção das mensagens

void erro(char *s) {
    perror(s);
    exit(1);
}

int main(void) {
    struct sockaddr_in si_minha, si_outra;

    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char buf[BUFLEN];

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }
    while (1) {
        // Espera recepção de mensagem (a chamada é bloqueante)
        if ((recv_len =
                 recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_outra,
                          (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        // Para ignorar o restante conteúdo (anterior do buffer)
        buf[recv_len] = '\0';

        printf(
            "Recebi uma mensagem do sistema com o endereço %s e o porto %d\n",
            inet_ntoa(si_outra.sin_addr), ntohs(si_outra.sin_port));
        printf("Conteúdo da mensagem: %s\n", buf);

        // Enviar o tamanho do buffer lido ao cliente
        int len = strlen(buf);
        int sent_len =
            sendto(s, &len, sizeof(len), 0, (struct sockaddr *)&si_outra, slen);
        if (sent_len < 0) {
            perror("Sent 0 bytes... humm\n");
        }
    }
    // Fecha socket e termina programa
    close(s);
    return 0;
}
