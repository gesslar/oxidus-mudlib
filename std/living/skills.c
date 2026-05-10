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
 * Improvement is use-based: callers invoke use_skill() and a small,
 * randomised amount of progress is distributed along the skill's path
 * from root to leaf. NPCs are seeded at setup with every skill set to
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
 *                        no-floor / no-boon semantics.
 */

#include <skills.h>
#include <advancement.h>
#include <boon.h>
#include <npc.h>

private nomask mapping skills = ([]);

/**
 * Reset the skill tree to an empty mapping, discarding all skills
 * and progress.
 */
void wipe_skills() {
  skills = ([]);
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
  mapping current = skills;
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
  mapping current = skills;
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
 * Get the raw float level of a skill — no flooring, no boon
 * modifier. Use query_skill_level() for combat math.
 *
 * @param {string} skill - The dot-path of the skill.
 * @returns {float} The raw float level, or null if the skill is not
 *                  found or input is invalid.
 */
float query_raw_skill(string skill) {
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz;

  if(!stringp(skill))
    return null;

  sz = sizeof(path);
  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return null;
    if(x == sz - 1)
      return current[path[x]]["level"];
    current = current[path[x]]["subskills"];
  }

  return null;
}

/**
 * Get the floored level of a skill, optionally including boon
 * modifiers.
 *
 * Combat formulas use this function rather than query_raw_skill()
 * because the boon modifier is applied here.
 *
 * @param {string} skill - The dot-path of the skill.
 * @param {int} [raw] - If truthy, omit boon modifiers.
 * @returns {float} The floored level (with the boon modifier applied
 *                  unless raw is set), or null if the skill is not
 *                  found or input is invalid.
 */
varargs float query_skill_level(string skill, int raw) {
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz;

  if(!stringp(skill))
    return null;

  sz = sizeof(path);
  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return null;

    if(x == sz - 1) {
      float lvl = floor(current[path[x]]["level"]);
      if(raw)
        return lvl;
      else
        return lvl + query_effective_boon("skill", skill);
    }

    current = current[path[x]]["subskills"];
  }

  return null;
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
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz;

  if(!stringp(skill) || nullp(level) || level < 1.0)
    return null;

  sz = sizeof(path);
  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return 0;
    if(x == sz - 1) {
      current[path[x]]["level"] = level;
      return 1;
    }
    current = current[path[x]]["subskills"];
  }

  return null;
}

/**
 * Get a copy of the entire skill tree.
 *
 * @returns {mapping} A copy of the skills mapping.
 */
mapping query_skills() {
  return copy(skills);
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
  skills = copy(s);
}

/**
 * Invoke a skill, granting it a 20% chance to improve. If the
 * skill does not yet exist, it is created at level 1.0 via
 * assure_skill() and no improvement is rolled this call.
 *
 * @param {string} skill - The dot-path of the skill being used.
 * @returns {int} 1 if the skill improved this call, 0 otherwise,
 *                null on invalid input.
 */
int use_skill(string skill) {
  float chance_to_improve = 20.0;

  if(!stringp(skill))
    return null;

  if(query_raw_skill(skill)) {
    if(random_float(100.0) < chance_to_improve) {
      improve_skill(skill);
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
 * Apply progress to a skill.
 *
 * When progress is omitted (the standard use_skill() path), a
 * random skill from the dot-path is chosen via a weighted
 * distribution that favours leaves over roots, and a tiny progress
 * amount (random_float(0.01)) is applied to it. Parent skills
 * therefore grow organically as their children are used.
 *
 * When progress is supplied, it is applied directly to the named
 * skill without weighted path selection. If the integer level rises
 * as a result, a notification is sent to the living object.
 *
 * @param {string} skill - The dot-path of the skill.
 * @param {float} [progress] - Fractional progress to add. Omit to
 *                             use the weighted random path.
 * @returns {float} The progress actually applied, 0 if an
 *                  intermediate is missing, or null on invalid
 *                  input.
 */
varargs float improve_skill(string skill, float progress) {
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz = sizeof(path);

  if(!stringp(skill))
    return null;

  if(nullp(progress)) {
    mapping chances = ([]);
    int i = sz;

    while(i--)
      chances[implode(path[0..i], ".")] = (i + 1) * 3;

    skill = element_of_weighted(chances);
    progress = random_float(0.01);

    path = explode(skill, ".");
    sz = sizeof(path);
  }

  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return 0;

    if(x == sz - 1) {
      float level = query_skill_level(skill, 1);
      float new_level;

      current[path[x]]["level"] += progress;
      new_level = query_skill_level(skill, 1);

      if(new_level > level)
        tell(this_object(), "{{9c6}}You have improved your {{re1}}" + skill + "{{re0}} skill.{{res}}\n");

      return progress;
    }
    current = current[path[x]]["subskills"];
  }

  return null;
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
  string *path;
  mapping current = skills;
  int x, sz;
  float level, fractional_part;

  if(!stringp(skill))
    return null;

  path = explode(skill, ".");
  sz = sizeof(path);

  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return null;

    if(x == sz - 1) {
      level = current[path[x]]["level"];
      fractional_part = level - floor(level);
      return to_int(fractional_part * 100.0);
    }

    current = current[path[x]]["subskills"];
  }

  return null;
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
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz;

  if(!stringp(skill) || nullp(level))
    return null;

  sz = sizeof(path);
  for(x = 0; x < sz; x++) {
    if(!mapp(current[path[x]]))
      return 0;
    if(x == sz - 1) {
      current[path[x]]["level"] = level;
      return 1;
    }
    current = current[path[x]]["subskills"];
  }

  return null;
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
  if(nullp(skills) || !mapp(skills))
    return 0;

  adjust_skill_levels(skills, level);

  return 1;
}

/**
 * Recursively set every skill's level to level * 3.0. NPC-only —
 * errors out if called on a user.
 *
 * @param {mapping} current_skills - The skills submapping at the
 *                                   current recursion depth.
 * @param {float} level - The NPC's current level.
 * @returns {mapping} The same submapping, mutated in place.
 * @errors If invoked on a user (userp()).
 */
private nomask mapping adjust_skill_levels(mapping current_skills, float level) {
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
  if(nullp(query_raw_skill(skill))) {
    if(add_skill(skill, 1.0)) {
      tell(this_object(), "{{9c6}}You have gained a new skill: {{re1}}" + skill + "{{re0}}.{{res}}\n");
      return 1;
    } else {
      return 0;
    }
  }

  return 1;
}
