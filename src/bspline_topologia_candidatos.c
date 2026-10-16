#define _POSIX_C_SOURCE 199309L

#include "bspline_topologia.h"

#include <math.h>
#include <stdlib.h>

static int modulo_indice_candidato(int indice, int quantidade){
  if(quantidade <= 0){ return 0; }
  int resto = indice % quantidade;
  return resto < 0 ? resto + quantidade : resto;
}

static int adicionar_candidato(float *nos, int quantidade, int capacidade, float candidato){
  for(int i = 0; i < quantidade; i++){
    if(fabsf(nos[i] - candidato) <= 1e-6f){ return quantidade; }
  }
  if(quantidade >= capacidade){ return -1; }
  nos[quantidade] = candidato;
  return quantidade + 1;
}

static int limites_locais(const BSplineTopologia *spline, int central, int *primeiro,
                          int *ultimo){
  float inicio, fim;
  int quantidade = 0;
  while(bspline_topologia_obter_span(spline, quantidade, &inicio, &fim, NULL) == 0){
    quantidade++;
  }
  if(central < 0 || central >= quantidade){ return -1; }
  *primeiro = central - 3;
  *ultimo = central + 3;
  if(!spline->fechado){
    if(*primeiro < 0){ *primeiro = 0; }
    if(*ultimo >= quantidade){ *ultimo = quantidade - 1; }
  }
  return quantidade;
}

int bspline_topologia_candidatos_insercao(const BSplineTopologia *spline, int span_central,
                                          float limiar, float *nos, int capacidade){
  if(!spline || !nos || capacidade < 1 || !isfinite(limiar) || limiar < 0.0f){ return -1; }
  int primeiro, ultimo;
  int quantidade_spans = limites_locais(spline, span_central, &primeiro, &ultimo);
  if(quantidade_spans < 0){ return -1; }
  int quantidade = 0;
  for(int bruto = primeiro; bruto <= ultimo; bruto++){
    int ordinal = spline->fechado ? (bruto + quantidade_spans) % quantidade_spans : bruto;
    float inicio, fim;
    int indice_no;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, &indice_no) != 0){ continue; }
    for(int i = indice_no - (int)spline->grau; i < indice_no; i++){
      int esquerda = modulo_indice_candidato(i, spline->controles.quantidade_atual);
      int direita = modulo_indice_candidato(i + 1, spline->controles.quantidade_atual);
      if(calcula_distancia(spline->controles.data[esquerda], spline->controles.data[direita]) >
         limiar){
        quantidade = adicionar_candidato(nos, quantidade, capacidade, (inicio + fim) * 0.5f);
        if(quantidade < 0){ return -1; }
        break;
      }
    }
  }
  return quantidade;
}

int bspline_topologia_candidatos_remocao(const BSplineTopologia *spline, int span_central,
                                         float limiar, float *nos, int capacidade){
  if(!spline || !nos || capacidade < 1 || !isfinite(limiar) || limiar < 0.0f){ return -1; }
  int primeiro, ultimo;
  int quantidade_spans = limites_locais(spline, span_central, &primeiro, &ultimo);
  if(quantidade_spans < 0){ return -1; }
  int quantidade = 0;
  for(int bruto = primeiro; bruto <= ultimo; bruto++){
    int ordinal = spline->fechado ? (bruto + quantidade_spans) % quantidade_spans : bruto;
    float inicio, fim;
    int indice_no;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, &indice_no) != 0){ continue; }
    float maior_distancia = 0.0f;
    for(int i = indice_no - (int)spline->grau; i < indice_no; i++){
      int esquerda = modulo_indice_candidato(i, spline->controles.quantidade_atual);
      int direita = modulo_indice_candidato(i + 1, spline->controles.quantidade_atual);
      float distancia = calcula_distancia(spline->controles.data[esquerda],
                                           spline->controles.data[direita]);
      if(distancia > maior_distancia){ maior_distancia = distancia; }
    }
    if((spline->fechado || inicio > spline->nos[spline->grau]) && maior_distancia < limiar){
      quantidade = adicionar_candidato(nos, quantidade, capacidade, inicio);
      if(quantidade < 0){ return -1; }
    }
    (void)fim;
  }
  return quantidade;
}

static int candidatos_por_distancias(const BSplineTopologia *spline, float limiar,
                                     float *nos, int capacidade, int remover){
  if(!spline || !nos || capacidade < 1 || !isfinite(limiar) || limiar < 0.0f ||
     spline->controles.quantidade_atual < (int)spline->grau + 1){ return -1; }
  int quantidade_controles = spline->controles.quantidade_atual;
  int quantidade_arestas = spline->fechado ? quantidade_controles : quantidade_controles - 1;
  float *distancias = malloc((size_t)quantidade_arestas * sizeof(*distancias));
  if(!distancias){ return -1; }
  for(int i = 0; i < quantidade_arestas; i++){
    int direita = spline->fechado ? (i + 1) % quantidade_controles : i + 1;
    distancias[i] = calcula_distancia(spline->controles.data[i],
                                       spline->controles.data[direita]);
  }
  int quantidade = 0;
  for(int ordinal = 0;; ordinal++){
    float inicio, fim;
    int indice_no;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, &indice_no) != 0){ break; }
    float maior_distancia = 0.0f;
    for(int i = indice_no - (int)spline->grau; i < indice_no; i++){
      int aresta = spline->fechado ? modulo_indice_candidato(i, quantidade_controles) : i;
      if(aresta >= 0 && aresta < quantidade_arestas && distancias[aresta] > maior_distancia){
        maior_distancia = distancias[aresta];
      }
    }
    int elegivel = remover ? maior_distancia < limiar : maior_distancia > limiar;
    if(remover){ elegivel = elegivel && (spline->fechado || inicio > spline->nos[spline->grau]); }
    if(elegivel){
      float candidato = remover ? inicio : (inicio + fim) * 0.5f;
      quantidade = adicionar_candidato(nos, quantidade, capacidade, candidato);
      if(quantidade < 0){ free(distancias); return -1; }
    }
  }
  free(distancias);
  return quantidade;
}

int bspline_topologia_candidatos_insercao_todos(const BSplineTopologia *spline,
                                                float limiar, float *nos, int capacidade){
  return candidatos_por_distancias(spline, limiar, nos, capacidade, 0);
}

int bspline_topologia_candidatos_remocao_todos(const BSplineTopologia *spline,
                                               float limiar, float *nos, int capacidade){
  return candidatos_por_distancias(spline, limiar, nos, capacidade, 1);
}
