---
name: colour-coding
description: Thresh MUD 256-color XTerm color code reference. Consult when writing or modifying output strings that use color codes, or when choosing colors for new content.
---

# Thresh MUD Extended Color Code Reference for LLMs

This document provides a comprehensive mapping of the 256-color XTerm color codes used in Thresh MUD. This reference will help LLMs understand the specific colors being referenced in code and accurately describe them in documentation or new code.

## Basic Syntax

Color codes in Thresh MUD use the format:

- `` `<code>` `` - Begin a color or formatting effect
- `` `<res>` `` - Reset all formatting back to default
- `` `[code]` `` - Background color variant (uses same codes as foreground)

## 16 Basic ANSI Colors (`z00`-`z15`)

These are the standard terminal colors:

| Code | Color Name     | ANSI Escape    |
|------|---------------|----------------|
| z00  | Black         | \e[38;5;0m    |
| z01  | Red           | \e[38;5;1m    |
| z02  | Green         | \e[38;5;2m    |
| z03  | Dog Poop      | \e[38;5;3m    |
| z04  | Blue          | \e[38;5;4m    |
| z05  | Twilight Magenta | \e[38;5;5m    |
| z06  | Cyan          | \e[38;5;6m    |
| z07  | Gray          | \e[38;5;7m    |
| z08  | Dark Gray     | \e[38;5;8m    |
| z09  | Bright Red    | \e[38;5;9m    |
| z10  | Bright Green  | \e[38;5;10m   |
| z11  | Yellow        | \e[38;5;11m   |
| z12  | Bright Blue   | \e[38;5;12m   |
| z13  | Bright Magenta | \e[38;5;13m   |
| z14  | Bright Cyan   | \e[38;5;14m   |
| z15  | Brilliant White | \e[38;5;15m   |

## Grayscale Colors (`g00`-`g23`)

These form a gradient from nearly black to nearly white:

| Code | Color Name       | ANSI Escape    |
|------|-----------------|----------------|
| g00  | Impossible Black | \e[38;5;232m  |
| g01  | Abyssal Black   | \e[38;5;233m  |
| g02  | Pitch Black     | \e[38;5;234m  |
| g03  | Midnight Black  | \e[38;5;235m  |
| g04  | Oily Black      | \e[38;5;236m  |
| g05  | Deep Charcoal Black | \e[38;5;237m  |
| g06  | Charcoal Black  | \e[38;5;238m  |
| g07  | Deep Ash Gray   | \e[38;5;239m  |
| g08  | Mournful Gray   | \e[38;5;240m  |
| g09  | Ash Gray        | \e[38;5;241m  |
| g10  | Ghoulish White  | \e[38;5;242m  |
| g11  | Light Ash Gray  | \e[38;5;243m  |
| g12  | Grimey White    | \e[38;5;244m  |
| g13  | Dark Gray       | \e[38;5;245m  |
| g14  | Twilight Gray   | \e[38;5;246m  |
| g15  | Dusty Gray      | \e[38;5;247m  |
| g16  | Smudged White   | \e[38;5;248m  |
| g17  | Light Ash       | \e[38;5;249m  |
| g18  | Dull White      | \e[38;5;250m  |
| g19  | Smokey White    | \e[38;5;251m  |
| g20  | White           | \e[38;5;252m  |
| g21  | Cloud White     | \e[38;5;253m  |
| g22  | Tarnished White | \e[38;5;254m  |
| g23  | Nearly Bright White | \e[38;5;255m  |

## Color System Organization

The extended color set uses a three-character code where each character ranges from 'a' to 'f'. These form a 6×6×6 cube of colors:

- The first character represents the red component (a=lowest, f=highest)
- The second character represents the green component (a=lowest, f=highest)
- The third character represents the blue component (a=lowest, f=highest)

For example, `aaa` is black (minimal intensity in all channels), `fff` is white (maximum intensity in all channels),
and `faa` is bright red (maximum red, minimal green and blue).

## Color Restrictions

The following colors are considered "too dark" and are restricted from player use:

- `z00` (Black)
- `aaa` (Raven Black)
- `aab` (Dark Ultramarine)
- `aac` (Deep Ultramarine)
- `g00` to `g05` (The darkest grayscale colors)

These are automatically converted to more visible alternatives when players try to use them.

## Complete Color Reference Table

This table contains all 216 extended colors (`aaa`-`fff`) sorted alphabetically by code:

| Code | Color Name           | ANSI Escape    |
|------|---------------------|----------------|
| aaa  | Raven Black         | \e[38;5;16m   |
| aab  | Dark Ultramarine    | \e[38;5;17m   |
| aac  | Deep Ultramarine    | \e[38;5;18m   |
| aad  | Murky Ultramarine   | \e[38;5;19m   |
| aae  | Soft Ultramarine    | \e[38;5;20m   |
| aaf  | Ultramarine         | \e[38;5;21m   |
| aba  | Dark Myrtle Green   | \e[38;5;22m   |
| abb  | Dark Teal           | \e[38;5;23m   |
| abc  | Dark Prussian Blue  | \e[38;5;24m   |
| abd  | Deep Prussian Blue  | \e[38;5;25m   |
| abe  | Murky Prussian Blue | \e[38;5;26m   |
| abf  | Soft Prussian Blue  | \e[38;5;27m   |
| aca  | Deep Myrtle Green   | \e[38;5;28m   |
| acb  | Murky Myrtle Green  | \e[38;5;29m   |
| acc  | Deep Teal           | \e[38;5;30m   |
| acd  | Dark Steel Blue     | \e[38;5;31m   |
| ace  | Deep Steel Blue     | \e[38;5;32m   |
| acf  | Murky Steel Blue    | \e[38;5;33m   |
| ada  | Soft Myrtle Green   | \e[38;5;34m   |
| adb  | Dull Myrtle Green   | \e[38;5;35m   |
| adc  | Pale Myrtle Green   | \e[38;5;36m   |
| add  | Murky Teal          | \e[38;5;37m   |
| ade  | Soft Steel Blue     | \e[38;5;38m   |
| adf  | Dull Steel Blue     | \e[38;5;39m   |
| aea  | Dark Viridian       | \e[38;5;40m   |
| aeb  | Deep Viridian       | \e[38;5;41m   |
| aec  | Murky Viridian      | \e[38;5;42m   |
| aed  | Soft Viridian       | \e[38;5;43m   |
| aee  | Soft Teal           | \e[38;5;44m   |
| aef  | Pale Steel Blue     | \e[38;5;45m   |
| afa  | Light Viridian      | \e[38;5;46m   |
| afb  | Vivid Viridian      | \e[38;5;47m   |
| afc  | Bright Viridian     | \e[38;5;48m   |
| afd  | Dull Viridian       | \e[38;5;49m   |
| afe  | Pale Viridian       | \e[38;5;50m   |
| aff  | Bright Turquoise    | \e[38;5;51m   |
| baa  | Dark Maroon         | \e[38;5;52m   |
| bab  | Dark Violet         | \e[38;5;53m   |
| bac  | Dark Purple         | \e[38;5;54m   |
| bad  | Dark Indigo         | \e[38;5;55m   |
| bae  | Deep Indigo         | \e[38;5;56m   |
| baf  | Prussian Blue       | \e[38;5;57m   |
| bba  | Dark Brass Yellow   | \e[38;5;58m   |
| bbb  | Charcoal Gray       | \e[38;5;59m   |
| bbc  | Dark Cobalt Blue    | \e[38;5;60m   |
| bbd  | Deep Cobalt Blue    | \e[38;5;61m   |
| bbe  | Murky Cobalt Blue   | \e[38;5;62m   |
| bbf  | Soft Cobalt Blue    | \e[38;5;63m   |
| bca  | Dark Sea Green      | \e[38;5;64m   |
| bcb  | Deep Sea Green      | \e[38;5;65m   |
| bcc  | Dark Turquoise      | \e[38;5;66m   |
| bcd  | Dark Sapphire Blue  | \e[38;5;67m   |
| bce  | Deep Sapphire Blue  | \e[38;5;68m   |
| bcf  | Murky Sapphire Blue | \e[38;5;69m   |
| bda  | Murky Sea Green     | \e[38;5;70m   |
| bdb  | Soft Sea Green      | \e[38;5;71m   |
| bdc  | Dull Sea Green      | \e[38;5;72m   |
| bdd  | Deep Turquoise      | \e[38;5;73m   |
| bde  | Soft Sapphire Blue  | \e[38;5;74m   |
| bdf  | Dull Sapphire Blue  | \e[38;5;75m   |
| bea  | Dark Spring Green   | \e[38;5;76m   |
| beb  | Deep Spring Green   | \e[38;5;77m   |
| bec  | Murky Spring Green  | \e[38;5;78m   |
| bed  | Soft Spring Green   | \e[38;5;79m   |
| bee  | Murky Turquoise     | \e[38;5;80m   |
| bef  | Pale Sapphire Blue  | \e[38;5;81m   |
| bfa  | Light Spring Green  | \e[38;5;82m   |
| bfb  | Vivid Spring Green  | \e[38;5;83m   |
| bfc  | Bright Spring Green | \e[38;5;84m   |
| bfd  | Dull Spring Green   | \e[38;5;85m   |
| bfe  | Pale Spring Green   | \e[38;5;86m   |
| bff  | Soft Turquoise      | \e[38;5;87m   |
| caa  | Deep Maroon         | \e[38;5;88m   |
| cab  | Murky Maroon        | \e[38;5;89m   |
| cac  | Deep Violet         | \e[38;5;90m   |
| cad  | Dark Indigo         | \e[38;5;91m   |
| cae  | Deep Indigo         | \e[38;5;92m   |
| caf  | Murky Indigo        | \e[38;5;93m   |
| cba  | Dark Orange         | \e[38;5;94m   |
| cbb  | Deep Orange         | \e[38;5;95m   |
| cbc  | Dark Rosey Brown    | \e[38;5;96m   |
| cbd  | Murky Purple        | \e[38;5;97m   |
| cbe  | Soft Purple         | \e[38;5;98m   |
| cbf  | Purple              | \e[38;5;99m   |
| cca  | Deep Brass Yellow   | \e[38;5;100m  |
| ccb  | Murky Brass Yellow  | \e[38;5;101m  |
| ccc  | Deep Gray           | \e[38;5;102m  |
| ccd  | Pale Indigo         | \e[38;5;103m  |
| cce  | Dull Indigo         | \e[38;5;104m  |
| ccf  | Indigo              | \e[38;5;105m  |
| cda  | Deep Olive Green    | \e[38;5;106m  |
| cdb  | Murky Olive Green   | \e[38;5;107m  |
| cdc  | Soft Olive Green    | \e[38;5;108m  |
| cdd  | Dull Teal           | \e[38;5;109m  |
| cde  | Light Prussian Blue | \e[38;5;110m  |
| cdf  | Vivid Prussian Blue | \e[38;5;111m  |
| cea  | Olive Green         | \e[38;5;112m  |
| ceb  | Dull Olive Green    | \e[38;5;113m  |
| cec  | Pale Olive Green    | \e[38;5;114m  |
| ced  | Faded Olive Green   | \e[38;5;115m  |
| cee  | Pale Teal           | \e[38;5;116m  |
| cef  | Bright Prussian Blue| \e[38;5;117m  |
| cfa  | Bright Jade Green   | \e[38;5;118m  |
| cfb  | Vivid Jade Green    | \e[38;5;119m  |
| cfc  | Light Jade Green    | \e[38;5;120m  |
| cfd  | Pale Jade Green     | \e[38;5;121m  |
| cfe  | Faded Jade Green    | \e[38;5;122m  |
| cff  | Faded Teal          | \e[38;5;123m  |
| daa  | Dark Crimson        | \e[38;5;124m  |
| dab  | Deep Crimson        | \e[38;5;125m  |
| dac  | Murky Crimson       | \e[38;5;126m  |
| dad  | Murky Violet        | \e[38;5;127m  |
| dae  | Dark Magenta        | \e[38;5;128m  |
| daf  | Deep Magenta        | \e[38;5;129m  |
| dba  | Murky Orange        | \e[38;5;130m  |
| dbb  | Dark Coral Red      | \e[38;5;131m  |
| dbc  | Dark Pink           | \e[38;5;132m  |
| dbd  | Murky Magenta       | \e[38;5;133m  |
| dbe  | Soft Magenta        | \e[38;5;134m  |
| dbf  | Magenta             | \e[38;5;135m  |
| dca  | Soft Brass Yellow   | \e[38;5;136m  |
| dcb  | Brass Yellow        | \e[38;5;137m  |
| dcc  | Deep Rosy Brown     | \e[38;5;138m  |
| dcd  | Murky Rosey Brown   | \e[38;5;139m  |
| dce  | Light Purple        | \e[38;5;140m  |
| dcf  | Vivid Purple        | \e[38;5;141m  |
| dda  | Dark Golden Yellow  | \e[38;5;142m  |
| ddb  | Deep Golden Yellow  | \e[38;5;143m  |
| ddc  | Dull Khaki          | \e[38;5;144m  |
| ddd  | Silver Gray         | \e[38;5;145m  |
| dde  | Pale Purple         | \e[38;5;146m  |
| ddf  | Faded Purple        | \e[38;5;147m  |
| dea  | Dark Lime Green     | \e[38;5;148m  |
| deb  | Deep Lime Green     | \e[38;5;149m  |
| dec  | Murky Lime Green    | \e[38;5;150m  |
| ded  | Soft Lime Green     | \e[38;5;151m  |
| dee  | Faded Teal          | \e[38;5;152m  |
| def  | Dull Prussian Blue  | \e[38;5;153m  |
| dfa  | Light Lime Green    | \e[38;5;154m  |
| dfb  | Vivid Lime Green    | \e[38;5;155m  |
| dfc  | Bright Lime Green   | \e[38;5;156m  |
| dfd  | Pale Lime Green     | \e[38;5;157m  |
| dfe  | Faded Lime Green    | \e[38;5;158m  |
| dff  | Pale Prussian Blue  | \e[38;5;159m  |
| eaa  | Dark Cherry Red     | \e[38;5;160m  |
| eab  | Deep Cherry Red     | \e[38;5;161m  |
| eac  | Murky Cherry Red    | \e[38;5;162m  |
| ead  | Soft Cherry Red     | \e[38;5;163m  |
| eae  | Soft Violet         | \e[38;5;164m  |
| eaf  | Light Magenta       | \e[38;5;165m  |
| eba  | Soft Orange         | \e[38;5;166m  |
| ebb  | Maroon              | \e[38;5;167m  |
| ebc  | Deep Pink           | \e[38;5;168m  |
| ebd  | Murky Pink          | \e[38;5;169m  |
| ebe  | Violet              | \e[38;5;170m  |
| ebf  | Vivid Magenta       | \e[38;5;171m  |
| eca  | Dusty Orange        | \e[38;5;172m  |
| ecb  | Light Orange        | \e[38;5;173m  |
| ecc  | Deep Coral Red      | \e[38;5;174m  |
| ecd  | Soft Pink           | \e[38;5;175m  |
| ece  | Lilac               | \e[38;5;176m  |
| ecf  | Dull Violet         | \e[38;5;177m  |
| eda  | Murky Golden Yellow | \e[38;5;178m  |
| edb  | Soft Golden Yellow  | \e[38;5;179m  |
| edc  | Murky Coral Red     | \e[38;5;180m  |
| edd  | Soft Coral Red      | \e[38;5;181m  |
| ede  | Faded Magenta       | \e[38;5;182m  |
| edf  | Pale Violet         | \e[38;5;183m  |
| eea  | Dark Khaki          | \e[38;5;184m  |
| eeb  | Deep Khaki          | \e[38;5;185m  |
| eec  | Dull Khaki          | \e[38;5;186m  |
| eed  | Pale Khaki          | \e[38;5;187m  |
| eee  | Platinum White      | \e[38;5;188m  |
| eef  | Faded Violet        | \e[38;5;189m  |
| efa  | Light Chartreuse Yellow | \e[38;5;190m  |
| efb  | Vivid Chartreuse Yellow | \e[38;5;191m  |
| efc  | Bright Chartreuse Yellow | \e[38;5;192m  |
| efd  | Pale Chartreuse Yellow | \e[38;5;193m  |
| efe  | Faded Chartreuse Yellow | \e[38;5;194m  |
| eff  | Faded Prussian Blue | \e[38;5;195m  |
| faa  | Deep Vermillion     | \e[38;5;196m  |
| fab  | Murky Vermillion    | \e[38;5;197m  |
| fac  | Light Pink          | \e[38;5;198m  |
| fad  | Vivid Pink          | \e[38;5;199m  |
| fae  | Bright Pink         | \e[38;5;200m  |
| faf  | Violet              | \e[38;5;201m  |
| fba  | Vivid Orange        | \e[38;5;202m  |
| fbb  | Light Maroon        | \e[38;5;203m  |
| fbc  | Vivid Maroon        | \e[38;5;204m  |
| fbd  | Pink                | \e[38;5;205m  |
| fbe  | Light Lilac         | \e[38;5;206m  |
| fbf  | Vivid Lilac         | \e[38;5;207m  |
| fca  | Orange              | \e[38;5;208m  |
| fcb  | Bright Orange       | \e[38;5;209m  |
| fcc  | Coral Red           | \e[38;5;210m  |
| fcd  | Soft Maroon         | \e[38;5;211m  |
| fce  | Soft Lilac          | \e[38;5;212m  |
| fcf  | Dull Lilac          | \e[38;5;213m  |
| fda  | Pale Orange         | \e[38;5;214m  |
| fdb  | Faded Orange        | \e[38;5;215m  |
| fdc  | Light Coral Red     | \e[38;5;216m  |
| fdd  | Vivid Coral Red     | \e[38;5;217m  |
| fde  | Faded Lilac         | \e[38;5;218m  |
| fdf  | Pale Lilac          | \e[38;5;219m  |
| fea  | Light Golden Yellow | \e[38;5;220m  |
| feb  | Vivid Golden Yellow | \e[38;5;221m  |
| fec  | Bright Golden Yellow| \e[38;5;222m  |
| fed  | Pale Coral Red      | \e[38;5;223m  |
| fee  | Faded Coral Red     | \e[38;5;224m  |
| fef  | Faded Lilac         | \e[38;5;225m  |
| ffa  | Light Yellow        | \e[38;5;226m  |
| ffb  | Vivid Yellow        | \e[38;5;227m  |
| ffc  | Bright Yellow       | \e[38;5;228m  |
| ffd  | Pale Yellow         | \e[38;5;229m  |
| ffe  | Faded Yellow        | \e[38;5;230m  |
| fff  | Bright White        | \e[38;5;231m  |

### Text Formatting Codes

| Code | Function | ANSI Escape |
|------|----------|-------------|
| `<bl0>` | Bold off | \e[22m |
| `<bl1>` | Bold on | \e[1m |
| `<it0>` | Italic off | \e[23m |
| `<it1>` | Italic on | \e[3m |
| `<ul0>` | Underline off | \e[24m |
| `<ul1>` | Underline on | \e[4m |
| `<fl0>` | Flash/blink off | \e[25m |
| `<fl1>` | Flash/blink on | \e[5m |
| `<re0>` | Reverse video off | \e[27m |
| `<re1>` | Reverse video on | \e[7m |
| `<di0>` | Dim off | \e[22m |
| `<di1>` | Dim on | \e[2m |
| `<st0>` | Strikethrough off | \e[29m |
| `<st1>` | Strikethrough on | \e[9m |
| `<ol0>` | Overline off | \e[55m |
| `<ol1>` | Overline on | \e[53m |

### Special Purpose Codes

| Code | Function | Description |
|------|----------|-------------|
| `<res>` | Reset all formatting | Returns to terminal default |

Unlike Pinkfish codes, it is not required to reset the color after each use. The system will simply switch colours, because in Pinkfish codes, the BOLD will bleed into the next color, and the system will not reset the color after each use. Our implementation just switches colours on need and you can reset it when you're done with the color.

## Technical Encoding

In the internal system:

- The 216 extended colors are stored at ANSI codes 16-231
- The grayscale colors are stored at ANSI codes 232-255
- The 16 basic colors are stored at ANSI codes 0-15

These are then referenced in the format `\e[38;5;NNNm` for foreground colors and `\e[48;5;NNNm` for background colors,
where NNN is the numeric code.

Foreground colors are typically expressed in the format of `` `<code>` `` and background colors in the format of `` `[code]` ``.

This allows for easy parsing and rendering in the terminal.
