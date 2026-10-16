#!/usr/bin/env bash
set -euo pipefail

CC=${CC:-gcc}
CFLAGS=(-std=c99 -Wall -Wextra -Wpedantic -Werror -Iincludes)
CURVAS=(src/pontos.c src/aabb.c src/curvas/utils.c src/curvas/curva_generica.c src/curvas/bezier.c src/curvas/bspline.c src/curvas/hermite.c src/curvas/catmullrom.c)
BSPLINE=(src/bspline_topologia.c src/bspline_topologia_candidatos.c)
GEOMETRIA=(src/geometria_curvas.c)
OBB=(src/obb.c)

build_run() {
  local nome=$1
  shift
  "$CC" "${CFLAGS[@]}" "tests/$nome.c" "$@" -lm -o "/tmp/$nome"
  "/tmp/$nome"
}

build_run test_distancia src/pontos.c
build_run test_aabb src/pontos.c src/aabb.c src/curvas/utils.c
build_run test_obb src/pontos.c src/curvas/utils.c "${BSPLINE[@]}" "${OBB[@]}"
build_run test_curvas "${CURVAS[@]}"
build_run test_bspline_topologia src/pontos.c src/aabb.c src/curvas/utils.c src/densidade.c "${BSPLINE[@]}"
build_run test_picking "${CURVAS[@]}"
build_run test_geometria_curvas "${CURVAS[@]}" "${BSPLINE[@]}" "${GEOMETRIA[@]}"
build_run test_transformacoes src/pontos.c src/matriz.c src/transformacoes.c
build_run test_slider src/slider.c
build_run test_densidade src/densidade.c
build_run test_contadores_interface src/pontos.c src/contadores_interface.c
build_run test_benchmark_painel "${CURVAS[@]}" "${BSPLINE[@]}" "${OBB[@]}" "${GEOMETRIA[@]}" src/benchmark_resultados.c src/benchmark_painel.c
build_run test_alocacao src/pontos.c src/aabb.c src/vetor.c -Wl,--wrap=realloc
