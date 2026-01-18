# clanker — security model

## Threat model

clanker runs local commands as the current user account, like a traditional shell.
Agent/LLM backends are remote services accessed via HTTPS exchanging JSON.

## Trust boundaries

* Local execution: external commands have full user privileges.
* Remote agents: cannot execute locally; they only receive requests and return responses.
* clanker-managed file operations may be restricted to a configured root.

## Key rules

* Never store secrets in history.
* Never emit API keys to stdout/stderr.
* Prefer explicit allow-lists for clanker-managed file paths.


