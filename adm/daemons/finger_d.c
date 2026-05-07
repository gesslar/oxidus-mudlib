/* finger_d.c

 Tacitus @ LPUniversity
 26-MAR-06
 Finger Daemon

*/

//Last edited on October 6th by Tacitus

/*

 The finger daemon provides an interface to load user objects
 to gather information.

 */

#include <socket.h>
#include <origin.h>

inherit STD_DAEMON;

#define BORDER1 "=+=--=+=--=+=--=+=--=+=--=--=+=--=+=--=+=--=+=--=+=--=+=--=+=--=+=--=+=--=+=\n"

mixed get_body(string name);

varargs string finger_user(string username) {
    string ret, group, *users;
    string rank, last_t, last, idle, plan;
    int idle_time;
    /** @type {STD_BODY} */ mixed body;

    username = lower_case(username);

    if(sscanf(username, "(%s)", group)) {
        users = master()->query_group(group);

        if(!pointerp(users) || sizeof(users) <= 0)
            return "Error: Group '" + group + "' does not exist or has no members.\n";

        ret = BORDER1;
        ret += "\t\tGROUP: " + group + "\n";
        ret += BORDER1;
        ret += sprintf("%-20s %-40s %s\n", "Login", "Rank", "On");
        ret += BORDER1;

        foreach(username in users) {
            if(sscanf(username, "(%*s)")) ret += sprintf("%-20s %-40s %s\n", username, "(GROUP)", "---");
            else {
                if(find_player(username)) {
                    last_t = "On Since";
                    body = find_player(username);
                    last = ctime(body->query_last_login());
                } else {
                    if(!objectp(body = get_body(username)))
                        continue;
                    else {
                        last_t = "Last on";
                        last =  ctime(body->query_last_login());
                        body->remove();
                    }
                }
                if(adminp(username)) rank = "Admin";
                else if(devp(username)) rank = "Developer";
                else rank = "User";

                ret += sprintf("%-20s %-40s %s\n", capitalize(username), rank, last_t + " " + last);
            }
        }

        return ret += BORDER1;
    } else {
        if(find_player(username)) {
            last_t = "On since";
            body = find_player(username);
            if(!interactive(body))
                idle = "(Link-Dead)";
            else {
                if(query_idle(body)) {
                    idle_time = query_idle(body)/60;
                    if(idle_time > 0)
                        idle = "(Idle: " + idle_time + ")";
                    else
                        idle = "";
                }
                else idle = "";
            }
        } else {
            last_t = "Last on";
            idle = "(Offline)";
            if(!objectp(body = get_body(username))) {
                return "Error [finger]: User data unavailable.\n";
            }
        }

        last = ctime((body->query_last_login()));

        if(adminp(username)) rank = "Admin";
        else if(devp(username)) rank = "Developer";
        else rank = "User";

        plan = home_path(username) + ".plan";
        if(!file_exists(plan))
            plan = "This user has no plan.\n";
        else
            plan = read_file(plan);

        ret = sprintf("\n"
            "Username: %-10s \tRank: %-10s\n" +
            "E-mail: %-10s\n"
            "%s: %-10s %s\nPlan:\n%s", capitalize(username) + "", rank, "user@user.com", last_t, last, idle, plan);
    }
    return ret;
}

mixed get_body(string name) {
    object body;
    string error;

    if(origin() != ORIGIN_LOCAL && !adminp(query_privs(previous_object()))) return 0;

    error = catch(body = BODY_D->create_body(name));
    if(error)
      return -1;

    if(!body)
      return -2;

    return body;
}
