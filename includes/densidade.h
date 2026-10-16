#ifndef DENSIDADE_H
#define DENSIDADE_H

enum { DENSIDADE_POSICOES = 5 };

typedef struct {
  int x, y, largura, altura;
  int posicao_hover;
} SeletorDensidade;

float densidade_limiar_por_posicao(int posicao);
int densidade_posicao_por_x(int x_mouse, int x_inicio, int largura);
int densidade_contem(const SeletorDensidade *seletor, int x, int y);

#endif
