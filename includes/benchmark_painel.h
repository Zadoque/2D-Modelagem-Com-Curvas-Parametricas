#ifndef BENCHMARK_PAINEL_H
#define BENCHMARK_PAINEL_H

#include <stdint.h>

typedef enum {
  BENCH_GERACAO = 1 << 0,
  BENCH_DESENHO = 1 << 1,
  BENCH_PICKING = 1 << 2,
  BENCH_ARRASTO = 1 << 3,
  BENCH_AUTOINTERFERENCIA = 1 << 4
} TipoBenchmark;

typedef struct {
  unsigned mascara_curvas;
  unsigned mascara_testes;
  int repeticoes;
  int warmup;
  float rigidez;
  int posicao_densidade;
  int quantidade_controles;
  int anti_autointerferencia;
  float raio_arrasto;
  uint32_t semente;
} ConfiguracaoBenchmark;

typedef struct {
  double soma_ms;
  double minimo_ms;
  double maximo_ms;
  int amostras;
  int insercoes;
  int remocoes;
  double soma_insercao_ms;
  double soma_remocao_ms;
} ResultadoBenchmarkPainel;

enum { BENCH_CURVAS = 4, BENCH_TESTES = 5 };

/* Executa uma única amostra CPU isolada da cena interativa. O tipo de curva
 * segue o enum Curva (Hermite, Catmull-Rom, B-spline e Bezier). Desenho é
 * medido pelo adaptador OpenGL, pois exige contexto gráfico. */
int benchmark_painel_amostra_cpu(const ConfiguracaoBenchmark *configuracao, int curva,
                                 TipoBenchmark tipo, uint32_t *aleatorio,
                                 ResultadoBenchmarkPainel *resultado);

void benchmark_configuracao_padrao(ConfiguracaoBenchmark *configuracao);
uint32_t benchmark_proximo_aleatorio(uint32_t *estado);
float benchmark_aleatorio_intervalo(uint32_t *estado, float minimo, float maximo);
void benchmark_resultado_limpar(ResultadoBenchmarkPainel *resultado);
void benchmark_resultado_adicionar(ResultadoBenchmarkPainel *resultado, double ms);
double benchmark_resultado_media(const ResultadoBenchmarkPainel *resultado);

#endif
