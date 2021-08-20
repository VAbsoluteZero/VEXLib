#pragma once

namespace vex
{
    class non_copyable
    {
    public:
        non_copyable() = default;
        ~non_copyable() = default;

        non_copyable(non_copyable&&) = default;

        non_copyable(non_copyable&) = delete;
        non_copyable& operator=(const non_copyable& other) = delete;
    };
}