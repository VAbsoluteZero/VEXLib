
#include "VUtils.h"
#include <spdlog/fmt/fmt.h>

std::string vex::toString(Color c)
{
    return fmt::format("The answer is {}.", 42);
    //return fmt::format("RGBA({0:F3}, {1:F3}, {2:F3}, {3:F3})", c.r, c.g, c.b, c.a);
}