/**
 * @file /std/modules/currency.c
 * @description Currency module that can be inherited for money handling
 *
 * @created 2024-08-01 - Gesslar
 * @last_modified 2024-09-03 - Gesslar
 *
 * @history
 * 2024-08-01 - Gesslar - Created
 * 2024-09-03 - Gesslar - Converted currency to use an integer representing
 *                          the lowest denomination.
 */

#include <daemons.h>

// Function prototypes
private mixed check_funds(object tp, int amount);
private mixed transfer_funds(object from, object to, string currency, int amount);
private mixed check_capacity(object tp, string currency, int amount);
private mixed complex_transaction(object tp, int cost);
private mixed *format_return_currency(mapping m);
public mapping least_coins(int total_amount);

// Main transaction function
varargs mixed handle_transaction(object tp, int cost) {
    if(!tp || !objectp(tp)) {
        return "Invalid player object.";
    }

    return complex_transaction(tp, cost);
}

mixed complex_transaction(object tp, int cost) {
    mapping wealth;
    string *currencies;
    int total_wealth, remaining_cost, change_amount, used_value;
    mapping to_subtract, change;
    string curr;
    int i, available, to_use, curr_value;
    int amount;
    int currency_index;

    wealth = tp->query_all_wealth();
    currencies = reverse_array(CURRENCY_D->currency_list());
    total_wealth = tp->query_total_wealth(); // Total wealth in base currency
    remaining_cost = cost; // Set remaining cost directly to cost
    change_amount = 0;
    used_value = 0;
    to_subtract = ([]);
    change = ([]);

    // Sanity checks
    if(cost <= 0) return "Transaction amount must be positive.";
    if(total_wealth < remaining_cost) return "You cannot afford this transaction.";

    // Find the index of the transaction currency
    currency_index = 0; // Set to 0 as we're starting from the base currency

    // There is still a slight issue where sometimes it tries to grab an extra
    // coin from a higher denomination to cover the cost. This is only really
    // an issue if you don't have enough where it will result in a "no correct
    // "combination" message when you do actually have enough, it's just not
    // picking the right combo. - Gesslar 2024-08-04

    // Example of buying something costed at 25 silver
    //    > do reset,buy car
    //    • Reset called on Olum Village Shop (/d/village/shop).
    //    You buy toy car for 2 gold, 9 silver and receive 4 silver in change.
    //    > do reset,buy car
    //    • Reset called on Olum Village Shop (/d/village/shop).
    //    You buy toy car for 3 gold, 4 silver and receive 9 silver in change.

    // Process currencies starting from the transaction currency, then higher, then lower
    for(i = currency_index; i >= 0; i--) {
        curr = currencies[i];
        curr_value = CURRENCY_D->currency_value(curr);
        available = wealth[curr];

        // printf("DEBUG: Currency: %s, Available: %d units (%d copper each)\n", curr, available, curr_value);
        // printf("DEBUG: Available in copper: %d, Remaining cost in copper: %d\n", available * curr_value, remaining_cost);

        to_use = min(({available, (remaining_cost + curr_value - 1) / curr_value}));
        used_value = to_use * curr_value;

        if(to_use > 0) {
            to_subtract[curr] = (to_subtract[curr] || 0) + to_use;
            remaining_cost -= used_value;
            // printf("DEBUG: Using %d units of %s (value: %d copper)\n", to_use, curr, used_value);
            // printf("DEBUG: Subtracted %d units of %s, New remaining cost: %d copper\n", to_use, curr, remaining_cost);
        }

        if(remaining_cost <= 0) break;
    }

    // If still not enough, go for lower denominations
    if(remaining_cost > 0) {
        for(i = currency_index + 1; i < sizeof(currencies); i++) {
            curr = currencies[i];
            curr_value = CURRENCY_D->currency_value(curr);
            available = wealth[curr];

            to_use = min(({available, (remaining_cost + curr_value - 1) / curr_value}));
            used_value = to_use * curr_value;

            if(to_use > 0) {
                to_subtract[curr] = (to_subtract[curr] || 0) + to_use;
                remaining_cost -= used_value;
                // printf("DEBUG: Using %d units of %s (value: %d copper)\n", to_use, curr, used_value);
                // printf("DEBUG: Subtracted %d units of %s, New remaining cost: %d copper\n", to_use, curr, remaining_cost);
            }

            if(remaining_cost <= 0) break;
        }
    }

    if(remaining_cost > 0) return "You don't have the right combination of coins for this transaction.";

    // Calculate change
    if(remaining_cost < 0) {
        change_amount = -remaining_cost;
        for(i = 0; i < sizeof(currencies); i++) {
            int change_in_curr;
            curr = currencies[i];
            curr_value = CURRENCY_D->currency_value(curr);
            change_in_curr = change_amount / curr_value;
            if(change_in_curr > 0) {
                change[curr] = change_in_curr;
                change_amount %= curr_value;
            }
            if(change_amount == 0) break;
        }
    }

    // Capacity checks
    {
        int use_mass = mudConfig("USE_MASS");

        if(use_mass) {
            int capacity = tp->query_capacity();
            int subtract_mass = sum(values(to_subtract));
            int adjust_mass = sum(values(change));
            int current_fill = tp->query_fill();
            int net = adjust_mass - subtract_mass;

            if(current_fill + net > capacity) {
                return "You can't carry that much currency.";
            }
        }
    }

    // Apply the transaction
    foreach(curr, amount in to_subtract) tp->adjust_wealth(curr, -amount);
    foreach(curr, amount in change) tp->adjust_wealth(curr, amount);

    return ({ format_return_currency(to_subtract), format_return_currency(change) });
}

private mixed *format_return_currency(mapping m) {
    mixed *result = ({});
    string *currencies = reverse_array(CURRENCY_D->currency_list());

    foreach(string currency in currencies) {
        if(m[currency] && m[currency] > 0) {
            result += ({ ({ currency, m[currency] }) });
        }
    }

    return result;
}

mixed reverse_transaction(object tp, mixed transaction_result) {
    mixed *subtracted;
    mixed *change;
    mapping to_add;
    mapping to_subtract;
    string currency;
    int amount;

    if(!arrayp(transaction_result) || sizeof(transaction_result) != 2) {
        return "Invalid transaction result";
    }

    subtracted = transaction_result[0];
    change = transaction_result[1];
    to_add = ([]);
    to_subtract = ([]);

    // First, subtract the change
    foreach(mixed *currency_info in change) {
        currency = currency_info[0];
        amount = currency_info[1];
        if(tp->query_wealth(currency) < amount) {
            return "Not enough " + currency + " to reverse the transaction";
        }
        tp->adjust_wealth(currency, -amount);
        to_subtract[currency] = (to_subtract[currency] || 0) + amount;
    }

    // Then, add back the subtracted coins
    foreach(mixed *currency_info in subtracted) {
        currency = currency_info[0];
        amount = currency_info[1];
        tp->adjust_wealth(currency, amount);
        to_add[currency] = (to_add[currency] || 0) + amount;
    }

    // Return the net change
    return ({ format_return_currency(to_add), format_return_currency(to_subtract) });
}

string format_return_currency_string(mixed *currency_array) {
    string result = "";

    if(!sizeof(currency_array)) {
        return "None";
    }

    foreach(mixed *currency_info in currency_array) {
        string currency = currency_info[0];
        int amount = currency_info[1];

        if(result != "") {
            result += ", ";
        }

        result += amount + " " + currency;
    }

    return result;
}

// Check if the player has enough funds
private mixed check_funds(object tp, int amount) { // Remove currency parameter
    int total_wealth = tp->query_total_wealth(); // Get total wealth in base currency
    if(total_wealth < amount) {
        return "You don't have enough funds for this transaction.";
    }
    return 1;
}

// Check if the player can handle the change in capacity
private mixed check_capacity(object tp, string currency, int amount) {
    int current_fill = tp->query_fill();
    int capacity = tp->query_capacity();
    int coin_difference = amount - tp->query_wealth(currency);

    if(current_fill + coin_difference > capacity) {
        return "You can't carry that much currency.";
    }
    return 1;
}

// Transfer funds from one object to another
private mixed transfer_funds(object from, object to, string currency, int amount) {
    int from_result, to_result;

    from_result = from->adjust_wealth(currency, -amount);
    if(from_result < 0) {
        return "Failed to remove funds from the source.";
    }

    to_result = to->adjust_wealth(currency, amount);
    if(to_result < 0) {
        // Revert the transaction if adding to the destination fails
        from->adjust_wealth(currency, amount);
        return "Failed to add funds to the destination.";
    }

    return 1;
}

// Convert currency for a transaction
mixed convert_for_transaction(object tp, int cost, string from_currency, string to_currency) {
    int converted_amount;
    mixed result;

    if(!CURRENCY_D->valid_currency_type(from_currency) ||
        !CURRENCY_D->valid_currency_type(to_currency)) {
        return "Invalid currency type.";
    }

    converted_amount = CURRENCY_D->convert_currency(cost, from_currency, to_currency);

    result = check_funds(tp, cost);
    if(stringp(result)) return result;

    result = check_capacity(tp, to_currency, converted_amount);
    if(stringp(result)) return result;

    if(tp->convert_wealth(from_currency, to_currency, cost)) {
        return converted_amount;
    }

    return "Currency conversion failed.";
}

// Function to check if an object can afford a transaction
mixed can_afford(object ob, int cost, string currency) {
    if(!ob || !objectp(ob)) {
        return "Invalid object.";
    }

    if(ob->query_wealth(currency) >= cost) {
        return 1;
    }

    return "Not enough funds.";
}

// Function to format currency for display
string format_currency(int amount, string currency) {
    return amount + " " + currency;
}

public mapping least_coins(int total_amount) {
    string *currencies;
    mapping result = ([]);
    string currency;
    int value;
    int amount;

    currencies = CURRENCY_D->currency_list(); // Get the list of currencies
    currencies = reverse_array(currencies); // Reverse the array for highest to lowest

    foreach(currency in currencies) {
        value = CURRENCY_D->currency_value(currency); // Get the value of the currency
        if(value > 0) {
            amount = total_amount / value; // Calculate how many of this currency
            if(amount > 0) {
                result[currency] = amount; // Store the amount in the result
                total_amount %= value; // Update the remaining amount
            }
        }
    }

    return result;
}
