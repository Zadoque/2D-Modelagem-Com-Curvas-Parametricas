#include "../includes/densidade.h"

/* Posição 1 é a menor densidade e posição 5 a maior. */
static const float limiares[DENSIDADE_POSICOES] = {230.0f, 210.0f, 190.0f, 170.0f, 150.0f};

float densidade_limiar_por_posicao(int posicao){
  if(posicao < 0){
    posicao = 0;
  }
  if(posicao >= DENSIDADE_POSICOES){
    posicao = DENSIDADE_POSICOES - 1;
  }
  return limiares[posicao];
}

int densidade_posicao_por_x(int x_mouse, int x_inicio, int largura){
  if(largura <= 0 || x_mouse <= x_inicio){
    return 0;
  }
  if(x_mouse >= x_inicio + largura){
    return DENSIDADE_POSICOES - 1;
  }
  return (x_mouse - x_inicio) * DENSIDADE_POSICOES / largura;
}

int densidade_contem(const SeletorDensidade *seletor, int x, int y){
  return seletor && x >= seletor->x && x <= seletor->x + seletor->largura &&
         y >= seletor->y && y <= seletor->y + seletor->altura;
}
