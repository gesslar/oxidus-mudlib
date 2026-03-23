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
- Leave an empty line (with just ` * `) between the description and tags
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

For objects of specific types:

```c
{"/path/to/object.c"}
```

Example: `{"/std/living/player.c"}`

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
   - @param
   - @returns
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
2. Standard setup functions:
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

## Imperative Information

Do not touch the code. For documentation purposes, we only need comments.
There will be no reason to opine on things like parens placement, semicolon
changes, etc., by updating the code sections of a file. Restrict activities
to only documentation.
