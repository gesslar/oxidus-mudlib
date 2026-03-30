#ifndef __COMMAND_H__
#define __COMMAND_H__

// Command handler functions
public void addCommand(mixed command, mixed action);
public void removeCommand(mixed command);
public void removeCommandAll(mixed action);
public mixed queryCommand(string command);
public mapping queryCommands();
public string *queryMatchingCommands(string command);
public string *queryAllCommands();
public void initCommands();
public mixed evaluateCommand(object tp, string command,
  string arg);

// Path functions
public string *queryPath();
public int addPath(string str);
public void setPath(mixed path);
public int remPath(string str);
public void addWizardPaths();
public void removeWizardPaths();
public void addStandardPaths();
public void addGhostPaths();
public string findCommandPath(string verb);

// Command history and execution
public nomask varargs string *queryCommandHistory(int index,
  int range);
public int commandHook(string arg);
private nomask int evaluateResult(mixed result);
public int forceMe(string cmd);

#endif // __COMMAND_H__
