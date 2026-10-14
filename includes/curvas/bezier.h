#ifndef BEZIER_H
#define BEZIER_H

#include "../pontos.h"
#include "../aabb.h"
#include "ultis/ultis.h"
/**
 * Curva de Bézier
 * - Usa pontos de controle que "atraem" a curva
 * - Sempre passa pelos pontos inicial e final
 * - Pontos intermediários controlam a forma
 * - Ideal para design gráfico
 */
void gerar_curva_bezier(ponto P0, ponto P1, ponto P2, ponto P3, Pontos *curva, AABB *box);
ponto calcular_ponto_bezier(ponto P0, ponto P1, ponto P2, ponto P3, float t);
void arrastar_ponto_bezier(Pontos *clicks, int seg, float ti, ponto mouse);
ResultadoPicking picking_bezier(AABBTREE *arvore, Pontos *clicks, ponto mouse, float tolerancia, float melhor_dist);
#endif
