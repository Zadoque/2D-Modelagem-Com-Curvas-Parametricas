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
- A renderização da curva e dos controles requer suporte a VBOs no contexto
  OpenGL. Elementos auxiliares da interface, eixos e caixas de depuração ainda
  usam o modo imediato legado.
- São necessários ao menos quatro controles para um segmento cúbico.
- O benchmark mede geração isolada, não FPS da aplicação.

## Renderização por VBO

A geometria principal não é mais enviada vértice a vértice em cada frame. Há
dois *Vertex Buffer Objects* (VBOs): um para a nuvem que aproxima a curva e
outro para os controles. Quando a geometria muda, o projeto realoca o buffer
com `glBufferData` somente se necessário; nos demais casos atualiza seu
conteúdo com `glBufferSubData`. Em frames estáticos, nada é reenviado. Cada
objeto é desenhado com uma chamada `glDrawArrays`.

Trecho em [`src/opengl.c`](src/opengl.c):

```c
if(bytes > *capacidade){
  glBufferData(GL_ARRAY_BUFFER, bytes, pontos->data, GL_DYNAMIC_DRAW);
  *capacidade = bytes;
} else {
  glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, pontos->data);
}
/* ... */
glDrawArrays(modo, 0, quantidade);
```

Isso remove uma chamada de API por vértice da curva e desloca o gargalo para a
geração geométrica, picking e validação de autointerferência. Eixos, widgets do
menu e visualizações de depuração continuam em modo imediato porque contêm
poucos vértices e não determinam o limite prático de renderização.

## Métodos matemáticos utilizados

As fórmulas usam controles \(P_i\), parâmetro \(t\in[0,1]\) e pontos 2D. Os
trechos são curtos; cada link leva à implementação completa no projeto.

### Bézier cúbica — Pierre Bézier e Paul de Casteljau

Pierre Bézier popularizou a representação para CAD; Paul de Casteljau
desenvolveu independentemente seu algoritmo de avaliação. O projeto usa a base
de Bernstein:

\[
B(t)=(1-t)^3P_0+3t(1-t)^2P_1+3t^2(1-t)P_2+t^3P_3.
\]

Implementação: [`src/curvas/bezier.c`](src/curvas/bezier.c).

```c
float u = 1 - t, w[4] = {u*u*u, 3*t*u*u, 3*t*t*u, t*t*t};
r.point[e] += w[i] * p[i].point[e];
```

### Hermite cúbica — Charles Hermite

A forma de Hermite interpola extremidades e tangentes. Aqui,
\(T_1=P_2-P_0\), \(T_2=P_3-P_1\), e

\[
H(t)=h_{00}P_1+h_{01}P_2+h_{10}T_1+h_{11}T_2,
\]

onde \(h_{00}=2t^3-3t^2+1\), \(h_{01}=-2t^3+3t^2\),
\(h_{10}=t^3-2t^2+t\) e \(h_{11}=t^3-t^2\).

Implementação: [`src/curvas/hermite.c`](src/curvas/hermite.c).

```c
t1->point[e] = p2.point[e] - p0.point[e];
t2->point[e] = p3.point[e] - p1.point[e];
float w[4] = {2*c - 3*q + 1, -2*c + 3*q, c - 2*q + t, c - q};
```

### Catmull–Rom cardinal — Edwin Catmull e Raphael Rom

Catmull e Rom propuseram a spline interpolante cardinal. Para tensão padrão,

\[
C(t)=\tfrac12[(-t^3+2t^2-t)P_0+(3t^3-5t^2+2)P_1+
(-3t^3+4t^2+t)P_2+(t^3-t^2)P_3].
\]

Implementação: [`src/curvas/catmullrom.c`](src/curvas/catmullrom.c).

```c
w[0] = (-t3 + 2*t2 - t) / 2;
w[1] = (3*t3 - 5*t2 + 2) / 2;
w[2] = (-3*t3 + 4*t2 + t) / 2;
w[3] = (t3 - t2) / 2;
```

### B-spline cúbica uniforme — Isaac J. Schoenberg; Oslo e Lyche–Mørken

As B-splines foram formalizadas por Isaac J. Schoenberg. O trecho uniforme
cúbico usa as bases

\[
\tfrac16\{(1-t)^3,\;3t^3-6t^2+4,\;-3t^3+3t^2+3t+1,\;t^3\}.
\]

Implementação: [`src/curvas/bspline.c`](src/curvas/bspline.c).

```c
w[0] = (-t3 + 3*t2 - 3*t + 1) / 6;
w[1] = (3*t3 - 6*t2 + 4) / 6;
w[2] = (-3*t3 + 3*t2 + 3*t + 1) / 6;
w[3] = t3 / 6;
```

A adaptação de densidade usa inserção de nós de Oslo (Elaine Cohen, Tom Lyche
e Richard Riesenfeld). A remoção Lyche–Mørken (Tom Lyche e Knut Mørken) só é
aceita se a reconstrução ficar abaixo da tolerância. Implementação:
[`src/bspline_topologia.c`](src/bspline_topologia.c).

```c
int inserir_nos_oslo(BSplineTopologia *spline, const float *nos, int quantidade);
int remover_nos_lyche_morken(BSplineTopologia *spline, const float *nos,
                              int quantidade, float tolerancia);
```

### OBB por covariância e SAT — Rivera, Carvalho e Velho

As OBBs usam a orientação por covariância de L. Rivera, P. C. Carvalho e L.
Velho (2003). Para amostras \(x_i\), calcula-se \(\mu\) e
\(\Sigma=\frac{1}{n-1}\sum_i(x_i-\mu)(x_i-\mu)^T\). O autovetor principal
define o eixo da OBB; em 2D,

\[
\theta=\tfrac12\operatorname{atan2}(2\Sigma_{xy},\Sigma_{xx}-\Sigma_{yy}).
\]

Implementação: [`src/obb.c`](src/obb.c).

```c
float angulo = 0.5f * atan2f(2.0f * xy, xx - yy);
eixos[0] = (ponto){{cosf(angulo), sinf(angulo), 1.0f}};
eixos[1] = (ponto){{-sinf(angulo), cosf(angulo), 1.0f}};
```

Os extents são ampliados com os controles locais, pois uma B-spline fica no
casco convexo desses controles. A fase ampla usa o *Separating Axis Theorem*
(SAT): dois convexos estão separados se existe um eixo cujas projeções não se
sobrepõem. A implementação está na função `obb_intersecta_sat` do mesmo
arquivo.

### Transformações homogêneas 2D

Translação, rotação, escala e cisalhamento usam matrizes 3×3 e vetores-linha
homogêneos \([x\;y\;1]\). A rotação é

\[
R(\alpha)=\begin{bmatrix}\cos\alpha&\sin\alpha&0\\-\sin\alpha&\cos\alpha&0\\0&0&1\end{bmatrix}.
\]

Implementação: [`src/matriz.c`](src/matriz.c).

```c
float c = cosf(a), s = sinf(a);
Matriz3 m = {{{c, s, 0}, {-s, c, 0}, {0, 0, 1}}};
```

## Referências

- Bézier, P. *Numerical Control: Mathematics and Applications*. Wiley, 1972.
- de Casteljau, P. *Outillage méthodes de calcul*. Citroën, 1959.
- Schoenberg, I. J. “Contributions to the Problem of Approximation of
  Equidistant Data by Analytic Functions”. *Quarterly of Applied Mathematics*,
  1946.
- Catmull, E.; Rom, R. “A Class of Local Interpolating Splines”. In:
  *Computer Aided Geometric Design*, 1974.
- Cohen, E.; Lyche, T.; Riesenfeld, R. “Discrete B-Splines and Subdivision
  Techniques in Computer-Aided Geometric Design”. *Computer Graphics and
  Image Processing*, 1980.
- Lyche, T.; Mørken, K. “Knot Removal for Parametric B-Spline Curves and
  Surfaces”. *Computer Aided Geometric Design*, 1987.
- Rivera, L.; Carvalho, P. C.; Velho, L. “Oriented Bounding Boxes Based on
  Multiresolution Contours”. *I Congreso Internacional de Científicos
  Peruanos*, 2003.

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
