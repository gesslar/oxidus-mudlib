// /std/modules/bank.c
//
// Created:     2024/02/29: Gesslar
// Last Change: 2024/02/29: Gesslar
//
// 2024/02/29: Gesslar - Created

#include <daemons.h>

void add_command(string cmd, string fun);

void init_bank() {
  add_command("register", "cmd_register");
  add_command("deposit", "cmd_deposit");
  add_command("withdraw", "cmd_withdraw");
  add_command("balance", "cmd_balance");
}

mixed cmd_balance(object tp, string arg) {
  mixed result;

  result = BANK_D->query_balance(tp->query_name());
  if(nullp(result))
    return "You do not have an account with the bank.\n";

  return "You have " + add_commas(result) + " coins in your account.\n";
}
mixed cmd_register(object tp, string arg) {
  mixed result;

  result = BANK_D->new_account(tp->query_name());
  if(stringp(result))
    return result + "\n";

  if(result == 0)
    return "You already have an account with the bank.\n";

  return "You have successfully registered an account with the bank.\n";
}

mixed cmd_deposit(object tp, string str) {
  mixed *config = mudConfig("CURRENCY");
  int num, conv, have;
  string type;
  mixed result;
  string name;

  if(!str)
    return "Deposit what?\n";

  if(sscanf(str, "%d %s", num, type) != 2)
    return "Syntax: deposit <number> <type>\n";

  if(num < 1)
    return "You must deposit at least one coin.\n";

  if(!CURRENCY_D->valid_currency_type(type))
    return "That is not a valid currency type.\n";

  name = tp->query_name();

  result = BANK_D->query_balance(name);
  if(nullp(result))
    return "You do not have an account with the bank.\n";

  have = tp->query_wealth(type);
  if(have < num)
    return "You do not have that many " + type + " coins.\n";

  if(nullp(tp->adjust_wealth(type, -num)))
    return "We were unable to process your transaction.\n";

  conv = to_int(CURRENCY_D->convert_currency(num, type, "copper"));

  result = BANK_D->add_balance(name, conv);
  if(stringp(result))
    return result + "\n";

  return "You have deposited " + add_commas(num) + " " + type + " coins.\n";
}

mixed cmd_withdraw(object tp, string str) {
  mixed *config = mudConfig("CURRENCY");
  int num, base, have, rate;
  string type;
  mixed result;
  string name;

  if(!str)
    return "Withdraw what?\n";

  if(sscanf(str, "%d %s", num, type) != 2)
    return "Syntax: withdraw <number> <type>\n";

  if(num < 1)
    return "You must withdraw at least one coin.\n";

  if(!CURRENCY_D->valid_currency_type(type))
    return "That is not a valid currency type.\n";

  name = tp->query_name();

  result = BANK_D->query_balance(name);
  if(nullp(result))
    return "You do not have an account with the bank.\n";

  base = to_int(CURRENCY_D->convert_currency(num, type, "copper"));

  result = BANK_D->query_balance(name);
  if(base > result)
    return "You do not have that many coins in your account.\n";

  result = BANK_D->add_balance(name, -base);
  if(stringp(result))
    return result + "\n";

  result = tp->adjust_wealth(type, num);
  if(stringp(result)) {
    BANK_D->add_balance(name, base);
    return append(result, "\n");
  }

  return "You have withdrawn " + add_commas(num) + " " + type + " coins.\n";
}
