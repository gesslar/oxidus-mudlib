---
name: lpml
description: LPML (LPC Markup Language) specification reference. Consult when reading, writing, editing, or validating .lpml files. Covers syntax, data types, string concatenation, spacey keys, file includes, multiline folding, and all extensions over JSON5.
---

# LPML (LPC Markup Language) Specification v1.0.0

LPML is a human-friendly data serialization format designed for MUD configuration files. It extends JSON5 with LPC-environment-specific features.

## Design Goals

1. Easy to read and write by hand
2. Optional file includes for modular configs
3. Natural multiline strings without escape hell
4. Full JSON/JSON5 compatibility
5. Fast enough for config files (not hot paths)

## File Extension

The recommended extension is `.lpml`. Alternatives `.json` or `.json5` may be used for editor syntax highlighting.

## Base Syntax (JSON5)

LPML inherits all JSON5 features:

### Comments

```lpml
// Single-line comment
/* Multi-line
   comment */
```

### Unquoted Keys

Keys can be written without quotes if they are valid identifiers:

```lpml
{ name: "Gesslar", level: 10 }
```

### Trailing Commas

Permitted in objects and arrays:

```lpml
{ a: 1, b: 2, }
[1, 2, 3,]
```

### Single-Quoted Strings

```lpml
{ name: 'single quotes work too' }
```

### Number Formats

```lpml
{
  hex: 0xFF,
  octal: 0o77,
  binary: 0b1010,
  decimal: 3.14,
  leading_dot: .5,
  trailing_dot: 5.,
  plus_sign: +42,
}
```

## LPML Extensions

These features go beyond JSON5:

### Spacey Keys

Keys with spaces do not require quotes. The parser reads everything from the start of the key until it finds `:`. Leading and trailing whitespace is trimmed.

```lpml
{
  hit points: 100,
  max hit points: 120,
  experience points: 1500,
}
```

Spacey keys work alongside quoted keys and standard unquoted keys.

### String Concatenation

Adjacent string values automatically concatenate. The joining rule depends on whether the preceding string ends with `\n`:

- If a string ends with `\n`, the next string concatenates **without** adding a space.
- Otherwise, strings are joined with a single space.

This works across quote styles (double and single quotes can be mixed).

```lpml
{
  // These join with spaces (no trailing \n):
  bio: "A seasoned adventurer from the West."
       "Known for incredible fashion sense."
       "Has a pet dragon named Sparky.",
  // Result: "A seasoned adventurer from the West. Known for incredible fashion sense. Has a pet dragon named Sparky."

  // Lines ending with \n concatenate WITHOUT a space:
  poem: "Roses are red,\n"
        "Violets are blue,\n"
        "LPML is awesome,\n"
        "And so are you.",
  // Result: "Roses are red,\nViolets are blue,\nLPML is awesome,\nAnd so are you."

  // Mixed: \n suppresses the space, no \n adds a space
  help: "Usage: command [options]\n"
        "\n"
        "This command does something useful."
        "It has multiple paragraphs.\n"
        "\n"
        "See 'help topics' for more info.",
  // Result: "Usage: command [options]\n\nThis command does something useful. It has multiple paragraphs.\n\nSee 'help topics' for more info."
}
```

### Multiline Strings (Source Line Folding)

Actual newlines in the source (pressing Enter) are converted to spaces. Escape sequences like `\n` remain as literal newline characters. This means you write long strings naturally across source lines without worrying about embedded newlines.

### File Includes

Values beginning with `#` followed by a path denote file includes.

```lpml
{
  stats: "#./stats.lpml",
  inventory: "#./inventory.lpml",
  database: "#./db-config.lpml",
}
```

Rules:
- A conforming implementation SHOULD replace the `#path` value with the parsed contents of the referenced file.
- File includes are processed recursively (included files may themselves contain includes).
- If a file cannot be found, an implementation SHOULD keep the include string as its literal value.
- To produce a literal `#` at the start of a string (preventing include processing), escape it: `\#`.

```lpml
{
  color_code: "\#FF0000",  // Literal string "#FF0000", not a file include
}
```

## Data Types

### Primitives

```lpml
{
  string: "text",
  number: 42,
  float: 3.14,
  boolean: true,
  null: null,
}
```

### Arrays

Arrays support homogeneous, mixed-type, and nested elements with trailing commas:

```lpml
{
  array: [1, 2, 3],
  mixed: ["string", 42, true, null],
  nested: [[1, 2], [3, 4]],
  trailing: [1, 2, 3,],
}
```

### Objects (Mappings)

Objects support nesting and trailing commas:

```lpml
{
  simple: { x: 1, y: 2 },
  nested: {
    stats: { str: 10, dex: 15 },
  },
  trailing: { a: 1, b: 2, },
}
```

### Special Values

#### Infinity and NaN

`Infinity`, `-Infinity`, and `NaN` are valid values. In LPC, these map to `undefined` (equivalent to `([])[0]`) because LPC does not have native Infinity/NaN support.

```lpml
{
  inf: Infinity,
  neg_inf: -Infinity,
  nan: NaN,
}
```

#### MAX_INT and MAX_FLOAT

LPC-specific constants that support sign prefixes (`+`/`-`):

```lpml
{
  maxInt: MAX_INT,
  minInt: -MAX_INT,
  maxFloat: MAX_FLOAT,
  minFloat: -MAX_FLOAT,
}
```

### Escape Sequences

Standard JSON escape sequences are supported in strings:

| Escape | Meaning |
|--------|---------|
| `\"` | Double quote |
| `\'` | Single quote |
| `\\` | Backslash |
| `\/` | Forward slash |
| `\b` | Backspace |
| `\f` | Form feed |
| `\n` | Newline |
| `\r` | Carriage return |
| `\t` | Tab |
| `\uXXXX` | Unicode character |
| `\#` | Literal `#` (prevents file include processing) |

## Differences from JSON5

| Feature | JSON5 | LPML |
|---------|-------|------|
| Comments | yes | yes |
| Trailing commas | yes | yes |
| Unquoted keys | yes | yes |
| Spacey keys | no | yes |
| Single quotes | yes | yes |
| Hex numbers | yes | yes |
| Octal/binary numbers | no | yes |
| MAX_INT/MAX_FLOAT | no | yes |
| String concatenation | no | yes |
| File includes | no | yes |
| Multiline folding | no | yes |

## Formatting Guidelines

- Keep lines within 80 columns. LPML files are MUD configuration data consumed in terminal contexts where 80 columns is the norm.
- Break long string concatenation values across multiple lines to stay within the limit.
- Break long arrays across multiple lines if they would exceed 80 columns.

## Complete Example

```lpml
// character.lpml
{
  name: "Gesslar",
  title: "Wielder of Sharp Things",

  // File includes
  stats: "#./stats.lpml",
  inventory: "#./inventory.lpml",

  // Spacey keys
  hit points: 100,
  max hit points: 120,
  experience points: 1500,

  // String concatenation (space-joined)
  bio: "A seasoned adventurer from the West."
       "Known for incredible fashion sense."
       "Has a pet dragon named Sparky.",

  skills: {
    combat: 85,
    magic: 60,
    social: 75,
  },
}
```

## Item Configuration Example

```lpml
// cat_fur.lpml
{
  id: ["fur"],
  additional ids: ["hide", "piece"],
  adj: ["cat", "soft"],
  name: "cat fur",
  short: "a piece of cat fur",
  long: "This is a soft piece of fur from a wild cat."
        "It could be useful for crafting.",
  mass: 20,
  material: ["fur"],
  properties: {
    autovalue: "yes",
    crafting material: "yes",
  },
}
```

## Modular Configuration Example

```lpml
// main.lpml
{
  game: {
    name: "My MUD",
    port: 4000,
    database: "#./db-config.lpml",
    features: "#./features.lpml",
    discord: "#./discord-config.lpml",
  },
}
```

```lpml
// db-config.lpml
{
  host: "localhost",
  port: 5432,
  name: "mud_db",
  pool_size: 10,
}
```

## Credits

LPML was created by Gesslar, based on the JSON5 specification. Licensed under the Unlicense (public domain).
