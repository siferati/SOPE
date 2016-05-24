/**
* Programa Parque
* Feito por João Pedro Silva e Tiago Silva
* Turma 6 de SOPE
*/
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

unsigned int tempoAbertura; /**< Tempo de abertura do Parque */
unsigned int numeroLugares; /**< Numero total de lugares do Parque */
unsigned int lugaresDisponiveis; /**< Numero de lugares disponiveis para estacionar do Parque */
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER; /**< Mutex para sincronizacao das threads */
double elapsed; /**< Tempo decorrido desde a abertura do parque em ticks */
clock_t tcks; /**< Numero de ticks em 1 segundo */
FILE *logFile; /**< Ficheiro para o qual é feito log */

/**
* @brief Thread Arrumador
*
* @param arg Viatura a acompanhar
* @return NULL
*/
void *thr_arrumador(void * arg) {
    // Torna a thread detached
    if ((pthread_detach(pthread_self())) != 0) {
        fprintf(stderr, "Erro ao tornar a thread %d detached (%d %s)\n", (int) pthread_self(), errno, strerror(errno));
        deleteViatura((viatura_t *) arg);
        return NULL;
    }

    viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t)); // Apontador para a viatura sobre a qual e responsavel
    viatura = (viatura_t *) arg; // Recolhe a informacao da viatura de que esta encarregue

    int fd;
    // Abre o fifo privado da viatura para escrita
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
            // Se houver vaga no parque, reserva-a e notifica o veiculo da sua entrada
            sprintf(message, "entrada");
            if ((write(fd, message, strlen(message) + 1)) == -1) {
                fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
                unlink(viatura->nomeFifo);
                deleteViatura(viatura);
                return NULL;
            }

            // Log do evento 'entrada'
            fprintf(logFile, "%7d  ; %4d ; %5d   ; estacionamento\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
            printf("VIATURA ID = %03d ADMITIDA (NUM_LUGARES = %d)\n", viatura->id, lugaresDisponiveis);

            // Temporizador que controla o tempo de estacionamento da viatura
            clock_t startL = times(NULL), currentL = times(NULL), elapsedL;
            while ((elapsedL = currentL - startL) < viatura->duracao) {
                currentL = times(NULL);
            }

            // Terminado o tempo de estacionamento, notifica o veiculo da sua saida
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

            // Log do evento 'saida'
            printf("VIATURA ID = %03d DE SAIDA (DURACAO = %d    NUM_LUGARES = %d)\n", viatura->id, (int) elapsed, lugaresDisponiveis);
            fprintf(logFile, "%7d  ; %4d ; %5d   ; saida\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
        } else {
            pthread_mutex_unlock(&mut);
            char message[256];
            // Nao havendo vaga no parque, notifica o veiculo que o parque esta cheio
            sprintf(message, "cheio!");
            if ((write(fd, message, strlen(message) + 1)) == -1) {
                fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
                unlink(viatura->nomeFifo);
                deleteViatura(viatura);
                return NULL;
            }

            // Log do evento 'cheio!'
            printf("VIATURA ID = %03d RECUSADA\n", viatura->id);
            fprintf(logFile, "%7d  ; %4d ; %5d   ; cheio\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
        }
    } else {
        char *message = "encerrado";
        // Se o parque estiver encerrado e o veiculo estiver no fifo do controlador, e notificado que o parque nao aceita novas viaturas
        if ((write(fd, message, strlen(message))) == -1) {
            fprintf(stderr, "Erro ao escrever para o fifo %s (%d %s)\n", viatura->nomeFifo, errno, strerror(errno));
            unlink(viatura->nomeFifo);
            deleteViatura(viatura);
            return NULL;
        }

        // Log do evento 'encerrado'
        fprintf(logFile, "%7d  ; %4d ; %5d   ; encerrado\n", (int) elapsed, numeroLugares - lugaresDisponiveis, viatura->id);
    }

    // Fecha o fifo da viatura
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

/**
* @brief Thread Controlador
*
* @param arg Acesso do controlador (N, S, E ou O)
* @return NULL
*/
void *thr_controlador(void *arg) {
    char acesso = *(char *) arg; // Char correspondente ao acesso do controlador
    char nomeFifoAcesso[16]; // Nome do fifo proprio do controlador

    // Cria o seu fifo proprio, identificado por "fifo?"
    // (Onde ? e um de: 'N', 'S', 'E', 'O')
    sprintf(nomeFifoAcesso, "/tmp/fifo%c", acesso);
    if ((mkfifo(nomeFifoAcesso, 0600)) != 0) {
        fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        return NULL;
    }

    int fd;
    // Abre o seu fifo para leitura
    if ((fd = open(nomeFifoAcesso, O_RDONLY | O_NONBLOCK)) == -1) {
        fprintf(stderr, "Erro ao abrir o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        unlink(nomeFifoAcesso);
        return NULL;
    }

    // Enquanto o parque esta aberto, recebe pedidos de acesso atraves do seu fifo
    viatura_t *buf = (viatura_t *) malloc(sizeof(viatura_t)); // Buffer para guardar informacao recebida
    while (elapsed/tcks < tempoAbertura) {
        if ((read(fd, buf, sizeof(viatura_t))) > 0) {

            pthread_t arrumador; // Thread arrumador
            viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t)); // Buffer para guardar informacao recebida
            *viatura = *buf; // Informacao e copiada do buffer principal para um especifico para a thread arrumador
            // Encaminha a viatura recebida para uma thread arrumador
            if ((pthread_create(&arrumador, NULL, thr_arrumador, viatura)) != 0) {
                fprintf(stderr, "Erro ao criar a thread arrumador ( veiculo.id = %d )\n", viatura->id);
                deleteViatura(buf);
                deleteViatura(viatura);
                unlink(nomeFifoAcesso);
                return NULL;
            }
        }
    }
    // Quando o parque fecha encaminha as viaturas que se encontram no fifo para serem lidas para threads arrumador
    while ((read(fd, buf, sizeof(viatura_t))) > 0) {
        pthread_t arrumador; // Thread arrumador
        viatura_t *viatura = (viatura_t *) malloc(sizeof(viatura_t)); // Buffer para guardar informacao recebida
        *viatura = *buf; // Informacao e copiada do buffer principal para um especifico para a thread arrumador
        // Encaminha a viatura recebida para uma thread arrumador
        if ((pthread_create(&arrumador, NULL, thr_arrumador, viatura)) != 0) {
            fprintf(stderr, "Erro ao criar a thread arrumador ( veiculo.id = %d )\n", viatura->id);
            deleteViatura(buf);
            deleteViatura(viatura);
            unlink(nomeFifoAcesso);
            return NULL;
        }
    }

    // Fecha o seu fifo
    // Potenciais novos clientes sao notificados que o parque esta fechado porque nao conseguem aceder ao fifo do controlador
    if ((close(fd)) != 0) {
        fprintf(stderr, "Erro ao fechar o fifo %s (%d %s)\n", nomeFifoAcesso, errno, strerror(errno));
        deleteViatura(buf);
        unlink(nomeFifoAcesso);
        return NULL;
    }

    // Liberta memoria e termina o programa, permitindo que as threads por si iniciadas continuem o funcionamento
    printf("FECHANDO CONTROLADOR %c\n", acesso);
    deleteViatura(buf);
    unlink(nomeFifoAcesso);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    pthread_t controladores[NUM_ACESSOS]; // Threads controladores
    char acessos[] = {ACESSO_NORTE, ACESSO_SUL, ACESSO_ESTE, ACESSO_OESTE}; // Chars que representam os acessos
    clock_t start, current; // Tempo de inicio do programa e tempo atual, respectivamente

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <n_lugares> <t_abertura>\n", argv[0]);
        exit(1);
    }

    numeroLugares = atoi(argv[1]);
    tempoAbertura = atoi(argv[2]);
    lugaresDisponiveis = numeroLugares;

    tcks = sysconf(_SC_CLK_TCK);
    start = times(NULL);

    // Abre o ficheiro de log, e escreve a primeira linha
    logFile = fopen("parque.log", "w");
    fprintf(logFile, "t(ticks) ; nlug ; id_viat ; observ\n");

    int i;
    // Cria as threads controlador
    for (i = 0; i < NUM_ACESSOS; i++) {
        if ((pthread_create(&controladores[i], NULL, thr_controlador, &acessos[i])) != 0) {
            fprintf(stderr, "Erro ao criar a thread controlador %c\n", acessos[i]);
            exit(1);
        }
    }

    // Temporizador que controla o tempo de execucao do programa
    while ((elapsed = (double)(current - start))/tcks < tempoAbertura) {
        current = times(NULL);
    }

    // Faz join das threads controlador
    for (i = 0; i < NUM_ACESSOS; i++) {
        if ((pthread_join(controladores[i], NULL)) != 0) {
            fprintf(stderr, "Erro ao fazer join da thread controlador %c\n", acessos[i]);
            exit(1);
        }
    }

    // Destroi o mutex e sai do programa
    // Deixa as threads que criou terminarem a sua execucao
    pthread_mutex_destroy(&mut);
    pthread_exit(0);
}
