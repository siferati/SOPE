#include "viatura.h"

viatura_t *createViatura(unsigned int id, unsigned int portaEntrada, clock_t duracao, char* nomeFifo) {
    viatura_t *v = (viatura_t *) malloc(sizeof(viatura_t));
    v->id = id;
    v->portaEntrada = portaEntrada;
    v->duracao = duracao;
    strcpy(v->nomeFifo, nomeFifo);
    return v;
}

void deleteViatura(viatura_t *viatura)
{
    free(viatura);
}
