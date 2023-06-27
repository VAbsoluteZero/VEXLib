#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include <VCore/Containers/Union.h>
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/VUtilsBase.h>

namespace vex
{
    /* 
    * 
        Simple Static Ring Buffer, meant mainly for PODs.

       *** NOTE: Has 'stack' interface, meaning last added element considered 'first' ***
       
       fill_forward == true means that buffer will grow from 0 to k_capacity, otherwise k_capacity
       to 0. This means that fill_forward is better for insert, !fill_forward - for iteration (you
       can still iterate unordered even with fill_forward).
    */
    template <class T, i32 k_capacity, bool fill_forward = false>
    class StaticRing
    {
        inline auto wrapAroundIndexIfNeeded(i32 i) const -> i32
        {
            i = i % k_capacity;
            if (i < 0)
            {
                i = k_capacity + i;
            }
            return i;
        }
        inline auto growIndex(i32 i) const -> i32
        {
            i32 v = i + k_grow_dir;
            if constexpr (fill_forward)
            {
                if (v >= k_capacity)
                    return 0;
            }
            else
            {
                if (v < 0)
                    return k_capacity - 1;
            }
            return v;
        }
        inline auto shrinkIndex(i32 i) const -> i32
        {
            i32 v = i - k_grow_dir;
            if constexpr (!fill_forward)
            {
                if (v >= k_capacity)
                    return 0;
            }
            else
            {
                if (v < 0)
                    return k_capacity - 1;
            }
            return v;
        }

    public:
        using ValueType = T;
        static constexpr bool k_fill_forward = fill_forward;
        static constexpr i32 k_grow_dir = fill_forward ? 1 : -1;

        static_assert(k_capacity >= 2, //
            "k_capacity must be >= 2, otherwise ring just doesnt make any sense.");

        inline auto is_full() const -> bool { return k_capacity == num_elements; }
        inline auto is_empty() const -> bool { return 0 == num_elements; }
        inline auto size() const -> i32 { return num_elements; }
        inline auto capacity() const -> i32 { return k_capacity; }

        inline auto clear() -> void
        {
            while (!is_empty())
            {
                popDiscard();
            }
            checkAlways_(0 == size());
        }

        template <typename TFwd>
        inline T& push(TFwd&& newItem)
        {
            first_ind = growIndex(first_ind);
            T* curItem = item(first_ind);
            if (num_elements < k_capacity)
            {
                num_elements++;
                new (curItem) T(std::forward<TFwd>(newItem));
            }
            else
            {
                *curItem = std::forward<TFwd>(newItem);
            }
            return *curItem;
        }

        inline void addUninitialized(i32 number)
        {
            static_assert(std::is_trivial_v<T>, "only for trivial POD types");
            num_elements += number;
            num_elements = num_elements >= k_capacity ? k_capacity : num_elements;
        }
        inline void addDefaulted(i32 number)
        {
            static_assert(std::is_default_constructible_v<T>, "incompatible with type");
            for (i32 i = 0; i < number; ++i)
                put(T{});
        }

        [[nodiscard]] inline auto popUnchecked() -> T
        {
            checkAlways_(num_elements > 0);

            auto top_item = item(first_ind);
            auto tmp = std::move(*top_item);
            top_item->~T();

            --num_elements;
            first_ind = shrinkIndex(first_ind);

            return tmp;
        }
        [[nodiscard]] inline auto pop() -> vex::Option<T>
        {
            if (num_elements <= 0)
                return {};

            auto top_item = item(first_ind);
            Option<T> out_val{std::move(*top_item)};
            top_item->~T();

            --num_elements;
            first_ind -= k_grow_dir;
            first_ind = wrapAroundIndexIfNeeded(first_ind);

            return out_val;
        }
        // maybe pop back?
        [[nodiscard]] inline auto dequeueUnchecked() -> T
        {
            checkAlways_(num_elements > 0);

            auto top_item = item(toRawIndex(num_elements - 1));
            auto tmp = std::move(*top_item);
            top_item->~T();

            --num_elements;

            return tmp;
        }
        [[nodiscard]] inline auto dequeue() -> vex::Option<T>
        {
            if (num_elements <= 0)
                return {};

            auto top_item = item(toRawIndex(num_elements - 1));
            Option<T> out_val{std::move(*top_item)};
            top_item->~T(); 
            --num_elements;  

            return out_val;
        }

        inline void popDiscard()
        {
            T* top_item = peek();
            if (nullptr != top_item)
            {
                top_item->~T();
                --num_elements;
                first_ind -= k_grow_dir;
                first_ind = wrapAroundIndexIfNeeded(first_ind);
            }
        }

        inline auto peek() -> T*
        {
            if (num_elements <= 0)
                return nullptr;
            return item(first_ind);
        }
        inline auto peek() const -> const T*
        {
            if (num_elements <= 0)
                return nullptr;
            return item(first_ind);
        }
        inline auto peekUnchecked() -> T& { return *(item(first_ind)); }

        StaticRing(){};

        template <typename... Args>
        StaticRing(Args&&... elems)
        {
            (this->push(std::forward<Args>(elems)), ...);
        }

        StaticRing(StaticRing& other) : StaticRing{const_cast<StaticRing const&>(other)} {}
        StaticRing(const StaticRing& other) : num_elements(0), first_ind(0)
        {
            if (&other == this)
                return;

            deepCopyFrom(other);
        }
        StaticRing& operator=(const StaticRing& other)
        {
            if (&other == this)
                return *this;
            deepCopyFrom(other);
            return *this;
        }

        StaticRing(StaticRing&& other) = delete;
        ~StaticRing()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                clear();
            }
        }

        T& at(i32 i)
        {
            i32 rel = toRawIndex(i);
            checkAlways_(rel >= 0 && rel < k_capacity);
            return data_typed[rel];
        }
        const T& at(i32 i) const
        {
            i32 rel = toRawIndex(i);
            checkAlways_(rel >= 0 && rel < k_capacity);
            return data_typed[rel];
        }

        const T* rawDataUnsafe() const { return data_typed; }

        // last added element considered 'first'
        // do not use this type directly, it is meant for ranged for exclusively
        template <bool Const>
        struct SqIterator
        {
            using RingType =
                typename AddConst<StaticRing<T, k_capacity, fill_forward>, Const>::type;
            // as it is used in It == End by range-for loop, it is esentialy stop condition
            friend auto operator==(SqIterator lhs, impl::vxSentinel rhs) { return lhs.isDone(); }
            friend auto operator==(impl::vxSentinel lhs, SqIterator rhs) { return rhs == lhs; }
            friend auto operator!=(SqIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
            friend auto operator!=(impl::vxSentinel lhs, SqIterator rhs) { return !(lhs == rhs); }
            bool isDone() const { return head_offset >= ring.num_elements; }

            inline decltype(auto) operator*() const { return ring.at(head_offset); }
            inline decltype(auto) operator++()
            {
                head_offset += 1;
                return *this;
            }

            RingType& ring;
            i32 head_offset = 0;

            SqIterator(const SqIterator&) = default;
            SqIterator(RingType& ring) : ring(ring) {}
        };
        auto begin() noexcept { return SqIterator<false>{*this}; };
        auto end() const noexcept { return k_seq_end; };
        auto begin() const noexcept { return SqIterator<true>{*this}; };
        // first added element considered 'first'(in contrast to SqIterator)
        // do not use this type directly, it is meant for ranged for exclusively
        template <bool Const>
        struct RevIterator
        {
            using RingType =
                typename AddConst<StaticRing<T, k_capacity, fill_forward>, Const>::type;
            // as it is used in It == End by range-for loop, it is esentialy stop condition
            friend auto operator==(RevIterator lhs, impl::vxSentinel rhs) { return lhs.isDone(); }
            friend auto operator==(impl::vxSentinel lhs, RevIterator rhs) { return rhs == lhs; }
            friend auto operator!=(RevIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
            friend auto operator!=(impl::vxSentinel lhs, RevIterator rhs) { return !(lhs == rhs); }
            bool isDone() const { return tail_offset >= ring.num_elements; }

            inline decltype(auto) operator*() const
            {
                return ring.at(ring.num_elements - tail_offset - 1);
            }
            inline decltype(auto) operator++()
            {
                tail_offset += 1;
                return *this;
            }

            RingType& ring;
            i32 tail_offset = 0;

            RevIterator(const RevIterator&) = default;
            RevIterator(RingType& in_ring) : ring(in_ring) {}
        };
        auto rbegin() noexcept { return RevIterator<false>{*this}; };
        auto rend() const noexcept { return k_seq_end; };
        auto rbegin() const noexcept { return RevIterator<true>{*this}; };

        template <bool Const>
        struct ReverseEnumerable
        {
            auto begin() noexcept { return RevIterator<false>{*ring}; };
            auto end() const noexcept { return k_seq_end; };
            auto begin() const noexcept { return RevIterator<true>{*ring}; };
            using RingType =
                typename AddConst<StaticRing<T, k_capacity, fill_forward>, Const>::type;
            RingType* ring;
        };
        auto reverseEnum() noexcept { return ReverseEnumerable<false>{this}; };
        auto reverseEnum() const noexcept { return ReverseEnumerable<true>{this}; };

    private:
        static constexpr auto k_align = alignof(T) < 8 ? 8 : alignof(T);
        union
        {
            alignas(k_align) byte data_bytes[k_capacity * sizeof(T)];
            alignas(k_align) T data_typed[k_capacity];
        };

        inline auto toRawIndex(i32 i) const -> i32
        { //
            i32 rel = first_ind - i * k_grow_dir;
            return wrapAroundIndexIfNeeded(rel);
        }

        inline void deepCopyFrom(const StaticRing& other)
        {
            clear();
            if (other.is_full())
            {
                this->num_elements = other.num_elements;
                this->first_ind = other.first_ind;
                for (i32 i = 0; i < num_elements; ++i)
                {
                    new (item(i)) T(*(other.item(i)));
                }
            }
            else
            {
                this->num_elements = other.num_elements;
                this->first_ind = other.first_ind;
                auto cur = first_ind;
                for (i32 i = num_elements; i > 0; --i)
                {
                    new (item(cur)) T(*(other.item(cur)));
                    cur -= k_grow_dir;
                    cur = wrapAroundIndexIfNeeded(cur);
                }
            }
        }

        inline T* item(i32 i) { return &data_typed[i]; }
        inline const T* item(i32 i) const { return &data_typed[i]; }

        i32 num_elements = 0;
        // index of a 'first' element in ring/stack (meaning last added)
        // initialized with invalid index, that would be valid after first 'put'
        i32 first_ind = fill_forward ? -1 : k_capacity;
    };

} // namespace vex
