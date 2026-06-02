/**
 * @file /std/living/skills.c
 *
 * Trainable skills for living objects.
 *
 * Skills are stored as a nested tree where each node carries a float
 * "level" (integer part = effective level, fractional part = progress
 * toward the next level) and a "subskills" submapping. Dot-paths
 * (e.g. "combat.melee.slashing") address nodes within the tree.
 *
 * Improvement is use-based: callers invoke use_skill() and, on a
 * 20% roll, improve_skill() picks a node from the skill's dot-path
 * via a weighted draw favouring leaves over roots, then applies a
 * single random_float against the supplied cap to that node.
 * Parent skills therefore grow organically alongside the children
 * that are used, but more slowly because they are picked less
 * often. Callers may pass mod_adjust to raise the upper bound on
 * the random draw — useful for low-frequency call sites (specific
 * spells, niche abilities) where the default 0.01 cap would feel
 * too slow. NPCs are seeded at setup with every skill set to
 * level * 3.0 so that query_skill_level() is honest in combat math
 * without any special-case branching.
 *
 * @created 2024-07-31 - Gesslar
 * @last_modified 2026-05-10 - Gesslar
 *
 * @history
 * 2024-07-31 - Gesslar - Created
 * 2026-05-10 - Gesslar - Documentation pass; NPC skills seeded at
 *                        level*3 instead of zero so query_skill_level
 *                        reflects them; dropped NPC shortcut in
 *                        query_skill, then renamed query_skill to
 *                        query_raw_skill so the name advertises its
 *                        no-floor / no-boon semantics; use_skill
 *                        gained an optional mod_adjust that raises
 *                        the upper bound on per-call random progress,
 *                        and improve_skill's progress argument is
 *                        now mixed (float/int/closure) and is always
 *                        treated as the random_float cap, with the
 *                        weighted bubble-up running every call;
 *                        extracted find_skill_node() helper so the
 *                        nested-tree walk is no longer duplicated
 *                        across every read/leaf-mutate function;
 *                        added query_raw_skill_level (floored, no
 *                        boon) and has_skill (1/0 existence check);
 *                        improve_skill explored per-node depth-
 *                        tightening but settled on the simpler
 *                        shape: every node on the path shares the
 *                        same cap, with bubble-up balance living
 *                        entirely in the pick weights — easier to
 *                        tune and faithful to the call-site's
 *                        request regardless of which node is
 *                        chosen.
 */

#include <skills.h>
#include <advancement.h>
#include <boon.h>
#include <npc.h>

private nomask mapping __skills = ([]);

/**
 * Reset the skill tree to an empty mapping, discarding all skills
 * and progress.
 */
void wipe_skills() {
  __skills = ([]);
}

/**
 * Walk a config-shaped skill tree and create any missing skills at
 * level 1.0. Submappings recurse into nested categories; arrays of
 * strings are treated as leaf-skill names rooted at the current
 * path.
 *
 * @param {mapping} skill_set - A mapping of skill categories. Values
 *                              may be submappings (recurse) or arrays
 *                              of leaf names.
 * @param {string} [curr_path] - The dot-path of the current recursion
 *                               level. Omit on the initial call.
 */
varargs void initialize_missing_skills(mapping skill_set, string curr_path) {
  string cat;
  mixed element;

  if(!curr_path)
    curr_path = "";

  foreach(cat, element in skill_set) {
    string full_path = curr_path != "" ?
      curr_path + "." + cat : cat;

    if(pointerp(element)) {
      foreach(string sk in element) {
        string skill_path = full_path + "." + sk;
        add_skill(skill_path, 1.0);
      }
    } else if(mapp(element)) {
      initialize_missing_skills(element, full_path);
    } else {
      add_skill(full_path, 1.0);
    }
  }
}

/**
 * Create a skill at the given dot-path, building intermediate nodes
 * at level 1.0 as needed. Existing nodes along the path are
 * preserved — this function does not overwrite a skill that already
 * exists.
 *
 * @param {string} skill - The dot-path of the skill to create.
 * @param {float} level - The starting level for the leaf node. Must
 *                        be >= 1.0.
 * @returns {int} 1 on success, null on invalid input.
 */
varargs int add_skill(string skill, float level) {
  string *path = explode(skill, ".");
  mapping current = __skills;
  int x, sz;

  if(!stringp(skill) || nullp(level) || level < 1.0)
    return null;

  sz = sizeof(path);
  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]])) {
      current[path[x]] = ([
        "level" : (x == sz - 1 ? level : 1.0),
        "subskills" : ([]),
      ]);
    }
    current = current[path[x]]["subskills"];
  }

  return 1;
}

/**
 * Remove a skill node from the tree.
 *
 * Walks the dot-path; if any intermediate node is missing the call
 * is a no-op and returns 0. Only the addressed leaf is removed —
 * intermediate nodes are left in place even if they become empty.
 *
 * @param {string} skill - The dot-path of the skill to remove.
 * @returns {int} 1 if removed, 0 if not found, null on invalid input.
 */
int remove_skill(string skill) {
  string *path;
  mapping current = __skills;
  int x, sz;

  if(!stringp(skill))
    return null;

  path = explode(skill, ".");
  sz = sizeof(path);

  for(x = 0; x < sz - 1; x++) {
    if(!mapp(current)
    || !current[path[x]]
    || !mapp(current[path[x]]["subskills"]))
      return 0;
    current = current[path[x]]["subskills"];
  }

  if(!mapp(current) || !current[path[sz - 1]])
    return 0;

  map_delete(current, path[sz - 1]);

  return 1;
}

/**
 * Walk the skill tree to the node addressed by a dot-path and
 * return its mapping. The returned mapping is the live tree node —
 * callers can both read node["level"] and mutate it in place.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {mapping} The node's mapping (with "level" and
 *                    "subskills" keys), or 0 if any intermediate is
 *                    missing or the input is invalid.
 */
private nomask mapping find_skill_node(string skill) {
  string *path;
  mapping current = __skills;
  int x, sz;

  if(!stringp(skill))
    return 0;

  path = explode(skill, ".");
  sz = sizeof(path);

  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return 0;
    if(x == sz - 1)
      return current[path[x]];
    current = current[path[x]]["subskills"];
  }

  return 0;
}

/**
 * Get the raw float level of a skill — no flooring, no boon
 * modifier. Use query_skill_level() for combat math.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {float} The raw float level, or null if the skill is not
 *                  found or input is invalid.
 */
float query_raw_skill(string skill) {
  mapping node = find_skill_node(skill);

  if(!node)
    return null;

  return node["level"];
}

/**
 * Get the float level of a skill with boon modifiers applied — the
 * unfloored counterpart to query_skill_level(). Use this when the
 * fractional progress matters (proc rolls, scaling formulas); use
 * query_skill_level() when you want the integer level.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {float} The raw float level plus the boon modifier, or
 *                  null if the skill is not found or input is
 *                  invalid.
 */
float query_skill(string skill) {
  mapping node = find_skill_node(skill);

  if(!node)
    return null;

  return node["level"] + query_effective_boon("skill", skill);
}

/**
 * Get the floored level of a skill with boon modifiers applied.
 * Combat formulas use this function rather than query_raw_skill()
 * because the boon modifier is applied here. Use
 * query_raw_skill_level() to get the floored level without boons.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {float} The floored level with the boon modifier applied,
 *                  or null if the skill is not found or input is
 *                  invalid.
 */
float query_skill_level(string skill) {
  mapping node = find_skill_node(skill);

  if(!node)
    return null;

  return floor(node["level"]) + query_effective_boon("skill", skill);
}

/**
 * Get the floored level of a skill with no boon modifier applied.
 * Equivalent to query_skill_level(skill, 1) under a name that makes
 * the unbuffed intent obvious at the call site.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {float} The floored level with no boon applied, or null
 *                  if the skill is not found or input is invalid.
 */
float query_raw_skill_level(string skill) {
  mapping node = find_skill_node(skill);

  if(!node)
    return null;

  return floor(node["level"]);
}

/**
 * Check whether a skill exists in the tree.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {int} 1 if the skill exists, 0 otherwise.
 */
int has_skill(string skill) {
  return mapp(find_skill_node(skill));
}

/**
 * Replace a skill's float level. Intermediate nodes must already
 * exist — this function will not create them.
 *
 * @param {string} skill - The dot-path of the skill.
 * @param {float} level - The new level value (must be >= 1.0).
 * @returns {int} 1 on success, 0 if an intermediate is missing,
 *                null on invalid input.
 */
int set_skill_level(string skill, float level) {
  mapping node;

  if(!stringp(skill) || nullp(level) || level < 1.0)
    return null;

  node = find_skill_node(skill);
  if(!node)
    return 0;

  node["level"] = level;
  return 1;
}

/**
 * Get a copy of the entire skill tree.
 *
 * @returns {mapping} A copy of the skills mapping.
 */
mapping query_skills() {
  return copy(__skills);
}

/**
 * Replace the skill tree wholesale. No-op when the argument is not
 * a mapping; the input is copied before being stored.
 *
 * @param {mapping} s - The skills mapping to install.
 */
void set_skills(mapping s) {
  if(!mapp(s))
    return;
  __skills = copy(s);
}

/**
 * Invoke a skill, granting it a 20% chance to improve. On a
 * successful roll, improve_skill() is called with the named skill
 * and mod_adjust as the random_float cap; weighted bubble-up still
 * runs, so the node that actually gains progress may be a parent
 * on the dot-path rather than the named leaf. If the skill does
 * not yet exist, it is created at level 1.0 via assure_skill() and
 * no improvement is rolled this call.
 *
 * @param {string} skill - The dot-path of the skill being used.
 * @param {mixed} [mod_adjust] - Upper bound for the per-call random
 *                               progress. Accepts a float, an int,
 *                               or a closure that evaluates to one
 *                               of those. Omit (or null) to fall
 *                               back to the 0.01 default in
 *                               improve_skill().
 * @returns {int | undefined} 1 if the roll fired this call, 0
 *                            otherwise, or null on invalid input.
 */
varargs int use_skill(string skill, mixed mod_adjust) {
  if(!stringp(skill))
    return null;

  float raw = query_raw_skill(skill);

  if(!nullp(raw)) {
    float chance_to_improve = 5.0+dim_hyperbolic(raw, 20.0);

    if(random_float(100.0) < chance_to_improve) {
      improve_skill(skill, mod_adjust);
      return 1;
    }
  } else {
    assure_skill(skill);
  }

  return 0;
}

/**
 * Split a dot-path into its components.
 *
 * @param {string} skill - A skill dot-path (e.g. "combat.melee").
 * @returns {string*} The path components.
 */
string *query_skill_path(string skill) {
  return explode(skill, ".");
}

/**
 * Apply progress to a skill, with weighted bubble-up.
 *
 * A target node is chosen from the skill's dot-path via a weighted
 * draw favouring leaves over roots (weights are (depth+1)*3, so
 * for a 3-segment path the leaf is picked 50% of the time, the
 * middle 33%, the root 17%). A single random_float draw against
 * the supplied cap is then added to the chosen node's level.
 * Parent skills therefore grow organically alongside the children
 * that are used, but more slowly because they are picked less
 * often. The cap is the same for every node on the path — bubble-
 * up balance lives entirely in the pick weights.
 *
 * The progress argument is the upper bound for the random draw,
 * not the literal amount applied. It accepts:
 *
 *   - omitted / null — defaults to 0.01.
 *   - float — used as-is.
 *   - int — promoted to float.
 *   - closure — evaluated against this_object(), then coerced.
 *
 * If the integer level of the chosen node rises as a result, a
 * notification is sent to the living object.
 *
 * @param {string} skill - The dot-path used to seed the weighted
 *                         walk.
 * @param {mixed} [potential_progress=0.01] - Upper bound for the
 *                            random roll. See description for
 *                            accepted forms.
 * @returns {float} The new raw level of the chosen node after
 *                  progress is applied, 0 if an intermediate is
 *                  missing, or null on invalid input.
 */
varargs float improve_skill(string skill, mixed potential_progress: (: 0.01 :)) {
  if(!stringp(skill))
    return null;

  string *path = explode(skill, ".");
  float progress;

  if(nullp(potential_progress))
    progress = 0.01;
  else if(valid_function(potential_progress))
    progress = evaluate(potential_progress, this_object());
  else if(intp(potential_progress))
    progress = to_float(potential_progress);
  else if(floatp(potential_progress))
    progress = potential_progress;

  if(!floatp(progress))
    return null;

  mapping chances = ([]);
  int sz = sizeof(path);
  int i = sz;

  while(i--) {
    string n = implode(path[0..i], ".");

    chances[n] = (i + 1) * 3;
  }

  string chosen = element_of_weighted(chances);

  mapping node = find_skill_node(chosen);
  if(!node)
    return 0;

  float level = query_raw_skill(chosen);
  node["level"] += random_float(progress);
  float new_level = query_raw_skill(chosen);

  if(floor(new_level) > floor(level))
    tell(this_object(), "{{9c6}}You have improved your {{re1}}" + chosen + "{{re0}} skill.{{res}}\n");

  return node["level"];
}

/**
 * Get the progress toward the next level for a skill.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {int} The fractional part of the level expressed as
 *                0-99, or null if the skill is not found or input
 *                is invalid.
 */
int query_skill_progress(string skill) {
  mapping node = find_skill_node(skill);
  float level;

  if(!node)
    return null;

  level = node["level"];
  return to_int((level - floor(level)) * 100.0);
}

/**
 * Set a skill's level to an integer value, replacing whatever was
 * stored.
 *
 * Unlike set_skill_level(), this accepts an int and does not
 * enforce a minimum level. Intermediate nodes must already exist —
 * this function will not create them.
 *
 * @param {string} skill - The dot-path of the skill.
 * @param {int} level - The new level value.
 * @returns {int} 1 on success, 0 if an intermediate is missing,
 *                null on invalid input.
 */
int modify_skill_level(string skill, int level) {
  mapping node;

  if(!stringp(skill) || nullp(level))
    return null;

  node = find_skill_node(skill);
  if(!node)
    return 0;

  node["level"] = level;
  return 1;
}

/**
 * Seed every NPC skill to level * 3.0 so that query_skill_level()
 * is honest for combat math. Called from npc.c::set_level() after
 * the level is updated.
 *
 * @param {float} level - The NPC's current level. Each skill is
 *                        stored as level * 3.0.
 * @returns {int} 1 if the skill tree was traversed, 0 if there were
 *                no skills to adjust.
 */
public int adjust_skills_by_npc_level(float level) {
  if(nullp(__skills) || !mapp(__skills))
    return 0;

  adjust_skill_levels(__skills, level);

  return 1;
}

/**
 * Recursively set every skill's level to level * 3.0. NPC-only —
 * errors out if called on a user.
 *
 * @param {mapping} current_skills - The skills submapping at the
 *                                   current recursion depth.
 * @returns {mapping} The same submapping, mutated in place.
 * @errors If invoked on a user (userp()).
 */
private nomask mapping adjust_skill_levels(mapping current_skills) {
  string sk;
  mixed skill_data;

  if(userp())
    error("This function is only intended for NPCs.");

  foreach(sk, skill_data in current_skills) {
    if(mapp(skill_data) && mapp(skill_data["subskills"]))
      adjust_skill_levels(skill_data["subskills"], level);

    current_skills[sk]["level"] = level * 3.0;
  }

  return current_skills;
}

/**
 * Ensure a skill exists. If it does not, create it at level 1.0
 * and notify the living object that they have gained a new skill.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {int} 1 if the skill exists (or was just created), 0
 *                if creation failed.
 */
int assure_skill(string skill) {
  if(!has_skill(skill)) {
    if(add_skill(skill, 1.0)) {
      tell(this_object(), "{{9c6}}You have gained a new skill: {{re1}}" + skill + "{{re0}}.{{res}}\n");
      return 1;
    } else {
      return 0;
    }
  }

  return 1;
}
