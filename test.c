#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *logfileaddr;
pthread_mutex_t logmutex;
int pos = 0;

void writetolog(char *string) {
    pthread_mutex_lock(&logmutex);
    int len = strlen(string);
    // falta gerar uma string com o tempo antes de escrever esta
    // por exemplo [10:12:23] : Erro ao (...)
    memcpy((logfileaddr + pos), string, len);
    memcpy(logfileaddr + pos + len, "\0", 1);
    pos += len;
    pthread_mutex_unlock(&logmutex);
}

int main(int argc, char const *argv[]) {
    int size = 1024; // Temporario, para um ficheiro de 1024 bytes

    int fd = open("data.txt", O_RDWR,
                  0600); // 6 = read+write for me!

    // char writenPos[sizeof(int)];
    // read(fd, writenPos, sizeof(int));
    // pos = atoi(writenPos);
    lseek(fd, size, SEEK_SET);
    write(fd, "A", 1);

    logfileaddr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (pthread_mutex_init(&logmutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    writetolog("test this out\n");
    writetolog("Welp, does this work?\n");

    pthread_mutex_destroy(&logmutex);
    // lseek(fd, 0, SEEK_SET);
    // itoa()
    // write(fd, , sizeof(itoa(pos)));
    munmap(logfileaddr, size); // POR NO FIM, QUANDO O PROGRAMA TERMINAR
    return 0;
}
