/**
 * @file /std/object/setup.c
 * Provides a standardised setup and teardown chain for objects,
 * ensuring proper initialization and cleanup order.
 *
 * @created 2024-01-29 - Gesslar
 * @last_modified 2025-03-16 - GitHub Copilot
 *
 * @history
 * 2024-01-29 - Gesslar - Created
 * 2025-03-16 - GitHub Copilot - Added documentation
 */

/**
 * Executes the complete setup chain for an object in the proper order.
 *
 * The setup chain is executed in this order:
 * 1. mudlib_setup - Low-level setup for mudlib core functionality
 * 2. base_setup - Base type setup (room types, item types, etc.)
 * 3. pre_setup_0 through pre_setup_4 - Pre-setup hooks
 * 4. setup - Main setup function for specific object configuration
 * 5. post_setup_0 through post_setup_4 - Post-setup hooks
 * 6. restore_data and post_restore - For persistent objects
 * 7. mudlib_complete_setup - Final setup completion
 *
 * @param {mixed...} args - Variable arguments to pass to setup functions
 */
varargs void setup_chain(mixed args...) {
  int x;

  // Intended for the most base stup in /std
  call_if(this_object(), "std_setup", args...);

  // Intended for objects at their very mudlib level
  call_if(this_object(), "mudlib_setup", args...);

  // Intended for objects that that are base for things like rooms of
  // a common zone, room-type, or anything else that needs to setup
  // the base of its object, after the mudlib_setup has been called.
  call_if(this_object(), "base_setup", args...);

  x = 5;
  for(x = 0; x < 5; x++)
    call_if(this_object(), "pre_setup_"+x, args...);

  // The actual setup function that is ordinarily used by any object
  // that isn't the master or a base object.
  call_if(this_object(), "setup", args...);

  x = 5;
  for(x = 0; x < 5; x++)
    call_if(this_object(), "post_setup_"+x, args...);

  if(call_if(this_object(), "query_persistent")) {
    call_if(this_object(), "restore_data");
    call_if(this_object(), "post_restore");
  }

  call_if(this_object(), "restore_data");
  call_if(this_object(), "post_restore");

  call_if(this_object(), "mudlib_complete_setup");
}

/**
 * Executes the complete teardown chain for an object in the proper order.
 *
 * The unsetup chain is executed in this order:
 * 1. mudlib_unsetup - Low-level teardown for mudlib core functionality
 * 2. base_unsetup - Base type teardown
 * 3. pre_unsetup_0 through pre_unsetup_4 - Pre-unsetup hooks
 * 4. unsetup - Main unsetup function for specific object cleanup
 * 5. post_unsetup_0 through post_unsetup_4 - Post-unsetup hooks
 * 6. save_data and post_save - For persistent objects
 */
void unsetup_chain() {
  int x;

  // Itended for objects at their very mudlib level
  call_if(this_object(), "mudlib_unsetup", previous_object(1));

  // Intended for objects that that are base for things like rooms of
  // a common zone, room-type, or anything else that needs to unsetup
  // the base of its object, after the mudlib_unsetup has been called.
  call_if(this_object(), "base_unsetup", previous_object(1));

  for(x = 0; x < 5; x++)
    call_if(this_object(), "pre_unsetup_"+x, previous_object(1));

  // The actual unsetup function that is ordinarily used by any object
  // that isn't the master or a base object.
  call_if(this_object(), "unsetup", previous_object(1));

  for(x = 0; x < 5; x++)
    call_if(this_object(), "post_unsetup_"+x, previous_object(1));

  if(call_if(this_object(), "query_persistent")) {
    call_if(this_object(), "save_data");
    call_if(this_object(), "post_save");
  }
}

/**
 * Executes the setup chain for a virtual object.
 *
 * Similar to setup_chain() but calls virtual-specific setup functions.
 * Used for dynamically created objects like procedurally generated rooms.
 *
 * @param {mixed...} args - Variable arguments to pass to setup functions
 */
varargs void virtual_setup_chain(mixed args...) {
  int x;

  call_if(this_object(), "virtual_mudlib_setup", args...);

  for(x = 0; x < 5; x++)
    call_if(this_object(), "pre_virtual_setup_" + x, args...);

  call_if(this_object(), "virtual_setup", args...);

  x = 5;
  while(x--)
    call_if(this_object(), "post_virtual_setup_" + x, args...);

  call_if(this_object(), "virtual_mudlib_complete_setup");
}
