/**
 * @file /cmds/spell/motes.c
 * @description Motes spell.
 *
 * @created 2024-08-09 - Gesslar
 * @last_modified 2024-08-09 - Gesslar
 *
 * @history
 * 2024-08-09 - Gesslar - Created
 */

inherit STD_SPELL;

void setup() {
  set_name("motes of light");

  aggressive = true;
  target_current = true;
  sp_cost = 5.0;

  cooldowns = ([
    "motes" : ({ "", 10 }),
  ]);

  usage_text = "motes <target>";
  help_text = sprintf(
"Summon motes of light to strike a target. This spell costs "
"%.1f SP and has a cooldown of %d seconds.",
    evaluate(sp_cost), evaluate(cooldowns["motes"][1])
  );
}

mixed use(/** @type {STD_BODY} */ object tp, string arg) {
  object victim;
  mixed result;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  if(!result = delay_act("motes", 2.0, assemble_call_back(
    function(int status,
      /** @type {STD_BODY} */ object tp,
      /** @type {STD_BODY} */ object victim
    ) {
      if(!status)
        return;

      if(!same_env_check(tp, victim))
        return;

      if(tp->can_strike(victim, "combat.spell.light")) {
        float damage = 5.0 + tp->query_damage();

        tp->targetted_action(
          "{{FF0033}}Motes of light strike $t!{{res}}",
          victim
        );
        tp->deliver_damage(victim, damage, "light");
        tp->use_skill("combat.spell.light");
      } else {
        tp->simple_action(
          "The motes of light disperse harmlessly."
        );
        victim->use_skill("combat.defense.evade");
      }

      victim->start_attack(tp);
    }, tp, victim
  ))) {
    return "You are already doing something.";
  }

  apply_cost(tp, arg);
  apply_cooldown(tp, arg);

  tp->simple_action("$N $vsummon motes of light...");

  return 1;
}
