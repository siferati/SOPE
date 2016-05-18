/**
* Programa Gerador
* Feito por João Pedro Silva e Tiago Silva
* Turma 6 de SOPE
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/times.h>
#include "viatura.h"

#define NUM_ACESSOS 5
#define ACESSO_NORTE 0
#define ACESSO_SUL 1
#define ACESSO_ESTE 2
#define ACESSO_OESTE 3

#define MAX_TEMPO_ESTAC 11
#define GER_PROB_TOTAL 100
#define GER_PROB_0 50
#define GER_PROB_1 30
#define GER_PROB_2 20

int main(int argc, char *argv[]) {
    unsigned int tGeracao, tProxGeracao, id = 0;
    clock_t uRelogio, start;
    double elapsed;
    long tcks;

    if (argc != 3) {
        printf("Usage: %s <t_geracao> <u_relogio>\n", argv[0]);
        exit(1);
    }

    tGeracao = atoi(argv[1]);
    uRelogio = atoi(argv[2]);

    srand(time(NULL));

    tcks = sysconf(_SC_CLK_TCK);
    start = times(NULL);
    while ((elapsed = (double)(times(NULL) - start))/tcks < tGeracao) {
        unsigned int nAcesso = rand() % NUM_ACESSOS;
        unsigned int tEstacionamento = (rand() % MAX_TEMPO_ESTAC + 1) * uRelogio;

        char acesso = "\0";
        switch(nAcesso) {
            case ACESSO_NORTE:
                acesso = 'N';
                break;
            case ACESSO_SUL:
                acesso = 'S';
                break;
            case ACESSO_ESTE:
                acesso = 'E';
                break;
            case ACESSO_OESTE:
                acesso = 'O';
                break;
        }

        viatura *v = createViatura(id++, acesso, tEstacionamento);

        unsigned int intGeracao = rand() % GER_PROB_TOTAL;
        if (intGeracao < GER_PROB_0) tProxGeracao = elapsed;
        if (intGeracao >= GER_PROB_0 && intGeracao < GER_PROB_0 + GER_PROB_1) tProxGeracao = elapsed + uRelogio;
        if (intGeracao >= GER_PROB_0 + GER_PROB_0 && intGeracao < GER_PROB_TOTAL) tProxGeracao = elapsed + 2*uRelogio;
    }
    return 0;
}
