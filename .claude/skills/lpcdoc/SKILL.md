---
name: lpcdoc
description: LPCDoc comment block generation guide. Consult when writing or updating documentation headers for LPC functions, classes, and modules.
---

# LPCDoc Generation Guide

This document provides instructions for generating LPCDoc comment blocks for
LPC source code. Follow these guidelines to create accurate, consistent, and
useful documentation.

## Updating Existing Code

When working on a file, **update any existing comments and headers that do not
match these conventions**. This includes:

- File headers that use old formats (convert to the standard `@file` format)
- Function docs that use snake_case names (update to camelCase)
- Comments with American spelling (update to Canadian English)
- Missing or incomplete documentation on functions you are modifying
- Malformed or unclear comment blocks

Do not go out of your way to document the entire file — but if you encounter
non-conforming documentation while working, fix it.

## Comment Block Structure

LPCDoc comments must be placed directly before the code element they document
and follow this structure:

```c
/**
 * Description of the element (function, variable, etc.)
 *
 * Additional details if needed, forming a complete paragraph.
 *
 * @tags and other structured elements
 */
```

Key points:

- Start with `/**` and end with `*/`
- Each line within the comment begins with ` * ` (space, asterisk, space)
- **Always** leave a blank doc line (` *`) between the description and
  the first tag. This is mandatory even for short one-line descriptions.
- Place tags after the description

## Function Documentation

For functions, document:

1. Purpose/behaviour of the function
2. Each parameter
3. Return value
4. Any errors/exceptions

Example:

```c
/**
 * Calculates the total damage based on attack power and defence.
 *
 * @param {int} attack - The attack power value
 * @param {int} defence - The defence value
 * @returns {int} The calculated damage amount
 * @throws If either value is negative
 */
int calculateDamage(int attack, int defence) {
    // Implementation
}
```

## Variable Documentation

For variables, document:

1. Purpose/use of the variable
2. Type information

Example:

```c
/**
 * Maximum health points allowed for any player character.
 *
 * @type {int}
 */
int MAX_PLAYER_HP = 100;
```

## Tags Reference

### `@param`

Documents a function parameter.

**Syntax:** `@param {type} name - Description`

```c
/**
 * @param {int} attack - The attack power value.
 * @param {string} target - The name of the target.
 */
```

#### Optional parameters

Wrap the parameter name in brackets to indicate it is optional.

```c
/**
 * @param {mapping} [options] - Optional configuration settings.
 */
```

#### Default values

Append `=value` inside the brackets to document a default.

```c
/**
 * @param {string} [which="door"] - The specific door to unlock.
 */
int unlock(string which) {
  which = which || "door";
}
```

#### Reference parameters

In FluffOS, parameters passed by reference can be documented with `&`.

```c
/**
 * @param {int} &value - A reference to an integer that will be
 *                       modified.
 */
void increment(int ref value) {
  value++;
}
```

### `@returns`

Documents the return value of a function.

**Syntax:** `@returns {type} Description`

```c
/**
 * @returns {int} The calculated damage amount.
 */
```

When a function may return different types depending on conditions, use a
union type.

```c
/**
 * @returns {object | string} The entity if found, or an error
 *                            message.
 */
```

#### Type Predicates

A special form of `@returns` enables **type narrowing** in conditional
branches. Instead of documenting a return type, it declares that the
function acts as a type guard for one of its parameters.

**Syntax:** `@returns {paramName is type}`

Where `paramName` is the name of a parameter in the function signature
and `type` is the type it should be narrowed to when the function
returns a truthy value.

```c
/**
 * @param {mixed} arg
 * @returns {arg is string}
 */
int stringp(mixed arg);
```

When this function is called inside a conditional, the language server
narrows the tested variable to the predicate type within the true
branch:

```c
void test() {
    mixed o;
    if(stringp(o)) {
        // o is narrowed to string here
        string s = o; // OK
    }
}
```

The type predicate does not change the function's actual return type.
The function still returns its declared type (e.g., `int`); the
predicate only informs the language server's flow analysis.

Type predicates work with any valid type, including primitives,
composites, and object file paths:

```c
/**
 * @param {mixed} arg
 * @returns {arg is mixed*}
 */
int pointerp(mixed arg);

/**
 * @param {mixed} o
 * @returns {o is "/std/living.c"}
 */
int is_living(mixed o);
```

Type predicates are not limited to simple type-checking functions. Any
function that returns a truthy or falsy value can use a predicate to
narrow a parameter. A description may follow the predicate to document
the return value for human readers:

```c
/**
 * Returns the original object if it is a user, otherwise 0.
 *
 * @param {object} ob - Some object.
 * @returns {ob is "/std/user.c"} The original object if it is a
 *                                user, or 0.
 */
object get_user(object ob) {
    return ob->is_user() ? ob : 0;
}
```

Preprocessor defines are resolved in type predicates, so you can use
macros as the target type:

```c
#define STD_USER "/std/user.c"

/**
 * @param {object} ob - Some object.
 * @returns {ob is STD_USER} 1 if ob is a user object.
 */
int is_user(object ob);
```

### `@throws`

Documents conditions that cause a `throw()`. A `throw()` is a soft
error — it can be intercepted by `catch()` and does not generate a
stack trace. This is the mechanism for recoverable exceptions.

**Syntax:** `@throws Description of the condition`

```c
/**
 * @throws If the configuration file was not found.
 */
```

Multiple `@throws` tags can be used when a function has several throw
conditions.

### `@errors`

Documents conditions that trigger a hard error — `error()` in FluffOS
or `raise_error()` in LDMud. Unlike `throw()`, a hard error generates
a full stack trace and is expensive. LPC distinguishes between soft
errors (`throw()`) and hard errors, and `@errors` exists to document
that distinction.

**Syntax:** `@errors Description of the condition`

```c
/**
 * @errors If the crafter lacks required skills.
 * @errors If components are missing or of insufficient quality.
 */
```

This will be evident by the appearance of the `error()` function
within the function body. Also applies to `assert()` and
`assert_arg()`.

### `@type`

Documents the type of a variable or expression.

**Syntax:** `@type {type}`

#### Variable annotation

```c
/**
 * @type {int} Maximum health points for a player.
 */
int MAX_PLAYER_HP = 1000;

/**
 * @type {([ string: int ])} Mapping of damage types to resistance
 *                           values.
 */
mapping resistances = ([ "fire": 10, "cold": 5, "physical": 3 ]);
```

#### Inline expression casting

You can annotate an expression inline to assert its type.

```c
object p = /** @type {"/std/player.c"} */(get_player());
```

#### Inline Parameter Type Narrowing

The `@type` tag can also be used inline in a function signature to
narrow an `object` parameter for the LSP. Place it as a comment
immediately before the parameter:

```c
mixed main(/** @type {STD_PLAYER} */ object caller, string str) {
```

This lets the LSP resolve `call_other` methods (e.g.,
`caller->set_env(...)`) that exist on the typed object. Use the
most specific `STD_*` macro whose interface matches the methods
called on that parameter. See the **lpc-coding-style** skill for
full guidance.

### `@var`

Documents the type of an inherited variable. Use this when a variable
is declared in a parent object and you want to provide type information
in the inheriting file.

**Syntax:** `@var {type} Description`

```c
/**
 * @var {([ string: int ])} Inherited mapping of skill names to
 *                          levels.
 */
```

### `@typedef`

Defines a named type alias or a structured shape. This is useful for
documenting complex data structures — like the expected shape of a
mapping — without needing a `class` or `struct` definition. The
language server resolves object paths in `@typedef` tags and provides
IntelliSense for the defined type.

**Syntax:** `@typedef {type} Name` or `@typedef Name` followed by
`@property` tags

#### Simple type alias

```c
/**
 * @typedef {int | string} Identifier
 */
```

#### Structured shape with properties

Use `@property` tags to define the members of the type. This is the
"shape definer" — it lets you describe what keys a mapping or data
structure is expected to have, along with their types.

```c
/**
 * @typedef PlayerData
 * @property {string} name - The player's display name.
 * @property {int} level - Current experience level.
 * @property {"/std/guild.c"} guild - The player's guild object.
 * @property {int} hp - Current hit points.
 * @property {int} max_hp - Maximum hit points.
 */
```

You can then use the typedef name in other annotations:

```c
/**
 * @param {string} player_name - The name to look up.
 * @returns {PlayerData} The player's data record.
 */
mapping get_player_data(string player_name) {
  // Implementation
}
```

#### Object path resolution

Object paths used within `@typedef` are resolved by the language
server, giving you full IntelliSense when referencing those types:

```c
/**
 * @typedef PartyMember
 * @property {"/std/player.c"} player - The player object.
 * @property {string} role - Role in the party (tank, healer, etc.).
 * @property {int} joined - Timestamp when they joined.
 */
```

### `@callback`

Documents a function that is passed as an argument to another function.
Use this to describe the expected signature of callback parameters.

**Syntax:** `@callback name`

```c
/**
 * @callback sort_func
 * @param {mixed} a - The first element to compare.
 * @param {mixed} b - The second element to compare.
 * @returns {int} Negative, zero, or positive comparison result.
 */
```

### `@property`

Documents a property of a class or struct. Used in the doc comment
immediately above the class/struct definition.

**Syntax:** `@property {type} name - Description`

```c
/**
 * Represents an item available for purchase.
 *
 * @property {string} short - Display name shown in shop menus.
 * @property {string} file - Full path to the item's source file.
 * @property {int} cost - Purchase price.
 * @property {int} stock - Current quantity available.
 */
class ShopItem {
  string short;
  string file;
  int cost;
  int stock;
}
```

### `@overload`

Documents multiple calling signatures for a single `varargs` function
that accepts `mixed *args...`. Each `@overload` block describes one
valid way to call the function, with its own `@param` and `@returns`
tags.

Place the description before the first `@overload`. Each `@overload`
starts a new signature — the `@param` and `@returns` tags that follow
it belong to that overload until the next `@overload` or the end of
the comment.

```c
/**
 * Sends a direct message to the specified object.
 *
 * @overload
 * @param {string} str - The message string (sent to
 *                       previous_object()).
 *
 * @overload
 * @param {object} ob - The target object.
 * @param {string} str - The message string.
 * @param {int} [msg_type] - The message type.
 *
 * @errors If insufficient arguments are provided.
 */
varargs void tell(mixed *args...) {
    // Implementation
}
```

Tags that apply to all overloads (such as `@errors`, `@throws`, or
`@example`) should be placed after the last `@overload` block.

### `@example`

Provides an example code snippet demonstrating usage.

**Syntax:** `@example` followed by code on subsequent lines

```c
/**
 * Transfers items between two containers.
 *
 * @example
 * int moved = transfer_items(player, chest, "gold_coin", 100);
 * if(moved < 100) {
 *     write("Could only move " + moved + " coins.");
 * }
 */
```

### `@deprecated`

Marks a function, variable, or other element as deprecated. Include a
description of what to use instead.

**Syntax:** `@deprecated Description or replacement`

```c
/**
 * @deprecated Use query_experience() instead.
 */
int get_exp(string player_name) {
  return find_player(player_name)->query_experience();
}
```

### `@file`

Provides file-level documentation. Placed at the top of a file to
describe its purpose.

**Syntax:** `@file path/to/file.c`

```c
/**
 * @file /d/area/monsters/dragon.c
 *
 * Implements the elder dragon NPC with fire-breath attacks
 * and treasure hoarding behaviour.
 */
```

### `@see`

Creates a reference to another function, file, or resource.

**Syntax:** `@see reference`

```c
/**
 * @see check_crafting_skills
 * @see /std/container.c
 */
```

### `@override`

Indicates that a function overrides an inherited definition.

```c
/**
 * @override
 * @param {string} msg - The message to receive.
 */
void receive_message(string msg) {
  // Custom implementation
}
```

### `@inheritdoc`

Indicates that a function's documentation should be inherited from
the parent definition. When the language server encounters this tag,
it pulls the description, parameters, and return documentation from
the inherited function.

This is particularly useful in LPC where `inherit` is common —
rather than duplicating documentation across overrides, you can
inherit it and only document what changes.

```c
/**
 * @inheritdoc
 */
void create() {
  ::create();
  // Additional setup
}
```

You can also add to the inherited documentation. Your description
and any additional tags are merged with the parent's:

```c
/**
 * @inheritdoc
 * Also initialises the combat subsystem.
 */
void create() {
  ::create();
  init_combat();
}
```

### `@author`

Identifies the author of the code.

**Syntax:** `@author name`

### `@version`

Specifies the version of the code.

**Syntax:** `@version version`

### `@since`

Indicates when a feature was introduced.

**Syntax:** `@since version or date`

### `@private` / `@protected` / `@public`

Documents the visibility of a function or variable. These tags are
useful when the visibility cannot be inferred from the code or when
you want to be explicit.

```c
/**
 * @private
 * @param {string} name - The name to validate.
 * @returns {int} 1 if valid, 0 if invalid.
 */
static int is_valid_name(string name) {
  // Implementation
}
```

### `@link`

Creates an inline link to another element. Used within descriptions,
wrapped in `{@link ...}`.

**Syntax:** `{@link reference}`

```c
/**
 * This method works like {@link other_function} but has improved
 * performance.
 */
```

## Types Reference

### Primitive Types

- `int` - Integer
- `string` - Text string
- `float` - Floating-point number
- `object` - Generic object
- `mapping` - Key-value structure
- `function` - Function reference
- `buffer` - Binary data
- `mixed` - Any type
- `void` - No return value
- `class`/`struct` - Structured data types

### Named Objects

For objects of specific types, use the closest matching `STD_*` macro rather
than a raw file path:

```c
{STD_MACRO}
```

If no macro exists, fall back to a full path:

```c
{"/path/to/object.c"}
```

**Choosing the right macro:** Pick the most specific macro whose interface
matches how the object is actually used within the function — not what the
object might be at runtime. Look at which methods the function calls on the
object, and choose the macro that defines those methods.

- If the function calls methods specific to players (e.g. account operations),
  use `{STD_PLAYER}`.
- If it calls methods shared by players and NPCs (e.g. combat, vitals), use
  `{STD_BODY}`.
- If it only uses container operations (capacity, inventory) and the object
  could be a bag, a room, or a body, use `{STD_CONTAINER}`.
- If it only calls base object methods (e.g. query_id, move), use
  `{STD_OBJECT}`.

In short: the type should reflect the **interface the function depends on**,
not the concrete type the caller is likely to pass.

### Class/Struct Types

For class or struct instances, prefix with `class` or `struct`:

```c
{class ShopItem}
```

For arrays of class instances:

```c
{class ShopItem*}
```

### Arrays

For arrays of a specific type:

```c
{type*}
```

Example: `{string*}` for string array

Typed arrays work with any type — primitives, named objects, classes,
and other composites:

```c
{("/std/player.c")*}      // array of named objects
{([ string: int ])*}      // array of mappings
{class ShopItem*}          // array of class instances
```

### Mappings

For mappings with specific key/value types:

```c
{([ keytype: valuetype ])}
```

Example: `{([ string: int ])}` for string->int mapping

### Union Types

For values that could be multiple types:

```c
{type1 | type2}
```

Example: `{int | string}` for int or string

### Function Types

For function references with signature:

```c
{function(paramtype1, paramtype2): returntype}
```

Example: `{function(int, int): int}`

### Optional Parameters

For optional parameters:

```c
@param {type} [name] - Description
```

With default values:

```c
@param {type} [name=default] - Description
```

### Special Types

#### Undefined

Use `undefined` to distinguish between a legitimate `0` return and a
"not found" result. Use comma syntax for multiple possible return
types:

```c
@returns {int, undefined} The score, or undefined if not found.
```

#### Any Type (`*`)

For cases where any type is acceptable and you want to document this
explicitly, distinct from `mixed`:

```c
@param {*} value - Any value to be stored.
```

### Tuples

Tuples are represented as arrays with the member types within:

```c
({ string, int, float })
```

### Nested Composite Types

Complex data structures can use nested type annotations:

```c
{([ string: ([ string: int ]) ])}
```

## Practical Guidelines

1. Be concise but complete in descriptions
2. Document all parameters and return values
3. Note any side effects or state changes
4. Include examples for complex functions
5. Specify types as precisely as possible
6. Document error conditions and exceptions

## Example Documentation for Complex Function

```c
/**
 * Transfers items between two containers.
 *
 * This function handles weight limits and ownership restrictions.
 *
 * @param {"/std/container.c"} source - The source container
 * @param {"/std/container.c"} target - The target container
 * @param {string} itemId - The identifier of the item to transfer
 * @param {int} [count=1] - The number of items to transfer
 * @returns {int} The number of items successfully transferred
 * @throws If either container does not exist
 * @errors If the item cannot be found in the source
 * @errors If the target is full or over weight limit
 * @example
 * int moved = transferItems(player, chest, "gold_coin", 100);
 * if(moved < 100) {
 *     write("Could only move " + moved + " coins.");
 * }
 */
```

### Classes

Classes should be documented to describe their properties:

```c
/**
 * Represents a parsed GMCP message with its components.
 *
 * @property {string} name - Full message string
 * @property {string} package - First component of the message
 * @property {string} module - Second component of the message
 * @property {string} submodule - Third component, if present
 * @property {mixed} payload - Decoded payload data
 */
class ClassGMCP {
    // Implementation
}
```

Key points for class documentation:

- Document each property with @property tags
- Include property types in curly braces
- Add descriptions for both class and properties
- Optional additional paragraph for implementation details at the top

## Documentation Order

1. Visibility tags (@public, @protected, @private) should always come first
2. Other tags should follow in this order:
   - @override (if applicable)
   - @apply (if applicable)
   - @overload (if applicable — each followed by its own @param
     and @returns)
   - @param (when not using @overload)
   - @returns (when not using @overload)
   - @errors (only if error() is called)
   - @throws (only if throw() is called)
   - @example
   - Any other tags

## Line Length

- All documentation lines should wrap at 79 characters
- Maintain proper indentation when wrapping
- Use complete sentences even when wrapping

## What Not to Document

Unless specifically instructed, do not document:

1. Forward declarations
2. Ubiquitous entry points and lifecycle functions — these are
   required infrastructure repeated in every file of their kind
   and are not notable enough to warrant documentation:
   - `mixed main()` in commands
   - `void setup()`
   - `void mudlib_setup()`
   - `void base_setup()`
   - `void pre_setup_0()` through `void pre_setup_4()`
   - `void post_setup_0()` through `void post_setup_4()`
3. Preprocessor directives (#include, #define, etc.)
4. Inherit statements
5. Global variables (unless specifically required)

## Header Documentation

File headers should use this format:

```c
/**
 * @file /std/living/boon.c
 *
 * Buffs/debuffs and other boons for living objects.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */
```

When encountering file headers that do not follow this format, update them.
Preserve the existing information (author, dates, history) but restructure
into the standard format. Clarify any vague or unclear descriptions.

## Additional Notes

- Overridden lfuns may be documented with an @override tag
- Driver applies may use the tag @apply before its params
- Nested data structures should document the expected structure as precisely
  as possible. For complex structures, use nested type annotations
- If you see assert() or assert_arg() these are @errors, and not @throws
- Use Canadian English spelling in all documentation (colour, behaviour,
  defence, initialise, etc.)

Otherwise, if unsure, ask.

## Unsupported Tags

The following tags are **not supported** and should be removed on sight
when encountered in existing documentation. Do not introduce them in
new documentation.

- `@description` — Remove the tag and keep the text as the leading
  description paragraph (the description is always the first thing in
  the comment block; it does not need a tag).

## Imperative Information

Do not touch the code. For documentation purposes, we only need comments.
There will be no reason to opine on things like parens placement, semicolon
changes, etc., by updating the code sections of a file. Restrict activities
to only documentation.
