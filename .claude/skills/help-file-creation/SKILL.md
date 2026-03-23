---
name: help-file-creation
description: Create new help files for Threshold RPG using the autohelp .help format. Covers file structure, markup syntax, categories, cross-referencing, and testing.
---

# Help File Creation

You are creating help files for Threshold RPG's autohelp system. Help files use a custom `.help` format processed by the autohelp daemon (`adm/daemons/autohelp.c`).

## File Location and Naming

- Help files live in `doc/help/` with the `.help` extension.
- Filenames use underscores for multi-word topics: `skill_ranks.help`
- Underscores convert to spaces for the topic name automatically (so `skill_ranks.help` becomes topic "skill ranks").
- The topic name is auto-generated from the filename — there is no `topic` field.

## File Structure

Help files use `###` as section delimiters:

```
###cat
<category>
###alt
({alternate,names,for,searching})
###related
({related,topic,names})
###wiki
<optional wiki URL>
###text
<main help content with markup>
###wizard
<admin-only content, only shown to wizards>
```

**Not all sections are required.** Only include what's needed. At minimum, a help file needs `###cat` and `###text`.

### Section Reference

| Section | Required | Format | Purpose |
|---|---|---|---|
| `###cat` | Yes | Single word/phrase | Category for grouping (see categories below) |
| `###alt` | No | `({name1,name2,name3})` | Alternate names/typos players might search for |
| `###related` | No | `({topic1,topic2})` | Cross-references to other help topics |
| `###wiki` | No | URL | Link to wiki page |
| `###text` | Yes | Free text with markup | Main content shown to all players |
| `###wizard` | No | Free text with markup | Additional content only shown to admin |

### Array Format

Arrays use LPC-style syntax: `({item1,item2,item3})` — no spaces after commas.

## Text Markup

| Markup | Color/Style | Use For |
|---|---|---|
| `{{text}}` | Cyan/italic (z06/it1) | Commands, keywords, things the player types |
| `[[text]]` | Pale teal (cee) | Emphasis, section headers within text |
| `((text))` | Dark cherry red (eaa) | Important warnings or notes |
| `` `<code>` `` | Standard color codes | Any color from the 256-color system |
| `` `<res>` `` | Reset | Reset formatting back to default |

### Special Line Syntax

- Lines starting with `!` call external functions: `!file:function:args`
- Standard color codes like `` `<faa>` `` work throughout the text body.

## Common Categories

| Category | Use For |
|---|---|
| `command` | Player commands (task, look, say, etc.) |
| `general` | General game information |
| `newbie` | New player content |
| `event` | Game events |
| `accessibility` | Accessibility features |

## Cross-Referencing Rules

- **Bidirectional links are mandatory.** When adding topic B to file A's `###related`, you must also add topic A to file B's `###related`.
- Include common misspellings or shorthand in `###alt` (e.g., `({transmog})` for transmogrification).
- Check existing related topics to ensure consistency.

## Examples

### Command Help File

```
###cat
command
###alt
({tasks})
###related
({overachieving,lodges})
###wiki
https://wiki.thresholdrpg.com/w/Tasks
###text
[[At a taskmaster:]]
{{task list}} - See if this NPC has any tasks available to you.
{{task info}} <{{id}}> - Ask this NPC for more information about a task.
{{task accept}} <{{id}}> - Accept a named task from this NPC.
{{task accept all}} - Accept all available tasks

[[Anywhere:]]
{{task}} - Check what tasks you are presently on.
{{task drop}} <{{id}}> - Abandon a task.

((Note:)) Tasks are lost on death.
```

### General Help File

```
###cat
general
###related
({task,overachieving,lodges})
###wiki
https://wiki.thresholdrpg.com/w/Tasks
###text
Tasks are assigned by NPCs to players. They can range from the incredibly simple
to the more complex.

[[Overachieving]]

Some tasks allow progress beyond 100%, up to 200%, at a reduced rate.
```

### Help File with Wizard Section

```
###cat
command
###related
({combat,weapons})
###text
{{attack}} <{{target}}> - Attack a target in combat.

This is the basic combat command available to all players.
###wizard
Admin notes: Attack formula defined in adm/daemons/combat.c
Damage scaling uses query("level") * base_multiplier.
```

## Testing

After creating a help file:

1. In-game, run `help rebuild` to reload all help files.
2. Test with `help <topic>` to verify content displays correctly.
3. Test alternate names with `help <alt>` to verify they resolve.
4. Check related topics link back bidirectionally.

## Validation Checklist

Before finalizing a help file:

- Filename uses underscores, has `.help` extension
- `###cat` is present with a valid category
- `###text` is present with content
- Commands and keywords are wrapped in `{{}}` markup
- Section headers within text use `[[]]` markup
- Important warnings use `(())` markup
- `###related` topics exist as actual help files
- Related links are bidirectional — every file you reference also references you back
- `###alt` includes common abbreviations, misspellings, or shorthand
- Array syntax is correct: `({item1,item2})` with no spaces after commas
- No trailing whitespace or blank lines at end of file
