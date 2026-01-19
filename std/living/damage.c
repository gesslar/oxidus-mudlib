/**
 * @file /std/user/damage.c
 * @description Damage object for livings
 *
 * @created 2024-07-25 - Gesslar
 * @last_modified 2024-07-25 - Gesslar
 *
 * @history
 * 2024-07-25 - Gesslar - Created
 */

#include <damage.h>
#include <vitals.h>

// Functions from other objects
float query_defense_amount(string type);
float query_effective_level();
object set_last_damaged_by(object ob);
object set_killed_by(object ob);

float deliver_damage(object victim, float damage, string type) {
    if(!victim)
        return 0;

    if(damage < 0)
        return 0;

    return victim->receive_damage(this_object(), damage, type);
}

/**
 * @description Calculates the damage received by the victim based on the attacker's level,
 *              defense amount, and a damage level modifier.
 * @param {object} attacker - The attacker object.
 * @param {float} damage - The initial damage value.
 * @param {string} type - The type of damage.
 * @returns {float} - The final damage after applying reductions and level-based modifications.
 */
float receive_damage(object attacker, float damage, string type) {
    float def, red, mod, level, alevel, level_difference;

    if(!attacker)
        return 0.0;

    if(damage < 0.0)
        return 0.0;

    def = query_defense_amount(type);
    red = percent_of(def, damage);

    // The global damage level modifier
    mod = mudConfig("DAMAGE_LEVEL_MODIFIER");

    // The level of the attacker and the victim
    level = query_effective_level();
    alevel = attacker->query_effective_level();

    // The difference in levels between the attacker and the victim
    level_difference = alevel - level;

    // The damage level modifier is multiplied by the difference in levels
    mod = mod * level_difference;
    red -= percent_of(mod, damage);

    // The damage is reduced by the level modifier
    damage -= red;

    if(damage < 0.0)
        damage = 0.0;

    if(query_hp() <= 0.0)
        return 0.0;

    // Take the damage
    adjust_hp(-damage);

    // Set the last damaged by object
    set_last_damaged_by(attacker);

    if(query_hp() <= 0.0)
        set_killed_by(attacker);

    return damage;
}
