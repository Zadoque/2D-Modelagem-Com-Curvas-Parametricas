#define _POSIX_C_SOURCE 199309L

#include "../includes/benchmark_painel.h"
#include "../includes/geometria_curvas.h"
#include "../includes/curvas/bezier.h"
#include "../includes/curvas/bspline.h"
#include "../includes/curvas/catmullrom.h"
#include "../includes/curvas/hermite.h"
#include "../includes/obb.h"

#include <float.h>
#include <stdlib.h>
#include <time.h>

static double agora_ms_benchmark(void){
  struct timespec tempo;
  clock_gettime(CLOCK_MONOTONIC, &tempo);
  return (double)tempo.tv_sec * 1000.0 + (double)tempo.tv_nsec / 1e6;
}

static int controles_padrao(Pontos *controles){
  static const float valores[][2] = {{-310, -100}, {-220, 180}, {-120, -140}, {-20, 150},
                                     {80, -120}, {170, 160}, {250, -100}, {330, 80}};
  pontos_init(controles, 8);
  for(size_t i = 0; i < sizeof(valores) / sizeof(valores[0]); i++){
    ponto ponto = {{valores[i][0], valores[i][1], 1.0f}};
    if(pontos_push(controles, ponto) != 0){
      pontos_free(controles);
      return -1;
    }
  }
  return 0;
}

static ResultadoPicking fazer_picking(int curva, AABBTREE *arvore, Pontos *controles, ponto alvo){
  switch(curva){
  case MODO_CURVA_HERMITE: return picking_hermite(arvore, controles, alvo, 5.0f, FLT_MAX);
  case MODO_CURVA_CATMULLROM: return picking_catmullrom(arvore, controles, alvo, 5.0f, FLT_MAX);
  case MODO_CURVA_BSPLINE: return picking_bspline(arvore, controles, alvo, 5.0f, FLT_MAX);
  case MODO_CURVA_BEZIER: return picking_bezier(arvore, controles, alvo, 5.0f, FLT_MAX);
  default: return (ResultadoPicking){.segmento_indice = -1};
  }
}

static void fazer_arrasto(int curva, Pontos *controles, ponto alvo, float rigidez){
  switch(curva){
  case MODO_CURVA_HERMITE: arrastar_ponto_hermite(controles, 0, 0.5f, alvo, rigidez); break;
  case MODO_CURVA_CATMULLROM: arrastar_ponto_catmullrom(controles, 0, 0.5f, alvo, rigidez); break;
  case MODO_CURVA_BSPLINE: arrastar_ponto_bspline(controles, 0, 0.5f, alvo, rigidez); break;
  case MODO_CURVA_BEZIER: arrastar_ponto_bezier(controles, 0, 0.5f, alvo, rigidez); break;
  }
}

void benchmark_configuracao_padrao(ConfiguracaoBenchmark *configuracao){
  if(!configuracao){ return; }
  *configuracao = (ConfiguracaoBenchmark){
      .mascara_curvas = 0x0fu,
      .mascara_testes = BENCH_GERACAO | BENCH_DESENHO | BENCH_PICKING | BENCH_ARRASTO |
                         BENCH_AUTOINTERFERENCIA,
      .repeticoes = 100,
      .warmup = 10,
      .rigidez = 0.5f,
      .posicao_densidade = 2,
      .anti_autointerferencia = 1,
      .raio_arrasto = 30.0f,
      .semente = UINT32_C(0x2d435552)};
}

uint32_t benchmark_proximo_aleatorio(uint32_t *estado){
  if(!estado){ return 0; }
  uint32_t x = *estado ? *estado : UINT32_C(0x6d2b79f5);
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *estado = x;
  return x;
}

float benchmark_aleatorio_intervalo(uint32_t *estado, float minimo, float maximo){
  const float unidade = (float)(benchmark_proximo_aleatorio(estado) >> 8) / 16777215.0f;
  return minimo + (maximo - minimo) * unidade;
}

void benchmark_resultado_limpar(ResultadoBenchmarkPainel *resultado){
  if(!resultado){ return; }
  *resultado = (ResultadoBenchmarkPainel){.minimo_ms = DBL_MAX};
}

void benchmark_resultado_adicionar(ResultadoBenchmarkPainel *resultado, double ms){
  if(!resultado || ms < 0.0){ return; }
  resultado->soma_ms += ms;
  if(ms < resultado->minimo_ms){ resultado->minimo_ms = ms; }
  if(ms > resultado->maximo_ms){ resultado->maximo_ms = ms; }
  resultado->amostras++;
}

double benchmark_resultado_media(const ResultadoBenchmarkPainel *resultado){
  return resultado && resultado->amostras ? resultado->soma_ms / resultado->amostras : 0.0;
}

int benchmark_painel_amostra_cpu(const ConfiguracaoBenchmark *configuracao, int curva,
                                 TipoBenchmark tipo, uint32_t *aleatorio,
                                 ResultadoBenchmarkPainel *resultado){
  if(!configuracao || !aleatorio || !resultado || curva < MODO_CURVA_HERMITE ||
     curva > MODO_CURVA_BEZIER || tipo == BENCH_DESENHO){
    return -1;
  }
  Pontos controles, scratch;
  GeometriaCurvas geometria;
  AABB_vec caixas;
  if(controles_padrao(&controles) != 0){ return -1; }
  pontos_init(&scratch, 64);
  geometria_curvas_init(&geometria);
  aabb_vec_init(&caixas, 16);
  int fechado = 0;
  double inicio = agora_ms_benchmark();
  if(tipo == BENCH_GERACAO){
    (void)geometria_curvas_atualizar(&geometria, &scratch, &caixas, &controles, (Curva)curva,
                                      fechado);
  } else {
    (void)geometria_curvas_atualizar(&geometria, &scratch, &caixas, &controles, (Curva)curva,
                                      fechado);
    AABBTREE *arvore = aabb_vec_para_arvore(&caixas);
    ponto alvo = {{benchmark_aleatorio_intervalo(aleatorio, -400.0f, 400.0f),
                   benchmark_aleatorio_intervalo(aleatorio, -400.0f, 400.0f), 1.0f}};
    inicio = agora_ms_benchmark();
    if(tipo == BENCH_PICKING){
      (void)fazer_picking(curva, arvore, &controles, alvo);
    } else if(tipo == BENCH_ARRASTO){
      alvo = controles.data[2];
      alvo.point[0] += benchmark_aleatorio_intervalo(aleatorio, -configuracao->raio_arrasto,
                                                       configuracao->raio_arrasto);
      alvo.point[1] += benchmark_aleatorio_intervalo(aleatorio, -configuracao->raio_arrasto,
                                                       configuracao->raio_arrasto);
      fazer_arrasto(curva, &controles, alvo, configuracao->rigidez);
    } else if(tipo == BENCH_AUTOINTERFERENCIA && curva == MODO_CURVA_BSPLINE){
      BSplineTopologia spline;
      OBBArvore obbs;
      obb_arvore_init(&obbs);
      if(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0){
        (void)obb_arvore_construir_bspline(&obbs, &spline);
        (void)obb_bspline_autointerseccoes(&obbs, &spline, 0.05f, NULL, 0);
        obb_arvore_liberar(&obbs);
        bspline_topologia_liberar(&spline);
      }
    }
    aabb_tree_free(arvore);
  }
  benchmark_resultado_adicionar(resultado, agora_ms_benchmark() - inicio);
  free(caixas.dados);
  geometria_curvas_free(&geometria);
  pontos_free(&scratch);
  pontos_free(&controles);
  return 0;
}
