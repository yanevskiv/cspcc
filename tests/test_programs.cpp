// Tests that translate, build and run small CSP programs.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

TEST_CASE("skip does nothing, and a hook prints its process and its call", "[programs]")
{
    const char *source = R"csp(
X:: [ skip; done() ]
)csp";
    Run result = run("skip", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: done()",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("Integer arithmetic, mod and div", "[programs]")
{
    const char *source = R"csp(
X:: [ a, b: integer; a := 2 + 3 * 5; b := a / 5;
      print(a, b, a mod 2, a div 5, -(a - 12)) ]
)csp";
    Run result = run("integer", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(17, 3, 1, 3, -5)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("Reals, and integers turned into reals where a real is needed", "[programs]")
{
    const char *source = R"csp(
X:: [ x, y: real; x := 2.5; y := 1; print(x, y, 3 / 4.0, x > 2) ]
)csp";
    Run result = run("real", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(2.5, 1.0, 0.75, true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("Characters, booleans, comparisons and the logical operators", "[programs]")
{
    const char *source = R"csp(
X:: [ c: character; b: boolean; c := '*'; b := c = '*';
      print(c, b, not b, c < 'a' and b, false or c <> 'x') ]
)csp";
    Run result = run("char_bool", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print('*', true, false, true, true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("One- and two-dimensional arrays", "[programs]")
{
    const char *source = R"csp(
const N = 3
X:: [ a: (0..N) integer; m: (1..2, 1..2) integer; i: integer;
      i := 0; *[ i <= N -> [ a(i) := i * i; i := i + 1 ] ];
      m(2, 1) := a(2) + 8;
      print(a(0), a(N), m(2, 1)) ]
)csp";
    Run result = run("array", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(0, 9, 12)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A tuple assignment computes every value before it assigns any", "[programs]")
{
    const char *source = R"csp(
X:: [ x, y: integer; x := 1; y := 2; (x, y) := (y, x); print(x, y) ]
)csp";
    Run result = run("tuple_assign", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(2, 1)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A declaration in a block hides an outer one until the block ends", "[programs]")
{
    const char *source = R"csp(
X:: [ n: integer; n := 1; [ n: integer; n := 2; print(n) ]; print(n) ]
)csp";
    Run result = run("scopes", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(2)",
        "X: print(1)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("An alternative command runs the commands of a guard that is true", "[programs]")
{
    const char *source = R"csp(
X:: [ n: integer; n := -1;
      *[ n < 2 -> [ [ n < 0 -> negative() [] n = 0 -> zero() [] n > 0 -> positive() ]; n := n + 1 ] ] ]
)csp";
    Run result = run("alternative", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: negative()",
        "X: zero()",
        "X: positive()",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A repetitive command repeats until no guard is true", "[programs]")
{
    const char *source = R"csp(
X:: [ i, sum: integer; i := 1; sum := 0; *[ i <= 10 -> [ sum := sum + i; i := i + 1 ] ]; print(sum) ]
)csp";
    Run result = run("repetitive", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(55)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("Values travel from west to X to east, the slides' first example", "[programs]")
{
    const char *source = R"csp(
west:: [ X!'a'; X!'b'; X!'c' ]
X:: *[ c: character; west?c -> east!c ]
east:: *[ c: character; X?c -> print(c) ]
)csp";
    Run result = run("communication", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "east: print('a')",
        "east: print('b')",
        "east: print('c')",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("An input takes only a message with its constructor, and a signal has no values",
          "[programs]")
{
    const char *source = R"csp(
X:: [ S!insert(4); S!has(4); S!stop() ]
S:: [ n: integer; X?insert(n); insert(n); X?has(n); has(n); X?stop(); stop() ]
)csp";
    Run result = run("constructors", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "S: insert(4)",
        "S: has(4)",
        "S: stop()",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A message can be a tuple of values of different types", "[programs]")
{
    const char *source = R"csp(
X:: Y!(1 + 2, 'z', 1.5)
Y:: [ a: integer; c: character; r: real; X?(a, c, r); print(a, c, r) ]
)csp";
    Run result = run("tuple_message", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "Y: print(3, 'z', 1.5)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("An alternative of input guards takes whichever sender is ready", "[programs]")
{
    const char *source = R"csp(
A:: [ Y!1; Y!2 ]
B:: [ Y!10; Y!20 ]
Y:: *[ n: integer; A?n -> from_a(n) [] n: integer; B?n -> from_b(n) ]
)csp";
    Run result = run("input_guards", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "Y: from_a(1)",
        "Y: from_a(2)",
        "Y: from_b(10)",
        "Y: from_b(20)",
    };
    CHECK(sorted_lines(result.m_output) == expected);
}

TEST_CASE("A range before a guard repeats it for every process of an array", "[programs]")
{
    const char *source = R"csp(
const N = 3
[ X(i:1..N):: S!value(i * 10) || S:: [ sum, n: integer; sum := 0;
    *[ (i:1..N) X(i)?value(n) -> sum := sum + n ]; print(sum) ] ]
)csp";
    Run result = run("range_guard", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "S: print(60)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A loop whose inputs come from terminated processes ends, and the commands after it run",
          "[programs]")
{
    const char *source = R"csp(
P:: [ C!1; C!1; C!1 ]
C:: [ n, k: integer; k := 0; *[ P?n -> k := k + n ]; print(k); done() ]
)csp";
    Run result = run("termination", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "C: print(3)",
        "C: done()",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE(
    "A macro's body replaces each use; a body in brackets is spliced, so its names stay visible",
    "[programs]")
{
    const char *source = R"csp(
X:: [ content: (0..9) integer; size, n: integer;
      content(0) := 7; content(1) := 5; content(2) := 9; size := 3;
      n := 9; SEARCH; print(i, size) ]
SEARCH:: [ i: integer; i := 0; *[ i < size; content(i) <> n -> i := i + 1 ] ]
)csp";
    Run result = run("macro", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(2, 3)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A -D overrides a const, or adds a constant of its own", "[programs]")
{
    const char *source = R"csp(
const N = 1
const K = N + M
X:: print(N, M, K)
)csp";
    Run result = run("define", source, "--delay=0", {{"N", "4"}, {"M", "2"}});
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(4, 2, 6)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A process family can have two subscripts and several definitions", "[programs]")
{
    const char *source = R"csp(
[ M(i:1..2, 0):: M(i, 1)!i
|| M(i:1..2, j:1..2):: [ x: integer; M(i, j - 1)?x; M(i, j + 1)!x ]
|| M(i:1..2, 3):: [ x: integer; M(i, 2)?x; S!x ]
|| S:: [ a, b: integer; M(1, 3)?a; M(2, 3)?b; print(a + a + b) ] ]
)csp";
    Run result = run("matrix_family", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "S: print(4)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("The slides' symbols: →, □, ‖, ≠, ≤ and ≥", "[programs]")
{
    const char *source = R"csp(
[ X:: [ n: integer; Y?n; [ n ≠ 3 → skip □ n ≤ 3 ∧ n ≥ 3 → print(true) ] ] ‖ Y:: X!3 ]
)csp";
    Run result = run("unicode", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("--trace prints each communication and the end of each hook; a hook prints its start too",
          "[programs]")
{
    const char *source = R"csp(
A:: B!has(1, 'x')
B:: [ n: integer; c: character; A?has(n, c); got() ]
)csp";
    Run result = run("trace", source, "--delay=0 --trace");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "A -> B: has(1, 'x')",
        "B: got()",
        "B: got() end",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("An alternative command whose guards are all false fails, and so does the program",
          "[programs]")
{
    const char *source = R"csp(
X:: [ n: integer; n := 1; [ n = 0 -> skip ] ]
)csp";
    Run result = run("alternative_fails", source, "--delay=0");
    CHECK(result.m_status == 1);
    CHECK(contains(result.m_error, "no guard of the alternative command"));
}

TEST_CASE("Two processes that wait for each other's output deadlock, and the watchdog says so",
          "[programs]")
{
    const char *source = R"csp(
A:: [ n: integer; B?n ]
B:: [ n: integer; A?n ]
)csp";
    Run result = run("deadlock", source, "--delay=0");
    CHECK(result.m_status == 3);
    CHECK(contains(result.m_error, "csp: deadlock"));
}

TEST_CASE("An output to a process that has terminated fails", "[programs]")
{
    const char *source = R"csp(
A:: [ B!1; B!2 ]
B:: [ n: integer; A?n ]
)csp";
    Run result = run("output_terminated", source, "--delay=0");
    CHECK(result.m_status == 1);
    CHECK(contains(result.m_error, "output to terminated process B"));
}

TEST_CASE("An integer may be sent to an input of a real, as NORTH sends 0 on the slides",
          "[programs]")
{
    const char *source = R"csp(
A:: B!0
B:: [ x: real; A?x; print(x) ]
)csp";
    Run result = run("integer_to_real", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "B: print(0.0)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("A type CSP doesn't know, such as the slides' portion, is taken as integer", "[programs]")
{
    const char *source = R"csp(
X:: [ data: portion; data := 5; print(data) ]
)csp";
    Run result = run("unknown_type", source, "--delay=0");
    CHECK(contains(result.m_messages, "[WARN_SEMA_UNKNOWN_TYPE]"));
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(5)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE(
    "When several boolean guards are true one is chosen at random, so in 200 runs each is chosen",
    "[programs]")
{
    const char *source = R"csp(
X:: [ a, b, k: integer; a := 0; b := 0; k := 0;
      *[ k < 200 -> [ [ true -> a := a + 1 [] true -> b := b + 1 ]; k := k + 1 ] ];
      print(a > 0 and b > 0) ]
)csp";
    Run result = run("nondeterminism", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("An output no input of its receiver matches is only a warning, since it may never run; "
          "when it runs, it waits forever",
          "[programs]")
{
    const char *source = R"csp(
A:: B!has(1)
B:: [ c: character; A?has(c) ]
)csp";
    Run result = run("unmatched_output", source, "--delay=0");
    CHECK(contains(result.m_messages, "[WARN_SEMA_NO_MATCHING_INPUT]"));
    CHECK(result.m_status == 3);
    CHECK(contains(result.m_error, "csp: deadlock"));
}

TEST_CASE("Among inputs whose senders wait, one is chosen at random, so no sender is starved; R "
          "counts to 100000 after each input, so that every sender waits again",
          "[programs]")
{
    const char *source = R"csp(
[ S(i:1..3):: [ k: integer; k := 0; *[ k < 200 -> [ R!k; k := k + 1 ] ] ]
|| R:: [ count: (1..3) integer; total, v, j: integer;
         count(1) := 0; count(2) := 0; count(3) := 0; total := 0;
         *[ (i:1..3) S(i)?v -> [ count(i) := count(i) + 1; total := total + 1;
                                 j := 0; *[ j < 100000 -> j := j + 1 ];
                                 [ total = 300 -> print(count(1) > 50 and count(2) > 50 and count(3) > 50)
                                 [] total <> 300 -> skip ] ] ] ] ]
)csp";
    Run result = run("fair_choice", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "R: print(true)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("The box spelled □, [] and |, also at the start of a line", "[programs]")
{
    const char *source = R"csp(
X:: [ n: integer; n := 1;
      *[ n > 0 and n <= 3 ->
           [ n = 1 -> small(n)
           [] n = 2 -> middle(n)
           |  n = 3 -> large(n)
           ];
           n := n + 1
       □ n = 4 -> n := 10
       [] n = 10 -> skip; n := -1
       ] ]
)csp";
    Run result = run("box_spellings", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: small(1)",
        "X: middle(2)",
        "X: large(3)",
    };
    CHECK(lines(result.m_output) == expected);
}

TEST_CASE("The notes' style: NAME :: value, (i:1..N) Name(i) :: BODY, Name(1..N), Integer, commas "
          "in guards, -> [ ... ] bodies and /* */ comments",
          "[programs]")
{
    const char *source = R"csp(
N :: 2

/* Senders send their number,
   the receiver adds them up. */
[
    (i:1..N) Sender(i) :: SENDER
 || Echo(1..N) :: ECHO
 || Receiver :: RECEIVER
]

SENDER :: [
    Receiver!value(i);
    sent(i);
]

ECHO :: ECHOING;

RECEIVER :: [
    sum, k, v : Integer;
    sum := 0;
    k := 0;
    *[
        k < N, (i:1..N) Sender(i)?value(v) -> [
            sum := sum + v;
            k := k + 1;
        ]
    ];
    print(sum);
]
)csp";
    Run result = run("notes_style", source, "--delay=0");
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "Echo(1): ECHOING",   "Echo(2): ECHOING",   "Receiver: print(3)",
        "Sender(1): sent(1)", "Sender(2): sent(2)",
    };
    CHECK(sorted_lines(result.m_output) == expected);
}

TEST_CASE("A -D overrides NAME :: value too", "[programs]")
{
    const char *source = R"csp(
SIZE :: 10
K :: 3
M :: -1
R :: 2.5

X :: print(SIZE, K, M, R)
)csp";
    Run result = run("notes_define", source, "--delay=0", {{"SIZE", "7"}});
    CHECK(result.m_status == 0);
    std::vector<std::string> expected = {
        "X: print(7, 3, -1, 2.5)",
    };
    CHECK(lines(result.m_output) == expected);
}
