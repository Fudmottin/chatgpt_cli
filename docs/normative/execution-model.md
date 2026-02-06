# clanker Command Language — Execution Model (Normative)

## 1. Overview

clanker is a deterministic command orchestration language.

It separates:

- Syntax (shell-like process wiring)
- Expansion (value evaluation)
- Execution (process and builtin dispatch)

Parsing always completes before execution begins.

---

## 2. Evaluation Phases

Given input:

    echo @(f(x)) > out

Evaluation occurs in this order:

1. Lexing → tokens
2. Parsing → AST
3. Expansion (left-to-right, deterministic)
4. Redirection setup
5. Execution

---

## 3. Expansion Semantics

A WORD may contain embedded expansion forms:

    @(expression)

Expansion rules:

- Evaluation order is strictly left-to-right.
- Each expansion produces a Value.
- Values are converted to strings when forming argv.
- Lists are NOT implicitly flattened.
- No implicit whitespace splitting.
- No implicit globbing.

Explicit splicing must be used (future feature).

---

## 4. Pipelines

In:

    a | b | c

- Each stage runs concurrently.
- Exit status of pipeline = exit status of last stage.
- Builtins may run in:
  - Parent (if single command)
  - Subprocess (if in pipeline)

---

## 5. Redirections

Redirections apply to the command they are attached to.

    echo hi > out

Rules:

- `>` truncates
- `>>` appends
- `<` opens for read
- `2>file` redirects stderr

Redirection errors cause the command to fail before execution.

---

## 6. Lists

    a ; b
    a && b
    a || b
    a &

- `;` always continues
- `&&` executes RHS only if LHS succeeded
- `||` executes RHS only if LHS failed
- `&` runs command in background

---

## 7. Determinism

clanker guarantees:

- No context-sensitive parsing
- No runtime grammar mutation
- No global splitting side effects
- No implicit environment-dependent behavior

---

## 8. Typed Runtime (Target)

Future versions introduce Value types:

- String
- Int
- Bool
- Null
- List<Value>
- Object
- Procedure

These are manipulated inside `@( … )`.

Shell syntax remains process-oriented.

