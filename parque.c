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
#include "viatura.h"

#define NORTE 0
#define SUL 1
#define ESTE 2
#define OESTE 3

int lotacao; /**< Numero de lugares do Parque */
int tempo; /**< Tempo de abertura do Parque (basicamente, funciona como um timer que decrementa ate chegar a 0) */

/**
* @brief getline() que funciona com file descriptors
* Funcao fornecida nos slides das aulas teoricas
*
* @param fd File Descriptor do ficheiro a ler
* @param str String onde se guarda a linha lida
* @return TRUE se foi lida uma linha, FALSE se nao ha linhas para ler (possivelmente EOF). -1 se erro
*/
int readline(int fd, char *str)
{
  int n;
  do
  {
    n = read(fd,str,1);
  }
  while (n>0 && *str++ != '\0');
  return (n>0);
}

/**
* @brief Thread Arrumador
*
* @param arg Acesso do controlador (N, S, E ou O)
* @return NULL em caso de erro
*/
void* arrumador(void* arg)
{
exit(0);
}

/**
* @brief Thread Controlador
*
* @param arg Acesso do controlador (N, S, E ou O)
* @return NULL em caso de erro
*/
void* controlador(void* arg)
{
  //Indica qual o ponto de acesso deste controlador: N, S, E ou O
  char acesso = *(char *) arg;
  //nome do FIFO (5 chars + '\0')
  char nomeFifoPrivado[6];
  sprintf(nomeFifoPrivado, "/tmp/fifo%c", acesso);
  //criar FIFO
  if ((mkfifo(nomeFifoPrivado, 0600)) != 0) {
    fprintf(stderr, "Erro ao criar o fifo %s (%d %s)\n", nomeFifoPrivado, errno, strerror(errno));
    return NULL;
  }
  //file descriptor do fifo
  int fd;
  //open fifo para leitura
  if ((fd = open(nomeFifoPrivado, O_RDONLY | O_NONBLOCK)) == -1) {
    fprintf(stderr, "Erro ao abrir o fifo %s (%d %s) \n", nomeFifoPrivado, errno, strerror(errno));
    unlink(nomeFifoPrivado);
    return NULL;
  }
  //criar viatura
  viatura *viatura = createViatura(0,0,0, NULL);
  //buffer de leitura
  char buffer[64];
  //Index. Ordem de leitura: portaEntrada, duracao, id, nomeFifo
  int i = 0;
  //boolean que indica se a thread esta a correr
  int running = 1;
  //ler linha a linha.
  //TODO atoi(buffer) pode nao funcionar uma vez que a maior parte do buffer tem "lixo", se isto acontecer e preciso corrigir
  while (running) {
    while (readline(fd, buffer))
    {
      switch (i) {
        case 0: //id
        viatura->id = atoi(buffer);
        break;
        case 1:
        viatura->portaEntrada = atoi(buffer);
        break;
        case 2:
        viatura->duracao = atoi(buffer);
        break;
        case 3:
        strcpy(viatura->nomeFifo, buffer);
        //criar arrumador para a viatura
        pthread_t threadArrumador;
        if (pthread_create(&threadArrumador, NULL, arrumador, &viatura) != 0)
        {
          fprintf(stderr, "Erro ao criar a thread controlador Norte (%d %s)\n", errno, strerror(errno));
          deleteViatura(viatura);
          unlink(nomeFifoPrivado);
          exit(1);
        }
        break;
        default:
        break;
      }
      //se o parque tiver encerrado, fechar o fifo.
      if (tempo <= 0)
      {
        //thread will stop running as soon as there are no more things to read on the FIFO
        running = 0;
        if ((close(fd)) != 0) {
          fprintf(stderr, "Erro ao fechar o fifo %s (%d %s) \n", nomeFifoPrivado, errno, strerror(errno));
          deleteViatura(viatura);
          unlink(nomeFifoPrivado);
          return NULL;
        }
      }
      //set next ite
      i++;
      if (i > 3)
      i = 0;
    }
  }
  //when it's over, delete the fifo (it was already closed prev)
  unlink(nomeFifoPrivado);
  exit(0);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Usage: %s <n_lugares> <t_abertura>\n", argv[0]);
    exit(1);
  }
  //set lotacao
  lotacao = atoi(argv[1]);
  //set tempo
  tempo = atoi(argv[2]);
  //criar o array de controladores
  pthread_t controladores[4];
  //controlador Norte
  if (pthread_create(&controladores[NORTE], NULL, controlador, "N") != 0)
  {
    fprintf(stderr, "Erro ao criar a thread controlador Norte (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //controlador Sul
  if (pthread_create(&controladores[SUL], NULL, controlador, "S") != 0)
  {
    fprintf(stderr, "Erro ao criar a thread controlador Sul (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //controlador Este
  if (pthread_create(&controladores[ESTE], NULL, controlador, "E") != 0)
  {
    fprintf(stderr, "Erro ao criar a thread controlador Este (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //controlador Oeste
  if (pthread_create(&controladores[OESTE], NULL, controlador, "O") != 0)
  {
    fprintf(stderr, "Erro ao criar a thread controlador Oeste (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //holder para o valor de retorno das threads
  int *retval;
  //esperar que a thread controlador Norte termine
  if (pthread_join(controladores[NORTE], (void **) &retval) != 0 || retval == NULL)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Norte (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Sul termine
  if (pthread_join(controladores[SUL], (void **) &retval) != 0 || retval == NULL)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Sul (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Este termine
  if (pthread_join(controladores[ESTE], (void **) &retval) != 0 || retval == NULL)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Este (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Oeste termine
  if (pthread_join(controladores[OESTE], (void **) &retval) != 0 || retval == NULL)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Oeste (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  printf("Todas as threads terminaram\n");

  //TODO efetuar e publicar estatísticas globais

  return 0;
}
