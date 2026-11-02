# cspcc
A compiler from Hoare's CSP (1978) to Ada. Each process becomes an Ada task, so a
CSP exercise can be checked by running it.

# Example
```
-- readers_writers.csp

NR :: 3
NW :: 2

[
    (i:1..NR) Reader(i) :: READER
 || (i:1..NW) Writer(i) :: WRITER
 || rw :: RW
]

READER :: *[
    true -> [
        rw!Start_Read();
        READING;
        rw!Stop_Read();
    ]
]

WRITER :: *[
    true -> [
        rw!Start_Write();
        WRITING;
        rw!Stop_Write();
    ]
]

RW :: [
    rCount : Integer;
    wCount : Integer;
    rCount := 0;
    wCount := 0;
    *[
            wCount = 0, (i:1..NR) Reader(i)?Start_Read() -> [
                rCount := rCount + 1;
            ]
        []  wCount = 0, rCount = 0, (i:1..NW) Writer(i)?Start_Write() -> [
                wCount := wCount + 1;
            ]
        []  (i:1..NR) Reader(i)?Stop_Read() -> [
                rCount := rCount - 1;
            ]
        []  (i:1..NW) Writer(i)?Stop_Write() -> [
                wCount := wCount - 1;
            ]
    ]
]
```

```sh
make                                   # builds ./cspcc
./cspcc examples/readers_writers.csp   # writes readers_writers.adb and csp_runtime.ad[sb]
gnatmake readers_writers.adb
./readers_writers --trace --timeout=2
```

`READING` and `WRITING` are hooks: a command nothing defines prints the process
and its name, then pauses for a moment.

# Building
You need `g++` (C++20) and `make`; running a translated program needs GNAT.

```sh
make            # ./cspcc
make test       # the CSP tests in tests/
make examples   # each examples/NAME.csp built into examples/NAME
make clean
```

# Options
```
cspcc [options] INPUT
  -o OUTPUT         the Ada main procedure, a lower-case name ending in .adb
  -D NAME=VALUE     define a constant, or override one
  --no-runtime      don't write csp_runtime.ads and csp_runtime.adb
  --dump-tokens     print the tokens and stop
  -w, -Werror       no warnings, or warnings as errors
```

A translated program takes `--trace` (print every message), `--quiet` (don't
print hooks), `--delay=MS` (the longest pause of a hook, default 10),
`--seed=N` and `--timeout=S` (stop after S seconds). It exits with 1 when a
process fails, such as an alternative command with no true guard, and with 3
on a deadlock, listing what each process waits for.

# The language
| CSP | Meaning |
| --- | --- |
| `[ A :: P \|\| (i:1..N) B(i) :: Q ]` | Processes and arrays of processes; also `B(i:1..N)`, `B(1..N)` and `(i:1..3, j:1..3) M(i, j)` |
| `NAME :: commands` | A macro, used as the command `NAME` |
| `N :: 10`, `const N = 10` | A constant, which `-D` overrides |
| `x, y : Integer`, `a : (0..9) Real` | Variables of `Integer`, `Real`, `Character`, `Boolean`, and arrays |
| `x := e`, `(x, y) := (y, x)` | Assignment |
| `X!e`, `X(i)?v`, `S!insert(n)`, `rw!Start_Read()` | Output and input of values, messages and signals |
| `[ b1 -> c1 [] b2 -> c2 ]` | Alternative command; also `□` or `\|` for `[]`, and `→` for `->` |
| `*[ b1 -> c1 [] b2 -> c2 ]` | Repetitive command, which ends when no guard can be true |
| `b, (i:1..N) X(i)?m() -> [ ... ]` | A guard: conditions, declarations and ranges, separated by `,` or `;`, and an input last |
| `-- ...`, `// ...`, `/* ... */` | Comments |

# License
GPLv3 or later; see [LICENSE.md](LICENSE.md).
