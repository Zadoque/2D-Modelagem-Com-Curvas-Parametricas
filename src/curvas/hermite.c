#include "../../includes/curvas/hermite.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifndef FLT_MAX
#define FLT_MAX 99999999.0f
#endif

static const float MATRIZ_HERMITE[4][4] = {
    { 2.0f, -2.0f,  1.0f,  1.0f},
    {-3.0f,  3.0f, -2.0f, -1.0f},
    { 0.0f,  0.0f,  1.0f,  0.0f},
    { 1.0f,  0.0f,  0.0f,  0.0f}};

// Hermite recebe: P1, P2 (pontos), T1, T2 (tangentes)
ponto calcular_ponto_hermite(ponto P1, ponto P2, ponto T1, ponto T2, float t) {
    float t2 = t*t, t3 = t2*t;
    float vetor_t[4] = {t3, t2, t, 1.0f};
    float c[4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            c[i] += vetor_t[j] * MATRIZ_HERMITE[j][i];
    ponto r;
    r.point[0] = c[0]*P1.point[0] + c[1]*P2.point[0] + c[2]*T1.point[0] + c[3]*T2.point[0];
    r.point[1] = c[0]*P1.point[1] + c[1]*P2.point[1] + c[2]*T1.point[1] + c[3]*T2.point[1];
    return r;
}

void calcular_tangentes_hermite(ponto P0, ponto P1, ponto P2, ponto P3,
                                 ponto *T1, ponto *T2) {
    T1->point[0] = P2.point[0] - P0.point[0];
    T1->point[1] = P2.point[1] - P0.point[1];
    T2->point[0] = P3.point[0] - P1.point[0];
    T2->point[1] = P3.point[1] - P1.point[1];
}

// Wrapper para subdivide: usa os 4 clicks para derivar P1,P2,T1,T2
static ponto hermite_eval(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    ponto T1, T2;
    calcular_tangentes_hermite(P0, P1, P2, P3, &T1, &T2);
    return calcular_ponto_hermite(P1, P2, T1, T2, t);
}

static ponto derivada_hermite(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    ponto T1, T2;
    calcular_tangentes_hermite(P0, P1, P2, P3, &T1, &T2);
    float t2 = t * t;
    float pesos[4] = {
        6.0f*t2 - 6.0f*t,
       -6.0f*t2 + 6.0f*t,
        3.0f*t2 - 4.0f*t + 1.0f,
        3.0f*t2 - 2.0f*t
    };
    ponto controles[4] = {P1, P2, T1, T2};
    ponto r = {{0.0f, 0.0f, 0.0f}};
    for (int i = 0; i < 4; i++) {
        r.point[0] += pesos[i] * controles[i].point[0];
        r.point[1] += pesos[i] * controles[i].point[1];
    }
    return r;
}

static void subdivide_hermite(ponto P0, ponto P1, ponto P2, ponto P3,
                               Pontos *curva, ponto A, ponto B,
                               float t0, float t1, float tol, AABB *box) {
    float tmid = (t0+t1)/2.0f;
    ponto M = hermite_eval(P0,P1,P2,P3, tmid);
    if (calcula_dist_ponto_segmento(M, A, B) < tol) {
        pontos_push(curva, M);
        box->x_min = fminf(box->x_min, M.point[0]); box->y_min = fminf(box->y_min, M.point[1]);
        box->x_max = fmaxf(box->x_max, M.point[0]); box->y_max = fmaxf(box->y_max, M.point[1]);
    } else {
        subdivide_hermite(P0,P1,P2,P3, curva, A, M, t0, tmid, tol, box);
        subdivide_hermite(P0,P1,P2,P3, curva, M, B, tmid, t1, tol, box);
    }
}

void gerar_curva_hermite(ponto P0, ponto P1, ponto P2, ponto P3,
                          Pontos *curva, AABB *box) {
    curva->quantidade_atual = 0;
    ponto A = hermite_eval(P0, P1, P2, P3, 0.0f);
    ponto M = hermite_eval(P0, P1, P2, P3, 0.5f);
    ponto B = hermite_eval(P0, P1, P2, P3, 1.0f);

    // Inicializa caixas temporárias com os limites dos extremos
    AABB box_esq; reset_box(&box_esq);
    incluir_intervalo_curva_cubica_aabb(&box_esq, P0, P1, P2, P3, 0.0f, 0.5f,
                                        hermite_eval, derivada_hermite);

    AABB box_dir; reset_box(&box_dir);
    incluir_intervalo_curva_cubica_aabb(&box_dir, P0, P1, P2, P3, 0.5f, 1.0f,
                                        hermite_eval, derivada_hermite);
     // Subdivide as metades direcionando os resultados para as caixas temporárias
    pontos_push(curva, A);
    subdivide_hermite(P0, P1, P2, P3, curva, A, M, 0.0f, 0.5f, 0.1f, &box_esq);
    pontos_push(curva, M);
    subdivide_hermite(P0, P1, P2, P3, curva, M, B, 0.5f, 1.0f, 0.1f, &box_dir);
    pontos_push(curva, B);

    // Salva as metades nos campos corretos da struct principal
    box->esq_x_min = box_esq.x_min; box->esq_y_min = box_esq.y_min;
    box->esq_x_max = box_esq.x_max; box->esq_y_max = box_esq.y_max;
    
    box->dir_x_min = box_dir.x_min; box->dir_y_min = box_dir.y_min;
    box->dir_x_max = box_dir.x_max; box->dir_y_max = box_dir.y_max;

    // A caixa global é a união das duas metades
    box->x_min = fminf(box_esq.x_min, box_dir.x_min);
    box->y_min = fminf(box_esq.y_min, box_dir.y_min);
    box->x_max = fmaxf(box_esq.x_max, box_dir.x_max);
    box->y_max = fmaxf(box_esq.y_max, box_dir.y_max);
}



static void criar_box_picking_h(ponto P0, ponto P1, ponto P2, ponto P3,
                                  float t0, float t1, AABB *box) {
    incluir_intervalo_curva_cubica_aabb(box, P0, P1, P2, P3, t0, t1,
                                        hermite_eval, derivada_hermite);
}

static ResultadoPicking subdivide_picking_hermite(
        ponto P0, ponto P1, ponto P2, ponto P3, ponto mouse,
        float t0, float t1, float melhor_dist, float tolerancia_t) {
    ResultadoPicking sem = { -1, 0.0f, FLT_MAX };
    if ((t1-t0) < tolerancia_t) {
        float tmid = (t0+t1)/2.0f;
        ponto p = hermite_eval(P0,P1,P2,P3, tmid);
        float dx = p.point[0]-mouse.point[0], dy = p.point[1]-mouse.point[1];
        ResultadoPicking r = { 0, tmid, sqrtf(dx*dx+dy*dy) }; return r;
    }
    float tmid = (t0+t1)/2.0f;
    AABB be; reset_box(&be); criar_box_picking_h(P0,P1,P2,P3, t0, tmid, &be);
    AABB bd; reset_box(&bd); criar_box_picking_h(P0,P1,P2,P3, tmid, t1, &bd);
    float de = dist_mouse_aabb(mouse, be), dd = dist_mouse_aabb(mouse, bd);
    ResultadoPicking melhor = sem;
    if (de <= dd) {
        if (de < melhor_dist) melhor = subdivide_picking_hermite(P0,P1,P2,P3, mouse, t0, tmid, melhor_dist, tolerancia_t);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (dd < lim) { ResultadoPicking r = subdivide_picking_hermite(P0,P1,P2,P3, mouse, tmid, t1, lim, tolerancia_t); if (r.distancia < melhor.distancia) melhor = r; }
    } else {
        if (dd < melhor_dist) melhor = subdivide_picking_hermite(P0,P1,P2,P3, mouse, tmid, t1, melhor_dist, tolerancia_t);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (de < lim) { ResultadoPicking r = subdivide_picking_hermite(P0,P1,P2,P3, mouse, t0, tmid, lim, tolerancia_t); if (r.distancia < melhor.distancia) melhor = r; }
    }
    return melhor;
}

static void pesos_hermite(float t, float B[4]) {
    float t2 = t*t, t3 = t2*t;
    B[0] = 2.0f*t3 - 3.0f*t2 + 1.0f;
    B[1] = -2.0f*t3 + 3.0f*t2;
    B[2] = t3 - 2.0f*t2 + t;
    B[3] = t3 - t2;
}

void arrastar_ponto_hermite(Pontos *clicks, int seg, float ti, ponto mouse) {
    if (!clicks || clicks->quantidade_atual < 4) return;
    int n = clicks->quantidade_atual;
    int i0 = (seg+0)%n, i1 = (seg+1)%n, i2 = (seg+2)%n, i3 = (seg+3)%n;
    ponto P0 = clicks->data[i0], P1 = clicks->data[i1];
    ponto P2 = clicks->data[i2], P3 = clicks->data[i3];
    ponto T1, T2; calcular_tangentes_hermite(P0,P1,P2,P3, &T1, &T2);
    ponto C = calcular_ponto_hermite(P1, P2, T1, T2, ti);
    float ex = mouse.point[0]-C.point[0], ey = mouse.point[1]-C.point[1];
    float B[4]; pesos_hermite(ti, B);
    float soma = B[0]*B[0]+B[1]*B[1]+B[2]*B[2]+B[3]*B[3];
    if (soma < 1e-8f) return;
    float fx = ex/soma, fy = ey/soma;
    // Hermite interpola P1 e P2 — arrasta os pontos P1 e P2 diretamente
    clicks->data[i1].point[0] += fx*B[0]; clicks->data[i1].point[1] += fy*B[0];
    clicks->data[i2].point[0] += fx*B[1]; clicks->data[i2].point[1] += fy*B[1];
    // Tangentes afetam indiretamente via P0 e P3
    clicks->data[i0].point[0] -= fx*B[2]; clicks->data[i0].point[1] -= fy*B[2];
    clicks->data[i3].point[0] += fx*B[3]; clicks->data[i3].point[1] += fy*B[3];
}

ResultadoPicking picking_hermite(AABBTREE *arvore, Pontos *clicks,
                                  ponto mouse, float tolerancia, float melhor_dist) {
    ResultadoPicking sem_resultado = { -1, 0.0f, FLT_MAX };
    if (!arvore) return sem_resultado;
    float dc = dist_mouse_aabb(mouse, arvore->box);
      if (dc > tolerancia || (melhor_dist != FLT_MAX && dc > melhor_dist)) return sem_resultado;

    if (arvore->esquerda == NULL && arvore->direita == NULL) {
        int i = arvore->box.segmento_indice, n = clicks->quantidade_atual;
        ponto P0 = clicks->data[i%n], P1 = clicks->data[(i+1)%n];
        ponto P2 = clicks->data[(i+2)%n], P3 = clicks->data[(i+3)%n];
        float dist_metade_esq = dist_mouse_aabb_left( mouse, arvore->box);
        float dist_metade_dir = dist_mouse_aabb_right(mouse, arvore->box);
        ResultadoPicking resultado = sem_resultado;

        // Explora primeiro o lado mais próximo
        if (dist_metade_esq <= dist_metade_dir) {
            // Tenta a esquerda
            if (dist_metade_esq <= tolerancia) {
                resultado = subdivide_picking_hermite(P0, P1, P2, P3, mouse, 0.0f, 0.5f, melhor_dist, 0.001f);
            }
            
            // Tenta a direita usando o resultado da esquerda como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_dir < limite && dist_metade_dir <= tolerancia) {
                ResultadoPicking r_dir = subdivide_picking_hermite(P0, P1, P2, P3, mouse, 0.5f, 1.0f, limite, 0.001f);
                if (r_dir.distancia < resultado.distancia) {
                    resultado = r_dir;
                }
            }
        } else {
            // Tenta a direita
            if (dist_metade_dir <= tolerancia) {
                resultado = subdivide_picking_hermite(P0, P1, P2, P3, mouse, 0.5f, 1.0f, melhor_dist, 0.001f);
            }
            
            // Tenta a esquerda usando o resultado da direita como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_esq < limite && dist_metade_esq <= tolerancia) {
                ResultadoPicking r_esq = subdivide_picking_hermite(P0, P1, P2, P3, mouse, 0.0f, 0.5f, limite, 0.001f);
                if (r_esq.distancia < resultado.distancia) {
                    resultado = r_esq;
                }
            }
        }
          
        // Associa o índice real se a distância final for válida
        resultado.segmento_indice = (resultado.distancia <= tolerancia) ? i : -1;
        return resultado;
    }
    float de = arvore->esquerda ? dist_mouse_aabb(mouse, arvore->esquerda->box) : FLT_MAX;
    float dd = arvore->direita  ? dist_mouse_aabb(mouse, arvore->direita->box)  : FLT_MAX;
    ResultadoPicking melhor = sem_resultado;
    if (de <= dd) {
        melhor = picking_hermite(arvore->esquerda, clicks, mouse, tolerancia, melhor_dist);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking r = picking_hermite(arvore->direita, clicks, mouse, tolerancia, lim);
        if (r.distancia < melhor.distancia) melhor = r;
    } else {
        melhor = picking_hermite(arvore->direita, clicks, mouse, tolerancia, melhor_dist);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking r = picking_hermite(arvore->esquerda, clicks, mouse, tolerancia, lim);
        if (r.distancia < melhor.distancia) melhor = r;
    }
    return melhor;
}
