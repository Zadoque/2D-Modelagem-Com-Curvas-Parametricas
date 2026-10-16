#define _POSIX_C_SOURCE 199309L

#include "benchmark_painel.h"
#include "geometria_curvas.h"
#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
#include "curvas/hermite.h"
#include "obb.h"
#include "bspline_topologia.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

static double agora_ms_benchmark(void){
  struct timespec tempo;
  clock_gettime(CLOCK_MONOTONIC, &tempo);
  return (double)tempo.tv_sec * 1000.0 + (double)tempo.tv_nsec / 1e6;
}

static float limiar_densidade_benchmark(int posicao){
  static const float limiares[] = {230.0f, 210.0f, 190.0f, 170.0f, 150.0f};
  if(posicao < 0){ posicao = 0; }
  if(posicao > 4){ posicao = 4; }
  return limiares[posicao];
}

static int controles_aleatorios(Pontos *controles, int quantidade, uint32_t *aleatorio){
  if(quantidade < 4){ quantidade = 4; }
  if(quantidade > 256){ quantidade = 256; }
  pontos_init(controles, (size_t)quantidade);
  for(int i = 0; i < quantidade; i++){
    ponto ponto = {{benchmark_aleatorio_intervalo(aleatorio, -400.0f, 400.0f),
                    benchmark_aleatorio_intervalo(aleatorio, -400.0f, 400.0f), 1.0f}};
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

static void fazer_arrasto(int curva, Pontos *controles, int segmento, float t, ponto alvo,
                          float rigidez){
  switch(curva){
  case MODO_CURVA_HERMITE: arrastar_ponto_hermite(controles, segmento, t, alvo, rigidez); break;
  case MODO_CURVA_CATMULLROM: arrastar_ponto_catmullrom(controles, segmento, t, alvo, rigidez); break;
  case MODO_CURVA_BSPLINE: arrastar_ponto_bspline(controles, segmento, t, alvo, rigidez); break;
  case MODO_CURVA_BEZIER: arrastar_ponto_bezier(controles, segmento, t, alvo, rigidez); break;
  }
}

static ponto avaliar_segmento(int curva, const Pontos *controles, int segmento, float t){
  ponto p0 = controles->data[segmento];
  ponto p1 = controles->data[segmento + 1];
  ponto p2 = controles->data[segmento + 2];
  ponto p3 = controles->data[segmento + 3];
  switch(curva){
  case MODO_CURVA_HERMITE: {
    ponto t1, t2;
    calcular_tangentes_hermite(p0, p1, p2, p3, &t1, &t2);
    return calcular_ponto_hermite(p1, p2, t1, t2, t);
  }
  case MODO_CURVA_CATMULLROM: return calcular_ponto_catmullrom(p0, p1, p2, p3, t);
  case MODO_CURVA_BSPLINE: return calcular_ponto_bspline(p0, p1, p2, p3, t);
  default: return calcular_ponto_bezier(p0, p1, p2, p3, t);
  }
}

static ponto destino_no_raio(ponto origem, uint32_t *aleatorio, float raio){
  float angulo = benchmark_aleatorio_intervalo(aleatorio, 0.0f, 6.283185307f);
  float distancia = sqrtf(benchmark_aleatorio_intervalo(aleatorio, 0.0f, 1.0f)) * raio;
  ponto destino = origem;
  destino.point[0] += cosf(angulo) * distancia;
  destino.point[1] += sinf(angulo) * distancia;
  return destino;
}

static void ponto_interpolado(ponto *resultado, ponto origem, ponto destino, int passo,
                              int quantidade_passos){
  float fracao = (float)passo / (float)quantidade_passos;
  *resultado = origem;
  resultado->point[0] += (destino.point[0] - origem.point[0]) * fracao;
  resultado->point[1] += (destino.point[1] - origem.point[1]) * fracao;
}

static int quantidade_passos_arrasto(ponto origem, ponto destino){
  float dx = destino.point[0] - origem.point[0];
  float dy = destino.point[1] - origem.point[1];
  int passos = (int)ceilf(sqrtf(dx * dx + dy * dy) / 5.0f);
  return passos > 0 ? passos : 1;
}

static int arrastar_curva_discreto(int curva, Pontos *controles, int segmento, float t,
                                   ponto origem, ponto destino, float rigidez){
  int passos = quantidade_passos_arrasto(origem, destino);
  for(int passo = 1; passo <= passos; passo++){
    ponto mouse;
    ponto_interpolado(&mouse, origem, destino, passo, passos);
    fazer_arrasto(curva, controles, segmento, t, mouse, rigidez);
  }
  return 0;
}

static int arrastar_bspline_discreto(BSplineTopologia *spline, float u, ponto origem,
                                     ponto destino, float rigidez, int *segmento,
                                     float limiar, int remover,
                                     EstatisticasArrastoBSpline *estatisticas){
  int passos = quantidade_passos_arrasto(origem, destino);
  *estatisticas = (EstatisticasArrastoBSpline){0};
  for(int passo = 1; passo <= passos; passo++){
    ponto mouse;
    ponto_interpolado(&mouse, origem, destino, passo, passos);
    EstatisticasArrastoBSpline parcial;
    if(bspline_topologia_arrastar_com_estatisticas(spline, u, mouse, rigidez, *segmento,
                                                    limiar, remover, &parcial) < 0){
      return -1;
    }
    estatisticas->insercoes += parcial.insercoes;
    estatisticas->remocoes += parcial.remocoes;
    estatisticas->tempo_insercao_ms += parcial.tempo_insercao_ms;
    estatisticas->tempo_remocao_ms += parcial.tempo_remocao_ms;
    float t_local;
    if(bspline_topologia_remapear_parametro(spline, u, segmento, &t_local) != 0){
      return -1;
    }
  }
  return 0;
}

static int arrastar_bspline_discreto_simples(BSplineTopologia *spline, float u, ponto origem,
                                              ponto destino, float rigidez){
  int passos = quantidade_passos_arrasto(origem, destino);
  for(int passo = 1; passo <= passos; passo++){
    ponto mouse;
    ponto_interpolado(&mouse, origem, destino, passo, passos);
    if(bspline_topologia_arrastar(spline, u, mouse, rigidez) != 0){
      return -1;
    }
  }
  return 0;
}

static int arrastar_bspline_discreto_com_checada(BSplineTopologia *spline, float u, ponto origem,
                                                  ponto destino, float rigidez, OBBArvore *obbs){
  int passos = quantidade_passos_arrasto(origem, destino);
  for(int passo = 1; passo <= passos; passo++){
    ponto mouse;
    ponto_interpolado(&mouse, origem, destino, passo, passos);
    BSplineTopologia anterior = {0};
    if(bspline_topologia_copiar(spline, &anterior) != 0 ||
       bspline_topologia_arrastar(spline, u, mouse, rigidez) != 0){
      bspline_topologia_liberar(&anterior);
      return -1;
    }
    obb_arvore_liberar(obbs);
    obb_arvore_init(obbs);
    if(obb_arvore_construir_bspline(obbs, spline) != 0){
      bspline_topologia_liberar(spline);
      *spline = anterior;
      return -1;
    }
    if(obb_bspline_autointerseccoes(obbs, spline, 0.05f, NULL, 0) != 0){
      bspline_topologia_liberar(spline);
      *spline = anterior;
    } else {
      bspline_topologia_liberar(&anterior);
    }
  }
  return 0;
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
  if(controles_aleatorios(&controles, configuracao->quantidade_controles, aleatorio) != 0){
    return -1;
  }
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
      if(curva == MODO_CURVA_BSPLINE){
        BSplineTopologia spline;
        if(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) != 0){
          aabb_tree_free(arvore); free(caixas.dados); geometria_curvas_free(&geometria);
          pontos_free(&scratch); pontos_free(&controles); return -1;
        }
        int segmentos = spline.controles.quantidade_atual - 3;
        int segmento = (int)(benchmark_proximo_aleatorio(aleatorio) % (uint32_t)segmentos);
        float inicio_u, fim_u;
        (void)bspline_topologia_obter_span(&spline, segmento, &inicio_u, &fim_u, NULL);
        float t = benchmark_aleatorio_intervalo(aleatorio, 0.0f, 1.0f);
        alvo = bspline_topologia_avaliar(&spline, inicio_u + (fim_u - inicio_u) * t);
        ponto origem = alvo;
        alvo = destino_no_raio(origem, aleatorio, configuracao->raio_arrasto);
        inicio = agora_ms_benchmark();
        EstatisticasArrastoBSpline estatisticas;
        int remover = configuracao->posicao_densidade < 2;
        (void)arrastar_bspline_discreto(
            &spline, inicio_u + (fim_u - inicio_u) * t, origem, alvo, configuracao->rigidez,
            &segmento, limiar_densidade_benchmark(configuracao->posicao_densidade), remover,
            &estatisticas);
        double arrasto_ms = agora_ms_benchmark() - inicio;
        resultado->insercoes += estatisticas.insercoes;
        resultado->remocoes += estatisticas.remocoes;
        resultado->soma_insercao_ms += estatisticas.tempo_insercao_ms;
        resultado->soma_remocao_ms += estatisticas.tempo_remocao_ms;
        bspline_topologia_liberar(&spline);
        benchmark_resultado_adicionar(resultado, arrasto_ms);
        aabb_tree_free(arvore); free(caixas.dados); geometria_curvas_free(&geometria);
        pontos_free(&scratch); pontos_free(&controles); return 0;
      }
      int segmentos = controles.quantidade_atual - 3;
      int segmento = (int)(benchmark_proximo_aleatorio(aleatorio) % (uint32_t)segmentos);
      float t = benchmark_aleatorio_intervalo(aleatorio, 0.0f, 1.0f);
      alvo = avaliar_segmento(curva, &controles, segmento, t);
      ponto origem = alvo;
      alvo = destino_no_raio(origem, aleatorio, configuracao->raio_arrasto);
      inicio = agora_ms_benchmark();
      (void)arrastar_curva_discreto(curva, &controles, segmento, t, origem, alvo,
                                     configuracao->rigidez);
    } else if(tipo == BENCH_AUTOINTERFERENCIA && curva == MODO_CURVA_BSPLINE){
      BSplineTopologia spline;
      OBBArvore obbs;
      obb_arvore_init(&obbs);
      if(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0){
        (void)obb_arvore_construir_bspline(&obbs, &spline);
        int segmento = (int)(benchmark_proximo_aleatorio(aleatorio) % (uint32_t)(spline.controles.quantidade_atual - 3));
        float t = benchmark_aleatorio_intervalo(aleatorio, 0.0f, 1.0f);
        float inicio_u, fim_u;
        (void)bspline_topologia_obter_span(&spline, segmento, &inicio_u, &fim_u, NULL);
        ponto ponto_curva = bspline_topologia_avaliar(&spline, inicio_u + (fim_u - inicio_u) * t);
        ponto destino = destino_no_raio(ponto_curva, aleatorio, configuracao->raio_arrasto);
        BSplineTopologia sem = {0}, com = {0};
        (void)bspline_topologia_copiar(&spline, &sem);
        (void)bspline_topologia_copiar(&spline, &com);
        double sem_inicio = agora_ms_benchmark();
        (void)arrastar_bspline_discreto_simples(
            &sem, inicio_u + (fim_u - inicio_u) * t, ponto_curva, destino,
            configuracao->rigidez);
        double sem_ms = agora_ms_benchmark() - sem_inicio;
        double com_inicio = agora_ms_benchmark();
        (void)arrastar_bspline_discreto_com_checada(
            &com, inicio_u + (fim_u - inicio_u) * t, ponto_curva, destino,
            configuracao->rigidez, &obbs);
        double com_ms = agora_ms_benchmark() - com_inicio;
        benchmark_resultado_adicionar(resultado, fmax(0.0, com_ms - sem_ms));
        bspline_topologia_liberar(&sem);
        bspline_topologia_liberar(&com);
        obb_arvore_liberar(&obbs);
        bspline_topologia_liberar(&spline);
        free(caixas.dados);
        geometria_curvas_free(&geometria);
        pontos_free(&scratch);
        pontos_free(&controles);
        return 0;
      }
      obb_arvore_liberar(&obbs);
    }
    if(tipo == BENCH_AUTOINTERFERENCIA){
      /* A métrica de overhead existe apenas para B-spline. */
      aabb_tree_free(arvore);
      free(caixas.dados);
      geometria_curvas_free(&geometria);
      pontos_free(&scratch);
      pontos_free(&controles);
      return 0;
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
