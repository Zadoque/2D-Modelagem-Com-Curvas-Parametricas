#include "./../includes/opengl.h"
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "desenvolvimento"
#endif

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--version") == 0) {
    printf("curvas-parametricas-2d %s\n", PROJECT_VERSION);
    return 0;
  }
  if (argc > 1 && strcmp(argv[1], "--benchmark") == 0) {
    int execucoes = argc > 2 ? atoi(argv[2]) : 1000;
    performance_benchmark_executar(execucoes);
    return 0;
  }
  printf("=== Trabalho de Computação Gráfica ===\n");

  // Inicializar GLUT
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(800, 800);
  glutCreateWindow("Trabalho 1 de computação Gráfica");

  // Registrando callbacks
  glutReshapeFunc(AlteraTamanhoJanela);
  glutDisplayFunc(display);
  glutMouseFunc(onMouse);
  glutPassiveMotionFunc(onMouseMove);
  glutMotionFunc(onMotion);

  // Inicializar menu
  inicializar_menu();
  // Inicializar OpenGL
  initGL();

  printf("Clique na janela para adicionar pontos!\n");

  // Loop principal
  glutMainLoop();

  // Algumas implementações de GLUT permitem que o loop principal retorne.
  finalizar_aplicacao();
  return 0;
}
