#define _POSIX_C_SOURCE 199309L
#define GL_GLEXT_PROTOTYPES
#include "./../includes/opengl.h"
#include "./../includes/obb.h"
#include <GL/freeglut_std.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef FLT_MAX
#define FLT_MAX 99999999.0f
#endif
/* Estado privado da interface. */
static Pontos g_clicks;
static Pontos g_curva_atual;
static Selecao selecao_ponto;
static Selecao_curva selecao_curva;
static int selecao_poligono;
static ponto centro;
static int precisa_refazer_curva;
static AABB_vec vetor_boxes;
static AABBTREE *arvore_boxes;
static GeometriaCurvas geometria_curva;
static BSplineTopologia bspline_topologica;
static OBBArvore arvore_obb;
static int bspline_topologica_inicializada;
static int selecao_bspline_topologica;
static unsigned revisao_densidade_selecionada;
static float limiar_densidade_selecionado;
static int interacao_menu;
static Pontos controles_shear_iniciais;
static ponto centro_shear_inicial;
static int shear_arrastando;
static int eixo_shear;
static int modo_anti_autointerferencia;
typedef struct {
  GLuint curva;
  GLuint controles;
  size_t capacidade_curva;
  size_t capacidade_controles;
  uint64_t assinatura_controles;
  int disponivel;
} BuffersRenderizacao;
static BuffersRenderizacao buffers_renderizacao;
#define TOLERANCIA_FORMA_DENSIDADE 1e-3f
#define TOLERANCIA_AUTOINTERFERENCIA 0.05f
/* Configurações visuais. */
static float cor_fundo[3] = {0.2f, 0.3f, 0.4f};

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

static void atualizar_buffer_vertices(GLuint buffer, size_t *capacidade, const Pontos *pontos){
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

static void atualizar_buffers_renderizacao(void){
  if(!buffers_renderizacao.disponivel){ return; }
  atualizar_buffer_vertices(buffers_renderizacao.curva, &buffers_renderizacao.capacidade_curva,
                            &geometria_curva.vertices);
  uint64_t assinatura = assinatura_pontos(&g_clicks);
  if(assinatura != buffers_renderizacao.assinatura_controles){
    atualizar_buffer_vertices(buffers_renderizacao.controles,
                              &buffers_renderizacao.capacidade_controles, &g_clicks);
    buffers_renderizacao.assinatura_controles = assinatura;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void desenhar_buffer_vertices(GLuint buffer, GLenum modo, int quantidade){
  if(!buffers_renderizacao.disponivel || quantidade <= 0){ return; }
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, (GLsizei)sizeof(ponto), (const void *)0);
  glDrawArrays(modo, 0, quantidade);
  glDisableClientState(GL_VERTEX_ARRAY);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static double agora_benchmark_ms(void){
  struct timespec instante;
  clock_gettime(CLOCK_MONOTONIC, &instante);
  return (double)instante.tv_sec * 1000.0 + (double)instante.tv_nsec / 1000000.0;
}

static void desenhar_quadro_benchmark_vbo(void){
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(1.0f, 0.0f, 0.0f);
  glPointSize(8.0f);
  desenhar_buffer_vertices(buffers_renderizacao.controles, GL_POINTS, g_clicks.quantidade_atual);
  glColor3f(0.0f, 1.0f, 0.0f);
  glLineWidth(1.0f);
  desenhar_buffer_vertices(buffers_renderizacao.curva,
                           geometria_curva.fechado ? GL_LINE_LOOP : GL_LINE_STRIP,
                           geometria_curva.vertices.quantidade_atual);
  glFinish();
}

void initGL(){
  glClearColor(cor_fundo[0], cor_fundo[1], cor_fundo[2], 1.0f);
  glPointSize(7.0f);
  glLineWidth(2.0f);

  // Inicializar estrutura para curva
  pontos_init(&g_clicks, 20);
  pontos_init(&g_curva_atual, 100);
  pontos_init(&controles_shear_iniciais, 20);
  performance_definir_buffers(&g_clicks, &g_curva_atual);
  geometria_curvas_init(&geometria_curva);
  contadores_interface_atualizar(&estado_atual, &g_clicks, &geometria_curva.vertices);
  bspline_topologica_inicializada = 0;
  obb_arvore_init(&arvore_obb);
  selecao_bspline_topologica = 0;
  aabb_vec_init(&vetor_boxes, 20);
  selecao_ponto.selecionado = 0;
  selecao_ponto.indice = 0;
  selecao_poligono = 0;
  shear_arrastando = 0;
  eixo_shear = 0;
  glGenBuffers(1, &buffers_renderizacao.curva);
  glGenBuffers(1, &buffers_renderizacao.controles);
  buffers_renderizacao.disponivel =
      buffers_renderizacao.curva != 0 && buffers_renderizacao.controles != 0;
  buffers_renderizacao.assinatura_controles = UINT64_MAX;
  centro.point[0] = 0;
  centro.point[1] = 0;
  centro.point[2] = 1;
  printf("OpenGL inicializado com sucesso!\n");
}

void finalizar_aplicacao(void){
  if(buffers_renderizacao.curva){ glDeleteBuffers(1, &buffers_renderizacao.curva); }
  if(buffers_renderizacao.controles){ glDeleteBuffers(1, &buffers_renderizacao.controles); }
  aabb_tree_free(arvore_boxes);
  free(vetor_boxes.dados);
  geometria_curvas_free(&geometria_curva);
  if(bspline_topologica_inicializada){
    bspline_topologia_liberar(&bspline_topologica);
  }
  obb_arvore_liberar(&arvore_obb);
  pontos_free(&g_curva_atual);
  pontos_free(&g_clicks);
  pontos_free(&controles_shear_iniciais);
}

void opengl_benchmark_vbo_executar(int quantidade_controles, int quadros){
  if(quantidade_controles < 4){ quantidade_controles = 4; }
  if(quadros < 20){ quadros = 20; }
  estado_atual.curva = MODO_CURVA_BSPLINE;
  estado_atual.poligono = MODO_POLIGONO_ABERTO;
  for(int i = 0; i < quantidade_controles; i++){
    float fracao = (float)i / (float)(quantidade_controles - 1);
    ponto controle = {{-350.0f + 700.0f * fracao,
                       sinf((float)i * 1.6180339f) * 260.0f, 1.0f}};
    if(pontos_push(&g_clicks, controle) != 0){
      fprintf(stderr, "Erro ao criar controles para benchmark VBO\n");
      return;
    }
  }
  precisa_refazer_curva = 1;
  gerar_curva_selecionada();
  glViewport(0, 0, 800, 800);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-400.0, 400.0, -400.0, 400.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  for(int i = 0; i < 20; i++){
    desenhar_quadro_benchmark_vbo();
  }
  double inicio = agora_benchmark_ms();
  for(int i = 0; i < quadros; i++){
    desenhar_quadro_benchmark_vbo();
  }
  double desenho_ms = (agora_benchmark_ms() - inicio) / (double)quadros;

  inicio = agora_benchmark_ms();
  for(int i = 0; i < quadros; i++){
    int indice = quantidade_controles / 2;
    g_clicks.data[indice].point[1] = sinf((float)i * 0.1f) * 260.0f;
    precisa_refazer_curva = 1;
    gerar_curva_selecionada();
    desenhar_quadro_benchmark_vbo();
  }
  double completo_ms = (agora_benchmark_ms() - inicio) / (double)quadros;
  printf("[BENCH-VBO] controles=%d | vertices=%d | estatico=%.3f ms (%.1f FPS) | "
         "atualizacao+upload+desenho=%.3f ms (%.1f FPS)\n",
         g_clicks.quantidade_atual, geometria_curva.vertices.quantidade_atual, desenho_ms,
         desenho_ms > 0.0 ? 1000.0 / desenho_ms : 0.0, completo_ms,
         completo_ms > 0.0 ? 1000.0 / completo_ms : 0.0);
}

static int sincronizar_bspline_topologica(int fechado){
  if(g_clicks.quantidade_atual < 4){
    return -1;
  }
  if(!bspline_topologica_inicializada ||
     bspline_topologica.fechado != fechado ||
     bspline_topologica.quantidade_controles_base != g_clicks.quantidade_atual){
    if(bspline_topologica_inicializada){
      bspline_topologia_liberar(&bspline_topologica);
    }
    int resultado = fechado ?
                        bspline_topologia_inicializar_uniforme_fechada(&bspline_topologica, &g_clicks, 3) :
                        bspline_topologia_inicializar_uniforme_aberta(&bspline_topologica, &g_clicks, 3);
    if(resultado != 0){
      return -1;
    }
    bspline_topologica_inicializada = 1;
    obb_arvore_liberar(&arvore_obb);
    return 0;
  }
  for(int i = 0; i < g_clicks.quantidade_atual; i++){
    bspline_topologica.controles.data[i] = g_clicks.data[i];
  }
  return 0;
}

static int reconstruir_arvore_obb_bspline(void){
  if(!bspline_topologica_inicializada){
    return -1;
  }
  return obb_arvore_construir_bspline(&arvore_obb, &bspline_topologica);
}

static int refit_obb_bspline_arrastada(int span_central){
  if(arvore_obb.quantidade_folhas <= 0){
    return -1;
  }
  int segmentos[7];
  int quantidade = 0;
  for(int deslocamento = -3; deslocamento <= 3; deslocamento++){
    int segmento = span_central + deslocamento;
    if(bspline_topologica.fechado){
      segmento %= arvore_obb.quantidade_folhas;
      if(segmento < 0){ segmento += arvore_obb.quantidade_folhas; }
    } else if(segmento < 0 || segmento >= arvore_obb.quantidade_folhas){
      continue;
    }
    segmentos[quantidade++] = segmento;
  }
  return obb_arvore_refit_bspline(&arvore_obb, &bspline_topologica, segmentos, quantidade);
}

static int sincronizar_controles_da_bspline(void){
  if(!bspline_topologica_inicializada){
    return -1;
  }
  g_clicks.quantidade_atual = 0;
  for(int i = 0; i < bspline_topologica.quantidade_controles_base; i++){
    if(pontos_push(&g_clicks, bspline_topologica.controles.data[i]) != 0){
      return -1;
    }
  }
  contadores_interface_atualizar(&estado_atual, &g_clicks, &geometria_curva.vertices);
  return 0;
}

typedef struct {
  BSplineTopologia spline;
  Selecao_curva selecao;
  unsigned revisao_densidade;
  float limiar_densidade;
  ponto centro;
  int valido;
} SnapshotArrastoBSpline;

static void snapshot_liberar(SnapshotArrastoBSpline *snapshot){
  if(snapshot->valido){ bspline_topologia_liberar(&snapshot->spline); }
  *snapshot = (SnapshotArrastoBSpline){0};
}

static int snapshot_capturar(SnapshotArrastoBSpline *snapshot){
  *snapshot = (SnapshotArrastoBSpline){0};
  if(bspline_topologia_copiar(&bspline_topologica, &snapshot->spline) != 0){ return -1; }
  snapshot->selecao = selecao_curva;
  snapshot->revisao_densidade = revisao_densidade_selecionada;
  snapshot->limiar_densidade = limiar_densidade_selecionado;
  snapshot->centro = centro;
  snapshot->valido = 1;
  return 0;
}

static int snapshot_restaurar(SnapshotArrastoBSpline *snapshot){
  if(!snapshot->valido){ return -1; }
  bspline_topologia_liberar(&bspline_topologica);
  bspline_topologica = snapshot->spline;
  snapshot->spline = (BSplineTopologia){0};
  snapshot->valido = 0;
  selecao_curva = snapshot->selecao;
  revisao_densidade_selecionada = snapshot->revisao_densidade;
  limiar_densidade_selecionado = snapshot->limiar_densidade;
  centro = snapshot->centro;
  if(reconstruir_arvore_obb_bspline() != 0 || sincronizar_controles_da_bspline() != 0){ return -1; }
  return 0;
}

static int bspline_possui_autointerseccao(void){
  return obb_bspline_autointerseccoes(&arvore_obb, &bspline_topologica,
                                      TOLERANCIA_AUTOINTERFERENCIA, NULL, 0);
}

static int bspline_possui_autointerseccao_local(int span_central){
  int segmentos[7];
  int quantidade = 0;
  for(int deslocamento = -3; deslocamento <= 3; deslocamento++){
    int segmento = span_central + deslocamento;
    if(bspline_topologica.fechado){
      segmento %= arvore_obb.quantidade_folhas;
      if(segmento < 0){ segmento += arvore_obb.quantidade_folhas; }
    } else if(segmento < 0 || segmento >= arvore_obb.quantidade_folhas){
      continue;
    }
    segmentos[quantidade++] = segmento;
  }
  return obb_bspline_autointerseccoes_locais(&arvore_obb, &bspline_topologica, segmentos,
                                              quantidade, TOLERANCIA_AUTOINTERFERENCIA,
                                              NULL, 0);
}

int definir_modo_anti_autointerferencia(int ativo){
  if(!ativo){
    modo_anti_autointerferencia = 0;
    estado_atual.anti_autointerferencia = 0;
    return 0;
  }
  if(estado_atual.curva != MODO_CURVA_BSPLINE || !bspline_topologica_inicializada ||
     arvore_obb.quantidade_folhas <= 0 || bspline_possui_autointerseccao() != 0){
    modo_anti_autointerferencia = 0;
    estado_atual.anti_autointerferencia = 0;
    return -1;
  }
  modo_anti_autointerferencia = 1;
  estado_atual.anti_autointerferencia = 1;
  return 0;
}

/* Toda alteração de nós feita pela interface passa por aqui. A referência
 * selecionada é preservada em u_global e só recebe um novo span/t local após
 * a transação de Oslo ou Lyche-Morken ter terminado. */
static int mutar_topologia_bspline(const float *nos, int quantidade, int remover){
  ReferenciaArrastoBSpline referencia;
  int possui_referencia = selecao_bspline_topologica && selecao_curva.seg_curva >= 0;
  if(possui_referencia &&
     bspline_topologia_capturar_referencia_arrasto(&bspline_topologica,
                                                   selecao_curva.u_global, &referencia) != 0){
    return -1;
  }
  int resultado = remover ?
      remover_nos_lyche_morken(&bspline_topologica, nos, quantidade, TOLERANCIA_FORMA_DENSIDADE) :
      inserir_nos_oslo(&bspline_topologica, nos, quantidade);
  if(resultado < 0){
    return -1;
  }
  int alterou = remover ? resultado > 0 : 1;
  if(alterou && possui_referencia){
    if(bspline_topologia_remapear_referencia_arrasto(&bspline_topologica, &referencia,
                                                     TOLERANCIA_FORMA_DENSIDADE) != 0){
      return -1;
    }
    selecao_curva.u_global = referencia.u_global;
    selecao_curva.seg_curva = referencia.segmento_indice;
    selecao_curva.t_local = referencia.t_local;
  }
  if(alterou && reconstruir_arvore_obb_bspline() != 0){
    return -1;
  }
  return resultado;
}

static void normalizar_densidade_bspline(int preferir_remocao, int todos_os_spans){
  float nos[256];
  if(estado_atual.curva != MODO_CURVA_BSPLINE ||
     sincronizar_bspline_topologica(estado_atual.poligono == MODO_POLIGONO_FECHADO) != 0){
    return;
  }
  int quantidade = 0;
  int ultimo_span = 0;
  float inicio, fim;
  while(bspline_topologia_obter_span(&bspline_topologica, ultimo_span, &inicio, &fim, NULL) == 0){
    ultimo_span++;
  }
  int primeiro = todos_os_spans ? 0 : ultimo_span - 1;
  for(int central = primeiro; central < ultimo_span; central++){
    float locais[7];
    int locais_qtd = preferir_remocao ?
        bspline_topologia_candidatos_remocao(&bspline_topologica, central,
                                              obter_limiar_densidade(), locais, 7) :
        bspline_topologia_candidatos_insercao(&bspline_topologica, central,
                                               obter_limiar_densidade(), locais, 7);
    for(int i = 0; i < locais_qtd; i++){
      int repetido = 0;
      for(int j = 0; j < quantidade; j++){
        if(fabsf(nos[j] - locais[i]) <= 1e-6f){ repetido = 1; break; }
      }
      if(!repetido && quantidade < (int)(sizeof(nos) / sizeof(nos[0]))){
        nos[quantidade++] = locais[i];
      }
    }
  }
  if(quantidade > 0){
    if(mutar_topologia_bspline(nos, quantidade, preferir_remocao) >= 0){
      sincronizar_controles_da_bspline();
    }
  }
}

static int atualizar_densidade_bspline_arrastada(void){
  float nos[7];
  float limiar = obter_limiar_densidade();
  unsigned revisao = obter_revisao_densidade();
  int alterados;

  if(!bspline_topologica_inicializada){
    return 0;
  }

  if(revisao != revisao_densidade_selecionada && limiar > limiar_densidade_selecionado){
    alterados = bspline_topologia_candidatos_remocao(&bspline_topologica,
                                                      selecao_curva.seg_curva, limiar, nos, 7);
    if(alterados > 0){
      alterados = mutar_topologia_bspline(nos, alterados, 1);
    }
  } else {
    alterados = bspline_topologia_candidatos_insercao(&bspline_topologica,
                                                       selecao_curva.seg_curva, limiar, nos, 7);
    if(alterados > 0 && mutar_topologia_bspline(nos, alterados, 0) < 0){
      return -1;
    }
  }
  revisao_densidade_selecionada = revisao;
  limiar_densidade_selecionado = limiar;
  if(alterados < 0){
    return -1;
  }
  if(alterados > 0){
    if(sincronizar_controles_da_bspline() != 0){
      return -1;
    }
  }
  return alterados;
}

static int iniciar_shear(void){
  if(g_clicks.quantidade_atual < 2){
    return 0;
  }
  if(g_clicks.quantidade_atual > controles_shear_iniciais.quantidade_max){
    pontos_free(&controles_shear_iniciais);
    pontos_init(&controles_shear_iniciais, (size_t)g_clicks.quantidade_atual);
  }
  memcpy(controles_shear_iniciais.data, g_clicks.data,
         (size_t)g_clicks.quantidade_atual * sizeof(*g_clicks.data));
  controles_shear_iniciais.quantidade_atual = g_clicks.quantidade_atual;
  centro_shear_inicial = centro;
  shear_arrastando = 1;
  eixo_shear = 0;
  return 1;
}

static int restaurar_controles_shear(void){
  if(!shear_arrastando || controles_shear_iniciais.quantidade_atual != g_clicks.quantidade_atual){
    return 0;
  }
  memcpy(g_clicks.data, controles_shear_iniciais.data,
         (size_t)g_clicks.quantidade_atual * sizeof(*g_clicks.data));
  centro = centro_shear_inicial;
  return 1;
}
static int profundidade_arvore(AABBTREE *no){
  if(no == NULL){
    return 0;
  }
  int e = profundidade_arvore(no->esquerda);
  int d = profundidade_arvore(no->direita);
  return 1 + (e > d ? e : d);
}

static void desenhar_no_arvore(AABBTREE *no, int profundidade, int prof_max){
  if(no == NULL){
    return;
  }

  float t = (prof_max > 0) ? (float)profundidade / (float)prof_max : 0.0f;
  glColor3f(1.0f - t, t * 0.8f, t);
  glLineWidth(fmaxf(1.0f, 4.0f - profundidade * 0.5f));

  glBegin(GL_LINE_LOOP);
  glVertex2f(no->box.x_min, no->box.y_min);
  glVertex2f(no->box.x_max, no->box.y_min);
  glVertex2f(no->box.x_max, no->box.y_max);
  glVertex2f(no->box.x_min, no->box.y_max);
  glEnd();

  desenhar_no_arvore(no->esquerda, profundidade + 1, prof_max);
  desenhar_no_arvore(no->direita, profundidade + 1, prof_max);
}

void desenhar_arvore_aabb(AABBTREE *raiz){
  if(raiz == NULL){
    return;
  }
  int prof_max = profundidade_arvore(raiz) - 1;
  desenhar_no_arvore(raiz, 0, prof_max);
  glLineWidth(1.0f);
}
void gerar_curva_selecionada(){
  if(!precisa_refazer_curva){
    return;
  }
  performance_iniciar_medicao();
  int fechado = estado_atual.poligono == MODO_POLIGONO_FECHADO;
  int resultado;
  if(estado_atual.curva == MODO_CURVA_BSPLINE && sincronizar_bspline_topologica(fechado) == 0){
    performance_definir_buffers(&g_clicks, &geometria_curva.vertices);
    performance_iniciar_geracao_topologica();
    resultado = geometria_curvas_atualizar_bspline_topologica(&geometria_curva, &vetor_boxes,
                                                               &bspline_topologica);
    performance_finalizar_geracao_topologica((size_t)geometria_curva.vertices.quantidade_atual);
    if(arvore_obb.quantidade_folhas == 0 && reconstruir_arvore_obb_bspline() != 0){
      fprintf(stderr, "Erro ao construir arvore OBB\n");
    }
  } else {
    resultado = geometria_curvas_atualizar(&geometria_curva, &g_curva_atual, &vetor_boxes,
                                            &g_clicks, estado_atual.curva, fechado);
  }
  if(resultado != 0){
    fprintf(stderr, "Erro ao atualizar geometria da curva\n");
  }
  contadores_interface_atualizar(&estado_atual, &g_clicks, &geometria_curva.vertices);
  performance_definir_buffers(&g_clicks, &geometria_curva.vertices);
  atualizar_buffers_renderizacao();
  precisa_refazer_curva = 0;
}
void desenhar_aabbs(AABB_vec *v){
  if(v == NULL || v->quantidade == 0){
    return;
  }

  for(int i = 0; i < v->quantidade; i++){
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
void desenhar_curva_atual(){
  glLineWidth(0.1f);
  desenhar_buffer_vertices(buffers_renderizacao.curva,
                           geometria_curva.fechado ? GL_LINE_LOOP : GL_LINE_STRIP,
                           geometria_curva.vertices.quantidade_atual);
}

void AlteraTamanhoJanela(int w, int h){
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

void desenhar_centro_poligono(){
  if(g_clicks.quantidade_atual < 2){
    return;
  }
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

void desenhar_conteudo_principal(){
  gerar_curva_selecionada();
  // Desenhar pontos de controle
  glColor3f(1.0f, 0.0f, 0.0f); // Vermelho
  glPointSize(8.0f);
  desenhar_buffer_vertices(buffers_renderizacao.controles, GL_POINTS, g_clicks.quantidade_atual);
  glColor3f(0.0f, 1.0f, 0.0f); // Verde
  glLineWidth(0.1f);

  switch(estado_atual.poligono){
  case MODO_POLIGONO_FECHADO:
    if(g_clicks.quantidade_atual <= 2){
      desenhar_buffer_vertices(buffers_renderizacao.controles, GL_LINE_STRIP,
                               g_clicks.quantidade_atual);
    } else {
      desenhar_buffer_vertices(buffers_renderizacao.controles, GL_LINE_LOOP,
                               g_clicks.quantidade_atual);
    }
    break;
  case MODO_POLIGONO_ABERTO:
    if(g_clicks.quantidade_atual >= 2){
      desenhar_buffer_vertices(buffers_renderizacao.controles, GL_LINE_STRIP,
                               g_clicks.quantidade_atual);
    }
    break;
  }

  desenhar_centro_poligono();
  desenhar_curva_atual();
}

void display(){
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

void processar_clique_desenho(int x, int y){
  x = traduzCoordenadaX(x);
  y = traduzCoordenadaY(y);
  ponto mouse;
  mouse.point[0] = x;
  mouse.point[1] = y;
  mouse.point[2] = 1;
  switch(estado_atual.criacao_ou_selecao){
  case MODO_APAGAR_PONTO:
    for(int i = 0; i < g_clicks.quantidade_atual; i++){
      if(calcula_distancia(mouse, g_clicks.data[i]) < 4){
        for(int j = i; j < g_clicks.quantidade_atual - 1; j++){
          g_clicks.data[j] = g_clicks.data[j + 1];
        }
        g_clicks.quantidade_atual--;
        normalizar_densidade_bspline(0, 0);
        contadores_interface_atualizar(&estado_atual, &g_clicks, &geometria_curva.vertices);
        calcular_centro_medio(&centro, &g_clicks);
        precisa_refazer_curva = 1;
        glutPostRedisplay();
        break;
      }
    }

    break;
  case MODO_CRIAR_PONTO:
    pontos_push(&g_clicks, mouse);
    normalizar_densidade_bspline(0, 0);
    contadores_interface_atualizar(&estado_atual, &g_clicks, &geometria_curva.vertices);
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
    break;
  case MODO_SELECIONAR_PONTO:
    for(int i = 0; i < g_clicks.quantidade_atual; i++){
      if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
        selecao_ponto.selecionado = 1;
        selecao_ponto.indice = i;
        break;
      }
    }
    break;
  case MODO_SELECIONAR_CURVA:
    switch(estado_atual.curva){
    case MODO_CURVA_BSPLINE: {
      ResultadoPicking resultado;
      selecao_bspline_topologica =
          sincronizar_bspline_topologica(estado_atual.poligono == MODO_POLIGONO_FECHADO) == 0;
      if(selecao_bspline_topologica){
        resultado = bspline_topologia_picking_arvore(&bspline_topologica, arvore_boxes, mouse, 5.0f);
      } else {
        resultado = picking_bspline(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);
      }

      if(resultado.segmento_indice != -1){
        ReferenciaArrastoBSpline referencia;
        if(bspline_topologia_capturar_referencia_arrasto(&bspline_topologica, resultado.t,
                                                          &referencia) != 0){
          break;
        }
        selecao_curva.seg_curva = referencia.segmento_indice;
        selecao_curva.t = resultado.t;
        selecao_curva.u_global = referencia.u_global;
        selecao_curva.t_local = referencia.t_local;
        revisao_densidade_selecionada = obter_revisao_densidade();
        limiar_densidade_selecionado = obter_limiar_densidade();
        selecao_curva.curva_selecionada = MODO_CURVA_BSPLINE;
      }
      break;
    }
    case MODO_CURVA_BEZIER: {
      ResultadoPicking resultado = picking_bezier(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);

      if(resultado.segmento_indice != -1){
        selecao_curva.seg_curva = resultado.segmento_indice;
        selecao_curva.t = resultado.t;
        selecao_curva.curva_selecionada = MODO_CURVA_BEZIER;
      }
      break;
    }
    case MODO_CURVA_HERMITE: {
      ResultadoPicking resultado = picking_hermite(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);
      if(resultado.segmento_indice != -1){
        selecao_curva.seg_curva = resultado.segmento_indice;
        selecao_curva.t = resultado.t;
        selecao_curva.curva_selecionada = MODO_CURVA_HERMITE;
      }
      break;
    }
    case MODO_CURVA_CATMULLROM: {
      ResultadoPicking resultado =
          picking_catmullrom(arvore_boxes, &g_clicks, mouse, 5.0f, FLT_MAX);
      if(resultado.segmento_indice != -1){
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
    switch(estado_atual.operacao){
    case TRANSLACAO:
      if(g_clicks.quantidade_atual >= 2){
        if((calcula_distancia(centro, mouse)) < 6){
          selecao_poligono = 1;
          break;
        }
        for(int i = 0; i < g_clicks.quantidade_atual; i++){
          if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
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
        for(int i = 0; i < g_clicks.quantidade_atual; i++){
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
        for(int i = 0; i < g_clicks.quantidade_atual; i++){
          if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
            selecao_poligono = 4;
            selecao_ponto.indice = i;
          }
        }
      }
      break;
    case SHEAR:
      if(g_clicks.quantidade_atual >= 2){
        for(int i = 0; i < g_clicks.quantidade_atual; i++){
          if(calcula_distancia(mouse, g_clicks.data[i]) < 6){
            if(iniciar_shear()){
              selecao_poligono = 5;
              selecao_ponto.indice = i;
            }
            break;
          }
        }
      }
      break;
    case NENHUMA:
      break;
    }
  }
}
int traduzCoordenadaX(int x){
  int largura = (glutGet(GLUT_WINDOW_WIDTH) - menu_largura) / 2;
  return (x < largura) ? (largura - x) * (-1) : x - largura;
}

int traduzCoordenadaY(int y){
  int altura = glutGet(GLUT_WINDOW_HEIGHT) / 2;
  return (y < altura) ? (altura - y) : (y - altura) * (-1);
}

void verificar_clique_botao_generico(void *botao, TipoBotao tipo, int x, int y){
  int x_botao = 0, y_botao = 0, altura_botao = 0, largura_botao = 0, mudanca = 0;
  Criacao_ou_selecao criacao_ou_selecao_ponto = MODO_CRIAR_PONTO;
  Poligono poligono = MODO_POLIGONO_ABERTO;
  Curva curva = MODO_CURVA_HERMITE;
  Operacoes operacao = NENHUMA;
  switch(tipo){
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
  default:
    return;
  }
  //  Verificar se clique foi dentro do botão
  if(x >= x_botao && x <= x_botao + largura_botao && y >= y_botao && y <= y_botao + altura_botao){
    switch(mudanca){
    case 0:
      precisa_refazer_curva = (criacao_ou_selecao_ponto == estado_atual.criacao_ou_selecao) ? 0 : 1;
      estado_atual.criacao_ou_selecao = criacao_ou_selecao_ponto;
      if(estado_atual.criacao_ou_selecao == MODO_CRIAR_PONTO){
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
      break;
    case 3:
      precisa_refazer_curva = (operacao == estado_atual.operacao) ? 0 : 1;
      estado_atual.operacao = operacao;
      break;
    }
    if(precisa_refazer_curva){
      glutPostRedisplay();
    }
  }
}
int processar_clique_menu(int x, int y){
  int altura_janela = glutGet(GLUT_WINDOW_HEIGHT);
  y = altura_janela - y; // Inverter Y (GLUT usa origem superior esquerda)
  if(estado_atual.criacao_ou_selecao == MODO_SELECIONAR_CURVA &&
     slider_contem(&slider_rigidez, x, y)){
    estado_atual.rigidez_movimento =
        slider_valor_por_x(x, slider_rigidez.x, slider_rigidez.largura);
    slider_rigidez.arrastando = 1;
    glutPostRedisplay();
    return 1;
  }
  if(densidade_contem(&seletor_densidade, x, y)){
    int posicao =
        densidade_posicao_por_x(x, seletor_densidade.x, seletor_densidade.largura);
    if(posicao != estado_atual.posicao_densidade){
      int posicao_anterior = estado_atual.posicao_densidade;
      estado_atual.posicao_densidade = posicao;
      estado_atual.limiar_distancia_controles = densidade_limiar_por_posicao(posicao);
      estado_atual.revisao_densidade++;
      normalizar_densidade_bspline(posicao < posicao_anterior, 1);
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
    }
    return 1;
  }
  if(estado_atual.criacao_ou_selecao == MODO_SELECIONAR_CURVA &&
     estado_atual.curva == MODO_CURVA_BSPLINE &&
     botao_anti_autointerferencia_contem(x, y)){
    definir_modo_anti_autointerferencia(!modo_anti_autointerferencia);
    glutPostRedisplay();
    return 1;
  }
  if(y <= botoes.botoes1[4].y + botoes.botoes1[4].altura + 2){
    for(int i = 0; i < 5; i++){
      verificar_clique_botao_generico(&botoes.botoes1[i], TIPO_BOTAO1, x, y);
    }
  } else if(y <= botoes.botoes2[1].y + botoes.botoes2[1].altura + 22){
    for(int i = 0; i < 2; i++){
      verificar_clique_botao_generico(&botoes.botoes2[i], TIPO_BOTAO2, x, y);
    }
  } else if(y <= botoes.botoes3[3].y + botoes.botoes3[3].altura + 2){
    for(int i = 0; i < 4; i++){
      verificar_clique_botao_generico(&botoes.botoes3[i], TIPO_BOTAO3, x, y);
    }
  } else if(estado_atual.criacao_ou_selecao != MODO_CRIAR_PONTO){
    for(int i = 0; i < 4; i++){
      verificar_clique_botao_generico(&botoes_operacoes[i], TIPO_OPERACAO, x, y);
    }
  }
  if(estado_atual.anti_autointerferencia &&
     (estado_atual.criacao_ou_selecao != MODO_SELECIONAR_CURVA ||
      estado_atual.curva != MODO_CURVA_BSPLINE)){
    definir_modo_anti_autointerferencia(0);
  }
  return 0;
}

void onMouse(int button, int state, int x, int y){
  if(button == GLUT_LEFT_BUTTON){
    if(state == GLUT_DOWN){
      int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
      int largura_desenho = largura_janela - menu_largura;
      if(x < largura_desenho){
        // Clique na área de desenho
        processar_clique_desenho(x, y);
      } else {
        // Clique no menu
        interacao_menu = processar_clique_menu(x - largura_desenho, y);
      }
    } else {
      if(interacao_menu){
        interacao_menu = 0;
        finalizar_arrasto_menu();
        return;
      }
      if(finalizar_arrasto_menu()){
        return;
      }
      /* A geometria fica atualizada durante o arrasto; a árvore, usada apenas
         por picking, continua sendo reconstruída somente ao soltar o mouse. */
      gerar_curva_selecionada();
      // Libera árvore anterior antes de recriar
      aabb_tree_free(arvore_boxes);
      arvore_boxes = aabb_vec_para_arvore(&vetor_boxes);
      if(selecao_ponto.selecionado){
        selecao_ponto.selecionado = 0;
        selecao_ponto.indice = 0;
      }
      if(selecao_poligono){
        selecao_poligono = 0;
      }
      shear_arrastando = 0;
      eixo_shear = 0;
      if(selecao_curva.seg_curva >= 0){
        selecao_curva.seg_curva = -1;
      }
      selecao_bspline_topologica = 0;
    }
  }
  if((button == 3 || button == 4) && estado_atual.operacao == ESCALA){
    ponto mouse;
    mouse.point[0] = traduzCoordenadaX(x);
    mouse.point[1] = traduzCoordenadaY(y);
    mouse.point[2] = 1;
    if(calcula_distancia(mouse, centro) < 3){
      if(button == 3){
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

void onMouseMove(int x, int y){
  int largura_janela = glutGet(GLUT_WINDOW_WIDTH);
  int largura_desenho = largura_janela - menu_largura;
  int change = 0;
  int estado = 0;
  int menu_x = x - largura_desenho;
  int menu_y = glutGet(GLUT_WINDOW_HEIGHT) - y;

  if(x >= largura_desenho){
    // Mouse no menu

    for(int i = 0; i < 5; i++){
      Botoes1 *botao = &botoes.botoes1[i];
      estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y &&
                menu_y <= botao->y + botao->altura);
      change += ((botao->destacado) != estado) ? 1 : 0;
      botao->destacado = estado;
    }
    for(int i = 0; i < 2; i++){
      Botoes2 *botao = &botoes.botoes2[i];
      estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y &&
                menu_y <= botao->y + botao->altura);
      change += ((botao->destacado) != estado) ? 1 : 0;
      botao->destacado = estado;
    }
    for(int i = 0; i < 4; i++){
      Botoes3 *botao = &botoes.botoes3[i];
      estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y &&
                menu_y <= botao->y + botao->altura);
      change += ((botao->destacado) != estado) ? 1 : 0;
      botao->destacado = estado;
    }
    estado = estado_atual.criacao_ou_selecao == MODO_SELECIONAR_CURVA &&
             estado_atual.curva == MODO_CURVA_BSPLINE &&
             botao_anti_autointerferencia_contem(menu_x, menu_y);
    change += (botao_anti_autointerferencia.destacado != estado) ? 1 : 0;
    botao_anti_autointerferencia.destacado = estado;
    if(estado_atual.criacao_ou_selecao != MODO_CRIAR_PONTO){
      for(int i = 0; i < 4; i++){
        BotoesOperacoes *botao = &botoes_operacoes[i];
        estado = (menu_x >= botao->x && menu_x <= botao->x + botao->largura && menu_y >= botao->y &&
                  menu_y <= botao->y + botao->altura);
        change += ((botao->destacado) != estado) ? 1 : 0;
        botao->destacado = estado;
      }
    }
    for(int i = 0; i < 2; i++){
      InfoItem *information = &info[i];
      estado = (menu_x >= information->x && menu_x <= information->x + information->largura &&
                menu_y >= information->y && menu_y <= information->y + information->altura);
      change += ((information->destacado) != estado) ? 1 : 0;
      information->destacado = estado;
    }
    processar_mouse_move_menu(menu_x, menu_y);
    if(change){
      precisa_refazer_curva = 1;
      glutPostRedisplay();
    }
  } else {
    botao_anti_autointerferencia.destacado = 0;
    processar_mouse_move_menu(-1, -1);
  }
}

void onMotion(int x, int y){
  int largura_desenho = glutGet(GLUT_WINDOW_WIDTH) - menu_largura;
  int menu_y = glutGet(GLUT_WINDOW_HEIGHT) - y;
  if(processar_arrasto_menu(x - largura_desenho, menu_y)){
    return;
  }
  x = traduzCoordenadaX(x);
  y = traduzCoordenadaY(y);
  ponto mouse;
  mouse.point[0] = x;
  mouse.point[1] = y;
  if(selecao_curva.seg_curva >= 0){
    switch(selecao_curva.curva_selecionada){
    case MODO_CURVA_BSPLINE:
      if(selecao_bspline_topologica){
        SnapshotArrastoBSpline snapshot;
        int protegido = modo_anti_autointerferencia;
        int resultado = protegido ? snapshot_capturar(&snapshot) : 0;
        if(resultado == 0 &&
           bspline_topologia_arrastar(&bspline_topologica, selecao_curva.u_global, mouse,
                                      obter_rigidez_movimento()) == 0 &&
           refit_obb_bspline_arrastada(selecao_curva.seg_curva) == 0 &&
           sincronizar_controles_da_bspline() == 0 &&
           atualizar_densidade_bspline_arrastada() >= 0){
          if(protegido && bspline_possui_autointerseccao_local(selecao_curva.seg_curva) != 0){
            snapshot_restaurar(&snapshot);
          }
        } else if(protegido){
          snapshot_restaurar(&snapshot);
        }
        if(protegido){
          snapshot_liberar(&snapshot);
        }
      } else {
        arrastar_ponto_bspline(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse,
                               obter_rigidez_movimento());
      }
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
      break;
    case MODO_CURVA_HERMITE:
      arrastar_ponto_hermite(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse,
                             obter_rigidez_movimento());
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
      break;
    case MODO_CURVA_BEZIER:
      arrastar_ponto_bezier(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse,
                            obter_rigidez_movimento());
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
      break;
    case MODO_CURVA_CATMULLROM:
      arrastar_ponto_catmullrom(&g_clicks, selecao_curva.seg_curva, selecao_curva.t, mouse,
                                obter_rigidez_movimento());
      calcular_centro_medio(&centro, &g_clicks);
      precisa_refazer_curva = 1;
      glutPostRedisplay();
      break;
    default:
      printf("\n\tNão era para chegar aqui");
      exit(-1);
      break;
    }
  } else if(selecao_ponto.selecionado){
    g_clicks.data[selecao_ponto.indice].point[0] = x;
    g_clicks.data[selecao_ponto.indice].point[1] = y;
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay(); // Redesenhar se necessário
  } else if(selecao_poligono == 1 && estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
    translacao_com_mouse(&g_clicks, centro, x, y);
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if(selecao_poligono == 2 && estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
    translacao_com_mouse(&g_clicks, g_clicks.data[selecao_ponto.indice], x, y);
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if(selecao_poligono == 3 && estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
    rotacionar_com_mouse(&g_clicks, &centro, mouse, selecao_ponto.indice);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if(selecao_poligono == 4 && estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
    escala_com_mouse(&g_clicks, &centro, mouse, selecao_ponto.indice);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  } else if(selecao_poligono == 5 && estado_atual.criacao_ou_selecao == MODO_SELECIONAR_POLIGONO){
    if(!restaurar_controles_shear()){
      return;
    }
    ponto controle = controles_shear_iniciais.data[selecao_ponto.indice];
    float dx = mouse.point[0] - controle.point[0];
    float dy = mouse.point[1] - controle.point[1];
    float vx = controle.point[0] - centro_shear_inicial.point[0];
    float vy = controle.point[1] - centro_shear_inicial.point[1];
    if(eixo_shear == 0){
      if(fabsf(dx) >= fabsf(dy) && fabsf(vy) > 1e-6f){
        eixo_shear = 1;
      } else if(fabsf(vx) > 1e-6f){
        eixo_shear = 2;
      }
    }
    if(eixo_shear == 1){
      cisalhar_x_com_mouse(&g_clicks, &centro_shear_inicial, mouse, selecao_ponto.indice);
    } else if(eixo_shear == 2){
      cisalhar_y_com_mouse(&g_clicks, &centro_shear_inicial, mouse, selecao_ponto.indice);
    }
    calcular_centro_medio(&centro, &g_clicks);
    precisa_refazer_curva = 1;
    glutPostRedisplay();
  }
}
