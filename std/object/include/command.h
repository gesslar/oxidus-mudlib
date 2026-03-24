#ifndef __COMMAND_H__
#define __COMMAND_H__

// Function declarations for command handling
void add_command(mixed command, mixed action) ;
void remove_command(mixed command) ;
void remove_command_all(mixed action) ;
mixed query_command(string command) ;
mapping query_commands() ;
string *query_matching_commands(string command) ;
void init_commands() ;
mixed evaluate_command(object tp, string command, string arg) ;
string *query_all_commands();
string *query_path();
public void set_path(mixed path);
void add_path(string str);
void rem_path(string str);
nomask varargs string *query_command_history(int index, int range);
int command_hook(string arg);
private nomask int evaluate_result(mixed result);
int force_me(string cmd);
void add_wizard_paths();
void remove_wizard_paths();
void add_standard_paths();
public string find_command_path(string verb);
varargs int verb_hook(string arg, int debug_level);

#endif // __COMMAND_H__
