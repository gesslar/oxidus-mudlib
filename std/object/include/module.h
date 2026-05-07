#ifndef __OBJECT_MODULE_H__
#define __OBJECT_MODULE_H__

varargs object add_module(string module_file, mixed args...);
mixed query_module(string module_name);
int remove_module(string module_name);
mapping query_modules();
varargs mixed module(string module_name, string function_name, mixed args...);
void remove_all_modules();

#endif // __OBJECT_MODULE_H__
