#ifndef CURVAS_UTILS_H
#define CURVAS_UTILS_H

#include "../pontos.h"
#include "../aabb.h"

float dist_mouse_aabb(ponto mouse, AABB box);
float dist_mouse_aabb_left(ponto mouse, AABB box);
float dist_mouse_aabb_right(ponto mouse, AABB box);

typedef ponto (*CurvaCubicaEval)(ponto, ponto, ponto, ponto, float);
typedef ponto (*CurvaCubicaDerivada)(ponto, ponto, ponto, ponto, float);

void incluir_intervalo_curva_cubica_aabb(AABB *box, ponto P0, ponto P1, ponto P2, ponto P3,
                                         float t0, float t1, CurvaCubicaEval avaliar,
                                         CurvaCubicaDerivada derivar);

#endif
