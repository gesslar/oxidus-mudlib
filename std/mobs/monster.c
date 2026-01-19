/**
 * @file /std/mobs/monster.c
 * @description Standard monster inheritable for all types of monsters
 *
 * @created 2024-08-20 - Gesslar
 * @last_modified 2024-08-20 - Gesslar
 *
 * @history
 * 2024-08-20 - Gesslar - Created
 */

inherit STD_NPC;

void virtual_setup(mixed args...) {
    mapping data;

    if(!args || !mapp(args[0]))
        return;

    data = args[0];

    // Set basic properties
    set_name(data["name"]);
    set_short(data["short"]);
    set_long(data["long"]);
    set_id(data["id"]);

    // Set level
    if(nullp(data["level"]))
        setLevel(1);
    else if(intp(data["level"]))
        setLevel(data["level"]);
    else if(pointerp(data["level"]))
        setLevel(to_float(random(data["level"][1] - data["level"][0]) + data["level"][0]));
    else
        setLevel(1);

    // Set gender
    if(!nullp(data["gender"])) {
        if(stringp(data["gender"]))
            set_gender(data["gender"]);
        else if(pointerp(data["gender"]))
            set_gender(element_of(data["gender"]));
        else if(mapp(data["gender"]))
            set_gender(element_of_weighted(data["gender"]));
    }

    // Set combat-related properties
    if(!nullp(data["damage"]))
        set_damage(data["damage"]);
    if(!nullp(data["weapon_name"]))
        set_weapon_name(data["weapon_name"]);
    if(!nullp(data["weapon_type"]))
        set_weapon_type(data["weapon_type"]);

    // Set race
    if(!nullp(data["race"]))
        set_race(data["race"]);

    // Handle loot
    if(!nullp(data["loot"])) {
        if(mapp(data["loot"])) {
            foreach(mixed item, float chance in data["loot"]) {
                add_loot(item, chance);
            }
        } else if(pointerp(data["loot"])) {
            float default_chance = data["loot_chance"] || 50.0;
            foreach(mixed item in data["loot"]) {
                if(pointerp(item) && sizeof(item) == 2) {
                    add_loot(item[0], item[1]);
                } else {
                    add_loot(item, default_chance);
                }
            }
        }
    }

    // Handle coins
    if(!nullp(data["coins"])) {
        foreach(string coin_type, mixed coin_data in data["coins"]) {
            if(pointerp(coin_data) && sizeof(coin_data) == 2)
                add_coin(coin_type, coin_data[0], coin_data[1]);
        }
    }

    call_if(this_object(), "monster_setup", data);
}
