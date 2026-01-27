# clanker — execution model

This document defines how clanker executes parsed commands.

It specifies the phases that occur **after parsing** and **before execution**,
and describes the responsibilities and boundaries of each phase.

---

## 1. Overview

Execution in clanker proceeds in distinct phases:

1. Parsing (completed before this document applies)
2. Expansion
3. Command classification
4. Execution
5. Status propagation

Each phase has a single responsibility and a clearly defined input and output.

---

## 2. Inputs to execution

The execution model consumes a parsed abstract syntax tree (AST) consisting of:

* `SimpleCommand`
* `Pipeline`
* `CommandList`

The AST contains **no execution concerns**:
* no file descriptors
* no processes
* no threads
* no side effects

---

## 3. Expansion phase

Expansion operates on `WORD` tokens produced by the lexer and grouped by the
parser.

Expansion does not alter syntactic structure; it only transforms words.

### 3.1 Brace expansion

Brace expansion is applied to individual `WORD`s.

Rules:

* A brace expansion contains one or more comma-separated elements.
* Whitespace surrounding commas is ignored.
* Whitespace within an element is preserved.
* Each element produces a distinct expanded `WORD`.

Examples:

```

{a,b,c}        → a b c
{a, b, c}      → a b c
{a,b c}        → a "b c"

```

Brace expansion may be recursive or non-recursive; the initial implementation
may restrict recursion.

### 3.2 Other expansions (planned)

The following expansions may be implemented in later phases:

* parameter expansion
* command substitution
* globbing
* arithmetic expansion

Each expansion must be:
* deterministic
* testable in isolation
* documented before being enabled

---

## 4. Command classification

After expansion, each `SimpleCommand` is classified as one of:

* built-in command
* external command

Classification is performed by the executor using the built-in registry.

---

## 5. Execution phase

### 5.1 Built-in commands

* Execute in the shell process
* May mutate shell state
* Receive explicit execution context
* Participate in pipelines via file descriptors

### 5.2 External commands

* Execute in child processes
* Inherit environment and file descriptors explicitly
* Are wired together using pipelines when required

Process creation is isolated in the process subsystem.

---

## 6. Pipelines

A pipeline connects the standard output of one command to the standard input of
the next.

Rules:

* Pipelines execute concurrently
* The exit status of a pipeline is the exit status of the last command
* Built-ins may participate in pipelines where feasible

---

## 7. Command lists and control operators

Command lists execute pipelines sequentially.

Future extensions may include:

* `&&` and `||` conditional execution
* background execution (`&`)
* job control

---

## 8. Exit status propagation

Every executed command produces an integer exit status.

* Status is propagated through pipelines and command lists
* The shell maintains the last exit status
* Built-ins and external commands follow the same conventions

---

## 9. Error handling

Errors are classified as:

* parse errors (before execution)
* expansion errors
* execution errors

Errors during expansion or execution do not corrupt shell state.

---

## 10. Design principles

The execution model is designed to be:

* explicit rather than clever
* easy to test
* easy to extend
* unsurprising to programmers

Complexity is introduced only when semantics are fully specified.

---

