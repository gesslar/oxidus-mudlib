---
title: Combat
description: How fights run -- rounds, hitting, damage, armour, death, and what you can tune.
---

Combat in Oxidus runs itself. Once two livings are fighting, a round fires on a
timer, picks a target, swings, works out whether it landed and how much it hurt,
and schedules the next one. You do not drive that loop -- you start a fight and
the loop takes it from there.

Most of what a builder does with combat is *feeding* it: giving an NPC a level
and a weapon, putting armour on a player, occasionally writing an ability that
hits someone. This page is about what happens in between.

## A Round

Every combatant runs its own round on its own timer. One round looks like this:

1. Is this body dead, or out of hit points? Then stop fighting.
2. Drop any enemies who have died or left.
3. Pick the enemy with the most **threat** -- see [below](#who-you-swing-at).
4. Swing at them.
5. Tell the player's client what is going on, via GMCP.
6. Book the next round.

The gap between rounds is the body's **attack speed**, in seconds. Two things
about the functions that change it are worth knowing before you use them:

```lpc
tp->set_attack_speed(2.0);   // seconds between rounds
tp->add_attack_speed(0.5);   // makes you FASTER -- it shortens the gap
```

`add_attack_speed()` subtracts from the interval, so a positive number speeds
you up. Both clamp the result to somewhere sane, so you cannot buff yourself
into a hundred swings a second.

### Swinging Twice

If you have something in your off hand, each swing rolls for a second one. The
chance starts at a flat base and creeps up with your `combat.melee` skill. Get
the extra swing and it uses a weapon from your off-hand slots rather than the
main one.

No off-hand weapon means no roll -- this is not a general "attack twice" chance,
it is specifically dual wielding.

## Starting and Stopping a Fight

```lpc
attacker->start_attack(victim);
```

That is the whole thing. It registers the victim, kicks off the round timer, and
-- importantly -- calls `start_attack()` back the other way, so the victim
fights back. NPCs also record the attacker at this point, which is what makes
them jump you next time you walk in.

`stop_all_attacks()` ends it: cancels the timer, forgets the current enemies,
clears the client's combat display.

Before you start a fight, ask whether you are allowed to:

```lpc
mixed err = tp->prevent_combat(victim);

if(stringp(err))
  return err;    // a reason you cannot -- show it to the player
```

It says no if the victim is peaceful, if the victim refuses combat, or if the
room does not allow fighting. Rooms where you do not want brawling should set
that rather than relying on nobody trying.

## Who You Swing At

You can be fighting several things at once, so something has to choose. That
something is **threat**, and threat is simply damage dealt. Whoever has hurt you
most is who you turn on.

| Function | What it does |
|---|---|
| `add_threat` | Bump an enemy's threat -- combat does this automatically with the damage dealt |
| `highest_threat` | Who you are swinging at right now |
| `lowest_threat` | The other end, if you want it |
| `add_seen_threat` | A longer memory that survives the fight ending |

There is no separate aggro stat and no taunt mechanic. If you want something to
pull threat, it has to do damage -- or you add threat by hand.

## Whether You Hit

`can_strike()` decides. It starts from a configured base chance and adjusts for:

- the **level gap** between you and your target
- your **skill** with whatever you are attacking with
- their **armour class**, which counts double
- their **defence skill** -- `combat.defence.dodge` normally, or
  `combat.defence.evade` against arcane attacks
- whether they are **out of movement points**, which makes them much easier to hit

Then -- and this part matters -- the result is squashed through a curve before
the roll. Piling on more bonuses gives you less and less, and the number never
quite reaches 0% or 100%. A hopelessly outmatched attacker still lands the
occasional blow, and no amount of stacking makes you unmissable.

Whether the blow lands or not, **the defender trains their defence skill.**
Getting attacked is how you learn to not get hit.

## How Much It Hurts

Damage is worked out twice: once on the way out, once on the way in.

**On the way out**, the attacker's side adds up a small configured base with
some random variance, the weapon's own damage, their skill with it, and half
their general `combat` skill -- then subtracts the target's defence skill. A
result that comes out negative is turned into 1; a result of exactly zero is
left alone, so a hit can land for nothing.

**On the way in**, the target's side takes that number and reduces it by:

- **armour of the matching damage type** -- slashing armour helps against
  slashing
- a **level adjustment**, so a higher-level attacker cuts through armour better
  than a lower-level one does
- anything a **shield effect** absorbs

What is left comes off hit points, and the game notes who did it -- which is how
it knows who to give the experience to if that was the killing blow.

:::note
The exact numbers here are tuning knobs and they move. Read them from
`adm/etc/default.lpml` under `COMBAT` -- `BASE_DAMAGE`, `DAMAGE_VARIANCE`,
`DEFAULT_HIT_CHANCE`, `DAMAGE_LEVEL_MODIFIER` -- rather than copying a number
into your own code, or you will be wrong the next time somebody tunes it.
:::

## Armour

You do not set armour on a body. You put armour *on* it, and the body works out
its own totals whenever equipment changes.

An armour item carries two things: an **AC**, which makes you harder to hit at
all, and a **defence mapping** of damage types to amounts, which makes the hits
you do take hurt less:

```lpc
set_ac(2.0);
set_defence(([ "slashing": 15.0, "piercing": 10.0 ]));
```

Wear it and those get folded into the body's totals. Take it off and they come
back out. `query_ac()` and `query_defence_amount(type)` read the result.

## Weapons and Procs

A weapon carries a name, a damage type, and a base damage. An NPC with no weapon
at all falls back to its own settings:

```lpc
set_weapon_name("claws");
set_weapon_type("slashing");
set_damage(6.0);
```

**Procs** are the special effects -- a weapon that sometimes burns, an NPC that
sometimes does something nastier than a normal swing. Register one and combat
rolls for it after every hit that lands:

```lpc
add_proc("scorch", ([
  "function" : "scorch_proc",
  "cooldown" : 10,
  "weight"   : 100,
]));
```

`cooldown` is seconds before it can fire again; `weight` decides how often it is
picked when several procs are eligible. The proc function gets the attacker and
the victim.

NPCs can carry procs directly rather than on a weapon. That is a deliberately
simple system for now -- one proc type, driven from the NPC's data file. See the
`npc-creation` skill.

## Vitals and Getting Better

Three pools, all floats, all starting at 100:

| Pool | What it is |
|---|---|
| **HP** | Hit points. Zero means dead. |
| **SP** | Skill points -- what spells and abilities cost to cast. |
| **MP** | Movement points. Walking costs them, and so does every swing you take. |

Recovery ticks on the heartbeat. **HP and SP do not come back while you are
fighting. Movement points do** -- which is deliberate, because bottoming out on
MP makes you drastically easier to hit, and you would never dig out of that hole
mid-fight otherwise.

Rates come from the race module. A living with no race module still recovers, at
a built-in fallback rate -- and NPCs recover considerably faster than players do.

`query_condition_string()` gives you the words players see -- "wounded",
"exhausted", "fully charged" -- rather than raw numbers.

## Dying

When something's hit points hit zero, it is spotted on the next heartbeat and
`die()` runs:

1. Stop fighting.
2. Announce it to the room, and save the character.
3. Fire the `SIG_PLAYER_DIED` signal -- see [Signals](/systems/signal/).
4. Make a corpse.
5. Roll [loot and coins](/systems/loot/) into the body.
6. Tip everything the body was carrying, plus its money, into the corpse.
7. Drop the corpse in the room.
8. **A player** becomes a ghost. **An NPC** hands its killer the experience and
   is destroyed.

There is a small gap between hit points reaching zero and the heartbeat noticing.
Nothing goes wrong in it, but do not write code that assumes death is instant.

## Experience and Levels

Kill something and you get experience based on **its** level, not yours, with a
little randomness on top. The gap between you matters: kill things far below you
and the reward is cut, kill things above you and it goes up. The thresholds and
the size of each adjustment are config keys -- `OVERLEVEL_THRESHOLD`,
`OVERLEVEL_XP_PUNISH`, `UNDERLEVEL_THRESHOLD`, `UNDERLEVEL_XP_BONUS`.

Experience needed per level is `BASE_TNL * TNL_RATE^(level - 1)`. It is a curve,
not a line -- each level costs `TNL_RATE` times the one before, so raising that
key makes the whole game longer. Ask for a real number rather than working one
out yourself:

```lpc
int needed = ADVANCE_D->to_next_level(tp->query_level());
```

With `PLAYER_AUTOLEVEL` on, which is the default, players level the moment they
have the experience. To hand out experience from a quest or anything else that
is not a kill:

```lpc
ADVANCE_D->earn_xp(tp, 250);
```

## NPCs in a Fight

Two things about NPCs will surprise you if nobody tells you.

**NPCs stop thinking in empty rooms -- once they are healthy.** An NPC only
starts a heartbeat when a player is present, and only stops one when the room is
empty *and* it is back to full health. So a wounded monster left alone keeps
ticking long enough to heal, then goes quiet. Once it has, there is no recovery,
no AI, and no boon expiry. If you want something happening in the background whether or not anyone
is watching, an NPC heartbeat is the wrong place for it. Use a daemon.

**`set_level()` on an NPC reseeds its skills, so it goes first.** An NPC's
skills are derived from its level by design -- which is why a monster needs no
skill setup at all -- but it does mean setting the level rewrites the tree.
Anything you added beforehand is overwritten silently:

```lpc
set_level(12.0);                              // do this first
add_skill("combat.melee.slashing", 20.0);     // then this
```

**NPCs remember who hit them, and they remember you by name.** Attack one, walk
out, walk back in, and it comes straight for you with two immediate swings before
the normal round timer even starts.

The by-name part is what makes it stick. The NPC records the string, not a
reference to your body -- and dying, being revived, or logging out and back in
all hand you a *brand new* body object with the same name written on it. None of
those lose you the grudge. You do get a pass while you are actually dead, since
ghosts are skipped: nothing jumps you on the walk back to your corpse.

What clears it is the NPC itself going away. The grudge lives on that object and
nowhere else, so a reboot -- or a `renew` on the NPC, or anything else that
replaces it -- brings back something that has never met you.

## Abilities With a Wind-Up

Anything that takes time to go off -- a spell with a cast time, an ability with
a wind-up -- uses `async_act()`. It registers a proper act, so walking away or
being interrupted cancels it the way a player expects:

```lpc
if(!await tp->async_act("punch", 2.0))
  return;    // interrupted -- drop it quietly
```

That `0` means somebody or something stopped you. Treat it as "abandon
silently", not as an error worth reporting.

The command itself must **not** be async. See [Async](/systems/async/) for the
full picture and why.

## Gotchas

- **Death is noticed on the heartbeat**, not at the moment damage lands.
- **No recovery of HP or SP while fighting.** Movement points are the exception.
- **NPCs in empty rooms are asleep.** No heartbeat, no anything.
- **`set_level()` reseeds an NPC's skills, so call it first.** Anything added
  beforehand is overwritten with no error -- just a line that did nothing.
- **An NPC's skills come from its level**, by design -- so a monster needs no
  skill setup at all. See [Skills](/systems/skills/).
- **Threat is just damage.** No taunts, no aggro table beyond who hurt you most.
- **Attack speed is a gap, not a rate.** `add_attack_speed()` makes you faster.

## Key Files

| File | Role |
|---|---|
| `std/living/combat.lpc` | The round loop, threat, hit chance, AC |
| `std/living/damage.lpc` | Working out and applying damage |
| `std/living/vitals.lpc` | HP, SP, MP, recovery, condition strings |
| `std/living/advancement.lpc` | A living's own level and experience |
| `std/living/npc.lpc` | NPC heartbeat, death checking, `set_level()` |
| `std/living/proc.lpc` | NPC procs |
| `std/ext/proc.lpc` | Weapon procs |
| `std/equip/armour.lpc` | AC and per-type defence on armour |
| `std/modules/mob/combat_memory.lpc` | NPCs remembering who hit them |
| `adm/daemons/advance.lpc` | Experience and level maths |
| `adm/daemons/death.lpc` | Death logging |
| `adm/etc/default.lpml` | The `COMBAT` block and the levelling keys |
