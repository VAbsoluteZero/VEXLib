#pragma once
#include <cstdint>
#include <type_traits>

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;

using u8 = unsigned char;
using byte = unsigned char;

namespace std // needed to avoid <tuple> deps while using str bindings
{
	template <class _Tuple>
	struct tuple_size
	{
	};

	template <size_t _Index, class _Tuple>
	struct tuple_element
	{
	};
} // namespace std

namespace vex::memory
{
	template <typename... Types>
	constexpr auto MaxSizeOf()
	{
		constexpr size_t list[sizeof...(Types)] = {sizeof(Types)...};
		auto It = &list[0];
		auto End = &list[sizeof...(Types)];
		auto Curmax = *It;
		for (; It != End; ++It)
		{
			Curmax = (Curmax > *It) ? Curmax : *It;
		}
		return Curmax;
	};

	template <typename... Types>
	constexpr auto MaxAlignOf()
	{
		constexpr size_t list[sizeof...(Types)] = {alignof(Types)...};
		auto It = &list[0];
		auto End = &list[sizeof...(Types)];
		auto Curmax = *It;
		for (; It != End; ++It)
		{
			Curmax = (Curmax > *It) ? Curmax : *It;
		}
		return Curmax;
	};
} // namespace vex::memory


namespace vex::traits
{
	static constexpr size_t kTypeIndexNone = 0xff;
	template <typename TType>
	constexpr size_t GetTypeIndexInList() // #todo => rename
	{
		return kTypeIndexNone;
	}
	template <typename TType, typename TFirstType, typename... TRest>
	constexpr size_t GetTypeIndexInList()
	{
		if constexpr (!std::is_same<TType, TFirstType>::value)
			return 1 + GetTypeIndexInList<TType, TRest...>();
		return 0;
	}

	template <typename TType, typename... TRest>
	constexpr size_t GetIndex()
	{
		constexpr auto val = GetTypeIndexInList<TType, TRest...>();
		return val > kTypeIndexNone ? kTypeIndexNone : val;
	}

	template <typename TType>
	constexpr bool HasType()
	{
		return false;
	}
	template <>
	constexpr bool HasType<void>()
	{
		return false;
	}
	template <typename TType, typename TFirstType, typename... TRest>
	constexpr bool HasType()
	{
		if constexpr (!std::is_same<TType, TFirstType>::value)
			return HasType<TType, TRest...>();
		return true;
	}
	template <typename... TRest>
	constexpr bool AreAllTrivial()
	{
		return (... && std::is_trivial_v<TRest>);
	}

	template <typename T, typename... TRest>
	constexpr bool IsConvertible()
	{
		return (... && std::is_convertible_v<T, TRest>);
	}
	namespace private_impl
	{
		template <bool HasConversion, typename T, typename... TRest>
		struct SelectConvertibleTarget
		{
		};
		template <typename T, typename... TRest>
		struct SelectConvertibleTarget<false, T, TRest...>
		{
			using Type = void;
		};
		template <typename T, typename... TRest>
		struct SelectConvertibleTarget<true, T, TRest...>
		{
			using Type = void;
		};
	} // namespace private_impl

	template <typename T, typename... TRest>
	using ConvertionType = private_impl::SelectConvertibleTarget<IsConvertible<T, TRest...>, T, TRest...>;

	template <typename... TArgs>
	struct TTypeList
	{
	};

	template <typename T>
	struct FunctorTraits : public FunctorTraits<decltype(&T::operator())>
	{
	};

	template <typename ClassType, typename ReturnType, typename... Args>
	struct FunctorTraits<ReturnType (ClassType::*)(Args...) const>
	{
		typedef ReturnType TResult;

		static constexpr size_t Arity = sizeof...(Args);

		template <std::size_t I, typename T>
		struct ArgTypes;

		template <std::size_t I, typename Head, typename... Tail>
		struct ArgTypes<I, TTypeList<Head, Tail...>> : ArgTypes<I - 1, TTypeList<Tail...>>
		{
			typedef Head type;
		};

		template <typename Head, typename... Tail>
		struct ArgTypes<0, TTypeList<Head, Tail...>>
		{
			typedef Head type;
		};

		template <std::size_t I>
		using ArgTypesT = typename std::decay_t<typename ArgTypes<I, TTypeList<Args...>>::type>;
	};
} // namespace vex::traits

namespace vex
{
	namespace impl
	{
		struct vxSentinel
		{
		};
	} // namespace impl

	template <std::size_t index, typename Cur, typename... Args>
	struct GetTypeByIndex
	{
		using type = typename GetTypeByIndex<index - 1, Args...>::type;
	};
	template <typename Cur, typename... Args>
	struct GetTypeByIndex<0, Cur, Args...>
	{
		using type = Cur;
	};

	template <int Start, int End>
	struct CRange
	{
		static constexpr const impl::vxSentinel kSqEnd = impl::vxSentinel{};

		static constexpr int32_t _s = Start;

		static constexpr int32_t Signum(int32_t v) { return (0 < v) - (v < 0); }
		static constexpr int32_t _step = Signum(End - Start);

		struct SqIterator
		{
			friend auto operator==(SqIterator lhs, impl::vxSentinel rhs) { return lhs.IsDone(); }
			friend auto operator==(impl::vxSentinel lhs, SqIterator rhs) { return rhs == lhs; }
			friend auto operator!=(SqIterator lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
			friend auto operator!=(impl::vxSentinel lhs, SqIterator rhs) { return !(lhs == rhs); }
			bool IsDone() const { return Current == End; }
			inline int operator*() const { return Current; }
			inline auto& operator++()
			{
				Current += _step;
				return Current;
			}
			int32_t Current = _s;
		};

		SqIterator begin() noexcept { return SqIterator{}; };
		impl::vxSentinel end() const noexcept { return kSqEnd; };
	};

	template <int End>
	using ZeroTo = CRange<0, End>;

	struct Range
	{
		static constexpr const impl::vxSentinel kSqEnd = impl::vxSentinel{};
		friend auto operator==(Range lhs, impl::vxSentinel rhs) { return lhs.IsDone(); }
		friend auto operator==(impl::vxSentinel lhs, Range rhs) { return rhs == lhs; }
		friend auto operator!=(Range lhs, impl::vxSentinel rhs) { return !(lhs == rhs); }
		friend auto operator!=(impl::vxSentinel lhs, Range rhs) { return !(lhs == rhs); }

		bool IsDone() const { return Current >= End; }
		inline int operator*() const { return Current; }

		inline auto& operator++()
		{
			Current++;
			return *this;
		}

		Range() = delete;
		Range(int e) : End(e) {}
		Range(int s, int e) : Start(s), End(e) {}
		Range(int c, int s, int e) : Start(s), End(e), Current(c) {}
		Range(const Range&) = default;

		int Start = 0;
		int End = 0;
		int Current = 0;

		Range begin() noexcept { return Range{0, Start, End}; };
		impl::vxSentinel end() const noexcept { return kSqEnd; };
	};

	Range operator"" _times(unsigned long long x) { return Range((int)x); }
} // namespace vex
