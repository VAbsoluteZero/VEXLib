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
        StorageBase(tMask m, tTypeID tid) : kMask(m), kTypeID(tid){};
        virtual ~StorageBase(){};
        virtual bool remove(EntityHandle h) = 0;
        virtual bool Clone(EntityHandle original, EntityHandle newOne) = 0;

        virtual std::unique_ptr<StorageBase> DuplicateSelf() = 0;
        virtual bool MoveFrom(StorageBase* other) = 0;

        virtual void Clear() = 0;

        const tMask kMask = 0;
        const tTypeID kTypeID = 0;
    };

    // ! #todo change API to resemble Chandler Carruth's idea of hashtable api
    template <class TComp>
    class Storage final : public StorageBase
    {
    public:
        inline TComp* Find(EntityHandle handle) const noexcept { return _data.tryGet(handle); }

        static inline TComp* Find(const Storage<TComp>& store, EntityHandle handle) noexcept
        {
            return store.Find(handle);
        }

        template <typename T = TComp>
        inline typename std::enable_if_t<std::is_default_constructible<T>::value, T&> get(EntityHandle handle)
        {
            return _data[handle];
        }

        template <typename T = TComp>
        inline typename std::enable_if_t<std::is_default_constructible<T>::value, T*> First()
        {
            return _data.Any();
        }

        template <class... TArgs>
        inline TComp& Emplace(EntityHandle handle, TArgs&&... arguments)
        {
            // create or replace
            return _data.emplaceAndGet(handle, std::forward<TArgs>(arguments)...);
        }

        // Inherited via StorageBase
        virtual bool Clone(EntityHandle original, EntityHandle newOne) override
        {
            if (!_data.contains(original))
                return false;
            TComp& ncomp = *_data.tryGet(original);
            _data.emplace(newOne, ncomp);
            ;
            return true;
        }
        virtual bool remove(EntityHandle handle) override { return _data.remove(handle); }

        virtual void Clear() override { _data.clear(); }

        struct DataEnumerable
        {
            auto begin() { return _owner._data.begin(); }
            auto end() { return _owner._data.end(); }

        private:
            DataEnumerable(const Storage& owner) : _owner(owner) {}
            const Storage& _owner;
            friend class Storage;
        };

        DataEnumerable Enumerable() { return *this; } // implicit conversion to enumerable
        // Inherited via StorageBase
        virtual std::unique_ptr<StorageBase> DuplicateSelf() override
        {
            Storage* inst = new Storage();
            inst->_data = _data;
            std::unique_ptr<StorageBase> ret(inst);
            return std::move(ret);
        }

        virtual bool MoveFrom(StorageBase* other) override
        {
            if (!other || other->kMask != kMask)
                return false;

            // should be safe to do after above type check,
            // and it is only way if RTTI is disabled
            Storage* casted = static_cast<Storage*>(other);
            _data = std::move(casted->_data);
            return true;
        }


        Storage() : StorageBase(TComp::Mask, tinfo::typeID<TComp>()) {}

        virtual ~Storage() {}

    private:
        // underlying container may (and should) be replaced to tailor-made storage,
        // this will do for now though. Usually I would have direct lookup table and raw
        // array with it, but since this (Dict) hashtable implementation
        // stores elements in continuous memory region thats ok.

        Dict<EntityHandle, TComp, EntHandleHasher> _data;
        // const int GenID = 0;
    };
} // namespace vex
