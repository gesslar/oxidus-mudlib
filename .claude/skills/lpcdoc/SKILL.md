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

## Essential Tags

### `@param`

Documents a function parameter.

```c
@param {type} name - Description
```

### `@returns`

Documents the function's return value.

```c
@returns {type} Description
```

### `@throws`

Documents what condition(s) result(s) in a throw().

```c
@throws Condition that causes a throw
```

This will be evident by the appearance of the throw() function within the
function body.

### `@errors`

Documents what condition(s) result(s) in an error().

```c
@errors Condition that causes an error
```

This will be evident by the appearance of the error() function within the
function body.

### `@type`

Documents a variable's type.

```c
@type {type}
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

Provides example usage.

```c
@example
codeExampleHere
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

### Arrays

For arrays of a specific type:

```c
{type*}
```

Example: `{string*}` for string array

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

### Undefined Values

For return values that might be undefined:

```c
@returns {type | undefined} Description
```

### Undefined Parameters

Parameters may be undefined when not provided:

```c
@param {type | undefined} name - Description
```

This is often handled with default values inside the function.

### Tuples

Tuples are represented as arrays with the member types within:

```c
({ string, int, float })
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
 * @description Buffs/debuffs and other boons for living objects.
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
