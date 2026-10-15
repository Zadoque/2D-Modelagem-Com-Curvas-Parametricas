#include "geometria_curvas.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(void){
  Pontos controles, scratch;
  pontos_init(&controles, 6);
  pontos_init(&scratch, 16);
  for(int i = 0; i < 6; i++){
    ponto p = {{i * 20.0f, (i % 2) * 30.0f, 1}};
    assert(pontos_push(&controles, p) == 0);
  }
  AABB_vec boxes;
  aabb_vec_init(&boxes, 8);
  GeometriaCurvas g;
  geometria_curvas_init(&g);
  assert(geometria_curvas_atualizar(&g, &scratch, &boxes, &controles, MODO_CURVA_BEZIER, 1) == 0);
  assert(g.quantidade_faixas == 2 && boxes.quantidade == 2);
  assert(boxes.dados[0].segmento_indice == 0 && boxes.dados[1].segmento_indice == 3);
  assert(g.vertices.quantidade_atual > 0);
  assert(g.fechado == 1);
  ponto primeiro = g.vertices.data[0];
  ponto ultimo = g.vertices.data[g.vertices.quantidade_atual - 1];
  assert(fabsf(primeiro.point[0] - ultimo.point[0]) > 1e-5f ||
         fabsf(primeiro.point[1] - ultimo.point[1]) > 1e-5f);
  assert(geometria_curvas_atualizar(&g, &scratch, &boxes, &controles, MODO_CURVA_HERMITE, 1) == 0);
  assert(g.quantidade_faixas == 6 && boxes.quantidade == 6);
  for(int i = 0; i < 6; i++){
    assert(boxes.dados[i].segmento_indice == i);
  }

  controles.quantidade_atual = 0;
  for(int i = 0; i < 7; i++){
    ponto p = {{i * 20.0f, (i % 2) * 30.0f, 1}};
    assert(pontos_push(&controles, p) == 0);
  }
  assert(geometria_curvas_atualizar(&g, &scratch, &boxes, &controles, MODO_CURVA_BEZIER, 0) == 0);
  assert(g.quantidade_faixas == 2 && g.fechado == 0);
  ultimo = g.vertices.data[g.vertices.quantidade_atual - 1];
  assert(fabsf(ultimo.point[0] - controles.data[6].point[0]) < 1e-5f);
  assert(fabsf(ultimo.point[1] - controles.data[6].point[1]) < 1e-5f);
  FaixaCurva ultima_faixa = g.faixas[g.quantidade_faixas - 1];
  assert(ultima_faixa.inicio + ultima_faixa.quantidade == g.vertices.quantidade_atual);

  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  assert(geometria_curvas_atualizar_bspline_topologica(&g, &boxes, &spline) == 0);
  AABBTREE *arvore = aabb_vec_para_arvore(&boxes);
  ponto sobre_curva = bspline_topologia_avaliar(&spline, 1.25f);
  ResultadoPicking resultado = bspline_topologia_picking_arvore(&spline, arvore, sobre_curva, 0.01f);
  assert(resultado.segmento_indice >= 0);
  assert(fabsf(resultado.t - 1.25f) < 0.05f);
  aabb_tree_free(arvore);
  bspline_topologia_liberar(&spline);
  geometria_curvas_free(&g);
  free(boxes.dados);
  pontos_free(&scratch);
  pontos_free(&controles);
  puts("test_geometria_curvas: OK");
  return 0;
}
