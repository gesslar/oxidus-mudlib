/**
 * @file /cmds/spell/embers.c
 *
 * Embers spell.
 *
 * @created 2026-05-11 - Gesslar
 * @last_modified 2026-05-11 - Gesslar
 *
 * @history
 * 2026-05-11 - Gesslar - Created
 */

inherit STD_SPELL;

void setup() {
  set_name("embers");

  aggressive = true;
  target_current = true;
  sp_cost = 6.0;

  cooldowns = ([
    "embers" : ({ "", 10 }),
  ]);

  usage_text = "embers <target>";
  help_text = sprintf(
"Hurl glowing embers at a target that cling and burn over a short "
"time. This spell costs %.1f SP and has a cooldown of %d seconds.",
    evaluate(sp_cost), evaluate(cooldowns["embers"][1])
  );
}

mixed use(/** @type {STD_BODY} */ object tp, string arg) {
  /** @type {STD_BODY} */ object victim;
  mixed result;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  if(!result = delay_act("embers", 2.0, assemble_call_back(
    function(
      int status,
      /** @type {STD_BODY} */ object tp,
      /** @type {STD_BODY} */ object victim
    ) {
      if(!status)
        return;

      if(!same_env_check(tp, victim))
        return;

      if(tp->can_strike(victim, "arcane.discipline.fire")) {
        float total = 5.0 + tp->query_damage();
        float per_tick = (total / 2.0) * 1.15;

        tp->targetted_action(
          "{{f73}}Glowing embers cling to $t!{{res}}",
          victim
        );
        tp->use_skill("arcane.discipline.fire", 0.15);

        // Recasting reapplies a fresh burn rather than stacking.
        mapping existing = victim->query_curse_object(
          (: $1["object"]->id("burn") :)
        );
        if(mapp(existing)) {
          foreach(int tag in keys(existing))
            victim->remove_curse_object(tag);
        }
        /** @type {"obj/effect/burn"} */ object eff = clone_object("/obj/effect/burn");
        eff->set_caster(tp);
        eff->set_burn(per_tick, 2);
        eff->move(victim);
        eff->set_fixed(1);
        victim->curse_object(eff, 6);
        eff->start();
      } else {
        tp->simple_action(
          "The embers fizzle and fall away harmlessly."
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

  tp->simple_action("$N $vsummon a handful of glowing embers...");

  return 1;
}
