/**
 * @file /cmds/spell/wither.c
 *
 * Wither spell.
 *
 * @created 2026-05-11 - Gesslar
 * @last_modified 2026-05-11 - Gesslar
 *
 * @history
 * 2026-05-11 - Gesslar - Created
 */

inherit STD_SPELL;

void setup() {
  set_name("wither");

  aggressive = true;
  target_current = true;
  sp_cost = 7.0;

  cooldowns = ([
    "wither" : ({ "", 15 }),
  ]);

  usage_text = "wither <target>";
  help_text = sprintf(
"Weave grasping shadow that clings to a target and weakens "
"their defenses for a time. This spell costs %.1f SP and has "
"a cooldown of %d seconds.",
    evaluate(sp_cost), evaluate(cooldowns["wither"][1])
  );
}

mixed use(/** @type {STD_BODY} */ object tp, string arg) {
  /** @type {STD_BODY} */ object victim;
  mixed result;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  if(!result = delay_act("wither", 2.0, assemble_call_back(
    function(int status,
      /** @type {STD_BODY} */ object tp,
      /** @type {STD_BODY} */ object victim
    ) {
      if(!status)
        return;

      if(!same_env_check(tp, victim))
        return;

      if(tp->can_strike(victim, "arcane.discipline.shadow")) {
        /** @type {"obj/effect/wither"} */ object eff;

        tp->targetted_action(
          "{{83a}}$N $vreach out with a grasping shadow toward $t!{{res}}",
          victim
        );
        tp->use_skill("arcane.discipline.shadow", 0.15);

        // Withers stack across casters — each contributes its own
        // -amount to the three defense skills.
        eff = clone_object("/obj/effect/wither");
        eff->set_caster(tp);
        eff->set_amount(10);
        eff->set_duration(30);
        eff->move(victim);
        eff->set_fixed(1);
        victim->curse_object(eff, 30);
        eff->apply();

        victim->simple_action("$N $vlook weakened.");
      } else {
        tp->targetted_action(
          "The grasping shadow recoils from $t.",
          victim
        );
        victim->use_skill("arcane.combat.evade", 0.1);
      }

      victim->start_attack(tp);
    }, tp, victim
  ))) {
    return "You are already doing something.";
  }

  apply_cost(tp, arg);
  apply_cooldown(tp, arg);

  tp->simple_action("$N $vweave threads of grasping shadow...");

  return 1;
}
