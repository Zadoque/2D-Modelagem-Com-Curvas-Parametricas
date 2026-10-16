#include "../includes/matriz.h"
#include <math.h>

static Matriz3 identidade(void){
  Matriz3 m = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
  return m;
}
void matriz3_multiplicar_ponto(const float v[3], Matriz3 m, float r[3]){
  for(int j = 0; j < 3; j++){
    r[j] = v[0] * m.dados[0][j] + v[1] * m.dados[1][j] + v[2] * m.dados[2][j];
  }
}
Matriz3 matriz3_multiplicar(Matriz3 a, Matriz3 b){
  Matriz3 r = {{{0}}};
  for(int i = 0; i < 3; i++){
    for(int j = 0; j < 3; j++){
      for(int k = 0; k < 3; k++){
        r.dados[i][j] += a.dados[i][k] * b.dados[k][j];
      }
    }
  }
  return r;
}
Matriz3 matriz3_translacao(float x, float y){
  Matriz3 m = identidade();
  m.dados[2][0] = x;
  m.dados[2][1] = y;
  return m;
}
Matriz3 matriz3_escala(float x, float y){
  Matriz3 m = {{{x, 0, 0}, {0, y, 0}, {0, 0, 1}}};
  return m;
}
Matriz3 matriz3_rotacao(float a){
  float c = cosf(a), s = sinf(a);
  Matriz3 m = {{{c, s, 0}, {-s, c, 0}, {0, 0, 1}}};
  return m;
}
Matriz3 matriz3_cisalhamento_x(float x){
  Matriz3 m = identidade();
  m.dados[1][0] = x;
  return m;
}
Matriz3 matriz3_cisalhamento_y(float y){
  Matriz3 m = identidade();
  m.dados[0][1] = y;
  return m;
}
Matriz3 matriz3_espelho_x(void){
  return matriz3_escala(1, -1);
}
Matriz3 matriz3_espelho_y(void){
  return matriz3_escala(-1, 1);
}
Matriz3 matriz3_espelho_origem(void){
  return matriz3_escala(-1, -1);
}
