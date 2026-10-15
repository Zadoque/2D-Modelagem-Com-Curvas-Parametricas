#define _POSIX_C_SOURCE 200809L
#include "../includes/geometria_curvas.h"
#include "../includes/curvas/curva_generica.h"
#include "../includes/curvas/bezier.h"
#include "../includes/curvas/bspline.h"
#include "../includes/curvas/catmullrom.h"
#include "../includes/curvas/hermite.h"
#include <stdlib.h>
#include <time.h>
#include <float.h>

typedef void (*Gerador)(ponto, ponto, ponto, ponto, Pontos *, AABB *, ponto *);

void geometria_curvas_init(GeometriaCurvas *g){
  pontos_init(&g->vertices, 100);
  g->faixas = NULL;
  g->quantidade_faixas = 0;
  g->capacidade_faixas = 0;
  g->fechado = 0;
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
                     int inicio, Gerador gerar, ponto *ponto_final){
  int n = c->quantidade_atual;
  AABB box = {0};
  box.segmento_indice = inicio;
  gerar(c->data[inicio % n], c->data[(inicio + 1) % n], c->data[(inicio + 2) % n],
        c->data[(inicio + 3) % n], scratch, &box, ponto_final);
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

static int amostrar_span_bspline(GeometriaCurvas *g, const BSplineTopologia *spline, ponto inicio,
                                 ponto fim, float u_inicio, float u_fim, unsigned profundidade){
  float u_meio = u_inicio + (u_fim - u_inicio) * 0.5f;
  ponto meio = bspline_topologia_avaliar(spline, u_meio);
  if(profundidade >= 10 || calcula_dist_ponto_segmento(meio, inicio, fim) <= 0.1f){
    return pontos_push(&g->vertices, fim);
  }
  if(amostrar_span_bspline(g, spline, inicio, meio, u_inicio, u_meio, profundidade + 1) != 0){
    return -1;
  }
  return amostrar_span_bspline(g, spline, meio, fim, u_meio, u_fim, profundidade + 1);
}
int geometria_curvas_atualizar(GeometriaCurvas *g, Pontos *scratch, AABB_vec *boxes,
                               const Pontos *c, Curva tipo, int fechado){
  struct timespec inicio, fim;
  clock_gettime(CLOCK_MONOTONIC, &inicio);
  curva_subdivisao_iniciar_medicao();
  g->vertices.quantidade_atual = 0;
  g->quantidade_faixas = 0;
  g->fechado = fechado;
  boxes->quantidade = 0;
  int n = c->quantidade_atual;
  if(n < 4){
    return 0;
  }
  Gerador gerar;
  ponto ponto_final = {{0}};
  int possui_segmento = 0;
  int fechamento_bezier_incompleto;
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
    if(adicionar(g, scratch, boxes, c, i, gerar, &ponto_final)){
      return -1;
    }
    possui_segmento = 1;
  }
  if(!fechado){
    goto concluir;
  }
  if(tipo == MODO_CURVA_BEZIER){
    if(n % 3 == 0 && adicionar(g, scratch, boxes, c, n - 3, gerar, &ponto_final)){
      return -1;
    }
    possui_segmento = n % 3 == 0 || possui_segmento;
  } else {
    for(int i = n - 3; i < n; i++){
      if(adicionar(g, scratch, boxes, c, i, gerar, &ponto_final)){
        return -1;
      }
      possui_segmento = 1;
    }
  }
concluir:
  fechamento_bezier_incompleto = fechado && tipo == MODO_CURVA_BEZIER && n % 3 != 0;
  if(possui_segmento && (!fechado || fechamento_bezier_incompleto)){
    if(pontos_push(&g->vertices, ponto_final)){
      return -1;
    }
    g->faixas[g->quantidade_faixas - 1].quantidade++;
  }
  clock_gettime(CLOCK_MONOTONIC, &fim);
  double ms = (fim.tv_sec - inicio.tv_sec) * 1000.0 + (fim.tv_nsec - inicio.tv_nsec) / 1000000.0;
  curva_subdivisao_ajustar_orcamento(ms, (size_t)g->quantidade_faixas);
  return 0;
}

int geometria_curvas_atualizar_bspline_topologica(GeometriaCurvas *g, AABB_vec *boxes,
                                                   const BSplineTopologia *spline){
  if(!g || !boxes || !spline || spline->grau != 3 || spline->controles.quantidade_atual < 4){
    return -1;
  }
  g->vertices.quantidade_atual = 0;
  g->quantidade_faixas = 0;
  g->fechado = spline->fechado;
  boxes->quantidade = 0;
  for(int ordinal = 0;; ordinal++){
    float inicio, fim;
    int indice_no;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, &indice_no) != 0){
      break;
    }
    AABB box = {0};
    box.x_min = box.y_min = FLT_MAX;
    box.x_max = box.y_max = -FLT_MAX;
    box.segmento_indice = ordinal;
    for(int i = indice_no - (int)spline->grau; i <= indice_no; i++){
      int indice_controle = i;
      if(spline->fechado){
        indice_controle %= spline->controles.quantidade_atual;
        if(indice_controle < 0){ indice_controle += spline->controles.quantidade_atual; }
      }
      ponto controle = spline->controles.data[indice_controle];
      if(controle.point[0] < box.x_min){ box.x_min = controle.point[0]; }
      if(controle.point[0] > box.x_max){ box.x_max = controle.point[0]; }
      if(controle.point[1] < box.y_min){ box.y_min = controle.point[1]; }
      if(controle.point[1] > box.y_max){ box.y_max = controle.point[1]; }
    }
    box.esq_x_min = box.dir_x_min = box.x_min;
    box.esq_x_max = box.dir_x_max = box.x_max;
    box.esq_y_min = box.dir_y_min = box.y_min;
    box.esq_y_max = box.dir_y_max = box.y_max;
    if(reservar_faixa(g) || aabb_vec_push(boxes, box)){
      return -1;
    }
    FaixaCurva faixa = {g->vertices.quantidade_atual, 0};
    ponto ponto_inicio = bspline_topologia_avaliar(spline, inicio);
    ponto ponto_fim = bspline_topologia_avaliar(spline, fim);
    if(ordinal == 0 && pontos_push(&g->vertices, ponto_inicio) != 0){
      return -1;
    }
    if(amostrar_span_bspline(g, spline, ponto_inicio, ponto_fim, inicio, fim, 0) != 0){
      return -1;
    }
    faixa.quantidade = g->vertices.quantidade_atual - faixa.inicio;
    g->faixas[g->quantidade_faixas++] = faixa;
  }
  return 0;
}
