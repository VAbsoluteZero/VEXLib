#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include <string.h>

#include "vexcore/memory/Memory.h"
#include "vexcore/utils/CoreTemplates.h"

namespace vex {
    template <typename ValType>
    struct ROSpan {
        const ValType* data = nullptr;
        const i32 len = 0;
        FORCE_INLINE auto byteSize() const -> u64 { return len * sizeof(ValType); }
        FORCE_INLINE auto size() const -> const i32 { return len; }
        FORCE_INLINE auto operator[](i32 i) const -> const ValType& {
            check((i >= 0) && (i < len), "Span access out of bounds");
            return *(data + i);
        }
        FORCE_INLINE auto at(i32 i) const -> const ValType& {
            checkLethal((i >= 0) && (i < len), "Span access out of bounds");
            return *(data + i);
        }
        FORCE_INLINE auto begin() -> const ValType* { return data; }
        FORCE_INLINE auto end() -> const ValType* { return len > 0 ? data + len : nullptr; }
        FORCE_INLINE auto slice(int start) -> ROSpan<ValType> {
            checkLethal((start >= 0) && (start <= len), "Span slice is out of bounds");
            return ROSpan<ValType>{data + start, len};
        }
        FORCE_INLINE auto slice(int start, int slice_len) -> ROSpan<ValType> {
            checkLethal( // should it allow 0 len slices?
                ((start >= 0) && (start + slice_len <= this->len) && (slice_len >= 0)),
                "Span slice is out of bounds");

            return ROSpan<ValType>{data + start, slice_len};
        }
    };

    // Optimized for POD types that are fine with memcpy for copying and don't need dtor call
    template <typename ValType>
    struct Buffer {
        static_assert( // take your fancy non trivial types somewhere else
            std::is_trivially_copyable_v<ValType> && std::is_trivially_destructible_v<ValType>);

        Buffer() = default;
        explicit Buffer(Allocator al) : allocator(al) {}
        Buffer(Allocator al, i32 cap) : allocator(al) { reserve(cap); }
        Buffer(const Buffer& other) {
            allocator = other.allocator;
            len = other.len;
            cap = other.cap;
            if (other.first) {
                first = vexAllocTyped<ValType>(allocator, cap, alignof(ValType));
                if (len > 0)
                    memcpy(first, other.first, len * sizeof(ValType));
            }
        }
        Buffer(std::initializer_list<ValType> initlist, vex ::Allocator in_alloc = {})
        : allocator(in_alloc) {
            addList(initlist);
        }
        Buffer(Buffer&& other) { *this = std::move(other); }
        Buffer& operator=(Buffer&& other) {
            if (this != &other) {
                if (first)
                    allocator.dealloc(first);
                allocator = other.allocator;
                len = other.len;
                cap = other.cap;
                first = other.first;
                other.first = nullptr;
                other.cap = 0;
                other.len = 0;
            }

            return *this;
        }
        ~Buffer() {
            if (first)
                allocator.dealloc(first);
        }

        FORCE_INLINE auto byteSize() const -> u64 { return len * sizeof(ValType); }
        FORCE_INLINE auto size() const -> i32 { return len; }
        FORCE_INLINE auto capacity() const -> i32 { return cap; }
        FORCE_INLINE auto data() -> ValType* { return first; }
        FORCE_INLINE auto data() const -> const ValType* { return first; }
        FORCE_INLINE auto back() const -> ValType* { return len > 0 ? first + len - 1 : nullptr; }
        FORCE_INLINE auto begin() -> ValType* { return first; }
        FORCE_INLINE auto end() -> ValType* { return len > 0 ? first + len : nullptr; }
        FORCE_INLINE auto begin() const -> const ValType* { return first; }
        FORCE_INLINE auto end() const -> const ValType* { return len > 0 ? first + len : nullptr; }

        FORCE_INLINE auto operator[](i32 i) const -> ValType& {
            checkLethal((i >= 0) && (i <= len), "out of bounds");
            return *(first + i);
        }
        FORCE_INLINE auto at(i32 i) const -> ValType& {
            checkLethal((i >= 0) && (i <= len), "out of bounds");
            return *(first + i);
        }
        FORCE_INLINE auto atUnchecked(i32 i) const -> ValType& { return *(first + i); }

        template <typename InValType>
        FORCE_INLINE void add(InValType&& in_val) {
            if (cap <= len) {
                grow();
            }
            new (first + len) ValType(in_val);
            len++;
        }
        FORCE_INLINE void addUninitialized(i32 num) {
            auto new_len = len + (num < 0 ? 0 : num);
            reserve(new_len);
            len = new_len;
        }
        FORCE_INLINE void addZeroed(i32 num) {
            if (num > 0) {
                auto new_len = len + num;
                reserve(new_len);
                memset(first + len, 0, num * sizeof(ValType));
                len = new_len;
            }
        }
        FORCE_INLINE void addRange(ValType* src, i32 num) {
            if (num > 0) {
                if ((num + len) > cap)
                    reserve(num + len);

                memcpy(first + len, src, num * sizeof(ValType));
                len += num;
            }
        }
        FORCE_INLINE void addList(std::initializer_list<ValType> initlist) {
            reserve(len + initlist.size());
            for (auto&& rec : initlist)
                add(rec);
        }
        FORCE_INLINE void setSizeInit(i32 num) {
            if (num > 0) {
                reserve(num);
                for (i32 i = len; i < num; i++) {
                    add({});
                }
            } else
                len = num;
        }

        // 'Swaps' element with the last and reduces len by 1, so it CHANGES order.
        // In actuality it moves (len - 1) into (i)th, leaving (len - 1) as it was, since it is
        // POD-only array
        FORCE_INLINE void removeSwapAt(i32 i) noexcept {
            if (len == 0 || i > len)
                return;
            if (i == len) {
                len--;
                return;
            }
            atUnchecked(i) = std::move(atUnchecked(len - 1));
            len--;
        }
        FORCE_INLINE void removeShift(i32 i, i32 num = 1) noexcept {
            if (len == 0 || i > len || num <= 0)
                return;

            const i32 mod_range_size = (len - i);
            i32 num_clamped = num > mod_range_size ? mod_range_size : num;
            i32 rest = max(len - (i + num_clamped), 0);

            if (rest > 0) {
                memmove(first + i, first + i + num_clamped, rest * sizeof(ValType));
            }

            len -= num_clamped;
        }

        FORCE_INLINE void grow() {
            // 1.5 growth factor, (cap + cap / 2) would work only for 2 and higher cap
            // default new capacity for 'growth' is 5 (if cap is < 2)
            const i32 grow_cap = cap >= 2 ? (cap + cap / 2) : 5;
            reserve(grow_cap);
        }
        // #fixme change
        FORCE_INLINE void reserve(i32 num) {
            if (num <= cap)
                return;

            u64 new_cap = cap + (u64)num;

            auto new_first = vexAllocTyped<ValType>(allocator, new_cap, alignof(ValType));
            cap = new_cap;
            if (first == nullptr) {
                first = new_first;
                return;
            }
            if (new_first != first) {
                memcpy(new_first, first, len * sizeof(ValType));
                vexFree(allocator, (u8*)first);
                first = new_first;
            }
        }

        FORCE_INLINE ROSpan<ValType> constSpan() const { return {first, len}; }

    private:
        Allocator allocator;
        ValType* first = nullptr;
        i32 len = 0;
        i32 cap = 0;
    };
} // namespace vex
