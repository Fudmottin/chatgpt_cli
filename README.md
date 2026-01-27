# clanker

`clanker` is an experimental, POSIX-inspired shell written in modern C++23.
Its focus is **model-agnostic LLM orchestration**: treating LLM backends as
composable tools, similar to ordinary shell commands.

> Note: This README is informational. The authoritative language specification
> lives in `docs/`.

## Status

Early development. Parsing/execution is being built incrementally with unit
tests and a strict separation of concerns (lexer → parser/AST → executor).

## Authoritative documentation

- `docs/grammar.txt` — grammar truth (EBNF-ish)
- `docs/command-language.md` — human-readable command language spec
- `docs/current-implementation-status.md` — what works today
- `docs/architecture.md` — architecture and responsibilities
- `docs/built-ins.md` — builtin command intent and compatibility goals

## Build (CMake)

Typical out-of-source build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

## License

MIT: see LICENSE

