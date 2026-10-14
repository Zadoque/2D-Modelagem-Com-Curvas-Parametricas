#include "./../includes/opengl.h"
#include <GL/glut.h>
#include <stdio.h>

int main(int argc, char **argv) {
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

  // Limpeza (nunca será executada devido ao glutMainLoop)
  finalizar_aplicacao();
  return 0;
}
