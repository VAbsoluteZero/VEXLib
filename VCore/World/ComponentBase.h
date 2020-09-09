#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <cassert>
#include <cstdint>
#include <string>

namespace vex
{
	using tMask = uint64_t;
	struct ComponentCounter
	{
	private:
		static int gCount;

	public:
		static constexpr int Capacity = sizeof(tMask);
		static int CurrentCount() { return gCount; }
		static int Next()
		{
			// ensure tMaskSize is big enough to hold all registered components
			// shame it could not be constexpr + static_assert duo in C++17
			assert(gCount < Capacity);
			return ++gCount;
		}
	};

	template <typename TComp>
	struct IComp
	{
		static const tMask Mask;
	};

	struct DebugName : IComp<DebugName>
	{
		const char* Name = nullptr;
	};

	// unfortunately not a compile time constant, #todo try macro+constexpr CNT_INC/CNT_READ pair
	template <typename TComp>
	const tMask IComp<TComp>::Mask = 1ull << ComponentCounter::Next(); // _ull -> uint64 literal
} // namespace vex
