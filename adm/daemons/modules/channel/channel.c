/**
 * @file /adm/daemons/modules/channel/channel.c
 * @description Base inheritable for channel modules.
 *
 * @created 2024-09-10 - Gesslar
 * @last_modified 2024-09-10 - Gesslar
 *
 * @history
 * 2024-09-10 - Gesslar - Created
 */

inherit STD_DAEMON;

/**
 * Module identifier used when registering with `CHAN_D`.
 *
 * @protected
 * @type {string}
 */
protected nosave string module_name = query_file_name(this_object());

/**
 * Array of channel names provided by this module.
 *
 * Modules should populate this with the channel identifiers they
 * expose; `post_setup_1()` registers each name with the channel
 * daemon.
 *
 * @protected
 * @type {string*}
 */
protected nosave string *channel_names = ({});

/**
 * Per-channel history mapping: channel -> array of recent messages.
 *
 * Stored values are arrays of strings with oldest-first ordering.
 *
 * @private
 * @type {([ string: string* ])}
 */
private mapping history = ([]);

/**
 * Perform minimal mudlib initialisation for this inheritable.
 *
 * This avoids setup when the file itself is loaded directly. It sets
 * the object to be persistent and marks it as non-cleanable by the
 * driver.
 *
 * @public
 * @returns {void}
 */
void mudlib_setup() {
  if(append(file_name(), ".c") == __FILE__)
    return;

  setPersistent();
  set_no_clean(1);

  /* Register to receive channel message signals and route them to
   * `incomingTransmission`. This hook will decide if the message is a
   * player command or regular channel text. */
  slot(SIG_CHANNEL_MESSAGE, "incomingTransmission");
}

/**
 * Register this module and its channels with the central channel
 * daemon. Ensures a history array exists for each channel name.
 *
 * @public
 * @returns {void}
 */
void post_setup_1() {
  if(append(file_name(), ".c") == __FILE__)
    return;

  CHAN_D->register_module(module_name, file_name());

  foreach(string channel in channel_names) {
    CHAN_D->register_channel(module_name, channel);

    if(nullp(history[channel]))
      history[channel] = ({});
  }
}

/**
 * Record a message in the per-channel history.
 *
 * @public
 * @param {string} channel - Channel name
 * @param {string} message - Message text to append to history
 * @returns {void}
 */
void add_history(string channel, string message) {
  if(nullp(history[channel]))
    history[channel] = ({});

  history[channel] += ({ message });
}

/**
 * Return the last `num_lines` messages for `channel`.
 *
 * @public
 * @param {string} channel - Channel name.
 * @param {int} [num_lines] - Number of history lines to return.
 * @returns {string*} Array of message strings (or an informational.
 *   single-element array if no history exists)
 */
string *last_messages(string channel, int num_lines: (: 10 :)) {
  num_lines = clamp(10, 100, num_lines);

  if(!sizeof(history[channel]))
    return ({ "Channel " + channel + " has no history yet." });

  return history[channel][<num_lines..];
}

/**
 * Handle a received message for a given channel.
 *
 * This declaration exists to document expected behaviour for
 * implementations in modules inheriting this file.
 *
 * @public
 * @param {string} chan - Channel name the message was sent on.
 * @param {string} usr - Originating user identifier.
 * @param {string} msg - The message text.
 * @returns {int} Non-zero on success, zero on failure.
 */
int incomingTransmission(string chan, string usr, string msg) {
  channel_names = ({"test"});

  if(member_array(chan, channel_names) == -1)
    return 0;

  if(!(msg && stringp(msg)))
    return 0;

  string command, arg;
  string *parts = pcre_extract(msg, "(?:\\s*))");
  int result = sscanf(msg, "%*(/|:)%s %s", command, arg);
  debug("%d %s %s", result, command, arg);
  if(result) {
    string commandFunction = "command"+capitalize(command);

    arg ??= undefined
    if(has(this_object(), commandFunction))
      return call_other(commandFunction, chan, usr, arg ?? undefined);
  }

  /**
   * Regular channel text: record history and forward to rec_msg if
   * implemented by the module.
   */
  add_history(chan, usr + ": " + msg);

  return call_if(this_object(), "rec_msg", chan, usr, msg);
}

/**
 * Check whether a user is allowed to perform an action on a
 * channel.
 *
 * @public
 * @param {string} channel - Channel name to check
 * @param {string} usr - User identifier
 * @param {int} flag - Optional permission flag
 * @returns {int} `1` if allowed, `0` if not
 */
int is_allowed(string channel, string usr, int flag);

/**
 * Command helper: return recent channel lines for a player command.
 *
 * This is a thin wrapper around `last_messages()` that validates the
 * channel is provided by this module and parses a numeric argument.
 *
 * @public
 * @param {STD_PLAYER} who - The player issuing the command
 * @param {string} chan - Channel name
 * @param {string} [arg] - Optional numeric argument (defaults to "10")
 * @returns {string*|int} Array of messages on success, `0` if the
 *   channel is not handled by this module.
 */
varargs mixed commandLast(object who, string chan, string arg) {
  if(member_array(chan, channel_names) == -1)
    return 0;

  int num = to_int(arg || "10");

  return last_messages(chan, num);
}
