/**
 * @file viatura.h
 * @author João Pedro Gomes Silva (SOPE T06)
 * @author Tiago Rafael Ferreira da Silva (SOPE T06)
 * @date Maio 2016
 * @brief Ficheiro .h com a declaraçao da struct viatura e os metodos a esta associados
 */

#pragma once
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define NUM_ACESSOS 4
#define ACESSO_NORTE 78
#define ACESSO_SUL 83
#define ACESSO_ESTE 69
#define ACESSO_OESTE 79

#define MAX_TEMPO_ESTAC 11
#define GER_PROB_0 100
#define GER_PROB_1 50
#define GER_PROB_2 20

/**
 * @brief Struct que representa uma viatura
 */
typedef struct viatura {
    unsigned int id; /**< ID da viatura (unique) */
    unsigned int portaEntrada; /**< Porta de Entrada ao Parque (N, S, E ou O) */
    clock_t duracao; /**< Duracao da estadia da viatura no Parque */
    char nomeFifo[16]; /**< Nome do fifo criado pela thread responsavel por esta viatura */
} viatura_t;

/**
* @brief Constructor
*
* @param id ID da viatura
* @param portaEntrada Porta de Entrada da viatura ao Parque (N, S, E ou O)
* @param duracao duracao Duracao da estadia da viatura no parque
* @param Nome do fifo criado pela thread responsavel por esta viatura
* @return Apontador para a viatura criada
*/
viatura_t *createViatura(unsigned int id, unsigned int portaEntrada, clock_t duracao, char* nomeFifo);

/**
* @brief Elimina viatura (liberta memoria alocada)
*
* @param viatura Viatura a eliminar
*/
void deleteViatura(viatura_t *viatura);
