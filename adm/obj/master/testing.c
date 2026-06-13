/**
 * @file /adm/obj/master/testing.c
 *
 * Testing-related master object functionality.
 *
 * @created 2026-06-12 - Gesslar
 * @last_modified 2026-06-12 - Gesslar
 *
 * @history
 * 2026-06-12 - Gesslar - Created
 */

// Deadline timestamp (epoch seconds) until which caught errors are suppressed
// in error_handler(). 0 means inactive. Auto-expires so a crashed/missing
// clear_test_mode() call can never leave the master permanently squelched.
private nosave int testing_in_progress = 0;

/**
 * Enable test-mode error suppression for `duration` seconds. May only be
 * called by an object inheriting STD_TEST_RUNNER. Pass 0 (or negative) to
 * disable.
 *
 * @param {int} duration - Seconds to suppress caught-error logging.
 * @returns {int} 1 on success, 0 if caller is not authorized.
 */
int set_test_mode(int duration) {
  object po = previous_object();

  if(!po || !inherits(STD_TEST_RUNNER, po))
    return 0;

  testing_in_progress = duration > 0 ? time() + duration : 0;
  return 1;
}

/**
 * Disable test-mode error suppression. Same priv-check as set_test_mode().
 */
void clear_test_mode() {
  object po = previous_object();

  if(!po || !inherits(STD_TEST_RUNNER, po))
    return;

  testing_in_progress = 0;
}

/**
 * @returns {int} 1 if test-mode error suppression is currently active.
 */
int query_test_mode() {
  return testing_in_progress > time();
}
