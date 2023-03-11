#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include <string.h>

#include "VCore/Memory/Memory.h"
#include "VCore/Utils/CoreTemplates.h"

namespace vex
{
    // Optimized for POD types that are fine with memcpy for copying and don't need dtor call
    template <typename ValType>
    struct Buffer
    {
        static_assert(std::is_trivially_copyable_v<ValType> && std::is_trivially_destructible_v<ValType>);
        Allocator allocator;
        ValType* first = nullptr;
        i32 len = 0;
        i32 cap = 0;

        Buffer() = default;
        explicit Buffer(Allocator al) : allocator(al) {}
        Buffer(Allocator al, i32 cap) : allocator(al) { reserve(cap); }
        Buffer(const Buffer& other)
        {
            allocator = other.allocator;
            len = other.len;
            cap = other.cap;
            if (other.first)
            {
                first = vexAllocTyped<ValType>(allocator, cap, alignof(ValType));
                if (len > 0)
                    memcpy(first, other.first, len * sizeof(ValType));
            }
        }
        Buffer(std::initializer_list<ValType> initlist, vex ::Allocator in_alloc = {}) : allocator(in_alloc)
        {
            addRange(initlist);
        }
        Buffer(Buffer&& other) { *this = std::move(other); }
        Buffer& operator=(Buffer&& other)
        {
            if (this != &other)
            {
                allocator = other.allocator;
                len = other.len;
                cap = other.cap;
                first = other.first;
                other.first = nullptr;
            }

            return *this;
        }
        ~Buffer()
        {
            if (first)
                allocator.dealloc(first);
        }

        FORCE_INLINE auto size() const -> i32 { return len; }
        FORCE_INLINE auto capacity() const -> i32 { return cap; }
        FORCE_INLINE auto data() -> ValType* { return first; }
        FORCE_INLINE auto data() const -> const ValType* { return first; }

        FORCE_INLINE auto operator[](i32 i) const -> ValType& { return *(first + i); }
        FORCE_INLINE auto at(i32 i) const -> ValType&
        {
            checkLethal((i >= 0) && (i <= len), "out of bounds");
            return *(first + i);
        }
        FORCE_INLINE auto atUnsafe(i32 i) const -> ValType& { return *(first + i); }

        template <typename InValType>
        FORCE_INLINE void add(InValType&& in_val)
        {
            if (cap <= len)
            {
                grow();
            }
            new (first + len) ValType(in_val);
            len++;
        }
        FORCE_INLINE void addUninitialized(i32 num)
        {
            auto new_len = len + (num < 0 ? 0 : num);
            reserve(new_len);
            len = new_len;
        }
        FORCE_INLINE void addZeroed(i32 num)
        {
            if (num > 0)
            {
                auto new_len = len + num;
                reserve(new_len);
                memset(first + len, 0, num * sizeof(ValType));
                len = new_len;
            }
        }

        FORCE_INLINE void addRange(ValType* src, i32 num)
        {
            if (num > 0)
            {
                if ((num + len) > cap)
                    reserve(num + len);

                memcpy(first + len, src, num * sizeof(ValType));
                len += num;
            }
        }
        FORCE_INLINE void addRange(std::initializer_list<ValType> initlist)
        {
            reserve(len + std::size(initlist));
            for (auto&& rec : initlist)
                add(rec);
        }

        // 'Swaps' element with the last and reduces len by 1, so it CHANGES order.
        // In actuallity it moves (len - 1) into (i)th, leaving (len - 1) as it was, since it is POD-only
        // array
        FORCE_INLINE void removeSwapAt(i32 i) noexcept
        {
            if (len == 0 || i > len)
                return;
            if (i == len)
            {
                len--;
                return;
            }
            atUnsafe(i) = std::move(atUnsafe(len - 1));
            len--;
        }
        FORCE_INLINE void removeShift(i32 i, i32 num = 1) noexcept
        {
            if (len == 0 || i > len || num <= 0)
                return;

            const i32 mod_range_size = (len - i);
            i32 num_clamped = num > mod_range_size ? mod_range_size : num;
            i32 rest = max(len - (i + num_clamped), 0);

            if (rest > 0)
            {
                memmove(first + i, first + i + num_clamped, rest * sizeof(ValType));
            }

            len -= num_clamped;
        }

        FORCE_INLINE void grow()
        {
            // 1.5 growth factor, (cap + cap / 2) would work only for 2 and higher cap 
            // default new capacity for 'growth' is 5 (if cap is < 2)
            const i32 grow_cap = cap >= 2 ? (cap + cap / 2) : 5; 
            reserve(grow_cap);
        }
        FORCE_INLINE void reserve(u32 num)
        {
            if (num <= cap)
                return;
            auto new_first = vexAllocTyped<ValType>(allocator, num, alignof(ValType));
            cap = num;
            if (first == nullptr)
            {
                first = new_first;
                return;
            }
            if (new_first != first) // if it was not realloc-ated
            {
                memcpy(new_first, first, len * sizeof(ValType));
                vexFree(allocator, (u8*)first);
                first = new_first;
            }
        }
    };
} // namespace vex
