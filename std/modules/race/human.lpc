/**
 * @file /std/modules/race/human.c
 * Human racial module
 *
 * @created 2024-07-25 - Gesslar
 * @last_modified 2024-07-25 - Gesslar
 *
 * @history
 * 2024-07-25 - Gesslar - Created
 */

inherit __DIR__ "race";

void setup() {
  ::setup();

  regen_rate = ([
    "hp" : 2.0,
    "sp" : 2.0,
    "mp" : 4.0,
  ]);
}

int set_up_body_parts(object ob, mixed args...) {
  return use_default_body_parts();
}
