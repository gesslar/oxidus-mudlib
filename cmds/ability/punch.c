/**
 * @file /cmds/ability/punch.c
 * @description Punch command
 *
 * @created 2024-08-08 - Gesslar
 * @last_modified 2024-08-08 - Gesslar
 *
 * @history
 * 2024-08-08 - Gesslar - Created
 */

inherit STD_ABILITY;

void setup() {
  set_name("punch");

  aggressive = true;
  target_current = true;
  mp_cost = 5.0;

  cooldowns = ([
      "punch" : ({ "", 10 }),
  ]);

  usage_text = "punch <target>";
  help_text = sprintf(
"Use your fist to punch a target. This ability costs %.1f MP and has a "
"cooldown of %d seconds.",
  evaluate(mp_cost), evaluate(cooldowns["punch"][1])
  );
}

mixed use(/** @type {STD_BODY} */ object tp, string arg) {
  object victim;
  mixed result;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  if(!result = delay_act("punch", 2.0, assemble_call_back(
    function(int status,
      /** @type {STD_BODY} */ object tp,
      /** @type {STD_BODY} */ object victim
    ) {
      if(!status)
          return;

      if(!same_env_check(tp, victim))
          return;

      if(tp->can_strike(victim)) {
          float damage = percent_of(25.0, tp->query_damage());

          tp->targetted_action("$N $vpunch $t!", victim);
          tp->deliver_damage(victim, damage, "bludgeoning");
          tp->use_skill("combat.melee.unarmed");
      } else {
          tp->targetted_action("$N $vtry to punch $t, but $vmiss.", victim);
          victim->use_skill("combat.defense.dodge");
      }

      victim->start_attack(tp);
    }, tp, victim
  ))) {
    return "You are already doing something.";
  }

  apply_cost(tp, arg);
  apply_cooldown(tp, arg);

  tp->simple_action("$N $vpull back a fist...");

  return 1;
}
