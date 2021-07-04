#pragma once

#include <string>
#include <VLang/Misc/TTraits.h>

#include <VLang/Misc/VUtilsBase.h>

#include <VLang/Misc/Color.h> 
#include <spdlog/fmt/fmt.h> 
 

namespace vex
{ 
    inline std::string ToString(Color c)
    {
        return fmt::format("RGBA({0:F3}, {1:F3}, {2:F3}, {3:F3})",
            c.r, c.g, c.b, c.a);
    } 
}  