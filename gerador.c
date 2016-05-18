/**
Programa Gerador
Feito por João Pedro Silva e Tiago Silva
Turma 6 de SOPE
*/

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
  //caso a chamada na shell nao tenha o formato correcto
  if (argc != 3)
  {
    printf("Program call should follow this format: gerador <T_GERACAO> <U_RELOGIO>\n");
    exit(1);
  }
  return 0;
}
