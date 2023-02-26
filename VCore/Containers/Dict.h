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
#include <VCore/Containers/SOAJoint.h>
#include <VCore/Utils/HashUtils.h>
#include <VCore/Utils/VUtilsBase.h>

#include <optional>
#include <vector>

#ifdef VEXCORE_x64
    #include <VCore/Dependencies/fastmod.h>
#endif

#ifndef FORCE_INLINE
    #if defined(_MSC_VER)

        #define FORCE_INLINE __forceinline

    #else // defined(_MSC_VER)

        #define FORCE_INLINE inline __attribute__((always_inline))

    #endif
#endif // ! FORCE_INLINE

namespace vex
{
    template <typename TKey>
    struct hasher
    {
        inline static i32 hash(const TKey& key)
        {
            // '-val' hash range is reserved for
            // empty records (effectively taking one bit from .hash field)
            return (int)std::hash<TKey>{}(key);
        }
    };
    template <>
    struct hasher<int>
    {
        inline static i32 hash(const int& key) { return key; }
    };

    template <>
    struct hasher<std::string>
    {
        inline static i32 hash(const std::string& key)
        {
            return murmur::MurmurHash3_x86_32(key.data(), (int)key.size());
        }
    };
    template <>
    struct hasher<const char*>
    {
        inline static i32 hash(const char* key) { return vex::util::fnv1a(key); }
    };
    template <>
    struct hasher<char*>
    {
        inline static i32 hash(char* key) { return vex::util::fnv1a(key); }
    };
    struct DSentinel
    {
    };
    static constexpr const DSentinel kEndIteratorSentinel{};
    /*
     * Simple and readable Hashtable/Map/Dictionary implementation.
     * Loosely based on the .net (C#) dictionary implementation,
     * optimized for fast insert/iteration/lookup. Uses linear probing scheme.
     * It preforms 1.5x to 10x faster than std::unordered_map in certain situations.
     * It is about 2x to 8x faster than  UnrealEngine's TMap in Development config.
     * All around it is much faster to fill and iterate through it
     * regardless of element size due to cache friendliness.
     * Lookup for an element takes about the same time.
     *
     * Data storage itself is a flat SOA-like structure, all logical parts of the dictionary (buckets, recs..)
     * are allocated inside same buffer/memory region.
     * There are some checks to ensure that POD-like data is being processed as such (e.g. memcpy'd).
     * This container does not require neither Key nor Val to have default constructor.
     *
     * NOTE (1): this container does NOT resemble unordered_map in a way it is allocated -
     * it is using Flat memory buffer for all elements (instead of just storing pointers and
     * allocating storage for elements later), so if you are using HUGE (lets say
     * more than 2048 bytes) structures and lots of them - it could be better
     * to use std version instead OR consider supplying different allocator.
     * Otherwise there could be spikes on alloc/realloc or free.
     * Basically allocating 2MB+ upfront could be expensive.
     */

    template <typename TBuckets, typename TCtrlBlock, typename TRecord>
    class DictAllocator
    {
        static const size_t alignment = vex::maxAlignOf<TBuckets, TCtrlBlock, TRecord>();

    public:
        DictAllocator() = delete;

        explicit DictAllocator(vex::Allocator in_alloc, u32 in_cap) noexcept : allocator(in_alloc)
        {
            checkLethal(in_cap > 0, "invalid capacity");

            capacity = in_cap;
            constexpr size_t size_list[3] = {sizeof(TBuckets), sizeof(TCtrlBlock), sizeof(TRecord)};

            u32 total_size = 0;
            for (int i = 0; i < 3; ++i)
            {
                total_size += (u32)(size_list[i] * capacity + alignment * 2);
            }

            // #todo write guard block
            auto memory_region = (u8*)in_alloc.alloc(total_size, alignof(TRecord));
            buckets = reinterpret_cast<TBuckets*>(memory_region);
            checkLethal(buckets, "failure of allocator");

            // 'buckets' should be at the start and considered owning ptr
            u32 new_top = sizeof(TBuckets) * in_cap;

            vex::BumpAllocator bumper{memory_region, total_size};
            bumper.state.top = new_top;

            auto bumper_handle = bumper.makeAllocatorHandle();
            this->blocks = vexAllocTyped<TCtrlBlock>(bumper_handle, in_cap);
            this->recs = vexAllocTyped<TRecord>(bumper_handle, in_cap);
            bool invalid = (recs == nullptr) || (blocks == nullptr) || (buckets == nullptr);

            checkLethal(!invalid, " bug: buffer could not contain all of the arrays ");
        }

        DictAllocator(const DictAllocator& other) = delete;
        DictAllocator(DictAllocator&& other) noexcept { *this = std::move(other); }
        DictAllocator& operator=(DictAllocator&& other) noexcept
        {
            if (this != &other)
            {
                if (buckets)
                    allocator.dealloc(buckets);
                capacity = other.capacity;

                buckets = other.buckets;
                blocks = other.blocks;
                recs = other.recs;
                allocator = other.allocator;

                other.buckets = nullptr;
                other.blocks = nullptr;
                other.recs = nullptr;
            }

            return *this;
        }

        ~DictAllocator()
        {
            if (buckets)
                allocator.dealloc(buckets);
        }

        auto bucketsBuffer() const { return RawBuffer<TBuckets>{buckets, capacity}; }
        auto blocksBuffer() const { return RawBuffer<TCtrlBlock>{blocks, capacity}; }
        auto recordsBuffer() const { return RawBuffer<TRecord>{recs, capacity}; }

        // this pointer is OWNING and should be free'd
        vex::Allocator allocator;
        TBuckets* buckets = nullptr;
        TCtrlBlock* blocks = nullptr;
        TRecord* recs = nullptr;
        u32 capacity = 0;
    };

    // #todo partially specialize for strings
    // #TODO: implement shrink/realloc
    template <typename TKey, typename TVal, typename THasher = hasher<TKey>>
    class alignas(64) Dict
    {
    public:
        typedef TKey KeyType;
        typedef TVal ValueType;
        struct Record
        {
            TKey key;
            TVal value;
        };
        struct ControlBlock
        {
            i32 hash = -1;
            i32 next = -1;
        };

        using CombinedStorage = DictAllocator<i32, ControlBlock, Record>;

        FORCE_INLINE i32 size() const noexcept { return top_idx - free_count; }
        FORCE_INLINE u32 capacity() const noexcept { return data.capacity; }

        Dict(u32 in_capacity = 7, vex ::Allocator in_alloc = {})
            : data(in_alloc, vex::util::closestPrimeSearch(in_capacity))
        {
            refreshState();

            std::fill_n(data.buckets, capacity(), -1);
            auto b = ControlBlock{-1, -1};
            std::fill_n(data.blocks, capacity(), b);
        }
        Dict(std::initializer_list<Record> initlist, vex ::Allocator in_alloc = {})
            : data(in_alloc, vex::util::closestPrimeSearch(std::size(initlist)))
        {
            refreshState();

            std::fill_n(data.buckets, capacity(), -1);
            auto b = ControlBlock{-1, -1};
            std::fill_n(data.blocks, capacity(), b);
            for (auto&& rec : initlist)
                emplace(rec.key, rec.value);
        }
        Dict(const Dict& other) : data(other.data.allocator, other.capacity())
        {
            refreshState();

            top_idx = other.top_idx;
            free_idx = other.free_idx;
            free_count = other.free_count;

            RawBuffer<i32> o_buckets = other.data.bucketsBuffer();
            RawBuffer<ControlBlock> o_blocks = other.data.blocksBuffer();
            RawBuffer<Record> o_recs = other.data.recordsBuffer();

            RawBuffer<i32> s_buckets = data.bucketsBuffer();
            RawBuffer<ControlBlock> s_blocks = data.blocksBuffer();
            RawBuffer<Record> s_recs = data.recordsBuffer();

            o_buckets.copyTo(s_buckets, {});
            o_blocks.copyTo(s_blocks);

            if constexpr (std::is_trivially_copyable<Record>::value)
            {
                std::memcpy(s_recs.first, o_recs.first, o_recs.capacity * sizeof(Record));
            }
            else
            {
                for (i32 i = 0; i < o_recs.size(); ++i)
                {
                    if (o_blocks[i].hash >= 0)
                    {
                        new (&s_recs[i]) Record(o_recs[i]);
                    }
                }
            }
        }

        Dict& operator=(const Dict& other)
        {
            if (this != &other)
            {
                Dict tmp(other);
                *this = std::move(tmp);
            }

            return *this;
        }

        Dict(Dict&& other) { *this = std::move(other); }

        Dict& operator=(Dict&& other)
        {
            if (this != &other)
            {
                this->clear();
                data = std::move(other.data);

                top_idx = other.top_idx;
                free_idx = other.free_idx;
                free_count = other.free_count;
                refreshState();

                // reset other
                other.top_idx = 0;
                other.free_count = 0;
                other.free_idx = 0;
                other.data = CombinedStorage(other.data.allocator, vex::util::closestPrimeSearch(7));

                other.refreshState();
                std::fill_n(other.data.buckets, other.data.capacity, -1);
                auto b = ControlBlock{-1, -1};
                std::fill_n(other.data.blocks, other.data.capacity, b);
            }

            return *this;
        }

        ~Dict()
        {
            if constexpr (!std::is_trivially_destructible<Record>::value)
            {
                for (u32 i = 0; i < capacity(); ++i)
                {
                    if (data.blocks[i].hash >= 0)
                        data.recs[i].~Record();
                    data.blocks[i].hash = -1;
                }
            }
        }

        template <typename T = TVal>
        inline T* any() const
        {
            if (size() > 0)
            {
                for (u32 i = 0; i < size(); ++i)
                {
                    if (data.blocks[i].hash > 0)
                        return &data.recs[i].value;
                }
            }

            return nullptr;
        }

        template <class... Types>
        inline void emplace(const TKey& key, Types&&... arguments)
        {
            i32 i = findRec(key);
            if (i >= 0)
            {
                data.recs[i].value.~TVal();
                new (&data.recs[i].value) TVal(std::forward<Types>(arguments)...);
            }
            else
            {
                Record& r = createRecord(key);
                new (&r.value) TVal(std::forward<Types>(arguments)...);
            }
        }
        static inline volatile i32 dbg;
        template <class... Types>
        inline TVal& emplaceAndGet(const TKey& key, Types&&... arguments)
        {
            i32 i = findRec(key);
            if (i >= 0)
            {
                data.recs[i].value.~TVal();
                new (&data.recs[i].value) TVal(std::forward<Types>(arguments)...);
                return data.recs[i].value;
            }
            else
            {
                Record& r = createRecord(key);
                new (&r.value) TVal(std::forward<Types>(arguments)...);
                return r.value;
            }
        }

        template <typename T = TVal>
        inline typename std::enable_if_t<std::is_default_constructible<T>::value, TVal> valueOrDefault(
            const TKey& key) const
        {
            i32 ind = findRec(key);
            return ind >= 0 ? data.recs[ind].value : TVal();
        }

        inline TVal* tryGet(const TKey& key) const noexcept
        {
            i32 ind = findRec(key);
            return ind >= 0 ? &data.recs[ind].value : nullptr;
        }

        bool remove(const TKey& key) noexcept
        {
            i32 hash_ccode = THasher::hash(key) & 0x7FFFFFFF;
            i32 bucket = mod(hash_ccode, capacity());
            i32 previous = -1;

            for (i32 i = data.buckets[bucket]; i >= 0; previous = i, i = data.blocks[i].next)
            {
                if (data.blocks[i].hash == hash_ccode && (data.recs[i].key == key))
                {
                    // only
                    if (previous < 0)
                    {
                        data.buckets[bucket] = data.blocks[i].next;
                    }
                    // middle or last
                    else
                    {
                        data.blocks[previous].next = data.blocks[i].next;
                    }

                    Record& entry = data.recs[i];
                    {
                        data.blocks[i].hash = -1;
                        data.blocks[i].next = free_idx;

                        entry.key.~TKey();
                        entry.value.~TVal();
                    }
                    free_idx = i;
                    free_count++;

                    return true;
                }
            }

            return false;
        }

        inline void clear()
        {
            if (top_idx == 0) // it is already empty
                return;

            free_count = 0;
            free_idx = -1;
            top_idx = 0;

            if constexpr (std::is_trivially_destructible<Record>::value)
            {
                // basically do nothing
            }
            else
            {
                for (u32 i = 0; i < capacity(); ++i)
                {
                    if (data.blocks[i].hash >= 0)
                        data.recs[i].~Record();
                }
            }

            std::fill_n(data.buckets, capacity(), -1);
            auto b = ControlBlock{-1, -1};
            std::fill_n(data.blocks, capacity(), b);
        }


        struct DIterator
        {
            FORCE_INLINE bool advance()
            {
                i32 count = owner_map.size();
                while (index < (count - 1)) [[likely]]
                {
                    index++;
                    const auto hash = owner_map.data.blocks[index].hash;
                    if (hash >= 0) [[likely]]
                    {
                        return true;
                    }
                }

                index = owner_map.size() + 1;
                return false;
            }

            FORCE_INLINE bool isDone() const { return index >= (owner_map.size()); }

            friend auto operator==(DIterator lhs, DSentinel rhs) { return lhs.isDone(); }
            friend auto operator==(DSentinel lhs, DIterator rhs) { return rhs == lhs; }
            friend auto operator!=(DIterator lhs, DSentinel rhs) { return !(lhs == rhs); }
            friend auto operator!=(DSentinel lhs, DIterator rhs) { return !(lhs == rhs); }

            inline Record& operator*() const { return current(); }
            inline auto& operator++()
            {
                advance();
                return *this;
            }

            inline Record& current() const
            {
                // checkAlways_(_index < _map.Size());
                return *(owner_map.data.recs + index);
            }

        private:
            DIterator(const Dict& owner) : owner_map(owner) {}
            const Dict& owner_map;
            i32 index = 0;

            friend class Dict;
        };

        FORCE_INLINE DIterator begin() const noexcept { return DIterator{*this}; };

        FORCE_INLINE DIterator begin() noexcept { return DIterator{*this}; };
        FORCE_INLINE DSentinel end() const noexcept { return kEndIteratorSentinel; };

        template <typename T = TVal>
        inline typename std::enable_if_t<std::is_default_constructible<T>::value, T&> operator[](
            const TKey& key)
        {
            i32 i = findRec(key);

            if (i < 0)
            {
                Record& r = createRecord(key);
                new (&r.value) TVal();
                return r.value;
            }

            return data.recs[i].value;
        }
        FORCE_INLINE bool contains(const TKey& item) const { return findRec(item) >= 0; }

    private:
        FORCE_INLINE i32 findRec(const TKey& key) const noexcept
        {
            // ensure abs value
            i32 hash_code = THasher::hash(key) & 0x7FFFFFFF;
            i32 bucket_index = mod(hash_code, (int)capacity());

            for (i32 i = data.buckets[bucket_index]; i >= 0; i = data.blocks[i].next)
            {
                if constexpr (std::is_same_v<TKey, char*> || std::is_same_v<TKey, const char*>)
                {
                    if (data.blocks[i].hash == hash_code)
                        if (strcmp(data.recs[i].key, key) == 0)
                            return i;
                }
                else
                {
                    if (data.blocks[i].hash == hash_code)
                        if (data.recs[i].key == key)
                            return i;
                }
            }
            return -1;
        }


        CombinedStorage data;
        // end of used space (0 <= top_idx < cap), grows when andding element and
        // [0, top_idx] area all filled up
        i32 top_idx = 0;
        // 'hole' in the used space
        i32 free_idx = 0;
        // num of 'holes' in the used space
        i32 free_count = 0;

#ifdef VEXCORE_x64
        uint64_t fastmod_m = 0;
#endif
        inline void refreshState()
        {
#ifdef VEXCORE_x64
            fastmod_m = fastmod::computeM_s32(capacity());
#endif
        }

        FORCE_INLINE i32 mod(i32 a, i32 b) const noexcept
        {
#ifdef VEXCORE_x64
            return fastmod::fastmod_s32(a, fastmod_m, b);
#else
            return a % b;
#endif
        }

        void grow()
        {
            using namespace vex::util;
            const auto new_cap = data.capacity + data.capacity / 2; // grows by factor of 1.5
            i32 new_size = closestPrimeSearch((i32)(new_cap + 1));

            // checkAlwaysRel(new_size == data.capacity, "max number of elements reached");

            CombinedStorage new_data(data.allocator, new_size);
            RawBuffer<Record> new_recs = new_data.recordsBuffer();
            auto s_hash_blocks = data.blocksBuffer();

            if constexpr (std::is_trivially_copyable<Record>::value)
            {
                std::memcpy(new_recs.first, data.recs, capacity() * sizeof(Record));
            }
            else if constexpr (std::is_move_constructible<Record>::value)
            {
                for (u32 i = 0; i < capacity(); ++i)
                {
                    if (data.blocks[i].hash >= 0)
                    {
                        new (&new_recs[i]) Record(std::move(data.recs[i]));
                        data.recs[i].~Record();
                    }
                }
            }
            else
            {
                for (u32 i = 0; i < capacity(); ++i)
                {
                    if (data.blocks[i].hash >= 0)
                    {
                        new (&new_recs[i]) Record(data.recs[i]);
                        data.recs[i].~Record();
                    }
                }
            }

            s_hash_blocks.copyTo(new_data.blocksBuffer(), ControlBlock{});

            data = std::move(new_data);

            refreshState();

            std::fill_n(data.buckets, capacity(), -1);

            for (i32 i = 0; i < top_idx; i++)
            {
                if (data.blocks[i].hash >= 0)
                {
                    i32 bucket = mod(data.blocks[i].hash, new_size); // == hash % size

                    data.blocks[i].next = data.buckets[bucket];
                    data.buckets[bucket] = i; // old i-th element hash would not lead here
                }
            }
        }

        inline Record& createRecord(const TKey& key)
        {
            i32 hash_code = THasher::hash(key) & 0x7FFFFFFF;
            i32 bucket_ind = mod(hash_code, (int)capacity());
            i32 index = 0;

            if (free_count > 0)
            {
                index = free_idx;
                free_idx = data.blocks[index].next;
                free_count--;
            }
            else
            {
                // capacity reached
                if (top_idx == capacity()) [[unlikely]]
                {
                    grow();
                    bucket_ind = mod(hash_code, (int)capacity());
                }
                index = top_idx;
                top_idx++;
            }

            Record& entry = data.recs[index];
            {
                data.blocks[index].hash = hash_code;
                data.blocks[index].next = data.buckets[bucket_ind];

                new (&entry.key) TKey(key);
                // value should be initialized in getter
            }
            data.buckets[bucket_ind] = index;
            return entry;
        }
    };

} // namespace vex
