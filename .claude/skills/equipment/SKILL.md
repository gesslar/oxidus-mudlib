---
name: equipment
description: Understand and work with the equipment system in Oxidus. Covers weapons (hands, damage coefficient, damage type), armour (AC, defence mappings), clothing, body slots, the equip/unequip dispatch, combat integration, and creating new equipment items.
---

# Equipment Skill

You are helping work with the Oxidus equipment system. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

```
STD_ITEM
  ├── STD_WEAPON  (std/equip/weapon.lpc)     — weapons with damage
  └── STD_EQUIP   (std/equip/equip.lpc)      — base wearable
        └── STD_CLOTHING (std/equip/clothing.lpc) — cosmetic wear
              └── STD_ARMOUR (std/equip/armour.lpc) — defensive gear
```

Equipment slots live on the body (`std/living/body.lpc`), and equip/unequip dispatch happens in `std/living/equipment.lpc`.

## Body Slots — `std/living/body.lpc`

**Wearable slots:**
```lpc
"head", "neck", "torso", "back", "arms", "hands", "legs", "feet"
```

**Weapon slots:**
```lpc
"right hand", "left hand"
```

Query with `query_body_slots()` and `query_weapon_slots()`.

Living bodies call `set_ignore_mass(1)` in `mudlib_setup()`.

## Weapons — `std/equip/weapon.lpc`

Inherits `STD_ITEM`.

### Properties

| Property | Type | Default | Purpose |
|---|---|---|---|
| `__hands` | `int` | `1` | Hands required (1 or 2) |
| `__dc` | `mixed` | `1.0` | Damage coefficient — float or function |
| `__damage_type` | `string` | `"bludgeoning"` | Damage type (matches defence types) |
| `__equipped` | `int` | `0` | Current equipped state |
| `__slot` | `string` | | Preferred weapon slot |

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_hands` | `void set_hands(int i)` | Set hands required (1 or 2) |
| `query_hands` | `int query_hands()` | Get hands required |
| `set_dc` | `void set_dc(mixed x)` | Set damage coefficient (float or function) |
| `query_dc` | `float query_dc()` | Get DC — evaluates function if callable |
| `set_damage_type` | `void set_damage_type(string dt)` | Set damage type string |
| `query_damage_type` | `string query_damage_type()` | Get damage type |
| `set_slot` | `void set_slot(string str)` | Set preferred slot |
| `query_slot` | `string query_slot()` | Get preferred slot |
| `can_equip` | `mixed can_equip(object tp)` | Validate equip (calls `equip_check` if defined) |
| `equip` | `mixed equip(object tp, string slot)` | Equip into wielder's hand |
| `can_unequip` | `mixed can_unequip(object tp)` | Validate unequip (calls `unequip_check` if defined) |
| `unequip` | `varargs int unequip(object tp, int silent)` | Remove from wielder |
| `equipped` | `int equipped()` | Check if currently equipped |
| `is_weapon` | `int is_weapon()` | Identity — returns 1 |

### Equip Flow (Weapons)

1. Validates weapon is in wielder's inventory
2. Checks not already equipped
3. Checks target slot is free
4. Calls `tp->equip(this_object(), slot)` on the living
5. Sets `_equipped = 1`
6. Sends GMCP update

**Multi-handed weapons** automatically occupy consecutive weapon slots. A 2-handed weapon fills both "right hand" and "left hand".

## Wearables — `std/equip/equip.lpc`

Base class for all worn items. Inherits `STD_ITEM`.

### Properties

| Property | Type | Purpose |
|---|---|---|
| `__slot` | `string` | Body slot this item occupies |
| `__equipped` | `int` | Current equipped state |

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_slot` | `void set_slot(string str)` | Assign to body slot |
| `query_slot` | `string query_slot()` | Get assigned slot |
| `can_equip` | `mixed can_equip(string slot, object tp)` | Validate equip |
| `equip` | `mixed equip(object tp, string slot)` | Equip onto wearer |
| `can_unequip` | `mixed can_unequip(object tp)` | Validate unequip |
| `unequip` | `varargs int unequip(object tp, int silent)` | Remove from wearer |
| `equipped` | `int equipped()` | Check if currently equipped |

## Clothing — `std/equip/clothing.lpc`

Inherits `STD_EQUIP`. Minimal addition:

- `set_id()` auto-adds `"clothing"` identifier
- `int is_clothing()` — returns 1

## Armour — `std/equip/armour.lpc`

Inherits `STD_CLOTHING`. Adds defensive properties.

### Properties

| Property | Type | Default | Purpose |
|---|---|---|---|
| `__defence` | `mapping` | `([])` | Damage type to defence factor mapping |
| `__ac` | `float` | `0.0` | Armour class rating |

### Functions

| Function | Signature | Purpose |
|---|---|---|
| `set_defence` | `void set_defence(mapping def)` | Set full defence mapping |
| `add_defence` | `void add_defence(string type, float amount)` | Add/set defence for one damage type |
| `query_defence` | `mapping query_defence()` | Get defence mapping |
| `query_defence_amount` | `float query_defence_amount(string type)` | Get defence for specific type |
| `set_ac` | `void set_ac(float ac)` | Set armour class |
| `query_ac` | `float query_ac()` | Get armour class |
| `add_ac` | `float add_ac(float ac)` | Increment armour class |
| `is_armour` | `int is_armour()` | Identity — returns 1 |

When armour is equipped or unequipped, it calls `tp->adjust_protection()` to recalculate the living's total defences.

## Equipment Manager — `std/living/equipment.lpc`

On the living side, manages slot-to-object mappings.

| Function | Signature | Purpose |
|---|---|---|
| `query_equipped` | `mapping query_equipped()` | Copy of slot → wearable mapping |
| `query_wielded` | `mapping query_wielded()` | Copy of slot → weapon mapping |
| `equipped_on` | `object equipped_on(string slot)` | Item on a body slot |
| `wielded_in` | `object wielded_in(string slot)` | Weapon in a hand slot |
| `equip` | `int equip(object ob, string slot)` | Dispatch: detects weapon/armour/clothing |
| `unequip` | `int unequip(mixed ob)` | Remove item (accepts object or slot string) |
| `has_equipped` | `int has_equipped(object ob)` | Check if specific object is worn |
| `has_wielded` | `int has_wielded(object ob)` | Check if specific object is wielded |

**Dispatch logic**: `equip()` checks `has("is_weapon")`, `has("is_armour")`, `has("is_clothing")` to route to `equip_weapon()` or `equip_wearable()`.

## Combat Integration — `std/living/combat.lpc`

### adjust_protection()

Called when armour is equipped or unequipped. Iterates all equipped items and aggregates:

```lpc
mapping adjust_protection() {
  // Sum __defence mappings from all equipped armour
  // Sum _ac from all equipped items
  // Store in living's __defence and __ac
}
```

### Combat Use

- **Hit chance**: `chance -= (ac * 2.0)` — AC reduces hit probability
- **Damage reduction**: `damage -= defence[damage_type]` — type-specific defence subtracted from damage
- **Weapon damage**: `dc` (damage coefficient) scales attack damage

## Creating Equipment

### Weapon Example

```lpc
inherit STD_WEAPON;

void setup() {
  set_id(({ "sword", "rusty sword" }));
  set_short("rusty sword");
  set_long("A battered blade with flecks of rust.");
  set_hands(1);
  set_dc(1.5);
  set_damage_type("slashing");
  set_mass(30);
  set_value(10);
}
```

### Armour Example

```lpc
inherit STD_ARMOUR;

void setup() {
  set_id(({ "jerkin", "leather jerkin" }));
  set_short("sturdy leather jerkin");
  set_long("A well-made jerkin of thick leather.");
  set_slot("torso");
  set_ac(2.0);
  set_defence(([
    "slashing"   : 2.0,
    "piercing"   : 1.0,
    "bludgeoning": 1.0,
  ]));
  set_mass(20);
  set_value(25);
}
```

### Clothing Example

```lpc
inherit STD_CLOTHING;

void setup() {
  set_id(({ "shoes", "leather shoes" }));
  set_short("pair of black leather shoes");
  set_long("Simple but well-made leather shoes.");
  set_slot("feet");
  set_mass(15);
}
```

### Weapon with Custom Equip Check

```lpc
inherit STD_WEAPON;

mixed equip_check(object tp) {
  if(tp->get_attribute("strength") < 10)
    return "You are not strong enough to wield this.";
  return 1;
}
```

## Defines

```lpc
#define STD_WEAPON    DIR_STD "equip/weapon"
#define STD_EQUIP     DIR_STD "equip/equip"
#define STD_CLOTHING  DIR_STD "equip/clothing"
#define STD_ARMOUR    DIR_STD "equip/armour"
```

## Important Notes

- Each body slot holds one item. Equipping to an occupied slot requires unequipping first.
- Multi-handed weapons fill consecutive weapon slots automatically.
- `__dc` can be a function for dynamic damage (e.g., scaling with level).
- Defence is type-matched — `"slashing"` defence only reduces `"slashing"` damage.
- Auto-unequip happens if an equipped item is moved out of the living's inventory.
- GMCP updates (`GMCP_PKG_CHAR_ITEMS_UPDATE`) fire on all equip/unequip events.
- Equipment state (`__equipped`) is `nosave` on the item — it's re-established from the living's equipment mapping on restore.
