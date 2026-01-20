# clanker — architecture

This document describes the high-level architecture of the clanker shell.
It is intended to guide implementation and keep responsibilities separated.

---

## Goals

* Clear separation of concerns: input, parsing, execution, services
* Deterministic behavior and testable components
* Interactive responsiveness (fast prompt, interruptible work)
* Minimal dependencies (C++23 + standard library)

---

## Top-Level Components

### 1) REPL / Shell Loop

Responsibilities:

* Display prompts
* Read user input (support continued input until parse-complete)
* Dispatch complete input to the parser
* Execute the resulting AST
* Maintain shell state (cwd, variables, last status, job table, history)
* Handle Ctrl-C and EOF

Key interfaces:

* `LineEditor` (interactive input)
* `Parser` (string → AST or error / incomplete)
* `Executor` (AST + shell state → status)

---

### 2) Line Editor and History

Responsibilities:

* Provide bash-like line editing and history behavior
* Maintain an in-memory history list
* Read/write a history file
* Support incremental reverse search (Ctrl-R)
* Never record secrets (e.g., API keys) in history

Implementation approach:

* Phase 1: minimal line editor (raw mode optional) with history and basic editing
* Phase 2: full bash-like keybindings and search
* History expansion (`!`) is deferred; if implemented it must be bash-compatible

Note: Line editing is a UI layer; it must not affect parsing semantics beyond
providing a complete input string.

---

### 3) Lexer / Parser

Responsibilities:

* Tokenize input according to `command-language.md`
* Parse tokens into an AST:
  * simple commands
  * pipelines
  * `&&` / `||`
  * optional background marker
* Provide “parse incomplete” feedback to the REPL (e.g., open quote, dangling `|`)

Non-goals (initially):

* Bash-compatible expansion
* Functions, here-docs, command substitution

Suggested structure:

* `Lexer` produces a vector of tokens with source locations
* `Parser` consumes tokens and produces:
  * `ParseResult::Complete(AST)`
  * `ParseResult::Incomplete(reason)`
  * `ParseResult::Error(message, location)`

---

### 4) AST (Abstract Syntax Tree)

The AST should be small, explicit, and stable.

Minimal nodes:

* `SimpleCommand`
  * `std::vector<std::string> argv`
  * (later) redirections, assignments

* `Pipeline`
  * `std::vector<SimpleCommand> stages`

* `CommandList`
  * sequence of `(Pipeline, Op)` where `Op` is `AndThen` or `OrElse`

* `JobSpec`
  * `CommandList list`
  * `bool background`

The AST must not embed execution concerns (file descriptors, threads, etc.).

---

### 5) Executor

Responsibilities:

* Execute AST nodes and return an exit status
* Dispatch a `SimpleCommand` to:
  * built-in implementation, or
  * external program execution
* Implement pipelines and conditional execution:
  * pipeline wiring
  * short-circuit for `&&` and `||`
* Integrate Ctrl-C handling and cleanup
* Maintain “last status” in shell state

Design constraints:

* Built-ins execute in-process and may mutate shell state
* External commands execute in child processes (platform-appropriate)
* Pipelines require concurrent execution of stages (processes and/or threads)

---

### 6) Built-in Registry

Responsibilities:

* Provide a mapping from command name → built-in implementation
* Provide metadata for help, completion, and documentation:
  * synopsis, flags, category

Interface shape:

* `BuiltinResult run(const BuiltinContext&, std::span<std::string> argv)`

The registry must include clanker-specific built-ins (LLM orchestration) and
bash-compatible built-ins as defined in `built-ins.md`.

---

### 7) External Command Runner

Responsibilities:

* Locate executables (PATH handling policy is explicit)
* Launch process with argv/environment
* Support pipelines by exposing stdin/stdout/stderr wiring
* Support foreground/background jobs (later)

Platform notes:

* macOS/Linux: `posix_spawn` or `fork`/`exec`
* Windows: deferred; architecture should keep it isolated

---

### 8) LLM Service Layer (Stub First)

Responsibilities:

* Provide a stable interface for LLM requests and responses
* Hide backend differences (OpenAI, Anthropic, etc.)
* Provide streaming or non-streaming responses (later)

Interfaces:

* `ModelBackend` (polymorphic)
  * `Completion request(CompletionRequest)`
* `ModelRegistry`
  * configured backends and default selection

Phase 1: stub implementation returns fixed text or echoes input.

---

## Shell State

Shell state is owned by the REPL and passed by reference to the executor and
built-ins.

Suggested state:

* Current working directory (logical root enforced by policy)
* Environment snapshot / overlay (what external commands inherit)
* Built-in registry
* Model registry + current model selection
* Context set (files/directories selected for LLM prompting)
* History manager
* Job table (background jobs; later)
* Last exit status
* Last LLM response (for `last`, `save`, etc.)

---

## Control Flow

### Interactive Loop

1. Display primary prompt
2. Read a line
3. Append to an input buffer
4. Attempt parse:
   * Complete → execute AST → clear buffer
   * Incomplete → show secondary prompt → continue reading
   * Error → report → clear buffer
5. Repeat until `exit` or EOF

### Batch Process

1. Take name of a file containing a script and optional arguments
2. Process the script for proper syntax
3. Best effort script execution:
   * parse script
   * If syntax errors are discovered, emit error message and terminate
   * Proceed with execution of AST
   * If a runtime error occurs, notify, roll back if possible, cleanup,
     terminate
   * Clean up and terminate returning error code like any bash script would

---

## Signals and Interrupts

Requirements:

* Ctrl-C interrupts the foreground activity
* Shell returns to a fresh prompt on a new line
* No resource leaks (pipes, child processes, threads)

Approach:

* Keep signal handler minimal: set an atomic flag
* Executor periodically checks the flag and aborts/cleans up
* For external processes, forward signals to the foreground process group (later)

---

## Asynchrony and Jobs (Phased)

Phase 1:

* Synchronous execution of simple commands and pipelines
* No job table (or minimal “background start” placeholder)

Phase 2:

* Background execution with `&`
* `jobs`, `fg`, `bg`, `wait` built-ins
* Job table with process identifiers and states

---

## Testing Strategy

* Lexer tests: token sequences from inputs
* Parser tests: AST shapes and parse-incomplete cases
* Executor tests:
  * built-ins in isolation
  * pipeline wiring using small test commands
* LLM service tests: stub deterministic outputs

Focus: deterministic unit tests before integrating interactive I/O.

---

## Implementation Order (Recommended)

1. Lexer + parser + AST (no execution)
2. Minimal REPL (no line editing; `std::getline`)
3. Built-in registry + a few built-ins (`exit`, `pwd`, `cd`, `help`)
4. External command runner
5. Pipelines and `&&`/`||`
6. Ctrl-C interrupt handling
7. LLM service stub + LLM built-ins (`prompt`, `ask`, `use`, `models`)
8. History and line editor improvements
9. Background jobs and job control

---

## Command Execution Model

A pipeline stage is either:
* a built-in executed in-process, or
* an external command executed as a child process.

Both are wired through stdin/stdout/stderr so that built-ins can participate in
pipelines wherever practical.

## Trust Boundaries

* External commands: local execution under the user account (traditional shell risk).
* Agents/LLMs: remote HTTPS services exchanging JSON; they do not run local code.
* clanker-managed filesystem operations may be restricted to a configured root.

---
