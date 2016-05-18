#include "viatura.h"
#include <stdlib.h>

viatura *createViatura(int id, char pontoAcesso, int duracao)
{
  viatura *viatura = malloc(sizeof(viatura));
  viatura->id = id;
  viatura->pontoAcesso = pontoAcesso;
  viatura->duracao = duracao;
  return viatura;
}

void deleteViatura(viatura *viatura)
{
  free(viatura);
}
