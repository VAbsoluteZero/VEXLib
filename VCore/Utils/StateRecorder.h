#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Containers/Ring.h>
#include <VCore/World/World.h>

namespace core
{ 
	template<size_t N = 30>
	class StateRecorder
	{
	public:  
		int Size() { return  _snapshots.Size(); }

		void Write(core::World& world)
		{
			Snapshot shot;
			shot.Entities = world.Entities;
			shot.FreeStack = world._freeStack;
			for (auto& storage : world.Storages)
			{
				shot.Storages.Emplace(storage.Key,
					std::move(storage.Value.Get()->DuplicateSelf())
				);
			}
			_snapshots.Put(std::move(shot));
		}

		bool Read(core::World& world)
		{
			if (_snapshots.Empty())
				return false;

			Snapshot shot(_snapshots.Pop());

			std::vector<tTypeID> copied;
			copied.reserve(32);

			for (auto& kv : shot.Storages)
			{
				StorageBase* storage = world.GetStorage(kv.Key);
				if (storage)
				{
					storage->MoveFrom(kv.Value.get());
				}
				// add case for unrealted worlds
			}

			for (auto& kv : world.Storages)
			{
				if (!shot.Storages.Contains(kv.Key))
					kv.Value.Get()->Clear();
			}

			world.Entities = shot.Entities;
			world._freeStack = shot.FreeStack;

			return true;
		}
	private:
		struct Snapshot
		{
			Dict<tTypeID, std::unique_ptr<StorageBase>> Storages;
			std::vector<Entity> Entities;
			std::vector<int> FreeStack;

			Snapshot() = default;
			Snapshot(Snapshot&& other) noexcept
			{
				*this = std::move(other);
			}

			Snapshot& operator=(Snapshot&& other) noexcept
			{
				if (this != &other)
				{
					Storages = std::move(other.Storages);
					Entities = std::move(other.Entities);
					FreeStack = std::move(other.FreeStack);
				}

				return *this;
			}
		};
	public:
		Ring<N, Snapshot> _snapshots;
	};
}