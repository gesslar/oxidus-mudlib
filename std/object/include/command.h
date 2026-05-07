#ifndef __COMMAND_H__
#define __COMMAND_H__

// Command handler functions
public void add_command(mixed command, mixed action);
public void remove_command(mixed command);
public void remove_command_all(mixed action);
public mixed query_command(string command);
public mapping query_commands();
public string *query_matching_commands(string command);
public string *query_all_commands();
public void init_commands();
public mixed evaluate_command(object tp, string command,
  string arg);

// Path functions
public string *query_path();
public int add_path(string str);
public void set_path(mixed path);
public int rem_path(string str);
public void add_wizard_path();
public void remove_wizard_paths();
public void add_standard_paths();
public void add_ghost_paths();
public string find_command_path(string verb);

// Command history and execution
public nomask varargs string *query_command_history(int index,
  int range);
public int command_hook(string arg);
private nomask int evaluate_result(mixed result);
public int force_me(string cmd);

#endif // __COMMAND_H__
