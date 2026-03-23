/**
 * @file /cmds/wiz/gr
 * @description Grapevine command
 *
 * @created 2024-07-18 - Gesslar
 * @last_modified 2024-07-18 - Gesslar
 *
 * @history
 * 2024-07-18 - Gesslar - Created
 */

#include <daemons.h>

inherit STD_CMD;

void handle_pstatus(mapping request, mapping response, object tp);
void handle_gstatus(mapping request, mapping response, object tp);

mixed main(object tp, string str) {
  string cmd, arg;

  if(sscanf(str, "%s %s", cmd, arg) != 2) {
    cmd = str;
  }

  switch(cmd) {
    case "pstatus" :
      GRAPEVINE_D->grapevine_send_event_players_status(
        arg,
        assemble_call_back((: handle_pstatus :), tp),
      );
      return _ok(sprintf("Sending players status request for %s", arg));
    case "gstatus" :
      GRAPEVINE_D->grapevine_send_event_games_status(
        arg,
        assemble_call_back((: handle_gstatus :), tp, arg),
      );
      return _ok(sprintf("Sending game status request for %s", arg));
  }

  return _error("Unknown Grapevine command.");
}

void handle_pstatus(mapping request, mixed response, object tp) {
  if(!tp) {
    return;
  }

  if(stringp(response)) {
    _error(tp, sprintf("%s: %s", request["game"], response));
  } else if(sizeof(response["players"])) {
    _info(tp,
      sprintf("The following players are playing %s:\n%s\n",
        response["game"],
        sizeof(response["players"]) ? simple_list(response["players"]) : "<none>"
      )
    );
  } else {
    _info(tp, sprintf("Nobody is playing %s.", response["game"]));
  }
}

void handle_gstatus(mapping request, mixed response, object tp, string game) {
  if(!tp) {
    return;
  }

  _info(tp, sprintf("Response: %O", response));
}
