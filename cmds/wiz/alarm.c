// /cmds/wiz/alarms.c
// Interface to see alarms
//
// Created:     2024/02/25: Gesslar
// Last Change: 2024/02/25: Gesslar
//
// 2024/02/25: Gesslar - Created

#include <daemons.h>
#include <classes.h>

inherit STD_CMD;
inherit CLASS_ALARM;

mixed main(object _tp, string arg) {
    class Alarm *alarms = ALARM_D->query_alarms();
    class Alarm *boots;
    string *out;
    mixed *info;
    int sz;
    int next_poll;

    if(arg == "reload") {
        ALARM_D->reloadAlarms();
        return "Alarms reloaded.";
    }

    sz = sizeof(alarms);
    info = allocate(sz);
    while(sz--) {
        info[sz] = ({
            alarms[sz],
            ALARM_D->calculateAlarmTime(alarms[sz], 1)
        });
    }

    info = sort_array(info, (:
        $2[1] - $1[1]
    :) );

    boots = filter(info, (:
        $1[0].type == "B"
    :));
    info -= boots;
    info = boots + info;

    out = ({
        "Type File->Function                                      Next Time",
        "---- --------------------------------------------------- -------------------"
    });
    if(arg == "time") {
        out += map(info, (:
            sprintf("%|4s %-51s %-19s",
                $1[0].type,
                $1[0].file+"->"+$1[0].func,
                $1[0].type == "B" ? "" : ctime($1[1])
            )
        :));
    } else {
        out += map(info, (:
            sprintf("%|4s %-51s %18s",
                $1[0].type,
                $1[0].file+"->"+$1[0].func,
                $1[0].type == "B" ? sprintf("boot+%ss", $1[0].pattern) : sprintf("%ds", $1[1] - time())
            )
        :));
    }

    next_poll = ALARM_D->time_to_next_poll();
    out += ({ "", sprintf(sprintf("%|80s",
                    sprintf("Next poll in %d seconds.", next_poll)
    )) });

    return out;
}

string help() {
    return
"Syntax: alarms [reload|time]\n\n"
"This command will display all the alarms that are currently set. The alarms "
"are sorted in the reverse order they will fire. The next time is displayed "
"in the seconds until the alarm will fire. If you want to see the next time "
"in a human readable format, use the command 'alarms time'.\n\n"
"If you have modified any alarms and wanted to reload them, use the command "
"'alarms reload'.";
}
