/**
 * Build-time utility that resolves syntax highlight colors from
 * Starlight's bundled Night Owl themes.
 *
 * Falls back gracefully if theme files can't be loaded.
 */

import { readFileSync, readdirSync } from "node:fs";
import { createRequire } from "node:module";

// --- JSONC parser (strip comments + trailing commas) ----

function parseJsonc(raw: string): any {
  const stripped = raw
    .replace(/\/\/.*$/gm, "")
    .replace(/\/\*[\s\S]*?\*\//g, "")
    .replace(/,\s*([}\]])/g, "$1");
  return JSON.parse(stripped);
}

// --- Scope matching -------------------------------------

interface TokenColor {
  scope?: string | string[];
  name?: string;
  settings: { foreground?: string };
}

interface ThemeData {
  colors?: Record<string, string>;
  tokenColors: TokenColor[];
}

/**
 * Scores how well `candidate` matches `target`.
 * Returns -1 for no match, or a positive specificity score.
 */
function scopeScore(target: string, candidate: string): number {
  if (target === candidate) return 1000 + candidate.length;
  if (target.startsWith(candidate + ".")) return candidate.length;
  if (candidate.startsWith(target + ".")) return target.length;
  return -1;
}

function resolveColor(theme: ThemeData, scopes: string[]): string | null {
  let bestColor: string | null = null;
  let bestScore = -1;

  for (const rule of theme.tokenColors) {
    const fg = rule.settings?.foreground;
    if (!fg) continue;

    const ruleScopes = Array.isArray(rule.scope)
      ? rule.scope
      : typeof rule.scope === "string"
        ? rule.scope.split(",").map((s) => s.trim())
        : [];

    for (const target of scopes) {
      for (const rs of ruleScopes) {
        const score = scopeScore(target, rs);
        if (score > bestScore) {
          bestScore = score;
          bestColor = fg;
        }
      }
    }
  }

  return bestColor;
}

// --- Highlight type → TextMate scope mapping ------------

const SCOPE_MAP: Record<string, string[]> = {
  function: ["entity.name.function"],
  type: ["storage.type"],
  object: ["entity.name.class", "entity.name.type.class"],
  parameter: ["variable.parameter.function", "variable.parameter"],
  variable: ["variable"],
  keyword: ["keyword"],
  string: ["string"],
  number: ["constant.numeric"],
  comment: ["comment"],
  constant: ["constant.language", "constant"],
  support: ["support.function", "support.constant"],
  tag: ["entity.name.tag"],
  property: ["meta.property-name"],
};

// --- Theme loading --------------------------------------

let themes: { dark: ThemeData; light: ThemeData } | null = null;

/**
 * Auto-detect Starlight theme plugins that ship syntax themes.
 * Scans node_modules for starlight-theme-* packages containing
 * JSON files with tokenColors.
 */
function loadPluginThemes(): { dark: ThemeData; light: ThemeData } | null {
  const require = createRequire(import.meta.url);

  // Find the node_modules directory
  let nodeModulesDir: string;
  try {
    const starlightPath = require.resolve("@astrojs/starlight");
    nodeModulesDir = starlightPath.replace(/\/@astrojs\/starlight\/.*$/, "/");
  } catch {
    return null;
  }

  // Scan for starlight-theme-* packages
  let themeDirs: string[];
  try {
    themeDirs = readdirSync(nodeModulesDir)
      .filter((name) => name.startsWith("starlight-theme-"));
  } catch {
    return null;
  }

  for (const pkg of themeDirs) {
    const pkgDir = nodeModulesDir + pkg + "/";

    // Recursively find JSON files that look like syntax themes
    const themeFiles = findThemeFiles(pkgDir);
    if (themeFiles.length === 0) continue;

    // Try to pair them as dark/light
    const pair = pairThemes(themeFiles);
    if (pair) return pair;

    // Single theme — use for both
    if (themeFiles.length === 1) {
      const theme = themeFiles[0]!;
      return { dark: theme, light: theme };
    }
  }

  return null;
}

/**
 * Find JSON files containing tokenColors in a directory (shallow + common subdirs).
 */
function findThemeFiles(dir: string): ThemeData[] {
  const results: ThemeData[] = [];
  const searchDirs = [dir];

  // Check common subdirectory names for theme files
  for (const sub of ["themes", "syntax-themes", "syntax", "dist"]) {
    searchDirs.push(dir + sub + "/");
  }

  for (const searchDir of searchDirs) {
    let files: string[];
    try {
      files = readdirSync(searchDir).filter((f) => f.endsWith(".json"));
    } catch {
      continue;
    }

    for (const file of files) {
      try {
        const data = JSON.parse(readFileSync(searchDir + file, "utf-8"));
        if (data.tokenColors && Array.isArray(data.tokenColors)) {
          results.push(data);
        }
      } catch {
        continue;
      }
    }
  }

  return results;
}

/**
 * Given theme files, try to pair them as dark/light.
 */
function pairThemes(themes: ThemeData[]): { dark: ThemeData; light: ThemeData } | null {
  if (themes.length < 2) return null;

  let dark: ThemeData | null = null;
  let light: ThemeData | null = null;

  for (const theme of themes) {
    const type = (theme as any).type as string | undefined;
    if (type === "dark" && !dark) dark = theme;
    else if (type === "light" && !light) light = theme;
  }

  if (dark && light) return { dark, light };

  // Fallback: just use first two
  return { dark: themes[0]!, light: themes[1]! };
}

/**
 * Load Starlight's bundled Night Owl themes (the default).
 */
function loadStarlightThemes(): { dark: ThemeData; light: ThemeData } | null {
  try {
    const require = createRequire(import.meta.url);
    const entryPath = require.resolve("@astrojs/starlight");
    const dir = entryPath.replace(/\/[^/]*$/, "/");
    const darkPath = dir + "integrations/expressive-code/themes/night-owl-dark.jsonc";
    const lightPath = dir + "integrations/expressive-code/themes/night-owl-light.jsonc";

    return {
      dark: parseJsonc(readFileSync(darkPath, "utf-8")),
      light: parseJsonc(readFileSync(lightPath, "utf-8")),
    };
  } catch {
    return null;
  }
}

/**
 * Load a Shiki bundled theme by name (e.g. "dracula", "nord", "monokai").
 */
async function loadShikiTheme(name: string): Promise<ThemeData | null> {
  try {
    const { bundledThemes } = await import("shiki/themes");
    const loader = bundledThemes[name as keyof typeof bundledThemes];
    if (!loader) return null;
    const mod = await loader();
    return (mod.default || mod) as ThemeData;
  } catch {
    return null;
  }
}

async function loadThemes(): Promise<{ dark: ThemeData; light: ThemeData } | null> {
  if (themes) return themes;
  themes = loadPluginThemes() || loadStarlightThemes();
  return themes;
}

// --- Public API -----------------------------------------

export interface ResolvedColors {
  [key: string]: { dark: string; light: string };
}

export async function resolveHighlightColors(): Promise<ResolvedColors | null> {
  const data = await loadThemes();
  if (!data) return null;

  const darkFg = data.dark.colors?.["editor.foreground"] || "#d6deeb";
  const lightFg = data.light.colors?.["editor.foreground"] || "#403f53";

  const result: ResolvedColors = {};

  for (const [name, scopes] of Object.entries(SCOPE_MAP)) {
    result[name] = {
      dark: resolveColor(data.dark, scopes) || darkFg,
      light: resolveColor(data.light, scopes) || lightFg,
    };
  }

  return result;
}

/**
 * Returns a CSS string that sets --cb-hl-* custom properties
 * for both light and dark themes, derived from the active theme.
 */
export async function generateThemeCSS(): Promise<string> {
  const colors = await resolveHighlightColors();
  if (!colors) return "";

  const darkVars = Object.entries(colors)
    .map(([name, c]) => `  --cb-hl-${name}: ${c.dark};`)
    .join("\n");

  const lightVars = Object.entries(colors)
    .map(([name, c]) => `  --cb-hl-${name}: ${c.light};`)
    .join("\n");

  return `:root[data-theme="dark"] .cb-browser {\n${darkVars}\n}\n:root[data-theme="light"] .cb-browser {\n${lightVars}\n}`;
}
