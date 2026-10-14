#ifndef CURVA_GENERICA_H
#define CURVA_GENERICA_H

#include "../aabb.h"
#include "../pontos.h"
#include "ultis/ultis.h"
#include <stddef.h>

typedef void (*CurvaCubicaPesos)(float t, float pesos[4]);

typedef struct {
    CurvaCubicaEval avaliar;
    CurvaCubicaDerivada derivar;
    CurvaCubicaPesos pesos_controles;
} CurvaCubicaOps;

typedef struct {
    float tolerancia_geometrica;
    float tolerancia_parametrica;
    unsigned profundidade_maxima;
    size_t max_nos_geracao;
    size_t max_nos_picking;
} CurvaSubdivisaoConfig;

CurvaSubdivisaoConfig curva_subdivisao_config_padrao(void);
void curva_subdivisao_definir_config(CurvaSubdivisaoConfig config);
CurvaSubdivisaoConfig curva_subdivisao_obter_config(void);

void gerar_segmento_curva_cubica(const CurvaCubicaOps *ops,
                                 ponto P0, ponto P1, ponto P2, ponto P3,
                                 Pontos *resultado, AABB *box);

ResultadoPicking picking_curva_cubica(const CurvaCubicaOps *ops,
                                      AABBTREE *arvore, Pontos *controles,
                                      ponto mouse, float tolerancia,
                                      float melhor_dist);

void arrastar_segmento_curva_cubica(const CurvaCubicaOps *ops,
                                    Pontos *controles, int segmento,
                                    float t, ponto mouse);

#endif
