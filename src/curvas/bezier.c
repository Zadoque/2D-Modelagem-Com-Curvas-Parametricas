#include "curvas/bezier.h"
#include "curvas/curva_generica.h"
#include <math.h>

#define FATOR_RIGIDEZ 8.0f

ponto calcular_ponto_bezier(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  float u = 1 - t, w[4] = {u * u * u, 3 * t * u * u, 3 * t * t * u, t * t * t};
  ponto r = {{0}};
  ponto p[4] = {p0, p1, p2, p3};
  for(int i = 0; i < 4; i++){
    for(int e = 0; e < 2; e++){
      r.point[e] += w[i] * p[i].point[e];
    }
  }
  return r;
}
static ponto derivar(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  float u = 1 - t;
  ponto r = {{0}};
  for(int e = 0; e < 2; e++){
    r.point[e] = 3 * u * u * (p1.point[e] - p0.point[e]) + 6 * u * t * (p2.point[e] - p1.point[e]) +
                 3 * t * t * (p3.point[e] - p2.point[e]);
  }
  return r;
}
static void pesos(float t, float w[4]){
  float u = 1 - t;
  w[0] = u * u * u;
  w[1] = 3 * t * u * u;
  w[2] = 3 * t * t * u;
  w[3] = t * t * t;
}
static const CurvaCubicaOps OPS = {calcular_ponto_bezier, derivar, pesos};
void gerar_curva_bezier(ponto p0, ponto p1, ponto p2, ponto p3, Pontos *c, AABB *b, ponto *final){
  gerar_segmento_curva_cubica(&OPS, p0, p1, p2, p3, c, b, final);
}
ResultadoPicking picking_bezier(AABBTREE *a, Pontos *c, ponto m, float t, float d){
  return picking_curva_cubica(&OPS, a, c, m, t, d);
}
void arrastar_ponto_bezier(Pontos *c, int s, float t, ponto m, float rigidez){
  if(!isfinite(rigidez)){
    return;
  }
  rigidez = fmaxf(0.0f, fminf(1.0f, rigidez));
  float penalizacoes[4];
  for(int i = 0; i < 4; i++){
    penalizacoes[i] = 1.0f + FATOR_RIGIDEZ * rigidez * fabsf((float)i - 3.0f * t);
  }
  arrastar_segmento_curva_cubica_ponderado(&OPS, c, s, t, m, penalizacoes);
}
