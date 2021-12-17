#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */

#include <assert.h>

#include "VCore/Utils/CoreTemplates.h"

namespace vex
{
	/*
		Simple Static Ring Buffer, meant mainly for PODs.
		FillForward == true means that buffer will grow from 0 to Size, otherwise Size to 0.
		This means that FillForward is better for insert, !FillForward - for iteration
		(you can still iterate unordered even with FillForward).
	*/
	template <class T, i32 Size, bool FillForward = false>
	class StaticRing
	{
	public:
		static constexpr i32 GrowDirection = FillForward ? 1 : -1;
		static_assert(Size > 2, "Size must be >= 2, otherwise ring just doesnt make any sense.");

		inline auto WrapAroundIndexIfNeeded(i32 i) const -> i32
		{
			i = i % Size;
			if (i < 0)
			{
				i = Size - abs(i);
			}
			else if (i >= Size)
			{
				i = (i - Size);
			}
			return i;
		}
		inline auto TailIndex() -> i32 const
		{
			if (_first <= 0)
			{
				return _first;
			}
			i32 i = _first + GrowDirection * (_count - 1);
			return WrapAroundIndexIfNeeded(i);
		}

		inline auto Full() const -> bool { return Size == _count; }
		inline auto Empty() const -> bool { return 0 == _count; }
		inline auto Count() const -> i32 { return _count; }

		template <typename TFwd = T>
		inline T& Put(TFwd&& newItem)
		{
			_first += GrowDirection;
			_first = WrapAroundIndexIfNeeded(_first);
			T* curItem = item(_first);
			if (_count < Size)
			{
				_count++;
				new (curItem) T(std::forward<TFwd>(newItem));
			}
			else
			{
				*(curItem) = std::forward<TFwd>(newItem);
			}
			return *(curItem);
		}

		// #todo enable if for PODs only
		inline void AddUninitialized(i32 number)
		{
			_count += number;
			_count = _count >= Size ? Size : _count;
		}

		inline T&& Pop()
		{
			assert(_count > 0);

			i32 i = _first;
			--_count;

			if (_first++ >= Size)
			{
				_first = 0;
			}

			auto Tmp = std::move(*item(i));
			(item(i))->~T();
			return Tmp;
		}

		inline void PopDiscard()
		{
			if (_count > 0)
			{
				i32 i = _first;
				--_count;
				++_first;
				if (_first >= Size)
				{
					_first = 0;
				}
				(item(i))->~T();
			}
		}

		inline T& Peek()
		{
			assert(_count > 0);
			return *(item(_first));
		}

		StaticRing(){};

		// todo
		StaticRing(const StaticRing& other) = delete;
		StaticRing(StaticRing&& other) = delete;
		~StaticRing()
		{
			static_assert(Size > 1, "invalid ring size");
			if constexpr (!std::is_trivially_destructible_v<T>)
			{
				while (!Empty())
				{
					PopDiscard();
				}
			}
		}

		T& AtAbsIndexUnchecked(i32 i)
		{
			assert(i >= 0 && i < Size);
			return dataTyped[i];
		}

		T& At(i32 i)
		{
			i32 rel = ToRawIndex(i);
			assert(rel >= 0 && rel < Size);
			return dataTyped[rel];
		}
		const T& At(i32 i) const
		{
			i32 rel = ToRawIndex(i);
			assert(rel >= 0 && rel < Size);
			return dataTyped[rel];
		}

		// do not use this type directly, it is meant for ranged for exclusively
		template <bool Const>
		struct SqIterator
		{
			using RingType = typename AddConst<StaticRing<T, Size, FillForward>, Const>::type;
			// as it is used in It == End by range-for loop, it is esentialy stop condition
			friend auto operator==(SqIterator lhs, impl::vxSentinel rhs) { return lhs.IsDone(); }
			friend auto operator==(impl::vxSentinel lhs, SqIterator rhs) { return rhs == lhs; }
			friend auto operator!=(SqIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
			friend auto operator!=(impl::vxSentinel lhs, SqIterator rhs) { return !(lhs == rhs); }
			bool IsDone() const { return HeadOffset >= Ring.Count(); }

			inline decltype(auto) operator*() const { return Ring.At(HeadOffset); }
			inline decltype(auto) operator++()
			{
				HeadOffset += 1;
				return *this;
			}

			i32 HeadOffset = 0;
			RingType& Ring;

			SqIterator(const SqIterator&) = default;
			SqIterator(RingType& ring) : Ring(ring) {}
		};
		auto begin() noexcept { return SqIterator<false>{*this}; };
		auto end() const noexcept { return kSqEnd; };
		auto begin() const noexcept { return SqIterator<true>{*this}; };
		// do not use this type directly, it is meant for ranged for exclusively
		template <bool Const>
		struct RevIterator
		{
			using RingType = typename AddConst<StaticRing<T, Size>, Const>::type;
			// as it is used in It == End by range-for loop, it is esentialy stop condition
			friend auto operator==(RevIterator lhs, impl::vxSentinel rhs) { return lhs.IsDone(); }
			friend auto operator==(impl::vxSentinel lhs, RevIterator rhs) { return rhs == lhs; }
			friend auto operator!=(RevIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
			friend auto operator!=(impl::vxSentinel lhs, RevIterator rhs) { return !(lhs == rhs); }
			bool IsDone() const { return TailOffset >= Ring.Count(); }

			inline decltype(auto) operator*() const { return Ring.At(Ring.Count() - TailOffset - 1); }
			inline decltype(auto) operator++()
			{
				TailOffset += 1;
				return *this;
			}

			i32 TailOffset = 0;
			RingType& Ring;
			RevIterator(const RevIterator&) = default;
			RevIterator(RingType& ring) : Ring(ring) {}
		};
		auto rbegin() noexcept { return RevIterator<false>{*this}; };
		auto rend() const noexcept { return kSqEnd; };
		auto rbegin() const noexcept { return RevIterator<true>{*this}; };

	private:
		static constexpr auto kAlign = alignof(T) < 8 ? 8 : alignof(T);
		union
		{
			alignas(kAlign) byte dataBytes[Size * sizeof(T)];
			alignas(kAlign) T dataTyped[Size];
		};

		inline auto ToRawIndex(i32 i) const -> i32
		{ //
			i32 rel = _first + i * GrowDirection;
			return WrapAroundIndexIfNeeded(rel);
		}


		inline T* item(i32 i) { return &dataTyped[i]; }

		i32 _count = 0;
		i32 _first = -1; // uninitialized
	};

	using Inst = StaticRing<i32, 4, true>;

	static inline Inst TestInst = {};
} // namespace vex
