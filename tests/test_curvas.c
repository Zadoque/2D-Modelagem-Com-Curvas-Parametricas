#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
#include "curvas/hermite.h"
#include "curvas/curva_generica.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

typedef ponto (*Avaliador)(ponto, ponto, ponto, ponto, float);
typedef void (*Gerador)(ponto, ponto, ponto, ponto, Pontos *, AABB *, ponto *);

static ponto avaliar_hermite_local(ponto P0, ponto P1, ponto P2, ponto P3, float t){
  ponto T1, T2;
  calcular_tangentes_hermite(P0, P1, P2, P3, &T1, &T2);
  return calcular_ponto_hermite(P1, P2, T1, T2, t);
}

static int dentro(float valor, float minimo, float maximo){
  return valor >= minimo && valor <= maximo;
}

static void verificar_caixa(const char *nome, Gerador gerar, Avaliador avaliar, ponto P0, ponto P1,
                            ponto P2, ponto P3){
  Pontos curva;
  AABB box = {0};
  ponto ponto_final;
  pontos_init(&curva, 8);
  gerar(P0, P1, P2, P3, &curva, &box, &ponto_final);

  for(int i = 0; i <= 10000; i++){
    float t = (float)i / 10000.0f;
    ponto p = avaliar(P0, P1, P2, P3, t);
    int contido = t <= 0.5f ? dentro(p.point[0], box.esq_x_min, box.esq_x_max) &&
                                  dentro(p.point[1], box.esq_y_min, box.esq_y_max)
                            : dentro(p.point[0], box.dir_x_min, box.dir_x_max) &&
                                  dentro(p.point[1], box.dir_y_min, box.dir_y_max);
    if(!contido){
      fprintf(stderr, "%s fora da AABB em t=%f: (%f, %f)\n", nome, t, p.point[0], p.point[1]);
    }
    assert(contido);
    assert(dentro(p.point[0], box.x_min, box.x_max));
    assert(dentro(p.point[1], box.y_min, box.y_max));
  }

  pontos_free(&curva);
}

static ponto p(float x, float y){
  ponto resultado = {{x, y, 1.0f}};
  return resultado;
}

static void copiar_controles(Pontos *destino, const Pontos *origem){
  pontos_init(destino, (size_t)origem->quantidade_atual);
  for(int i = 0; i < origem->quantidade_atual; i++){
    assert(pontos_push(destino, origem->data[i]) == 0);
  }
}

static float deslocamento(ponto antes, ponto depois){
  return calcula_distancia(antes, depois);
}

static void verificar_ponto_arrastado(const Pontos *controles, int segmento, float t){
  Pontos deformados;
  copiar_controles(&deformados, controles);
  int n = controles->quantidade_atual;
  ponto antes = calcular_ponto_bspline(controles->data[segmento % n],
                                       controles->data[(segmento + 1) % n],
                                       controles->data[(segmento + 2) % n],
                                       controles->data[(segmento + 3) % n], t);
  ponto mouse = p(antes.point[0] + 19.0f, antes.point[1] - 13.0f);
  arrastar_ponto_bspline(&deformados, segmento, t, mouse, 1.0f);
  ponto depois = calcular_ponto_bspline(deformados.data[segmento % n],
                                        deformados.data[(segmento + 1) % n],
                                        deformados.data[(segmento + 2) % n],
                                        deformados.data[(segmento + 3) % n], t);
  assert(calcula_distancia(depois, mouse) < 1e-4f);
  pontos_free(&deformados);
}

static void verificar_arrasto_bspline(void){
  Pontos original;
  pontos_init(&original, 5);
  assert(pontos_push(&original, p(-40.0f, 10.0f)) == 0);
  assert(pontos_push(&original, p(0.0f, 70.0f)) == 0);
  assert(pontos_push(&original, p(90.0f, -30.0f)) == 0);
  assert(pontos_push(&original, p(180.0f, 45.0f)) == 0);
  assert(pontos_push(&original, p(250.0f, -20.0f)) == 0);

  const int segmento = 4;
  const float t = 0.35f;
  ponto antes = calcular_ponto_bspline(original.data[4], original.data[0], original.data[1],
                                       original.data[2], t);
  ponto mouse = p(antes.point[0] + 37.0f, antes.point[1] - 23.0f);

  Pontos classico, sem_rigidez, rigido;
  copiar_controles(&classico, &original);
  copiar_controles(&sem_rigidez, &original);
  copiar_controles(&rigido, &original);
  arrastar_ponto_bspline(&classico, segmento, t, mouse, 0.0f);
  arrastar_ponto_bspline(&sem_rigidez, segmento, t, mouse, -1.0f);
  arrastar_ponto_bspline(&rigido, segmento, t, mouse, 1.0f);

  for(int i = 0; i < original.quantidade_atual; i++){
    assert(fabsf(classico.data[i].point[0] - sem_rigidez.data[i].point[0]) < 1e-6f);
    assert(fabsf(classico.data[i].point[1] - sem_rigidez.data[i].point[1]) < 1e-6f);
  }

  ponto depois = calcular_ponto_bspline(rigido.data[4], rigido.data[0], rigido.data[1],
                                        rigido.data[2], t);
  assert(calcula_distancia(depois, mouse) < 1e-4f);
  assert(deslocamento(original.data[0], rigido.data[0]) > deslocamento(original.data[4], rigido.data[4]));
  assert(deslocamento(original.data[1], rigido.data[1]) > deslocamento(original.data[4], rigido.data[4]));
  assert(deslocamento(original.data[3], rigido.data[3]) == 0.0f);

  Pontos entrada_invalida;
  copiar_controles(&entrada_invalida, &original);
  arrastar_ponto_bspline(&entrada_invalida, segmento, t, mouse, NAN);
  for(int i = 0; i < original.quantidade_atual; i++){
    assert(entrada_invalida.data[i].point[0] == original.data[i].point[0]);
    assert(entrada_invalida.data[i].point[1] == original.data[i].point[1]);
  }
  pontos_free(&entrada_invalida);

  verificar_ponto_arrastado(&original, segmento, 0.0f);
  verificar_ponto_arrastado(&original, segmento, 0.5f);
  verificar_ponto_arrastado(&original, segmento, 1.0f);

  pontos_free(&rigido);
  pontos_free(&sem_rigidez);
  pontos_free(&classico);
  pontos_free(&original);
}

static void verificar_rigidez_outras_curvas(void){
  Pontos original, livre, rigido;
  pontos_init(&original, 4);
  for(int i = 0; i < 4; i++){
    assert(pontos_push(&original, p((float)i * 30.0f, (float)(i % 2) * 40.0f)) == 0);
  }
  copiar_controles(&livre, &original);
  copiar_controles(&rigido, &original);
  ponto alvo = calcular_ponto_bezier(original.data[0], original.data[1], original.data[2],
                                     original.data[3], 0.2f);
  alvo.point[0] += 20.0f;
  arrastar_ponto_bezier(&livre, 0, 0.2f, alvo, 0.0f);
  arrastar_ponto_bezier(&rigido, 0, 0.2f, alvo, 1.0f);
  assert(calcula_distancia(calcular_ponto_bezier(rigido.data[0], rigido.data[1], rigido.data[2],
                                                 rigido.data[3], 0.2f), alvo) < 1e-4f);
  assert(deslocamento(original.data[3], rigido.data[3]) < deslocamento(original.data[3], livre.data[3]));

  copiar_controles(&livre, &original);
  copiar_controles(&rigido, &original);
  alvo = calcular_ponto_catmullrom(original.data[0], original.data[1], original.data[2],
                                   original.data[3], 0.3f);
  alvo.point[1] -= 20.0f;
  arrastar_ponto_catmullrom(&livre, 0, 0.3f, alvo, 0.0f);
  arrastar_ponto_catmullrom(&rigido, 0, 0.3f, alvo, 1.0f);
  assert(deslocamento(original.data[0], rigido.data[0]) < deslocamento(original.data[0], livre.data[0]));

  copiar_controles(&livre, &original);
  copiar_controles(&rigido, &original);
  alvo = avaliar_hermite_local(original.data[0], original.data[1], original.data[2], original.data[3],
                                0.5f);
  alvo.point[0] += 20.0f;
  arrastar_ponto_hermite(&livre, 0, 0.5f, alvo, 0.0f);
  arrastar_ponto_hermite(&rigido, 0, 0.5f, alvo, 1.0f);
  assert(deslocamento(original.data[0], rigido.data[0]) < deslocamento(original.data[0], livre.data[0]));
  pontos_free(&rigido);
  pontos_free(&livre);
  pontos_free(&original);
}

int main(void){
  ponto P0 = {{-300.0f, 180.0f, 1.0f}};
  ponto P1 = {{-40.0f, -260.0f, 1.0f}};
  ponto P2 = {{170.0f, 330.0f, 1.0f}};
  ponto P3 = {{420.0f, -140.0f, 1.0f}};

  verificar_caixa("Bezier", gerar_curva_bezier, calcular_ponto_bezier, P0, P1, P2, P3);
  verificar_caixa("B-Spline", gerar_curva_bspline, calcular_ponto_bspline, P0, P1, P2, P3);
  verificar_caixa("Catmull-Rom", gerar_curva_catmullrom, calcular_ponto_catmullrom, P0, P1, P2, P3);
  verificar_caixa("Hermite", gerar_curva_hermite, avaliar_hermite_local, P0, P1, P2, P3);

  Pontos hermite;
  AABB box = {0};
  pontos_init(&hermite, 8);
  ponto ponto_final;
  gerar_curva_hermite(P0, P1, P2, P3, &hermite, &box, &ponto_final);
  assert(fabsf(hermite.data[0].point[0] - P1.point[0]) < 1e-5f);
  assert(fabsf(hermite.data[0].point[1] - P1.point[1]) < 1e-5f);
  assert(fabsf(ponto_final.point[0] - P2.point[0]) < 1e-5f);
  assert(fabsf(ponto_final.point[1] - P2.point[1]) < 1e-5f);
  assert(fabsf(hermite.data[hermite.quantidade_atual - 1].point[0] - P2.point[0]) > 1e-5f ||
         fabsf(hermite.data[hermite.quantidade_atual - 1].point[1] - P2.point[1]) > 1e-5f);
  pontos_free(&hermite);

  CurvaSubdivisaoConfig limitada = curva_subdivisao_config_padrao();
  limitada.tolerancia_geometrica = 1e-12f;
  limitada.profundidade_maxima = 1;
  limitada.max_nos_geracao = 1;
  curva_subdivisao_definir_config(limitada);
  pontos_init(&hermite, 8);
  gerar_curva_bezier(P0, P1, P2, P3, &hermite, &box, &ponto_final);
  assert(hermite.quantidade_atual <= 7);
  pontos_free(&hermite);

  limitada.profundidade_maxima = 100;
  curva_subdivisao_definir_config(limitada);
  assert(curva_subdivisao_obter_config().profundidade_maxima == 20);
  curva_subdivisao_definir_config(curva_subdivisao_config_padrao());

  verificar_arrasto_bspline();
  verificar_rigidez_outras_curvas();

  puts("test_curvas: OK");
  return 0;
}
