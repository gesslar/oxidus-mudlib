/**
 * @file /cmds/spell/shard.c
 * Shard spell.
 *
 * @created 2026-05-11 - Gesslar
 * @last_modified 2026-05-11 - Gesslar
 *
 * @history
 * 2026-05-11 - Gesslar - Created
 */

inherit STD_SPELL;

void setup() {
  set_name("shard");

  aggressive = true;
  target_current = true;
  sp_cost = 6.0;

  cooldowns = ([
    "shard" : ({ "", 12 }),
  ]);

  usage_text = "shard <target>";
  help_text = sprintf(
"Drive a wicked shard of magical ice into a target. The shard "
"strikes for a small initial wound, then shatters a few seconds "
"later for a burst of cold damage. This spell costs %.1f SP and "
"has a cooldown of %d seconds.",
    evaluate(sp_cost), evaluate(cooldowns["shard"][1])
  );
}

mixed use(/** @type {STD_BODY} */ object tp, string arg) {
  /** @type {STD_BODY} */ object victim;
  mixed result;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  if(!result = delay_act("shard", 2.0, assemble_call_back(
    function(int status,
      /** @type {STD_BODY} */ object tp,
      /** @type {STD_BODY} */ object victim
    ) {
      if(!status)
        return;

      if(!same_env_check(tp, victim))
        return;

      if(tp->can_strike(victim, "arcane.discipline.frost")) {
        float initial = 2.0 + tp->query_damage() * 0.3;
        float burst = 6.0 + tp->query_damage();

        tp->targetted_action(
          "{{cef}}A wicked shard of magical ice drives into $t!{{res}}",
          victim
        );
        tp->deliver_damage(victim, initial, "cold");
        tp->use_skill("arcane.discipline.frost", 0.15);

        // Shards stack — each caster's shard shatters on its own
        // schedule, so group casts don't trample each other.
        /** @type {"obj/effect/ice_shard"} */ object eff = clone_object("/obj/effect/ice_shard");
        eff->set_caster(tp);
        eff->set_burst(burst);
        eff->move(victim);
        eff->set_fixed(1);
        victim->curse_object(eff, 3);
      } else {
        tp->simple_action(
          "The shard of ice splinters harmlessly."
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

  tp->simple_action("$N $vconjure a wicked shard of magical ice...");

  return 1;
}
