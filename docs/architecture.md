# clanker — architecture

This document describes the high-level architecture of the clanker shell.
It defines responsibilities and boundaries between components, and serves
as an architectural guide rather than a commitment to specific future
features.

This document must remain consistent with `execution-model.md`, which is
authoritative for execution semantics.

---

## Goals

* Clear separation of concerns: input, parsing, expansion, execution, services
* Deterministic behavior and testable components
* Interactive responsiveness (fast prompt, interruptible work)
* Minimal dependencies (C++23 + standard library)

---

## Top-Level Components

### 1) REPL / Shell Loop

Responsibilities:

* Display prompts
* Read user input (continuing until syntactically complete)
* Dispatch complete input to the parser
* Execute the resulting AST
* Maintain minimal shell state
* Handle Ctrl-C and EOF

Current shell state includes:

* Current working directory
* Last exit status

Additional state (history, variables, job table, model registry) is introduced
incrementally and documented when implemented.

Key interfaces:

* `LineEditor` (interactive input)
* `Parser` (string → AST or error / incomplete)
* `Executor` (AST + shell state → status)

---

### 2) Line Editor and History

Responsibilities:

* Provide interactive line editing
* Support continued input for incomplete syntax
* Maintain an in-memory history list
* Never record secrets (e.g., API keys) in history

Phased implementation:

* Phase 1: minimal input (e.g. `std::getline`)
* Phase 2: editable input and navigation
* Phase 3: advanced features (search, history persistence)

Line editing is a UI concern only; it must not affect parsing or execution
semantics.

---

### 3) Lexer / Parser

Responsibilities:

* Tokenize input according to `command-language.md`
* Parse tokens into an AST representing command structure:
  * simple commands
  * pipelines
  * conditional operators (`&&`, `||`)
  * optional background marker (`&`)
* Report incomplete input (open constructs, dangling operators)
* Report syntax errors with precise diagnostics

Notes:

* Command substitution and brace groups are recognized lexically
* Execution semantics are not handled by the parser

The parser produces one of:

* `Complete(AST)`
* `Incomplete(reason)`
* `Error(message, location)`

---

### 4) AST (Abstract Syntax Tree)

The AST is small, explicit, and execution-agnostic.

Principles:

* Encodes structure only
* No file descriptors, threads, or process concepts
* No implicit control flow

Representative nodes include:

* `SimpleCommand`
  * argument vector (`argv`)
  * (later) redirections

* `Pipeline`
  * ordered sequence of command stages

* Control operators (`&&`, `||`, `;`, `&`) represented as explicit structural
  nodes rather than flags

The exact shape of the AST is defined in `ast.h` and may evolve as features
are added.

---

### 5) Executor

Responsibilities:

* Execute AST nodes and return an exit status
* Dispatch simple commands to:
  * built-in implementations, or
  * external programs
* Implement:
  * pipeline wiring
  * conditional execution (`&&`, `||`)
* Propagate exit status according to shell rules
* Integrate Ctrl-C handling and cleanup

Design constraints:

* Built-ins execute in-process and may mutate shell state
* External commands execute in child processes
* Built-ins are subject to execution constraints defined in
  `execution-model.md`

---

### 6) Built-in Registry

Responsibilities:

* Map command names to built-in implementations
* Provide metadata for help and documentation

Interface shape:

```text
BuiltinResult run(const BuiltinContext&, std::span<std::string> argv)
```

The registry includes both core shell built-ins and clanker-specific
built-ins (e.g. LLM orchestration), as defined in `built-ins.md`.

---

### 7) External Command Runner

Responsibilities:

* Locate executables according to an explicit PATH policy
* Launch external programs with argv and environment
* Support pipeline wiring via stdin/stdout/stderr
* Support background execution in later phases

Platform notes:

* macOS/Linux: `posix_spawn` (preferred) or equivalent
* Windows support is deferred but architecturally isolated

---

### 8) LLM Service Layer (Stub First)

Responsibilities:

* Provide a stable interface for LLM requests and responses
* Abstract backend differences (OpenAI, Anthropic, etc.)
* Support deterministic stub behavior during early development

Interfaces include:

* `ModelBackend`
* `ModelRegistry`

Phase 1 implementations return fixed or echoed data.

---

## Shell State

Shell state is owned by the REPL and passed explicitly to the executor and
built-ins.

Currently defined state:

* Current working directory
* Last exit status

Planned additions (introduced incrementally):

* History manager
* Environment overlays
* Job table
* Model registry and selection
* Context sets for LLM prompting

No filesystem confinement is currently enforced.

---

## Control Flow

### Interactive Loop

1. Display primary prompt
2. Read input line
3. Append to input buffer
4. Attempt parse:

   * Complete → execute AST → clear buffer
   * Incomplete → show secondary prompt → continue
   * Error → report → clear buffer
5. Repeat until `exit` or EOF

### Batch Execution

1. Read entire script input
2. Parse script
3. If syntax errors exist, report and terminate
4. Execute AST
5. Terminate with final exit status

---

## Signals and Interrupts

Requirements:

* Ctrl-C interrupts foreground execution
* Shell returns to a clean prompt
* No resource leaks

Approach:

* Signal handlers set atomic flags only
* Executor checks flags and performs cleanup
* Forwarding signals to child processes is phased in later

---

## Asynchrony and Jobs

Phase 1:

* Synchronous execution only
* Background marker may be parsed but executed synchronously

Phase 2:

* Background execution with `&`
* Job table and related built-ins (`jobs`, `fg`, `bg`, `wait`)

---

## Testing Strategy

* Lexer tests: token sequences
* Parser tests: AST structure and incomplete cases
* Executor tests:

  * built-ins in isolation
  * pipeline wiring with test commands
* LLM tests: deterministic stub responses

Priority is deterministic unit tests before interactive features.

---

## Trust Boundaries

* External commands execute locally under the user account
* LLMs are remote HTTPS services exchanging JSON
* clanker remains the authority for all local execution


---

