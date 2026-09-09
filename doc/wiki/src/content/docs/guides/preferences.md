---
title: Preferences
description: Per-character settings managed with the set command, and developer settings managed with env.
sidebar:
  order: 2
---

Preferences are per-character settings you manage with the `set` command. They
control colour, paging, notifications, GMCP, and combat readouts. They are saved
with your character.

Developers have a second, separate store managed with the `env` command. The two
are not interchangeable -- see [Developer Settings](#developer-settings) below.

## Setting a Preference

```text
set                          list your current preferences
set <preference> <value>     set a preference
set <preference>             clear a preference
set <name>_colour prompt     pick a colour interactively
```

Any preference whose name ends in `_colour` accepts the value `prompt` to choose
from the available colours instead of typing a code. The colour you pick is run
through the daemon's minimum-luminance check before it is stored, so an
unreadable choice is corrected on the way in -- see
[Colour](/systems/colour/#dark-colour-substitution).

## Player Preferences

| Preference | Default | Description |
|---|---|---|
| `colour` | `on` (`off` for ghosts) | Sends colour to your terminal. Any value other than `on` strips colour from everything you receive. |
| `prompt` | `>` | The text written before each of your commands. A space is added after it. |
| `morelines` | `40` | Lines shown per page before the pager pauses. Also governs how many lines the `log` command tails. |
| `page_display` | `line` | How the pager reports your position: `line` for line numbers, anything else for a percentage readout. |
| `unicode` | unset (off) | Set to `on` to receive Unicode characters where available. Always treated as off while a screen reader is active. |
| `screenreader` | unset (off) | Set to `on` for screen reader mode, which suppresses Unicode and message decoration. Auto-detected clients get this without setting it. |
| `feedback` | `on` | Whether system messages (`ok`, `error`, `warn`, `info`) carry their decoration symbol. Set to `off` for plain text. |
| `gmcp` | unset (enabled) | [GMCP](/systems/gmcp/) data is sent to clients that negotiate it. Set to `off` to stop sending it. |
| `keepalive` | unset (off) | Set to any value other than `off` to send periodic keepalive packets once you have been idle for five minutes. |
| `health_bar` | unset (on) | Shows your HP, SP, and MP each combat round, and whenever they change while you recover out of combat. Set to `off` to hide it. |
| `recovery_messages` | unset (on) | Tells you when your health, mind, or body has finished recovering. Set to `off` to stay quiet. |
| `combat_hit_colour` | unset | Colour applied to combat messages where you land a hit. Unset leaves them uncoloured. |
| `combat_miss_colour` | unset | Colour applied to combat messages where you miss. Unset leaves them uncoloured. |

## Developer Preferences

These take effect only for characters holding the developer role. They are still
`set` preferences, not `env` settings.

| Preference | Default | Description |
|---|---|---|
| `look_filename` | unset (off) | Set to `on` to show the source filename above the description of rooms, objects, and livings when you `look`. |
| `error_output` | unset (enabled) | Compile and runtime errors attributed to you are echoed to your screen as well as written to your home log. Set to `off` to silence them. |
| `log_level` | `0` | Verbosity of body event logging, `1` (least) to `4` (most). `0` or unset disables it; higher levels are noisy. |

### `ls` Colours

The `ls` listing tints each entry by file kind, and each kind is its own
preference. `set ls_source 6cf` recolours source files.

| Preference | Default |
|---|---|
| `ls_directory` | `5087b4` |
| `ls_source` | `b0cabf` |
| `ls_loaded` | `04e5ad` |
| `ls_header` | `a2c5b2` |
| `ls_save` | `c5baa5` |
| `ls_data` | `9e93d4` |
| `ls_other` | `bbb` |

## Administrator Preferences

There are no preferences specific to the administrator role. Administrators use
the player preferences above, plus the developer preferences if they also hold
the developer role.

## Developer Settings

`env` manages a second store, separate from `set`. Both persist with your
character; the split is by purpose. Preferences are how the *game* presents
itself to you. Settings are wizard shell state -- where you are working, and how
your presence reads to other players.

```text
env                          list your current settings
env <setting> <value>        set a setting
env <setting>                clear a setting
env <name>_colour prompt     pick a colour interactively
```

| Setting | Description |
|---|---|
| `move_in` | Replaces the arrival message shown to the room you move into. `$N` -> your name. Defaults to `$N arrives.` |
| `move_out` | Replaces the departure message shown to the room you move out of. `$N` -> your name, `$D` -> the exit taken. Defaults to `$N leaves $D.` |
| `start_location` | Set to `last_location` to log in wherever you left off. Any other value -- including unset -- puts you in the workroom in your home directory. |
| `cwd` | Your current working directory. Managed by the file commands (`cd`, `ls`); you rarely set it by hand. |
| `cwf` | Your current working file, updated by commands like `more` and `mv` so the next command can default to it. |

Changing either store emits a signal -- `SIG_USER_PREF_CHANGED` for `set`,
`SIG_USER_ENV_CHANGED` for `env` -- so other systems can react. See
[Signals](/systems/signal/).
