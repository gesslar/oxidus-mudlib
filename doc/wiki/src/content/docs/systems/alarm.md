---
title: Alarms
description: Scheduling recurring and one-shot events with ALARM_D.
---

The alarm system schedules events to fire at wall-clock times -- daily maintenance, hourly housekeeping, yearly holidays, or a one-off reminder. Alarms are defined in plain text files or registered at runtime, and the alarm daemon (`ALARM_D`) re-evaluates the full list once per minute, firing any whose pattern matches.

## How It Works

On startup, `ALARM_D` scans the configured alarms directory -- the `ALARMS_PATH` config key, which ships pointing at `adm/custom/alarms/` -- for `*.txt` files and parses each line into an alarm. It then schedules itself to poll at the top of every minute. Each poll, it checks every alarm against the current time and, for any that match, loads the alarm's target file and calls the named function with the alarm's arguments. One-shot alarms are dropped once they fire or once their time has passed.

The whole system can be toggled with the `ALARMS` config key (`on`/`off`).

## Alarm Types

Each alarm has a one-character type and a pattern whose format depends on that type:

| Type | Name | Pattern | Fires |
|---|---|---|---|
| `B` | Boot | `SS` (seconds) | Once per boot, `SS` seconds after startup |
| `O` | Once | `YY-MM-DD@HH:MM` | Once at the given date and time |
| `H` | Hourly | `MM` | Every hour at minute `MM` |
| `D` | Daily | `HH:MM` | Every day at `HH:MM` |
| `W` | Weekly | `D@HH:MM` | Weekly on day-of-week `D` (Sunday = 0) at `HH:MM` |
| `M` | Monthly | `D@HH:MM` | Monthly on day-of-month `D` at `HH:MM` |
| `Y` | Yearly | `MM-DD@HH:MM` | Yearly on month `MM`, day `DD`, at `HH:MM` |

## Alarm File Format

Each line in an alarm file is a single alarm:

```text
TYPE PATTERN MASTER FILE FUNCTION [ARGUMENTS]
```

| Field | Meaning |
|---|---|
| `TYPE` | One of the type characters above |
| `PATTERN` | The time pattern for that type |
| `MASTER` | `true` if the alarm targets the master file, `false` otherwise |
| `FILE` | Absolute path to the target object |
| `FUNCTION` | Function on the target to call |
| `ARGUMENTS` | Optional trailing arguments, passed to the function when it fires |

Arguments are converted from their literal text to typed values, and quoted strings are preserved as single arguments. For example:

```text
Y 12-25@10:00 false /adm/daemons/holiday broadcast_message "Merry Christmas!"
H 30 false /adm/daemons/config rehash_config
```

A worked example ships at `adm/custom/alarms/alarm.txt.example`.

## Adding Alarms at Runtime

Alarms can be registered programmatically. `add_once()` is the convenience path for a single future event; `add_alarm()` takes any type:

```lpc
// Fire once on Christmas morning:
ALARM_D->add_once("false", "12-25@10:00", "/adm/daemons/holiday",
  "broadcast_message", "Merry Christmas!");
```

`add_once()` and `add_alarm()` return `1` on success or `0` if the alarm fails validation (missing file, missing function, or a time already in the past). On success the alarm is stored with a generated id you can later pass to `remove_alarm()`.

Runtime-added alarms are persisted across reboots through the daemon's save/restore cycle. However, a rehash rebuilds the list **from the config files only** -- so rehashing discards any alarms that were added at runtime rather than written to a file.

## The `alarm` Command

`alarm` is the developer command for inspecting and managing the live list:

| Usage | Effect |
|---|---|
| `alarm` | List all alarms |
| `alarm time` | List all alarms, showing raw timestamps |
| `alarm list [type]` | List alarms filtered by type |
| `alarm info <#>` | Show metadata for one alarm |
| `alarm next` | Seconds until the next poll |
| `alarm add [type]` | Add an alarm interactively |
| `alarm add root [type]` | Add an alarm with root permissions (admin) |
| `alarm remove` | Remove an alarm by number |
| `alarm rehash` | Rebuild the list from the config files (admin) |

Types are given as any unambiguous prefix of `once`, `hourly`, `daily`,
`weekly`, `monthly`, or `yearly`. Boot alarms cannot be added this way -- they
must come from a config file.

## Rehashing

After editing alarm files, rebuild the live list with `alarm rehash` (which
calls `ALARM_D->rehash_alarms()`). This clears the in-memory list and re-parses
every file under `ALARMS_PATH`.

## Key Files

| File | Role |
|---|---|
| `adm/daemons/alarm.lpc` | `ALARM_D` -- parsing, scheduling, dispatch, persistence |
| `cmds/dev/alarm.lpc` | `alarm` developer command -- list, inspect, add, remove, rehash |
| `adm/custom/alarms/` | Shipped `ALARMS_PATH` -- alarm definition files |
| `adm/custom/alarms/alarm.txt.example` | Annotated example alarm file |
