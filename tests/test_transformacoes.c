#include "transformacoes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ponto p(float x, float y){
  ponto r = {{x, y, 1}};
  return r;
}
static void perto(float a, float b){
  assert(fabsf(a - b) < 1e-4f);
}

int main(void){
  Matriz3 t = matriz3_translacao(2, 3), s = matriz3_escala(2, 4);
  Matriz3 composta = matriz3_multiplicar(s, t);
  float entrada[3] = {1, 2, 1}, saida[3];
  matriz3_multiplicar_ponto(entrada, composta, saida);
  perto(saida[0], 4);
  perto(saida[1], 11);
  Pontos vazia;
  pontos_init(&vazia, 0);
  ponto centro_vazio = {{9, 9, 0}};
  calcular_centro_medio(&centro_vazio, &vazia);
  perto(centro_vazio.point[0], 0);
  perto(centro_vazio.point[1], 0);
  perto(centro_vazio.point[2], 1);
  pontos_free(&vazia);
  Pontos v;
  pontos_init(&v, 2);
  pontos_push(&v, p(1, 0));
  pontos_push(&v, p(3, 0));
  ponto c;
  calcular_centro_medio(&c, &v);
  perto(c.point[0], 2);
  perto(c.point[1], 0);
  translacao_com_mouse(&v, c, 5, 7);
  perto(v.data[0].point[0], 4);
  perto(v.data[1].point[0], 6);
  perto(v.data[0].point[1], 7);
  perto(v.data[1].point[1], 7);
  calcular_centro_medio(&c, &v);
  rotacionar_com_mouse(&v, &c, p(5, 8), 1);
  perto(v.data[0].point[0], 5);
  perto(v.data[0].point[1], 6);
  perto(v.data[1].point[0], 5);
  perto(v.data[1].point[1], 8);
  aumentar_escala(&v, c);
  perto(calcula_distancia(v.data[0], c), 1.05f);
  diminuir_escala(&v, c);
  perto(calcula_distancia(v.data[0], c), 0.9975f);
  pontos_free(&v);
  puts("test_transformacoes: OK");
  return 0;
}
