/**
 * @file /adm/daemons/persist.c
 *
 * Persist daemon that periodically saves all registered persistent
 * objects. Discovers persistent objects at startup and responds to
 * system crash and persist signals.
 *
 * @created 2024-03-05 - Gesslar
 * @last_modified 2024-03-05 - Gesslar
 *
 * @history
 * 2024-03-05 - Gesslar - Created
 */

private void findPersistentObjects();
public void registerPersistent(object ob);
public void unregisterPersistent(object ob);
public void persistObjects();

private nosave object *__persistents = ({});

void setup() {
  set_heart_beat(30);
  findPersistentObjects();
  slot(SIG_SYS_CRASH, "persistObjects");
  slot(SIG_SYS_PERSIST, "persistObjects");
}

/**
 * Scans all loaded objects and populates the persistent object list
 * with those that report as persistent.
 */
private void findPersistentObjects() {
  __persistents = objects((: $1->query_persistent() :));
}

/**
 * Registers an object for periodic persistence saving.
 *
 * @param {object} ob - The object to register
 */
public void registerPersistent(object ob) {
  if(member_array(ob, __persistents) == -1)
    __persistents += ({ ob });
}

/**
 * Unregisters an object from periodic persistence saving.
 *
 * @param {object} ob - The object to unregister
 */
public void unregisterPersistent(object ob) {
  if(member_array(ob, __persistents) != -1)
    __persistents -= ({ ob });
}

/**
 * Saves all registered persistent objects, removing any that have
 * been destructed.
 */
public void persistObjects() {
  __persistents -= ({ 0 });
  catch(filter(__persistents, (: $1->save_data() :)));
}

void heart_beat() {
  persistObjects();
}
