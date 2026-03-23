# LLM Guide for gLPU

This document provides guidance for LLMs (Language Learning Models) when working with the gLPU (Gesslar's LPUniversity) mudlib codebase.

## IMPORTANT: Required Reading

Before working on this codebase, you MUST review these essential documentation files:

1. **[LLM LPC Coding Style.md](LLM LPC Coding Style.md)** - Coding style guidelines (spacing, bracing, naming conventions)
2. **[LLM LPCDoc Documentation Guide.md](LLM LPCDoc Documentation Guide.md)** - Documentation standards for LPCDoc comments
3. **[LLM Colour Coding Guide.md](LLM Colour Coding Guide.md)** - Extended color code reference for the MUD

These files contain critical information about code formatting, documentation standards, and project-specific conventions that you must follow.

## Project Overview

**gLPU** is a MUD (Multi-User Dungeon) library written in LPC (Lars Pensjö C), designed to run on FluffOS 2019+. It is a fork of LPUniversity, actively maintained and extended by Gesslar.

- **Language**: LPC (a C-like scripting language for MUDs)
- **Driver**: FluffOS 2019+
- **Project Type**: Game development framework/library for text-based multiplayer games
- **Documentation**: https://glpu.gesslar.dev/

## Key Directory Structure

```
/mud/ox/lib/
├── adm/          # Administrative files (daemons, simul_efuns, etc.)
├── cmds/         # Player and system commands
├── d/            # Domain areas (game world content)
├── data/         # Persistent data storage
├── doc/          # Documentation
├── home/         # Player home directories
├── include/      # Header files (.h)
├── lib/          # Library objects (coins, corpses, keys, daemons)
├── obj/          # Object files (items, NPCs)
├── open/         # Publicly accessible files
├── std/          # Standard inheritable objects
└── tmp/          # Temporary files
```

## LPC Language Characteristics

### File Extensions
- `.c` - LPC source files (objects, daemons, commands)
- `.h` - Header files (defines, macros, includes)

### Key Differences from C
1. **No explicit compilation**: Files are loaded/compiled by the driver on-demand
2. **Object-oriented**: Everything is an object, inheritance with `inherit`
3. **Special functions**: `create()`, `setup()`, `init()` are lifecycle hooks
4. **Data types**: Includes `mapping` (hash/dict), `object`, `mixed`
5. **Arrays**: Called "arrays" but syntax is `string *arr` or `int *arr`
6. **No pointers**: Despite `*` syntax, these aren't C pointers
7. **Built-in functions**: `sprintf()`, `call_other()`, `this_player()`, etc.

### Common LPC Data Types
- `int` - Integer
- `string` - String
- `float` - Floating point
- `object` - Reference to an LPC object
- `mapping` - Associative array/dictionary: `([ key: value ])`
- `mixed` - Any type
- `void` - No return value
- `<type> *` - Array of type (e.g., `string *names`)

### LPC Syntax Examples

```c
// Variable declarations
string name;
int *numbers = ({ 1, 2, 3 });
mapping data = ([ "key": "value" ]);

// Function definition
varargs int my_function(string arg, int optional) {
    // varargs means optional parameters allowed
    return 1;
}

// Inheritance
inherit STD_OBJECT;

// Preprocessor
#include <simul_efun.h>
#define MY_CONSTANT 100
```

## Code Style Guidelines

### Indentation
- **2 spaces** for indentation (NOT tabs)
- Consistent indentation throughout the file

### Brace Style
- Opening brace on same line as control statement
- Closing brace on its own line

```c
if(condition) {
    // code
} else {
    // code
}

void function() {
    // code
}
```

### Naming Conventions
- **Functions**: snake_case (`create_account`, `load_account`)
- **Variables**: snake_case (`account_name`, `player_data`)
- **Constants/Defines**: UPPER_SNAKE_CASE (`STD_DAEMON`, `MAX_PLAYERS`)
- **Private functions**: Prefix with underscore is optional but not required
- **Files**: snake_case.c (`account.c`, `string.c`)

### Spacing
- Space after `if`, `while`, `for`, `switch`
- NO space between function name and parentheses: `function(args)`
- Spaces around operators: `a + b`, `x == y`
- NO space after unary operators: `!flag`, `++i`

### Example from codebase:
```c
varargs string simple_list(string *arr, string conjunction) {
  assert_arg(pointerp(arr) && uniform_array(arr, T_STRING), 1, "Invalid or missing array.");

  conjunction = conjunction || "and";

  if(sizeof(arr) == 1)
    return arr[0];
  else if(sizeof(arr) == 2)
    return arr[0] + " " + conjunction + " " + arr[1];
  else
    return implode(arr[0..<2], ", ") + ", " + conjunction + " " + arr[<1];
}
```

## Documentation Standards

### LPCDoc Format
This project uses **LPCDoc** - a JSDoc-like documentation system for LPC code.

#### Function Documentation
```c
/**
 * @simul_efun append
 * @description Appends a string to another string if it is not already there.
 *              If the string is already present, the original string is returned.
 * @param {string} source - The string to append to.
 * @param {string} to_append - The string to append.
 * @returns {string} - The original string with the appended string if it was not
 *                     already present.
 */
string append(string source, string to_append) {
    // implementation
}
```

#### File Documentation
```c
/**
 * @file /adm/daemons/account.c
 *
 * Account daemon responsible for managing player accounts, including creation,
 * modification, and character associations.
 *
 * @created 2024-08-09 - Gesslar
 * @last_modified 2024-08-09 - Gesslar
 *
 * @history
 * 2024-08-09 - Gesslar - Created
 */
```

#### Common LPCDoc Tags
- `@file` - File path and purpose
- `@description` - Detailed description
- `@param {type} name - Description` - Parameter documentation
- `@returns {type} - Description` - Return value documentation
- `@example` - Usage examples
- `@created` - Creation date and author
- `@last_modified` - Last modification date and author
- `@history` - Change log
- `@simul_efun` - Marks simulated efuns (built-in function replacements)

## Common Patterns in gLPU

### Object Lifecycle
1. `void create()` - Called when object is first created
2. `void setup()` - Called after create, for additional setup
3. `void init()` - Called when object enters environment or player enters room

### Inheritance Patterns
```c
inherit STD_DAEMON;    // For daemon objects
inherit STD_OBJECT;    // For basic objects
inherit STD_ROOM;      // For room objects
```

### Simulated Efuns
- Located in [adm/simul_efun/](adm/simul_efun/)
- Extend or replace driver functions
- Examples: `append()`, `prepend()`, `simple_list()`, `no_ansi()`

### Common Utility Functions
- `nullp(x)` - Check if null/undefined
- `stringp(x)` - Check if string
- `intp(x)` - Check if integer
- `objectp(x)` - Check if object
- `pointerp(x)` - Check if array
- `mapp(x)` - Check if mapping
- `sizeof(arr)` - Get array/mapping size
- `strlen(str)` - Get string length

## Important Considerations

### Security
- **Permission checks**: Always validate permissions before sensitive operations
- **Input validation**: Validate all user input
- **Encryption**: Passwords must be encrypted (see `$6$` prefix in account.c)
- Use `assert_arg()` for parameter validation

### Performance
- **Lazy loading**: Objects loaded on-demand
- **Clean-up**: Use `set_no_clean(1)` for persistent objects
- **Persistence**: Use `set_persistent(1)` for data that survives reboots

### String Operations
- Use simul_efuns for common operations: `append()`, `prepend()`, `chop()`
- `extract()` for substring operations
- Range syntax: `str[0..5]`, `str[<3..]` (last 3 chars)

### Arrays and Mappings
- Arrays: `({ elem1, elem2, elem3 })`
- Mappings: `([ key1: value1, key2: value2 ])`
- Access: `arr[index]`, `map[key]`
- Operators: `+` for concatenation, `-` for removal

## Testing and Development

### File Paths
- Always use absolute paths from `/mud/ox/lib/`
- Relative paths in code start from driver root

### Common Commands (for reference)
- Code reloading happens automatically on file change
- Use `update` command to force object reload
- Check `/log/` directory for error logs

## Git Workflow

Current state (from git status):
- Main branch: `main`
- Modified files indicate active development
- Follow standard git practices for commits

## Additional Resources

- Documentation site: https://glpu.gesslar.dev/
- Contributing guide: https://glpu.gesslar.dev/contributing
- Original LPUniversity: http://dead-souls.net/

## Tips for Claude Code

1. **Always read before editing**: LPC files may have complex inheritance chains
2. **Respect existing patterns**: Match the coding style of surrounding code
3. **Document thoroughly**: Use LPCDoc format for all functions
4. **Test assumptions**: LPC syntax differs from C in subtle ways
5. **Check includes**: Header files define crucial constants and macros
6. **Understand inheritance**: Many features come from inherited objects
7. **Watch for simul_efuns**: Custom functions that override built-ins
8. **Be mindful of scope**: `private`, `public`, `nomask`, `protected` modifiers matter

## Common Gotchas

1. **Arrays aren't C pointers**: `string *arr` is an array, not a pointer
2. **No semicolon after closing brace**: Unlike C++, no `;` after class/struct
3. **Mapping syntax**: Use `:` not `=>` or `=` in mappings
4. **Array literals**: Use `({ })` not `[]`
5. **String concatenation**: Use `+` operator
6. **Range indexing**: `str[<3..]` means "last 3 characters"
7. **varargs**: Optional parameters must use `varargs` keyword
8. **No main()**: Entry points are lifecycle functions (`create`, `init`, etc.)
