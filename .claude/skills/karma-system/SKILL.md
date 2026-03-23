---
name: karma-system
description: Understand and work with the karma (death penalty) system in Threshold RPG. Karma is a debt of XP incurred on death, automatically repaid from future XP earnings. Consult when modifying death penalties, XP flow, or karma-related code.
---

# Karma System

Karma is the death penalty system. When a player dies, they accrue a karma debt — a pool of XP they owe. While indebted, 50% of all earned XP goes to paying off the debt. Players can also manually pay with `xp karma <amount>`.

## Key Concepts

- **Karma is not created if**: player is below guild level 5, `free_death` toggle is on, player has forgiveness/boon/cycle of life, killed by invader or PvP, or vengeance/kismet triggers
- **Karma amount**: 50% of XP required for the player's next level
- **Karma cap**: maximum 4x the base amount (200% of level cost) across accumulated deaths
- **Repayment**: automatic 50/50 split on all earned XP while indebted
- **Repayment order**: karma is deducted BEFORE the XP split system (expertise, blood favor, etc.)
- **Karma Bitch toggle**: when enabled, adds 15% bonus to repayment (debt shrinks faster, player loses same XP)

## Core API (`KARMA_D` / `adm/daemons/karma.c`)

```c
KARMA_D->query_karma(player)                    // Current debt (int)
KARMA_D->add_karma(player, amount)              // Incur karma (caps at max)
KARMA_D->pay_karma(player, amount)              // Pay down debt (returns 0 when fully paid)
KARMA_D->query_karma_by_level(level, rebirth)   // Calculate karma for a death at this level
KARMA_D->query_karma_repayment_divisor()        // Returns 2 (the 50/50 divisor)
KARMA_D->delta_karma(player, amount)            // Adjust by delta
KARMA_D->set_karma(player, amount)              // Set exact value
KARMA_D->wipe_player_karma(player)              // Reset to 0
KARMA_D->query_karma_incurred(player)           // Lifetime total incurred
KARMA_D->query_karma_paid(player)               // Lifetime total paid
```

## Where Karma Happens in the Code

| Event | File | What Happens |
|---|---|---|
| Player dies | `std/ghost.c` (in `revive()`) | Checks exemptions, calls `KARMA_D->add_karma()` |
| Player earns XP | `adm/daemons/xp.c` (in `award_xp()`) | Splits XP 50/50, calls `KARMA_D->pay_karma()` |
| Player pays manually | `cmds/std/_xp.c` | `xp karma <amount>` trades XP for karma reduction |
| Toggle bonus | `adm/daemons/karma.c` (in `pay_karma()`) | Checks `karma_bitch` toggle for 15% bonus |

## XP Flow with Karma

```
Earned XP
  → Bonuses applied (buffs, double XP, etc.)
  → Karma check: if debt > 0, split 50/50
    → Half pays karma
    → Half continues as player XP
  → XP split system (expertise, blood favor, etc.)
  → Player receives final amount
```

## Death Exemption Cascade (in `ghost.c`)

Checked in order — first match prevents karma:

1. `free_death` toggle (global, admin-controlled)
2. Forgiveness (time-limited, from Loyalty system)
3. Killed by invader (invasion monsters)
4. Killed by player (PvP)
5. Boon (stored count, decremented on use)
6. Cycle of Life (annual character anniversary, one-time)
7. Vengeance (10% random chance)
8. Kismet (starts low, +10% per consecutive death, caps 50%, resets on trigger)

## Constants

| Constant | Value | Location |
|---|---|---|
| `karma_percentage_of_level` | 50 | `karma.c` — % of next-level cost per death |
| `max_karma_factor` | 4 | `karma.c` — max karma as multiple of base |
| `karma_repayment_divisor` | 2 | `karma.c` — 50/50 split ratio |
| `karma_bitch.rate` | 15.0 | `lib/toggle/karma_bitch.json` — bonus % on repayment |

## Data Storage

The daemon stores three persistent mappings (saved to disk):
- `karma[player_name]` — current debt
- `karma_incurred[player_name]` — lifetime incurred
- `karma_paid[player_name]` — lifetime paid

## Key Files

```
adm/daemons/karma.c                — Karma daemon
std/ghost.c                         — Death/revive (karma creation)
adm/daemons/xp.c                   — XP awards (karma deduction)
cmds/std/_xp.c                     — Player command
cmds/wiz/_karma.c                  — Admin command
lib/toggle/karma_bitch.json        — Repayment bonus toggle
doc/help/karma.help                — Player help
doc/help/death_consequences.help   — Death system help
```
