/**
 * @file /obj/weapon/piercing/rusty_sword.c
 * @description A basic sword for stabbing things
 *
 * @created 2024-08-04 - Gesslar
 * @last_modified 2024-08-04 - Gesslar
 *
 * @history
 * 2024-08-04 - Gesslar - Created
 */

inherit STD_WEAPON;
inherit EXT_PROC;

void bite(object attacker, object defender);

void setup() {
  set_id(({ "sword", "rusty sword" }));
  set_short("rusty sword");
  set_name("rusty sword");
  set_long("This rusty sword is a simple, straight blade with a single "
           "edge. The blade is pitted and worn, and the hilt is wrapped in "
           "cracked leather. Despite its appearance, the sword is still "
           "sharp and serviceable, and the blade is straight and true.");
  set_mass(50);
  set_hands(1);
  set_dc(1);
  set_damage_type("piercing");
  set_value(10);

  set_procs(({
    ({ "bite", (: bite :) }),
  }));

  set_proc_chance(100.0);
}

/**
 *
 * @param {STD_BODY} attacker - The attacking living.
 * @param {STD_BODY} defender - The defending living.
 */
void bite(object attacker, object defender) {
  if(attacker->can_strike(defender)) {
    float damage = percent_of(25.0, query_dc());

    attacker->targetted_action(
      "$P $o strikes $t viciously, biting deep into $p1 flesh.",
      defender, this_object());

    attacker->deliver_damage(defender, damage, "piercing");
    attacker->use_skill("combat.melee.piercing");
  } else {
    attacker->targetted_action("$N $vswing $p $o wildly, missing $t.",
      defender, this_object());
    defender->use_skill("combat.defense.dodge");
  }
}
