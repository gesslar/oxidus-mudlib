/**
 * @file /adm/daemons/colour.c
 *
 * Colour Parser daemon for handling and converting colour codes in text.
 * Provides functionality for colour code substitution, text wrapping, and
 * colour transformations.
 *
 * @created 2022-08-22 - Gesslar
 * @last_modified 2024-03-11 - Gesslar
 *
 * @history
 * 2022-08-22 - Gesslar - Created
 */

#include <colour.h>

inherit STD_DAEMON;
inherit DM_CSS;

// Forward declarations
int *hexToRgb(string hex);
int colourToGreyscale(int colour_code);
int colourp(string text);
private int tooDarkCheck();
private string cached(string tag);
private void normalizeHex(string ref hex);
public int *colourToRgb(int colour_code);
public int rgbToColour(int r, int g, int b);
public string rgbToSequence(int *rgb, int mode);
string ansi256To3hex(int ansi);
string bodyColourReplace(object body, string text, int message_type);
string hextToSequence(string hex);
string substituteColour(string text, string mode);
string substituteTooDark(string hex);
string wrap(string str, int wrap_at, int indent_at);
int getLuminance(int *rgb);
float getAccessibleLuminanceMultiplier(int *rgb);
public string rgbToHex(int *rgb);
void cache256();
void cacheAttributes();
void resync();

private nosave float minimum_luminance;
private nosave mapping attributes = ([ ]);
private nosave mapping cache = ([]);

private nosave mixed *ansi_rgb = ({
  ({ 0, 0, 0 }), ({ 128, 0, 0 }), ({ 0, 128, 0 }), ({ 128, 128, 0 }),
  ({ 0, 0, 128 }), ({ 128, 0, 128 }), ({ 0, 128, 128 }), ({ 192, 192, 192 }),
  ({ 128, 128, 128 }), ({ 255, 0, 0 }), ({ 0, 255, 0 }), ({ 255, 255, 0 }),
  ({ 0, 0, 255 }), ({ 255, 0, 255 }), ({ 0, 255, 255 }), ({ 255, 255, 255 })
});

/**
 * Base setup function for the daemon.
 *
 * @returns {void}
 */
void setup() {
  cacheAttributes();
  cache256();

  minimum_luminance = mudConfig("COLOUR_MININUM_LUMINANCE") || 50.0;
}

/**
 * Checks if colour too dark feature is enabled.
 *
 * @returns {int} 1 if enabled, 0 if disabled
 * @private
 */
private int tooDarkCheck() {
  return 1;
  // return mud_config("COLOUR_TOO_DARK") == "on";
}

/**
 * Gets a cached colour sequence for the given tag.
 *
 * @param {string} tag - The colour tag to look up
 * @returns {string} The cached colour sequence or newly generated one
 * @private
 */
private string cached(string tag) {
  if(!cache[tag])
    cache[tag] = hextToSequence(tag);

  return cache[tag];
}

/**
 * Initializes the cache with standard ANSI attributes.
 *
 * Caches common text attributes like bold, dim, italics, etc.
 *
 * @returns {void}
 */
void cacheAttributes() {
  cache += ([
    "{{res}}" : "\e[0m",  // reset
    "{{RES}}" : "\e[0m",  // reset
    "{{bl0}}" : "\e[22m", // bold off
    "{{bl1}}" : "\e[1m",  // bold on
    "{{di0}}" : "\e[22m", // dim off
    "{{di1}}" : "\e[2m",  // dim on
    "{{fl0}}" : "\e[25m", // flash off
    "{{fl1}}" : "\e[5m",  // flash on
    "{{it0}}" : "\e[23m", // italics off
    "{{it1}}" : "\e[3m",  // italics on
    "{{ol0}}" : "\e[55m", // overline off
    "{{ol1}}" : "\e[53m", // overline on
    "{{re0}}" : "\e[27m", // reverse video off
    "{{re1}}" : "\e[7m",  // reverse video on
    "{{st0}}" : "\e[29m", // strike off
    "{{st1}}" : "\e[9m",  // strike on
    "{{ul0}}" : "\e[24m", // underline off
    "{{ul1}}" : "\e[4m",  // underline on
  ]);
}

/**
 * Caches the 256 colour codes and their RGB equivalents.
 *
 * @returns {void}
 */
private void cache256() {
  int x;

  for(; x < 256; x++) {
    int *rgb = colourToRgb(x);
    string hex = rgbToHex(rgb);
    string lookup = sprintf("{{%s}}", hex);
    string code = rgbToSequence(rgb, 0);

    cache[lookup] = code;
  }
}

/**
 * Returns a copy of the current colour cache.
 *
 * @returns {mapping} Copy of the colour cache mapping
 */
public mapping query_colour_cache() { return copy(cache); }

/**
 * Substitutes colour codes in text according to the specified mode.
 *
 * @param {string} text - The text containing colour codes
 * @param {string} mode - The mode to use ("on" or "off")
 * @returns {string} Text with colour codes processed according to mode
 */
public string substituteColour(string text, string mode) {
  mixed *parts;
  string *tags;
  int *matches;
  int sz;

  if(!text)
    return text;

  parts = pcre_assoc(text, ({COLOUR_REGEX}), ({ 1 }), 0);
  tags = parts[0];
  matches = parts[1];
  sz = sizeof(matches);

  if(mode == "on") {
    while(sz--) {
      string good;

      if(!matches[sz])
        continue;

      if(good = cached(tags[sz])) {
        tags[sz] = good;
      } else {
        string sequence = hextToSequence(tags[sz]);

        cache[tags[sz]] = sequence;
        tags[sz] = sequence;
      }
    }
  } else {
    while(sz--)
      tags[sz] = matches[sz] ? "" : tags[sz];
  }

  return implode(tags, "");
}

/**
 * Wraps text to a specified width while preserving colour codes.
 *
 * Handles indentation of wrapped lines and preserves existing line breaks.
 * Leading spaces are maintained in the output.
 *
 * @param {string} str - The text content to wrap
 * @param {int} wrap_at - Maximum line length before wrapping
 * @param {int} indent_at - Number of spaces to indent wrapped lines
 * @returns {string} The text wrapped and formatted with preserved colours
 */
public string wrap(string str, int wrap_at, int indent_at) {
  string *sections;

  sections = map(explode(str, "\n"), function(string section, int wrap_at, int indent_at) {
    string *parts = explode(section, " ");
    mapping running = ([ "length" : 0 ]);

    // this routine strips out the first space, put it back into the
    // array if the original string had a leading space
    if(section[0..0] == " ")
      parts = ({ "", parts... });

    parts = map(parts, function(string part, mapping running, int max, int indent) {
      string plain;
      int len;

      // new lines
      if(part[0..0] == "\n") {
        running["length"] = 0;
        return part;
      }

      plain = substituteColour(part, "off");
      len = sizeof(plain);

      running["length"] += (len + 1);

      if(running["length"] >= max) {
        running["length"] = sizeof(plain) + indent;
        return "\n" + repeat_string(" ", indent) + part;
      }

      return part;

    }, running, wrap_at, indent_at);

    return implode(map(explode(implode(parts, " "), "\n"), (: rtrim :)), "\n");
  }, wrap_at, indent_at);

  return implode(sections, "\n");
}

/**
 * Checks if a string contains any colour codes.
 *
 * Tests for the presence of valid colour markup sequences in the text.
 *
 * @param {string} text - Text to check for colour codes
 * @returns {int} 1 if colour codes are present, 0 otherwise
 */
int colourp(string text) {
  return pcre_match(text, COLOUR_REGEX);
}

private nosave int *colours = ({
  016,  017,  018,  019,  020,  021,  022,  023,  024,  025,  026,  027,
  028,  029,  030,  031,  032,  033,  034,  035,  036,  037,  038,  039,
  040,  041,  042,  043,  044,  045,  046,  047,  048,  049,  050,  051,
  082,  083,  084,  085,  086,  087,  076,  077,  078,  079,  080,  081,
  070,  071,  072,  073,  074,  075,  064,  065,  066,  067,  068,  069,
  058,  059,  060,  061,  062,  063,  052,  053,  054,  055,  056,  057,
  093,  092,  091,  090,  089,  088,  099,  098,  097,  096,  095,  094,
  105,  104,  103,  102,  101,  100,  111,  110,  109,  108,  107,  106,
  117,  116,  115,  114,  113,  112,  123,  122,  121,  120,  119,  118,
  159,  158,  157,  156,  155,  154,  153,  152,  151,  150,  149,  148,
  147,  146,  145,  144,  143,  142,  141,  140,  139,  138,  137,  136,
  135,  134,  133,  132,  131,  130,  129,  128,  127,  126,  125,  124,
  160,  161,  162,  163,  164,  165,  166,  167,  168,  169,  170,  171,
  172,  173,  174,  175,  176,  177,  178,  179,  180,  181,  182,  183,
  184,  185,  186,  187,  188,  189,  190,  191,  192,  193,  194,  195,
  226,  227,  228,  229,  230,  231,  220,  221,  222,  223,  224,  225,
  214,  215,  216,  217,  218,  219,  208,  209,  210,  211,  212,  213,
  202,  203,  204,  205,  206,  207,  196,  197,  198,  199,  200,  201,
});

// render as 3 5 cell rows
private nosave int *colours_greyscale = ({
  232, 233, 235, 237, 238,
  248, 246, 244, 242, 240,
  255, 254, 252, 250, 249,
});

// render as two 8 cell rows
private nosave int *colours_low = ({
  000,  001,  002,  003,  004,  005,  006,  007,
  008,  009,  010,  011,  012,  013,  014,  015,
});

/**
 * Generates a formatted display of all available colours.
 *
 * Creates a visual representation of colours organized in sections:
 * - Rainbow colours (16-231)
 * - Greyscale colours (232-255)
 * - Base system colours (0-15)
 *
 * Each colour is shown with its hex code and visual representation.
 *
 * @returns {string} Formatted string containing all colour codes
 */
string get_colour_list() {
  string output = "";

  // Rainbow colors (16-231)
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 6; j++) {
      for(int k = 0; k < 12; k++) {
        int colour = i*(6*12) + j + k*6;
        string hex = ansi256To3hex(colours[colour]); // Convert ANSI to #RGB

        output += sprintf("  {{%s}}#%s{{res}}",
          hex, hex
        );
      }
      output += "\n";
    }
  }
  output += "\n";

  // Base 16 Colors (0-15)
  for(int i = 0; i < 2; i++) {
    for(int j = 0; j < 8; j++) {
      int colour = i*8 + j;
      string hex = ansi256To3hex(colours_low[colour]);

      output += sprintf("  {{%s}}#%s{{res}}",
        hex, hex
      );
    }
    output += "\n";
  }

  output += "\n";

  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 5; j++) {
      int colour = i*5 + j;
      string hex = ansi256To3hex(colours_greyscale[colour]);

      output += sprintf("  {{%s}}#%s{{res}}",
        hex, hex
      );
    }
    output += "\n";
  }

  return output;
}

mixed *_base_colours = ({
  ({ "{{000000}}", "black"          }),
  ({ "{{800000}}", "red"            }),
  ({ "{{008000}}", "green"          }),
  ({ "{{808000}}", "orange"         }),
  ({ "{{000080}}", "blue"           }),
  ({ "{{800080}}", "magenta"        }),
  ({ "{{008080}}", "cyan"           }),
  ({ "{{C0C0C0}}", "white"          }),
  ({ "{{808080}}", "bright black"   }),
  ({ "{{FF0000}}", "bright red"     }),
  ({ "{{00FF00}}", "bright green"   }),
  ({ "{{FFFF00}}", "bright orange"  }),
  ({ "{{0000FF}}", "bright blue"    }),
  ({ "{{FF00FF}}", "bright magenta" }),
  ({ "{{00FFFF}}", "bright cyan"    }),
  ({ "{{FFFFFF}}", "bright white"   }),
});

mixed *base_colours() {
  return copy(_base_colours);
}

/**
 * Converts an ANSI 256 color code to a 3-digit hex representation.
 *
 * @param {int} ansi - ANSI 256 color code (0-255)
 * @returns {string} 3-digit hex color code
 */
string ansi256To3hex(int ansi) {
  int *rgb = colourToRgb(ansi); // Get the RGB triplet
  return sprintf("%X%X%X", rgb[0] / 17, rgb[1] / 17, rgb[2] / 17); // Convert to 3-hex
}

int getLuminance(int *rgb) {
  assert_arg(uniform_array(rgb, T_INT), 1, "Invalid value.");
  assert_arg(every(rgb, (: !nullp($1) && clamped($1, 0, 255) :)), 1, "Invalid value.");

  return 0.2126 * rgb[0] + 0.7152 * rgb[1] + 0.0722 * rgb[2];
}

/**
 * Checks if a colour value is considered too dark for display.
 *
 * Tests colour values against the mud's dark colour configuration.
 *
 * @param {int*} rgb - The colour code to check
 * @returns {int} 1 if the colour is too dark, 0 otherwise
 */
float getAccessibleLuminanceMultiplier(int *rgb) {
  float lum = getLuminance(rgb);

  return lum >= minimum_luminance ? 1.0 : minimum_luminance / lum;
}


/**
 * Substitutes dark colours with more visible alternatives.
 *
 * Uses the mud's dark colour substitution mapping to replace colours
 * that are considered too dark with more visible alternatives.
 *
 * @param {string} hex - The colour code to potentially substitute
 * @returns {string} Substituted colour code or original if not too dark
 */
string substituteTooDark(string hex) {
  int *rgb;
  int max_distance;
  float scale;

  if(find_index(base_colours(), (: $1[0] == $(hex) :)) > -1)
    return hex;

  rgb = hexToRgb(hex);

  scale = getAccessibleLuminanceMultiplier(rgb);
  if(scale == 1.0)
    return hex;

  rgb[0] = clamp(to_int(rgb[0] * scale), 0, 255);
  rgb[1] = clamp(to_int(rgb[1] * scale), 0, 255);
  rgb[2] = clamp(to_int(rgb[2] * scale), 0, 255);

  return sprintf("{{%s}}", rgbToHex(rgb));
}

private void normalizeHex(string ref hex) {
  int r, g, b;
  string *match;

  hex = all_caps(hex);

  match = pcre_extract(hex, TRUE_COLOUR_REGEX);

  if(!sizeof(match))
    return;

  hex = match[0];

  // We good!
  if(strlen(hex) == 6)
    return;

  sscanf(hex[0..0], "%x", r);
  sscanf(hex[1..1], "%x", g);
  sscanf(hex[2..2], "%x", b);

  r *= 17;
  g *= 17;
  b *= 17;

  hex = sprintf("%02x%02x%02x", r, g, b);
}

/**
 * Converts a colour code to its greyscale equivalent.
 *
 * Converts any colour to its closest matching greyscale value
 * in the range 232-255.
 *
 * @param {int} colour_code - Original colour code (0-255)
 * @returns {int} Equivalent greyscale colour code (232-255)
 */
int colourToGreyscale(int colour_code) {
  float normalized;
  int greyscale_code;

  normalized = colour_code / 255.0;
  greyscale_code = to_int(normalized * 23) + 232;

  // Return the corresponding greyscale colour code
  return greyscale_code;
}

/**
 * Applies body-specific colour preferences to message text.
 *
 * Processes text according to the body's colour preferences and message type.
 * Handles special cases like combat messages differently.
 *
 * @param {object STD_BODY} body - The body object with colour preferences
 * @param {string} text - Text to process
 * @param {int} message_type - Type of message (NO_COLOUR, MSG_COMBAT_HIT, etc)
 * @returns {string} Text with applied colour preferences
 */
public string bodyColourReplace(object body, string text, int message_type) {
  int col;
  string pref, colour;

  if(!objectp(body) || !stringp(text) || !strlen(text) || !message_type)
    return text;

  if(message_type & NO_COLOUR)
    return text;
  else if(message_type & MSG_COMBAT_HIT)
    pref = body->query_pref("combat_hit_colour");
  else if(message_type & MSG_COMBAT_MISS)
    pref = body->query_pref("combat_miss_colour");
  else
    return text;

  if(!stringp(pref) || !strlen(pref))
    return text;

  colour = sprintf("{{%s}}", pref);

  if(pcre_match(colour, TRUE_COLOUR_REGEX))
    text = sprintf("%s%s{{res}}", colour, text);

  return text;
}

/**
 * Converts RGB values to the nearest 256-colour code.
 *
 * Maps RGB values to the closest matching colour in the 256-colour space:
 * - Values 0-15: System colours
 * - Values 16-231: 6x6x6 colour cube
 * - Values 232-255: Greyscale ramp
 *
 * @param {int} r - Red component (0-255)
 * @param {int} g - Green component (0-255)
 * @param {int} b - Blue component (0-255)
 * @returns {int} Best matching 256-colour code
 * @errors If any RGB value is outside the valid range
 */
public int rgbToColour(int r, int g, int b) {
  if(r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
    error("Invalid RGB values: " + r + ", " + g + ", " + b);

  if(r == g && g == b)
    // Grayscale
    return (r - 8) / 10 + 232; // 232-255 maps to 8-255
  else if(r >= 0 && r < 128 && g >= 0 && g < 128 && b >= 0 && b < 128)
    // Standard 16 colours
    return (r > 0 ? 1 : 0) + (g > 0 ? 2 : 0) + (b > 0 ? 4 : 0);
  else {
    // 6x6x6 colour cube
    int cube_r = r / 51;
    int cube_g = g / 51;
    int cube_b = b / 51;

    return 16 + (cube_r * 36) + (cube_g * 6) + cube_b;
  }
}

/**
 * Converts RGB values to a 256-color code.
 *
 * @param {int} colourCode - Array containing RGB values [r, g, b]
 * @returns {int*} The corresponding 256-color code
 */
int *colourToRgb(int colourCode) {
  int r, g, b;

  if(colourCode < 16) { // ANSI basic colors
    return ansi_rgb[colourCode]; // Assuming you have a predefined array like before
  } else if(clamped(colourCode, 16, 232)) { // 6x6x6 color cube (216 colors)
    colourCode -= 16;
    r = (colourCode / 36) * 51; // 6 levels (0, 51, 102, 153, 204, 255)
    g = ((colourCode / 6) % 6) * 51;
    b = (colourCode % 6) * 51;
  } else { // 24 grayscale shades (232-255)
    r = g = b = 8 + (colourCode - 232) * 10; // Start at 8, increase by 10
  }

  return ({ r, g, b });
}

/**
 * Converts RGB values to a hexadecimal color string.
 *
 * @param {int*} rgb - Array containing RGB values [r, g, b]
 * @returns {string} Hexadecimal color string in format "RRGGBB"
 */
varargs string rgbToHex(int *rgb) {
  return sprintf("%02X%02X%02X", rgb[0], rgb[1], rgb[2]);
}

/**
 * Converts an RGB color array to a color sequence string.
 *
 * @param {int*} rgb - Array of 3 integers representing RGB values
 * @param {int} mode - 0 for foreground, 1 for background
 * @returns {string} ANSI escape sequence for the color
 */
public string rgbToSequence(int *rgb, int mode) {
  int r, g, b;
  int bg = !!mode ? 48 : 38;

  r = rgb[0];
  g = rgb[1];
  b = rgb[2];

  return sprintf("\e[%d;2;%d;%d;%dm", bg, r, g, b);
}

int *hexToRgb(string hex) {
  int r, g, b;
  string *match;

  normalizeHex(ref hex);

  sscanf(hex[0..1], "%x", r);
  sscanf(hex[2..3], "%x", g);
  sscanf(hex[4..5], "%x", b);

  return ({r, g, b});
}

/**
 * Converts a hex color string to an ANSI escape sequence.
 *
 * @param {string} hex - Hex color code (3 or 6 digits)
 * @param {int} mode - 0 for foreground, 1 for background
 * @returns {string} ANSI escape sequence for the color
 */
varargs string hextToSequence(string hex, int mode) {
  int *rgb;

  rgb = hexToRgb(hex);

  return rgbToSequence(rgb, mode);
}

/**
 * Performs resynchronization of the colour system.
 *
 * @returns {void}
 */
void resync() {
  cacheAttributes();
  cache256();
}
