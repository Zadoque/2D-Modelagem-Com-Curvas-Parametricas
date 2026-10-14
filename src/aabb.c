#include <stdlib.h>
#include "./../includes/aabb.h"
#include <math.h>
#ifndef FLT_MAX
#define FLT_MAX 999999999.0f
#endif
int ponto_dentro_aabb(AABB box, float px, float py){
  // Cada comparação retorna 0 ou 1 como int
  return (px >= box.x_min) & (px <= box.x_max) & (py >= box.y_min) & (py <= box.y_max);
}

int aabb_vec_push(AABB_vec *v, AABB box){
  if(v == NULL || v->quantidade < 0 || v->capacidade < 0){
    return -1;
  }
  if(v->quantidade >= v->capacidade){
    int nova_capacidade = v->capacidade + v->capacidade / 2 + 1;
    if(nova_capacidade <= v->capacidade){
      return -1;
    }
    AABB *novos_dados = realloc(v->dados, (size_t)nova_capacidade * sizeof(AABB));
    if(novos_dados == NULL){
      return -1;
    }
    v->dados = novos_dados;
    v->capacidade = nova_capacidade;
  }
  v->dados[v->quantidade] = box;
  v->quantidade++;
  return 0;
}

void aabb_vec_init(AABB_vec *v, int capacidade_inicial){
  v->dados = (AABB *)malloc(capacidade_inicial * sizeof(AABB));
  v->quantidade = 0;
  v->capacidade = capacidade_inicial;
  if(v->dados == NULL){
    v->capacidade = 0;
    exit(-1);
  }
}

/* Constrói a hierarquia diretamente na arena contígua. */
static AABBTREE *construir_recursivo(AABB *dados, int inicio, int fim, AABBTREE *arena,
                                     int *proximo){
  // Reserva o próximo nó disponível na arena.
  AABBTREE *no = &arena[(*proximo)++];

  if(inicio == fim){
    // Nó folha
    no->box = dados[inicio];
    no->esquerda = NULL;
    no->direita = NULL;
    no->altura = 0;
    return no;
  }

  int meio = (inicio + fim) / 2;
  no->esquerda = construir_recursivo(dados, inicio, meio, arena, proximo);
  no->direita = construir_recursivo(dados, meio + 1, fim, arena, proximo);

  // União das caixas dos filhos
  AABB *e = &no->esquerda->box;
  AABB *d = &no->direita->box;

  no->box.x_min = fminf(e->x_min, d->x_min);
  no->box.y_min = fminf(e->y_min, d->y_min);
  no->box.x_max = fmaxf(e->x_max, d->x_max);
  no->box.y_max = fmaxf(e->y_max, d->y_max);
  no->box.segmento_indice = -1;

  // Altura = 1 + maior filho
  int alt_e = no->esquerda ? no->esquerda->altura : -1;
  int alt_d = no->direita ? no->direita->altura : -1;
  no->altura = 1 + (alt_e > alt_d ? alt_e : alt_d);

  return no;
}

AABBTREE *aabb_vec_para_arvore(AABB_vec *v){
  if(v == NULL || v->quantidade == 0){
    return NULL;
  }

  // Aloca todos os nós necessários de uma única vez.
  // Uma árvore binária com N folhas tem no máximo 2N - 1 nós no total.
  int max_nos = 2 * v->quantidade - 1;
  AABBTREE *arena = (AABBTREE *)malloc(max_nos * sizeof(AABBTREE));

  if(arena == NULL){
    return NULL;
  }

  int proximo = 0;

  // O retorno desta função (arena[0]) é garantidamente o ponteiro inicial
  // do bloco alocado pelo malloc, pois é o primeiro nó a ser processado.
  return construir_recursivo(v->dados, 0, v->quantidade - 1, arena, &proximo);
}

void aabb_tree_free(AABBTREE *no){
  // Como alocamos tudo de uma vez em `aabb_vec_para_arvore`,
  // a árvore inteira agora é um único bloco de memória contíguo.
  // O ponteiro 'no' da raiz é o endereço base desse bloco (&arena[0]).
  // Logo, um único free() resolve tudo e não precisa mais da recursão.
  if(no != NULL){
    free(no);
  }
}

void reset_box(AABB *box){
  box->x_min = FLT_MAX;
  box->y_min = FLT_MAX;
  box->x_max = FLT_MAX * (-1.0f);
  box->y_max = FLT_MAX * (-1.0f);
}

float calcula_area_box(AABB *box){
  if(!box){
    return 0.0f;
  }
  float largura = fabsf(box->x_max - box->x_min);
  float altura = fabsf(box->y_max - box->y_min);
  return largura * altura;
}
