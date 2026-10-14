#include "geometria_curvas.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void){
    Pontos controles,scratch;pontos_init(&controles,6);pontos_init(&scratch,16);
    for(int i=0;i<6;i++){ponto p={{i*20.0f,(i%2)*30.0f,1}};assert(pontos_push(&controles,p)==0);}
    AABB_vec boxes;aabb_vec_init(&boxes,8);GeometriaCurvas g;geometria_curvas_init(&g);
    assert(geometria_curvas_atualizar(&g,&scratch,&boxes,&controles,MODO_CURVA_BEZIER,1)==0);
    assert(g.quantidade_faixas==2&&boxes.quantidade==2);
    assert(boxes.dados[0].segmento_indice==0&&boxes.dados[1].segmento_indice==3);
    assert(g.vertices.quantidade_atual>0);
    assert(geometria_curvas_atualizar(&g,&scratch,&boxes,&controles,MODO_CURVA_HERMITE,1)==0);
    assert(g.quantidade_faixas==6&&boxes.quantidade==6);
    for(int i=0;i<6;i++)assert(boxes.dados[i].segmento_indice==i);
    geometria_curvas_free(&g);free(boxes.dados);pontos_free(&scratch);pontos_free(&controles);
    puts("test_geometria_curvas: OK");return 0;
}
