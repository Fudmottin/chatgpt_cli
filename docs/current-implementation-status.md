# clanker — current implementation status

This document describes what the current `clanker` implementation accepts and
executes **today**, based on the present lexer/parser/AST implementation.

It is intentionally separate from the target language specification
(`docs/command-language.md` and `docs/grammar.txt`) and is expected to change
frequently.

---

## 1. Files covered

* `src/clanker/lexer.h`, `src/clanker/lexer.cpp`
* `src/clanker/parser.h`, `src/clanker/parser.cpp`
* `src/clanker/ast.h`

---

## 2. Implemented: lexical layer

### 2.1 Tokens produced

The lexer produces these token kinds:

* `Word`
* `Pipe` (`|`)
* `AndIf` (`&&`)
* `OrIf` (`||`)
* `Ampersand` (`&`)
* `Semicolon` (`;`)
* `Newline` (physical `\n`)
* `End` (sentinel)

### 2.2 Whitespace and terminators

* Horizontal whitespace: space, tab, carriage return (`' '`, `'\t'`, `'\r'`)
  separates tokens.
* Newline is emitted as a `Newline` token and acts as a command terminator
  (subject to parse completeness rules below).

### 2.3 Comments

* `#` begins a comment only when seen at a token boundary (or after whitespace).
* The comment runs to end-of-line.
* The newline is not consumed by comment skipping; it still becomes a `Newline`
  token.
* A `#` inside quotes is literal (because quotes are handled within `Word`).

### 2.4 Quoting and escapes inside WORD

A `Word` token is built by scanning until a token boundary is reached.

#### Single quotes `'...'`

* Contents are taken literally.
* No escapes are recognized.
* May span multiple lines.
* If EOF is reached before the closing quote, lexing returns `Incomplete`.

#### Double quotes `"..."`

* May span multiple lines.
* Supported escapes:
  * `\"` -> `"`
  * `\\` -> `\`
  * `\n` -> newline character
  * `\<newline>` -> line continuation (newline removed)
* Any other escape sequence inside double quotes is a lexing `Error`.

#### Backslash outside quotes

* `\c` appends `c` literally to the word.
* `\<newline>` removes the newline (line continuation).
* If `\` is the last character in the input, lexing returns `Incomplete`.

### 2.5 Not implemented lexically (treated as ordinary characters)

The lexer does **not** recognize any of the following as structured constructs:

* brace-groups `{ ... }` as a word construct
* triple-quoted strings `'''...'''` or `"""..."""`
* command substitution `$(...)`
* backticks `` `...` `` (they are not special; they are just characters)
* parameter expansion `$name`, `${...}`
* arithmetic expansion `$((...))`
* any redirection operators (`>`, `>>`, `<`, etc.) as operators

As a consequence, whitespace inside `{...}` currently *does* split words. For
example:

* `test/{foo,bar}` is one `Word`
* `test/{foo, bar}` becomes multiple `Word` tokens

---

## 3. Implemented: parser and AST

### 3.1 AST types (current)

* `SimpleCommand`:
  * `std::vector<std::string> argv`
* `Pipeline`:
  * `std::vector<SimpleCommand> stages`
* `CommandList`:
  * `std::vector<Pipeline> pipelines`

No redirections, assignments, compound commands, or functions are represented.

### 3.2 Grammar accepted (effective)

The parser currently accepts:

* words forming simple commands
* pipelines using `|`
* sequential lists separated by `;` or newline

Conceptually:

```

word            := WORD
simple_command  := word { word }
pipeline        := simple_command { '|' simple_command }
list            := pipeline { (';' | NEWLINE) pipeline } [ (';' | NEWLINE) ]

```

Notes:

* The parser creates an initial empty pipeline stage before reading tokens.
* Empty input or comment-only input returns `Complete` with an empty `Pipeline`
  (legacy behavior).

### 3.3 Parse completeness

The parser reports `Incomplete` if:

* the lexer reports `Incomplete` (open quote, dangling escape)
* the last non-`End` token is one of:
  * `|`, `&&`, `||`

Trailing `;` or newline is considered complete.

### 3.4 Errors

The parser reports `Error` for:

* an empty pipeline stage (e.g. `a | | b`, or `| a`)
* encountering any of:
  * `&&`, `||`, `&` (message: "operators not implemented yet")

---

## 4. Not implemented (parser/execution-facing)

The following are not parsed and therefore not executable as language features:

* `&&` / `||` conditional lists
* background execution `&`
* redirections of any kind
* compound commands (`if/for/while/case`, brace groups, subshells)
* functions
* here-documents (POSIX style)
* triple-quoted multiline strings
* command substitution `$(...)` and backticks with substitution semantics
* variable/parameter expansion, arithmetic expansion, globbing, brace expansion

---

## 5. Divergences from the target spec (high impact)

The target language spec requires, but the current implementation does not yet
provide:

1. Brace-groups as a WORD construct (whitespace inside braces must not split).
2. Triple-quoted multiline strings.
3. Command substitution (`$(...)` and backticks) as WORD constructs.
4. Rich operator set and precedence (at minimum: `&&`, `||`, `&` execution).
5. Redirections and IO numbers.
6. Compound commands and functions as grammar constructs.

---

## 6. Minimal test cases that currently work

* `echo hi`
* `echo one two three`
* `echo hi | wc`
* `echo a; echo b`
* `echo a` followed by newline terminator
* `echo "a b"`
* `echo 'a b'`
* multiline quotes:
  * `echo "a` (incomplete) then `b"` (complete)
  * `echo 'a` (incomplete) then `b'` (complete)
* escaped newline:
  * `echo a \`newline` b`  -> one WORD sequence across lines

---

## 7. Minimal test cases that currently fail (expected)

* `a && b`  -> error ("operators not implemented yet")
* `a || b`  -> error
* `a &`      -> error
* `echo hi > out.txt` -> `>` is part of a WORD, not a redirection operator
* `test/{foo, bar}` -> tokenization differs from `test/{foo,bar}`
* `echo $(date)` -> `$(` treated as literal characters
* `` echo `date` `` -> backticks treated as literal characters
* `"""multi\nline"""` -> treated as plain characters, not a string construct
```

