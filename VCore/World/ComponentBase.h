#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/VUtilsBase.h>

#include <cstdint>

// must be power of 2
#ifndef VEX_MAX_COMP_CNT
#define VEX_MAX_COMP_CNT 128
#endif
#define VEX_MAXCOMP_CHUNKS (VEX_MAX_COMP_CNT / 64)
#define VEX_MAXCOMP_BYTES (VEX_MAXCOMP_CHUNKS * 8)

namespace vex
{
    using CompIdType = u16;
    using CompMaskType = u64;
    struct CompBitset
    {
        static constexpr auto size_chunks = VEX_MAXCOMP_CHUNKS;
        static_assert(size_chunks >= 1);
        u64 chunks[size_chunks] = {};

        template <class... TTypes>
        FORCE_INLINE constexpr void set() noexcept
        {
            ((chunks[(TTypes::type_id / 64)] |= TTypes::mod_mask), ...);
        }
        FORCE_INLINE constexpr void set(CompIdType type_id, CompMaskType mod_mask) noexcept
        {
            chunks[type_id / 64] |= mod_mask;
        }
        template <class... TTypes>
        FORCE_INLINE constexpr void unset() noexcept
        {
            ((chunks[(TTypes::type_id / 64)] &= ~TTypes::mod_mask), ...);
        }
        FORCE_INLINE constexpr void unset(CompIdType type_id, CompMaskType mod_mask) noexcept
        {
            chunks[type_id / 64] &= ~mod_mask;
        }

        template <class... TTypes>
        FORCE_INLINE constexpr bool has() const noexcept
        {
            return (... && (chunks[(TTypes::type_id / 64)] & TTypes::mod_mask));
        }
        FORCE_INLINE constexpr bool has(CompIdType type_id, CompMaskType mod_mask) const noexcept
        {
            return (chunks[type_id / 64] & mod_mask) != 0ull;
        }
        FORCE_INLINE constexpr void addOpaque(u16* flag_bit, i16 len) noexcept
        {
            for (; len > 0; len--, flag_bit++)
            {
                u16 v = *flag_bit;
                chunks[v / 64] |= (1ull << (v % 64));
            }
        }
        FORCE_INLINE constexpr bool testOpaque(u16* flag_bit, i16 len) const noexcept
        {
            for (; len > 0; len--, flag_bit++)
            {
                u16 v = *flag_bit;
                if (0ull != (chunks[v] & (1ull << (v % 64))))
                    return false;
            }
            return true;
        }
        FORCE_INLINE constexpr bool operator==(const CompBitset& other) const noexcept
        {
            if (chunks[0] != other.chunks[0])
                return false;
            if constexpr (size_chunks > 1)
                if (chunks[1] != other.chunks[1])
                    return false;
            if constexpr (size_chunks > 2)
                if (chunks[2] != other.chunks[2])
                    return false;
            if constexpr (size_chunks > 3)
                if (chunks[3] != other.chunks[3])
                    return false;
            for (i32 i = 4; i < size_chunks; i++)
            {
                if (chunks[i] != other.chunks[i])
                    return false;
            }
            return true;
        }
    };

    struct ComponentCounter
    {
    private:
        static i32 g_comp_count;

    public:
        static constexpr i32 cap = VEX_MAXCOMP_BYTES;
        static i32 count() { return g_comp_count; }
        static i32 next()
        {
            checkAlways_(g_comp_count < cap);
            return ++g_comp_count;
        }
    };

    template <typename T>
    struct BaseComp
    {
        static const CompIdType type_id;
        static const CompMaskType mod_mask;
    };

    template <typename T>
    struct CompTraits
    {
        static constexpr i32 default_cap = 32;
    };

    // #fixme make it an id to some case-insensetive lookup table
    struct DebugName : BaseComp<DebugName>
    {
        const char* name = nullptr;
    };
    template <>
    struct CompTraits<DebugName>
    {
        static constexpr i32 default_cap = 512;
    };

    template <typename T>
    const CompIdType BaseComp<T>::type_id = ComponentCounter::next();
    template <typename T>
    const CompMaskType BaseComp<T>::mod_mask = 1ull << (BaseComp<T>::type_id % 64);
} // namespace vex
