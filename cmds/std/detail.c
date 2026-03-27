/**
 * @file /cmds/std/detail.c
 *
 * This command is used for detailing your description and other
 * features of your character.
 *
 * @created 2024-07-30 - Gesslar
 * @last_modified 2024-07-30 - Gesslar
 *
 * @history
 * 2024-07-30 - Gesslar - Created
 */

#include <ed.h>

inherit STD_CMD;

void setup() {
    usage_text = "detail features - list the available features to detail\n"
                 "detail <feature> - detail a feature\n";
}

mixed main(object tp, string str) {
    mixed result;

    if(!str)
        return "Usage:\n" + usage_text;

    result = call_if(this_object(), sprintf("detail_%s", str), tp);

    return result || "Unknown feature.";
}

private void finish_description(int status, string file, string temp_file, object tp);
private mixed detail_description(object tp) {
    string prompt;

    prompt = "Enter your long description. You are limited to 800 characters\n\n";
    tell(tp, prompt);
    tp->start_edit(
        null,
        assemble_call_back((: finish_description :), tp)
    );

    return 1;
}

private void finish_description(int status, string file, string temp_file, object tp) {
    string text;

    defer((: rm, temp_file :));

    if(status == ED_STATUS_ABORTED || file_size(temp_file) < 1) {
        if(interactive(tp))
            _info(tp, "Description aborted.");
        return;
    }

    text = read_file(temp_file);

    tp->set_long(text);

    tell(tp, "Description updated.\n");
}
