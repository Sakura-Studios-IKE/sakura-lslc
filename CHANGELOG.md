# sakura-lslc — Changelog

All notable changes are documented here. Format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/);
versions follow [SemVer](https://semver.org/).
The `[Unreleased]` section is what's on `main`; the release pipeline
promotes it to a numbered version on tag.

## [Unreleased]

## [1.0.0] — 2026-05-15

### Added
- Initial release of the Sakura LSL compiler: full front-end for the
  Linden Scripting Language across `src/lexer.c`, `src/preproc.c`,
  `src/parser.c`, `src/ast.c`, `src/sem.c`, `src/symtab.c`,
  `src/types.c`, `src/fold.c`, `src/diag.c`, `src/builtins.c`,
  `src/util.c` and the `src/main.c` driver.
- C-style preprocessor with `#define` (object and function-like),
  `#include`, `#ifdef` / `#ifndef` / `#if` / `#elif` / `#else` /
  `#endif`, `#undef`, `#error`, and `#warning`.
- Semantic analyser covering all LSL types (integer, float, string,
  key, vector, rotation/quaternion, list), member access, implicit
  conversions, ternary type-checking, state/event signature validation,
  jump/label resolution, redeclaration detection, and `-Wall`-style
  warnings (unused globals, shadowed builtins, unknown `ll*`).
- Constant folding for arithmetic, bitwise, comparison, and logical
  expressions.
- Full LSL built-in library: 540 functions, 534 constants, 49 events
  baked into `src/builtins.c`.
- gcc-style diagnostics with file/line/column, source caret, and
  colourised output (auto-disabled with `-fno-color`).
- Test suite: 13 pass / 17 fail fixtures under `tests/` with
  `tests/run_tests.sh` runner.
- `-c` / `--emit-bytecode` flag that lowers the typed AST to SLBC
  (Sakura LSL ByteCode) via `src/emit.c` for execution by `slemu`.
- SLBC v2 container with embedded source line numbers per opcode,
  enabling source-level tracing and debugging in `slemu` / `lsldb`.
- Coverage corpus under `tests/coverage/` (80+ pass + fail fixtures)
  exercising every type, operator, control-flow construct, event,
  preprocessor feature, and memory-budget edge case, plus
  `tests/coverage/run_coverage.sh` and `COVERAGE_REPORT.md`.
- `--list-builtins-detailed` flag that dumps the full built-in table
  (signatures, return types, Mono-only flag) in a machine-readable
  form for IDE tooling — consumed by the IntelliJ plugin.

### Changed
- README cross-references the other four toolchain repos
  (`sakura-slemu`, `sakura-lsldb`, `sakura-lsltest`,
  `sakura-lsl-toolchain`) and reports current test counts.
