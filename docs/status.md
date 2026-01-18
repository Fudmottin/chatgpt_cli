# Status

## Working

* REPL loop
* Lexer + parser producing Pipeline AST
* External pipelines work (posix_spawnp, pipe, dup2)
* Built-ins: exit/cd/pwd/help + LLM stubs
* Built-ins rejected inside pipelines (for now)

## Next

1. Make built-ins pipeline-capable via stdin/stdout/stderr in BuiltinContext
2. Add `type` builtin
3. Add history + line editing layer (bash-like)
4. Add `&&` / `||`

