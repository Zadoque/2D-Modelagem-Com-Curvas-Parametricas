#include "obb.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static ponto p(float x, float y){ return (ponto){{x, y, 1.0f}}; }

static int dentro(const OBB *caixa, ponto ponto_teste){
  float dx = ponto_teste.point[0] - caixa->centro.point[0];
  float dy = ponto_teste.point[1] - caixa->centro.point[1];
  for(int i = 0; i < 2; i++){
    float projecao = dx * caixa->eixo[i].point[0] + dy * caixa->eixo[i].point[1];
    if(fabsf(projecao) > caixa->extents[i] + 1e-4f){ return 0; }
  }
  return 1;
}

static void vertices(const OBB *caixa, ponto resultado[4]){
  int sinais[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
  for(int i = 0; i < 4; i++){
    resultado[i] = p(caixa->centro.point[0] + sinais[i][0] * caixa->extents[0] * caixa->eixo[0].point[0] +
                         sinais[i][1] * caixa->extents[1] * caixa->eixo[1].point[0],
                     caixa->centro.point[1] + sinais[i][0] * caixa->extents[0] * caixa->eixo[0].point[1] +
                         sinais[i][1] * caixa->extents[1] * caixa->eixo[1].point[1]);
  }
}

static OBB caixa(float x, float y, float ex, float ey){
  OBB resultado = {0};
  resultado.centro = p(x, y);
  resultado.eixo[0] = p(1.0f, 0.0f);
  resultado.eixo[1] = p(0.0f, 1.0f);
  resultado.extents[0] = ex;
  resultado.extents[1] = ey;
  return resultado;
}

static unsigned proximo_aleatorio(unsigned *estado){
  *estado = *estado * 1664525u + 1013904223u;
  return *estado;
}

static void verificar_lacada_entre_spans_adjacentes(void){
  unsigned estado = 0x2d0bb123u;
  int encontrada = 0;
  for(int tentativa = 0; tentativa < 160 && !encontrada; tentativa++){
    Pontos controles;
    BSplineTopologia spline = {0};
    OBBArvore arvore;
    InterseccaoBSpline contatos[32];
    pontos_init(&controles, 8);
    for(int i = 0; i < 8; i++){
      float x = (float)(proximo_aleatorio(&estado) % 401u) - 200.0f;
      float y = (float)(proximo_aleatorio(&estado) % 401u) - 200.0f;
      assert(pontos_push(&controles, p(x, y)) == 0);
    }
    assert(bspline_topologia_inicializar_uniforme_fechada(&spline, &controles, 3) == 0);
    obb_arvore_init(&arvore);
    assert(obb_arvore_construir_bspline(&arvore, &spline) == 0);
    int quantidade = obb_bspline_autointerseccoes(&arvore, &spline, 0.05f, contatos, 32);
    assert(quantidade >= 0);
    for(int i = 0; i < quantidade && i < 32; i++){
      int diferenca = abs(contatos[i].segmento_a - contatos[i].segmento_b);
      if(diferenca == 1 || diferenca == 7){
        encontrada = 1;
        break;
      }
    }
    obb_arvore_liberar(&arvore);
    bspline_topologia_liberar(&spline);
    pontos_free(&controles);
  }
  assert(encontrada);
}

int main(void){
  OBB a = caixa(0.0f, 0.0f, 2.0f, 1.0f);
  OBB b = caixa(3.0f, 0.0f, 1.0f, 1.0f);
  OBB c = caixa(3.1f, 0.0f, 1.0f, 1.0f);
  assert(obb_intersecta_sat(&a, &b));
  assert(!obb_intersecta_sat(&a, &c));
  Pontos controles;
  pontos_init(&controles, 8);
  for(int i = 0; i < 8; i++){
    assert(pontos_push(&controles, p((float)i * 40.0f, (float)(i % 3) * 30.0f)) == 0);
  }
  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  OBB folha;
  float comprimento;
  assert(obb_bspline_elementar(&spline, 1, &folha, &comprimento) == 0);
  assert(comprimento > 0.0f);
  assert(fabsf(folha.eixo[0].point[0] * folha.eixo[1].point[0] +
               folha.eixo[0].point[1] * folha.eixo[1].point[1]) < 1e-5f);
  float inicio, fim;
  assert(bspline_topologia_obter_span(&spline, 1, &inicio, &fim, NULL) == 0);
  for(int i = 0; i < 5; i++){
    assert(dentro(&folha, bspline_topologia_avaliar(&spline, inicio + (fim - inicio) *
                                                     (0.1f + 0.2f * i))));
  }
  OBBArvore arvore;
  obb_arvore_init(&arvore);
  assert(obb_arvore_construir_bspline(&arvore, &spline) == 0);
  assert(arvore.quantidade_folhas == 5 && arvore.raiz >= 0);
  OBBCandidato candidatos[32];
  int quantidade_candidatos =
      obb_arvore_candidatos_autointerferencia(&arvore, candidatos, 32);
  assert(quantidade_candidatos >= 0);
  for(int i = 0; i < quantidade_candidatos && i < 32; i++){
    assert(candidatos[i].segmento_a < candidatos[i].segmento_b);
    for(int j = i + 1; j < quantidade_candidatos && j < 32; j++){
      assert(candidatos[i].segmento_a != candidatos[j].segmento_a ||
             candidatos[i].segmento_b != candidatos[j].segmento_b);
    }
  }
  for(int segmento = 0; segmento < arvore.quantidade_folhas; segmento++){
    ponto cantos[4];
    vertices(&arvore.nos[arvore.folha_por_segmento[segmento]].caixa, cantos);
    for(int i = 0; i < 4; i++){
      assert(dentro(&arvore.nos[arvore.raiz].caixa, cantos[i]));
    }
    float inicio_span, fim_span;
    assert(bspline_topologia_obter_span(&spline, segmento, &inicio_span, &fim_span, NULL) == 0);
    /* A folha precisa conter a curva inteira, não apenas as cinco amostras
     * usadas para orientar a caixa. */
    for(int amostra = 0; amostra <= 100; amostra++){
      float t = (float)amostra / 100.0f;
      assert(dentro(&arvore.nos[arvore.folha_por_segmento[segmento]].caixa,
                    bspline_topologia_avaliar(&spline,
                                               inicio_span + (fim_span - inicio_span) * t)));
    }
  }
  ponto antes = arvore.nos[arvore.folha_por_segmento[1]].caixa.centro;
  assert(bspline_topologia_arrastar(&spline, 1.5f, p(80.0f, 120.0f), 0.0f) == 0);
  int afetados[] = {0, 1, 2, 3, 4};
  assert(obb_arvore_refit_bspline(&arvore, &spline, afetados, 5) == 0);
  ponto depois = arvore.nos[arvore.folha_por_segmento[1]].caixa.centro;
  assert(fabsf(antes.point[0] - depois.point[0]) > 1e-4f ||
         fabsf(antes.point[1] - depois.point[1]) > 1e-4f);
  obb_arvore_liberar(&arvore);
  bspline_topologia_liberar(&spline);
  pontos_free(&controles);

  Pontos degenerados;
  pontos_init(&degenerados, 4);
  for(int i = 0; i < 4; i++){
    assert(pontos_push(&degenerados, p(7.0f, -3.0f)) == 0);
  }
  assert(bspline_topologia_inicializar_uniforme_fechada(&spline, &degenerados, 3) == 0);
  assert(obb_bspline_elementar(&spline, 0, &folha, &comprimento) == 0);
  assert(fabsf(folha.eixo[0].point[0] - 1.0f) < 1e-6f);
  assert(fabsf(folha.eixo[1].point[1] - 1.0f) < 1e-6f);
  assert(folha.extents[0] <= 1e-6f && folha.extents[1] <= 1e-6f);
  assert(obb_arvore_construir_bspline(&arvore, &spline) == 0);
  int costura[] = {3, 0, 1};
  assert(obb_arvore_refit_bspline(&arvore, &spline, costura, 3) == 0);
  obb_arvore_liberar(&arvore);
  bspline_topologia_liberar(&spline);
  pontos_free(&degenerados);

  /* As caixas de spans vizinhos em uma reta se sobrepõem nos limites, mas a
   * única coincidência é a junção paramétrica compartilhada e não é uma
   * autointerseção. */
  Pontos reta;
  pontos_init(&reta, 6);
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&reta, p((float)i * 40.0f, 0.0f)) == 0);
  }
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &reta, 3) == 0);
  assert(obb_arvore_construir_bspline(&arvore, &spline) == 0);
  assert(obb_bspline_autointerseccoes(&arvore, &spline, 0.05f, NULL, 0) == 0);
  obb_arvore_liberar(&arvore);
  bspline_topologia_liberar(&spline);
  pontos_free(&reta);

  Pontos cruzada;
  pontos_init(&cruzada, 8);
  ponto controles_cruzados[] = {
      {{-80.0f, -80.0f, 1.0f}}, {{80.0f, 80.0f, 1.0f}}, {{-80.0f, 80.0f, 1.0f}},
      {{80.0f, -80.0f, 1.0f}}, {{-80.0f, -80.0f, 1.0f}}, {{80.0f, 80.0f, 1.0f}},
      {{-80.0f, 80.0f, 1.0f}}, {{80.0f, -80.0f, 1.0f}}};
  for(int i = 0; i < 8; i++){
    assert(pontos_push(&cruzada, controles_cruzados[i]) == 0);
  }
  assert(bspline_topologia_inicializar_uniforme_fechada(&spline, &cruzada, 3) == 0);
  assert(obb_arvore_construir_bspline(&arvore, &spline) == 0);
  InterseccaoBSpline contatos[16];
  int total_contatos = obb_bspline_autointerseccoes(&arvore, &spline, 0.05f, contatos, 16);
  assert(total_contatos > 0);
  int afetado = contatos[0].segmento_a;
  assert(obb_bspline_autointerseccoes_locais(&arvore, &spline, &afetado, 1,
                                              0.05f, contatos, 16) > 0);
  obb_arvore_liberar(&arvore);
  bspline_topologia_liberar(&spline);
  pontos_free(&cruzada);
  verificar_lacada_entre_spans_adjacentes();
  puts("test_obb: OK");
  return 0;
}
