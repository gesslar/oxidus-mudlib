---
title: Skills
description: How characters learn by doing -- the skill tree, the level cap, and reading a skill correctly.
---

Nobody in Oxidus spends skill points. There is no training hall and no
allocation screen. You get better at things by doing them, and the only thing
standing between you and a higher skill is your level.

## Skills Are a Tree

Skills nest. `combat` has `melee` under it, which has `slashing` under that, and
you address any of them with a dot path:

```lpc
tp->query_skill_level("combat.melee.slashing");
```

The tree that ships lives in `adm/etc/default.lpml` under `SKILLS.learnable`:

```text
combat
  defence   dodge, parry
  melee     attack, bludgeoning, piercing, slashing, unarmed
social      barter, charm, intimidate, persuade
general     appraise, hide, jump, listen, search, spot, swim
```

Add to that list and every character picks the new skills up. You can also
create one on the fly -- `add_skill()` will build any missing steps along the
path for you, and simply using a skill nobody has heard of creates it. That is
how `combat.defence.evade` exists despite not being in the list above.

### The Number Means Two Things

A skill level is a float, and both halves matter:

```text
   4.82
   ↑  ↑
   │  └─ 82% of the way to 5
   └──── you are skill level 4
```

The whole part is the level anything that cares actually uses. The fraction is
progress toward the next one. `query_skill_progress()` gives you that fraction
as a 0-99 number if you want to show a player a progress bar.

## Learning by Doing

One function drives all of it:

```lpc
tp->use_skill("general.swim");
```

Call that whenever a character does the thing. Combat already does it for you --
attackers train their weapon skill on every swing, defenders train their defence
skill every time somebody swings at them -- but anything else is up to you. If
you write a lockpicking command, call `use_skill()` in it.

Each call rolls. Most rolls fail and nothing happens, which is fine and normal.

The odd part, and it catches people out: **the better you are, the more often
the roll succeeds.** Improvement gets easier as you climb, not harder. What
slows you down is not the roll -- it is the cap.

### It Spreads Upward

When a roll does succeed, the game does not automatically improve the skill you
named. It picks one node from that skill *and all of its parents*:

```text
use_skill("combat.melee.slashing")
    ↓  one of these gets the progress
    ├── combat.melee.slashing   ← most likely
    ├── combat.melee
    └── combat                  ← least likely
```

The leaf is weighted heaviest and the root lightest, so your specific skill
climbs fastest while the general ones drift up behind it. Exactly one node
improves per successful roll.

This is why a character who has only ever used a sword still has *some*
`combat.melee` and *some* `combat` -- and why those broad skills, which feed into
things like your dual-wield chance, come along without ever being trained
directly.

## The Cap

Here is the actual pacing mechanism:

> **A skill cannot go past your level times `SKILLS.cap_factor`.**

Nothing else limits you. Progress rolls straight through level boundaries; it
only stops when it hits the ceiling. When a node is close to the cap, the amount
it can gain is trimmed to whatever room is left, so you creep the last bit rather
than jumping over.

If every candidate node is capped, a successful roll awards nothing at all.
Level up and the ceiling moves.

:::caution
The cap reads your **base** level, not your effective one. A boon or a spell that
raises your level makes you hit harder, but it will not let you train past your
real ceiling.
:::

## Reading a Skill

There are four functions to read a skill and picking the wrong one is the most
common mistake in this system. They vary on two questions: do you want the whole
number or the exact one, and should temporary buffs count?

|  | Exact float | Whole number |
|---|---|---|
| **Buffs ignored** | `query_raw_skill` | `query_raw_skill_level` |
| **Buffs counted** | `query_skill` | `query_skill_level` |

In practice:

- **Doing game maths?** `query_skill_level()`. Whole number, buffs count. This
  is what combat uses and it is almost always what you want.
- **Showing a player their real progress?** `query_raw_skill()`. The exact
  number, no buffs -- what they have actually earned.
- **Checking whether a skill exists at all?** `has_skill()`. It returns 1 or 0.
  Do not test the others against null for this; there is a reason below.

## An NPC's Skills Come From Its Level

Players earn their skills one use at a time. Monsters do not need to, and Oxidus
does not make them: **an NPC's skill in anything is derived from its level.**

That is the design, and it is the good kind of shortcut. A monster exists to be
a level-appropriate opponent for about ninety seconds. Hand-authoring a skill
tree for each one would be a lot of typing to arrive at "a level 3 wolf fights
like a level 3 wolf" -- so the level says that on its own.

What it means in practice is the nicest thing on this page:

```lpc
set_level(3.0);   // that is the skill setup. There is no step two.
```

Ask a monster for `query_skill_level()` on any skill at all -- one it has, one
it has never heard of -- and you get a number worked out from its level. Combat
reads those, which is why a monster with no skill setup whatsoever fights
perfectly well.

Two practical notes follow from it, and both are about which function to call
rather than anything conceptual:

- **Use `has_skill()` to test whether a skill exists.** The level-derived
  queries always answer, so "did it come back empty" is not a test on an NPC.
  `query_raw_skill()` and `query_raw_skill_level()` are the ones that read an
  NPC's actual stored tree.
- **Call `set_level()` before `add_skill()`**, always. Setting the level reseeds
  the whole tree from it, so anything you added first is quietly gone:

  ```lpc
  // Wrong -- set_level() reseeds the tree and eats the line above it.
  add_skill("combat.melee.slashing", 20.0);
  set_level(3.0);

  // Right.
  set_level(3.0);
  add_skill("combat.melee.slashing", 20.0);
  ```

  Nothing errors. You just get a monster that is not the one you wrote.

## Attributes

Six of them -- strength, dexterity, constitution, intelligence, wisdom, charisma
-- listed under `ATTRIBUTES` in config and starting at 5.

```lpc
int str = tp->get_attribute("strength");        // buffs included
int raw = tp->get_attribute("strength", 1);     // just the stored value
tp->modify_attribute("strength", 1);            // adjust by a delta
```

Worth knowing: **attributes are tracked but not wired into any formula yet.**
They take buffs, they save, they display -- but nothing in combat or skills reads
them. That is deliberate groundwork, not an oversight.

## Levels and Experience

Levels are the ceiling on everything above, so they matter here even though the
maths lives with [Combat](/systems/combat/#experience-and-levels).

```lpc
tp->query_level();             // your real level
tp->query_effective_level();   // level plus any temporary modifier
tp->query_xp();
tp->query_tnl();               // experience needed for the next one
```

Use `query_effective_level()` when a buff should count, and `query_level()` when
it should not -- which is exactly the distinction the skill cap makes.

## Adding a Skill

Most of the time, add it to `SKILLS.learnable` in config and you are done --
characters pick it up and `use_skill()` handles the rest.

To put one on a single character:

```lpc
tp->add_skill("general.lockpicking", 1.0);
```

Then call `use_skill("general.lockpicking")` wherever a character picks a lock.
That is the whole integration.

`use_skill()` takes an optional second argument that changes how much progress a
successful roll can award. Leave it off. It is an absolute bound rather than a
multiplier, so whether a number you pick is faster or slower than normal depends
entirely on where `SKILLS.default_gain` currently sits -- and some older call
sites pass literals that were chosen against a different default.

## Gotchas

- **Pick the right query.** `query_skill_level()` for maths, `query_raw_skill()`
  for showing progress, `has_skill()` for existence.
- **`set_level()` reseeds an NPC's skills, so call it first.** Custom skills
  added beforehand are silently overwritten -- you get no error, just a line of
  code that did nothing and a monster that is not what you wrote. Level, *then*
  customize.
- **On an NPC, use `has_skill()` for existence checks.** The level-derived
  queries always return a number, so a null test never fires.
- **The cap uses base level.** Level buffs do not raise it.
- **Using a leaf can improve its parents.** One node per successful roll, chosen
  by weight.
- **It gets easier, not harder.** The roll improves with skill; the cap is what
  paces you.
- **It is `defence`, everywhere.** `combat.defence.dodge`, `set_defence()`,
  `query_defence_amount()`. There is no `defense` anywhere in the lib.

## Key Files

| File | Role |
|---|---|
| `std/living/skills.lpc` | The tree, `use_skill()`, the cap, the four queries |
| `std/living/advancement.lpc` | A living's level and experience |
| `std/living/attributes.lpc` | The six attributes |
| `adm/daemons/advance.lpc` | Experience and level maths |
| `adm/etc/default.lpml` | `SKILLS` and `ATTRIBUTES` |
