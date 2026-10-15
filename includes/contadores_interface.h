#ifndef CONTADORES_INTERFACE_H
#define CONTADORES_INTERFACE_H

#include "menu.h"
#include "pontos.h"

/* Mantém os valores numéricos e as strings exibidas no menu sincronizados
 * com os buffers que efetivamente alimentam renderização e benchmark. */
void contadores_interface_atualizar(EstadoAplicacao *estado, const Pontos *controles,
                                    const Pontos *nuvem);

#endif
