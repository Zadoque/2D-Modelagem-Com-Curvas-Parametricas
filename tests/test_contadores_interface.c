#include "contadores_interface.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void){
  EstadoAplicacao estado = {0};
  Pontos controles, nuvem;
  pontos_init(&controles, 4);
  pontos_init(&nuvem, 8);
  ponto ponto_zero = {{0.0f, 0.0f, 1.0f}};
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&controles, ponto_zero) == 0);
  }
  for(int i = 0; i < 37; i++){
    assert(pontos_push(&nuvem, ponto_zero) == 0);
  }
  contadores_interface_atualizar(&estado, &controles, &nuvem);
  assert(estado.qtd_pontos_controle_number == controles.quantidade_atual);
  assert(estado.qtd_nuvem_pontos_number == nuvem.quantidade_atual);
  assert(strcmp(estado.qtd_pontos_controle, "6") == 0);
  assert(strcmp(estado.qtd_nuvem_pontos, "37") == 0);
  controles.quantidade_atual = 4;
  nuvem.quantidade_atual = 12;
  contadores_interface_atualizar(&estado, &controles, &nuvem);
  assert(strcmp(estado.qtd_pontos_controle, "4") == 0);
  assert(strcmp(estado.qtd_nuvem_pontos, "12") == 0);
  pontos_free(&nuvem);
  pontos_free(&controles);
  puts("test_contadores_interface: OK");
  return 0;
}
