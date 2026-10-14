#ifndef VETOR_H
#define VETOR_H

#include <stdlib.h>
#include <math.h>

/* Fator de crescimento 1.5x (não linear, não quadrático) */
#define VEC_GROWTH_FACTOR 1.5

/*
 * Limiar de encolhimento: 1.5^4 ≈ 5.06.
 * Se a capacidade máxima for maior que a atual multiplicada por esse valor,
 * significa que há folga de mais de 4 realocações de crescimento,
 * então compactamos o vetor.
 */
#define VEC_SHRINK_THRESHOLD 5.0625

typedef struct {
    float *vector;
    int quantidade_atual;
    int quantidade_max;
} vec;

/* Inicializa o vetor dinâmico */
void vec_init(vec *v);

/* Libera a memória alocada e zera os campos */
void vec_free(vec *v);

/*
 * Adiciona um elemento ao final do vetor.
 * Retorna 0 em sucesso ou -1 em caso de falha de alocação.
 */
int vec_push(float x, vec *v);

/*
 * Verifica se há capacidade disponível.
 * Se quantidade_atual == quantidade_max, realoca com fator 1.5x.
 */
int vec_ensure(vec *v);

/*
 * Reduz a capacidade máxima se houver folga excessiva
 * (mais de 4 realocações de 1.5x acima do necessário).
 */
void vec_shrink(vec *v);

#endif
