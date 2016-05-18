#include "viatura.h"
#include <stdlib.h>

viatura *createViatura(unsigned int id, char portaEntrada, clock_t duracao) {
    viatura *viatura = malloc(sizeof(viatura));
    viatura->id = id;
    viatura->portaEntrada = portaEntrada;
    viatura->duracao = duracao;
    return viatura;
}

void deleteViatura(viatura *viatura)
{
    free(viatura);
}
