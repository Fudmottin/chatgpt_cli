# clanker — security model

This document defines the security model and trust boundaries of the clanker
shell as currently implemented.

clanker is not a sandbox. It is a shell for programmers, and its security model
is explicit and conservative rather than restrictive.

---

## Threat model

* clanker runs local commands as the invoking user account, like a traditional
  Unix shell.
* External commands have the full privileges of that user.
* LLM / agent backends are remote services accessed via HTTPS and exchange JSON
  data only.

clanker does not attempt to defend against a malicious local user or malicious
external programs executed by the user.

---

## Trust boundaries

### Local execution

* External commands execute with full user privileges.
* Built-in commands execute in the clanker process and may mutate shell state.
* clanker does not restrict filesystem traversal or system calls made by
  external commands.

### Remote agents (LLMs)

* LLM backends are treated as untrusted remote services.
* They cannot execute local commands directly.
* They receive explicit prompt data and return responses only.
* clanker remains the sole authority for any local execution or file access.

---

## Privilege and identity guarantees

clanker enforces the following invariants at runtime:

* clanker refuses to start if invoked as root (uid or euid 0).
* clanker records its startup identity (uid, euid, gid).
* If a privilege or identity change is detected at execution boundaries,
  execution is aborted deterministically.

This prevents accidental or unexpected privilege escalation during shell
operation.

---

## Secrets and sensitive data

* Secrets (API keys, tokens) must never be recorded in command history.
* Built-ins that handle secrets must read them from environment variables or
  secure storage only.
* Secrets must never be printed to stdout or stderr.
* LLM prompts should avoid including secrets unless explicitly intended.

---

## File access and policy

* clanker currently does **not** enforce filesystem confinement.
* Built-ins may operate on files and paths provided by the user.
* External commands are unrestricted.

Stronger filesystem policies (e.g. logical roots or allow-lists) may be
introduced later, but are not part of the current security model.

---

## Design principles

* Prefer explicit checks over implicit trust.
* Fail fast and deterministically on security violations.
* Avoid hidden privilege changes or ambient authority.
* Document security-relevant behavior explicitly.

---

## Non-goals

The following are explicitly **out of scope**:

* Sandboxing or capability-based isolation
* Mandatory access control
* Preventing data exfiltration by executed programs
* Protecting the user from themselves

clanker assumes an informed user operating in a trusted local environment.

---

