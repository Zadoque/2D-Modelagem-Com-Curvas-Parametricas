/* Necessário para expor clock_gettime()/CLOCK_MONOTONIC do POSIX.1b,
 * já que o projeto compila com -std=c99, que não inclui essas extensões
 * por padrão. Precisa vir antes de qualquer
 * include, inclusive opengl.h, pois alguns headers do sistema travam
 * as feature test macros no primeiro include. */
#define _POSIX_C_SOURCE 199309L

#include "performance.h"
#include "bspline_topologia.h"
#include "geometria_curvas.h"
#include <stdio.h>
#include <time.h>

static MetricasPerformance metricas_performance = {0};
static const Pontos *buffer_controles;
static const Pontos *buffer_segmento;

void __attribute__((no_instrument_function)) performance_definir_buffers(const Pontos *controles,
                                                                         const Pontos *segmento){
  buffer_controles = controles;
  buffer_segmento = segmento;
}

static double __attribute__((no_instrument_function)) agora_ms(void){
  return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

/* Cronômetro de maior resolução, usado apenas pelo benchmark controlado
 * (não pela instrumentação -finstrument-functions, executada durante o uso
 * interativo e não pode pagar o custo de uma syscall por chamada). */
static double __attribute__((no_instrument_function)) agora_ms_precisa(void){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

static __thread double inicio_geracao_hermite;
static __thread double inicio_geracao_bezier;
static __thread double inicio_geracao_bspline;
static __thread double inicio_geracao_catmullrom;
static __thread double inicio_desenho;
static __thread double inicio_geracao_topologica;

void __attribute__((no_instrument_function)) performance_iniciar_medicao(void){
  metricas_performance.tempo_geracao_ms = 0.0;
  metricas_performance.tempo_desenho_ms = 0.0;
  metricas_performance.pontos_gerados = 0;

  /* Estes campos representam estado instantâneo, não acumuladores. */
  metricas_performance.buffer_curva_uso = 0;
  metricas_performance.buffer_curva_capacidade =
      buffer_segmento ? buffer_segmento->quantidade_max : 0;
  metricas_performance.buffer_controle_uso =
      buffer_controles ? buffer_controles->quantidade_atual : 0;
  metricas_performance.buffer_controle_capacidade =
      buffer_controles ? buffer_controles->quantidade_max : 0;
}

void __attribute__((no_instrument_function)) performance_iniciar_geracao_topologica(void){
  inicio_geracao_topologica = agora_ms_precisa();
}

void __attribute__((no_instrument_function)) performance_finalizar_geracao_topologica(
    size_t pontos_gerados){
  metricas_performance.tempo_geracao_ms += agora_ms_precisa() - inicio_geracao_topologica;
  metricas_performance.pontos_gerados += pontos_gerados;
}

void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *func, void *caller){
  (void)caller;
  if(func == (void *)&gerar_curva_hermite){
    inicio_geracao_hermite = agora_ms();
  } else if(func == (void *)&gerar_curva_bezier){
    inicio_geracao_bezier = agora_ms();
  } else if(func == (void *)&gerar_curva_bspline){
    inicio_geracao_bspline = agora_ms();
  } else if(func == (void *)&gerar_curva_catmullrom){
    inicio_geracao_catmullrom = agora_ms();
  } else if(func == (void *)&desenhar_curva_atual){
    inicio_desenho = agora_ms();
  }
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *func, void *caller){
  (void)caller;

  if(func == (void *)&gerar_curva_hermite || func == (void *)&gerar_curva_bezier ||
     func == (void *)&gerar_curva_bspline || func == (void *)&gerar_curva_catmullrom){
    double inicio;
    if(func == (void *)&gerar_curva_hermite){
      inicio = inicio_geracao_hermite;
    } else if(func == (void *)&gerar_curva_bezier){
      inicio = inicio_geracao_bezier;
    } else if(func == (void *)&gerar_curva_bspline){
      inicio = inicio_geracao_bspline;
    } else {
      inicio = inicio_geracao_catmullrom;
    }

    /* Soma apenas os segmentos da reconstrução atual. O início de cada
     * reconstrução chama performance_iniciar_medicao() e zera o frame anterior. */
    metricas_performance.tempo_geracao_ms += agora_ms() - inicio;
    metricas_performance.pontos_gerados += buffer_segmento ? buffer_segmento->quantidade_atual : 0;
  } else if(func == (void *)&desenhar_curva_atual){
    metricas_performance.tempo_desenho_ms += agora_ms() - inicio_desenho;

    metricas_performance.buffer_curva_uso = buffer_segmento ? buffer_segmento->quantidade_atual : 0;
    metricas_performance.buffer_curva_capacidade =
        buffer_segmento ? buffer_segmento->quantidade_max : 0;
    metricas_performance.buffer_controle_uso =
        buffer_controles ? buffer_controles->quantidade_atual : 0;
    metricas_performance.buffer_controle_capacidade =
        buffer_controles ? buffer_controles->quantidade_max : 0;

    printf("[PERF] geracao=%.3f ms | desenho=%.3f ms | pontos=%zu | "
           "buffer curva=%zu/%zu | buffer controle=%zu/%zu\n",
           metricas_performance.tempo_geracao_ms, metricas_performance.tempo_desenho_ms,
           metricas_performance.pontos_gerados, metricas_performance.buffer_curva_uso,
           metricas_performance.buffer_curva_capacidade, metricas_performance.buffer_controle_uso,
           metricas_performance.buffer_controle_capacidade);
  }
}

/* =========================================================================
 * BENCHMARK DETERMINÍSTICO
 * =========================================================================
 * A rotina abaixo permanece isolada da medição da interface e:
 *   - usa pontos de controle FIXOS (não vem de clique de mouse);
 *   - roda um número FIXO e conhecido de execuções (n_execucoes);
 *   - descarta um "aquecimento" (warmup) antes de medir, evitando ruído
 *     de cache e predição de desvios da primeira chamada;
 *   - mede com clock_gettime(CLOCK_MONOTONIC), que tem resolução muito
 *     maior que clock() (clock() no glibc/Linux costuma ter granularidade
 *     de ~10ms, o que é grosseiro demais para medir uma única geração de
 *     curva, que leva microssegundos);
 *   - reseta o buffer de pontos a cada execução (curva.quantidade_atual)
 *     antes de gerar de novo, então o total de pontos é sempre
 *     pontos_por_execucao * n_execucoes, nunca um acumulado "vazando"
 *     entre chamadas.
 * ========================================================================= */

typedef void (*FuncGeradoraCurva)(ponto, ponto, ponto, ponto, Pontos *, AABB *, ponto *);

typedef struct {
  const char *nome;
  double tempo_total_ms;
  double tempo_medio_ms;
  double pontos_por_execucao;
  double pontos_por_segundo;
  int n_execucoes;
} ResultadoBenchmark;

static ResultadoBenchmark benchmark_executar_curva(const char *nome, FuncGeradoraCurva gerar,
                                                   ponto P0, ponto P1, ponto P2, ponto P3,
                                                   int n_warmup, int n_execucoes){
  Pontos curva;
  AABB box;
  ponto ponto_final;
  ResultadoBenchmark resultado = {0};

  pontos_init(&curva, 256);

  for(int i = 0; i < n_warmup; i++){
    curva.quantidade_atual = 0;
    gerar(P0, P1, P2, P3, &curva, &box, &ponto_final);
  }

  long total_pontos = 0;
  double inicio = agora_ms_precisa();
  for(int i = 0; i < n_execucoes; i++){
    curva.quantidade_atual = 0;
    gerar(P0, P1, P2, P3, &curva, &box, &ponto_final);
    total_pontos += curva.quantidade_atual;
  }
  double fim = agora_ms_precisa();

  resultado.nome = nome;
  resultado.n_execucoes = n_execucoes;
  resultado.tempo_total_ms = fim - inicio;
  resultado.tempo_medio_ms = resultado.tempo_total_ms / (double)n_execucoes;
  resultado.pontos_por_execucao = (double)total_pontos / (double)n_execucoes;
  resultado.pontos_por_segundo = (resultado.tempo_total_ms > 0.0)
                                     ? (double)total_pontos / (resultado.tempo_total_ms / 1000.0)
                                     : 0.0;

  pontos_free(&curva);
  return resultado;
}

static void benchmark_executar_fluxo_topologico(int n_warmup, int n_execucoes){
  Pontos controles;
  BSplineTopologia spline;
  GeometriaCurvas geometria;
  AABB_vec boxes;
  pontos_init(&controles, 64);
  for(int i = 0; i < 64; i++){
    ponto controle = {{(float)i * 8.0f, sinf((float)i * 0.37f) * 120.0f, 1.0f}};
    pontos_push(&controles, controle);
  }
  if(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) != 0){
    pontos_free(&controles);
    return;
  }
  geometria_curvas_init(&geometria);
  aabb_vec_init(&boxes, 64);
  for(int i = 0; i < n_warmup; i++){
    geometria_curvas_atualizar_bspline_topologica(&geometria, &boxes, &spline);
  }
  double inicio = agora_ms_precisa();
  for(int i = 0; i < n_execucoes; i++){
    geometria_curvas_atualizar_bspline_topologica(&geometria, &boxes, &spline);
    AABBTREE *arvore = aabb_vec_para_arvore(&boxes);
    ponto alvo = bspline_topologia_avaliar(&spline, 30.5f);
    (void)bspline_topologia_picking_arvore(&spline, arvore, alvo, 1.0f);
    aabb_tree_free(arvore);
  }
  double tempo = agora_ms_precisa() - inicio;
  printf("[BENCH] B-spline topologica | execucoes=%-6d | geracao+AABB+arvore+picking=%8.5f ms | "
         "vertices=%d\n", n_execucoes, tempo / (double)n_execucoes,
         geometria.vertices.quantidade_atual);
  free(boxes.dados);
  geometria_curvas_free(&geometria);
  bspline_topologia_liberar(&spline);
  pontos_free(&controles);
}

static void imprimir_resultado_benchmark(const ResultadoBenchmark *r){
  printf("[BENCH] %-12s | execucoes=%-6d | tempo_total=%9.3f ms | "
         "tempo_medio=%8.5f ms | pontos/execucao=%8.2f | pontos/s=%12.0f\n",
         r->nome, r->n_execucoes, r->tempo_total_ms, r->tempo_medio_ms, r->pontos_por_execucao,
         r->pontos_por_segundo);
}

/* Ponto de entrada do benchmark. Chame explicitamente de main.c, por
 * exemplo atrás de uma flag de linha de comando (ex.: "--benchmark"),
 * antes de entrar no glutMainLoop(). Não é chamado automaticamente para
 * não interferir no fluxo interativo normal do programa. */
void performance_benchmark_executar(int n_execucoes){
  if(n_execucoes <= 0){
    n_execucoes = 1000;
  }
  const int n_warmup = n_execucoes / 10 > 10 ? n_execucoes / 10 : 10;

  /* Pontos de controle fixos e não-triviais (evitam casos degenerados
   * como pontos colineares, que gerariam curvas com poucos segmentos e
   * mascarariam o custo real do algoritmo). */
  ponto P0 = {{0.0f, 0.0f, 0.0f}};
  ponto P1 = {{50.0f, 120.0f, 0.0f}};
  ponto P2 = {{150.0f, -80.0f, 0.0f}};
  ponto P3 = {{200.0f, 40.0f, 0.0f}};

  printf("\n=== BENCHMARK DE GERACAO DE CURVAS (%d execucoes, %d warmup) ===\n", n_execucoes,
         n_warmup);

  ResultadoBenchmark r_hermite = benchmark_executar_curva("Hermite", gerar_curva_hermite, P0, P1,
                                                          P2, P3, n_warmup, n_execucoes);
  ResultadoBenchmark r_bezier =
      benchmark_executar_curva("Bezier", gerar_curva_bezier, P0, P1, P2, P3, n_warmup, n_execucoes);
  ResultadoBenchmark r_bspline = benchmark_executar_curva("B-Spline", gerar_curva_bspline, P0, P1,
                                                          P2, P3, n_warmup, n_execucoes);
  ResultadoBenchmark r_catmullrom = benchmark_executar_curva("Catmull-Rom", gerar_curva_catmullrom,
                                                             P0, P1, P2, P3, n_warmup, n_execucoes);

  imprimir_resultado_benchmark(&r_hermite);
  imprimir_resultado_benchmark(&r_bezier);
  imprimir_resultado_benchmark(&r_bspline);
  imprimir_resultado_benchmark(&r_catmullrom);
  benchmark_executar_fluxo_topologico(n_warmup, n_execucoes);
  printf("=== FIM DO BENCHMARK ===\n\n");
}
