#include "../includes/benchmark_painel.h"

#include <float.h>

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
