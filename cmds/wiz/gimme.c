/**
 * @file /cmds/wiz/gimme.c
 * @description Command to give coins to the wizard
 *
 * @created 2024-08-01 - Gesslar
 * @last_modified 2024-08-01 - Gesslar
 *
 * @history
 * 2024-08-01 - Gesslar - Created
 */

inherit STD_CMD;

mixed main(object tp, string str) {
  string type;
  int number;

  if(!str) {
    return _info("Usage: gimme <number> <type>");
  }

  if(sscanf(str, "%d %s", number, type) != 2) {
    return _info("Usage: gimme <number> <type>");
  }

  if(number < 1) {
    return _error("You must give at least one coin.");
  }

  if(!of(type, CURRENCY_D->currency_list())) {
    return _error("Invalid currency type.");
  }

  if(!tp->adjust_wealth(type, number)) {
    return _error("You can't add that many coins.");
  }

  return _ok("You receive %d %s.", number, type);
}
