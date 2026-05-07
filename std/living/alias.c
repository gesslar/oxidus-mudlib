/*

 Tacitus @ LPUniversity
 02-JAN-05
 Parses aliases

*/

/* Last edited on 19-JAN-06 by Tacitus. */

#include <alias.h>

mapping xverb = ([]);
mapping alias = ([]);

void add_alias(string verb, string cmd) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return;

  if(verb[0] == '$' && strlen(verb) > 1) {
    if(!mapp(xverb))
      xverb = ([]);

    xverb += ([ verb[1..<1] : cmd ]);
  } else {
    if(!mapp(alias))
      alias = ([]);

    alias += ([ verb : cmd ]);
  }
}

int remove_alias(string verb) {
  if(!adminp(previous_object()) && this_body() != this_object())
    return 0;

  if(alias[verb]) {
    map_delete(alias, verb);
    return 1;
  }

  if(xverb[verb]) {
    map_delete(xverb, verb);
    return 1;
  }

  return 0;
}

mapping get_aliases(int all) {
  if(!all)
    return alias + xverb + ([]);
  else
    return GA_SERVER->get_alias(query_privs()) + GA_SERVER->get_xverb(query_privs()) + ([]);
}

string compute_alias(string al, string args) {
  string *words = ({});
  string imp;
  int n = -1;

  if(args)
    words = explode(args, " ");

  al = replace_string(al, "$*", "::*::");

  while(n != 0) {
    sscanf(al, "%*s $%d %*s", n);
    sscanf(al, "%*s$%d%*s", n);

    sscanf(al, "%*s $%d", n);
    sscanf(al, "%*s$%d", n);

    sscanf(al, "$%d %*s", n);
    sscanf(al, "$%d%*s", n);

    sscanf(al, "$%d", n);

    if(n == -1)
      break;

    if(n < 1 || n > sizeof(words)) {
      al = replace_string(al, " $" + n + " ", "");
      al = replace_string(al, " $" + n, "");
      al = replace_string(al, "$" + n + " ", "");
      n = -1;
      continue;
    }

    if(stringp(words[n - 1])) {
      al = replace_string(al, "$" + n, words[n - 1]);
      words[n - 1] = 0;
      n = -1;
    } else {
      al = replace_string(al, "$" + n + " ", "");
      n = -1;
    }
  }

  while(al[<1] == ' ')
    al = al[0..<2];

  al = replace_string(al, "::*::", "$*");

  if(sizeof(words)) {
    imp = implode(words, " ");

    if(al[<2..<1] == "$*")
      al = replace_string(al, "$*", imp);

    if(!stringp(imp) || imp == "")
      al = replace_string(al, " $*", "");
    else
      al = replace_string(al, "$*", imp);
  } else {
    al = replace_string(al, " $*", "");
  }

  return al;
}

string alias_parse(string verb, string args) {
  string *keys;
  int i;
  mapping tmp_alias;
  mapping tmp_xverb;

  alias = mapp(alias) ? alias : ([]);
  xverb = mapp(xverb) ? xverb : ([]);

  tmp_alias = GA_SERVER->get_alias(query_privs()) + alias;
  tmp_xverb = GA_SERVER->get_xverb(query_privs()) + xverb;

  if(sizeof(tmp_alias))
    if(tmp_alias[verb])
      return(compute_alias(tmp_alias[verb], args));

  if(sizeof(tmp_xverb)) {
    keys = keys(tmp_xverb);

    for(i = 0; i < sizeof(keys); i++) {
      if(keys[i] == verb[0..(strlen(keys[i]) - 1)]) {
        if(args)
          return compute_alias(tmp_xverb[keys[i]], verb[(strlen(keys[i]))..(strlen(verb) - 1)] + " " + args);
        else
        return compute_alias(tmp_xverb[keys[i]], verb[(strlen(keys[i]))..(strlen(verb) - 1)]);
      }
    }
  }

  if(args && args != "")
    return(verb + " " + args);
  else
    return(verb);
}
