/* inventory.c

 Tacitus @ LPUniversity
 28-OCT-05
 Standard command

*/

inherit STD_CMD;

string query_wealth(object tp);

/**
 * Main entry point to the command.
 *
 * @param {STD_BODY} tp - This player.
 * @param {string} args - The arguments to the command.
 * @returns {mixed} The result of the command.
 */
mixed main(object tp, string args) {
  string* shorts;
  object* equipped = values(tp->query_equipped());
  object* wielded = values(tp->query_wielded());
  /** @type {STD_ITEM*} */ object* inventory;

  inventory = find_targets(tp, args, tp);

  shorts = map(inventory, function(
    /** @type {STD_ITEM} */ object ob,
    /** @type {STD_BODY} */ object tp,
    /** @type {STD_CLOTHING*} */ object* equipped,
    /** @type {STD_WEAPON*} */ object* wielded
  ) {
    string result;

    if(!ob->query_short())
      return 0;

    result = get_short(ob);

    if(of(ob, equipped))
      result += " (equipped)";
    else if(of(ob, wielded))
      result += " (wielded)";

    if(devp(tp))
      result += " (" + file_name(ob) + ")";

    return result;
  }, tp, equipped, wielded);

  shorts -= ({ 0 });

  if(sizeof(shorts) > 1)
    shorts += ({ "" });

  if(sizeof(shorts) > 0)
    shorts = ({ "Inventory:" }) + shorts;

  string wealth = query_wealth(tp);

  if(sizeof(wealth))
    shorts += ({ "Your purse contains " + wealth + ".", "" });

  int fill = tp->query_fill();
  int cap = tp->query_capacity();

  shorts += ({ sprintf("Carrying: %d/%d", fill, cap) });

  return shorts;
}

/**
 *
 * @param {STD_BODY} tp - This player.
 * @returns {string} The wealth string.
 */
string query_wealth(object tp) {
  string *currencies = CURRENCY_D->currency_list();
  string *out = ({ });

  if(!sizeof(currencies))
    return "No currency is currently in use.";

  currencies = reverse_array(currencies);
  foreach(string currency in currencies) {
    int num = tp->query_wealth(currency);

    if(num > 0)
      out += ({ sprintf("%d %s", tp->query_wealth(currency), currency) });
  }

  return implode(out, ", ");
}

/**
 * Help interface for this command.
 *
 * @param {STD_BODY} _tp - This player.
 * @returns {string} The help text.
 */
string query_help(object _tp) {
  return(" SYNTAX: inventory\n\n"
      "This command displays a list of everything that is currently\n"
      "in your inventory.\n");
}
