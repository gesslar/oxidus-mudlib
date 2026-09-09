---
title: Monsters
description: Building NPCs -- data files, levels, loot, procs, and putting them in the world.
---

Almost every monster in Oxidus is a data file. You write what it is called, what
level it is, and what it drops, and the game builds you a working creature that
fights, dies, and hands out loot. There is no LPC to write.

The whole lib contains exactly one hand-coded NPC, and seventeen data-driven
ones. That ratio is the recommendation.

## Your First Monster

Drop a `.lpml` file in `d/mobs/`:

```lpml title="d/mobs/marsh_toad.lpml"
{
  type: "monster",
  name: "marsh toad",
  short: "a bloated marsh toad",
  long: "A toad the size of a dog, slick with brackish water and breathing "
        "in slow, wet heaves.",
  id: ["toad", "marsh toad"],
  level: [3, 5],
  race: "amphibian",
  weapon name: "tongue",
  weapon type: "bludgeoning",
}
```

That is a complete, working monster. It has hit points, it fights back, it can
be killed, and it leaves a corpse.

| Field | What it does |
|---|---|
| `type` | **Required.** Picks the base class -- see [Types](#types) below |
| `name` | What the game calls it internally |
| `short` | The one-line description in a room |
| `long` | What a player sees when they `look` at it |
| `id` | What a player can type to refer to it |
| `level` | A number, or `[low, high]` for a range rolled per spawn |
| `gender` | A string, a list to pick from, or a weighted mapping |
| `race` | Mostly flavour -- see [Race](#race) |
| `weapon name` | What its attacks are called. Defaults to `fist` |
| `weapon type` | The damage type it deals. Defaults to `bludgeoning` |
| `damage` | Base damage. Leave it out and the level decides |
| `attack speed` | Seconds between swings |
| `plus attack speed` | Adjusts the default -- positive makes it **faster** |
| `loot` | See [Loot](#loot-and-coins) |
| `loot chance` | Default drop chance for the `loot` list |
| `coins` | See [Loot](#loot-and-coins) |
| `simple procs` | See [Procs](#procs) |
| `proc chance` | How often a proc fires at all |

### Types

`type` names a base class in `std/mobs/`. Four exist:

| Type | For |
|---|---|
| `monster` | The generic base. Use this unless one of the others fits |
| `mammal` | Furred things |
| `rodent` | Small scurrying things |
| `insect` | Insects |

A `type` with no matching file in `std/mobs/` means the monster does not compile
at all, so check the name.

## Level Is the Whole Setup

The single most useful thing to know here: **a monster's skills come from its
level.** You do not give a monster combat skills, defence skills, or anything
else. Set the level and it is done.

```lpml
level: 3,
```

A level 3 monster fights like a level 3 monster. That is deliberate -- monsters
exist to be level-appropriate opposition, so authoring a skill tree for each one
would be a great deal of typing to arrive somewhere the level already describes.
See [Skills](/systems/skills/#an-npcs-skills-come-from-its-level).

A range rolls fresh on every spawn, which is usually what you want:

```lpml
level: [3, 5],
```

Both ends are included, so `[3, 5]` gives 3, 4, or 5. An area's `spawn.lpml`
rolls its range the same way.

## Loot and Coins

Two fields, both optional. Give the monster a list of things it might drop and a
chance that applies to each:

```lpml
loot: [
  "/obj/loot/toad_skin.loot",
  "/obj/loot/swamp_gland.loot",
],
loot chance: 60.0,
coins: {
  copper: [12, 100.0],
  silver: [2, 25.0],
},
```

Coins read as `[amount, chance]`. Each entry is rolled on its own, so the toad
above always drops twelve copper and drops two silver a quarter of the time.

For per-item chances, use a mapping instead of a list, or nest pairs inside it:

```lpml
loot: {
  "/obj/loot/toad_skin.loot": 80.0,
  "/obj/loot/rare_gizzard.loot": 5.0,
},
```

The items themselves are data files too. See [Loot](/systems/loot/).

## Procs

A proc is a special attack a monster sometimes uses instead of swinging
normally. Give it a tag, the message players see, and a damage type:

```lpml
simple procs: [
  {
    tag: "gore",
    messages: [
      "{{630}}$N $vgore $t with $p short tusks.{{res}}",
    ],
    damage_type: "piercing",
    attack_type: "melee",
    cooldown: 8,
    weight: 100,
    severity: "normal",
  },
],
```

The message uses action tokens -- `$N` is the monster, `$t` is the target, `$v`
conjugates the verb so each reader sees it correctly, and `$p` is a possessive.
Give two messages and the first goes to the target while the second goes to
everyone else. The `action-messages` skill has the full token list.

`severity` scales the damage, `cooldown` is seconds before it can fire again,
and `weight` decides how often it is picked when several are eligible. The proc
still has to hit -- it rolls against the target's defences like any other attack.

:::note
`simple` is currently the only proc type, and it is deliberately a starting
point -- one attack, one message, one damage type. Curses, damage over time, and
area effects are not built yet.
:::

## Race

`race` takes any string:

```lpml
race: "amphibian",
```

If a matching module exists in `std/modules/race/`, the monster gets that race's
body parts and recovery rates. If not -- and for every animal race in the lib
today, there is not -- the name is simply recorded and nothing else happens. No
error, no missing behaviour.

So on a monster, race is mostly identity. It becomes load-bearing only when you
write a module for it. The ones that exist are the playable races: human, elf,
dwarf, gnome, orc, troll.

## Putting Monsters in the World

There is no central spawner. Areas do it themselves, which keeps the pacing of
an area in the area that owns it. The forest is the pattern worth copying:
a weighted pool in a data file, and rooms that draw from it.

```lpml title="d/forest/spawn.lpml"
[
  { path: "mob/crimson_fox", level: [5, 8],  weight: 4 },
  { path: "mob/shadow_wolf", level: [8, 10], weight: 2 },
]
```

```lpc
object mob = add_inventory(file);

mob->set_level(level);
mob->simple_action("$N $vemerge from the undergrowth.");
```

Note the path: **`mob/<name>`**, with no leading slash and no extension.

File paths in FluffOS resolve from the mudlib root, so `mob/crimson_fox` *is*
`/mob/crimson_fox` -- and because `mob` is the first directory in it, the
request goes to the monster compiler, which looks up `d/mobs/crimson_fox.lpml`
by filename. There is no `/mob/` directory on disk; the path is purely a routing
instruction. That is what lets any area draw from the shared monster pool
without copying anything.

```lpc
new("mob/crimson_fox");   // builds from d/mobs/crimson_fox.lpml
```

A `.mob` extension routes there too, and is the way to reach the compiler from a
path that does not start with `mob/`. Either form works; the areas in the lib
all use the short one.

Also note the ordering -- `set_level()` is called on the new monster, *after*
creation. That is correct and deliberate, and it matters:

:::caution
**`set_level()` reseeds a monster's skills, so it must come first.** If you set
skills by hand and then set the level, the level wins and your skills are gone.
Nothing errors -- you just get a monster that is not the one you wrote.

```lpc
set_level(3.0);                              // level first
add_skill("combat.melee.slashing", 20.0);    // then anything custom
```

:::

## When Data Is Not Enough

Some things a data file cannot express -- a monster that talks back, guards
something, or runs its own logic. Then you write one:

```lpc
inherit STD_NPC;

void setup() {
  set_name("the gatekeeper");
  set_short("a scarred gatekeeper");
  set_id(({ "gatekeeper", "keeper" }));
  set_level(12.0);
}
```

Everything from the data path is available here -- `add_loot()`, `add_coin()`,
`set_weapon_type()`, and the rest are ordinary functions. The data file is a
convenient front end to exactly these calls.

`d/village/manor/mon/arcanist.lpc` is the lib's only example, and worth reading
before you write your own.

## Things Worth Knowing

- **Monsters stop thinking in empty rooms, once they are healthy.** A monster
  stops its heartbeat only when the room is empty *and* it is back to full
  health -- so a wounded one heals up first, then goes quiet. After that: no
  recovery, no AI, no boon expiry. Anything that must run whether or not
  somebody is watching belongs in a daemon, not on a monster.
- **`set_level()` before custom skills.** See the caution above.
- **A monster remembers who attacked it, by name.** Walk back in and it comes
  for you -- through your death, revival, and relog. See
  [Combat](/systems/combat/#npcs-in-a-fight).
- **`type` must match a file in `std/mobs/`** or the monster silently fails to
  compile.
- **Monsters do not persist.** Nothing about one is saved. A reboot brings back
  a fresh one with no memory.

## Key Files

| File | Role |
|---|---|
| `d/mobs/*.lpml` | The monster data files |
| `std/mobs/monster.lpc` | Reads the data and builds the monster |
| `std/mobs/mammal.lpc`, `rodent.lpc`, `insect.lpc` | The other `type` bases |
| `std/living/npc.lpc` | NPC behaviour -- heartbeat, death, `set_level()` |
| `std/living/proc.lpc` | Simple procs |
| `std/living/race.lpc` | `set_race()` |
| `adm/daemons/modules/virtual/mob.lpc` | Turns a `mob/<name>` path into a monster |
| `d/forest/` | A worked example of area spawning |
