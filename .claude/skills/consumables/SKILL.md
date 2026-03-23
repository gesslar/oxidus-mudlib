---
name: consumables
description: Understand and work with the consumable system in Oxidus. Covers the M_USES base module, M_EDIBLE (food/eat/nibble), M_POTABLE (drink/sip), STD_FOOD, STD_DRINK, action message customization, and creating food and drink items.
---

# Consumables Skill

You are helping work with the Oxidus consumable system. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
M_USES (std/modules/uses.c)         — base use-count tracking
  ├── M_EDIBLE (std/modules/edible.c)  — eat/nibble mechanics
  └── M_POTABLE (std/modules/potable.c) — drink/sip mechanics

STD_ITEM + M_EDIBLE  → STD_FOOD (std/consume/food.c)
STD_ITEM + M_POTABLE → STD_DRINK (std/consume/drink.c)
```

## M_USES — `std/modules/uses.c`

Base module tracking consumable quantities.

### Properties

- `int _max_uses` — maximum available uses
- `int _uses` — current remaining uses
- `string _use_status_message` — optional custom status message

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_uses` | `int set_uses(int uses)` | Set current uses; initializes `_max_uses` if null |
| `query_uses` | `int query_uses()` | Get remaining uses |
| `query_max_uses` | `int query_max_uses()` | Get maximum uses |
| `adjust_uses` | `mixed adjust_uses(int uses)` | Adjust by delta; returns null if would go negative or exceed max |
| `reset_uses` | `void reset_uses()` | Reset `_uses` to `_max_uses` |
| `set_use_status_message` | `void set_use_status_message(string msg)` | Set custom status message |
| `query_use_status_message` | `string query_use_status_message()` | Get status message |

## M_EDIBLE — `std/modules/edible.c`

Inherits `M_USES`. Adds eat/nibble mechanics with customizable action messages.

### Properties

- `int _edible` — flag (1 = edible)
- `mapping _actions` — custom action messages per action type

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_edible` | `int set_edible(int edible)` | Mark as edible |
| `is_edible` | `int is_edible()` | Check edibility |
| `consume` | `int consume(object tp)` | Eat entirely — depletes all remaining uses |
| `nibble` | `int nibble(object tp, int amount)` | Eat specified amount |
| `reset_edible` | `void reset_edible()` | Calls `reset_uses()` |

**Validation hooks** (called by command system):
- `try_to_eat(object ob, string arg)` — checks object is in user's environment
- `direct_eat_obj(object ob, string arg)` — command validation for `eat`
- `direct_nibble_obj(object ob, string arg)` — command validation for `nibble`

### Action Message Customization

Each action type (`"consume"`, `"nibble"`) supports three message slots:

| Setter | Purpose |
|---|---|
| `set_consume_action(string)` | Combined message (overrides self + room) |
| `set_self_consume_action(string)` | Message to the consumer only |
| `set_room_consume_action(string)` | Message to the room only |
| `set_nibble_action(string)` | Combined nibble message |
| `set_self_nibble_action(string)` | Nibble message to consumer |
| `set_room_nibble_action(string)` | Nibble message to room |

**Default messages:**
- Consume: `"$N $veat a $o."`
- Nibble: `"$N $vnibble on a $o."`

**Display logic:**
- If combined `action` is set → `tp->simple_action(action)`
- If both self and room are null → `tp->simple_action(default)`
- Otherwise → `tp->my_action(self_msg)` + `tp->other_action(room_msg)`

## M_POTABLE — `std/modules/potable.c`

Inherits `M_USES`. Adds drink/sip mechanics. Mirrors M_EDIBLE's structure.

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_potable` | `int set_potable(int potable)` | Mark as drinkable |
| `is_potable` | `int is_potable()` | Check potability |
| `drink` | `mixed drink(object user)` | Drink entirely |
| `sip` | `mixed sip(object user, int amount)` | Sip specified amount |
| `reset_potable` | `void reset_potable()` | Calls `reset_uses()` |

### Action Message Customization

| Setter | Purpose |
|---|---|
| `set_drink_action(string)` | Combined drink message |
| `set_self_drink_action(string)` | Drink message to drinker |
| `set_room_drink_action(string)` | Drink message to room |
| `set_sip_action(string)` | Combined sip message |
| `set_self_sip_action(string)` | Sip message to sipper |
| `set_room_sip_action(string)` | Sip message to room |

**Default messages:**
- Drink: `"$N $vdrink a $o."`
- Sip: `"$N $vsip from a $o."`

**Error returns:**
- `"You can't drink that."` — not potable
- `"There is nothing left to drink."` — no uses remaining

## STD_FOOD — `std/consume/food.c`

Inherits `STD_ITEM` + `M_EDIBLE`. Ready-to-use food inheritable.

**Automatic behavior:**
- Calls `set_edible(1)` in `mudlib_setup()`
- Marks `_uses`, `_max_uses`, `_use_status_message` for persistence via `save_var()`
- Auto-adds `"food"` to IDs when `set_id()` is called
- Adds a `"consume"` extra_long description showing consumption status
- Auto-removes item when uses reach 0

**Status messages** (based on remaining percentage):
- 100%: `"This [name] hasn't been touched."`
- 80–99%: `"This [name] has been nibbled on."`
- 50–79%: `"A lot of this [name] has been eaten."`
- 25–49%: `"Most of this [name] has been eaten."`
- 0–24%: `"There is very little left of this [name]."`

## STD_DRINK — `std/consume/drink.c`

Inherits `STD_ITEM` + `M_POTABLE`. Ready-to-use drink inheritable.

**Automatic behavior:**
- Calls `set_potable(1)` in `mudlib_setup()`
- Marks `_uses`, `_max_uses`, `_use_status_message` for persistence via `save_var()`
- Auto-adds `"drink"` to IDs when `set_id()` is called
- Adds a `"consume"` extra_long description showing consumption status
- Auto-removes item when uses reach 0

**Status messages** (based on remaining percentage):
- 100%: `"This [name] is full."`
- 80–99%: `"This [name] has barely been touched."`
- 50–79%: `"A lot of this [name] has been drunk."`
- 25–49%: `"Most of this [name] has been drunk."`
- 0–24%: `"There is very little left of this [name]."`

**Additional validation:**
- `direct_drink_obj` / `direct_sip_obj` — checks item is in user's inventory
- `e_drink_obj` / `e_sip_obj` — execute drink/sip; show "have drunk the last" message if depleted

## Commands

**Eat** (`cmds/std/eat.c`): Finds target in inventory, checks `is_edible()`, checks uses, calls `consume()`.

**Drink** (`cmds/std/drink.c`): Finds target in inventory, checks `is_drink()`, checks uses, calls `drink()`.

## Creating Consumables

### Simple Food

```lpc
inherit STD_FOOD;

void setup() {
  set_id("muffin");
  set_name("muffin");
  set_short("a muffin");
  set_long("A delicious muffin.");
  set_mass(5);
  set_value(2);
  set_uses(1);  // One bite consumes it
}
```

### Multi-Use Food

```lpc
inherit STD_FOOD;

void setup() {
  set_id("loaf");
  set_adj("bread");
  set_name("bread loaf");
  set_short("a bread loaf");
  set_long("A crusty loaf of bread.");
  set_mass(10);
  set_value(5);
  set_uses(4);  // Four bites/nibbles
}
```

### Drink with Custom Action

```lpc
inherit STD_DRINK;

void setup() {
  set_id("juice");
  set_adj("strawberry");
  set_name("strawberry juice");
  set_short("a strawberry juice");
  set_long("A delicious strawberry juice.");
  set_mass(5);
  set_value(2);
  set_uses(5);  // Five sips
  set_drink_action("$N $vgulp down the $o with gusto!");
}
```

### Virtual Food (LPML Data File)

Food items can also be created as virtual objects via `.food` LPML files (see the `virtual-object-creation` skill). Supported fields: `id`, `adj`, `name`, `short`, `long`, `mass`, `value`, `uses`, `additional_ids`.

## Defines

```lpc
#define STD_FOOD   DIR_STD "consume/food"
#define STD_DRINK  DIR_STD "consume/drink"
#define M_EDIBLE   DIR_STD_MODULES "edible"
#define M_POTABLE  DIR_STD_MODULES "potable"
#define M_USES     DIR_STD_MODULES "uses"
```

## Important Notes

- `set_uses()` initializes `_max_uses` on the first call. Subsequent calls only change `_uses`.
- `adjust_uses()` returns null (not 0) on boundary violation — check with `nullp()`.
- STD_FOOD/STD_DRINK auto-remove the object when uses hit 0. If you don't want this, inherit M_EDIBLE/M_POTABLE directly instead.
- Action messages use the `$-token` system (see `action-messages` skill): `$N` = actor, `$v` = verb conjugation, `$o` = object.
- Uses are persisted via `save_var()` — they survive storage in containers and player inventory saves.
