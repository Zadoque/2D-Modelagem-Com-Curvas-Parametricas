#ifndef GEOMETRIA_CURVAS_H
#define GEOMETRIA_CURVAS_H

#include "aabb.h"
#include "bspline_topologia.h"
#include "menu.h"

typedef struct {
  int inicio, quantidade;
} FaixaCurva;
typedef struct {
  Pontos vertices;
  FaixaCurva *faixas;
  int quantidade_faixas;
  int capacidade_faixas;
  int fechado;
} GeometriaCurvas;

void geometria_curvas_init(GeometriaCurvas *geometria);
void geometria_curvas_free(GeometriaCurvas *geometria);
int geometria_curvas_atualizar(GeometriaCurvas *geometria, Pontos *scratch, AABB_vec *boxes,
                               const Pontos *controles, Curva tipo, int fechado);
int geometria_curvas_atualizar_bspline_topologica(GeometriaCurvas *geometria, AABB_vec *boxes,
                                                   const BSplineTopologia *spline);

#endif
