#ifndef RENDERIZACAO_BUFFERS_H
#define RENDERIZACAO_BUFFERS_H

#include "geometria_curvas.h"
#include <GL/gl.h>
#include <stdint.h>

typedef struct {
  GLuint curva;
  GLuint controles;
  size_t capacidade_curva;
  size_t capacidade_controles;
  uint64_t assinatura_controles;
  int disponivel;
} BuffersRenderizacao;

void renderizacao_buffers_inicializar(BuffersRenderizacao *buffers);
void renderizacao_buffers_liberar(BuffersRenderizacao *buffers);
void renderizacao_buffers_atualizar(BuffersRenderizacao *buffers,
                                    const GeometriaCurvas *geometria, const Pontos *controles);
void renderizacao_desenhar_buffer(const BuffersRenderizacao *buffers, GLuint buffer, GLenum modo,
                                  int quantidade);

#endif
