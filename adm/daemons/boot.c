/**
 * @file /adm/daemons/boot.c
 * @description Daemon to manage incrementing boot number and calling the
 * boot alarm.
 *
 * @created 2024-03-05 - Gesslar
 * @last_modified 2024-03-05 - Gesslar
 *
 * @history
 * 2024-03-05 - Gesslar - Created
 */

#include <daemons.h>
#include <origin.h>

inherit STD_DAEMON;
inherit M_LOG;

private nomask int boot_number;

void setup() {
  set_log_level(0);
  setPersistent(1);
  slot(SIG_SYS_BOOT, "boot");
}

/**
 * @function boot
 * @description Called from the master object when the mud boots up.
 */
void boot(mixed arg...) {
  if(previous_object() != signal_d())
    return;

  _log(1, "Boot #%d loaded.", ++boot_number);

  saveData();
}

/**
 * @daemon_function query_boot_number
 * @description Get the current boot number.
 * @returns {int} - current boot number
 */
int query_boot_number() {
  return boot_number;
}
