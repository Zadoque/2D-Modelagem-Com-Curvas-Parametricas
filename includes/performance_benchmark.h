#ifndef PERFORMANCE_BENCHMARK_H
#define PERFORMANCE_BENCHMARK_H

/*
 * Benchmark determinístico de geração de curvas.
 *
 * Ao contrário da instrumentação em performance.c (que usa
 * -finstrument-functions e mede o programa em uso interativo real),
 * esta função roda um número FIXO e conhecido de gerações de curva,
 * com pontos de controle fixos, e reporta médias e throughput.
 *
 * Uso sugerido em main.c, antes do glutMainLoop():
 *
 *   if (argc > 1 && strcmp(argv[1], "--benchmark") == 0) {
 *     performance_benchmark_executar(1000);
 *     return 0;
 *   }
 *
 * n_execucoes: quantas vezes cada curva será gerada (recomendado >= 1000
 * para diluir ruído de medição). Se <= 0, usa 1000 como padrão.
 */
void performance_benchmark_executar(int n_execucoes);

#endif /* PERFORMANCE_BENCHMARK_H */
