#include "viatura.h"
#include <stdlib.h>

viatura *createViatura(unsigned int id, unsigned int portaEntrada, clock_t duracao) {
    viatura *v = (viatura *) malloc(sizeof(viatura));
    v->id = id;
    v->portaEntrada = portaEntrada;
    v->duracao = duracao;
    return v;
}

void deleteViatura(viatura *viatura)
{
    free(viatura);
}
