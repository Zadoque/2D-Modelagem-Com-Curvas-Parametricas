#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
#include "curvas/hermite.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef ponto (*Eval)(ponto,ponto,ponto,ponto,float);
typedef void (*Gerar)(ponto,ponto,ponto,ponto,Pontos*,AABB*);
typedef ResultadoPicking (*Picking)(AABBTREE*,Pontos*,ponto,float,float);

static ponto hermite_local(ponto a,ponto b,ponto c,ponto d,float t) {
    ponto t1,t2; calcular_tangentes_hermite(a,b,c,d,&t1,&t2);
    return calcular_ponto_hermite(b,c,t1,t2,t);
}

static void verificar(const char *nome, Eval eval, Gerar gerar, Picking picking) {
    Pontos controles, curva; pontos_init(&controles,4); pontos_init(&curva,32);
    ponto ps[4] = {{{-30,20,1}},{{10,100,1}},{{90,-80,1}},{{150,30,1}}};
    for (int i=0;i<4;i++) assert(pontos_push(&controles,ps[i])==0);
    AABB_vec boxes; aabb_vec_init(&boxes,1); AABB b={0}; b.segmento_indice=0;
    gerar(ps[0],ps[1],ps[2],ps[3],&curva,&b); assert(aabb_vec_push(&boxes,b)==0);
    AABBTREE *arvore=aabb_vec_para_arvore(&boxes);
    float alvo_t=0.371f; ponto mouse=eval(ps[0],ps[1],ps[2],ps[3],alvo_t);
    ResultadoPicking r=picking(arvore,&controles,mouse,2.0f,FLT_MAX);
    if (r.segmento_indice != 0 || fabsf(r.t-alvo_t)>0.003f || r.distancia>0.2f)
        fprintf(stderr,"%s: seg=%d t=%f dist=%f\n",nome,r.segmento_indice,r.t,r.distancia);
    assert(r.segmento_indice==0); assert(fabsf(r.t-alvo_t)<=0.003f); assert(r.distancia<=0.2f);
    mouse.point[0]+=1000; mouse.point[1]+=1000;
    r=picking(arvore,&controles,mouse,2.0f,FLT_MAX); assert(r.segmento_indice==-1);
    aabb_tree_free(arvore); free(boxes.dados); pontos_free(&curva); pontos_free(&controles);
}

int main(void) {
    verificar("Bezier",calcular_ponto_bezier,gerar_curva_bezier,picking_bezier);
    verificar("B-Spline",calcular_ponto_bspline,gerar_curva_bspline,picking_bspline);
    verificar("Catmull-Rom",calcular_ponto_catmullrom,gerar_curva_catmullrom,picking_catmullrom);
    verificar("Hermite",hermite_local,gerar_curva_hermite,picking_hermite);
    puts("test_picking: OK"); return 0;
}
