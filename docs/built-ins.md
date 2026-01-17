# clanker — built-in commands

This document defines clanker built-in commands.

When a command name and purpose matches a bash built-in, clanker shall provide
compatible behavior and exit status, unless explicitly noted here.

Built-ins are grouped by priority and purpose. Not all bash built-ins are
required for the initial implementation.

---

## Conventions

* Exit status: 0 on success, non-zero on failure (bash-compatible where applicable)
* Built-ins execute within the shell process (they may affect shell state)
* External commands are executed via fork/exec (or equivalent)

---

## Minimum viable built-ins (Phase 1)

These are sufficient for a usable interactive shell and for orchestrating LLMs.

### Shell lifecycle

* `exit [n]`  
  Exit clanker with status `n` (default: last command status).

* `logout`  
  Exit a login shell (bash-compatible; may be an alias of `exit` depending on mode).

### Navigation and environment

* `cd [dir]`  
  Change the current working directory.

* `pwd [-LP]`  
  Print the current working directory.

* `export [name[=value] ...]`  
  Set environment variables for subsequently executed programs.

* `unset [-f] [-v] name...`  
  Remove variables or functions from the environment/shell.

### Introspection and help

* `help [pattern]`  
  Describe built-ins and their usage.

* `type [-a] name...`  
  Describe how a name would be interpreted (built-in, external, etc.).

### Job control (optional early; common later)

* `jobs [-lpnrs] [jobspec...]`  
  List active jobs.

* `fg [jobspec]`  
  Bring a job to the foreground.

* `bg [jobspec]`  
  Resume a job in the background.

---

## LLM built-ins (Phase 1)

These are clanker-specific commands for orchestration. During early development
they may be stubbed.

* `models`  
  List available configured model backends and model identifiers.

* `use <backend> [model=<id>]`  
  Select the default backend/model for subsequent LLM commands.

* `prompt <text...>`  
  Send `<text...>` to the default model and print the response.

* `ask <backend> <text...>`  
  Send `<text...>` to the specified backend (e.g. `ask openai "..."`).

* `system <text...>`  
  Set or clear a session/system instruction for subsequent LLM calls.

* `context add <path>`  
  Add a file or directory (within clanker root) to the LLM context set.

* `context rm <path>`  
  Remove a path from the context set.

* `context ls`  
  List currently selected context inputs.

* `context clear`  
  Clear all selected context inputs.

* `pipe`  
  Read stdin fully and treat it as the prompt input (enables pipelines).

  Example:
  `cat foo.cpp | pipe | prompt "Refactor this code."`

* `save <path>`  
  Save the most recent LLM response to `<path>` (within clanker root).

* `last`  
  Print the most recent LLM response (or the last command’s captured output).

* `setkey <backend> <envvar>`  
  Configure where an API key is read from (e.g. `OPENAI_API_KEY`).
  The key value itself should never be stored in shell history.

---

## Bash built-ins (compatibility set)

These command names are reserved for bash-compatible built-ins. clanker may
implement them in phases.

### Shell state and configuration

* `alias [name[=value] ...]`  
  Define or list aliases.

* `unalias [-a] name...`  
  Remove aliases.

* `set [options] [name=value ...]`  
  Set shell options and positional parameters (subset may be implemented).

* `shopt [-pqsu] [-o] [optname...]`  
  Set/list shell options (optional).

* `declare [-aAfgiIlnrtux] [name[=value] ...]`  
  Declare variables and attributes (optional).

* `typeset`  
  Korn-shell-compatible synonym for `declare` (optional).

* `readonly [name[=value] ...]`  
  Mark variables as read-only.

* `local [name[=value] ...]`  
  Local variables within functions (requires function support).

* `unset`  
  (See Phase 1.)

### Execution and evaluation

* `:`  
  No-op; returns success.

* `true` / `false`  
  Return success / failure.

* `command [-pVv] command [arg...]`  
  Run command bypassing functions/aliases; inspect command type.

* `builtin [shell-builtin [args...]]`  
  Execute a shell builtin explicitly.

* `eval [arg...]`  
  Evaluate arguments as shell code (high risk; likely deferred).

* `exec [-cl] [-a name] [command [args...]]`  
  Replace shell process with a command.

* `hash [-lr] [-p pathname] [-dt] [name...]`  
  Remember/forget command locations (optional).

* `return [n]`  
  Return from a function or sourced file (requires those features).

### Scripting and file inclusion

* `source file [args...]` / `.` file [args...]  
  Read and execute commands from file (candidate for “clanker scripts”).

* `trap [-lp] [[arg] signal_spec ...]`  
  Signal handling (optional).

### I/O and strings

* `echo [-neE] [arg...]`  
  Print arguments (bash semantics).

* `printf format [arguments...]`  
  Formatted output.

* `read [-ers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars]
  [-p prompt] [-t timeout] [-u fd] [name ...]`  
  Read a line from input (partial implementation acceptable early).

### Arithmetic

* `let arg [arg...]`  
  Arithmetic evaluation (optional).

### Directories and stack

* `pushd [-n] [+N | -N | dir]`  
  Push directory onto stack (optional).

* `popd [-n] [+N | -N]`  
  Pop directory stack (optional).

* `dirs [-clpv] [+N | -N]`  
  List directory stack (optional).

### Job control and process

* `jobs`, `fg`, `bg`  
  (See Phase 1.)

* `kill [-s sigspec | -n signum | -sigspec] pid | jobspec ...`  
  Send signals.

* `wait [-fn] [-p var] [id ...]`  
  Wait for jobs/processes.

* `disown [-h] [-ar] [jobspec ... | pid ...]`  
  Remove jobs from job table (optional).

* `suspend [-f]`  
  Suspend shell (optional).

### History and editing

* `history [n]` / `history -c` / `history -d offset` / `history -r` / `history -w` ...  
  Display and manage command history.

---

## Line editing and history requirements

Interactive input shall provide bash-like line editing and history behavior:

* Emacs-style editing keys by default
* Incremental reverse search (Ctrl-R)
* Persistent history file (configurable location)
* History expansion may be implemented later; if implemented, it must be bash-compatible

Implementation detail:
clanker may implement a small built-in line editor, or use an OS-provided
facility when available, but behavior visible to the user should match bash
closely.

