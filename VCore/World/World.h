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
#include <VCore/World/IGlobal.h>
#include <VCore/World/Storage.h>
#include <VCore/Utils/CoreTemplates.h>

#include <functional>
#include <string>

namespace vex
{
    class World;
    using tfArchetypeBuilder = std::function<bool(World&, EntityHandle)>;
    // #todo add fluent-interface wrapper to allow world.Add(...).Add(...)Add..LogState();
    class World
    {
    public:
        template <size_t N>
        friend class StateRecorder;

        World(int expectedMaxEntCount = 64)
        {
            FreeEntitySlotsStack.reserve((expectedMaxEntCount / 2));
            Entities.reserve(expectedMaxEntCount);
            Entities.resize(1); //'Null-Object'
        }
        ~World() { Storages.clear(); }

        inline const Entity& Find(EntityHandle handle) const noexcept
        {
            if (handle.ID >= Entities.size())
                return Entities[0]; // null-entity instead of nullptr
            return Entities[handle.ID];
        }

        inline bool Contains(EntityHandle handle) const noexcept
        {
            bool inValidRange = (Entities.size() > handle.ID);
            return handle && inValidRange && Entities[handle.ID].handle; // #todo add UID check
        }

        template <typename Callable>
        bool RegisterArchetype(const std::string& id, Callable&& builder, bool replace = false)
        {
            if (!replace && Archetypes.contains(id))
                return false;

            Archetypes.emplace(id, std::forward(builder));

            return true;
        }

        EntityHandle InstantiateArchetype(const std::string& id);

        bool Destroy(EntityHandle handle);


        // name is just for debug, it is Not an ID
        EntityHandle CreateBlank();
        EntityHandle CreateBlank(const char* name);

        EntityHandle Clone(EntityHandle original);

        // default constructs if not present
        template <class TSingle>
        TSingle* const GetGlobal()
        {
            static_assert(std::is_base_of<IGlobal, TSingle>::value, "type must inherit from global");
            const auto type_id = tinfo::typeID<TSingle>();
            if (!Singles.contains(type_id))
            {
                auto unique = std::make_unique<TSingle>();
                IGlobal* inst = Singles.emplaceAndGet(type_id, std::move(unique)).get();

                return static_cast<TSingle*>(inst);
            }

            return static_cast<TSingle*>(Singles[type_id].get());
        }

        // * Component operations
        // Get first component of type;
        template <class TComp>
        inline TComp* const First()
        {
            return GetStorage<TComp>().First();
        }

        template <class TComp>
        inline bool RemoveComponent(EntityHandle handle)
        {
            // if Entity does not have component
            // remove would do nothing - so check Has<T> is not needed
            auto& ent = Entities[handle.ID];
            if (!ent)
                return false;

            ent.ComponentMask &= ~(TComp::Mask);
            return GetStorage<TComp>().remove(handle);
        }

        template <class TComp, class... TArgs>
        inline TComp& CreateComponent(EntityHandle handle, TArgs&&... arguments)
        {
            Storage<TComp>& store = GetStorage<TComp>();
            auto& ent = Entities[handle.ID];
            if (!ent)
            {
                checkAlways(false, "null entity cannot own components");
                return *store.Find(EntityHandle{});
            }
            else if (ent.Has<TComp>())
            {
                // assert? return existing? #todo figure out
            }

            ent.ComponentMask |= TComp::Mask;

            return store.Emplace(handle, std::forward<TArgs>(arguments)...);
        }

        template <class TComp>
        inline TComp* const Find(EntityHandle handle)
        {
            auto& ent = Entities[handle.ID];
            if (!ent || !ent.Has<TComp>())
                return nullptr;
            return GetStorage<TComp>().Find(handle);
        }
        template <class TComp>
        inline TComp& get(EntityHandle handle)
        {
            auto& ent = Entities[handle.ID];
            if (!ent || !ent.Has<TComp>())
                checkAlways_(false);

            return GetStorage<TComp>().get(handle);
        }

        template <class... TTypes>
        inline std::tuple<TTypes*...> FindTuple(EntityHandle handle)
        {
            auto& ent = Entities[handle.ID];

            if (!ent)
                return {};

            return std::make_tuple<TTypes*...>(GetStorage<TTypes>().Find(handle)...);
        }
        // ! component operations

        template <class TComp>
        Storage<TComp>& GetStorage() noexcept
        {
            auto type_id = tinfo::typeID<TComp>();
            const auto mask = TComp::Mask;
            (void)mask; // debug

            auto* existingHandle = Storages.tryGet(type_id);
            if (nullptr != existingHandle)
                return *(existingHandle->template As<Storage<TComp>>());

            // #todo add CompTraits to set default storage cap per type
            Storage<TComp>* storage = new Storage<TComp>();

            // creates UniqueHandler in-place which should free storage upon destruction
            // like std::unique_ptr<Storage<TComp>, decltype(deleter)> would
            Storages.emplace(type_id, UniqueHandle{type_id, storage}); // temporary is moved-from
            return *storage;
        }

        StorageBase* GetStorage(tTypeID tid)
        {
            auto* existingHandle = Storages.tryGet(tid);
            if (nullptr != existingHandle)
                return (existingHandle->get());
            return nullptr;
        }

        inline int k_size() const { return (int)(Entities.size() - FreeEntitySlotsStack.size()); }

        void Clear()
        {
            Entities.resize(1);
            FreeEntitySlotsStack.resize(0);
            Storages.clear();
            Archetypes.clear();
        }

    private:
        struct UniqueHandle
        {
            using tDataPtr = StorageBase*;
#ifndef NDEBUG
            static constexpr int kDebugGuard = 100200300;
            int DebugGuard = kDebugGuard;
#endif
            tTypeID const kTypeID;
            tMask const kMask;

            template <typename T>
            T* const As() const
            {
                return static_cast<T*>(_data); // no need for dynamic, types guaranteed to match
            }

            StorageBase* const get() const { return _data; }

            UniqueHandle(tTypeID id, StorageBase* data) noexcept : kTypeID(id), kMask(data->kMask), _data(data) {}

            UniqueHandle(UniqueHandle&& other) noexcept : kTypeID(other.kTypeID), kMask(other.kMask)
            {
                *this = std::move(other);
            }

            UniqueHandle(const UniqueHandle&) = delete;
            UniqueHandle& operator=(UniqueHandle&& other) noexcept;
            ~UniqueHandle();

        private:
            tDataPtr _data = nullptr;
        };

        // #todo allocate world as chunk

        Dict<tTypeID, UniqueHandle> Storages;             // <- components
        Dict<std::string, tfArchetypeBuilder> Archetypes; // <- archetype builders
        Dict<tTypeID, std::unique_ptr<IGlobal>> Singles;  // <- non-entity singletones
        std::vector<i32> FreeEntitySlotsStack;            // <- removed entity index will go here, so we pack 'em tight
        std::vector<Entity> Entities;

    public:
        class EntityBuilder
        {
        public:
            template <typename TComp, class... TArgs>
            inline const EntityBuilder& add(TArgs&&... arguments)
            {
                SelfWorld->CreateComponent(EntID, std::forward<TArgs>(arguments));
                return *this;
            }
            template <typename TComp, class... TArgs>
            inline const auto& AddAndGet(TArgs&&... arguments)
            {
                return SelfWorld->CreateComponent(EntID, std::forward<TArgs...>(arguments)...);
            }
            const EntityHandle EntID;

            operator EntityHandle() const { return EntID; }

        private:
            EntityBuilder(World* owner, EntityHandle h) : EntID(h), SelfWorld(owner) {}
            EntityBuilder() = delete;
            World* SelfWorld = nullptr;
            friend class World;
        };

        EntityBuilder CreateBlankBuilder(const char* name = nullptr) { return EntityBuilder(this, CreateBlank(name)); }

    public:
        // Enumerators
        struct EntityEnumerable
        {
            auto begin() noexcept { return _owner.Entities.begin(); }
            auto end() noexcept { return _owner.Entities.end(); }

            auto cbegin() const noexcept { return _owner.Entities.cbegin(); }
            auto cend() const noexcept { return _owner.Entities.cend(); }

            auto size() const noexcept { return _owner.Entities.size(); }

            ~EntityEnumerable() = default;
            EntityEnumerable& operator=(const EntityEnumerable& other) = delete;
            EntityEnumerable(EntityEnumerable&&) = delete;
            EntityEnumerable(const EntityEnumerable&) = delete;

        private:
            friend class World;
            EntityEnumerable(World& owner) : _owner(owner) {}

            World& _owner;
        };

        EntityEnumerable GetEntityEnumerator() { return *this; }

        template <typename... TTypes>
        struct Iterator
        {
            inline bool Advance()
            {
                i32 count = _owner.Entities.size();
                // -1 since there is ++_index in body immidiately after
                while (_index < (count - 1))
                {
                    _index++;
                    bool hasAll = _owner.Entities[_index].Has<TTypes...>();
                    if (hasAll)
                        return true;
                }

                _index = _owner.Entities.size() + 1;
                return false;
            }

            template <size_t... I>
            inline std::tuple<TTypes&...> Current(std::index_sequence<I...>)
            {
                auto& ent = _owner.Entities[_index];
                return std::tuple<TTypes&...>(*Storage<TTypes>::Find(std::get<I>(_storages), ent)...);
            }

            inline std::tuple<TTypes&...> operator*() { return Current(std::index_sequence_for<TTypes...>{}); }

            inline auto& operator++()
            {
                Advance();
                return *this;
            }

            inline bool IsDone() const { return (_owner.Entities.size()) <= _index; }

            EntityHandle CurrentHandle() const noexcept { return EntityHandle{_index}; }

            friend auto operator!=(Iterator lhs, DSentinel rhs) { return !(lhs == rhs); }
            friend auto operator!=(DSentinel lhs, Iterator rhs) { return !(lhs == rhs); }

            friend auto operator==(Iterator lhs, DSentinel rhs) { return lhs.IsDone(); }
            friend auto operator==(DSentinel lhs, Iterator rhs) { return rhs == lhs; }

            ~Iterator() = default;
            Iterator& operator=(const Iterator& other) = default;
            Iterator(Iterator&&) = default;
            Iterator(const Iterator&) = default;

        private:
            Iterator(World& owner) : _owner(owner), _storages(_owner.GetStorage<TTypes>()...)
            {
                Advance(); // find correct 1st
            }
            int _index = 0; // 0th component is null-object
            World& _owner;
            std::tuple<Storage<TTypes>&...> _storages;
            friend class World;
        };

        template <>
        struct Iterator<>
        {
        };

        template <typename... TTypes>
        struct CompEnumerable
        {
            auto begin() noexcept { return Iterator<TTypes...>(OwnerWorld); }
            constexpr auto end() noexcept { return kEndIteratorSentinel; }

            ~CompEnumerable() = default;
            CompEnumerable& operator=(const CompEnumerable& other) = delete;
            CompEnumerable(CompEnumerable&&) = delete;
            CompEnumerable(const CompEnumerable&) = delete;

        private:
            friend class World;
            CompEnumerable(World& owner) : OwnerWorld(owner) {}
            World& OwnerWorld;
        };

        template <typename... TTypes>
        CompEnumerable<TTypes...> GetCompEnum()
        {
            return *this;
        }

        template <>
        struct CompEnumerable<>
        {
        };
    };
} // namespace vex
