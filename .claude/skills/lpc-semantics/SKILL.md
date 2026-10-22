---
name: lpc-semantics
description: LPC and FluffOS semantics most often mistaken for defects in Oxidus — language shape, execution model, sscanf counts, path resolution, privs, numeric coercion, calls to undefined functions, functionals, truthiness, how errors surface, and the async/promise behaviours that do and do not indicate a bug. Consult before reporting any suspected defect in LPC code, whether in a review, a logic-check, or while debugging.
---

# LPC Semantics

The facts below are the ones most often mistaken for defects when reading
Oxidus code. Check a suspected finding against them before reporting it.

## Language and driver

**Language shape.** `string *arr` is an array, not a pointer; LPC has no raw
pointers. Array literals are `({ ... })` and mappings are `([ key: value ])`.
There is no `main()` outside commands — entry points are lifecycle hooks
(`create`, `setup`, `init`).

**Execution model.** The driver is single-threaded and runs one execution at a
time, with no preemption. Synchronous code is first-class here. Locking, async
restructuring, and race conditions do not apply; a call is worth raising only
when its *duration* would stall the driver.

**Mid-block declarations.** C99-style declaration at first use is supported and
idiomatic. It does not need hoisting or a scope block.

**`sscanf` match counts.** Unlike C, FluffOS counts `%*s` conversions in the
return value. A format with assignment-suppressed fields returns more than the
number of values stored, so guards test the full match count.

**Path resolution.** File efuns resolve from the mudlib root; a leading `/` is
optional, and the author's convention omits it (`std/object.lpc`). Compilation
does not demonstrate path resolution in either direction.

**Security model.** Oxidus uses the FluffOS privs model, not `PACKAGE_UIDS`.
`valid_seteuid` and euid machinery never fire, and their absence is not a gap.
The defensive `set_privs("[master]")` re-assertions in
[adm/obj/master.lpc](../../../adm/obj/master.lpc) are intentional.

**Numeric coercion.** FluffOS preserves float values through int-typed locals.
`int x = <float expr>` does not necessarily truncate; confirm against the
driver before treating it as one.

**Calls to undefined functions.** `ob->foo()` and `call_other(ob, "foo")` return
`0` when `ob` does not define `foo()`; this is not a runtime error.
`apply_low()` in `src/vm/internal/apply.cc` pops the arguments and returns 0.
Duck-typed calls across mixed inventories are a normal idiom, so an unguarded
`->` is not a crash. A `function_exists()` guard is meaningful only where the
code must distinguish "absent" from a legitimately returned `0`.

**Functionals.** FluffOS has functionals, not closures — nothing captures the
enclosing scope. In `(: :)`, `$N` is a call-time positional argument and
`$(EXPR)` is a value bound at creation; `$(2)` is not `$2`. Guard invocation
with `valid_function()` rather than bare `functionp()`, which stays truthy for
functionals whose owner has been destructed.

**Truthiness.** `""` and `0.0` are truthy in LPC; only `0`, null, and undefined
are falsy. `if(!str)` does not catch an empty string. The `truthy()` / `falsy()`
simul_efuns do treat `""` as empty, so the two are chosen deliberately.

**`typeof()`** is overridden to return `T_UNDEFINED` for null, where the native
version reports an int.

**Conventions with a reason.** `pointerp()` is preferred over its alias
`arrayp()`. `mudlib.h` arrives via `global.h` and is not included directly.
Comments and documentation use lib-relative paths (`/std/object.lpc`), never
host-absolute ones, since the repository is meant to be forkable.

## How errors surface

Errors here are logged, not lost. The driver routes caught and runtime errors
through `error_handler` in
[adm/obj/master.lpc](../../../adm/obj/master.lpc), which writes a full trace to
the catch and runtime logs unconditionally. On-screen developer notification is
gated separately, on whether an interactive developer is present — so a failure
that prints nothing to a screen has still been recorded. The one exception is a
test-runner sweep, where caught errors are deliberate sad-path assertions and
suppression expires automatically.

Returning `0` or null, swallowing an error, and no-op-ing are deliberate idioms
throughout the lib: guard returns, unauthorised-caller no-ops such as the
`set_privs` override, and uniform result passthrough such as `evaluate_result`
handling `1`, `0`, and strings alike. "Fails silently" is therefore not a
defect on its own. It qualifies only with a specific traced caller that is
harmed by the swallowed signal, named alongside the concrete outcome.

Dormant-by-design scaffolding is likewise intended: unused configuration keys,
partially-wired APIs, and pinned procedural-generation seeds, which exist to
make reboots reproducible.

## Async, await, and promises

The async surface has several behaviours that read like defects and are not,
and a few genuine defects that produce no error at all. Both directions cost
rounds of false or missed reports, so check against this list before reporting.

### Not defects

- **Fire-and-forget async is idiomatic.** A sync function calling an `async`
  helper and discarding the promise — `strike(tp, victim); return 1;` — is the
  normal shape, not a leaked promise. Command entry points and applies *cannot*
  be async (the driver reads their return immediately), so the chain is
  required to terminate somewhere. In MUD code the async work is usually a side
  effect on the world, not a value for the caller.
- **`acatch` wrapping a whole async body** is usually deliberate: it guarantees
  a floated promise cannot reject, since an unobserved rejection is reported to
  the debug log at deallocation.
- **`@returns {promise<T>}` above a function declared `async T`** is correct.
  The declared type is the payload; the driver supplies the wrapper.
- **An `async` function that never awaits** is legal. It returns an
  already-fulfilled promise.
- **A rejection appearing in `/log/catch`** is documented driver behaviour — an
  async body's uncaught error is reported like a caught error — not something
  the mudlib code added.

### Genuine defects that are silent

Each of these compiles cleanly, passes a live smoke test, and reports nothing.

- **`await` on a non-promise** passes the value straight through with no
  suspension. `await call_out("fn", 1)` returns the int handle instantly; only
  the delay-only `call_out(delay)` form yields a promise. Likewise
  `await ob->missing_fn()` awaits the `0` that call_other returns.
- **A `private` call_out target declared in an inherited file never fires.**
  Name-based dispatch does not cross the inherit boundary for privates, and
  there is no error — the chain simply stops. Chain steps in anything
  inheritable must be at least `protected`.
- **`call_back()` discards an async callback's promise.** It returns
  `catch(fun(...))`, so it reports success the moment the callback parks, before
  it has done anything. `async_call_back()` is the async-aware sibling.
- **An apply or entry point that merely *returns* the result of an async call**
  is not caught by the compiler's `async`-on-apply check. Consumers treat a
  returned promise as "no": `check_valid_path()` denies, master approval gates
  deny, the parser treats a verb function as having declined the command.

### Evidence required

Do not assert that a promise leaks, a frame never resumes, or a settle races
without tracing it. Suspension state is inspectable at runtime — `async_info()`
lists every parked frame with what it awaits, and awaited timers appear in
`call_out_info()` with `"<timer>"` in the function slot. Prefer that over
reasoning about the scheduler.
