#pragma once

namespace vex
{
    class t_non_copyable
    {
    public:
        t_non_copyable() = default;
        ~t_non_copyable() = default;

        t_non_copyable(t_non_copyable&&) = default;

        t_non_copyable(t_non_copyable&) = delete;
        t_non_copyable& operator=(const t_non_copyable& other) = delete;
    };
}