#include "../includes/obb.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define OBB_AMOSTRAS 5
#define OBB_EPSILON 1e-8f
#define OBB_EPSILON_PARAMETRICO 1e-5f

static float produto(ponto a, ponto b){
  return a.point[0] * b.point[0] + a.point[1] * b.point[1];
}

static ponto subtrair(ponto a, ponto b){
  ponto resultado = {{a.point[0] - b.point[0], a.point[1] - b.point[1], 1.0f}};
  return resultado;
}

static ponto somar_escalado(ponto origem, ponto direcao, float escala){
  ponto resultado = {{origem.point[0] + direcao.point[0] * escala,
                      origem.point[1] + direcao.point[1] * escala, 1.0f}};
  return resultado;
}

static void eixos_covariancia(float xx, float xy, float yy, ponto eixos[2]){
  if(xx + yy <= OBB_EPSILON){
    eixos[0] = (ponto){{1.0f, 0.0f, 1.0f}};
    eixos[1] = (ponto){{0.0f, 1.0f, 1.0f}};
    return;
  }
  float angulo = 0.5f * atan2f(2.0f * xy, xx - yy);
  eixos[0] = (ponto){{cosf(angulo), sinf(angulo), 1.0f}};
  eixos[1] = (ponto){{-sinf(angulo), cosf(angulo), 1.0f}};
}

static void ajustar(OBB *caixa, const ponto *pontos, int quantidade){
  float minimo[2] = {FLT_MAX, FLT_MAX};
  float maximo[2] = {-FLT_MAX, -FLT_MAX};
  for(int i = 0; i < quantidade; i++){
    ponto relativo = subtrair(pontos[i], caixa->centro);
    for(int eixo = 0; eixo < 2; eixo++){
      float projecao = produto(relativo, caixa->eixo[eixo]);
      if(projecao < minimo[eixo]){ minimo[eixo] = projecao; }
      if(projecao > maximo[eixo]){ maximo[eixo] = projecao; }
    }
  }
  for(int eixo = 0; eixo < 2; eixo++){
    float meio = (minimo[eixo] + maximo[eixo]) * 0.5f;
    caixa->centro = somar_escalado(caixa->centro, caixa->eixo[eixo], meio);
    caixa->extents[eixo] = (maximo[eixo] - minimo[eixo]) * 0.5f;
  }
}

static void vertices_obb(const OBB *caixa, ponto vertices[4]){
  int sinais[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
  for(int i = 0; i < 4; i++){
    ponto vertice = somar_escalado(caixa->centro, caixa->eixo[0],
                                   (float)sinais[i][0] * caixa->extents[0]);
    vertices[i] = somar_escalado(vertice, caixa->eixo[1],
                                 (float)sinais[i][1] * caixa->extents[1]);
  }
}

static int contar_spans(const BSplineTopologia *spline){
  int quantidade = 0;
  float inicio, fim;
  while(bspline_topologia_obter_span(spline, quantidade, &inicio, &fim, NULL) == 0){
    quantidade++;
  }
  return quantidade;
}

/* Em um span de B-spline, os pesos locais são não negativos e somam um.
 * Logo, a curva inteira está no casco convexo destes grau + 1 controles. */
static int controles_do_span(const BSplineTopologia *spline, int segmento, ponto controles[4]){
  float inicio, fim;
  int indice_no;
  if(!spline || spline->grau > 3 ||
     bspline_topologia_obter_span(spline, segmento, &inicio, &fim, &indice_no) != 0){
    return -1;
  }
  for(unsigned j = 0; j <= spline->grau; j++){
    int indice = indice_no - (int)spline->grau + (int)j;
    if(spline->fechado){
      int quantidade = spline->controles.quantidade_atual;
      indice %= quantidade;
      if(indice < 0){ indice += quantidade; }
    }
    if(indice < 0 || indice >= spline->controles.quantidade_atual){ return -1; }
    controles[j] = spline->controles.data[indice];
  }
  return 0;
}

int obb_bspline_elementar(const BSplineTopologia *spline, int segmento, OBB *resultado,
                          float *comprimento){
  if(!spline || !resultado || segmento < 0){
    return -1;
  }
  float inicio, fim;
  if(bspline_topologia_obter_span(spline, segmento, &inicio, &fim, NULL) != 0){
    return -1;
  }
  ponto amostras[OBB_AMOSTRAS];
  ponto media = {{0.0f, 0.0f, 1.0f}};
  for(int i = 0; i < OBB_AMOSTRAS; i++){
    float t = 0.1f + 0.2f * (float)i;
    amostras[i] = bspline_topologia_avaliar(spline, inicio + (fim - inicio) * t);
    media.point[0] += amostras[i].point[0];
    media.point[1] += amostras[i].point[1];
  }
  media.point[0] /= (float)OBB_AMOSTRAS;
  media.point[1] /= (float)OBB_AMOSTRAS;
  float xx = 0.0f, xy = 0.0f, yy = 0.0f;
  for(int i = 0; i < OBB_AMOSTRAS; i++){
    ponto d = subtrair(amostras[i], media);
    xx += d.point[0] * d.point[0];
    xy += d.point[0] * d.point[1];
    yy += d.point[1] * d.point[1];
  }
  OBB caixa = {0};
  caixa.centro = media;
  caixa.segmento_indice = segmento;
  eixos_covariancia(xx / (float)(OBB_AMOSTRAS - 1), xy / (float)(OBB_AMOSTRAS - 1),
                     yy / (float)(OBB_AMOSTRAS - 1), caixa.eixo);
  ajustar(&caixa, amostras, OBB_AMOSTRAS);
  ponto controles[4];
  if(controles_do_span(spline, segmento, controles) != 0){ return -1; }
  /* Mantém a orientação de Rivera, mas amplia o ajuste ao casco convexo
   * local. Isso transforma a folha em uma caixa conservadora da B-spline. */
  ajustar(&caixa, controles, (int)spline->grau + 1);
  if(comprimento){
    *comprimento = 0.0f;
    ponto anterior = bspline_topologia_avaliar(spline, inicio);
    for(int i = 1; i <= OBB_AMOSTRAS + 1; i++){
      ponto atual = bspline_topologia_avaliar(spline,
                                              inicio + (fim - inicio) * (float)i /
                                                           (float)(OBB_AMOSTRAS + 1));
      float dx = atual.point[0] - anterior.point[0];
      float dy = atual.point[1] - anterior.point[1];
      *comprimento += sqrtf(dx * dx + dy * dy);
      anterior = atual;
    }
  }
  *resultado = caixa;
  return 0;
}

void obb_arvore_init(OBBArvore *arvore){
  if(arvore){ *arvore = (OBBArvore){.raiz = -1}; }
}

void obb_arvore_liberar(OBBArvore *arvore){
  if(!arvore){ return; }
  free(arvore->nos);
  free(arvore->folha_por_segmento);
  obb_arvore_init(arvore);
}

static int combinar_filhos(OBBNo *pai, const OBBNo *esquerda, const OBBNo *direita){
  ponto centros[2] = {esquerda->caixa.centro, direita->caixa.centro};
  float comprimento = esquerda->comprimento + direita->comprimento;
  if(comprimento <= OBB_EPSILON){ comprimento = 1.0f; }
  ponto media = {{(esquerda->comprimento * centros[0].point[0] +
                   direita->comprimento * centros[1].point[0]) / comprimento,
                  (esquerda->comprimento * centros[0].point[1] +
                   direita->comprimento * centros[1].point[1]) / comprimento, 1.0f}};
  float xx = 0.0f, xy = 0.0f, yy = 0.0f;
  for(int i = 0; i < 2; i++){
    float peso = i == 0 ? esquerda->comprimento : direita->comprimento;
    ponto d = subtrair(centros[i], media);
    xx += peso * d.point[0] * d.point[0] / comprimento;
    xy += peso * d.point[0] * d.point[1] / comprimento;
    yy += peso * d.point[1] * d.point[1] / comprimento;
  }
  pai->caixa = (OBB){0};
  pai->caixa.centro = media;
  pai->caixa.segmento_indice = -1;
  eixos_covariancia(xx, xy, yy, pai->caixa.eixo);
  ponto pontos[8];
  vertices_obb(&esquerda->caixa, pontos);
  vertices_obb(&direita->caixa, &pontos[4]);
  ajustar(&pai->caixa, pontos, 8);
  pai->comprimento = esquerda->comprimento + direita->comprimento;
  return 0;
}

int obb_arvore_construir_bspline(OBBArvore *arvore, const BSplineTopologia *spline){
  if(!arvore || !spline){ return -1; }
  int folhas = contar_spans(spline);
  if(folhas <= 0){ return -1; }
  OBBArvore nova;
  obb_arvore_init(&nova);
  nova.capacidade_nos = 2 * folhas - 1;
  nova.nos = calloc((size_t)nova.capacidade_nos, sizeof(*nova.nos));
  nova.folha_por_segmento = malloc((size_t)folhas * sizeof(*nova.folha_por_segmento));
  if(!nova.nos || !nova.folha_por_segmento){ obb_arvore_liberar(&nova); return -1; }
  nova.quantidade_folhas = folhas;
  for(int i = 0; i < folhas; i++){
    OBBNo *no = &nova.nos[nova.quantidade_nos];
    if(obb_bspline_elementar(spline, i, &no->caixa, &no->comprimento) != 0){
      obb_arvore_liberar(&nova);
      return -1;
    }
    no->pai = no->esquerda = no->direita = -1;
    no->segmento_indice = i;
    nova.folha_por_segmento[i] = nova.quantidade_nos++;
  }
  int *nivel = malloc((size_t)folhas * sizeof(*nivel));
  int *proximo = malloc((size_t)folhas * sizeof(*proximo));
  if(!nivel || !proximo){ free(nivel); free(proximo); obb_arvore_liberar(&nova); return -1; }
  for(int i = 0; i < folhas; i++){ nivel[i] = i; }
  int quantidade = folhas;
  while(quantidade > 1){
    int nova_quantidade = 0;
    for(int i = 0; i < quantidade; i += 2){
      if(i + 1 == quantidade){ proximo[nova_quantidade++] = nivel[i]; continue; }
      int indice = nova.quantidade_nos++;
      OBBNo *pai = &nova.nos[indice];
      pai->pai = -1;
      pai->esquerda = nivel[i];
      pai->direita = nivel[i + 1];
      pai->segmento_indice = -1;
      nova.nos[nivel[i]].pai = indice;
      nova.nos[nivel[i + 1]].pai = indice;
      combinar_filhos(pai, &nova.nos[nivel[i]], &nova.nos[nivel[i + 1]]);
      proximo[nova_quantidade++] = indice;
    }
    int *troca = nivel; nivel = proximo; proximo = troca;
    quantidade = nova_quantidade;
  }
  nova.raiz = nivel[0];
  free(nivel);
  free(proximo);
  obb_arvore_liberar(arvore);
  *arvore = nova;
  return 0;
}

int obb_arvore_refit_bspline(OBBArvore *arvore, const BSplineTopologia *spline,
                              const int *segmentos, int quantidade){
  if(!arvore || !spline || !segmentos || quantidade <= 0 ||
     arvore->quantidade_folhas != contar_spans(spline)){
    return -1;
  }
  unsigned revisao = ++arvore->revisao;
  if(revisao == 0){ revisao = ++arvore->revisao; }
  for(int i = 0; i < quantidade; i++){
    int segmento = segmentos[i];
    if(segmento < 0 || segmento >= arvore->quantidade_folhas){ continue; }
    OBBNo *folha = &arvore->nos[arvore->folha_por_segmento[segmento]];
    if(folha->revisao == revisao){ continue; }
    if(obb_bspline_elementar(spline, segmento, &folha->caixa, &folha->comprimento) != 0){
      return -1;
    }
    folha->revisao = revisao;
  }
  for(int i = 0; i < quantidade; i++){
    int segmento = segmentos[i];
    if(segmento < 0 || segmento >= arvore->quantidade_folhas){ continue; }
    for(int no = arvore->nos[arvore->folha_por_segmento[segmento]].pai; no >= 0;
        no = arvore->nos[no].pai){
      if(arvore->nos[no].revisao == revisao){ break; }
      OBBNo *pai = &arvore->nos[no];
      combinar_filhos(pai, &arvore->nos[pai->esquerda], &arvore->nos[pai->direita]);
      pai->revisao = revisao;
    }
  }
  return 0;
}

int obb_intersecta_sat(const OBB *a, const OBB *b){
  if(!a || !b){ return 0; }
  ponto centro = subtrair(b->centro, a->centro);
  for(int origem = 0; origem < 2; origem++){
    const ponto eixo = a->eixo[origem];
    float distancia = fabsf(produto(centro, eixo));
    float raio_a = a->extents[0] * fabsf(produto(a->eixo[0], eixo)) +
                    a->extents[1] * fabsf(produto(a->eixo[1], eixo));
    float raio_b = b->extents[0] * fabsf(produto(b->eixo[0], eixo)) +
                    b->extents[1] * fabsf(produto(b->eixo[1], eixo));
    if(distancia > raio_a + raio_b + OBB_EPSILON){ return 0; }
  }
  for(int origem = 0; origem < 2; origem++){
    const ponto eixo = b->eixo[origem];
    float distancia = fabsf(produto(centro, eixo));
    float raio_a = a->extents[0] * fabsf(produto(a->eixo[0], eixo)) +
                    a->extents[1] * fabsf(produto(a->eixo[1], eixo));
    float raio_b = b->extents[0] * fabsf(produto(b->eixo[0], eixo)) +
                    b->extents[1] * fabsf(produto(b->eixo[1], eixo));
    if(distancia > raio_a + raio_b + OBB_EPSILON){ return 0; }
  }
  return 1;
}

typedef struct {
  const OBBArvore *arvore;
  OBBCandidato *candidatos;
  int capacidade;
  int quantidade;
} ColetaCandidatos;

static int folha(const OBBNo *no){
  return no->esquerda < 0 && no->direita < 0;
}

static void registrar_candidato(ColetaCandidatos *coleta, int a, int b){
  if(a == b){ return; }
  if(a > b){ int troca = a; a = b; b = troca; }
  if(coleta->candidatos){
    for(int i = 0; i < coleta->quantidade && i < coleta->capacidade; i++){
      if(coleta->candidatos[i].segmento_a == a && coleta->candidatos[i].segmento_b == b){
        return;
      }
    }
  }
  if(coleta->quantidade < coleta->capacidade && coleta->candidatos){
    coleta->candidatos[coleta->quantidade] = (OBBCandidato){.segmento_a = a, .segmento_b = b};
  }
  coleta->quantidade++;
}

static void percorrer_par(ColetaCandidatos *coleta, int indice_a, int indice_b){
  const OBBNo *a = &coleta->arvore->nos[indice_a];
  const OBBNo *b = &coleta->arvore->nos[indice_b];
  if(!obb_intersecta_sat(&a->caixa, &b->caixa)){ return; }
  if(indice_a == indice_b){
    if(folha(a)){ return; }
    percorrer_par(coleta, a->esquerda, a->esquerda);
    percorrer_par(coleta, a->esquerda, a->direita);
    percorrer_par(coleta, a->direita, a->direita);
    return;
  }
  if(folha(a) && folha(b)){
    registrar_candidato(coleta, a->segmento_indice, b->segmento_indice);
    return;
  }
  if(folha(a)){
    percorrer_par(coleta, indice_a, b->esquerda);
    percorrer_par(coleta, indice_a, b->direita);
    return;
  }
  if(folha(b)){
    percorrer_par(coleta, a->esquerda, indice_b);
    percorrer_par(coleta, a->direita, indice_b);
    return;
  }
  percorrer_par(coleta, a->esquerda, b->esquerda);
  percorrer_par(coleta, a->esquerda, b->direita);
  percorrer_par(coleta, a->direita, b->esquerda);
  percorrer_par(coleta, a->direita, b->direita);
}

static void registrar_candidato_unico(ColetaCandidatos *coleta, int a, int b){
  if(a == b){ return; }
  if(a > b){ int troca = a; a = b; b = troca; }
  for(int i = 0; i < coleta->quantidade; i++){
    if(coleta->candidatos[i].segmento_a == a && coleta->candidatos[i].segmento_b == b){
      return;
    }
  }
  coleta->candidatos[coleta->quantidade++] = (OBBCandidato){.segmento_a = a, .segmento_b = b};
}

/* Consulta uma folha contra a árvore. Cada span local inicia na raiz; pares
 * entre dois spans locais podem surgir duas vezes, por isso são deduplicados. */
static void percorrer_folha_contra_arvore(ColetaCandidatos *coleta, int indice_folha,
                                          int indice_no){
  const OBBNo *folha_atual = &coleta->arvore->nos[indice_folha];
  const OBBNo *no = &coleta->arvore->nos[indice_no];
  if(!obb_intersecta_sat(&folha_atual->caixa, &no->caixa)){ return; }
  if(folha(no)){
    registrar_candidato_unico(coleta, folha_atual->segmento_indice, no->segmento_indice);
    return;
  }
  percorrer_folha_contra_arvore(coleta, indice_folha, no->esquerda);
  percorrer_folha_contra_arvore(coleta, indice_folha, no->direita);
}

int obb_arvore_candidatos_autointerferencia(const OBBArvore *arvore,
                                            OBBCandidato *candidatos, int capacidade){
  if(!arvore || !arvore->nos || arvore->raiz < 0 || capacidade < 0){
    return -1;
  }
  ColetaCandidatos coleta = {arvore, candidatos, capacidade, 0};
  percorrer_par(&coleta, arvore->raiz, arvore->raiz);
  return coleta.quantidade;
}

typedef struct {
  ponto ponto;
  float u;
} AmostraCurva;

typedef struct {
  AmostraCurva *dados;
  int quantidade;
  int capacidade;
} AmostrasCurva;

static void amostras_liberar(AmostrasCurva *amostras){
  free(amostras->dados);
  *amostras = (AmostrasCurva){0};
}

static int amostras_adicionar(AmostrasCurva *amostras, ponto ponto_atual, float u){
  if(amostras->quantidade == amostras->capacidade){
    int capacidade = amostras->capacidade ? amostras->capacidade * 2 : 16;
    AmostraCurva *novos = realloc(amostras->dados, (size_t)capacidade * sizeof(*novos));
    if(!novos){ return -1; }
    amostras->dados = novos;
    amostras->capacidade = capacidade;
  }
  amostras->dados[amostras->quantidade++] = (AmostraCurva){ponto_atual, u};
  return 0;
}

static int amostrar_intervalo(const BSplineTopologia *spline, float u0, ponto p0, float u1,
                              ponto p1, float tolerancia, int profundidade,
                              AmostrasCurva *amostras){
  float meio_u = (u0 + u1) * 0.5f;
  ponto meio = bspline_topologia_avaliar(spline, meio_u);
  if(profundidade >= 14 || calcula_dist_ponto_segmento(meio, p0, p1) <= tolerancia * 0.25f){
    return amostras_adicionar(amostras, p1, u1);
  }
  if(amostrar_intervalo(spline, u0, p0, meio_u, meio, tolerancia, profundidade + 1,
                        amostras) != 0){
    return -1;
  }
  return amostrar_intervalo(spline, meio_u, meio, u1, p1, tolerancia, profundidade + 1,
                            amostras);
}

static int amostrar_span(const BSplineTopologia *spline, int segmento, float tolerancia,
                         AmostrasCurva *amostras){
  float inicio, fim;
  if(bspline_topologia_obter_span(spline, segmento, &inicio, &fim, NULL) != 0){ return -1; }
  ponto primeiro = bspline_topologia_avaliar(spline, inicio);
  ponto ultimo = bspline_topologia_avaliar(spline, fim);
  if(amostras_adicionar(amostras, primeiro, inicio) != 0){ return -1; }
  return amostrar_intervalo(spline, inicio, primeiro, fim, ultimo, tolerancia, 0, amostras);
}

static float cruzar(ponto a, ponto b){
  return a.point[0] * b.point[1] - a.point[1] * b.point[0];
}

static float distancia2(ponto a, ponto b){
  float dx = a.point[0] - b.point[0];
  float dy = a.point[1] - b.point[1];
  return dx * dx + dy * dy;
}

static float projetar_parametro(ponto ponto_teste, ponto inicio, ponto fim){
  ponto direcao = subtrair(fim, inicio);
  float comprimento2 = produto(direcao, direcao);
  if(comprimento2 <= OBB_EPSILON){ return 0.0f; }
  return fmaxf(0.0f, fminf(1.0f, produto(subtrair(ponto_teste, inicio), direcao) / comprimento2));
}

static int intersectar_segmentos(ponto a, ponto b, ponto c, ponto d, float tolerancia,
                                 float *ta, float *tb, ponto *interseccao, int *tipo){
  ponto r = subtrair(b, a);
  ponto s = subtrair(d, c);
  ponto c_menos_a = subtrair(c, a);
  float denominador = cruzar(r, s);
  if(fabsf(denominador) <= OBB_EPSILON){
    float melhor_distancia2 = FLT_MAX;
    float melhor_a = 0.0f, melhor_b = 0.0f;
    float candidatos[4][2] = {{0.0f, projetar_parametro(a, c, d)},
                               {1.0f, projetar_parametro(b, c, d)},
                               {projetar_parametro(c, a, b), 0.0f},
                               {projetar_parametro(d, a, b), 1.0f}};
    for(int i = 0; i < 4; i++){
      ponto pa = somar_escalado(a, r, candidatos[i][0]);
      ponto pb = somar_escalado(c, s, candidatos[i][1]);
      float distancia_atual2 = distancia2(pa, pb);
      if(distancia_atual2 < melhor_distancia2){
        melhor_distancia2 = distancia_atual2;
        melhor_a = candidatos[i][0];
        melhor_b = candidatos[i][1];
      }
    }
    if(melhor_distancia2 > tolerancia * tolerancia){ return 0; }
    *ta = melhor_a;
    *tb = melhor_b;
    ponto pa = somar_escalado(a, r, melhor_a);
    ponto pb = somar_escalado(c, s, melhor_b);
    *interseccao = (ponto){{(pa.point[0] + pb.point[0]) * 0.5f,
                            (pa.point[1] + pb.point[1]) * 0.5f, 1.0f}};
    *tipo = fabsf(cruzar(c_menos_a, r)) <= tolerancia * fmaxf(1.0f, hypotf(r.point[0], r.point[1])) ?
                CONTATO_SOBREPOSTO : CONTATO_TANGENCIAL;
    return 1;
  }
  float parametro_a = cruzar(c_menos_a, s) / denominador;
  float parametro_b = cruzar(c_menos_a, r) / denominador;
  float margem_a = tolerancia / fmaxf(1.0f, hypotf(r.point[0], r.point[1]));
  float margem_b = tolerancia / fmaxf(1.0f, hypotf(s.point[0], s.point[1]));
  if(parametro_a < -margem_a || parametro_a > 1.0f + margem_a ||
     parametro_b < -margem_b || parametro_b > 1.0f + margem_b){
    return 0;
  }
  parametro_a = fmaxf(0.0f, fminf(1.0f, parametro_a));
  parametro_b = fmaxf(0.0f, fminf(1.0f, parametro_b));
  *ta = parametro_a;
  *tb = parametro_b;
  *interseccao = somar_escalado(a, r, parametro_a);
  *tipo = CONTATO_TRANSVERSAL;
  return 1;
}

static int contato_e_juncao(const BSplineTopologia *spline, int a, int b, float u_a, float u_b){
  int spans = contar_spans(spline);
  int adjacentes = abs(a - b) == 1 || (spline->fechado && abs(a - b) == spans - 1);
  if(!adjacentes){ return 0; }
  int anterior = a;
  int posterior = b;
  if(spline->fechado && a == 0 && b == spans - 1){
    anterior = b;
    posterior = a;
  }
  float inicio_anterior, fim_anterior, inicio_posterior, fim_posterior;
  if(bspline_topologia_obter_span(spline, anterior, &inicio_anterior, &fim_anterior, NULL) != 0 ||
     bspline_topologia_obter_span(spline, posterior, &inicio_posterior, &fim_posterior, NULL) != 0){
    return 0;
  }
  float escala_a = fmaxf(1.0f, fabsf(fim_anterior - inicio_anterior));
  float escala_b = fmaxf(1.0f, fabsf(fim_posterior - inicio_posterior));
  float parametro_anterior = anterior == a ? u_a : u_b;
  float parametro_posterior = posterior == a ? u_a : u_b;
  return fabsf(parametro_anterior - fim_anterior) <= OBB_EPSILON_PARAMETRICO * escala_a &&
         fabsf(parametro_posterior - inicio_posterior) <= OBB_EPSILON_PARAMETRICO * escala_b;
}

static int confirmar_par(const BSplineTopologia *spline, int segmento_a, int segmento_b,
                         float tolerancia, InterseccaoBSpline *resultado){
  AmostrasCurva a = {0}, b = {0};
  int encontrado = 0;
  if(amostrar_span(spline, segmento_a, tolerancia, &a) != 0 ||
     amostrar_span(spline, segmento_b, tolerancia, &b) != 0){
    amostras_liberar(&a); amostras_liberar(&b); return -1;
  }
  for(int i = 1; i < a.quantidade && !encontrado; i++){
    for(int j = 1; j < b.quantidade; j++){
      float ta, tb;
      ponto contato;
      int tipo;
      if(!intersectar_segmentos(a.dados[i - 1].ponto, a.dados[i].ponto,
                                b.dados[j - 1].ponto, b.dados[j].ponto, tolerancia,
                                &ta, &tb, &contato, &tipo)){
        continue;
      }
      float u_a = a.dados[i - 1].u + (a.dados[i].u - a.dados[i - 1].u) * ta;
      float u_b = b.dados[j - 1].u + (b.dados[j].u - b.dados[j - 1].u) * tb;
      if(contato_e_juncao(spline, segmento_a, segmento_b, u_a, u_b)){
        continue;
      }
      *resultado = (InterseccaoBSpline){.tipo = tipo,
                                        .segmento_a = segmento_a,
                                        .segmento_b = segmento_b,
                                        .u_a = u_a,
                                        .u_b = u_b,
                                        .ponto = contato};
      encontrado = 1;
      break;
    }
  }
  amostras_liberar(&a);
  amostras_liberar(&b);
  return encontrado;
}

int obb_bspline_autointerseccoes(const OBBArvore *arvore, const BSplineTopologia *spline,
                                 float tolerancia, InterseccaoBSpline *resultado, int capacidade){
  if(!arvore || !spline || tolerancia <= 0.0f || capacidade < 0){ return -1; }
  int candidatos_total = obb_arvore_candidatos_autointerferencia(arvore, NULL, 0);
  if(candidatos_total <= 0){ return candidatos_total; }
  OBBCandidato *candidatos = malloc((size_t)candidatos_total * sizeof(*candidatos));
  if(!candidatos){ return -1; }
  candidatos_total = obb_arvore_candidatos_autointerferencia(arvore, candidatos, candidatos_total);
  if(candidatos_total < 0){ free(candidatos); return -1; }
  int quantidade = 0;
  for(int i = 0; i < candidatos_total; i++){
    InterseccaoBSpline contato;
    int confirmado = confirmar_par(spline, candidatos[i].segmento_a, candidatos[i].segmento_b,
                                   tolerancia, &contato);
    if(confirmado < 0){ free(candidatos); return -1; }
    if(confirmado){
      if(quantidade < capacidade && resultado){ resultado[quantidade] = contato; }
      quantidade++;
    }
  }
  free(candidatos);
  return quantidade;
}

int obb_bspline_autointerseccoes_locais(const OBBArvore *arvore,
                                        const BSplineTopologia *spline,
                                        const int *segmentos, int quantidade_segmentos,
                                        float tolerancia, InterseccaoBSpline *resultado,
                                        int capacidade){
  if(!arvore || !spline || !segmentos || quantidade_segmentos < 0 || tolerancia <= 0.0f ||
     capacidade < 0 || !arvore->nos || arvore->raiz < 0){
    return -1;
  }
  if(quantidade_segmentos == 0 || arvore->quantidade_folhas <= 1){ return 0; }
  if(quantidade_segmentos > arvore->quantidade_folhas){ return -1; }

  int *unicos = calloc((size_t)arvore->quantidade_folhas, sizeof(*unicos));
  if(!unicos){ return -1; }
  int quantidade_unicos = 0;
  for(int i = 0; i < quantidade_segmentos; i++){
    int segmento = segmentos[i];
    if(segmento < 0 || segmento >= arvore->quantidade_folhas){ free(unicos); return -1; }
    if(!unicos[segmento]){
      unicos[segmento] = 1;
      quantidade_unicos++;
    }
  }
  size_t maximo = (size_t)quantidade_unicos * (size_t)(arvore->quantidade_folhas - 1);
  if(maximo == 0 || maximo > SIZE_MAX / sizeof(OBBCandidato) || maximo > INT_MAX){
    free(unicos);
    return -1;
  }
  OBBCandidato *candidatos = malloc(maximo * sizeof(*candidatos));
  if(!candidatos){ free(unicos); return -1; }
  ColetaCandidatos coleta = {arvore, candidatos, (int)maximo, 0};
  for(int segmento = 0; segmento < arvore->quantidade_folhas; segmento++){
    if(unicos[segmento]){
      percorrer_folha_contra_arvore(&coleta, arvore->folha_por_segmento[segmento], arvore->raiz);
    }
  }
  free(unicos);

  int quantidade = 0;
  for(int i = 0; i < coleta.quantidade; i++){
    InterseccaoBSpline contato;
    int confirmado = confirmar_par(spline, candidatos[i].segmento_a, candidatos[i].segmento_b,
                                   tolerancia, &contato);
    if(confirmado < 0){ free(candidatos); return -1; }
    if(confirmado){
      if(quantidade < capacidade && resultado){ resultado[quantidade] = contato; }
      quantidade++;
    }
  }
  free(candidatos);
  return quantidade;
}
