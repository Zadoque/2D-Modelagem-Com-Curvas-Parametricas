#include "../../includes/curvas/curva_generica.h"
#include <float.h>
#include <math.h>

static CurvaSubdivisaoConfig config_atual = {
    0.1f, 0.001f, 20u, 131071u, 8191u
};
static size_t nos_ultima_medicao;
static double vazao_media_nos_ms;

void curva_subdivisao_iniciar_medicao(void) { nos_ultima_medicao = 0; }
size_t curva_subdivisao_nos_ultima_medicao(void) { return nos_ultima_medicao; }
void curva_subdivisao_ajustar_orcamento(double ms, size_t segmentos) {
    const double alvo_ms = 4.0, margem = 0.75;
    if (ms < 0.05 || segmentos == 0 || nos_ultima_medicao < 16) return;
    double vazao = (double)nos_ultima_medicao / ms;
    vazao_media_nos_ms = vazao_media_nos_ms == 0.0
        ? vazao : vazao_media_nos_ms * 0.8 + vazao * 0.2;
    size_t por_segmento = (size_t)(vazao_media_nos_ms * alvo_ms * margem /
                                    (double)segmentos);
    if (por_segmento < 255u) por_segmento = 255u;
    if (por_segmento > 131071u) por_segmento = 131071u;
    config_atual.max_nos_geracao = por_segmento;
}

CurvaSubdivisaoConfig curva_subdivisao_config_padrao(void) {
    CurvaSubdivisaoConfig c = {0.1f, 0.001f, 20u, 131071u, 8191u};
    return c;
}

void curva_subdivisao_definir_config(CurvaSubdivisaoConfig c) {
    CurvaSubdivisaoConfig padrao = curva_subdivisao_config_padrao();
    if (!(c.tolerancia_geometrica > 0.0f)) c.tolerancia_geometrica = padrao.tolerancia_geometrica;
    if (!(c.tolerancia_parametrica > 0.0f)) c.tolerancia_parametrica = padrao.tolerancia_parametrica;
    if (c.profundidade_maxima == 0 || c.profundidade_maxima > 20u)
        c.profundidade_maxima = padrao.profundidade_maxima;
    if (c.max_nos_geracao == 0 || c.max_nos_geracao > 131071u)
        c.max_nos_geracao = padrao.max_nos_geracao;
    if (c.max_nos_picking == 0 || c.max_nos_picking > 8191u)
        c.max_nos_picking = padrao.max_nos_picking;
    config_atual = c;
}

CurvaSubdivisaoConfig curva_subdivisao_obter_config(void) { return config_atual; }

static int consumir_no(size_t *restantes) {
    if (*restantes == 0) return 0;
    --*restantes;
    return 1;
}

static void subdividir_geracao(const CurvaCubicaOps *ops, ponto P0, ponto P1,
                               ponto P2, ponto P3, Pontos *resultado,
                               ponto A, ponto B, float t0, float t1,
                               unsigned profundidade, size_t *nos) {
    float tmid = t0 + (t1 - t0) * 0.5f;
    nos_ultima_medicao++;
    ponto M = ops->avaliar(P0, P1, P2, P3, tmid);
    float desvio = calcula_dist_ponto_segmento(M, A, B);
    int parar = profundidade >= config_atual.profundidade_maxima ||
                tmid == t0 || tmid == t1 || !isfinite(desvio) ||
                desvio <= config_atual.tolerancia_geometrica ||
                !consumir_no(nos);
    if (parar) {
        if (isfinite(M.point[0]) && isfinite(M.point[1])) pontos_push(resultado, M);
        return;
    }
    subdividir_geracao(ops,P0,P1,P2,P3,resultado,A,M,t0,tmid,profundidade+1,nos);
    subdividir_geracao(ops,P0,P1,P2,P3,resultado,M,B,tmid,t1,profundidade+1,nos);
}

void gerar_segmento_curva_cubica(const CurvaCubicaOps *ops,
                                 ponto P0, ponto P1, ponto P2, ponto P3,
                                 Pontos *resultado, AABB *box) {
    resultado->quantidade_atual = 0;
    ponto A=ops->avaliar(P0,P1,P2,P3,0.0f);
    ponto M=ops->avaliar(P0,P1,P2,P3,0.5f);
    ponto B=ops->avaliar(P0,P1,P2,P3,1.0f);
    AABB esquerda, direita; reset_box(&esquerda); reset_box(&direita);
    incluir_intervalo_curva_cubica_aabb(&esquerda,P0,P1,P2,P3,0,0.5f,ops->avaliar,ops->derivar);
    incluir_intervalo_curva_cubica_aabb(&direita,P0,P1,P2,P3,0.5f,1,ops->avaliar,ops->derivar);
    size_t nos=config_atual.max_nos_geracao;
    pontos_push(resultado,A);
    subdividir_geracao(ops,P0,P1,P2,P3,resultado,A,M,0,0.5f,0,&nos);
    pontos_push(resultado,M);
    subdividir_geracao(ops,P0,P1,P2,P3,resultado,M,B,0.5f,1,0,&nos);
    pontos_push(resultado,B);
    box->esq_x_min=esquerda.x_min; box->esq_y_min=esquerda.y_min;
    box->esq_x_max=esquerda.x_max; box->esq_y_max=esquerda.y_max;
    box->dir_x_min=direita.x_min; box->dir_y_min=direita.y_min;
    box->dir_x_max=direita.x_max; box->dir_y_max=direita.y_max;
    box->x_min=fminf(esquerda.x_min,direita.x_min);
    box->y_min=fminf(esquerda.y_min,direita.y_min);
    box->x_max=fmaxf(esquerda.x_max,direita.x_max);
    box->y_max=fmaxf(esquerda.y_max,direita.y_max);
}

static ResultadoPicking refinar(const CurvaCubicaOps *ops,ponto P0,ponto P1,
    ponto P2,ponto P3,ponto mouse,float t0,float t1,float limite,
    unsigned profundidade,size_t *nos) {
    ResultadoPicking sem={-1,0,FLT_MAX};
    float tmid=t0+(t1-t0)*0.5f;
    if (profundidade>=config_atual.profundidade_maxima || tmid==t0 || tmid==t1 ||
        (t1-t0)<=config_atual.tolerancia_parametrica || !consumir_no(nos)) {
        ponto p=ops->avaliar(P0,P1,P2,P3,tmid);
        float dx=p.point[0]-mouse.point[0],dy=p.point[1]-mouse.point[1];
        ResultadoPicking r={0,tmid,sqrtf(dx*dx+dy*dy)}; return r;
    }
    AABB e,d; reset_box(&e); reset_box(&d);
    incluir_intervalo_curva_cubica_aabb(&e,P0,P1,P2,P3,t0,tmid,ops->avaliar,ops->derivar);
    incluir_intervalo_curva_cubica_aabb(&d,P0,P1,P2,P3,tmid,t1,ops->avaliar,ops->derivar);
    float de=dist_mouse_aabb(mouse,e),dd=dist_mouse_aabb(mouse,d);
    ResultadoPicking melhor=sem;
    float primeiro_t0=de<=dd?t0:tmid, primeiro_t1=de<=dd?tmid:t1;
    float segundo_t0=de<=dd?tmid:t0, segundo_t1=de<=dd?t1:tmid;
    float primeira_dist=de<=dd?de:dd, segunda_dist=de<=dd?dd:de;
    if (primeira_dist<limite)
        melhor=refinar(ops,P0,P1,P2,P3,mouse,primeiro_t0,primeiro_t1,
                       limite,profundidade+1,nos);
    float novo_limite=melhor.segmento_indice!=-1?melhor.distancia:limite;
    if (segunda_dist<novo_limite) {
        ResultadoPicking r=refinar(ops,P0,P1,P2,P3,mouse,segundo_t0,segundo_t1,
                                    novo_limite,profundidade+1,nos);
        if (r.distancia<melhor.distancia) melhor=r;
    }
    return melhor;
}

/* Percurso branch-and-bound compartilhado pelas quatro famílias. */
ResultadoPicking picking_curva_cubica(const CurvaCubicaOps *ops,AABBTREE *arvore,
    Pontos *controles,ponto mouse,float tolerancia,float melhor_dist) {
    ResultadoPicking sem={-1,0,FLT_MAX};
    if (!arvore || !controles || controles->quantidade_atual<4) return sem;
    float dc=dist_mouse_aabb(mouse,arvore->box);
    if (dc>tolerancia || dc>melhor_dist) return sem;
    if (!arvore->esquerda && !arvore->direita) {
        int i=arvore->box.segmento_indice,n=controles->quantidade_atual;
        ponto P0=controles->data[i%n],P1=controles->data[(i+1)%n];
        ponto P2=controles->data[(i+2)%n],P3=controles->data[(i+3)%n];
        float de=dist_mouse_aabb_left(mouse,arvore->box);
        float dd=dist_mouse_aabb_right(mouse,arvore->box);
        size_t nos=config_atual.max_nos_picking;
        ResultadoPicking melhor=sem;
        if (de<=dd) {
            if (de<=tolerancia) melhor=refinar(ops,P0,P1,P2,P3,mouse,0,0.5f,melhor_dist,0,&nos);
            float lim=melhor.segmento_indice!=-1?melhor.distancia:melhor_dist;
            if (dd<lim && dd<=tolerancia) { ResultadoPicking r=refinar(ops,P0,P1,P2,P3,mouse,0.5f,1,lim,0,&nos); if(r.distancia<melhor.distancia)melhor=r; }
        } else {
            if (dd<=tolerancia) melhor=refinar(ops,P0,P1,P2,P3,mouse,0.5f,1,melhor_dist,0,&nos);
            float lim=melhor.segmento_indice!=-1?melhor.distancia:melhor_dist;
            if (de<lim && de<=tolerancia) { ResultadoPicking r=refinar(ops,P0,P1,P2,P3,mouse,0,0.5f,lim,0,&nos); if(r.distancia<melhor.distancia)melhor=r; }
        }
        melhor.segmento_indice=melhor.distancia<=tolerancia?i:-1;
        return melhor;
    }
    float de=arvore->esquerda?dist_mouse_aabb(mouse,arvore->esquerda->box):FLT_MAX;
    float dd=arvore->direita?dist_mouse_aabb(mouse,arvore->direita->box):FLT_MAX;
    AABBTREE *primeiro=de<=dd?arvore->esquerda:arvore->direita;
    AABBTREE *segundo=de<=dd?arvore->direita:arvore->esquerda;
    ResultadoPicking melhor=picking_curva_cubica(ops,primeiro,controles,mouse,tolerancia,melhor_dist);
    float lim=melhor.segmento_indice!=-1?melhor.distancia:melhor_dist;
    ResultadoPicking outro=picking_curva_cubica(ops,segundo,controles,mouse,tolerancia,lim);
    return outro.distancia<melhor.distancia?outro:melhor;
}

void arrastar_segmento_curva_cubica(const CurvaCubicaOps *ops,Pontos *controles,
    int segmento,float t,ponto mouse) {
    if (!controles || controles->quantidade_atual<4) return;
    int n=controles->quantidade_atual,indices[4]; ponto P[4];
    for(int i=0;i<4;i++){indices[i]=(segmento+i)%n;P[i]=controles->data[indices[i]];}
    ponto c=ops->avaliar(P[0],P[1],P[2],P[3],t);
    float dx=mouse.point[0]-c.point[0],dy=mouse.point[1]-c.point[1],w[4];
    ops->pesos_controles(t,w);
    float soma=0;for(int i=0;i<4;i++)soma+=w[i]*w[i];
    if(soma<1e-8f)return;
    for(int i=0;i<4;i++){
        controles->data[indices[i]].point[0]+=dx*w[i]/soma;
        controles->data[indices[i]].point[1]+=dy*w[i]/soma;
    }
}
