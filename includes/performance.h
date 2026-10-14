#ifndef PERFORMANCE_H
#define PERFORMANCE_H
#include "pontos.h"
#include "vetor.h"
#include "menu.h"
#include "aabb.h"
#include "opengl.h"
#include "curvas/hermite.h"
#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
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

// Métricas da etapa de geração/desenho.
typedef struct {
  double tempo_geracao_ms;
  double tempo_desenho_ms;
  size_t pontos_gerados;
  size_t buffer_curva_uso;
  size_t buffer_curva_capacidade;
  size_t buffer_controle_uso;
  size_t buffer_controle_capacidade;
} MetricasPerformance;

/* Inicia uma nova medição da curva exibida, sem carregar o frame anterior. */
void performance_definir_buffers(const Pontos *controles,
                                 const Pontos *segmento_temporario);
void performance_iniciar_medicao(void);
void performance_benchmark_executar(int n_execucoes);

#endif /* PERFORMANCE_H */
