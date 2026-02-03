# clanker — overview

`clanker` is an interactive, a small, shell-like command interpreter inspired
by POSIX shells written in modern C++ (C++23). Its primary purpose is to
orchestrate one or more Large Language Models (LLMs) for tasks such as code
generation.

The shell provides a controlled execution environment with built-in commands,
external command execution, and LLM-aware built-in commands.

---

## Design Goals

* Familiar shell-like user experience inspired by `bash`
* Deterministic behavior with well-defined semantics
* Readability and maintainability over micro-optimizations
* Strict adherence to the C++ Core Guidelines
* Minimal dependencies: C++ standard library only

---

## Implementation Constraints

* Language: C++23
* Compiler: clang
* Build system: CMake
* Code formatting: clang-format
* Linting: clang-tidy
* No third-party runtime dependencies

---

## Execution Environment

* The shell’s current working directory is treated as the logical root
  by convention
* Stronger filesystem confinement may be introduced later
* External commands may be executed without restriction
* Environment variables are inherited by default. Mutation semantics are
  defined by clanker built-ins

---

## Shell Behavior

### Prompt

```
clanker >
```

### Command Input

* Input is line-oriented
* A command is executed only when the input is syntactically complete
* Multi-line input follows familiar shell rules:
  * a trailing backslash (`\`) escapes the newline
  * open quotes or incomplete syntactic constructs cause the shell to continue reading
  * newlines within a logical command are treated as whitespace
* Empty input produces no action

---

## Built-in Commands

* A set of built-in commands is provided
* These commands are inspired by common POSIX shell built-ins
  (e.g. `cd`, `pwd`, `exit`, `help`)
* The exact set and semantics are defined by clanker, not inherited implicitly

---

## LLM Commands

* LLM functionality is exposed via built-in commands (e.g. `prompt`, `ask`)
* LLM commands participate in normal shell parsing and composition
* Built-in and LLM commands may appear in pipelines, subject to execution
  constraints defined in the execution model
* Example:

```
prompt "Write a C++ function to tokenize shell input."
```

---

## LLM Integration (Initial Stub)

* LLM backends (e.g. OpenAI, Anthropic) are abstracted behind interfaces
* API calls are stubbed during early development
* LLM output may be:
  * displayed to the user
  * captured as structured data
  * passed to subsequent commands or models

---

## Control and Signals

* Long-running commands may be interrupted with Ctrl-C
* Interrupts trigger cleanup and return control to the shell prompt
* The `exit` command exits the shell
* An end-of-file condition (Ctrl-D) also exits the shell

---

## Concurrency

* Concurrency is limited and explicit. Background execution and asynchronous 
  behavior are documented separately
* Behavior should align with traditional POSIX shells where practical
* Concurrency must be explicit and well-documented

---

## Coding Style

* Follow standard C++ naming conventions
* Prefer C++23 language features where they improve clarity
* Code should favor correctness and clarity over cleverness

## Philosophy

clanker is modeled after `bash`:
* built-ins exist to mutate shell state (e.g. `cd`, `export`)
* most functionality is provided by external commands that compose via stdin/stdout
* pipelines are the primary composition mechanism

## Agents and Security

LLM/agent backends are treated as remote services:
* clanker sends requests over HTTPS and receives JSON responses
* agents do not execute local commands on behalf of clanker
* clanker remains the authority for local execution and filesystem access

