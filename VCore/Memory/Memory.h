#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Containers/Union.h>
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/TTraits.h>
#include <cmath>
#include <string.h>

namespace vex
{
    // #todo ADD DEBUG PAGES

    // ==========================================================================================
    // allocator
    // ==========================================================================================
    struct IAllocResource
    {
        virtual u8* alloc(u64 size, u64 al) = 0;
        virtual void dealloc(void* ptr) = 0;
        virtual ~IAllocResource(){};
    };

    struct Mallocator final : public IAllocResource
    {
        using Self = Mallocator;

        static inline Mallocator* getMallocator()
        {
            static Mallocator dyn_mal;
            return &dyn_mal;
        };

        u8* alloc(u64 sz, u64 al) override { return (u8*)::malloc(sz); }
        void dealloc(void* ptr) override { ::free(ptr); }
    };

    // It is an allocator HANDLE
    // IAllocResource is what actually does the thing.
    // if dyn_alloc is nullptr then malloc/free are used.
    struct Allocator
    {
        IAllocResource* dyn_alloc = nullptr;

        FORCE_INLINE u8* alloc(u64 size, u64 al)
        {
            if (nullptr != dyn_alloc)
                return dyn_alloc->alloc(size, al);
            return (u8*)::malloc(size);
        }
        FORCE_INLINE void dealloc(void* ptr)
        {
            if (nullptr != ptr)
            {
                if (nullptr != dyn_alloc)
                    dyn_alloc->dealloc(ptr);
                else
                    ::free(ptr);
            }
        }
    };

    static inline Allocator makeAllocatorHandle() { return {Mallocator::getMallocator()}; }
    static inline Allocator gMallocator = {Mallocator::getMallocator()};

    // does not own buffer
    template <bool k_abort_on_failure = false>
    struct BumpAllocatorBase : public IAllocResource
    {
        using Self = BumpAllocatorBase;

        struct State
        {
            u8* buffer_base = nullptr; // does not own the buffer
            u32 capacity = 0;
            u32 top = 0;
        } state;

        Allocator makeAllocatorHandle() { return {this}; }

        BumpAllocatorBase() = default;
        BumpAllocatorBase(State in_state) : state(in_state) {}
        BumpAllocatorBase(u8* in_buffer, u32 buffer_size)
        {
            state.buffer_base = in_buffer;
            state.capacity = buffer_size;
        }

        inline u8* alloc(u64 in_size, u64 al) override
        {
            Self* self = this;
            auto al_offset = al - (self->state.top % al);
            in_size += al_offset;

            u64 new_top = self->state.top + in_size;
            if (new_top >= self->state.capacity)
            {
                if constexpr (k_abort_on_failure)
                {
                    std::abort();
                }
                return nullptr;
            }

            u8* mem = (self->state.buffer_base + self->state.top + al_offset);

            self->state.top = (u32)new_top;

            return mem;
        }

        inline void dealloc(void* ptr) override {} // no-op

        void reset() { state.top = 0; }
    };

    using BumpAllocatorDbg = BumpAllocatorBase<true>;
    using BumpAllocator = BumpAllocatorBase<false>;

    template <u32 buffer_size>
    struct InlineBufferAllocator final : public IAllocResource
    {
        using Self = InlineBufferAllocator;

        u8 buffer[buffer_size];
        BumpAllocator bump{buffer, buffer_size};
        Allocator fallback_allocator;

        Allocator makeAllocatorHandle() { return {this}; }

        InlineBufferAllocator() { fallback_allocator = {Mallocator::getMallocator()}; }
        InlineBufferAllocator(Allocator fallback) : fallback_allocator(fallback) {}

        u8* alloc(u64 in_size, u64 al) override
        {
            if (u8* fixed_result = bump.alloc(in_size, al); fixed_result != nullptr)
            {
                return fixed_result;
            }
            return fallback_allocator.alloc(in_size, al);
        }

        void dealloc(void* ptr) override
        {
            Self* self = this;
            if (ptr < self->buffer || ptr > (self->buffer + buffer_size))
            {
                fallback_allocator.dealloc(ptr);
                return;
            }
            bump.dealloc(ptr);
        }

        void reset() { bump.reset(); }
    };

    struct ExpandableBufferAllocator final : public IAllocResource
    {
        using Self = ExpandableBufferAllocator;
        struct BufferHeader
        {
            BufferHeader* prev = nullptr;
            u32 size = 0;
        };

        static constexpr u32 header_size = (u32)sizeof(BufferHeader);

        struct State
        {
            // allocator that gets actual buffers from system or other buffers
            Allocator outer_allocator = {Mallocator::getMallocator()};
            // bump allocator handles tmp allocations in a fast way
            BumpAllocatorBase<false> bump;
            u32 total_reserved = 0;
            float grow_mult = 1.5f;
        } state;

        Allocator makeAllocatorHandle() { return {this}; }

        ExpandableBufferAllocator(){};
        ExpandableBufferAllocator(u32 start_size, State in_state) : state(in_state) { makeNode(start_size); };
        ExpandableBufferAllocator(u32 start_size, float growth_factor = 1.5f) //
        {
            state.grow_mult = growth_factor;
            makeNode(start_size);
        }

        u8* alloc(u64 in_size, u64 al) override
        {
            auto& bump = state.bump;
            for (u8 i = 0; i < 2; ++i)
            {
                if (u8* rptr = bump.alloc(in_size, al); rptr != nullptr)
                {
                    return rptr;
                }

                u64 grow = static_cast<u64>(std::ceil(bump.state.capacity * state.grow_mult));
                u64 new_size = grow > in_size ? grow : in_size;

                makeNode(new_size);
            }

            checkAlwaysRel(false, "should never happen, possibly outer_allocator is at fault.");
            return nullptr;
        }

        void dealloc(void* ptr) override
        {
            // noop
        }
         
        void release()
        {
            auto& bump = state.bump;
            auto& outer_allocator = state.outer_allocator;

            u8* current_buffer = bump.state.buffer_base;
            BufferHeader* node = reinterpret_cast<BufferHeader*>(current_buffer);
            while (node != nullptr && node->prev != nullptr)
            {
                BufferHeader* next_to_dealoc = node->prev;
                // will free whole buffer as header has same adress as buffer[0]
                outer_allocator.dealloc(node);
                node = next_to_dealoc;
            }

            if (node != nullptr)
            {
                bump = BumpAllocatorBase<false>{reinterpret_cast<u8*>(node), node->size};
                bump.state.top = header_size;
                state.total_reserved = node->size;

                node->prev = nullptr;
                memset(bump.state.buffer_base + header_size, 0xff, node->size - header_size);
            }
        }

        void releaseAndReserveUsedSize()
        {
            auto& bump = state.bump;
            auto& outer_allocator = state.outer_allocator;

            BufferHeader* node = reinterpret_cast<BufferHeader*>(bump.state.buffer_base);
            while (node != nullptr)
            {
                BufferHeader* next_to_dealoc = node->prev;
                // will free whole buffer as header has same adress as buffer[0]
                outer_allocator.dealloc(node);
                node = next_to_dealoc;
            }
            bump.state = {};

            makeNode(std::exchange(state.total_reserved, 0));
        }

    private:
        void makeNode(u32 buffer_size)
        {
            auto& bump = state.bump;
            auto& outer_allocator = state.outer_allocator;

            const auto sz = header_size + buffer_size;
            checkAlwaysRel(buffer_size >= 32, "buffer must be at least 32 bytes long.");
            u8* bytes = outer_allocator.alloc(sz, 16);

            BufferHeader* prev = reinterpret_cast<BufferHeader*>(bytes);
            // null by default
            new (prev) BufferHeader{reinterpret_cast<BufferHeader*>(bump.state.buffer_base), sz};

            check_(prev != prev->prev);

            bump = BumpAllocatorBase<false>{bytes, sz};
            bump.state.top = header_size;
            state.total_reserved += sz;
        }
    };

    // enum class EAllocOwns
    //{
    //     Yes,
    //     No,
    //     Mallocated
    // };

    // template <u32 num>
    // struct CompositeAlloc final : public IAllocResource
    //{
    //     Allocator allocators[num];
    //     u8* alloc(u64 in_size, u64 al) override
    //     {
    //         for (u32 i = 0; i < num; ++i)
    //             if (u8* mem = allocators[i].alloc(in_size, al); nullptr != mem)
    //                 return mem;
    //         return nullptr;
    //     }
    //     void dealloc(void* ptr) override
    //     {
    //         for (u32 i = 0; i < num; ++i)
    //         {
    //             if (allocators[i].owns(ptr))
    //             {
    //                 allocators[i].dealloc(ptr);
    //                 return;
    //             }
    //         }
    //     }
    //     bool owns(void* ptr) const override
    //     {
    //         for (u32 i = 0; i < num; ++i)
    //             if (allocators[i].owns(ptr))
    //                 return true;
    //         return false;
    //     }
    // };

} // namespace vex


static inline u8* vexAlloc(vex::Allocator& allocator, u64 size_bytes, u64 al = (u64)alignof(std::max_align_t))
{
    return allocator.alloc(size_bytes, al);
}
template <typename T>
static inline T* vexAllocTyped(vex::Allocator& allocator, u64 num, u64 al = (u64)alignof(T))
{
    return (T*)vexAlloc(allocator, num * sizeof(T), al);
}
template <typename T>
static inline void vexAllocTypedOutParam(
    vex::Allocator& allocator, T*& out_ptr, u64 num, u64 al = (u64)alignof(T))
{
    out_ptr = vexAlloc(allocator, num * sizeof(T), al);
}
template <typename TAllocator>
static inline void vexFree(TAllocator& allocator, void* ptr)
{
    return allocator.dealloc(ptr);
}

#if 0
namespace vex::wip
{


    struct MemoryNode
    {
        MemoryNode* previous = nullptr;
        u32 node_block_size = 0;
        u8* memStart()
        {
            constexpr auto offset = sizeof(MemoryNode) + alignof(max_align_t);
            return ((u8*)this) + offset;
        }

        static MemoryNode* makeNode(MemoryNode* prev = nullptr, u32 in_node_size = 2048)
        {
            constexpr auto offset = sizeof(MemoryNode) + alignof(max_align_t);
            MemoryNode* nd = (MemoryNode*)std::malloc(in_node_size + offset);
            nd->previous = prev;
            nd->node_block_size = in_node_size;
            return nd;
        }
        static void freeNode(MemoryNode* node) { std::free(node); }
    };

    struct FrameAlloc
    {
        using NodeType = MemoryNode;

        NodeType* list_head = nullptr;
        u8* cur_block_top = nullptr;
        u32 default_block_size = 2048;
        u32 total_size = 0;

        u32 total_commited_size = 0;
        u32 cur_block_size = 0;
        u32 dbg_chain_len = 1;

        u64 stackOffsetFromStart() const
        {
            // cur_block_top is a byte after last allocated byte
            return (u64)(cur_block_top - list_head->memStart());
        }

        u8* alloc(u32 size, u32 align)
        {
            u64 req_size = size;
            u64 stack_top = stackOffsetFromStart();
            u64 al_offset = align - (stack_top % align);
            u64 new_stack_top = stack_top + al_offset;

            // add new block if allocation does not fit here
            // ! wasteful, rest of the block is forever lost
            if ((new_stack_top + req_size) > cur_block_size)
            {
                if (req_size >= default_block_size)
                {
                    // commit whole block to allocation
                    list_head = NodeType::makeNode(list_head, req_size);
                    total_commited_size += req_size;
                }
                else
                {
                    list_head = NodeType::makeNode(list_head, default_block_size);
                    total_commited_size += default_block_size;
                }
                cur_block_top = ;

                dbg_chain_len++;
                new_stack_top = 0;
                stack_top = 0;
            }
            cur_block_top = ;

            total_size += al_offset + req_size;
            return mem;
        }

        void free(void* ptr) {} // no-op

        void leak_allocated()
        {
            list_head = NodeType::makeNode(nullptr);
            dbg_chain_len = 1;
        }

        void release()
        {
            NodeType* prev = list_head;
            while (prev != nullptr)
            {
                auto cur = prev;
                prev = prev->previous;
                NodeType::freeNode(cur);
            }
            list_head = NodeType::makeNode(nullptr);
            dbg_chain_len = 1;
        }

        FrameAlloc() { list_head = NodeType::makeNode(nullptr); }
        FrameAlloc(const FrameAlloc&) = delete;
        FrameAlloc(FrameAlloc&& other)
        {
            if (this != &other)
            {
                list_head = other.list_head;
                stack_top = other.stack_top;

                other.list_head = nullptr;
                other.stack_top = 0;
            }
        }
        FrameAlloc& operator=(FrameAlloc&& other)
        {
            if (this != &other)
            {
                NodeType::freeNode(list_head);
                list_head = other.list_head;
                stack_top = other.stack_top;

                other.list_head = nullptr;
                other.stack_top = 0;
            }
        }
        ~FrameAlloc()
        {
            release();
            list_head = nullptr;
        }
    };
}

#endif