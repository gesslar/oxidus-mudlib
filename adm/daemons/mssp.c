/**
 * @file /adm/daemons/mssp.c
 *
 * MSSP (Mud Server Status Protocol) daemon. Called by the master
 * object to respond to the get_mud_stats() apply. Dynamic and
 * driver hard-coded values are computed here; static values are
 * read from /adm/etc/mssp.json.
 *
 * @see https://tintin.mudhalla.net/protocols/mssp/
 *
 * @created 2024-02-03 - Gesslar
 * @last_modified 2024-02-03 - Gesslar
 *
 * @history
 * 2024-02-03 - Gesslar - Created
 */

#include <runtime_config.h>

inherit STD_DAEMON;

private nosave mapping mud_stats = ([]);

void setup() {
  if(file_exists("/adm/etc/mssp.json"))
    mud_stats = json_decode(read_file("/adm/etc/mssp.json"));
}

/**
 * Returns the current MUD server status as an MSSP mapping.
 * Merges dynamic values (name, port, uptime, player count,
 * protocol support) with static values loaded from the JSON
 * configuration file.
 *
 * @apply
 * @returns {([ string: string ])} MSSP key-value pairs
 */
mapping get_mud_stats() {
  return ([
    "NAME"      : mud_name(),
    "PORT"      : sprintf("%d", __PORT__),
    "UPTIME"    : sprintf("%d", time() - uptime()),
    "PLAYERS"   : sprintf("%d", sizeof(users())),
    "MCP"       : "0",
    "GMCP"      : sprintf("%d", get_config(__RC_ENABLE_GMCP__)),
    "MXP"       : sprintf("%d", get_config(__RC_ENABLE_MXP__)),
    "MSP"       : sprintf("%d", get_config(__RC_ENABLE_MSP__)),
    "MCCP"      : "1",
    "UTF-8"     : "1",
  ]) + mud_stats;
}
