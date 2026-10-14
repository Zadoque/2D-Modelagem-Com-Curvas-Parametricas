#include "aabb.h"
#include "curvas/ultis/ultis.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static ponto p(float x, float y) { ponto r = {{x, y, 1.0f}}; return r; }
static AABB box(float xmin, float ymin, float xmax, float ymax, int indice) {
    AABB b = {0}; b.x_min=xmin; b.y_min=ymin; b.x_max=xmax; b.y_max=ymax;
    b.segmento_indice=indice; return b;
}

static int contar_folhas(AABBTREE *n) {
    if (!n) return 0;
    if (!n->esquerda && !n->direita) return 1;
    assert(n->esquerda && n->direita);
    assert(n->box.x_min <= n->esquerda->box.x_min);
    assert(n->box.y_min <= n->esquerda->box.y_min);
    assert(n->box.x_max >= n->direita->box.x_max || n->box.x_max >= n->esquerda->box.x_max);
    return contar_folhas(n->esquerda) + contar_folhas(n->direita);
}

int main(void) {
    AABB b = box(-2, -1, 3, 5, 0);
    assert(ponto_dentro_aabb(b, -2, -1));
    assert(ponto_dentro_aabb(b, 0, 0));
    assert(!ponto_dentro_aabb(b, 3.1f, 0));
    assert(dist_mouse_aabb(p(0, 0), b) == 0.0f);
    assert(fabsf(dist_mouse_aabb(p(6, 9), b) - 5.0f) < 1e-5f);
    assert(fabsf(calcula_area_box(&b) - 30.0f) < 1e-5f);

    AABB_vec v; aabb_vec_init(&v, 1);
    assert(aabb_vec_para_arvore(&v) == NULL);
    assert(aabb_vec_push(&v, box(0, 0, 1, 1, 0)) == 0);
    assert(aabb_vec_push(&v, box(2, -1, 4, 2, 1)) == 0);
    assert(aabb_vec_push(&v, box(-3, 3, -2, 5, 2)) == 0);
    AABBTREE *raiz = aabb_vec_para_arvore(&v);
    assert(raiz && contar_folhas(raiz) == 3);
    assert(raiz->box.x_min == -3 && raiz->box.x_max == 4);
    assert(raiz->box.y_min == -1 && raiz->box.y_max == 5);
    assert(raiz->altura == 2);
    aabb_tree_free(raiz);
    free(v.dados);
    puts("test_aabb: OK");
    return 0;
}
