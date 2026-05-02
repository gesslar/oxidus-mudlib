#ifndef __OBJECT_MODULE_H__
#define __OBJECT_MODULE_H__

varargs object add_module(string moduleFile, mixed args...);
mixed query_module(string moduleName);
int remove_module(string moduleName);
mapping query_modules();
varargs mixed module(string moduleName, string functionName,
  mixed args...);
void remove_all_modules();

#endif // __OBJECT_MODULE_H__
