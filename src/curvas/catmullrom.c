#include "../../includes/curvas/catmullrom.h"
#include "../../includes/curvas/curva_generica.h"

static void pesos(float t,float w[4]){float t2=t*t,t3=t2*t;w[0]=(-t3+2*t2-t)/2;w[1]=(3*t3-5*t2+2)/2;w[2]=(-3*t3+4*t2+t)/2;w[3]=(t3-t2)/2;}
ponto calcular_ponto_catmullrom(ponto p0,ponto p1,ponto p2,ponto p3,float t){float w[4];pesos(t,w);ponto p[4]={p0,p1,p2,p3},r={{0}};for(int i=0;i<4;i++)for(int e=0;e<2;e++)r.point[e]+=w[i]*p[i].point[e];return r;}
static ponto derivar(ponto p0,ponto p1,ponto p2,ponto p3,float t){float w[4]={(-3*t*t+4*t-1)/2,(9*t*t-10*t)/2,(-9*t*t+8*t+1)/2,(3*t*t-2*t)/2};ponto p[4]={p0,p1,p2,p3},r={{0}};for(int i=0;i<4;i++)for(int e=0;e<2;e++)r.point[e]+=w[i]*p[i].point[e];return r;}
static const CurvaCubicaOps OPS={calcular_ponto_catmullrom,derivar,pesos};
void gerar_curva_catmullrom(ponto p0,ponto p1,ponto p2,ponto p3,Pontos*c,AABB*b){gerar_segmento_curva_cubica(&OPS,p0,p1,p2,p3,c,b);}
ResultadoPicking picking_catmullrom(AABBTREE*a,Pontos*c,ponto m,float t,float d){return picking_curva_cubica(&OPS,a,c,m,t,d);}
void arrastar_ponto_catmullrom(Pontos*c,int s,float t,ponto m){arrastar_segmento_curva_cubica(&OPS,c,s,t,m);}
