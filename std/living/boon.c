/**
 * @file /std/living/boon.c
 * @description Buffs/debuffs and other boons for living objects.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */

#include <boon.h>
#include <attributes.h>
#include <living.h>
#include <skills.h>

private nomask mapping boons = ([]);
private nomask mapping curses = ([]);

private nomask nosave int BOON = 1;
private nomask nosave int CURSE = 2;

// Forward declarations for private helpers
private nomask mapping getStore(int which);
private nomask int apply(
  int which, string name, string cl,
  string type, int amt, int dur
);
private nomask int query(int which, string cl, string type);
private nomask void process(int which);
private nomask int removeByTag(
  int which, string cl, string type, int tag
);
private nomask int removeByName(
  int which, string cl, string type, string name
);

public nomask void init_boon() {
  boons = boons || ([]);
  curses = curses || ([]);
}

public nomask int boon(
  string name, string cl, string type,
  int amt, int dur
) {
  return apply(BOON, name, cl, type, amt, dur);
}

public nomask int curse(
  string name, string cl, string type,
  int amt, int dur
) {
  return apply(CURSE, name, cl, type, amt, dur);
}

public nomask int query_boon(string cl, string type) {
  return query(BOON, cl, type);
}

public nomask int query_curse(string cl, string type) {
  return query(CURSE, cl, type);
}

public mapping query_boon_data() {
  return copy(boons);
}

public mapping query_curse_data() {
  return copy(curses);
}

public nomask int query_effective_boon(
  string cl, string type
) {
  return query_boon(cl, type) - query_curse(cl, type);
}

public nomask int remove_boon(
  string cl, string type, int tag
) {
  return removeByTag(BOON, cl, type, tag);
}

public nomask int remove_curse(
  string cl, string type, int tag
) {
  return removeByTag(CURSE, cl, type, tag);
}

public nomask int remove_boon_by_name(
  string cl, string type, string name
) {
  return removeByName(BOON, cl, type, name);
}

public nomask int remove_curse_by_name(
  string cl, string type, string name
) {
  return removeByName(CURSE, cl, type, name);
}

protected nomask void process_boon() {
  process(BOON);
  process(CURSE);
}

// --- Private helpers ---

private nomask mapping getStore(int which) {
  return which == BOON ? boons : curses;
}

private nomask int apply(
  int which, string name, string cl,
  string type, int amt, int dur
) {
  mapping src = getStore(which);
  int tag = time_ns();

  if(nullp(name) || nullp(cl) || nullp(type)
      || nullp(amt) || nullp(dur))
    return 0;

  if(!of(cl, src))
    src[cl] = ([]);

  if(!of(type, src[cl]))
    src[cl][type] = ([]);

  // Avoid tag collision on same-nanosecond calls
  while(of(tag, src[cl][type]))
    tag++;

  src[cl][type][tag] = ([
    "name" : name,
    "amt" : amt,
    "expires" : time() + dur,
  ]);

  return tag;
}

private nomask int query(
  int which, string cl, string type
) {
  mapping src = getStore(which);
  int total = 0;

  if(!of(cl, src) || !of(type, src[cl]))
    return 0;

  foreach(mapping data in src[cl][type]) {
    total += data["amt"];
  }

  return total;
}

private nomask void process(int which) {
  mapping src = getStore(which);
  int now = time();
  string cl, type;
  mapping classData, typeData;

  // Collect expired tags first to avoid mutating
  // during iteration.
  mixed *expired = ({});

  foreach(cl, classData in src) {
    foreach(type, typeData in classData) {
      foreach(int tag, mapping entry in typeData) {
        if(entry["expires"] < now) {
          expired += ({ ({
            cl, type, tag, entry["name"],
          }) });
        }
      }
    }
  }

  // Now remove and notify.
  foreach(mixed *info in expired) {
    string eCl = info[0];
    string eType = info[1];
    int eTag = info[2];
    string eName = info[3];

    map_delete(src[eCl][eType], eTag);
    tell(
      this_object(),
      "Your " + eName + " has worn off.\n"
    );

    // Prune empty inner mappings.
    if(!sizeof(src[eCl][eType]))
      map_delete(src[eCl], eType);

    if(!sizeof(src[eCl]))
      map_delete(src, eCl);
  }
}

private nomask int removeByTag(
  int which, string cl, string type, int tag
) {
  mapping src = getStore(which);

  if(!of(cl, src)
      || !of(type, src[cl])
      || !of(tag, src[cl][type]))
    return 0;

  map_delete(src[cl][type], tag);

  if(!sizeof(src[cl][type]))
    map_delete(src[cl], type);

  if(!sizeof(src[cl]))
    map_delete(src, cl);

  return 1;
}

private nomask int removeByName(
  int which, string cl, string type, string name
) {
  mapping src = getStore(which);
  int removed = 0;

  if(!of(cl, src) || !of(type, src[cl]))
    return 0;

  int *tags = ({});
  foreach(int tag, mapping data in src[cl][type]) {
    if(data["name"] == name)
      tags += ({ tag });
  }

  foreach(int tag in tags) {
    map_delete(src[cl][type], tag);
    removed++;
  }

  if(!sizeof(src[cl][type]))
    map_delete(src[cl], type);

  if(!sizeof(src[cl]))
    map_delete(src, cl);

  return removed;
}
