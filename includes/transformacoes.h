#ifndef TRANSFORMACOES_H
#define TRANSFORMACOES_H

#include "pontos.h"
#include "menu.h" 
#include "matriz.h" 


void calcular_centro_medio(ponto *centro,Pontos* pontos);

void translacao_com_mouse(Pontos* pontos, ponto centro, int x_mouse, int y_mouse);
void transladar_para_origem(Pontos *pontos, ponto centro);
void transladar_da_origem(Pontos *pontos, ponto centro);

void rotacionar_com_mouse(Pontos *pontos, ponto* centro, ponto mouse, int indice);
void escala_com_mouse(Pontos* pontos, ponto* centro, ponto mouse, int indice);
void cisalhar_com_mouse(Pontos *pontos, ponto *centro, ponto mouse, int indice);
void cisalhar_x_com_mouse(Pontos *pontos, ponto *centro, ponto mouse, int indice);
void cisalhar_y_com_mouse(Pontos *pontos, ponto *centro, ponto mouse, int indice);
void aumentar_escala(Pontos* pontos, ponto centro);

void diminuir_escala(Pontos* pontos, ponto centro);


#endif
