#ifndef HERMITE_H
#define HERMITE_H

#include "../pontos.h"
#include "../aabb.h"
#include "ultis/ultis.h"
/**
 * Curva de Hermite
 * Interpola os dois pontos centrais. As tangentes são calculadas a partir
 * dos quatro controles recebidos por gerar_curva_hermite.
 */

// Gerar curva Hermite completa
void gerar_curva_hermite(ponto P0, ponto P1, ponto P2, ponto P3, Pontos *curva, AABB *box);
// Calcular um ponto específico na curva (t entre 0 e 1)
ponto calcular_ponto_hermite(ponto P1, ponto P2, ponto T1, ponto T2, float t);

// Calcular tangentes automáticas baseadas nos pontos
void calcular_tangentes_hermite(ponto P1, ponto P2, ponto P3, ponto P4, ponto *T1, ponto *T2);

ResultadoPicking picking_hermite(AABBTREE *arvore, Pontos *clicks, ponto mouse, float tolerancia,
                                 float melhor_dist);
void arrastar_ponto_hermite(Pontos *clicks, int seg, float ti, ponto mouse);

#endif
