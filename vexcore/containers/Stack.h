#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include "vexcore/memory/Memory.h"
#include "vexcore/utils/CoreTemplates.h"
#include "vexcore/utils/VUtilsBase.h"

namespace vex
{
    template <typename ValType>
    struct Stack
    {
        static constexpr float grow_factor = 1.6f;
        Allocator allocator;
        ValType* first = nullptr;
        i32 len = 0;
        i32 cap = 0;

        explicit Stack(Allocator in_allocator, i32 in_cap) : allocator(in_allocator), cap(in_cap)
        {
            first = vexAllocTyped<ValType>(allocator, cap, alignof(ValType));
            check_(first);
        }
        explicit Stack(Allocator in_allocator) { allocator = in_allocator; }
        ~Stack()
        {
            if (first)
                allocator.dealloc(first);
        }

        auto size() const -> i32 { return len; }
        auto capacity() const -> i32 { return cap; }
        auto data() -> ValType* { return first; }
        auto data() const -> const ValType* { return first; }

        template <typename InValType>
        void push(InValType&& in_val)
        {
            if (cap <= len)
            {
                grow();
            }
            new (first + len) ValType(in_val);
            len++;
        }

        ValType* peek() const { return len > 0 ? (first + len - 1) : nullptr; }
        ValType& peekUnchecked() const { return *(peek()); }

        auto pop() -> Union<ValType, Error>
        {
            ValType* top_val = peek();

            if (nullptr == top_val)
                return Error{};

            Union<ValType, Error> out_val{std::move(*top_val)};

            len--;
            top_val->~ValType();

            return out_val;
        }

        void grow()
        {
            const u32 new_cap = (u32)((cap > 0 ? (cap * grow_factor) : 5.0f));
            // should try realloc
            auto new_first = vexAllocTyped<ValType>(allocator, new_cap, alignof(ValType));
            cap = new_cap;
            if (nullptr == first)
            {
                first = new_first;
                return;
            }
            std::move(first, first + len, new_first);
            for (i32 i = 0; i < len; ++i)
            {
                (first + i)->~ValType();
            }
            vexFree(allocator, (u8*)first);
            first = new_first;
        }
    };
} // namespace vex
