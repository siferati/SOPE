/**
* Programa Gerador
* Feito por João Pedro Silva e Tiago Silva
* Turma 6 de SOPE
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "viatura.h"

#define MAX_THREADS 100000

const char acessos[] = {ACESSO_NORTE, ACESSO_SUL, ACESSO_ESTE, ACESSO_OESTE};
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
double elapsed;
FILE *logFile;

int readline(int fd, char *str) {
    int n;
    do {
        n = read(fd, str, 1);
    } while (n>0 && *str++ != '\0');
    return (n>0);
}

void *thr_viatura (void *arg) {
    if ((pthread_detach(pthread_self())) != 0) {
        fprintf(stderr, "Erro ao tornar a thread %d detached (%d %s)\n", (int) pthread_self(), errno, strerror(errno));
        deleteViatura((viatura_t *) arg);
        return NULL;
    }

    viatura_t *v;
    v = (viatura_t *) malloc(sizeof(viatura_t));
    v = (viatura_t *) arg;

    if ((mkfifo(v->nomeFifo, 0600)) != 0) {
        fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", v->nomeFifo, errno, strerror(errno));
        deleteViatura(v);
        return NULL;
    }

    int fdPrivado;
    if ((fdPrivado = open(v->nomeFifo, O_RDONLY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "Erro ao abrir o fifo %s (%d %s)\n", v->nomeFifo, errno, strerror(errno));
        unlink(v->nomeFifo);
        deleteViatura(v);
        return NULL;
    }

    char nomeFifoAcesso[16];
    sprintf(nomeFifoAcesso, "/tmp/fifo%c", v->portaEntrada);

    printf("ID = %03d    ACESSO = %c    DURACAO = %03d    FIFO = %s\n", v->id, v->portaEntrada, (int) v->duracao, v->nomeFifo);

    pthread_mutex_lock(&mut);
    int fd;
    if ((fd = open(nomeFifoAcesso, O_WRONLY | O_NONBLOCK)) == -1) {
        //fprintf(stderr, "Erro ao abrir o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; encerrado\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
        unlink(v->nomeFifo);
        deleteViatura(v);
        pthread_mutex_unlock(&mut);
        return NULL;
    }

    clock_t begin = times(NULL);
    write(fd, v, sizeof(viatura_t));

    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        unlink(v->nomeFifo);
        deleteViatura(v);
        pthread_mutex_unlock(&mut);
        return NULL;
    }
    pthread_mutex_unlock(&mut);

    char buf[256]; buf[0] = '\0';
    while ((strcmp(buf, "saida")) != 0 && (strcmp(buf, "cheio!")) != 0 && (strcmp(buf, "encerrado")) != 0) {
        buf[0] = '\0';
        if (readline(fdPrivado, buf) > 0)
            if (strcmp(buf, "entrada") == 0)
                fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; entrada\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
    }
    if ((strcmp(buf, "cheio!")) == 0) {
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; cheio!\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
    }
    clock_t lifetime = times(NULL) - begin;
    if ((strcmp(buf, "saida")) == 0) {
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ; %4d   ; saída\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao, (int) lifetime);
    }

    printf("FECHANDO FIFO PRIVADO = %s\n", v->nomeFifo);
    if ((close(fdPrivado)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", v->nomeFifo, errno, strerror(errno));
        unlink(v->nomeFifo);
        deleteViatura(v);
        return NULL;
    }

    unlink(v->nomeFifo);
    deleteViatura(v);
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned int tGeracao, tProxGeracao, id = 0;
    clock_t uRelogio, start;
    long tcks;

    pthread_t threads[MAX_THREADS];

    if (argc != 3) {
        printf("Usage: %s <t_geracao> <u_relogio>\n", argv[0]);
        exit(1);
    }

    logFile = fopen("gerador.log", "w");
    fprintf(logFile, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n");

    tGeracao = atoi(argv[1]);
    uRelogio = atoi(argv[2]);

    srand(time(NULL));
    tcks = sysconf(_SC_CLK_TCK);
    start = times(NULL);

    unsigned int intGeracao = rand() % GER_PROB_0;
    if (intGeracao < GER_PROB_2) tProxGeracao = 2 * uRelogio;
    else if (intGeracao < GER_PROB_1) tProxGeracao = uRelogio;
    else if (intGeracao < GER_PROB_0) tProxGeracao = 0;

    while ((elapsed = (double)(times(NULL) - start))/tcks < tGeracao) {
        while (elapsed >= tProxGeracao) {
            unsigned int nAcesso = rand() % NUM_ACESSOS;
            char acesso = acessos[nAcesso];
            unsigned int tEstacionamento = (rand() % MAX_TEMPO_ESTAC + 1) * uRelogio;

            char nomeFifoPrivado[16];
            sprintf(nomeFifoPrivado, "/tmp/fifo%03d", id);

            viatura_t *v = createViatura(id, acesso, tEstacionamento, nomeFifoPrivado);
            if ((pthread_create(&threads[id++], NULL, thr_viatura, (void *) v)) != 0) {
                fprintf(stderr, "Erro ao criar a thread com id %d (%d %s)\n", id - 1, errno, strerror(errno));
                deleteViatura(v);
                pthread_exit(NULL);
            }

            intGeracao = rand() % GER_PROB_0;
            if (intGeracao < GER_PROB_2) tProxGeracao = elapsed + 2 * uRelogio;
            else if (intGeracao < GER_PROB_1) tProxGeracao = elapsed + uRelogio;
            else if (intGeracao < GER_PROB_0) tProxGeracao = elapsed;
            //printf("creating next when elapsed >= %d\n", tProxGeracao);
        }
    }

    pthread_mutex_destroy(&mut);
    pthread_exit(NULL);
}
