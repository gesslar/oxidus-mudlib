/**
 * @file /std/living/env.c
 * Environment variables and preferences, etc.
 *
 * @created 2024-08-17 - Gesslar
 * @last_modified 2024-08-17 - Gesslar
 *
 * @history
 * 2024-08-17 - Gesslar - Created
 */

#include <env.h>
#include <player.h>

private mapping env_settings = ([]);
private mapping preferences = ([]);

void init_env() {
    if(nullp(env_settings))
        env_settings = ([]);
    if(nullp(preferences))
        preferences = ([]);
}

int set_env(string var_name, string var_value) {
    if(!env_settings)
        init_env();

    if(!var_value) map_delete(env_settings, var_name);
    else env_settings += ([var_name : var_value]);

    if(userp(this_object()))
        this_object()->save_body();

    return 1;
}

varargs mixed query_env(string var_name, mixed def) {
    if(!env_settings)
        init_env();

    if(env_settings[var_name]) return env_settings[var_name];
    else return def;
}

mapping list_env() {
    return copy(env_settings);
}

int set_pref(string pref_name, string pref_value) {
    if(!preferences)
        init_env();

    if(!pref_value) map_delete(preferences, pref_name);
    else preferences += ([pref_name : pref_value]);

    if(userp(this_object()))
        this_object()->save_body();

    return 1;
}

varargs string query_pref(string pref_name, string def) {
    if(!preferences)
        init_env();

    if(preferences[pref_name]) return preferences[pref_name];
    else return def;
}

mapping list_pref() {
    return copy(preferences);
}
