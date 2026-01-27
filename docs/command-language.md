# clanker — command language

This document defines the **command language accepted by the clanker shell**.

It is the *normative* specification of the language.  
The grammar is inspired by POSIX shells (`sh`, `bash`, `zsh`), but POSIX
compliance is a **guide**, not a constraint. Where trade-offs exist,
clanker prefers:

* minimal surprise for programmers,
* deterministic parsing,
* lexical rules that scale as the language grows.

The reference grammar is expressed in EBNF style and is derived from
`docs/grammar.txt`.

---

## 1. Design principles

* Familiar shell syntax for experienced users
* C-like whitespace handling
* Deterministic, lexer-driven parsing
* Clear separation of:
  * **lexing & parsing** (defined here)
  * **expansion & execution** (defined elsewhere)
* Features are added only when their semantics are fully specified

---

## 2. Input model

* Input is read as a sequence of characters
* Commands are executed only when the input is **syntactically complete**
* While input is incomplete, clanker continues reading additional lines
* Newlines inside a logical command are treated as whitespace

This applies equally to:
* interactive REPL input
* batch/script input

---

## 3. Whitespace (C-like rule)

Whitespace is treated **as in the C programming language**.

### 3.1 General rule

* Outside of word constructs, whitespace separates tokens
* Inside a word construct, whitespace is **ordinary data**
* No construct requires “special spacing” to parse correctly

Therefore, the following commands **must be lexically equivalent**:

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

* `#` begins a comment
* A comment extends to the end of the physical line
* A `#` inside a word construct is literal
* Comments are not recognized inside triple-quoted strings

---

## 5. Tokens

### 5.1 Operators (longest match wins)

```

&&  ||  >>  <<
|&  |
;;  ;&  ;|
<>  <&  >&  >|

> <
> ;   &
> (   )   {   }

```

### 5.2 Reserved words (contextual)

Reserved words are recognized only in *command position*:

```

if then elif else fi
for in do done
while until case esac
function select
time

```

---

## 6. WORD tokens (lexical)

The parser treats `WORD` as an **opaque token**.  
All structure described below is handled by the **lexer**, not the parser.

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

* Whitespace is preserved
* Escape sequences are recognized:
  * `\"` → `"`
  * `\\` → `\`
  * `\n` → newline
  * `\<newline>` → line continuation
* Other escapes are errors
* May span multiple lines

---

### 6.4 Triple-quoted strings (Python-style)

Triple-quoted strings are supported to minimize surprise for programmers
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

This feature replaces the need for POSIX here-documents.

---

### 6.5 Backslash escapes

Outside of quotes:

* `\c` escapes the next character
* `\<newline>` escapes the newline entirely

Inside double quotes, escapes follow the rules in §6.3.

---

### 6.6 Brace-groups (lexical)

A brace-group is a **word-level construct**:

```

{ ... }

```

Rules:

* Brace-groups may nest
* Whitespace inside a brace-group is **not a token separator**
* Commas and whitespace are ordinary characters
* Newlines inside a brace-group are treated as whitespace

Therefore:

```

{foo, bar, baz}
{foo,bar,baz}

```

are lexically identical.

Whether brace-groups participate in **expansion** is an execution-phase
decision and is not defined here.

---

### 6.7 Command substitution

Command substitution produces a word whose value is the output of a command.

Two forms are supported.

#### 6.7.1 Modern form

```

$( list )

```

* Nesting is allowed
* Uses full shell grammar inside
* Preferred form

#### 6.7.2 Backticks (legacy, but supported)

```

`list`

```

Rules follow the more flexible behavior found in `zsh` where it differs
from `bash`:

* Backticks may nest when escaped
* Backslash escaping applies inside backticks
* Newlines are allowed
* Intended for short, REPL-friendly commands

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

While incomplete, clanker continues reading input and presents a
secondary prompt.

---

## 8. Grammar (EBNF)

### 8.1 Start symbols

```

program
::= { complete_command terminator }
[ complete_command ]
[ terminator ]
EOF ;

terminator
::= ";" | NEWLINE ;

complete_command
::= list ;

```

---

### 8.2 Lists and pipelines

Precedence (tightest → loosest):

1. simple command elements
2. pipeline
3. logical AND / OR
4. sequential list

```

list
::= and_or { ( ";" | "&" | NEWLINE ) and_or }
[ ( ";" | "&" | NEWLINE ) ] ;

and_or
::= pipeline { ( "&&" | "||" ) linebreak pipeline } ;

pipeline
::= [ "!" ] pipe_sequence ;

pipe_sequence
::= command { ( "|" | "|&" ) linebreak command } ;

linebreak
::= { NEWLINE } ;

```

---

### 8.3 Commands

```

command
::= simple_command
| compound_command [ redirect_list ]
| function_definition ;

```

---

### 8.4 Simple commands

```

simple_command
::= { prefix } [ command_name { element } ] ;

prefix
::= ASSIGNMENT_WORD | redirection ;

element
::= WORD | redirection ;

command_name
::= WORD ;

redirect_list
::= { redirection } ;

```

---

### 8.5 Redirections

```

redirection
::= [ IO_NUMBER ] redir_op WORD ;

redir_op
::= ">" | ">>" | "<" | "<>" | ">|" | "<&" | ">&" ;

```

---

### 8.6 Compound commands

```

compound_command
::= brace_group
| subshell
| if_clause
| while_clause
| until_clause
| for_clause
| case_clause ;

```

(Definitions follow POSIX shape; details omitted here for brevity and
defined in `grammar.txt`.)

---

## 9. Expansion and execution

This document defines **syntax only**.
After parsing, clanker performs a sequence of **expansion and execution
steps** before running commands.

The exact order of expansions is defined by the execution model.

### 9.1 Brace expansion

Brace expansion is an execution-phase transformation applied to `WORD` tokens.

In brace expansions:

* Commas separate elements.
* Whitespace surrounding commas is ignored.
* Whitespace within an element is preserved.

Brace expansion produces zero or more `WORD`s, replacing the original `WORD`.

Examples:

```
{foo,bar,baz} → foo bar baz
{foo, bar, baz} → foo bar baz
{foo,bar baz} → foo "bar baz"
{foo, bar baz} → foo "bar baz"

```


Brace expansion operates on the textual contents of a `WORD`.
Whether brace expansion is applied is an execution-phase decision.

### 9.2 Other expansions

The following expansions are defined by the execution model and are not
specified here:

* parameter expansion
* command substitution
* globbing
* field splitting
* redirection processing

Their semantics are specified separately.

---

## 10. Philosophy

clanker is a shell for programmers.

When faced with a choice between strict POSIX behavior and a model that:

* reduces cognitive load,
* supports multiline structured input,
* behaves consistently with modern languages,

clanker prefers the latter.

---

