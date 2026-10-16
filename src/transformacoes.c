#include "../includes/transformacoes.h"
#include <math.h>

#define FATOR_CISALHAMENTO_MAXIMO 10.0f

static float limitar_fator_cisalhamento(float fator){
  if(fator > FATOR_CISALHAMENTO_MAXIMO){
    return FATOR_CISALHAMENTO_MAXIMO;
  }
  if(fator < -FATOR_CISALHAMENTO_MAXIMO){
    return -FATOR_CISALHAMENTO_MAXIMO;
  }
  return fator;
}

static void aplicar(Pontos *p, Matriz3 m){
  for(int i = 0; i < p->quantidade_atual; i++){
    float r[3];
    matriz3_multiplicar_ponto(p->data[i].point, m, r);
    for(int e = 0; e < 3; e++){
      p->data[i].point[e] = r[e];
    }
  }
}
static Matriz3 ao_redor(ponto c, Matriz3 operacao){
  Matriz3 a = matriz3_translacao(-c.point[0], -c.point[1]);
  Matriz3 b = matriz3_multiplicar(a, operacao);
  return matriz3_multiplicar(b, matriz3_translacao(c.point[0], c.point[1]));
}
static int indice_valido(const Pontos *p, int i){
  return p && i >= 0 && i < p->quantidade_atual;
}

void calcular_centro_medio(ponto *c, Pontos *p){
  c->point[0] = c->point[1] = 0;
  c->point[2] = 1;
  if(!p || p->quantidade_atual == 0){
    return;
  }
  for(int i = 0; i < p->quantidade_atual; i++){
    c->point[0] += p->data[i].point[0];
    c->point[1] += p->data[i].point[1];
  }
  c->point[0] /= p->quantidade_atual;
  c->point[1] /= p->quantidade_atual;
}
void translacao_com_mouse(Pontos *p, ponto c, int x, int y){
  if(!p){
    return;
  }
  aplicar(p, matriz3_translacao(x - c.point[0], y - c.point[1]));
}
void transladar_para_origem(Pontos *p, ponto c){
  aplicar(p, matriz3_translacao(-c.point[0], -c.point[1]));
}
void transladar_da_origem(Pontos *p, ponto c){
  aplicar(p, matriz3_translacao(c.point[0], c.point[1]));
}
void rotacionar_com_mouse(Pontos *p, ponto *c, ponto mouse, int i){
  if(!indice_valido(p, i) || !c){
    return;
  }
  float a = atan2f(p->data[i].point[1] - c->point[1], p->data[i].point[0] - c->point[0]);
  float b = atan2f(mouse.point[1] - c->point[1], mouse.point[0] - c->point[0]);
  aplicar(p, ao_redor(*c, matriz3_rotacao(b - a)));
}
void escala_com_mouse(Pontos *p, ponto *c, ponto mouse, int i){
  if(!indice_valido(p, i) || !c){
    return;
  }
  float d = calcula_distancia(p->data[i], *c);
  if(d < 1e-6f){
    return;
  }
  float s = calcula_distancia(mouse, *c) / d;
  aplicar(p, ao_redor(*c, matriz3_escala(s, s)));
}
void cisalhar_x_com_mouse(Pontos *p, ponto *c, ponto mouse, int i){
  if(!indice_valido(p, i) || !c){
    return;
  }
  float vx = p->data[i].point[0] - c->point[0], vy = p->data[i].point[1] - c->point[1];
  float s = fabsf(vy) > 1e-6f ? (mouse.point[0] - c->point[0] - vx) / vy : 0;
  aplicar(p, ao_redor(*c, matriz3_cisalhamento_x(limitar_fator_cisalhamento(s))));
}
void cisalhar_y_com_mouse(Pontos *p, ponto *c, ponto mouse, int i){
  if(!indice_valido(p, i) || !c){
    return;
  }
  float vx = p->data[i].point[0] - c->point[0], vy = p->data[i].point[1] - c->point[1];
  float s = fabsf(vx) > 1e-6f ? (mouse.point[1] - c->point[1] - vy) / vx : 0;
  aplicar(p, ao_redor(*c, matriz3_cisalhamento_y(limitar_fator_cisalhamento(s))));
}
void cisalhar_com_mouse(Pontos *p, ponto *c, ponto mouse, int i){
  if(!indice_valido(p, i) || !c){
    return;
  }
  float vx = p->data[i].point[0] - c->point[0], vy = p->data[i].point[1] - c->point[1];
  float dx = mouse.point[0] - p->data[i].point[0];
  float dy = mouse.point[1] - p->data[i].point[1];
  if(fabsf(dx) >= fabsf(dy) && fabsf(vy) > 1e-6f){
    cisalhar_x_com_mouse(p, c, mouse, i);
  } else if(fabsf(vx) > 1e-6f){
    cisalhar_y_com_mouse(p, c, mouse, i);
  }
}
void aumentar_escala(Pontos *p, ponto c){
  if(p){
    aplicar(p, ao_redor(c, matriz3_escala(1.05f, 1.05f)));
  }
}
void diminuir_escala(Pontos *p, ponto c){
  if(p){
    aplicar(p, ao_redor(c, matriz3_escala(0.95f, 0.95f)));
  }
}
