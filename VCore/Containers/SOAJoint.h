#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <VCore/Memory/Memory.h>
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/HashUtils.h>

#include <array>
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <type_traits> 
 
namespace vex
{
    inline constexpr u32 roundUpToMultiplyOf(u32 num, u32 mult) { return ((num + mult - 1) / mult) * mult; }
    /* this is just mutable span/region of SOABuffer*/
    template <typename T>
    struct RawBuffer
    {
        T* first = nullptr;
        u32 capacity = 0;

        inline int size() const { return (int)capacity; }
        const T* begin() const { return first; }
        const T* end() const { return first + capacity; }
        FORCE_INLINE T& operator[](int i) const { return *(first + i); }

        void copyTo(const RawBuffer<T>& other)
        {
            for (u32 i = 0u; i < capacity && i < other.capacity; i++)
            {
                other[i] = (*this)[i];
            }
        }
        void copyTo(const RawBuffer<T>& other, const T& fillVal)
        {
            for (auto i = 0u; i < capacity && i < other.capacity; i++)
            {
                other[i] = (*this)[i];
            }

            for (u32 i = (u32)this->capacity; i < other.capacity; i++)
            {
                other[i] = fillVal;
            }
        }

        RawBuffer(T* first, u32 capacity) : first(first), capacity(capacity) {}
        RawBuffer(){};

        RawBuffer(const RawBuffer& x) : first(x.first), capacity(x.capacity){};
        RawBuffer(RawBuffer&& a) = delete;
        RawBuffer& operator=(const RawBuffer& a) = default;
        RawBuffer& operator=(RawBuffer&& a) = default;
        ~RawBuffer() = default;
    };

    // use it to allocate several buffers in one continuous memory region
    // DOES NOT CALL THE DESTRUCTORS OF AN ELEMENT, call it yourself.
    // It does free() the memory region itslef.
    // use only as inner container
    template <typename... TTypes>
    class SOAUniBuffer
    {
        static const size_t alignment = vex::maxAlignOf<TTypes...>();

    public:
        static constexpr size_t array_count = sizeof...(TTypes);
        struct Block
        {
            u32 first_byte = 0;
            template <typename T>
            FORCE_INLINE T* start(u8* base) const noexcept
            {
                return reinterpret_cast<T*>(base + first_byte);
            }
        };

        template <size_t ARRAY_ID, typename T>
        inline T& get(i32 index) const
        {
            return *(std::get<ARRAY_ID>(blocks).template range_start<T>(memory_region) + index);
        }

        inline u32 size() const noexcept { return capacity; }

        template <i32 N, typename... Ts>
        using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;

        template <size_t ARRAY_ID, typename T>
        inline void emplaceNewAt(i32 i, const T& val)
        {
            static_assert(ARRAY_ID < array_count, "accessing an element outside of bounds");
            static_assert(std::is_same<NthTypeOf<ARRAY_ID, TTypes...>, T>::value,
                " ::Add<__I__, __TYPE__>(elem) -> invalid type when accessing element of SOA");

            auto& v = std::get<ARRAY_ID>(blocks);
            new (v.template start<T>(memory_region) + i) T(val);
        }
        template <size_t ARRAY_ID, typename T, typename... TArgs>
        inline void emplaceNewAt(i32 i, TArgs... args)
        {
            static_assert(ARRAY_ID < array_count, "accessing an element outside of bounds");
            static_assert(std::is_same<NthTypeOf<ARRAY_ID, TTypes...>, T>::value,
                " ::EmplaceBack<__I__, __TYPE__>(args) -> invalid type when accessing element of SOA");

            auto& v = std::get<ARRAY_ID>(blocks);
            new (v.template start<T>(memory_region) + i) T(std::forward<TArgs>(args)...);
        }

        template <size_t ARRAY_ID, typename T>
        inline RawBuffer<T> getBuffer() const noexcept
        {
            static_assert(ARRAY_ID < array_count, "accessing an element outside of bounds");
            static_assert(std::is_same<NthTypeOf<ARRAY_ID, TTypes...>, T>::value,
                " ::GetView<__I__, __TYPE__>(void) -> invalid type when accessing sub array of SOA");

            const Block& v = std::get<ARRAY_ID>(blocks);
            return RawBuffer<T>(v.template start<T>(memory_region), capacity);
        }

        template <size_t ARRAY_ID, typename T>
        inline T* getBuffPtr() const noexcept
        {
            static_assert(ARRAY_ID < array_count, "accessing an element outside of bounds");
            static_assert(std::is_same<NthTypeOf<ARRAY_ID, TTypes...>, T>::value,
                " ::GetView<__I__, __TYPE__>(void) -> invalid type when accessing sub array of SOA");

            const Block& v = std::get<ARRAY_ID>(blocks);
            return v.template start<T>(memory_region);
        }

        SOAUniBuffer() = delete;

        explicit SOAUniBuffer(size_t in_capacity) noexcept
        {
            assert(in_capacity > 0);
            createBuffer(in_capacity);
        }

        SOAUniBuffer(const SOAUniBuffer& other) = delete;
        SOAUniBuffer(SOAUniBuffer&& other) noexcept { *this = std::move(other); }
        SOAUniBuffer& operator=(SOAUniBuffer&& other) noexcept
        {
            if (this != &other)
            {
                free(memory_region);
                memory_region = other.memory_region;
                blocks = std::move(other.blocks);
                capacity = other.capacity;

                other.memory_region = nullptr;
                other.capacity = 0;
            }

            return *this;
        }

        ~SOAUniBuffer() { free(memory_region); }

    private:
        // alloc:

        u8* memory_region = nullptr;
        std::array<Block, sizeof...(TTypes)> blocks{Block{0u}};
        u32 capacity = 0;

        FORCE_INLINE bool isAllocated() const noexcept { return capacity > 0; }

        // size_t byte_cnt = 0;
        // bool allocated = false;

        inline void createBuffer(u32 in_capacity)
        {
            capacity = in_capacity;
            constexpr size_t al_list[sizeof...(TTypes)] = {alignof(TTypes)...};
            constexpr size_t size_list[sizeof...(TTypes)] = {sizeof(TTypes)...};

            u32 total_size = 0;
            const auto list_sz = sizeof...(TTypes);
            for (int i = 0; i < list_sz; ++i)
            {
                total_size += size_list[i] * capacity + alignment;
            }
            memory_region = (u8*)malloc(total_size); // write guard block
            size_t addr = (size_t)memory_region;     // 80(+64) = 144 (16off)

            u32 cur_offset = (u32)((u64)roundUpToMultiplyOf(addr, alignment) - addr);
            u32 size = cur_offset;

            for (int i = 0; i < blocks.size(); ++i)
            {
                Block& b = blocks[i];
                const auto alof = al_list[i];

                // #todo write override guard to padding
                cur_offset = (u32)roundUpToMultiplyOf((u32)size, (u32)alof);
                size = cur_offset;

                b.first_byte = cur_offset;
                size += size_list[i] * capacity;
            }
        }
    };

} // namespace vex3
