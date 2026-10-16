#include "benchmark_painel.h"

#include <assert.h>
#include <stdio.h>

int main(void){
  ConfiguracaoBenchmark configuracao;
  benchmark_configuracao_padrao(&configuracao);
  assert(configuracao.repeticoes == 100);
  assert(configuracao.warmup == 10);
  assert(configuracao.raio_arrasto == 30.0f);

  uint32_t primeiro = configuracao.semente;
  uint32_t segundo = configuracao.semente;
  for(int i = 0; i < 20; i++){
    assert(benchmark_proximo_aleatorio(&primeiro) == benchmark_proximo_aleatorio(&segundo));
  }

  ResultadoBenchmarkPainel resultado;
  benchmark_resultado_limpar(&resultado);
  benchmark_resultado_adicionar(&resultado, 1.0);
  benchmark_resultado_adicionar(&resultado, 3.0);
  assert(resultado.amostras == 2);
  assert(benchmark_resultado_media(&resultado) == 2.0);
  assert(resultado.minimo_ms == 1.0 && resultado.maximo_ms == 3.0);
  benchmark_resultado_limpar(&resultado);
  uint32_t amostra = configuracao.semente;
  assert(benchmark_painel_amostra_cpu(&configuracao, 0, BENCH_GERACAO, &amostra, &resultado) == 0);
  assert(resultado.amostras == 1);
  assert(benchmark_painel_amostra_cpu(&configuracao, 2, BENCH_PICKING, &amostra, &resultado) == 0);
  assert(resultado.amostras == 2);
  puts("test_benchmark_painel: OK");
  return 0;
}
