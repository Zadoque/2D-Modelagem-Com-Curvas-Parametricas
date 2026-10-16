#ifndef BSPLINE_H
#define BSPLINE_H
#include "../pontos.h"
#include "../aabb.h"
#include "utils.h"
// ─── Avaliação da curva ───────────────────────────────────────────────────────

ponto calcular_ponto_bspline(ponto P0, ponto P1, ponto P2, ponto P3, float t);

// ─── Geração da curva (subdivisão adaptativa) ────────────────────────────────

void gerar_curva_bspline(ponto P0, ponto P1, ponto P2, ponto P3, Pontos *curva_resultado,
                         AABB *box, ponto *ponto_final);

// ─── Picking ─────────────────────────────────────────────────────────────────

ResultadoPicking picking_bspline(AABBTREE *arvore, Pontos *clicks, ponto mouse, float tolerancia,
                                 float melhor_dist);

void arrastar_ponto_bspline(Pontos *clicks, int segmento_indice, float ti, ponto mouse,
                            float rigidez);
#endif // BSPLINE_H
