#include "pontos.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

void pontos_init(Pontos *v, size_t cap0){
  size_t capacidade = cap0 ? cap0 : 20;
  if(capacidade > INT_MAX || capacidade > SIZE_MAX / sizeof(ponto)){
    fprintf(stderr, "Erro: capacidade inicial de pontos inválida\n");
    exit(1);
  }
  v->quantidade_atual = 0;
  v->quantidade_max = (int)capacidade;
  v->data = (ponto *)malloc(capacidade * sizeof(ponto));
  if(!v->data){
    printf("Erro: falha ao alocar memória para pontos\n");
    exit(1);
  }
}

static int pontos_ensure(Pontos *v, size_t needed){
  if(needed <= (size_t)v->quantidade_max){
    return 0;
  }
  if(needed > INT_MAX){
    return -1;
  }

  size_t newcap = (size_t)v->quantidade_max + (size_t)v->quantidade_max / 2;
  if(newcap < needed){
    newcap = needed;
  }
  if(newcap > INT_MAX || newcap > SIZE_MAX / sizeof(ponto)){
    return -1;
  }

  ponto *tmp = (ponto *)realloc(v->data, newcap * sizeof(ponto));
  if(!tmp){
    fprintf(stderr, "Erro: falha ao realocar memória para pontos\n");
    return -1;
  }
  v->data = tmp;
  v->quantidade_max = (int)newcap;
  return 0;
}

int pontos_push(Pontos *v, ponto p){
  if(!v || v->quantidade_atual < 0 || pontos_ensure(v, (size_t)v->quantidade_atual + 1) != 0){
    return -1;
  }
  v->data[v->quantidade_atual++] = p;
  return 0;
}

void pontos_free(Pontos *v){
  free(v->data);
  v->data = NULL;
  v->quantidade_atual = v->quantidade_max = 0;
}

float calcula_distancia(ponto P1, ponto P2){
  double dist_x = (double)(P2.point[0] - P1.point[0]);
  double dist_y = (double)(P2.point[1] - P1.point[1]);
  return sqrt(dist_x * dist_x + dist_y * dist_y);
}

float calcula_dist_ponto_segmento(ponto x, ponto A, ponto B){
  double abx = (double)(B.point[0] - A.point[0]);
  double aby = (double)(B.point[1] - A.point[1]);

  double apx = (double)(x.point[0] - A.point[0]);
  double apy = (double)(x.point[1] - A.point[1]);

  double ab2 = abx * abx + aby * aby;

  /* Caso degenerado: A e B são o mesmo ponto */
  if(ab2 == 0.0){
    double dx = (double)(x.point[0] - A.point[0]);
    double dy = (double)(x.point[1] - A.point[1]);
    return (float)sqrt(dx * dx + dy * dy);
  }

  double t = (apx * abx + apy * aby) / ab2;

  double px, py;

  if(t <= 0.0){
    px = (double)A.point[0];
    py = (double)A.point[1];
  } else if(t >= 1.0){
    px = (double)B.point[0];
    py = (double)B.point[1];
  } else {
    px = (double)A.point[0] + t * abx;
    py = (double)A.point[1] + t * aby;
  }

  double dx = (double)x.point[0] - px;
  double dy = (double)x.point[1] - py;

  return (float)sqrt(dx * dx + dy * dy);
}

ponto lerp_ponto(ponto A, ponto B, float t){
  ponto r;
  r.point[0] = A.point[0] + t * (B.point[0] - A.point[0]);
  r.point[1] = A.point[1] + t * (B.point[1] - A.point[1]);
  r.point[2] = 1.0f;
  return r;
}
