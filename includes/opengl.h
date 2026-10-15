#ifndef OPENGL_H
#define OPENGL_H

#include "pontos.h"
#include "vetor.h"
#include "menu.h"
#include "aabb.h"
#include "performance.h"
#include "geometria_curvas.h"
// Incluir todas as curvas
#include "curvas/hermite.h"
#include "curvas/bezier.h"
#include "curvas/bspline.h"
#include "curvas/catmullrom.h"
#include "transformacoes.h"
#include "contadores_interface.h"

typedef struct {
  int indice;
  int selecionado;
} Selecao;

typedef struct {
  int seg_curva;
  float t;
  float u_global;
  float t_local;
  Curva curva_selecionada;
} Selecao_curva;

void initGL();
void finalizar_aplicacao(void);
void AlteraTamanhoJanela(int w, int h);
void display();
void desenhar_conteudo_principal();
void gerar_curva_selecionada();
void desenhar_curva_atual();
void onMouse(int button, int state, int x, int y);
void onMouseMove(int x, int y);
void onMotion(int x, int y);
/* Núcleo da issue #19; a interface da issue #18 chama esta função. */
int definir_modo_anti_autointerferencia(int ativo);
void opengl_benchmark_vbo_executar(int quantidade_controles, int quadros);
int traduzCoordenadaX(int x);
int traduzCoordenadaY(int y);

#endif
