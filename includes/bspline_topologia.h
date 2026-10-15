#ifndef BSPLINE_TOPOLOGIA_H
#define BSPLINE_TOPOLOGIA_H

#include "pontos.h"
#include "aabb.h"

typedef struct {
  unsigned grau;
  int fechado;
  int quantidade_controles_base;
  Pontos controles;
  float *nos;
  int quantidade_nos;
  int capacidade_nos;
  float periodo;
} BSplineTopologia;

/* Referência estável de um ponto selecionado durante alterações de topologia.
 * u_global é a fonte de verdade; span e t_local são apenas sua representação
 * no vetor de nós atual. */
typedef struct {
  float u_global;
  ponto ancora;
  int segmento_indice;
  float t_local;
} ReferenciaArrastoBSpline;

/* Cria a B-spline cardinal uniforme usada pelo modelo legado de controles. */
int bspline_topologia_inicializar_uniforme_aberta(BSplineTopologia *spline,
                                                   const Pontos *controles, unsigned grau);
int bspline_topologia_inicializar_uniforme_fechada(BSplineTopologia *spline,
                                                    const Pontos *controles, unsigned grau);
void bspline_topologia_liberar(BSplineTopologia *spline);
int bspline_topologia_copiar(const BSplineTopologia *origem, BSplineTopologia *destino);
ponto bspline_topologia_avaliar(const BSplineTopologia *spline, float u);
int bspline_topologia_obter_span(const BSplineTopologia *spline, int ordinal, float *inicio,
                                 float *fim, int *indice_no);
int bspline_topologia_arrastar(BSplineTopologia *spline, float u, ponto mouse, float rigidez);
ResultadoPicking bspline_topologia_picking(const BSplineTopologia *spline, ponto mouse,
                                           float tolerancia);
ResultadoPicking bspline_topologia_picking_arvore(const BSplineTopologia *spline,
                                                   const AABBTREE *arvore, ponto mouse,
                                                   float tolerancia);
int bspline_topologia_candidatos_insercao(const BSplineTopologia *spline, int span_central,
                                          float limiar, float *nos, int capacidade);
int bspline_topologia_candidatos_remocao(const BSplineTopologia *spline, int span_central,
                                         float limiar, float *nos, int capacidade);
int bspline_topologia_remapear_parametro(const BSplineTopologia *spline, float u, int *span,
                                         float *t_local);
int bspline_topologia_capturar_referencia_arrasto(const BSplineTopologia *spline, float u,
                                                  ReferenciaArrastoBSpline *referencia);
/* Atualiza span/t_local após Oslo ou Lyche-Morken e confirma que a curva
 * resultante ainda passa pela âncora dentro da tolerância informada. */
int bspline_topologia_remapear_referencia_arrasto(const BSplineTopologia *spline,
                                                   ReferenciaArrastoBSpline *referencia,
                                                   float tolerancia);

/* Insere todos os nós do lote de forma transacional e preserva a curva. */
int inserir_nos_oslo(BSplineTopologia *spline, const float *nos, int quantidade);

/* Remove candidatos em lote quando a reinserção tiver erro máximo <= tolerância. */
int remover_nos_lyche_morken(BSplineTopologia *spline, const float *nos, int quantidade,
                              float tolerancia);

#endif
