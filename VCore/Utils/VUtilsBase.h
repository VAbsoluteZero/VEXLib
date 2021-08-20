#pragma once

#include <VCore/Utils/TTraits.h>
 
namespace vex 
{
    namespace __internal
    {
        template<typename TFunc>
        struct defer_guard : private vex::non_copyable
        {
            TFunc func;
            defer_guard(TFunc f) : func(f) {  }
            ~defer_guard() { func(); }
        };
    }
}

#define vpint_COMBINE1(X,Y) X##Y  
#define vpint_COMBINE(X,Y) vpint_COMBINE1(X,Y)
#define defer_ vex::__internal::defer_guard vpint_COMBINE(defer_anon_, __LINE__) = [&]() 