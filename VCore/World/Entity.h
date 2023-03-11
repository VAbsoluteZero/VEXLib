#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Utils/TypeInfoUtils.h>
#include <VCore/World/ComponentBase.h>

namespace vex
{
    using EntId = u16;

    struct EntityHandle
    {
        EntId id = 0;
        u16 gen = 0; 

        FORCE_INLINE constexpr operator bool() const { return id > 0; }
        FORCE_INLINE constexpr bool operator==(EntityHandle b) const
        {
            return id == b.id && gen == b.gen;
        }
    };

    struct EntHandleHasher
    {
        static FORCE_INLINE constexpr int hash(const EntityHandle& h) { return h.id; }
        static FORCE_INLINE constexpr bool is_equal(const EntityHandle& a, const EntityHandle& b)
        {
            return a == b;
        }
    };

    struct EntComponentSet
    {
        CompBitset bitset;

        template <class... TArgs>
        inline bool has() const noexcept
        {
            return bitset.has<TArgs...>();
        } 
        inline bool has(auto a, auto b) const noexcept
        {
            return bitset.has(a, b);
        }
        template <class... TArgs>
        inline void setFlags() noexcept
        {
            bitset.set<TArgs...>();
        }
        template <class... TArgs>
        inline void unsetFlags() noexcept
        {
            bitset.unset<TArgs...>();
        }
         
        FORCE_INLINE constexpr bool operator==(EntComponentSet b) const
        {
            return bitset == b.bitset;
        }
    };
} // namespace vex
