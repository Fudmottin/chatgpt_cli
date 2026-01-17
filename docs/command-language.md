# clanker — command language

This document specifies the command language accepted by the clanker shell.

The language is inspired by POSIX shells (notably `bash`), but is intentionally
smaller and more predictable. Only features explicitly described here are
supported.

---

## Design Principles

* Familiar surface syntax for shell users
* Minimal and explicit grammar
* Deterministic parsing with no context-sensitive ambiguities
* Features added only when their semantics are fully specified

---

## Input Model

* Input is read line by line
* Commands are executed only when the input is syntactically complete
* If the input is incomplete, clanker continues reading additional lines
* Newlines within a logical command are treated as whitespace

---

## Comments

* A `#` character begins a comment
* A comment extends to the end of the line
* Comments are ignored by the parser
* A `#` inside quotes is treated as a literal character

---

## Tokenization

The input is tokenized into a sequence of tokens.

### Whitespace

* Whitespace separates tokens
* Whitespace inside quotes is preserved

### Words

* A *word* is a sequence of characters not containing unquoted whitespace or
  operator characters
* Words may be formed from a mix of quoted and unquoted segments

Example:

```

foo"bar baz"qux

```

is a single word with value `foorbar bazqux`.

---

## Quoting

### Single quotes

* Enclosed in `'...'`
* Contents are taken literally
* No escape sequences are recognized
* A single quote may not appear inside single quotes

Example:

```

'foo bar \ baz'

```

### Double quotes

* Enclosed in `"..."`  
* Whitespace is preserved
* Backslash escapes are recognized:
  * `\"` → `"`
  * `\\` → `\`
  * `\n` → newline
* Other escape sequences are unspecified and may be rejected

---

## Backslash Escaping

* A backslash escapes the following character
* A backslash immediately preceding a newline escapes the newline
* Escaped newlines do not appear in the resulting token stream

---

## Operators

The following operators are recognized:

* `|`   pipeline
* `&&`  logical AND
* `||`  logical OR
* `&`   background execution

Operators must be separated from words unless otherwise unambiguous.

---

## Grammar

The grammar is defined informally below.

### Simple command

```

simple-command :=
word { word }

```

The first word is the command name.
Subsequent words are arguments.

---

### Pipeline

```

pipeline :=
simple-command { '|' simple-command }

```

The standard output of each command is connected to the standard input of the
next command.

---

### Command list

```

command-list :=
pipeline { ( '&&' | '||' ) pipeline }

```

* `&&` executes the right-hand pipeline only if the left-hand pipeline succeeds
* `||` executes the right-hand pipeline only if the left-hand pipeline fails

---

### Background execution

* A command list may be followed by a trailing `&`
* Background execution detaches the job from the foreground shell
* Job control behavior is defined elsewhere

---

## Redirection (initial subset)

The following redirections may be supported in later phases:

* `>`   redirect stdout to file
* `>>`  append stdout to file
* `<`   redirect stdin from file
* `2>`  redirect stderr to file

Redirections bind to the nearest command to their left.

---

## Parse Completeness

Input is considered incomplete if any of the following conditions hold:

* An open single or double quote is not closed
* The input ends with a backslash escaping a newline
* The input ends with a pipeline operator (`|`)
* The input ends with a logical operator (`&&` or `||`)

While input is incomplete, the shell continues reading additional lines.

---

## Secondary Prompt

When reading additional input for an incomplete command, clanker displays a
secondary prompt. The exact prompt string is implementation-defined.

---

## Expansion Policy

Initially, clanker performs **no expansions** other than quoting and escaping:

* No globbing (`*`, `?`, `[]`)
* No command substitution (`$(...)`, `` `...` ``)
* No arithmetic expansion
* No parameter expansion

Expansion features may be introduced incrementally and must be explicitly
documented before being enabled.

---

## Error Handling

* Syntax errors prevent execution of the command
* Errors are reported before any part of the command is executed
* Partial execution of a syntactically invalid command shall not occur

---

## Implementation Notes (Non-Normative)

* The lexer should not depend on command semantics
* The parser should produce a small, explicit AST
* Execution distinguishes built-in commands from external commands
* LLM commands are built-ins and follow the same execution path as other commands

