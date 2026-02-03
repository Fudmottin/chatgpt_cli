# clanker — current implementation status

This document describes what the **current clanker implementation actually
accepts and executes today**, based on the present lexer, parser, AST, and
executor.

It is intentionally separate from the language specification
(`command-language.md` and `grammar.txt`) and is expected to change frequently.

This document is descriptive, not aspirational.

---

## 1. Files covered

This document reflects behavior implemented in:

* `src/clanker/lexer.h`, `src/clanker/lexer.cpp`
* `src/clanker/parser.h`, `src/clanker/parser.cpp`
* `src/clanker/ast.h`
* executor and process code as currently wired

---

## 2. Implemented: lexical layer

### 2.1 Tokens produced

The lexer currently produces the following token kinds:

* `Word`
* `Pipe`        (`|`)
* `AndIf`       (`&&`)
* `OrIf`        (`||`)
* `Ampersand`   (`&`)
* `Semicolon`   (`;`)
* `Newline`     (physical `\n`)
* `End`         (sentinel)

All other characters are incorporated into `Word` tokens.

---

### 2.2 Whitespace and terminators

* Horizontal whitespace (`' '`, `'\t'`, `'\r'`) separates tokens.
* Newline is emitted as a `Newline` token.
* Newline acts as a command terminator, subject to parse completeness rules.

---

### 2.3 Comments

* `#` begins a comment only when it appears at a token boundary (or after
  whitespace).
* A comment extends to the end of the physical line.
* The terminating newline is preserved and emitted as a `Newline` token.
* A `#` inside a `Word` is literal.

---

### 2.4 Quoting and escapes inside WORD

A `Word` token is constructed by scanning until a token boundary is reached.

#### Single quotes (`'...'`)

* Contents are taken literally.
* No escape processing is performed.
* May span multiple lines.
* If EOF is reached before the closing quote, lexing returns `Incomplete`.

#### Double quotes (`"..."`)

* May span multiple lines.
* Supported escape sequences:
  * `\"` → `"`
  * `\\` → `\`
  * `\n` → newline character
  * `\<newline>` → line continuation (newline removed)
* Any other escape sequence inside double quotes is a lexing `Error`.

#### Backslash outside quotes

* `\c` appends `c` literally to the word.
* `\<newline>` removes the newline (line continuation).
* A trailing backslash at end-of-input produces `Incomplete`.

---

### 2.5 Lexical features not implemented

The lexer does **not** currently recognize any of the following as structured
constructs; they are treated as ordinary characters within `Word` tokens:

* brace-groups `{ ... }`
* triple-quoted strings `'''...'''` or `"""..."""`
* command substitution `$(...)`
* backticks `` `...` `` (no substitution semantics)
* parameter expansion `$name`, `${...}`
* arithmetic expansion `$((...))`
* redirection operators (`>`, `>>`, `<`, etc.)

As a consequence:

* Whitespace inside `{...}` **does** split words.
* `{foo, bar}` and `{foo,bar}` tokenize differently.
* `$(`, `` ` ``, and `$` have no special meaning.

---

## 3. Implemented: parser and AST

### 3.1 AST types (current)

The AST currently consists of:

* `SimpleCommand`
  * `std::vector<std::string> argv`

* `Pipeline`
  * `std::vector<SimpleCommand> stages`

* `CommandList`
  * `std::vector<Pipeline> pipelines`

No redirections, assignments, compound commands, control operators, or functions
are represented in the AST.

---

### 3.2 Grammar accepted (effective)

The parser currently accepts:

* simple commands composed of `WORD`s
* pipelines using `|`
* sequential lists separated by `;` or newline

Effective grammar:

```

simple_command ::= WORD { WORD }
pipeline       ::= simple_command { "|" simple_command }
list           ::= pipeline { (";" | NEWLINE) pipeline }
[ (";" | NEWLINE) ]

```

Notes:

* Empty input or comment-only input produces a complete, empty command list.
* Trailing `;` or newline is accepted and considered complete.

---

### 3.3 Parse completeness

The parser reports `Incomplete` if:

* the lexer reports `Incomplete`
* the final non-`End` token is:
  * `|`
  * `&&`
  * `||`

A trailing `;` or newline does **not** cause incompleteness.

---

### 3.4 Parser errors

The parser reports `Error` for:

* empty pipeline stages (e.g. `| a`, `a | | b`)
* encountering any of the following tokens:
  * `&&`
  * `||`
  * `&`

These tokens are lexed but explicitly rejected with a diagnostic
(e.g. “operator not implemented”).

---

## 4. Implemented: execution behavior

* Built-in commands execute in-process.
* External commands execute via the platform process subsystem.
* Pipelines are supported.
* Exit status of a pipeline is the exit status of the final stage.
* Ctrl-C interrupts execution and returns control to the shell prompt.
* No job control or background execution exists.

---

## 5. Not implemented (language features)

The following language features are **not** currently parsed or executed:

* logical AND / OR (`&&`, `||`)
* background execution (`&`)
* redirections of any kind
* brace-groups as lexical WORD constructs
* brace expansion
* triple-quoted strings
* command substitution
* parameter expansion
* globbing
* arithmetic expansion
* compound commands (`if`, `for`, `while`, `case`, subshells)
* functions

These features appear in the *target* specification but are not yet present in
the implementation.

---

## 6. Known divergences from the target specification

High-impact gaps relative to the target language:

1. Brace-groups do not suppress whitespace splitting.
2. Triple-quoted strings are not recognized.
3. Command substitution is lexical-only (no execution semantics).
4. Control operators are lexed but rejected.
5. No redirections or IO numbers.
6. Grammar is limited to simple commands, pipelines, and lists.

These are tracked explicitly in `clanker-continuation.txt`.

---

## 7. Minimal test cases that currently work

```

echo hi
echo one two three
echo hi | wc
echo a; echo b
echo "a b"
echo 'a b'

```

Multiline input:

* unterminated quotes cause a secondary prompt
* escaped newlines join logical lines

---

## 8. Minimal test cases that currently fail (expected)

```

a && b
a || b
a &
echo hi > out.txt
test/{foo, bar}
echo $(date)
echo `date`
"""multi
line"""

```

Failures are expected and intentional at this stage.

---

