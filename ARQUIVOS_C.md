# Arquivos `.c` do projeto

Este documento descreve a responsabilidade de cada arquivo-fonte C do projeto de modelagem 2D com curvas paramétricas. Os arquivos estão organizados por função para facilitar a navegação, mas todos os arquivos `.c` existentes em `src/` e `tests/` estão listados aqui.

## Visão geral do fluxo

1. [`src/main.c`](src/main.c) inicializa a aplicação, a janela OpenGL e os modos de execução pela linha de comando.
2. [`src/opengl.c`](src/opengl.c) coordena o estado interativo, eventos de mouse/teclado e renderização da interface.
3. Os módulos de curvas avaliam e geram pontos das curvas.
4. [`src/geometria_curvas.c`](src/geometria_curvas.c), [`src/aabb.c`](src/aabb.c) e [`src/obb.c`](src/obb.c) fornecem amostragem, caixas envolventes, picking e detecção geométrica.
5. [`src/renderizacao_buffers.c`](src/renderizacao_buffers.c) mantém os buffers OpenGL usados para desenhar as curvas.
6. Os módulos de benchmark medem geração, desenho, picking, arrasto e o custo adicional da autointerferência.

## Aplicação e interface

### [`src/main.c`](src/main.c)

É o ponto de entrada do executável. Processa opções como `--version`, `--benchmark` e `--benchmark-vbo`, inicializa GLUT/OpenGL, cria a janela e registra os callbacks de desenho, entrada e redimensionamento.

### [`src/opengl.c`](src/opengl.c)

Implementa o ciclo principal da aplicação gráfica. Mantém o estado da curva selecionada, transforma coordenadas de tela em coordenadas do projeto, trata seleção e arrasto de pontos, desenha a cena e coordena os painéis de menu e benchmark. Também contém a integração das operações topológicas de B-spline durante a edição.

### [`src/menu.c`](src/menu.c)

Implementa os controles da interface: menu superior, botões, caixas de seleção, controles `+`/`-`, sliders e abertura do painel de benchmark. Encaminha alterações de densidade, rigidez, seletor de distância e autointerferência para o estado da aplicação.

### [`src/renderizacao_buffers.c`](src/renderizacao_buffers.c)

Gerencia os VBOs usados na renderização. Cria, atualiza, desenha e libera buffers de vértices e índices. Também evita atualizações redundantes quando a assinatura dos dados geométricos não mudou.

### [`src/contadores_interface.c`](src/contadores_interface.c)

Mantém e exibe contadores usados pela interface, como operações de inserção e remoção de nós durante o arrasto de B-splines.

## Geometria, álgebra e transformações

### [`src/pontos.c`](src/pontos.c)

Fornece a estrutura dinâmica de pontos de controle e suas operações básicas: criação, cópia, inserção, remoção, realocação, distância entre pontos, interpolação e distância ponto-segmento.

### [`src/vetor.c`](src/vetor.c)

Implementa operações vetoriais em 2D, incluindo soma, subtração, produto por escalar, produto interno, norma, normalização e operações auxiliares de geometria.

### [`src/matriz.c`](src/matriz.c)

Implementa operações com matrizes utilizadas nas transformações 2D, como criação, identidade, multiplicação e aplicação a pontos/vetores.

### [`src/transformacoes.c`](src/transformacoes.c)

Converte entre coordenadas de tela, coordenadas do projeto e coordenadas normalizadas. Também implementa translação, escala e outras transformações usadas pela câmera e pela interação.

### [`src/slider.c`](src/slider.c)

Implementa o widget de slider da interface, incluindo conversão entre posição do mouse e valor, limites, desenho e tratamento de interação.

### [`src/densidade.c`](src/densidade.c)

Centraliza a configuração da densidade de amostragem das curvas. Aplica os limites permitidos e converte a posição do seletor em quantidade efetiva de amostras.

### [`src/aabb.c`](src/aabb.c)

Implementa caixas delimitadoras alinhadas aos eixos (AABB), interseção, expansão, distância mínima e a árvore AABB usada para acelerar picking e consultas espaciais.

### [`src/obb.c`](src/obb.c)

Implementa caixas delimitadoras orientadas (OBB), árvores de OBB, testes de interseção entre segmentos e a verificação de autointerferência. Usa amostras/cache geométrico quando apropriado para reduzir recomputações.

### [`src/geometria_curvas.c`](src/geometria_curvas.c)

É a camada comum de geometria das curvas. Gera amostras, faixas de parâmetros, AABBs e estruturas espaciais para as famílias de curvas. Para B-splines topológicas, usa os spans e a representação atual dos nós.

## Famílias de curvas

### [`src/curvas/curva_generica.c`](src/curvas/curva_generica.c)

Contém a infraestrutura compartilhada pelas curvas: subdivisão adaptativa, geração de polilinhas, picking por árvore espacial e suporte ao arrasto. O critério geométrico de subdivisão compara o ponto em `Tmid` com o segmento formado pelos extremos do intervalo.

### [`src/curvas/bezier.c`](src/curvas/bezier.c)

Implementa avaliação e geração de curvas Bézier, incluindo a base de Bernstein e as operações genéricas de picking e arrasto adaptadas a essa curva.

### [`src/curvas/hermite.c`](src/curvas/hermite.c)

Implementa avaliação e geração de curvas Hermite a partir dos pontos e tangentes correspondentes.

### [`src/curvas/catmullrom.c`](src/curvas/catmullrom.c)

Implementa avaliação e geração de curvas Catmull–Rom, incluindo o tratamento das janelas de pontos e interpolação dos segmentos.

### [`src/curvas/bspline.c`](src/curvas/bspline.c)

Implementa avaliação e geração de B-splines na representação geométrica convencional. A manipulação da topologia, inserção e remoção de nós fica em [`src/bspline_topologia.c`](src/bspline_topologia.c).

### [`src/curvas/utils.c`](src/curvas/utils.c)

Reúne utilitários comuns das curvas, como distância entre AABBs e construção de caixas conservadoras para intervalos de curvas cúbicas. O arquivo anteriormente chamado `ultis.c` foi padronizado para `utils.c`.

## Topologia de B-spline

### [`src/bspline_topologia.c`](src/bspline_topologia.c)

Implementa a representação topológica da B-spline, nós e spans, avaliação por intervalo, obtenção de span, remapeamento de parâmetros e arrasto. Também contém as operações Oslo de inserção e Lyche–Mørken de remoção, incluindo restauração do estado anterior quando uma operação não é aceita.

### [`src/bspline_topologia_candidatos.c`](src/bspline_topologia_candidatos.c)

Seleciona candidatos locais e globais para remoção de nós. A seleção usa as distâncias entre pontos de controle consecutivos e percorre cada aresta uma única vez, evitando recalcular a mesma distância para vários pontos candidatos.

## Benchmark e instrumentação

### [`src/performance.c`](src/performance.c)

Contém a instrumentação de desempenho das rotinas principais e o benchmark de linha de comando. Mede com temporizador monotônico de alta resolução, acumulando contagens e tempos de geração, desenho e outras operações instrumentadas.

### [`src/benchmark_painel.c`](src/benchmark_painel.c)

Implementa o painel OpenGL de configuração e execução. Permite selecionar curvas, tipos de teste, repetições, warm-up, rigidez, seletor de distância, autointerferência, raio máximo do deslocamento e quantidade de pontos de controle. Gera pontos aleatórios para as curvas, simula arrasto discretizado e, para B-splines, registra inserções e remoções topológicas. O teste de autointerferência compara arrastos pareados com a checagem desligada e ligada.

### [`src/benchmark_resultados.c`](src/benchmark_resultados.c)

Define os resultados, valores padrão, gerador pseudoaleatório e operações de acumulação e média dos testes. As médias de tempo e de inserções/remoções são calculadas separadamente das iterações de warm-up.

## Testes automatizados

Os arquivos abaixo são executáveis de teste, compilados pelo script [`tests/run_tests.sh`](tests/run_tests.sh). Cada um cobre um subsistema específico.

### [`tests/test_aabb.c`](tests/test_aabb.c)

Testa criação, expansão, interseção e consultas da AABB, além dos utilitários geométricos associados.

### [`tests/test_alocacao.c`](tests/test_alocacao.c)

Testa caminhos de falha de alocação e o comportamento dos wrappers de memória usados pelo projeto.

### [`tests/test_benchmark_painel.c`](tests/test_benchmark_painel.c)

Testa configuração do painel, limites dos parâmetros, geração aleatória, estado do gerador e acumulação dos resultados de benchmark.

### [`tests/test_bspline_topologia.c`](tests/test_bspline_topologia.c)

Testa criação da topologia, avaliação por span, inserção Oslo, remoção Lyche–Mørken, seleção de candidatos, remapeamento, rollback e estatísticas do arrasto.

### [`tests/test_contadores_interface.c`](tests/test_contadores_interface.c)

Testa inicialização, incremento, limpeza e apresentação dos contadores de inserções e remoções da interface.

### [`tests/test_curvas.c`](tests/test_curvas.c)

Testa as operações comuns e específicas das famílias Bézier, Hermite, Catmull–Rom e B-spline, incluindo avaliação e geração de pontos.

### [`tests/test_densidade.c`](tests/test_densidade.c)

Testa limites, conversão e atualização dos valores de densidade de amostragem.

### [`tests/test_distancia.c`](tests/test_distancia.c)

Testa distâncias entre pontos, ponto-segmento e demais primitivas métricas usadas na subdivisão e na seleção de candidatos.

### [`tests/test_geometria_curvas.c`](tests/test_geometria_curvas.c)

Testa amostragem, faixas paramétricas, caixas envolventes e a construção da geometria renderizável das curvas.

### [`tests/test_obb.c`](tests/test_obb.c)

Testa OBBs, árvore espacial, interseções e detecção de autointerferência, inclusive nos casos de spans compartilhados.

### [`tests/test_picking.c`](tests/test_picking.c)

Testa a seleção de segmentos e pontos por coordenada, usando as estruturas AABB/OBB e os critérios de distância da curva.

### [`tests/test_slider.c`](tests/test_slider.c)

Testa conversão de posição, valores mínimo/máximo e atualização do slider.

### [`tests/test_transformacoes.c`](tests/test_transformacoes.c)

Testa transformações entre coordenadas de tela e do projeto, além das operações de câmera usadas pela interface.

## Relação com a compilação

O [`Makefile`](Makefile) compila os módulos de `src/` e gera dependências automáticas de headers (`.d`). Os testes compartilham suas listas de fontes por meio de [`tests/run_tests.sh`](tests/run_tests.sh), evitando duplicação da relação entre módulos e executáveis.

