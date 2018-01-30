#include <stdio.h>

int plug_func_stub(const char* func, ...);

#define VA_ARGS(...) , ##__VA_ARGS__
#define FUNCK(func, ...) ((!func)?(plug_func_stub(__FUNCTION__ VA_ARGS(__VA_ARGS__))):(func(__VA_ARGS__)))
#ifndef ATTRI_CHECK
#define ATTRIBUTE_WIKE  __attribute__((weak))
#else
#define ATTRIBUTE_WIKE
#endif



