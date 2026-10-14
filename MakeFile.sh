#!/bin/sh

# Nome do projeto
PROJECT_NAME="computacao_grafica"

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== Compilando $PROJECT_NAME ===${NC}"

# Criar diretórios necessários
echo "Criando diretórios..."
mkdir -p obj bin

# Verificar se o diretório de curvas existe
if [ ! -d "src/curvas" ]; then
    echo -e "${YELLOW}Aviso: Diretório src/curvas/ não encontrado${NC}"
    echo "Criando diretório src/curvas/..."
    mkdir -p src/curvas
fi

# Definir lista de fontes para compilar
SRC_FILES=(
  src/main.c
  src/pontos.c
  src/opengl.c
  src/performance.c
  src/menu.c
  src/matriz.c
  src/vetor.c
  src/aabb.c
  src/transformacoes.c
  src/curvas/hermite.c
  src/curvas/curva_generica.c
  src/curvas/bezier.c
  src/curvas/bspline.c
  src/curvas/catmullrom.c
  src/curvas/ultis/ultis.c
)

# Verificar se os arquivos existem
echo -e "${BLUE}Verificando arquivos fonte...${NC}"
MISSING_FILES=0
for file in "${SRC_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${YELLOW}⚠ Arquivo não encontrado: $(basename $file)${NC}"
        MISSING_FILES=1
    else
        echo -e "${GREEN}✓ $(basename $file)${NC}"
    fi
done

if [ $MISSING_FILES -eq 1 ]; then
    echo -e "${YELLOW}Alguns arquivos estão faltando, mas continuando com os disponíveis...${NC}"
fi

# Listar arquivos que serão compilados
echo -e "${BLUE}Compilando arquivos encontrados:${NC}"

# Compilar arquivos .c individualmente
COMPILE_ERROR=0
COMPILED_COUNT=0

for srcfile in "${SRC_FILES[@]}"; do
    if [ -f "$srcfile" ]; then
        objfile="obj/$(basename ${srcfile%.c}.o)"
        echo -n "  $(basename $srcfile) -> $(basename $objfile)... "

        # A instrumentação é aplicada às funções de curva e ao desenho da curva.
        EXTRA_CFLAGS=""
        case "$srcfile" in
          src/opengl.c|src/curvas/*.c) EXTRA_CFLAGS="-finstrument-functions" ;;
        esac

        if gcc -c -Wall -Wextra -std=c99 $EXTRA_CFLAGS -Iinclude "$srcfile" -o "$objfile" 2>/dev/null; then
            echo -e "${GREEN}OK${NC}"
            ((COMPILED_COUNT++))
        else
            echo -e "${RED}ERRO${NC}"
            echo -e "${RED}Detalhes do erro para $(basename $srcfile):${NC}"
            gcc -c -Wall -Wextra -std=c99 $EXTRA_CFLAGS -Iinclude "$srcfile" -o "$objfile"
            COMPILE_ERROR=1
        fi
    fi
done

# Verificar se houve erros na compilação
if [ $COMPILE_ERROR -eq 1 ]; then
    echo -e "${RED}Falha na compilação!${NC}"
    exit 1
fi

if [ $COMPILED_COUNT -eq 0 ]; then
    echo -e "${RED}Nenhum arquivo foi compilado!${NC}"
    exit 1
fi

# Verificar se todos os arquivos objeto foram criados
echo -e "${BLUE}Verificando arquivos objeto...${NC}"
EXPECTED_FILES=(main pontos opengl menu hermite bezier bspline catmullrom)
FOUND_OBJECTS=0

for file in "${EXPECTED_FILES[@]}"; do
    if [ -f "obj/${file}.o" ]; then
        echo -e "  ${file}.o: ${GREEN}✓${NC}"
        ((FOUND_OBJECTS++))
    else
        echo -e "  ${file}.o: ${YELLOW}⚠ (não encontrado)${NC}"
    fi
done

echo -e "${BLUE}Total de objetos compilados: ${FOUND_OBJECTS}/${#EXPECTED_FILES[@]}${NC}"

# Linkar e gerar executável
echo -e "${BLUE}Linkando executável...${NC}"
if gcc obj/*.o -lGL -lGLU -lglut -lm -o bin/$PROJECT_NAME 2>/dev/null; then
    echo -e "${GREEN}Linkagem concluída com sucesso!${NC}"
else
    echo -e "${RED}Erro ao linkar o executável!${NC}"
    echo -e "${RED}Detalhes do erro:${NC}"
    gcc obj/*.o -lGL -lGLU -lglut -lm -o bin/$PROJECT_NAME
    exit 1
fi


echo -e "${GREEN}╔════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     Compilação Bem Sucedida!       ║${NC}"
echo -e "${GREEN}║   Arquivos compilados: $COMPILED_COUNT/8          ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════╝${NC}"
echo -e "${GREEN}Executável: bin/$PROJECT_NAME${NC}"

# Verificar se o executável foi criado
if [ -f "bin/$PROJECT_NAME" ]; then
    echo -e "${YELLOW}Executando programa...${NC}"
    echo -e "${BLUE}(Pressione Ctrl+C para sair)${NC}"
    ./bin/$PROJECT_NAME
else
    echo -e "${RED}Erro: Executável não foi gerado!${NC}"
    exit 1
fi
