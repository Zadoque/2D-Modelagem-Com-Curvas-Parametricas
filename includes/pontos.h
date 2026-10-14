#ifndef CURVA_H
#define CURVA_H

#include <stdlib.h>

// Matrizes constantes para curvas
extern const float HERMITE[4][4];
extern const float BEZIER[4][4];
extern const float BSPLINE[4][4];
extern const float CATMULLROM[4][4];

// Tipos para curvas e pontos
typedef enum {
  CURVE_HERMITE,
  CURVE_BEZIER,
  CURVE_BSPLINE,
  CURVE_CATMULLROM
} CurveType;

typedef struct {
    int   segmento_indice;  // qual segmento foi clicado (-1 se nenhum)
    float t;                // parâmetro no segmento [0,1]
    float distancia;        // distância real ao ponto — útil para desempate
} ResultadoPicking;

typedef struct {
  float point[3];
} ponto;

typedef struct {
  int quantidade_atual;
  int quantidade_max;
  ponto *data;
} Pontos;

// Funções relacionadas a Pontos
void pontos_init(Pontos *v, size_t cap0);
int pontos_push(Pontos *v, ponto p);
void pontos_free(Pontos *v);
void esvazia_points(Pontos *v);
double raiz_quadrada_newton(double n);
/**
* Recebe dois pontos e retorna a distância entre eles como float
* @param ponto P1 -> primeiro ponto
* @param ponto P2 -> segundo ponto
*/
float calcula_distancia(ponto P1, ponto P2);
float calcula_dist_ponto_segmento(ponto x, ponto A, ponto B); 
ponto lerp_ponto(ponto A, ponto B, float t);
#endif
