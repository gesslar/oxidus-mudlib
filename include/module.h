#ifndef __MODULE_H__
#define __MODULE_H__

varargs object add_module(string moduleFile, mixed args...);
object query_module(string moduleFile);
int remove_module(string moduleFile);
mapping query_modules();
varargs mixed module(string moduleFile, string functionName,
  mixed args...);
void remove_all_modules();

#endif // __MODULE_H__
