#!/usr/bin/env bash
set -euo pipefail

# Script de build para Git Bash/MSYS2/WSL no Windows.
# Requer gcc, bash e FreeGLUT instalados no ambiente escolhido.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

CC="${CC:-gcc}"
BUILD_DIR="${BUILD_DIR:-build-windows}"
TARGET="${TARGET:-bin/curvas-parametricas-2d.exe}"
VERSION="$(tr -d '\r\n' < VERSION)"

CPPFLAGS=(-Iincludes "-DPROJECT_VERSION=\"${VERSION}\"")
CFLAGS=(-O2 -g -std=c99 -Wall -Wextra -Wpedantic -MMD -MP)

case "$(uname -s 2>/dev/null || printf '%s' unknown)" in
  MINGW*|MSYS*|CYGWIN*)
    LDLIBS=(-lfreeglut -lopengl32 -lglu32 -lm)
    ;;
  *)
    # Permite usar o mesmo script no WSL e em ambientes Unix para validação.
    LDLIBS=(-lGL -lGLU -lglut -lm)
    ;;
esac

fontes(){
  find src -type f -name '*.c' -print | sort
}

objeto_para_fonte(){
  local fonte="$1"
  local relativo="${fonte#src/}"
  relativo="${relativo//\//__}"
  printf '%s/%s.o\n' "$BUILD_DIR" "${relativo%.c}"
}

compilar(){
  mkdir -p "$BUILD_DIR" "$(dirname "$TARGET")"
  local objetos=()
  while IFS= read -r fonte; do
    local objeto extra
    objeto="$(objeto_para_fonte "$fonte")"
    mkdir -p "$(dirname "$objeto")"
    extra=()
    case "$fonte" in
      src/opengl.c|src/curvas/*.c) extra=(-finstrument-functions) ;;
      src/performance.c) extra=(-Wno-pedantic) ;;
    esac
    "$CC" "${CPPFLAGS[@]}" "${CFLAGS[@]}" "${extra[@]}" -c "$fonte" -o "$objeto"
    objetos+=("$objeto")
  done < <(fontes)
  "$CC" "${objetos[@]}" "${LDLIBS[@]}" -o "$TARGET"
}

limpar(){
  rm -rf "$BUILD_DIR" "$TARGET"
}

ajuda(){
  cat <<'EOF'
Uso: ./MakeFile.sh [comando]

Comandos:
  build       Compila o executável (padrão)
  test        Compila e executa os testes automatizados
  run         Compila e inicia a aplicação
  benchmark   Compila e executa o benchmark de linha de comando
  clean       Remove os artefatos gerados
  help        Exibe esta ajuda
EOF
}

comando="${1:-build}"
case "$comando" in
  build)
    compilar
    ;;
  test)
    compilar
    bash tests/run_tests.sh
    ;;
  run)
    compilar
    "./$TARGET"
    ;;
  benchmark)
    compilar
    "./$TARGET" --benchmark "${2:-1000}"
    ;;
  clean)
    limpar
    ;;
  help|-h|--help)
    ajuda
    ;;
  *)
    printf 'Comando desconhecido: %s\n\n' "$comando" >&2
    ajuda >&2
    exit 2
    ;;
esac
