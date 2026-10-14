#include "../../includes/curvas/bspline.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#ifndef FLT_MAX
#define FLT_MAX  99999999.0f
#endif

float *t;
int qtd;

static const float MATRIZ_BSPLINE[4][4] = {
    {-1.0f / 6.0f,  3.0f / 6.0f, -3.0f / 6.0f, 1.0f / 6.0f},
    { 3.0f / 6.0f, -6.0f / 6.0f,  3.0f / 6.0f, 0.0f / 6.0f},
    {-3.0f / 6.0f,  0.0f / 6.0f,  3.0f / 6.0f, 0.0f / 6.0f},
    { 1.0f / 6.0f,  4.0f / 6.0f,  1.0f / 6.0f, 0.0f / 6.0f}};

ponto calcular_ponto_bspline(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float vetor_t[4] = {t3, t2, t, 1.0f};

    float coeficientes[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            coeficientes[i] += vetor_t[j] * MATRIZ_BSPLINE[j][i];

    ponto resultado;
    resultado.point[0] = coeficientes[0] * P0.point[0] + coeficientes[1] * P1.point[0] +
                         coeficientes[2] * P2.point[0] + coeficientes[3] * P3.point[0];
    resultado.point[1] = coeficientes[0] * P0.point[1] + coeficientes[1] * P1.point[1] +
                         coeficientes[2] * P2.point[1] + coeficientes[3] * P3.point[1];
    return resultado;
}

static ponto derivada_bspline(ponto P0, ponto P1, ponto P2, ponto P3, float t) {
    float t2 = t * t;
    float pesos[4] = {
        (-3.0f*t2 + 6.0f*t - 3.0f) / 6.0f,
        ( 9.0f*t2 - 12.0f*t) / 6.0f,
        (-9.0f*t2 + 6.0f*t + 3.0f) / 6.0f,
        ( 3.0f*t2) / 6.0f
    };
    ponto controles[4] = {P0, P1, P2, P3};
    ponto r = {{0.0f, 0.0f, 0.0f}};
    for (int i = 0; i < 4; i++) {
        r.point[0] += pesos[i] * controles[i].point[0];
        r.point[1] += pesos[i] * controles[i].point[1];
    }
    return r;
}
void subdivide(ponto P0, ponto P1, ponto P2, ponto P3, Pontos *curva_resultado,
               ponto A, ponto B, float t0, float t1, float tolerance, AABB *box) {
    float tmid = (t0 + t1) / 2.0f;
    ponto M = calcular_ponto_bspline(P0, P1, P2, P3, tmid);
    float dist = calcula_dist_ponto_segmento(M, A, B);
    
    if (dist < tolerance) {
        pontos_push(curva_resultado, M);
        box->x_min = fminf(box->x_min, M.point[0]);
        box->y_min = fminf(box->y_min, M.point[1]);
        box->x_max = fmaxf(box->x_max, M.point[0]);
        box->y_max = fmaxf(box->y_max, M.point[1]);
    } else {
        subdivide(P0, P1, P2, P3, curva_resultado, A, M, t0, tmid, tolerance, box);
        subdivide(P0, P1, P2, P3, curva_resultado, M, B, tmid, t1, tolerance, box);
    }
}
// Unificação das duas funções left/right — lado: 0=esquerda, 1=direita
static void criar_box_picking(ponto P0, ponto P1, ponto P2, ponto P3,
                               float t0, float t1, AABB *box) {
    incluir_intervalo_curva_cubica_aabb(box, P0, P1, P2, P3, t0, t1,
                                        calcular_ponto_bspline, derivada_bspline);
}



void gerar_curva_bspline(ponto P0, ponto P1, ponto P2, ponto P3,
                         Pontos *curva_resultado, AABB *box) {
    curva_resultado->quantidade_atual = 0;
    ponto A = calcular_ponto_bspline(P0, P1, P2, P3, 0.0f);
    ponto M = calcular_ponto_bspline(P0, P1, P2, P3, 0.5f);
    ponto B = calcular_ponto_bspline(P0, P1, P2, P3, 1.0f);

    // Inicializa caixas temporárias com os limites dos extremos
    AABB box_esq; reset_box(&box_esq);
    criar_box_picking(P0, P1, P2, P3, 0.0f, 0.5f, &box_esq);

    AABB box_dir; reset_box(&box_dir);
    criar_box_picking(P0, P1, P2, P3, 0.5f, 1.0f, &box_dir);

    // Subdivide as metades direcionando os resultados para as caixas temporárias
    pontos_push(curva_resultado, A);
    subdivide(P0, P1, P2, P3, curva_resultado, A, M, 0.0f, 0.5f, 0.1f, &box_esq);
    pontos_push(curva_resultado, M);
    subdivide(P0, P1, P2, P3, curva_resultado, M, B, 0.5f, 1.0f, 0.1f, &box_dir);
    pontos_push(curva_resultado, B);

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

static ResultadoPicking subdivide_picking_bspline(
        ponto P0, ponto P1, ponto P2, ponto P3,
        ponto mouse,
        float t0, float t1,
        float melhor_dist,
        float tolerancia_t) {

    ResultadoPicking sem_resultado = { -1, 0.0f, FLT_MAX };

    // Caso base: intervalo pequeno o suficiente
    if ((t1 - t0) < tolerancia_t) {
        float tmid = (t0 + t1) / 2.0f;
        ponto p = calcular_ponto_bspline(P0, P1, P2, P3, tmid);
        float dx = p.point[0] - mouse.point[0];
        float dy = p.point[1] - mouse.point[1];
        ResultadoPicking r = { 0, tmid, sqrtf(dx*dx + dy*dy) };
        return r;
    }

    float tmid = (t0 + t1) / 2.0f;

    // Caixa da metade esquerda [t0, tmid]
    AABB box_esq; reset_box(&box_esq);
    criar_box_picking(P0, P1, P2, P3, t0, tmid, &box_esq);
    float dist_esq = dist_mouse_aabb(mouse, box_esq);

    // Caixa da metade direita [tmid, t1]
    AABB box_dir; reset_box(&box_dir);
    criar_box_picking(P0, P1, P2, P3, tmid, t1, &box_dir);
    float dist_dir = dist_mouse_aabb(mouse, box_dir);

    ResultadoPicking melhor = sem_resultado;

    // Explora primeiro o lado mais próximo do mouse
    if (dist_esq <= dist_dir) {
        if (dist_esq < melhor_dist) {
            melhor = subdivide_picking_bspline(P0, P1, P2, P3, mouse,
                                               t0, tmid, melhor_dist, tolerancia_t);
        }
        // Usa melhor distância atualizada para podar o outro lado
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (dist_dir < limite) {
            ResultadoPicking r = subdivide_picking_bspline(P0, P1, P2, P3, mouse,
                                                           tmid, t1, limite, tolerancia_t);
            if (r.distancia < melhor.distancia) melhor = r;
        }
    } else {
        if (dist_dir < melhor_dist) {
            melhor = subdivide_picking_bspline(P0, P1, P2, P3, mouse,
                                               tmid, t1, melhor_dist, tolerancia_t);
        }
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        if (dist_esq < limite) {
            ResultadoPicking r = subdivide_picking_bspline(P0, P1, P2, P3, mouse,
                                                           t0, tmid, limite, tolerancia_t);
            if (r.distancia < melhor.distancia) melhor = r;
        }
    }
    return melhor;
}

ResultadoPicking picking_bspline(AABBTREE* arvore, Pontos* clicks,
                                  ponto mouse, float tolerancia, float melhor_dist) {
    ResultadoPicking sem_resultado = { -1, 0.0f, FLT_MAX };
    if (arvore == NULL) return sem_resultado;

    float dist_caixa = dist_mouse_aabb(mouse, arvore->box);

    // Poda: fora da caixa E mais longe que a melhor distância já encontrada
  if (dist_caixa > tolerancia || (melhor_dist != FLT_MAX && dist_caixa > melhor_dist)) return sem_resultado;

    // Nó folha — refina com subdivisão
    if (arvore->esquerda == NULL && arvore->direita == NULL) {
        int i = arvore->box.segmento_indice;
        int n = clicks->quantidade_atual;
        ponto P0 = clicks->data[ i      % n];
        ponto P1 = clicks->data[(i + 1) % n];
        ponto P2 = clicks->data[(i + 2) % n];
        ponto P3 = clicks->data[(i + 3) % n];
       
        float dist_metade_esq = dist_mouse_aabb_left( mouse, arvore->box);
        float dist_metade_dir = dist_mouse_aabb_right(mouse, arvore->box);
        ResultadoPicking resultado = sem_resultado;

        // Explora primeiro o lado mais próximo
        if (dist_metade_esq <= dist_metade_dir) {
            // Tenta a esquerda
            if (dist_metade_esq <= tolerancia) {
                resultado = subdivide_picking_bspline(P0, P1, P2, P3, mouse, 0.0f, 0.5f, melhor_dist, 0.001f);
            }
            
            // Tenta a direita usando o resultado da esquerda como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_dir < limite && dist_metade_dir <= tolerancia) {
                ResultadoPicking r_dir = subdivide_picking_bspline(P0, P1, P2, P3, mouse, 0.5f, 1.0f, limite, 0.001f);
                if (r_dir.distancia < resultado.distancia) {
                    resultado = r_dir;
                }
            }
        } else {
            // Tenta a direita
            if (dist_metade_dir <= tolerancia) {
                resultado = subdivide_picking_bspline(P0, P1, P2, P3, mouse, 0.5f, 1.0f, melhor_dist, 0.001f);
            }
            
            // Tenta a esquerda usando o resultado da direita como limite para otimizar
            float limite = (resultado.segmento_indice != -1) ? resultado.distancia : melhor_dist;
            if (dist_metade_esq < limite && dist_metade_esq <= tolerancia) {
                ResultadoPicking r_esq = subdivide_picking_bspline(P0, P1, P2, P3, mouse, 0.0f, 0.5f, limite, 0.001f);
                if (r_esq.distancia < resultado.distancia) {
                    resultado = r_esq;
                }
            }
        }
          
        // Associa o índice real se a distância final for válida
        resultado.segmento_indice = (resultado.distancia <= tolerancia) ? i : -1;
        return resultado;
    }

    // Nó interno — explora primeiro o filho mais próximo
    float dist_esq = (arvore->esquerda != NULL)
                     ? dist_mouse_aabb(mouse, arvore->esquerda->box) : FLT_MAX;
    float dist_dir = (arvore->direita  != NULL)
                     ? dist_mouse_aabb(mouse, arvore->direita->box)  : FLT_MAX;

    ResultadoPicking melhor = sem_resultado;

    if (dist_esq <= dist_dir) {
        melhor = picking_bspline(arvore->esquerda, clicks, mouse, tolerancia, melhor_dist);
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking dir = picking_bspline(arvore->direita, clicks, mouse, tolerancia, limite);
        if (dir.distancia < melhor.distancia) melhor = dir;
    } else {
        melhor = picking_bspline(arvore->direita, clicks, mouse, tolerancia, melhor_dist);
        float limite = (melhor.segmento_indice != -1) ? melhor.distancia : melhor_dist;
        ResultadoPicking esq = picking_bspline(arvore->esquerda, clicks, mouse, tolerancia, limite);
        if (esq.distancia < melhor.distancia) melhor = esq;
    }
    return melhor;
}
static void pesos_bspline_cubica(float t, float B[4]) {
    float t2 = t * t;
    float t3 = t2 * t;

    B[0] = (-t3 + 3.0f*t2 - 3.0f*t + 1.0f) / 6.0f;
    B[1] = ( 3.0f*t3 - 6.0f*t2 + 4.0f) / 6.0f;
    B[2] = (-3.0f*t3 + 3.0f*t2 + 3.0f*t + 1.0f) / 6.0f;
    B[3] = t3 / 6.0f;
}

void arrastar_ponto_bspline(Pontos *clicks, int segmento_indice, float ti, ponto mouse) {
    if (clicks == NULL || clicks->quantidade_atual < 4) return;

    int n = clicks->quantidade_atual;

    int i0 = (segmento_indice + 0) % n;
    int i1 = (segmento_indice + 1) % n;
    int i2 = (segmento_indice + 2) % n;
    int i3 = (segmento_indice + 3) % n;

    ponto P0 = clicks->data[i0];
    ponto P1 = clicks->data[i1];
    ponto P2 = clicks->data[i2];
    ponto P3 = clicks->data[i3];

    ponto C = calcular_ponto_bspline(P0, P1, P2, P3, ti);

    float ex = mouse.point[0] - C.point[0];
    float ey = mouse.point[1] - C.point[1];

    float B[4];
    pesos_bspline_cubica(ti, B);

    float soma_quadrados =
        B[0]*B[0] + B[1]*B[1] + B[2]*B[2] + B[3]*B[3];

    if (soma_quadrados < 1e-8f) return;

    float fx = ex / soma_quadrados;
    float fy = ey / soma_quadrados;

    clicks->data[i0].point[0] += fx * B[0];
    clicks->data[i0].point[1] += fy * B[0];

    clicks->data[i1].point[0] += fx * B[1];
    clicks->data[i1].point[1] += fy * B[1];

    clicks->data[i2].point[0] += fx * B[2];
    clicks->data[i2].point[1] += fy * B[2];

    clicks->data[i3].point[0] += fx * B[3];
    clicks->data[i3].point[1] += fy * B[3];
}
