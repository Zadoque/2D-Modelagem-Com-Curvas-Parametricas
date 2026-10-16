#include "slider.h"

static float limitar(float valor){
  if(valor < 0.0f){
    return 0.0f;
  }
  if(valor > 1.0f){
    return 1.0f;
  }
  return valor;
}

float slider_valor_por_x(int x_mouse, int x_inicio, int largura){
  if(largura <= 0){
    return 0.0f;
  }
  return limitar((float)(x_mouse - x_inicio) / (float)largura);
}

int slider_x_por_valor(float valor, int x_inicio, int largura){
  if(largura <= 0){
    return x_inicio;
  }
  return x_inicio + (int)(limitar(valor) * (float)largura + 0.5f);
}

int slider_contem(const SliderRigidez *slider, int x, int y){
  return slider && x >= slider->x && x <= slider->x + slider->largura &&
         y >= slider->y && y <= slider->y + slider->altura;
}
