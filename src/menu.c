#include "./../includes/menu.h"
#include <GL/glut.h>
#include <string.h>
#include <stdio.h>
EstadoAplicacao estado_atual = {MODO_CRIAR_PONTO, MODO_POLIGONO_ABERTO,
                                MODO_CURVA_HERMITE, NENHUMA, "0", "0", 0};

int menu_largura = 200;
float menu_escala_fonte = 0.1f;
float menu_altura_fonte = 12.0f;
float info_escala_fonte = 5.0f;
void *menu_fonte_stroke = GLUT_STROKE_ROMAN;
Botoes botoes;
BotoesOperacoes botoes_operacoes[4];
InfoItem info[2];
int info_hover = -1;

void atualizar_dimensoes_menu() {
    int altura_janela = glutGet(GLUT_WINDOW_HEIGHT);
    int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
    printf("\n\tA largura da janela é %d e a altura %d", altura_janela, largura_janela);

    // Menu ocupa 15% da largura ou mínimo 150px.
    menu_largura = (largura_janela * 0.15 < 150) ? 150 : largura_janela * 0.15;

    int margem = menu_largura * 0.05;
    int largura_botao = menu_largura - (2 * margem);
    int altura_botao = altura_janela * 0.03;
    int espacamento = altura_janela * 0.01;

    menu_altura_fonte = altura_botao * 0.45f;
    menu_escala_fonte = (largura_botao * 0.09f) / 121.05f;
    info_escala_fonte = (largura_botao * 0.05f) / 121.0f;

    // Painel de status: canto inferior direito do menu.
    const int painel_margem = 8;
    const int painel_espacamento = 8;
    const int painel_largura = menu_largura - 2 * painel_margem;
    const int item_altura = 24;
    const int y_base = painel_margem;

    strcpy(info[0].texto, "Pontos de controle: 0");
    strcpy(info[0].tooltip, "Quantidade de pontos que define a forma da curva.");
    info[0].x = painel_margem;
    info[0].y = y_base + item_altura + painel_espacamento;
    info[0].largura = painel_largura;
    info[0].altura = item_altura;

    strcpy(info[1].texto, "Nuvem de pontos: 0");
    strcpy(info[1].tooltip, "Quantidade de pontos amostrados da curva atual.");
    info[1].x = painel_margem;
    info[1].y = y_base;
    info[1].largura = painel_largura;
    info[1].altura = item_altura;

    // === BOTÕES DE CRIAÇÃO/SELEÇÃO (Grupo 1) ===
    int y_inicio_grupo1 = altura_janela * 0.05;
    strcpy(botoes.botoes1[0].texto, "Criar Ponto");
    botoes.botoes1[0].x = margem;
    botoes.botoes1[0].y = y_inicio_grupo1;
    botoes.botoes1[0].largura = largura_botao;
    botoes.botoes1[0].altura = altura_botao;
    botoes.botoes1[0].acao = MODO_CRIAR_PONTO;

    strcpy(botoes.botoes1[1].texto, "Apagar Ponto");
    botoes.botoes1[1].x = margem;
    botoes.botoes1[1].y = y_inicio_grupo1 + (altura_botao + espacamento);
    botoes.botoes1[1].largura = largura_botao;
    botoes.botoes1[1].altura = altura_botao;
    botoes.botoes1[1].acao = MODO_APAGAR_PONTO;

    strcpy(botoes.botoes1[2].texto, "Selecionar vertice");
    botoes.botoes1[2].x = margem;
    botoes.botoes1[2].y = y_inicio_grupo1 + 2 * (altura_botao + espacamento);
    botoes.botoes1[2].largura = largura_botao;
    botoes.botoes1[2].altura = altura_botao;
    botoes.botoes1[2].acao = MODO_SELECIONAR_PONTO;

    strcpy(botoes.botoes1[3].texto, "Operar Poligono");
    botoes.botoes1[3].x = margem;
    botoes.botoes1[3].y = y_inicio_grupo1 + 3 * (altura_botao + espacamento);
    botoes.botoes1[3].largura = largura_botao;
    botoes.botoes1[3].altura = altura_botao;
    botoes.botoes1[3].acao = MODO_SELECIONAR_POLIGONO;

    strcpy(botoes.botoes1[4].texto, "Selecionar Curva");
    botoes.botoes1[4].x = margem;
    botoes.botoes1[4].y = y_inicio_grupo1 + 4 * (altura_botao + espacamento);
    botoes.botoes1[4].largura = largura_botao;
    botoes.botoes1[4].altura = altura_botao;
    botoes.botoes1[4].acao = MODO_SELEICIONAR_CURVA;

    // === BOTÕES DE POLÍGONO (Grupo 2) ===
    int y_inicio_grupo2 = y_inicio_grupo1 + 5 * (altura_botao + espacamento) + altura_janela * 0.03;

    strcpy(botoes.botoes2[0].texto, "Poligono Aberto");
    botoes.botoes2[0].x = margem;
    botoes.botoes2[0].y = y_inicio_grupo2;
    botoes.botoes2[0].largura = largura_botao;
    botoes.botoes2[0].altura = altura_botao;
    botoes.botoes2[0].acao = MODO_POLIGONO_ABERTO;

    strcpy(botoes.botoes2[1].texto, "Poligono Fechado");
    botoes.botoes2[1].x = margem;
    botoes.botoes2[1].y = y_inicio_grupo2 + (altura_botao + espacamento);
    botoes.botoes2[1].largura = largura_botao;
    botoes.botoes2[1].altura = altura_botao;
    botoes.botoes2[1].acao = MODO_POLIGONO_FECHADO;

    // === BOTÕES DE CURVA (Grupo 3) ===
    int y_inicio_grupo3 = y_inicio_grupo2 + 2 * (altura_botao + espacamento) + altura_janela * 0.03;

    strcpy(botoes.botoes3[0].texto, "Hermite");
    botoes.botoes3[0].x = margem;
    botoes.botoes3[0].y = y_inicio_grupo3;
    botoes.botoes3[0].largura = largura_botao;
    botoes.botoes3[0].altura = altura_botao;
    botoes.botoes3[0].acao = MODO_CURVA_HERMITE;

    strcpy(botoes.botoes3[1].texto, "Catmull-Rom");
    botoes.botoes3[1].x = margem;
    botoes.botoes3[1].y = y_inicio_grupo3 + (altura_botao + espacamento);
    botoes.botoes3[1].largura = largura_botao;
    botoes.botoes3[1].altura = altura_botao;
    botoes.botoes3[1].acao = MODO_CURVA_CATMULLROM;

    strcpy(botoes.botoes3[2].texto, "B-Spline");
    botoes.botoes3[2].x = margem;
    botoes.botoes3[2].y = y_inicio_grupo3 + 2 * (altura_botao + espacamento);
    botoes.botoes3[2].largura = largura_botao;
    botoes.botoes3[2].altura = altura_botao;
    botoes.botoes3[2].acao = MODO_CURVA_BSPLINE;

    strcpy(botoes.botoes3[3].texto, "Bezier");
    botoes.botoes3[3].x = margem;
    botoes.botoes3[3].y = y_inicio_grupo3 + 3 * (altura_botao + espacamento);
    botoes.botoes3[3].largura = largura_botao;
    botoes.botoes3[3].altura = altura_botao;
    botoes.botoes3[3].acao = MODO_CURVA_BEZIER;

    // === BOTÕES DE OPERAÇÃO (Grupo 4) ===
    int y_inicio_grupo4 = y_inicio_grupo3 + 4 * (altura_botao + espacamento) + altura_janela * 0.03;

    strcpy(botoes_operacoes[0].texto, "Translacao");
    botoes_operacoes[0].x = margem;
    botoes_operacoes[0].y = y_inicio_grupo4;
    botoes_operacoes[0].largura = largura_botao;
    botoes_operacoes[0].altura = altura_botao;
    botoes_operacoes[0].acao = TRANSLACAO;

    strcpy(botoes_operacoes[1].texto, "Rotacao");
    botoes_operacoes[1].x = margem;
    botoes_operacoes[1].y = y_inicio_grupo4 + (altura_botao + espacamento);
    botoes_operacoes[1].largura = largura_botao;
    botoes_operacoes[1].altura = altura_botao;
    botoes_operacoes[1].acao = ROTACAO;

    strcpy(botoes_operacoes[2].texto, "Escala");
    botoes_operacoes[2].x = margem;
    botoes_operacoes[2].y = y_inicio_grupo4 + 2 * (altura_botao + espacamento);
    botoes_operacoes[2].largura = largura_botao;
    botoes_operacoes[2].altura = altura_botao;
    botoes_operacoes[2].acao = ESCALA;

    strcpy(botoes_operacoes[3].texto, "Shear");
    botoes_operacoes[3].x = margem;
    botoes_operacoes[3].y = y_inicio_grupo4 + 3 * (altura_botao + espacamento);
    botoes_operacoes[3].largura = largura_botao;
    botoes_operacoes[3].altura = altura_botao;
    botoes_operacoes[3].acao = SHEAR;
}

void inicializar_menu() {
    atualizar_dimensoes_menu();
}

void desenhar_texto(float x, float y, void *font, const char *texto, float escala) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(escala, escala, 1.0f);

    for (const char *c = texto; *c != '\0'; c++) {
        glutStrokeCharacter(font, *c);
    }

    glPopMatrix();
}

void desenhar_botao_generico(void *botao, TipoBotao tipo, int indice) {
    int x, y, largura, altura;
    char *texto;
    int ativo = 0;
    int destacado = 0;

    switch (tipo) {
    case TIPO_BOTAO1: {
        Botoes1 *b = (Botoes1 *)botao;
        x = b->x; y = b->y; largura = b->largura; altura = b->altura;
        texto = b->texto; destacado = b->destacado;
        ativo = (estado_atual.criacao_ou_selecao == b->acao);
        break;
    }
    case TIPO_BOTAO2: {
        Botoes2 *b = (Botoes2 *)botao;
        x = b->x; y = b->y; largura = b->largura; altura = b->altura;
        texto = b->texto; destacado = b->destacado;
        ativo = (estado_atual.poligono == b->acao);
        break;
    }
    case TIPO_BOTAO3: {
        Botoes3 *b = (Botoes3 *)botao;
        x = b->x; y = b->y; largura = b->largura; altura = b->altura;
        texto = b->texto; destacado = b->destacado;
        ativo = (estado_atual.curva == b->acao);
        break;
    }
    case TIPO_OPERACAO: {
        BotoesOperacoes *b = (BotoesOperacoes *)botao;
        x = b->x; y = b->y; largura = b->largura; altura = b->altura;
        texto = b->texto; destacado = b->destacado;
        ativo = (estado_atual.operacao == b->acao);
        break;
    }
    }

    if (ativo) glColor3f(0.2f, 0.7f, 0.2f);
    else if (destacado) glColor3f(0.9f, 0.9f, 0.7f);
    else glColor3f(0.95f, 0.95f, 0.95f);

    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x + largura, y);
    glVertex2f(x + largura, y + altura); glVertex2f(x, y + altura);
    glEnd();

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x + largura, y);
    glVertex2f(x + largura, y + altura); glVertex2f(x, y + altura);
    glEnd();

    float texto_largura = calcular_largura_texto_stroke(menu_fonte_stroke, texto, menu_escala_fonte);
    float texto_altura = calcular_altura_fonte_stroke(menu_escala_fonte);
    float texto_x = x + (largura - texto_largura) / 2.0f;
    float texto_y = y + (altura - texto_altura) / 2.0f;
    desenhar_texto(texto_x, texto_y, menu_fonte_stroke, texto, menu_escala_fonte);
}

void desenhar_botoes_menu() {
    for (int i = 0; i < 5; i++) desenhar_botao_generico(&botoes.botoes1[i], TIPO_BOTAO1, i);
    for (int i = 0; i < 2; i++) desenhar_botao_generico(&botoes.botoes2[i], TIPO_BOTAO2, i);
    for (int i = 0; i < 4; i++) desenhar_botao_generico(&botoes.botoes3[i], TIPO_BOTAO3, i);

    if (estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO) {
        for (int i = 0; i < 4; i++) desenhar_botao_generico(&botoes_operacoes[i], TIPO_OPERACAO, i);
    } else if (estado_atual.criacao_ou_selecao == MODO_SELECIONAR_PONTO) {
        desenhar_botao_generico(&botoes_operacoes[0], TIPO_OPERACAO, 0);
        estado_atual.operacao = TRANSLACAO;
    }
}

float calcular_largura_texto_stroke(void *font, const char *texto, float escala) {
    float largura = 0.0f;
    for (const char *c = texto; *c != '\0'; c++) largura += glutStrokeWidth(font, *c);
    return largura * escala;
}

float calcular_altura_fonte_stroke(float escala) {
    return 119.05f * escala;
}

void recalcula_dimensoes_info(){
  const float escala = info_escala_fonte * 0.72f;
  for (int i = 0; i < 2; i++) {
    info[i].largura = calcular_largura_texto_stroke(menu_fonte_stroke, info[i].texto, escala);
    info[i].altura = 24.0f;
    info[i].x = (int)(menu_largura - 8.0f - info[i].largura);
  }
}

void atualiza_info(){
  snprintf(info[0].texto, sizeof(info[0].texto), "Pontos de controle: %s", estado_atual.qtd_pontos_controle);
  snprintf(info[1].texto, sizeof(info[1].texto), "Nuvem de pontos: %s", estado_atual.qtd_nuvem_pontos);
}

static void desenhar_retangulo_info(const InfoItem *item) {
  const float largura = item->largura + 16.0f;
  const float altura = 22.0f;
  const float x = item->x - 8.0f;
  const float y = item->y - 4.0f;

  glColor3f(0.12f, 0.12f, 0.12f);
  glBegin(GL_QUADS);
  glVertex2f(x, y); glVertex2f(x + largura, y);
  glVertex2f(x + largura, y + altura); glVertex2f(x, y + altura);
  glEnd();
}

static void desenhar_tooltip(const InfoItem *item) {
  const float escala = info_escala_fonte * 0.55f;
  const float margem = 7.0f;
  const float altura = 28.0f;
  const float largura_texto = calcular_largura_texto_stroke(menu_fonte_stroke, item->tooltip, escala);
  float x = item->x - largura_texto - 2.0f * margem;
  float y = item->y + item->altura + 6.0f;

  if (x < 4.0f) x = 4.0f;
  const int altura_janela = glutGet(GLUT_WINDOW_HEIGHT);
  if (y + altura > altura_janela - 4.0f) y = item->y - altura - 6.0f;

  glColor3f(0.05f, 0.05f, 0.05f);
  glBegin(GL_QUADS);
  glVertex2f(x, y); glVertex2f(x + largura_texto + 2.0f * margem, y);
  glVertex2f(x + largura_texto + 2.0f * margem, y + altura); glVertex2f(x, y + altura);
  glEnd();

  glColor3f(1.0f, 1.0f, 1.0f);
  desenhar_texto(x + margem, y + 7.0f, menu_fonte_stroke, item->tooltip, escala);
}

void desenhar_info(){
  atualiza_info();
  recalcula_dimensoes_info();

  for (int i = 0; i < 2; i++) {
    if (info_hover == i) {
      desenhar_retangulo_info(&info[i]);
      glColor3f(1.0f, 1.0f, 1.0f);
    } else {
      glColor3f(0.92f, 0.92f, 0.92f);
    }
    desenhar_texto((float)info[i].x, (float)info[i].y, menu_fonte_stroke, info[i].texto, info_escala_fonte * 0.72f);
  }

  if (info_hover >= 0 && info_hover < 2) desenhar_tooltip(&info[info_hover]);
}

void processar_mouse_move_info(int x, int y) {
  int novo_hover = -1;
  for (int i = 0; i < 2; i++) {
    const float margem = 5.0f;
    if (x >= info[i].x - margem && x <= info[i].x + info[i].largura + margem &&
        y >= info[i].y - margem && y <= info[i].y + info[i].altura + margem) {
      novo_hover = i;
      break;
    }
  }

  if (novo_hover != info_hover) {
    info_hover = novo_hover;
    glutPostRedisplay();
  }
}
