# CSP syntax and its translation

cspcc reads the CSP of Hoare's 1978 paper as the lecture slides write it. This
page gives the grammar it accepts and how each construct becomes Ada.

## Tokens

| Token | Spellings |
| --- | --- |
| Arrow | `→`, `->` |
| Box | `□`, `[]`, `\|` |
| Parallel | `‖`, `\|\|` |
| Comparisons | `=`, `<>` (also `≠`, `/=`), `<`, `<=` (`≤`), `>`, `>=` (`≥`) |
| Logic | `and` (`∧`), `or` (`∨`), `not` (`¬`) |
| Arithmetic | `+`, `-`, `*`, `/`, `mod`, `div` |
| Literals | `12`, `1.5`, `2e3`, `'c'`, `"c"` (one character), `true`, `false` |
| Comments | `-- to the end of the line`, `// to the end of the line` |

Identifiers are ASCII letters, digits and `_`, and they are case-sensitive:
`Fork` and `fork` are different names, and cspcc suggests the one that exists.

## Grammar

```
program        = { item } ;
item           = "const" ident "=" expression [ ";" ]
               | "[" definition { "||" definition } "]" [ ";" ]
               | definition ;
definition     = ident [ "(" subscript { "," subscript } ")" ] "::" commands ;
subscript      = ident ":" expression ".." expression | expression ;

commands       = command { ";" command } [ ";" ] ;
command        = "skip"
               | declaration
               | target ":=" expression
               | "(" target { "," target } ")" ":=" "(" expression { "," expression } ")"
               | process "?" message
               | process "!" message
               | ident [ "(" [ expression { "," expression } ] ")" ]
               | "[" commands "]"
               | "[" guarded { box guarded } "]"
               | "*" "[" guarded { box guarded } "]"
               | "*" "[" commands "]" ;
declaration    = ident { "," ident } ":" [ "(" bounds { "," bounds } ")" ] ident ;
bounds         = expression ".." expression ;
process        = ident [ "(" expression { "," expression } ")" ] ;
message        = expression ;

guarded        = guard arrow commands ;
guard          = { range_prefix } element { ( "," | ";" ) { range_prefix } element } [ ";" ] ;
range_prefix   = "(" ident ":" expression ".." expression { "," ... } ")" ;
element        = expression | declaration | process "?" message ;
```

The input, when a guard has one, is its last element; an output is never part
of a guard. A range prefix may stand anywhere in a guard, as in the slides'
`s > 0, (i:1..100) X(i)?wait()`, and repeats the whole guarded command once for
each value. `*[ commands ]` with no arrow repeats forever.

Expressions bind as usual: `or` loosest, then `and`, then `not`, then one
comparison (they don't chain), then `+` and `-`, then `*`, `/`, `mod` and
`div`, then unary `-`.

## Names

* A definition in a parallel command, a definition with subscripts, and a
  top-level definition no command uses are processes. The processes of one name
  form a family, such as `M(i:1..3, 0)` and `M(i:1..3, j:1..3)`, each process
  running the commands of the definition that covers its subscripts.
* A top-level definition some command uses, such as `PRODUCER` or `SEARCH`, is a
  macro. Each use is replaced by its body; a body that is one bracketed block
  is spliced, so the names it declares stay visible after the use, as the
  slides' `SEARCH` needs.
* A command that names nothing defined, such as `THINK`, `eat()` or `print(x)`,
  is a hook. It prints the process and the call with its arguments' values,
  then pauses a moment.
* `name(...)` is an array element when `name` is a declared array, and a
  constructor otherwise, so `Producer(i)?buffer(in mod 10)` stores into the
  array `buffer`, while `S?has(n)` receives a `has` message.

## Types

`integer`, `real`, `character` and `boolean`, with `int`, `double`, `float`,
`char` and `bool` as other names; a type cspcc doesn't know, such as the slides'
`portion`, is taken as `integer` with a warning. An array has constant bounds.
An integer goes wherever a real is expected, also in messages: `M(1, j)!0` may
reach an input of a real.

## Translation

| CSP | Ada |
| --- | --- |
| A family `Fork(i:0..4)` | `task type Fork_Task (Id : Pid)`, an array `Fork_T` of tasks, and `Fork_Id (Self, I1)` from subscripts to process numbers |
| A message kind `in()` that `Fork` receives | `entry C_in (Pid);`, a family indexed by the sender |
| `Fork(right)!in()` | `Fork_T (Fork_Id (Id, right)).C_in (Id);` |
| `Phil(left)?in()` | `accept C_in (Phil_Id (Id, left));`, polling for the sender's termination |
| Guards with inputs | `select` with `when Open (k) => accept ...` per unrolled alternative, and `or delay Poll` |
| Guards without inputs | A random choice among the true ones; when inputs are open too, `select ... else` |
| Declarations | Locals of the process's block, with unique Ada names |
| Constants | Named numbers, or typed constants |

## Semantics

* Communication is synchronous: an output waits until the receiver accepts it.
* An alternative command whose guards are all false fails, and so does the
  program, with status 1. A repetitive command in that state ends.
* An input guard whose source has terminated is false. A repetitive command
  whose guards are all false that way ends, which is Hoare's distributed
  termination: `*[ west?c → east!c ]` ends after `west` does.
* An input or output command naming a terminated process fails.
* When every process waits, the watchdog reports a deadlock with what each
  process waits for, and the program exits with status 3.
