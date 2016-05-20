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
#include "viatura.h"

#define NORTE 0
#define SUL 1
#define ESTE 2
#define OESTE 3

int lotacao; /**< Numero de lugares do Parque */
int tempo; /**< Tempo de abertura do Parque (basicamente, funciona como um timer que decrementa ate chegar a 0) */

/**
* @brief Thread Controlador
*
* @param arg Acesso do controlador (N, S, E ou O)
* @return (void *) 0 se foi bem sucessido, nao 0 em contrario
*/
void* controlador(void* arg)
{
  //Indica qual o ponto de acesso deste controlador: N, S, E ou O
  char acesso = *(char *) arg;
  printf("Acesso: %c\n", acesso);
  return (void *) 0;
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
  int retval;
  //esperar que a thread controlador Norte termine
  if (pthread_join(controladores[NORTE], (void *) &retval) != 0 || retval != 0)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Norte (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Sul termine
  if (pthread_join(controladores[SUL], (void *) &retval) != 0 || retval != 0)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Sul (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Este termine
  if (pthread_join(controladores[ESTE], (void *) &retval) != 0 || retval != 0)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Este (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  //esperar que a thread controlador Oeste termine
  if (pthread_join(controladores[OESTE], (void *) &retval) != 0 || retval != 0)
  {
    fprintf(stderr, "Erro ao fazer join da thread controlador Oeste (%d %s)\n", errno, strerror(errno));
    exit(1);
  }
  printf("Todas as threads terminaram\n");
  return 0;
  //test
}
