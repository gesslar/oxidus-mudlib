#ifndef __SKILLS_H__
#define __SKILLS_H__

void wipe_skills() ;
varargs void initialize_missing_skills(mapping skill_set, string curr_path) ;
varargs int add_skill(string skill, float level) ;
int remove_skill(string skill) ;
float query_skill(string skill) ;
varargs float query_skill_level(string skill, int raw) ;
int set_skill_level(string skill, float level) ;
mapping query_skills() ;
void set_skills(mapping s) ;
varargs float improve_skill(string skill, float progress) ;
int query_skill_progress(string skill) ;
int modify_skill_level(string skill, int level) ;
int adjust_skills_by_npc_level(float _level) ;
private nomask mapping adjust_skill_levels(mapping current_skills) ;
string *query_skill_path(string skill) ;
int use_skill(string skill) ;
int assure_skill(string skill) ;

#endif // __SKILLS_H__
