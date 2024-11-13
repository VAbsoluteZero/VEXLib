#pragma once

namespace vex
{
    class ttNCpy
    {
    public:
        ttNCpy() = default;
        ~ttNCpy() = default;

        ttNCpy(ttNCpy&&) = default;

        ttNCpy(ttNCpy&) = delete;
        ttNCpy& operator=(const ttNCpy& other) = delete;
    };

    class ttNMovNCpy : ttNCpy
    {
    public:
        ttNMovNCpy() = default;
        ~ttNMovNCpy() = default;

        ttNMovNCpy(ttNMovNCpy&&) = delete;
        ttNMovNCpy& operator=(ttNMovNCpy&& other) = delete;
    };
}