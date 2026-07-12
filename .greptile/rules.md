# Greptile reviewer guide — Oxidus

This is an **LPC** mudlib running on the **FluffOS 2019+** driver. Much LPC
looks like C but behaves differently. Read this before flagging anything, and
treat every item below as a reason **not** to raise a comment unless you have
positive evidence of a real defect.

## Language / driver facts that produce false positives

- **LPC is not C.** `string *arr` is an **array**, not a pointer. There are no
  raw pointers. Mappings are `([ key: value ])`; array literals are `({ ... })`.
  No `main()`; entry points are lifecycle hooks (`create`, `setup`, `init`).
- **Single-threaded and synchronous is first-class.** The driver runs one
  execution at a time. Do not suggest async/await, locking, or "race
  condition" fixes; there is no preemption. Only flag a call if its *duration*
  would stall the driver, not because it is synchronous.
- **C99 mid-block declarations are supported.** Declaring a variable partway
  through a block is fine; do not ask for it to be hoisted or wrapped in a
  scope block.
- **`sscanf` counts `%*s` matches too** (unlike C). A format with assignment-
  suppressed fields returns a higher count than the number of stored values.
  Guard against the *full* match count, not `!= 1`.
- **File efuns resolve from the mudlib root.** A leading `/` is optional and the
  author's convention is slashless (e.g. `std/object.lpc`). Do not "correct"
  slashless paths, and do not claim a path is broken — compilation does not
  prove path resolution either way.
- **Security uses the FluffOS privs model, not `PACKAGE_UIDS`.** `valid_seteuid`
  and euid machinery never fire here; do not flag their absence. `master.lpc`'s
  defensive `set_privs("[master]")` re-asserts are intentional and working.
- **Numeric coercion preserves float values** through int-typed locals in
  FluffOS. `int x = <float expr>` does not necessarily truncate; verify before
  claiming a truncation bug.
- **Reserved type words** (`buffer`, `function`, `class`, `mapping`, `object`,
  `mixed`, …) may not be used as variable names — that one *is* worth flagging.
- **Closures.** In `(: :)` closures, `$N` is the call-time positional argument
  and `$(EXPR)` is a value captured lexically at creation. `$(2)` is not `$2`.

## Review philosophy

- **Judge this as a living single-author MUD**, not an enterprise
  JS/C++/service codebase. Favour a small number of high-confidence
  correctness findings over breadth.
- **"Touches a lot / might break things" is never a blocker** and not a
  warning on its own. Scope of impact is a scheduling question, not a defect.
- **Dormant-by-design scaffolding is not dead code.** Unused config keys,
  partially-wired APIs, and pinned/fixed procedural-generation seeds (used for
  reproducible reboots) are deliberate. Do not report them as bugs.
- **Do not raise style, formatting, spacing, naming, or documentation-format
  comments.** Those conventions live in project skills you cannot see, and a
  generic guess about LPC style will usually be wrong. Notably: private
  functions are **not** underscore-prefixed here.
- **Answer at the altitude of the change.** Stay within the diff; surface
  adjacent observations sparingly, as a brief aside at most.

## Trace, don't hedge

You have the whole repository. Use it before you write a comment.

- **State findings as facts, not conditionals.** "If `foo()` returns null then
  this dereferences null" is not a finding — it is unfinished homework. Go read
  `foo()` and its callers. Either it *can* return null on a real path (now it is
  a fact worth raising, with that path named) or it cannot (drop it). Phrasings
  like "if this...", "this could...", "it's possible that..." are a signal that
  the trace was not completed. Complete it or say nothing.
- **Unresolved uncertainty lowers confidence; it does not become a caveat.** Do
  not post a low-confidence guess wrapped in hedging language and let the author
  do the verification. That inverts the job — the reviewer that has the code
  should look, not ask the human to look. If you genuinely cannot resolve it
  from the repository, that is a reason to omit the comment, not to hedge it.
- **A claim that merely looks like a claim is worse than silence.** A confident-
  sounding comment that turns out to be "well, *if*..." wastes more trust than
  no comment at all.
- **Front-load the trace — the first comment must already carry the validated
  premise.** A finding is a conclusion you verified *before* posting, not an
  opening hypothesis you refine across a reply thread. Do not use the author's
  responses as your verification loop: if a premise only becomes valid after the
  author pushes back, it should have been checked before the comment existed.
  The author's time is not your compute. Disagreement is welcome — it is useful
  rubber-ducking and you are sometimes right — but it must start from a premise
  you already did the work to stand behind, not one you reach on reply #11.

## "Fails silently" is not a defect by itself — and nothing here is truly silent

First, the premise is usually false. **Every caught error is logged.** The
driver routes caught errors (and runtime errors) through `master`'s
`error_handler` (see /adm/obj/master.lpc), which unconditionally writes a full
trace to the catch log / runtime log. Dev notification on-screen is separately
gated by whether an interactive dev is online — so an error that raises no
on-screen message when no interactive is involved is **logged, not lost**.
"Silent" here means "did not print to someone's screen," which is correct,
intended behaviour, not a swallowed failure. (The one exception is a test-runner
sweep, where caught errors are intentional sad-path tests and suppression
auto-expires.)

Second, returning `0`/null, swallowing an error, or no-op-ing is a **deliberate
idiom** throughout this lib — guard returns, unauthorized-caller no-ops (e.g. the
`set_privs` override), and uniform result passthrough (e.g. `evaluate_result`
handling 1/0/string). Do **not** raise "this fails silently" on its own. Raise
it only when you have traced a **specific caller** that is actually harmed by the
swallowed signal — and name that caller and the concrete bad outcome. If you
cannot point to the victim, there is no bug.

## Things that are genuinely worth flagging

- Real logic errors: wrong operator, off-by-one, inverted condition, unhandled
  null (`nullp`) where an object/array is dereferenced **on a path you traced**.
- Calling a possibly-destructed object or invalid closure without a guard.
- Using a reserved type word as an identifier.
- Missing permission/input validation before a sensitive operation.
- New `.c` source files (should be `.lpc`).
