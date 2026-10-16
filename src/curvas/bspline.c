#include "../../includes/curvas/bspline.h"
#include "../../includes/curvas/curva_generica.h"
#include <math.h>

#define BSPLINE_FATOR_RIGIDEZ 8.0f

static void pesos(float t, float w[4]){
  float t2 = t * t, t3 = t2 * t;
  w[0] = (-t3 + 3 * t2 - 3 * t + 1) / 6;
  w[1] = (3 * t3 - 6 * t2 + 4) / 6;
  w[2] = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6;
  w[3] = t3 / 6;
}
ponto calcular_ponto_bspline(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  float w[4];
  pesos(t, w);
  ponto p[4] = {p0, p1, p2, p3}, r = {{0}};
  for(int i = 0; i < 4; i++){
    for(int e = 0; e < 2; e++){
      r.point[e] += w[i] * p[i].point[e];
    }
  }
  return r;
}
static ponto derivar(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  float t2 = t * t, w[4] = {(-3 * t2 + 6 * t - 3) / 6, (9 * t2 - 12 * t) / 6,
                            (-9 * t2 + 6 * t + 3) / 6, (3 * t2) / 6};
  ponto p[4] = {p0, p1, p2, p3}, r = {{0}};
  for(int i = 0; i < 4; i++){
    for(int e = 0; e < 2; e++){
      r.point[e] += w[i] * p[i].point[e];
    }
  }
  return r;
}
static const CurvaCubicaOps OPS = {calcular_ponto_bspline, derivar, pesos};
void gerar_curva_bspline(ponto p0, ponto p1, ponto p2, ponto p3, Pontos *c, AABB *b, ponto *final){
  gerar_segmento_curva_cubica(&OPS, p0, p1, p2, p3, c, b, final);
}
ResultadoPicking picking_bspline(AABBTREE *a, Pontos *c, ponto m, float t, float d){
  return picking_curva_cubica(&OPS, a, c, m, t, d);
}
void arrastar_ponto_bspline(Pontos *c, int s, float t, ponto m, float rigidez){
  if(!isfinite(rigidez)){
    return;
  }
  if(rigidez <= 0.0f){
    arrastar_segmento_curva_cubica(&OPS, c, s, t, m);
    return;
  }
  rigidez = fminf(rigidez, 1.0f);
  t = fmaxf(0.0f, fminf(1.0f, t));
  float pesos_penalizacao[4];
  for(int i = 0; i < 4; i++){
    float distancia = fabsf((float)i - (1.0f + t));
    pesos_penalizacao[i] = 1.0f + BSPLINE_FATOR_RIGIDEZ * rigidez * distancia;
  }
  arrastar_segmento_curva_cubica_ponderado(&OPS, c, s, t, m, pesos_penalizacao);
}
