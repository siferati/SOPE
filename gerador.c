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

const char acessos[] = {ACESSO_NORTE, ACESSO_SUL, ACESSO_ESTE, ACESSO_OESTE}; /**< Acessos disponiveis no Parque */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; /**< Mutex para sincronizacao das threads */
double elapsed; /**< Tempo decorrido desde o inicio da geracao em ticks */
FILE *logFile; /**< Ficheiro para o qual é feito log */

/**
* @brief getline() que funciona com file descriptors
* Funcao fornecida nos slides das aulas teoricas
*
* @param fd File Descriptor do ficheiro a ler
* @param str String onde se guarda a linha lida
* @return TRUE se foi lida uma linha, FALSE se nao ha linhas para ler (possivelmente EOF). -1 se erro
*/
int readline(int fd, char *str) {
    int n;
    do {
        n = read(fd, str, 1);
    } while (n>0 && *str++ != '\0');
    return (n>0);
}

/**
* @brief Thread Viatura
*
* @param arg Viatura pela qual a thread e responsavel
* @return NULL
*/
void *thr_viatura (void *arg) {
    // Torna a thread detached
    if ((pthread_detach(pthread_self())) != 0) {
        fprintf(stderr, "Erro ao tornar a thread %d detached (%d %s)\n", (int) pthread_self(), errno, strerror(errno));
        deleteViatura((viatura_t *) arg);
        return NULL;
    }

    viatura_t *v;
    v = (viatura_t *) malloc(sizeof(viatura_t));
    v = (viatura_t *) arg;

    // Cria o fifo privado da viatura
    if ((mkfifo(v->nomeFifo, 0600)) != 0) {
        fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", v->nomeFifo, errno, strerror(errno));
        deleteViatura(v);
        return NULL;
    }

    int fdPrivado;
    // Abre o fifo privado da viatura em modo leitura
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
    // Abre o fifo do controlador em causa
    if ((fd = open(nomeFifoAcesso, O_WRONLY | O_NONBLOCK)) == -1) {
        // Log do evento 'encerrado'
        // Isto pode acontecer porque o fifo esta fechado (parque encerrado) ou porque a abertura do fifo falhou
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; encerrado\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
        close(fdPrivado);
        unlink(v->nomeFifo);
        deleteViatura(v);
        pthread_mutex_unlock(&mut);
        return NULL;
    }

    clock_t begin = times(NULL); // Tempo de entrada da viatura no parque
    // Escreve a informacao da viatura no fifo do controlador
    write(fd, v, sizeof(viatura_t));

    // Fecha o fifo do controlador
    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        unlink(v->nomeFifo);
        deleteViatura(v);
        pthread_mutex_unlock(&mut);
        return NULL;
    }
    pthread_mutex_unlock(&mut);

    // Le o seu fifo privado ate receber uma mensagem indicativa de que deve sair
    char buf[256]; buf[0] = '\0';
    while ((strcmp(buf, "saida")) != 0 && (strcmp(buf, "cheio!")) != 0 && (strcmp(buf, "encerrado")) != 0) {
        buf[0] = '\0';
        // Log do evento 'entrada'
        if (readline(fdPrivado, buf) > 0)
            if (strcmp(buf, "entrada") == 0)
                fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; entrada\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
    }
    // Log do evento 'cheio!'
    if ((strcmp(buf, "cheio!")) == 0) {
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; cheio!\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
    }
    // Log do evento 'encerrado'
    if ((strcmp(buf, "encerrado")) == 0) {
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ;    ?   ; encerrado\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao);
    }
    clock_t lifetime = times(NULL) - begin;
    // Log do evento 'saida'
    if ((strcmp(buf, "saida")) == 0) {
        fprintf(logFile, "%7d  ; %4d    ; %4c   ; %6d     ; %4d   ; saída\n", (int) elapsed, v->id, v->portaEntrada, (int) v->duracao, (int) lifetime);
    }

    // Fecha o fifo privado da viatura
    if ((close(fdPrivado)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", v->nomeFifo, errno, strerror(errno));
        unlink(v->nomeFifo);
        deleteViatura(v);
        return NULL;
    }

    // Liberta memoria e termina a thread
    unlink(v->nomeFifo);
    deleteViatura(v);
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned int tGeracao, tProxGeracao, id = 0; // Tempo total de geracao, tempo ate a proxima geracao e id dos veiculos, respectivamente
    clock_t uRelogio, start; // Unidades de relogio e tempo de inicio da geracao, respectivamente
    long tcks; // Ticks por segundo
    pthread_t threads[MAX_THREADS]; // Threads viatura

    if (argc != 3) {
        printf("Usage: %s <t_geracao> <u_relogio>\n", argv[0]);
        exit(1);
    }

    // Abre o ficheiro de log, e escreve a primeira linha
    logFile = fopen("gerador.log", "w");
    fprintf(logFile, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n");

    tGeracao = atoi(argv[1]);
    uRelogio = atoi(argv[2]);

    srand(time(NULL));
    tcks = sysconf(_SC_CLK_TCK);
    start = times(NULL);

    // Gera aleatoriamente o tempo para a proxima geracao
    unsigned int intGeracao = rand() % GER_PROB_0;
    if (intGeracao < GER_PROB_2) tProxGeracao = 2 * uRelogio;
    else if (intGeracao < GER_PROB_1) tProxGeracao = uRelogio;
    else if (intGeracao < GER_PROB_0) tProxGeracao = 0;

    // Enquanto o tempo decorrido for menor que o tempo de geracao, cria novas viaturas e encaminha-as para threads viatura
    while ((elapsed = (double)(times(NULL) - start))/tcks < tGeracao) {
        while (elapsed >= tProxGeracao) {
            unsigned int nAcesso = rand() % NUM_ACESSOS;
            char acesso = acessos[nAcesso]; // Acesso ao parque
            unsigned int tEstacionamento = (rand() % MAX_TEMPO_ESTAC + 1) * uRelogio; // Duracao do estacionamento

            char nomeFifoPrivado[16]; // Nome do fifo privado da viatura
            sprintf(nomeFifoPrivado, "/tmp/fifo%03d", id);

            // Cria a viatura e encaminha-a para uma thread viatura
            viatura_t *v = createViatura(id, acesso, tEstacionamento, nomeFifoPrivado);
            if ((pthread_create(&threads[id++], NULL, thr_viatura, (void *) v)) != 0) {
                fprintf(stderr, "Erro ao criar a thread com id %d (%d %s)\n", id - 1, errno, strerror(errno));
                deleteViatura(v);
                pthread_exit(NULL);
            }

            // Gera aleatoriamente o tempo para a proxima geracao
            intGeracao = rand() % GER_PROB_0;
            if (intGeracao < GER_PROB_2) tProxGeracao = elapsed + 2 * uRelogio;
            else if (intGeracao < GER_PROB_1) tProxGeracao = elapsed + uRelogio;
            else if (intGeracao < GER_PROB_0) tProxGeracao = elapsed;
        }
    }

    // Destroi o mutex e termina a main thread, permitindo que as threads por si criadas terminem
    pthread_mutex_destroy(&mut);
    pthread_exit(NULL);
}
