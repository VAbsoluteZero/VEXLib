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
#include <VCore/Containers/Tuple.h>
#include <VCore/Utils/CoreTemplates.h>
#include <VCore/World/IGlobal.h>
#include <VCore/World/Storage.h>

#include <functional>
#include <string>

namespace vex
{
    class World;
    using FnEntityConstructor = std::function<bool(World&, EntityHandle)>;

    struct StoragePtr
    {
        const CompIdType type_id;
        const CompMaskType mod_mask;

        template <typename T>
        T* const castTo() const
        {
            return static_cast<T*>(storage); // no need for dynamic, types guaranteed to match
        }
        StorageBase* const get() const { return storage; }
        StoragePtr(CompIdType id, StorageBase* storage) noexcept
            : type_id(id), mod_mask(storage->mod_mask), storage(storage)
        {
        }
        StoragePtr(StoragePtr&& other) noexcept : type_id(other.type_id), mod_mask(other.mod_mask)
        {
            *this = std::move(other);
        }

        StoragePtr(const StoragePtr&) = delete;
        StoragePtr& operator=(StoragePtr&& other) noexcept;
        ~StoragePtr()
        {
            delete storage;
            storage = nullptr;
        }

    private:
        StorageBase* storage = nullptr;
    };

    class World
    {
    public:
        Dict<std::string, FnEntityConstructor> constructors;
        Dict<CompIdType, StoragePtr> storages;

        std::vector<i32> free_list;
        std::vector<EntityHandle> ent_handles;
        std::vector<EntComponentSet> component_sets;

        World(int expectedMaxEntCount = 1028)
        {
            free_list.reserve((expectedMaxEntCount / 2));
            ent_handles.reserve(expectedMaxEntCount);
            ent_handles.resize(1); //'Null-Object'

            component_sets.reserve(expectedMaxEntCount);
            component_sets.resize(1); //'Null-Object'
        }

        FORCE_INLINE bool contains(EntityHandle handle)
        {
            return handle.id < ent_handles.size() && (handle == ent_handles[handle.id]);
        }
        template <class TComp>
        inline TComp* const find(EntityHandle handle)
        {
            auto& comp_set = component_sets[handle.id];
            if (!contains(handle) || !comp_set.has<TComp>())
                return nullptr;
            return getStorage<TComp>().find(handle);
        }
        template <class TComp>
        inline TComp& get(EntityHandle handle)
        {
            auto& comp_set = component_sets[handle.id];
            if (!contains(handle) || !comp_set.has<TComp>())
                checkAlwaysRel(false, "component does not exist");

            return getStorage<TComp>().get(handle);
        }

        template <class... TTypes>
        inline Tuple<TTypes*...> findComponents(EntityHandle handle)
        {
            if (!contains(handle))
                return {};

            return Tuple<TTypes*...>(getStorage<TTypes>().find(handle)...);
        }

        template <class TComp>
        Storage<TComp>& getStorage() noexcept
        {
            auto type_id = TComp::type_id;
            auto* base_ptr = storages.find(type_id);
            if (nullptr != base_ptr)
                return *(base_ptr->template castTo<Storage<TComp>>());

            // #todo add CompTraits to set default storage cap per type
            Storage<TComp>* storage = new Storage<TComp>();
            storages.emplace(type_id, StoragePtr{type_id, storage});
            return *storage;
        }

        EntityHandle createBlank();
        EntityHandle createBlank(const char* name);
        bool destroy(EntityHandle handle);
        inline int size() const { return (int)(ent_handles.size() - free_list.size()); }

        void clear()
        {
            component_sets.resize(1);
            free_list.resize(0);
            storages.clear();
            constructors.clear();
        }

        template <class TComp>
        inline bool remove(EntityHandle handle)
        {
            if (!contains(handle))
                return false;

            component_sets[handle.id].bitset.unset<TComp>();
            return getStorage<TComp>().remove(handle);
        }
        template <class TComp, class... TArgs>
        inline TComp& add(EntityHandle handle, TArgs&&... arguments)
        {
            Storage<TComp>& store = getStorage<TComp>();

            if (!contains(handle))
            {
                checkLethal(false, "null entity cannot own components");
            }
            else if (component_sets[handle.id].has<TComp>())
            {
                // assert? return existing? #todo figure out
            }

            component_sets[handle.id].setFlags<TComp>(); 
            return store.Emplace(handle, std::forward<TArgs>(arguments)...);
        }

        template <typename Callable>
        bool registerCtor(const std::string& id, Callable&& builder, bool replace = false)
        {
            if (!replace && constructors.contains(id))
                return false;

            constructors.emplace(id, std::forward(builder));

            return true;
        } 
        EntityHandle createWithCtor(const std::string& id)
        {
            if (!constructors.contains(id))
                return EntityHandle{};

            EntityHandle newOne = createBlank();
            bool created = constructors[id](*this, newOne);
            if (!created)
            {
                destroy(newOne); 
                return EntityHandle{};
            }
            return newOne;
        }

        // extras:
    public:
        class EntityBuilder
        {
        public:
            template <typename TComp, class... TArgs>
            inline const EntityBuilder& add(TArgs&&... arguments)
            {
                owner_world->add(ent_id, std::forward<TArgs...>(arguments...));
                return *this;
            }
            template <typename TComp, class... TArgs>
            inline const auto& addAndGet(TArgs&&... arguments)
            {
                return owner_world->add(ent_id, std::forward<TArgs...>(arguments)...);
            }
            const EntityHandle ent_id;

        private:
            EntityBuilder(World* owner, EntityHandle h) : ent_id(h), owner_world(owner) {}
            EntityBuilder() = delete;
            World* owner_world;
            friend class World;
        };
        EntityBuilder entityBuilder(const char* name = nullptr)
        {
            return EntityBuilder(this, this->createBlank(name));
        }
    };
} // namespace vex
