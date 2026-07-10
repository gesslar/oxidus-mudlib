// /cmds/wiz/mudinfo.c
// Mud information
//
// Created:     2024/02/04: Gesslar
// Last Change: 2024/02/04: Gesslar
//
// 2024/02/04: Gesslar - Created

inherit STD_CMD;

mixed main(object tp, string arg) {
    object *obs, *tmp;
    string out = "";

    out += "Mud Name: " + mud_name() + "\n";
    out += "Boot Number: " + BOOT_D->query_boot_number() + "\n";
    out += "Objects: " + sizeof(obs = objects()) + "\n";

    tmp = filter(obs, (: virtualp :));
    out += "Virtual: " + sizeof(tmp) + "\n";
    tmp = filter(obs, (: environment($1) && living($1) :));
    out += "Living: " + sizeof(tmp) + "\n";
    tmp = filter(obs, (: userp :));
    out += "Players: " + sizeof(tmp) + "\n";
    tmp = filter(obs, (: environment($1) && living($1) && !userp($1) :));
    out += "Non-Players: " + sizeof(tmp) + "\n";
    tmp = filter(obs, (: $1->is_room() :));
    out += "Rooms: " + sizeof(tmp) + "\n";

    return out;
}
