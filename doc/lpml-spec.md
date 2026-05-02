# LPML Specification

**LPML** (LPC Markup Language) is a human-friendly data serialization format designed for MUD configuration files. It extends JSON5 with features tailored for LPC environments.

## Version

Version: 1.0.0
Created: 2025-11-09

## Design Goals

1. **Human-readable**: Easy to read and write by hand
2. **Composable**: Support file includes for modular configs
3. **Expressive**: Natural multiline strings without escape hell
4. **Compatible**: Full JSON/JSON5 compatibility
5. **Practical**: Fast enough for config files (not hot paths)

---

## Base Syntax (JSON5)

LPML is built on JSON5, which extends JSON with:

### Comments
```lpml
// Single-line comments
/* Multi-line
   comments */
```

### Unquoted Keys
```lpml
{
  name: "Gesslar",           // Simple identifier
  my cool key: "value",      // Spacey keys (YAML-style)
  additional ids: [1, 2, 3], // Works great!
  'quoted-key': "value"      // Use quotes when needed
}
```

**Spacey Keys:** LPML supports YAML-style keys with spaces - just write the key naturally and end it with `:`. The parser reads everything until the colon as the key name.

### Trailing Commas
```lpml
{
  foo: "bar",
  baz: 42,  // ← trailing comma is fine
}
```

### Single-Quoted Strings
```lpml
name: 'single quotes work too'
```

### Number Formats
```lpml
{
  hex: 0xFF,           // Hexadecimal
  octal: 0o77,         // Octal
  binary: 0b1010,      // Binary
  decimal: 3.14,       // Standard
  leadingDot: .5,      // Leading decimal point
  trailingDot: 5.,     // Trailing decimal point
  positive: +42        // Plus sign allowed
}
```

---

## LPML Extensions

### Spacey Keys (YAML-style)

Unlike JSON5, LPML allows keys with spaces without requiring quotes:

```lpml
{
  // All of these are valid:
  simple: "value",
  two words: "value",
  multiple word key: "value",
  crafting material: "leather",
  
  // Still works with quotes if you need them:
  "key: with colon": "value",
  'single quoted': "value"
}
```

**How it works:**
- The parser reads everything from the start of the key until it finds `:`
- Leading and trailing whitespace is trimmed
- Works alongside traditional identifiers and quoted keys

**Examples:**
```lpml
{
  // Character data with natural keys
  hit points: 100,
  mana points: 50,
  experience points: 1500,
  
  // Crafting materials
  crafting material: "yes",
  material type: "leather",
  quality level: "high"
}
```

### String Concatenation

Adjacent strings are automatically concatenated with intelligent spacing:

```lpml
// Concatenation with spaces (default)
bio: "A seasoned adventurer"
     "from the West."
// Result: "A seasoned adventurer from the West."

// Preserve newlines when strings end with \n
poem: "Line 1\n"
      "Line 2\n"
      "Line 3"
// Result: "Line 1\nLine 2\nLine 3"

// Mixed mode
text: "First paragraph."
      "Second sentence.\n"
      "New paragraph."
// Result: "First paragraph. Second sentence.\nNew paragraph."
```

**Rules:**
1. If a string ends with `\n`, the next string concatenates **without** adding a space
2. Otherwise, strings are joined with a single space
3. Works with any quote style: `"..."`, `'...'`

### Multiline Strings

Strings can span multiple lines in the source:

```lpml
// Source spans multiple lines
description: "This is a long
              description that spans
              multiple lines."
// Result: "This is a long description that spans multiple lines."
```

**Folding behavior:**
- Actual newlines in the source (pressing Enter) are converted to spaces
- Escape sequences like `\n` are preserved as actual newlines

### File Includes

Include external files with `"#path"` syntax:

```lpml
{
  // Absolute path
  config: "#/home/user/config.lpml",

  // Relative path (requires base_path parameter)
  stats: "#./stats.lpml",
  parent: "#../shared.lpml",

  // Missing files stay as literal strings (graceful degradation)
  optional: "#/missing/file.lpml"  // → "#/missing/file.lpml"
}
```

**Include behavior:**
1. The `#path` pattern is replaced with the file's contents during preprocessing
2. Included files are recursively preprocessed (supports nested includes)
3. Relative paths are resolved based on the `base_path` parameter
4. If a file doesn't exist, the include string is kept as-is (no error)
5. Works with both double and single quotes

**Escaping includes:**
```lpml
{
  channel: "\#general"  // → "#general" (literal)
}
```

The `\#` escape prevents include processing and removes the backslash.

---

## Data Types

### Primitives

```lpml
{
  string: "text",
  number: 42,
  float: 3.14,
  boolean: true,
  null: null
}
```

### Arrays

```lpml
{
  array: [1, 2, 3],
  mixed: ["string", 42, true, null],
  nested: [[1, 2], [3, 4]],
  trailing: [1, 2, 3,]  // Trailing comma OK
}
```

### Objects (Mappings)

```lpml
{
  simple: { x: 1, y: 2 },
  nested: {
    stats: {
      str: 10,
      dex: 15
    }
  },
  trailing: { a: 1, b: 2, }  // Trailing comma OK
}
```

---

## Special Values

### Infinity and NaN

```lpml
{
  inf: Infinity,      // Maps to undefined in LPC
  negInf: -Infinity,  // Maps to undefined in LPC
  notANumber: NaN     // Maps to undefined in LPC
}
```

**Note:** LPC doesn't have native Infinity/NaN support, so these are converted to `undefined` (accessed as `([])[0]`).

### MAX_INT and MAX_FLOAT

```lpml
{
  maxInt: MAX_INT,       // LPC maximum integer value
  minInt: -MAX_INT,      // Negated MAX_INT
  maxFloat: MAX_FLOAT,   // LPC maximum float value
  minFloat: -MAX_FLOAT,  // Negated MAX_FLOAT
}
```

**Note:** These are LPC-specific constants. Signs (`+`/`-`) are supported.

---

## Escape Sequences

Standard JSON escape sequences are supported:

```lpml
{
  newline: "line1\nline2",
  tab: "col1\tcol2",
  quote: "He said \"hello\"",
  backslash: "path\\to\\file",
  unicode: "\u0041"  // 'A'
}
```

Supported escapes:
- `\"` - Double quote
- `\'` - Single quote
- `\\` - Backslash
- `\/` - Forward slash
- `\b` - Backspace
- `\f` - Form feed
- `\n` - Newline
- `\r` - Carriage return
- `\t` - Tab
- `\uXXXX` - Unicode code point

---

## Usage in LPC

### Decoding

```lpc
// Basic usage
mixed data = lpml_decode(file_text);

// With base_path for relative includes
mixed data = lpml_decode(
  read_file("/config/main.lpml"),
  "/config"
);
```

**Signature:**
```lpc
varargs mixed lpml_decode(string text, string base_path)
```

**Parameters:**
- `text` - The LPML string to parse
- `base_path` - (Optional) Base directory for resolving relative includes

**Returns:**
- Parsed LPC data structure (mapping, array, or primitive)
- `0` if text is null/empty

### Encoding

```lpc
// Encode LPC data to LPML/JSON
string text = lpml_encode(data);
```

**Signature:**
```lpc
string lpml_encode(mixed value)
```

**Returns:**
- JSON-formatted string (standard JSON, not LPML extensions)

**Note:** Encoding produces standard JSON. LPML extensions (comments, includes, string concatenation) are **read-only** features.

---

## File Extension

Recommended: `.lpml`

Alternative: `.json` or `.json5` for editor syntax highlighting

---

## Examples

### Character Configuration

```lpml
// character.lpml
{
  name: "Gesslar",
  title: "Wielder of Sharp Things",
  
  // Load external configs
  stats: "#./stats.lpml",
  inventory: "#./inventory.lpml",
  
  // Spacey keys for natural language
  hit points: 100,
  max hit points: 120,
  experience points: 1500,
  
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

### Item/Loot Configuration

```lpml
// cat_fur.lpml
{
  id: ["fur"],
  additional ids: ["hide", "piece"],  // Spacey key!
  adj: ["cat", "soft"],
  name: "cat fur",
  short: "a piece of cat fur",
  long: "This is a soft piece of fur from a wild cat."
        "It could be useful for crafting.",
  mass: 20,
  material: ["fur"],
  properties: {
    autovalue: "yes",
    crafting material: "yes",  // Spacey key!
  }
}
```

### Multi-line Text with Formatting

```lpml
{
  // Paragraph with auto-folding
  description: "This is a long description that"
               "spans multiple lines in the source"
               "but will be a single paragraph.",

  // Preserve line breaks with \n
  poem: "Roses are red,\n"
        "Violets are blue,\n"
        "LPML is awesome,\n"
        "And so are you.",

  // Mix folding and newlines
  help: "Usage: command [options]\n"
        "\n"
        "This command does something useful."
        "It has multiple paragraphs.\n"
        "\n"
        "See 'help topics' for more info."
}
```

### Modular Configuration

**main.lpml:**
```lpml
{
  game: {
    name: "My MUD",
    port: 4000,

    // Pull in modular configs
    database: "#./db-config.lpml",
    features: "#./features.lpml",
    discord: "#./discord-config.lpml"
  }
}
```

**db-config.lpml:**
```lpml
{
  host: "localhost",
  port: 5432,
  name: "mud_db",
  pool_size: 10
}
```

---

## Implementation Notes

### Performance

- **Parsing cost:** ~2x slower than `json_decode()`
- **Why:** UTF-8 safety, comment parsing, string concatenation, includes
- **Recommendation:** Use for config files (parsed once), not hot paths

### UTF-8 Safety

LPML uses buffer-based encoding for UTF-8 safety:
- Source text is converted to buffer via `string_encode(text, "utf-8")`
- Buffer indexing is O(1) byte access (fast and safe)
- Final strings are decoded via `string_decode(buffer, "utf-8")`

### Compatibility

- **Reads:** JSON, JSON5, LPML
- **Writes:** JSON (standard format)

---

## Differences from JSON5

| Feature | JSON5 | LPML |
|---------|-------|------|
| Comments | ✓ | ✓ |
| Trailing commas | ✓ | ✓ |
| Unquoted keys | ✓ | ✓ |
| Spacey keys | ✗ | ✓ |
| Single quotes | ✓ | ✓ |
| Hex numbers | ✓ | ✓ |
| Octal/binary numbers | ✗ | ✓ |
| MAX_INT/MAX_FLOAT | ✗ | ✓ |
| String concatenation | ✗ | ✓ |
| File includes | ✗ | ✓ |
| Multiline folding | ✗ | ✓ |

---

## Limitations

1. **Encoding is JSON only** - LPML extensions are not preserved when encoding
2. **Include preprocessing** - Includes are resolved before parsing (text substitution)
3. **No schema validation** - LPML doesn't enforce structure (like JSON)
4. **Platform-specific** - Designed for LPC/FluffOS environments

---

## Grammar Summary

```
value       ::= object | array | string | number | boolean | null
                | 'undefined' | 'Infinity' | 'NaN' | 'MAX_INT' | 'MAX_FLOAT'
object      ::= '{' members? '}'
members     ::= pair (',' pair)* ','?
pair        ::= key ':' value
key         ::= identifier | spacey_key | string
spacey_key  ::= (char - ':')+ ':'           // trimmed, unquoted key with spaces
array       ::= '[' elements? ']'
elements    ::= value (',' value)* ','?
string      ::= '"' chars '"' | "'" chars "'" | string string  // concatenation
number      ::= hex | octal | binary | decimal
boolean     ::= 'true' | 'false'
null        ::= 'null'
comment     ::= '//' [^\n]* | '/*' .*? '*/'
```

---

## Credits

Created by Gesslar for the Oxidus MUD codebase.

Based on:
- JSON5 specification
- LPC/FluffOS runtime
- Lessons learned from YAML complexity

---

## License

This specification is provided as-is for use in LPC MUD development.
