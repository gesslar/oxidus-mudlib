/**
 * @file /std/living/skills.c
 * @description Trainable skills for living objects
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
 * @description Initialize missing skills from the given mapping
 * @param {mapping} skill_set - A mapping of skill names and their details
 */
/**
 * @description Initialize missing skills from the given mapping
 * @param {mapping} skill_set - A mapping of skill categories and skills
 * @param {string} curr_path - The current path in the skill hierarchy
 */
varargs void initialize_missing_skills(mapping skill_set, string curr_path) {
    string cat, skill;
    mixed element;

    if(!curr_path)
        curr_path = "";

    foreach(cat, element in skill_set){
        string full_path = curr_path != "" ? curr_path + "." + cat : cat;

        if(pointerp(element)){
            // Element is an array of skills
            foreach(skill in element){
                string skill_path = full_path + "." + skill;
                add_skill(skill_path, 1.0);
            }
        } else if(mapp(element)){
            // Element is a mapping of further subskills or categories
            initialize_missing_skills(element, full_path);
        } else {
            // Single skill entry (though this case might not occur in this structure)
            add_skill(full_path, 1.0);
        }
    }
}

/**
 * @description Add a skill to the living object
 * @param {string} skill - The name of the skill
 * @param {float} level - The level of the skill with fractional progress
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
        if(!mapp(current) || !current[path[x]] || !mapp(current[path[x]]["subskills"]))
            return 0;  // Unable to find
        current = current[path[x]]["subskills"];
    }

    if(!mapp(current) || !current[path[sz-1]])
        return 0;  // Unable to find

    map_delete(current, path[sz-1]);
    return 1;  // Success
}

/**
 * @description Get the level of a skill
 * @param {string} skill - The name of the skill
 * @returns {float} The level of the skill as an integer
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
 * @description Set the level of a skill
 * @param {string} skill - The name of the skill
 * @param {float} level - The level of the skill with fractional progress
 */
int set_skill_level(string skill, float level) {
    string *path = explode(skill, ".");
    mapping current = skills;
    int x, sz;

    if(!stringp(skill) || nullp(level) || level < 1.0)
        return null;

    sz = sizeof(path);
    for(x = 0; x < sz; x++) {
        if(!mapp(current[path[x]])) return 0;
        if(x == sz - 1) {
            current[path[x]]["level"] = level;
            return 1;
        }
        current = current[path[x]]["subskills"];
    }
    return null;
}

/**
 * @description Get the skills of the living object
 * @returns {mapping} The skills of the living object
 */

mapping query_skills() {
    return copy(skills);
}

/**
 * @description Set the skills of the living object
 * @param {mapping} skills - The skills of the living object
 */

void set_skills(mapping s) {
    if(!mapp(s)) return;
    skills = copy(s);
}

/**
 * @description Use a skill with a chance to improve it
 * @param {string} skill - The name of the skill
 * @param {object} target - The target of the skill
 * @returns {int} - 1 if the skill was improved, 0 otherwise
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
 * @description Get the path of a skill
 * @param {string} skill - The name of the skill
 * @returns {string*} The path of the skill
 */
string *query_skill_path(string skill) {
    return explode(skill, ".");
}

/**
 * @description Train a skill
 * @param {string} skill - The name of the skill
 * @param {float} progress - The fractional progress to add to the skill level
 */
varargs float improve_skill(string skill, float progress) {
    string *path = explode(skill, ".");
    mapping current = skills;
    int x, sz = sizeof(path);

    if(!stringp(skill))
        return null;

    // Automatic progress here and skill skill selection.
    // If you didn't pass a progress, it will automatically
    // pick a skill from a weighted distrbution among the
    // skill tree leading to the specified skill and apply
    // a random amount of progress to it.
    if(nullp(progress)) {
        mapping chances = ([]);
        string p;
        int i = sz;

        while(i--)
            chances[implode(path[0..i], ".")] = (i+1) * 3;

        skill = element_of_weighted(chances);
        progress = random_float(0.01);

        path = explode(skill, ".");
        sz = sizeof(path);
    }

    for(x = 0; x < sz; x++) {
        if(!mapp(current[path[x]]))
            return 0;
        if(x == sz - 1) {
            float level = query_skill_level(skill, 1), new_level;
            current[path[x]]["level"] += progress;
            new_level = query_skill_level(skill, 1);
            if(new_level > level)
                tell(this_object(), "You have improved your " + skill + " skill.\n");

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
        return null; // Return null for invalid input

    path = explode(skill, ".");
    sz = sizeof(path);

    for(x = 0; x < sz; x++) {
        if(!mapp(current[path[x]]))
            return null; // Return null if the skill path doesn't exist

        if(x == sz - 1) {
            level = current[path[x]]["level"];
            fractional_part = level - floor(level);
            return to_int(fractional_part * 100.0); // Convert fractional part to percentage
        }

        current = current[path[x]]["subskills"];
    }

    return null; // Return null if the skill was not found
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
 * @description Adjust all skills for NPCs based on their level.
 * @param {float} level - The level of the NPC.
 * @returns {int} 1 if adjustments were made, 0 otherwise.
 */
public int adjust_skills_by_npc_level(float level) {
    if(nullp(skills) || !mapp(skills)) {
        return 0; // No skills to adjust
    }

    adjust_skill_levels(skills);

    return 1;
}

/**
 * @description Helper function to adjust skill levels recursively.
 * @param {mapping} current_skills - The current mapping of skills to adjust.
 * @param {float} new_level - The new level to set for each skill.
 */
private nomask mapping adjust_skill_levels(mapping current_skills) {
    string skill;
    mixed skill_data;

    if(userp())
        error("This function is only intended for NPCs.");

    foreach(skill, skill_data in current_skills) {
        if(mapp(skill_data) && mapp(skill_data["subskills"])) {
            // If there are subskills, recurse into them
            adjust_skill_levels(skill_data["subskills"]);
        }

        // Set the skill level
        current_skills[skill]["level"] = random_float(0.01);
    }

    return current_skills;
}

/**
 * @description Assure that an entire skill path exists. If not, create it and
 * set the level to 1.0.
 * @param {string} skill - The name of the skill
 * @returns {int} - 1 if the skill was assured, 0 otherwise
 */
int assure_skill(string skill) {
    if(nullp(query_skill(skill))) {
        if(add_skill(skill, 1.0)) {
            tell(this_object(), "You have gained a new skill: " + skill + ".\n");
            return 1;
        } else
            return 0;
    }

    return 1;
}
