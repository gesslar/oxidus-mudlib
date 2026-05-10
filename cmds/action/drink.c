/**
 * @file /cmds/action/drink.c
 *
 * Drink command.
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
 * @param {string} str - The item to be drunk.
 * @returns {string | int} 1 if successful, an error message otherwise.
 */
mixed main(object tp, string str) {
  /** @type {STD_DRINK} */ object ob;
  int uses;

  if(!ob = find_target(tp, str, tp))
    return "You don't have that.";

  if(!ob->is_drink())
    return "You can't drink that.";

  uses = ob->query_uses();

  if(uses < 1)
    return "There is nothing left to drink.";

  if(!ob->drink_obj(tp))
    return "You couldn't drink that.";

  return 1;
}
