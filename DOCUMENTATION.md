# `sakura-lslc` — an offline LSL Mono / LSO compiler

> A Sakura Studios, IKE project · maintained by Shiho Sakura
> ([@ShihoSakura](https://github.com/ShihoSakura)) · MIT-licensed.

`sakura-lslc` (binary: `lslc`) is a pure-C, zero-dependency, command-line
compiler for **LSL Mono** (the Linden Scripting Language as it runs on
the Second Life Mono VM, also the OpenSim XEngine dialect), with an
optional `--lso` mode that enforces the older LSO VM's tighter memory
budget and rejects Mono-only built-ins. It performs the same syntactic
and semantic analysis the Second Life uploader does, but with much
richer diagnostics and from your shell — no viewer, no network
round-trip, no in-world prim.

The compiler stops at the *front-end*: it does not emit bytecode for the
SL Mono VM, because the bytecode format is not a stable public artefact
and uploading is the viewer's job. Everything you need to know whether
your script will be accepted by the official compiler — every undeclared
identifier, every type mismatch, every malformed state — is reported here.

This document covers:

1. [What works (language coverage)](#1--language-coverage)
2. [How to build](#2--how-to-build)
3. [How to use](#3--how-to-use)
4. [Diagnostic style](#4--diagnostic-style)
5. [Project layout](#5--project-layout)
6. [Test corpus](#6--test-corpus)
7. [Known limitations](#7--known-limitations)
8. [License & attribution](#8--license--attribution)

---

## 1 — Language coverage

`lslc` accepts the LSL Mono surface as documented on the
[LSL Wiki](https://wiki.secondlife.com/wiki/LSL_Portal). Concretely:

### Lexical structure

| Feature                                       | Supported |
|-----------------------------------------------|-----------|
| `//` line comments                            | ✓         |
| `/* */` block comments (incl. unterminated diagnostic) | ✓ |
| UTF-8 source with optional BOM                | ✓         |
| Decimal integer literals                      | ✓         |
| Hex integer literals (`0x…`)                  | ✓         |
| Float literals incl. `e`/`E` exponents and trailing `f` suffix | ✓ |
| Leading-dot floats (`.5`)                     | ✓         |
| String literals with `\n \t \" \\ \\n` escapes | ✓        |
| Multi-line string literals                    | ✓         |

### Types

`integer`, `float`, `string`, `key`, `vector`, `rotation` (alias
`quaternion`), `list`, plus the implicit `void` return type for
procedures.

Full implicit and explicit conversion rules of LSL are enforced:

| From → To | implicit | explicit |
|-----------|----------|----------|
| integer → float       | ✓ | ✓ |
| key ↔ string          | ✓ | ✓ |
| anything → string     | ✗ | ✓ |
| anything → list       | ✗ | ✓ (single-element wrap) |
| string → integer/float/vector/rotation/key | ✗ | ✓ |
| list → anything other than string/list | ✗ | ✗ |

### Operators (with correct LSL precedence)

`= += -= *= /= %=` ·
`|| &&` ·
`| ^ &` ·
`== !=` ·
`< > <= >=` ·
`<< >>` ·
`+ -` ·
`* / %` ·
unary `+ - ! ~ ++ --` ·
postfix `++ --` ·
member access (`v.x v.y v.z r.s`) ·
function call ·
`(TYPE) expr` cast.

The vector-cross product `%`, vector·rotation multiplication, and
rotation·rotation multiplication are recognised with their proper result
types.

> **Note.** Like the real LSL grammar, the ternary operator `?:` does
> **not** exist. `lslc` detects it explicitly and tells you to rewrite
> the conditional using `if`/`else`.

### Statements

`if / else if / else`, `while`, `do / while`, `for` (with comma-separated
`init` and `step` lists, all C-style), `return [expr];`, `jump label;`,
`@label;`, `state name;` (and `state default;`), block statements,
expression statements, the legacy `print(expr);`.

### Top-level forms

```lsl
TYPE  ident [= expr] ;         // global variable
TYPE? ident ( params? ) { … }  // function (return type optional)
default      { events … }      // mandatory default state
state name   { events … }      // additional states
```

The `default { … }` state is required; `lslc` reports its absence the same
way the SL uploader does.

### Events

All standard Mono event handlers are recognised with their canonical
signatures, including modern additions:

```
state_entry              state_exit
touch_start / touch / touch_end
collision_start / collision / collision_end
land_collision_start / land_collision / land_collision_end
listen   timer   sensor   no_sensor
on_rez   changed   attach   dataserver
email    http_request   http_response
link_message   money   moving_start   moving_end
not_at_target / at_target / not_at_rot_target / at_rot_target
object_rez   remote_data   run_time_permissions
transaction_result   experience_permissions
experience_permissions_denied   path_update
linkset_data (new LSD events)
final_damage / on_damage / on_death   (Combat 2)
game_control
```

Mismatched arity, mismatched parameter types, and typos in event names
are detected; the canonical signature is shown alongside the error.

### Built-in functions

The shipped table covers the documented LSL Mono library — including
inventory, prim-params, lists, strings, JSON, HTTP, money, HMAC/SHA,
sensors, physics, vehicles, particles, animations, link-set data,
experience tools, key-value store, pathfinding, and combat 2 hooks —
along with multiple-arity overloads where the LSL Wiki lists them.

A function not in the table but starting with `ll` is still **accepted**
under a `warning`, because Linden Lab continues to add new functions
faster than any third-party tool can keep up. Other unknown calls are
errors with a "did you mean?" suggestion. Run `lslc --list-builtins`
to see the full list compiled in.

### Built-in constants

All `TRUE / FALSE / NULL_KEY`, the math constants (`PI`, `TWO_PI`,
`DEG_TO_RAD`, …), status codes, link constants (`LINK_*`), attachment
points, permission masks (`PERMISSION_*`), changed flags (`CHANGED_*`),
HTTP option keys (`HTTP_*`), inventory types (`INVENTORY_*`), prim
parameter keys (`PRIM_*`), JSON markers, parcel/region flags, click
actions, agent info bits, particle keys, list-statistics codes, vehicle
types, content-type constants, and the channel sentinels (`PUBLIC_CHANNEL`,
`DEBUG_CHANNEL`).

### What is enforced semantically

* Every reference to an identifier must resolve (global, local, parameter,
  built-in constant) — otherwise an error with a `did you mean?` hint.
* `state X;` requires `X` to be a declared state (or `default`). Suggested
  alternatives are listed when the name is wrong.
* `state X;` may **only** appear in event handlers, never in a user
  function — a real LSL constraint that the SL uploader silently allows
  in some viewers and breaks at runtime.
* Functions check argument **count** and **types** against their
  declared signature; built-ins are checked the same way.
* `return` matches the function's declared type. Missing return values
  in non-void functions are errors. A `return value;` inside a void
  function or event handler is an error.
* `@label` and `jump label` are matched within their function/event
  scope; duplicate labels and unresolved jumps are detected with
  suggestions.
* All overloaded operator combinations of LSL are typed correctly —
  `vector * vector` (dot product, returns float), `vector % vector`
  (cross product, returns vector), `vector * rotation`,
  `rotation * rotation`, list concatenation with `+` for any element
  type, string concat (`string + string`), and rejection of
  meaningless combinations like `integer + string` (with the helpful
  suggestion to cast).
* Vector and rotation member accesses are typed to `float`; rotation has
  `.s`, vector does not.
* `?:` is rejected with a clear note that LSL has no ternary.

### What is warned about (under `-Wall`)

* Local variables that shadow globals, parameters, or built-in
  constants/functions.
* Unused globals and unused functions (functions whose name starts with
  `_` are exempt — a private-by-convention escape hatch).
* Calls to ll-prefixed functions absent from the built-in table.

---

## 2 — How to build

`lslc` is pure C99 with no third-party dependencies. It builds against
glibc, musl, BSD libc, macOS libSystem, and MSVCRT/UCRT. CMake or plain
`make` will produce the same binary; the source is small enough that
either approach takes a couple of seconds.

### Linux (Arch, Debian, Ubuntu, Fedora, NixOS, Alpine, …)

```sh
cd lslc
make            # produces ./lslc
sudo make install      # optional, installs to /usr/local/bin
```

On Arch Linux specifically:

```sh
pacman -Sy --needed base-devel              # gcc + make
cd lslc && make
```

The default `cc` is fine; you can override with `make CC=clang` or
`make CC=tcc`.

### macOS

```sh
xcode-select --install        # once, to get clang + make
cd lslc && make
```

### Windows

* **MinGW-w64 / MSYS2**: `cd lslc && mingw32-make` (or just `make`) —
  produces `lslc.exe`.
* **MSVC** via CMake:

  ```cmd
  cd lslc
  cmake -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ```

* The diagnostic ANSI colours are auto-detected; they will simply not be
  emitted when running under cmd.exe without a colour-capable terminal.

### Cross-platform via CMake

```sh
cd lslc
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build       # optional
```

---

## 3 — How to use

### Quick start

```sh
lslc my_script.lsl
```

* Exits 0 and prints `…: OK` on success.
* Exits 1 with detailed diagnostics on any error.
* Exits 2 only on internal errors (out of memory, file not readable).

### Multiple files

```sh
lslc src/*.lsl
```

Each file is compiled independently; the exit code is the worst result.

### Useful flags

| Flag                | Effect |
|---------------------|--------|
| `--mono`            | Target the Mono VM (default; 64 KiB script-memory budget). |
| `--lso`             | Target the legacy LSO VM (16 KiB budget; rejects Mono-only built-ins). |
| `--memory-budget=N` | Override the per-script memory budget in bytes. |
| `--no-memory-check` | Disable the source-size budget warning/error entirely. |
| `-Wall`             | Enable extra warnings (shadowing, unused locals/globals, unknown ll-functions). |
| `-Werror`           | Treat warnings as errors. Pair with `-Wall` for CI runs. |
| `-fsyntax-only`     | (Default; accepted for gcc parity.) |
| `-fno-color`        | Disable ANSI colour, regardless of TTY. |
| `-fmax-errors=N`    | Stop after N errors (default 50). |
| `-q`, `--quiet`     | Suppress the source-caret rendering and the "OK" line. |
| `--list-builtins`   | Dump every function / constant / event known to the compiler. |
| `-o FILE`           | Redirect diagnostics into FILE. |
| `--version`         | Print version and exit. |
| `-h`, `--help`      | Print the usage summary. |

### Memory-budget check

After reading the file `lslc` produces a rough bytecode-size proxy by
stripping comments and whitespace from the source and counting what
remains (string-literal bytes are preserved verbatim because they end
up in the bytecode constant table). That number is compared against
the per-script budget:

| Target    | Default budget | Threshold |
|-----------|----------------|-----------|
| Mono      | 65 536 bytes   | warning at 80%, error above 100% |
| LSO       | 16 384 bytes   | warning at 80%, error above 100% |

This is the same proxy the SL viewer's status bar uses; it is *not* the
final post-compile bytecode size, but it correlates closely enough to
catch the common case ("this script will not upload"). Pass
`--memory-budget=N` to override, or `--no-memory-check` to disable.

### LSO mode

`--lso` enables LSO-target rules:

* the memory budget drops from 64 KiB to 16 KiB,
* and any call to a built-in that was added after the LSO era is
  rejected as an error.

The flagged Mono-only families are:

* `llJsonGetValue`, `llJsonSetValue`, `llJsonValueType`, `llJson2List`,
  `llList2Json` (JSON helpers — added when LL shipped Mono);
* the entire `llLinksetData*` family (Linkset Data Store);
* `llCastRay` (ray-casting added with the modern havok-physics work);
* `llChar`, `llOrd` (Unicode helpers);
* `llHMAC` (HMAC helpers);
* `llTransferLindenDollars` (Mono-only money API; `llGiveMoney` is the
  LSO-compatible alternative).

If you need to support more LSO-incompatible built-ins, mark them with
`BFN_MONO_ONLY` by switching their entry in `src/builtins.c` from
`FN…` to `MN…` (e.g. `MN2(...)`); the diagnostic surfaces automatically.

### Editor / CI integration

The diagnostic format is gcc-compatible:

```
path/to/file.lsl:42:13: error: cannot assign integer to string
   42 |         string s = 42;
      |                    ^~
  help: an explicit cast is required:  (string)expr
```

Vim's `:make` and `:cnext`, Emacs's `M-x compile`, Sublime Text's "Find
Results", VS Code's problem matcher, and any pretty-much-anything-else
already parse this format.

### Use from a Makefile

```make
LSLC ?= lslc
LSLFLAGS ?= -Wall -Werror

check: $(LSL_SOURCES)
	$(LSLC) $(LSLFLAGS) $^
```

---

## 4 — Diagnostic style

`lslc` was written specifically because the in-viewer LSL compiler's
messages are terse and the line/column positions can be misleading.
Every diagnostic carries:

* a `path:line:col:` prefix and `error|warning|note|help` label,
* the offending source line, with the bad token highlighted by a caret
  and tildes,
* one or more **help** lines that explain *why* it is wrong and *how*
  to fix it,
* and where applicable, a `did you mean` suggestion built from
  Levenshtein-distance ranking against in-scope identifiers, built-in
  functions/constants, and known event/state names.

Example — a real bug found in one of the production sample scripts:

```
sakura_payroll.lsl:326:25: error: argument 4 of built-in 'llDialog' has
type key, but it expects integer
  326 |                         llGetScriptID()
      |                         ^~~~~~~~~~~~~~~
  help: if the conversion is intended, write it explicitly: (integer)expr
```

(That code was passing the script's UUID as the dialog channel, which
LSL Mono would silently accept at runtime but never deliver listens to.
`lslc` catches it at compile time.)

Some examples of the extra detail you get over the SL uploader:

* **typo in event** → `did you mean event 'state_entry'?`
* **typo in state** → lists every defined state name
* **typo in function** → suggestion from both built-ins and user
  functions
* **wrong event signature** → prints the canonical one alongside
* **`?:` ternary** → "rewrite the conditional expression using if/else
  statements"
* **`(integer)[…]`** → "lists can only be cast to (string) — use
  llList2Integer to extract a typed value"

---

## 5 — Project layout

```
sakura-lslc/
├── README.md             quick-start overview
├── DOCUMENTATION.md      this file — complete reference
├── LICENSE               MIT
├── .gitignore
├── CMakeLists.txt        Windows / MSVC / cross-platform
├── Makefile              Linux / macOS / *BSD / MinGW
├── src/                  C99 source
│   ├── lsl.h             shared types and prototypes
│   ├── ast.c             AST node allocation / destruction
│   ├── builtins.c        tables of built-in functions / consts / events
│   ├── diag.c            gcc-style diagnostic engine
│   ├── fold.c            compile-time constant folder
│   ├── lexer.c           tokenizer + #line marker handling
│   ├── main.c            driver, command-line interface
│   ├── parser.c          recursive-descent parser
│   ├── preproc.c         C-style preprocessor (#include / #define / …)
│   ├── sem.c             name resolution + type checking
│   ├── symtab.c          lexical scope and symbol table
│   ├── types.c           LSL type system rules
│   └── util.c            memory + file + edit-distance helpers
└── tests/
    ├── pass/             scripts that must compile cleanly
    ├── fail/             scripts that must be rejected
    └── run_tests.sh      portable POSIX test harness
```

There are no external libraries, no generated files, and no
platform-specific code beyond a single `_POSIX_C_SOURCE` define to
surface `isatty(3)` and `fileno(3)`. The whole tree is ~5 000 lines.

---

## 6 — Test corpus

The bundled corpus in `tests/` exercises the language end-to-end.
It is split into:

* `pass/` — 13 scripts that demonstrate every supported construct
  (types, control-flow, every operator, user functions, events,
  string/list/JSON helpers, state transitions, jumps with labels,
  comments and string escapes, constant-folded globals, and
  `#define` / `#include` preprocessor use).
* `fail/` — 17 scripts that each provoke a specific class of
  diagnostic (undeclared identifier, type mismatch, wrong arity,
  missing default state, return-type mismatch, state in user function,
  wrong vector/rotation member, wrong event signature, attempted
  ternary, redeclaration, undefined state target, unresolved jump,
  illegal cast, syntax errors, non-constant global initialiser,
  missing #include, #error directive).

Run the suite:

```sh
cd lslc
make            # builds ./lslc
make test       # runs tests/run_tests.sh against ./lslc
```

Expected output:

```
== positive tests ==
  PASS  01_minimal.lsl
  PASS  02_all_types.lsl
  …
== negative tests ==
  PASS  01_undeclared.lsl (rejected as expected)
  …
Result: 30 passing / 30 tests
```

The 14 production scripts in `scripts/release/*.lsl` (including the
legacy `.lsl.txt` one) also compile through `lslc`; in fact two of them
turned out to have real bugs that `lslc` flags and the in-viewer
compiler has been quietly accepting:

* `sakura_investment_kiosk.lsl` — declares a *listen handle* as `key`
  but `llListen` returns `integer`, so the handle would never round-trip
  to `llListenRemove`.
* `sakura_payroll.lsl` — passes `llGetScriptID()` (a `key`) as the
  channel number argument to `llDialog`, which the in-viewer compiler
  accepts via runtime coercion but which silently breaks listen
  delivery.

Run them against the compiler yourself:

```sh
./lslc /path/to/scripts/release/sakura_payroll.lsl
```

---

## 7 — Built-ins, constants, folding & preprocessor

### Built-in coverage

The built-in table currently registers **429 unique LSL Mono functions**
(matching the LSL Wiki function count), plus their multi-arity overloads,
and **530+ constants** (every documented `PRIM_*`, `STATUS_*`, `CHANGED_*`,
`PERMISSION_*`, `VEHICLE_*`, `INVENTORY_*`, `ESTATE_*`, `XP_ERROR_*`,
`PU_*`, `RC_*`, `LINKSETDATA_*`, `PARCEL_*`, `JSON_*`, attachment-point
enum, content-type enum, status-codes, debug channels, math constants,
…). All 50+ event handlers are recognised with their canonical
signatures.

Unknown `ll`-prefixed names produce a one-line *warning* (so that LL
shipping a new function after a release doesn't break your build) but
known names get full type-checking. To add a future function, just
append one `FN…(name, return, args…)` line in `src/builtins.c`.

### Constant folding

Global initialisers are folded at compile time:

```lsl
integer N    = 60 * 60 * 24;           // folded to 86400
float   AREA = 3.14 * 5.0 * 5.0;       // folded to ~78.5
string  G    = "hello, " + "world";    // folded to "hello, world"
integer M    = (1 << 4) | (1 << 2);    // folded to 20
```

If folding succeeds the result is the same literal the SL uploader
produces. If a global initialiser **cannot** be reduced to a constant —
because it calls a function or names another variable — `lslc` rejects
it with the same error the in-viewer compiler does:

```
key owner = llGetOwner();
     ^~~~~~~~~~~~
error: global 'owner' must be initialised with a constant expression
help: move the call into state_entry() and assign to 'owner' there
```

Folding currently understands integer / float / string / cast /
operator / `defined()` constants and per-component member access on
vector / rotation literals; runtime-only expressions are left alone.

### Preprocessor

`lslc` includes a full C-style preprocessor by default, compatible with
Firestorm's LSL preprocessor and the OpenSim `lslc` macro tool:

| Directive       | Behaviour |
|-----------------|-----------|
| `#include "p"`  | Inline file `p`, searched first in the including file's directory, then in every `-I` path. |
| `#include <p>`  | System-style include; searched only along `-I` paths. |
| `#define N V`   | Object-like macro. |
| `#define N(a,b) body` | Function-like macro with parameter substitution. |
| `#undef N`      | Remove a macro. |
| `#ifdef N`      | Take the branch if `N` is defined. |
| `#ifndef N`     | Take the branch if `N` is not defined. |
| `#if expr`      | Evaluate `defined(N)` / integer constants / `! && ||`. |
| `#elif expr`    | Conditional else-if chain. |
| `#else`         | Else branch. |
| `#endif`        | Close the most recent conditional. |
| `#error msg`    | Emit a compile error from the preprocessor. |
| `#warning msg`  | Emit a compile warning. |
| `#pragma …`     | Passed through (ignored). |
| `#line N "file"`| Internal — emitted by the preprocessor so the lexer can show diagnostics at the original include site. |

Driver flags:

| Flag              | Behaviour |
|-------------------|-----------|
| `-I PATH`         | Add `PATH` to the include search list (repeatable). |
| `-D NAME[=value]` | Pre-define a macro (repeatable). |
| `-E`              | Print the preprocessed text and exit, like `gcc -E`. |
| `--no-preprocess` | Skip the preprocessor entirely. |

Diagnostics inside `#include`d files point at the right path and line —
the preprocessor emits `#line` markers that the lexer honours, and file
paths are interned so they stay valid for the whole compilation.

Example:

```lsl
// main.lsl
#define API_BASE "https://example.com/api/v1"
#include "common.lslh"
#ifdef DEBUG
log_msg(string m) { llOwnerSay("[debug] " + m); }
#else
log_msg(string m) { }
#endif

default { state_entry() { log_msg(API_BASE); } }
```

Build with `lslc -DDEBUG -I shared/ main.lsl`.

---

## 8 — What's still out of scope

* `lslc` is a **front-end**: it does not emit Mono bytecode or upload
  to a region. The SL viewer / region-side tool still does the final
  upload. We've considered shipping a small headless runtime that walks
  the typed AST, models the event loop, mocks `ll*` side effects,
  routes `llMessageLinked` / `llRegionSay` / `llListen` across
  scripts, and pretends to be a region — that's a tractable follow-on
  project ("lsl-sim") rather than part of this compiler.
* The compiler reads files as UTF-8. Other encodings should be
  converted first (`iconv -f WINDOWS-1252 -t UTF-8`).

---

## 9 — License & attribution

`sakura-lslc` is © 2026 **Sakura Studios, IKE**, authored and maintained
by **Shiho Sakura** ([@ShihoSakura](https://github.com/ShihoSakura)),
and distributed under the **MIT License** — see the
[`LICENSE`](./LICENSE) file at the project root for the full text.

The LSL language itself, the built-in function set, the constants, and
the event signatures are © Linden Research, Inc. The tables in
`src/builtins.c` are factual data, not creative content, and no
ownership of the LSL surface is claimed.

`sakura-lslc` is the first half of Sakura Studios' open-source LSL
toolchain. The companion project, **`sakura-lsl-sim`** (forthcoming),
will be a headless region emulator that walks the typed AST produced
here, models the LSL event loop, mocks `ll*` side effects, and routes
`llMessageLinked` / `llRegionSay` / `llListen` between scripts — so
entire LSL systems can be integration-tested from the command line.

Bug reports, test-corpus additions, and built-in-table updates very
welcome — the architecture was deliberately kept simple so that
*any* LSL developer can add new entries by editing one file. Open an
issue or PR on [github.com/ShihoSakura/sakura-lslc](https://github.com/ShihoSakura/sakura-lslc).
