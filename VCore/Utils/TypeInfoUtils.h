#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <cstdint>
#include <cassert>

namespace core
{
	using tTypeID = uint32_t;
	// #todo to be replaced by decent lib https://github.com/Manu343726/ctti
	namespace __internal
	{
		struct tidCounter {
		private:
			static int gCount;
		public:
			static int CurrentCount() { return gCount; }
			static int Next() { return ++gCount; }
		};
		template <typename T>
		struct tidHolder
		{
			static const tTypeID GenTid;
		};
		template<typename T>
		const tTypeID tidHolder<T>::GenTid = tidCounter::Next();
	}

	namespace tinfo
	{
		template <typename T>
		static tTypeID typeID() noexcept
		{
			return __internal::tidHolder<T>::GenTid;
			//return &(__internal::IDHolder<T>::Identifier);
		}
		template <typename T>
		const uint64_t typeIDUInt() noexcept
		{
			return (uint64_t)typeID<T>();
		}
	} 
}