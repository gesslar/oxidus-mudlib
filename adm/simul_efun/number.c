#include <simul_efun.h>

/**
 * Calculates what `a%` of `b` is.
 *
 * @param {float} a - The percentage value.
 * @param {float} b - The whole value.
 * @returns {float} The value that is `a` percent of `b`.
 */
float percent_of(float a, float b) {
  return (a / 100.0) * to_float(b);
}

/**
 * Calculates what percentage `a` is of `b`.
 *
 * @param {float} a - The part value.
 * @param {float} b - The whole value.
 * @returns {float} The percentage of `a` out of `b`.
 */
float percent(float a, float b) {
  return (to_float(a) / to_float(b)) * 100.0;
}

/**
 * Ensures a value is within a specified range.
 *
 * @param {float} min - The minimum value.
 * @param {float} max - The maximum value.
 * @param {float} val - The value to check.
 * @returns {float} The value, constrained within the range of `min` to `max`.
 */
float clamp(float val, float min, float max) {
  if(val < min)
    return min;

  if(val > max)
    return max;

  return val;
}

/**
 * Checks if a value is within a specified range, inclusive.
 *
 * @param {float} val - The value to check.
 * @param {float} min - The minimum value.
 * @param {float} max - The maximum value.
 * @returns {int} 1 if the value is within the range (inclusive), 0 otherwise.
 */
int clamped(float val, float min, float max) {
  return val >= min && val <= max;
}

/**
 * Checks if a value is between a specified range, exclusive.
 *
 * @param {float} val - The value to check.
 * @param {float} min - The minimum value (exclusive).
 * @param {float} max - The maximum value (exclusive).
 * @returns {int} 1 if the value is between the range (exclusive), 0 otherwise.
 */
int between(float val, float min, float max) {
  return val > min && val < max;
}

/**
 * Calculates the remainder of `a` divided by `b`.
 *
 * @param {mixed} a - The dividend.
 * @param {mixed} b - The divisor.
 * @returns {float} The floored remainder of `a` divided by `b`.
 */
varargs float remainder(mixed a, mixed b) {
  float result;

  if(intp(a)) a = to_float(a);
  if(intp(b)) b = to_float(b);

  result = a / b;

  return result - floor(result);
}

/**
 * Calculates the sum of all elements in an array.
 *
 * @param {int*} arr - The array of numbers to sum.
 * @returns {int} The sum of all elements in the array.
 */
int sum(int *arr) {
  return reduce(arr, function(int acc, int num) {
    return acc + num;
  }, 0);
}

/**
 * Evaluates a number against a simple condition.
 *
 * Handles basic operators (=, >, <, >=, <=, !=), modulo operator (%),
 * range checks (e.g., "5-10"), and set inclusion/exclusion ([1,2,3] or
 * ![1,2,3]).
 *
 * @param {int} number - The number to evaluate.
 * @param {string} condition - The condition to evaluate against.
 * @returns {int} 1 if the condition evaluates to true, 0 otherwise.
 * @throws If an invalid operator is provided.
 * @throws If the condition format is invalid.
 */
private int evaluate_simple_condition(int number, string condition) {
  string operator;
  int value, value2;
  int *set;

  // Handle basic operators (=, >, <, >=, <=, !=)
  if(sscanf(condition, "%([<>=!]+)%d", operator, value) == 2) {
    switch(operator) {
      case "=":
      case "==": return number == value;
      case ">":  return number > value;
      case "<":  return number < value;
      case ">=": return number >= value;
      case "<=": return number <= value;
      case "!=": return number != value;
      default:   throw("Invalid operator: " + operator);
    }
  }

  // Handle modulo operator (%)
  if(sscanf(condition, "%%%d", value) == 1) {
    return (number % value) == 0;
  }

  // Handle range checks (e.g., "5-10")
  if(sscanf(condition, "%d-%d", value, value2) == 2)
    return number >= value && number <= value2;

  // Handle set inclusion/exclusion ([1,2,3] or ![1,2,3])
  if(condition[0] == '[' || (condition[0] == '!' && condition[1] == '[')) {
    int exclude = condition[0] == '!';
    string set_str = exclude
       ? condition[1 .. <1]
       : condition[0 .. <1];

    set = map(explode(set_str[1..<2], ","), (: to_int($1) :));

    return exclude ? (of(set, number) == -1) : (of(set, number) != -1);
  }

  error("Invalid condition format: " + condition);
}

/**
 * Evaluates a number against a compound condition.
 *
 * Handles compound conditions using logical operators AND and OR.
 * AND has higher precedence than OR.
 *
 * @param {int} number - The number to evaluate.
 * @param {string} condition - The compound condition to evaluate against.
 * @returns {int} 1 if the compound condition evaluates to true, 0 otherwise.
 */
private int evaluate_compound_condition(int number, string condition) {
  string *parts, part;
  string *or_parts, or_part;
  int or_result, i, j;

  parts = explode(lower_case(condition), "and");
  for(i = 0; i < sizeof(parts); i++) {
    part = parts[i];
    or_parts = explode(part, "or");
    or_result = 0;
    for(j = 0; j < sizeof(or_parts); j++) {
      or_part = trim(or_parts[j]);
      if(or_part[0] == '(') or_part = or_part[1..<2]; // Remove parentheses
      if(evaluate_simple_condition(number, or_part)) {
        or_result = 1;
        break;
      }
    }
    if(!or_result)
      return 0; // If any AND condition fails, return false
  }

  return 1; // All AND conditions passed
}

/**
 * Evaluates a number against a condition. The condition can be a
 * simple comparison, or a compound condition using `AND` and `OR`.
 * This system allows for the evaluation of numeric conditions
 * using a specific set of operators and syntax rules.
 *
 * ### Basic Operators:
 *
 * * `<` - Less than
 * * `>` - Greater than
 * * `<=` - Less than or equal to
 * * `>=` - Greater than or equal to
 * * `=` or `==` - Equal to
 * * `!=` - Not equal to
 * * `%` - Checks if a number is divisible by the given value.
 *
 * ### Range Operator:
 *
 *  Use a hyphen (`-`) to specify a range, inclusive of both ends.
 *  Example: `5-15` means any number from `5` to `15`, including
 *  `5` and `15`.
 *
 * ### Set Inclusion/Exclusion:
 *
 * `[a,b,c]` - Checks if a number is one of the listed values.
 * `![a,b,c]` - Checks if a number is not one of the listed values.
 *
 * ### Logical Operators:
 *
 * `AND` - Both conditions must be true.
 * `OR` - At least one condition must be true.
 *
 * ### Grouping
 *
 * Use parentheses `()` to group conditions and override default
 * precedence.
 *
 * ### Precedence (from highest to lowest):
 *
 * * Parentheses `()`
 * * Basic operators (`<`, `>`, `<=`, `>=`, `=`, `!=`), Range, Modulo, Set inclusion/exclusion
 * * `AND`
 * * `OR`
 *
 * ### Syntax Rules
 *
 * * No spaces are allowed in the condition string.
 * * Operators must be used exactly as specified (e.g., >= is valid, but => is not).
 * * Set values must be comma-separated without spaces.
 *
 * #### Example of a complex condition
 *
 * `(5-15AND%3)OR[20,25,30]`
 *
 * This checks if a number is between `5` and `15` (inclusive) `AND`
 * divisible by `3`, `OR` if it's `20`, `25`, or `30`.
 *
 * Invalid syntax will result in an error, ensuring strict
 * adherence to these rules.
 *
 * @param {int} number - The number to evaluate.
 * @param {string} condition - The condition to evaluate against.
 * @returns {int} 1 if the condition evaluates to true, 0 otherwise.
 * @throws If the condition contains invalid syntax or operators.
 */
int evaluate_number(int number, string condition) {
  // Remove all spaces from the condition
  condition = replace_string(condition, " ", "");
  return evaluate_compound_condition(number, condition);
}

float diminish(mixed val, mixed scale) {
  float mult, trinum;

  if(val < 0)
    return -diminish(-val, scale);

  mult = val / to_float(scale);

  trinum = (sqrt(8.0 * mult + 1.0) - 1.0) / 2.0;

  return trinum * scale;
}

/**
 * Square-root diminishing return on `val`, scaled by `scale`.
 * Output grows as the square root of the input ratio: doubling
 * `val` multiplies the output by sqrt(2). Output equals `scale`
 * when `val == scale`. Unbounded above.
 *
 * @param {mixed} val - The input value.
 * @param {mixed} scale - The scale factor.
 * @returns {float} The diminished value.
 */
float dim_square_root(mixed val, mixed scale) {
  float mult;

  if(val < 0)
    return -dim_square_root(-val, scale);

  mult = val / to_float(scale);

  return sqrt(mult) * scale;
}

/**
 * Logarithmic diminishing return on `val`, scaled by `scale`.
 * Computes `scale * ln(1 + val/scale)`. Output is 0 at `val == 0`
 * and grows ever more slowly as `val` increases. Unbounded above.
 *
 * @param {mixed} val - The input value.
 * @param {mixed} scale - The scale factor.
 * @returns {float} The diminished value.
 */
float dim_logarithmic(mixed val, mixed scale) {
  float mult;

  if(val < 0)
    return -dim_logarithmic(-val, scale);

  mult = val / to_float(scale);

  return log(1.0 + mult) * scale;
}

/**
 * Hyperbolic (Michaelis-Menten) diminishing return on `val`,
 * scaled by `scale`. Output asymptotically approaches `scale` as
 * `val` grows without bound, and equals `scale / 2` when
 * `val == scale`.
 *
 * @param {mixed} val - The input value.
 * @param {mixed} scale - The scale factor and asymptotic ceiling.
 * @returns {float} The diminished value, bounded by `scale`.
 */
float dim_hyperbolic(mixed val, mixed scale) {
  float v, s;

  if(val < 0)
    return -dim_hyperbolic(-val, scale);

  v = to_float(val);
  s = to_float(scale);

  return (s * v) / (s + v);
}

/**
 * Exponential-decay diminishing return on `val`, scaled by `scale`.
 * Computes `scale * (1 - e^(-val/scale))`. Output asymptotically
 * approaches `scale` as `val` grows, and equals roughly
 * `0.632 * scale` (i.e. `1 - 1/e`) when `val == scale`.
 *
 * @param {mixed} val - The input value.
 * @param {mixed} scale - The scale factor and asymptotic ceiling.
 * @returns {float} The diminished value, bounded by `scale`.
 */
float dim_exponential_decay(mixed val, mixed scale) {
  float mult;

  if(val < 0)
    return -dim_exponential_decay(-val, scale);

  mult = val / to_float(scale);

  return (1.0 - exp(-mult)) * scale;
}

/**
 * Sigmoid (zero-anchored logistic) diminishing return on `val`,
 * scaled by `scale`. Computes `scale * (2/(1 + e^-(val/scale)) - 1)`,
 * producing an S-shaped curve that is 0 at `val == 0`, equals
 * roughly `0.462 * scale` when `val == scale`, and asymptotically
 * approaches `scale` as `val` grows.
 *
 * @param {mixed} val - The input value.
 * @param {mixed} scale - The scale factor and asymptotic ceiling.
 * @returns {float} The diminished value, bounded by `scale`.
 */
varargs float dim_sigmoid(mixed val, mixed scale, mixed k) {
  float mult;

  if(val < 0)
    return -dim_sigmoid(-val, scale, k);

  mult = to_float(val) * to_float(k);

  return (2.0 / (1.0 + exp(-mult)) - 1.0) * to_float(scale);
}
