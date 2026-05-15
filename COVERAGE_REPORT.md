# sakura-lslc Coverage Report

## Goal & Methodology

This report accompanies the corpus in `tests/coverage/`. The corpus exercises
every LSL Mono language construct that `sakura-lslc` currently implements,
plus negative tests for every diagnostic the compiler is expected to surface.

Methodology:

1. Enumerated the surface language by reading `src/types.c`,
   `src/builtins.c` (the `BI_FN` and `BI_EVENT` tables),
   `src/parser.c`, `src/sem.c`, `src/preproc.c`, and `src/main.c`.
2. Wrote one focused, self-contained `.lsl` (or `.lslh`) file per construct.
3. Encoded the expected outcome in each file with a header comment:
   - `// Expected: PASS` or `// Expected: FAIL`
   - `// FLAGS: <extra lslc flags>` for tests that need a non-default invocation
   - `// EXPECT: <substring>` for substrings that must appear in the
     diagnostic output (asserts diagnostic quality, not just rc).
4. Built a single driver, `tests/coverage/run_coverage.sh`, that:
   - Compiles every `pass/*.lsl`, asserts rc=0, and verifies all
     `// EXPECT:` substrings appear in `stdout+stderr`.
   - Compiles every `fail/*.lsl`, asserts rc≠0, and verifies all
     `// EXPECT:` substrings appear.
   - Prints a green / red bar summary; exits 0 iff every test matched.

**Result: 82 / 82 tests passing (71 positive + 11 negative).**

The corpus is additive — `tests/pass/` and `tests/fail/` (the original
30-test suite under `tests/run_tests.sh`) continue to pass unmodified.

## Coverage Matrix

### Types (`pass/types_*.lsl` + `fail/types_mismatch_*.lsl`)

| Feature                                  | Test file                                          | Outcome | Notes                                          |
| ---------------------------------------- | -------------------------------------------------- | ------- | ---------------------------------------------- |
| `integer` decl, literal, casts           | `pass/types_integer.lsl`                           | PASS    | dec / hex / negative literals; in/out casts    |
| `float` decl, literal, casts             | `pass/types_float.lsl`                             | PASS    | `3.14`, `1.0e2`, `.5`, neg; in/out             |
| `string` decl, escapes, casts            | `pass/types_string.lsl`                            | PASS    | `\"`, `\n`; cast to int/float/key/vec/rot/list |
| `key` decl, NULL_KEY, key↔string         | `pass/types_key.lsl`                               | PASS    | implicit and explicit                          |
| `vector` decl, member access, casts      | `pass/types_vector.lsl`                            | PASS    | `.x .y .z`; vec↔string; vec→list               |
| `rotation` decl, .x .y .z .s, casts      | `pass/types_rotation.lsl`                          | PASS    | all four members; rot↔string                   |
| `quaternion` alias = `rotation`          | `pass/types_quaternion_alias.lsl`                  | PASS    | both directions are implicit-assignable        |
| `list` decl, mixed literal, casts        | `pass/types_list.lsl`                              | PASS    | every typed value casts to list                |
| string→integer mismatch                  | `fail/types_mismatch_string_to_integer.lsl`        | FAIL    | "cannot initialize integer with string"        |
| float→integer mismatch (no implicit)     | `fail/types_mismatch_float_to_integer.lsl`         | FAIL    | "cannot initialize integer with float"         |
| vector→rotation mismatch                 | `fail/types_mismatch_vector_to_rotation.lsl`       | FAIL    | "cannot initialize rotation with vector"       |
| list→string mismatch (no implicit)       | `fail/types_mismatch_list_to_string.lsl`           | FAIL    | "cannot initialize string with list"           |
| integer→vector (not castable at all)     | `fail/types_mismatch_integer_to_vector.lsl`        | FAIL    | "cannot initialize vector with integer"        |

### Operators (`pass/op_*.lsl` + `fail/op_*.lsl`)

| Feature                                            | Test file                              | Outcome | Notes                                                |
| -------------------------------------------------- | -------------------------------------- | ------- | ---------------------------------------------------- |
| Numeric arithmetic (int+int, int+float, float+f)   | `pass/op_arithmetic_numeric.lsl`       | PASS    | `+ - * / %` over int / float / mixed                 |
| Vector arithmetic (sum, diff, neg, dot, cross)     | `pass/op_arithmetic_vector.lsl`        | PASS    | `v+v v-v v*f f*v v/f v*v v%v`                        |
| Rotation arithmetic                                | `pass/op_arithmetic_rotation.lsl`      | PASS    | `r+r r-r r*r r/r v*r v/r`                            |
| String concat                                      | `pass/op_arithmetic_string.lsl`        | PASS    | `+` and `+=`                                         |
| `list + anything`                                  | `pass/op_arithmetic_list.lsl`          | PASS    | every typed value can be appended/prepended          |
| Comparison `== != < > <= >=`                       | `pass/op_comparison.lsl`               | PASS    | int/float/mixed/string/key/vec/rot/list              |
| Logical `&& \|\| !`                                | `pass/op_logical.lsl`                  | PASS    | combinations including `!!`                          |
| Bitwise `& \| ^ ~ << >>` (integer-only)            | `pass/op_bitwise.lsl`                  | PASS    | compound bitwise (`&=`) NOT implemented — see gaps  |
| Compound assign `+= -= *= /= %=`                   | `pass/op_compound_assign.lsl`          | PASS    | int / float / vector / string / list                 |
| Prefix and postfix `++ / --`                       | `pass/op_inc_dec.lsl`                  | PASS    | including in for-loop step                            |
| Vector/rotation member access (LHS and RHS)        | `pass/op_member_access.lsl`            | PASS    | `v.x = ...`, `v.y += ...`, `r.s = ...`               |
| Bitwise on float (rejected)                        | `fail/op_bitwise_on_float.lsl`         | FAIL    | "operator '&' is not defined for ... float"          |
| Shift on string (rejected)                         | `fail/op_shift_on_string.lsl`          | FAIL    | "operator '<<' is not defined for ... string"        |

### Control flow (`pass/ctl_*.lsl` + `fail/ctl_*.lsl`)

| Feature                                  | Test file                              | Outcome |
| ---------------------------------------- | -------------------------------------- | ------- |
| `if / else if / else` nested             | `pass/ctl_if_else.lsl`                 | PASS    |
| `while`                                  | `pass/ctl_while.lsl`                   | PASS    |
| `do / while`                             | `pass/ctl_do_while.lsl`                | PASS    |
| `for` with comma-init/step               | `pass/ctl_for.lsl`                     | PASS    |
| `jump` + `@label` (forward and back)     | `pass/ctl_jump_label.lsl`              | PASS    |
| `return` with/without value, fn + event  | `pass/ctl_return.lsl`                  | PASS    |
| `state X;` inside an event handler       | `pass/ctl_state_in_event.lsl`          | PASS    |
| `state X;` inside a function (rejected)  | `fail/ctl_state_in_function.lsl`       | FAIL    |

### Top-level forms (`pass/top_*.lsl`)

| Feature                                      | Test file                  | Outcome |
| -------------------------------------------- | -------------------------- | ------- |
| Globals of every type, uninitialised globals | `pass/top_globals.lsl`     | PASS    |
| Functions: void / every return type / recursion | `pass/top_functions.lsl` | PASS    |
| Default state + multiple named states        | `pass/top_states.lsl`      | PASS    |

### Events (`pass/ev_*.lsl`)

Every canonical signature from `BI_EVENT[]` in `src/builtins.c` is exercised
exactly once. Each test calls (i.e., uses or stringifies) the params declared
for that signature, where applicable.

| Event                                                       | Test file                              |
| ----------------------------------------------------------- | -------------------------------------- |
| `state_entry()`                                             | `pass/ev_state_entry.lsl`              |
| `state_exit()`                                              | `pass/ev_state_exit.lsl`               |
| `touch_start / touch / touch_end` (integer total_number)    | `pass/ev_touch.lsl`                    |
| `collision_start / collision / collision_end`               | `pass/ev_collision.lsl`                |
| `land_collision_start / _ / _end` (vector position)         | `pass/ev_land_collision.lsl`           |
| `listen(integer, string, key, string)`                      | `pass/ev_listen.lsl`                   |
| `timer()`                                                   | `pass/ev_timer.lsl`                    |
| `sensor(integer) / no_sensor()`                             | `pass/ev_sensor.lsl`                   |
| `on_rez(integer start_param)`                               | `pass/ev_on_rez.lsl`                   |
| `changed(integer change)`                                   | `pass/ev_changed.lsl`                  |
| `attach(key id)`                                            | `pass/ev_attach.lsl`                   |
| `dataserver(key, string)`                                   | `pass/ev_dataserver.lsl`               |
| `email(string,string,string,string,integer)` (5-arg variant)| `pass/ev_email.lsl`                    |
| `http_response(key, integer, list, string)`                 | `pass/ev_http_response.lsl`            |
| `http_request(key, string, string)` (3-arg variant)         | `pass/ev_http_request.lsl`             |
| `link_message(integer, integer, string, key)`               | `pass/ev_link_message.lsl`             |
| `money(key, integer)`                                       | `pass/ev_money.lsl`                    |
| `moving_start() / moving_end()`                             | `pass/ev_moving.lsl`                   |
| `at_rot_target / not_at_rot_target`                         | `pass/ev_at_rot_target.lsl`            |
| `at_target / not_at_target`                                 | `pass/ev_at_target.lsl`                |
| `object_rez(key id)` (1-arg variant)                        | `pass/ev_object_rez.lsl`               |
| `remote_data(integer, key, key)`                            | `pass/ev_remote_data.lsl`              |
| `run_time_permissions(integer)`                             | `pass/ev_run_time_permissions.lsl`     |
| `transaction_result(key, integer, string)`                  | `pass/ev_transaction_result.lsl`       |
| `experience_permissions / _denied`                          | `pass/ev_experience_permissions.lsl`   |
| `path_update(integer, list)`                                | `pass/ev_path_update.lsl`              |
| `linkset_data(integer, string, string)` (3-arg variant)     | `pass/ev_linkset_data.lsl`             |
| `final_damage / on_damage / on_death`                       | `pass/ev_damage.lsl`                   |
| `game_control(key, integer)`                                | `pass/ev_game_control.lsl`             |

### Preprocessor (`pass/pp_*.lsl` + `fail/pp_*.lsl` + `include/helper.lslh`)

| Feature                                            | Test file                      | Outcome |
| -------------------------------------------------- | ------------------------------ | ------- |
| `#define` object-like                              | `pass/pp_define_obj.lsl`       | PASS    |
| `#define` function-like with args                  | `pass/pp_define_fn.lsl`        | PASS    |
| `#undef` then redefine                             | `pass/pp_undef_redef.lsl`      | PASS    |
| `#ifdef / #else / #endif` (both branches taken)    | `pass/pp_ifdef.lsl`            | PASS    |
| `#ifndef`                                          | `pass/pp_ifndef.lsl`           | PASS    |
| `#if defined() / #elif / #else`                    | `pass/pp_if_elif_else.lsl`     | PASS    |
| `#include "..."` (relative path)                   | `pass/pp_include.lsl`          | PASS    |
| `#error` inside skipped branch must NOT fire       | `pass/pp_error_skipped.lsl`    | PASS    |
| `#error` inside taken branch fires                 | `fail/pp_error_taken.lsl`      | FAIL    |
| `#warning` under -Wall (warning emitted, rc=0)     | `pass/pp_warning_wall.lsl`     | PASS    |

### Constant folding (`pass/const_folding.lsl`)

| Feature                                            | Test file                  | Outcome |
| -------------------------------------------------- | -------------------------- | ------- |
| Literal arithmetic in global initialiser           | `pass/const_folding.lsl`   | PASS    |
| String concatenation in global init                | `pass/const_folding.lsl`   | PASS    |
| Vector-member access on a vector literal           | `pass/const_folding.lsl`   | PASS    |
| Cast of a literal                                  | `pass/const_folding.lsl`   | PASS    |
| Built-in constant (`PI`) inside global init        | `pass/const_folding.lsl`   | PASS    |

### Memory budget (`pass/mem_within_budget.lsl` + `fail/mem_exceeds_budget.lsl`)

| Feature                                            | Test file                              | Outcome |
| -------------------------------------------------- | -------------------------------------- | ------- |
| ~54 KB script: warns at 80% but compiles           | `pass/mem_within_budget.lsl`           | PASS    |
| ~75 KB script: exceeds 64 KiB Mono budget          | `fail/mem_exceeds_budget.lsl`          | FAIL    |

### LSO mode (`pass/lso_json_default.lsl` + `fail/lso_json_in_lso.lsl`)

| Feature                                            | Test file                          | Flags     | Outcome |
| -------------------------------------------------- | ---------------------------------- | --------- | ------- |
| `llJsonGetValue` under default (Mono) target       | `pass/lso_json_default.lsl`        | (none)    | PASS    |
| `llJsonGetValue` under `--lso` (Mono-only error)   | `fail/lso_json_in_lso.lsl`         | `--lso`   | FAIL    |

### Diagnostics quality (`pass/diag_wall_unused.lsl`)

| Feature                                            | Test file                          | Flags     | Outcome |
| -------------------------------------------------- | ---------------------------------- | --------- | ------- |
| `-Wall` surfaces unused-global warning             | `pass/diag_wall_unused.lsl`        | `-Wall`   | PASS    |

(Numerous additional diagnostics — "operator X not defined for type Y",
"cannot initialize T with U", "#error: ...", "is a Mono-only built-in",
"'state' change is only allowed inside event handlers" — are asserted by the
`// EXPECT:` lines in their corresponding `fail/` tests above.)

## Gap Analysis

These items appear in real-world LSL, but **deliberately are not** in this
corpus because `sakura-lslc` does not implement them:

- **Compound bitwise assignment** (`&= |= ^= <<= >>=`). The parser rejects
  these at the expression-statement boundary; `op_bitwise.lsl` works around
  it with `c = c & X`. Adding these would be a one-line lexer/parser change.
- **Multi-declarator declarations** (`integer i, j;`). The parser only
  accepts one variable per declaration. `ctl_for.lsl` uses two separate
  `integer` declarations for what would normally be `integer i, j;`.
- **Ternary `?:`** — not in LSL by design, already covered by
  `tests/fail/09_ternary.lsl` in the original suite.
- **`do { } while` without the trailing semicolon** — not tested; trailing-
  semicolon insensitivity is not a documented LSL feature.
- **Stringification (`#x`) and token-pasting (`##`)** in preprocessor macros
  — `src/preproc.c` does plain identifier-substitution only.
- **Variadic macros** (`__VA_ARGS__`) — not implemented.
- **`#pragma` / `#line`** — currently a pass-through (per `preproc.c:570`);
  not asserted because there is no observable behavioural change to test.
- **`-c` / SLBC emission** — outside the scope of the coverage corpus; the
  bytecode emitter (`emit.c`) is exercised separately.
- **`integer foo()` followed by `;`** (function forward declaration) —
  not part of LSL; functions must be defined whole.
- **`++` / `--` on non-integers** — `op_inc_dec.lsl` only tests on integers;
  behaviour on float/vector via member access (`++v.x`) is unverified.
- **Per-event arity variants** — `BI_EVENT` lists multiple arities for
  `object_rez`, `email`, `http_request`, `linkset_data`,
  `experience_permissions`, and `at_rot_target`. The corpus exercises the
  canonical (most-common) variant for each; the alternative arity is
  available via the same lookup table and would need only a copy-paste
  test if you want it explicitly covered.
- **Every built-in function call** (`BI_FN` is 540 entries). The corpus
  uses ~30 built-ins incidentally; full per-built-in coverage is a
  separate effort.
- **State-change inside a `for`/`while`/`do` body in a function** —
  unverified; only the top-of-function form is rejected.
- **`-Werror`** — not asserted; `-Wall` alone keeps the rc semantics
  predictable.

## How to extend the corpus

Drop a new file into `tests/coverage/pass/` (if it should compile with rc=0)
or `tests/coverage/fail/` (if it should fail). Put a `// Feature:` and
`// Expected:` line at the top so it's self-describing. If the file needs
non-default `lslc` flags, add a `// FLAGS: …` line. To assert that a
specific diagnostic appears, add one or more `// EXPECT: <substring>` lines
— each is treated as a substring search against the combined stdout+stderr,
so be specific enough to be unambiguous but loose enough to survive cosmetic
changes (e.g., assert `"cannot initialize integer with float"`, not the
whole formatted block). Re-run `./tests/coverage/run_coverage.sh ./lslc` and
the runner will pick the new file up automatically.
