#include "../../includes/curvas/bezier.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifndef FLT_MAX
#define FLT_MAX 99999999.0f
#endif

static const float MATRIZ_BEZIER[4][4] = {
    {-1.0f,  3.0f, -3.0f, 1.0f},
    { 3.0f, -6.0f,  3.0f, 0.0f},
    {-3.0f,  3.0f,  0.0f, 0.0f},
    { 1.0f,  0.0f,  0.0f, 0.0f}};

ponto calcular_ponto_bezier(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    float t2 = t * t, t3 = t2 * t;
    float vetor_t[4] = {t3, t2, t, 1.0f};
    float coeficientes[4] = {0};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            coeficientes[i] += vetor_t[j] * MATRIZ_BEZIER[j][i];
    ponto r;
    r.point[0] = coeficientes[0]*P0.point[0] + coeficientes[1]*P1.point[0] +
                 coeficientes[2]*P2.point[0] + coeficientes[3]*P3.point[0];
    r.point[1] = coeficientes[0]*P0.point[1] + coeficientes[1]*P1.point[1] +
                 coeficientes[2]*P2.point[1] + coeficientes[3]*P3.point[1];
    return r;
}

static ponto derivada_bezier(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    float u = 1.0f - t;
    ponto r = {{0.0f, 0.0f, 0.0f}};
    for (int eixo = 0; eixo < 2; eixo++) {
        r.point[eixo] = 3.0f*u*u*(P1.point[eixo] - P0.point[eixo])
                      + 6.0f*u*t*(P2.point[eixo] - P1.point[eixo])
                      + 3.0f*t*t*(P3.point[eixo] - P2.point[eixo]);
    }
    return r;
}

static void subdivide_bezier(ponto P0, ponto P1, ponto P2, ponto P3,
                              Pontos *curva, ponto A, ponto B,
                              float t0, float t1, float tol, AABB *box) {
    float tmid = (t0 + t1) / 2.0f;
    ponto M = calcular_ponto_bezier(P0, P1, P2, P3, tmid);
    if (calcula_dist_ponto_segmento(M, A, B) < tol) {
        pontos_push(curva, M);
        box->x_min = fminf(box->x_min, M.point[0]);
        box->y_min = fminf(box->y_min, M.point[1]);
        box->x_max = fmaxf(box->x_max, M.point[0]);
        box->y_max = fmaxf(box->y_max, M.point[1]);
    } else {
        subdivide_bezier(P0, P1, P2, P3, curva, A, M, t0, tmid, tol, box);
        subdivide_bezier(P0, P1, P2, P3, curva, M, B, tmid, t1, tol, box);
    }
}

void gerar_curva_bezier(ponto P0, ponto P1, ponto P2, ponto P3,
                         Pontos *curva, AABB *box) {
    curva->quantidade_atual = 0;
    ponto A = calcular_ponto_bezier(P0, P1, P2, P3, 0.0f);
    ponto M = calcular_ponto_bezier(P0, P1, P2, P3, 0.5f);
    ponto B = calcular_ponto_bezier(P0, P1, P2, P3, 1.0f);

    // Inicializa caixas temporárias com os limites dos extremos
    AABB box_esq; reset_box(&box_esq);
    incluir_intervalo_curva_cubica_aabb(&box_esq, P0, P1, P2, P3, 0.0f, 0.5f,
                                        calcular_ponto_bezier, derivada_bezier);

    AABB box_dir; reset_box(&box_dir);
    incluir_intervalo_curva_cubica_aabb(&box_dir, P0, P1, P2, P3, 0.5f, 1.0f,
                                        calcular_ponto_bezier, derivada_bezier);
     // Subdivide as metades direcionando os resultados para as caixas temporárias
    pontos_push(curva, A);
    subdivide_bezier(P0, P1, P2, P3, curva, A, M, 0.0f, 0.5f, 0.1f, &box_esq);
    pontos_push(curva, M);
    subdivide_bezier(P0, P1, P2, P3, curva, M, B, 0.5f, 1.0f, 0.1f, &box_dir);
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



// Unificação das duas funções left/right — lado: 0=esquerda, 1=direita
static void criar_box_picking_b(ponto P0, ponto P1, ponto P2, ponto P3,
                               float t0, float t1, AABB *box) {
    incluir_intervalo_curva_cubica_aabb(box, P0, P1, P2, P3, t0, t1,
                                        calcular_ponto_bezier, derivada_bezier);
}


static ResultadoPicking subdivide_picking_bezier(
        ponto P0, ponto P1, ponto P2, ponto P3,
        ponto mouse,
        float t0, float t1,
        float melhor_dist,
        float tolerancia_t) {

    ResultadoPicking sem_resultado = { -1, 0.0f, FLT_MAX };

    // Caso base: intervalo pequeno o suficiente
    if ((t1 - t0) < tolerancia_t) {
        float tmid = (t0 + t1) / 2.0f;
        ponto p = calcular_ponto_bezier(P0, P1, P2, P3, tmid);
        float dx = p.point[0] - mouse.point[0];
        float dy = p.point[1] - mouse.point[1];
        ResultadoPicking r = { 0, tmid, sqrtf(dx*dx + dy*dy) };
        return r;
    }

    float tmid = (t0 + t1) / 2.0f;

    // Caixa da metade esquerda [t0, tmid]
    AABB box_esq; reset_box(&box_esq);
    criar_box_picking_b(P0, P1, P2, P3, t0, tmid, &box_esq);
    float dist_esq = dist_mouse_aabb(mouse, box_esq);

    // Caixa da metade direita [tmid, t1]
    AABB box_dir; reset_box(&box_dir);
    criar_box_picking_b(P0, P1, P2, P3, tmid, t1, &box_dir);
    float dist_dir = dist_mouse_aabb(mouse, box_dir);

    ResultadoPicking melhor = sem_resultado;

    // Explora primeiro o lado mais próximo do mouse
    if (dist_esq <= dist_dir) {
        if (dist_esq < melhor_dist) {
            melhor = subdivide_picking_bezier(P0, P1, P2, P3, mouse,
                                               t0, tmid, melhor_dist, tolerancia_t);
        }
        // Usa melhor distância atualizada para podar o outro lado
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (dist_dir < limite) {
            ResultadoPicking r = subdivide_picking_bezier(P0, P1, P2, P3, mouse,
                                                           tmid, t1, limite, tolerancia_t);
            if (r.distancia < melhor.distancia) melhor = r;
        }
    } else {
        if (dist_dir < melhor_dist) {
            melhor = subdivide_picking_bezier(P0, P1, P2, P3, mouse,
                                               tmid, t1, melhor_dist, tolerancia_t);
        }
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (dist_esq < limite) {
            ResultadoPicking r = subdivide_picking_bezier(P0, P1, P2, P3, mouse,
                                                           t0, tmid, limite, tolerancia_t);
            if (r.distancia < melhor.distancia) melhor = r;
        }
    }
    return melhor;
}

static void pesos_bezier_cubica(float t, float B[4]) {
    float u = 1.0f - t;
    B[0] = u*u*u;
    B[1] = 3.0f*t*u*u;
    B[2] = 3.0f*t*t*u;
    B[3] = t*t*t;
}

void arrastar_ponto_bezier(Pontos *clicks, int seg, float ti, ponto mouse) {
    if (!clicks || clicks->quantidade_atual < 4) return;
    int n = clicks->quantidade_atual;
    int i0 = (seg + 0) % n, i1 = (seg + 1) % n;
    int i2 = (seg + 2) % n, i3 = (seg + 3) % n;
    ponto P0 = clicks->data[i0], P1 = clicks->data[i1];
    ponto P2 = clicks->data[i2], P3 = clicks->data[i3];
    ponto C = calcular_ponto_bezier(P0, P1, P2, P3, ti);
    float ex = mouse.point[0] - C.point[0], ey = mouse.point[1] - C.point[1];
    float B[4]; pesos_bezier_cubica(ti, B);
    float soma = B[0]*B[0] + B[1]*B[1] + B[2]*B[2] + B[3]*B[3];
    if (soma < 1e-8f) return;
    float fx = ex/soma, fy = ey/soma;
    clicks->data[i0].point[0] += fx*B[0]; clicks->data[i0].point[1] += fy*B[0];
    clicks->data[i1].point[0] += fx*B[1]; clicks->data[i1].point[1] += fy*B[1];
    clicks->data[i2].point[0] += fx*B[2]; clicks->data[i2].point[1] += fy*B[2];
    clicks->data[i3].point[0] += fx*B[3]; clicks->data[i3].point[1] += fy*B[3];
}

ResultadoPicking picking_bezier(AABBTREE *arvore, Pontos *clicks,
                                 ponto mouse, float tolerancia, float melhor_dist) {
    ResultadoPicking sem_resultado = { -1, 0.0f, FLT_MAX };
    if (!arvore) return sem_resultado;
    float dc = dist_mouse_aabb(mouse, arvore->box);
    if (dc > tolerancia || (melhor_dist != FLT_MAX && dc > melhor_dist)) return sem_resultado;

    if (arvore->esquerda == NULL && arvore->direita == NULL) {
        printf("\n\tchegou a um nó folha na curva bezier com segmento: %d", arvore->box.segmento_indice);
        int i = arvore->box.segmento_indice, n = clicks->quantidade_atual;
        ponto P0 = clicks->data[ i    % n], P1 = clicks->data[(i+1) % n];
        ponto P2 = clicks->data[(i+2) % n], P3 = clicks->data[(i+3) % n];
        
        float dist_metade_esq = dist_mouse_aabb_left( mouse, arvore->box);
        float dist_metade_dir = dist_mouse_aabb_right(mouse, arvore->box);
        ResultadoPicking resultado = sem_resultado;

        // Explora primeiro o lado mais próximo
        if (dist_metade_esq <= dist_metade_dir) {
            printf("\n\tO ponto clicado está na esquerda");
            // Tenta a esquerda
            if (dist_metade_esq <= tolerancia) {
                resultado = subdivide_picking_bezier(P0, P1, P2, P3, mouse, 0.0f, 0.5f, melhor_dist, 0.001f);
            }
            
            // Tenta a direita usando o resultado da esquerda como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_dir < limite && dist_metade_dir <= tolerancia) {
                ResultadoPicking r_dir = subdivide_picking_bezier(P0, P1, P2, P3, mouse, 0.5f, 1.0f, limite, 0.001f);
                if (r_dir.distancia < resultado.distancia) {
                    resultado = r_dir;
                }
            }
        } else {
            // Tenta a direita
            if (dist_metade_dir <= tolerancia) {
                resultado = subdivide_picking_bezier(P0, P1, P2, P3, mouse, 0.5f, 1.0f, melhor_dist, 0.001f);
            }
            
            // Tenta a esquerda usando o resultado da direita como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_esq < limite && dist_metade_esq <= tolerancia) {
                ResultadoPicking r_esq = subdivide_picking_bezier(P0, P1, P2, P3, mouse, 0.0f, 0.5f, limite, 0.001f);
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
        melhor = picking_bezier(arvore->esquerda, clicks, mouse, tolerancia, melhor_dist);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking r = picking_bezier(arvore->direita, clicks, mouse, tolerancia, lim);
        if (r.distancia < melhor.distancia) melhor = r;
    } else {
        melhor = picking_bezier(arvore->direita, clicks, mouse, tolerancia, melhor_dist);
        float lim = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking r = picking_bezier(arvore->esquerda, clicks, mouse, tolerancia, lim);
        if (r.distancia < melhor.distancia) melhor = r;
    }
    return melhor;
}
