/**
 * @file /adm/daemons/help.c
 *
 * Help! Daemon!
 *
 * @created 2026-06-07 - Gesslar
 * @last_modified 2026-06-07 - Gesslar
 *
 * @history
 * 2026-06-07 - Gesslar - Created
 */

inherit STD_DAEMON;

#include <pcre_flags.h>

public void rehash();
private void rehash_directory(mapping directory);

private nosave string  __help_root;
private nosave string *__excludes; // wired, not yet filtered on — intentional
private nosave mapping __cache = ([]);

void setup() {
  set_no_clean();

  __help_root = mud_config("DOC.ROOT") + mud_config("DOC.HELP.ROOT");
  __excludes = map(
    mud_config("DOC.HELP.EXCLUDE") ?? ({}),
    (: __help_root + $1 :)
  );

  rehash();
}

public void rehash() {
  __cache = ([]);

  mapping fd = read_directory(__help_root);

  int sz = sizeof(fd["directories"]);

  while(sz--)
    call_out_walltime((: rehash_directory, fd["directories"][sz] :), sz * 0.1);
}

private void rehash_directory(mapping directory) {
  if(file_size(directory["path"]) != -2)
    return;

  mapping fd = read_directory(directory["path"], "*.help");
  mapping *files = fd["files"];
  mapping help = ([]);

  if(!sizeof(files))
    return;

  foreach(mapping file in files) {
    mapping front_matter;
    string front_matter_text;
    string content = read_file(file["path"]);

    if(falsy(content))
      continue;

    // Do we have front matter?
    string *matches =
    pcre_extract(
      content,
      "^---$([\\s\\S]+?)^---$([\\s\\S]+)",
      // @lpc-expect-error: the LSP doesn't know about these arguments
      0,
      PCRE_M
    );

    if(sizeof(matches) == 2) {
      content = matches[1];
      front_matter_text = matches[0];
    }

    if(truthy(front_matter_text)) {
      string e = catch(front_matter = lpml_decode(front_matter_text));

      if(e) {
        front_matter = ([
          "title": all_caps(file["base"]),
        ]);
      }
    } else {
      front_matter = ([
        "title": all_caps(file["base"])
      ]);
    }

    front_matter += ([ "topic" : file["base"] ]);

    help[file["base"]] = front_matter + ([ "content": content ]);
  }

  __cache[directory["name"]] = help;
}

mapping *query_help(string topic, object who) {
  who ??= this_body();

  string name = who
    ? query_privs(who)
    : NONAME;

  string *groups = master()->query_group_names();
  mapping *result = ({});

  foreach(string cat, mapping helps in __cache) {
    if(cat == "all" || !includes(groups, cat)) {
      if(helps[topic]) {
        push(ref result, ({ cat, helps[topic] }));
      }
    } else {
      if(helps[topic] && is_member(name, cat)) {
        push(ref result, ({ cat, helps[topic] }));
      }
    }
  }

  return result;
}
