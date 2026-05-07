//emote_client.c

//Parthenon @ LPUniversity
//25-JUN-06
//LPUni emote interface client

//Last updated: July 16th, 2006 by Tacitus



#define PROMPT "Emote Editor > "
#define MARKERS ({ \
                  "$I - Emoter", "$T - Target", \
                  "$PI - Possessive emoter", "$PT - Possessive target", \
                  "$SI - Subjective emoter", "$PT - Possessive target", \
                  "$OI - Objective emoter", "$OT - Objective target", \
                  "$M - Modifier", "$V - Verb" \
                })

inherit STD_ITEM;

/** @type {SOUL_D} */ object soul_d;

nosave string tmp_emote_targeted_name;
nosave string tmp_emote_untargeted_name;
nosave string *tmp_emote_untargeted_msgs;
nosave string *tmp_emote_targeted_msgs;

int print_all_emotes();

void create() {
    set_id(({ "/emote_client" }));
    soul_d = load_object(SOUL_D);
}

int main_menu() {
    tell_me("\n");
    tell_me("1. Add new emote\n");
    tell_me("2. Remove existing emote\n");
    tell_me("3. See message syntax for specific emote\n");
    tell_me("4. See all available emotes\n");
    tell_me("5. Remove all emotes (CAREFUL)\n");
    tell_me("\nType 'q' to exit.\n");
    tell_me("\n" + PROMPT);

    input_to("menu_selection");
    return 1;
}

int menu_selection(string arg) {
    if(lower_case(arg) == "q" || lower_case(arg) == "quit") {
        tell_me("Exiting emote editor.\n");
        remove();
        return 1;
    }

    switch(arg) {
        case "1":
            tell_me("\n");
            tell_me("Please enter the command word for the new emote.\nOr type 'q' to return to the main menu.\n\n"+PROMPT);
            input_to("get_emote_name");
            break;
        case "2":
            tell_me("\n");
            tell_me("Which emote would you like to delete?\nOr type 'q' to return to the main menu.\n\n" + PROMPT);
            input_to("remove_emote");
            break;
        case "3":
            tell_me("\n");
            tell_me("Which emote would you like to see messages for?\nOr type 'q' to return to the main menu.\n"+
            "(NOTE: if you wish to see messages for the targeted version, add a '/t' to the end of the emote)\n\n"+
            PROMPT);
            input_to("get_emote_msgs");
            break;
        case "4":
            tell_me("\n");
            print_all_emotes();
            tell_me("\n");
            tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
            input_to("main_or_quit");
            break;
        case "5":
            tell_me("\n");
            tell_me("Are you sure you wish to delete all emotes from the system? (y/n)\n\n" + PROMPT);
            input_to("confirm_delete_all");
            break;
        default:
            tell_me("\n");
            tell_me("Please make a selection from the menu or type 'q' or 'quit' to exit.\n\n"+PROMPT);
            main_menu();
            break;
    }//END SWITCH

    return 1;
}

nomask void get_emote_name(string arg) {
    if(!arg || arg == "") {
        tell_me("\nPlease enter the command word for the new emote.\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("get_emote_name");
        return;
    }//END IF

    if(sizeof(explode(arg, " ")) > 1) {
        tell_me("\nThe emote command must be only one word. Please try again.\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("get_emote_name");
        return;
    }//END IF

    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }//END IF

    tmp_emote_untargeted_name = arg;
    tmp_emote_targeted_name = arg+"/t";

    tell_me("\nWill this emote be (t)argeted, (u)ntargeted or (b)oth?\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);

    input_to("targeted_or_not");
}

nomask void targeted_or_not(string arg) {
    int i;
    string tmp = "";
    string *existing_emotes = soul_d->query_emotes();

    if(!arg || arg == "") {
        tell_me("\nPlease enter 't', 'u' or 'b'.\n"+
        "Will this emote be (t)argeted, (u)ntargeted or (b)oth?\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("targeted_or_not");
        return;
    }//END IF

    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }//END IF

    if(member_array(lower_case(arg), ({ "b", "t", "u" })) == -1) {
        tell_me("\nPlease enter 't', 'u' or 'b'.\n"+
        "Will this emote be (t)argeted, (u)ntargeted or (b)oth?\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("targeted_or_not");
        return;
    }//END IF

    if(((lower_case(arg) == "t") || (lower_case(arg) == "b")) && (member_array(tmp_emote_targeted_name, existing_emotes) != -1)) {
        tell_me("\nThere is already a targeted emote of that name. Please try another.\n"+
        "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("get_emote_name");
        return;
    } else if(((lower_case(arg) == "u") || (lower_case(arg) == "b")) && (member_array(tmp_emote_untargeted_name, existing_emotes) != -1)) {
        tell_me("\nThere is already an untargeted emote of that name. Please try another.\n"+
            "Or type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("get_emote_name");
        return;
    }//END IF

    if(lower_case(arg) == "u") {
        if(tmp_emote_targeted_msgs)
        tmp_emote_targeted_msgs = ({});

        tell_me("\nPlease enter the message printed to the emoter if there is no target.\n"+
        "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

        for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

        tell_me(tmp + "\n"+PROMPT);

        input_to("get_player_msg", 0, 1);
        return;
    } else if(lower_case(arg) == "t") {
        if(tmp_emote_untargeted_msgs)
            tmp_emote_untargeted_msgs = ({});

        tell_me("\nPlease enter the message printed to the emoter if there is a target.\n"+
        "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

        for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

        tell_me(tmp + "\n"+PROMPT);

        input_to("get_player_msg", 0, 2);
        return;
    } else {
        if(tmp_emote_untargeted_msgs)
        tmp_emote_untargeted_msgs = ({});

        if(tmp_emote_targeted_msgs)
            tmp_emote_targeted_msgs = ({});

        tell_me("\nPlease enter the message printed to the emoter if there is no target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

        for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

        tell_me(tmp + "\n"+PROMPT);

        input_to("get_player_msg", 0, 3);
        return;
    }//END ELSE
}

nomask void get_player_msg(string arg, int flag) {
    int i;
    string tmp = "";

    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }//END IF

    switch(flag) {
        case 1:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the emoter if there is no target.\n"+
                    "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_player_msg", 0, 1);
                return;
            }//END IF

            tmp_emote_untargeted_msgs = ({ arg });

            tell_me("\nPlease enter the message printed to the room if there is no target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

            for(i = 0; i < sizeof(MARKERS); i++)
                tmp += MARKERS[i]+"\n";

            tell_me(tmp + "\n"+PROMPT);

            input_to("get_room_msg", 0, 1);
            return;
        case 2:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the emoter if there is a target.\n"+
                "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_player_msg", 0, 2);
                return;
            }//END IF

            tmp_emote_targeted_msgs = ({ arg });

            tell_me("\nPlease enter the message printed to the room if there is a target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the folowing wherever you like in the string:\n");

            for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

            tell_me(tmp + "\n"+PROMPT);

            input_to("get_room_msg", 0, 2);
            return;
        case 3:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the emoter if there is no target.\n"+
                    "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_player_msg", 0, 3);
                return;
            }//END IF

            tmp_emote_untargeted_msgs = ({ arg });

            tell_me("\nPlease enter the message printed to the room if there is no target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

            for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

            tell_me(tmp + "\n"+PROMPT);

            input_to("get_room_msg", 0, 3);
            return;
    }//END SWITCH
}

nomask void get_room_msg(string arg, int flag) {
    int i;
    string tmp = "";

    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }//END IF

    switch(flag) {
        case 1:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the room if there is no target.\n"+
                    "Or type 'q' to return to the main menu.\n\nSubstitue the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_room_msg", 0, 1);
                return;
            }//END IF

            tmp_emote_untargeted_msgs += ({ arg });

            tell_me("\nPlease enter the default mod to the UNTARGETED emote. Such as 'happily',\n"+
            "or 'haphazardly'. If you do not wish to have a default, then press enter.\nOr press 'q'"+
            "to return to the main menu.\n\n"+PROMPT);

            input_to("get_mods", 0, 1);
            return;
        case 2:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the room if there is a target.\n"+
                    "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_room_msg", 0, 2);
                return;
            }//END IF

            tmp_emote_targeted_msgs += ({ arg });

            tell_me("\nPlease enter the message to print to the target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

            for(i = 0; i < sizeof(MARKERS); i++)
                tmp += MARKERS[i]+"\n";

            tell_me(tmp + "\n"+PROMPT);

            input_to("get_target_msg");
            return;
        case 3:
            if(!arg || arg == "") {
                tell_me("\nPlease enter the message printed to the room if there is no target.\n"+
                    "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

                for(i = 0; i < sizeof(MARKERS); i++)
                    tmp += MARKERS[i]+"\n";

                tell_me(tmp + "\n"+PROMPT);

                input_to("get_room_msg", 0, 3);
                return;
            }//END IF

            tmp_emote_untargeted_msgs += ({ arg });

            tell_me("\nPlease enter the default mod to the UNTARGETED emote. Such as 'happily',\n"+
            "or 'haphazardly'. If you do not wish to have a default, then press enter.\nOr press 'q'"+
            "to return to the main menu.\n\n"+PROMPT);

            input_to("get_mods", 0, 3);
            return;
    }//END SWITCH
}

nomask void get_target_msg(string arg) {
    int i;
    string tmp = "";

    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }

    if(!arg || arg == "") {
        tell_me("\nPlease enter the message printed to the target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitue the following wherever you like in the string:\n");

        for(i = 0; i < sizeof(MARKERS); i++)
            tmp += MARKERS[i]+"\n";

        tell_me(tmp + "\n"+PROMPT);

        input_to("get_target_msg");
        return;
    }

    tmp_emote_targeted_msgs += ({ arg });

    tell_me("\nPlease enter the default mod to the TARGETED emote. Such as 'happily',\n"+
        "or 'haphazardly'. If you do not wish to have a default, then press enter.\nOr press 'q' "+
        "to return to the main menu.\n\n"+PROMPT);

    input_to("get_mods", 0, 2);
}

nomask void get_mods(string arg, int flag) {
    int i;
    string tmp = "";

    switch(flag) {
        case 1:
            if(!arg || arg == "")
                tmp_emote_untargeted_msgs += ({ "" });
            else
                tmp_emote_untargeted_msgs += ({ arg });

            if(!soul_d->query_emotes())
                soul_d->reset_emotes();

            soul_d->set_emote(tmp_emote_untargeted_name, tmp_emote_untargeted_msgs);

            tell_me("\nNew untargeted emote '"+tmp_emote_untargeted_name+"' added.\n");

            tell_me("\n");
            tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
            input_to("main_or_quit");
            return;
        case 2:
            if(!arg || arg == "")
                tmp_emote_targeted_msgs += ({ "" });
            else
                tmp_emote_targeted_msgs += ({ arg });

            soul_d->set_emote(tmp_emote_targeted_name, tmp_emote_targeted_msgs);

            tell_me("\nNew targeted emote '"+tmp_emote_targeted_name[0..<3]+"' added.\n");

            tell_me("\n");
            tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
            input_to("main_or_quit");
            return;
        case 3:
            if(!arg || arg == "")
                tmp_emote_untargeted_msgs += ({ "" });
            else
                tmp_emote_untargeted_msgs += ({ arg });

            soul_d->set_emote(tmp_emote_untargeted_name, tmp_emote_untargeted_msgs);

            tell_me("\nNew untargeted emote '"+tmp_emote_untargeted_name+"' added.\n");

            tell_me("\nPlease enter the message printed to the emoter if there is a target.\n"+
            "Or type 'q' to return to the main menu.\n\nSubstitute the following wherever you like in the string:\n");

            for(i = 0; i < sizeof(MARKERS); i++)
                tmp += MARKERS[i]+"\n";

            tell_me(tmp + "\n"+PROMPT);

            input_to("get_player_msg", 0, 2);
            return;
    }//END SWITCH
}

nomask void remove_emote(string emote) {
    string *existing_emotes;

    if(emote == "q" || emote == "quit") {
        main_menu();
        return;
    }

    if(!(existing_emotes = soul_d->query_emotes())) {
        tell_me("There is no emote by that name. Please try another.\nOr type 'q' to return to the main menu.\n\n"+PROMPT);
            input_to("remove_emote");
        return;
    }//END IF

    if((member_array(emote, existing_emotes) == -1) && (member_array(emote+"/t", existing_emotes) == -1)) {
        tell_me("There is no emote by that name. Please try another.\nOr type 'q' to return to the main menu.\n\n"+PROMPT);
        input_to("remove_emote");
        return;
    }//END IF

    if((member_array(emote, existing_emotes) != -1) && (member_array(emote+"/t", existing_emotes) != -1)) {
        tell_me("\nBoth a targeted version and an untargeted version of this emote exists\n"+
        "exists. Would you like to delete the (t)argeted version, the (u)ntargeted\n"+
        "version or (b)oth?\nOr type 'q' to return to the main menu.\n\n"+PROMPT);

        input_to("which_to_delete", 0, emote, emote+"/t");
        return;
    }

    if(member_array(emote, existing_emotes) != -1)
        soul_d->delete_emote(emote);
    else
        soul_d->delete_emote(emote+"/t");

    tell_me("\n");
    tell_me("Emote '"+emote+"' deleted.\n");

    tell_me("\n");
    tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
    input_to("main_or_quit");
}

nomask void which_to_delete(string arg, string emote, string targeted_emote) {
    if(arg == "q" || arg == "Q") {
        main_menu();
        return;
    }

    if(!arg || arg == "") {
        tell_me("\nBoth a targeted version and an untargeted version of "
            "this emote exists exists.\n"
            "Would you like to delete the (t)argeted version, the (u)ntargeted\n" +
            "version or (b)oth?\nOr type 'q' to return to the main menu.\n\n"+PROMPT);

        input_to("which_to_delete", 0, emote, targeted_emote);
        return;
    }

    switch(arg) {
        case "t":
            soul_d->delete_emote(targeted_emote);
            tell_me("\nTargeted emote '"+emote+"' deleted.\n");
            break;
        case "u":
            soul_d->delete_emote(emote);
            tell_me("\nUntargeted emote '"+emote+"' deleted.\n");
            break;
        case "b":
            soul_d->delete_emote(targeted_emote);
            soul_d->delete_emote(emote);
            tell_me("\nTargeted emote '"+emote+"' deleted.\n");
            tell_me("Untargeted emote '"+emote+"' deleted.\n");
            break;
    }//END SWITCH

    tell_me("\n");
    tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
    input_to("main_or_quit");
}

int confirm_delete_all(string arg) {
    if(!arg || arg == "")
        tell_me("\nDeletion of all emotes cancelled.\n");
    else if(lower_case(arg) == "y" || lower_case(arg) == "yes") {
        soul_d->reset_emotes();
        tell_me("\nAll emotes have been deleted.\n");
    } else if(lower_case(arg) == "n" || lower_case(arg) == "no") {
        tell_me("\n");
        tell_me("Deletion of all emotes cancelled.\n");
    } else
        tell_me("\nDeletion of all emotes cancelled.\n");

    tell_me("\n");
    tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n"
        + PROMPT);
    input_to("main_or_quit");
    return 1;
}

int get_emote_msgs(string arg) {
    string *msgs;
    int i;

    if(arg == "q" || arg == "Q") {
        main_menu();
        return 1;
    }//END IF

    msgs = soul_d->query_emote_msgs(arg);

    if(!msgs)
        tell_me("That emote does not exist, or has no output.\n");
    else {
        for(i = 0; i < sizeof(msgs); i++) {
            if(i == 0)
                tell_me("Message to emoter: " + msgs[i] + "\n");
            if(i == 1)
                tell_me("Message to room: " + msgs[i] + "\n");
            if(i == 2 && sizeof(msgs) == 3)
                tell_me("Default modifier: " + msgs[i] + "\n");
            else if(i == 2 && sizeof(msgs) == 4)
                tell_me("Message to target: " + msgs[i] + "\n");
            if(i == 3)
                tell_me("Default modifier: " + msgs[i] + "\n");
        }
    }

    tell_me("\n");
    tell_me("Hit enter to go back to the main menu or type 'q' to quit.\n\n" + PROMPT);
    input_to("main_or_quit");
    return 1;
}

int main_or_quit(string arg) {
    if(lower_case(arg) == "q" || lower_case(arg) == "quit") {
        tell_me("Exiting emote editor.\n");
        remove();
        return 1;
    }

    main_menu();
    return 1;
}

int print_all_emotes() {
    string *emotes;
    int i;

    emotes = soul_d->query_emotes();

    if(!emotes)
        tell_me("There are no emotes to list.\n");
    else {
        emotes = sort_array(emotes, 1);

        for(i = 0; i < sizeof(emotes); i++)
            tell_me(emotes[i]+"\n");
    }

    return 1;
}
