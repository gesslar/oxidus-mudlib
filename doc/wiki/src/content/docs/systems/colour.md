---
title: Colour
description: True colour rendering, text formatting, accessibility, and player preferences.
---

Oxidus uses a true colour system based on hexadecimal RGB values enclosed in double braces. It supports full 24-bit colour, the 256-colour ANSI palette, and a range of text formatting attributes. Colours are applied throughout the game -- in descriptions, system messages, combat output, and more -- and are processed through a central daemon before reaching the player's terminal.

## Colour Code Syntax

Foreground colour codes use double-brace delimiters (`{{...}}`), and background colour codes use brace-caret delimiters (`{^...^}`).

### Hex Colours

Specify a hex RGB value to set the text (foreground) colour:

| Syntax | Example | Result |
|---|---|---|
| `{{RRGGBB}}` | `{{FF0000}}` | Red text |
| `{{RGB}}` | `{{F00}}` | Red text (shorthand) |

The 3-digit shorthand expands each digit by doubling it, so `{{F00}}` is equivalent to `{{FF0000}}`, and `{{096}}` becomes `{{009966}}`.

### Background Colours

Specify a hex RGB value with brace-caret delimiters to set the background colour:

| Syntax | Example | Result |
|---|---|---|
| `{^RRGGBB^}` | `{^0000FF^}` | Blue background |
| `{^RGB^}` | `{^00F^}` | Blue background (shorthand) |

The same 3-to-6 digit expansion applies. Background and foreground colours can be combined freely:

```lpc
set_short("a {{FFFFFF}}{^FF0000^}warning sign{{res}}");  // white text on red
```

You do not need to reset between colour changes -- the system simply switches to the new colour. Use `{{res}}` or `{{RES}}` when you want to return to the terminal's default.

```lpc
// These are equivalent
set_short("a {{FF0000}}red{{res}} ball");
set_short("a {{F00}}red{{res}} ball");
```

### Reset

| Code | Purpose |
|---|---|
| `{{res}}` | Reset all formatting and colour to default |
| `{{RES}}` | Same (case-insensitive alias) |

### Text Formatting

Formatting attributes use a two-letter code followed by `1` (on) or `0` (off):

| Code | On | Off | Effect |
|---|---|---|---|
| `bl` | `{{bl1}}` | `{{bl0}}` | **Bold** |
| `di` | `{{di1}}` | `{{di0}}` | Dim |
| `it` | `{{it1}}` | `{{it0}}` | *Italic* |
| `ul` | `{{ul1}}` | `{{ul0}}` | Underline |
| `fl` | `{{fl1}}` | `{{fl0}}` | Flash (blink) |
| `re` | `{{re1}}` | `{{re0}}` | Reverse video |
| `st` | `{{st1}}` | `{{st0}}` | ~~Strikethrough~~ |
| `ol` | `{{ol1}}` | `{{ol0}}` | Overline |

Formatting attributes can be combined with colours:

```lpc
tell(caller, "{{bl1}}{{FF0000}}Bold red text{{res}}");
tell(caller, "This is {{ul1}}underlined{{ul0}} and this is not.");
```

:::caution
Not all terminals support every attribute. Flash, overline, and strikethrough may not render in every client.
:::

## System Colour Constants

The header `include/colour.h` defines named constants for consistent use across game systems. Use these instead of hard-coded hex values for system-level messages.

| Constant | Hex | Colour | Use |
|---|---|---|---|
| `SYSTEM_OK` | `{{009966}}` | Green | Success messages |
| `SYSTEM_ERROR` | `{{CC0000}}` | Red | Error messages |
| `SYSTEM_WARNING` | `{{FF9900}}` | Orange | Warning messages |
| `SYSTEM_INFO` | `{{0099CC}}` | Cyan | Informational messages |
| `SYSTEM_QUERY` | `{{0066FF}}` | Blue | Query/prompt messages |
| `SYSTEM_DEBUG` | `{{CC00CC}}` | Magenta | Debug messages |

```lpc
#include <colour.h>

// In a command
tell(caller, SYSTEM_ERROR + "Something went wrong.{{res}}");
tell(caller, SYSTEM_OK + "Operation complete.{{res}}");
```

## Usage in Game Content

### Descriptions

Colour codes can appear anywhere in short descriptions, long descriptions, and extra descriptions:

```lpc
set_short("a {{FC3}}massive hammer{{res}}");
set_long("The hammer glows with {{FF6600}}fiery orange light{{res}}, "
         "its head etched with {{AAAAAA}}silvery runes{{res}}.");
```

### Dynamic Colour

Build colour codes dynamically from computed hex values:

```lpc
string hex = "FF8800";
string coloured = sprintf("{{%s}}%s{{res}}", hex, text);
```

### Gradients

The `gradient_hex()` simul_efun shifts a colour by a uniform step across all RGB channels:

```lpc
string base = "FF0000";
string lighter = gradient_hex(base, 40.0);   // brighter red
string darker  = gradient_hex(base, -40.0);  // darker red
```

## The Colour Daemon (COLOUR_D)

The colour daemon at `adm/daemons/colour.lpc` is the central engine for colour processing. It parses colour codes in text, converts them to ANSI escape sequences, handles accessibility adjustments, and provides colour conversion utilities.

### Core Functions

| Function | Description |
|---|---|
| `substitute_colour(text, mode)` | Parse and replace colour codes. Mode `"on"` converts to ANSI escape sequences; any other mode strips them entirely. |
| `wrap(str, wrap_at, indent_at)` | Word-wrap text to a given width while preserving colour codes. Wrapped lines are indented by `indent_at` spaces. |
| `bod_colour_replace(body, text, message_type)` | Apply body-specific colour overrides (e.g. combat hit/miss colours) based on player preferences and message type flags. |
| `colourp(text)` | Returns `1` if the text contains any colour code, `0` otherwise. |
| `get_colour_list()` | The rendered swatch listing used by `colour list`. |
| `base_colours()` | The 16 base ANSI colours and their RGB values. |
| `query_colour_cache()` | A copy of the tag-to-escape-sequence cache. |
| `resync()` | Rebuilds the attribute and 256-colour caches and re-reads the luminance floor. |

### Conversion Functions

| Function | Description |
|---|---|
| `hex_to_rgb(hex)` | Convert a hex string (3 or 6 digit, bare or wrapped in `{{ }}` / `{^ ^}`) to an `({ r, g, b })` array. |
| `rgb_to_hex(rgb)` | Convert an `({ r, g, b })` array to a 6-digit hex string. |
| `rgb_to_sequence(rgb, mode)` | Convert RGB to an ANSI escape sequence. Mode `0` = foreground, `1` = background. |
| `hex_to_sequence(hex, mode)` | Convert a hex string directly to an ANSI escape sequence. `mode` is optional and defaults to foreground. |
| `colour_to_rgb(code)` | Convert a 256-colour ANSI code (0--255) to an `({ r, g, b })` array. |
| `rgb_to_colour(r, g, b)` | Convert RGB values to the nearest 256-colour code. |
| `colour_to_greyscale(code)` | Convert a colour code to its closest greyscale equivalent (232--255). |
| `get_luminance(rgb)` | Perceived brightness of an RGB triple, `0.2126R + 0.7152G + 0.0722B`. |
| `substitute_too_dark(hex)` | Lighten a colour that falls below the luminance floor. See [Accessibility](#accessibility). |
| `ansi256_to_3hex(ansi)` | Convert an ANSI 256 code to the nearest 3-digit hex shorthand. Lossy by construction -- shorthand digits sit 17 apart and the xterm cube's levels do not land on those boundaries -- so it rounds to the nearest rather than truncating. |
| `random_bright_rgb()` / `random_bright_hex()` | A random colour with every channel in 80--255, bright enough to skip the floor check. |

### How Substitution Works

When `substitute_colour()` processes a string:

1. The text is split into colour-code tokens and plain-text segments using `pcre_assoc()` with the regex patterns from `include/colour.h`. Foreground and background tokens are matched as separate groups, so each is converted with the right SGR base.
2. In `"on"` mode, each colour token is looked up in a cache (or converted and cached on first use)
3. In any other mode, each colour token is replaced with an empty string
4. The segments are joined back together

The daemon pre-caches all 256 ANSI colours and all text formatting attributes at startup for performance.

## The Message Pipeline

Colours are resolved at the point of delivery, not at the point of creation. This means game code writes colour markup (e.g. `{{FF0000}}`), and the messaging system converts or strips it based on the recipient's preferences.

The flow in `std/ext/messaging.lpc` is:

1. A message arrives at `do_receive()` with a message type flag
2. The player's `colour` preference is checked
3. If colour is off, the `NO_COLOUR` flag is set
4. If colour is on, `bod_colour_replace()` is called to apply per-message-type colour overrides (e.g. combat colours)
5. `substitute_colour()` converts the markup to ANSI sequences (or strips it)
6. The processed text is sent to the player's connection

For non-user objects (NPCs, items), colour is always stripped.

## Colour-Aware Text Wrapping

The `wrap()` function handles line wrapping without breaking colour codes. It:

- Splits text on spaces and tracks the visible (non-colour) length of each line
- Strips colour codes when measuring width so that escape sequences do not count towards the wrap column
- Inserts line breaks and indentation while preserving the current colour state
- Maintains existing newlines and leading whitespace

```lpc
// Wrap text at 78 columns, indent continuation lines by 4 spaces
string wrapped = COLOUR_D->wrap(long_text, 78, 4);
```

## Accessibility

### Dark Colour Substitution

`substitute_too_dark()` lightens a colour that would be too dark to read on a dark terminal background. The luminance floor comes from the `COLOUR_MININUM_LUMINANCE` configuration key.

Given a colour code:

1. `get_luminance()` calculates perceived brightness using the standard formula: `0.2126R + 0.7152G + 0.0722B`
2. A colour already at or above the floor is returned untouched
3. Anything darker is lifted **additively** -- the same amount is added to every channel, clamped to 0--255, and repeated until the colour clears the floor or no channel can rise any further. Because the luminance weights total 1.0, one lift raises luminance by exactly the amount added; the repeat exists for colours with a channel already saturated at 255.

The answer comes back in whichever form it was asked in -- bare hex, `{{fg}}`, or `{^bg^}`.

:::note
This is applied at the point a colour is *chosen*, not on every message. The `set <name>_colour prompt` and `env <name>_colour prompt` flows run the player's selection through `substitute_too_dark()` before storing it, so an unreadable choice is corrected once rather than on every render.
:::

### Greyscale Conversion

`colour_to_greyscale()` maps any 256-colour code to the closest greyscale shade (codes 232--255), useful for accessibility modes or effects.

### Colour Stripping

Two convenience functions strip all colour codes from text:

- `COLOUR_D->substitute_colour(text, "off")` -- daemon call
- `no_ansi(text)` -- simul_efun wrapper (calls the daemon internally)

These are used for length calculations, logging, screen reader output, and anywhere colour markup would be inappropriate.

## Player Preferences

Players configure colour through the `set` and `colour` commands. The following preferences are available:

| Preference | Default | Description |
|---|---|---|
| `colour` | `on` (`off` for ghosts) | Enable or disable colour output. Any value other than `on` strips colour from everything you receive. |
| `combat_hit_colour` | unset | Colour applied to combat messages where you land a hit |
| `combat_miss_colour` | unset | Colour applied to combat messages where you miss |

Developers get a further set of `ls_*` colour preferences that tint the `ls`
listing by file kind. See [Preferences](/guides/preferences/) for the full list.

### The `colour` Command

| Usage | Effect |
|---|---|
| `colour` | Display current colour status |
| `colour on` | Enable colour |
| `colour off` | Disable colour |
| `colour list` | Show all available colours |
| `colour show <0-255>` | Preview a specific ANSI colour code in foreground and background |

## The CSS Module

The colour daemon inherits a CSS mapping module (`adm/daemons/modules/colour/css.lpc`) that provides bidirectional lookup between all 256 ANSI colour codes and their hex equivalents:

| Function | Description |
|---|---|
| `colour_to_hex(code)` | Convert an ANSI code (0--255) to a hex string like `#FF0000`. Call with no argument to get the entire mapping. |
| `hex_to_colour(hex)` | Convert a hex string to an ANSI code string. Call with no argument to get the entire mapping. |

## Simul_efun Utilities

These globally available functions are defined in `adm/simul_efun/`:

| Function | File | Description |
|---|---|---|
| `gradient_hex(hex, step)` | `colour.lpc` | Shift a hex colour by a uniform step across all channels, returning a `{{RRGGBB}}` colour code string. |
| `colourp(str)` | `string.lpc` | Returns `1` if the string contains any colour codes, `0` otherwise. |
| `no_ansi(str)` | `string.lpc` | Strip all colour codes from a string. |

## ANSI Escape Sequences

Under the hood, the daemon converts hex codes to 24-bit true colour ANSI escape sequences:

- Foreground: `\e[38;2;R;G;Bm` (from `{{RRGGBB}}`)
- Background: `\e[48;2;R;G;Bm` (from `{^RRGGBB^}`)

For example, `{{FF0000}}` becomes `\e[38;2;255;0;0m`, `{^0000FF^}` becomes `\e[48;2;0;0;255m`, and `{{res}}` becomes `\e[0m`.

Text formatting attributes map to standard SGR codes:

| Attribute | On | Off |
|---|---|---|
| Bold | `\e[1m` | `\e[22m` |
| Dim | `\e[2m` | `\e[22m` |
| Italic | `\e[3m` | `\e[23m` |
| Underline | `\e[4m` | `\e[24m` |
| Blink | `\e[5m` | `\e[25m` |
| Reverse | `\e[7m` | `\e[27m` |
| Strikethrough | `\e[9m` | `\e[29m` |
| Overline | `\e[53m` | `\e[55m` |

## Key Files

| File | Role |
|---|---|
| `adm/daemons/colour.lpc` | Colour daemon -- parsing, conversion, wrapping, accessibility |
| `adm/daemons/modules/colour/css.lpc` | ANSI 256 to hex bidirectional mapping |
| `adm/simul_efun/colour.lpc` | `gradient_hex()` simul_efun |
| `adm/simul_efun/string.lpc` | `colourp()` and `no_ansi()` simul_efuns |
| `include/colour.h` | Regex patterns and system colour constants |
| `cmds/std/colour.lpc` | Player `colour` command |
| `std/ext/messaging.lpc` | Message delivery pipeline with colour integration |
| `doc/help/general/preferences.help` | In-game `help preferences` text |
