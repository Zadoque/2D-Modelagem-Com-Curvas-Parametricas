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

static float area_dupla(const Pontos *pontos){
  float area = 0.0f;
  for(int i = 0; i < pontos->quantidade_atual; i++){
    const ponto atual = pontos->data[i];
    const ponto proximo = pontos->data[(i + 1) % pontos->quantidade_atual];
    area += atual.point[0] * proximo.point[1] - atual.point[1] * proximo.point[0];
  }
  return area;
}

static void verificar_shear(void){
  float entrada[3] = {2, 3, 1}, saida[3];
  matriz3_multiplicar_ponto(entrada, matriz3_cisalhamento_x(4), saida);
  perto(saida[0], 14);
  perto(saida[1], 3);
  matriz3_multiplicar_ponto(entrada, matriz3_cisalhamento_y(-2), saida);
  perto(saida[0], 2);
  perto(saida[1], -1);

  Pontos v;
  pontos_init(&v, 3);
  pontos_push(&v, p(-2, -1));
  pontos_push(&v, p(2, 3));
  pontos_push(&v, p(3, -2));
  ponto c = p(1, 0);
  ponto alvo = p(5, 4);
  cisalhar_com_mouse(&v, &c, alvo, 1);
  perto(v.data[1].point[0], alvo.point[0]);
  perto(v.data[1].point[1], 3);
  perto(v.data[0].point[0], -3);
  perto(v.data[0].point[1], -1);
  perto(v.data[2].point[0], 1);
  perto(v.data[2].point[1], -2);
  perto(v.data[0].point[2], 1);
  perto(v.data[1].point[2], 1);
  perto(v.data[2].point[2], 1);

  Pontos origem, repetido;
  pontos_init(&origem, 3);
  pontos_init(&repetido, 3);
  pontos_push(&origem, p(-2, -1));
  pontos_push(&origem, p(2, 3));
  pontos_push(&origem, p(3, -2));
  float area_origem = area_dupla(&origem);
  for(int passo = 0; passo < 200; passo++){
    for(int i = 0; i < origem.quantidade_atual; i++){
      repetido.data[i] = origem.data[i];
    }
    repetido.quantidade_atual = origem.quantidade_atual;
    ponto destino = p(2.0f + (float)(passo % 7), 3.0f);
    cisalhar_com_mouse(&repetido, &c, destino, 1);
    perto(repetido.data[1].point[0], destino.point[0]);
    perto(repetido.data[1].point[1], 3.0f);
    perto(area_dupla(&repetido), area_origem);
  }
  pontos_free(&repetido);
  pontos_free(&origem);

  ponto antes = v.data[0];
  cisalhar_com_mouse(&v, &c, alvo, -1);
  perto(v.data[0].point[0], antes.point[0]);
  perto(v.data[0].point[1], antes.point[1]);
  pontos_free(&v);
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
  verificar_shear();
  puts("test_transformacoes: OK");
  return 0;
}
