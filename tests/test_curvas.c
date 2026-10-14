#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
#include "curvas/hermite.h"
#include "curvas/curva_generica.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

typedef ponto (*Avaliador)(ponto, ponto, ponto, ponto, float);
typedef void (*Gerador)(ponto, ponto, ponto, ponto, Pontos *, AABB *);

static ponto avaliar_hermite_local(ponto P0, ponto P1, ponto P2, ponto P3,
                                    float t) {
    ponto T1, T2;
    calcular_tangentes_hermite(P0, P1, P2, P3, &T1, &T2);
    return calcular_ponto_hermite(P1, P2, T1, T2, t);
}

static int dentro(float valor, float minimo, float maximo) {
    return valor >= minimo && valor <= maximo;
}

static void verificar_caixa(const char *nome, Gerador gerar, Avaliador avaliar,
                            ponto P0, ponto P1, ponto P2, ponto P3) {
    Pontos curva;
    AABB box = {0};
    pontos_init(&curva, 8);
    gerar(P0, P1, P2, P3, &curva, &box);

    for (int i = 0; i <= 10000; i++) {
        float t = (float)i / 10000.0f;
        ponto p = avaliar(P0, P1, P2, P3, t);
        int contido = t <= 0.5f
            ? dentro(p.point[0], box.esq_x_min, box.esq_x_max) &&
              dentro(p.point[1], box.esq_y_min, box.esq_y_max)
            : dentro(p.point[0], box.dir_x_min, box.dir_x_max) &&
              dentro(p.point[1], box.dir_y_min, box.dir_y_max);
        if (!contido) {
            fprintf(stderr, "%s fora da AABB em t=%f: (%f, %f)\n",
                    nome, t, p.point[0], p.point[1]);
        }
        assert(contido);
        assert(dentro(p.point[0], box.x_min, box.x_max));
        assert(dentro(p.point[1], box.y_min, box.y_max));
    }

    pontos_free(&curva);
}

int main(void) {
    ponto P0 = {{-300.0f,  180.0f, 1.0f}};
    ponto P1 = {{ -40.0f, -260.0f, 1.0f}};
    ponto P2 = {{ 170.0f,  330.0f, 1.0f}};
    ponto P3 = {{ 420.0f, -140.0f, 1.0f}};

    verificar_caixa("Bezier", gerar_curva_bezier, calcular_ponto_bezier,
                    P0, P1, P2, P3);
    verificar_caixa("B-Spline", gerar_curva_bspline, calcular_ponto_bspline,
                    P0, P1, P2, P3);
    verificar_caixa("Catmull-Rom", gerar_curva_catmullrom,
                    calcular_ponto_catmullrom, P0, P1, P2, P3);
    verificar_caixa("Hermite", gerar_curva_hermite, avaliar_hermite_local,
                    P0, P1, P2, P3);

    Pontos hermite;
    AABB box = {0};
    pontos_init(&hermite, 8);
    gerar_curva_hermite(P0, P1, P2, P3, &hermite, &box);
    assert(fabsf(hermite.data[0].point[0] - P1.point[0]) < 1e-5f);
    assert(fabsf(hermite.data[0].point[1] - P1.point[1]) < 1e-5f);
    assert(fabsf(hermite.data[hermite.quantidade_atual - 1].point[0] - P2.point[0]) < 1e-5f);
    assert(fabsf(hermite.data[hermite.quantidade_atual - 1].point[1] - P2.point[1]) < 1e-5f);
    pontos_free(&hermite);

    CurvaSubdivisaoConfig limitada = curva_subdivisao_config_padrao();
    limitada.tolerancia_geometrica = 1e-12f;
    limitada.profundidade_maxima = 1;
    limitada.max_nos_geracao = 1;
    curva_subdivisao_definir_config(limitada);
    pontos_init(&hermite, 8);
    gerar_curva_bezier(P0, P1, P2, P3, &hermite, &box);
    assert(hermite.quantidade_atual <= 7);
    pontos_free(&hermite);

    limitada.profundidade_maxima = 100;
    curva_subdivisao_definir_config(limitada);
    assert(curva_subdivisao_obter_config().profundidade_maxima == 20);
    curva_subdivisao_definir_config(curva_subdivisao_config_padrao());

    puts("test_curvas: OK");
    return 0;
}
