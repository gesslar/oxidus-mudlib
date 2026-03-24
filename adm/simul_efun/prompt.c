#include <simul_efun.h>

private nomask void _password(string input, object body, int attempts, int count, mixed *cb) {
  string current_password;

  if(!input || input == "") {
    _error(body, "Password cannot be empty.");
    if(++count > attempts) {
      call_back(cb, 0);
      return;
    }

    if(attempts > 1)
      _question(body, "Password (attempt %d): ", count);
    else
      _question(body, "Password: ");

    input_to("_password", I_NOECHO | I_NOESC, body, attempts, count, cb);

    return;
  }

  current_password = ACCOUNT_D->readAccount(body->query_real_name(), "password");
  if(crypt(input, current_password) != current_password) {
    _error(body, "Incorrect password.");
    if(++count > attempts) {
      call_back(cb, 0);
      return;
    }

    if(attempts > 1)
      _question(body, "Password (attempt %d): ", count);
    else
      _question(body, "Password: ");

    input_to("_password", I_NOECHO | I_NOESC, body, attempts, count, cb);

    return;
  }

  _ok(body, "Password accepted.");
  call_back(cb, 1);
}

/**
 * Prompt the user for a password.
 *
 * @param {STD_PLAYER} body - The user's body object.
 * @param {int} attempts - The number of attempts allowed.
 * @param {mixed*} cb - The callback array.
 * @example
 * prompt_password(this_body(), 3, assemble_callback(
 *    this_object(), "password_callback", this_body()
 * ));
 */
void prompt_password(object body, int attempts, mixed *cb) {
  _question(body, "Password: ");

  input_to("_password", I_NOECHO | I_NOESC, body, attempts, 1, cb);
}

/**
 * Prompt the user for a colour. The user will be presented with a list of
 * colours to choose from. The user can also enter a number corresponding
 * to a colour, or the word "plain" to select no colour.
 *
 * Upon selection, the callback function will be called with the selected
 * colour as the argument.
 *
 * @param {STD_PLAYER} body - The user's body object.
 * @param {mixed*} cb - The callback array.
 * @param {string} prompt - The prompt message.
 * @example prompt_colour(this_body(), assemble_callback(this_object(), "colour_callback", this_body()), "Select a colour from the following list:");
 */
varargs void prompt_colour(object body, mixed *cb, string prompt) {
  string mess;
  mixed *base = COLOUR_D->base_colours();

  if(nullp(body) || !userp(body))
    error("Bad argument 1 to prompt_colour(): expected body object, got " + typeof(body) + ".");

  if(nullp(cb) || !pointerp(cb))
    error("Bad argument 2 to prompt_colour(): expected callback array, got " + typeof(cb) + ".");

  if(stringp(prompt))
    mess = prompt;
  else
    mess = "Select a colour from the following list:";

  mess = append(mess, "\n");
  mess += "\n";

  mess += reduce(base, function(string acc, string *colours, int index) {
    if(index && !(index % 4))
      acc += "\n";
    if(index && !(index % 8))
      acc += "\n";

    return sprintf("%s%s%-16s{{res}}", acc, colours[0], colours[1]);
  }, "");

  mess += "\n\n";

  mess += "Select one of (colour), (0-255), (c)olour list, (p)lain, or (q)uit.\n"
          "Your choice: ";

  tell(body, mess);

  input_to("_prompt_colour", I_NOESC, mess, body, base, cb);
}

private void _prompt_colour(string input, string prompt, object body, mixed *base, mixed *cb) {
  string *colours;

  if(!input || input == "") {
    _error(body, "Invalid selection.");
    tell(body, prompt);
    input_to("_prompt_colour", I_NOESC, prompt, body, base, cb);
    return;
  }

  if(input == "c") {
    string list = COLOUR_D->get_colour_list();

    prompt = "Select a colour from the following list:\n";
    prompt += "\n";
    prompt += list;
    prompt += "\n";
    prompt += "Select (0-255), (p)lain), or (q)uit.\n";
    prompt += "Your choice: ";

    tell(body, prompt);

    input_to("_prompt_colour", I_NOESC, prompt, body, base, cb);
    return;
  }

  if(input == "p" || input == "plain") {
    call_back(cb, "plain");
    return;
  }

  if(input == "q" || input == "quit") {
    call_back(cb, "cancel");
    return;
  }

  if(input[0] == '#') {
    input = "{{"+input[1..]+"}}";

    if(!colourp(input)) {
      _error(body, "Invalid selection.");
      tell(body, prompt);
      input_to("_prompt_colour", I_NOESC, prompt, body, base, cb);
      return;
    }
  } else {
    string *found;

    found = find(base, (: $1[1] == $(input) :));

    if(found) {
      input = found[0];
    } else {
      _error(body, "Invalid selection.");
      tell(body, prompt);
      input_to("_prompt_colour", I_NOESC, prompt, body, base, cb);
      return;
    }
  }

  call_back(cb, input);
}
