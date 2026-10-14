#include "pontos.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ponto p(float x, float y){
  ponto r = {{x, y, 1.0f}};
  return r;
}
static void perto(float atual, float esperado){
  assert(fabsf(atual - esperado) < 1e-5f);
}

int main(void){
  perto(calcula_dist_ponto_segmento(p(5, 3), p(0, 0), p(10, 0)), 3.0f);
  perto(calcula_dist_ponto_segmento(p(-2, 4), p(0, 0), p(10, 0)), sqrtf(20.0f));
  perto(calcula_dist_ponto_segmento(p(13, 4), p(0, 0), p(10, 0)), 5.0f);
  perto(calcula_dist_ponto_segmento(p(3, 4), p(1, 2), p(1, 2)), sqrtf(8.0f));
  perto(calcula_dist_ponto_segmento(p(2, 2), p(0, 0), p(4, 4)), 0.0f);
  perto(calcula_dist_ponto_segmento(p(3, 1), p(0, 0), p(4, 4)), sqrtf(2.0f));
  perto(calcula_dist_ponto_segmento(p(3, 1), p(4, 4), p(0, 0)), sqrtf(2.0f));
  puts("test_distancia: OK");
  return 0;
}
