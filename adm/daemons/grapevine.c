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
protected nomask varargs string z_time_string(int time);

private nomask varargs void send_outgoing_message(string ev, string reff, mapping message);
private nomask void grapevine_handle_event_restart(string reff, mapping data);

// Send events
private nomask void grapevine_send_event_authenticate(mapping auth);
private nomask void grapevine_send_event_heartbeat();

private nosave mapping config, restart, reffs;
private nosave mapping games;

void setup() {
    set_log_level(1);
    set_log_prefix("(GRAPEVINE)");

    slot(SIG_USER_LOGIN, "grapevine_send_event_players_sign_in");
    slot(SIG_USER_LINK_RESTORE, "grapevine_send_event_players_sign_in");
    slot(SIG_USER_LOGOUT, "grapevine_send_event_players_sign_out");
    slot(SIG_USER_LINKDEAD, "grapevine_send_event_players_sign_out");

    call_out("startup", 3);
}

// Start the Connection
private nomask void startup() {
    mapping result;

    if(server) {
        _log(2, "Already connected to a server");
        return;
    }

    if(restart) {
        _log(2, "Restart in progress");
        return;
    }

    config = mudConfig("GRAPEVINE");

    result = connect(config);

    _log(3, "Result: %O", result);

    switch(result["status"]) {
        case GR_STATUS_OK :
            call_if(this_object(), "grapevine_handle_connecting", result);
            break;
        case GR_STATUS_FAIL :
            call_if(this_object(), "grapevine_handle_error", result);
            break;
        default:
            _log(2, "Unknown status: %s", result["status"]);
            break;
    }
}

private nomask void restart() {
    mapping result;

    if(server) {
        _log(2, "Already connected to a server");
        return;
    }

    if(!restart) {
        _log(2, "No restart in progress");
        return;
    }

    if(!restart["restarting"]) {
        _log(2, "No restart in progress");
        return;
    }

    if(find_call_out(restart["restarting"]) != -1) {
        _log(2, "Restart already in progress");
        return;
    }

    _log(1, "Restarting Grapevine connection");

    result = connect(config);

    switch(result["status"]) {
        case GR_STATUS_OK :
            restart["attempts"]++;
            call_if(this_object(), "grapevine_handle_connecting", result);
            break;
        case GR_STATUS_ERROR :
            restart = null;
            call_if(this_object(), "grapevine_handle_error", result);
            break;
        default:
            restart = null;
            _log(2, "Unknown status: %s", result["status"]);
            break;
    }
}

void restart_attempt() {
    if(restart) {
        if(restart["attempts"]++ > config["max_restart"]) {
            _log(1, "Max restart attempts reached");
            restart = null;
            return;
        }

        _log(1, "Restarting Grapevine connection");
        restart["restarting"] = call_out((:restart:), config["restart"] + 60);
        return;
    }

}

private nomask mapping connect(mapping grapevine) {
    if(server) {
        _log(2, "Already connected to a server");
        return;
    }

    if(restart) {
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

    reffs = ([ ]);

    return websocket_connect(grapevine["host"]);
}

/* ************************************************************************* */
/* WebSocket Event Handlers                                                  */
/* ************************************************************************* */

void websocket_handle_connected() {
    mapping grapevine = ([]);
    mapping auth = ([
        "client_id" : config["client_id"],
        "client_secret" : config["client_secret"],
        "supports" : config["supports"],
        "channels" : config["channels"],
        "version" : config["version"],
        "user_agent" : get_config(__MUD_NAME__)
    ]);

    if(restart) {
        _log(1, "Connection re-established");
        restart = null;
    }

    grapevine_send_event_authenticate(auth);

    server["grapevine"] = grapevine;
}

void websocket_handle_connection_error(int result) {
    _log(2, "Connection error: %O", result);

    if(restart)
        return restart_attempt();
}

void websocket_handle_resolve_error() {
    _log(2, "Failed to resolve host");

    if(restart)
        return restart_attempt();
}

void websocket_handle_handshake_error(int result) {
    _log(2, "Handshake error: %O", result);

    if(restart)
        return restart_attempt();
}

// Handle eventuality that the connection has been closed
void websocket_handle_shutdown() {
    _log(1, "Grapevine connection closed");

    if(restart)
        return restart_attempt();
}

// Handle incoming text frames
void websocket_handle_text_frame(mapping payload) {
    string event, status, reff, err;
    mixed data;

    if(!payload) {
        _log(2, "No frame info");
        return;
    }

    if(!server["grapevine"])
        return;

    status = payload["status"];
    event = payload["event"];
    data = payload["payload"];
    reff = payload["ref"];
    err = payload["error"];

    _log(3, "Received status: %s", status);
    _log(3, "Received event: %s", event);
    _log(3, "Received ref: %O", reff);
    _log(3, "Payload: %O", data);
    if(err)
        _log(3, "Error: %s", err);

    switch(event) {
        // Authentice
        case GR_EVENT_AUTHENTICATE:
            _log(2, "Received authenticate event");
            call_if(this_object(), "grapevine_handle_event_authenticate", status, err, data);
            break;
        // Heartbeat
        case GR_EVENT_HEARTBEAT:
            _log(2, "Received heartbeat event");
            call_if(this_object(), "grapevine_handle_event_heartbeat", data);
            break;
        // Restart
        case GR_EVENT_RESTART:
            _log(2, "Received restart event");
            call_if(this_object(), "grapevine_handle_event_restart", reff, data);
            break;
        // Channels
        case GR_EVENT_CHANNELS_SUBSCRIBE:
            _log(2, "Received subscribe event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_channels_subscribe", reff, status, err);
            break;
        case GR_EVENT_CHANNELS_UNSUBSCRIBE :
            _log(2, "Received unsubscribe event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_channels_unsubscribe", reff);
        case GR_EVENT_CHANNELS_BROADCAST :
            _log(2, "Received broadcast event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_channels_broadcast", reff, data);
            break;
        case GR_EVENT_CHANNELS_SEND :
            _log(2, "Received send event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_channels_send", reff);
            break;
        // Players
        case GR_EVENT_PLAYERS_SIGN_IN:
            _log(2, "Received sign-in event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_players_sign_in", reff, data);
            break;
        case GR_EVENT_PLAYERS_SIGN_OUT:
            _log(2, "Received sign-out event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_players_sign_out", reff, data);
            break;
        case GR_EVENT_PLAYERS_STATUS:
            _log(2, "Received status event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_players_status", reff, status, err, data);
            break;
        // Tells
        case GR_EVENT_TELLS_SEND:
            _log(2, "Received send event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_tells_send", reff, status, err);
            break;
        case GR_EVENT_TELLS_RECEIVE:
            _log(1, "Received receive event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_tells_receive", reff, data);
            break;
        // Game
        case GR_EVENT_GAMES_CONNECT:
            _log(2, "Received connect event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_games_connect", reff, data);
            break;
        case GR_EVENT_GAMES_DISCONNECT:
            _log(2, "Received disconnect event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_games_disconnect", reff, data);
            break;
        case GR_EVENT_GAMES_STATUS:
            _log(2, "Received status event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_games_status", reff, status, err, data);
            break;
        case GR_EVENT_ACHIEVEMENTS:
            _log(2, "Received sync event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_achievements_sync", reff, data);
            break;
        case GR_EVENT_ACHIEVEMENTS_CREATE:
            _log(2, "Received create event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_achievements_create", reff, status, data);
            break;
        case GR_EVENT_ACHIEVEMENTS_UPDATE:
            _log(2, "Received update event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_achievements_update", reff, status, data);
            break;
        case GR_EVENT_ACHIEVEMENTS_DELETE:
            _log(2, "Received delete event, reference: %s", reff);
            call_if(this_object(), "grapevine_handle_achievements_delete", reff, data);
            break;
        default:
            _log(2, "Unknown event: %s", event);
            break;
    }
}

protected void websocket_handle_close_frame(mapping payload) {
    // If we are closing and have recieved a restart message, restart
    // the connection after the specified time. Adding some seconds
    // to the restart time to ensure the server is ready to accept
    if(server["grapevine"]["restart"]) {
        _log(1, "Grapevine connection closed, restarting in %d seconds",
            server["grapevine"]["restart"]);

        restart = ([
            "restart" : server["grapevine"]["restart"],
            "attempts" : 0,
            "restarting" : call_out_walltime(
                (:restart_attempt:),
                server["grapevine"]["restart"] + 60
            ),
        ]);
    }
}

/* ************************************************************************* */
/* Grapevine Event Handlers                                                  */
/* ************************************************************************* */

// Authentication

void grapevine_handle_event_authenticate(string status, string err, mapping data) {
    _log(2, "Received authentication response: %s", identify(data));
    if(status == GR_STATUS_OK) {
        _log(2, "Authenticated with Grapevine");
    } else {
        _log(1, "Failed to authenticate with Grapevine, error: %s", err);
    }
}

// Heartbeat

void grapevine_handle_event_heartbeat(mapping data) {
    _log(2, "Received heartbeat");
    grapevine_send_event_heartbeat();
}

// Restart

private nomask void grapevine_handle_event_restart(string reff, mapping data) {
    // We don't really care about the reff? I guess I'll ignore it. I'm not
    // sure what it's for, but I don't think we need it.

    _log(1, "Grapevine restart imminent, duration: %d", data["downtime"]);
    server["grapevine"]["restart"] = data["downtime"];
}

// Channels

void grapevine_handle_channels_subscribe(string reff, string status, string err) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(err) {
        _log(1, "Failed to subscribe to channel `%s`: %s", request["channel"], err);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Subscribed to channel: %s", request["channel"]);
}

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_channels_unsubscribe(string reff) {
    mapping request = reffs[reff];
    mixed cerr;

    map_delete(reffs, reff);

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Unsubscribed from channel: %s", request["channel"]);
}

void grapevine_handle_channels_broadcast(string reff, mapping data) {
    _log(1, "Received broadcast on channel: %s", data["channel"]);
    catch(CHAN_D->grapevine_chat(data));
}

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_channels_send(string reff) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Received message confirmation on channel: %s", request["channel"]);
}

// Players

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_players_sign_in(string reff, mapping data) {
    // We have a reff, so this is a response to our own player signing in
    if(reff) {
        mapping request = reffs[reff];
        mixed cerr;

        if(!request)
            return;

        map_delete(reffs, reff);

        if(request["callback"])
            cerr = catch(call_back(request["callback"], request));

        if(cerr)
            _log(1, "Error in callback: %s", cerr);

        _log(1, "Received sign-in confirmation: %s", request["name"]);

        return;
    }

    catch(CHAN_D->grapevine_chat(([
        "channel" : mudConfig("GRAPEVINE")["notice"],
        "message" : sprintf("%s has signed in to %s", data["name"], data["game"]),
        "name" : "Grapevine",
        "game" : mud_name(),
    ])));

    _log(1, "Player signed in: %s@%s", data["name"], data["game"]);
}

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_players_sign_out(string reff, mapping data) {
    if(reff) {
        mapping request = reffs[reff];
        mixed cerr;

        if(!request)
            return;

        map_delete(reffs, reff);

        if(request["callback"])
            cerr = catch(call_back(request["callback"], request));

        if(cerr)
            _log(1, "Error in callback: %s", cerr);

        _log(1, "Received sign-out confirmation: %s", request["name"]);

        return;
    }

    catch(CHAN_D->grapevine_chat(([
        "channel" : mudConfig("GRAPEVINE")["notice"],
        "message" : sprintf("%s has signed out of %s", data["name"], data["game"]),
        "name" : "Grapevine",
        "game" : mud_name(),
    ])));

    _log(1, "Player signed out: %s@%s", data["name"], data["game"]);
}

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_players_status(string reff, string status, string err, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    _log(1, "Callback %O\n", request["callback"]);

    if(err) {
        if(request["callback"])
            cerr = catch(call_back(request["callback"], request, err));
        if(cerr)
            _log(1, "Error in callback: %s", cerr);
        _log(1, "Failed to get player status for game `%s`: %s", request["game"], err);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Received player status for game %s: %O", data["game"], data["players"]);
}

// Tells

void grapevine_handle_tells_send(string reff, string status, string err) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(err) {
        _log(1, "Failed to send tell to %s: %s", request["player"], err);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Sent tell to %s", request["player"]);
}

// TODO: We need to ask them what the significance is of the ref in this event.
void grapevine_handle_tells_receive(string reff, mapping data) {
    _log(1, "Received tell: %s", identify(data));
}

// Game

void grapevine_handle_games_connect(string reff, mapping data) {
    catch(CHAN_D->grapevine_chat(([
        "channel" : mudConfig("GRAPEVINE")["notice"],
        "message" : sprintf("%s has connected to Grapevine", data["game"]),
        "name" : "Grapevine",
        "game" : mud_name(),
    ])));

    _log(1, "Game connected: %s", data["game"]);
}

void grapevine_handle_games_disconnect(string reff, mapping data) {
    catch(CHAN_D->grapevine_chat(([
        "channel" : mudConfig("GRAPEVINE")["notice"],
        "message" : sprintf("%s has disconnected from Grapevine", data["game"]),
        "name" : "Grapevine",
        "game" : mud_name(),
    ])));

    _log(1, "Game disconnected: %s", data["game"]);
}

// TODO: Ask if it's possible to receive information about how many responses
// one will get, so we know when the request has completed fully.
// TODO: When doing a single request, the game appears to be case sensitive, despite
// tells/send not being case sensitive.
void grapevine_handle_games_status(string reff, string status, string err, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(err) {
        _log(1, "Failed to get game status for `%s`: %s", request["game"], err);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Game status: %s", data["game"]);
}

// Achievements
// TODO: We need to ask if we could receive a status and error message for this
// event.
void grapevine_handle_achievements_sync(string reff, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Achievements synced: %s", identify(data));
}

void grapevine_handle_achievements_create(string reff, string status, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(status == GR_STATUS_FAIL) {
        _log(1, "Failed to create achievement: %O\nErrors: %O", request, data["errors"]);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Achievement created: %s", identify(data));
}

void grapevine_handle_achievements_update(string reff, string status, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(status == GR_STATUS_FAIL) {
        _log(1, "Failed to update achievement: %O\nErrors: %O", request, data["errors"]);
        return;
    }

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Achievement updated: %s", identify(data));
}

// TODO: We need to ask them if we could receive a status and error message for
// this event.
void grapevine_handle_achievements_delete(string reff, mapping data) {
    mapping request = reffs[reff];
    mixed cerr;

    if(!request)
        return;

    map_delete(reffs, reff);

    if(request["callback"])
        cerr = catch(call_back(request["callback"], request, data));

    if(cerr)
        _log(1, "Error in callback: %s", cerr);

    _log(1, "Achievement deleted: %s", identify(data));
}

// Unknown
void grapevine_handle_unknown_channels(string reff, mapping data) {
    _log(1, "Unknown channels event: %s", identify(data));
}

void grapevine_handle_unknown_players(string reff, mapping data) {
    _log(1, "Unknown players event: %s", identify(data));
}

void grapevine_handle_unknown_game(string reff, mapping data) {
    _log(1, "Unknown game event: %s", identify(data));
}

void grapevine_handle_unknown_tells(string reff, mapping data) {
    _log(1, "Unknown tells event: %s", identify(data));
}

void grapevine_handle_unknown_achievements(string reff, mapping data) {
    _log(1, "Unknown achievements event: %s", identify(data));
}

/* ************************************************************************* */
/* Grapevine Sending Functions                                               */
/* ************************************************************************* */

// Authenticate

private nomask void grapevine_send_event_authenticate(mapping auth) {
    _log(2, "Sending authentication");
    send_outgoing_message(GR_EVENT_AUTHENTICATE, null, auth);
}

// Heartbeat

private nomask void grapevine_send_event_heartbeat() {
    object *u = users();
    mapping payload;

    _log(2, "Sending heartbeat");

    u = filter(u, (: interactive($1) && objectp(environment($1)) :));

    payload = ([
        "game" : get_config(__MUD_NAME__),
        "players" : map(u, (: $1->query_real_name() :)),
    ]);

    _log(2, "Sending heartbeat");
    send_outgoing_message(GR_EVENT_HEARTBEAT, null, payload);
}

// Channels

public nomask void grapevine_send_event_channels_subscribe(string chan, mixed *cb) {
    mapping payload = ([
        "channel" : chan,
    ]);

    send_outgoing_message(GR_EVENT_CHANNELS_SUBSCRIBE, generate_uuid(), payload, cb);
}

public nomask void grapevine_send_event_channels_unsubscribe(string chan, mixed *cb) {
    mapping payload = ([
        "channel" : chan,
    ]);

    send_outgoing_message(GR_EVENT_CHANNELS_UNSUBSCRIBE, generate_uuid(), payload, cb);
}

public nomask void grapevine_send_event_channels_send(object who, string chan, string msg, mixed *cb) {
    mapping payload = ([
        "channel" : chan,
        "message" : msg,
        "name" : who->query_real_name(),
    ]);

    send_outgoing_message(GR_EVENT_CHANNELS_SEND, generate_uuid(), payload, cb);
}

// Players

public nomask void grapevine_send_event_players_sign_in(object who) {
    mapping payload = ([
        "name" : who->query_real_name(),
        "game" : get_config(__MUD_NAME__)
    ]);

    send_outgoing_message(GR_EVENT_PLAYERS_SIGN_IN, generate_uuid(), payload);
}

// TODO: Ask why this event does not take a game, but sign-in does
public nomask void grapevine_send_event_players_sign_out(object who) {
    mapping payload = ([
        "name" : who->query_real_name(),
    ]);

    send_outgoing_message(GR_EVENT_PLAYERS_SIGN_OUT, generate_uuid(), payload);
}

// TODO: Ask if this can be made case-insensitive like tells/send
public nomask void grapevine_send_event_players_status(string game, mixed *cb) {
    mapping payload;

    if(game)
        payload = ([
            "game" : game
        ]);

    send_outgoing_message(GR_EVENT_PLAYERS_STATUS, generate_uuid(), payload, cb);
}

// Tells

public nomask void grapevine_send_event_tells_send(object who, string to, string game, string msg, mixed *cb) {
    mapping payload = ([
        "from_name" : who->query_real_name(),
        "to_name" : to,
        "to_game" : game,
        "message" : msg,
        "sent_at" : z_time_string()
    ]);

    send_outgoing_message(GR_EVENT_TELLS_SEND, generate_uuid(), payload, cb);
}

private nomask varargs void send_outgoing_message(string ev, string reff, mapping message, mixed *cb) {
    mapping outgoing;

    if(!server) {
        _log(1, "Not connected to Grapevine");
        return;
    }

    outgoing = ([ "event" : ev ]);

    if(!nullp(message))
    outgoing["payload"] = message;

    if(!nullp(reff)) {
        outgoing["ref"] = reff;
    }

    if(reff)
        reffs[reff] = copy(message || ([])) + ([ "event" : ev, "callback" : cb ]);

    _log(4, "Sending message: %O", outgoing);
    websocket_message(WS_TEXT_FRAME, json_encode(outgoing));
}
// Games

public nomask void grapevine_send_event_games_status(mixed game, mixed *cb) {
    mapping payload;

    if(stringp(game))
        payload = ([
            "game" : game
        ]);
    else
        payload = null;

    send_outgoing_message(GR_EVENT_GAMES_STATUS, generate_uuid(), payload, cb);
}

// Achievements

// TODO: Ask if these persist across connections. I think it does, because
// otherwise why would we have a sync event?
// If so, then we need to manage keys. 🤢

public nomask void grapevine_send_event_achievements_sync() {
    send_outgoing_message(GR_EVENT_ACHIEVEMENTS_SYNC, generate_uuid(), null);
}

public nomask void grapevine_send_event_achievements_create(mapping achievement) {
    send_outgoing_message(GR_EVENT_ACHIEVEMENTS_CREATE, generate_uuid(), achievement);
}

public nomask void grapevine_send_event_achievements_update(mapping achievement) {
    send_outgoing_message(GR_EVENT_ACHIEVEMENTS_UPDATE, generate_uuid(), achievement);
}

public nomask void grapevine_send_event_achievements_delete(string key) {
    mapping payload = ([
        "key" : key
    ]);

    send_outgoing_message(GR_EVENT_ACHIEVEMENTS_DELETE, generate_uuid(), payload);
}

public nomask void grapevine_broadcast_message(string chan, string usr, string msg) {
    mapping message = ([
        "channel" : chan,
        "message" : msg,
        "name" : usr,
        "game" : get_config(__MUD_NAME__)
    ]);

    send_outgoing_message(GR_EVENT_CHANNELS_SEND, generate_uuid(), message);
}

varargs public nomask string z_time_string(int time: (: time() :)) {
    mixed *lt = localtime(time), *gt;
    int off = lt[LT_GMTOFF];

    time += off + (-1 * lt[LT_ISDST]) * 3600;
    return strftime("%Y-%m-%dT%H:%M:%SZ", time);
}

// This event will run when this object is being destructed, so we should
// let the server know that we are going away.
void unsetup() {
    _log(1, "Grapevine destructing.");
    if(server) {
        _log(1, "Closing Grapevine connection.");
        websocket_close(WS_CLOSE_GOING_AWAY);
    }
}
