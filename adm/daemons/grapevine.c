/**
 * @file /adm/daemons/grapevine.c
 * @description Grapevine WebSockets client
 *
 * @created 2024-07-17 - Gesslar
 * @last_modified 2024-07-17 - Gesslar
 *
 * @history
 * 2024-07-17 - Gesslar - Created
 */

#include <grapevine.h>
#include <localtime.h>

inherit STD_WS_CLIENT;

private nomask void startup();
private nomask void restart();
private nomask mapping connect(mapping grapevine);
protected nomask varargs string zTimeString(int time);

private nomask varargs void sendOutgoingMessage(string ev, string reff, mapping message, mixed *cb);
private nomask void grapevineHandleEventRestart(string reff, mapping data);

// Send events
private nomask void grapevineSendEventAuthenticate(mapping auth);
private nomask void grapevineSendEventHeartbeat();

private nosave mapping __config, __restart, __reffs;
private nosave mapping __games;

protected void setup() {
  set_log_level(1);
  set_log_prefix("(GRAPEVINE)");

  slot(SIG_USER_LOGIN, "grapevineSendEventPlayersSignIn");
  slot(SIG_USER_LINK_RESTORE, "grapevineSendEventPlayersSignIn");
  slot(SIG_USER_LOGOUT, "grapevineSendEventPlayersSignOut");
  slot(SIG_USER_LINKDEAD, "grapevineSendEventPlayersSignOut");

  call_out("startup", 3);
}

// Start the Connection
private nomask void startup() {
  if(server) {
    _log(2, "Already connected to a server");
    return;
  }

  if(__restart) {
    _log(2, "Restart in progress");
    return;
  }

  __config = mudConfig("GRAPEVINE");

  mapping result = connect(__config);

  _log(3, "Result: %O", result);

  switch(result["status"]) {
    case GR_STATUS_OK:
      call_if(this_object(), "grapevineHandleConnecting", result);
      break;

    case GR_STATUS_FAIL:
      call_if(this_object(), "grapevineHandleError", result);
      break;

    default:
      _log(2, "Unknown status: %s", result["status"]);
      break;
  }
}

private nomask void restart() {
  if(server) {
    _log(2, "Already connected to a server");
    return;
  }

  if(!__restart) {
    _log(2, "No restart in progress");
    return;
  }

  if(!__restart["restarting"]) {
    _log(2, "No restart in progress");
    return;
  }

  if(find_call_out(__restart["restarting"]) != -1) {
    _log(2, "Restart already in progress");
    return;
  }

  _log(1, "Restarting Grapevine connection");

  mapping result = connect(__config);

  switch(result["status"]) {
    case GR_STATUS_OK:
      __restart["attempts"]++;
      call_if(this_object(), "grapevineHandleConnecting", result);
      break;

    case GR_STATUS_ERROR:
      __restart = null;
      call_if(this_object(), "grapevineHandleError", result);
      break;

    default:
      __restart = null;
      _log(2, "Unknown status: %s", result["status"]);
      break;
  }
}

private void restartAttempt() {
  if(__restart) {
    if(__restart["attempts"]++ > __config["max_restart"]) {
      _log(1, "Max restart attempts reached");
      __restart = null;

      return;
    }

    _log(1, "Restarting Grapevine connection");
    __restart["restarting"] = call_out((:restart:), __config["restart"] + 60);

    return;
  }
}

private nomask mapping connect(mapping grapevine) {
  if(server) {
    _log(2, "Already connected to a server");

    return;
  }

  if(__restart) {
    _log(2, "Restart in progress");

    return;
  }

  if(!grapevine) {
    _log(2, "No Grapevine configuration");

    return;
  }

  if(!grapevine["host"]) {
    _log(2, "No Grapevine host");

    return;
  }

  __reffs = ([]);

  return websocket_connect(grapevine["host"]);
}

/* ************************************************************
 * WebSocket Event Handlers
 * ************************************************************ */

protected void websocket_handle_connected() {
  mapping auth = ([
    "client_id" : __config["client_id"],
    "client_secret" : __config["client_secret"],
    "supports" : __config["supports"],
    "channels" : __config["channels"],
    "version" : __config["version"],
    "user_agent" : get_config(__MUD_NAME__),
  ]);

  if(__restart) {
    _log(1, "Connection re-established");
    __restart = null;
  }

  grapevineSendEventAuthenticate(auth);

  server["grapevine"] = ([]);
}

protected void websocket_handle_connection_error(int result) {
  _log(2, "Connection error: %O", result);

  if(__restart)
    return restartAttempt();
}

protected void websocket_handle_resolve_error() {
  _log(2, "Failed to resolve host");

  if(__restart)
    return restartAttempt();
}

protected void websocket_handle_handshake_error(int result) {
  _log(2, "Handshake error: %O", result);

  if(__restart)
    return restartAttempt();
}

// Handle eventuality that the connection has been closed
protected void websocket_handle_shutdown() {
  _log(1, "Grapevine connection closed");

  if(__restart)
    return restartAttempt();
}

// Handle incoming text frames
protected void websocket_handle_text_frame(mapping payload) {
  if(!payload) {
    _log(2, "No frame info");

    return;
  }

  if(!server["grapevine"])
    return;

  string status = payload["status"];
  string event = payload["event"];
  mixed data = payload["payload"];
  string reff = payload["ref"];
  string err = payload["error"];

  _log(3, "Received status: %s", status);
  _log(3, "Received event: %s", event);
  _log(3, "Received ref: %O", reff);
  _log(3, "Payload: %O", data);

  if(err)
    _log(3, "Error: %s", err);

  switch(event) {
    // Authenticate
    case GR_EVENT_AUTHENTICATE:
      _log(2, "Received authenticate event");
      call_if(this_object(), "grapevineHandleEventAuthenticate", status, err, data);
      break;

    // Heartbeat
    case GR_EVENT_HEARTBEAT:
      _log(2, "Received heartbeat event");
      call_if(this_object(), "grapevineHandleEventHeartbeat", data);
      break;

    // Restart
    case GR_EVENT_RESTART:
      _log(2, "Received restart event");
      call_if(this_object(), "grapevineHandleEventRestart", reff, data);
      break;

    // Channels
    case GR_EVENT_CHANNELS_SUBSCRIBE:
      _log(2, "Received subscribe event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleChannelsSubscribe", reff, status, err);
      break;

    case GR_EVENT_CHANNELS_UNSUBSCRIBE:
      _log(2, "Received unsubscribe event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleChannelsUnsubscribe", reff);
      break;

    case GR_EVENT_CHANNELS_BROADCAST:
      _log(2, "Received broadcast event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleChannelsBroadcast", reff, data);
      break;

    case GR_EVENT_CHANNELS_SEND:
      _log(2, "Received send event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleChannelsSend", reff);
      break;

    // Players
    case GR_EVENT_PLAYERS_SIGN_IN:
      _log(2, "Received sign-in event, reference: %s", reff);
      call_if(this_object(), "grapevineHandlePlayersSignIn", reff, data);
      break;

    case GR_EVENT_PLAYERS_SIGN_OUT:
      _log(2, "Received sign-out event, reference: %s", reff);
      call_if(this_object(), "grapevineHandlePlayersSignOut", reff, data);
      break;

    case GR_EVENT_PLAYERS_STATUS:
      _log(2, "Received status event, reference: %s", reff);
      call_if(this_object(), "grapevineHandlePlayersStatus", reff, status, err, data);
      break;

    // Tells
    case GR_EVENT_TELLS_SEND:
      _log(2, "Received send event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleTellsSend", reff, status, err);
      break;

    case GR_EVENT_TELLS_RECEIVE:
      _log(1, "Received receive event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleTellsReceive", reff, data);
      break;

    // Game
    case GR_EVENT_GAMES_CONNECT:
      _log(2, "Received connect event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleGamesConnect", reff, data);
      break;

    case GR_EVENT_GAMES_DISCONNECT:
      _log(2, "Received disconnect event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleGamesDisconnect", reff, data);
      break;

    case GR_EVENT_GAMES_STATUS:
      _log(2, "Received status event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleGamesStatus", reff, status, err, data);
      break;

    case GR_EVENT_ACHIEVEMENTS:
      _log(2, "Received sync event, reference: %s", reff);
      call_if(this_object(),
        "grapevineHandleAchievementsSync", reff, data);
      break;

    case GR_EVENT_ACHIEVEMENTS_CREATE:
      _log(2, "Received create event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleAchievementsCreate", reff, status, data);
      break;

    case GR_EVENT_ACHIEVEMENTS_UPDATE:
      _log(2, "Received update event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleAchievementsUpdate", reff, status, data);
      break;

    case GR_EVENT_ACHIEVEMENTS_DELETE:
      _log(2, "Received delete event, reference: %s", reff);
      call_if(this_object(), "grapevineHandleAchievementsDelete", reff, data);
      break;

    default:
      _log(2, "Unknown event: %s", event);
      break;
  }
}

protected void websocket_handle_close_frame(mapping _payload) {
  // If we are closing and have received a restart message,
  // restart the connection after the specified time. Adding
  // some seconds to ensure the server is ready to accept.
  if(server["grapevine"]["restart"]) {
    _log(1,
      "Grapevine connection closed, restarting in %d seconds",
      server["grapevine"]["restart"]
    );

    __restart = ([
      "restart" : server["grapevine"]["restart"],
      "attempts" : 0,
      "restarting" : call_out_walltime((:restartAttempt:), server["grapevine"]["restart"] + 60),
    ]);
  }
}

/* ************************************************************
 * Grapevine Event Handlers
 * ************************************************************ */

// Authentication

private void grapevineHandleEventAuthenticate(
    string status, string err, mapping data) {
  _log(2, "Received authentication response: %s", identify(data));

  if(status == GR_STATUS_OK)
    _log(2, "Authenticated with Grapevine");

  else
    _log(1, "Failed to authenticate with Grapevine, error: %s", err);
}

// Heartbeat

private void grapevineHandleEventHeartbeat(mapping _data) {
  _log(2, "Received heartbeat");
  grapevineSendEventHeartbeat();
}

// Restart

private nomask void grapevineHandleEventRestart(string _reff, mapping data) {
  _log(1, "Grapevine restart imminent, duration: %d", data["downtime"]);
  server["grapevine"]["restart"] = data["downtime"];
}

// Channels
private void grapevineHandleChannelsSubscribe(string reff, string _status, string err) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  if(err) {
    _log(1, "Failed to subscribe to channel `%s`: %s", request["channel"], err);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Subscribed to channel: %s", request["channel"]);
}

// TODO: We need to ask them if we could receive a status and error message for this event.
private void grapevineHandleChannelsUnsubscribe(string reff) {
  mapping request = __reffs[reff];

  map_delete(__reffs, reff);

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Unsubscribed from channel: %s", request["channel"]);
}

private void grapevineHandleChannelsBroadcast(string _reff, mapping data) {
  _log(1, "Received broadcast on channel: %s", data["channel"]);
  // catch(CHAN_D->grapevine_chat(data));
}

// TODO: We need to ask them if we could receive a status and
// error message for this event.
private void grapevineHandleChannelsSend(string reff) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Received message confirmation on channel: %s", request["channel"]);
}

// Players

// TODO: We need to ask them if we could receive a status and
// error message for this event.
private void grapevineHandlePlayersSignIn(string reff, mapping data) {
  // We have a reff, so this is a response to our own player
  // signing in
  if(reff) {
    mapping request = __reffs[reff];

    if(!request)
      return;

    map_delete(__reffs, reff);

    mixed cerr;

    if(request["callback"])
      cerr = catch(call_back(request["callback"], request));

    if(cerr)
      _log(1, "Error in callback: %s", cerr);

    _log(1, "Received sign-in confirmation: %s", request["name"]);

    return;
  }

  // catch(CHAN_D->grapevine_chat(([
  //   "channel": mudConfig("GRAPEVINE")["notice"],
  //   "message": sprintf("%s has signed in to %s", data["name"], data["game"]),
  //   "name": "Grapevine",
  //   "game": mud_name(),
  // ])));

  _log(1, "Player signed in: %s@%s", data["name"], data["game"]);
}

// TODO: We need to ask them if we could receive a status and
// error message for this event.
private void grapevineHandlePlayersSignOut(string reff, mapping data) {
  if(reff) {
    mapping request = __reffs[reff];

    if(!request)
      return;

    map_delete(__reffs, reff);

    mixed cerr;

    if(request["callback"])
      cerr = catch(call_back(request["callback"], request));

    if(cerr)
      _log(1, "Error in callback: %s", cerr);

    _log(1, "Received sign-out confirmation: %s", request["name"]);

    return;
  }

  // catch(CHAN_D->grapevine_chat(([
  //   "channel": mudConfig("GRAPEVINE")["notice"],
  //   "message": sprintf("%s has signed out of %s", data["name"], data["game"]),
  //   "name": "Grapevine",
  //   "game": mud_name(),
  // ])));

  _log(1, "Player signed out: %s@%s", data["name"], data["game"]);
}

// TODO: We need to ask them if we could receive a status and
// error message for this event.
private void grapevineHandlePlayersStatus(string reff, string _status, string err, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  _log(1, "Callback %O\n", request["callback"]);

  if(err) {
    mixed cerr;

    if(request["callback"])
      cerr = catch(call_back(request["callback"], request, err));

    if(cerr)
      _log(1, "Error in callback: %s", cerr);

    _log(1, "Failed to get player status for game `%s`: %s", request["game"], err);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Received player status for game %s: %O", data["game"], data["players"]);
}

// Tells

private void grapevineHandleTellsSend(string reff, string _status, string err) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  if(err) {
    _log(1, "Failed to send tell to %s: %s", request["player"], err);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Sent tell to %s", request["player"]);
}

// TODO: We need to ask them what the significance is of the
// ref in this event.
private void grapevineHandleTellsReceive(string _reff, mapping data) {
  _log(1, "Received tell: %s", identify(data));
}

// Game
private void grapevineHandleGamesConnect(string _reff, mapping data) {
  // catch(CHAN_D->grapevine_chat(([
  //   "channel": mudConfig("GRAPEVINE")["notice"],
  //   "message": sprintf("%s has connected to Grapevine", data["game"]),
  //   "name": "Grapevine",
  //   "game": mud_name(),
  // ])));

  _log(1, "Game connected: %s", data["game"]);
}

private void grapevineHandleGamesDisconnect(string _reff, mapping data) {
  // catch(CHAN_D->grapevine_chat(([
  //   "channel": mudConfig("GRAPEVINE")["notice"],
  //   "message": sprintf("%s has disconnected from Grapevine", data["game"]),
  //   "name": "Grapevine",
  //   "game": mud_name(),
  // ])));

  _log(1, "Game disconnected: %s", data["game"]);
}

// TODO: Ask if it's possible to receive information about how
// many responses one will get, so we know when the request has
// completed fully.
// TODO: When doing a single request, the game appears to be
// case sensitive, despite tells/send not being case sensitive.
private void grapevineHandleGamesStatus(string reff, string _status, string err, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  if(err) {
    _log(1, "Failed to get game status for `%s`: %s", request["game"], err);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Game status: %s", data["game"]);
}

// Achievements

// TODO: We need to ask if we could receive a status and error
// message for this event.
private void grapevineHandleAchievementsSync(string reff, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Achievements synced: %s", identify(data));
}

private void grapevineHandleAchievementsCreate(string reff, string status, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  if(status == GR_STATUS_FAIL) {
    _log(1, "Failed to create achievement: %O\nErrors: %O", request, data["errors"]);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Achievement created: %s", identify(data));
}

private void grapevineHandleAchievementsUpdate(string reff, string status, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  if(status == GR_STATUS_FAIL) {
    _log(1, "Failed to update achievement: %O\nErrors: %O", request, data["errors"]);

    return;
  }

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Achievement updated: %s", identify(data));
}

// TODO: We need to ask them if we could receive a status and
// error message for this event.
private void grapevineHandleAchievementsDelete(string reff, mapping data) {
  mapping request = __reffs[reff];

  if(!request)
    return;

  map_delete(__reffs, reff);

  mixed cerr;

  if(request["callback"])
    cerr = catch(call_back(request["callback"], request, data));

  if(cerr)
    _log(1, "Error in callback: %s", cerr);

  _log(1, "Achievement deleted: %s", identify(data));
}

// Unknown
private void grapevineHandleUnknownChannels(string _reff, mapping data) {
  _log(1, "Unknown channels event: %s", identify(data));
}

private void grapevineHandleUnknownPlayers(string _reff, mapping data) {
  _log(1, "Unknown players event: %s", identify(data));
}

private void grapevineHandleUnknownGame(string _reff, mapping data) {
  _log(1, "Unknown game event: %s", identify(data));
}

private void grapevineHandleUnknownTells(string _reff, mapping data) {
  _log(1, "Unknown tells event: %s", identify(data));
}

private void grapevineHandleUnknownAchievements(string _reff, mapping data) {
  _log(1, "Unknown achievements event: %s", identify(data));
}

/* ************************************************************
 * Grapevine Sending Functions
 * ************************************************************ */

// Authenticate
private nomask void grapevineSendEventAuthenticate(mapping auth) {
  _log(2, "Sending authentication");
  sendOutgoingMessage(GR_EVENT_AUTHENTICATE, null, auth);
}

// Heartbeat
private nomask void grapevineSendEventHeartbeat() {
  object *u = users();

  _log(2, "Sending heartbeat");

  u = filter(u, (:interactive($1) && objectp(environment($1)):));

  mapping pl = ([
    "game": get_config(__MUD_NAME__),
    "players": map(u, (: $1->query_real_name() :)),
  ]);

  _log(2, "Sending heartbeat");

  sendOutgoingMessage(GR_EVENT_HEARTBEAT, null, pl);
}

// Channels

/**
 * Sends a request to subscribe to a Grapevine channel.
 *
 * @param {string} chan - The channel name to subscribe to.
 * @param {mixed*} cb - Callback to invoke when the subscription
 *                      is confirmed.
 */
public nomask void grapevineSendEventChannelsSubscribe(string chan, mixed *cb) {
  mapping pl = ([
    "channel": chan,
  ]);

  sendOutgoingMessage(GR_EVENT_CHANNELS_SUBSCRIBE,generate_uuid(), pl, cb);
}

/**
 * Sends a request to unsubscribe from a Grapevine channel.
 *
 * @param {string} chan - The channel name to unsubscribe from.
 * @param {mixed*} cb - Callback to invoke when the unsubscription
 *                      is confirmed.
 */
public nomask void grapevineSendEventChannelsUnsubscribe(string chan, mixed *cb) {
  mapping pl = ([
    "channel": chan,
  ]);

  sendOutgoingMessage(GR_EVENT_CHANNELS_UNSUBSCRIBE, generate_uuid(), pl, cb);
}

/**
 * Sends a chat message to a Grapevine channel on behalf of a
 * player.
 *
 * @param {STD_PLAYER} who - The player sending the message.
 * @param {string} chan - The channel to send to.
 * @param {string} msg - The message content.
 * @param {mixed*} cb - Callback to invoke when the send is
 *                      confirmed.
 */
public nomask void grapevineSendEventChannelsSend(object who, string chan, string msg, mixed *cb) {
  mapping pl = ([
    "channel": chan,
    "message": msg,
    "name": who->query_real_name(),
  ]);

  sendOutgoingMessage(GR_EVENT_CHANNELS_SEND, generate_uuid(), pl, cb);
}

// Players

/**
 * Notifies Grapevine that a player has signed in.
 *
 * @param {STD_PLAYER} who - The player who signed in.
 */
public nomask void grapevineSendEventPlayersSignIn(object who) {
  mapping pl = ([
    "name": who->query_real_name(),
    "game": get_config(__MUD_NAME__),
  ]);

  sendOutgoingMessage(GR_EVENT_PLAYERS_SIGN_IN, generate_uuid(), pl);
}

// TODO: Ask why this event does not take a game, but sign-in does
/**
 * Notifies Grapevine that a player has signed out.
 *
 * @param {STD_PLAYER} who - The player who signed out.
 */
public nomask void grapevineSendEventPlayersSignOut(object who) {
  mapping pl = ([
    "name" : who->query_real_name(),
  ]);

  sendOutgoingMessage(GR_EVENT_PLAYERS_SIGN_OUT, generate_uuid(), pl);
}

// TODO: Ask if this can be made case-insensitive like tells/send
/**
 * Requests the player status for a specific game or all games
 * on the Grapevine network.
 *
 * @param {string} game - The game name to query, or 0 for all
 *                        games.
 * @param {mixed*} cb - Callback to invoke with the status
 *                      response.
 */
public nomask void grapevineSendEventPlayersStatus(string game, mixed *cb) {
  mapping pl;

  if(game)
    pl = ([ "game" : game ]);

  sendOutgoingMessage(GR_EVENT_PLAYERS_STATUS, generate_uuid(), pl, cb);
}

// Tells

/**
 * Sends a private tell to a player on another game via
 * Grapevine.
 *
 * @param {STD_PLAYER} who - The player sending the tell.
 * @param {string} to - The recipient's name.
 * @param {string} game - The recipient's game name.
 * @param {string} msg - The message content.
 * @param {mixed*} cb - Callback to invoke when the send is
 *                      confirmed.
 */
public nomask void grapevineSendEventTellsSend(object who, string to, string game, string msg, mixed *cb) {
  mapping pl = ([
    "from_name": who->query_real_name(),
    "to_name": to,
    "to_game": game,
    "message": msg,
    "sent_at": zTimeString(),
  ]);

  sendOutgoingMessage(GR_EVENT_TELLS_SEND, generate_uuid(), pl, cb);
}

private nomask varargs void sendOutgoingMessage(string ev, string reff, mapping message, mixed *cb) {
  if(!server) {
    _log(1, "Not connected to Grapevine");

    return;
  }

  mapping outgoing = ([ "event" : ev ]);

  if(!nullp(message))
    outgoing["payload"] = message;

  if(!nullp(reff))
    outgoing["ref"] = reff;

  if(reff) {
    __reffs[reff] = copy(message || ([])) +
      ([ "event" : ev, "callback" : cb ]);
  }

  _log(4, "Sending message: %O", outgoing);

  websocket_message(WS_TEXT_FRAME, json_encode(outgoing));
}

// Games

/**
 * Requests the status of a specific game or all games on the
 * Grapevine network.
 *
 * @param {mixed} game - The game name as a string, or a
 *                       non-string value for all games.
 * @param {mixed*} cb - Callback to invoke with the status
 *                      response.
 */
public nomask void grapevineSendEventGamesStatus(mixed game, mixed *cb) {
  mapping pl;

  if(stringp(game))
    pl = ([ "game" : game ]);

  else
    pl = null;

  sendOutgoingMessage(GR_EVENT_GAMES_STATUS, generate_uuid(), pl, cb);
}

// Achievements

// TODO: Ask if these persist across connections. I think it
// does, because otherwise why would we have a sync event?
// If so, then we need to manage keys.

/**
 * Requests a sync of all achievements from the Grapevine
 * server.
 */
public nomask void grapevineSendEventAchievementsSync() {
  sendOutgoingMessage(GR_EVENT_ACHIEVEMENTS_SYNC, generate_uuid(), null);
}

/**
 * Sends a request to create a new achievement on Grapevine.
 *
 * @param {mapping} achievement - The achievement data to create.
 */
public nomask void grapevineSendEventAchievementsCreate(mapping achievement) {
  sendOutgoingMessage(GR_EVENT_ACHIEVEMENTS_CREATE, generate_uuid(), achievement);
}

/**
 * Sends a request to update an existing achievement on
 * Grapevine.
 *
 * @param {mapping} achievement - The updated achievement data.
 */
public nomask void grapevineSendEventAchievementsUpdate( mapping achievement) {
  sendOutgoingMessage(GR_EVENT_ACHIEVEMENTS_UPDATE, generate_uuid(), achievement);
}

/**
 * Sends a request to delete an achievement from Grapevine.
 *
 * @param {string} key - The key of the achievement to delete.
 */
public nomask void grapevineSendEventAchievementsDelete(string key) {
  mapping pl = ([
    "key": key,
  ]);

  sendOutgoingMessage(GR_EVENT_ACHIEVEMENTS_DELETE, generate_uuid(), pl);
}

/**
 * Broadcasts a message to a Grapevine channel with an
 * arbitrary sender name.
 *
 * @param {string} chan - The channel to broadcast to.
 * @param {string} usr - The sender's display name.
 * @param {string} msg - The message content.
 */
public nomask void grapevineBroadcastMessage(string chan, string usr, string msg) {
  mapping message = ([
    "channel": chan,
    "message": msg,
    "name": usr,
    "game": get_config(__MUD_NAME__),
  ]);

  sendOutgoingMessage(GR_EVENT_CHANNELS_SEND, generate_uuid(), message);
}

/**
 * Generates an ISO 8601 UTC timestamp string.
 *
 * @param {int} [time] - The epoch time to format. Defaults to
 *                       the current time.
 * @returns {string} The formatted timestamp in
 *                   YYYY-MM-DDTHH:MM:SSZ format.
 */
protected nomask varargs string zTimeString(int time: (: time() :)) {
  mixed *lt = localtime(time);
  int off = lt[LT_GMTOFF];

  time += off + (-1 * lt[LT_ISDST]) * 3600;

  return strftime("%Y-%m-%dT%H:%M:%SZ", time);
}

// This event will run when this object is being destructed,
// so we should let the server know that we are going away.
protected void unsetup() {
  _log(1, "Grapevine destructing.");

  if(server) {
    _log(1, "Closing Grapevine connection.");

    websocket_close(WS_CLOSE_GOING_AWAY);
  }
}
