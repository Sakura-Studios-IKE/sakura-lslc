# sakura-lslc

**An offline LSL Mono / LSO compiler in pure C99 — by Sakura Studios, IKE.**

`sakura-lslc` (binary: `lslc`) is a from-scratch Second Life / OpenSim
LSL compiler front-end. It does everything the in-viewer LSL compiler
does — lex, parse, name-resolve, type-check, fold constants, enforce
memory budgets — and a lot of things it doesn't: rich gcc-style
diagnostics with carets, ranges and "did you mean?" suggestions, a
full C-style preprocessor (`#include`, `#define`, `#ifdef`, …), and an
LSO compatibility mode that catches Mono-only built-ins.

If your script compiles here, it compiles when you upload it.

```sh
git clone https://github.com/ShihoSakura/sakura-lslc.git
cd sakura-lslc
make
./lslc my_script.lsl
```

```
my_script.lsl:42:13: error: cannot assign integer to string
   42 |         string s = 42;
      |                    ^~
  help: an explicit cast is required:  (string)expr
```

## Highlights

- **Complete LSL Mono front-end** — 429 built-in functions (matches the
  LSL Wiki function count), 530+ constants, every event handler with
  canonical signatures, the full operator/precedence table including
  vector / rotation arithmetic and list concatenation.
- **gcc-style diagnostics** — file:line:col, source-line carets with
  range underlines, `help:` hints with fix suggestions, "did you mean?"
  via Levenshtein distance over in-scope identifiers, built-ins,
  events, and states.
- **Constant folder** — global initialisers are evaluated at compile
  time (`integer N = 60 * 60 * 24;` → `86400`) and rejected if they
  reference variables or call functions, matching the SL uploader.
- **Memory budget check** — defaults are 64 KiB for Mono and 16 KiB
  for LSO; overridable with `--memory-budget=N`.
- **`--lso` mode** — switches the budget to 16 KiB and rejects
  Mono-only built-ins (`llJson*`, `llLinksetData*`, `llCastRay`,
  `llChar`, `llOrd`, `llHMAC`, `llTransferLindenDollars`, …).
- **C-style preprocessor** — `#include "p"` / `<p>`, object- and
  function-like `#define`, `#undef`, `#ifdef` / `#ifndef` /
  `#if defined()` / `#elif` / `#else` / `#endif`, `#error` / `#warning`,
  with `-I`, `-D`, and `-E` flags.
- **Pure C99, zero dependencies** — builds with gcc, clang, tcc, MSVC,
  or MinGW. ~5 000 lines of code; one statically-linked binary.
- **Portable** — Linux (Arch primary target), macOS, *BSD, Windows.

## Build

```sh
# Linux / macOS / *BSD
make

# Cross-platform via CMake (incl. MSVC)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Optional:

```sh
sudo make install            # to /usr/local/bin
make test                    # run the in-tree regression suite (30 cases)
```

## Use

```sh
lslc [options] file.lsl [more.lsl ...]
```

Common flags:

| Flag                | Effect |
|---------------------|--------|
| `-Wall`             | Enable extra warnings (shadowing, unused globals, unknown ll-functions). |
| `-Werror`           | Treat warnings as errors. |
| `--lso`             | Target LSO (16 KiB budget, no Mono-only built-ins). |
| `--memory-budget=N` | Override the per-script memory budget. |
| `-I PATH`           | Add directory to the `#include` search path. |
| `-D NAME[=value]`   | Pre-define a macro. |
| `-E`                | Print preprocessed text and exit. |
| `--list-builtins`   | Print every built-in function/constant/event known. |
| `-h, --help`        | Full usage. |

See [`DOCUMENTATION.md`](./DOCUMENTATION.md) for the complete language
coverage table, diagnostic style guide, memory-budget rules,
preprocessor reference, and test-corpus description.

## Project layout

```
sakura-lslc/
├── README.md
├── DOCUMENTATION.md      complete reference
├── LICENSE               MIT
├── Makefile              Linux / macOS / *BSD / MinGW
├── CMakeLists.txt        Windows / MSVC / cross-platform
├── src/                  C99 source (~5 000 lines)
│   ├── lsl.h
│   ├── ast.c
│   ├── builtins.c
│   ├── diag.c
│   ├── fold.c
│   ├── lexer.c
│   ├── main.c
│   ├── parser.c
│   ├── preproc.c
│   ├── sem.c
│   ├── symtab.c
│   ├── types.c
│   └── util.c
└── tests/
    ├── run_tests.sh
    ├── pass/             must compile cleanly
    └── fail/             must be rejected with the right diagnostic
```

## Status

`sakura-lslc` is the compiler in Sakura Studios' five-tool open-source
LSL toolchain:

1. **`sakura-lslc`** — this compiler. Catches every error the in-viewer
   uploader would, plus many it doesn't. Emits portable `.lslbc` bytecode.
2. [**`sakura-slemu`**](https://github.com/ShihoSakura/sakura-slemu) —
   headless region emulator that walks the typed AST, models the event
   loop, mocks `ll*` side effects, and routes
   `llMessageLinked` / `llRegionSay` / `llListen` across scripts.
3. [**`sakura-lsldb`**](https://github.com/ShihoSakura/sakura-lsldb) —
   gdb-style CLI debugger for `.lsl` and `.lslbc` (source breakpoints,
   step, print, catchpoints).
4. [**`sakura-lsltest`**](https://github.com/ShihoSakura/sakura-lsltest)
   — pytest-style test framework that drives `lslc` + `slemu` for
   automated LSL testing.
5. [**`sakura-intellij-lsl`**](https://github.com/ShihoSakura/sakura-intellij-lsl)
   — IntelliJ plugin: syntax / completion / inspections / run-in-emulator
   / hot-reload to Firestorm.

This repository's test status: 30/30 acceptance tests + 82/82 language-coverage
tests, all green on every supported platform.

## Author / Attribution

Authored and maintained by **Shiho Sakura**
([@ShihoSakura](https://github.com/ShihoSakura)) on behalf of
**Sakura Studios, IKE**.

The LSL language, built-in function set, constants, and event
signatures are © Linden Research, Inc. and are factual data referenced
here under fair use; nothing in this repository claims ownership of
them.

## License

MIT — see [`LICENSE`](./LICENSE).
