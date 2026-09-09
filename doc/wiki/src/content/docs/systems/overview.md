---
title: Overview
description: Overview of Systems in Oxidus
sidebar:
  order: 0
---

Oxidus is a set of interconnected systems that handle everything from
communication to combat to world generation. The pages in this section cover
them one at a time; this page is the map.

## Documented Here

| System | What it covers |
|---|---|
| [Alarms](/systems/alarm/) | Wall-clock scheduling -- boot, hourly, daily, weekly, monthly, yearly, and one-shot events |
| [Async](/systems/async/) | `async` and `await` -- letting slow things take their time without stopping the game |
| [Colour](/systems/colour/) | True colour markup, text attributes, wrapping, and the accessibility floor |
| [Combat](/systems/combat/) | Rounds, hitting, damage, armour, death, and what you can tune |
| [Configuration](/systems/config/) | Cascading LPML config through `CONFIG_D` and `mud_config()` |
| [Currency](/systems/currency/) | Denominations, conversion, wealth storage, and transactions |
| [GMCP](/systems/gmcp/) | Out-of-band client communication in both directions |
| [Loot](/systems/loot/) | Item and coin drops from NPCs |
| [Monsters](/systems/monsters/) | Building NPCs from data files -- levels, loot, procs, and spawning |
| [Signals](/systems/signal/) | The publish/subscribe event bus |
| [Skills](/systems/skills/) | Learning by doing, the level cap, and reading a skill correctly |

The [Daemons](/daemons/alarm/) section holds per-daemon API references for the
same subsystems.

## The Rest of the Lib

These are shipped and working, but do not have wiki pages yet. Each has a skill
under `.claude/skills/` that documents it in full.

**Boons and curses** -- temporary modifiers layered over attributes, vitals, and
skills, with stacking rules and expiry.

**Objects and the world** -- rooms, exits, doors, terrain, and zones; the
identification system that resolves what a player means by "the sharp tusk";
mass, capacity, and containers with a transactional move; equipment across a
body-part slot system; and consumables.

**Content without code** -- items and NPCs can be defined entirely in LPML data
files. A virtual compile layer reads the data, picks the right base class, and
produces a working object at load time. Virtual *areas* go further, generating
rooms procedurally from a coordinate space.

**Messaging** -- the `tell` family walks the containment hierarchy, action
messages conjugate verbs and resolve pronouns per recipient, and system feedback
(`_ok`, `_error`, `_warn`, `_info`) carries its own voice and decoration.

**Persistence** -- marked variables are saved and restored automatically.
Daemons opt in with `set_persistent()`; inventories serialize recursively.

**External integration** -- an HTTP client and server, a WebSocket client, a
SQLite database layer, a Discord bot, the Grapevine inter-MUD network, and MSSP.
