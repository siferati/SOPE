/**
 * @file viatura.h
 * @author João Pedro Gomes Silva (SOPE T06)
 * @author Tiago Rafael Ferreira da Silva (SOPE T06)
 * @date Maio 2016
 * @brief Ficheiro .h com a declaraçao da struct viatura e os metodos a esta associados
 */

#pragma once
#include <time.h>

/**
 * @brief Struct que representa uma viatura
 */
typedef struct viatura {
    unsigned int id; /**< ID da viatura (unique) */
    char portaEntrada; /**< Porta de Entrada ao Parque (N, S, E ou O) */
    clock_t duracao; /**< Duracao da estadia da viatura no Parque */
} viatura;

/**
* @brief Constructor
*
* @param id ID da viatura
* @param portaEntrada Porta de Entrada da viatura ao Parque (N, S, E ou O)
* @param duracao duracao Duracao da estadia da viatura no parque
* @return Apontador para a viatura criada
*/
viatura *createViatura(unsigned int id, char portaEntrada, clock_t duracao);

/**
* @brief Elimina viatura (liberta memoria alocada)
*
* @param viatura Viatura a eliminar
*/
void deleteViatura(viatura *viatura);
