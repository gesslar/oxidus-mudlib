/**
 * @file /adm/daemons/ga_server.c
 *
 * Global Alias Server. Parses a configuration file of global
 * aliases and extended verbs, then serves them to players based
 * on privilege group membership.
 *
 * @created 2006-01-14 - Tacitus @ LPUniversity
 * @last_modified 2006-01-14 - Tacitus @ LPUniversity
 *
 * @history
 * 2006-01-14 - Tacitus @ LPUniversity - Created
 */

#include <origin.h>

#define CONFIG_FILE "/adm/etc/aliases"

inherit STD_DAEMON;

private void parseConfig();
private string *parseLines(string str);
private void addAlias(string verb, string cm, string *groups);
public mapping getAlias(string priv);
public mapping getXverb(string priv);

private mapping __xverb = ([]);
private mapping __alias = ([]);

void setup() {
  set_no_clean(1);
  parseConfig();
}

/**
 * Reads and parses the global alias configuration file, populating
 * the alias and extended verb mappings by privilege group.
 */
private void parseConfig() {
  int i, totalErrors = 0, totalParsed = 0;
  string *conf;
  string *curGroups = ({});
  string out = "";
  float time;

  time = time_frac();
  conf = parseLines(read_file(CONFIG_FILE));

  for(i = 0; i < sizeof(conf); i++) {
    string groups, verb, al;

    if(!conf[i])
      continue;

    if(sscanf(conf[i], ":;%s:", groups)) {
      curGroups = explode(groups, " ");
      continue;
    }

    if(!sizeof(curGroups)) {
      out += "\n";
      out += "\tGlobal Alias Server Error: No assignment definition found.\n";
      out += "\tGlobal Alias Server Error: Global Aliases were not parsed.\n";

      return;
    }

    if(sscanf(conf[i], "%s %s", verb, al)) {
      addAlias(verb, al, curGroups);
      totalParsed++;
    } else {
      out += "\n";
      out += "\tGlobal Alias Server Error: "
        "Definition found (" + i + ") but in invalid format.\n";
      totalErrors++;
    }
  }

  out += "\nGlobal Alias Server: " + totalParsed
    + " global aliases parsed. " + totalErrors
    + " errors encountered. "
    + sprintf("(%.2fms)\n", time_frac() - time);
}

/**
 * Splits a raw config string into lines, stripping comments and
 * blank lines.
 *
 * @param {string} str - The raw file contents to parse
 * @returns {string*} Cleaned lines, with nulls where lines were
 *                    removed
 */
private string *parseLines(string str) {
  string *arr;
  int i;

  if(!str)
    return ({});

  arr = explode(str, "\n");

  for(i = 0; i < sizeof(arr); i++) {
    if(arr[i][0] == '#' || arr[i] == "") {
      arr[i] = 0;
      continue;
    }

    arr[i] = replace_string(arr[i], "\t", "");
  }

  return arr;
}

/**
 * Registers an alias or extended verb for the given privilege
 * groups. Verbs prefixed with `$` are stored as extended verbs.
 *
 * @param {string} verb - The alias verb (prefix with $ for xverb)
 * @param {string} cmd - The command the alias expands to
 * @param {string*} groups - Privilege groups this alias belongs to
 */
private void addAlias(string verb, string cmd, string *groups) {
  int i;

  if(origin() != ORIGIN_LOCAL)
    return;

  if(verb[0] == '$' && strlen(verb) > 1) {
    if(!mapp(__xverb))
      __xverb = ([]);

    for(i = 0; i < sizeof(groups); i++) {
      if(!__xverb[groups[i]])
        __xverb += ([groups[i] : ([])]);

      __xverb[groups[i]] += ([verb[1..<1] : cmd]);
    }
  } else {
    if(!mapp(__alias))
      __alias = ([]);

    for(i = 0; i < sizeof(groups); i++) {
      if(!__alias[groups[i]])
        __alias += ([groups[i] : ([])]);

      __alias[groups[i]] += ([verb : cmd]);
    }
  }
}

/**
 * Returns the merged alias mapping for a given privilege string,
 * combining all groups the caller belongs to plus the "all" group.
 *
 * @param {string} priv - The privilege string to match against
 * @returns {mapping} Combined alias mapping of verb to command
 */
public mapping getAlias(string priv) {
  int i;
  string *k = keys(__alias);
  mapping ret = ([]);

  for(i = 0; i < sizeof(k); i++)
    if(is_member(priv, k[i]) || k[i] == "all")
      ret += __alias[k[i]];

  return ret;
}

/**
 * Returns the merged extended verb mapping for a given privilege
 * string, combining all groups the caller belongs to plus the
 * "all" group.
 *
 * @param {string} priv - The privilege string to match against
 * @returns {mapping} Combined xverb mapping of verb to command
 */
public mapping getXverb(string priv) {
  int i;
  string *k = keys(__xverb);
  mapping ret = ([]);

  for(i = 0; i < sizeof(k); i++)
    if(is_member(priv, k[i]) || k[i] == "all")
      ret += __xverb[k[i]];

  return ret;
}
