---
name: mud-configuration
description: Understand and work with the MUD configuration system for Oxidus. Covers the CONFIG_D daemon, cascading LPML config files (default.lpml and config.lpml), the mudConfig() simul_efun, available configuration keys, adding new keys, the mudconfig command, and rehashing configuration at runtime.
---

# MUD Configuration Skill

You are helping work with the Oxidus MUD configuration system. Follow the `lpc-coding-style` skill for all LPC formatting.

## Architecture Overview

The configuration system uses a **cascading two-file pattern**:

1. `/adm/etc/default.lpml` — shipped defaults (tracked in git)
2. `/adm/etc/config.lpml` — local overrides (not in git, survives upgrades)

Both files use LPML format (see the `lpml` skill). Local overrides merge on top of defaults using mapping addition (`+=`), so any key in `config.lpml` replaces the same key from `default.lpml`.

## Key Components

### CONFIG_D — `/adm/daemons/config.c`

The central daemon. Inherits `STD_DAEMON`.

| Function | Signature | Purpose |
|---|---|---|
| `get_mud_config` | `mixed get_mud_config(string key)` | Get a single config value by key. Errors if key is missing or invalid. |
| `get_all_config` | `mapping get_all_config()` | Returns a copy of the entire config mapping. |
| `rehash_config` | `void rehash_config()` | Reloads both files and re-merges. Called automatically on startup. |

### mudConfig() simul_efun — `/adm/simul_efun/system.c`

```lpc
mixed mudConfig(string key)
```

Convenience wrapper. Calls `CONFIG_D->get_mud_config(key)`. This is the standard way to read config from anywhere in the mudlib.

### mudconfig command — `/cmds/wiz/mudconfig.c`

Wizard command that dumps all current configuration via `pretty_map()`. No arguments.

### Rehashing at runtime

Use the `master` admin command, which calls `CONFIG_D->rehash_config()` among other reloads. Or call `CONFIG_D->rehash_config()` directly.

## Configuration Keys

All keys live in the top-level mapping of the LPML files. Keys are uppercase by convention. The following categories exist in `default.lpml`:

### Library metadata
- `LIB_VERSION` — library version string
- `LIB_NAME` — library name
- `OPEN_STATUS` — mud status description

### System paths and logging
- `ADMIN_EMAIL` — admin contact
- `LOG_DIR`, `LOG_CATCH`, `LOG_RUNTIME` — log paths
- `TMP_DIR` — temporary file directory
- `DB_PATH`, `DB_SUFFIX`, `DB_TABLE_SUFFIX`, `DB_CHUNK_SIZE` — database settings
- `OBJECT_DATA_DIR` — persistent object data directory
- `STORAGE_DATA_DIR` — storage container data directory

### Login and display
- `DISPLAY_NEWS` — show news on login
- `LOGIN_MSG`, `LOGIN_NEWS`, `FLOGIN_NEWS` — login splash/motd/first-login file paths
- `FIRST_USER` — first user setup file
- `MORELINES` — lines per page for paging
- `PAGE_DISPLAY` — paging mode (`"line"`)

### Colour
- `LOOK_HIGHLIGHT` — highlight objects in look (`"on"`/`"off"`)
- `LOOK_HIGHLIGHT_COLOUR` — colour code for look highlights
- `COLOUR_TOO_DARK` — whether to enforce minimum luminance (`"on"`/`"off"`)
- `COLOUR_MININUM_LUMINANCE` — minimum colour luminance (0–100)

### Gameplay
- `USE_MASS` — use mass system (true) vs. weight
- `DEFAULT_RACE` — default player race
- `ATTRIBUTES` — array of attribute names
- `SKILLS` — nested mapping of skill categories and skill names
- `CURRENCY` — array of `[name, value]` pairs for the currency system
- `COIN_VALUE_PER_LEVEL`, `COIN_VARIANCE` — mob coin drop tuning

### Combat
- `DAMAGE_LEVEL_MODIFIER` — damage scaling factor
- `DEFAULT_HIT_CHANCE` — base hit percentage

### Leveling
- `PLAYER_AUTOLEVEL` — auto-level on XP gain
- `BASE_TNL` — base XP to next level
- `TNL_RATE` — geometric scaling rate per level
- `OVERLEVEL_THRESHOLD`, `OVERLEVEL_XP_PUNISH` — XP penalty for overleveled kills
- `UNDERLEVEL_THRESHOLD`, `UNDERLEVEL_XP_BONUS` — XP bonus for underleveled kills

### Vitals and heartbeat
- `HEART_PULSE` — heartbeat interval in milliseconds
- `HEARTBEATS_TO_REGEN` — heartbeats between regen ticks
- `DEFAULT_HEART_RATE` — default heart rate

### Timeouts
- `TIMEOUT_PLAYERS` / `TIMEOUT_PLAYERS_TIME` — player idle timeout (`"on"`/`"off"`, seconds)
- `TIMEOUT_DEVS` / `TIMEOUT_DEVS_TIME` — developer idle timeout
- `TIMEOUT_ADMINS` / `TIMEOUT_ADMINS_TIME` — admin idle timeout

### Travel
- `TRAVEL_DESTINATIONS` — mapping of `name: room_path` for fast travel
- `WAYPOINTS_MAX` — max saved waypoints per player

### Documentation
- `DOC_DIR` — documentation root
- `AUTODOC_ROOT` — autodoc output directory
- `AUTODOC_SOURCE_DIRS` — directories to scan for autodoc
- `DOCS` — mapping of doc categories to directory lists

### Security
- `ALLOW_SHUTDOWN` — objects allowed to initiate shutdown
- `ALLOW_RECURSE_RMDIR` — objects allowed recursive directory removal

### External services
These are typically set in `config.lpml` (not defaults):
- `GITHUB_REPORTER` — GitHub integration settings
- `DISCORD_BOTS` — Discord bot configurations
- `GRAPEVINE` — Grapevine network settings

### GUI
- `GUI` — enable GUI/GMCP features

### Alarms
- `ALARMS` — enable alarm system (`"on"`/`"off"`)
- `ALARMS_PATH` — directory for alarm definitions

### Formatting
- `DECIMAL` — decimal separator character
- `THOUSANDS` — thousands separator character

## How to Add a New Configuration Key

1. **Add the default value** in `/adm/etc/default.lpml`:
   ```
   MY_NEW_KEY: "default_value",
   ```

2. **Use it in code** via `mudConfig()`:
   ```lpc
   string val = mudConfig("MY_NEW_KEY");
   ```

3. **To override locally**, add to `/adm/etc/config.lpml` (create if it doesn't exist):
   ```
   {
     MY_NEW_KEY: "local_override",
   }
   ```

4. **Rehash** to pick up changes without restarting: call `CONFIG_D->rehash_config()` or use the `master` admin command.

## Common Patterns

### Reading config in object code
```lpc
void setup() {
  int pulse = mudConfig("HEART_PULSE");
  set_heart_beat(pulse / 1000);  // convert ms to seconds
}
```

### Checking a toggle
```lpc
if(mudConfig("ALARMS") == "on") {
  // alarm system enabled
}
```

### Using structured config
```lpc
mapping skills = mudConfig("SKILLS");
string *combat_melee = skills["combat"]["melee"];
```

### Config in simul_efuns
Several simul_efuns in `/adm/simul_efun/system.c` wrap specific config keys for convenience:
- `log_dir()` — returns `mudConfig("LOG_DIR")`
- `tmp_dir()` — returns `mudConfig("TMP_DIR")`
- `lib_name()` — returns `mudConfig("LIB_NAME")`
- `lib_version()` — returns `mudConfig("LIB_VERSION")`
- `open_status()` — returns `mudConfig("OPEN_STATUS")`
- `admin_email()` — returns `mudConfig("ADMIN_EMAIL")`

## Important Notes

- Keys are **case-sensitive** and **uppercase by convention**.
- `get_mud_config()` raises an error for unknown keys — do not use it speculatively. If a key might not exist, check `get_all_config()` first.
- `config.lpml` is `.gitignore`d. Sensitive values (API tokens, webhooks) belong there, not in `default.lpml`.
- The merge is a **shallow mapping addition** (`+=`). Nested mappings in `config.lpml` replace the entire nested value from defaults, they do not deep-merge.
- Driver-level configuration (ports, stack sizes, protocol toggles) is separate — those use `get_config()`/`set_config()` with constants from `/include/runtime_config.h` and are not part of this system.
