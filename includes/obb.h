#ifndef OBB_H
#define OBB_H

#include "bspline_topologia.h"

typedef struct {
  ponto centro;
  ponto eixo[2];
  float extents[2];
  int segmento_indice;
} OBB;

typedef struct {
  OBB caixa;
  float comprimento;
  int pai;
  int esquerda;
  int direita;
  int segmento_indice;
  unsigned revisao;
} OBBNo;

typedef struct {
  OBBNo *nos;
  int quantidade_nos;
  int capacidade_nos;
  int raiz;
  int *folha_por_segmento;
  int quantidade_folhas;
  unsigned revisao;
} OBBArvore;

typedef struct {
  int segmento_a;
  int segmento_b;
} OBBCandidato;

typedef struct {
  enum {
    CONTATO_TRANSVERSAL,
    CONTATO_TANGENCIAL,
    CONTATO_SOBREPOSTO
  } tipo;
  int segmento_a;
  int segmento_b;
  float u_a;
  float u_b;
  ponto ponto;
} InterseccaoBSpline;

void obb_arvore_init(OBBArvore *arvore);
void obb_arvore_liberar(OBBArvore *arvore);
/* Orientação calculada a partir de cinco pontos da curva; as extensões são
 * ajustadas com uma amostragem adicional da própria curva para conter o span. */
int obb_bspline_elementar(const BSplineTopologia *spline, int segmento, OBB *resultado,
                          float *comprimento);
int obb_arvore_construir_bspline(OBBArvore *arvore, const BSplineTopologia *spline);
int obb_arvore_refit_bspline(OBBArvore *arvore, const BSplineTopologia *spline,
                              const int *segmentos, int quantidade);
/* SAT em 2D: sobreposição de caixas, não interseção confirmada das curvas. */
int obb_intersecta_sat(const OBB *a, const OBB *b);
/* Retorna a quantidade total de pares folha candidatos. Quando capacidade for
 * menor que o total, apenas os primeiros pares são gravados. */
int obb_arvore_candidatos_autointerferencia(const OBBArvore *arvore,
                                            OBBCandidato *candidatos, int capacidade);
/* Confirma candidatos por aproximação adaptativa, dentro da tolerância em
 * coordenadas do mundo. Não retorna o contato trivial entre spans vizinhos. */
int obb_bspline_autointerseccoes(const OBBArvore *arvore, const BSplineTopologia *spline,
                                 float tolerancia, InterseccaoBSpline *resultado, int capacidade);
/* Variante incremental: consulta somente os pares que contêm ao menos um dos
 * spans informados. A travessia ainda usa a árvore inteira como índice, mas a
 * aproximação adaptativa final não é executada para pares não afetados. */
int obb_bspline_autointerseccoes_locais(const OBBArvore *arvore,
                                        const BSplineTopologia *spline,
                                        const int *segmentos, int quantidade_segmentos,
                                        float tolerancia, InterseccaoBSpline *resultado,
                                        int capacidade);

#endif
