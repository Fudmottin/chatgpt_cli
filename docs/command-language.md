# clanker — command language

This document defines the **command language accepted by the clanker shell**.

It is the *normative* specification of clanker’s syntax and lexical structure.
Execution semantics (expansion, control flow, process behavior) are defined
separately in `execution-model.md`.

The language is inspired by POSIX shells (`sh`, `bash`, `zsh`), but POSIX
compatibility is a **reference**, not a constraint. Where trade-offs exist,
clanker prefers:

* deterministic parsing,
* minimal surprise for programmers,
* lexical rules that scale as the language grows.

The reference grammar is expressed in EBNF and derived from `docs/grammar.txt`.

---

## 1. Design principles

* Familiar shell-like syntax
* Lexer-driven, deterministic parsing
* Clear separation of concerns:
  * **lexing & parsing** — defined here
  * **expansion & execution** — defined elsewhere
* Syntax is accepted only when its semantics are explicitly specified
* Incomplete input is detected precisely and recoverably

---

## 2. Input model

* Input is read as a sequence of characters
* Commands are executed only when input is **syntactically complete**
* While input is incomplete, clanker continues reading additional lines
* Newlines inside a logical command are treated as whitespace

This applies equally to:

* interactive REPL input
* batch or script input

---

## 3. Whitespace

Whitespace handling follows a **C-like rule**.

### 3.1 General rule

* Outside of word constructs, whitespace separates tokens
* Inside a word construct, whitespace is ordinary data
* No construct relies on special spacing to parse correctly

Therefore, the following commands are lexically equivalent:

```

mkdir -p test/{foo, bar, baz}
mkdir -p test/{foo,bar,baz}

```

### 3.2 Newlines

* A newline normally terminates a command (like `;`)
* A newline does **not** terminate a command when:
  * inside a word construct
  * escaped by a trailing `\`
  * the parser is awaiting completion of a syntactic construct

---

## 4. Comments

* `#` begins a comment when it appears at a token boundary
* A comment extends to the end of the physical line
* A `#` inside a word construct is literal
* Comments are not recognized inside triple-quoted strings

---

## 5. Tokens

### 5.1 Operators

Operators are recognized using a **longest-match-wins** rule.

Currently recognized operators include:

```

|    &&    ||    &    ;

```

Additional operators may be introduced only when their semantics are fully
specified and implemented.

---

## 6. WORD tokens (lexical)

The parser treats `WORD` as an **opaque lexical unit**.

All internal structure described below is handled entirely by the **lexer**.
The parser never inspects the contents of a `WORD`.

A `WORD` may be composed of any mixture of the following constructs.

---

### 6.1 Unquoted text

Any character not otherwise special contributes directly to the word.

---

### 6.2 Single quotes

```

' ... '

```

* Contents are taken literally
* No escape sequences are recognized
* May span multiple lines
* A single quote cannot appear inside single quotes

---

### 6.3 Double quotes

```

" ... "

```

Rules:

* Whitespace is preserved
* Escape sequences recognized:
  * `\"` → `"`
  * `\\` → `\`
  * `\n` → newline
  * `\<newline>` → line continuation
* Other escape sequences are errors
* May span multiple lines

---

### 6.4 Triple-quoted strings (Python-style)

Triple-quoted strings are supported to reduce surprise for programmers
familiar with Python.

```

''' ... '''
""" ... """

```

Rules:

* Contents are taken literally
* No escape processing is performed
* All characters (including newlines and `#`) are preserved
* Terminated only by the matching triple-quote sequence
* May span arbitrarily many lines

Triple-quoted strings produce a single `WORD`.

This feature replaces POSIX here-documents.

---

### 6.5 Backslash escapes

Outside of quotes:

* `\c` escapes the next character
* `\<newline>` escapes the newline entirely

Inside double quotes, escapes follow §6.3.

---

### 6.6 Brace-groups (lexical)

A brace-group is a **word-level construct**:

```

{ ... }

```

Rules:

* Brace-groups may nest
* Whitespace inside a brace-group is not a token separator
* Commas and whitespace are ordinary characters
* Newlines inside a brace-group are treated as whitespace

Therefore:

```

{foo, bar, baz}
{foo,bar,baz}

```

are lexically identical.

Whether brace-groups participate in **brace expansion** is defined by the
execution model and is not a parsing concern.

---

### 6.7 Command substitution

Command substitution produces a `WORD` whose value is the output of a command.

Two forms are recognized lexically.

#### 6.7.1 Modern form

```

$( list )

```

* Nesting is allowed
* Full shell grammar is used inside
* Preferred form

#### 6.7.2 Backticks (legacy)

```

`list`

```

Rules follow `zsh`-like behavior:

* Backticks may nest when escaped
* Backslash escaping applies inside backticks
* Newlines are allowed

Command substitution is lexically recognized; execution semantics are defined
by the execution model.

---

## 7. Parse completeness

Input is considered **incomplete** if any of the following hold:

* An open single, double, or triple quote is not closed
* An open brace-group is not closed
* An open command substitution is not closed
* The input ends with an operator requiring a right operand:
  * `|`
  * `||`
  * `&&`
* A trailing backslash escapes the newline

While incomplete, clanker continues reading input and presents a secondary
prompt.

---

## 8. Grammar (EBNF)

This grammar describes **only the subset of shell syntax currently accepted
by clanker**.

### 8.1 Start symbols

```

program
::= { command terminator }
[ command ]
[ terminator ]
EOF ;

terminator
::= ";" | NEWLINE ;

```

---

### 8.2 Commands and pipelines

Operator precedence (tightest → loosest):

1. simple command
2. pipeline (`|`)
3. logical AND / OR (`&&`, `||`)
4. sequential list (`;`, newline)

```

command
::= and_or ;

and_or
::= pipeline { ( "&&" | "||" ) pipeline } ;

pipeline
::= simple_command { "|" simple_command } ;

```

---

### 8.3 Simple commands

```

simple_command
::= WORD { WORD } ;

```

Redirections, assignments, and compound commands are **not yet part of the
accepted grammar** and are introduced only when their semantics are specified
and implemented.

---

## 9. Expansion and execution

This document defines **syntax only**.

After parsing, clanker performs a sequence of **expansion and execution**
steps defined by the execution model.

### 9.1 Brace expansion

Brace expansion is an execution-phase transformation applied to `WORD`s.

Rules:

* Commas separate elements
* Whitespace surrounding commas is ignored
* Whitespace within an element is preserved
* Expansion preserves left-to-right order

Examples:

```

{foo,bar,baz}     → foo  bar  baz
{foo, bar, baz}   → foo  bar  baz
{foo,bar baz}     → foo  "bar baz"

```

Whether brace expansion is applied is an execution-phase decision.

### 9.2 Other expansions

The following expansions are defined by the execution model and are not
specified here:

* parameter expansion
* command substitution execution
* globbing
* field splitting
* redirection processing

---

## 10. Philosophy

clanker is a shell for programmers.

When faced with a choice between strict POSIX behavior and a model that:

* reduces cognitive load,
* supports multiline structured input,
* behaves predictably and deterministically,

clanker prefers the latter.

---


