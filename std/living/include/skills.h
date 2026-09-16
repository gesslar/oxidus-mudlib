#ifndef __SKILLS_H__
#define __SKILLS_H__

public void wipe_skills();
public varargs void initialize_missing_skills(mapping skill_set, string curr_path);
public varargs int add_skill(string skill, float level);
public int remove_skill(string skill);
private nomask mapping find_skill_node(string skill);
public float query_raw_skill(string skill);
public float query_skill(string skill);
public float query_skill_level(string skill);
public float query_raw_skill_level(string skill);
public int has_skill(string skill);
public int set_skill_level(string skill, float level);
public mapping query_skills();
public void set_skills(mapping s);
public varargs int use_skill(string skill, mixed improvement, mixed improvement_chance);
private float clamp_improvement(string skill_name, float improvement);
public string *query_skill_path(string skill);
private string determine_skill_to_improve(string skill_name, float skill_cap);
public varargs float improve_skill(string skill_name, mixed potential_progress);
public int query_skill_progress(string skill);
public int modify_skill_level(string skill, int level);
public int adjust_skills_by_npc_level(float level);
private nomask mapping adjust_skill_levels(mapping current_skills, float level);
public int assure_skill(string skill);

#endif // __SKILLS_H__
