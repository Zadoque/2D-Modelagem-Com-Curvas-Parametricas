#include "densidade.h"
#include <assert.h>
#include <stdio.h>

int main(void){
  SeletorDensidade seletor = {10, 20, 100, 20, -1};
  const float esperado[DENSIDADE_POSICOES] = {230.0f, 210.0f, 190.0f, 170.0f, 150.0f};
  for(int i = 0; i < DENSIDADE_POSICOES; i++){
    assert(densidade_limiar_por_posicao(i) == esperado[i]);
    assert(densidade_posicao_por_x(10 + i * 20 + 10, 10, 100) == i);
  }
  assert(densidade_limiar_por_posicao(-1) == 230.0f);
  assert(densidade_limiar_por_posicao(9) == 150.0f);
  assert(densidade_posicao_por_x(0, 10, 100) == 0);
  assert(densidade_posicao_por_x(110, 10, 100) == 4);
  assert(densidade_posicao_por_x(10, 10, 0) == 0);
  assert(densidade_contem(&seletor, 10, 20));
  assert(densidade_contem(&seletor, 110, 40));
  assert(!densidade_contem(&seletor, 9, 20));
  assert(!densidade_contem(&seletor, 10, 41));
  puts("test_densidade: OK");
  return 0;
}
