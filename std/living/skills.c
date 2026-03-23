/**
 * @file /std/living/skills.c
 * Trainable skills for living objects
 *
 * @created 2024-07-31 - Gesslar
 * @last_modified 2024-07-31 - Gesslar
 *
 * @history
 * 2024-07-31 - Gesslar - Created
 */

#include <skills.h>
#include <advancement.h>
#include <boon.h>
#include <npc.h>

private nomask mapping skills = ([]);

void wipe_skills() {
  skills = ([]);
}

/**
 * Initialise missing skills from the given mapping.
 * @param {mapping} skill_set - A mapping of skill categories and skills
 * @param {string} curr_path - The current path in the skill hierarchy
 */
varargs void initialize_missing_skills(mapping skill_set,
    string curr_path) {
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
 * Add a skill to the living object.
 * @param {string} skill - The name of the skill
 * @param {float} level - The level of the skill with fractional
 *   progress
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
 * Get the level of a skill.
 * @param {string} skill - The name of the skill
 * @returns {float} The raw float level of the skill. For NPCs,
 *   returns queryLevel() * 3.0.
 */
float query_skill(string skill) {
  string *path = explode(skill, ".");
  mapping current = skills;
  int x, sz;

  if(!stringp(skill))
    return null;

  if(function_exists("is_npc") && is_npc())
    return queryLevel() * 3.0;

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
 * Get the floored level of a skill, optionally
 *   including boon modifiers.
 * @param {string} skill - The name of the skill
 * @param {int} raw - If true, return without boon modifiers
 * @returns {float} The floored skill level
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
 * Set the level of a skill.
 * @param {string} skill - The name of the skill
 * @param {float} level - The level of the skill with fractional
 *   progress
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
 * Get the skills of the living object.
 * @returns {mapping} The skills of the living object
 */
mapping query_skills() {
  return copy(skills);
}

/**
 * Set the skills of the living object.
 * @param {mapping} s - The skills mapping to set
 */
void set_skills(mapping s) {
  if(!mapp(s))
    return;
  skills = copy(s);
}

/**
 * Use a skill with a chance to improve it.
 * @param {string} skill - The name of the skill
 * @returns {int} 1 if the skill was improved, 0 otherwise
 */
int use_skill(string skill) {
  float chance_to_improve = 20.0;

  if(!stringp(skill))
    return null;

  if(query_skill(skill)) {
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
 * Get the path of a skill.
 * @param {string} skill - The name of the skill
 * @returns {string*} The path of the skill
 */
string *query_skill_path(string skill) {
  return explode(skill, ".");
}

/**
 * Train a skill. If progress is null, automatically
 *   selects a skill from a weighted distribution among the skill
 *   tree path and applies a random amount of progress.
 * @param {string} skill - The name of the skill
 * @param {float} progress - The fractional progress to add
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
        tell(this_object(),
          "You have improved your " + skill + " skill.\n");

      return progress;
    }
    current = current[path[x]]["subskills"];
  }

  return null;
}

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
 * Adjust all skills for NPCs based on their level.
 * @param {float} _level - The NPC's level (unused; skills are
 *   reset to near-zero regardless)
 * @returns {int} 1 if adjustments were made, 0 otherwise
 */
public int adjust_skills_by_npc_level(float _level) {
  if(nullp(skills) || !mapp(skills))
    return 0;

  adjust_skill_levels(skills);

  return 1;
}

/**
 * Helper to recursively reset NPC skill levels.
 * @param {mapping} current_skills - The current mapping of skills
 *   to adjust
 */
private nomask mapping adjust_skill_levels(
    mapping current_skills) {
  string sk;
  mixed skill_data;

  if(userp())
    error("This function is only intended for NPCs.");

  foreach(sk, skill_data in current_skills) {
    if(mapp(skill_data) && mapp(skill_data["subskills"]))
      adjust_skill_levels(skill_data["subskills"]);

    current_skills[sk]["level"] = random_float(0.01);
  }

  return current_skills;
}

/**
 * Assure that an entire skill path exists. If not,
 *   create it and set the level to 1.0.
 * @param {string} skill - The name of the skill
 * @returns {int} 1 if the skill was assured, 0 otherwise
 */
int assure_skill(string skill) {
  if(nullp(query_skill(skill))) {
    if(add_skill(skill, 1.0)) {
      tell(this_object(),
        "You have gained a new skill: " + skill + ".\n");
      return 1;
    } else {
      return 0;
    }
  }

  return 1;
}
