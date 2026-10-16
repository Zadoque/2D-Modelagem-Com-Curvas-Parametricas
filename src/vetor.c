#include "vetor.h"
#include <stdlib.h>
/* Capacidade mínima para evitar realocações excessivas */
#define VEC_MIN_CAP 4

void vec_init(vec *v){
  v->vector = NULL;
  v->quantidade_atual = 0;
  v->quantidade_max = 0;
}

void vec_free(vec *v){
  free(v->vector);
  v->vector = NULL;
  v->quantidade_atual = 0;
  v->quantidade_max = 0;
}

int vec_ensure(vec *v){
  if(v->quantidade_atual < v->quantidade_max){
    return 0;
  }

  int nova_cap;
  if(v->quantidade_max == 0){
    nova_cap = VEC_MIN_CAP;
  } else {
    nova_cap = (int)(v->quantidade_max * VEC_GROWTH_FACTOR);
    /* Garante que a capacidade cresça de fato (evita travamento em int pequeno) */
    if(nova_cap <= v->quantidade_max){
      nova_cap = v->quantidade_max + 1;
    }
  }

  float *novo = realloc(v->vector, nova_cap * sizeof(float));
  if(novo == NULL){
    /* Falha de alocação: mantém estado anterior */
    return -1;
  }

  v->vector = novo;
  v->quantidade_max = nova_cap;
  return 0;
}

int vec_push(float x, vec *v){
  if(vec_ensure(v) != 0 || v->vector == NULL || v->quantidade_atual >= v->quantidade_max){
    return -1;
  }

  v->vector[v->quantidade_atual] = x;
  v->quantidade_atual++;
  return 0;
}

void vec_shrink(vec *v){
  if(v->quantidade_atual == 0){
    vec_free(v);
    return;
  }

  /*
   * Se a capacidade máxima é tão grande que, partindo da quantidade_atual,
   * seriam necessários mais de 4 crescimentos de 1.5x para alcançá-la,
   * reduzimos para ~1.5x a quantidade_atual.
   *
   * Exemplo: atual=50, max=2500.
   * 50 * 1.5^4 ≈ 253. 2500 > 253, então reduzimos.
   */
  int limite = (int)(v->quantidade_atual * VEC_SHRINK_THRESHOLD);
  if(v->quantidade_max <= limite){
    return;
  }

  int nova_cap = (int)(v->quantidade_atual * VEC_GROWTH_FACTOR);
  if(nova_cap <= v->quantidade_atual){
    nova_cap = v->quantidade_atual + 1;
  }
  if(nova_cap < VEC_MIN_CAP){
    nova_cap = VEC_MIN_CAP;
  }

  float *novo = realloc(v->vector, nova_cap * sizeof(float));
  if(novo != NULL){
    v->vector = novo;
    v->quantidade_max = nova_cap;
  }
}
