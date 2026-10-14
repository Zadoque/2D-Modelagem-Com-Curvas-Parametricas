#include "../../../includes/curvas/ultis/ultis.h"
#include <float.h>
#include <math.h>


 float dist_mouse_aabb(ponto mouse, AABB box) {
    float dx = fmaxf(0.0f, fmaxf(box.x_min - mouse.point[0], mouse.point[0] - box.x_max));
    float dy = fmaxf(0.0f, fmaxf(box.y_min - mouse.point[1], mouse.point[1] - box.y_max));
    return sqrtf(dx*dx + dy*dy);
}


 float dist_mouse_aabb_left(ponto mouse, AABB box){
    float dx = fmaxf(0.0f, fmaxf(box.esq_x_min - mouse.point[0], mouse.point[0] - box.esq_x_max));
    float dy = fmaxf(0.0f, fmaxf(box.esq_y_min - mouse.point[1], mouse.point[1] - box.esq_y_max));
    return sqrtf(dx*dx + dy*dy);
}

 float dist_mouse_aabb_right(ponto mouse, AABB box){
    float dx = fmaxf(0.0f, fmaxf(box.dir_x_min - mouse.point[0], mouse.point[0] - box.dir_x_max));
    float dy = fmaxf(0.0f, fmaxf(box.dir_y_min - mouse.point[1], mouse.point[1] - box.dir_y_max));
    return sqrtf(dx*dx + dy*dy);
}

static void incluir_ponto_aabb(AABB *box, ponto p) {
    box->x_min = fminf(box->x_min, p.point[0]);
    box->y_min = fminf(box->y_min, p.point[1]);
    box->x_max = fmaxf(box->x_max, p.point[0]);
    box->y_max = fmaxf(box->y_max, p.point[1]);
}

void incluir_intervalo_curva_cubica_aabb(
    AABB *box, ponto P0, ponto P1, ponto P2, ponto P3,
    float t0, float t1, CurvaCubicaEval avaliar,
    CurvaCubicaDerivada derivar) {
    float fator = (t1 - t0) / 3.0f;
    ponto q0 = avaliar(P0, P1, P2, P3, t0);
    ponto q3 = avaliar(P0, P1, P2, P3, t1);
    ponto d0 = derivar(P0, P1, P2, P3, t0);
    ponto d1 = derivar(P0, P1, P2, P3, t1);
    ponto q1 = q0;
    ponto q2 = q3;

    q1.point[0] += fator * d0.point[0];
    q1.point[1] += fator * d0.point[1];
    q2.point[0] -= fator * d1.point[0];
    q2.point[1] -= fator * d1.point[1];

    /* O fecho convexo destes controles Bezier contém todo o trecho. */
    incluir_ponto_aabb(box, q0);
    incluir_ponto_aabb(box, q1);
    incluir_ponto_aabb(box, q2);
    incluir_ponto_aabb(box, q3);

    /* Protege a propriedade conservadora contra arredondamento em float. */
    float magnitude = fmaxf(1.0f, fmaxf(fmaxf(fabsf(box->x_min), fabsf(box->x_max)),
                                       fmaxf(fabsf(box->y_min), fabsf(box->y_max))));
    float margem = 16.0f * FLT_EPSILON * magnitude;
    box->x_min -= margem;
    box->y_min -= margem;
    box->x_max += margem;
    box->y_max += margem;
}
