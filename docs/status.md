## Current status

* Shell loop works
* Built-ins: exit, cd, pwd, help, prompt/ask (stub)
* External commands via posix_spawnp
* No real lexer/AST yet (parser only checks completeness)
* No pipelines yet
* No line editing/history yet

## Next planned steps

1. Replace naive split with real lexer
2. Introduce AST for simple commands + pipelines
3. Pipe execution

