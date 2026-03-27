/**
 * @file /cmds/std/wealth.c
 *
 * Command to display the player's current wealth.
 *
 * @created 2024-03-02 - Gesslar
 * @last_modified 2024-03-02 - Gesslar
 *
 * @history
 * 2024-03-02 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(/** @type {STD_BODY} */ object tp, string arg) {
    string *currencies = CURRENCY_D->currency_list();
    string *out = ({ });

    if(!sizeof(currencies))
        return "No currency is currently in use.";

    currencies = reverse_array(currencies);
    foreach(string currency in currencies)
        out += ({ sprintf("%s: %d", currency, tp->query_wealth(currency)) });

    out = ({ "You are carrying the following currencies:" }) + out;

    return out;
}
