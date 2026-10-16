#include "../includes/contadores_interface.h"

#include <stdio.h>

void contadores_interface_atualizar(EstadoAplicacao *estado, const Pontos *controles,
                                    const Pontos *nuvem){
  int quantidade_controles = controles ? controles->quantidade_atual : 0;
  int quantidade_nuvem = nuvem ? nuvem->quantidade_atual : 0;
  if(!estado){
    return;
  }
  estado->qtd_pontos_controle_number = quantidade_controles;
  estado->qtd_nuvem_pontos_number = quantidade_nuvem;
  snprintf(estado->qtd_pontos_controle, sizeof(estado->qtd_pontos_controle), "%d",
           quantidade_controles);
  snprintf(estado->qtd_nuvem_pontos, sizeof(estado->qtd_nuvem_pontos), "%d", quantidade_nuvem);
}
