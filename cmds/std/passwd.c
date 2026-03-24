/**
 * @file /cmds/std/passwd.c
 * @description Command to manage your own password or the password of another
 *              user.
 *
 * @created 2024-07-14 - Gesslar
 * @last_modified 2024-07-14 - Gesslar
 *
 * @history
 * 2024-07-14 - Gesslar - Created
 */

inherit STD_CMD;

void password_confirmed(int status, object tp);
void new_password(string str, object tp);
void confirm_password(string str, string pass, object tp);

mixed main(object tp, string arg) {
    _info("Changing your password.");
    prompt_password(tp, 3, assemble_call_back((:password_confirmed:), tp));

    return 1;
}

void password_confirmed(int status, object tp) {
    if(status) {
        _question("Please enter your new password: ");
        input_to((:new_password:), I_NOECHO | I_NOESC, tp);
    } else {
        _error("Password change failed.");
    }
}

void new_password(string str, object tp) {
    if(!str || str == "") {
        _error("Password cannot be empty.");
        return;
    }
    _question("Please confirm your new password: ");
    input_to((:confirm_password:), I_NOECHO | I_NOESC, str, tp);
}

void confirm_password(string str, string pass, object tp) {
    if(str == pass) {
        _ok("Password accepted.");
        ACCOUNT_D->writeAccount(tp->query_real_name(), "password", crypt(str, 0));
        _ok("Password changed.");
    } else {
        _error("Passwords do not match.");
    }
}

string query_help(object tp) {
    return
"SYNTAX: passwd\n\n"
"This command allows you to change your current passwd. You will be asked to "
"enter your current passwd for security purposes and then it will request you "
"to input your new passwd and then will ask again to confirm. After you have "
"provided the requested information, the command will inform you if the change "
"was succesful.";
}
