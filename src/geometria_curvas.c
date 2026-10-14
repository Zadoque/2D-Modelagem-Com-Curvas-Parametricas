#define _POSIX_C_SOURCE 200809L
#include "../includes/geometria_curvas.h"
#include "../includes/curvas/curva_generica.h"
#include "../includes/curvas/bezier.h"
#include "../includes/curvas/bspline.h"
#include "../includes/curvas/catmullrom.h"
#include "../includes/curvas/hermite.h"
#include <stdlib.h>
#include <time.h>

typedef void (*Gerador)(ponto, ponto, ponto, ponto, Pontos *, AABB *);

void geometria_curvas_init(GeometriaCurvas *g){
  pontos_init(&g->vertices, 100);
  g->faixas = NULL;
  g->quantidade_faixas = 0;
  g->capacidade_faixas = 0;
}
void geometria_curvas_free(GeometriaCurvas *g){
  pontos_free(&g->vertices);
  free(g->faixas);
  g->faixas = NULL;
  g->quantidade_faixas = g->capacidade_faixas = 0;
}

static int reservar_faixa(GeometriaCurvas *g){
  if(g->quantidade_faixas < g->capacidade_faixas){
    return 0;
  }
  int capacidade = g->capacidade_faixas ? g->capacidade_faixas * 2 : 16;
  FaixaCurva *novo = realloc(g->faixas, (size_t)capacidade * sizeof(*novo));
  if(!novo){
    return -1;
  }
  g->faixas = novo;
  g->capacidade_faixas = capacidade;
  return 0;
}
static int adicionar(GeometriaCurvas *g, Pontos *scratch, AABB_vec *boxes, const Pontos *c,
                     int inicio, Gerador gerar){
  int n = c->quantidade_atual;
  AABB box = {0};
  box.segmento_indice = inicio;
  gerar(c->data[inicio % n], c->data[(inicio + 1) % n], c->data[(inicio + 2) % n],
        c->data[(inicio + 3) % n], scratch, &box);
  if(reservar_faixa(g) || aabb_vec_push(boxes, box)){
    return -1;
  }
  FaixaCurva faixa = {g->vertices.quantidade_atual, scratch->quantidade_atual};
  for(int i = 0; i < scratch->quantidade_atual; i++){
    if(pontos_push(&g->vertices, scratch->data[i])){
      return -1;
    }
  }
  g->faixas[g->quantidade_faixas++] = faixa;
  return 0;
}
int geometria_curvas_atualizar(GeometriaCurvas *g, Pontos *scratch, AABB_vec *boxes,
                               const Pontos *c, Curva tipo, int fechado){
  struct timespec inicio, fim;
  clock_gettime(CLOCK_MONOTONIC, &inicio);
  curva_subdivisao_iniciar_medicao();
  g->vertices.quantidade_atual = 0;
  g->quantidade_faixas = 0;
  boxes->quantidade = 0;
  int n = c->quantidade_atual;
  if(n < 4){
    return 0;
  }
  Gerador gerar;
  int passo = 1;
  switch(tipo){
  case MODO_CURVA_HERMITE:
    gerar = gerar_curva_hermite;
    break;
  case MODO_CURVA_CATMULLROM:
    gerar = gerar_curva_catmullrom;
    break;
  case MODO_CURVA_BSPLINE:
    gerar = gerar_curva_bspline;
    break;
  default:
    gerar = gerar_curva_bezier;
    passo = 3;
    break;
  }
  for(int i = 0; i <= n - 4; i += passo){
    if(adicionar(g, scratch, boxes, c, i, gerar)){
      return -1;
    }
  }
  if(!fechado){
    goto concluir;
  }
  if(tipo == MODO_CURVA_BEZIER){
    if(n % 3 == 0 && adicionar(g, scratch, boxes, c, n - 3, gerar)){
      return -1;
    }
  } else {
    for(int i = n - 3; i < n; i++){
      if(adicionar(g, scratch, boxes, c, i, gerar)){
        return -1;
      }
    }
  }
concluir:
  clock_gettime(CLOCK_MONOTONIC, &fim);
  double ms = (fim.tv_sec - inicio.tv_sec) * 1000.0 + (fim.tv_nsec - inicio.tv_nsec) / 1000000.0;
  curva_subdivisao_ajustar_orcamento(ms, (size_t)g->quantidade_faixas);
  return 0;
}
