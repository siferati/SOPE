/**
* Struct que representa uma viatura
* Feito por João Pedro Silva e Tiago Silva
* Turma 6 de SOPE
*/

typedef struct viatura{
  int id;
  char pontoAcesso;
  int duracao;
} viatura;

/**
* Constructor
* @param id ID da viatura
* @param pontoAcesso Char que representa o ponto de acesso da viatura
* @param int duracao Duracao da estadia da viatura no parque
* @return Apontador para a viatura criada
*/
viatura *createViatura(int id, char pontoAcesso, int duracao);

/**
* Delete da Viatura (free da memoria)
*
* @param viatura Viatura a eliminar
*/
void deleteViatura(viatura *viatura);
