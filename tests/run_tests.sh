#!/usr/bin/env bash
set -euo pipefail

CC=${CC:-gcc}
CFLAGS=(-std=c99 -Wall -Wextra -Wpedantic -Werror -Iincludes)
CURVAS=(src/pontos.c src/aabb.c src/curvas/ultis/ultis.c src/curvas/bezier.c src/curvas/bspline.c src/curvas/hermite.c src/curvas/catmullrom.c)

build_run() {
  local nome=$1
  shift
  "$CC" "${CFLAGS[@]}" "tests/$nome.c" "$@" -lm -o "/tmp/$nome"
  "/tmp/$nome"
}

build_run test_distancia src/pontos.c
build_run test_aabb src/pontos.c src/aabb.c src/curvas/ultis/ultis.c
build_run test_curvas "${CURVAS[@]}"
build_run test_picking "${CURVAS[@]}"
build_run test_transformacoes src/pontos.c src/matriz.c src/transformacoes.c
build_run test_alocacao src/pontos.c src/aabb.c src/vetor.c -Wl,--wrap=realloc
