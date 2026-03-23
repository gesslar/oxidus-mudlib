---
name: virtual-object-creation
description: Create virtual (data-driven) items for Threshold RPG. Generates .loot, .food, .gem, .geode data files and new virtual item types with extension-based auto-discovery modules.
---

# Virtual Object Creation Skill

You are helping create virtual items for Threshold RPG. Virtual items are data-driven objects — instead of writing a `.c` file with code, you write a data file containing an LPC mapping of properties. The system compiles it into a game object by cloning the appropriate base class and applying the properties.

## How Virtual Items Work

1. A data file exists at a path like `/obj/gems/ruby.gem`
2. When the game needs the object, the driver calls the virtual daemon
3. The daemon reads the file, parses the LPC mapping
4. It clones the base class for that extension (`.gem` → `STD_GEM`)
5. It applies all properties from the mapping to the object
6. The object is ready to use — no `.c` file needed

## Existing Item Types

| Extension | Base Class | Use For |
|---|---|---|
| `.loot` | `STD_LOOT` | Monster drops, vendor trash, crafting materials |
| `.food` | `STD_FOOD` | Consumable food and drink |
| `.gem` | `STD_GEM` | Gemstones with color display |
| `.geode` | `STD_GEODE` | Collectible geode specimens |

## Data File Format

All virtual items use the same format — an LPC mapping:

```c
([
    "id"    : "/item name/alt id/category",
    "name"  : "item name",
    "short" : "item name",
    "long"  : "Description of the item.",
    "mass"  : 30,
    "value" : ({ 100, "danar" }),
])
```

### Syntax Rules

- File starts with `([` and ends with `])`
- Each property is `"key" : value,` — trailing comma on last entry is OK
- Strings use `"double quotes"` — multi-line strings use LPC string concatenation
- Arrays use `({ element1, element2 })`
- `\\n` in strings is converted to actual newlines at compile time
- `id` uses slash-separated values: `"/primary name/alt1/alt2/category"`

## Common Properties (All Types)

| Property | Type | Description |
|---|---|---|
| `id` | string | Slash-separated IDs. First element is primary. |
| `name` | string | Object name. |
| `short` | string | One-line display name. Supports `` `<color>` `` codes. |
| `no_ansi_short` | string | Short without color codes (for sorting). |
| `long` | string | Full description. Indent with spaces for formatting. |
| `mass` | int | Weight in units. |
| `value` | `({ int, "currency" })` | Fixed value, e.g. `({ 115, "danar" })`. |
| `autovalue` | `"auto"` | Enable auto-value calculation from level. |
| `autotype` | string | Currency for auto-value: `"danar"`, `"slag"`, `"crown"`. |
| `autobonus` | int | Level bonus for auto-value calculation. |
| `material` | `({ "type", "subtype" })` | Material tags. |
| `stack_id` | string | Grouping ID for stackable items. |
| `stack_max` | int | Maximum stack size. |
| `stack_properties` | `({ "prop1" })` | Properties preserved in stack operations. |
| `vendor_quantity` | int | Default shop quantity. |
| `vendor_type` | string | Shop category. |

## Type-Specific Properties

### Food (`.food`)

| Property | Type | Description |
|---|---|---|
| `filling` | int | Core stat. Derives: healing (x10), mass (x20), cost (x50 slag). |
| `healing` | int | Override all healing (HP/SP/EP). |
| `hp_healing` | int | Override HP healing only. |
| `sp_healing` | int | Override SP healing only. |
| `ep_healing` | int | Override EP healing only. |
| `cost` | int | Override derived cost. |
| `eat_message` | string | Custom message shown when eaten. |
| `no_grill` | int | Set to 1 to prevent grilling. |

### Gem/Geode (`.gem`, `.geode`)

| Property | Type | Description |
|---|---|---|
| `color` | string | ANSI color code, e.g. `` "`<dab>`" ``. Applied to display. |
| `collectible` | int | Mark as collectible (geodes default to 1). |

## Creating a Virtual Item

### Step 1: Choose the type and location

- Loot: `obj/loot/drop/{name}.loot` or `obj/loot/d/{domain}/{zone}/{name}.loot`
- Food: `obj/food/{category}/{name}.food`
- Gems: `obj/gems/{name}.gem`
- Geodes: `obj/collectible/geode/{name}.geode`

### Step 2: Write the data file

Use the LPC mapping format. Include at minimum: `id`, `short`, `name`, `long`, and a value mechanism (`value`, `loot_value`, or `autovalue`+`autotype`).

### Step 3: Reference it in code

```c
// Clone the virtual item
object ob = new("/obj/gems/ruby.gem");

// Or add to a loot table, drop list, etc.
```

No registration needed — the virtual system handles compilation automatically.

## Templates

### Loot Drop (auto-valued, stackable)

```c
([
    "short" : "ITEM_NAME",
    "name" : "ITEM_NAME",
    "long" :
"   DESCRIPTION_TEXT",
    "id" : "/ITEM_NAME/ALT_ID/CATEGORY",
    "autovalue" : "auto",
    "autotype" : "danar",
    "mass" : MASS,
    "material" : ({ "MATERIAL_TYPE", "SUBTYPE" }),
    "stack_id" : "ITEM_NAME",
])
```

### Loot Drop (fixed value)

```c
([
    "short" : "ITEM_NAME",
    "name" : "ITEM_NAME",
    "long" :
"   DESCRIPTION_TEXT",
    "id" : "/ITEM_NAME/ALT_ID",
    "loot_value" : ({ AMOUNT, "danar" }),
    "mass" : MASS,
    "material" : ({ "MATERIAL_TYPE", "SUBTYPE" }),
    "stack_id" : "ITEM_NAME",
    "vendor_quantity" : 1,
    "vendor_type" : "VENDOR_CATEGORY",
])
```

### Food Item

```c
([
    "id" : "/FOOD_NAME/ALT_ID/food",
    "short" : "FOOD_NAME",
    "name" : "FOOD_NAME",
    "long" :
"   DESCRIPTION_TEXT",
    "eat_message" :
"CUSTOM_EAT_MESSAGE\n",
    "filling" : FILLING_VALUE,
    "sp_healing" : HEALING_OVERRIDE,
    "cost" : COST_OVERRIDE,
    "mass" : MASS,
    "no_grill" : 1,
])
```

### Gem

```c
([
    "id" : "/GEM_NAME/gem/stone",
    "name" : "GEM_NAME",
    "short" : "`<COLOR_CODE>`GEM_NAME`<res>`",
    "color" : "`<COLOR_CODE>`",
    "no_ansi_short" : "GEM_NAME",
    "long" : "\n"
"   DESCRIPTION_TEXT\n"
"\n",
    "value" : ({ VALUE, "danar" }),
])
```

### Geode

```c
([
    "short" : "`<COLOR_CODE>`GEODE_NAME`<res>`",
    "color" : "`<COLOR_CODE>`",
    "id" : "/GEODE_NAME geode/GEODE_NAME/geode",
    "long" :
"   DESCRIPTION_TEXT",
    "stack_id" : "GEODE_NAME",
])
```

## Creating a New Virtual Item Type

To add an entirely new item type (e.g. `.potion`, `.scroll`, `.rune`):

### Option A: Add to Legacy Item Module (Simple)

1. **Create the base class** at `std/{type}.c`:

```c
inherit STD_ITEM;

void mudlib_setup() {
    set("TYPE_FLAG", 1);
    // Default properties
}
```

2. **Add the define** in `include/mudlib.h`:

```c
#define STD_POTION DIR_STD "potion"
```

3. **Register in item module** — add to `STANDARD_INHERITIBLES` in `adm/daemons/modules/virtual/item.c`:

```c
private nosave mapping STANDARD_INHERITIBLES = ([
    "food"   : STD_FOOD,
    "loot"   : STD_LOOT,
    "gem"    : STD_GEM,
    "geode"  : STD_GEODE,
    "potion" : STD_POTION,
]);
```

4. **Create `.potion` data files** using the standard mapping format.

### Option B: Extension-Based Module (Modern)

Create a standalone module at `adm/daemons/modules/virtual/{extension}.c`. The virtual daemon auto-discovers it by matching the file extension to the module name. No registration needed.

```c
// adm/daemons/modules/virtual/potion.c
#include <daemons.h>

inherit STD_DAEMON;

object compile_object(string path) {
    string data = read_file(path);
    if(!data) return 0;

    mapping item_data = GLOBALS->from_string(data);
    object ob = new(STD_POTION);

    item_data = map(item_data, function(mixed key, mixed value) {
        return stringp(value)
            ? replace_string(value, "\\n", "\n")
            : value;
    });

    LOOT_D->setup_loot(ob, item_data);

    return ob;
}
```

The module name (`potion.c`) matches the extension (`.potion`). The virtual daemon finds it automatically — just create the module and start writing `.potion` data files.

**Option B is preferred for new types.** It's cleaner, self-contained, and doesn't modify shared code.

## Common Pitfalls

- **Missing `([` and `])` delimiters**: the data file must be a valid LPC mapping.
- **Unquoted keys**: all keys must be strings in `"double quotes"`.
- **Array syntax**: use `({ })` not `[ ]` — this is LPC, not JSON.
- **Multi-line strings**: use LPC string concatenation (adjacent strings auto-concat), not `+`.
- **Value format**: `value` must be `({ amount, "currency_type" })`, not just a number.
- **`id` format**: slash-separated string, first element is primary: `"/sword/weapon/loot"`.
- **Color codes in `short`**: always pair with `no_ansi_short` for clean sorting/logging.
- **Color reset**: always end colored `short` with `` `<res>` ``.

## Validation Checklist

- [ ] File has correct extension (`.loot`, `.food`, `.gem`, `.geode`, or custom)
- [ ] Content is a valid LPC mapping (`([` ... `])`)
- [ ] `id`, `name`, `short`, and `long` are present
- [ ] `long` description is properly formatted with leading spaces for indent
- [ ] Value mechanism is present: `value`, `loot_value`, or `autovalue`+`autotype`
- [ ] If colored `short`: includes `` `<res>` `` reset and `no_ansi_short`
- [ ] If stackable: `stack_id` is present
- [ ] If food: `filling` is set (core stat that derives others)
- [ ] If gem/geode: `color` property matches the color code in `short`
- [ ] File path is logical for the item type
