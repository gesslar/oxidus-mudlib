---
name: dokuwiki-syntax
description: DokuWiki markup syntax reference. Consult when writing or editing wiki pages in doc/wiki/data/pages/.
---

# DokuWiki Syntax Reference

Wiki pages live in `doc/wiki/data/pages/` as `.txt` files using DokuWiki markup.

## Text Formatting

| Syntax | Result |
|---|---|
| `**bold**` | **bold** |
| `//italic//` | italic |
| `__underlined__` | underlined |
| `''monospaced''` | monospaced |
| `<del>deleted</del>` | strikethrough |
| `<sub>subscript</sub>` | subscript |
| `<sup>superscript</sup>` | superscript |

Combine freely: `**__//''all combined''//__**`

Forced line break: two backslashes `\\` followed by a space or end of line.

## Headings

```
====== Level 1 (page title) ======
===== Level 2 =====
==== Level 3 ====
=== Level 4 ===
== Level 5 ==
```

Three or more headings auto-generates a table of contents. Suppress with `~~NOTOC~~`.

Horizontal rule: four or more dashes `----`.

## Links

| Syntax | Description |
|---|---|
| `[[pagename]]` | Internal link |
| `[[pagename\|link text]]` | Internal link with custom text |
| `[[some:namespace]]` | Namespaced page (colon-separated) |
| `[[syntax#section\|text]]` | Link to section anchor |
| `http://example.com` | Auto-linked external URL |
| `[[http://example.com\|text]]` | External link with text |
| `[[wp>Wiki]]` | Interwiki link (Wikipedia) |
| `[[doku>pagename]]` | Interwiki link (DokuWiki docs) |
| `<user@example.com>` | Email link |

## Images and Media

```
{{wiki:image.png}}                    real size
{{wiki:image.png?50}}                 resize to width
{{wiki:image.png?200x50}}             resize to width x height
{{ wiki:image.png}}                   right-aligned (space on left)
{{wiki:image.png }}                   left-aligned (space on right)
{{ wiki:image.png }}                  centered (spaces both sides)
{{ wiki:image.png |Caption text}}     with caption/tooltip
```

Image as link: `[[http://example.com|{{wiki:image.png}}]]`

Supported formats:
- Image: gif, jpg, png
- Video: webm, ogv, mp4
- Audio: ogg, mp3, wav

Link-only (no inline display): `{{wiki:image.png?linkonly}}`

## Lists

Indent by two spaces, then `*` (unordered) or `-` (ordered):

```
  * Item one
  * Item two
    * Nested item
  * Item three

  - First
  - Second
    - Nested
  - Third
```

## Tables

Use `|` for data cells, `^` for header cells:

```
^ Heading 1   ^ Heading 2   ^ Heading 3   ^
| Row 1 Col 1 | Row 1 Col 2 | Row 1 Col 3 |
| Row 2 Col 1 | colspan (empty next cell) ||
```

Alignment via whitespace:
- `|  right-aligned|` — two+ spaces on left
- `|left-aligned  |` — two+ spaces on right
- `|  centered  |` — two+ spaces both sides

Rowspan: use `:::` in cells below the spanning cell.

Vertical headers:

```
|              ^ Heading 1   ^ Heading 2   ^
^ Row header   | data        | data        |
```

## Code Blocks

Indent by two spaces for preformatted text, or use tags:

```
<code>
preformatted block
</code>

<file>
file-style block
</file>
```

With syntax highlighting: `<code java>...</code>`

Downloadable: `<file php myfile.php>...</file>`

### Custom Language Highlighters

Custom GeSHi language files live in `doc/wiki/vendor/geshi/geshi/src/geshi/`. This wiki has:

| Tag | Language | Notes |
|---|---|---|
| `<code lpc>` | LPC | Keywords, types, modifiers, efuns, `->` / `::` splitters, preprocessor |
| `<code lpml>` | LPML | JSON5 base + `Infinity`, `NaN`, `MAX_INT`, `MAX_FLOAT`, file includes |

Use `lpc` and `lpml` instead of `c` or `javascript` for proper highlighting.

## No Formatting

- `<nowiki>...</nowiki>` — disable all formatting
- `%%...%%` — inline no-format

## Footnotes

`((This is a footnote))` — double parentheses create footnotes.

## Quoting

```
> First level quote
>> Second level quote
>>> Third level quote
```

## RSS/ATOM Feeds

```
{{rss>http://example.com/feed.rss 5 author date 1h}}
```

Parameters: number (max items), `reverse`, `author`, `date`, `description`, `nosort`, refresh period (`12h`, `1d`, etc.).

## Control Macros

| Macro | Description |
|---|---|
| `~~NOTOC~~` | Suppress table of contents |
| `~~NOCACHE~~` | Disable page caching |

## File Organization

- Pages: `doc/wiki/data/pages/<namespace>/<pagename>.txt`
- Media: `doc/wiki/data/media/<namespace>/`
- Namespaces map to subdirectories; colons in wiki links map to `/`
