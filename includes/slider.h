#ifndef SLIDER_H
#define SLIDER_H

typedef struct {
  int x, y, largura, altura;
  int destacado;
  int arrastando;
} SliderRigidez;

float slider_valor_por_x(int x_mouse, int x_inicio, int largura);
int slider_x_por_valor(float valor, int x_inicio, int largura);
int slider_contem(const SliderRigidez *slider, int x, int y);

#endif
