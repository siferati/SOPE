#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/times.h>
#include "viatura.h"

#define NORTE 0
#define SUL 1
#define ESTE 2
#define OESTE 3

unsigned int tempoAbertura, numeroLugares, lugaresDisponiveis;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
double elapsed;
clock_t tcks;
FILE *logFile;

void *thr_arrumador(void * arg) {
    if ((pthread_detach(pthread_self())) != 0) {
        fprintf(stderr, "Erro ao tornar a thread %d detached (%d %s)\n", (int) pthread_self(), errno, strerror(errno));
        deleteViatura((viatura_t *) arg);
        return NULL;
    }

    viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t));
    viatura = (viatura_t *) arg;
    //printf("THREAD ARRUMADOR ID = %d\n", viatura->id);

    int fd;
    if ((fd = open(viatura->nomeFifo, O_WRONLY)) == -1) {
        fprintf(stderr, "Erro ao abrir o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
        deleteViatura(viatura);
        return NULL;
    }

    if (elapsed/tcks < tempoAbertura) {
        pthread_mutex_lock(&mut);
        if (lugaresDisponiveis > 0) {
            lugaresDisponiveis--;
            pthread_mutex_unlock(&mut);

            char message[256];
            sprintf(message, "entrada");
            if ((write(fd, message, strlen(message) + 1)) == -1) {
                fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
                unlink(viatura->nomeFifo);
                deleteViatura(viatura);
                return NULL;
            }

            fprintf(logFile, "%7d  ; %4d ; %5d   ; estacionamento\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
            printf("VIATURA ID = %03d ADMITIDA (NUM_LUGARES = %d)\n", viatura->id, lugaresDisponiveis);

            clock_t startL = times(NULL), currentL = times(NULL), elapsedL;
            while ((elapsedL = currentL - startL) < viatura->duracao) {
                currentL = times(NULL);
            }

            sprintf(message, "saida");
            if ((write(fd, message, strlen(message) + 1)) == -1) {
                fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
                unlink(viatura->nomeFifo);
                deleteViatura(viatura);
                return NULL;
            }

            pthread_mutex_lock(&mut);
            lugaresDisponiveis++;
            pthread_mutex_unlock(&mut);

            printf("VIATURA ID = %03d DE SAIDA (DURACAO = %d    NUM_LUGARES = %d)\n", viatura->id, (int) elapsed, lugaresDisponiveis);
            fprintf(logFile, "%7d  ; %4d ; %5d   ; saida\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
        } else {
            pthread_mutex_unlock(&mut);
            char message[256];
            sprintf(message, "cheio!");
            if ((write(fd, message, strlen(message) + 1)) == -1) {
                fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
                unlink(viatura->nomeFifo);
                deleteViatura(viatura);
                return NULL;
            }

            printf("VIATURA ID = %03d RECUSADA\n", viatura->id);
            fprintf(logFile, "%7d  ; %4d ; %5d   ; cheio\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
        }
    } else {
        char *message = "encerrado";
        if ((write(fd, message, strlen(message))) == -1) {
            fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
            unlink(viatura->nomeFifo);
            deleteViatura(viatura);
            return NULL;
        }

        fprintf(logFile, "%7d  ; %4d ; %5d   ; encerrado\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
    }

    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
        unlink(viatura->nomeFifo);
        deleteViatura(viatura);
        return NULL;
    }

    //printf("FECHANDO ARRUMADOR %d\n", viatura->id);
    deleteViatura(viatura);
    return NULL;
}

void *thr_controlador(void *arg) {
    char acesso = *(char *) arg;
    //printf("THREAD CONTROLADOR %c\n", acesso);

    char nomeFifoAcesso[16];
    sprintf(nomeFifoAcesso, "/tmp/fifo%c", acesso);
    if ((mkfifo(nomeFifoAcesso, 0600)) != 0) {
        fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        return NULL;
    }

    int fd;
    if ((fd = open(nomeFifoAcesso, O_RDONLY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "Erro ao abrir o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        unlink(nomeFifoAcesso);
        return NULL;
    }

    viatura_t *buf = (viatura_t *) malloc(sizeof(viatura_t));
    while (elapsed/tcks < tempoAbertura) {
        if ((read(fd, buf, sizeof(viatura_t))) > 0) {
            //printf("%c : ID = %03d    ACESSO = %c    DURACAO = %03d    FIFO = %s\n", acesso, buf->id, buf->portaEntrada, (int) buf->duracao, buf->nomeFifo);

            pthread_t arrumador;
            viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t));
            *viatura = *buf;
            if ((pthread_create(&arrumador, NULL, thr_arrumador, viatura)) != 0) {
                fprintf(stderr, "Erro ao criar a thread arrumador ( veiculo.id = %d )\n", viatura->id);
                deleteViatura(buf);
                deleteViatura(viatura);
                unlink(nomeFifoAcesso);
                return NULL;
            }
        }
    }
    while ((read(fd, buf, sizeof(viatura_t))) > 0) {
        pthread_t arrumador;
        viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t));
        *viatura = *buf;
        if ((pthread_create(&arrumador, NULL, thr_arrumador, viatura)) != 0) {
            fprintf(stderr, "Erro ao criar a thread arrumador ( veiculo.id = %d )\n", viatura->id);
            deleteViatura(buf);
            deleteViatura(viatura);
            unlink(nomeFifoAcesso);
            return NULL;
        }
    }

    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        deleteViatura(buf);
        unlink(nomeFifoAcesso);
        return NULL;
    }

    printf("FECHANDO CONTROLADOR %c\n", acesso);
    deleteViatura(buf);
    unlink(nomeFifoAcesso);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    pthread_t controladores[NUM_ACESSOS];
    char acessos[] = {ACESSO_NORTE, ACESSO_SUL, ACESSO_ESTE, ACESSO_OESTE};
    clock_t start, current;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n_lugares> <t_abertura>\n", argv[0]);
        exit(1);
    }

    numeroLugares = atoi(argv[1]);
    tempoAbertura = atoi(argv[2]);
    lugaresDisponiveis = numeroLugares;

    tcks = sysconf(_SC_CLK_TCK);
    start = times(NULL);

    logFile = fopen("parque.log", "w");
    fprintf(logFile, "t(ticks) ; nlug ; id_viat ; observ\n");

    int i;
    for (i = 0; i < NUM_ACESSOS; i++) {
        if ((pthread_create(&controladores[i], NULL, thr_controlador, &acessos[i])) != 0) {
            fprintf(stderr, "Erro ao criar a thread controlador %c\n", acessos[i]);
            exit(1);
        }
    }

    while ((elapsed = (double)(current - start))/tcks < tempoAbertura) {
        current = times(NULL);
    }
    printf("CORREU DURANTE: %f s\n", elapsed/tcks);

    for (i = 0; i < NUM_ACESSOS; i++) {
        if ((pthread_join(controladores[i], NULL)) != 0) {
            fprintf(stderr, "Erro ao fazer join da thread controlador %c\n", acessos[i]);
            exit(1);
        }
    }

    pthread_mutex_destroy(&mut);
    pthread_exit(0);
}
