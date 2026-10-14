#include "transformacoes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static ponto p(float x,float y){ponto r={{x,y,1}};return r;}
static void perto(float a,float b){assert(fabsf(a-b)<1e-4f);}

int main(void) {
    Pontos v; pontos_init(&v,2); pontos_push(&v,p(1,0)); pontos_push(&v,p(3,0));
    ponto c; calcular_centro_medio(&c,&v); perto(c.point[0],2); perto(c.point[1],0);
    translacao_com_mouse(&v,c,5,7); perto(v.data[0].point[0],4); perto(v.data[1].point[0],6);
    perto(v.data[0].point[1],7); perto(v.data[1].point[1],7);
    calcular_centro_medio(&c,&v);
    rotacionar_com_mouse(&v,&c,p(5,8),1);
    perto(v.data[0].point[0],5); perto(v.data[0].point[1],6);
    perto(v.data[1].point[0],5); perto(v.data[1].point[1],8);
    aumentar_escala(&v,c); perto(calcula_distancia(v.data[0],c),1.05f);
    diminuir_escala(&v,c); perto(calcula_distancia(v.data[0],c),0.9975f);
    pontos_free(&v); puts("test_transformacoes: OK"); return 0;
}
