---
name: lpc-coding-style
description: LPC coding style and formatting conventions for this project. Consult when writing or modifying any LPC code — covers spacing, indentation, braces, naming, control flow, and idiomatic patterns.
---

# LPC Coding Style Guide

This document prepares an LLM for how code should be written in LPC for this project.

## Language and Spelling

Use **Canadian English** spelling throughout all code, comments, documentation, strings, and file names. Key examples:

- colour (not color)
- behaviour (not behavior)
- honour (not honor)
- favourite (not favorite)
- centre (not center)
- defence (not defense)
- analyse (not analyze)
- catalogue (not catalog)
- modelling (not modeling)
- travelling (not traveling)
- initialise/initialisation are acceptable but initialize/initialization is also fine (both are used in Canadian English)

This applies to identifiers (variable names, function names, file names), comments, strings, and documentation.

## Spacing

- Use 2 spaces for indentation.
- Use spaces around operators and after commas.
- NO space after keywords (e.g., `if`, `while`, `for`).
- No trailing spaces at the end of lines or files.
- Use a single space after `//` for comments.
- NO space after `#` for preprocessor directives.
- Use a single space before and after `=` for assignments.
- Use a single space after `,` for function arguments and parameter lists.
- NO space before or after `;` for statement terminators.
- NO space after `(` and before `)` for function calls and definitions.
- NO space after `{` and before `}` for blocks.
- NO space after `[` and before `]` for array indexing.
- NO space after `.` for member access.
- NO space after `->` for pointer member access.
- Use a single space before and after `?` and before and after `:` for ternary operators.
- NO space after `!` for negation.

## Bracing Style

- Opening braces for control structures are placed on the same line as the statement.
- Closing braces get their own line unless followed by an `else` or similar continuations.
- Single-statement blocks still use braces.
- Example:

  ```lpc
  if(condition) {
    // code
  } else {
    // more code
  }
  ```

## Declarations

FluffOS LPC now supports C-style mixed declarations. Declare things close to where they are first used for clarity, unless grouping them helps readability.

### Variables

Prefer declaring locals near their first use inside the narrowest scope that makes sense. Group related locals together when it improves readability. Global variables may be declared anywhere before use; placing them together near the top of the file is still helpful for discoverability.

### Functions

Forward declarations at the top of the file are recommended when functions are referenced before their definitions. They are no longer strictly required by the driver but can improve clarity in larger files.

## Naming Conventions

### camelCase (Primary Convention)

All identifiers use **camelCase** (lowercase first letter, uppercase on subsequent word boundaries):

- **Function names:** `queryName()`, `setValue()`, `findTarget()`, `calculateDamage()`
- **Variable names:** `maxHealth`, `currentEnemy`, `regenInterval`, `attackSpeed`
- **Function parameters:** `object targetObject`, `string playerName`, `int damageAmount`

Accessor functions use the prefix `query` rather than `get`:
- `queryLevel()`, `queryName()`, `queryMaxHp()`

Mutator functions use the prefix `set`:
- `setLevel()`, `setName()`, `setMaxHp()`

### Private Functions

Do **not** use an underscore `_` prefix to denote private functions — the `private` keyword is sufficient. Private helpers follow the same camelCase convention as all other functions.

### Local Variable Names

Do **not** name a local variable the same as any function in scope. This avoids shadowing and potential confusion. Choose a distinct name instead.

### Exceptions

- **Constants** (`#define`) use `ALL_CAPS_WITH_UNDERSCORES`: `MAX_LEVEL`, `COLOUR_D`
- **Class names** use PascalCase (uppercase first letter): `ClassGMCP`, `ClassEvent`
- **GMCP handler functions** use PascalCase to match protocol convention: `Hello()`, `Supports()`, `Items()`

### Examples

```lpc
// Function names
mixed queryEffectiveBoon(string cl, string type) { ... }
void processExpiredBuffs() { ... }
object findNearestEnemy(object tp) { ... }

// Variable names
int regenInterval;
string targetName;
mapping currentEnemies = ([]);
float attackSpeed = 2.0;

// Constants
#define MAX_CAPACITY 100
#define COMBAT_TICK_RATE 2
```

## Control Structures

- Blank lines between logical sections of code.
- For switch statements, the `case` keyword is not indented, but case content is indented one level.
- Always include a `default` case in switch statements when appropriate.
- Example:

  ```lpc
  switch(value) {
    case 1:
      // code
      break;
    case 2:
      // code
      break;
    default:
      // code
      break;
  }
  ```

## Comments

- File headers use block comments to describe the file's purpose.
- Function documentation is placed directly above the function definition.
- Use line comments (`//`) for inline clarifications.
- Complex algorithms should be explained with comments.
- Comment style should follow LPCDoc conventions for public APIs.

## Error Handling

- Use `catch` blocks for error handling where appropriate.
- Consider returning error messages or error codes rather than throwing errors when possible.
- Use `error()` for fatal conditions that should stop execution.
- Log errors with appropriate information for later debugging.

## Type Handling

- Use explicit type checks where necessary (e.g., `objectp()`, `stringp()`, etc.).
- Prefer null-safe code using `nullp()` checks rather than relying on implicit conversions.
- When working with potentially missing or undefined values, check with `nullp()` before use.
- Remember that null/undefined values are ints and are 0, but they are not the same as 0, and can be tested with `nullp()`.
- After testing for null, you can then use `intp()` to check if the value is an int, particularly when validating call_other function arguments.

## File Organisation

- Each file should have a header comment describing its purpose and author.
- Header includes come first.
- Inherit statements, if necessary, follow header includes.
- Forward declarations come next.
- Global variables follow forward declarations.
- Helper/utility functions placed at the bottom of the file.
- Primary functions should be at the top of the file.
- Related functions should be grouped together.
- Keep file length manageable — consider splitting very large files (>1000 lines) into modules.

## Special LPC Features

### Efuns and Apply Functions

- Driver efuns (built-in functions) are called directly without a namespace.
- Apply functions (like `init()`, `heart_beat()`) are special methods called by the driver — don't rename these.
- Simulated efuns may be provided by the mudlib and called as global functions.

### Inheritance and Object References

- Use the available macros when inheriting (e.g., `inherit STD_ROOM;`).
- When inheriting, use standard method calling convention for inherited functions (e.g., `::setup();`).
- For loading objects, prefer `load_object()` for static instances and `clone_object()` for creating new instances.
- Use `this_object()` to refer to the current object and `previous_object()` for the calling object.

### LPC-Specific Data Handling

- Use `copy()` when returning mappings or arrays to avoid reference issues, unless you want to share the reference.
- Prefer explicit casting (e.g., `to_int()`, `to_string()`) over implicit type conversion.
- Use varargs for optional parameters: `varargs void func(int required, int optional)`
- The rest operator `...` can be used to handle variable argument lists, but only if the function is declared with `varargs`, and the syntax is `varargs void func(int required, int optional...)`.
- The spread operator `...` can be used to handle variable argument lists, and the syntax is `func(x, arg....)` where `arg` is an array of arguments to be spread by the function.

### Call Methods

- Use `call_other(obj, "func", args...)` or `obj->func(args...)` for dynamic method calls.

## Defensive Programming

- Validate all inputs, especially those from user input or network sources.
- Check for null with `nullp()` before operations that might fail on null values.
- For user-provided objects, verify they exist with `objectp()` before use.
- When dealing with user input, never trust it — sanitise it first.

## Line Width

- Keep lines within 80 columns. This is a MUD — content is consumed in terminal contexts where 80 columns is the norm.
- This applies to LPC source files and LPML data files alike.

## Performance Considerations

- Avoid expensive operations in frequently called functions.
- Cache results of expensive calculations where appropriate.
- Be mindful of eval cost limits for complex operations.
- Use call_out, or call_out_walltime for delayed execution rather than busy loops.
- Prefer filter/map/member_array over manual iteration where appropriate.
