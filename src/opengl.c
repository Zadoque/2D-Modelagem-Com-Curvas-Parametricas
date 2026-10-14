#include "./../includes/opengl.h"
#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef FLT_MAX
#define FLT_MAX  99999999.0f
#endif
// Variáveis globais
Pontos g_clicks;
Pontos g_curva_atual; // Nova: armazenar pontos da curva gerada
Selecao selecao_ponto;
Selecao_curva selecao_curva;
int selecao_poligono;
ponto centro;
int precisa_refazer_curva = 0;
AABB_vec vetor_boxes;
AABBTREE *arvore_boxes = NULL;
static GeometriaCurvas geometria_curva;
// Configurações visuais
static float cor_fundo[3] = {0.2f, 0.3f, 0.4f};

void initGL() {
  glClearColor(cor_fundo[0], cor_fundo[1], cor_fundo[2], 1.0f);
  glPointSize(7.0f);
  glLineWidth(2.0f);

  // Inicializar estrutura para curva
  pontos_init(&g_curva_atual, 100);
  geometria_curvas_init(&geometria_curva);
  aabb_vec_init(&vetor_boxes, 20);
  selecao_ponto.selecionado = 0;
  selecao_ponto.indice = 0;
  selecao_poligono = 0;
  centro.point[0] = 0;
  centro.point[1] = 0;
  centro.point[2] = 1;
  printf("OpenGL inicializado com sucesso!\n");

}
static int profundidade_arvore(AABBTREE* no) {
    if (no == NULL) return 0;
    int e = profundidade_arvore(no->esquerda);
    int d = profundidade_arvore(no->direita);
    return 1 + (e > d ? e : d);
}

static void desenhar_no_arvore(AABBTREE* no, int profundidade, int prof_max) {
    if (no == NULL) return;

    float t = (prof_max > 0) ? (float)profundidade / (float)prof_max : 0.0f;
    glColor3f(1.0f - t, t * 0.8f, t);
    glLineWidth(fmaxf(1.0f, 4.0f - profundidade * 0.5f));

    glBegin(GL_LINE_LOOP);
        glVertex2f(no->box.x_min, no->box.y_min);
        glVertex2f(no->box.x_max, no->box.y_min);
        glVertex2f(no->box.x_max, no->box.y_max);
        glVertex2f(no->box.x_min, no->box.y_max);
    glEnd();

    desenhar_no_arvore(no->esquerda,  profundidade + 1, prof_max);
    desenhar_no_arvore(no->direita,   profundidade + 1, prof_max);
}

void desenhar_arvore_aabb(AABBTREE* raiz) {
    if (raiz == NULL) return;
    int prof_max = profundidade_arvore(raiz) - 1;
    desenhar_no_arvore(raiz, 0, prof_max);
    glLineWidth(1.0f);
}
void gerar_curva_selecionada() {
  if (!precisa_refazer_curva) return;
  performance_iniciar_medicao();
  int fechado = estado_atual.poligono == MODO_POLIGONO_FECHADO;
  if (geometria_curvas_atualizar(&geometria_curva, &g_curva_atual,
                                 &vetor_boxes, &g_clicks,
                                 estado_atual.curva, fechado) != 0) {
    fprintf(stderr, "Erro ao atualizar geometria da curva\n");
  }
  estado_atual.qtd_nuvem_pontos_number = geometria_curva.vertices.quantidade_atual;
  sprintf(estado_atual.qtd_nuvem_pontos, "%d", estado_atual.qtd_nuvem_pontos_number);
  precisa_refazer_curva = 0;
}
void desenhar_aabbs(AABB_vec* v) {
    if (v == NULL || v->quantidade == 0) return;

    for (int i = 0; i < v->quantidade; i++) {
        // Varia a cor pelo índice — ciclo por matiz HSV simples
        float t = (float)i / (float)(v->quantidade);
        // R G B variando em ciclos defasados
        float r = 0.5f + 0.5f * sinf(t * 6.2831f);
        float g = 0.5f + 0.5f * sinf(t * 6.2831f + 2.094f); // +120 graus
        float b = 0.5f + 0.5f * sinf(t * 6.2831f + 4.189f); // +240 graus
        glColor3f(r, g, b);

        float x0 = v->dados[i].x_min;
        float y0 = v->dados[i].y_min;
        float x1 = v->dados[i].x_max;
        float y1 = v->dados[i].y_max;

        // Desenha os 4 lados da caixa
        glBegin(GL_LINE_LOOP);
            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        glEnd();
    }
}
void desenhar_curva_atual() {
  glLineWidth(0.1f);
  for (int faixa = 0; faixa < geometria_curva.quantidade_faixas; faixa++) {
    int inicio = geometria_curva.faixas[faixa].inicio;
    int fim = inicio + geometria_curva.faixas[faixa].quantidade;
    glBegin(GL_LINE_STRIP);
    for (int i = inicio; i < fim; i++) {
      glVertex2f(geometria_curva.vertices.data[i].point[0],
                 geometria_curva.vertices.data[i].point[1]);
    }
    glEnd();
  }
}

// Suas funções existentes...
void AlteraTamanhoJanela(int w, int h) {
  // Recalcular dimensões do menu
  atualizar_dimensoes_menu();

  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  int meio_x = (w - menu_largura) / 2;
  int meio_y = h / 2;
  gluOrtho2D(-meio_x, meio_x, -meio_y, meio_y);

  glMatrixMode(GL_MODELVIEW);

  glutPostRedisplay(); // Forçar redesenho
}

void desenhar_centro_poligono() {
  if (g_clicks.quantidade_atual < 2)
    return;
  // Desenhar cruz no centro
  glColor3f(1.0f, 1.0f, 0.0f); // Magenta
  glLineWidth(2.0f);

  glBegin(GL_LINES);
  glVertex2f(centro.point[0] - 10, centro.point[1]);
  glVertex2f(centro.point[0] + 10, centro.point[1]);
  glVertex2f(centro.point[0], centro.point[1] - 10);
  glVertex2f(centro.point[0], centro.point[1] + 10);
  glEnd();

  glPointSize(8.0f);
  glBegin(GL_POINTS);
  glVertex2f(centro.point[0], centro.point[1]);
  glEnd();
}

void desenhar_conteudo_principal() {
  // Desenhar pontos de controle
  glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
  glPointSize(8.0f);
  glBegin(GL_POINTS);
  for (int i = 0; i < g_clicks.quantidade_atual; i++) {
    glVertex2f(g_clicks.data[i].point[0], g_clicks.data[i].point[1]);
  }
  glEnd();
  glColor3f(0.0f, 1.0f, 0.0f); // Verde
  glLineWidth(0.1f);

  switch (estado_atual.poligono) {
  case MODO_POLIGONO_FECHADO:
    if (g_clicks.quantidade_atual <= 2) {
      glBegin(GL_LINE_SMOOTH);
      for (int i = 0; i < g_clicks.quantidade_atual; i++) {
        glVertex2f(g_clicks.data[i].point[0], g_clicks.data[i].point[1]);
      }
      glEnd();
    } else {
      glBegin(GL_LINE_LOOP);
      for (int i = 0; i < g_clicks.quantidade_atual; i++) {
        glVertex2f(g_clicks.data[i].point[0], g_clicks.data[i].point[1]);
      }
      glEnd();
    }
    break;
  case MODO_POLIGONO_ABERTO:
    if (g_clicks.quantidade_atual >= 2) {
      glBegin(GL_LINE_STRIP);
      for (int i = 0; i < g_clicks.quantidade_atual; i++) {
        glVertex2f(g_clicks.data[i].point[0], g_clicks.data[i].point[1]);
      }
      glEnd();
    }
    break;
  }

  desenhar_centro_poligono();
  gerar_curva_selecionada();
  desenhar_curva_atual();
}

void display() {
  int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
  int altura_janela = glutGet(GLUT_WINDOW_HEIGHT);
  int largura_desenho = largura_janela - menu_largura;

  glClear(GL_COLOR_BUFFER_BIT);

  //  PARTE 1: DESENHAR ÁREA PRINCIPAL
  glViewport(0, 0, largura_desenho, altura_janela);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D((float)(largura_desenho) / 2 * (-1), (float)(largura_desenho) / 2,
             (float)(altura_janela) / 2 * (-1), (float)(altura_janela) / 2);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Desenhar eixos
  glColor3f(0.5f, 0.5f, 0.5f);
  glBegin(GL_LINES);
  glVertex2f(-(int)(largura_desenho / 2), 0);
  glVertex2f((int)(largura_desenho / 2), 0);
  glVertex2f(0, -(int)(altura_janela / 2));
  glVertex2f(0, (int)(altura_janela / 2));
  glEnd();

  // Desenhar conteúdo
  desenhar_conteudo_principal();

  // === PARTE 2: DESENHAR MENU LATERAL ===
  glViewport(largura_desenho, 0, menu_largura, altura_janela);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, menu_largura, 0, altura_janela);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Fundo do menu
  glColor3f(0.8f, 0.8f, 0.8f);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(menu_largura, 0);
  glVertex2f(menu_largura, altura_janela);
  glVertex2f(0, altura_janela);
  glEnd();

  desenhar_botoes_menu();
  desenhar_info();
  glFlush();
}

void processar_clique_desenho(int x, int y) {
  x = traduzCoordenadaX(x);
  y = traduzCoordenadaY(y);
  ponto mouse;
  mouse.point[0] = x;
  mouse.point[1] = y;
  mouse.point[2] = 1;
  switch (estado_atual.criacao_ou_selecao) {
  case MODO_APAGAR_PONTO: 
    for (int i = 0; i < g_clicks.quantidade_atual; i++){
      if( calcula_distancia( mouse, g_clicks.data[i]) < 4){
        for(int j = i; j < g_clicks.quantidade_atual - 1;j++){
          g_clicks.data[j] = g_clicks.data[j + 1];
        }
        g_clicks.quantidade_atual--;
        sprintf(estado_atual.qtd_pontos_controle, "%d", g_clicks.quantidade_atual); 
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break;
      }
    }

    break;
  case MODO_CRIAR_PONTO:
    pontos_push(&g_clicks, mouse);
    sprintf(estado_atual.qtd_pontos_controle, "%d", g_clicks.quantidade_atual); 
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
    break;
  case MODO_SELECIONAR_PONTO:
      for (int i = 0; i < g_clicks.quantidade_atual; i++) {
        if (calcula_distancia(mouse, g_clicks.data[i]) < 6) {
          selecao_ponto.selecionado = 1;
          selecao_ponto.indice = i;
          break;
        }
      }
    break;
  case MODO_SELEICIONAR_CURVA:
    switch (estado_atual.curva) {
      case MODO_CURVA_BSPLINE: {
        ResultadoPicking resultado = picking_bspline(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);

        if(resultado.segmento_indice != -1){
          selecao_curva.seg_curva = resultado.segmento_indice;
          selecao_curva.t =  resultado.t;
          selecao_curva.curva_selecionada = MODO_CURVA_BSPLINE;
        }
        break;
      }
      case MODO_CURVA_BEZIER: {
        ResultadoPicking resultado = picking_bezier(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);

        if (resultado.segmento_indice != -1) {
          selecao_curva.seg_curva = resultado.segmento_indice;
          selecao_curva.t = resultado.t;
          selecao_curva.curva_selecionada = MODO_CURVA_BEZIER;
        }
        break;
      }
      case MODO_CURVA_HERMITE: {
        ResultadoPicking resultado = picking_hermite(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);
        if (resultado.segmento_indice != -1) {
          selecao_curva.seg_curva = resultado.segmento_indice;
          selecao_curva.t = resultado.t;
          selecao_curva.curva_selecionada = MODO_CURVA_HERMITE;
        }
        break;
      }
      case MODO_CURVA_CATMULLROM: {
        ResultadoPicking resultado = picking_catmullrom(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);
        if (resultado.segmento_indice != -1) {
          selecao_curva.seg_curva = resultado.segmento_indice;
          selecao_curva.t = resultado.t;
          selecao_curva.curva_selecionada = MODO_CURVA_CATMULLROM;
        }
        break;
      }
      default:
        printf("\n\tNão era para ser possível chegar aqui.");
        exit(-1);
        break;
    }
    break;
  case MODO_SELECIONAR_POLIGONO:
    switch (estado_atual.operacao) {
    case TRANSLACAO:
      if (g_clicks.quantidade_atual >= 2) {
        if ((calcula_distancia(centro, mouse)) < 6) {
          selecao_poligono = 1;
          break;
        }
        for (int i = 0; i < g_clicks.quantidade_atual; i++) {
          if (calcula_distancia(mouse, g_clicks.data[i]) < 6) {
            selecao_poligono = 2;
            selecao_ponto.indice = i;
            break;
          }
        }
        break;
      }
      break;
    case ROTACAO:
      printf("rotacao agora");
      if(g_clicks.quantidade_atual >= 2){
        for(int i = 0; i < g_clicks.quantidade_atual; i ++){
          if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
            selecao_poligono = 3;
            selecao_ponto.indice = i;
          }
        }
      }
      break;

    case ESCALA:
      printf("escala agora");
      if(g_clicks.quantidade_atual >= 2){
        for(int i = 0; i < g_clicks.quantidade_atual; i ++){
          if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
            selecao_poligono = 4;
            selecao_ponto.indice = i;
          }
        }
      }
      break;
    case SHEAR:
      printf("Shear agora");
      break;
    case NENHUMA:
      break;
    }

  }
}
int traduzCoordenadaX(int x) {
  int largura = (glutGet(GLUT_WINDOW_WIDTH) - menu_largura) / 2;
  return (x < largura) ? (largura - x) * (-1) : x - largura;
}

int traduzCoordenadaY(int y) {
  int altura = glutGet(GLUT_WINDOW_HEIGHT) / 2;
  return (y < altura) ? (altura - y) : (y - altura) * (-1);
}

void verificar_clique_botao_generico(void *botao, TipoBotao tipo, int x,
                                     int y) {
  int x_botao, y_botao, altura_botao, largura_botao, mudanca;
  Criacao_ou_selecao criacao_ou_selecao_ponto;
  Poligono poligono;
  Curva curva;
  Operacoes operacao;
  // printf("função começa\n");
  switch (tipo) {
  case TIPO_BOTAO1: {
    Botoes1 *b = (Botoes1 *)botao;
    x_botao = b->x;
    y_botao = b->y;
    altura_botao = b->altura;
    largura_botao = b->largura;
    criacao_ou_selecao_ponto = b->acao;
    mudanca = 0;
    break;
  }
  case TIPO_BOTAO2: {
    Botoes2 *b = (Botoes2 *)botao;
    x_botao = b->x;
    y_botao = b->y;
    altura_botao = b->altura;
    largura_botao = b->largura;
    poligono = b->acao;
    mudanca = 1;
    break;
  }
  case TIPO_BOTAO3: {
    Botoes3 *b = (Botoes3 *)botao;
    x_botao = b->x;
    y_botao = b->y;
    altura_botao = b->altura;
    largura_botao = b->largura;
    curva = b->acao;
    mudanca = 2;
    break;
  }
  case TIPO_OPERACAO: {
    BotoesOperacoes *b = (BotoesOperacoes *)botao;
    x_botao = b->x;
    y_botao = b->y;
    altura_botao = b->altura;
    largura_botao = b->largura;
    operacao = b->acao;
    mudanca = 3;
    break;
  }
  }
  //  Verificar se clique foi dentro do botão
  if (x >= x_botao && x <= x_botao + largura_botao && y >= y_botao &&
      y <= y_botao + altura_botao) {
    switch (mudanca) {
    case 0:
      precisa_refazer_curva = (criacao_ou_selecao_ponto == estado_atual.criacao_ou_selecao) ? 0 : 1;
      estado_atual.criacao_ou_selecao = criacao_ou_selecao_ponto;
      if (estado_atual.criacao_ou_selecao == MODO_CRIAR_PONTO) {
        estado_atual.operacao = NENHUMA;
      }
      break;
    case 1:
      precisa_refazer_curva = (poligono == estado_atual.poligono) ? 0 : 1;
      estado_atual.poligono = poligono;
      break;
    case 2:
      precisa_refazer_curva = (curva == estado_atual.curva) ? 0 : 1;
      estado_atual.curva = curva;
      printf("Agora é %u\n", estado_atual.curva);
      break;
    case 3:
      precisa_refazer_curva = (operacao == estado_atual.operacao ) ? 0 : 1;
      estado_atual.operacao = operacao;
      break;
    }
    if (precisa_refazer_curva) glutPostRedisplay();
  }
}
void processar_clique_menu(int x, int y) {
  int altura_janela = glutGet(GLUT_WINDOW_HEIGHT);
  y = altura_janela - y; // Inverter Y (GLUT usa origem superior esquerda)
  if (y <= botoes.botoes1[4].y +  botoes.botoes1[4].altura + 2) {
    for (int i = 0; i < 5; i++) {
      verificar_clique_botao_generico(&botoes.botoes1[i], TIPO_BOTAO1, x, y);
    }
  } else if (y <=  botoes.botoes2[1].y +  botoes.botoes2[1].altura + 22) {
    for (int i = 0; i < 2; i++) {
      verificar_clique_botao_generico(&botoes.botoes2[i], TIPO_BOTAO2, x, y);
    }
  } else if (y <=  botoes.botoes3[3].y + botoes.botoes3[3].altura + 2) {
    for (int i = 0; i < 4; i++) {
      verificar_clique_botao_generico(&botoes.botoes3[i], TIPO_BOTAO3, x, y);
    }
  } else if (estado_atual.criacao_ou_selecao != MODO_CRIAR_PONTO) {
    for (int i = 0; i < 4; i++) {
      verificar_clique_botao_generico(&botoes_operacoes[i], TIPO_OPERACAO, x,
                                      y);
    }
  }
}

void onMouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
      int largura_desenho = largura_janela - menu_largura;
      if (x < largura_desenho) {
        // Clique na área de desenho
        processar_clique_desenho(x, y);
      } else {
        // Clique no menu
        processar_clique_menu(x - largura_desenho, y);
      }
    } else {
      /* A geometria fica atualizada durante o arrasto; a árvore, usada apenas
         por picking, continua sendo reconstruída somente ao soltar o mouse. */
      gerar_curva_selecionada();
      // Libera árvore anterior antes de recriar
      aabb_tree_free(arvore_boxes);
      printf("a quantidade de caixas é: %d", vetor_boxes.quantidade);
      arvore_boxes = aabb_vec_para_arvore(&vetor_boxes);
      if (selecao_ponto.selecionado) {
        selecao_ponto.selecionado = 0;
        selecao_ponto.indice = 0;
      }
      if (selecao_poligono) {
        selecao_poligono = 0;
      }
      if(selecao_curva.seg_curva >= 0){
        selecao_curva.seg_curva = -1;
      }
    }
  }
  if ((button == 3 || button == 4) && estado_atual.operacao == ESCALA) {
    ponto mouse;
    mouse.point[0] = traduzCoordenadaX(x);
    mouse.point[1] = traduzCoordenadaY(y);
    mouse.point[2] = 1;
    if (calcula_distancia(mouse, centro) < 3) {
      if (button == 3) {
        aumentar_escala(&g_clicks, centro);
      } else {
        diminuir_escala(&g_clicks, centro);
      }
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
    }
  }
}

void onMouseMove(int x, int y) {
  int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
  int largura_desenho = largura_janela - menu_largura;
  int change = 0;
  int estado = 0;
  int menu_x = x - largura_desenho;
  int menu_y = glutGet(GLUT_WINDOW_HEIGHT) - y;

  if (x >= largura_desenho) {
    // Mouse no menu
    
    for (int i = 0; i < 5; i++) {
      Botoes1 *botao = &botoes.botoes1[i];
      estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y && menu_y <= botao->y + botao->altura);
      change += ((botao->destacado) != estado) ? 1: 0;
      botao->destacado = estado;
    }
    for (int i = 0; i < 2; i++) {
      Botoes2 *botao = &botoes.botoes2[i];
       estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y && menu_y <= botao->y + botao->altura);
        change += ((botao->destacado) != estado) ? 1: 0;
        botao->destacado = estado;
    }
    for (int i = 0; i < 4; i++) {
      Botoes3 *botao = &botoes.botoes3[i];
      estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y && menu_y <= botao->y + botao->altura);
      change += ((botao->destacado) != estado) ? 1: 0;
      botao->destacado = estado;
    }
    if (estado_atual.criacao_ou_selecao != MODO_CRIAR_PONTO) {
      for (int i = 0; i < 4; i++) {
        BotoesOperacoes *botao = &botoes_operacoes[i];
        estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y && menu_y <= botao->y + botao->altura);
        change += ((botao->destacado) != estado) ? 1: 0;
        botao->destacado = estado;
      }
    }
    for(int i = 0; i < 2; i++){
      InfoItem* information = &info[i];
      estado = (menu_x >= information->x && menu_x <= information->x + information->largura && menu_y >= information->y && menu_y <= information->y + information->altura);
      change += ((information->destacado) != estado) ? 1: 0;
      information->destacado =  estado;
    }
    if(change){ 
      //precis_recalcular_curva
      precisa_refazer_curva = 1;
      glutPostRedisplay();
    }
  } 
}

void onMotion(int x, int y) {
  x = traduzCoordenadaX(x);
  y = traduzCoordenadaY(y);
  ponto mouse;
  mouse.point[0] = x;
  mouse.point[1] = y;
  if(selecao_curva.seg_curva >=0){
    switch (selecao_curva.curva_selecionada) {
      case MODO_CURVA_BSPLINE:
        arrastar_ponto_bspline(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse);
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break;
      case MODO_CURVA_HERMITE:
        arrastar_ponto_hermite(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse);
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break;
      case MODO_CURVA_BEZIER:
        arrastar_ponto_bezier(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse);
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break;
      case MODO_CURVA_CATMULLROM:
        arrastar_ponto_catmullrom(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse);
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break; 
      default:
        printf("\n\tNão era para chegar aqui");
        exit(-1);
        break;
    }
 } else if (selecao_ponto.selecionado) {
    g_clicks.data[selecao_ponto.indice].point[0] = x;
    g_clicks.data[selecao_ponto.indice].point[1] = y;
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay(); // Redesenhar se necessário
  } else if (selecao_poligono == 1 &&
             estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO) {
    translacao_com_mouse(&g_clicks, centro, x, y);
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if (selecao_poligono == 2 &&
             estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO) {
    translacao_com_mouse(&g_clicks, g_clicks.data[selecao_ponto.indice], x, y);
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if(selecao_poligono == 3 &&
            estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
            rotacionar_com_mouse(&g_clicks, &centro, mouse, selecao_ponto.indice);
            precisa_refazer_curva = 1;
            glutPostRedisplay();
  } else if(selecao_poligono == 4 &&
            estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
            escala_com_mouse(&g_clicks, &centro, mouse, selecao_ponto.indice);
            precisa_refazer_curva = 1;
            glutPostRedisplay();
  }
}
