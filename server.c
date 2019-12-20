/*******************************************************************************
 * SERVIDOR no porto 9000, à escuta de novos clientes.  Quando surjem
 * novos clientes os dados por eles enviados são lidos e descarregados no ecran.
 *******************************************************************************/
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sodium.h>
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
#define FILE_DIR "server_files"

unsigned char key[crypto_secretbox_KEYBYTES] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1,
                                                2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3,
                                                4, 5, 6, 7, 8, 9, 1, 2, 3, 4};
unsigned char nonce[crypto_secretbox_NONCEBYTES] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 4, 5};

char *getFileList(char *path) {
    DIR *d;
    struct dirent *dir;
    d = opendir(path);
    char *fileListSt = malloc(1000);
    if (d) {
        char offset = 0;
        while ((dir = readdir(d)) != NULL) {
            char *file = dir->d_name;
            if (strcmp(file, ".") == 0 || strcmp(file, "..") == 0)
                continue;
            strcpy(fileListSt + offset, file);
            strcpy(fileListSt + offset + strlen(file), "\n");
            offset += strlen(file) + 1;
            // printf("|%s", fileListSt);
        }
        closedir(d);
    }
    fileListSt[strlen(fileListSt) - 1] = '\0';
    return fileListSt;
}

void process_client(int fd);
void erro(char *msg);
int num_clientes = 0;
int SERVER_PORT = 0;
int NUM_MAX_CLIENTES;

void server_tcp() {
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
    while (1) {
        // clean finished child processes, avoiding zombies
        // must use WNOHANG or would block whenever a child process was
        // working
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
                process_client(client);
                exit(0);
            }
            close(client);
        }
    }
}

void server_udp() {
    struct sockaddr_in si_minha, si_outra;
    char sendBuffer[BUF_SIZE];
    char recvBuffer[BUF_SIZE];
    int s, recv_len;
    socklen_t slen = sizeof(si_outra);
    char *saveptr;
    char *delim = " ";

    // Cria um socket para recepção de pacotes UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        erro("Erro na criação do socket");
    }

    si_minha.sin_family = AF_INET;
    si_minha.sin_port = htons(SERVER_PORT);
    si_minha.sin_addr.s_addr = htonl(INADDR_ANY);

    // Associa o socket à informação de endereço
    if (bind(s, (struct sockaddr *)&si_minha, sizeof(si_minha)) == -1) {
        erro("Erro no bind");
    }
    while (1) {
        printf("Server UDP waiting to receive\n");
        if ((recv_len = recvfrom(s, recvBuffer, BUF_SIZE, 0,
                                 (struct sockaddr *)&si_outra,
                                 (socklen_t *)&slen)) == -1) {
            erro("Erro no recvfrom");
        }
        recvBuffer[recv_len] = '\0';
        // Ver se 'e de um cliente ou do server
        // Guardar o endereço de votla

        printf("Client wrote: %s\n", recvBuffer);
        char *comando = strtok_r(recvBuffer, delim, &saveptr);
        if (strcmp(comando, "LIST") == 0) {
            printf("O Cliente quer uma listagem:\n");
            // TODO ler os dados do cliente
            char *list = getFileList(FILE_DIR);
            strcpy(sendBuffer, list);
            free(list);
            sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                   slen);
        } else if (strcmp(comando, "DOWNLOAD") == 0) {
            printf("O cliente quer um download\n");
            int encrypt = 0;
            char *enc = strtok_r(NULL, delim, &saveptr);
            char *name = "";
            if (enc != NULL) {
                if (strcmp(enc, "ENC") == 0)
                    encrypt = 1;
                name = strtok_r(NULL, delim, &saveptr);
                if (name == NULL)
                    printf("Download requested null file");
            } else {
                strcpy(sendBuffer, "Wrong DOWNLOAD syntax");
                sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                       slen);
                continue;
            }
            printf("ENC: %d Name: %s\n", encrypt, name);
            strcpy(sendBuffer, "OK, will send file");
            sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                   slen);
            char fullname[100];
            snprintf(fullname, 100, "./%s/%s", FILE_DIR, name);
            int file_fd = open(fullname, O_RDONLY);
            if (file_fd < 0) {
                perror("Open file");
                strcpy(sendBuffer, "Error getting file");
                sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                       slen);
                continue;
            }
            struct stat file_stat;
            fstat(file_fd, &file_stat); // TODO check return values
            // mandar o tamanho do ficheiro
            printf("Sending file size: %ld\n", file_stat.st_size);
            snprintf(sendBuffer, BUF_SIZE, "%ld", file_stat.st_size);
            sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                   slen);
            long int offset = 0;
            int sent_bytes = 0;
            int remaining_bytes = file_stat.st_size;
            while ((sent_bytes = sendfile(s, file_fd, &offset, BUF_SIZE)) > 0 &&
                   (remaining_bytes > 0)) {
                printf("Sent bytes: %d\n", sent_bytes);
                remaining_bytes -= sent_bytes;
            }
            printf("File sent with UDP!!\n");
        } else if (strcmp(recvBuffer, "EXIT") == 0) {
            printf("Client exited.\n");
            fflush(stdout);
            close(s);
            return;
        } else {
            printf("Unkown command.\n");
            strcpy(sendBuffer, "UNKNOWN COMMAND");
            sendto(s, sendBuffer, BUF_SIZE, 0, (struct sockaddr *)&si_outra,
                   slen);
        }

        memset(sendBuffer, 0, BUF_SIZE);
        char *file_list = getFileList(FILE_DIR);
        strcpy(sendBuffer, file_list);
        int sent_len = sendto(s, sendBuffer, BUF_SIZE, 0,
                              (struct sockaddr *)&si_outra, slen);
        if (sent_len < 0) {
            perror("Sent 0 bytes... humm\n");
        }
    }
    // Fecha socket e termina programa
    close(s);
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

    while (1) {
        char *saveptr;
        char *delim = " ";
        printf("Waiting for client...\n");
        nread = read(client_fd, recvBuffer, BUF_SIZE);
        if (nread <= 0) {
            printf("Client disconnected\n");
            return;
        }
        recvBuffer[nread] = '\0';
        printf("Client wrote: %s\n", recvBuffer);
        char *comando = strtok_r(recvBuffer, delim, &saveptr);
        if (strcmp(comando, "LIST") == 0) {
            printf("O Cliente quer uma listagem:\n");
            // TODO ler os dados do cliente
            char *list = getFileList(FILE_DIR);
            strcpy(sendBuffer, list);
            free(list);
            write(client_fd, sendBuffer, BUF_SIZE);
        } else if (strcmp(comando, "DOWNLOAD") == 0) {
            printf("O cliente quer um download\n");
            int encrypt = 0;
            char *enc = strtok_r(NULL, delim, &saveptr);
            char *name = "";
            if (enc != NULL) {
                if (strcmp(enc, "ENC") == 0)
                    encrypt = 1;
                name = strtok_r(NULL, delim, &saveptr);
                if (name == NULL)
                    printf("Download requested null file");
            } else {
                strcpy(sendBuffer, "Wrong DOWNLOAD syntax");
                write(client_fd, sendBuffer, BUF_SIZE);
                continue;
            }
            printf("ENC: %d Name: %s\n", encrypt, name);
            strcpy(sendBuffer, "OK, will send file");
            write(client_fd, sendBuffer, BUF_SIZE);
            char fullname[100];
            snprintf(fullname, 100, "./%s/%s", FILE_DIR, name);
            int file_fd = open(fullname, O_RDONLY);
            if (file_fd < 0) {
                perror("Open file");
                strcpy(sendBuffer, "Error getting file");
                write(client_fd, sendBuffer, BUF_SIZE);
                continue;
            }
            struct stat file_stat;
            fstat(file_fd, &file_stat); // TODO check return values
            // mandar o tamanho do ficheiro
            printf("Sending file size: %ld\n", file_stat.st_size);
            snprintf(sendBuffer, BUF_SIZE, "%ld", file_stat.st_size);
            write(client_fd, sendBuffer, BUF_SIZE);
            long int offset = 0;
            int sent_bytes = 0;
            int remaining_bytes = file_stat.st_size;
            if (!encrypt) {
                while ((sent_bytes = sendfile(client_fd, file_fd, &offset,
                                              BUF_SIZE)) > 0 &&
                       (remaining_bytes > 0)) {
                    printf("Sent bytes: %d\n", sent_bytes);
                    remaining_bytes -= sent_bytes;
                }
            } else {
                int mlen = BUF_SIZE - crypto_secretbox_MACBYTES;
                unsigned char readBuffer[mlen];
                unsigned char enctyptBuffer[BUF_SIZE];
                while (remaining_bytes > 0) {
                    int r = read(file_fd, readBuffer, mlen);
                    crypto_secretbox_easy(enctyptBuffer, readBuffer, mlen,
                                          nonce, key);
                    write(client_fd, enctyptBuffer, BUF_SIZE);
                    remaining_bytes -= r;
                }
            }
            printf("File sent!\n");

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

int main(int argc, char const *argv[]) {
    if (sodium_init() < 0) {
        /* panic! the library couldn't be initialized, it is not safe to use */
    }
    if (argc != 3) {
        perror("Numero errado de parametros. Usar:\n.\\server [PORT] "
               "[NUM_MAX_CLIENTES]");
        return -1;
    }
    SERVER_PORT = atoi(argv[1]);
    if (SERVER_PORT == 0) {
        perror("Server port is invalid");
        return -1;
    }
    NUM_MAX_CLIENTES = atoi(argv[2]);
    if (NUM_MAX_CLIENTES < 1) {
        perror("Numero maximo de clitntes invalido");
        return -1;
    }
    if (fork() == 0) {
        server_tcp();
    } else {
        server_udp();
    }
    return 0;
}
