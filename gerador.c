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

#define MAX_THREADS 1000

const char acessos[] = {ACESSO_NORTE, ACESSO_SUL, ACESSO_ESTE, ACESSO_OESTE};
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

void *thr_viatura (void *arg) {
    if ((pthread_detach(pthread_self())) != 0) {
        fprintf(stderr, "Erro ao tornar a thread %d detached (%d %s)\n", (int) pthread_self(), errno, strerror(errno));
        deleteViatura((viatura *) arg);
        return NULL;
    }

    viatura *v;
    v = (viatura *) malloc(sizeof(viatura));
    v = (viatura *) arg;

    char nomeFifoPrivado[16];
    sprintf(nomeFifoPrivado, "/tmp/fifo%03d", v->id);

    if ((mkfifo(nomeFifoPrivado, 0600)) != 0) {
        fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", nomeFifoPrivado, errno, strerror(errno));
        deleteViatura(v);
        return NULL;
    }

    char nomeFifoAcesso[16];
    sprintf(nomeFifoAcesso, "/tmp/fifo%c", v->portaEntrada);

    printf("ID = %03d    ACESSO = %c    DURACAO = %03d    FIFO ACESSO = %s\n", v->id, v->portaEntrada, (int) v->duracao, nomeFifoAcesso);

    pthread_mutex_lock(&mut);
    int fd;
    // para já não funciona, o fifo nao existe
    if ((fd = open(nomeFifoAcesso, O_WRONLY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "Erro ao abrir o fifo %s (%d %s) \n", nomeFifoAcesso, errno, strerror(errno));
        deleteViatura(v);
        unlink(nomeFifoPrivado);
        pthread_mutex_unlock(&mut);
        return NULL;
    }
    // escrever informacao sobre este veiculo
    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s) \n", nomeFifoAcesso, errno, strerror(errno));
        deleteViatura(v);
        unlink(nomeFifoPrivado);
        pthread_mutex_unlock(&mut);
        return NULL;
    }
    pthread_mutex_unlock(&mut);

    deleteViatura(v);
    unlink(nomeFifoPrivado);
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned int tGeracao, tProxGeracao, id = 0;
    clock_t uRelogio, start;
    double elapsed;
    long tcks;

    pthread_t threads[MAX_THREADS];

    if (argc != 3) {
        printf("Usage: %s <t_geracao> <u_relogio>\n", argv[0]);
        exit(1);
    }

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

            viatura *v = createViatura(id, acesso, tEstacionamento, NULL);
            //printf("created thread %d...\n", id);
            pthread_create(&threads[id++], NULL, thr_viatura, (void *) v);

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
