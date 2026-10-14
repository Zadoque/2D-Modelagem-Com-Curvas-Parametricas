#include "../includes/transformacoes.h"
#include <math.h>

static void aplicar(Pontos*p,Matriz3 m){for(int i=0;i<p->quantidade_atual;i++){float r[3];MultiplicaPonto(p->data[i].point,m,r);for(int e=0;e<3;e++)p->data[i].point[e]=r[e];}}
static Matriz3 ao_redor(ponto c,Matriz3 operacao){Matriz3 a=CriarMatrizTranslacao(-c.point[0],-c.point[1]);Matriz3 b=MultiplicaMatriz(a,operacao);return MultiplicaMatriz(b,CriarMatrizTranslacao(c.point[0],c.point[1]));}
static int indice_valido(const Pontos*p,int i){return p&&i>=0&&i<p->quantidade_atual;}

void calcular_centro_medio(ponto*c,Pontos*p){c->point[0]=c->point[1]=0;c->point[2]=1;if(!p||p->quantidade_atual==0)return;for(int i=0;i<p->quantidade_atual;i++){c->point[0]+=p->data[i].point[0];c->point[1]+=p->data[i].point[1];}c->point[0]/=p->quantidade_atual;c->point[1]/=p->quantidade_atual;}
void translacao_com_mouse(Pontos*p,ponto c,int x,int y){if(!p)return;aplicar(p,CriarMatrizTranslacao(x-c.point[0],y-c.point[1]));}
void transladar_para_origem(Pontos*p,ponto c){aplicar(p,CriarMatrizTranslacao(-c.point[0],-c.point[1]));}
void trasladar_de_volta(Pontos*p,ponto c){aplicar(p,CriarMatrizTranslacao(c.point[0],c.point[1]));}
void rotacionar_com_mouse(Pontos*p,ponto*c,ponto mouse,int i){if(!indice_valido(p,i)||!c)return;float a=atan2f(p->data[i].point[1]-c->point[1],p->data[i].point[0]-c->point[0]);float b=atan2f(mouse.point[1]-c->point[1],mouse.point[0]-c->point[0]);aplicar(p,ao_redor(*c,CriarMatrizRotacao(b-a)));}
void escala_com_mouse(Pontos*p,ponto*c,ponto mouse,int i){if(!indice_valido(p,i)||!c)return;float d=calcula_distancia(p->data[i],*c);if(d<1e-6f)return;float s=calcula_distancia(mouse,*c)/d;aplicar(p,ao_redor(*c,CriarMatrizEscala(s,s)));}
void shearX_com_mouse(Pontos*p,ponto*c,ponto mouse,int i){if(!indice_valido(p,i)||!c)return;float vx=p->data[i].point[0]-c->point[0],vy=p->data[i].point[1]-c->point[1];float s=fabsf(vy)>1e-6f?(mouse.point[0]-c->point[0]-vx)/vy:0;aplicar(p,ao_redor(*c,CriarMatrizCisalhamentoX(s)));}
void shearY_com_mouse(Pontos*p,ponto*c,ponto mouse,int i){if(!indice_valido(p,i)||!c)return;float vx=p->data[i].point[0]-c->point[0],vy=p->data[i].point[1]-c->point[1];float s=fabsf(vx)>1e-6f?(mouse.point[1]-c->point[1]-vy)/vx:0;aplicar(p,ao_redor(*c,CriarMatrizCisalhamentoY(s)));}
void shear_com_mouse(Pontos*p,ponto*c,ponto mouse,int i){if(!indice_valido(p,i)||!c)return;float vx=p->data[i].point[0]-c->point[0],vy=p->data[i].point[1]-c->point[1];float sx=fabsf(vy)>1e-6f?(mouse.point[0]-c->point[0]-vx)/vy:0;float sy=fabsf(vx)>1e-6f?(mouse.point[1]-c->point[1]-vy)/vx:0;Matriz3 sh=MultiplicaMatriz(CriarMatrizCisalhamentoX(sx),CriarMatrizCisalhamentoY(sy));aplicar(p,ao_redor(*c,sh));}
void aumentar_escala(Pontos*p,ponto c){if(p)aplicar(p,ao_redor(c,CriarMatrizEscala(1.05f,1.05f)));}
void diminuir_escala(Pontos*p,ponto c){if(p)aplicar(p,ao_redor(c,CriarMatrizEscala(0.95f,0.95f)));}
