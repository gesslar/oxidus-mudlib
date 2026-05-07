// /adm/daemons/currency.c
// Currency daemon
//
// Created:     2024/02/19: Gesslar
// Last Change: 2024/02/20: Assistant

inherit STD_DAEMON;

// Variables
private mapping currency_map;
private string *currency_order;

void setup() {
  mixed *currency_config;

  currency_config = mud_config("CURRENCY");

  // Transform the array into a mapping
  currency_map = ([]);
  foreach(mixed *currency in currency_config)
      currency_map[currency[0]] = currency[1];

  // Create an ordered list of currencies
  currency_order = sort_array(keys(currency_map),
    (: currency_map[$1] - currency_map[$2] :)
  );
}

int valid_currency_type(string currency) {
  return member_array(currency, currency_order) != -1;
}

int convert_currency(int amount, string from_currency, string to_currency) {
  int from_rate, to_rate;
  float result;

  if(!valid_currency_type(from_currency) || !valid_currency_type(to_currency))
    return -1;

  from_rate = currency_map[from_currency];
  to_rate = currency_map[to_currency];

  // Use float for intermediate calculation to avoid integer division issues
  result = to_float(amount) * to_float(from_rate) / to_float(to_rate);

  // Round to nearest integer
  return to_int(result + 0.5);
}

float fconvert_currency(int amount, string from_currency, string to_currency) {
  float from_rate, to_rate;
  float result;

  if(!valid_currency_type(from_currency) || !valid_currency_type(to_currency))
    return -1;

  from_rate = to_float(currency_map[from_currency]);
  to_rate = to_float(currency_map[to_currency]);

  // Use float for intermediate calculation to avoid integer division issues
  result = to_float(amount) * from_rate / to_rate;

  // Round to nearest integer
  return result;
}

string lowest_currency() {
  return currency_order[0];
}

string highest_currency() {
  return currency_order[<1];
}

string *currency_list() {
  return currency_order;
}

int currency_value(string currency) {
  return currency_map[currency];
}

mapping get_currency_map() {
  return copy(currency_map);
}
