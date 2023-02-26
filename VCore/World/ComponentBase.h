#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/VUtilsBase.h>

#include <cstdint>

namespace vex
{
    //template <u32 byte_size>
    //struct Flags64
    //{
    //    static_assert(byte_size >= 8);
    //    union
    //    {
    //        u8 bytes[byte_size];
    //    };
    //};

    using tMask = uint64_t;
    struct ComponentCounter
    {
    private:
        static int gCount;

    public:
        static constexpr int Capacity = sizeof(tMask);
        static int CurrentCount() { return gCount; }
        static int Next()
        {
            // ensure tMaskSize is big enough to hold all registered components
            // shame it could not be constexpr + static_assert duo in C++17
            checkAlways_(gCount < Capacity);
            return ++gCount;
        }
    };

    template <typename TComp>
    struct BaseComp
    {
        static const tMask Mask;
    };

    struct DebugName : BaseComp<DebugName>
    {
        const char* Name = nullptr;
    };

    // unfortunately not a compile time constant, #todo try macro+constexpr CNT_INC/CNT_READ pair
    template <typename TComp>
    const tMask BaseComp<TComp>::Mask = 1ull << ComponentCounter::Next(); // _ull -> uint64 literal
} // namespace vex
