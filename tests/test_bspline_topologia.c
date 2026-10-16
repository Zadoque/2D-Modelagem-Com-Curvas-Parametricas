#include "bspline_topologia.h"
#include "densidade.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static ponto p(float x, float y){
  ponto resultado = {{x, y, 1.0f}};
  return resultado;
}

static float distancia(ponto a, ponto b){
  float dx = a.point[0] - b.point[0];
  float dy = a.point[1] - b.point[1];
  return sqrtf(dx * dx + dy * dy);
}

static int coletar_candidatos_densidade(const BSplineTopologia *spline, int remover,
                                        float limiar, float *nos, int capacidade){
  int quantidade = 0;
  float inicio, fim;
  int spans = 0;
  while(bspline_topologia_obter_span(spline, spans, &inicio, &fim, NULL) == 0){
    spans++;
  }
  for(int central = 0; central < spans; central++){
    float locais[7];
    int locais_quantidade = remover ?
        bspline_topologia_candidatos_remocao(spline, central, limiar, locais, 7) :
        bspline_topologia_candidatos_insercao(spline, central, limiar, locais, 7);
    assert(locais_quantidade >= 0);
    for(int i = 0; i < locais_quantidade; i++){
      int repetido = 0;
      for(int j = 0; j < quantidade; j++){
        if(fabsf(nos[j] - locais[i]) <= 1e-6f){
          repetido = 1;
          break;
        }
      }
      if(!repetido){
        assert(quantidade < capacidade);
        nos[quantidade++] = locais[i];
      }
    }
  }
  return quantidade;
}

static void verificar_limiares_densidade(void){
  Pontos controles;
  pontos_init(&controles, 6);
  /* Cada par consecutivo mede exatamente 200 pela distância euclidiana,
   * e não por diferença em x ou y: sqrt(120² + 160²) = 200. */
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&controles, p(120.0f * i, 160.0f * i)) == 0);
  }
  assert(fabsf(calcula_distancia(controles.data[0], controles.data[1]) - 200.0f) < 1e-5f);
  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  float candidatos[7];
  for(int posicao = 0; posicao < DENSIDADE_POSICOES; posicao++){
    float limiar = densidade_limiar_por_posicao(posicao);
    int quantidade = bspline_topologia_candidatos_insercao(&spline, 1, limiar, candidatos, 7);
    if(limiar < 200.0f){
      assert(quantidade > 0);
    } else {
      assert(quantidade == 0);
    }
  }
  /* Igualdade não é excesso: a regra é distância estritamente maior. */
  assert(bspline_topologia_candidatos_insercao(&spline, 1, 200.0f, candidatos, 7) == 0);
  assert(bspline_topologia_candidatos_insercao(&spline, 1, 199.999f, candidatos, 7) > 0);
  bspline_topologia_liberar(&spline);
  pontos_free(&controles);
}

static void verificar_ciclos_densidade_sem_acumulo(void){
  Pontos controles;
  pontos_init(&controles, 6);
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&controles, p((float)i * 220.0f, (i % 2) ? 90.0f : -70.0f)) == 0);
  }
  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  ponto referencia[121];
  for(int i = 0; i <= 120; i++){
    referencia[i] = bspline_topologia_avaliar(&spline, 3.0f * i / 120.0f);
  }
  for(int ciclo = 0; ciclo < 10; ciclo++){
    float nos[7];
    int quantidade = bspline_topologia_candidatos_insercao(&spline, 1, 150.0f, nos, 7);
    assert(quantidade > 0);
    assert(inserir_nos_oslo(&spline, nos, quantidade) == 0);
    assert(remover_nos_lyche_morken(&spline, nos, quantidade, 1e-3f) == quantidade);
    for(int i = 0; i <= 120; i++){
      ponto atual = bspline_topologia_avaliar(&spline, 3.0f * i / 120.0f);
      assert(distancia(referencia[i], atual) <= 1e-3f);
    }
  }
  bspline_topologia_liberar(&spline);
  pontos_free(&controles);
}

static void verificar_alteracao_real_5_1_5(void){
  Pontos controles;
  pontos_init(&controles, 6);
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&controles, p((float)i * 220.0f,
                                     (i % 2) ? 90.0f : -70.0f)) == 0);
  }
  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  ponto referencia[121];
  for(int i = 0; i <= 120; i++){
    referencia[i] = bspline_topologia_avaliar(&spline, 3.0f * i / 120.0f);
  }

  float nos[64];
  int controles_iniciais = spline.controles.quantidade_atual;
  int quantidade = coletar_candidatos_densidade(&spline, 0,
                                                 densidade_limiar_por_posicao(4), nos, 64);
  assert(quantidade > 0);
  assert(inserir_nos_oslo(&spline, nos, quantidade) == 0);
  int controles_densos = spline.controles.quantidade_atual;
  assert(controles_densos > controles_iniciais);

  quantidade = coletar_candidatos_densidade(&spline, 1,
                                             densidade_limiar_por_posicao(0), nos, 64);
  assert(quantidade > 0);
  assert(remover_nos_lyche_morken(&spline, nos, quantidade, 1e-3f) > 0);
  assert(spline.controles.quantidade_atual < controles_densos);
  assert(spline.controles.quantidade_atual == controles_iniciais);
  for(int i = 0; i <= 120; i++){
    assert(distancia(referencia[i], bspline_topologia_avaliar(&spline, 3.0f * i / 120.0f)) <= 1e-3f);
  }

  quantidade = coletar_candidatos_densidade(&spline, 0,
                                             densidade_limiar_por_posicao(4), nos, 64);
  assert(quantidade > 0);
  assert(inserir_nos_oslo(&spline, nos, quantidade) == 0);
  assert(spline.controles.quantidade_atual > controles_iniciais);
  for(int i = 0; i <= 120; i++){
    assert(distancia(referencia[i], bspline_topologia_avaliar(&spline, 3.0f * i / 120.0f)) <= 1e-3f);
  }
  bspline_topologia_liberar(&spline);
  pontos_free(&controles);
}

static void verificar_copia_independente(void){
  Pontos controles;
  pontos_init(&controles, 6);
  for(int i = 0; i < 6; i++){
    assert(pontos_push(&controles, p((float)i * 30.0f, (float)(i % 2) * 20.0f)) == 0);
  }
  BSplineTopologia original = {0};
  BSplineTopologia copia = {0};
  assert(bspline_topologia_inicializar_uniforme_aberta(&original, &controles, 3) == 0);
  assert(bspline_topologia_copiar(&original, &copia) == 0);
  ponto antes = bspline_topologia_avaliar(&copia, 1.25f);
  assert(bspline_topologia_arrastar(&original, 1.25f,
                                    p(antes.point[0] + 15.0f, antes.point[1] - 9.0f),
                                    1.0f) == 0);
  ponto depois = bspline_topologia_avaliar(&copia, 1.25f);
  assert(distancia(antes, depois) < 1e-6f);
  bspline_topologia_liberar(&copia);
  bspline_topologia_liberar(&original);
  pontos_free(&controles);
}

int main(void){
  verificar_limiares_densidade();
  verificar_ciclos_densidade_sem_acumulo();
  verificar_alteracao_real_5_1_5();
  verificar_copia_independente();
  Pontos controles;
  pontos_init(&controles, 6);
  assert(pontos_push(&controles, p(0.0f, 0.0f)) == 0);
  assert(pontos_push(&controles, p(1.0f, 3.0f)) == 0);
  assert(pontos_push(&controles, p(3.0f, -2.0f)) == 0);
  assert(pontos_push(&controles, p(5.0f, 4.0f)) == 0);
  assert(pontos_push(&controles, p(7.0f, -1.0f)) == 0);
  assert(pontos_push(&controles, p(9.0f, 2.0f)) == 0);

  BSplineTopologia spline;
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  ponto inicio_cardinal = bspline_topologia_avaliar(&spline, 0.0f);
  assert(fabsf(inicio_cardinal.point[0] - 7.0f / 6.0f) < 1e-6f);
  assert(fabsf(inicio_cardinal.point[1] - 10.0f / 6.0f) < 1e-6f);
  /* Fluxo da #15: o candidato é local, a inserção preserva o ponto e o
   * parâmetro global é convertido novamente para (span, t local). */
  float candidatos[7];
  int quantidade_candidatos =
      bspline_topologia_candidatos_insercao(&spline, 1, 2.0f, candidatos, 7);
  assert(quantidade_candidatos > 0 && quantidade_candidatos <= 7);
  ponto selecionado_antes = bspline_topologia_avaliar(&spline, 1.35f);
  ReferenciaArrastoBSpline referencia;
  assert(bspline_topologia_capturar_referencia_arrasto(&spline, 1.35f, &referencia) == 0);
  int span_antes = referencia.segmento_indice;
  assert(inserir_nos_oslo(&spline, candidatos, quantidade_candidatos) == 0);
  assert(bspline_topologia_remapear_referencia_arrasto(&spline, &referencia, 1e-5f) == 0);
  assert(referencia.segmento_indice >= span_antes);
  assert(referencia.t_local >= 0.0f && referencia.t_local <= 1.0f);
  assert(distancia(referencia.ancora,
                   bspline_topologia_avaliar(&spline, referencia.u_global)) < 1e-5f);
  assert(distancia(selecionado_antes, bspline_topologia_avaliar(&spline, 1.35f)) < 1e-5f);
  assert(bspline_topologia_capturar_referencia_arrasto(&spline, referencia.u_global,
                                                        &referencia) == 0);
  assert(remover_nos_lyche_morken(&spline, candidatos, quantidade_candidatos, 1e-5f) > 0);
  assert(bspline_topologia_remapear_referencia_arrasto(&spline, &referencia, 1e-5f) == 0);
  assert(distancia(referencia.ancora,
                   bspline_topologia_avaliar(&spline, referencia.u_global)) < 1e-5f);
  assert(bspline_topologia_candidatos_remocao(&spline, referencia.segmento_indice, 10000.0f,
                                               candidatos, 7) > 0);
  bspline_topologia_liberar(&spline);
  assert(bspline_topologia_inicializar_uniforme_aberta(&spline, &controles, 3) == 0);
  ponto antes[101];
  for(int i = 0; i <= 100; i++){
    antes[i] = bspline_topologia_avaliar(&spline, 3.0f * (float)i / 100.0f);
  }

  float lote[] = {2.25f, 0.75f, 1.5f};
  assert(inserir_nos_oslo(&spline, lote, 3) == 0);
  assert(spline.controles.quantidade_atual == 9);
  assert(spline.quantidade_nos == 13);
  for(int i = 0; i <= 100; i++){
    ponto depois = bspline_topologia_avaliar(&spline, 3.0f * (float)i / 100.0f);
    assert(distancia(antes[i], depois) < 1e-5f);
  }

  assert(remover_nos_lyche_morken(&spline, lote, 3, 1e-5f) == 3);
  assert(spline.controles.quantidade_atual == 6);
  assert(spline.quantidade_nos == 10);
  for(int i = 0; i <= 100; i++){
    ponto depois = bspline_topologia_avaliar(&spline, 3.0f * (float)i / 100.0f);
    assert(distancia(antes[i], depois) < 2e-5f);
  }

  ponto antes_arrasto = bspline_topologia_avaliar(&spline, 1.35f);
  ponto alvo_arrasto = p(antes_arrasto.point[0] + 11.0f, antes_arrasto.point[1] - 7.0f);
  assert(bspline_topologia_arrastar(&spline, 1.35f, alvo_arrasto, 1.0f) == 0);
  assert(distancia(bspline_topologia_avaliar(&spline, 1.35f), alvo_arrasto) < 1e-4f);
  ResultadoPicking picking = bspline_topologia_picking(&spline, alvo_arrasto, 0.1f);
  assert(picking.segmento_indice >= 0);
  assert(fabsf(picking.t - 1.35f) < 0.05f);

  int quantidade_controles = spline.controles.quantidade_atual;
  int quantidade_nos = spline.quantidade_nos;
  float invalido = 0.0f;
  assert(inserir_nos_oslo(&spline, &invalido, 1) == -1);
  assert(spline.controles.quantidade_atual == quantidade_controles);
  assert(spline.quantidade_nos == quantidade_nos);

  bspline_topologia_liberar(&spline);

  BSplineTopologia fechada;
  assert(bspline_topologia_inicializar_uniforme_fechada(&fechada, &controles, 3) == 0);
  assert(fechada.quantidade_controles_base == controles.quantidade_atual);
  assert(fechada.controles.quantidade_atual == controles.quantidade_atual);
  ponto inicio_fechado = bspline_topologia_avaliar(&fechada, 0.0f);
  ponto fim_fechado = bspline_topologia_avaliar(&fechada, (float)controles.quantidade_atual);
  assert(distancia(inicio_fechado, fim_fechado) < 1e-5f);
  float inicio_span, fim_span;
  int spans = 0;
  while(bspline_topologia_obter_span(&fechada, spans, &inicio_span, &fim_span, NULL) == 0){
    spans++;
  }
  assert(spans == controles.quantidade_atual);
  ponto amostras_fechadas[49];
  for(int i = 0; i < 49; i++){
    amostras_fechadas[i] = bspline_topologia_avaliar(&fechada,
                                                     (float)controles.quantidade_atual * i / 48.0f);
  }
  float no_periodico = 2.5f;
  assert(inserir_nos_oslo(&fechada, &no_periodico, 1) == 0);
  assert(fechada.controles.quantidade_atual == controles.quantidade_atual + 1);
  for(int i = 0; i < 49; i++){
    assert(distancia(amostras_fechadas[i],
                     bspline_topologia_avaliar(&fechada,
                                                (float)controles.quantidade_atual * i / 48.0f)) < 1e-3f);
  }
  assert(remover_nos_lyche_morken(&fechada, &no_periodico, 1, 1e-3f) == 1);
  assert(fechada.controles.quantidade_atual == controles.quantidade_atual);
  ponto antes_arrasto_fechado =
      bspline_topologia_avaliar(&fechada, (float)controles.quantidade_atual - 0.1f);
  ponto alvo_fechado = p(antes_arrasto_fechado.point[0] + 4.0f,
                          antes_arrasto_fechado.point[1] - 3.0f);
  assert(bspline_topologia_arrastar(&fechada, (float)controles.quantidade_atual - 0.1f,
                                    alvo_fechado, 0.5f) == 0);
  assert(distancia(bspline_topologia_avaliar(&fechada, (float)controles.quantidade_atual - 0.1f),
                   alvo_fechado) < 1e-4f);
  assert(distancia(bspline_topologia_avaliar(&fechada, 0.0f),
                   bspline_topologia_avaliar(&fechada, (float)controles.quantidade_atual)) < 1e-6f);
  ReferenciaArrastoBSpline referencia_fechada;
  float u_costura = (float)controles.quantidade_atual - 0.15f;
  assert(bspline_topologia_capturar_referencia_arrasto(&fechada, u_costura,
                                                        &referencia_fechada) == 0);
  float no_remapeamento_fechado = 1.5f;
  assert(inserir_nos_oslo(&fechada, &no_remapeamento_fechado, 1) == 0);
  assert(bspline_topologia_remapear_referencia_arrasto(&fechada, &referencia_fechada,
                                                        1e-3f) == 0);
  assert(referencia_fechada.segmento_indice >= 0);
  assert(referencia_fechada.t_local >= 0.0f && referencia_fechada.t_local <= 1.0f);
  assert(distancia(referencia_fechada.ancora,
                   bspline_topologia_avaliar(&fechada, referencia_fechada.u_global)) < 1e-3f);
  bspline_topologia_liberar(&fechada);
  pontos_free(&controles);
  puts("test_bspline_topologia: OK");
  return 0;
}
