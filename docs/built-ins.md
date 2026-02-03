# clanker — built-in commands

This document defines clanker built-in commands and their intended behavior.

When a command name and purpose matches a bash built-in, clanker aims to provide
compatible behavior and exit status unless explicitly noted otherwise.

This document describes *intended* built-ins. Availability and completeness
vary by implementation phase.

---

## Conventions

* Exit status: `0` on success, non-zero on failure
* Built-ins execute within the shell process
* Built-ins may mutate shell state
* External commands execute in child processes

---

## Built-ins and Pipelines

Built-ins are designed to be stream-oriented:

* they may read from stdin
* they write normal output to stdout
* they write diagnostics to stderr
* they return an exit status

**Current implementation constraint:**

* A built-in command is permitted only in the **first stage** of a pipeline.
* A built-in appearing in any later pipeline stage is a deterministic error.

This restriction may be relaxed in later versions once execution semantics are
fully specified and tested.

---

## When to Prefer an External Command

Prefer an external command when the command:

* does not need to mutate shell state, and
* is primarily a data transformer.

Prefer a built-in when the command:

* must mutate shell state (e.g. `cd`), or
* must be available without spawning a process, or
* participates in clanker-specific orchestration (e.g. LLM commands).

---

## Minimum viable built-ins (Phase 1)

These are sufficient for a usable interactive shell and for orchestrating LLMs.

### Shell lifecycle

* `exit [n]`  
  Exit clanker with status `n` (default: last command status).

---

### Navigation

* `cd [dir]`  
  Change the current working directory.

* `pwd [-LP]`  
  Print the current working directory.

---

### Introspection and help

* `help [pattern]`  
  Describe built-ins and their usage.

* `type [-a] name...`  
  Describe how a name would be interpreted (built-in or external).

---

## LLM built-ins (Phase 1)

These commands are clanker-specific and may be stubbed during early development.

* `models`  
  List available configured model backends and identifiers.

* `use <backend> [model=<id>]`  
  Select the default backend/model for subsequent LLM commands.

* `prompt <text...>`  
  Send `<text...>` to the default model and print the response.

* `ask <backend> <text...>`  
  Send `<text...>` to the specified backend.

* `system <text...>`  
  Set or clear a system instruction for subsequent LLM calls.

* `context add <path>`  
  Add a file or directory to the LLM context set.

* `context rm <path>`  
  Remove a path from the context set.

* `context ls`  
  List currently selected context inputs.

* `context clear`  
  Clear all selected context inputs.

* `pipe`  
  Read stdin fully and treat it as prompt input.

  Example:
```

cat foo.cpp | pipe | prompt "Refactor this code."

```

* `last`  
Print the most recent LLM response or captured output.

* `save <path>`  
Save the most recent LLM response to a file.

* `setkey <backend> <envvar>`  
Configure which environment variable supplies an API key.
Key values must never be recorded in history.

---

## Planned bash-compatible built-ins

The following command names are reserved for bash-compatible behavior and may be
implemented incrementally.

### Shell state and configuration (planned)

* `export`
* `unset`
* `alias`, `unalias`
* `set`, `shopt`, `declare`, `typeset`, `readonly`, `local`

These require a defined variable and environment model.

---

### Execution and evaluation (planned / high risk)

* `:`
* `true`, `false`
* `command`
* `builtin`
* `eval`
* `exec`
* `hash`
* `return`

Several of these introduce complex or potentially unsafe semantics and are
expected to be implemented cautiously or deferred.

---

### I/O and strings (planned)

* `echo`
* `printf`
* `read`

Semantics should follow bash/zsh where feasible, but deterministic behavior
takes precedence over compatibility edge cases.

---

### Job control (planned)

* `jobs`
* `fg`
* `bg`
* `wait`
* `kill`
* `disown`
* `suspend`

These require background execution, job tables, and signal coordination.

---

### History and editing (planned)

* `history`

History expansion (`!`) may be implemented later; if implemented, it must be
bash-compatible.

---

## Line editing and history requirements

Interactive input should provide bash-like line editing behavior:

* Emacs-style key bindings by default
* Incremental reverse search (Ctrl-R)
* Persistent history file (configurable)

Line editing is a UI concern; it must not affect parsing or execution semantics.

---

## Notes on compatibility

Compatibility with bash and zsh is a **goal**, not a guarantee.

Where exact compatibility conflicts with determinism, clarity, or security,
clanker documents and prefers its own explicitly defined behavior.

---

