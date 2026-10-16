#define GL_GLEXT_PROTOTYPES

#include "renderizacao_buffers.h"

#include <GL/glext.h>
#include <string.h>

static uint64_t assinatura_pontos(const Pontos *pontos){
  uint64_t assinatura = UINT64_C(1469598103934665603);
  for(int i = 0; i < pontos->quantidade_atual; i++){
    for(int eixo = 0; eixo < 3; eixo++){
      uint32_t bits;
      memcpy(&bits, &pontos->data[i].point[eixo], sizeof(bits));
      assinatura ^= bits;
      assinatura *= UINT64_C(1099511628211);
    }
  }
  return assinatura ^ (uint64_t)pontos->quantidade_atual;
}

static void atualizar_buffer(GLuint buffer, size_t *capacidade, const Pontos *pontos){
  size_t bytes = (size_t)pontos->quantidade_atual * sizeof(*pontos->data);
  if(bytes == 0){ return; }
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  if(bytes > *capacidade){
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)bytes, pontos->data, GL_DYNAMIC_DRAW);
    *capacidade = bytes;
  } else {
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)bytes, pontos->data);
  }
}

void renderizacao_buffers_inicializar(BuffersRenderizacao *buffers){
  if(!buffers){ return; }
  *buffers = (BuffersRenderizacao){.assinatura_controles = UINT64_MAX};
  glGenBuffers(1, &buffers->curva);
  glGenBuffers(1, &buffers->controles);
  buffers->disponivel = buffers->curva != 0 && buffers->controles != 0;
}

void renderizacao_buffers_liberar(BuffersRenderizacao *buffers){
  if(!buffers){ return; }
  if(buffers->curva){ glDeleteBuffers(1, &buffers->curva); }
  if(buffers->controles){ glDeleteBuffers(1, &buffers->controles); }
  *buffers = (BuffersRenderizacao){0};
}

void renderizacao_buffers_atualizar(BuffersRenderizacao *buffers,
                                    const GeometriaCurvas *geometria, const Pontos *controles){
  if(!buffers || !buffers->disponivel || !geometria || !controles){ return; }
  atualizar_buffer(buffers->curva, &buffers->capacidade_curva, &geometria->vertices);
  uint64_t assinatura = assinatura_pontos(controles);
  if(assinatura != buffers->assinatura_controles){
    atualizar_buffer(buffers->controles, &buffers->capacidade_controles, controles);
    buffers->assinatura_controles = assinatura;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void renderizacao_desenhar_buffer(const BuffersRenderizacao *buffers, GLuint buffer, GLenum modo,
                                  int quantidade){
  if(!buffers || !buffers->disponivel || quantidade <= 0){ return; }
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, (GLsizei)sizeof(ponto), (const void *)0);
  glDrawArrays(modo, 0, quantidade);
  glDisableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}
