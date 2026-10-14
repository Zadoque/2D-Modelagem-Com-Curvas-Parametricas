# 2D modeling with parametric curves

Interactive C99/OpenGL application for Bézier, Hermite, uniform B-Spline, and
Catmull–Rom curves, with adaptive subdivision, transformations, and AABB-tree
picking. Current version: **0.1.0**. [Portuguese documentation](README.md).

## Build

Install a C compiler, GNU Make, OpenGL, GLU, and FreeGLUT. On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install build-essential freeglut3-dev libgl1-mesa-dev libglu1-mesa-dev
make
make run
```

Use `make test`, `make benchmark`, and `make clean` for the corresponding
tasks. A non-pinned `shell.nix` is also available.

## Guarantees and limitations

- Conservative AABBs derive from equivalent Bézier control hulls.
- Recursion is capped at 20 levels and constrained by node budgets.
- Generation work adapts to measured machine throughput.
- Picking and rendering are approximations and may hit work limits before the
  requested tolerance in extreme cases.
- Picking trees are intentionally rebuilt only on mouse release.
- Tolerances use world coordinates rather than pixels.
- Only one active curve is supported; there is no persistence or undo/redo.
- The immediate-mode OpenGL interface has been validated on Linux only.

See the [Portuguese README](README.md) for all installation commands,
architecture, test coverage, benchmark usage, and installation targets.

## License

Released under the [MIT License](LICENSE). Copyright © 2026 Zadoque Carneiro.
