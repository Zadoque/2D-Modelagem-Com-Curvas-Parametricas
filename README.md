# Modelagem 2D com curvas paramétricas

Aplicativo interativo em C99 e OpenGL/FreeGLUT para construir e manipular uma
curva 2D. Implementa Bézier, Hermite, B-Spline uniforme e Catmull–Rom, com
geração adaptativa, transformações e picking por árvore AABB.

Versão atual: **0.1.0**. [English documentation](README_EN.md).

## Funcionalidades

- Polígonos de controle abertos e fechados e uma curva ativa por vez.
- Criação, remoção e arrasto de controles e da própria curva.
- Translação, rotação e escala pela interface.
- Subdivisão adaptativa compartilhada pelas quatro famílias.
- Picking por branch-and-bound e benchmark executado sob solicitação.

## Garantias técnicas

- As AABBs são conservadoras para entradas finitas: usam o fecho convexo dos
  controles Bézier equivalentes e uma margem para arredondamento em `float`.
- A recursão tem limite estrutural de 20 níveis e orçamentos absolutos de nós.
- O orçamento de geração se adapta à vazão observada da máquina; o limite da
  pilha continua independente do hardware.
- Falhas de `realloc` preservam o buffer anterior nas estruturas testadas.
- Matrizes 3×3 são armazenadas por valor, sem alocação dinâmica.
- Atualização geométrica e renderização são etapas separadas.

## Limitações conhecidas

- Picking e renderização são aproximações. Em casos extremos, os limites
  de profundidade ou trabalho podem ser atingidos antes da tolerância pedida.
- As tolerâncias usam coordenadas do mundo, não pixels.
- A árvore de picking é reconstruída somente no `mouse-up`. Durante o arrasto,
  a geometria muda, mas a árvore anterior não é usada para novos cliques.
- Não há undo/redo, persistência, exportação ou múltiplas curvas.
- O OpenGL em modo imediato foi validado apenas no Linux.
- São necessários ao menos quatro controles para um segmento cúbico.
- O benchmark mede geração isolada, não FPS da aplicação.

## Dependências

- Compilador C99, GNU Make, OpenGL, GLU e FreeGLUT;
- servidor gráfico com suporte a OpenGL para executar a interface.

Ubuntu/Debian:

```sh
sudo apt update
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
```

Fedora:

```sh
sudo dnf install gcc make freeglut-devel mesa-libGL-devel mesa-libGLU-devel
```

Arch Linux:

```sh
sudo pacman -S base-devel freeglut glu
```

No Nix, execute `nix-shell`. O `shell.nix` usa o canal Nixpkgs da máquina e
não é pinado.

## Compilação, testes e instalação

```sh
git clone https://github.com/Zadoque/2D-Modelagem-Com-Curvas-Parametricas.git
cd 2D-Modelagem-Com-Curvas-Parametricas
make
make run
```

```sh
make test
make benchmark
make clean
make install PREFIX="$HOME/.local"
make uninstall PREFIX="$HOME/.local"
```

Uso direto:

```sh
./bin/curvas-parametricas-2d --version
./bin/curvas-parametricas-2d --benchmark 5000
```

A suíte cobre curvas, distância ponto–segmento, AABBs, árvore, picking,
cache geométrico, transformações e falhas simuladas de realocação.

## Arquitetura

- `src/curvas/curva_generica.c`: subdivisão, picking e arrasto compartilhados.
- `src/geometria_curvas.c`: cache geométrico sem dependência de OpenGL.
- `src/opengl.c`: eventos e renderização do cache.
- `src/aabb.c`: vetor de caixas e árvore em arena contígua.
- `src/matriz.c` e `src/transformacoes.c`: transformações homogêneas 2D.
- `src/performance.c`: instrumentação e benchmark explícito.
- `tests/`: testes independentes da janela gráfica.

## Versão e licença

O projeto usa versionamento semântico a partir de `0.1.0`; o build lê a
versão de `VERSION`. Distribuído sob a [licença MIT](LICENSE).

Copyright © 2026 Zadoque Carneiro.
