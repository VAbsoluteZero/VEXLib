#pragma once

#include <VCore/Utils/TTraits.h>

namespace vex
{
    namespace _internal
    {
        template <typename TFunc>
        struct defer_guard : private vex::t_non_copyable
        {
            TFunc func;
            defer_guard(TFunc f) : func(f) {}
            ~defer_guard() { func(); }
        };
    } 
} 

#define vpint_COMBINE1(X, Y) X##Y
#define vpint_COMBINE(X, Y) vpint_COMBINE1(X, Y)
#define defer_ vex::_internal::defer_guard vpint_COMBINE(defer_anon_, __LINE__) = [&]()

namespace vex::debug
{
    using tLogFuncPtr = auto (*)(const char*, int, const char*) -> void;
    struct DebugLogHook
    {
        static tLogFuncPtr log_override;
        static void default_print(const char* file, int line, const char* msg);
        static void print(const char* file, int line, const char* msg);
    };
} 
//===========================================================================================================
// VEX Check (Assert) implementation
//===========================================================================================================
#ifndef VEX_CHECK_LEVEL
    #ifdef NDEBUG
        #define VEX_CHECK_LEVEL 1
    #else
        #define VEX_CHECK_LEVEL 2
    #endif
#endif


#if defined(_MSC_VER)
    #define FORCE_NOINLINE __declspec(noinline)
#endif

#if defined(_MSC_VER)
extern void __cdecl __debugbreak(void);
    #define VEX_DBGBREAK() __debugbreak()
#elif defined(__clang__)
    #define VEX_DBGBREAK() __asm__ __volatile__("int $3\n\t")
#elif
    #error No debug break for platform
#endif 
#ifndef VEX_ABORT_ON_CHECK_FAIL
    #define VEX_ABORT_ON_CHECK_FAIL 0
#endif

#if VEX_ABORT_ON_CHECK_FAIL
    #define VEXpriv_MaybeCrashOnCheck() \
        {                               \
            std::abort();               \
        }
#else
    #define VEXpriv_MaybeCrashOnCheck()
#endif

#define VEXpriv_IgnoreCheck(ConditionExp) (!!(ConditionExp))

// FORCE_NOINLINE ?
#define VEXpriv_DoCheck(ConditionExp, File, Line, Msg) \
    (!!(ConditionExp)) || ([]() -> bool \
		{ \
				vex::debug::DebugLogHook::print(File, Line, Msg); \
				return true; \
		}()) &&  ([] { VEX_DBGBREAK(); VEXpriv_MaybeCrashOnCheck(); return false;} ())

#define VEXpriv_DoCheckAlwaysTrigger(ConditionExp, File, Line, Msg) \
    (!!(ConditionExp)) || ([]() -> bool \
		{ \
			static bool g_executed = false; \
			if (!g_executed) \
			{ \
				g_executed = true; \
				vex::debug::DebugLogHook::print(File, Line, Msg); \
				return true; \
			} \
			return false; \
		}()) &&  ([] { VEX_DBGBREAK(); VEXpriv_MaybeCrashOnCheck(); return false;} ())

// toggle which checks should be enabled depending on CHECK_LEVEL
// by default 'check' will work in debug only, 'checkRel' will also work in release,
// 'checkParanoid' would nod work unless VEX_CHECK_LEVEL is set to 3 or more
#if VEX_CHECK_LEVEL == 0 // none
    #define check(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define check_(ConditionExpr) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkRel(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)

    #define checkAlways(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkAlways_(ConditionExpr) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkAlwaysRel(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkAlwaysParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
#elif VEX_CHECK_LEVEL == 1 // release
    #define check(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define check_(ConditionExpr) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkRel(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)

    #define checkAlways(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkAlways_(ConditionExpr) VEXpriv_IgnoreCheck(ConditionExpr)
    #define checkAlwaysRel(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkAlwaysParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
#elif VEX_CHECK_LEVEL == 2 // default
    #define check(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)
    #define check_(ConditionExpr) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, " no message ")
    #define checkRel(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)

    #define checkAlways(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkAlways_(ConditionExpr) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, " no message ")
    #define checkAlwaysRel(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkAlwaysParanoid(ConditionExpr, Msg) VEXpriv_IgnoreCheck(ConditionExpr)
#elif VEX_CHECK_LEVEL > 3 // paranoid
    #define check(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)
    #define check_(ConditionExpr) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, " no message ")
    #define checkRel(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkParanoid(ConditionExpr, Msg) VEXpriv_DoCheck(ConditionExpr, __FILE__, __LINE__, Msg)

    #define checkAlways(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkAlways_(ConditionExpr) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, " no message ")
    #define checkAlwaysRel(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
    #define checkAlwaysParanoid(ConditionExpr, Msg) \
        VEXpriv_DoCheckAlwaysTrigger(ConditionExpr, __FILE__, __LINE__, Msg)
#endif