/**
 * @file /std/ext/clean.c
 * Clean up routine
 *
 * @created 2022/08/24 - Gesslar
 * @last_modified 2022/08/24 - Gesslar
 *
 * @history
 * 2022/08/24 - Gesslar - Created
 */

#include <clean.h>

// Variables
private nosave int no_clean_up = 0;
private nosave int debug_clean = 0;

// Functions
int can_clean_up();
varargs int set_no_clean(int no_clean);
int query_no_clean();
int request_clean_up() { return 1 ; }
private int clean_up_check(mixed ob);

// Functions from other objects
int is_daemon();
int is_command();
int remove();

void set_debug_clean(int i) {
  debug_clean = !!i;
}

/*protected*/ int clean_up(int refs) {
  /** @type {STD_OBJECT*} */
  object *contents;
  int check;

  if(debug_clean) debug("%O checking if we are to clean up.", this_object());

  // If we have an environment, straight up don't ask again. We can never
  // lose our environment and only non-environmented things are cleaned up.
  // Things with an environment rely on their environment to clean them up.
  if(environment()) {
    if(debug_clean) debug("   ❌ %O never cleaning again because it has an environment.", this_object());
    return CLEAN_NEVER_AGAIN;
  }

  // Never clean up if:
  // * We are a user
  // * We are interactive
  // * We have the no_clean_up flag set
  check = clean_up_check(this_object());

  if(check > 0) {
    if(debug_clean) debug("   ❌ %O never cleaning again because it is a user, interactive, or is no_clean.", this_object());
    return CLEAN_NEVER_AGAIN;
  }

  // Now ask permission to clean up. If we answer false, we'll check again
  // later.
  if(request_clean_up() == 0) {
    if(debug_clean) debug("   ⌛ %O cleaning up later because it requested not to clean up.", this_object());
    return CLEAN_LATER;
  }

  // If we have more than one reference to use, such as when we're cloned,
  // clean up later when we have no references. We don't even check for
  // virtual items, because refs is weird with them.
  // if(clonep())
  if(refs > 1) {
    if(debug_clean) debug("   ⌛ Number of references: %O", refs);
    if(!virtualp()) {
      if(debug_clean) debug("   ⌛ %O cleaning up later because it has more than one reference.", this_object());
      return CLEAN_LATER;
    }
  }

  // If we are a command or we are a daemon, we need to not clean up if
  // we have any pending call_outs. We'll try again later.
  if(call_if(this_object(), "is_command") || call_if(this_object(), "is_daemon")) {
    mixed *calls;

    calls = call_out_info();
    calls = filter(calls, (: $1[0] == $2 :), this_object());

    if(sizeof(calls)) {
      if(debug_clean) debug("   ⌛ %O cleaning up later because it has pending call_outs.", this_object());
      return CLEAN_LATER;
    }
  }

  // If we're a room, or a container, or anything of the like, we need to
  // determine if we have any items in us that need to be cleaned up.
  contents = deep_inventory();
  if(clean_up_check(contents) > 0) {
    if(debug_clean) debug("   ⌛ %O cleaning up later because it has users or interactives in it.", this_object());
    return CLEAN_LATER;
  }

  contents = filter(contents, (: $1->request_clean_up() == 0 :));
  if(sizeof(contents)) {
    if(debug_clean) debug("   ⌛ %O cleaning up later because it has items that requested not to clean up.", this_object());
    return CLEAN_LATER;
  }

  // We're definitely cleaning up now. We'll tell everyone we're cleaning
  // up, and then we'll remove ourselves.
  event(this_object(), "cleaning_up");

  contents = all_inventory();
  filter(contents, (: $1->remove() :));
  contents -= ({ 0 });
  contents->move(ROOM_VOID);

  if(debug_clean) debug("   ✔️ %O cleaning up now.", this_object());

  call_if(this_object(), "remove");

  if(this_object())
    destruct();

  return CLEAN_NEVER_AGAIN;
}

int can_clean() {
  return request_clean_up();
}

varargs int set_no_clean(int no_clean: (: 1 :)) {
  no_clean_up = !!no_clean;

  return no_clean_up;
}

int query_no_clean() {
  return no_clean_up;
}

/**
 *
 * @param {STD_OBJECT*} obs - The objects
 */
private int clean_up_check(mixed obs) {
  int result;

  if(objectp(obs))
    obs = ({ obs });

  if(!pointerp(obs))
    return 0;

  foreach(object ob in obs) {
    if(!objectp(ob))
      continue;

    if(ob->query_no_clean())
      result++;
    else if(userp(ob) || interactive(ob))
      result++;
  }

  return result;
}
