#ifndef __VITALS_H__
#define __VITALS_H__

void init_vitals();
float query_hp();
varargs float query_max_hp(int raw);
float hp_ratio();
float query_sp();
varargs float query_max_sp(int raw);
float sp_ratio();
float query_mp();
varargs float query_max_mp(int raw);
float mp_ratio();
void set_hp(float x);
void set_max_hp(float x);
void set_sp(float x);
void set_max_sp(float x);
void set_mp(float x);
void set_max_mp(float x);
float adjust_hp(float x);
float adjust_sp(float x);
float adjust_mp(float x);
protected varargs void heal_tick(int force);
int set_heart_rate(int x);
int add_heart_rate(int x);
void update_regen_interval();
void initialize_healing();
int query_heart_rate();
int query_regen_duration();
void restore();
int set_dead(int x);
float *query_condition();
string *query_condition_string();
int is_dead();

#endif // __VITALS_H__
