#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Containers/Dict.h>
#include <VCore/World/Entity.h>

#include <memory>

namespace vex
{
    class StorageBase
    {
    public:
        StorageBase(CompMaskType m, CompIdType tid, Allocator in_alloc)
            : allocator(in_alloc), mod_mask(m), type_id(tid){};
        virtual ~StorageBase(){};
        virtual bool remove(EntityHandle h) = 0;
        virtual bool clone(EntityHandle original, EntityHandle newOne) = 0;
        virtual bool stealDataFrom(StorageBase* other) = 0;
        virtual void clear() = 0;

        Allocator allocator;
        const CompMaskType mod_mask = 0;
        const CompIdType type_id = 0;
    };

    // #fixme - temporary impl. real bad. to be completely replaced

    template <class TComp>
    class Storage final : public StorageBase
    {
    public:
        virtual ~Storage() {}
        Storage() : StorageBase(TComp::type_id, TComp::mod_mask, {}) {}
        // #fixme - replace with sparse set
        Dict<EntityHandle, TComp, EntHandleHasher> data{CompTraits<TComp>::default_cap, allocator};

        inline TComp* find(EntityHandle handle) const noexcept { return data.tryGet(handle); }

        static inline TComp* find(const Storage<TComp>& store, EntityHandle handle) noexcept
        {
            return store.find(handle);
        }

        template <typename T = TComp>
        inline typename std::enable_if_t<std::is_default_constructible<T>::value, T&> get(
            EntityHandle handle)
        {
            return data[handle];
        }

        template <class... TArgs>
        inline TComp& Emplace(EntityHandle handle, TArgs&&... arguments)
        {
            // create or replace
            return data.emplaceAndGet(handle, std::forward<TArgs>(arguments)...);
        }

        // Inherited via StorageBase
        virtual bool clone(EntityHandle original, EntityHandle newOne) override
        {
            if (!data.contains(original))
                return false;
            TComp& ncomp = *data.find(original);
            data.emplace(newOne, ncomp);
            ;
            return true;
        }
        virtual bool remove(EntityHandle handle) override { return data.remove(handle); }

        virtual void clear() override { data.clear(); }

        struct DataEnumerable
        {
            auto begin() { return _owner.data.begin(); }
            auto end() { return _owner.data.end(); }

        private:
            DataEnumerable(const Storage& owner) : _owner(owner) {}
            const Storage& _owner;
            friend class Storage;
        };

        DataEnumerable Enumerable() { return *this; } // implicit conversion to enumerable

        virtual bool stealDataFrom(StorageBase* other) override
        {
            if (!other || other->mod_mask != mod_mask)
                return false;

            // should be safe to do after above type check,
            // and it is only way if RTTI is disabled
            Storage* casted = static_cast<Storage*>(other);
            data = std::move(casted->data);
            return true;
        }
    };
} // namespace vex
