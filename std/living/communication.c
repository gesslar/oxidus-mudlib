/**
 * @file /std/living/communication.c
 * says/tells/etc
 *
 * @created 2024-08-17 - Gesslar
 * @last_modified 2024-08-17 - Gesslar
 *
 * @history
 * 2024-08-17 - Gesslar - Created
 */

#include <communication.h>

private nosave int _comm_limit = 25;

private string *_says = ({});
private string *_tells = ({});
private string *_whispers = ({});
private nosave string _tell_reply = "";
private nosave string _whisper_reply = "";

void add_say(string text) {
    _says += ({text});
    _says = _says[_comm_limit..];
}

void add_tell(string text) {
    _tells += ({text});
    _tells = _tells[_comm_limit..];
}

void set_tell_reply(string name) {
    _tell_reply = lower_case(name);
}

string query_tell_reply() { return _tell_reply ; }

void add_whisper(string text) {
    _whispers += ({text});
    _whispers = _whispers[_comm_limit..];
}

void set_whisper_reply(string name) {
    _whisper_reply = lower_case(name);
}

string query_whisper_reply() { return _whisper_reply ; }

string *query_says() { return _says ; }
string *query_tells() { return _tells ; }
string *query_whispers() { return _whispers ; }
