#define _POSIX_C_SOURCE 199309L
#include "bspline_topologia.h"
#include "curvas/utils.h"

#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BSPLINE_FATOR_RIGIDEZ 24.0f

static double agora_ms_topologia(void){
  struct timespec instante;
  clock_gettime(CLOCK_MONOTONIC, &instante);
  return (double)instante.tv_sec * 1000.0 + (double)instante.tv_nsec / 1e6;
}

typedef struct {
  ponto *controles;
  int quantidade_controles;
  float *nos;
  int quantidade_nos;
} EstadoSpline;

static int comparar_float(const void *a, const void *b){
  float esquerda = *(const float *)a;
  float direita = *(const float *)b;
  return (esquerda > direita) - (esquerda < direita);
}

static void estado_liberar(EstadoSpline *estado){
  free(estado->controles);
  free(estado->nos);
  *estado = (EstadoSpline){0};
}

static int estado_copiar(const BSplineTopologia *spline, EstadoSpline *estado){
  size_t bytes_controles = (size_t)spline->controles.quantidade_atual * sizeof(*estado->controles);
  size_t bytes_nos = (size_t)spline->quantidade_nos * sizeof(*estado->nos);
  *estado = (EstadoSpline){0};
  estado->controles = malloc(bytes_controles);
  estado->nos = malloc(bytes_nos);
  if(!estado->controles || !estado->nos){
    estado_liberar(estado);
    return -1;
  }
  memcpy(estado->controles, spline->controles.data, bytes_controles);
  memcpy(estado->nos, spline->nos, bytes_nos);
  estado->quantidade_controles = spline->controles.quantidade_atual;
  estado->quantidade_nos = spline->quantidade_nos;
  return 0;
}

static int multiplicidade(const EstadoSpline *estado, float u){
  int quantidade = 0;
  for(int i = 0; i < estado->quantidade_nos; i++){
    if(fabsf(estado->nos[i] - u) <= 1e-6f){
      quantidade++;
    }
  }
  return quantidade;
}

static int encontrar_span(const EstadoSpline *estado, unsigned grau, float u){
  int n = estado->quantidade_controles - 1;
  if(u >= estado->nos[n + 1]){
    return n;
  }
  int baixo = (int)grau;
  int alto = n + 1;
  while(alto - baixo > 1){
    int meio = (baixo + alto) / 2;
    if(u < estado->nos[meio]){
      alto = meio;
    } else {
      baixo = meio;
    }
  }
  return baixo;
}

static ponto combinar(float alfa, ponto esquerda, ponto direita){
  ponto resultado = {{0.0f, 0.0f, 1.0f}};
  for(int eixo = 0; eixo < 2; eixo++){
    resultado.point[eixo] = alfa * esquerda.point[eixo] + (1.0f - alfa) * direita.point[eixo];
  }
  return resultado;
}

static int modulo_indice(int indice, int quantidade){
  int resultado = indice % quantidade;
  return resultado < 0 ? resultado + quantidade : resultado;
}

static float no_periodico(const BSplineTopologia *spline, int indice){
  int periodo_indice = spline->quantidade_nos;
  int ciclos = indice / periodo_indice;
  int resto = indice % periodo_indice;
  if(resto < 0){
    resto += periodo_indice;
    ciclos--;
  }
  return spline->nos[resto] + (float)ciclos * spline->periodo;
}

static float normalizar_parametro_periodico(const BSplineTopologia *spline, float u){
  float inicio = spline->nos[0];
  float deslocado = fmodf(u - inicio, spline->periodo);
  if(deslocado < 0.0f){
    deslocado += spline->periodo;
  }
  return inicio + deslocado;
}

static int encontrar_span_periodico(const BSplineTopologia *spline, float u){
  u = normalizar_parametro_periodico(spline, u);
  int baixo = 0;
  int alto = spline->quantidade_nos;
  while(alto - baixo > 1){
    int meio = baixo + (alto - baixo) / 2;
    if(u < spline->nos[meio]){
      alto = meio;
    } else {
      baixo = meio;
    }
  }
  return baixo;
}

static int pesos_locais(const BSplineTopologia *spline, float u, int indices[4], float pesos[4]){
  if(!spline || spline->grau != 3 || !spline->nos || !spline->controles.data || !isfinite(u)){
    return -1;
  }
  if(spline->fechado){
    int k = encontrar_span_periodico(spline, u);
    float u_local = normalizar_parametro_periodico(spline, u);
    float esquerda[4] = {0.0f};
    float direita[4] = {0.0f};
    pesos[0] = 1.0f;
    for(unsigned j = 1; j <= spline->grau; j++){
      esquerda[j] = u_local - no_periodico(spline, k + 1 - (int)j);
      direita[j] = no_periodico(spline, k + (int)j) - u_local;
      float salvo = 0.0f;
      for(unsigned r = 0; r < j; r++){
        float denominador = direita[r + 1] + esquerda[j - r];
        float termo = fabsf(denominador) <= 1e-7f ? 0.0f : pesos[r] / denominador;
        pesos[r] = salvo + direita[r + 1] * termo;
        salvo = esquerda[j - r] * termo;
      }
      pesos[j] = salvo;
    }
    for(unsigned i = 0; i <= spline->grau; i++){
      indices[i] = modulo_indice(k - (int)spline->grau + (int)i,
                                  spline->controles.quantidade_atual);
    }
    return 0;
  }
  EstadoSpline estado = {spline->controles.data, spline->controles.quantidade_atual, spline->nos,
                          spline->quantidade_nos};
  int n = estado.quantidade_controles - 1;
  float inicio = estado.nos[spline->grau];
  float fim = estado.nos[n + 1];
  u = fmaxf(inicio, fminf(fim, u));
  int k = encontrar_span(&estado, spline->grau, u);
  float esquerda[4] = {0.0f};
  float direita[4] = {0.0f};
  pesos[0] = 1.0f;
  for(unsigned j = 1; j <= spline->grau; j++){
    esquerda[j] = u - estado.nos[k + 1 - (int)j];
    direita[j] = estado.nos[k + (int)j] - u;
    float salvo = 0.0f;
    for(unsigned r = 0; r < j; r++){
      float denominador = direita[r + 1] + esquerda[j - r];
      float termo = fabsf(denominador) <= 1e-7f ? 0.0f : pesos[r] / denominador;
      pesos[r] = salvo + direita[r + 1] * termo;
      salvo = esquerda[j - r] * termo;
    }
    pesos[j] = salvo;
  }
  for(unsigned i = 0; i <= spline->grau; i++){
    indices[i] = k - (int)spline->grau + (int)i;
  }
  return 0;
}

static int inserir_um(EstadoSpline *estado, unsigned grau, float u){
  int n = estado->quantidade_controles - 1;
  int k = encontrar_span(estado, grau, u);
  int s = multiplicidade(estado, u);
  if(s >= (int)grau || u <= estado->nos[grau] || u >= estado->nos[n + 1]){
    return -1;
  }
  ponto *novos_controles = malloc((size_t)(n + 2) * sizeof(*novos_controles));
  float *novos_nos = malloc((size_t)(estado->quantidade_nos + 1) * sizeof(*novos_nos));
  if(!novos_controles || !novos_nos){
    free(novos_controles);
    free(novos_nos);
    return -1;
  }
  memcpy(novos_nos, estado->nos, (size_t)(k + 1) * sizeof(*novos_nos));
  novos_nos[k + 1] = u;
  memcpy(&novos_nos[k + 2], &estado->nos[k + 1],
         (size_t)(estado->quantidade_nos - k - 1) * sizeof(*novos_nos));
  for(int i = 0; i <= k - (int)grau; i++){
    novos_controles[i] = estado->controles[i];
  }
  for(int i = k - s; i <= n; i++){
    novos_controles[i + 1] = estado->controles[i];
  }
  for(int i = k - (int)grau + 1; i <= k - s; i++){
    float denominador = estado->nos[i + grau] - estado->nos[i];
    if(fabsf(denominador) <= 1e-7f){
      free(novos_controles);
      free(novos_nos);
      return -1;
    }
    float alfa = (u - estado->nos[i]) / denominador;
    novos_controles[i] = combinar(alfa, estado->controles[i], estado->controles[i - 1]);
  }
  free(estado->controles);
  free(estado->nos);
  estado->controles = novos_controles;
  estado->nos = novos_nos;
  estado->quantidade_controles++;
  estado->quantidade_nos++;
  return 0;
}

static int localizar_no_interno(const EstadoSpline *estado, unsigned grau, float u){
  for(int i = (int)grau + 1; i < estado->quantidade_nos - (int)grau - 1; i++){
    if(fabsf(estado->nos[i] - u) <= 1e-6f){
      return i;
    }
  }
  return -1;
}

static float distancia_controles(ponto a, ponto b){
  float dx = a.point[0] - b.point[0];
  float dy = a.point[1] - b.point[1];
  return sqrtf(dx * dx + dy * dy);
}

static int tentar_remover_um(const EstadoSpline *atual, unsigned grau, float u, float tolerancia,
                             EstadoSpline *resultado){
  int posicao = localizar_no_interno(atual, grau, u);
  if(posicao < 0 || atual->quantidade_controles <= (int)grau + 1){
    return 0;
  }
  EstadoSpline reduzido = {0};
  reduzido.quantidade_controles = atual->quantidade_controles - 1;
  reduzido.quantidade_nos = atual->quantidade_nos - 1;
  reduzido.controles = calloc((size_t)reduzido.quantidade_controles, sizeof(*reduzido.controles));
  reduzido.nos = malloc((size_t)reduzido.quantidade_nos * sizeof(*reduzido.nos));
  if(!reduzido.controles || !reduzido.nos){
    estado_liberar(&reduzido);
    return -1;
  }
  memcpy(reduzido.nos, atual->nos, (size_t)posicao * sizeof(*reduzido.nos));
  memcpy(&reduzido.nos[posicao], &atual->nos[posicao + 1],
         (size_t)(reduzido.quantidade_nos - posicao) * sizeof(*reduzido.nos));

  int n = reduzido.quantidade_controles - 1;
  int k = encontrar_span(&reduzido, grau, u);
  int s = multiplicidade(&reduzido, u);
  for(int i = 0; i <= k - (int)grau; i++){
    reduzido.controles[i] = atual->controles[i];
  }
  for(int i = k - s; i <= n; i++){
    reduzido.controles[i] = atual->controles[i + 1];
  }
  for(int i = k - (int)grau + 1; i <= k - s; i++){
    float denominador = reduzido.nos[i + grau] - reduzido.nos[i];
    if(fabsf(denominador) <= 1e-7f){
      estado_liberar(&reduzido);
      return 0;
    }
    float alfa = (u - reduzido.nos[i]) / denominador;
    if(fabsf(alfa) <= 1e-7f){
      estado_liberar(&reduzido);
      return 0;
    }
    for(int eixo = 0; eixo < 2; eixo++){
      reduzido.controles[i].point[eixo] =
          (atual->controles[i].point[eixo] - (1.0f - alfa) * reduzido.controles[i - 1].point[eixo]) /
          alfa;
    }
    reduzido.controles[i].point[2] = 1.0f;
  }

  EstadoSpline reinserido = {0};
  reinserido.quantidade_controles = reduzido.quantidade_controles;
  reinserido.quantidade_nos = reduzido.quantidade_nos;
  reinserido.controles = malloc((size_t)reinserido.quantidade_controles * sizeof(*reinserido.controles));
  reinserido.nos = malloc((size_t)reinserido.quantidade_nos * sizeof(*reinserido.nos));
  if(!reinserido.controles || !reinserido.nos){
    estado_liberar(&reduzido);
    estado_liberar(&reinserido);
    return -1;
  }
  memcpy(reinserido.controles, reduzido.controles,
         (size_t)reinserido.quantidade_controles * sizeof(*reinserido.controles));
  memcpy(reinserido.nos, reduzido.nos, (size_t)reinserido.quantidade_nos * sizeof(*reinserido.nos));
  if(inserir_um(&reinserido, grau, u) != 0){
    estado_liberar(&reduzido);
    estado_liberar(&reinserido);
    return 0;
  }
  float erro = 0.0f;
  for(int i = 0; i < atual->quantidade_controles; i++){
    float distancia = distancia_controles(atual->controles[i], reinserido.controles[i]);
    if(distancia > erro){
      erro = distancia;
    }
  }
  estado_liberar(&reinserido);
  if(erro > tolerancia){
    estado_liberar(&reduzido);
    return 0;
  }
  *resultado = reduzido;
  return 1;
}

static int substituir_estado(BSplineTopologia *spline, EstadoSpline *estado){
  pontos_free(&spline->controles);
  free(spline->nos);
  spline->controles.data = estado->controles;
  spline->controles.quantidade_atual = estado->quantidade_controles;
  spline->controles.quantidade_max = estado->quantidade_controles;
  spline->nos = estado->nos;
  spline->quantidade_nos = estado->quantidade_nos;
  spline->capacidade_nos = estado->quantidade_nos;
  if(!spline->fechado){
    spline->quantidade_controles_base = estado->quantidade_controles;
  }
  estado->controles = NULL;
  estado->nos = NULL;
  estado->quantidade_controles = 0;
  estado->quantidade_nos = 0;
  return 0;
}

int bspline_topologia_inicializar_uniforme_aberta(BSplineTopologia *spline,
                                                   const Pontos *controles, unsigned grau){
  if(!spline || !controles || !controles->data || grau != 3 ||
     controles->quantidade_atual < (int)grau + 1){
    return -1;
  }
  *spline = (BSplineTopologia){0};
  pontos_init(&spline->controles, (size_t)controles->quantidade_atual);
  for(int i = 0; i < controles->quantidade_atual; i++){
    if(pontos_push(&spline->controles, controles->data[i]) != 0){
      bspline_topologia_liberar(spline);
      return -1;
    }
  }
  spline->grau = grau;
  spline->fechado = 0;
  spline->quantidade_controles_base = controles->quantidade_atual;
  spline->quantidade_nos = controles->quantidade_atual + (int)grau + 1;
  spline->capacidade_nos = spline->quantidade_nos;
  spline->nos = malloc((size_t)spline->quantidade_nos * sizeof(*spline->nos));
  if(!spline->nos){
    bspline_topologia_liberar(spline);
    return -1;
  }
  for(int i = 0; i < spline->quantidade_nos; i++){
    spline->nos[i] = (float)(i - (int)grau);
  }
  return 0;
}

int bspline_topologia_inicializar_uniforme_fechada(BSplineTopologia *spline,
                                                    const Pontos *controles, unsigned grau){
  if(!spline || !controles || !controles->data || grau != 3 ||
     controles->quantidade_atual < (int)grau + 1){
    return -1;
  }
  *spline = (BSplineTopologia){0};
  pontos_init(&spline->controles, (size_t)controles->quantidade_atual);
  for(int i = 0; i < controles->quantidade_atual; i++){
    if(pontos_push(&spline->controles, controles->data[i]) != 0){
      bspline_topologia_liberar(spline);
      return -1;
    }
  }
  spline->grau = grau;
  spline->fechado = 1;
  spline->quantidade_controles_base = controles->quantidade_atual;
  spline->quantidade_nos = controles->quantidade_atual;
  spline->capacidade_nos = spline->quantidade_nos;
  spline->nos = malloc((size_t)spline->quantidade_nos * sizeof(*spline->nos));
  if(!spline->nos){
    bspline_topologia_liberar(spline);
    return -1;
  }
  for(int i = 0; i < spline->quantidade_nos; i++){
    spline->nos[i] = (float)i;
  }
  spline->periodo = (float)controles->quantidade_atual;
  return 0;
}

void bspline_topologia_liberar(BSplineTopologia *spline){
  if(!spline){
    return;
  }
  pontos_free(&spline->controles);
  free(spline->nos);
  *spline = (BSplineTopologia){0};
}

int bspline_topologia_copiar(const BSplineTopologia *origem, BSplineTopologia *destino){
  if(!origem || !destino || !origem->controles.data || !origem->nos ||
     origem->controles.quantidade_atual < 0 || origem->quantidade_nos <= 0){
    return -1;
  }
  BSplineTopologia copia = {0};
  pontos_init(&copia.controles, (size_t)origem->controles.quantidade_atual);
  if(!copia.controles.data){ return -1; }
  memcpy(copia.controles.data, origem->controles.data,
         (size_t)origem->controles.quantidade_atual * sizeof(*copia.controles.data));
  copia.controles.quantidade_atual = origem->controles.quantidade_atual;
  copia.grau = origem->grau;
  copia.fechado = origem->fechado;
  copia.quantidade_controles_base = origem->quantidade_controles_base;
  copia.quantidade_nos = copia.capacidade_nos = origem->quantidade_nos;
  copia.periodo = origem->periodo;
  copia.nos = malloc((size_t)copia.quantidade_nos * sizeof(*copia.nos));
  if(!copia.nos){
    bspline_topologia_liberar(&copia);
    return -1;
  }
  memcpy(copia.nos, origem->nos, (size_t)copia.quantidade_nos * sizeof(*copia.nos));
  *destino = copia;
  return 0;
}

ponto bspline_topologia_avaliar(const BSplineTopologia *spline, float u){
  ponto zero = {{0.0f, 0.0f, 1.0f}};
  if(!spline || !spline->nos || !spline->controles.data || spline->grau == 0 ||
     spline->controles.quantidade_atual < (int)spline->grau + 1 || !isfinite(u)){
    return zero;
  }
  if(spline->fechado){
    int k = encontrar_span_periodico(spline, u);
    u = normalizar_parametro_periodico(spline, u);
    ponto d[4];
    for(unsigned j = 0; j <= spline->grau; j++){
      d[j] = spline->controles.data[modulo_indice(k - (int)spline->grau + (int)j,
                                                   spline->controles.quantidade_atual)];
    }
    for(unsigned r = 1; r <= spline->grau; r++){
      for(int j = (int)spline->grau; j >= (int)r; j--){
        int i = k - (int)spline->grau + j;
        float denominador = no_periodico(spline, i + spline->grau - r + 1) -
                            no_periodico(spline, i);
        float alfa = fabsf(denominador) <= 1e-7f ? 0.0f :
                      (u - no_periodico(spline, i)) / denominador;
        d[j] = combinar(alfa, d[j], d[j - 1]);
      }
    }
    return d[spline->grau];
  }
  EstadoSpline estado = {spline->controles.data, spline->controles.quantidade_atual, spline->nos,
                          spline->quantidade_nos};
  int n = estado.quantidade_controles - 1;
  float inicio = estado.nos[spline->grau];
  float fim = estado.nos[n + 1];
  u = fmaxf(inicio, fminf(fim, u));
  int k = encontrar_span(&estado, spline->grau, u);
  ponto d[4];
  if(spline->grau > 3){
    return zero;
  }
  for(unsigned j = 0; j <= spline->grau; j++){
    d[j] = estado.controles[k - (int)spline->grau + (int)j];
  }
  for(unsigned r = 1; r <= spline->grau; r++){
    for(int j = (int)spline->grau; j >= (int)r; j--){
      int i = k - (int)spline->grau + j;
      float denominador = estado.nos[i + spline->grau - r + 1] - estado.nos[i];
      float alfa = fabsf(denominador) <= 1e-7f ? 0.0f : (u - estado.nos[i]) / denominador;
      d[j] = combinar(alfa, d[j], d[j - 1]);
    }
  }
  return d[spline->grau];
}

int bspline_topologia_obter_span(const BSplineTopologia *spline, int ordinal, float *inicio,
                                 float *fim, int *indice_no){
  if(!spline || !inicio || !fim || ordinal < 0){
    return -1;
  }
  if(spline->fechado){
    if(ordinal >= spline->quantidade_nos){
      return -1;
    }
    *inicio = no_periodico(spline, ordinal);
    *fim = no_periodico(spline, ordinal + 1);
    if(indice_no){ *indice_no = ordinal; }
    return 0;
  }
  int encontrados = 0;
  for(int i = (int)spline->grau; i < spline->quantidade_nos - (int)spline->grau - 1; i++){
    if(spline->nos[i] < spline->nos[i + 1]){
      if(encontrados++ == ordinal){
        *inicio = spline->nos[i];
        *fim = spline->nos[i + 1];
        if(indice_no){
          *indice_no = i;
        }
        return 0;
      }
    }
  }
  return -1;
}

int bspline_topologia_arrastar(BSplineTopologia *spline, float u, ponto mouse, float rigidez){
  int indices[4];
  float pesos[4];
  if(!spline || !isfinite(rigidez) || !isfinite(mouse.point[0]) || !isfinite(mouse.point[1]) ||
     pesos_locais(spline, u, indices, pesos) != 0){
    return -1;
  }
  rigidez = fmaxf(0.0f, fminf(1.0f, rigidez));
  ponto atual = {{0.0f, 0.0f, 1.0f}};
  float penalizacoes[4];
  float soma = 0.0f;
  for(int i = 0; i < 4; i++){
    atual.point[0] += pesos[i] * spline->controles.data[indices[i]].point[0];
    atual.point[1] += pesos[i] * spline->controles.data[indices[i]].point[1];
    float distancia = fabsf((float)i - 1.5f);
    penalizacoes[i] = 1.0f + BSPLINE_FATOR_RIGIDEZ * rigidez * distancia;
    soma += pesos[i] * pesos[i] / penalizacoes[i];
  }
  if(!(soma >= 1e-8f) || !isfinite(soma)){
    return -1;
  }
  float dx = mouse.point[0] - atual.point[0];
  float dy = mouse.point[1] - atual.point[1];
  for(int i = 0; i < 4; i++){
    float fator = pesos[i] / penalizacoes[i] / soma;
    spline->controles.data[indices[i]].point[0] += dx * fator;
    spline->controles.data[indices[i]].point[1] += dy * fator;
  }
  return 0;
}

int bspline_topologia_arrastar_com_estatisticas(BSplineTopologia *spline, float u, ponto mouse,
                                                 float rigidez, int span_central, float limiar,
                                                 int remover, EstatisticasArrastoBSpline *estatisticas){
  if(!spline || !estatisticas || span_central < 0 || !isfinite(limiar) || limiar < 0.0f){
    return -1;
  }
  *estatisticas = (EstatisticasArrastoBSpline){0};
  if(bspline_topologia_arrastar(spline, u, mouse, rigidez) != 0){
    return -1;
  }

  float nos[7];
  int quantidade = remover ?
      bspline_topologia_candidatos_remocao(spline, span_central, limiar, nos, 7) :
      bspline_topologia_candidatos_insercao(spline, span_central, limiar, nos, 7);
  if(quantidade <= 0){
    return quantidade;
  }

  double inicio = agora_ms_topologia();
  int alterados = remover ? remover_nos_lyche_morken(spline, nos, quantidade, 1e-3f) :
                           (inserir_nos_oslo(spline, nos, quantidade) == 0 ? quantidade : -1);
  double duracao = agora_ms_topologia() - inicio;
  if(alterados < 0){
    return -1;
  }
  if(remover){
    estatisticas->remocoes = alterados;
    estatisticas->tempo_remocao_ms = duracao;
  } else {
    estatisticas->insercoes = alterados;
    estatisticas->tempo_insercao_ms = duracao;
  }
  return alterados;
}

ResultadoPicking bspline_topologia_picking(const BSplineTopologia *spline, ponto mouse,
                                           float tolerancia){
  ResultadoPicking melhor = {-1, 0.0f, FLT_MAX};
  if(!spline || !isfinite(mouse.point[0]) || !isfinite(mouse.point[1]) || tolerancia < 0.0f){
    return melhor;
  }
  for(int ordinal = 0;; ordinal++){
    float inicio, fim;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, NULL) != 0){
      break;
    }
    ponto anterior = bspline_topologia_avaliar(spline, inicio);
    for(int amostra = 1; amostra <= 64; amostra++){
      float fracao = (float)amostra / 64.0f;
      ponto atual = bspline_topologia_avaliar(spline, inicio + (fim - inicio) * fracao);
      float dx = atual.point[0] - anterior.point[0];
      float dy = atual.point[1] - anterior.point[1];
      float comprimento2 = dx * dx + dy * dy;
      float projecao = comprimento2 > 1e-8f ?
                            ((mouse.point[0] - anterior.point[0]) * dx +
                             (mouse.point[1] - anterior.point[1]) * dy) / comprimento2 :
                            0.0f;
      projecao = fmaxf(0.0f, fminf(1.0f, projecao));
      float px = anterior.point[0] + projecao * dx;
      float py = anterior.point[1] + projecao * dy;
      float erro_x = mouse.point[0] - px;
      float erro_y = mouse.point[1] - py;
      float distancia = sqrtf(erro_x * erro_x + erro_y * erro_y);
      if(distancia <= tolerancia && distancia < melhor.distancia){
        melhor.segmento_indice = ordinal;
        melhor.t = inicio + (fim - inicio) * ((float)(amostra - 1) + projecao) / 64.0f;
        melhor.distancia = distancia;
      }
      anterior = atual;
    }
  }
  return melhor;
}

static ResultadoPicking refinar_span(const BSplineTopologia *spline, int ordinal, ponto mouse,
                                     float tolerancia, ResultadoPicking melhor){
  float inicio, fim;
  if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, NULL) != 0){
    return melhor;
  }
  ponto anterior = bspline_topologia_avaliar(spline, inicio);
  for(int amostra = 1; amostra <= 64; amostra++){
    float fracao = (float)amostra / 64.0f;
    ponto atual = bspline_topologia_avaliar(spline, inicio + (fim - inicio) * fracao);
    float dx = atual.point[0] - anterior.point[0];
    float dy = atual.point[1] - anterior.point[1];
    float comprimento2 = dx * dx + dy * dy;
    float projecao = comprimento2 > 1e-8f ?
                          ((mouse.point[0] - anterior.point[0]) * dx +
                           (mouse.point[1] - anterior.point[1]) * dy) / comprimento2 :
                          0.0f;
    projecao = fmaxf(0.0f, fminf(1.0f, projecao));
    float px = anterior.point[0] + projecao * dx;
    float py = anterior.point[1] + projecao * dy;
    float erro_x = mouse.point[0] - px;
    float erro_y = mouse.point[1] - py;
    float distancia = sqrtf(erro_x * erro_x + erro_y * erro_y);
    if(distancia <= tolerancia && distancia < melhor.distancia){
      melhor.segmento_indice = ordinal;
      melhor.t = inicio + (fim - inicio) * ((float)(amostra - 1) + projecao) / 64.0f;
      melhor.distancia = distancia;
    }
    anterior = atual;
  }
  return melhor;
}

static ResultadoPicking percorrer_arvore(const BSplineTopologia *spline, const AABBTREE *no,
                                         ponto mouse, float tolerancia, ResultadoPicking melhor){
  if(!no || dist_mouse_aabb(mouse, no->box) > tolerancia ||
     dist_mouse_aabb(mouse, no->box) > melhor.distancia){
    return melhor;
  }
  if(!no->esquerda && !no->direita){
    return refinar_span(spline, no->box.segmento_indice, mouse, tolerancia, melhor);
  }
  float esquerda = no->esquerda ? dist_mouse_aabb(mouse, no->esquerda->box) : FLT_MAX;
  float direita = no->direita ? dist_mouse_aabb(mouse, no->direita->box) : FLT_MAX;
  const AABBTREE *primeiro = esquerda <= direita ? no->esquerda : no->direita;
  const AABBTREE *segundo = esquerda <= direita ? no->direita : no->esquerda;
  melhor = percorrer_arvore(spline, primeiro, mouse, tolerancia, melhor);
  return percorrer_arvore(spline, segundo, mouse, tolerancia, melhor);
}

ResultadoPicking bspline_topologia_picking_arvore(const BSplineTopologia *spline,
                                                   const AABBTREE *arvore, ponto mouse,
                                                   float tolerancia){
  ResultadoPicking sem = {-1, 0.0f, FLT_MAX};
  if(!spline || !arvore || !isfinite(mouse.point[0]) || !isfinite(mouse.point[1]) ||
     tolerancia < 0.0f){
    return sem;
  }
  return percorrer_arvore(spline, arvore, mouse, tolerancia, sem);
}

int bspline_topologia_remapear_parametro(const BSplineTopologia *spline, float u, int *span,
                                         float *t_local){
  if(!spline || !span || !t_local || !isfinite(u)){
    return -1;
  }
  for(int ordinal = 0;; ordinal++){
    float inicio, fim;
    if(bspline_topologia_obter_span(spline, ordinal, &inicio, &fim, NULL) != 0){
      return -1;
    }
    if(u >= inicio && (u < fim || fabsf(u - fim) <= 1e-6f)){
      *span = ordinal;
      *t_local = fim > inicio ? (u - inicio) / (fim - inicio) : 0.0f;
      *t_local = fmaxf(0.0f, fminf(1.0f, *t_local));
      return 0;
    }
  }
}

int bspline_topologia_capturar_referencia_arrasto(const BSplineTopologia *spline, float u,
                                                  ReferenciaArrastoBSpline *referencia){
  if(!spline || !referencia || !isfinite(u)){
    return -1;
  }
  ReferenciaArrastoBSpline nova = {0};
  nova.u_global = spline->fechado ? normalizar_parametro_periodico(spline, u) : u;
  nova.ancora = bspline_topologia_avaliar(spline, nova.u_global);
  if(bspline_topologia_remapear_parametro(spline, nova.u_global, &nova.segmento_indice,
                                          &nova.t_local) != 0){
    return -1;
  }
  *referencia = nova;
  return 0;
}

int bspline_topologia_remapear_referencia_arrasto(const BSplineTopologia *spline,
                                                   ReferenciaArrastoBSpline *referencia,
                                                   float tolerancia){
  if(!spline || !referencia || tolerancia < 0.0f || !isfinite(tolerancia)){
    return -1;
  }
  referencia->u_global = spline->fechado ?
                            normalizar_parametro_periodico(spline, referencia->u_global) :
                            referencia->u_global;
  if(bspline_topologia_remapear_parametro(spline, referencia->u_global,
                                          &referencia->segmento_indice,
                                          &referencia->t_local) != 0){
    return -1;
  }
  ponto atual = bspline_topologia_avaliar(spline, referencia->u_global);
  return distancia_controles(atual, referencia->ancora) <= tolerancia ? 0 : -1;
}

static int resolver_sistema(float *matriz, float *termos, float *solucao, int ordem){
  for(int coluna = 0; coluna < ordem; coluna++){
    int pivo = coluna;
    for(int linha = coluna + 1; linha < ordem; linha++){
      if(fabsf(matriz[linha * ordem + coluna]) > fabsf(matriz[pivo * ordem + coluna])){
        pivo = linha;
      }
    }
    if(fabsf(matriz[pivo * ordem + coluna]) < 1e-7f){
      return -1;
    }
    if(pivo != coluna){
      for(int j = coluna; j < ordem; j++){
        float temporario = matriz[coluna * ordem + j];
        matriz[coluna * ordem + j] = matriz[pivo * ordem + j];
        matriz[pivo * ordem + j] = temporario;
      }
      float temporario = termos[coluna];
      termos[coluna] = termos[pivo];
      termos[pivo] = temporario;
    }
    float divisor = matriz[coluna * ordem + coluna];
    for(int j = coluna; j < ordem; j++){
      matriz[coluna * ordem + j] /= divisor;
    }
    termos[coluna] /= divisor;
    for(int linha = 0; linha < ordem; linha++){
      if(linha == coluna){
        continue;
      }
      float fator = matriz[linha * ordem + coluna];
      for(int j = coluna; j < ordem; j++){
        matriz[linha * ordem + j] -= fator * matriz[coluna * ordem + j];
      }
      termos[linha] -= fator * termos[coluna];
    }
  }
  memcpy(solucao, termos, (size_t)ordem * sizeof(*solucao));
  return 0;
}

/* Refina ou reduz um período completo. A colocalização em abscissas de
 * Greville representa exatamente a mesma spline quando o novo espaço a
 * contém; a validação posterior torna a troca transacional. */
static int reconstruir_periodica(BSplineTopologia *spline, const float *nos, int quantidade,
                                 int inserir, float tolerancia){
  int antiga = spline->quantidade_nos;
  int nova_quantidade = antiga + (inserir ? quantidade : -quantidade);
  if(!spline->fechado || quantidade <= 0 || nova_quantidade < (int)spline->grau + 1){
    return -1;
  }
  BSplineTopologia nova = {0};
  nova.grau = spline->grau;
  nova.fechado = 1;
  nova.quantidade_controles_base = nova_quantidade;
  nova.quantidade_nos = nova.capacidade_nos = nova_quantidade;
  nova.periodo = spline->periodo;
  nova.nos = malloc((size_t)nova_quantidade * sizeof(*nova.nos));
  nova.controles.data = calloc((size_t)nova_quantidade, sizeof(*nova.controles.data));
  nova.controles.quantidade_atual = nova.controles.quantidade_max = nova_quantidade;
  if(!nova.nos || !nova.controles.data){
    bspline_topologia_liberar(&nova);
    return -1;
  }
  if(inserir){
    memcpy(nova.nos, spline->nos, (size_t)antiga * sizeof(*nova.nos));
    for(int i = 0; i < quantidade; i++){
      nova.nos[antiga + i] = normalizar_parametro_periodico(spline, nos[i]);
    }
    qsort(nova.nos, (size_t)nova_quantidade, sizeof(*nova.nos), comparar_float);
  } else {
    int destino = 0;
    for(int i = 0; i < antiga; i++){
      int remover = 0;
      for(int j = 0; j < quantidade; j++){
        if(fabsf(spline->nos[i] - normalizar_parametro_periodico(spline, nos[j])) <= 1e-6f){
          remover = 1;
          break;
        }
      }
      if(!remover){
        nova.nos[destino++] = spline->nos[i];
      }
    }
    if(destino != nova_quantidade){
      bspline_topologia_liberar(&nova);
      return -1;
    }
  }
  size_t elementos = (size_t)nova_quantidade * (size_t)nova_quantidade;
  float *matriz = calloc(elementos, sizeof(*matriz));
  float *matriz_trabalho = malloc(elementos * sizeof(*matriz_trabalho));
  float *termos = malloc((size_t)nova_quantidade * sizeof(*termos));
  float *solucao = malloc((size_t)nova_quantidade * sizeof(*solucao));
  if(!matriz || !matriz_trabalho || !termos || !solucao){
    free(matriz); free(matriz_trabalho); free(termos); free(solucao);
    bspline_topologia_liberar(&nova);
    return -1;
  }
  for(int linha = 0; linha < nova_quantidade; linha++){
    float u = 0.0f;
    for(unsigned j = 1; j <= nova.grau; j++){
      u += no_periodico(&nova, linha + (int)j);
    }
    u /= (float)nova.grau;
    int indices[4];
    float pesos[4];
    if(pesos_locais(&nova, u, indices, pesos) != 0){
      free(matriz); free(matriz_trabalho); free(termos); free(solucao);
      bspline_topologia_liberar(&nova);
      return -1;
    }
    for(int j = 0; j < 4; j++){
      matriz[linha * nova_quantidade + indices[j]] += pesos[j];
    }
    ponto valor = bspline_topologia_avaliar(spline, u);
    termos[linha] = valor.point[0];
  }
  for(int eixo = 0; eixo < 2; eixo++){
    if(eixo != 0){
      for(int linha = 0; linha < nova_quantidade; linha++){
        float u = 0.0f;
        for(unsigned j = 1; j <= nova.grau; j++){
          u += no_periodico(&nova, linha + (int)j);
        }
        u /= (float)nova.grau;
        termos[linha] = bspline_topologia_avaliar(spline, u).point[eixo];
      }
    }
    memcpy(matriz_trabalho, matriz, elementos * sizeof(*matriz));
    if(resolver_sistema(matriz_trabalho, termos, solucao, nova_quantidade) != 0){
      free(matriz); free(matriz_trabalho); free(termos); free(solucao);
      bspline_topologia_liberar(&nova);
      return -1;
    }
    for(int i = 0; i < nova_quantidade; i++){
      nova.controles.data[i].point[eixo] = solucao[i];
      nova.controles.data[i].point[2] = 1.0f;
    }
  }
  float erro = 0.0f;
  for(int i = 0; i < 8 * nova_quantidade; i++){
    float u = spline->nos[0] + spline->periodo * (float)i / (float)(8 * nova_quantidade);
    float diferenca = distancia_controles(bspline_topologia_avaliar(spline, u),
                                         bspline_topologia_avaliar(&nova, u));
    if(diferenca > erro){ erro = diferenca; }
  }
  free(matriz); free(matriz_trabalho); free(termos); free(solucao);
  if(erro > tolerancia){
    bspline_topologia_liberar(&nova);
    return inserir ? -1 : 0;
  }
  bspline_topologia_liberar(spline);
  *spline = nova;
  return inserir ? 0 : quantidade;
}

int inserir_nos_oslo(BSplineTopologia *spline, const float *nos, int quantidade){
  if(!spline || !nos || quantidade <= 0 || spline->grau != 3){
    return -1;
  }
  if(spline->fechado){
    return reconstruir_periodica(spline, nos, quantidade, 1, 1e-3f);
  }
  EstadoSpline estado;
  if(estado_copiar(spline, &estado) != 0){
    return -1;
  }
  float *ordenados = malloc((size_t)quantidade * sizeof(*ordenados));
  if(!ordenados){
    estado_liberar(&estado);
    return -1;
  }
  memcpy(ordenados, nos, (size_t)quantidade * sizeof(*ordenados));
  qsort(ordenados, (size_t)quantidade, sizeof(*ordenados), comparar_float);
  for(int i = 0; i < quantidade; i++){
    if(!isfinite(ordenados[i]) || inserir_um(&estado, spline->grau, ordenados[i]) != 0){
      free(ordenados);
      estado_liberar(&estado);
      return -1;
    }
  }
  free(ordenados);
  if(substituir_estado(spline, &estado) != 0){
    estado_liberar(&estado);
    return -1;
  }
  return 0;
}

int remover_nos_lyche_morken(BSplineTopologia *spline, const float *nos, int quantidade,
                              float tolerancia){
  if(!spline || !nos || quantidade <= 0 || spline->grau != 3 || tolerancia < 0.0f ||
     !isfinite(tolerancia)){
    return -1;
  }
  for(int i = 0; i < quantidade; i++){
    if(!isfinite(nos[i])){
      return -1;
    }
  }
  if(spline->fechado){
    return reconstruir_periodica(spline, nos, quantidade, 0, tolerancia);
  }
  EstadoSpline estado;
  if(estado_copiar(spline, &estado) != 0){
    return -1;
  }
  float *ordenados = malloc((size_t)quantidade * sizeof(*ordenados));
  if(!ordenados){
    estado_liberar(&estado);
    return -1;
  }
  memcpy(ordenados, nos, (size_t)quantidade * sizeof(*ordenados));
  qsort(ordenados, (size_t)quantidade, sizeof(*ordenados), comparar_float);
  int removidos = 0;
  for(int i = quantidade - 1; i >= 0; i--){
    EstadoSpline reduzido = {0};
    int resultado = tentar_remover_um(&estado, spline->grau, ordenados[i], tolerancia, &reduzido);
    if(resultado < 0){
      free(ordenados);
      estado_liberar(&estado);
      return -1;
    }
    if(resultado > 0){
      estado_liberar(&estado);
      estado = reduzido;
      removidos++;
    }
  }
  if(removidos == 0){
    free(ordenados);
    estado_liberar(&estado);
    return 0;
  }
  if(substituir_estado(spline, &estado) != 0){
    free(ordenados);
    estado_liberar(&estado);
    return -1;
  }
  free(ordenados);
  return removidos;
}
