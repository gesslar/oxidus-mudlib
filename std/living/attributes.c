/**
 * @file /std/living/attributes.c
 * Player attributes.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */

#include <attributes.h>
#include <boon.h>

private nomask nosave string *__default_attributes = ({});
private nomask mapping __attributes = ([]);

void init_attributes() {
    string key;
    mixed data;

    __default_attributes = mud_config("ATTRIBUTES");

    __attributes ??= ([]);

    foreach(key in __default_attributes) {
        if(!of(key, __attributes)) {
            __attributes[key] = 5;
        }
    }

    foreach(key, data in __attributes) {
        if(!of(key, __default_attributes)) {
            map_delete(__attributes, key);
        }
    }
}

int set_attribute(string key, int value) {
    if(!of(key, __attributes)) {
        return null;
    }

    __attributes[key] = value;

    return __attributes[key];
}

varargs int query_attribute(string key, int raw) {
    if(!of(key, __attributes)) {
        return null;
    }

    if(raw)
        return __attributes[key];

    return __attributes[key] + query_effective_boon("attribute", key);
}

int modify_attribute(string key, int value) {
    if(!of(key, __attributes)) {
        return null;
    }

    __attributes[key] += value;

    return __attributes[key];
}

mapping query_attributes() {
    return copy(__attributes);
}
