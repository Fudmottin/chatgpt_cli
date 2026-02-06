# clanker Expansion Model (Normative Target)

## 1. Purpose

The expansion model defines how values are computed and lowered into argv.

This is distinct from parsing.

---

## 2. Expansion Forms

Primary form:

    @( expression )

Expressions are evaluated in the current runtime environment.

---

## 3. Evaluation Order

Within a command:

    cmd A @(B) C @(D)

Evaluation order:

1. A
2. @(B)
3. C
4. @(D)

Strict left-to-right.

No reordering.

---

## 4. Lowering Rules

Value → argv element:

| Value Type | Lowering |
|------------|----------|
| String     | unchanged |
| Int        | decimal string |
| Bool       | "true"/"false" |
| Null       | "" |
| List       | error (unless explicitly spliced) |
| Object     | JSON encoding |
| Procedure  | error |

No implicit splitting.

---

## 5. Safety

- No implicit globbing.
- No implicit IFS splitting.
- All splitting must be explicit.

---

## 6. Eval (Future)

A future `eval(string)` form parses and executes in the current environment.

Eval must be explicit.

No implicit dynamic parsing.

