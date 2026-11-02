# cspcc (CSP compiler)

**cspcc** translates programs written in Communicating Sequential Processes, the
language of C. A. R. Hoare's 1978 paper, into Ada. GNAT then builds them into
programs that run every process as an Ada task. The input is the CSP of the
lecture slides, with their symbols (`→`, `□`, `‖`) or their ASCII spellings
(`->`, `[]` or `|`, `||`), so an exercise can be checked by running it.

This CSP is not the later process algebra, and not Go's channels. Processes
are named directly (`Fork(i)!in()`), messages are matched by their constructor,
guards end with an input and never an output, and a repetitive command ends on
its own once the processes its inputs name have terminated.

```
[ Phil(i:0..4)::PHILOSOPHER || Fork(i:0..4)::FORK || Room::ROOM ]

ROOM:: [ v: integer; v := 4;
  *[ v > 0; (i:0..4) Phil(i)?ticket() → v := v - 1
   □ (i:0..4) Phil(i)?back() → v := v + 1 ] ]
...
```

```sh
cspcc philosophers.csp          # writes philosophers.adb and the runtime
gnatmake philosophers.adb
./philosophers --trace
```

## Building

Build on Linux with `g++` (C++20), `make`, and GNAT for running the
translated programs.

```sh
make            # builds ./cspcc
make test       # runs the CSP tests in tests/
make examples   # translates and builds each program in examples/
make clean
```

## Using cspcc

```
cspcc [options] INPUT
```

| Option | Meaning |
| --- | --- |
| `-o OUTPUT` | Write the Ada main procedure to `OUTPUT`, a lower-case unit name ending in `.adb` (default: the input's name with `.adb`, in the current directory) |
| `-D NAME=VALUE` | Define the constant `NAME`, or override its `const` |
| `--no-runtime` | Don't write `csp_runtime.ads` and `csp_runtime.adb` next to the output |
| `--dump-tokens` | Print the tokens and stop |
| `-w`, `-Werror` | Report no warnings, or report them as errors |

A translated program takes these options:

| Option | Meaning |
| --- | --- |
| `--trace` | Print every communication, `Phil(1) -> Fork(1): in()`, and the end of every hook |
| `--quiet` | Don't print hooks |
| `--delay=MS` | Pause each hook a random time up to `MS` milliseconds (default 10) |
| `--seed=N` | Seed the random choices among true guards and the pauses |
| `--timeout=S` | Stop after `S` seconds with status 0, for programs that run forever |

It exits with status 1 when a process fails, such as an alternative command
whose guards are all false or an output to a terminated process, and with
status 3 when every process is waiting, which it reports as a deadlock with
what each process waits for.

## The language

| CSP | Meaning |
| --- | --- |
| `[ A::... ‖ B(i:1..n)::... ]` | Processes and arrays of processes, also with two subscripts, `M(i:1..3, j:1..3)` |
| `X:: commands` | A process defined at the top level, like the slides' `X::*[...]` |
| `NAME:: commands` | A macro, when a command uses `NAME`; a body in brackets is spliced, so its names stay visible |
| `eat()`, `print(x)` | A hook, a command nothing defines: it prints the process and the call, then pauses |
| `const N = 5` | A constant, which `-D` can override |
| `x, y: integer`, `a: (0..9) real` | Declarations of `integer`, `real`, `character` and `boolean`, and arrays |
| `x := e`, `(x, y) := (y, x)` | Assignments |
| `X!e`, `X(i)?v` | Output and input of a value |
| `S!insert(n)`, `S?has(n)`, `sem!P()` | Messages with a constructor, and signals |
| `X!(a, b)`, `X?(x, y)` | Tuples |
| `[ g1 → c1 □ g2 → c2 ]`, `[ g1 -> c1 [] g2 -> c2 ]` | Alternative command, with `□`, `[]` or `\|` between guarded commands |
| `*[ g1 → c1 □ g2 → c2 ]`, `*[ commands ]` | Repetitive command, and a loop without a guard |
| `b; x: real; (i:1..n) X(i)?m → c` | A guard: conditions, declarations, ranges, and a last input |

## How the translation works

Each family of processes becomes an Ada task type, and each kind of message a
family receives becomes an entry family indexed by the sender's number, so
`Fork(i)?in()` accepts only from `Fork(i)`'s sender. An output is an entry
call; an input is an accept. An alternative command evaluates its guards,
unrolls their ranges into select alternatives, and polls the sources of its
inputs so that a source's termination closes its guards, which is how a
repetitive command ends. The runtime package `Csp_Runtime` names the
processes, prints traces and hooks, picks among true guards at random, and
watches for deadlocks.

## Layout

| Path | Contents |
| --- | --- |
| `include/cspcc/`, `src/cspcc/` | The compiler: lexer, parser, sema, Ada emitter and driver |
| `include/cspcc/data/` | Tokens, the syntax tree and the checked program |
| `examples/` | The slides' programs, made whole |
| `tests/lang/`, `tests/errs/`, `tests/examples/` | CSP tests, their claims in `-- (Test)` lines read by `tests/run_test` |

## License

cspcc is free software under the GNU General Public License, version 3 or
later; see [LICENSE.md](LICENSE.md).
