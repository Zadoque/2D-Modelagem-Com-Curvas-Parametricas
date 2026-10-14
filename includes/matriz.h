#ifndef MATRIZ_H
#define MATRIZ_H

typedef struct { float dados[3][3]; } Matriz3;

void matriz3_multiplicar_ponto(const float vetor[3], Matriz3 matriz, float resultado[3]);
Matriz3 matriz3_multiplicar(Matriz3 matriz1, Matriz3 matriz2);
Matriz3 matriz3_translacao(float tx,float ty);
Matriz3 matriz3_escala(float sx,float sy);
Matriz3 matriz3_rotacao(float angulo);
Matriz3 matriz3_cisalhamento_x(float fator_x);
Matriz3 matriz3_cisalhamento_y(float fator_y);
Matriz3 matriz3_espelho_x(void);
Matriz3 matriz3_espelho_y(void);
Matriz3 matriz3_espelho_origem(void);

#endif
