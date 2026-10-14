#include "../../includes/curvas/hermite.h"
#include "../../includes/curvas/curva_generica.h"

void calcular_tangentes_hermite(ponto p0, ponto p1, ponto p2, ponto p3, ponto *t1, ponto *t2){
  for(int e = 0; e < 2; e++){
    t1->point[e] = p2.point[e] - p0.point[e];
    t2->point[e] = p3.point[e] - p1.point[e];
  }
}
ponto calcular_ponto_hermite(ponto p1, ponto p2, ponto t1, ponto t2, float t){
  float q = t * t, c = q * t, w[4] = {2 * c - 3 * q + 1, -2 * c + 3 * q, c - 2 * q + t, c - q};
  ponto p[4] = {p1, p2, t1, t2}, r = {{0}};
  for(int i = 0; i < 4; i++){
    for(int e = 0; e < 2; e++){
      r.point[e] += w[i] * p[i].point[e];
    }
  }
  return r;
}
static ponto avaliar(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  ponto t1 = {{0}}, t2 = {{0}};
  calcular_tangentes_hermite(p0, p1, p2, p3, &t1, &t2);
  return calcular_ponto_hermite(p1, p2, t1, t2, t);
}
static void pesos(float t, float w[4]){
  float q = t * t, c = q * t, h00 = 2 * c - 3 * q + 1, h01 = -2 * c + 3 * q, h10 = c - 2 * q + t,
        h11 = c - q;
  w[0] = -h10;
  w[1] = h00 - h11;
  w[2] = h01 + h10;
  w[3] = h11;
}
static ponto derivar(ponto p0, ponto p1, ponto p2, ponto p3, float t){
  float w[4] = {-3 * t * t + 4 * t - 1, 3 * t * t - 4 * t + 1, -3 * t * t + 2 * t,
                3 * t * t - 2 * t};
  ponto p[4] = {p0, p1, p2, p3}, r = {{0}};
  for(int i = 0; i < 4; i++){
    for(int e = 0; e < 2; e++){
      r.point[e] += w[i] * p[i].point[e];
    }
  }
  return r;
}
static const CurvaCubicaOps OPS = {avaliar, derivar, pesos};
void gerar_curva_hermite(ponto p0, ponto p1, ponto p2, ponto p3, Pontos *c, AABB *b){
  gerar_segmento_curva_cubica(&OPS, p0, p1, p2, p3, c, b);
}
ResultadoPicking picking_hermite(AABBTREE *a, Pontos *c, ponto m, float t, float d){
  return picking_curva_cubica(&OPS, a, c, m, t, d);
}
void arrastar_ponto_hermite(Pontos *c, int s, float t, ponto m){
  arrastar_segmento_curva_cubica(&OPS, c, s, t, m);
}
