#ifndef CATMULLROM_H
#define CATMULLROM_H

#include "../pontos.h"
#include "../aabb.h"
#include "utils.h"
/**
 * Curva Catmull-Rom
 * Cada trecho definido por quatro controles interpola os dois controles
 * centrais; os controles externos determinam as tangentes.
 */
void arrastar_ponto_catmullrom(Pontos *clicks, int seg, float ti, ponto mouse, float rigidez);
ResultadoPicking picking_catmullrom(AABBTREE *arvore, Pontos *clicks, ponto mouse, float tolerancia,
                                    float melhor_dist);
ponto calcular_ponto_catmullrom(ponto P0, ponto P1, ponto P2, ponto P3, float t);
void gerar_curva_catmullrom(ponto P0, ponto P1, ponto P2, ponto P3, Pontos *curva, AABB *box,
                            ponto *ponto_final);

#endif
