#ifndef MATRIZ_H
#define MATRIZ_H

typedef struct { float dados[3][3]; } Matriz3;

void MultiplicaPonto(const float vetor[3], Matriz3 matriz, float resultado[3]);
Matriz3 MultiplicaMatriz(Matriz3 matriz1, Matriz3 matriz2);
Matriz3 CriarMatrizTranslacao(float Tx,float Ty);
Matriz3 CriarMatrizEscala(float Sx,float Sy);
Matriz3 CriarMatrizRotacao(float angulo);
Matriz3 CriarMatrizCisalhamentoX(float shx);
Matriz3 CriarMatrizCisalhamentoY(float shy);
Matriz3 CriarMatrizEspelhoX(void);
Matriz3 CriarMatrizEspelhoY(void);
Matriz3 CriarMatrizEspelhoOrigem(void);

#endif
