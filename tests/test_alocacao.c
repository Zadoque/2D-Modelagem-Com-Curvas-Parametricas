#include "aabb.h"
#include "pontos.h"
#include "vetor.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static int falhar_realloc;

void *__real_realloc(void *ptr, size_t tamanho);

void *__wrap_realloc(void *ptr, size_t tamanho) {
    if (falhar_realloc) return NULL;
    return __real_realloc(ptr, tamanho);
}

int main(void) {
    ponto p = {{1.0f, 2.0f, 1.0f}};

    Pontos pontos;
    pontos_init(&pontos, 1);
    assert(pontos_push(&pontos, p) == 0);
    falhar_realloc = 1;
    assert(pontos_push(&pontos, p) == -1);
    assert(pontos.quantidade_atual == 1);
    assert(pontos.quantidade_max == 1);
    falhar_realloc = 0;
    pontos_free(&pontos);

    AABB_vec caixas;
    aabb_vec_init(&caixas, 1);
    AABB box = {0};
    assert(aabb_vec_push(&caixas, box) == 0);
    falhar_realloc = 1;
    assert(aabb_vec_push(&caixas, box) == -1);
    assert(caixas.quantidade == 1);
    assert(caixas.capacidade == 1);
    falhar_realloc = 0;
    free(caixas.dados);

    vec valores;
    vec_init(&valores);
    for (int i = 0; i < 4; i++) assert(vec_push((float)i, &valores) == 0);
    falhar_realloc = 1;
    assert(vec_push(4.0f, &valores) == -1);
    assert(valores.quantidade_atual == 4);
    assert(valores.quantidade_max == 4);
    falhar_realloc = 0;
    vec_free(&valores);

    puts("test_alocacao: OK");
    return 0;
}
