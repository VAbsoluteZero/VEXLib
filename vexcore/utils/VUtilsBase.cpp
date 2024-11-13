#include "VUtilsBase.h"

#include <iostream>
#include <string>

#if defined(_MSC_VER)
    #include <windows.h>
#endif

vex::debug::tLogFuncPtr vex::debug::DebugLogHook::log_override = nullptr;

void vex::debug::DebugLogHook::print(const char* file, int line, const char* msg)
{
    if (nullptr != log_override)
    {
        log_override(file, line, msg);
        return;
    }
    default_print(file, line, msg);
}
void vex::debug::DebugLogHook::default_print(const char* file, int line, const char* msg)
{
    std::string print_me{file};
    print_me += "(";
    print_me += std::to_string(line);
    print_me += ",0):";
    print_me += "\n error : assertion failed"; 
    print_me += ", message: '";
    print_me += msg;
    print_me += "' \n";
#if defined(_MSC_VER)
    OutputDebugStringA(print_me.c_str());
#else
    std::cerr << print_me;
#endif
}
