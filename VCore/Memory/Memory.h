#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Containers/Union.h>
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/Utils/TTraits.h>
#include <assert.h>


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

    // Actually it is an allocator handle, 'allocator' is just shorter.
    // IAllocResource is what actually does the thing.
    struct Allocator
    {
        IAllocResource* dyn_alloc = nullptr;

        inline u8* alloc(u64 size, u64 al) { return dyn_alloc->alloc(size, al); }
        inline void dealloc(void* ptr) { return dyn_alloc->dealloc(ptr); }
    };

    struct Mallocator final : public IAllocResource
    {
        using Self = Mallocator;

        static Mallocator* getMallocator()
        {
            static Mallocator dyn_mal;
            return &dyn_mal;
        };
        static Allocator makeAllocatorHandle() { return {getMallocator()}; }

        u8* alloc(u64 sz, u64 al) override { return (u8*)::malloc(sz); }
        void dealloc(void* ptr) override { ::free(ptr); }
    };

    static inline Allocator gMallocator = {Mallocator::getMallocator()};

    // does not own buffer
    template <bool k_abort_on_failure = false>
    struct BumpAllocatorBase : public IAllocResource
    {
        using Self = BumpAllocatorBase;

        u8* buffer_base = nullptr; // does not own the buffer
        u32 top = 0;
        u32 capacity = 0;

        Allocator makeAllocatorHandle() { return {this}; }

        BumpAllocatorBase(u8* in_buffer, u32 buffer_size)
        {
            buffer_base = in_buffer;
            capacity = buffer_size;
        }

        inline u8* alloc(u64 in_size, u64 al) override
        {
            Self* self = this;
            auto al_offset = al - (self->top % al);
            in_size += al_offset;

            u64 new_top = self->top + in_size;
            if (new_top >= self->capacity)
            {
                if constexpr (k_abort_on_failure)
                {
                    std::abort();
                }
                return nullptr;
            }

            u8* mem = (self->buffer_base + self->top + al_offset);

            assert(new_top <= (u32)(-1));
            self->top = (u32)new_top;

            return mem;
        }

        inline void dealloc(void* ptr) override {} // no-op

        void reset() { top = 0; }
    };

    using BumpAllocatorDbg = BumpAllocatorBase<true>;
    using BumpAllocator = BumpAllocatorBase<false>;

    template <u32 buffer_size>
    struct FixedBufferAllocator final : public IAllocResource
    {
        using Self = FixedBufferAllocator;

        u8 buffer[buffer_size];
        BumpAllocator bump{buffer, buffer_size};
        Allocator fallback_allocator;

        Allocator makeAllocatorHandle() { return {this}; }

        FixedBufferAllocator() { fallback_allocator = {Mallocator::getMallocator()}; }
        FixedBufferAllocator(Allocator fallback) : fallback_allocator(fallback) {}

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
    return allocator.dyn_alloc->alloc(size_bytes, al);
}
template <typename T>
static inline T* vexAllocTyped(vex::Allocator& allocator, u64 num, u64 al = (u64)alignof(T))
{
    return (T*)vexAlloc(allocator, num * sizeof(T), al);
}
template <typename T>
static inline void vexAllocTypedOutParam(vex::Allocator& allocator, T*& out_ptr, u64 num, u64 al = (u64)alignof(T))
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