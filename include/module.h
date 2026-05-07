#ifndef __MODULE_H__
#define __MODULE_H__

varargs object add_module(string module_file, mixed args...);
object query_module(string module_file);
int remove_module(string module_file);
mapping query_modules();
varargs mixed module(string module_file, string function_name, mixed args...);
void remove_all_modules();

#endif // __MODULE_H__
