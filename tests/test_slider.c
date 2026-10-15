#include "slider.h"
#include <assert.h>
#include <stdio.h>

int main(void){
  SliderRigidez slider = {10, 20, 100, 12, 0, 0};
  assert(slider_valor_por_x(-20, 10, 100) == 0.0f);
  assert(slider_valor_por_x(10, 10, 100) == 0.0f);
  assert(slider_valor_por_x(60, 10, 100) == 0.5f);
  assert(slider_valor_por_x(110, 10, 100) == 1.0f);
  assert(slider_valor_por_x(200, 10, 100) == 1.0f);
  assert(slider_valor_por_x(10, 10, 0) == 0.0f);
  assert(slider_x_por_valor(-1.0f, 10, 100) == 10);
  assert(slider_x_por_valor(0.5f, 10, 100) == 60);
  assert(slider_x_por_valor(2.0f, 10, 100) == 110);
  assert(slider_contem(&slider, 10, 20));
  assert(slider_contem(&slider, 110, 32));
  assert(!slider_contem(&slider, 9, 20));
  assert(!slider_contem(&slider, 10, 33));
  puts("test_slider: OK");
  return 0;
}
