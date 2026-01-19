/**
 * @file /std/living/attributes.c
 * @description Player attributes.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */

#include <attributes.h>
#include <boon.h>

private nomask nosave string *default_attributes = ({});
private nomask mapping attributes = ([]);

void init_attributes() {
    string key;
    mixed data;

    default_attributes = mudConfig("ATTRIBUTES");

    attributes = attributes || ([]);

    foreach(key in default_attributes) {
        if(!of(key, attributes)) {
            attributes[key] = 5;
        }
    }

    foreach(key, data in attributes) {
        if(!of(key, default_attributes)) {
            map_delete(attributes, key);
        }
    }
}

int set_attribute(string key, int value) {
    if(!of(key, attributes)) {
        return null;
    }

    attributes[key] = value;

    return attributes[key];
}

varargs int query_attribute(string key, int raw) {
    if(!of(key, attributes)) {
        return null;
    }

    if(raw)
        return attributes[key];

    return attributes[key] + query_effective_boon("attribute", key);
}

int modify_attribute(string key, int value) {
    if(!of(key, attributes)) {
        return null;
    }

    attributes[key] += value;

    return attributes[key];
}

mapping query_attributes() {
    return copy(attributes);
}
