#ifndef MENU_H
#define MENU_H

#include "pontos.h"

// Estados da aplicação
typedef enum {
  MODO_CRIAR_PONTO,
  MODO_APAGAR_PONTO,
  MODO_SELECIONAR_PONTO,
  MODO_SELECIONAR_POLIGONO,
  MODO_SELEICIONAR_CURVA
} Criacao_ou_selecao;

typedef enum { TRANSLACAO, ROTACAO, ESCALA, SHEAR, NENHUMA } Operacoes;

typedef enum { MODO_POLIGONO_ABERTO, MODO_POLIGONO_FECHADO } Poligono;

typedef enum {
  MODO_CURVA_HERMITE,
  MODO_CURVA_CATMULLROM,
  MODO_CURVA_BSPLINE,
  MODO_CURVA_BEZIER
} Curva;

typedef enum { TIPO_BOTAO1, TIPO_BOTAO2, TIPO_BOTAO3, TIPO_OPERACAO } TipoBotao;

typedef struct {
  Criacao_ou_selecao criacao_ou_selecao;
  Poligono poligono;
  Curva curva;
  Operacoes operacao;
  char qtd_pontos_controle[50];
  char qtd_nuvem_pontos[50];
  int qtd_nuvem_pontos_number;
} EstadoAplicacao;

typedef struct {
  int x, y, largura, altura;
  char texto[20];
  Criacao_ou_selecao acao;
  int destacado;
} Botoes1;

typedef struct {
  int x, y, largura, altura;
  char texto[20];
  Poligono acao;
  int destacado;
} Botoes2;

typedef struct {
  int x, y, largura, altura;
  char texto[20];
  Curva acao;
  int destacado;
} Botoes3;

typedef struct {
  int x, y, largura, altura;
  char texto[20];
  Operacoes acao;
  int destacado;
} BotoesOperacoes;

typedef struct {
  Botoes1 botoes1[5];
  Botoes2 botoes2[2];
  Botoes3 botoes3[4];
} Botoes;

// Item do painel de status e seu tooltip.
typedef struct {
  int x, y;
  float largura, altura;
  char texto[80];
  char tooltip[160];
  int destacado;
} InfoItem;

// Variáveis globais do menu
extern EstadoAplicacao estado_atual;
extern Botoes botoes;
extern BotoesOperacoes botoes_operacoes[4];
extern int menu_largura;
extern InfoItem info[2];
extern int info_hover;

// Funções do menu
void atualizar_dimensoes_menu();
void inicializar_menu();
void desenhar_botao_generico(void* botao, TipoBotao tipo, int indice);
void desenhar_botoes_menu();
void processar_clique_menu(int x, int y);
void processar_mouse_move_menu(int x, int y);
float calcular_largura_texto_stroke(void *font, const char *texto, float escala);
float calcular_altura_fonte_stroke(float escala);
void desenhar_texto(float x, float y, void *font, const char *texto, float escala);
void recalcula_dimensoes_info();
void atualiza_info();
void desenhar_info();
void processar_mouse_move_info(int x, int y);
#endif
