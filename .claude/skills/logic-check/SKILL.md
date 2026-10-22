---
name: logic-check
description: Review a file, set of files, or system in Oxidus for logic flaws, errors, and crashers. Reports only concrete defects with a trigger path (or, if masked, what masks them) and evidence; no style, refactoring, or defensive suggestions, and no proposed fixes unless asked. Use when asked to logic-check, sanity-check, or hunt for bugs in specific code.
argument-hint: <file | files | system>
---

# Logic Check

Review the requested scope for logic flaws, errors, and crashers.

The scope may be a single file, a set of files, or a system. Stay within the
requested scope except where following dependencies or call sites is necessary
to establish behaviour.

## What to look for

Find concrete defects where the implementation may not behave as intended.

Determine intent from the code itself: function bodies, surrounding logic,
comments where useful, callers, and consumers of returned values.

Check for:

- Incorrect conditions, branches, comparisons, or boolean logic.
- Incorrect assumptions about values, types, state, or control flow.
- Code paths that produce the wrong result or unintended side effects.
- Missing or incorrect handling that can cause runtime errors or crashes.
- Mismatches between what a function does and what its callers expect.
- Mismatches between inputs supplied by callers and inputs expected by callees.
- Incorrect handling of return values.
- State transitions or sequencing that contradict the apparent intended
  behaviour.
- Reachable cases that cause invalid operations.

The test is whether the code does what it evidently means to do. Logic that is
wrong on its own terms — an off-by-one, a misaligned index range, an inverted
condition — is a finding even when other code currently cancels its effect.
That masking determines the severity, not whether the defect is reported.

## Verify behaviour

Do not infer API behaviour when it can be established from the codebase or
documentation.

When relevant:

- Inspect call sites to determine how a function is actually used and what its
  callers expect (`lpc_references`, or `rg` over the lib).
- Inspect called functions when their behaviour is necessary to determine
  whether the reviewed code is correct (`lpc_definition`).
- Check simul_efun implementations in `/adm/simul_efun/` (`.lpc`) for their actual
  behaviour.
- Check efun documentation (`fluffos_doc_lookup`) to confirm accepted inputs,
  return values, error behaviour, and other semantics relevant to a suspected
  defect.

Follow these dependencies only as far as necessary to establish whether a
concrete flaw exists.

## Scope discipline

This is not a general code review.

The scope is the requested code as it stands, not a change. A defect is in
scope whether or not it is new; the change-scoping rules of a PR review do not
apply.

Do not report:

- Style or formatting preferences.
- Refactoring opportunities.
- Architectural improvements.
- Defensive programming suggestions without a concrete failure case.
- Hypothetical concerns that are not reachable from the reviewed code.
- Issues outside the requested scope unless they directly cause a defect
  within it.

Do not assume that unusual behaviour is incorrect. Establish the intended
behaviour from the implementation and its usage before reporting it. The
**lpc-semantics** skill lists the LPC/FluffOS semantics most often mistaken for
defects — check a suspected finding against it.

## Findings

Report only concrete findings.

For each finding, identify:

1. Where the defect is (`file:line`).
2. What the code currently does.
3. Why that conflicts with the apparent intended behaviour.
4. A concrete path or circumstance that triggers the problem — or, for a
   masked defect, what is masking it and what change would expose it.
5. The resulting incorrect behaviour, error, or crash, or that the defect is
   currently masked.

If supporting evidence comes from a call site, simul_efun implementation, or
efun documentation, include that evidence.

Do not propose changes unless asked.

If no concrete logic flaws are found, say so.
