#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Containers/Dict.h>
#include <VCore/Systems/ISystem.h>
#include <VCore/Utils/TypeInfoUtils.h>

#include <unordered_set>

namespace vex
{
	class SystemManager
	{
	public:
		template <class TConcreteSystem>
		inline TConcreteSystem* Register() noexcept
		{
			auto tid = tinfo::typeID<TConcreteSystem>();
			static_assert(std::is_base_of<ISystem, TConcreteSystem>::value,
				"cannot register custom type as a system, should be subclass of ISystem");
			if (!_uniqueSystems.contains(tid))
			{
				_uniqueSystems.emplace(tid,
					new TConcreteSystem() // owned by unique_ptr
				);
			}
			//_lanes[lane].insert(_uniqueSystems[id].get());
			return static_cast<TConcreteSystem*>(_uniqueSystems[tid].get());
		}

		// creates system if not present
		// returns non-owning raw pointer
		template <class TConcreteSystem>
		inline TConcreteSystem* Get() noexcept
		{
			auto tid = tinfo::typeID<TConcreteSystem>();
			return Register<TConcreteSystem>(); // ensure it exists
		}

		template <class TConcreteSystem>
		inline bool Contains() const noexcept
		{
			return _uniqueSystems.contains(tinfo::typeID<TConcreteSystem>());
		}

		template <class TConcreteSystem>
		inline void Remove() noexcept
		{
			auto tid = tinfo::typeID<TConcreteSystem>();
			if (!_uniqueSystems.contains(tid))
				return;

			const ISystem* sys = _uniqueSystems.tryGet(tid)->get();
			for (auto& it : _lanes)
			{
				it.Value.erase(sys);
			}
			_uniqueSystems.remove(tid);
		}

	private:
		// only one system of any type is allowed
		Dict<tTypeID, std::unique_ptr<ISystem>> _uniqueSystems;
		// references system owned by _uniqueSystems
		// system may be referenced several times in different lanes,
		// string identifies lane - logic_update, late_update, cleanup etc
		Dict<std::string, std::unordered_set<ISystem>> _lanes; // #TODO implement per lane update
	};
} // namespace vex
