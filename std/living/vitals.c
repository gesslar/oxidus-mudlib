/**
 * @file /std/user/vitals.c
 * @description Vitals for livings
 *
 * @created 2024-07-24 - Gesslar
 * @last_modified 2024-07-24 - Gesslar
 *
 * @history
 * 2024-07-24 - Gesslar - Created
 */

#include <gmcp_defines.h>
#include <vitals.h>
#include <runtime_config.h>
#include <boon.h>
#include <module.h>
#include <combat.h>

void update_regen_interval();

private nomask float hp, max_hp, sp, max_sp, mp, max_mp;
private nomask int dead = false;
private nomask nosave int tick;
private nomask nosave int regen_interval_pulses; // Number of pulses to trigger a regen

void init_vitals() {
    hp = hp || 100.0;
    max_hp = max_hp || 100.0;
    sp = sp || 100.0;
    max_sp = max_sp || 100.0;
    mp = mp || 100.0;
    max_mp = max_mp || 100.0;

    update_regen_interval(); // Calculate regen interval based on the product of HEART_PULSE and HEARTBEATS_TO_REGEN
}

float query_hp() { return hp ; }
varargs float query_max_hp(int raw) {
    if(raw)
        return max_hp;

    return max_hp + query_effective_boon("vital", "max_hp");
}
float hp_ratio() { return percent(hp, query_max_hp()) ; }

float query_sp() { return sp ; }
varargs float query_max_sp(int raw) {
    if(raw)
        return max_sp;

    return max_sp + query_effective_boon("vital", "max_sp");
}
float sp_ratio() { return percent(sp, query_max_sp()) ; }

float query_mp() { return mp ; }
varargs float query_max_mp(int raw) {
    if(raw)
        return max_mp;

    return max_mp + query_effective_boon("vital", "max_mp");
}
float mp_ratio() { return percent(mp, query_max_mp()) ; }

void set_hp(float x) {
    hp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", hp),
    ]));
}
void set_max_hp(float x) {
    max_hp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MAX_HP: sprintf("%.2f", max_hp),
    ]));
}

void set_sp(float x) {
    sp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", sp),
    ]));
}

void set_max_sp(float x) {
    max_sp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MAX_SP: sprintf("%.2f", max_sp),
    ]));
}

void set_mp(float x) {
    mp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", mp),
    ]));
}

void set_max_mp(float x) {
    max_mp = to_float(x);

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MAX_MP: sprintf("%.2f", max_mp),
    ]));
}

float adjust_hp(float x) {
    hp += to_float(x);
    if(hp > max_hp)
        hp = max_hp;

    if(hp <= 0.0) {
        hp = 0.0;
    }

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", hp),
    ]));

    return hp;
}

float adjust_max_hp(float x) {
    max_hp += to_float(x);

    if(max_hp < 0.0)
        max_hp = 0.0;

    if(hp > max_hp) {
        hp = max_hp;
    }

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_HP: sprintf("%.2f", hp),
        GMCP_LBL_CHAR_VITALS_MAX_HP: sprintf("%.2f", max_hp),
    ]));

    return max_hp;
}

float adjust_sp(float x) {
    sp += to_float(x);
    if(sp > max_sp)
        sp = max_sp;

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", sp),
    ]));

    return sp;
}

float adjust_max_sp(float x) {
    max_sp += to_float(x);

    if(max_sp < 0.0)
        max_sp = 0.0;

    if(sp > max_sp) {
        sp = max_sp;
    }

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_SP: sprintf("%.2f", sp),
        GMCP_LBL_CHAR_VITALS_MAX_SP: sprintf("%.2f", max_sp),
    ]));

    return max_sp;
}

float adjust_mp(float x) {
    mp += to_float(x);
    if(mp > max_mp)
        mp = max_mp;

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", mp),
    ]));
    return mp;
}

float adjust_max_mp(float x) {
    max_mp += to_float(x);

    if(max_mp < 0.0)
        max_mp = 0.0;

    if(mp > max_mp) {
        mp = max_mp;
    }

    GMCP_D->send_gmcp(this_object(), GMCP_PKG_CHAR_VITALS, ([
        GMCP_LBL_CHAR_VITALS_MP: sprintf("%.2f", mp),
        GMCP_LBL_CHAR_VITALS_MAX_MP: sprintf("%.2f", max_mp),
    ]));

    return max_mp;
}

protected void heal_tick(int force: (: 0 :)) {
    mapping rate = module("race", "query_regen_rate");

    if(nullp(rate))
        return;

    if(in_combat())
        return;

    if(++tick >= regen_interval_pulses  || force) {
        if(!force)
            tick = 0;

        if(hp < max_hp)
            adjust_hp(rate["hp"]);
        if(sp < max_sp)
            adjust_sp(rate["sp"]);
        if(mp < max_mp)
            adjust_mp(rate["mp"]);
    }
}

int set_heart_rate(int x) {
    if(x < 5)
        x = 5;
    else if(x > 100)
        x = 100;

    set_heart_beat(x);

    return x;
}

int add_heart_rate(int x) {
    return set_heart_rate(query_heart_beat() + x);
}

// This function calculates the number of pulses needed based on HEART_PULSE and HEARTBEATS_TO_REGEN
void update_regen_interval() {
    // Calculate the number of pulses for the regen interval
    regen_interval_pulses = to_int((mudConfig("HEART_PULSE") * mudConfig("HEARTBEATS_TO_REGEN")) / 1000.0); // Convert ms to seconds
    tick = 0;
}

// This function initializes the healing process
void initialize_healing(object character) {
    update_regen_interval(); // Calculate regen interval based on the product of HEART_PULSE and HEARTBEATS_TO_REGEN
}

int query_heart_rate() {
    return query_heart_beat();
}

int query_regen_duration() {
    return regen_interval_pulses;
}

void restore() {
    set_hp(max_hp);
    set_sp(max_sp);
    set_mp(max_mp);
}

int set_dead(int x) {
    dead = !!x;
    return dead;
}

float *query_condition() {
    return ({
        hp_ratio(),
        sp_ratio(),
        mp_ratio(),
    });
}

string *query_condition_string() {
    string *result = allocate(3);
    float *ratio = query_condition();

    // HP Condition
    if(ratio[0] <= 0.0)
        result[0] = "dead";
    else if(ratio[0] <= 10.0)
        result[0] = "critical";
    else if(ratio[0] <= 30.0)
        result[0] = "severely injured";
    else if(ratio[0] <= 45.0)
        result[0] = "moderately injured";
    else if(ratio[0] <= 60.0)
        result[0] = "injured";
    else if(ratio[0] <= 75.0)
        result[0] = "hurt";
    else if(ratio[0] <= 90.0)
        result[0] = "wounded";
    else if(ratio[0] < 100.0)
        result[0] = "bruised and nicked";
    else
        result[0] = "healthy";

    // SP Condition
    if(ratio[1] <= 5.0)
        result[1] = "brain dead";
    else if(ratio[1] <= 15.5)
        result[1] = "depleted";
    else if(ratio[1] <= 30.0)
        result[1] = "unfocused";
    else if(ratio[1] <= 45.0)
        result[1] = "mentally fuzzy";
    else if(ratio[1] <= 60.0)
        result[1] = "losing focus";
    else if(ratio[1] <= 75.0)
        result[1] = "clear-headed";
    else if(ratio[1] <= 90.0)
        result[1] = "sharp";
    else if(ratio[1] < 100.0)
        result[1] = "focused";
    else
        result[1] = "fully charged";

    // MP Condition
    if(ratio[2] <= 5.0)
        result[2] = "exhausted";
    else if(ratio[2] <= 15.5)
        result[2] = "sluggish";
    else if(ratio[2] <= 30.0)
        result[2] = "fatigued";
    else if(ratio[2] <= 45.0)
        result[2] = "tired";
    else if(ratio[2] <= 60.0)
        result[2] = "somewhat tired";
    else if(ratio[2] <= 75.0)
        result[2] = "lively";
    else if(ratio[2] <= 90.0)
        result[2] = "energetic";
    else if(ratio[2] < 100.0)
        result[2] = "very lively";
    else
        result[2] = "full of stamina";

    return result;
}

int is_dead() { return dead ; }
