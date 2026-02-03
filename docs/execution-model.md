# clanker — execution model

This document defines how clanker executes parsed commands.

It specifies the phases that occur **after parsing** and **before execution**,
and defines the responsibilities and boundaries of each phase.

Where behavior is planned but not yet implemented, the intended semantics
emulate `bash` and `zsh` when feasible, while preserving determinism and
explicitness.

---

## 1. Overview

Execution in clanker proceeds in distinct, ordered phases:

1. Parsing (completed before this document applies)
2. Expansion
3. Command classification
4. Execution
5. Exit status propagation

Each phase has a single responsibility, a well-defined input, and a
well-defined output.

---

## 2. Inputs to execution

The execution model consumes a parsed abstract syntax tree (AST) consisting of
structural nodes such as:

* simple commands
* pipelines
* command lists
* control operators

The AST contains **no execution concerns**:

* no file descriptors
* no processes
* no threads
* no side effects

The AST encodes structure only.

---

## 3. Expansion phase

Expansion operates on **word atoms in the AST** (e.g. the argument elements of
a `SimpleCommand`), not on lexer tokens.

Expansion transforms words but does not alter AST structure.

Expansion is:

* pure (no I/O, no execution)
* deterministic
* testable in isolation

### 3.1 Brace expansion

Brace expansion is applied to individual words.

Rules:

* A brace expansion contains one or more comma-separated elements.
* Whitespace surrounding commas is ignored.
* Whitespace within an element is preserved.
* Each element produces a distinct expanded word.
* Expansion preserves left-to-right order.

Examples:

```

{a,b,c}   → a  b  c
{a, b,c}  → a  b  c
{a,b c}   → a  "b c"   (single word)

```

Initial implementation policy:

* Brace expansion is non-recursive.
* Nested brace groups are treated as literal text.

This matches a conservative subset of `bash`/`zsh` behavior and may be extended
later.

### 3.2 Expansion errors

Expansion does not handle malformed syntax; unbalanced constructs are detected
during lexing or parsing.

Expansion may fail only due to explicit policy limits (e.g. excessive expansion
count), producing an expansion error.

Expansion errors abort execution of the affected command.

### 3.3 Other expansions (planned)

The following expansions may be implemented in later phases, with semantics
modeled after `bash`/`zsh` where feasible:

* parameter expansion
* command substitution
* globbing
* arithmetic expansion

Each expansion must be:

* documented
* deterministic
* covered by unit tests

No expansion is enabled implicitly.

---

## 4. Command classification

After expansion, each simple command is classified as one of:

* built-in command
* external command

Classification is performed by the executor using the built-in registry.

---

## 5. Execution phase

### 5.1 Built-in commands

* Execute in the shell process
* May mutate shell state
* Receive an explicit execution context
* Produce an integer exit status

Built-ins do not implicitly inherit global state; all required context is
passed explicitly.

### 5.2 External commands

* Execute in child processes
* Inherit environment and file descriptors explicitly
* Are launched via the process subsystem

Process creation and management are isolated from parsing and expansion logic.

---

## 6. Pipelines

A pipeline connects the standard output of one command to the standard input of
the next.

Rules:

* Pipeline stages are started before waiting for completion.
* Pipeline stages execute concurrently in the shell sense.
* The exit status of a pipeline is the exit status of the final stage.

Built-in participation rules (current implementation):

* A built-in command is permitted only in the **first** pipeline stage.
* A built-in appearing in any later stage is a deterministic error.

This restriction may be relaxed in later versions.

---

## 7. Command lists and control operators

Command lists execute pipelines sequentially.

Planned control operators (modeled after `bash`/`zsh`):

* `A && B` — execute `B` only if `A` exits with status 0
* `A || B` — execute `B` only if `A` exits with non-zero status
* `A &`    — execute `A` in the background

Current status:

* `&&`, `||`, and `&` are lexed
* The parser recognizes them but rejects them as not implemented

Execution semantics are introduced only after full specification and testing.

---

## 8. Exit status propagation

Every executed command produces an integer exit status.

Rules:

* The shell records the exit status of the most recently executed command
* Pipeline status is the status of the final pipeline stage
* Conditional execution (`&&`, `||`) is based on exit status
* Built-ins and external commands follow the same conventions

Exit status semantics follow traditional shell behavior.

---

## 9. Error handling

Errors are classified as:

* parse errors (before execution)
* expansion errors
* execution errors

Rules:

* Parse errors prevent execution entirely
* Expansion or execution errors abort the affected command
* Shell state remains valid and consistent after errors

There is no rollback of external side effects.

---

## 10. Design principles

The execution model is designed to be:

* explicit rather than clever
* deterministic rather than permissive
* easy to test
* easy to extend incrementally

Compatibility with `bash` and `zsh` is a guiding reference, not a constraint.
Where behavior differs, clanker favors clarity and explicit specification.

---

