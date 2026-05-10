/**
 * @file /cmds/action/eat.c
 *
 * Eat command.
 *
 * @created 2024-08-06 - Gesslar
 * @last_modified 2024-08-06 - Gesslar
 *
 * @history
 * 2024-08-06 - Gesslar - Created
 */

inherit STD_ACT;

/**
 *
 * @param {STD_BODY} tp - The player
 * @param {string} str - The item to be eaten.
 * @returns {string | int} 1 if successful, an error message otherwise.
 */
mixed main(object tp, string str) {
  /** @type {STD_FOOD} */ object ob;
  int uses;

  if(!ob = find_target(tp, str, tp))
    return "You don't have that.";

  if(!ob->is_edible())
    return "You can't eat that.";

  uses = ob->query_uses();

  if(uses < 1)
    return "There is nothing left to eat.";

  if(!ob->eat_obj(tp))
    return "You couldn't eat that.";

  return 1;
}
